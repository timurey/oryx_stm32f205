/*
 * clock.c
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */
#include "clock.h"
#include "configs.h"
jsmn_parser parser;
jsmntok_t tokens[15]; // a number >= total number of tokens
int resultCode;

#define HOURS(a) (a*60*60)
#define MINUTES(a) (a*60)

register_rest_function(clock, "/clock", NULL, NULL, &restGetClock, &restPostClock, &restPutClock, &restDeleteClock);
static const char * default_config = "{\"timezone\":\"GMT+0500\"}";
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
   int i;
   int length;
   uint32_t period = 0;
   char tz[TIMEZONE_LENGTH];
   error_t error = NO_ERROR;
   int servers;
   //   ntpContex.needSave = FALSE;
   //   ntpContex.enabled = TRUE;

   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   if (resultCode>0)
   {
      error = NO_ERROR;

      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/timezone");

      if (tokNum>0)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         if (length < TIMEZONE_LENGTH)
         {
            memcpy(&tz[0], &data[jSMNtokens[tokNum].start], length);
            tz[length] = '\0';
            error = RTC_SetTimezone(&tz[0]);
         }

      }

      tokNum=jsmn_get_value(data, jSMNtokens, resultCode, "/unixtime");
      if (tokNum>0)
      {

            length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
            memcpy(buf, &data[jSMNtokens[tokNum].start], length);
            unixtime=atoi(buf);
            xprintf("unixtime: %lU\r\n", unixtime);
            RTC_CalendarConfig(unixtime);
            result++;


      }
      tokNum=jsmn_get_value(data, jSMNtokens, resultCode, "/localtime");
      if (tokNum)
      {
         length = jSMNtokens[tokNum].end - jSMNtokens[tokNum].start;
         memcpy(buf, &data[jSMNtokens[tokNum].start], length);
         xprintf("localtime: %S\r\n", buf);
      }
      error = NO_ERROR;
      tokNum = jsmn_get_value(data, jSMNtokens, resultCode, "/needSave");
      if (tokNum > 0)
      {
         if (strncmp (&data[jSMNtokens[tokNum].start], "true" ,4) == 0)
         {
            //         clockcontext.needSave = TRUE;
         }
      }
   }
   return NO_ERROR;
}
error_t restPutClock(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;
   int tokNum;
   size_t tok_length;
   size_t received;
   time_t unixtime;
   uint8_t result=0;
   char_t buf[128];
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

   resultCode = jsmn_parse(&parser, connection->buffer, received, tokens, 15);
   if (resultCode)
   {
      tokNum=jsmn_get_value(&connection->buffer[0], &tokens[0], resultCode, "/unixtime");
      if (tokNum)
      {
         if (tokens[tokNum].type == JSMN_PRIMITIVE)
         {
            tok_length = tokens[tokNum].end - tokens[tokNum].start;
            memcpy(buf, &connection->buffer[tokens[tokNum].start], tok_length);
            unixtime=atoi(buf);
            xprintf("unixtime: %lU\r\n", unixtime);
            RTC_CalendarConfig(unixtime);
            result++;
         }

      }
      tokNum=jsmn_get_value(&connection->buffer[0], &tokens[0], resultCode, "/localtime");
      if (tokNum)
      {
         tok_length = tokens[tokNum].end - tokens[tokNum].start;
         memcpy(buf, &connection->buffer[tokens[tokNum].start], tok_length);
         xprintf("localtime: %S\r\n", buf);
      }
   }
   if (result)
   {
      return rest_200_ok(connection,"all right");
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

