/*
 * executor.h
 *
 *  Created on: 04 нояб. 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_EXECUTOR_H_
#define MY_CYCLONE_REST_EXECUTOR_H_

#include "error.h"
#include "rest.h"

#include "output.h"
#include "os_port_config.h"
#include "compiler_port.h"

#define MAX_NUM_EXECUTORS (MAX_NUM_OUTPUTS)
#define MAX_EXECUTORS_LEN_SERIAL (OUTPUT_SERIAL_LENGTH)

typedef enum {
   O_NONE =0,
   O_DIGITAL,
   O_ANALOG,
   O_PWM,
   O_HTTP
} my_executor_type;

typedef union
{
   float fVal;
   uint16_t   uVal;
   char  cVal;
   char  *pVal;
} executorValue;

typedef struct
{
   uint8_t id;
   uint8_t serial[MAX_EXECUTORS_LEN_SERIAL];
   my_executor_type type;
   char* place;
   char* name;
   executorValue value;
   uint8_t status;
}executor_t;

void executorsConfigure(void);



#endif /* MY_CYCLONE_REST_EXECUTOR_H_ */
