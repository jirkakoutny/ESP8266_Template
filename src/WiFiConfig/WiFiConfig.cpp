/*
 * V4.7 - 30.8.2016 - Metody .begin se nyni vraci s wificonfigresult_t (podarilo se pripojit k AP, nepodarilo se pripojit a vyprsel timeout pro konfiguracni AP, nepodarilo se pripojit, ale je zakazane spousteni konfiguracniho AP).
 *					  ESP NYNI NENI RESETOVAN PO VYPRSENI TIMEOUTU!!!! Je to kvuli bateriovym cidlum a DeepSleep rezimu v situaci, kdy zmizi AP, ke kteremu se bezne pripojujeme
 *					  Odstraneno odesilani reakce na generate_204 (viz. https://github.com/tzapu/WiFiManager/issues/114)
 *					  Zbytek MAC adresy v pripadnem SSID je nyni vzdy velkymi pismeny
 *					  Kvalita signalu jednotlivych AP je nyni uvadena v % namisto nic nerikajicich -dBm
 *					  Pokud do polozky timeout v .begin() metode zadame hodnotu WC_DONT_RUN_CONFIGAP, tak i po neuspesnem pokusu o pripojeni do site NENI spusteny konfiguracni AP (bateriove pristroje, stale bezici pristroje)
 *
 * V4.6 - 26.7.2016 - Moznost nastavit cislo kanalu pro rezim AP, nove eye-candy rozhrani.
 *
 * V4.5 - 20.7.2016 - Moznost konfigurovat cislo kanalu pro nastavovaci AP (default je 3), po nacteni konfiguracni stranky je nastaveny timeout na 6 minut, pokud je ve jmenu SSID pro konfiguracni
 *					  AP znak ? (otaznik), tak je tento nahrazen HEXASCII reprezentaci 3 poslednich bytu MAC adresy AP (POZOR!!! je jina, nez STA!!!)
 *
 * V4.4 - 10.7.2016 - Zmena nazvu vsech privatnich metod (doplnene uvodni podtrzitko _). Pokud tam nebylo, tak napriklad nesel prelozit skript s handleNotFound pokud nebyl dopredne deklarovany (asi zmatek v Arduino preprocesoru)
 *
 * V4.3 - 23.6.2016 - Kazde nacteni webove stranky prodluzuje pripadny timeout pro restart zarizeni.
 *
 * V4.2 - 10.6.2016 - DNSServer a WebServer ukazatele jsou nyni staticke primo v modulu WiFiConfig. Predchozi verze neumoznila soucasne pouziti s knihovnami ESPAsyncWebServer z duvodu kolize nekterych definic HTTP metod
 *
 * V4.1 - 18.5.2016 - Nenechame SDK ukladat WiFi konfiguraci do Flash (setrime prepisy)
 *
 * V4.0 - 10.4.2016 - pridana moznost staticke konfigurace ip parametru. POZOR!!!! ukladani dat do EEPROM (verejne funkce) nyni nevolaji automaticky commit() kvuli snizeni poctu prepisu Flash EEPROM
 * 					  pridana moznost timeoutu pro konfiguracni AP. Pokud timeout vyprsi, tak se zarizeni automaticky restartuje, coz vyvola novy pokus o pripojeni.
 *
 * V3.0 - 27.2.2016 - pridana presmerovavaci stranka kvuli Captive portalu, doplneno servirovani favicon.ico, doplnena moznost zadavani uzivatelskych parametru, www server a DNS server jsou dynamicky vytvareny
 *
 * V2.4 - 7.2.2016  - upraveno casovani pri pripojovani k AP (zmizel delay(100))
 *
 * V2.3 - 5.2.2016  - pridano zverejneni jmena hosta a nastaveni tohoto jmena pro DHCP apod.
 *
 * V2.2 - 19.1.2016 - pridan Captive portal pri konfiguracnim AP.
 *
 * V2.1 - 20.9.2015 - pridano zadavani jmena zarizeni. Modul ho sice nevyuziva, ale aplikace ano a v pripade DHCP se dost hodi.
 *
 * V2.0 - 30.8.2015 - Podstatnym zpusobem vylepsena signalizace vnitrniho stavu, presunuto ulozeni rezimu (commit je uz zbytecny - udela se pri ukladani retezcu - uspora kodu/setreni prepisu pameti Flash)
 *
 * V1.2 - 23.8.2015 - BugFix - pridan chybejici eeprom.commit() po ulozeni rezimu prace
 *
 * V1.1 - 5.8.2015 - pridana moznost volani callback metody pri behu konfiguracniho AP (signalizace stavu uzivatelskym zpusobem)
 *
 * V1.0 - publikace na www.xpablo.cz
 *
 * TODO:
 * Prejit na pripojovani pomoci vnitrnich mechanizmu SDK ESP - zvysi se tim rychlost pripojeni kvuli bateriovym zarizenim
 */

