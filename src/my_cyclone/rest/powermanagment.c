/*
 * system_reset.c
 *
 *  Created on: 16 марта 2016 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_SYSTEM_RESET_C_
#define MY_CYCLONE_REST_SYSTEM_RESET_C_

#include <rest/powermanagment.h>
#include "stm32f2xx_hal.h"

static error_t restGetPowermanagment(HttpConnection *connection, RestApi_t* RestApi);
static error_t restPostPowermanagment(HttpConnection *connection, RestApi_t* RestApi);
static error_t restPutPowermanagment(HttpConnection *connection, RestApi_t* RestApi);
static error_t restDeletePowermanagment(HttpConnection *connection, RestApi_t* RestApi);

register_rest_function(powermanagment, "/powermanagment", NULL, NULL, &restGetPowermanagment, &restPostPowermanagment, &restPutPowermanagment, &restDeletePowermanagment);

void rebootNow(void)
{
   NVIC_SystemReset();
}
static error_t restGetPowermanagment(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   error_t error;
   error = rest_400_bad_request(connection,"See you...");

   return error; //Never reach this place, but it is needed by function defenition
}
static error_t restPostPowermanagment(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   error_t error;
   error = rest_400_bad_request(connection,"See you...");

   return error; //Never reach this place, but it is needed by function defenition
}
static error_t restPutPowermanagment(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   error_t error;
   error = rest_400_bad_request(connection,"See you...");

   return error; //Never reach this place, but it is needed by function defenition
}

static error_t restDeletePowermanagment(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   error_t error;
   error = rest_200_ok(connection,"See you...");

   rebootNow();
   return error; //Never reach this place, but it is needed by function defenition
}


#endif /* MY_CYCLONE_REST_SYSTEM_RESET_C_ */
