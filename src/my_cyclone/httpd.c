/*
 * httpd.c
 *
 *  Created on: 30 янв. 2015 г.
 *      Author: timurtaipov
 */

#include "httpd.h"
#include "rest/rest.h"
#include "configs.h"

//Forward declaration of functions
error_t httpServerCgiCallback(HttpConnection *connection, const char_t *param);

error_t httpServerUriNotFoundCallback(HttpConnection *connection,const char_t *uri);

//#define APP_HTTP_MAX_CONNECTIONS 1


HttpServerContext httpServerContext;
HttpConnection httpConnections[APP_HTTP_MAX_CONNECTIONS];
static 	HttpServerSettings httpServerSettings;


static void httpdUseDefaultConfig(void)
{
   //Listen to port 80
   httpServerSettings.port = HTTP_PORT;
   //Client connections
   httpServerSettings.maxConnections = APP_HTTP_MAX_CONNECTIONS;

   //Specify the server's root directory
   strcpy(httpServerSettings.rootDirectory, "/web/");
#if (HTTP_SERVER_GZIPED_FILES == ENABLED)
   strcpy(httpServerSettings.gzipedDirectory, "/gziped");
#endif
   //Set default home page
   strcpy(httpServerSettings.defaultDocument, "index.html");

}



static error_t parseHttpdConfig (char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   uint8_t httpEnable = 0;
   jsmnerr_t resultCode;
   int tokNum;
   int length;
   jsmn_init(jSMNparser);
   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   if (resultCode >0 )
   {
      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/port");
      if (tokNum >= 0)
      {
         httpServerSettings.port = atoi(&data[jSMNtokens[tokNum].start]);
         httpEnable++;

      }

      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/max connections");
      if (tokNum >= 0)
      {
         httpServerSettings.maxConnections = MIN (atoi(&data[jSMNtokens[tokNum].start]), APP_HTTP_MAX_CONNECTIONS);
         httpEnable++;

      }

      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/root directory");
      if (tokNum >= 0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<=HTTP_SERVER_ROOT_DIR_MAX_LEN)
         {
            memcpy(&httpServerSettings.rootDirectory, &data[jSMNtokens[tokNum].start], length);
            httpServerSettings.rootDirectory[length]='\0';

            httpEnable++;
         }

      }
#if (HTTP_SERVER_GZIPED_FILES == ENABLED)
      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/gziped directory");
      if (tokNum >= 0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<=HTTP_SERVER_ROOT_DIR_MAX_LEN)
         {
            memcpy(&httpServerSettings.gzipedDirectory, &data[jSMNtokens[tokNum].start], length);
            httpServerSettings.gzipedDirectory[length]='\0';

            httpEnable++;
         }

      }
#else
      httpEnable++;
#endif
      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/default document");
      if (tokNum >= 0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<=HTTP_SERVER_DEFAULT_DOC_MAX_LEN)
         {
            memcpy(&httpServerSettings.defaultDocument, &data[jSMNtokens[tokNum].start], length);
            httpServerSettings.defaultDocument[length]='\0';

            httpEnable++;
         }

      }

      if (httpEnable<5)
         httpdUseDefaultConfig();

      return NO_ERROR;
   }
   else
   {
      httpdUseDefaultConfig();
      return NO_ERROR;
   }
}




error_t httpdConfigure(void)
{
   error_t error;
   //Get default settings
   httpServerGetDefaultSettings(&httpServerSettings);
   //Bind HTTP server to the desired interface
   httpServerSettings.interface = &netInterface[0];

   httpServerSettings.connections = httpConnections;

   //Callback functions
   httpServerSettings.cgiCallback = httpServerCgiCallback;
   httpServerSettings.uriNotFoundCallback = httpServerUriNotFoundCallback;

   error = read_config("/config/httpd.json",&parseHttpdConfig);
   return error;
}

error_t httpdStart(void)
{
   error_t error;
   //HTTP server initialization
   error = httpServerInit(&httpServerContext, &httpServerSettings);
   //Failed to initialize HTTP server?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to initialize HTTP server!\r\n");
   }


   //Start HTTP server
   error = httpServerStart(&httpServerContext);
   //Failed to start HTTP server?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to start HTTP server!\r\n");
   }

   error = restInit();

   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to start REST service!\r\n");
   }
   return error;
}
/**
 * @brief CGI callback function
 * @param[in] connection Handle referencing a client connection
 * @param[in] param NULL-terminated string that contains the CGI parameter
 * @return Error code
 **/