#include "WiFiConfig.h"
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include "embHTML.h"

#define DEBUG_OUT(a) {}
//#define DEBUG_OUT(a) { Serial.print(a); Serial.flush(); }

extern "C" {
  #include "user_interface.h"
}

#define DNS_PORT 53

char WiFiDeviceName[elementSize(wificonfigarea_t, devname)]; // misto pro jmeno zarizeni (dodane do DNS, DHCP NBNS apod...)

static uint8_t testWifi(wificonfig_cb cb);

enum
{
	WIFIMODE_AP = 0x55, // rezim prace jako pristupovy bod (AP)
	WIFIMODE_STA = 0xAA // rezim prace jako klient
};

enum
{
	IPCONFIG_DHCP = 0x55, // DHCP konfigurace ip adres (default)
	IPCONFIG_STATIC = 0xaa // staticka konfigurace ip adres
};

static int configBase; // musi byt trvale ulozene, aby fungovaly metody pro ziskani retezcu z EEPROM

static std::unique_ptr<DNSServer>        dnsServer;
static std::unique_ptr<ESP8266WebServer> server;

IPAddress getOurIP(void)
{
	IPAddress ipa;
	WiFiMode_t wm = WiFi.getMode();

	switch (wm)
	{
	case WIFI_STA:
	  ipa = WiFi.localIP();
	break;

	case WIFI_AP:
	  ipa = WiFi.softAPIP();
	break;

	default:
	  ipa = IPAddress(0,0,0,0); // nelze urcit ip adresu (bud je AP + STA aktivni, nebo je vypnute WiFi)
	break;
	}
	return ipa;
}

uint8_t * getOurMAC(uint8_t *mac)
{

	if (WIFIMODE_STA == EEPROM.read(configBase + offsetof(wificonfigarea_t, mode)))
		return WiFi.macAddress(mac);
	else
		return WiFi.softAPmacAddress(mac);
}

uint32_t getEEPROMuint32(int start)
{
	uint32_t result = 0;

	for (int i=0; i<4; ++i)
	{
		result <<= 8;
		result += EEPROM.read(start);
		++start;
	}
	return result;
}

void setEEPROMuint32(int start, uint32_t val)
{

	for (int i=0; i<4; ++i)
	{
		EEPROM.write(start + 3 - i, (uint8_t)val);
		val >>= 8;
	}
}

String getEEPROMString(int start, int len)
{
  String string = "";

  for (int i = start; i < + start + len; ++i)
  {
    uint8_t b = EEPROM.read(i);

    if ((0xff == b) || (0 == b))
      break;
    string += char(b);
  }
  return string;
}

void setEEPROMString(int start, int len, String string)
{
  int si = 0;

  for (int i = start; i < start + len; ++i)
  {
    char c;

    if (si < string.length())
    {
      c = string[si];
    }
    else
    {
      c = 0;
    }
    EEPROM.write(i, c);
    ++si;
  }
}

WiFiConfigUsrParameter::WiFiConfigUsrParameter(const char *id, const char *label, const char *defaultValue, int length, storeparam_cb cb)
{
  _next = NULL;
  _cb = cb;
  _id = id;
  _label = label;
  _length = length;
  _value = new char[length + 1];
  for (int i = 0; i < length; i++)
  {
    _value[i] = 0;
  }
  if (defaultValue != NULL)
  {
    strncpy(_value, defaultValue, length);
  }
}

const char* WiFiConfigUsrParameter::getValue()
{

  return _value;
}

const char* WiFiConfigUsrParameter::getID()
{

  return _id;
}

const char* WiFiConfigUsrParameter::getLabel()
{

  return _label;
}

int WiFiConfigUsrParameter::getValueLength()
{

  return _length;
}

void WiFiConfigUsrParameter::setNext(WiFiConfigUsrParameter *n)
{

  _next = n;
}

