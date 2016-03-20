/*
 * sntp.c
 *
 *  Created on: 30 янв. 2015 г.
 *      Author: timurtaipov
 */
#define TRACE_LEVEL SNTP_TRACE_LEVEL

#include "sntpd.h"
#include "os_port.h"
#include "FreeRTOS.h"
#include "task.h"
#include "core/net.h"
#include "sntp/sntp_client.h"
#include "debug.h"
#include "date_time.h"
#include "rtc.h"
#include "configs.h"
#include <stdio.h>
#include <string.h>
/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include <ctype.h>
#include <stdlib.h>


#define HOURS(a) (a*60*60)
#define MINUTES(a) (a*60)
#define ISDIGIT(a) (((a)>='0') && ((a)<='9'))
#define __ctype_lookup(__c) ((__ctype_ptr__+sizeof(""[__c]))[(int)(__c)])

#define  isalpha(__c)   (__ctype_lookup(__c)&(_U|_L))
#define  isupper(__c)   ((__ctype_lookup(__c)&(_U|_L))==_U)
#define  islower(__c)   ((__ctype_lookup(__c)&(_U|_L))==_L)
#define  isdigit(__c)   (__ctype_lookup(__c)&_N)
#define  isxdigit(__c)  (__ctype_lookup(__c)&(_X|_N))
#define  isspace(__c)   (__ctype_lookup(__c)&_S)
#define ispunct(__c) (__ctype_lookup(__c)&_P)
#define isalnum(__c) (__ctype_lookup(__c)&(_U|_L|_N))
#define isprint(__c) (__ctype_lookup(__c)&(_P|_U|_L|_N|_B))
#define  isgraph(__c)   (__ctype_lookup(__c)&(_P|_U|_L|_N))
#define iscntrl(__c) (__ctype_lookup(__c)&_C)

OsTask *vNtpTask;
struct
{
   char ntp_candidates[NUM_OF_NTP_SERVERS][MAX_LENGTH_OF_NTP_SERVER_NAME];
   int servers;
   uint32_t period;
   bool_t enabled;
   bool_t needSave;
} ntpContex;


static BaseType_t prvTaskNtpdCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

static void sntpClientTread(void *pvParametrs);

static const CLI_Command_Definition_t xTaskNtpd =
{
   "ntpd",
   "\r\nntpd <...>:\r\n start/stop ntp daemon\r\n\r\n",
   prvTaskNtpdCommand, /* The function to run. */
   -1 /* The user can enter no limited commands. */
};

static BaseType_t prvTaskNtpdCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
   const char *pcParameter;
   BaseType_t lParameterStringLength, xReturn;
   static BaseType_t lParameterNumber = 1;
   int pointer=0;
   /* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
   ( void ) pcCommandString;
   ( void ) xWriteBufferLen;
   configASSERT( pcWriteBuffer );


   /* Obtain the parameter string. */
   pcParameter = FreeRTOS_CLIGetParameter
      (
         pcCommandString,		/* The command string itself. */
         lParameterNumber,		/* Return the next parameter. */
         &lParameterStringLength	/* Store the parameter string length. */
      );

   if( pcParameter != NULL )
   {
      /* Return the parameter string. */
      memset( pcWriteBuffer, 0x00, xWriteBufferLen );
      if (strcmp (pcParameter, "start")==0)
      {
         pointer+=sprintf(pcWriteBuffer+pointer, "\r\nntpd: Starting...");

         if (vNtpTask == NULL)
         {
            ntpdStart();
            if (vNtpTask != NULL)
            {
               pointer+=sprintf(pcWriteBuffer+pointer, " OK.\r\n");
            }
         }
         else
         {
            pointer+=sprintf(pcWriteBuffer+pointer, " Failed.\r\n");
         }

         if (vNtpTask == NULL)
         {
            pointer+=sprintf(pcWriteBuffer+pointer, " Failed.\r\n");
         }

         /* No more data to return. */
         xReturn = pdFALSE;
      }

      if (strcmp (pcParameter, "stop")==0)
      {
         pointer+=sprintf(pcWriteBuffer+pointer, "\r\nntpd: Stoping...");
         if (vNtpTask != NULL)
         {
            ntpdStop();
            pointer+=sprintf(pcWriteBuffer+pointer, " OK.\r\n");
         }
         else
         {
            pointer+=sprintf(pcWriteBuffer+pointer, " ntpd is not running.\r\n");
         }
         /* No more data to return. */
         xReturn = pdFALSE;
      }

   }
   else
   {
      /* No more parameters were found.  Make sure the write buffer does
			not contain a valid string. */
      memset( pcWriteBuffer, 0x00, xWriteBufferLen );
      sprintf(pcWriteBuffer,"%s",xTaskNtpd.pcHelpString);
      /* No more data to return. */
      xReturn = pdFALSE;

      /* Start over the next time this command is executed. */
      lParameterNumber = 1;
   }

   return xReturn;
}
static void v_NtpNextServer(char **cServerName, int iServerNum)
{
   *cServerName = &ntpContex.ntp_candidates[iServerNum][0];

}

