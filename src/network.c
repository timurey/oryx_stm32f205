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
#include "rest/clock.h"
#include "uuid.h"

#include "rest/sensors.h"
#include "rest/executor.h"
#include "rest/variables.h"

#include "expression_parser/logic.h"
#include "rest/rest.h"
static NetInterface *interface;

register_defalt_config( "{\"ipv4\":{\"useipv4\":true,\"usedhcp\":true,\"address\":\"192.168.1.211\",\"netmask\":\"255.255.255.0\",\"gateway\":\"192.168.1.1\",\"primarydns\":\"192.168.1.1\",\"secondarydns\":\"8.8.8.8\"},\"ipv6\":{\"useipv6\":false,\"useslaac\":false,\"linklocaladdress\":\"fe80::107\",\"ipv6prefix\":\"2001:db8::\",\"ipv6prefixlength\":64,\"ipv6globaladdress\":\"2001:db8::107\",\"ipv6router\":\"fe80::1\",\"ipv6primarydns\":\"2001:4860:4860::8888\",\"ipv6secondarydns\":\"2001:4860:486\"}}");


#if (AUTO_IP_SUPPORT == ENABLED)
AutoIpSettings autoIpSettings;
AutoIpContext autoIpContext;
#endif

DhcpClientSettings dhcpClientSettings;
DhcpClientCtx dhcpClientContext;

#if IPV6_SUPPORT == ENABLED

static Ipv6Addr ipv6Addr;

#if (SLAAC_SUPPORT == ENABLED)
SlaacSettings slaacSettings;
SlaacContext slaacContext;
#endif //SLAAC_SUPPORT

#endif //IPV6_SUPPORT

typedef struct
{
   bool_t useDhcp;
   bool_t useIpV4;
   bool_t useIpV6;
   bool_t needSave;
   Ipv4Addr ipv4Addr;
   Ipv4Addr ipv4Mask;
   Ipv4Addr ipv4Gateway;
   Ipv4Addr ipv4Dns1;
   Ipv4Addr ipv4Dns2;
   char sMacAddr[APP_MAC_ADDR_LEN];
   MacAddr macAddr;
   char hostname[NET_MAX_HOSTNAME_LEN];

} networkContext_t;

static networkContext_t networkContext;

static void networkSaveConfig (char * bufer, size_t maxLen);
static error_t parseNetwork(jsmnParserStruct * jsonParser);
register_rest_function(network, "/network", NULL, NULL, &restGetNetwork, NULL, &restPutNetwork, NULL);



#if (DNS_SD_SUPPORT == ENABLED)
DnsSdContext dnsSdContext;
DnsSdSettings dnsSdSettings;
#endif

#if (MDNS_CLIENT_SUPPORT == ENABLED | MDNS_RESPONDER_SUPPORT == ENABLED)
MdnsResponderContext mdnsResponderContext;
MdnsResponderSettings mdnsResponderSettings;
#endif

static error_t getNetworkContext (NetInterface *_interface, networkContext_t * context)
{
   ipv4GetHostAddr(_interface, &context->ipv4Addr);
   ipv4GetSubnetMask(_interface, &context->ipv4Mask);
   ipv4GetDefaultGateway(_interface, &context->ipv4Gateway);
   ipv4GetDnsServer(_interface, 0, &context->ipv4Dns1);
   ipv4GetDnsServer(_interface, 1, &context->ipv4Dns2);
   return NO_ERROR;
}
error_t restGetNetwork(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;
   int p=0;
   (void) RestApi;
   getNetworkContext(interface, &networkContext );
   if (RestApi->restVersion == 1)
   {
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"{\"ipv4\":{\"useipv4\":%s,\"usedhcp\":%s,", (networkContext.useIpV4 == TRUE)?"true":"false", (networkContext.useDhcp == TRUE)?"true":"false");
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"address\":\"%s\",",ipv4AddrToString(networkContext.ipv4Addr, NULL));
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"netmask\":\"%s\",",ipv4AddrToString(networkContext.ipv4Mask, NULL));
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"gateway\":\"%s\",",ipv4AddrToString(networkContext.ipv4Gateway, NULL));
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"primarydns\":\"%s\",",ipv4AddrToString(networkContext.ipv4Dns1, NULL));
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"secondarydns\":\"%s\"}}",ipv4AddrToString(networkContext.ipv4Dns2, NULL));

      connection->response.contentType = mimeGetType(".json");
   }
   else if (RestApi->restVersion == 2)
   {
      p+=sprintf(restBuffer+p,"{\"data\":{\"type\":\"network\", \"id\":0,\"attributes\":{");
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"useipv4\":%s,\"usedhcp\":%s,", (networkContext.useIpV4 == TRUE)?"true":"false", (networkContext.useDhcp == TRUE)?"true":"false");
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"address\":\"%s\",",ipv4AddrToString(networkContext.ipv4Addr, NULL));
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"netmask\":\"%s\",",ipv4AddrToString(networkContext.ipv4Mask, NULL));
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"gateway\":\"%s\",",ipv4AddrToString(networkContext.ipv4Gateway, NULL));
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"primarydns\":\"%s\",",ipv4AddrToString(networkContext.ipv4Dns1, NULL));
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"secondarydns\":\"%s\"}}}",ipv4AddrToString(networkContext.ipv4Dns2, NULL));
      connection->response.contentType = mimeGetType(".apijson");
   }
   connection->response.noCache = TRUE;

   error=rest_200_ok(connection, &restBuffer[0]);
   //Any error to report?

   return error;
}