WiFiConfigUsrParameter *WiFiConfigUsrParameter::getNext()
{

  return _next;
}

void WiFiConfigUsrParameter::setNewValue(const char *newval)
{

	if (0 != strcmp(_value, newval))
		_cb(newval);
}

WiFiConfigUsrParameter *WiFiConfig::_searchUsrParameter(const char *name)
{
  WiFiConfigUsrParameter *ptr = _params;

  while (NULL != ptr)
  {
  	if (0 == strcmp(name, ptr->getID()))
  		break;
  	ptr = ptr->getNext();
  }
  return ptr;
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
bool WiFiConfig::captivePortal()
{

//  if (!isIp(server->hostHeader()) )
  {
    server->sendHeader("Location", String("http://") + server->client().localIP().toString(), true);
    server->send (302, F("text/plain"), F("")); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server->client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void WiFiConfig::addParameter(WiFiConfigUsrParameter *p)
{
	p->setNext(_params);
	_params = p;
}

void WiFiConfig::_handleNotFound(void)
{

//	_time = millis() + (_timeout * 1000); // spocitame si novy cas, kdy budeme modul restartovat
	DEBUG_OUT("Requested URI: ");
	DEBUG_OUT(server->uri());
	DEBUG_OUT("\r\n");
/*
	if (server->uri().endsWith(String("favicon.ico")))
	{
   		server->send(404, F("text/plain"), F(""));
    	server->client().stop(); // Stop is needed because we sent no content length
	}
	else
*/
	{
    	server->sendHeader("Location", String("http://") + server->client().localIP().toString(), true);
    	server->send (302, F("text/plain"), F("")); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    	server->client().stop(); // Stop is needed because we sent no content length
//		String content = FPSTR(PAGE_CAPTIVEPORTALCATCH);
//		content.replace("{v}", String("http://") + server->client().localIP().toString() + String("/config"));
//    	server->send (200, TEXTHTML, content);
	}
}

void WiFiConfig::_handleRoot(void)
{
	String content = FPSTR(PAGE_CAPTIVEPORTALCATCH);
	content.replace("{v}", server->client().localIP().toString());
   	server->send (200, TEXTHTML, content);
	_time = millis() + (_timeout * 1000); // spocitame si novy cas, kdy budeme modul restartovat
}

void WiFiConfig::_handleDisplayAP(void)
{
	String s;
	String v;
	String content;

//	_time = millis() + (_timeout * 1000); // spocitame si novy cas, kdy budeme modul restartovat
	_time = millis() + (360 * 1000); // spocitame si novy cas, kdy budeme modul restartovat (6 minut)
	content = FPSTR(PAGE_INDEX1);
	int n = WiFi.scanNetworks();
	if (0 == n)
	{
		content += FPSTR(PAGE_NO_SSID);
	}
	else
	{
		for (int i = 0; i < n; ++i)
		{
			int quality;

  			if (WiFi.RSSI(i) <= -100)
  			{
    			quality = 0;
  			}
  			else if (WiFi.RSSI(i) >= -50)
  			{
    			quality = 100;
  			}
  			else
  			{
    			quality = 2 * (WiFi.RSSI(i) + 100);
  			}
			s = FPSTR(SSID_ITEM);
			s.replace("{v}", WiFi.SSID(i));
			s.replace("{a}", String(quality));
			s.replace("{s}", (ENC_TYPE_NONE == WiFi.encryptionType(i)) ? "" : "<img id=\"lock\">");
			content += s;
		}
	}
	s = FPSTR(PAGE_INDEX2);
	v = getEEPROMString(configBase + offsetof(wificonfigarea_t, ssid), elementSize(wificonfigarea_t, ssid));
	if (v.length())
		s.replace("{s}", "value='" + v + "'");
	else
		s.replace("{s}", "placeholder='SSID'");
	v = getEEPROMString(configBase + offsetof(wificonfigarea_t, pass), elementSize(wificonfigarea_t, pass));
	if (v.length())
		s.replace("{p}", "value='" + v + "'");
	else
		s.replace("{p}", "placeholder='password'");
	v = getEEPROMString(configBase + offsetof(wificonfigarea_t, devname), elementSize(wificonfigarea_t, devname));
	if (v.length())
		s.replace("{n}", "value='" + v + "'");
	else
		s.replace("{n}", "placeholder='name'");
	if (EEPROM.read(configBase + offsetof(wificonfigarea_t, mode)) == WIFIMODE_AP)
		s.replace("{a}", "checked='checked'");
	else
		s.replace("{a}", "");
	if (EEPROM.read(configBase + offsetof(wificonfigarea_t, ip)) == IPCONFIG_STATIC)
		s.replace("{c}", "checked='checked'");
	else
		s.replace("{c}", "");
	s.replace("{i}", IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, ipaddr))).toString());
	s.replace("{m}", IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, netmask))).toString());
	s.replace("{g}", IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, gateway))).toString());
	s.replace("{d}", IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, dns))).toString());

	uint8_t chan = EEPROM.read(configBase + offsetof(wificonfigarea_t, apchannel));
	if ((chan < 1) || (chan > 13))
		chan = 1; // neplatne cislo kanalu nahradime nejnizsim
	s.replace("{ch}", String(chan));

	content += s;

