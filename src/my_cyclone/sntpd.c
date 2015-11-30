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

OsTask *vNtpTask;
char ntp_candidates[NUM_OF_NTP_SERVERS][MAX_LEN_OF_NTP_SERVER];

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
            ntpd_stop();
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
   *cServerName = &ntp_candidates[iServerNum][0];

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
            unixTime = timestamp.seconds - 2208988800+timezone;

            //Set counter in RTC in localtime
            RTC_CalendarConfig(unixTime);

            //Convert Unix timestamp to date
            convertUnixTimeToDate(unixTime, &date);

            //Debug message
            TRACE_INFO("Current date/time: %s\r\nNext sync after 24 hours\r\n\r\n", formatDate(&date, NULL));

            //Next sync after 24 hours
            vTaskDelay(1000*60*60*24);
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
void ntpd_stop(void)
{
   if (vNtpTask != NULL)
   {
      osDeleteTask(vNtpTask);
      vNtpTask = NULL;
   }
}
static void ntp_use_default_servers (void)
{
   xprintf("Warning: incorrect config file \"/config/ntp.json\". Use default servers.");
#if (NUM_OF_NTP_SERVERS>0)
   memcpy(&ntp_candidates[0][0], "0.openwrt.pool.ntp.org\0", 23);
#if (NUM_OF_NTP_SERVERS>1)
   memcpy(&ntp_candidates[1][0], "1.openwrt.pool.ntp.org\0", 23);
#if (NUM_OF_NTP_SERVERS>2)
   memcpy(&ntp_candidates[2][0], "2.openwrt.pool.ntp.org\0", 23);
#if (NUM_OF_NTP_SERVERS>3)
   memcpy(&ntp_candidates[3][0], "3.openwrt.pool.ntp.org\0", 23);
#endif
#endif
#endif
#endif
}
static void ntp_use_default_timezone(void)
{
   xprintf("Warning: incorrect config file \"/config/ntp.json\". Use default timezone");
   timezone = 21600;
}
#define HOURS(a) (a*60*60)
#define MINUTES(a) (a*60)
#define ISDIGIT(a) (((a)>='0') && ((a)<='9'))
static error_t parseNTP (char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   jsmnerr_t resultCode;
   int tokNum;
   jsmn_init(jSMNparser);
   int i, correct=0;
   int length;
   time_t tz;
   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);

   if(resultCode)
   {
      for (i=0;i<4;i++)
      {
         tokNum = jsmn_get_array_value(data, jSMNtokens, resultCode, "/config/ntp_candidates", i);
         if ((!i) && (tokNum<1))
         {
            ntp_use_default_servers();  // If it is no data in array
            break;
         }
/*
 * todo: проверить с 5 и более серверами в конфиге
 */
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length<MAX_LEN_OF_NTP_SERVER)
         {
            memcpy(&ntp_candidates[i][0], &data[jSMNtokens[tokNum].start], length);
            ntp_candidates[i][length]='\0';
            correct++;
         }
         else
         {
            ntp_candidates[i][0]='\0';
            xprintf("Warning: Name of ntp server № %D in config file \"/config/ntp.json\" is too long. Skipping", i);
         }

      }

   }
   else
   {
      ntp_use_default_servers();
   }

   if (correct == 0)
   {
      ntp_use_default_servers();
   }

   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/timezone");

   if (tokNum)
   {
      if(strncmp (&data[jSMNtokens[tokNum].start], "GMT" ,3)==0)
      {
         //date.toString js method returns sonething like this:
         //           Sun Jun 14 2015 02:28:19 GMT+0500 (YEKT)
         //Try to parse this record            ^^^^^^^^

         //Check, what symbol after "GMT" is '+','-' or ' '
         //and next 4 symbols is a digit
         if (\
            (\
               ((*(&data[jSMNtokens[tokNum].start+3]))=='+') ||\
               ((*(&data[jSMNtokens[tokNum].start+3]))=='-') ||\
               ((*(&data[jSMNtokens[tokNum].start+3]))==' ')\
            ) &&\
            ISDIGIT(*(&data[jSMNtokens[tokNum].start+4])) &&\
            ISDIGIT(*(&data[jSMNtokens[tokNum].start+5])) &&\
            ISDIGIT(*(&data[jSMNtokens[tokNum].start+6])) &&\
            ISDIGIT(*(&data[jSMNtokens[tokNum].start+7])) \
         )
         {
            tz=(*(&data[jSMNtokens[tokNum].start+4])-'0')*10*60*60+\
               (*(&data[jSMNtokens[tokNum].start+5])-'0')*60*60+\
               (*(&data[jSMNtokens[tokNum].start+6])-'0')*10*60+\
               (*(&data[jSMNtokens[tokNum].start+7])-'0')*60;
            if (*(&data[jSMNtokens[tokNum].start+3])=='-')
               tz=tz*-1;
            timezone=tz;
         }
         else
         {
            ntp_use_default_timezone();
         }

      }
      else
      {
         ntp_use_default_timezone();
      }
   }

   else
   {
      ntp_use_default_timezone();
   }
   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/config/enabled");
   if (tokNum >= 0)
   {
      if (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0)
      {

      }
      else if (strncmp (&data[jSMNtokens[tokNum].start], "false" ,5) == 0)
      {

      }

   }
   tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/save");
   if (tokNum >= 0)
   {
      if (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0)
      {
         save_config("/config/ntp.json","{\"config\":{\"ntp_candidates\":[\"%S\",\"%S\",\"%S\",\"%S\"],\"timezone\":\"GMT%C%C%C%C%C\"}}\r\n",
            ntp_candidates[0],ntp_candidates[1],ntp_candidates[2],ntp_candidates[3],
            (timezone>0?'+':'-'),
            ('0'+abs(timezone) / (HOURS(10))), ('0'+abs(timezone%(HOURS(10))) / (HOURS(1))),
            ('0'+abs( (timezone % (HOURS(1))) / (MINUTES(10)))), ('0'+abs( (timezone % (MINUTES(10))) / (MINUTES(1)))));
         ntpd_stop();
         ntpdStart();
      }
      else if (strncmp (&data[jSMNtokens[tokNum].start], "false" ,5) == 0)
      {
         ntpd_stop();
         ntpdStart();
      }

   }
   return NO_ERROR;
}