error_t restPutNetwork(HttpConnection *connection, RestApi_t* RestApi)
{
   jsmn_parser parser;
   jsmntok_t tokens[32]; // a number >= total number of tokens

   jsmnParserStruct jsonParser;

   size_t received;
   error_t error = NO_ERROR;

   (void) RestApi;
   error = httpReadStream(connection, connection->buffer, connection->request.contentLength, &received, HTTP_FLAG_BREAK_CRLF);
   if (error) return error;
   if (received == connection->request.contentLength)
   {
      connection->buffer[received] = '\0';
   }

   jsonParser.jSMNparser = &parser;
   jsonParser.jSMNtokens = &tokens[0];
   jsonParser.numOfTokens = arraysize(tokens);
   jsonParser.data = connection->buffer;
   jsonParser.lengthOfData = connection->request.contentLength;

   error = parseNetwork(&jsonParser);

   if (networkContext.needSave == TRUE)
   {
      networkSaveConfig(connection->buffer, HTTP_SERVER_BUFFER_SIZE);
      networkContext.needSave = FALSE;
      //      networkRestart();
   }
   error=restGetNetwork(connection, RestApi);
   //Any error to report?
   return error;
}

static error_t useDefaultMacAddress(void)
{
   error_t error;
   sprintf(networkContext.sMacAddr, "00-50-C2-%X-%X-%X",uuid->b[10],uuid->b[9],uuid->b[8]);
   error = macStringToAddr(&(networkContext.sMacAddr[0]), &networkContext.macAddr);
   return error;
}
static error_t useDefaultHostName(void)
{
   sprintf(networkContext.hostname, "aurora_%X-%X-%X",uuid->b[10],uuid->b[9],uuid->b[8]);
   return NO_ERROR;
}


static uint8_t parseIpv4Config(jsmnParserStruct * jsonParser)
{
   error_t error;
#define IP_MAX_LEN 16 //Includeing '\0' at end
   char ipAddr[] = {"xxx.xxx.xxx.xxx\0"};
   char ipMask[] = {"xxx.xxx.xxx.xxx\0"};
   char ipGateway[] = {"xxx.xxx.xxx.xxx\0"};
   char ipDns1[] = {"xxx.xxx.xxx.xxx\0"};
   char ipDns2[] = {"xxx.xxx.xxx.xxx\0"};
   static Ipv4Addr testIpAddres;
   int length;
#if (IPV4_SUPPORT == ENABLED)

   jsmn_get_bool(jsonParser, "$.ipv4.usedhcp", &networkContext.useDhcp);


   length = jsmn_get_string(jsonParser, "$.ipv4.address", &ipAddr[0], IP_MAX_LEN);
   if (length ==0 || length > IP_MAX_LEN)
   {
      xprintf("Warning: wrong ip address in config file \"/config/lan.json\" ");
   }
   length = jsmn_get_string(jsonParser, "$.ipv4.netmask", &ipMask[0], IP_MAX_LEN);
   if (length ==0 || length > IP_MAX_LEN)
   {
      xprintf("Warning: wrong subnet mask in config file \"/lan.json\" ");
   }

   length = jsmn_get_string(jsonParser, "$.ipv4.gateway", &ipGateway[0], IP_MAX_LEN);
   if (length ==0 || length > IP_MAX_LEN)
   {
      xprintf("Warning: wrong default gateway in config file \"/config/lan.json\" ");
   }

   length = jsmn_get_string(jsonParser, "$.ipv4.primarydns", &ipDns1[0], IP_MAX_LEN);
   if (length ==0 || length > IP_MAX_LEN)
   {
      xprintf("Warning: wrong primary dns in config file \"/config/lan.json\" ");
   }

   length = jsmn_get_string(jsonParser, "$.ipv4.secondarydns", &ipDns2[0], IP_MAX_LEN);

   if (length ==0 || length > IP_MAX_LEN)
   {
      xprintf("Warning: wrong primary dns in config file \"/config/lan.json\" ");
   }

#endif
   //Check addresses
   error = ipv4StringToAddr(&ipAddr[0], &testIpAddres);
   if (error == NO_ERROR)
      error = ipv4StringToAddr(&ipMask[0], &testIpAddres);
   if (!error == NO_ERROR)
      error = ipv4StringToAddr(&ipGateway[0], &testIpAddres);

   if (error == NO_ERROR)
   {
      ipv4StringToAddr(&ipAddr[0], &networkContext.ipv4Addr);
      ipv4StringToAddr(&ipMask[0], &networkContext.ipv4Mask);
      ipv4StringToAddr(&ipGateway[0], &networkContext.ipv4Gateway);
   }
   ipv4StringToAddr(&ipDns1[0], &networkContext.ipv4Dns1);
   ipv4StringToAddr(&ipDns2[0], &networkContext.ipv4Dns2);


   return error;
}


