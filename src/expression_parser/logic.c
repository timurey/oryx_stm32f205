/*
 * logic.c
 *
 *  Created on: 17 янв. 2016 г.
 *      Author: timurtaipov
 */

#ifndef EXPRESSION_PARSER_LOGIC_C_
#define EXPRESSION_PARSER_LOGIC_C_

#include "logic.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <math.h>
#include "debug.h"


#include "configs.h"
#include "variables_def.h"
#include "DataManager.h"

char expressions[EXPRESSIONS_LENGHT];
char * pExpression[EXPRESSION_MAX_COUNT];

char rules[RESULT_LENGHT];
char * pRules[EXPRESSION_MAX_COUNT];

OsTask *logicTask;

static error_t parseRules (jsmnParserStruct * jsonParser)
{
   int i;
   char path[64];

   char * currExpr = &expressions[0];
   char * currRule = &rules[0];
   int strLen = 0;

   jsmn_init(jsonParser->jSMNparser);

   jsonParser->resultCode = xjsmn_parse(jsonParser);
   if(jsonParser->resultCode)
   {
      for (i=0;i<EXPRESSION_MAX_COUNT;i++)
      {

         sprintf(&path[0],"$.rules[%d].expression",i);
         strLen = jsmn_get_string(jsonParser, &path[0], currExpr, EXPRESSIONS_LENGHT-(currExpr-&expressions[0]));
         if (strLen>0)
         {
            pExpression[i]=currExpr;
            currExpr+=strLen+1;
         }
         strLen=0;
         sprintf(&path[0],"$.rules[%d].result",i);
         strLen = jsmn_get_string(jsonParser, &path[0], currRule, RESULT_LENGHT-(currRule-&rules[0]));
         if (strLen>0)
         {
            pRules[i]=currRule;
            currRule+=strLen+1;
         }


      }
   }
   return NO_ERROR;
}


/**
 @brief implementation of a user-defined function for testing
 */
static double user_func_0(void){
   return 10.0;
}

/**
 @brief implementation of a user-defined function for testing
 */
static double user_func_1( double x ){
   return fabs(x);
}

/**
 @brief implementation of a user-defined funtion for testing
 */
static double user_func_2( double x, double y ){
   return sqrt( x*x + y*y );
}

/**
 @brief implementation of a user-defined function for testing
 */
static double _user_func_3( double x, double y, double z ){
   return sqrt( x*x + y*y + z*z );
}


/**
 @brief user-function callback for the parser. reads name of function to be called and checks the number of arguments before calling the appropriate function. returns true if function was called successfully, false otherwise.
 @param[in] user_data pointer to (arbitrary) user-defined data that may be needed by the callback functions. not used for this example.
 @param[in] name name of the function to execute
 @param[in] num_args the number of arguments that were supplied in the function call
 @param[out] value the return argument, which should contain the functions evaluated value on return if the evaluation was successful.
 @return true if the function was evaluated successfully, false otherwise
 */
int user_fnc_cb( void *user_data, const char *name, const int num_args, const double *args, double *value ){
   if( strcmp( name, "user_func_0" ) == 0 && num_args == 0 ){
      *value = user_func_0();
      return PARSER_TRUE;
   } else if( strcmp( name, "user_func_1" ) == 0 && num_args == 1 ){
      *value = user_func_1( args[0] );
      return PARSER_TRUE;
   } else if( strcmp( name, "user_func_2" ) == 0 && num_args == 2 ){
      *value = user_func_2( args[0], args[1] );
      return PARSER_TRUE;
   } else if( strcmp( name, "_user_func_3" ) == 0 && num_args == 3 ){
      *value = _user_func_3( args[0], args[1], args[2] );
      return PARSER_TRUE;
   }
   return PARSER_FALSE;
}

/**
 @brief user-variable callback for the parser. reads the name of the variable and sets the appropriate value in the return argument. returns true if the variable exists and the return value set, false otherwise.
 @param[in] user_data pointer to (arbitrary) user-defined state data that may be needed by the callback functions
 @param[in] name name of the variable to lookup the value for
 @param[out] value output argument that should contain the variable value on return, if the evaluation is successful.
 @return true if the variable exists and the return argument was successfully set, false otherwise
 */
static int user_var_cb( void *user_data, const char *name, double *value ){

   char device[32];
   error_t error = ERROR_OBJECT_NOT_FOUND;
   peripheral_t fd; //file descriptor
   memset(&fd, 0, sizeof(fd));

   (void) user_data;
   snprintf(&device[0], 32, "/%s", name);
   error = driver_open(&fd, &device[0], READ);
   if (error)
   {
      return PARSER_FALSE;
   }
   else
   {
      driver_read(&fd, value, sizeof(double));
      driver_close(&fd);

      return PARSER_TRUE;
   }
}



static void parse_rules(uint_t exprNum)
{
   double result;
   int d1, d2;
   float f2;

   result = parse_expression_with_callbacks( pExpression[exprNum], &user_var_cb, &user_fnc_cb, NULL );

   d1 = result;
   f2 = result - d1;
   d2 = abs(trunc(f2 * 1000));
   xprintf("Parsed: %d.%01d\n", d1, d2 );
}

static void buildDeps(void)
{

   uint8_t i=0;
   while (pExpression[i])
   {
      parse_expression_with_callbacks( pExpression[i], &userVarFake, &user_fnc_cb, &i );
      i++;
   }

}

static void parser_task (void *pvParameters)
{
   (void) pvParameters;
   portBASE_TYPE xStatus;
   uint8_t exprNum;
   while (1)
   {
      xStatus = xQueueReceive(exprQueue, &exprNum, INFINITE_DELAY);
      if (xStatus == pdPASS) {
         if (exprNum<EXPRESSION_MAX_COUNT)
         {
            xprintf("\r\nParser is starting...\r\n");
            parse_rules(exprNum);
         }
      }
   }
   vTaskDelete(NULL);
}

error_t logicConfigure(void)
{
   error_t error;
   error = read_config("/config/rules.json",&parseRules);
   if (!error)
   {
      buildDeps();
   }
   if (!error)
   {
      exprQueue = xQueueCreate(EXPRESSION_MAX_COUNT *2, sizeof(uint8_t));

      if (exprQueue == NULL)
      {
         error = ERROR_OUT_OF_MEMORY;
      }
   }
   if (!error)
   {
      dataQueue = xQueueCreate(EXPRESSION_MAX_COUNT *2, sizeof(mask_t));

      if (dataQueue == NULL)
      {
         error = ERROR_OUT_OF_MEMORY;
      }
   }
   return error;
}

error_t logicStart(void)
{
   error_t error = NO_ERROR;

   dataManagerTask = osCreateTask("data_manger", dataManager_task, NULL, configMINIMAL_STACK_SIZE, 1);
   if (dataManagerTask == NULL)
   {
      error = ERROR_OUT_OF_MEMORY;
   }

   logicTask = osCreateTask("parser_Services", parser_task,  NULL, configMINIMAL_STACK_SIZE*4, 1);
   if (logicTask==NULL)
   {
      error = ERROR_OUT_OF_MEMORY;
   }

   return error;
}
#endif /* EXPRESSION_PARSER_LOGIC_C_ */
