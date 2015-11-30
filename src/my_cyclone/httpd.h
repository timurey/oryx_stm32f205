/*
 * httpd.h
 *
 *  Created on: 30 янв. 2015 г.
 *      Author: timurtaipov
 */

#ifndef STUFF_HTTPD_H_
#define STUFF_HTTPD_H_

#include <stdlib.h>
#include "os_port.h"
#include "core/net.h"
#include "http/http_server.h"
#include "http/mime.h"
#include "path.h"
#include "debug.h"
#include "date_time.h"
#include "rtc.h"

error_t httpdConfigure (void);
error_t httpdStart(void);
#endif /* STUFF_HTTPD_H_ */