inline void networkSaveConfig (char * bufer, size_t maxLen)
{
   int p=0;
   getNetworkContext(interface, &networkContext );
   p+=snprintf(bufer+p, maxLen-p,"{\"ipv4\":{\"useipv4\":%s,\"usedhcp\":%s,", (networkContext.useIpV4 == TRUE)?"true":"false", (networkContext.useDhcp == TRUE)?"true":"false");
   p+=snprintf(bufer+p, maxLen-p,"\"address\":\"%s\",",ipv4AddrToString(networkContext.ipv4Addr, NULL));
   p+=snprintf(bufer+p, maxLen-p,"\"netmask\":\"%s\",",ipv4AddrToString(networkContext.ipv4Mask, NULL));
   p+=snprintf(bufer+p, maxLen-p,"\"gateway\":\"%s\",",ipv4AddrToString(networkContext.ipv4Gateway, NULL));
   p+=snprintf(bufer+p, maxLen-p,"\"primarydns\":\"%s\",",ipv4AddrToString(networkContext.ipv4Dns1, NULL));
   p+=snprintf(bufer+p, maxLen-p,"\"secondarydns\":\"%s\"}}",ipv4AddrToString(networkContext.ipv4Dns2, NULL));

   save_config("/config/network.json","%S", bufer);

}

static error_t parseNetwork(jsmnParserStruct * jsonParser)
{
   int strLen;
   jsmn_init(jsonParser->jSMNparser);
   networkContext.needSave = FALSE;
   //   networkContext.useIpV4 = FALSE;
   //   networkContext.useIpV6 = FALSE;
   //   networkContext.useDhcp = FALSE;
   jsonParser->resultCode = xjsmn_parse(jsonParser);

   strLen = jsmn_get_string(jsonParser, "$.mac", &networkContext.sMacAddr[0], APP_MAC_ADDR_LEN);
   if (strLen == 0)
   {
      useDefaultMacAddress();
   }

   strLen = jsmn_get_string(jsonParser, "$.hostname", &networkContext.hostname[0], NET_MAX_HOSTNAME_LEN);
   if (strLen == 0)
   {
      useDefaultHostName();
   }

   jsmn_get_bool(jsonParser, "$.ipv4.useipv4", &networkContext.useIpV4);
   if (networkContext.useIpV4 == TRUE)
   {
      parseIpv4Config(jsonParser);
   }
#if IPV6_SUPPORT == ENABLED
   networkContext.useIpV6 = jsmn_get_bool(data, jSMNtokens, resultCode, "$.ipv6.useipv6");
   if (networkContext.useIpV6 == TRUE)
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

   jsmn_get_bool(jsonParser, "$.needSave", &networkContext.needSave);

   return NO_ERROR;
}

error_t networkConfigure (void)
{
   error_t error;
   error = read_default(&defaultConfig, &parseNetwork);

   error = read_config("/config/lan.json",&parseNetwork);
   return error;
}

