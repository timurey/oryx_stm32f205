/*
 * network.c
 *
 *  Created on: 11 янв. 2015 г.
 *      Author: timurtaipov
 */

#include "core/net.h"
#include "dhcp/dhcp_client.h"
#include "drivers/enc28j60.h"
#include "drivers/spi_driver.h"
#include "drivers/ext_int_driver.h"
#include "stuff/configs.h"

#include "network.h"
#include "debug.h"
#include "rtc.h"
#include "uuid.h"

#include "rest/sensors.h"

static NetInterface *interface;

#if (AUTO_IP_SUPPORT == ENABLED)
AutoIpSettings autoIpSettings;
AutoIpContext autoIpContext;
#endif

DhcpClientSettings dhcpClientSettings;
DhcpClientCtx dhcpClientContext;
SlaacSettings slaacSettings;
SlaacContext slaacContext;
static bool_t useDhcp = FALSE;

static Ipv4Addr ipv4Addr;
static Ipv4Addr ipv4Mask;
static Ipv4Addr ipv4Gateway;
static Ipv4Addr ipv4Dns1;
static Ipv4Addr ipv4Dns2;
static char sMacAddr[18];
static    MacAddr macAddr;
static char hostname[NET_MAX_HOSTNAME_LEN];
#if IPV6_SUPPORT == ENABLED
static Ipv6Addr ipv6Addr;
#endif

static void ipv4UseDefaultConfigs(void)
{
   useDhcp = TRUE;
}
static error_t useDefaultMacAddress(void)
{
   error_t error;
   sprintf(sMacAddr, "00-50-C2-%X-%X-%X",uuid->b[10],uuid->b[9],uuid->b[8]);
   error = macStringToAddr(&sMacAddr[0], &macAddr);
   return error;
}
static error_t useDefaultHostName(void)
{
   sprintf(hostname, "stm32_%X-%X-%X",uuid->b[10],uuid->b[9],uuid->b[8]);
   return NO_ERROR;
}
#if IPV6_SUPPORT == ENABLED
static void ipv6_use_default_configs (void)
{
   error_t error;
   //Get default settings
   slaacGetDefaultSettings(&slaacSettings);
   //Set the network interface to be configured
   slaacSettings.interface = interface;

   //SLAAC initialization
   error = slaacInit(&slaacContext, &slaacSettings);
   //Failed to initialize SLAAC?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to initialize SLAAC!\r\n");
   }

   //Start IPv6 address autoconfiguration process
   error = slaacStart(&slaacContext);
   //Failed to start SLAAC process?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to start SLAAC!\r\n");
   }
   //	//Set link-local address
   //	ipv6StringToAddr(APP_IPV6_LINK_LOCAL_ADDR, &ipv6Addr);
   //	ipv6SetLinkLocalAddr(interface, &ipv6Addr);
   //
   //	//Set IPv6 prefix
   //	ipv6StringToAddr(APP_IPV6_PREFIX, &ipv6Addr);
   //	ipv6SetPrefix(interface, &ipv6Addr, APP_IPV6_PREFIX_LENGTH);
   //
   //	//Set global address
   //	ipv6StringToAddr(APP_IPV6_GLOBAL_ADDR, &ipv6Addr);
   //	ipv6SetGlobalAddr(interface, &ipv6Addr);
   //
   //	//Set router
   //	ipv6StringToAddr(APP_IPV6_ROUTER, &ipv6Addr);
   //	ipv6SetRouter(interface, &ipv6Addr);
   //
   //	//Set primary and secondary DNS servers
   //	ipv6StringToAddr(APP_IPV6_PRIMARY_DNS, &ipv6Addr);
   //	ipv6SetDnsServer(interface, 0, &ipv6Addr);
   //	ipv6StringToAddr(APP_IPV6_SECONDARY_DNS, &ipv6Addr);
   //	ipv6SetDnsServer(interface, 1, &ipv6Addr);
}
#endif

