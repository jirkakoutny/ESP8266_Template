/**
 * Sablona pro ESP8266 skript
 *
 * @file et.ino
 * @author Pavel Brychta, http://www.xpablo.cz
 *
 * Copyright (c) 2016 Pavel Brychta. All rights reserved.
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

#include "configuration.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <SPI.h>
#include "src/interval/interval.h"
#include "src/espnbns/espnbns.h"
#include "src/WiFiConfig/WiFiConfig.h"
#if (LED_PIN != NO_PIN)
 #include "src/led/led.h"
#endif
#include "embeddedHTML.h"
#ifdef USE_SPIFFS
 #warning Pouzivame SPIFFS - nezapomen pripravit obsah flash!
 #include "FS.h"
#endif

// Definice obsazeni EEPROM
#define elementSize(type, element) sizeof(((type *)0)->element)
typedef struct
{
  wificonfigarea_t wc; // oblast, vyhrazena pro konfiguraci WiFi
// **** sem pokracuji dalsi polozky, ukladane do EEPROM

} eepromdata_t;

#if (LED_PIN != NO_PIN)
 LED led(LED_PIN, LOW, HIGH);
 const uint8_t LS_CONNECTING[] = {LEDS_ONFOR + 0, LEDS_OFFFOR + 8, LEDS_RESTART};
 const uint8_t LS_CONFIGAP[] = {LEDS_ONFOR + 0, LEDS_OFFFOR + 0, LEDS_ONFOR + 0, LEDS_OFFFOR + 6, LEDS_RESTART};
 const uint8_t LS_CONNECTED[] = {LEDS_ONFOR + 0, LEDS_OFFFOR + 0, LEDS_ONFOR + 0, LEDS_OFFFOR + 0, LEDS_ONFOR + 0, LEDS_OFFFOR + 6, LEDS_RESTART};
#endif

int otaActive = 0; // priznak moznosti aktivovat OTA
NBNS nbns; // Netbios
ESP8266WebServer www(HTTP_PORT); // webovy server
// **** sem je mozne dopsat dalsi globalni promenne

void wificfgcb(wificonfigstate_t state)
{

  switch (state)
  {
    case WCS_CONNECTSTART:
    // **** kod pro start signalizace, oznamujici zacatek pripojovani k WiFi siti (volano pouze jednou)
#if (LED_PIN != NO_PIN)
      led.set(LS_CONNECTING);
#endif

      break;

    case WCS_CONNECTING:
    // **** kod pro periodickou signalizaci probihajiciho pripojovani k WiFi siti (volano periodicky)

      break;

    case WCS_CONNECTED:
    // **** kod pro start signalizace uspesneho pripojeni k WiFi siti (volano pouze jednou)
#if (LED_PIN != NO_PIN)
      led.set(LS_CONNECTED);
#endif

      break;

    case WCS_CONFIGSTART:
    // **** kod pro start signalizace, oznamujici spusteni konfiguracniho AP (volano pouze jednou)
#if (LED_PIN != NO_PIN)
      led.set(LS_CONFIGAP);
#endif

      break;

    case WCS_CONFIGWAIT:
    // **** kod pro periodickou signalizaci beziciho konfiguracniho AP (volano periodicky)

      break;
  }
}

#ifdef USE_SPIFFS
String getContentType(String filename)
{
  if (www.hasArg("download")) return F("application/octet-stream");
  else if (filename.endsWith(".htm")) return F("text/html");
  else if (filename.endsWith(".html")) return F("text/html");
  else if (filename.endsWith(".css")) return F("text/css");
  else if (filename.endsWith(".js")) return F("application/javascript");
  else if (filename.endsWith(".png")) return F("image/png");
  else if (filename.endsWith(".gif")) return F("image/gif");
  else if (filename.endsWith(".jpg")) return F("image/jpeg");
  else if (filename.endsWith(".ico")) return F("image/x-icon");
  else if (filename.endsWith(".xml")) return F("text/xml");
  else if (filename.endsWith(".pdf")) return F("application/x-pdf");
  else if (filename.endsWith(".zip")) return F("application/x-zip");
  else if (filename.endsWith(".gz")) return F("application/x-gzip");
  return F("text/plain");
}

bool handleFileRead(String path)
{
  if (path.endsWith("/")) path += F("index.htm");
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
  {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz"; // prioritu ma komprimovany obsah
    File file = SPIFFS.open(path, "r");
    size_t sent = www.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}
#endif

void handleNotFound()
{
  String message = F("File Not Found\r\n\r\nURI:");
  message += www.uri();
  message += F("\r\nMethod: ");
  message += (www.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\r\nArguments: ");
  message += www.args();
  message += F("\r\n");

  for (int i = 0; i < www.args(); i++ )
  {
    message += " " + www.argName(i) + ": " + www.arg(i) + "\r\n";
  }
  www.send(404, "text/plain", message);
}

void ICACHE_FLASH_ATTR setup()
{
  int _fc;

  TRACE_INIT; // inicializace ladeni
  EEPROM.begin(sizeof(eepromdata_t) + 10); // zahajujeme praci s EEPROM (10 bytu je jen rezerva)
#if (LED_PIN != NO_PIN)
  led.begin(); // inicializace signalizace
#endif

#if (FORCE_CONFIG_BUTTON_PIN != NO_PIN)
  pinMode(FORCE_CONFIG_BUTTON_PIN, INPUT_PULLUP);
  delay(20); // male zpozdeni, aby se ustalila hodnota na vstupu
  _fc = digitalRead(FORCE_CONFIG_BUTTON_PIN); // pokud je na I/O pinu hodnota 0, tak vynutime nastavovaci AP
#else
  _fc = 1; // nevstupujeme do konfigurace
#endif
  {
    WiFiConfig wifi; // konfigurace WiFi casti ESP modulu

    if (WCR_OK != wifi.begin(offsetof(eepromdata_t, wc), _fc, 60, wificfgcb)) // startujeme pripojeni
      ESP.restart();
  }
  if (ESP.getFlashChipRealSize() > 1000000)
    otaActive = 1; // flash pameti je dost - povolime OTA

  if (strlen(WiFiDeviceName))
  {
    nbns.begin(WiFiDeviceName);
    if (otaActive)
    {
      ArduinoOTA.setHostname(WiFiDeviceName);
      ArduinoOTA.begin();
      TRACE(TRACE_INFO, F("OTA aktivovano"));
    }
    else
    {
  	  MDNS.begin(WiFiDeviceName);
      MDNS.addService("http", "tcp", HTTP_PORT);
    }
  }
// montaz souboroveho systemu
#ifdef USE_SPIFFS
  if (!SPIFFS.begin())
  	TRACE(TRACE_ERROR, F("SPIFFS neni pripojeny!"));
#endif
// Start weboveho serveru - sem je mozno pridavat odkazy na dalsi stranky

  www.on("/favicon.ico", []() {
    www.send(200, FPSTR(HTML), "");
  });

  www.onNotFound([]() {
#ifdef USE_SPIFFS
    if (!handleFileRead(www.uri()))
#endif
      handleNotFound();
  });

  TRACE_ADDWEB(&www); // ladici stranka
  www.begin(); // startujeme webovy server
// **** dalsi uzivatelska inicializace

}

void loop()
{

  if (otaActive)
  { // pripadna obsluha OTA aktualizace FW
    ArduinoOTA.handle();
  }
  www.handleClient();
  nbns.poll();
  TRACE_POLL;
// **** dalsi uzivatelske metody

}

