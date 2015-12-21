/*
 * onewire.c
 *
 *  Version 1.0.3
 */

#include "onewire.h"
#include <ctype.h>
#include "stdio.h"
#include "string.h"


UART_HandleTypeDef UartHandle;

// Буфер для приема/передачи по 1-wire
uint8_t ow_buf[8];
OsMutex oneWireMutex;
extern sensor_t sensors[MAX_ONEWIRE_COUNT];
#if (OW_DS1820_SUPPORT == ENABLE)
ds1820Scratchpad_t ds1820Buf;
#endif


#define OW_0	0x00
#define OW_1	0xff
#define OW_R_1	0xff

//-----------------------------------------------------------------------------
// функция преобразует один байт в восемь, для передачи через USART
// ow_byte - байт, который надо преобразовать
// ow_bits - ссылка на буфер, размером не менее 8 байт
//-----------------------------------------------------------------------------
static void OW_toBits(uint8_t ow_byte, uint8_t *ow_bits) {
   uint8_t i;
   for (i = 0; i < 8; i++) {
      if (ow_byte & 0x01) {
         *ow_bits = OW_1;
      } else {
         *ow_bits = OW_0;
      }
      ow_bits++;
      ow_byte = ow_byte >> 1;
   }
}

//-----------------------------------------------------------------------------
// обратное преобразование - из того, что получено через USART опять собирается байт
// ow_bits - ссылка на буфер, размером не менее 8 байт
//-----------------------------------------------------------------------------
static uint8_t OW_toByte(uint8_t *ow_bits) {
   uint8_t ow_byte, i;
   ow_byte = 0;
   for (i = 0; i < 8; i++) {
      ow_byte = ow_byte >> 1;
      if (*ow_bits == OW_R_1) {
         ow_byte |= 0x80;
      }
      ow_bits++;
   }

   return ow_byte;
}

//-----------------------------------------------------------------------------
// инициализирует USART и DMA
//-----------------------------------------------------------------------------
uint8_t oneWireHardwareInit(void) {
   UartHandle.Instance = USARTx;
   UartHandle.Init.BaudRate = 115200;
   UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
   UartHandle.Init.StopBits = UART_STOPBITS_1;
   UartHandle.Init.Parity = UART_PARITY_NONE;
   UartHandle.Init.Mode = UART_MODE_TX_RX;
   UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
   UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
   HAL_HalfDuplex_Init(&UartHandle);
   return OW_OK;
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
   static DMA_HandleTypeDef hdma_tx;
   static DMA_HandleTypeDef hdma_rx;

   GPIO_InitTypeDef  GPIO_InitStruct;

   /*##-1- Enable peripherals and GPIO Clocks #################################*/
   /* Enable GPIO clock */
   USARTx_TX_GPIO_CLK_ENABLE();
   /* Enable USARTx clock */
   USARTx_CLK_ENABLE();
   /* Enable DMA2 clock */
   DMAx_CLK_ENABLE();

   /*##-2- Configure peripheral GPIO ##########################################*/
   /* UART TX GPIO pin configuration  */
   GPIO_InitStruct.Pin       = USARTx_TX_PIN;
   GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
   GPIO_InitStruct.Pull      = GPIO_PULLUP;
   GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
   GPIO_InitStruct.Alternate = USARTx_TX_AF;

   HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);


   /*##-3- Configure the DMA streams ##########################################*/
   /* Configure the DMA handler for Transmission process */
   hdma_tx.Instance                 = USARTx_TX_DMA_STREAM;

   hdma_tx.Init.Channel             = USARTx_TX_DMA_CHANNEL;
   hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
   hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
   hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
   hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
   hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
   hdma_tx.Init.Mode                = DMA_NORMAL;
   hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
   hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
   hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
   hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;
   hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;

   HAL_DMA_Init(&hdma_tx);

   /* Associate the initialized DMA handle to the UART handle */
   __HAL_LINKDMA(huart, hdmatx, hdma_tx);

   /* Configure the DMA handler for reception process */
   hdma_rx.Instance                 = USARTx_RX_DMA_STREAM;

   hdma_rx.Init.Channel             = USARTx_RX_DMA_CHANNEL;
   hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
   hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
   hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
   hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
   hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
   hdma_rx.Init.Mode                = DMA_NORMAL;
   hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
   hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
   hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
   hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
   hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;

   HAL_DMA_Init(&hdma_rx);

   /* Associate the initialized DMA handle to the the UART handle */
   __HAL_LINKDMA(huart, hdmarx, hdma_rx);

   /*##-4- Configure the NVIC for DMA #########################################*/
   /* NVIC configuration for DMA transfer complete interrupt (USARTx_TX) */
   HAL_NVIC_SetPriority(USARTx_DMA_TX_IRQn, 0x0f, 1);
   HAL_NVIC_EnableIRQ(USARTx_DMA_TX_IRQn);

   /* NVIC configuration for DMA transfer complete interrupt (USARTx_RX) */
   HAL_NVIC_SetPriority(USARTx_DMA_RX_IRQn, 0x0f, 0);
   HAL_NVIC_EnableIRQ(USARTx_DMA_RX_IRQn);
}


