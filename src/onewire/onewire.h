/*
 * onewire.h
 *
 *  Version 1.0.3
 */

#ifndef ONEWIRE_H_
#define ONEWIRE_H_

#include "onewire_conf.h"
// ��� ������ ����������� ����������� ��������� ������� OW_Init
// �� ������� ������������ ����� USART
#include "stm32f205xx.h"
#include "stm32f2xx_hal.h"
#include "stm32f2xx_hal_rcc.h"

#include "stm32f2xx_hal_dma.h"
#include "stm32f2xx_hal_gpio.h"
#include "stm32f2xx_hal_uart.h"
#include "error.h"
#include "rest/sensors.h"
#include <stddef.h>


//#define  ONLINE   0x01  /* sensor is online */
//#define  MANAGED  0x02  /* sensor is managed */
//#define  READEBLE 0x04  /* simple mutex for sensor */

typedef struct
{
   union
   {
      uint8_t type;
      uint8_t serial[ONEWIRE_SERIAL_LENGTH];
   };
} __attribute__((__packed__)) OwSerial_t;

typedef struct
{
   OwSerial_t *serial;
   float *value;
   uint8_t *status;
   uint8_t *id;
   sensor_t *sensor;
} OwSensor_t;


OwSensor_t oneWireSensors[MAX_ONEWIRE_COUNT];
OwSerial_t foundedSerial[MAX_ONEWIRE_COUNT];
int oneWireFoundedDevices;
#if (OW_DS1820_SUPPORT == ENABLE)

#include "ds1820/ds1820.h"
#else
#define MAX_DS1820_COUNT 0
#endif



// ���� ����� �������� ���� FreeRTOS, �� �����������������
//#define OW_GIVE_TICK_RTOS

// ������ �������� ������� OW_Send
#define OW_SEND_RESET		1
#define OW_NO_RESET		2

// ������ �������� �������
#define OW_OK			0
#define OW_ERROR		1
#define OW_NO_DEVICE	2

#define OW_NO_READ		0xff

#define OW_READ_SLOT	0xff

#ifdef OW_USART1

#undef OW_USART2
#undef OW_USART3

#define USARTx                           USART1
#define USARTx_CLK_ENABLE()              __USART1_CLK_ENABLE();
#define DMAx_CLK_ENABLE()                __DMA2_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __USART1_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __USART1_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTx_TX_PIN                    GPIO_PIN_9
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_TX_AF                     GPIO_AF7_USART1

/* Definition for USARTx's DMA */
#define USARTx_TX_DMA_CHANNEL             DMA_CHANNEL_4
#define USARTx_TX_DMA_STREAM              DMA2_Stream7
#define USARTx_RX_DMA_CHANNEL             DMA_CHANNEL_4
#define USARTx_RX_DMA_STREAM              DMA2_Stream2

/* Definition for USARTx's NVIC */
#define USARTx_DMA_TX_IRQn                DMA2_Stream7_IRQn
#define USARTx_DMA_RX_IRQn                DMA2_Stream2_IRQn
#define USARTx_DMA_TX_IRQHandler          DMA2_Stream7_IRQHandler
#define USARTx_DMA_RX_IRQHandler          DMA2_Stream2_IRQHandler

#endif

#ifdef OW_USART2

#undef OW_USART1
#undef OW_USART3

#define USARTx                           USART2
#define USARTx_CLK_ENABLE()              __USART2_CLK_ENABLE();
#define DMAx_CLK_ENABLE()                __DMA1_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __USART2_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __USART2_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTx_TX_PIN                    GPIO_PIN_2
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_TX_AF                     GPIO_AF7_USART2

/* Definition for USARTx's DMA */
#define USARTx_TX_DMA_CHANNEL             DMA_CHANNEL_4
#define USARTx_TX_DMA_STREAM              DMA1_Stream6
#define USARTx_RX_DMA_CHANNEL             DMA_CHANNEL_4
#define USARTx_RX_DMA_STREAM              DMA1_Stream5

/* Definition for USARTx's NVIC */
#define USARTx_DMA_TX_IRQn                DMA1_Stream6_IRQn
#define USARTx_DMA_RX_IRQn                DMA1_Stream5_IRQn
#define USARTx_DMA_TX_IRQHandler          DMA1_Stream6_IRQHandler
#define USARTx_DMA_RX_IRQHandler          DMA1_Stream5_IRQHandler

#endif

#ifdef OW_USART3

#undef OW_USART1
#undef OW_USART2

#define USARTx                           USART3
#define USARTx_CLK_ENABLE()              __USART3_CLK_ENABLE();
#define DMAx_CLK_ENABLE()                __DMA1_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __GPIOB_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __USART3_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __USART3_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTx_TX_PIN                    GPIO_PIN_10
#define USARTx_TX_GPIO_PORT              GPIOB
#define USARTx_TX_AF                     GPIO_AF7_USART3

/* Definition for USARTx's DMA */
#define USARTx_TX_DMA_CHANNEL             DMA_CHANNEL_4
#define USARTx_TX_DMA_STREAM              DMA1_Stream3
#define USARTx_RX_DMA_CHANNEL             DMA_CHANNEL_4
#define USARTx_RX_DMA_STREAM              DMA1_Stream1

/* Definition for USARTx's NVIC */
#define USARTx_DMA_TX_IRQn                DMA1_Stream3_IRQn
#define USARTx_DMA_RX_IRQn                DMA1_Stream1_IRQn
#define USARTx_DMA_TX_IRQHandler          DMA1_Stream3_IRQHandler
#define USARTx_DMA_RX_IRQHandler          DMA1_Stream1_IRQHandler

#endif
uint8_t OW_Init(void);
uint8_t OW_Reset(void);
uint8_t OW_Scan(OwSerial_t *buf, uint8_t num);
uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen, uint8_t *data, uint8_t dLen, uint8_t readStart);

void USARTx_DMA_RX_IRQHandler(void);
void USARTx_DMA_TX_IRQHandler(void);


void oneWireTask(void *pvParameters);
uint8_t oneWireCRC(uint8_t *data, uint8_t len);
uint8_t oneWireHardwareInit(void);

typedef error_t (*tScanCommandHandler)(void);
typedef error_t (*tGrupCommandHandler)(void);
typedef error_t (*tIndividualCommandHandler)(void);

typedef struct
{
   tScanCommandHandler oneWireScanHadler;
   tGrupCommandHandler oneWireGroupHadler;
   tIndividualCommandHandler onewireIndividualHadler;

} oneWireFunctions;

#define register_onewire_function(name, scan_command, grup_command, individual_command) const oneWireFunctions handler_##name __attribute__ ((section ("onewire_functions"))) = \
   { \
   .oneWireScanHadler = scan_command, \
   .oneWireGroupHadler = grup_command, \
   .onewireIndividualHadler = individual_command \
   }
extern oneWireFunctions __start_onewire_functions; //предоставленный линкером символ начала секции onewire_functions
extern oneWireFunctions __stop_onewire_functions; //предоставленный линкером символ конца секции onewire_functions

extern uint16_t onewireHealth;
#endif /* ONEWIRE_H_ */
