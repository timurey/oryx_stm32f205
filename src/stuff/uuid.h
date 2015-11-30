/*
 * uuid.h
 *
 *  Created on: 01 авг. 2015 г.
 *      Author: timurtaipov
 */

#ifndef UUID_H_
#define UUID_H_

#include "compiler_port.h"

/**
 * The STM32 factory-programmed UUID memory.
 * Three values of 32 bits each starting at this address
 * Use like this: STM32_UUID[0], STM32_UUID[1], STM32_UUID[2]
 */

typedef  struct
{
   __start_packed union
   {
      uint8_t b[12];
      uint16_t hw[6];
      uint32_t w[3];
   };
} __attribute__((__packed__)) uuid_t;

extern uuid_t * uuid;

char * get_uuid (int * num_of_bits);

#define IsLocal(serial) (((serial[0] == uuid->b[6] && serial[1]==uuid->b[5] && serial[2]==uuid->b[4] && serial[3]==uuid->b[11] && serial[4]==uuid->b[10] && serial[5]==uuid->b[9] && serial[6]==uuid->b[8] )?TRUE:FALSE))

#endif /* UUID_H_ */
