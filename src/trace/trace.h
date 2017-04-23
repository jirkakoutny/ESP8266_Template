/**
 * @file trace.h
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
#ifndef _TRACE_H_
#define _TRACE_H_

#include <Arduino.h>
#include <ESP8266WebServer.h>

// Definice jednotlivych typu hlaseni do trasovani
#define TRACE_ERROR   0 // chybova zprava = cervena
#define TRACE_WARNING 1 // varovani - zluta
#define TRACE_INFO    2 // informacni zprava - zelena
#define TRACE_DEBUG   3 // ladici zprava - cerna

#define TRACE_CHECK_INTERVAL 200 // interval [ms], po kterem je testovano odesilani stopare

#define TRACE_WEBSOCKET 333 // cislo portu, pouziteho pro webovy soket

/**
 * @brief Inicializace modulu
 */
void trace_init(void);

/**
 * @brief Ziskani vypisu v HTML formatu
 * @details Ziskame retezec s HTML kodovanim, reprezentujici aktualni obsah trasovaciho bufferu
 *
 * @param str Retezec v HTML kodovani
 */
void trace_dumphtml(String &str);

/**
 * @brief      Vyprazdneni stopovaciho bufferu
 */
void trace_clear(void);

/**
 * @brief      Ukonceni prace stopare - vyprazdni buffer
 */
void trace_end(void);

/**
 * @brief      Ulozeni zpravy s obsahem z programove pameti (PROGMEM, F, ...)
 *
 * @param[in]  severity   Uroven - viz. TRACE_ERROR, TRACE_WARNING, ...
 * @param[in]  fmt        Formatovaci retezec vysledne zpravy. Je ulozeny v PROGMEM
 * @param[in]  <unnamed>  Parametry, ktere jsou dosazeny do formatovaciho retezce
 */
void trace_print(uint8_t severity, const __FlashStringHelper *fmt, ...);

/**
 * @brief      Ulozeni zpravy
 *
 * @param[in]  severity   Uroven - viz. TRACE_ERROR, TRACE_WARNING, ...
 * @param[in]  fmt        Formatovaci retezec vysledne zpravy.
 * @param[in]  <unnamed>  Parametry, ktere jsou dosazeny do formatovaciho retezce
 */
void trace_print(uint8_t severity, const char *fmt, ...);

/**
 * @brief      Registrace callback metody - je volana pri kazde zmene (zapisu) do stopare
 *
 * @param[in]  <unnamed>  Ukazatel na volanou metodu
 */
void trace_registermessagecb(void (*)(const char *msg));

/**
 * @brief      Pridani obsluhy stranky /trace do weboveho serveru
 *
 * @param      server  The server
 */
void trace_addweb(ESP8266WebServer *server);

/**
 * @brief      Periodicky tik modulem - osetruje odeslani novych dat v pripade modifikace
 */
void trace_poll();

#endif // _TRACE_H_
