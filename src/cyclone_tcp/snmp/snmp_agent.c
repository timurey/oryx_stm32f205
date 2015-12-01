/**
 * @file snmp_agent.c
 * @brief SNMP agent (Simple Network Management Protocol)
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
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"
#include "snmp/snmp_agent_pdu.h"
#include "snmp/snmp_agent_misc.h"
#include "crypto.h"
#include "asn1.h"
#include "oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED)

//sysUpTime.0 object (1.3.6.1.2.1.1.3.0)
static const uint8_t sysUpTimeObject[] = {43, 6, 1, 2, 1, 1, 3, 0};
//snmpTrapOID.0 object (1.3.6.1.6.3.1.1.4.1)
static const uint8_t snmpTrapOidObject[] = {43, 6, 1, 6, 3, 1, 1, 4, 1, 0};
//snmpTraps object (1.3.6.1.6.3.1.1.5)
static const uint8_t snmpTrapsObject[] = {43, 6, 1, 6, 3, 1, 1, 5};


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains SNMP agent settings
 **/

void snmpAgentGetDefaultSettings(SnmpAgentSettings *settings)
{
   //The SNMP agent is not bound to any interface
   settings->interface = NULL;

   //Default SNMP version
   settings->version = SNMP_VERSION_1;
   //Enterprise OID
   memset(settings->enterpriseOid, 0, SNMP_AGENT_MAX_OID_SIZE);
   //Length of the enterprise OID
   settings->enterpriseOidLen = 0;
   //Default read-only community string
   strcpy(settings->readOnlyCommunity, "public");
   //Default read-write community string
   strcpy(settings->readWriteCommunity, "private");
   //Default trap community string
   strcpy(settings->trapCommunity, "public");
}


/**
 * @brief SNMP agent initialization
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] settings SNMP agent specific settings
 * @return Error code
 **/

error_t snmpAgentInit(SnmpAgentContext *context, const SnmpAgentSettings *settings)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing SNMP agent...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear the SNMP agent context
   memset(context, 0, sizeof(SnmpAgentContext));

   //Save the underlying network interface
   context->interface = settings->interface;
   //SNMP version identifier
   context->version = settings->version;

   //Enterprise OID
   memcpy(context->enterpriseOid, settings->enterpriseOid, SNMP_AGENT_MAX_OID_SIZE);
   //Length of the enterprise OID
   context->enterpriseOidLen = settings->enterpriseOidLen;

   //Read-only community string
   strcpy(context->readOnlyCommunity, settings->readOnlyCommunity);
   //Read-write community string
   strcpy(context->readWriteCommunity, settings->readWriteCommunity);
   //Trap community string
   strcpy(context->trapCommunity, settings->trapCommunity);

   //Create a mutex to prevent simultaneous access to SNMP agent context
   if(!osCreateMutex(&context->mutex))
   {
      //Failed to create mutex
      return ERROR_OUT_OF_RESOURCES;
   }

   //Open a UDP socket
   context->socket = socketOpen(SOCKET_TYPE_DGRAM, SOCKET_IP_PROTO_UDP);

   //Failed to open socket?
   if(!context->socket)
   {
      //Clean up side effects
      osDeleteMutex(&context->mutex);
      //Report an error
      return ERROR_OPEN_FAILED;
   }

   //Start of exception handling block
   do
   {
      //Explicitly associate the socket with the relevant interface
      error = socketBindToInterface(context->socket, context->interface);
      //Unable to bind the socket to the desired interface?
      if(error)
         break;

      //The client listens for SNMP messages on port 161
      error = socketBind(context->socket, &IP_ADDR_ANY, SNMP_PORT);
      //Unable to bind the socket to the desired port?
      if(error)
         break;

      //End of exception handling block
   } while(0);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      osDeleteMutex(&context->mutex);
      //Close underlying socket
      socketClose(context->socket);
   }

   //Return status code
   return error;
}


/**
 * @brief Load a MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] module Pointer the MIB module
 * @return Error code
 **/

