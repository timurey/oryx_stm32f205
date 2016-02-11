/*
 * variables.h
 *
 *  Created on: 05 февр. 2016 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_VARIABLES_H_
#define MY_CYCLONE_REST_VARIABLES_H_

#include "rest.h"
#include "os_port.h"
#include "compiler_port.h"

#define NUM_VARIABLES 32

error_t getVariable(char *name, double * value);
error_t createVariable(char *name, double value);
error_t setVariable(char *name, double value);
error_t deleteVariable(char *name);

error_t restInitVariables(void);

error_t restGetVariable(HttpConnection *connection, RestApi_t* RestApi);

error_t restPostVariable(HttpConnection *connection, RestApi_t* RestApi);

error_t restPutVariable(HttpConnection *connection, RestApi_t* RestApi);

error_t restDeleteVariable(HttpConnection *connection, RestApi_t* RestApi);

typedef struct
{
   char * name;
   double value;
   OsMutex mutex;
} variables;



#endif /* MY_CYCLONE_REST_VARIABLES_H_ */