static uint8_t parseIpv4Config(char *data, jsmntok_t *jSMNtokens, jsmnerr_t resultCode)
{
   error_t error;
   int tokNum;
   uint8_t correct = 0;
   char ipString[] = "255.255.255.255\0";
   int length;
#if (IPV4_SUPPORT == ENABLED)

   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/use dhcp");
   if ((tokNum >= 0) & (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0))
   {
      useDhcp = TRUE;
      return 5;
   }

   else
   {
      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 address");
      if (tokNum >= 0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<=15)
         {
            memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
            ipString[length]='\0';
            //Set ip address
            error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
            if (error)
               return correct;
            correct++;
         }
         else
         {
            xprintf("Warning: wrong ip address in config file \"/config/lan.json\" ");
            return correct;
         }
      }
      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 netmask");
      if (tokNum >= 0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<=15)
         {
            memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
            ipString[length]='\0';
            //Set subnet mask
            error = ipv4StringToAddr(&ipString[0], &ipv4Mask);
            if (error)
               return correct;
            correct++;
         }
         else
         {
            xprintf("Warning: wrong subnet mask in config file \"/config/lan.json\" ");
            return correct;
         }
      }

      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 gateway");
      if (tokNum >= 0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<=15)
         {
            memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
            ipString[length]='\0';
            //Set default gateway
            error = ipv4StringToAddr(&ipString[0], &ipv4Gateway);
            if (error)
               return correct;
            correct++;
         }
         else
         {
            xprintf("Warning: wrong default gateway in config file \"/config/lan.json\" ");
            return correct;
         }
      }
      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 primary dns");
      if (tokNum >= 0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<=15)
         {
            memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
            ipString[length]='\0';
            //Set primary DNS server
            error = ipv4StringToAddr(&ipString[0], &ipv4Dns1);
            if (error)
               return correct;
            correct++;
         }
         else
         {
            xprintf("Warning: wrong primary dns in config file \"/config/lan.json\" ");
            return correct;
         }
      }
      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 secondary dns");
      if (tokNum >= 0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<=15)
         {
            memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
            ipString[length]='\0';
            //Set secondary DNS server
            error = ipv4StringToAddr(&ipString[0], &ipv4Dns2);
            if (error)
               return correct;
            correct++;
         }
         else
         {
            xprintf("Warning: wrong primary dns in config file \"/config/lan.json\" ");
            return correct;
         }
      }


#endif
      return correct;
   }
}

#if (IPV6_SUPPORT == ENABLED)

