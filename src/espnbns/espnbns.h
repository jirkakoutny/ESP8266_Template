/**
 * @file espnbns.h
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
#ifndef __ESPNBNS_h__
#define __ESPNBNS_h__

#include <ESP8266WiFi.h>

#define NBNS_PORT 137
/**
* @def NBNS_MAX_HOSTNAME_LEN
* @brief maximalni delka NBNS jmena zarizeni
* @remarks
* Jmeno zarizeni musi byt uvedeno VELKYMI pismenami a nesmi obsahovat mezery (whitespaces).
*/
#define NBNS_MAX_HOSTNAME_LEN 16

class NBNS
{
public:
	bool begin(const char *name);
	void poll(void);
private:
	char _name[NBNS_MAX_HOSTNAME_LEN + 1];
	void _getnbname(char *nbname, char *name, uint8_t maxlen);
	void _makenbname(char *name, char *nbname, uint8_t outlen);
};
#endif