static void sntpClientTread(void *pvParametrs)
{

   (void) pvParametrs;
   error_t error;
   static time_t unixTime;
   static IpAddr ipAddr;
   static NtpTimestamp timestamp;
   static DateTime date;
   static int i_NtpServerCandidate=0, i_NtpQueryCount=0;
   static char *cNtpServerName;
   //Start after 3 seconds
   vTaskDelay(3000);
   while (1)
   {
      v_NtpNextServer(&cNtpServerName, i_NtpServerCandidate);

      for (i_NtpQueryCount=0; i_NtpQueryCount<NTP_MAX_QUERY; i_NtpQueryCount++)
      {
         //Debug message
         //			TRACE_INFO("\r\n\r\nResolving server name...\r\n");
         //Resolve SNTP server name
         error = getHostByName(NULL, cNtpServerName, &ipAddr, 0);

         //Any error to report?
         if(error)
         {
            //Debug message
            TRACE_INFO("Failed to resolve server name! - %s\r\nTrying after 3 seconds\r\n", cNtpServerName);
            //Try after 3 seconds
            vTaskDelay(3000);
            continue;
         }

         //Debug message
         TRACE_INFO("Requesting time from SNTP server %s\r\n", ipAddrToString(&ipAddr, NULL));
         //Retrieve current time from NTP server using SNTP protocol
         error = sntpClientGetTimestamp(NULL, &ipAddr, &timestamp);

         //Any error to report?
         if(error)
         {
            //Debug message
            TRACE_INFO("Failed to retrieve NTP timestamp!\r\n\r\nTrying after 60 seconds\r\n");
            //Try after 60 seconds
            vTaskDelay(60000);
            continue;
         }
         else
         {

            //Unix time starts on January 1st, 1970
            unixTime = timestamp.seconds - 2208988800+RTC_GetTimezone();

            //Set counter in RTC in localtime
            RTC_CalendarConfig(unixTime);

            //Convert Unix timestamp to date
            convertUnixTimeToDate(unixTime, &date);

            //Debug message
            TRACE_INFO("Current date/time: %s\r\nNext sync after %"PRIu32" seconds\r\n\r\n", formatDate(&date, NULL), ntpContex.period);

            //Next sync after some period
            vTaskDelay(1000 * ntpContex.period);
            continue;
         }
      }
      if ((++i_NtpServerCandidate)>=NUM_OF_NTP_SERVERS)
      {
         i_NtpServerCandidate=0;
      }
   }
}
void ntpdStart(void)
{
   if (vNtpTask == NULL)
   {
      vNtpTask = osCreateTask("SNTP_client", sntpClientTread, NULL, 196, 1);
   }
}
void ntpdStop(void)
{
   if (vNtpTask != NULL)
   {
      osDeleteTask(vNtpTask);
      vNtpTask = NULL;
   }
}

inline void ntpdRestart(void)
{
   ntpdStop();
   ntpdStart();
}

