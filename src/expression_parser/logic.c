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
//#include<stdio.h>
#include<string.h>
#include <math.h>
#include "xprintf.h"
#include "rest/variables.h"
#include "rest/sensors.h"

#include"expression_parser.h"
#include "jsmn_extras.h"
#include "configs.h"

#define EXPRESSIONS_LENGHT 128
#define EXPRESSION_MAX_COUNT 8
#define RESULT_LENGHT 64
char expressions[EXPRESSIONS_LENGHT];
char * pExpression[EXPRESSION_MAX_COUNT];

char rules[RESULT_LENGHT];
char * pRules[EXPRESSION_MAX_COUNT];

static error_t parseRules (char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   int i;
   char path[64];
   int resultCode;

   char * currExpr = &expressions[0];
   char * currRule = &rules[0];
   int strLen = 0;
   jsmn_init(jSMNparser);

   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   if(resultCode)
   {
      for (i=0;i<EXPRESSION_MAX_COUNT;i++)
      {

         sprintf(&path[0],"$.rules[%d].expression",i);
         strLen = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], currExpr, EXPRESSIONS_LENGHT-(currExpr-&expressions[0]));
         if (strLen>0)
         {
            pExpression[i]=currExpr;
            currExpr+=strlen(currExpr)+1;
         }
         strLen=0;
         sprintf(&path[0],"$.rules[%d].result",i);
         strLen = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], currRule, RESULT_LENGHT-(currRule-&rules[0]));
         if (strLen>0)
         {
            pRules[i]=currRule;
            currRule+=strlen(currRule)+1;
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
   error_t error = ERROR_OBJECT_NOT_FOUND;
   (void) user_data;
   for (varFunctions *cur_varHandler = &__start_var_functions; cur_varHandler < &__stop_var_functions; cur_varHandler++)
   {
      error = cur_varHandler->varGetMethodHadler(name, value);
      if (!error)
      {
         break;
      }
   }

   if (error)
   {
      return PARSER_FALSE;
   }
   else
   {
      return PARSER_TRUE;
   }
}



static void parse_rules(void)
{
   double result;
   int d1, d2;
   float f2;
   int i=0;
   while (pExpression[i])
   {
      result = parse_expression_with_callbacks( pExpression[i], &user_var_cb, &user_fnc_cb, NULL );

      d1 = result;
      f2 = result - d1;
      d2 = abs(trunc(f2 * 1000));
      xprintf("Parsed: %d.%01d\n", d1, d2 );
      i++;
   }

}

static void parser_task (void *pvParameters)
{
   (void) pvParameters;
   while (1)
   {
      vTaskDelay(1000);
      xprintf("Parser is starting...\r\n");
      parse_rules();
   }

   vTaskDelete(NULL);
}

error_t logicConfigure(void)
{
   error_t error;
   error = read_config("/config/rules.json",&parseRules);
   return error;
}

error_t logicStart(void)
{
   if (osCreateTask("parser_Services", parser_task,  NULL, configMINIMAL_STACK_SIZE*8, 1)!=NULL)
      return NO_ERROR;
   return ERROR_OUT_OF_MEMORY;
}
#endif /* EXPRESSION_PARSER_LOGIC_C_ */
