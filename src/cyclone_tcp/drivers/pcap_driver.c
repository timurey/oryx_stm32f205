/**
 * @file pcap_driver.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL NIC_TRACE_LEVEL

//Dependencies
#include <conio.h>
#include "core/net.h"
#include "drivers/pcap_driver.h"
#include "debug.h"

//Undefine conflicting definitions
#undef Socket
#undef htons
#undef htonl
#undef ntohs
#undef ntohl

//PCAP dependencies
#include <pcap.h>

//Undefine conflicting definitions
#undef interface


/**
 * @brief PCAP driver context
 **/

typedef struct
{
   pcap_t *handle;
   uint8_t txBuffer[PCAP_DRIVER_TX_BUFFER_SIZE];
   uint8_t rxBuffer[PCAP_DRIVER_RX_BUFFER_SIZE];
} PcapDriverContext;


/**
 * @brief PCAP driver
 **/

const NicDriver pcapDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   pcapDriverInit,
   pcapDriverTick,
   pcapDriverEnableIrq,
   pcapDriverDisableIrq,
   pcapDriverEventHandler,
   pcapDriverSendPacket,
   pcapDriverSetMulticastFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief PCAP driver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t pcapDriverInit(NetInterface *interface)
{
   int_t ret;
   uint_t i;
   uint_t j;
   pcap_if_t *device;
   pcap_if_t *deviceList;
   struct bpf_program filerCode;
   char_t filterExpr[256];
   char_t buffer[PCAP_ERRBUF_SIZE];
   PcapDriverContext *context;

   //Debug message
   TRACE_INFO("Initializing PCAP driver...\r\n");

   //Point to the PCAP driver context
   context = (PcapDriverContext *) interface->nicContext;

   //Find all the devices
   ret = pcap_findalldevs(&deviceList, buffer);

   //Any error to report?
   if(ret != 0)
   {
      //Debug message
      printf("Failed to list devices!\r\n");
      //Report an error
      return ERROR_FAILURE;
   }

   //No network adapter found?
   if(deviceList == NULL)
   {
      //Debug message
      printf("No network adapter found!\r\n");
      //Exit immediately
      return ERROR_FAILURE;
   }

   //Network adapter selection
   while(1)
   {
      //Debug message
      printf("Network adapters:\r\n");

      //Point to the first device
      device = deviceList;
      i = 0;

      //Loop through the list of devices
      while(device != NULL)
      {
         //Display the name of the current network adapter
         if(device->description != NULL)
            printf("  %u - %s\r\n", i + 1, device->description);
         else
            printf("  %u - %s\r\n", i + 1, "No description available");

         //Next device
         device = device->next;
         i++;
      }

      //Display message
      printf("Select network adapter for %s interface (1-%u):", interface->name, i);
      //Get user choice
      scanf("%d", &j);

      //Valid selection?
      if(j > 1 && j <= i)
         break;
   }

   //Point to the first device
   device = deviceList;

   //Point to the desired network adapter
   for(i = 1; i < j; i++)
      device = device->next;

   //Open the device
   context->handle = pcap_open_live(device->name, 65535, TRUE, 10, buffer);

   //Failed to open device?
   if(context->handle == NULL)
   {
      //Debug message
      printf("Failed to open device!\r\n");
      //Clean up side effects
      pcap_freealldevs(deviceList);
      //Report an error
      return ERROR_FAILURE;
   }

   //Free the device list
   pcap_freealldevs(deviceList);

   //Filter expression
   sprintf(filterExpr, "!(ether src %02x:%02x:%02x:%02x:%02x:%02x) && "
      "((ether dst %02x:%02x:%02x:%02x:%02x:%02x) || (ether broadcast) || (ether multicast))",
      interface->macAddr.b[0], interface->macAddr.b[1], interface->macAddr.b[2],
      interface->macAddr.b[3], interface->macAddr.b[4], interface->macAddr.b[5],
      interface->macAddr.b[0], interface->macAddr.b[1], interface->macAddr.b[2],
      interface->macAddr.b[3], interface->macAddr.b[4], interface->macAddr.b[5]);

   //Compile the filter
   ret = pcap_compile(context->handle, &filerCode, filterExpr, 1, 0);

   //Failed to open device?
   if(ret != 0)
   {
      //Debug message
      printf("Failed to compile filter!\r\n");
      //Clean up side effects
      pcap_close(context->handle);
      //Report an error
      return ERROR_FAILURE;
   }

   //Set the filter
   ret = pcap_setfilter(context->handle, &filerCode);

   //Failed to open device?
   if(ret != 0)
   {
      //Debug message
      printf("Failed to set filter!\r\n");
      //Clean up side effects
      pcap_close(context->handle);
      //Report an error
      return ERROR_FAILURE;
   }

   //The transmitter is ready
   osSetEvent(&interface->nicTxEvent);

   //The receiver can accept a packet
   interface->nicEvent = TRUE;
   osSetEvent(&netEvent);

   //Return status code
   return NO_ERROR;
}