static void ntp_use_default_servers (void)
{
   xprintf("Warning: incorrect config file \"/config/ntp.json\". Use default servers.");
#if (NUM_OF_NTP_SERVERS>0)
   memcpy(&ntpContex.ntp_candidates[0][0], "0.openwrt.pool.ntp.org\0", 23);
   ntpContex.servers=1;
#if (NUM_OF_NTP_SERVERS>1)
   memcpy(&ntpContex.ntp_candidates[1][0], "1.openwrt.pool.ntp.org\0", 23);
   ntpContex.servers=2;
#if (NUM_OF_NTP_SERVERS>2)
   memcpy(&ntpContex.ntp_candidates[2][0], "2.openwrt.pool.ntp.org\0", 23);
   ntpContex.servers=3;
#if (NUM_OF_NTP_SERVERS>3)
   memcpy(&ntpContex.ntp_candidates[3][0], "3.openwrt.pool.ntp.org\0", 23);
   ntpContex.servers=4;
#endif
#endif
#endif
#endif
}

static void ntp_use_default_timezone(void)
{
   xprintf("Warning: incorrect config file \"/config/ntp.json\". Use default timezone");
   RTC_SetTimezone("GMT+0500");
}

static error_t parseNTP (char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   jsmnerr_t resultCode;
   int tokNum;
   jsmn_init(jSMNparser);
   int i;
   int length;
   uint32_t period = 0;
   char tz[TIMEZONE_LENGTH];
   error_t error = NO_ERROR;

   ntpContex.needSave = FALSE;
   ntpContex.enabled = TRUE;

   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);


   if(resultCode)
   {
      ntpContex.servers = 0;

      for (i=0;i<NUM_OF_NTP_SERVERS;i++)
      {
         tokNum = jsmn_get_array_value(data, jSMNtokens, resultCode, "/config/servers", i);
         if ((!i) && (tokNum<1))
         {
            error = ERROR_NOT_CONFIGURED;  // If it is no data in array
            break;
         }
         /*
          * todo: проверить с 5 и более серверами в конфиге
          */
         if (tokNum>0)
         {
            length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
            if (length<MAX_LENGTH_OF_NTP_SERVER_NAME)
            {
               memcpy(&ntpContex.ntp_candidates[i][0], &data[jSMNtokens[tokNum].start], length);
               ntpContex.ntp_candidates[i][length]='\0';
               ntpContex.servers++;
            }
            else
            {
               ntpContex.ntp_candidates[i][0]='\0';
               xprintf("Warning: Name of ntp server № %D in config file \"/config/ntp.json\" is too long. Skipping", i);
            }
         }
      }
   }

   if ((ntpContex.servers == 0) || (error))
   {
      ntp_use_default_servers();
   }

   error = NO_ERROR;

   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/timezone");

   if (tokNum)
   {
      length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
      if (length < TIMEZONE_LENGTH)
      {
         memcpy(&tz[0], &data[jSMNtokens[tokNum].start], length);
         tz[length] = '\0';
         error = RTC_SetTimezone(&tz[0]);
      }
      else
      {
         error = ERROR_QUERY_STRING_TOO_LONG;
      }
   }
   else
   {
      error = ERROR_NOT_CONFIGURED;
   }

   if (error)
   {
      ntp_use_default_timezone();
   }

   error = NO_ERROR;

   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/period");

   if (tokNum)
   {
      ntpContex.period = HOURS(2);

      if (ISDIGIT(data[jSMNtokens[tokNum].start] ))
      {
         period = atoi(&data[jSMNtokens[tokNum].start] );
         if ((period > 0) && (period<= HOURS(24)))
         {
            ntpContex.period = period;
         }
         else
         {
            error = ERROR_NOT_CONFIGURED;
         }
      }
      else
      {
         error = ERROR_NOT_CONFIGURED;
      }
   }
   else
   {
      error = ERROR_NOT_CONFIGURED;
   }

   if (error)
   {
      ntpContex.period = HOURS(24);
   }

   error = NO_ERROR;
   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/save");
   if (tokNum >= 0)
   {
      if (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0)
      {
         ntpContex.needSave = TRUE;
      }
   }

   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/enabled");
   if (tokNum >= 0)
   {
      if (strncmp (&data[jSMNtokens[tokNum].start], "false" ,5) == 0)
      {
         ntpContex.enabled = FALSE;
      }
   }
   else
   {

   }
   return NO_ERROR;
}

