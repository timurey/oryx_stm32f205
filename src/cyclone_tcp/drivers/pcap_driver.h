/**
 * @file pcap_driver.h
 * @brief PCAP driver
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

#ifndef _PCAP_DRIVER_H
#define _PCAP_DRIVER_H

//Dependencies
#include "core/nic.h"

//TX buffer size
#ifndef PCAP_DRIVER_TX_BUFFER_SIZE
   #define PCAP_DRIVER_TX_BUFFER_SIZE 1536
#elif (PCAP_DRIVER_TX_BUFFER_SIZE != 1536)
   #error PCAP_DRIVER_TX_BUFFER_SIZE parameter is not valid
#endif

//RX buffer size
#ifndef PCAP_DRIVER_RX_BUFFER_SIZE
   #define PCAP_DRIVER_RX_BUFFER_SIZE 1536
#elif (PCAP_DRIVER_RX_BUFFER_SIZE != 1536)
   #error PCAP_DRIVER_RX_BUFFER_SIZE parameter is not valid
#endif

//PCAP driver
extern const NicDriver pcapDriver;

//PCAP related functions
error_t pcapDriverInit(NetInterface *interface);

void pcapDriverTick(NetInterface *interface);

void pcapDriverEnableIrq(NetInterface *interface);
void pcapDriverDisableIrq(NetInterface *interface);

void pcapDriverEventHandler(NetInterface *interface);

error_t pcapDriverSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset);

error_t pcapDriverSetMulticastFilter(NetInterface *interface);

#endif