/**
 * @brief PCAP timer handler
 *
 * This routine is periodically called by the TCP/IP stack to
 * handle periodic operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void pcapDriverTick(NetInterface *interface)
{
   int_t ch;

   //Any key pressed?
   if(_kbhit())
   {
      //Get the corresponding character
      ch = _getch();

      //U key pressed?
      if(toupper(ch) == 'U' && !interface->linkState)
      {
         //Generate link-up event
         interface->linkState = TRUE;
         nicNotifyLinkChange(interface);
      }
      //D key pressed?
      else if(toupper(ch) == 'D' && interface->linkState)
      {
         ////Generate link-down event
         interface->linkState = FALSE;
         nicNotifyLinkChange(interface);
      }
   }
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void pcapDriverEnableIrq(NetInterface *interface)
{
   //Not implemented
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void pcapDriverDisableIrq(NetInterface *interface)
{
   //Not implemented
}


/**
 * @brief PCAP event handler
 * @param[in] interface Underlying network interface
 **/

void pcapDriverEventHandler(NetInterface *interface)
{
   int_t ret;
   size_t n;
   uint8_t *data;
   struct pcap_pkthdr *header;
   PcapDriverContext *context;

   //Point to the PCAP driver context
   context = (PcapDriverContext *) interface->nicContext;

   //Wait for an incoming packet
   ret = pcap_next_ex(context->handle, &header, &data);

   //Any packet received?
   if(ret != 0)
   {
      //Retrieve the length of the packet
      n = header->caplen;

      //Check the length of the received packet
      if((n + ETH_CRC_SIZE) < PCAP_DRIVER_RX_BUFFER_SIZE)
      {
         //Check whether the link is up
         if(interface->linkState)
         {
            //Copy the packet to the receive buffer
            memcpy(context->rxBuffer, data, n);

            //CRC is not included in the transfer...
            context->rxBuffer[n++] = 0xCC;
            context->rxBuffer[n++] = 0xCC;
            context->rxBuffer[n++] = 0xCC;
            context->rxBuffer[n++] = 0xCC;

            //Pass the packet to the upper layer
            nicProcessPacket(interface, context->rxBuffer, n);
         }
      }
   }

   //The receiver can accept another packet
   interface->nicEvent = TRUE;
   osSetEvent(&netEvent);
}


/**
 * @brief Send a packet
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the data to send
 * @param[in] offset Offset to the first data byte
 * @return Error code
 **/

error_t pcapDriverSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset)
{
   int_t ret;
   size_t length;
   PcapDriverContext *context;

   //Point to the PCAP driver context
   context = (PcapDriverContext *) interface->nicContext;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > PCAP_DRIVER_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Copy the packet to the transmit buffer
   netBufferRead(context->txBuffer, buffer, offset, length);

   //Send packet
   ret = pcap_sendpacket(context->handle, context->txBuffer, length);

   //The transmitter can accept another packet
   osSetEvent(&interface->nicTxEvent);

   //Return status code
   if(ret < 0)
      return ERROR_FAILURE;
   else
     return NO_ERROR;
}


/**
 * @brief Configure multicast MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t pcapDriverSetMulticastFilter(NetInterface *interface)
{
   //Not implemented
   return NO_ERROR;
}
