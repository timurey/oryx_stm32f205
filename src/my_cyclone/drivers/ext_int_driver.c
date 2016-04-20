/**
 * @file ext_int_driver.c
 * @brief Extern Interput driver for stm32f10x & enc28j60
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
#include "stm32f2xx_hal_gpio.h"
#include "stm32f2xx_hal_rcc.h"
#include "stm32f2xx_hal_cortex.h"
#include "core/net.h"
#include "drivers/enc28j60.h"
#include "ext_int_driver.h"
#include "debug.h"

void EXTI1_IRQHandler(void);

/**
 * @brief External interrupt line driver
 **/

const ExtIntDriver extIntDriver =
{
	extIntInit,
	extIntEnableIrq,
	extIntDisableIrq
};


/**
 * @brief EXTI configuration
 * @return Error code
 **/

error_t extIntInit(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	/* Enable GPIOA clock */
	__GPIOA_CLK_ENABLE();

	/* Configure PA1 pin as input floating */
	GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Pin = GPIO_PIN_1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Enable and set EXTI Line0 Interrupt to the medium priority */
	HAL_NVIC_SetPriority(EXTI1_IRQn, 0x0f, 0x0f);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);



	//Successful processing
	return NO_ERROR;
}


/**
 * @brief Enable external interrupts
 **/

void extIntEnableIrq(void)
{
	//Enable interrupts
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}


/**
 * @brief Disable external interrupts
 **/

void extIntDisableIrq(void)
{
	//Disable EXTI1_IRQn interrupt
	HAL_NVIC_DisableIRQ(EXTI1_IRQn);
}


/**
 * @brief External interrupt handler
 **/

void EXTI1_IRQHandler(void)
{
	portBASE_TYPE flag;
	NetInterface *interface;

	//Enter interrupt service routine
	osEnterIsr();

	//Point to the structure describing the network interface
	interface = &netInterface[0];
	//This flag will be set if a higher priority task must be woken
	flag = FALSE;
	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_1) != RESET)
	{
		//Clear interrupt flag
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);
		//A PHY event is pending...
		//Set event flag
//		interface->nicEvent = TRUE;

		//Notify the user that the link state has changed
		flag = enc28j60IrqHandler(interface);
	}
	//Leave interrupt service routine
	osExitIsr(flag);
}
