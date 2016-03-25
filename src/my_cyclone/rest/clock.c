/*
 * clock.c
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */
#include "clock.h"
#include "configs.h"
#include <ctype.h>

jsmn_parser parser;
jsmntok_t tokens[15]; // a number >= total number of tokens
//int resultCode;

#define HOURS(a) (a*60*60)
#define MINUTES(a) (a*60)
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

register_rest_function(clock, "/clock", NULL, NULL, &restGetClock, &restPostClock, &restPutClock, &restDeleteClock);
static const char * default_config = "{\"timezone\":\"GMT+0500 (YEKT)\"}";

struct clockContext_t{
   time_t unixtime;
   bool_t needSave;
   char tz[TIMEZONE_LENGTH];
}clockContext;

error_t restGetClock(HttpConnection *connection, RestApi_t* RestApi)
{
   char_t buf[128];
   error_t error = NO_ERROR;
   DateTime time;
   int p=0;

   (void) RestApi;
   getCurrentDate(&time);
#if (REST_JSON_TYPE == JSON)
   p=sprintf(restBuffer,"{\"clock\":{\r\n\"unixtime\":%lu,\r\n", getCurrentUnixTime());
   p+=sprintf(restBuffer+p,"\"localtime\":\"%s %s\",\r\n",
      htmlFormatDate(&time, &buf[0]),
      pRTC_GetTimezone()
   );
   p+=sprintf(restBuffer+p,"\"time\":\"%02d:%02d:%02d\",\r\n", time.hours, time.minutes, time.seconds);
   p+=sprintf(restBuffer+p,"\"date\":\"%04d.%02d.%02d\",\r\n", time.year,time.month,time.day);
   p+=sprintf(restBuffer+p,"\"timezone\":\"%s\"}}\r\n",pRTC_GetTimezone());
   connection->response.contentType = mimeGetType(".json");
#else
#if(REST_JSON_TYPE == JSON_API)
   p+=sprintf(restBuffer+p,"{\"data\":{\"type\":\"clock\", \"id\":0,\"attributes\":{");
   p+=sprintf(restBuffer+p,"\"unixtime\":%lu,\"localtime\":\"%s %s\",\"time\":\"%02d:%02d:%02d\",",getCurrentUnixTime(),htmlFormatDate(&time, &buf[0]),
      pRTC_GetTimezone(),time.hours, time.minutes, time.seconds);
   p+=sprintf(restBuffer+p,"\"date\":\"%04d.%02d.%02d\",",time.year,time.month,time.day);
   p+=sprintf(restBuffer+p,"\"timezone\":\"%s\"}}}",pRTC_GetTimezone());
   connection->response.contentType = mimeGetType(".apijson");
#endif //JSON_API
#endif //JSON
   connection->response.noCache = TRUE;

   error=rest_200_ok(connection, &restBuffer[0]);
   //Any error to report?

   return error;
}
error_t restPostClock(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection, "You can't create new clock...");
}

static error_t parseClock (char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   jsmnerr_t resultCode;
   int tokNum;
   jsmn_init(jSMNparser);
   int length;
   error_t error = ERROR_UNSUPPORTED_REQUEST;

   clockContext.needSave = FALSE;

   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   if (resultCode>0)
   {

      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/timezone");

      if (tokNum>0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length < TIMEZONE_LENGTH)
         {
            memcpy(&clockContext.tz, &data[jSMNtokens[tokNum].start], length);
            clockContext.tz[length] = '\0';
            data[jSMNtokens[tokNum].end] = '\0';
            RTC_SetTimezone(&clockContext.tz);
            error = NO_ERROR;
         }

      }

      tokNum=jsmn_get_value(data, jSMNtokens, resultCode, "/unixtime");
      if (tokNum>0)
      {
         if(isdigit(jSMNtokens[tokNum].start))
         {
            length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
            clockContext.unixtime=atoi(&data[jSMNtokens[tokNum].start]);
            xprintf("unixtime: %lU\r\n", clockContext.unixtime);
            RTC_CalendarConfig(clockContext.unixtime);
            error = NO_ERROR;
         }

      }

      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/needSave");
      if (tokNum > 0)
      {
         if (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0)
         {
            clockContext.needSave = TRUE;
         }
      }
   }
   return error;
}

error_t clock_defaults(void)
{
   return read_default(default_config, strlen(default_config), &parseClock);
}
error_t clockConfigure(void)
{
   error_t error;
   error = read_config("/config/clock.json",&parseClock);
   return error;
}
static void clockSaveConfig (void)
{
   save_config("/config/clock.json","{\"timezone\":\"%s\"}", &clockContext.tz);
}

error_t restPutClock(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;

   size_t received;

   (void) RestApi;

   connection->response.noCache = TRUE;

   error = httpReadStream(connection, connection->buffer, connection->request.contentLength, &received, HTTP_FLAG_BREAK_CRLF);

   if (error)
   {
      return error;
   }

   if (received == connection->request.contentLength)
   {
      connection->buffer[received] = '\0';
   }

   jsmn_init(&parser);

   error = parseClock(connection->buffer,  connection->request.contentLength,&parser, &tokens[0]);

   if (clockContext.needSave == TRUE)
   {
      clockSaveConfig();
      clockContext.needSave = FALSE;
//      ntpdRestart();

   }


   if (!error)
   {
      return restGetClock(connection, RestApi);
   }
   else
   {
      return rest_400_bad_request(connection,"No unixtime field in request");
   }
}



error_t restDeleteClock(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection,"You can't delete any clock...");
}