static uint8_t parseIpv6Config(char *data, jsmntok_t *jSMNtokens, jsmnerr_t resultCode)
{
   error_t error;
   int tokNum;
   uint8_t correct = 0;
   char ipString[] = "255.255.255.255\0";
   int length;


   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv6/use slaac");
   if ((tokNum >= 0) & (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0))
   {
      slaacGetDefaultSettings(&slaacSettings);
      //Set the network interface to be configured
      slaacSettings.interface = interface;

      //SLAAC initialization
      error = slaacInit(&slaacContext, &slaacSettings);
      //Failed to initialize SLAAC?
      if(error)
      {
         //Debug message
         TRACE_ERROR("Failed to initialize SLAAC!\r\n");
      }

      //Start IPv6 address autoconfiguration process
      error = slaacStart(&slaacContext);
      //Failed to start SLAAC process?
      if(error)
      {
         //Debug message
         TRACE_ERROR("Failed to start SLAAC!\r\n");
      }
      return 5;
   }

   //
   //	else
   //	{
   //		tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 address");
   //		if (tokNum >= 0)
   //		{
   //			length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //			if (length<=15)
   //			{
   //				memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //				ipString[length]='\0';
   //				//Set IPv4 host address
   //				error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //				if (error)
   //					return correct;
   //				error = ipv4SetHostAddr(interface, ipv4Addr);
   //				if (error)
   //					return correct;
   //				correct++;
   //			}
   //			else
   //			{
   //				xprintf("Warning: wrong ip address in config file \"/config/lan.json\" ");
   //				return correct;
   //			}
   //		}
   //		tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 netmask");
   //		if (tokNum >= 0)
   //		{
   //			length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //			if (length<=15)
   //			{
   //				memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //				ipString[length]='\0';
   //				//Set subnet mask
   //				error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //				if (error)
   //					return correct;
   //				error = ipv4SetSubnetMask(interface, ipv4Addr);
   //				if (error)
   //					return correct;
   //				correct++;
   //			}
   //			else
   //			{
   //				xprintf("Warning: wrong subnet mask in config file \"/config/lan.json\" ");
   //				return correct;
   //			}
   //		}
   //
   //		tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 gateway");
   //		if (tokNum >= 0)
   //		{
   //			length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //			if (length<=15)
   //			{
   //				memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //				ipString[length]='\0';
   //				//Set default gateway
   //				error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //				if (error)
   //					return correct;
   //				error = ipv4SetDefaultGateway(interface, ipv4Addr);
   //				if (error)
   //					return correct;
   //				correct++;
   //			}
   //			else
   //			{
   //				xprintf("Warning: wrong default gateway in config file \"/config/lan.json\" ");
   //				return correct;
   //			}
   //		}
   //		tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 primary dns");
   //		if (tokNum >= 0)
   //		{
   //			length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //			if (length<=15)
   //			{
   //				memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //				ipString[length]='\0';
   //				//Set primary DNS server
   //				error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //				if (error)
   //					return correct;
   //				error = ipv4SetDnsServer(interface, 0, ipv4Addr);
   //				if (error)
   //					return correct;
   //				correct++;
   //			}
   //			else
   //			{
   //				xprintf("Warning: wrong primary dns in config file \"/config/lan.json\" ");
   //				return correct;
   //			}
   //		}
   //		tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/ipv4 secondary dns");
   //		if (tokNum >= 0)
   //		{
   //			length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //			if (length<=15)
   //			{
   //				memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //				ipString[length]='\0';
   //				//Set secondary DNS server
   //				error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //				if (error)
   //					return correct;
   //				error = ipv4SetDnsServer(interface, 1, ipv4Addr);
   //				if (error)
   //					return correct;
   //				correct++;
   //			}
   //			else
   //			{
   //				xprintf("Warning: wrong primary dns in config file \"/config/lan.json\" ");
   //				return correct;
   //			}
   //		}
   //
   //

   return correct;
   //	}
}
#endif
static error_t parseIpConfig(char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   uint8_t ipEnable = 0;
   jsmnerr_t resultCode;
   int length;
   int tokNum;
   error_t error;
   jsmn_init(jSMNparser);
   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/mac address");
   if (tokNum >= 0)
   {
      length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;

      if (length<=18)
      {
         memcpy(&sMacAddr[0], &data[jSMNtokens[tokNum].start], length);
         sMacAddr[length]='\0';
         //Set ip address
         error = macStringToAddr(&sMacAddr[0], &macAddr);
         if (error)
         {
            useDefaultMacAddress();
         }
      }
   }
   else
   {
      useDefaultMacAddress();
   }
   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/hostname");
   if (tokNum >= 0)
   {
      length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;

      if (length<=NET_MAX_HOSTNAME_LEN)
      {
         memcpy(&hostname[0], &data[jSMNtokens[tokNum].start], length);
      }
   }
   else
   {
      useDefaultHostName();
   }

   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv4/use ipv4");
   if (tokNum >= 0)
   {
      if (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0)
      {
         if (parseIpv4Config(data, jSMNtokens, resultCode)!=5)
            ipv4UseDefaultConfigs();
         ipEnable++;
      }

   }
#if IPV6_SUPPORT == ENABLED
   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/ipv6/use ipv6");
   if (tokNum >= 0)
   {
      if (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0)
      {
         if (parseIpv6Config(data, jSMNtokens, resultCode)!=5)
            ipv6_use_default_configs();
      }

   }
   if (ipEnable == 0)
   {
      ipv4UseDefaultConfigs();
      ipv6_use_default_configs();
   }
#endif
   return NO_ERROR;
}
error_t networkConfigure (void)
{
   error_t error;
   error = read_config("/config/lan.json",&parseIpConfig);
   return error;
}

