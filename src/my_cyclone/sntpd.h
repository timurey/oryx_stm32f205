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
#define MAX_LEN_OF_NTP_SERVER 32
#define NTP_MAX_QUERY 5

extern char ntp_candidates[NUM_OF_NTP_SERVERS][MAX_LEN_OF_NTP_SERVER];

void ntpdConfigure(void);
void ntpd_stop(void);
void ntpdStart(void);
error_t getRestStnp(HttpConnection *connection, RestApi_t* RestApi);
error_t postRestStnp(HttpConnection *connection, RestApi_t* RestApi);
error_t putRestStnp(HttpConnection *connection, RestApi_t* RestApi);
error_t deleteRestStnp(HttpConnection *connection, RestApi_t* RestApi);
#endif /* STUFF_SNTPD_H_ */