error_t networkStart(void)
{

   error_t error; //for debugging
   char name[NET_MAX_HOSTNAME_LEN + 9];
   char *aurora = "aurora - ";
   char *pHostname = &(networkContext.hostname[0]);
   char *pName  = &name[0];

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
   netSetHostname(interface, pHostname);
   //Select the relevant network adapter
   netSetDriver(interface, &enc28j60Driver);
   //Underlying SPI driver
   netSetSpiDriver(interface, &spiDriver);
   //Set external interrupt line driver
   netSetExtIntDriver(interface, &extIntDriver);

   netSetMacAddr(interface, &networkContext.macAddr);
   error = netConfigInterface(interface);

   if (networkContext.useDhcp == TRUE)
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

      error = ipv4SetHostAddr(interface, networkContext.ipv4Addr);
      if (error)
         return error;
      error = ipv4SetSubnetMask(interface, networkContext.ipv4Mask);
      if (error)
         return error;
      error = ipv4SetDefaultGateway(interface, networkContext.ipv4Gateway);
      if (error)
         return error;
      error = ipv4SetDnsServer(interface, 0, networkContext.ipv4Dns1);
      if (error)
         return error;
      error = ipv4SetDnsServer(interface, 1, networkContext.ipv4Dns2);
      if (error)
         return error;

   }
   //	autoIpGetDefaultSettings(&autoIpSettings);
   //	autoIpSettings.interface = interface;
   //	autoIpInit(&autoIpContext, &autoIpSettings);
   //	autoIpStart(&autoIpContext);


#if (MDNS_RESPONDER_SUPPORT == ENABLED)
   //Get default settings
   mdnsResponderGetDefaultSettings(&mdnsResponderSettings);
   //Underlying network interface
   mdnsResponderSettings.interface = &netInterface[0];

   //mDNS responder initialization
   error = mdnsResponderInit(&mdnsResponderContext, &mdnsResponderSettings);
   //Failed to initialize mDNS responder?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to initialize mDNS responder!\r\n");
   }

   //Set mDNS hostname
   error = mdnsResponderSetHostname(&mdnsResponderContext, pHostname);
   //Any error to report?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to set hostname!\r\n");
   }

   //Start mDNS responder
   error = mdnsResponderStart(&mdnsResponderContext);
   //Failed to start mDNS responder?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to start mDNS responder!\r\n");
   }
#endif
#if (DNS_SD_SUPPORT == ENABLED)
   //Get default settings
   dnsSdGetDefaultSettings(&dnsSdSettings);
   //Underlying network interface
   dnsSdSettings.interface = &netInterface[0];

   //DNS-SD initialization
   error = dnsSdInit(&dnsSdContext, &dnsSdSettings);
   //Failed to initialize DNS-SD?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to initialize DNS-SD!\r\n");
   }

   //Unregister service
   error = dnsSdUnregisterService(&dnsSdContext, "_http._tcp");
   error = dnsSdUnregisterService(&dnsSdContext, "_ftp._tcp");

   strcpy(name, aurora);
   strcat(name, pHostname);

   //Set instance name
   error = dnsSdSetInstanceName(&dnsSdContext, pName);

   //Any error to report?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to set instance name!\r\n");
   }
   //Register DNS-SD service
   error = dnsSdRegisterService(&dnsSdContext,
      "_http._tcp",
      0,
      0,
      80,
      "txtvers=1;"
      "path=/");

   error = dnsSdRegisterService(&dnsSdContext,
      "_ftp._tcp",
      0,
      0,
      21,
      "");
   //   error = dnsSdRegisterService(&dnsSdContext,
   //      "_hap._tcp",
   //      0,
   //      0,
   //      80,
   //      "c#=5;"
   //      "ff=0x1;"
   //      "id=00:11:22:33:44:55;"
   //      "md=AcmeFan;"
   //      "pv=1.0;"
   //      "s#=1;"
   //      "sf=0x0;"
   //      "ci=5");
   //Start DNS-SD
   error = dnsSdStart(&dnsSdContext);
   //Failed to start DNS-SD?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to start DNS-SD!\r\n");
   }
#endif
   return NO_ERROR;
}


void networkServices(void *pvParametrs)
{
   (void) pvParametrs;
   configInit();
   sensorsConfigure();

   networkConfigure();

//   executorsConfigure();
   ntpdConfigure();
   clockConfigure();

   logicConfigure();
#if (FTP_SERVER_SUPPORT == ENABLED)
   ftpdConfigure();
#endif
   httpdConfigure();

   configDeinit();

   networkStart();
   ntpdStart();
   logicStart();
#if (FTP_SERVER_SUPPORT == ENABLED)
   ftpdStart();
#endif
   httpdStart();

   //   vTaskDelay(1000);

   vTaskDelete(NULL);
}

#if (IPV6_SUPPORT == ENABLED)

