/*
 * clock.h
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_CLOCK_H_
#define MY_CYCLONE_REST_CLOCK_H_

#include "rest.h"
#include "rtc.h"
error_t clock_defaults(void);
error_t clockConfigure(void);
error_t restGetClock(HttpConnection *connection, RestApi_t* RestApi);
error_t restPostClock(HttpConnection *connection, RestApi_t* RestApi);
error_t restPutClock(HttpConnection *connection, RestApi_t* RestApi);
error_t restDeleteClock(HttpConnection *connection, RestApi_t* RestApi);
#endif /* MY_CYCLONE_REST_CLOCK_H_ */