error_t httpServerCgiCallback(HttpConnection *connection,
   const char_t *param)
{
   static uint_t pageCounter = 0;
   uint_t length;
   time_t unixTime;
   DateTime date;

   //Underlying network interface
   NetInterface *interface = connection->socket->interface;

   //Check parameter name
   if(!strcasecmp(param, "PAGE_COUNTER"))
   {
      pageCounter++;
      sprintf(connection->buffer, "%u time%s", pageCounter, (pageCounter >= 2) ? "s" : "");
   }
   else if(!strcasecmp(param, "BOARD_NAME"))
   {
      strcpy(connection->buffer, interface->hostname);
   }
   else if(!strcasecmp(param, "SYSTEM_TIME"))
   {
      memset(&connection->buffer, 0x00, HTTP_SERVER_BUFFER_SIZE );
      //		unixTime = RTC_GetCounter();
      unixTime = 0;
      //Convert Unix timestamp to date
      convertUnixTimeToDate(unixTime, &date);
      formatDate(&date, connection->buffer);

   }
   else if(!strcasecmp(param, "MAC_ADDR"))
   {
      macAddrToString(&interface->macAddr, connection->buffer);
   }
   else if(!strcasecmp(param, "IPV4_ADDR"))
   {
      ipv4AddrToString(interface->ipv4Config.addr, connection->buffer);
   }
   else if(!strcasecmp(param, "SUBNET_MASK"))
   {
      ipv4AddrToString(interface->ipv4Config.subnetMask, connection->buffer);
   }
   else if(!strcasecmp(param, "DEFAULT_GATEWAY"))
   {
      ipv4AddrToString(interface->ipv4Config.defaultGateway, connection->buffer);
   }
   else if(!strcasecmp(param, "IPV4_PRIMARY_DNS"))
   {
      ipv4AddrToString(interface->ipv4Config.dnsServer[0], connection->buffer);
   }
   else if(!strcasecmp(param, "IPV4_SECONDARY_DNS"))
   {
      ipv4AddrToString(interface->ipv4Config.dnsServer[1], connection->buffer);
   }
#if (IPV6_SUPPORT == ENABLED)
   else if(!strcasecmp(param, "LINK_LOCAL_ADDR"))
   {
      ipv6AddrToString(&interface->ipv6Config.linkLocalAddr, connection->buffer);
   }
   else if(!strcasecmp(param, "GLOBAL_ADDR"))
   {
      ipv6AddrToString(&interface->ipv6Config.globalAddr, connection->buffer);
   }
   else if(!strcasecmp(param, "IPV6_PREFIX"))
   {
      ipv6AddrToString(&interface->ipv6Config.prefix, connection->buffer);
      length = strlen(connection->buffer);
      sprintf(connection->buffer + length, "/%u", interface->ipv6Config.prefixLength);
   }
   else if(!strcasecmp(param, "ROUTER"))
   {
      ipv6AddrToString(&interface->ipv6Config.router, connection->buffer);
   }
   else if(!strcasecmp(param, "IPV6_PRIMARY_DNS"))
   {
      ipv6AddrToString(&interface->ipv6Config.dnsServer[0], connection->buffer);
   }
   else if(!strcasecmp(param, "IPV6_SECONDARY_DNS"))
   {
      ipv6AddrToString(&interface->ipv6Config.dnsServer[1], connection->buffer);
   }
#endif
   else
   {
      return ERROR_INVALID_TAG;
   }

   //Get the length of the resulting string
   length = strlen(connection->buffer);

   //Send the contents of the specified environment variable
   return httpWriteStream(connection, connection->buffer, length);
}


/**
 * @brief URI not found callback
 * @param[in] connection Handle referencing a client connection
 * @param[in] uri NULL-terminated string containing the path to the requested resource
 * @return Error code
 **/

error_t httpServerUriNotFoundCallback(HttpConnection *connection,	const char_t *uri)
{
   error_t error;
   error = restTry (connection, uri);

   return error;
   //	return ERROR_NOT_FOUND;
}
