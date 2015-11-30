/*
 * ftpd.h
 *
 *  Created on: 25 апр. 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_FTPD_H_
#define MY_CYCLONE_FTPD_H_

#include <stdlib.h>
#include "os_port.h"
#include "core/net.h"
#include "ftp/ftp_server.h"
#include "path.h"
#include "debug.h"
#include "date_time.h"
#include "rtc.h"

FtpServerContext ftpServerContext;

error_t ftpdConfigure (void);
error_t ftpdStart (void);
uint_t ftpServerCheckUserCallback(FtpClientConnection *connection,
		const char_t *user);

uint_t ftpServerCheckPasswordCallback(FtpClientConnection *connection,
   const char_t *user, const char_t *password);

uint_t ftpServerGetFilePermCallback(FtpClientConnection *connection,
   const char_t *user, const char_t *path);

#endif /* MY_CYCLONE_FTPD_H_ */