static uint8_t parseIpv6Config(char *data, jsmntok_t *jSMNtokens, int resultCode)
{
   error_t error;
   int tokNum;
   uint8_t correct = 0;
   char ipString[] = "255.255.255.255\0";
   int length;


   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "$.ipv6.use slaac");
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
   // else
   // {
   //    tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/ipv4/ipv4 address");
   //    if (tokNum >= 0)
   //    {
   //       length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //       if (length<=15)
   //       {
   //          memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //          ipString[length]='\0';
   //          //Set IPv4 host address
   //          error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //          if (error)
   //             return correct;
   //          error = ipv4SetHostAddr(interface, ipv4Addr);
   //          if (error)
   //             return correct;
   //          correct++;
   //       }
   //       else
   //       {
   //          xprintf("Warning: wrong ip address in config file \"/config/lan.json\" ");
   //          return correct;
   //       }
   //    }
   //    tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/ipv4/ipv4 netmask");
   //    if (tokNum >= 0)
   //    {
   //       length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //       if (length<=15)
   //       {
   //          memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //          ipString[length]='\0';
   //          //Set subnet mask
   //          error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //          if (error)
   //             return correct;
   //          error = ipv4SetSubnetMask(interface, ipv4Addr);
   //          if (error)
   //             return correct;
   //          correct++;
   //       }
   //       else
   //       {
   //          xprintf("Warning: wrong subnet mask in config file \"/config/lan.json\" ");
   //          return correct;
   //       }
   //    }
   //
   //    tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/ipv4/ipv4 gateway");
   //    if (tokNum >= 0)
   //    {
   //       length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //       if (length<=15)
   //       {
   //          memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //          ipString[length]='\0';
   //          //Set default gateway
   //          error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //          if (error)
   //             return correct;
   //          error = ipv4SetDefaultGateway(interface, ipv4Addr);
   //          if (error)
   //             return correct;
   //          correct++;
   //       }
   //       else
   //       {
   //          xprintf("Warning: wrong default gateway in config file \"/config/lan.json\" ");
   //          return correct;
   //       }
   //    }
   //    tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/ipv4/ipv4 primary dns");
   //    if (tokNum >= 0)
   //    {
   //       length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //       if (length<=15)
   //       {
   //          memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //          ipString[length]='\0';
   //          //Set primary DNS server
   //          error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //          if (error)
   //             return correct;
   //          error = ipv4SetDnsServer(interface, 0, ipv4Addr);
   //          if (error)
   //             return correct;
   //          correct++;
   //       }
   //       else
   //       {
   //          xprintf("Warning: wrong primary dns in config file \"/config/lan.json\" ");
   //          return correct;
   //       }
   //    }
   //    tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/ipv4/ipv4 secondary dns");
   //    if (tokNum >= 0)
   //    {
   //       length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
   //       if (length<=15)
   //       {
   //          memcpy(&ipString[0], &data[jSMNtokens[tokNum].start], length);
   //          ipString[length]='\0';
   //          //Set secondary DNS server
   //          error = ipv4StringToAddr(&ipString[0], &ipv4Addr);
   //          if (error)
   //             return correct;
   //          error = ipv4SetDnsServer(interface, 1, ipv4Addr);
   //          if (error)
   //             return correct;
   //          correct++;
   //       }
   //       else
   //       {
   //          xprintf("Warning: wrong primary dns in config file \"/config/lan.json\" ");
   //          return correct;
   //       }
   //    }
   //
   //

   return correct;
   // }
}
#endif

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
   // //Set link-local address
   // ipv6StringToAddr(APP_IPV6_LINK_LOCAL_ADDR, &ipv6Addr);
   // ipv6SetLinkLocalAddr(interface, &ipv6Addr);
   //
   // //Set IPv6 prefix
   // ipv6StringToAddr(APP_IPV6_PREFIX, &ipv6Addr);
   // ipv6SetPrefix(interface, &ipv6Addr, APP_IPV6_PREFIX_LENGTH);
   //
   // //Set global address
   // ipv6StringToAddr(APP_IPV6_GLOBAL_ADDR, &ipv6Addr);
   // ipv6SetGlobalAddr(interface, &ipv6Addr);
   //
   // //Set router
   // ipv6StringToAddr(APP_IPV6_ROUTER, &ipv6Addr);
   // ipv6SetRouter(interface, &ipv6Addr);
   //
   // //Set primary and secondary DNS servers
   // ipv6StringToAddr(APP_IPV6_PRIMARY_DNS, &ipv6Addr);
   // ipv6SetDnsServer(interface, 0, &ipv6Addr);
   // ipv6StringToAddr(APP_IPV6_SECONDARY_DNS, &ipv6Addr);
   // ipv6SetDnsServer(interface, 1, &ipv6Addr);
}
#endif
