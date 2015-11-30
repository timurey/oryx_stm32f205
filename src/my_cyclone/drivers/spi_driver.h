/**
 * @file spi_driver.c
 * @brief SPI driver for stm32f10x
 *
 * @section License
 *
 * Copyright (C) 2010-2014 Oryx Embedded. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Timur Taipov
 * @version 0.0.1
 **/

#ifndef __SPI_DRIVER__
#define __SPI_DRIVER__
#include "stm32f2xx_hal_dma.h"
#include "stm32f2xx_hal_spi.h"

#define SPI_USE_DMA

#define SPI_ENC28J60                        SPI1
#define GPIO_ENC28J60_CS                    GPIOA
#define RCC_APB2Periph_GPIO_ENC28J60_CS     RCC_APB2Periph_GPIOA
#define GPIO_Pin_CS_ENC28J60                GPIO_PIN_4
#define RCC_APBPeriphClockCmd_CS_ENC28J60   RCC_APB2PeriphClockCmd

#define GPIO_ENC28J60_RESET                   GPIOE
#define RCC_APB2Periph_GPIO_ENC28J60_RESET    RCC_APB2Periph_GPIOE
#define GPIO_Pin_RESET_ENC28J60               GPIO_PIN_1
#define RCC_APBPeriphClockCmd_RESET_ENC28J60  RCC_APB2PeriphClockCmd

#define GPIO_ENC28J60_INT                   GPIOA
#define RCC_APB2Periph_GPIO_ENC28J60_INT    RCC_APB2Periph_GPIOA
#define GPIO_Pin_INT_ENC28J60               GPIO_PIN_1
#define RCC_APBPeriphClockCmd_INT_ENC28J60  RCC_APB2PeriphClockCmd


#define GPIO_SPI_ENC28J60                   GPIOA
#define GPIO_Pin_SPI_ENC28J60_SCK           GPIO_PIN_5
#define GPIO_Pin_SPI_ENC28J60_MISO          GPIO_PIN_6
#define GPIO_Pin_SPI_ENC28J60_MOSI          GPIO_PIN_7
#define RCC_APBPeriphClockCmd_SPI_ENC28J60  RCC_APB2PeriphClockCmd
#define RCC_APBPeriph_SPI_ENC28J60          RCC_APB2Periph_SPI1

#define SPI_BaudRatePrescaler_SPI_SD  SPI_BaudRatePrescaler_2

extern const SpiDriver spiDriver;
extern SPI_HandleTypeDef hspi1;

typedef enum {
	spiModeRxTx = 0,
	spiModeDmaRx,
	spiModeDmaTx
}spiTxRxMode_t;

error_t spiInit(void);
error_t spiSetMode(uint_t mode);
error_t spiSetBitrate(uint_t bitrate);
void spiAssertCs(void);
void spiDeassertCs(void);
uint8_t spiTransfer(uint8_t tx_data);


#endif //__SPI_DRIVER__
