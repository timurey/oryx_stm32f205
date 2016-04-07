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
   error_t error = NO_ERROR;
   executorFunctions * wantedExecutor;
   wantedExecutor = restFindExecutors(RestApi);
   if (wantedExecutor != NULL)
   {
      if (wantedExecutor->executorGetClassHadler != NULL)
      {
         error = wantedExecutor->executorGetClassHadler(connection, RestApi);
      }
      else
      {
         error = ERROR_NOT_IMPLEMENTED;
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
   uint8_t pos;
   executor_t * currentExecutor = &executors[0];

   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);

   if(resultCode)
   {
      for (executorFunctions *cur_executor = &__start_executor_functions; cur_executor < &__stop_executor_functions; cur_executor++)
      {
         cur_executor->executorInitClassHadler(data, jSMNtokens, &currentExecutor, &resultCode, &pos);
      }
   }
   else
   {
      //    ntp_use_default_servers();
   }

   return NO_ERROR;
}

void executorsConfigure(void)
{
   error_t error;
   memset (&executors[0],0,sizeof(executor_t)*MAX_NUM_EXECUTORS);
   error = read_config("/config/executors.json",&parseExecutors);
//   osCreateTask("oneWireTask",oneWireTask, NULL, configMINIMAL_STACK_SIZE*4, 1);
   //   osCreateTask("input",inputTask, NULL, configMINIMAL_STACK_SIZE*4, 1);
}
