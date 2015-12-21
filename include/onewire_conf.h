/*
 * onewire_conf.h
 *
 *  Version 1.0.3
 */

#ifndef ONEWIRE_CONF_H_
#define ONEWIRE_CONF_H_


// ��������, �� ����� USART ��������� 1-wire
//#define OW_USART1
#define OW_USART2
//#define OW_USART3

#define ONEWIRE_SERIAL_LENGTH 8

#define OW_DS1820_SUPPORT ENABLE

#include "rest/sensors.h"

#endif /* ONEWIRE_CONF_H_ */