// Uzivatelske parametry
	if (_params)
	{
		content += FPSTR(PAGE_PARAM_HDR);
	}

	WiFiConfigUsrParameter *up = _params;

	char parLength[4];

	while (NULL != up)
	{
		s = FPSTR(PAGE_PARAM);
		s.replace("{t}", up->getLabel());
		s.replace("{n}", up->getID());
		snprintf(parLength, sizeof(parLength), "%d", up->getValueLength());
		s.replace("{l}", parLength);
		s.replace("{v}", up->getValue());
		content += s;
		up = up->getNext();
	}

	content += FPSTR(PAGE_END);
	server->send(200, TEXTHTML, content);
}

void WiFiConfig::_handleSetAP(void)
{
	uint8_t mode;
	String str;

	str.reserve(128);
	str = server->arg("_s");
	if (str.length() > 0)
	{
		for (int i = 0; i < str.length(); i++)
		{
			// Deal with (potentially) plus-encoded ssid
			str[i] = (str[i] == '+' ? ' ' : str[i]);
		}
		setEEPROMString(configBase + offsetof(wificonfigarea_t, ssid), elementSize(wificonfigarea_t, ssid), str);

		str = server->arg("_p");
		for (int i = 0; i < str.length(); i++)
		{
			// Deal with (potentially) plus-encoded password
			str[i] = (str[i] == '+' ? ' ' : str[i]);
		}
		setEEPROMString(configBase + offsetof(wificonfigarea_t, pass), elementSize(wificonfigarea_t, pass), str);

		str = server->arg("_n");
		setEEPROMString(configBase + offsetof(wificonfigarea_t, devname), elementSize(wificonfigarea_t, devname), str);

		str = server->arg("_a");
		if (str.length() > 0)
		{
			mode = WIFIMODE_AP; // rezim AP
			str = server->arg("_ch"); // kanal AP
			EEPROM.write(configBase + offsetof(wificonfigarea_t, apchannel), (uint8_t)str.toInt());
		}
		else
			mode = WIFIMODE_STA; // rezim STA
		EEPROM.write(configBase + offsetof(wificonfigarea_t, mode), mode);

		str = server->arg("_st");
		if (0 == str.length())
			EEPROM.write(configBase + offsetof(wificonfigarea_t, ip), IPCONFIG_DHCP); // mame DHCP dynamickou konfiguraci
		else
		{ // staticka ip konfigurace
			IPAddress ipa;

			EEPROM.write(configBase + offsetof(wificonfigarea_t, ip), IPCONFIG_STATIC);
			str = server->arg("_i");
			ipa.fromString(str);
			setEEPROMuint32(configBase + offsetof(wificonfigarea_t, ipaddr), (uint32_t) ipa);

			str = server->arg("_m");
			ipa.fromString(str);
			setEEPROMuint32(configBase + offsetof(wificonfigarea_t, netmask), (uint32_t) ipa);

			str = server->arg("_g");
			ipa.fromString(str);
			setEEPROMuint32(configBase + offsetof(wificonfigarea_t, gateway), (uint32_t) ipa);

			str = server->arg("_d");
			ipa.fromString(str);
			setEEPROMuint32(configBase + offsetof(wificonfigarea_t, dns), (uint32_t) ipa);
		}

// Uzivatelske parametry
		for (int i = 0; i < server->args(); i++)
		{
			if (!server->argName(i).startsWith("_")) // vnitrni parametry WiFiConfig modulu zacinaji _, takze ty muzeme ignorovat
			{
				WiFiConfigUsrParameter *up = _searchUsrParameter(server->argName(i).c_str());
				if (NULL != up)
					up->setNewValue(server->arg(i).c_str());
			}
		}
		EEPROM.commit(); // skutecne ulozime data
	}
	server->send(200, F("text/html"), FPSTR(PAGE_SAVED));
	delay(10000); // cekame 10 sekund na odeslani dat
	ESP.restart();
}

