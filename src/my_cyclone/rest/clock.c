/*
 * clock.c
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */
#include "clock.h"
#include "configs.h"
#include "macros.h"

jsmn_parser parser;
jsmntok_t tokens[15]; // a number >= total number of tokens
//int resultCode;

#define HOURS(a) (a*60*60)
#define MINUTES(a) (a*60)


register_rest_function(clock, "/clock", NULL, NULL, &restGetClock, &restPostClock, &restPutClock, &restDeleteClock);
register_defalt_config( "{\"timezone\":\"GMT+0500 (YEKT)\"}");

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
   if (RestApi->restVersion == 1)
   {
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"{\"clock\":{\r\n\"unixtime\":%lu,\r\n", getCurrentUnixTime());
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"localtime\":\"%s %s\",\r\n",
         htmlFormatDate(&time, &buf[0]),
         pRTC_GetTimezone()
      );
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"time\":\"%02d:%02d:%02d\",\r\n", time.hours, time.minutes, time.seconds);
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"date\":\"%04d.%02d.%02d\",\r\n", time.year,time.month,time.day);
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"timezone\":\"%s\"}}\r\n",pRTC_GetTimezone());
      connection->response.contentType = mimeGetType(".json");
   }
   else if (RestApi->restVersion == 2)
   {
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"{\"data\":{\"type\":\"clock\", \"id\":0,\"attributes\":{");
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"unixtime\":%lu,\"localtime\":\"%s %s\",\"time\":\"%02d:%02d:%02d\",",getCurrentUnixTime(),htmlFormatDate(&time, &buf[0]),
         pRTC_GetTimezone(),time.hours, time.minutes, time.seconds);
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"date\":\"%04d.%02d.%02d\",",time.year,time.month,time.day);
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p,"\"timezone\":\"%s\"}}}",pRTC_GetTimezone());
      connection->response.contentType = mimeGetType(".apijson");
   }
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
   int resultCode;
   int strLen = 0;
   char str[12];
   error_t error = ERROR_UNSUPPORTED_REQUEST;

   jsmn_init(jSMNparser);
   clockContext.needSave = FALSE;

   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   if (resultCode>0)
   {
      strLen = jsmn_get_string(data, jSMNtokens, resultCode, "$.timezone", &clockContext.tz[0], TIMEZONE_LENGTH);
      if (strLen)
      {
         RTC_SetTimezone(&(clockContext.tz[0]));
         error = NO_ERROR;
      }


      strLen=jsmn_get_string(data, jSMNtokens, resultCode, "$.unixtime", &str[0], 12);
      if (strLen)
      {
         clockContext.unixtime=atoi(str);
         RTC_CalendarConfig(clockContext.unixtime+RTC_GetTimezone());
         error = NO_ERROR;
      }

       jsmn_get_bool(data, jSMNtokens, resultCode, "$.needSave", &clockContext.needSave);

   }
   return error;
}


error_t clockConfigure(void)
{
   error_t error;
   error = read_default(&defaultConfig, &parseClock);
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