/**
 * @brief UART MSP De-Initialization
 *        This function frees the hardware resources used in this example:
 *          - Disable the Peripheral's clock
 *          - Revert GPIO, DMA and NVIC configuration to their default state
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
   (void) huart;
   static DMA_HandleTypeDef hdma_tx;
   static DMA_HandleTypeDef hdma_rx;

   /*##-1- Reset peripherals ##################################################*/
   USARTx_FORCE_RESET();
   USARTx_RELEASE_RESET();

   /*##-2- Disable peripherals and GPIO Clocks #################################*/
   /* Configure UART Tx as alternate function  */
   HAL_GPIO_DeInit(USARTx_TX_GPIO_PORT, USARTx_TX_PIN);

   /*##-3- Disable the DMA Streams ############################################*/
   /* De-Initialize the DMA Stream associate to transmission process */
   HAL_DMA_DeInit(&hdma_tx);
   /* De-Initialize the DMA Stream associate to reception process */
   HAL_DMA_DeInit(&hdma_rx);

   /*##-4- Disable the NVIC for DMA ###########################################*/
   HAL_NVIC_DisableIRQ(USARTx_DMA_TX_IRQn);
   HAL_NVIC_DisableIRQ(USARTx_DMA_RX_IRQn);
}
//-----------------------------------------------------------------------------
// осуществляет сброс и проверку на наличие устройств на шине
//-----------------------------------------------------------------------------
uint8_t OW_Reset(void) {
   uint8_t ow_presence=0xf0;

   UartHandle.Init.BaudRate = 9600;

   HAL_HalfDuplex_Init(&UartHandle);

   // отправляем 0xf0 на скорости 9600
   HAL_UART_Transmit(&UartHandle, &ow_presence, 1, 10);

   while (UartHandle.State != HAL_UART_STATE_READY) {
#ifdef OW_GIVE_TICK_RTOS
      taskYIELD();
#endif
   }
   HAL_UART_Receive(&UartHandle,&ow_presence,1,10);

   UartHandle.Init.BaudRate = 115200;
   HAL_HalfDuplex_Init(&UartHandle);

   if (ow_presence != 0xf0) {
      return OW_OK;
   }

   return OW_NO_DEVICE;
}

