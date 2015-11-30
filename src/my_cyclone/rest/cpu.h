/*
 * cpu.h
 *
 *  Created on: 22 окт. 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_CPU_H_
#define MY_CYCLONE_REST_CPU_H_

#include "rest.h"
#include "FreeRTOS.h"

error_t restGetCpuStatus(HttpConnection *connection, RestApi_t* RestApi);

unsigned char GetCPU_Idle(void);
unsigned char GetCPU_Load(void);

void vApplicationIdleHook( void ) ;

#endif /* MY_CYCLONE_REST_CPU_H_ */
