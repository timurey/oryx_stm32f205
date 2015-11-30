/*
 * cli_hal.h
 *
 *  Created on: 29 янв. 2015 г.
 *      Author: timurtaipov
 */

#ifndef STUFF_CLI_HAL_H_
#define STUFF_CLI_HAL_H_

#include "freertos.h"
#include "queue.h"
#include "task.h"
extern xQueueHandle xRxQueue;
void vCommandInterpreterTask( void *pvParameters );

#endif /* STUFF_CLI_HAL_H_ */