error_t snmpAgentLoadMib(SnmpAgentContext *context, const MibModule *module)
{
   uint_t i;
   uint_t j;

   //Check parameters
   if(context == NULL || module == NULL)
      return ERROR_INVALID_PARAMETER;
   if(module->numObjects < 1)
      return ERROR_INVALID_PARAMETER;

   //Make sure there is enough room to add the specified MIB
   if(context->mibModuleCount >= SNMP_AGENT_MAX_MIB_COUNT)
      return ERROR_OUT_OF_RESOURCES;

   //Loop through existing MIBs
   for(i = 0; i < context->mibModuleCount; i++)
   {
      //Compare object identifiers
      if(oidComp(module->objects[0].oid, module->objects[0].oidLen,
         context->mibModule[i]->objects[0].oid, context->mibModule[i]->objects[0].oidLen) < 0)
      {
         //Make room for the new MIB
         for(j = context->mibModuleCount; j > i; j--)
            context->mibModule[j] = context->mibModule[j - 1];

         //We are done
         break;
      }
   }

   //Add the new MIB to the list
   context->mibModule[i] = module;
   //Update the number of MIBs
   context->mibModuleCount++;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Start SNMP agent
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpAgentStart(SnmpAgentContext *context)
{
   OsTask *task;

   //Debug message
   TRACE_INFO("Starting SNMP agent...\r\n");

   //Make sure the SNMP agent context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Start the SNMP agent service
   task = osCreateTask("SNMP Agent", (OsTaskCode) snmpAgentTask,
      context, SNMP_AGENT_STACK_SIZE, SNMP_AGENT_PRIORITY);

   //Unable to create the task?
   if(task == OS_INVALID_HANDLE)
      return ERROR_OUT_OF_RESOURCES;

   //The SNMP agent has successfully started
   return NO_ERROR;
}


/**
 * @brief Set trap destination IP address
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] index Zero-based index
 * @param[in] ipAddr Trap destination IP address
 * @return Error code
 **/

error_t snmpAgentSetTrapDestAddr(SnmpAgentContext *context,
   uint_t index, const IpAddr *ipAddr)
{
   //Check parameters
   if(context == NULL || ipAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= SNMP_AGENT_MAX_TRAP_DEST_ADDRS)
      return ERROR_OUT_OF_RANGE;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Save the trap destination IP address
   context->trapDestAddr[index] = *ipAddr;

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send SNMP trap message
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] genericTrapType Generic trap type
 * @param[in] specificTrapCode Specific code
 * @param[in] objectList List of object names
 * @param[in] objectListSize Number of entries in the list
 * @return Error code
 **/

error_t snmpSendTrap(SnmpAgentContext *context, uint_t genericTrapType,
   uint_t specificTrapCode, const SnmpTrapObject *objectList, uint_t objectListSize)
{
   error_t error;
   uint_t i;
   size_t n;
   systime_t time;
   SnmpVarBind var;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //A Trap-PDU or a SNMPv2-Trap-PDU will be sent depending on SNMP version
   if(context->version == SNMP_VERSION_1)
      context->response.pduType = SNMP_PDU_TRAP;
   else
      context->response.pduType = SNMP_PDU_TRAP_V2;

   //The message uses the trap community name
   context->response.community = (uint8_t *) context->trapCommunity;
   context->response.communityLen = strlen(context->trapCommunity);

   //Save trap type
   context->response.genericTrapType = genericTrapType;
   context->response.specificTrapCode = specificTrapCode;

   //Clear request identifier
   context->response.requestId = 0;

   //Clear error status
   context->response.errorStatus = SNMP_ERROR_NONE;
   context->response.errorIndex = 0;

   //Make room for the message header at the beginning of the buffer
   context->response.varBindList = context->response.buffer +
      context->response.communityLen + SNMP_AGENT_MAX_OID_SIZE +
      SNMP_MSG_HEADER_OVERHEAD;

   //Number of bytes available in the response buffer
   context->response.bytesAvailable = SNMP_MSG_MAX_SIZE -
      SNMP_AGENT_MAX_OID_SIZE - SNMP_MSG_HEADER_OVERHEAD -
      context->response.communityLen;

   //Reset byte counters
   context->response.varBindListLen = 0;
   context->response.oidLen = 0;

   //Clear status code
   error = NO_ERROR;

   //Start of exception handling block
   do
   {
      //SNMPv2 version?
      if(context->version == SNMP_VERSION_2)
      {
         //Get current time
         time = osGetSystemTime() / 10;

         //Encode the value object using ASN.1 rules
         error = snmpEncodeUnsignedInt32(time, context->response.buffer, &n);
         //Any error to report?
         if(error)
            break;

         //The first two variable bindings in the variable binding list of an
         //SNMPv2-Trap-PDU are sysUpTime.0 and snmpTrapOID.0 respectively
         var.oid = sysUpTimeObject;
         var.oidLen = sizeof(sysUpTimeObject);
         var.class = ASN1_CLASS_APPLICATION;
         var.type = MIB_TYPE_TIME_TICKS;
         var.value = context->response.buffer;
         var.valueLen = n;

         //Append sysUpTime.0 to the the variable binding list
         error = snmpWriteVarBinding(context, &var);
         //Any error to report?
         if(error)
            break;

         //Generic or enterprise-specific trap?
         if(genericTrapType < SNMP_TRAP_ENTERPRISE_SPECIFIC)
         {
            //Retrieve the length of the snmpTraps OID
            n = sizeof(snmpTrapsObject);
            //Copy the OID
            memcpy(context->response.buffer, snmpTrapsObject, n);

            //For generic traps, the SNMPv2 snmpTrapOID parameter shall be
            //the corresponding trap as defined in section 2 of 3418
            context->response.buffer[n] = genericTrapType + 1;

            //Update the length of the snmpTrapOID parameter
            n++;
         }
         else
         {
            //Retrieve the length of the enterprise OID
            n = context->enterpriseOidLen;

            //For enterprise specific traps, the SNMPv2 snmpTrapOID parameter shall
            //be the concatenation of the SNMPv1 enterprise OID and two additional
            //sub-identifiers: '0' and the SNMPv1 specific trap parameter
            memcpy(context->response.buffer, context->enterpriseOid, n);

            //Concatenate the '0' sub-identifier
            context->response.buffer[n++] = 0;

            //Concatenate the specific trap parameter
            context->response.buffer[n] = specificTrapCode % 128;

            //Loop as long as necessary
            for(i = 1; specificTrapCode > 128; i++)
            {
               //Split the binary representation into 7 bit chunks
               specificTrapCode /= 128;
               //Make room for the new chunk
               memmove(context->response.buffer + n + 1, context->response.buffer + n, i);
               //Set the most significant bit in the current chunk
               context->response.buffer[n] = OID_MORE_FLAG | (specificTrapCode % 128);
            }

            //Update the length of the snmpTrapOID parameter
            n += i;
         }

         //The snmpTrapOID.0 variable occurs as the second variable
         //binding in every SNMPv2-Trap-PDU
         var.oid = snmpTrapOidObject;
         var.oidLen = sizeof(snmpTrapOidObject);
         var.class = ASN1_CLASS_UNIVERSAL;
         var.type = ASN1_TYPE_OBJECT_IDENTIFIER;
         var.value = context->response.buffer;
         var.valueLen = n;

         //Append snmpTrapOID.0 to the the variable binding list
         error = snmpWriteVarBinding(context, &var);
         //Any error to report?
         if(error)
            break;
      }

      //Loop through the list of objects
      for(i = 0; i < objectListSize; i++)
      {
         //Get object identifier
         var.oid = objectList[i].oid;
         var.oidLen = objectList[i].oidLen;

         //Retrieve object value
         error = snmpGetObjectValue(context, &var);
         //Any error to report?
         if(error)
            break;

         //Append variable binding to the the list
         error = snmpWriteVarBinding(context, &var);
         //Any error to report?
         if(error)
            break;
      }

      //Propagate exception if necessary...
      if(error)
         break;

      //Format PDU header
      error = snmpWritePduHeader(context);
      //Any error to report?
      if(error)
         break;

      //Loop through the list of destination IP addresses
      for(i = 0; i < SNMP_AGENT_MAX_TRAP_DEST_ADDRS; i++)
      {
         //Ensure the IP address is valid
         if(context->trapDestAddr[i].length != 0)
         {
            //Debug message
            TRACE_INFO("Sending SNMP message to %s port %" PRIu16
               " (%" PRIuSIZE " bytes)...\r\n",
               ipAddrToString(&context->trapDestAddr[i], NULL),
               SNMP_TRAP_PORT, context->response.messageLen);

            //Display the contents of the SNMP message
            TRACE_DEBUG_ARRAY("  ", context->response.message,
               context->response.messageLen);

            //Display ASN.1 structure
            asn1DumpObject(context->response.message,
               context->response.messageLen, 0);

            //Send SNMP trap message
            socketSendTo(context->socket, &context->trapDestAddr[i],
               SNMP_TRAP_PORT, context->response.message,
               context->response.messageLen, NULL, 0);
         }
      }

      //End of exception handling block
   } while(0);

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
}


/**
 * @brief SNMP agent task
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpAgentTask(SnmpAgentContext *context)
{
   error_t error;

#if (NET_RTOS_SUPPORT == ENABLED)
   //Main loop
   while(1)
   {
#endif
      //Wait for an incoming datagram
      error = socketReceiveFrom(context->socket, &context->remoteIpAddr,
         &context->remotePort, context->request.buffer,
         SNMP_MSG_MAX_SIZE, &context->request.bufferLen, 0);

      //Any datagram received?
      if(!error)
      {
         //Acquire exclusive access to the SNMP agent context
         osAcquireMutex(&context->mutex);

         //Debug message
         TRACE_INFO("SNMP message received from %s port %" PRIu16
            " (%" PRIuSIZE " bytes)...\r\n",
            ipAddrToString(&context->remoteIpAddr, NULL),
            context->remotePort, context->request.bufferLen);

         //Display the contents of the SNMP message
         TRACE_DEBUG_ARRAY("  ", context->request.buffer,
            context->request.bufferLen);

         //Dump ASN.1 structure
         asn1DumpObject(context->request.buffer,
            context->request.bufferLen, 0);

         //Parse incoming SNMP message
         error = snmpParseMessage(context, context->request.buffer,
            context->request.bufferLen);

         //Check status code
         if(!error)
         {
            //Debug message
            TRACE_INFO("Sending SNMP message to %s port %" PRIu16
               " (%" PRIuSIZE " bytes)...\r\n",
               ipAddrToString(&context->remoteIpAddr, NULL),
               context->remotePort, context->response.messageLen);

            //Display the contents of the SNMP message
            TRACE_DEBUG_ARRAY("  ", context->response.message,
               context->response.messageLen);

            //Display ASN.1 structure
            asn1DumpObject(context->response.message,
               context->response.messageLen, 0);

            //Send SNMP response message
            error = socketSendTo(context->socket, &context->remoteIpAddr,
               context->remotePort, context->response.message,
               context->response.messageLen, NULL, 0);
         }

         //Release exclusive access to the SNMP agent context
         osReleaseMutex(&context->mutex);
      }
#if (NET_RTOS_SUPPORT == ENABLED)
   }
#endif
}

#endif
