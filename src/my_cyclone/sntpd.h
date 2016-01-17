/*
 * sntp.h
 *
 *  Created on: 30 янв. 2015 г.
 *      Author: timurtaipov
 */

#ifndef STUFF_SNTPD_H_
#define STUFF_SNTPD_H_
#include "error.h"
#include "rest/rest.h"


#define NUM_OF_NTP_SERVERS 4
#define MAX_LENGTH_OF_NTP_SERVER_NAME 32
#define NTP_MAX_QUERY 5


void ntpdConfigure(void);
void ntpdStop(void);
void ntpdStart(void);
inline void ntpdRestart(void);
error_t getRestSNTP(HttpConnection *connection, RestApi_t* RestApi);
error_t postRestSNTP(HttpConnection *connection, RestApi_t* RestApi);
error_t putRestSNTP(HttpConnection *connection, RestApi_t* RestApi);
error_t deleteRestSNTP(HttpConnection *connection, RestApi_t* RestApi);
#endif /* STUFF_SNTPD_H_ */