// Start WiFi v rezimu AP pro nastaveni modulu
wificonfigresult_t WiFiConfig::_setupAP(wificonfig_cb cb)
{
	String ssid = SETUP_SSID;

	if (WC_DONT_RUN_CONFIGAP == _timeout)
		return WCR_CONFIGAP_NOT_STARTED; // nemame spoustet konfiguracni AP - vracime se hned
	dnsServer.reset(new DNSServer());
	server.reset(new ESP8266WebServer(80));

	/* Soft AP network parameters */
	IPAddress apIP(192, 168, 4, 1);
	IPAddress netMsk(255, 255, 255, 0);

	WiFi.mode(WIFI_AP);

	WiFi.softAPConfig(apIP, apIP, netMsk);
	if (ssid.endsWith("?"))
	{
  		uint8_t amac[6];
  		char lmac[16];

  		WiFi.softAPmacAddress(amac);
  		sprintf(lmac, "%02X%02X%02X", amac[3], amac[4], amac[5]);
  		ssid.replace("?", String(lmac));
	}
	WiFi.softAP(ssid.c_str(), NULL, SETUP_CHANNEL);
	delay(500); // dulezite - jinak se nevraci spravna IP adresa !!! (udajne od verze 2.3.0 uz neni nutne)

	dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer->start(DNS_PORT, "*", WiFi.softAPIP()); // spustime tzv. Captive portal - vsechny DNS dotazy jsou smerovany na nasi ip adresu
	if (cb)
		cb(WCS_CONFIGSTART); // signalizujeme start konfiguracniho serveru
// Nastavime handlery weboveho serveru pro konfiguraci
	server->onNotFound(std::bind(&WiFiConfig::_handleNotFound, this));
	server->on("/config", std::bind(&WiFiConfig::_handleDisplayAP, this));
    server->on("/s", std::bind(&WiFiConfig::_handleSetAP, this));
    server->on("/", std::bind(&WiFiConfig::_handleRoot, this));
	server->begin(); // startujeme webovy server
	while (1)
	{
		server->handleClient(); // osetrujeme praci serveru
		if (cb)
			cb(WCS_CONFIGWAIT); // volame uzivatelsky callback (napr. signalizace)
		dnsServer->processNextRequest();
		yield(); // procesy uvnitr systemu ESP potrebuji take svuj cas
		if (_timeout)
		{
			if (millis() > _time)
			{
				DEBUG_OUT(F("AP timeout"));
				if (cb)
					cb(WCS_CONFIGTIMEOUT); // signalizujeme timeout
				break; // ukoncime cekani a vracime se
			}
		}
	}
	return WCR_TIMEOUT; // nepripojeno, vyprsel timeout konfiguracniho AP
}

// Testovani, zda se modul pripojil k AP
static uint8_t testWifi(wificonfig_cb cb)
{
	uint32_t startt = millis();

  	DEBUG_OUT(F("Trying to connect.\r\n"));
	while ((millis() - startt) < WIFI_STA_CONNECT_TIMEOUT)
	{
		if (WiFi.status() == WL_CONNECTED)
		{
  			DEBUG_OUT(F("Connected...\r\n"));
  			if (cb)
  				cb(WCS_CONNECTED);
			return 1; // jsme pripojeni
		}
		delay(0);
		if (cb)
			cb(WCS_CONNECTING); // signalizujeme pokracujici pokus o spojeni
	}
  	DEBUG_OUT(F("Not connected!\r\n"));
	return 0; // pripojeni se nezdarilo
}