// внутренняя процедура. Записывает указанное число бит
static void OW_SendBits(uint8_t num_bits) {

   HAL_UART_Receive_DMA(&UartHandle, (uint8_t *)ow_buf, num_bits);

   HAL_UART_Transmit_DMA(&UartHandle, (uint8_t *)ow_buf, num_bits);

   while (UartHandle.State != HAL_UART_STATE_READY) {
#ifdef OW_GIVE_TICK_RTOS
      taskYIELD();
#endif
   }

}
//-----------------------------------------------------------------------------
// процедура общения с шиной 1-wire
// sendReset - посылать RESET в начале общения.
// 		OW_SEND_RESET или OW_NO_RESET
// command - массив байт, отсылаемых в шину. Если нужно чтение - отправляем OW_READ_SLOT
// cLen - длина буфера команд, столько байт отошлется в шину
// data - если требуется чтение, то ссылка на буфер для чтения
// dLen - длина буфера для чтения. Прочитается не более этой длины
// readStart - с какого символа передачи начинать чтение (нумеруются с 0)
//		можно указать OW_NO_READ, тогда можно не задавать data и dLen
//-----------------------------------------------------------------------------
uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen,
   uint8_t *data, uint8_t dLen, uint8_t readStart) {

   // если требуется сброс - сбрасываем и проверяем на наличие устройств
   if (sendReset == OW_SEND_RESET) {
      if (OW_Reset() == OW_NO_DEVICE) {
         return OW_NO_DEVICE;
      }
   }

   while (cLen > 0) {

      OW_toBits(*command, ow_buf);
      command++;
      cLen--;

      OW_SendBits(8);

      // если прочитанные данные кому-то нужны - выкинем их в буфер
      if (readStart == 0 && dLen > 0) {
         *data = OW_toByte(ow_buf);
         data++;
         dLen--;
      } else {
         if (readStart != OW_NO_READ) {
            readStart--;
         }
      }
   }

   return OW_OK;
}
//-----------------------------------------------------------------------------
// Данная функция осуществляет сканирование сети 1-wire и записывает найденные
//   ID устройств в массив buf, по 8 байт на каждое устройство.
// переменная num ограничивает количество находимых устройств, чтобы не переполнить
// буфер.
//-----------------------------------------------------------------------------
uint8_t OW_Scan(OwSerial_t *buf, uint8_t num) {

   uint8_t found = 0;
   uint8_t *lastDevice;
   uint8_t *curDevice = &buf->serial[0];
   uint8_t numBit, lastCollision, currentCollision, currentSelection;

   lastCollision = 0;
   while (found < num) {
      numBit = 1;
      currentCollision = 0;

      // посылаем команду на поиск устройств
      OW_Send(OW_SEND_RESET, (uint8_t*)"\xf0", 1, 0, 0, OW_NO_READ);

      for (numBit = 1; numBit <= 64; numBit++) {
         // читаем два бита. Основной и комплементарный
         OW_toBits(OW_READ_SLOT, ow_buf);
         OW_SendBits(2);

         if (ow_buf[0] == OW_R_1) {
            if (ow_buf[1] == OW_R_1) {
               // две единицы, где-то провтыкали и заканчиваем поиск
               return found;
            } else {
               // 10 - на данном этапе только 1
               currentSelection = 1;
            }
         } else {
            if (ow_buf[1] == OW_R_1) {
               // 01 - на данном этапе только 0
               currentSelection = 0;
            } else {
               // 00 - коллизия
               if (numBit < lastCollision) {
                  // идем по дереву, не дошли до развилки
                  //todo: check this code (operand &)
                  if (lastDevice[(numBit - 1) >> 3]
                                 & 1 << ((numBit - 1) & 0x07)) {
                     // (numBit-1)>>3 - номер байта
                     // (numBit-1)&0x07 - номер бита в байте
                     currentSelection = 1;

                     // если пошли по правой ветке, запоминаем номер бита
                     if (currentCollision < numBit) {
                        currentCollision = numBit;
                     }
                  } else {
                     currentSelection = 0;
                  }
               } else {
                  if (numBit == lastCollision) {
                     currentSelection = 0;
                  } else {
                     // идем по правой ветке
                     currentSelection = 1;

                     // если пошли по правой ветке, запоминаем номер бита
                     if (currentCollision < numBit) {
                        currentCollision = numBit;
                     }
                  }
               }
            }
         }

         if (currentSelection == 1) {
            curDevice[(numBit - 1) >> 3] |= 1 << ((numBit - 1) & 0x07);
            OW_toBits(0x01, ow_buf);
         } else {
            curDevice[(numBit - 1) >> 3] &= ~(1 << ((numBit - 1) & 0x07));
            OW_toBits(0x00, ow_buf);
         }
         OW_SendBits(1);
      }
      found++;
      lastDevice = curDevice;
      curDevice += 8;
      if (currentCollision == 0)
         return found;

      lastCollision = currentCollision;
   }

   return found;
}

//---------------------------------------------------------------------------
// Обработчик прерываний от DMA
// Функция переопределена в onewire.h и относится к используюемому каналу DMA
//---------------------------------------------------------------------------
void USARTx_DMA_RX_IRQHandler(void)
{
   HAL_DMA_IRQHandler(UartHandle.hdmarx);
}

//---------------------------------------------------------------------------
// Обработчик прерываний от DMA
// Функция переопределена в onewire.h и относится к используюемому каналу DMA
//---------------------------------------------------------------------------
void USARTx_DMA_TX_IRQHandler(void)
{
   HAL_DMA_IRQHandler(UartHandle.hdmatx);
}



