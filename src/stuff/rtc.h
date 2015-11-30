/*
 * rtc.h
 *
 *  Created on: 11 нояб. 2014 г.
 *      Author: timurtaipov
 */

#ifndef MODULES_RTC_H_
#define MODULES_RTC_H_

#pragma once
#include "stm32f2xx_hal_rcc.h"
#include "stm32f2xx_hal_rtc.h"
#include "stm32f2xx_hal_pwr.h"

#include "time.h"
#include "date_time.h"



#ifndef __YEAR__
#define __YEAR__ 2015
#endif
#ifndef __MONTH__
#define __MONTH__ 04
#endif
#ifndef __DAY__
#define __DAY__ 23
#endif
#ifndef __HOURS__
#define __HOURS__ 00
#endif
#ifndef __MINUTES__
#define __MINUTES__ 05
#endif
#ifndef __SECONDS__
#define __SECONDS__ 49
#endif
#ifndef __TIMESTAMP__
#define __TIMESTAMP__ 1429729549
#endif

#ifndef __DAY_OF_WEEK__
#define __DAY_OF_WEEK__ 4
#endif



/***************************************************************************//**
 * Declare function prototypes
 ******************************************************************************/

extern time_t timezone;
extern RTC_HandleTypeDef RtcHandle;

void RTC_Init(void);
void RTC_CalendarConfig(time_t unixtime);
#endif /* MODULES_RTC_H_ */
