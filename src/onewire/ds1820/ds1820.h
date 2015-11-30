/*
 * ds1820.h
 *
 *  Created on: 25 июня 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_DS1820_H_
#define MY_CYCLONE_REST_DS1820_H_

#include "onewire.h"
#include "os_port.h"

#define MAX_DS1820_COUNT MAX_ONEWIRE_COUNT




typedef struct{
   OwSensor_t * onewire;
}
ds1820_t;

typedef __start_packed struct {
	uint8_t lsb;
	uint8_t msb;
	uint8_t th;
	uint8_t tl;
	uint8_t cr;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t reserved3;
	uint8_t crc;
} __end_packed ds1820Scratchpad_t;

ds1820_t sensorsDS1820[MAX_DS1820_COUNT];


void ds1820Task(void);
void setup_ds1820(void);

#endif /* MY_CYCLONE_REST_DS1820_H_ */