static void ntpdSaveConfig (void)
{
   save_config("/config/ntp.json","{\"config\":{\"servers\":[\"%s\",\"%s\",\"%s\",\"%s\"],\"timezone\":\"%s\"}}\r\n",
      &(ntpContex.ntp_candidates[0][0]),&ntpContex.ntp_candidates[1][0],&ntpContex.ntp_candidates[2][0],&ntpContex.ntp_candidates[3][0],
      pRTC_GetTimezone());
}
void ntpdConfigure(void)
{
   error_t error;
   error = read_config("/config/ntp.json",&parseNTP);
   if (error)
   {
      ntp_use_default_servers();
      ntp_use_default_timezone();
      ntpContex.period = HOURS(2);
      ntpContex.needSave = FALSE;
      ntpContex.enabled = TRUE;
   }
}
register_rest_function(ntp,"/ntp", NULL, NULL, &getRestSNTP, &postRestSNTP, &putRestSNTP, &deleteRestSNTP);

error_t getRestSNTP(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;
   int p=0;
   uint32_t i;
   (void) RestApi;
#if (REST_JSON_TYPE == JSON)
      p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER, "{\"ntp_servers\":[\"%s\",\"%s\",\"%s\",\"%s\"],\"timezone\":\"%s\",\"period\":%"PRIu32"}\r\n",
         &(ntpContex.ntp_candidates[0][0]),&ntpContex.ntp_candidates[1][0],&ntpContex.ntp_candidates[2][0],&ntpContex.ntp_candidates[3][0],
         pRTC_GetTimezone(),
         ntpContex.period
      );
      connection->response.contentType = mimeGetType(".json");
#else
#if(REST_JSON_TYPE == JSON_API)
   p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER,"{\"data\":[");

   for (i=0;i<NUM_OF_NTP_SERVERS;i++)
   {
      if (isalnum(ntpContex.ntp_candidates[i][0]))
      {
         p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER,"{\"type\": \"ntp\",\"id\":%"PRIu32",\"attributes\":{\"address\":\"%s\"}}", i, &ntpContex.ntp_candidates[i][0] );
      }
      else
      {
         break;
      }
      if ((i<NUM_OF_NTP_SERVERS) &&(isalnum(ntpContex.ntp_candidates[i+1][0])))
      {
         p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER,",");
      }
   }


   p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER,"],\"meta\": {\"period\":%"PRIu32", \"timezone\":\"%s\"}}\r\n", ntpContex.period,pRTC_GetTimezone());

   connection->response.contentType = mimeGetType(".apijson");
#endif //JSON_API
#endif //JSON
   connection->response.noCache = TRUE;
   error=rest_200_ok(connection, &restBuffer[0]);
   //Any error to report?
   return error;

}

error_t postRestSNTP(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection, "You can't create new ntp...");
}

error_t putRestSNTP(HttpConnection *connection, RestApi_t* RestApi)
{
   jsmn_parser parser;
   jsmntok_t tokens[32]; // a number >= total number of tokens
   size_t received;
   error_t error = NO_ERROR;
   (void) RestApi;
   error = httpReadStream(connection, connection->buffer, connection->request.contentLength, &received, HTTP_FLAG_BREAK_CRLF);
   if (error) return error;
   if (received == connection->request.contentLength)
   {
      connection->buffer[received] = '\0';
   }
   jsmn_init(&parser);
   error = parseNTP(connection->buffer,  connection->request.contentLength,&parser, &tokens[0]);
   if (ntpContex.needSave == TRUE)
   {
      ntpdSaveConfig();
      ntpdRestart();

   }
   connection->response.noCache = TRUE;

   if (error)
   {
      return error;
   }
   return rest_200_ok(connection,"all right");
}

error_t deleteRestSNTP(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection,"You can't delete any ntp config...");
}
