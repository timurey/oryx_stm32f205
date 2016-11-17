/*
 * input.h
 *
 *  Created on: 29 авг. 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_SENSORS_INPUT_H_
#define MY_CYCLONE_REST_SENSORS_INPUT_H_

#include "rest/sensors.h"
#include "stm32f2xx_hal.h"

#define MAX_SAMPLES_COUNT 10

//A0, A3, B0, B1, C0, C1, C2, C3, C4 - Digital and analog inputs
//C6, C7, D0, D1, D2, D3, D4 - Digital inputs

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

#define IPNUT_PIN_13 GPIO_PIN_3
#define IPNUT_PORT_13 GPIOD

#define IPNUT_PIN_14 GPIO_PIN_4
#define IPNUT_PORT_14 GPIOD

#define IPNUT_PIN_15 GPIO_PIN_4
#define IPNUT_PORT_15 GPIOD



static uint16_t inputPin[] =
   { IPNUT_PIN_0, IPNUT_PIN_1, IPNUT_PIN_2, IPNUT_PIN_3,
      IPNUT_PIN_4, IPNUT_PIN_5, IPNUT_PIN_6, IPNUT_PIN_7, IPNUT_PIN_8, IPNUT_PIN_9,
      IPNUT_PIN_10, IPNUT_PIN_11, IPNUT_PIN_12, IPNUT_PIN_13, IPNUT_PIN_14, IPNUT_PIN_15 };

static GPIO_TypeDef* inputPort[] =
   { IPNUT_PORT_0, IPNUT_PORT_1, IPNUT_PORT_2, IPNUT_PORT_3,
      IPNUT_PORT_4, IPNUT_PORT_5, IPNUT_PORT_6, IPNUT_PORT_7, IPNUT_PORT_8, IPNUT_PORT_9,
      IPNUT_PORT_10, IPNUT_PORT_11, IPNUT_PORT_12, IPNUT_PORT_13, IPNUT_PORT_14, IPNUT_PORT_15 };

static uint32_t adcChannel[] =
   { INPUT_ADC_CHANNEL_0, INPUT_ADC_CHANNEL_1, INPUT_ADC_CHANNEL_2, INPUT_ADC_CHANNEL_3,
      INPUT_ADC_CHANNEL_4, INPUT_ADC_CHANNEL_5, INPUT_ADC_CHANNEL_6, INPUT_ADC_CHANNEL_7, INPUT_ADC_CHANNEL_8 };

#define TOTAL_INPUTS arraysize(inputPin)
typedef enum
{
   GPIO_ACTIVELOW, GPIO_ACTIVEHIGH
} activeLevel_t;

typedef enum
{
   GPIO_Z = GPIO_NOPULL, GPIO_PULLHIGH = GPIO_PULLUP, GPIO_PULLLOW = GPIO_PULLDOWN,
} pull_t;

#define GPIO_PIN_ISTATE(PORT,PIN)  &(*(__I uint32_t *)(PERIPH_BB_BASE + ((((uint32_t)&((PORT)->IDR)) - PERIPH_BASE) << 5) + ((PIN) << 2)))
#define GPIO_PIN_ISET(PORT,PIN)  (*(__I uint32_t *)(PERIPH_BB_BASE + ((((uint32_t)&((PORT)->ODR)) - PERIPH_BASE) << 5) + ((PIN) << 2)))

#define IPNUT_PIN_STATE(a)   GPIO_PIN_ISTATE(inputPort[a], inputPin[a])
#define IPNUT_PIN_SET(a)   GPIO_PIN_ISET(inputPort[a], inputPin[a])


typedef struct
{
   uint8_t bt_time;           // Переменная времени работы автомата
   uint8_t bt_release_time;   // Переменная времени работы автомата
   uint8_t bt_result;         // Переменная результат. Бит-карта нажатий.
   uint8_t bt_cnt;            // Переменная счетчик нажатий
   mysensor_sensor_t inputMode;
   activeLevel_t activeLevel;
   pull_t pull;
   uint16_t value;
   uint16_t prevValue;
   char * formula;
} gpio_t;

gpio_t gpioContext[arraysize(inputPin)];
devState gpioStatus[arraysize(inputPin)];

static size_t gpio_open(peripheral_t const pxPeripheral);
static size_t gpio_write(peripheral_t const pxPeripheral, const void * pvBuffer, const size_t xBytes);
static size_t gpio_read(peripheral_t const pxPeripheral, void * const pvBuffer, const size_t xBytes);
static size_t set_active(peripheral_t const pxPeripheral, char *pcValue);
static size_t get_active(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t set_mode(peripheral_t const pxPeripheral, char *pcValue);
static size_t get_mode(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t set_activeLevel(peripheral_t const pxPeripheral, char *pcValue);
static size_t get_activeLevel(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t set_pull(peripheral_t const pxPeripheral, char *pcValue);
static size_t get_pull(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t set_formula(peripheral_t const pxPeripheral, char *pcValue);
static size_t get_formula(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t getMinValue(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t getMaxValue(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);
static void MX_ADC1_Init(void);
static ADC_HandleTypeDef hadc1;
static void inputTask(void * pvParameters);
static error_t initInputs(void);
static error_t startPeripheral(Tperipheral_t * peripheral);
static error_t stopPeripheral(Tperipheral_t * peripheral);

volatile const uint32_t * inputState[] =
   { GPIO_PIN_ISTATE(IPNUT_PORT_0, (IPNUT_PIN_0)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_1, (IPNUT_PIN_1)>>1), GPIO_PIN_ISTATE(
      IPNUT_PORT_2, (IPNUT_PIN_2)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_3, (IPNUT_PIN_3)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_4,
         (IPNUT_PIN_4)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_5, (IPNUT_PIN_5)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_6,
            (IPNUT_PIN_6)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_7, (IPNUT_PIN_7)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_8,
               (IPNUT_PIN_8)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_9, (IPNUT_PIN_9)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_10,
                  (IPNUT_PIN_10)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_11, (IPNUT_PIN_11)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_12,
                     (IPNUT_PIN_12)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_13, (IPNUT_PIN_13)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_14,
                        (IPNUT_PIN_14)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_15, (IPNUT_PIN_15)>>1) };

typedef enum
{
   NONE = 0x00, MinValueClick = NONE,
   //Sequence
   SingleShortClick, //0x01 Single short click   (.)
   DoubleShortClick, //0x02 Double short click   (..)
   SingleLongClick,  //0x03 Singl long click     (-)
   ShotAndLongClick, //0x04 Short and long click (.-)
   DoubleLongClick,  //0x05                      (--)
   TripleShortClick, //0x06                      (...)
   ShortShortLongClick, //0x07                   (..-)
   MaxValueClick,
   //Dimmer
   IncreaseValue = 0x11,   //0x11 17
   DecreaseValue,          //0x12 18
   PlatoValue,             //0x13 19
} clicType;

#endif /* MY_CYCLONE_REST_SENSORS_INPUT_H_ */