wificonfigresult_t WiFiConfig::begin(int configarea, uint8_t forceConfigure, wificonfig_cb cb)
{
	wificonfigresult_t result = WCR_OK; // predpokladame, ze se pripojeni podari

	DEBUG_OUT(F("\r\n\r\n")); // oddeleni vypisu

	WiFi.persistent(false); //neukladame konfiguraci WiFi v SDK do Flash
	configBase = configarea; // pocatek konfigurace v EEPROM
	if (0 == forceConfigure)
	{
		DEBUG_OUT(F("Force config.\r\n"));
		result = _setupAP(cb);
	}
	else
	{
		String s = getEEPROMString(configBase + offsetof(wificonfigarea_t, devname), elementSize(wificonfigarea_t, devname));
		strcpy(WiFiDeviceName, s.c_str());
		s = getEEPROMString(configBase + offsetof(wificonfigarea_t, ssid), elementSize(wificonfigarea_t, ssid));
		String pass =  getEEPROMString(configBase + offsetof(wificonfigarea_t, pass), elementSize(wificonfigarea_t, pass));

		DEBUG_OUT(F("SSID set to: "));
		DEBUG_OUT(s);
		DEBUG_OUT(F("\r\n"));
		DEBUG_OUT(F("Pass set to: "));
		DEBUG_OUT(pass);
		DEBUG_OUT(F("\r\n"));
		DEBUG_OUT(F("Name set to: "));
		DEBUG_OUT(WiFiDeviceName);
		DEBUG_OUT(F("\r\n"));

		switch (EEPROM.read(configBase + offsetof(wificonfigarea_t, mode)))
		{
		case WIFIMODE_STA:
		{
			DEBUG_OUT(F("STA mode.\r\n"));
			WiFi.mode(WIFI_STA); // startujeme WiFi v rezimu klienta
			if (strlen(WiFiDeviceName))
				WiFi.hostname(WiFiDeviceName); // nastavime jmeno zarizeni
			WiFi.begin(s.c_str(), pass.c_str());
			if (IPCONFIG_STATIC == EEPROM.read(configBase + offsetof(wificonfigarea_t, ip)))
			{
				DEBUG_OUT(F("Static configuration.\r\n"));
				WiFi.config(IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, ipaddr))), IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, gateway))),
					IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, netmask))), IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, dns))));
			}
			wifi_station_set_auto_connect(true);
			if (cb)
				cb(WCS_CONNECTSTART); // signalizujeme zacatek pokusu o pripojeni
			if (false == testWifi(cb))
				result = _setupAP(cb); // modul se nepripojil - startujeme AP rezim
		}
		break;

		case WIFIMODE_AP:
			DEBUG_OUT(F("AP mode.\r\n"));
			WiFi.mode(WIFI_AP); // startujeme AP
			if (pass.length())
			// je zadane heslo do AP
				WiFi.softAP(s.c_str(), pass.c_str(), EEPROM.read(configBase + offsetof(wificonfigarea_t, apchannel)));
			else
			// otevreny AP
				WiFi.softAP(s.c_str(), NULL, EEPROM.read(configBase + offsetof(wificonfigarea_t, apchannel)));
			if (IPCONFIG_STATIC == EEPROM.read(configBase + offsetof(wificonfigarea_t, ip)))
				WiFi.softAPConfig(IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, ipaddr))), IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, gateway))),
					IPAddress(getEEPROMuint32(configBase + offsetof(wificonfigarea_t, netmask))));
			if (cb)
				cb(WCS_CONNECTSTART); // signalizujeme zacatek pokusu o pripojeni (zde se nic jineho stejne nestane...)
		break;

		default: // jakykoliv neznamy rezim (mozna zavada na EEPROM???)
			DEBUG_OUT(F("Mode Error!!\r\n"));
			result = _setupAP(cb);
		break;
		}
	}
	return result; // mame vyreseno
}

wificonfigresult_t WiFiConfig::begin(int configarea, uint8_t forceConfigure, int timeout, wificonfig_cb cb = NULL)
{
	if (forceConfigure)
	{ // pouze pokud nemame vynucenou konfiguraci merime cas - to nam umozni dostat se ze spatne zadaneho timeoutu, ktery se neda zvladnout
		if ((timeout > 0) && (40 > timeout))
			timeout = 40; // timeout musi byt minimalne 40 sekund
		_timeout = timeout;
		if (_timeout > 0)
			_time = millis() + (_timeout * 1000); // spocitame si novy cas, kdy budeme modul restartovat
	}
	else
		_timeout = 0; // pri vynucene konfiguraci se parametr timeout neuplatni
	return begin(configarea, forceConfigure, cb); // spustime WiFi
}
// EOF
