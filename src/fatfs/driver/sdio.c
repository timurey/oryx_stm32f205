/**
 ******************************************************************************
 * File Name          : SDIO.c
 * Date               : 10/03/2015 22:05:11
 * Description        : This file provides code for the configuration
 *                      of the SDIO instances.
 ******************************************************************************
 *
 * COPYRIGHT(c) 2015 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "sdio.h"

#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "bsp_driver_sd.h"

#define SD_DMAx_Tx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Rx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Tx_STREAM                 DMA2_Stream6
#define SD_DMAx_Rx_STREAM                 DMA2_Stream3

DMA_HandleTypeDef dmaRxHandle;
DMA_HandleTypeDef dmaTxHandle;
/* USER CODE END 0 */

SD_HandleTypeDef hsd;
HAL_SD_CardInfoTypedef SDCardInfo;

/* SDIO init function */

void MX_SDIO_SD_Init(void)
{

	hsd.Instance = SDIO;
	hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
	hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
	hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
	hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
	hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
	hsd.Init.ClockDiv = SDIO_TRANSFER_CLK_DIV;


}

void HAL_SD_MspInit(SD_HandleTypeDef* hsd)
{

	GPIO_InitTypeDef GPIO_InitStruct;
	if(hsd->Instance==SDIO)
	{
		/* USER CODE BEGIN SDIO_MspInit 0 */
		dmaRxHandle.Init.Channel             = SD_DMAx_Rx_CHANNEL;
		dmaRxHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
		dmaRxHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
		dmaRxHandle.Init.MemInc              = DMA_MINC_ENABLE;
		dmaRxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
		dmaRxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
		dmaRxHandle.Init.Mode                = DMA_PFCTRL;
		dmaRxHandle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
		dmaRxHandle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
		dmaRxHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
		dmaRxHandle.Init.MemBurst            = DMA_MBURST_INC4;
		dmaRxHandle.Init.PeriphBurst         = DMA_PBURST_INC4;
		dmaRxHandle.Instance = SD_DMAx_Rx_STREAM;
		/* Configure DMA Tx parameters */
		dmaTxHandle.Init.Channel             = SD_DMAx_Tx_CHANNEL;
		dmaTxHandle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
		dmaTxHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
		dmaTxHandle.Init.MemInc              = DMA_MINC_ENABLE;
		dmaTxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
		dmaTxHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
		dmaTxHandle.Init.Mode                = DMA_PFCTRL;
		dmaTxHandle.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
		dmaTxHandle.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
		dmaTxHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
		dmaTxHandle.Init.MemBurst            = DMA_MBURST_INC4;
		dmaTxHandle.Init.PeriphBurst         = DMA_PBURST_INC4;
		dmaTxHandle.Instance = SD_DMAx_Tx_STREAM;

		/* USER CODE END SDIO_MspInit 0 */
		/* Peripheral clock enable */
		__SDIO_CLK_ENABLE();
		__GPIOC_CLK_ENABLE();
		__GPIOD_CLK_ENABLE();
		/* Enable DMA2 clocks */
		__DMAx_TxRx_CLK_ENABLE();
		/**SDIO GPIO Configuration
    PC8     ------> SDIO_D0
    PC9     ------> SDIO_D1
    PC10     ------> SDIO_D2
    PC11     ------> SDIO_D3
    PC12     ------> SDIO_CK
    PD2     ------> SDIO_CMD
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
				|GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;//GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_2;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;//GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

		/* USER CODE BEGIN SDIO_MspInit 1 */
		/* Associate the DMA handle */

		__HAL_LINKDMA(hsd, hdmarx, dmaRxHandle);

		/* Deinitialize the stream for new transfer */
		HAL_DMA_DeInit(&dmaRxHandle);

		/* Configure the DMA stream */
		HAL_DMA_Init(&dmaRxHandle);

		/* Associate the DMA handle */
		__HAL_LINKDMA(hsd, hdmatx, dmaTxHandle);

		/* Deinitialize the stream for new transfer */
		HAL_DMA_DeInit(&dmaTxHandle);

		/* Configure the DMA stream */
		HAL_DMA_Init(&dmaTxHandle);

		/* NVIC configuration for DMA transfer complete interrupt */
		HAL_NVIC_SetPriority(SD_DMAx_Rx_IRQn, 8, 0);
		HAL_NVIC_EnableIRQ(SD_DMAx_Rx_IRQn);

		/* NVIC configuration for DMA transfer complete interrupt */
		HAL_NVIC_SetPriority(SD_DMAx_Tx_IRQn, 8, 0);
		HAL_NVIC_EnableIRQ(SD_DMAx_Tx_IRQn);

		/* NVIC configuration for SDIO interrupts */
		HAL_NVIC_SetPriority(SDIO_IRQn, 7, 0);
		HAL_NVIC_EnableIRQ(SDIO_IRQn);
		/* USER CODE END SDIO_MspInit 1 */
	}
}

void HAL_SD_MspDeInit(SD_HandleTypeDef* hsd)
{

	if(hsd->Instance==SDIO)
	{
		/* USER CODE BEGIN SDIO_MspDeInit 0 */

		/* USER CODE END SDIO_MspDeInit 0 */
		/* Peripheral clock disable */
		__SDIO_CLK_DISABLE();

		/**SDIO GPIO Configuration
    PC8     ------> SDIO_D0
    PC9     ------> SDIO_D1
    PC10     ------> SDIO_D2
    PC11     ------> SDIO_D3
    PC12     ------> SDIO_CK
    PD2     ------> SDIO_CMD 
		 */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
				|GPIO_PIN_12);

		HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);

		/* USER CODE BEGIN SDIO_MspDeInit 1 */
		HAL_DMA_Init(&dmaRxHandle);

		/* Deinitialize the stream for new transfer */
		HAL_DMA_DeInit(&dmaTxHandle);
		/* USER CODE END SDIO_MspDeInit 1 */
	}
} 

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
