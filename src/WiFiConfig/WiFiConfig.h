/**
 * @file WiFiConfig.h
 * @author Pavel Brychta, http://www.xpablo.cz
 *
 * Copyright (c) 2015,16 Pavel Brychta. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* Nastaveni ESP modulu, ktere pracuje takto:
 * 1. Pokud je forceConfigure ==0, tak se aktivuje WiFi v rezimu AP a cele ESP je mozne nastavit webovym rozhranim (pocitam s prenosem hodnoty nouzoveho tlacitka pri startu).
 * 2. Neni-li tlacitko stisknute, tak se vezme rezim prace a AP se nastavi dle nej (WIFI_STA a WIFI_AP)
 * 3. Pokud byl rezim prace WIFI_STA a ESP se nepripoji k zadne siti do casu WIFI_STA_CONNECT_TIMEOUT, tak se pokracuje jako kdyby bylo stisknute rekonfiguracni tlacitko
 */

#ifndef __WiFiConfig_h__
#define __WiFiConfig_h__

#include <ESP8266WiFi.h>
#include <memory>

#define elementSize(type, element) sizeof(((type *)0)->element)

#define WC_DONT_RUN_CONFIGAP -1 // priznak, ze si neprejeme spoustet konfiguracni AP (uziva se misto parametru timeout). Urceno pro bateriove napajene pristroje

// Struktura konfigurace, ulozena v EEPROM
typedef struct
{
	uint8_t mode; // rezim prace AP/STA
	uint8_t ip; // konfigurace ip (staticka/DHCP)
	char ssid[32]; // SSID site
	char pass[64]; // heslo
	char devname[32]; // jmeno zarizeni (pro NBNS plati jen 16 znaku)
	uint32_t ipaddr; // ip adresa v pripade staticke konfigurace
	uint32_t netmask; // sitova maska v pripade staticke konfigurace
	uint32_t gateway; // sitova brana v pripade staticke konfigurace
	uint32_t dns; // ip adresa DNS serveru v pripade staticke konfigurace
	uint8_t apchannel; // kanal, na kterem pracuje AP (pokud je zapnuty rezim AP)
} wificonfigarea_t;

// Parametr, predany uzivatelske callback funkci, urceny pro aplikacni vizualizaci stavu konfigurace a pripojeni
typedef enum
{
	WCS_CONNECTSTART = 0, // zacatek pokusu o pripojeni k ulozene konfiguraci
	WCS_CONNECTING = 1, // probiha pokus o pripojeni
	WCS_CONNECTED = 2, // pripojeni bylo uspesne
	WCS_CONFIGSTART = 3, // zacatek startu konfiguracniho AP
	WCS_CONFIGWAIT = 4, // cekame na nastaveni konfigurace pres web
	WCS_CONFIGTIMEOUT = 5, // doslo k vyprseni timeoutu konfigurace, budeme se vracet s False jako vysledek z .begin(...)
}wificonfigstate_t;

// Navratovy parametr z volani begin() - udava, jak se podarilo WiFiConfig modulu pripojit k AP
typedef enum
{
	WCR_OK = 0, // wifi pripojena/AP nastartovane (dle parametru v EEPROM)
	WCR_TIMEOUT = 1, // wifi neni pripojena a vyprsel zadany timeout
	WCR_CONFIGAP_NOT_STARTED = 2, // wifi neni pripojena a spusteni konfiguracniho AP bylo zakazane parametrem timeout (WC_DONT_RUN_CONFIGAP)
}wificonfigresult_t;

typedef void (*wificonfig_cb)(wificonfigstate_t state); // definice callbacku

typedef void (*storeparam_cb)(const char *newvalue); // callback pro ulozeni uzivatelskeho parametru

#ifndef WIFI_STA_CONNECT_TIMEOUT
 #define WIFI_STA_CONNECT_TIMEOUT 15000UL // delka cekani na pripojeni k AP [ms]
#endif

#ifndef SETUP_SSID
 #define SETUP_SSID "ESPPBSetup_?"
#endif

#ifndef SETUP_CHANNEL
 #define SETUP_CHANNEL 3
#endif

/**
 *  \brief Ziskani retezce z EEPROM ze zadaneho offsetu
 *
 *  \param [in] start Ofset zacatku retezce
 *  \param [in] len Delka retezce
 *  \return Vycteny retezec
 */
String getEEPROMString(int start, int len);

/**
 *  \brief Ulozeni retezce do EEPROM
 *
 *  \param [in] start Ofset zacatku ukladani
 *  \param [in] len Maximalni delka ulozeneho retezce
 *  \param [in] string Ukladany retezec
 */
void setEEPROMString(int start, int len, String string);

/**
 *  \brief Ziskani nasi ip adresy
 *
 *  \return Nase ip adresa, ziskana dle rezimu prace modulu
 */
IPAddress getOurIP(void);

/**
 *  \brief Ziskani nasi MAC adresy
 *
 *  \param [in] mac kam ma byt MAC adresa ulozena
 *  \return Ukazatel na ulozenou MAC adresu (vraci parametr mac)
 *
 *  \details Details
 */
uint8_t * getOurMAC(uint8_t *mac);

uint32_t getEEPROMuint32(int start);

void setEEPROMuint32(int start, uint32_t val);

extern char WiFiDeviceName[]; // jmeno zarizeni, pouzivane i pro DHCP

class WiFiConfigUsrParameter
{
  public:
    WiFiConfigUsrParameter(const char *id, const char *label, const char *defaultValue, int length, storeparam_cb cb);

    const char *getID();
    const char *getValue();
    const char *getLabel();
    int         getValueLength();
	void		setNext(WiFiConfigUsrParameter *n);
	WiFiConfigUsrParameter *getNext();
	void		setNewValue(const char *newval);
  private:
    const char *_id;
    const char *_label;
    char       *_value;
    int         _length;

	storeparam_cb _cb;
	WiFiConfigUsrParameter *_next;

    friend class WiFiConfig;
};

class WiFiConfig
{
public:
	WiFiConfig(): _params(NULL), _timeout(0) {}
	wificonfigresult_t begin(int configarea, uint8_t forceConfigure, wificonfig_cb cb);
	wificonfigresult_t begin(int configarea, uint8_t forceConfigure, int timeout, wificonfig_cb cb);
    void addParameter(WiFiConfigUsrParameter *p);
private:
	wificonfigresult_t _setupAP(wificonfig_cb cb);
	void _handleDisplayAP(void);
	void _handleSetAP(void);
	void _handleNotFound(); // CaptivePortal redirector
	void _handleRoot(); // jen jednoducha stranka kvuli CaptivePortalu umoznuje prejit na spravnou stranku (ale nedela to...)
	bool captivePortal(void);
	WiFiConfigUsrParameter *_searchUsrParameter(const char *name);

	WiFiConfigUsrParameter *_params; // ukazatel na posledni zadany uzivatelsky parametr
	int _timeout; // timeout pri cekani na konfiguraci
	uint32_t _time; // hodnota, po ktere bude ESP restartovano (pokud je _timeout != 0)
};
#endif