error_t networkStart(void)
{

   error_t error; //for debugging

   xprintf("System clock: %uHz\n", SystemCoreClock);
   xprintf("**********************************\r\n");
   xprintf("*** CycloneTCP Web Server Demo ***\r\n");
   xprintf("**********************************\r\n");
   xprintf("Copyright: 2015 timurey\r\n");
   xprintf("Compiled: %s %s\r\n", __DATE__, __TIME__);
   xprintf("Target: STM32f205\r\n");
   osInitKernel();
   error = netInit();
   interface = &netInterface[0];

   //Set interface name
   netSetInterfaceName(interface, "eth0");
   //Set host name
   netSetHostname(interface, &hostname);
   //Select the relevant network adapter
   netSetDriver(interface, &enc28j60Driver);
   //Underlying SPI driver
   netSetSpiDriver(interface, &spiDriver);
   //Set external interrupt line driver
   netSetExtIntDriver(interface, &extIntDriver);

   netSetMacAddr(interface, &macAddr);
   error = netConfigInterface(interface);

   if (useDhcp == TRUE)
   {
      //Get default settings
      dhcpClientGetDefaultSettings(&dhcpClientSettings);
      //Set the network interface to be configured by DHCP
      dhcpClientSettings.interface = interface;
      //Disable rapid commit option
      dhcpClientSettings.rapidCommit = FALSE;

      //DHCP client initialization
      error = dhcpClientInit(&dhcpClientContext, &dhcpClientSettings);
      //Failed to initialize DHCP client?
      if(error)
      {
         //Debug message
         TRACE_ERROR("Failed to initialize DHCP client!\r\n");
      }

      //Start DHCP client
      error = dhcpClientStart(&dhcpClientContext);
      //Failed to start DHCP client?
      if(error)
      {
         //Debug message
         TRACE_ERROR("Failed to start DHCP client!\r\n");
      }
   }
   else
   {
      //Set IPv4 host address

      error = ipv4SetHostAddr(interface, ipv4Addr);
      if (error)
         return error;
      error = ipv4SetSubnetMask(interface, ipv4Mask);
      if (error)
         return error;
      error = ipv4SetDefaultGateway(interface, ipv4Gateway);
      if (error)
         return error;
      error = ipv4SetDnsServer(interface, 0, ipv4Dns1);
      if (error)
         return error;
      error = ipv4SetDnsServer(interface, 1, ipv4Dns2);
      if (error)
         return error;

   }
   //	autoIpGetDefaultSettings(&autoIpSettings);
   //	autoIpSettings.interface = interface;
   //	autoIpInit(&autoIpContext, &autoIpSettings);
   //	autoIpStart(&autoIpContext);

   error = dnsSdSetInstanceName(&netInterface[0], (char *)&hostname);
   if (error)
      return error;
   DnsSdServices[0].name="_http._tcp";
   DnsSdServices[0].priority=0;
   DnsSdServices[0].weight=0;
   DnsSdServices[0].port=80;
   DnsSdServices[0].info="path=/";

   DnsSdServices[1].name="_ftp._tcp";
   DnsSdServices[1].priority=0;
   DnsSdServices[1].weight=0;
   DnsSdServices[1].port=21;
   DnsSdServices[1].info="";

   error = dnsSdRegisterServices(&netInterface[0], &DnsSdServices[0], 2);

   return error;
   //
   //	dnsSdSetInstanceName(&netInterface[0], "Web Server");
}


void networkServices(void *pvParametrs)
{
   (void) pvParametrs;
   configInit();

   networkConfigure();
   sensorsConfigure();
   ntpdConfigure();
#if (FTP_SERVER_SUPPORT == ENABLED)
   ftpdConfigure();
#endif
   httpdConfigure();

   configDeinit();

   networkStart();
   ntpdStart();
#if (FTP_SERVER_SUPPORT == ENABLED)
   ftpdStart();
#endif
   httpdStart();

   vTaskDelay(1000);

   vTaskDelete(NULL);
}