void ntpdConfigure(void)
{
read_config("/config/ntp.json",&parseNTP);

   //	FreeRTOS_CLIRegisterCommand( &xTaskNtpd );
}
register_rest_function(ntp,"/ntp", NULL, NULL, &getRestStnp, &postRestStnp, &putRestStnp, &deleteRestStnp);

error_t getRestStnp(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;
   (void) RestApi;
   sprintf(restBuffer,"{\"ntp_candidates\":[\"%s\",\"%s\",\"%s\",\"%s\"],\"timezone\":\"GMT%c%c%c%c%c\"}\r\n",
      ntp_candidates[0],ntp_candidates[1],ntp_candidates[2],ntp_candidates[3],
      (timezone>0?'+':'-'),
      ('0'+abs(timezone) / (HOURS(10))), ('0'+abs(timezone%(HOURS(10))) / (HOURS(1))),
      ('0'+abs( (timezone % (HOURS(1))) / (MINUTES(10)))), ('0'+abs( (timezone % (MINUTES(10))) / (MINUTES(1)))));
   connection->response.contentType = mimeGetType(".json");
   connection->response.noCache = TRUE;
   error=rest_200_ok(connection, &restBuffer[0]);
   //Any error to report?
   return error;

}

error_t postRestStnp(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection, "You can't create new ntp...");
}
error_t putRestStnp(HttpConnection *connection, RestApi_t* RestApi)
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
   connection->response.noCache = TRUE;
   if (error)
   {
      return error;
   }
   return rest_200_ok(connection,"all right");
}

error_t deleteRestStnp(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection,"You can't delete any ntp config...");
}
