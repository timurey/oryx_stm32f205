/*
 * DataManager.h
 *
 *  Created on: 02 июня 2016 г.
 *      Author: timurtaipov
 */

#ifndef DRIVER_INCLUDE_DATAMANAGER_H_
#define DRIVER_INCLUDE_DATAMANAGER_H_
#include "DriverInterface.h"
#include "os_port.h"

extern OsTask * dataManagerTask;
extern QueueHandle_t exprQueue;
extern QueueHandle_t dataQueue;

void setExpressionNumber(uint32_t setExpressionNumber );
void dataManager_task(void * pvParameters);
int userVarFake( void *user_data, const char *name, double *value );

typedef struct
{
   uint32_t test;
}depedency_t;

#endif /* DRIVER_INCLUDE_DATAMANAGER_H_ */
