/*
 * network.h
 *
 *  Created on: 11 янв. 2015 г.
 *      Author: timurtaipov
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "sntpd.h"
#include "httpd.h"
#include "ftpd.h"
#include "dns_sd/dns_sd.h"

#define APP_MAC_ADDR "00-1C-37-EF-01-07"


#define APP_IPV4_HOST_ADDR "192.168.100.211"
#define APP_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IPV4_DEFAULT_GATEWAY "192.168.100.1"
#define APP_IPV4_PRIMARY_DNS "192.168.100.1"
#define APP_IPV4_SECONDARY_DNS "8.8.8.8"


#define APP_IPV6_LINK_LOCAL_ADDR "fe80::207"
#define APP_IPV6_PREFIX "2001:db8::"
#define APP_IPV6_PREFIX_LENGTH 64
#define APP_IPV6_GLOBAL_ADDR "2001:db8::207"
#define APP_IPV6_ROUTER "fe80::1"
#define APP_IPV6_PRIMARY_DNS "2001:4860:4860::8888"
#define APP_IPV6_SECONDARY_DNS "2001:4860:4860::8844"

DnsSdService DnsSdServices[1];


error_t networkConfigure (void);
error_t networkStart (void);

void networkServices(void *pvParametrs);
error_t restGetNetwork(HttpConnection *connection, RestApi_t* RestApi);
error_t restPutNetwork(HttpConnection *connection, RestApi_t* RestApi);
//void stop_ntp(void);
//void sntpClientTread(void *pvParametrs);

#endif /* NETWORK_H_ */
