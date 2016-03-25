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
   int period;
   bool_t enabled;
   bool_t needSave;
} ntpContext;



static const char * default_config = "{\"servers\":[\"0.ru.pool.ntp.org\",\"1.ru.pool.ntp.org\",\"2.ru.pool.ntp.org\",\"3.ru.pool.ntp.org\"],\"period\":86400,\"enabled\":true}";


static void v_NtpNextServer(char **cServerName, int iServerNum)
{
   *cServerName = &ntpContext.ntp_candidates[iServerNum][0];

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
            TRACE_INFO("Current date/time: %s\r\nNext sync after %"PRIu32" seconds\r\n\r\n", formatDate(&date, NULL), ntpContext.period);

            //Next sync after some period
            vTaskDelay(1000 * ntpContext.period);
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
   if (ntpContext.enabled == TRUE)
   {
      if (vNtpTask == NULL)
      {
         vNtpTask = osCreateTask("SNTP_client", sntpClientTread, NULL, 196, 1);
      }
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


static error_t parseNTP (char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   jsmnerr_t resultCode;
   int tokNum;
   jsmn_init(jSMNparser);
   int i;
   int length;
   int period = 0;

   int servers =0;
   ntpContext.needSave = FALSE;
   ntpContext.enabled = TRUE;

   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);

   if(resultCode)
   {

      for (i=0;i<NUM_OF_NTP_SERVERS;i++)
      {
         tokNum = jsmn_get_array_value(data, jSMNtokens, resultCode, "/servers", i);
         if ((!i) && (tokNum<1))
         {
            // If it is no data in array
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
               memcpy(&ntpContext.ntp_candidates[i][0], &data[jSMNtokens[tokNum].start], length);
               ntpContext.ntp_candidates[i][length]='\0';
               servers++;
            }
            else
            {
               xprintf("Warning: Name of ntp server № %D in config file \"/config/ntp.json\" is too long. Skipping", i);
            }
         }

      }
   }
   if (servers>0)
   {
      ntpContext.servers = servers;
      while (servers<NUM_OF_NTP_SERVERS)
      {
         ntpContext.ntp_candidates[servers][0]='\0';
         servers++;
      }
   }



   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/period");

   if (tokNum>0)
   {
      if (ISDIGIT(data[jSMNtokens[tokNum].start] ))
      {
         period = atoi(&data[jSMNtokens[tokNum].start] );
         if ((period > 0) && (period<= HOURS(48)))
         {
            ntpContext.period = period;
         }
      }
   }


   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/needSave");
   if (tokNum > 0)
   {
      if (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0)
      {
         ntpContext.needSave = TRUE;
      }
   }

   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/enabled");
   if (tokNum > 0)
   {
      if (strncmp (&data[jSMNtokens[tokNum].start], "false" ,5) == 0)
      {
         ntpContext.enabled = FALSE;
      }
   }
   else
   {

   }
   return NO_ERROR;
}

static void ntpdSaveConfig (void)
{
   save_config("/config/ntp.json","{\"servers\":[\"%s\",\"%s\",\"%s\",\"%s\"],\"period\":%D, \"enabled\":%S}",
      &ntpContext.ntp_candidates[0][0],&ntpContext.ntp_candidates[1][0],&ntpContext.ntp_candidates[2][0],&ntpContext.ntp_candidates[3][0],ntpContext.period,
      (ntpContext.enabled==TRUE)?"true":"false");
}

error_t ntp_defaults(void)
{
   return read_default(default_config, strlen(default_config), &parseNTP);
}

error_t ntpdConfigure(void)
{
   error_t error;
   error = read_config("/config/ntp.json",&parseNTP);
   return error;
}
register_rest_function(ntp,"/ntp", NULL, NULL, &getRestSNTP, &postRestSNTP, &putRestSNTP, &deleteRestSNTP);

error_t getRestSNTP(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;
   int p=0;

   (void) RestApi;
#if (REST_JSON_TYPE == JSON)
   p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER, "{\"servers\":[\"%s\",\"%s\",\"%s\",\"%s\"],\"timezone\":\"%s\",\"period\":%d,\"enabled\":%s}\r\n",
      &(ntpContext.ntp_candidates[0][0]),&ntpContext.ntp_candidates[1][0],&ntpContext.ntp_candidates[2][0],&ntpContext.ntp_candidates[3][0],
      pRTC_GetTimezone(),ntpContext.period,(ntpContext.enabled==TRUE)?"true":"false"
   );
   connection->response.contentType = mimeGetType(".json");
#else
#if(REST_JSON_TYPE == JSON_API)
   uint32_t i;
   p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER,"{\"data\":[");

   for (i=0;i<NUM_OF_NTP_SERVERS;i++)
   {
      if (isalnum(ntpContext.ntp_candidates[i][0]))
      {
         p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER,"{\"type\": \"ntp\",\"id\":%"PRIu32",\"attributes\":{\"address\":\"%s\"}}", i, &ntpContext.ntp_candidates[i][0] );
      }
      else
      {
         break;
      }
      if ((i<NUM_OF_NTP_SERVERS) &&(isalnum(ntpContext.ntp_candidates[i+1][0])))
      {
         p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER,",");
      }
   }


   p+=snprintf(restBuffer+p, SIZE_OF_REST_BUFFER,"],\"meta\": {\"period\":%"PRIu32", \"timezone\":\"%s\"}}\r\n", ntpContext.period,pRTC_GetTimezone());

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
   if (ntpContext.needSave == TRUE)
   {
      ntpdSaveConfig();
      ntpContext.needSave = FALSE;
      ntpdRestart();

   }
   error=getRestSNTP(connection, RestApi);
   //Any error to report?
   return error;
}

error_t deleteRestSNTP(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection,"You can't delete any ntp config...");
}
