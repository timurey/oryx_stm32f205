/*
 * executor.c
 *
 *  Created on: 04 нояб. 2015 г.
 *      Author: timurtaipov
 */
#include "rest/executor.h"
#include "rest/executor_def.h"

#include "rest/output.h"
#include "configs.h"
#include <ctype.h>

executor_t executors[MAX_NUM_EXECUTORS];
static error_t restInitExecutors(void);
static error_t restDenitExecutors(void);
static error_t restGetExecutors(HttpConnection *connection, RestApi_t* RestApi);
static error_t restPostExecutors(HttpConnection *connection, RestApi_t* RestApi);
static error_t restPutExecutors(HttpConnection *connection, RestApi_t* RestApi);
static error_t restDeleteExecutors(HttpConnection *connection, RestApi_t* RestApi);

register_rest_function(executors, "/executors", &restInitExecutors, &restDenitExecutors, &restGetExecutors, &restPostExecutors, &restPutExecutors, &restDeleteExecutors);

register_defalt_config("{\"executors\":{\"htmlGet\":[{\"name\":\"script1\",\"scheme\":\"http\",\"auth\":\"login:password\",\"server\":\"ya.ru\",\"path\":\"temperature\",\"params\":[\"temperature=%temperature_1\",\"switcher=%input_1\"]},{\"name\":\"script2\",\"scheme\":\"http\",\"auth\":\"login:password\",\"server\":\"ya.ru\",\"path\":\"temperature\",\"params\":[\"temperature=%temperature_1\",\"switcher=%input_1\"]}]}}");

static error_t restInitExecutors(void)
{
   error_t error = NO_ERROR;

   return error;
}

static error_t restDenitExecutors(void)
{
   error_t error = NO_ERROR;
   return error;
}

static executorFunctions * restFindExecutors(RestApi_t* RestApi)
{
   for (executorFunctions *cur_executor = &__start_executor_functions; cur_executor < &__stop_executor_functions; cur_executor++)
   {
      if (REST_CLASS_EQU(RestApi, cur_executor->executorClassPath))
      {
         return cur_executor;
      }
   }
   return NULL;
}

error_t restGetExecutors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t max_len = sizeof(restBuffer);
   error_t error = NO_ERROR;
   executorFunctions * wantedExecutor;
   if (RestApi->className!=NULL)
   {
      wantedExecutor = restFindExecutors(RestApi);
      if (wantedExecutor != NULL)
      {
         if (wantedExecutor->executorGetClassHadler != NULL)
         {
            error = wantedExecutor->executorGetClassHadler(connection, RestApi);
         }
         else
         {
            //            p = printfExecutors(restBuffer+p, max_len, wantedExecutor, RestApi->restVersion);
            p+=snprintf(restBuffer+p, max_len-p, "\r\n");
            rest_501_not_implemented(connection, &restBuffer[0]);
            error = ERROR_NOT_IMPLEMENTED;
         }
      }
   }
   else
   {
      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restPostExecutors(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;
   executorFunctions * wantedExecutor;
   wantedExecutor = restFindExecutors(RestApi);
   if (wantedExecutor != NULL)
   {
      if (wantedExecutor->executorPostClassHadler != NULL)
      {
         error = wantedExecutor->executorPostClassHadler(connection, RestApi);
      }
      else
      {
         error = ERROR_UNSUPPORTED_REQUEST;
      }
   }
   else
   {
      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restPutExecutors(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;
   executorFunctions * wantedExecutor;
   wantedExecutor = restFindExecutors(RestApi);
   if (wantedExecutor != NULL)
   {
      if (wantedExecutor->executorPutClassHadler != NULL)
      {
         error = wantedExecutor->executorPutClassHadler(connection, RestApi);
      }
      else
      {
         error = ERROR_UNSUPPORTED_REQUEST;
      }
   }
   else
   {
      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restDeleteExecutors(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;
   executorFunctions * wantedExecutor;
   wantedExecutor = restFindExecutors(RestApi);
   if (wantedExecutor != NULL)
   {
      if (wantedExecutor->executorDeleteClassHadler != NULL)
      {
         error = wantedExecutor->executorDeleteClassHadler(connection, RestApi);
      }
      else
      {
         error = ERROR_UNSUPPORTED_REQUEST;
      }
   }
   else
   {
      error = ERROR_NOT_FOUND;
   }
   return error;
}

static error_t parseExecutors (char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   jsmnerr_t resultCode;
   jsmn_init(jSMNparser);
   volatile int strLen;
   uint8_t pos;
   error_t error;
   char str[180];
   volatile int size;
   int toknum;
   char path[32];
   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   if (resultCode>0)
   {
      toknum = jsmn_get_value(data, jSMNtokens, resultCode, "$.executors.htmlGet");
      size = jSMNtokens[toknum].size;

      for (toknum=0;toknum<size;toknum++)
      {
         sprintf(&path[0],"$.executors.htmlGet[%d].name", toknum);
         strLen = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], &str[0], 180);
         if (strLen)
         {

            error = NO_ERROR;
         }
      }
   }


   //   executor_t * currentExecutor = &executors[0];
   //
   //   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   //
   //   if(resultCode)
   //   {
   //      for (executorFunctions *cur_executor = &__start_executor_functions; cur_executor < &__stop_executor_functions; cur_executor++)
   //      {
   //         cur_executor->executorInitClassHadler(data, jSMNtokens, &currentExecutor, &resultCode, &pos);
   //      }
   //   }
   //   else
   //   {
   //      //    ntp_use_default_servers();
   //   }

   return NO_ERROR;
}

void executorsConfigure(void)
{
   error_t error;
   //   memset (&executors[0],0,sizeof(executor_t)*MAX_NUM_EXECUTORS);
   error = read_default(&defaultConfig, &parseExecutors);
   //   error = read_config("/config/executors.json",&parseExecutors);
}
