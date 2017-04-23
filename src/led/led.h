/**
 * @file led.h
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
#ifndef _led_h_
#define _led_h_

#include <Ticker.h>
#include <pgmspace.h>

typedef struct
{

}ledsignal_t;

enum
{
	LEDS_ONFOR = 0x00, // rozsviti LED na delku, ktera je uvedena v nizsich 6-ti bitech (+1) [100ms], 0 znamena, ze se rozsviti na 100ms
	LEDS_OFFFOR = 0x40,
	LEDS_STOP = 0x80,
	LEDS_RESTART = 0xc0
};

class LED
{
//  typedef  void (LED::*runtime)(void);

  protected:
  	int _pin; // pin, na kterem je LED pripojena
  	Ticker _handler; // obsluha LED signalizace
  	const uint8_t *_signal; // ukazatel na vzor signalizace
  	PGM_P _psignal;
  	const uint8_t *_ptr; // ukazatel na aktualne zpracovavane misto v signalizaci
  	PGM_P _pptr;
  	uint32_t _timer; // casovani
  	int _ledon;
  	int _ledoff;
    enum
    {
      LS_IDLE, // klid, cekame na zmenu (zavolani set, nebo start)
      LS_RUN, // bezi automat
      LS_WAIT, // cekame v automatu
    }_state; // stav automatu
  public:
  	LED(int pin, int ledon, int ledoff);
  	void begin(void);
  	void set(const uint8_t *signal);
//  	void set(const __FlashStringHelper *signal);
  	void set(PGM_P signal);
    void start();
//    static void lh(void *ptr);
    static void lh(LED *ptr);
    void rtLed(void); // vykonna metoda
};
#endif
