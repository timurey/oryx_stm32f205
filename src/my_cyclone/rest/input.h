/*
 * input.h
 *
 *  Created on: 29 авг. 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_SENSORS_INPUT_H_
#define MY_CYCLONE_REST_SENSORS_INPUT_H_

#include "rest/sensors.h"

#define INPUT_SERIAL_LENGTH 8

#define MAX_SAMPLES_COUNT 10


//analog or digital inputs
#define IPNUT_PIN_0 GPIO_PIN_0
#define IPNUT_PORT_0 GPIOA
#define INPUT_ADC_CHANNEL_0 ADC_CHANNEL_0

#define IPNUT_PIN_1 GPIO_PIN_3
#define IPNUT_PORT_1 GPIOA
#define INPUT_ADC_CHANNEL_1 ADC_CHANNEL_3

#define IPNUT_PIN_2 GPIO_PIN_0
#define IPNUT_PORT_2 GPIOB
#define INPUT_ADC_CHANNEL_2 ADC_CHANNEL_8

#define IPNUT_PIN_3 GPIO_PIN_1
#define IPNUT_PORT_3 GPIOB
#define INPUT_ADC_CHANNEL_3 ADC_CHANNEL_9

#define IPNUT_PIN_4 GPIO_PIN_0
#define IPNUT_PORT_4 GPIOC
#define INPUT_ADC_CHANNEL_4 ADC_CHANNEL_10

#define IPNUT_PIN_5 GPIO_PIN_1
#define IPNUT_PORT_5 GPIOC
#define INPUT_ADC_CHANNEL_5 ADC_CHANNEL_11

#define IPNUT_PIN_6 GPIO_PIN_2
#define IPNUT_PORT_6 GPIOC
#define INPUT_ADC_CHANNEL_6 ADC_CHANNEL_12

#define IPNUT_PIN_7 GPIO_PIN_3
#define IPNUT_PORT_7 GPIOC
#define INPUT_ADC_CHANNEL_7 ADC_CHANNEL_13

#define IPNUT_PIN_8 GPIO_PIN_4
#define IPNUT_PORT_8 GPIOC
#define INPUT_ADC_CHANNEL_8 ADC_CHANNEL_14

//digital only inputs
#define IPNUT_PIN_9 GPIO_PIN_6
#define IPNUT_PORT_9 GPIOC

#define IPNUT_PIN_10 GPIO_PIN_7
#define IPNUT_PORT_10 GPIOC

#define IPNUT_PIN_11 GPIO_PIN_0
#define IPNUT_PORT_11 GPIOD

#define IPNUT_PIN_12 GPIO_PIN_1
#define IPNUT_PORT_12 GPIOD

#define IPNUT_PIN_13 GPIO_PIN_2
#define IPNUT_PORT_13 GPIOD

#define IPNUT_PIN_14 GPIO_PIN_3
#define IPNUT_PORT_14 GPIOD

#define IPNUT_PIN_15 GPIO_PIN_4
#define IPNUT_PORT_15 GPIOD

#define TOTAL_INPUTS 16

typedef enum {
   digital,
   invert_digital,
   analog,
   pwm
}inputMode_t;

typedef enum {
   click,
   double_click,
}inputEvent_t;



#define GPIO_PIN_ISTATE(PORT,PIN)  &(*(__I uint32_t *)(PERIPH_BB_BASE + ((((uint32_t)&((PORT)->IDR)) - PERIPH_BASE) << 5) + ((PIN) << 2)))
#define GPIO_PIN_ISET(PORT,PIN)  (*(__I uint32_t *)(PERIPH_BB_BASE + ((((uint32_t)&((PORT)->ODR)) - PERIPH_BASE) << 5) + ((PIN) << 2)))

#define IPNUT_PIN_STATE(a)   GPIO_PIN_ISTATE(inputPorts[a], inputPin[a])
#define IPNUT_PIN_SET(a)   GPIO_PIN_ISET(inputPorts[a], inputPin[a])

#endif /* MY_CYCLONE_REST_SENSORS_INPUT_H_ */