static void onewireCompare(void)
{
   int i,j;
   uint8_t online;
   OwSensor_t *currentOneWireSensor = &oneWireSensors[0];


   for (j=0; j<MAX_NUM_SENSORS; j++)
   {
      if (sensors[j].driver == D_ONEWIRE)
      {
         online=FALSE;
         for (i=0; i< oneWireFoundedDevices; i++)
         {
            if (foundedSerial[i].serial[0] == sensors[j].serial[0] &&
               foundedSerial[i].serial[1] == sensors[j].serial[1] &&
               foundedSerial[i].serial[2] == sensors[j].serial[2] &&
               foundedSerial[i].serial[3] == sensors[j].serial[3] &&
               foundedSerial[i].serial[4] == sensors[j].serial[4] &&
               foundedSerial[i].serial[5] == sensors[j].serial[5] &&
               foundedSerial[i].serial[6] == sensors[j].serial[6] &&
               foundedSerial[i].serial[7] == sensors[j].serial[7]
            )
            {
               if (!(foundedSerial[i].serial[0] == 0 &&
                  foundedSerial[i].serial[1] == 0 &&
                  foundedSerial[i].serial[2] == 0 &&
                  foundedSerial[i].serial[3] == 0 &&
                  foundedSerial[i].serial[4] == 0 &&
                  foundedSerial[i].serial[5] == 0 &&
                  foundedSerial[i].serial[6] == 0 &&
                  foundedSerial[i].serial[7] == 0)
               )
               {
                  currentOneWireSensor->serial = (OwSerial_t *)sensors[j].serial;
                  currentOneWireSensor->status = &sensors[j].status;
                  *(currentOneWireSensor->status) |= (ONLINE);
                  currentOneWireSensor->value = &sensors[j].value.fVal;
                  currentOneWireSensor->id = &sensors[j].id;
                  currentOneWireSensor->sensor = &sensors[j];
                  currentOneWireSensor++;
                  online=TRUE;
                  break;
               }
            }


         }
         if (online!=TRUE)
         { //sensor not found on line
            sensorsHealthDecValue(&sensors[j]);
         }
      }
   }
}

void oneWireTask(void *pvParameters)
{
   int i;
   (void) pvParameters;
   oneWireHardwareInit();

   for (i=0; i< MAX_ONEWIRE_COUNT; i++)
   {
      oneWireSensors[i].serial = NULL;
      oneWireSensors[i].value = NULL;
      oneWireSensors[i].status = NULL;
      oneWireSensors[i].id = NULL;
   }
   while (1)
   {
      for (i=0; i< MAX_ONEWIRE_COUNT; i++)
      {
         if (oneWireSensors[i].status!=NULL)
         {
            *oneWireSensors[i].status &=~ONLINE;
         }
      }
      memset (&foundedSerial[0], 0x00, sizeof(OwSerial_t) * MAX_ONEWIRE_COUNT);
      oneWireFoundedDevices = OW_Scan(&foundedSerial[0], MAX_ONEWIRE_COUNT);
      if(oneWireFoundedDevices>0)
      {
         onewireCompare();
         for (oneWireFunctions *curOwFunc = &__start_onewire_functions; curOwFunc < &__stop_onewire_functions; curOwFunc++)
         {
            if(curOwFunc->oneWireScanHadler!=NULL)
            {
               curOwFunc->oneWireScanHadler();
            }
            if(curOwFunc->oneWireGroupHadler!=NULL)
            {
               curOwFunc->oneWireGroupHadler();
            }

            if(curOwFunc->onewireIndividualHadler!=NULL)
            {
               curOwFunc->onewireIndividualHadler();
            }
         }
      }
      for (i=0; i< MAX_ONEWIRE_COUNT; i++)
      {
         if (oneWireSensors[i].status!=NULL)
         {
            *oneWireSensors[i].status |= READEBLE;
         }
         else
         {
            mtCOVERAGE_TEST_MARKER();
         }
      }
      //      vTaskDelay(2000);
   }

}

uint8_t oneWireCRC(uint8_t *data, uint8_t len)
{
   uint8_t crc = 0, n, c;

   while(len--)
   {
      c = *(data++);
      for(n = 8; n; --n)
      {
         if( (crc ^ c) & 1 )
         {
            crc ^= 0x18;
            crc >>= 1;
            crc |= 0x80;
         }
         else
         {
            crc >>= 1;
         }
         c >>= 1;
      }
   }
   return crc;
}

