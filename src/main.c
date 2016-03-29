//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include "stm32f2xx_hal.h"
#include "main.h"

//#include "mem.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"



#include "os_port.h"
#include "network.h"
#include "cli_hal.h"


/*For jsmn
 *
 */
#include "jsmn_extras.h"
char object[]="{\"sensors\":{\"temperature\":{\"onewire\":[{\"serial\":\"28:3A:CF:7B:04:00:00:D3\",\"name\":\"Температура воздуха\",\"place\":\"room\"},{\"serial\":\"10:86:85:9E:02:08:00:77\",\"name\":\"air\",\"place\":\"kitchen\"},{\"serial\":\"28:A7:74:7c:04:00:00:91\",\"name\":\"hot water\",\"place\":\"bath room\"}],\"analog\":[]},\"humidity\":{\"onewire\":[{\"id\":\"12345\",\"name\":\"Humidity\",\"place\":\"living room\"}]},\"inputs\":{\"analog\":[{\"serial\":\"34:51:0D:31:32:39:32:00\",\"name\":\"water level\",\"place\":\"bath\"}],\"digital\":[{\"serial\":\"34:51:0D:31:32:39:32:05\",\"name\":\"bath is full\",\"place\":\"bath\"}],\"dimmer\":[{\"serial\":\"34:51:0D:31:32:39:32:04\",\"name\":\"light dimmer\",\"place\":\"bedroom\"}],\"sequential\":[{\"serial\":\"34:51:0D:31:32:39:32:06\",\"name\":\"door is opened\",\"place\":\"room\"}]}},\"comment\":\"serial is hexademical: 00, 01..09, 0a,0b..0f\"}";
char path[]="$.sensors.temperature.onewire[0].serial";
#define CONFIG_JSMN_NUM_TOKENS 256
#define STRLEN(s) (sizeof(s)/sizeof(s[0]))
static jsmn_parser jSMNparser;
static jsmntok_t jSMNtokens[CONFIG_JSMN_NUM_TOKENS];

error_t configInit(void)
{
   int toknum;
   jsmnerr_t resultCode;

   jsmn_init(&jSMNparser);
   resultCode = jsmn_parse(jSMNparser, &object[0], STRLEN(object), jSMNtokens, CONFIG_JSMN_NUM_TOKENS);
   toknum = findvalue(jSMNtokens, &object[0], STRLEN(object), &path);
   return NO_ERROR;
}

/*
 * End for jsmn
 */
// ----- main() ---------------------------------------------------------------

int main(void)

{
   //   OsTask *task;
   RTC_Init();
   /* Configure the system clock to 120 MHz */
   SystemClock_Config();
   configInit();

   osCreateTask("Blinker", vBlinker, NULL, 38, 4);
   osCreateTask("startup", startup_task, NULL, configMINIMAL_STACK_SIZE*2, 3);
   osCreateTask("Network_Services", networkServices, NULL, configMINIMAL_STACK_SIZE*4, 1);

   //	task = osCreateTask("CLI\t", vCommandInterpreterTask, NULL, configMINIMAL_STACK_SIZE*4, 1);
   osStartKernel();
   while(1)
   {

   }
}

void startup_task (void *pvParameters)
{

   (void) pvParameters;
   MX_GPIO_Init();

   /* Init Device Library */
   USBD_Init(&hUsbDeviceFS, &VCP_Desc, 0);
   /* Add Supported Class */
   USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
   /* Add CDC Interface Class */
   USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);
   /* Start Device Process */
   USBD_Start(&hUsbDeviceFS);

   xdev_out(putchar);


   MX_SDIO_SD_Init();
   FATFS_LinkDriver(&SD_Driver, SD_Path);
   fsInit();


   vTaskDelete(NULL);
}

void vBlinker (void *pvParameters)
{
   (void) pvParameters;
   __GPIOB_CLK_ENABLE();
   GPIO_InitTypeDef GPIO_InitStruct;

   GPIO_InitStruct.Pin = GPIO_PIN_5;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_PULLUP;
   GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

   while(1)
   {

      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
      vTaskDelay(980);

      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
      vTaskDelay(20);
   }
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 120000000
 *            HCLK(Hz)                       = 120000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 4
 *            APB2 Prescaler                 = 2
 *            HSE Frequency(Hz)              = 25000000
 *            PLL_M                          = 25
 *            PLL_N                          = 240
 *            PLL_P                          = 2
 *            PLL_Q                          = 5
 *            VDD(V)                         = 3.3
 *            Flash Latency(WS)              = 3
 * @param  None
 * @retval None
 */

void SystemClock_Config(void)
{

   RCC_ClkInitTypeDef RCC_ClkInitStruct;
   RCC_OscInitTypeDef RCC_OscInitStruct;

   /* Enable HSE Oscillator and activate PLL with HSE as source */
   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
   RCC_OscInitStruct.HSEState = RCC_HSE_ON;
   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
   RCC_OscInitStruct.PLL.PLLM = 25;
   RCC_OscInitStruct.PLL.PLLN = 240;
   RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
   RCC_OscInitStruct.PLL.PLLQ = 5;
   HAL_RCC_OscConfig(&RCC_OscInitStruct);

   /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	     clocks dividers */
   RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
   HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);

}

void DMA2_Stream3_IRQHandler(void)
{
   BSP_SD_DMA_Rx_IRQHandler();
}

/**
 * @brief  This function handles DMA2 Stream 6 interrupt request.
 * @param  None
 * @retval None
 */
void DMA2_Stream6_IRQHandler(void)
{
   BSP_SD_DMA_Tx_IRQHandler();
}

/**
 * @brief  This function handles SDIO interrupt request.
 * @param  None
 * @retval None
 */
void SDIO_IRQHandler(void)
{
   BSP_SD_IRQHandler();
}


