/**
 * @file ping.h
 * @brief Ping utility
 *
 * @section License
 *
 * Copyright (C) 2010-2015 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 1.6.5
 **/

#ifndef _PING_H
#define _PING_H

#include "core/ip.h"

//Maximum size of ping requests
#ifndef PING_MAX_DATA_SIZE
   #define PING_MAX_DATA_SIZE 8192
#elif (PING_MAX_DATA_SIZE < 0)
   #error PING_MAX_DATA_SIZE parameter is not valid
#endif

//Ping related functions
error_t ping(NetInterface *interface, const IpAddr *ipAddr,
   size_t size, uint8_t ttl, systime_t timeout, systime_t *rtt);

#endif
