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

//Dependencies


//#include "stm32f10x.h"
//#include "cmsis_os.h"
#include "stm32f2xx_hal_dma.h"
#include "stm32f2xx_hal_spi.h"
#include "stm32f2xx_hal_gpio.h"
#include "stm32f2xx_hal_rcc.h"
#include "stm32f2xx_hal.h"
#include "core/net.h"
#include "spi_driver.h"
#include "debug.h"

SPI_HandleTypeDef hspi1;

#ifdef SPI_USE_DMA

DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

#endif
/**
 * @brief SPI driver
 **/

const SpiDriver spiDriver =
{
		spiInit,
		spiSetMode,
		spiSetBitrate,
		spiAssertCs,
		spiDeassertCs,
		spiTransfer
};

static void MX_SPI1_Init(void);

static void Delay_ms(uint32_t ms)
{
	//	HAL_Delay(ms);
	volatile uint32_t nCount, RCC_Clocks;
	RCC_Clocks= HAL_RCC_GetHCLKFreq();
	nCount=(RCC_Clocks/1000)*ms;
	for (; nCount!=0; nCount--);
}

/**
 * @brief SPI initialization
 * @return Error code
 **/

error_t spiInit(void)
{


	// Initialize GPIO:
	__GPIOA_CLK_ENABLE();
	__GPIOE_CLK_ENABLE();
	__SPI1_CLK_ENABLE();

	MX_SPI1_Init();
	GPIO_InitTypeDef GPIO_InitStruct;

	// CS
	GPIO_InitStruct.Pin = GPIO_Pin_CS_ENC28J60;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIO_ENC28J60_CS, &GPIO_InitStruct);

	// RESET
	GPIO_InitStruct.Pin = GPIO_Pin_RESET_ENC28J60;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FAST;

	HAL_GPIO_Init(GPIO_ENC28J60_RESET, &GPIO_InitStruct);

	//INT
	GPIO_InitStruct.Pin = GPIO_Pin_INT_ENC28J60;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = GPIO_SPEED_FAST;

	HAL_GPIO_Init(GPIO_ENC28J60_INT, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIO_ENC28J60_RESET, GPIO_Pin_RESET_ENC28J60, GPIO_PIN_RESET);
	Delay_ms(10);
	HAL_GPIO_WritePin(GPIO_ENC28J60_RESET, GPIO_Pin_RESET_ENC28J60, GPIO_PIN_SET);
	Delay_ms(10);



#ifdef DMA
	//Initing DMA for SPI
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	/* Deinitialize DMA Streams */
	DMA_DeInit(DMA1_Channel2);	//SPI1_TX_DMA_STREAM
	DMA_DeInit(DMA1_Channel3);	//SPI1_RX_DMA_STREAM

#endif

	//Successful processing
	return NO_ERROR;
}


/**
 * @brief Set SPI mode
 * @param mode SPI mode (0, 1, 2 or 3)
 **/

error_t spiSetMode(uint_t mode)
{
   (void) mode;
#ifdef SPI_USE_DMA
	switch (mode)
	{

	case spiModeDmaRx:
		/* Peripheral DMA init*/
		hdma_spi1_tx.Init.MemInc = DMA_MINC_DISABLE;
		HAL_DMA_Init(&hdma_spi1_tx);
		break;
	case spiModeDmaTx:
		hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
		HAL_DMA_Init(&hdma_spi1_tx);
		break;
	default:
		return ERROR_UNSUPPORTED_CONFIGURATION;
	}
#endif
	//Not implemented
	return NO_ERROR;
}


/**
 * @brief Set SPI bitrate
 * @param bitrate Bitrate value
 **/

error_t spiSetBitrate(uint_t bitrate)
{
	//Not implemented
   (void) bitrate;
	return ERROR_NOT_IMPLEMENTED;
}


/**
 * @brief Assert CS
 **/

void spiAssertCs(void)
{
	HAL_GPIO_WritePin(GPIO_ENC28J60_CS, GPIO_Pin_CS_ENC28J60, GPIO_PIN_RESET);
	//usleep(1);
}


/**
 * @brief Deassert CS
 **/

void spiDeassertCs(void)
{
	//usleep(1);
	HAL_GPIO_WritePin(GPIO_ENC28J60_CS, GPIO_Pin_CS_ENC28J60, GPIO_PIN_SET);
	//usleep(1);
}


/**
 * @brief Transfer a single byte
 * @param[in] data The data to be written
 * @return The data received from the slave device
 **/

uint8_t spiTransfer(uint8_t tx_data)
{
	uint8_t rx_data;

	HAL_SPI_TransmitReceive(&hspi1, &tx_data, &rx_data, sizeof(rx_data), 0x10);

	return rx_data;
}

static void MX_SPI1_Init(void)
{

	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLED;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	hspi1.Init.CRCPolynomial = 7;
	HAL_SPI_Init(&hspi1);

}

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if(hspi->Instance==SPI1)
	{

		/**SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* Peripheral DMA init*/
#ifdef SPI_USE_DMA
		__DMA2_CLK_ENABLE();
		hdma_spi1_rx.Instance = DMA2_Stream0;
		hdma_spi1_rx.Init.Channel = DMA_CHANNEL_3;
		hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
		hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_spi1_rx.Init.Mode = DMA_NORMAL;
		hdma_spi1_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
		hdma_spi1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

		HAL_DMA_Init(&hdma_spi1_rx);

		__HAL_LINKDMA(hspi,hdmarx,hdma_spi1_rx);

		hdma_spi1_tx.Instance = DMA2_Stream5;
		hdma_spi1_tx.Init.Channel = DMA_CHANNEL_3;
		hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_spi1_tx.Init.MemInc = DMA_MINC_DISABLE;
		hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_spi1_tx.Init.Mode = DMA_NORMAL;
		hdma_spi1_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
		hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		HAL_DMA_Init(&hdma_spi1_tx);

		__HAL_LINKDMA(hspi,hdmatx,hdma_spi1_tx);

		HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 7, 5);
		HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);

		/* NVIC configuration for DMA transfer complete interrupt (SPI3_RX) */
		HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 7, 5);
		HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
#endif
	}
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{

	if(hspi->Instance==SPI1)
	{
		__SPI1_CLK_DISABLE();

		/**SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
		 */
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

		/* Peripheral DMA DeInit*/
		HAL_DMA_DeInit(hspi->hdmarx);
		HAL_DMA_DeInit(hspi->hdmatx);
	}
}


