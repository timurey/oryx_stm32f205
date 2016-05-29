/*
 * executor_def.h
 *
 *  Created on: 04 нояб. 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_EXECUTOR_DEF_H_
#define MY_CYCLONE_REST_EXECUTOR_DEF_H_

#include "rest.h"
#include "executor.h"

typedef error_t (*tInitExecutorHandler)(jsmnParserStruct * jsonParser, executor_t ** pCurrentExecutor, uint8_t * pos);
typedef error_t (*tDeinitExecutorHandler)(void);
typedef error_t (*tGetExecutorHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPostExecutorHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPutExecutorHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tDeleteExecutorHandler)(HttpConnection *connection, RestApi_t * rest);

typedef struct
{
   const char *executorClassPath;
   const my_executor_type executorType;
   const tInitExecutorHandler executorInitClassHadler;
   const tDeinitExecutorHandler executorDenitClassHadler;
   const tGetExecutorHandler executorGetClassHadler;
   const tPostExecutorHandler executorPostClassHadler;
   const tPutExecutorHandler executorPutClassHadler;
   const tDeleteExecutorHandler executorDeleteClassHadler;
} executorFunctions;


#define register_executor_function(name, path, type, init_f, deinit_f, get_f, post_f, put_f, delete_f) \
   const executorFunctions handler_##name __attribute__ ((section ("executor_functions"))) = \
{ \
  .executorClassPath = path, \
  .executorType = type, \
  .executorInitClassHadler = init_f, \
  .executorDenitClassHadler = deinit_f, \
  .executorGetClassHadler = get_f, \
  .executorPostClassHadler = post_f, \
  .executorPutClassHadler = put_f, \
  .executorDeleteClassHadler = delete_f \
}
extern executorFunctions __start_executor_functions; //предоставленный линкером символ начала секции rest_functions
extern executorFunctions __stop_executor_functions; //предоставленный линкером символ конца секции rest_functions



#endif /* MY_CYCLONE_REST_EXECUTOR_DEF_H_ */
