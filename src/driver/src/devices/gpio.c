/*
 * input.c
 *
 *  Created on: 29 авг. 2015 г.
 *      Author: timurtaipov
 */
#include "gpio.h"
#include "os_port.h"
#include "DriverInterface.h"
#include "../expression_parser/expression_parser.h"
#include <math.h>

static OsTask *pInputTask;
int adc_used = 0;

static  uint16_t inputPin[]={IPNUT_PIN_0, IPNUT_PIN_1, IPNUT_PIN_2, IPNUT_PIN_3,
   IPNUT_PIN_4, IPNUT_PIN_5, IPNUT_PIN_6, IPNUT_PIN_7, IPNUT_PIN_8, IPNUT_PIN_9,
   IPNUT_PIN_10, IPNUT_PIN_11, IPNUT_PIN_12, IPNUT_PIN_13, IPNUT_PIN_14, IPNUT_PIN_15};

static  GPIO_TypeDef* inputPort[]={ IPNUT_PORT_0, IPNUT_PORT_1, IPNUT_PORT_2, IPNUT_PORT_3,
   IPNUT_PORT_4, IPNUT_PORT_5, IPNUT_PORT_6, IPNUT_PORT_7, IPNUT_PORT_8, IPNUT_PORT_9,
   IPNUT_PORT_10, IPNUT_PORT_11, IPNUT_PORT_12, IPNUT_PORT_13, IPNUT_PORT_14, IPNUT_PORT_15};

static  uint32_t adcChannel[]={INPUT_ADC_CHANNEL_0, INPUT_ADC_CHANNEL_1, INPUT_ADC_CHANNEL_2, INPUT_ADC_CHANNEL_3,
   INPUT_ADC_CHANNEL_4, INPUT_ADC_CHANNEL_5, INPUT_ADC_CHANNEL_6, INPUT_ADC_CHANNEL_7, INPUT_ADC_CHANNEL_8};

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
devStatusAttributes gpioStatus[arraysize(inputPin)];
OsMutex gpioMutexes[arraysize(inputPin)];



static size_t gpio_open(peripheral_t * const pxPeripheral);
static size_t gpio_write(peripheral_t * const pxPeripheral, const void * pvBuffer, const size_t xBytes);
static size_t gpio_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes);
static size_t set_active( peripheral_t const pxPeripheral, char *pcValue );
static size_t get_active(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t set_mode(peripheral_t  const pxPeripheral, char *pcValue);
static size_t get_mode(peripheral_t  const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t set_activeLevel(peripheral_t  const pxPeripheral, char *pcValue);
static size_t get_activeLevel(peripheral_t  const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t set_pull(peripheral_t  const pxPeripheral, char *pcValue);
static size_t get_pull(peripheral_t  const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t set_formula(peripheral_t  const pxPeripheral, char *pcValue);
static size_t get_formula(peripheral_t  const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t getMinValue(peripheral_t  const pxPeripheral, char *pcValue, const size_t xBytes);
static size_t getMaxValue(peripheral_t  const pxPeripheral, char *pcValue, const size_t xBytes);
static void MX_ADC1_Init(void);

static const property_t gpioPropList[] =
   {
      { "active", PCHAR, set_active, get_active},
      { "mode", PCHAR, set_mode, get_mode},
      { "active_level", PCHAR, set_activeLevel, get_activeLevel},
      { "pull", PCHAR, set_pull, get_pull},
      { "formula", PCHAR, set_formula, get_formula},
      { "min", PCHAR, NULL, getMinValue},
      { "max", PCHAR, NULL, getMaxValue},
   };

static const driver_functions_t gpioFunctions = {gpio_open, gpio_write, gpio_read};

register_driver(gpio, "/gpioew", gpioFunctions, gpioStatus, arraysize(inputPin), gpioPropList, UINT16);


static ADC_HandleTypeDef hadc1;
static void inputTask(void * pvParameters);
static error_t initInputs (void);
static error_t startPeripheral (Tperipheral_t * peripheral);
static error_t stopPeripheral (Tperipheral_t * peripheral);

#define release_timeout   40 //Таймаут обработки кнопки после отпускания. Максимум 255 при использовании типа s08 для bt_release_time
#define max_value 255
#define min_value 255
#define long_press   50 // Время длительного нажатия. если держим кнопку нажатой столько циклов или больше - это длинное нажатие
#define short_press   5 // а это короткое
#define debounce 4

#define dimmer_long_press 5 // время ступени max 127
#define dimmer_min_value 0
#define dimmer_max_value 100 // Число ступеней
#define plato_duration 20 //Продолжительность плато

#define command_max_len   2
#define scan_interval   10



#define direction bt_release_time


volatile const uint32_t * inputState[]={
   GPIO_PIN_ISTATE(IPNUT_PORT_0, (IPNUT_PIN_0)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_1, (IPNUT_PIN_1)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_2, (IPNUT_PIN_2)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_3, (IPNUT_PIN_3)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_4, (IPNUT_PIN_4)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_5, (IPNUT_PIN_5)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_6, (IPNUT_PIN_6)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_7, (IPNUT_PIN_7)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_8, (IPNUT_PIN_8)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_9, (IPNUT_PIN_9)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_10, (IPNUT_PIN_10)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_11, (IPNUT_PIN_11)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_12, (IPNUT_PIN_12)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_13, (IPNUT_PIN_13)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_14, (IPNUT_PIN_14)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_15, (IPNUT_PIN_15)>>1)};

typedef enum
{
   NONE = 0x00,
   MinValueClick = NONE,
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

static size_t gpioSetValue(uint8_t gpioNum, uint16_t value)
{
   size_t result = 0;

   if (gpioNum < arraysize(inputPin))
   {
      switch (gpioContext[gpioNum].inputMode)
      {
      case S_BINARY:
         /* We can't write into S_BINARY*/
         break;
      case S_MORZE:
         if ((value>=MinValueClick) && (value < MaxValueClick))
         {
            gpioContext[gpioNum].value = value;
            result = sizeof(value);
         }
         break;
      case S_DIMMER:
         if ((value>=dimmer_min_value) && (value <= dimmer_max_value))
         {
            gpioContext[gpioNum].value = value;
            result = sizeof(value);
         }
         break;
      case S_MULTIMETER:
         /* We can't write into S_MULTIMETER*/
      default:
         break;
      }
   }


   return result;
}

static size_t set_mode(peripheral_t const pxPeripheral, char *pcValue)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   size_t result =0 ;

   uint8_t gpioNum = peripheral->peripheralNum;

   error_t error = NO_ERROR;

   /*First, check on overflow*/
   if (gpioNum >= arraysize(inputPin))
   {
      error = ERROR_INVALID_ADDRESS;
   }
   /*Second, check peripheral, if it's running*/
   else if (peripheral->status->statusAttribute & DEV_STAT_ACTIVE)
   {
      error = ERROR_WRONG_STATE;
   }
   else if (strcmp(pcValue, sensorList[S_BINARY].string) == 0)
   {
      gpioContext[gpioNum].inputMode = S_BINARY;
   }
   else if (strcmp(pcValue, sensorList[S_MORZE].string) == 0)
   {
      gpioContext[gpioNum].inputMode = S_MORZE;
   }
   else if (strcmp(pcValue, sensorList[S_DIMMER].string) == 0)
   {
      gpioContext[gpioNum].inputMode = S_DIMMER;
   }
   else if (strcmp(pcValue, sensorList[S_MULTIMETER].string) == 0)
   {
      gpioContext[gpioNum].inputMode = S_MULTIMETER;
   }
   else
   {
      error = ERROR_INVALID_PARAMETER;
   }
   if (error == NO_ERROR)
   {
      result = 1;
   }
   return result;
}

static size_t get_mode(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   size_t result =0 ;

   uint8_t gpioNum = peripheral->peripheralNum;

   /*First, check on overflow*/
   if (gpioNum >= arraysize(inputPin))
   {
      //    ERROR_INVALID_ADDRESS;
      result = 0;
   }
   else if ((gpioContext[gpioNum].inputMode == S_BINARY)
      || (gpioContext[gpioNum].inputMode == S_DIMMER)
      || (gpioContext[gpioNum].inputMode == S_MORZE)
      || (gpioContext[gpioNum].inputMode == S_MULTIMETER))
   {
      result = snprintf(pcValue, xBytes, "\"%s\"", sensorList[gpioContext[peripheral->peripheralNum].inputMode].string);
   }
   return result;
}

static size_t set_activeLevel(peripheral_t const pxPeripheral, char *pcValue)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   size_t result =0 ;

   uint8_t gpioNum = peripheral->peripheralNum;

   error_t error = NO_ERROR;

   /*First, check on overflow*/
   if (gpioNum >= arraysize(inputPin))
   {
      error = ERROR_INVALID_ADDRESS;
   }
   /*Second, check peripheral, if it's running*/
   else if (peripheral->status->statusAttribute & DEV_STAT_ACTIVE)
   {
      error = ERROR_WRONG_STATE;
   }
   else if (strcmp(pcValue, "low") == 0)
   {
      gpioContext[gpioNum].activeLevel = GPIO_ACTIVELOW;
   }
   else if (strcmp(pcValue, "high") == 0)
   {
      gpioContext[gpioNum].activeLevel = GPIO_ACTIVEHIGH;
   }
   else
   {
      error = ERROR_INVALID_PARAMETER;
   }
   if (error == NO_ERROR)
   {
      result = 1;
   }
   return result;
}
static size_t get_activeLevel(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   size_t result =0 ;

   uint8_t gpioNum = peripheral->peripheralNum;

   /*First, check on overflow*/
   if (gpioNum >= arraysize(inputPin))
   {
      //      ERROR_INVALID_ADDRESS;
      result =0 ;
   }
   else if (gpioContext[gpioNum].activeLevel == GPIO_ACTIVELOW)
   {
      result = snprintf(pcValue, xBytes, "\"low\"");

   }
   else if (gpioContext[gpioNum].activeLevel == GPIO_ACTIVEHIGH)
   {
      result = snprintf(pcValue, xBytes, "\"high\"");
   }
   return result;
}

static size_t set_pull(peripheral_t const pxPeripheral, char *pcValue)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   size_t result =0 ;

   uint8_t gpioNum = peripheral->peripheralNum;

   error_t error = NO_ERROR;

   /*First, check on overflow*/
   if (gpioNum >= arraysize(inputPin))
   {
      error = ERROR_INVALID_ADDRESS;
   }
   /*Second, check peripheral, if it's running*/
   else if (peripheral->status->statusAttribute & DEV_STAT_ACTIVE)
   {
      error = ERROR_WRONG_STATE;
   }
   else if (strcmp(pcValue, "low") == 0)
   {
      gpioContext[gpioNum].pull = GPIO_PULLLOW;
   }
   else if (strcmp(pcValue, "high") == 0)
   {
      gpioContext[gpioNum].pull = GPIO_PULLHIGH;
   }
   else if (strcmp(pcValue, "z") == 0)
   {
      gpioContext[gpioNum].pull = GPIO_Z;
   }
   else
   {
      error = ERROR_INVALID_PARAMETER;
   }
   if (error == NO_ERROR)
   {
      result = 1;
   }
   return result;
}

static size_t get_pull(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   size_t result =0 ;

   uint8_t gpioNum = peripheral->peripheralNum;
   //   error_t error = NO_ERROR;

   /*First, check on overflow*/
   if (gpioNum >= arraysize(inputPin))
   {
      //      ERROR_INVALID_ADDRESS;
      result = 0;
   }
   else if (gpioContext[gpioNum].pull == GPIO_PULLLOW)
   {
      result = snprintf(pcValue, xBytes, "\"low\"");

   }
   else if (gpioContext[gpioNum].pull == GPIO_PULLHIGH)
   {
      result = snprintf(pcValue, xBytes, "\"high\"");
   }
   return result;
}

/*
 * Not supported yet
 */
static size_t gpio_open(peripheral_t * const pxPeripheral)
{
   (void ) pxPeripheral;
   error_t error = NO_ERROR;
   size_t result =0;
   /* Если драйвер не запущен, запустим его */
   if (pInputTask == NULL)
   {
      memset (&gpioContext, 0x00, sizeof(gpio_t) * arraysize(inputPin));
      error = initInputs();
   }

   if (error == NO_ERROR)
   {
      result = 1;
   }
   return result;
}

static size_t gpio_write(peripheral_t * const pxPeripheral, const void * pvBuffer, const size_t xBytes)
{
   size_t result =0 ;
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   uint8_t gpioNum = peripheral->peripheralNum;

   uint16_t * value;

   if (xBytes<= sizeof(uint16_t)) //gpio_t.value
   {
      value = (uint16_t *) pvBuffer;

      /* Check, if device is writeble */
      if (gpioStatus[gpioNum] & DEV_STAT_WRITEBLE)
      {
         result = gpioSetValue(gpioNum, *value);
      }
   }
   return result;
}

static int gpioVarCallBack( void *user_data, const char *name, double *value ){

   int result =  PARSER_FALSE;
   uint16_t rawValue  = * (uint16_t *) user_data;
   //   uint8_t expressionNumber = *( uint8_t *)(user_data);
   if (strcmp(name, "x") == 0)
   {
      result =  PARSER_TRUE;
      *value = (double) rawValue;
   }
   return result;
}

static size_t gpio_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   size_t result =0 ;

   uint8_t gpioNum = peripheral->peripheralNum;

   uint16_t * value = (uint16_t *)pvBuffer;

   /*First, check on overflow*/
   if (gpioNum >= arraysize(inputPin))
   {
      //      ERROR_INVALID_ADDRESS;
      result =0 ;
   }
   else if (peripheral->driver->dataType == UINT16 && xBytes >= sizeof(uint16_t))
   {
      osAcquireMutex(&gpioMutexes[gpioNum]);

      * value = gpioContext[gpioNum].value;

      osReleaseMutex(&gpioMutexes[gpioNum]);

      result = sizeof(uint16_t);
   }
   return result;
}


/*
 * Test supported yet
 */
static size_t set_formula(peripheral_t const pxPeripheral, char *pcValue)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   uint8_t gpioNum = peripheral->peripheralNum;

   size_t result =0;

   size_t len = strlen(pcValue);
   char* formula = osAllocMem(len+1);

   if (formula != NULL)
   {
      strncpy(formula, pcValue, len);
      formula[len] = '\0';
      result = 1;
   }
   if (result == 1)
   {
      gpioContext[gpioNum].formula = formula;
   }
   else
   {
      osFreeMem(formula);
   }
   return result;
}
static size_t getMinValue(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   uint8_t gpioNum = peripheral->peripheralNum;

   size_t result =0;

   double dValue;

   uint16_t dMinValue;
   if (gpioNum < arraysize(inputPin))
   {
      switch (gpioContext[gpioNum].inputMode)
      {
      case S_BINARY:
         result = snprintf(pcValue, xBytes, "0");
         break;
      case S_MORZE:
         result = snprintf(pcValue, xBytes, "%"PRIu16"", MinValueClick);
         break;
      case S_DIMMER:
         dMinValue =  dimmer_min_value;
         if (gpioContext[gpioNum].formula!=NULL)
         {
            dValue = parse_expression_with_callbacks(gpioContext[gpioNum].formula , &gpioVarCallBack, NULL, &dMinValue);
         }
         else
         {
            dValue = dMinValue;
         }
         result = snprintf(pcValue, xBytes, "%"PRIu16"", (uint16_t) dValue);
         break;
      case S_MULTIMETER:
         dMinValue =  0;
         if (gpioContext[gpioNum].formula!=NULL)
         {
            dValue = parse_expression_with_callbacks(gpioContext[gpioNum].formula , &gpioVarCallBack, NULL, &dMinValue);
         }
         else
         {
            dValue = dMinValue;
         }
         result = snprintf(pcValue, xBytes, "%"PRIu16"", (uint16_t) dValue);
         break;
      default:
         break;
      }
   }
   return result;
}
static size_t getMaxValue(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   uint8_t gpioNum = peripheral->peripheralNum;

   size_t result =0;

   double dValue;

   uint16_t dMaxValue;

   if (gpioNum < arraysize(inputPin))
   {
      switch (gpioContext[gpioNum].inputMode)
      {
      case S_BINARY:
         result = snprintf(pcValue, xBytes, "1");
         break;
      case S_MORZE:
         result = snprintf(pcValue, xBytes, "%"PRIu16"", ShortShortLongClick);
         break;
      case S_DIMMER:
         dMaxValue =  dimmer_max_value;
         if (gpioContext[gpioNum].formula!=NULL)
         {
            dValue = parse_expression_with_callbacks(gpioContext[gpioNum].formula , &gpioVarCallBack, NULL, &dMaxValue);
         }
         else
         {
            dValue = dMaxValue;
         }
         result = snprintf(pcValue, xBytes, "%"PRIu16"", (uint16_t) dValue);
         break;
      case S_MULTIMETER:
         dMaxValue = 4096;
         if (gpioContext[gpioNum].formula!=NULL)
         {
            dValue = parse_expression_with_callbacks(gpioContext[gpioNum].formula , &gpioVarCallBack, NULL, &dMaxValue);
         }
         else
         {
            dValue = dMaxValue;
         }
         result = snprintf(pcValue, xBytes, "%"PRIu16"", (uint16_t) dValue);
         break;
      default:
         break;
      }
   }
   return result;
}
/*
 * Test support yet
 */
static size_t get_formula(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   uint8_t gpioNum = peripheral->peripheralNum;

   size_t result =0;

   if (gpioContext[gpioNum].formula != NULL)
   {
      snprintf(pcValue, xBytes, "\"%s\"", gpioContext[gpioNum].formula);
      result = strlen(pcValue);
   }
   return result;
}

static size_t set_active( peripheral_t const pxPeripheral, char *pcValue )
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;

   uint8_t gpioNum = peripheral->peripheralNum;

   size_t result =0 ;

   error_t error = NO_ERROR;

   if (strcmp(pcValue, "true") == 0)
   {
      /* Create mutex, if it not exist*/
      if (gpioMutexes[gpioNum].handle == NULL)
      {
         if (osCreateMutex(&gpioMutexes[gpioNum]) == FALSE)
         {
            error = ERROR_OUT_OF_MEMORY;
         }
      }
      if (error == NO_ERROR)
      {
         /*Если вывод не задйствован, задействуем его*/
         if (!(peripheral->status->statusAttribute & DEV_STAT_ACTIVE))
         {
            /* Пробуем запустить */
            error = startPeripheral(peripheral);
            if (error == NO_ERROR)
            {
               peripheral->status->statusAttribute |= DEV_STAT_ACTIVE;
            }
         }
         else
         {
            /* restart gpio pin */
            osAcquireMutex(&gpioMutexes[gpioNum]);
            peripheral->status->statusAttribute &= ~DEV_STAT_ACTIVE;

            stopPeripheral(peripheral);
            error = startPeripheral(peripheral);

            if (error == NO_ERROR)
            {
               peripheral->status->statusAttribute |= DEV_STAT_ACTIVE;
            }
            osReleaseMutex(&gpioMutexes[gpioNum]);
         }
      }
   }
   else if (strcmp(pcValue, "false") == 0)
   {
      peripheral->status->statusAttribute &= ~DEV_STAT_ACTIVE;
      /*
       * todo: debug this place
       */
      if (gpioMutexes[gpioNum].handle != NULL)
      {
         osDeleteMutex(&gpioMutexes[gpioNum]);
      }
      error = NO_ERROR;
   }

   if (error == NO_ERROR)
   {
      result = 1;
   }
   return result;
}

static size_t get_active(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;
   size_t result =0 ;

   /*Если вывод задйствован и драйвер запущен, сообщим об этом*/
   if ((peripheral->status->statusAttribute & DEV_STAT_ACTIVE) &&
      (pInputTask != NULL))
   {
      result = snprintf(pcValue, xBytes, "true");
   }
   else
   {
      result = snprintf(pcValue, xBytes, "false");
   }

   return result;
}

static error_t startPeripheral(Tperipheral_t * peripheral)
{
   uint32_t gpioNum = peripheral->peripheralNum;
   error_t error = NO_ERROR;
   GPIO_InitTypeDef GPIO_InitStruct;

   /*First, check on overflow*/
   if (gpioNum >= arraysize(inputPin))
   {
      error = ERROR_INVALID_ADDRESS;
   }

   switch (gpioContext[gpioNum].inputMode)
   {
   case S_BINARY:
   case S_MORZE:
   case S_DIMMER:
      if (gpioContext[gpioNum].pull == GPIO_Z
         || gpioContext[gpioNum].pull == GPIO_PULLHIGH
         || gpioContext[gpioNum].pull == GPIO_PULLLOW)
      {
         GPIO_InitStruct.Pin = inputPin[gpioNum];
         GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
         GPIO_InitStruct.Pull = gpioContext[gpioNum].pull;
         GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
         HAL_GPIO_Init(inputPort[gpioNum], &GPIO_InitStruct);
         gpioStatus[gpioNum] |= DEV_STAT_MANAGED;
      }
      else
      {
         error = ERROR_INVALID_PARAMETER;
      }
      break;
   case S_MULTIMETER:
      if (gpioContext[gpioNum].pull == GPIO_Z
         || gpioContext[gpioNum].pull == GPIO_PULLHIGH
         || gpioContext[gpioNum].pull == GPIO_PULLLOW)
      {
         GPIO_InitStruct.Pin = inputPin[gpioNum];
         GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
         GPIO_InitStruct.Pull = gpioContext[gpioNum].pull;
         HAL_GPIO_Init(inputPort[gpioNum], &GPIO_InitStruct);
         gpioStatus[gpioNum] |= DEV_STAT_MANAGED;
         adc_used++;
      }
      else
      {
         error = ERROR_INVALID_PARAMETER;
      }
      break;
   default:
      error = ERROR_INVALID_PARAMETER;
      break;
   }
   if (error == NO_ERROR)
   {
      gpioStatus[gpioNum] |= DEV_STAT_ACTIVE;
   }
   if (adc_used>0)
   {
      MX_ADC1_Init();
      __ADC1_CLK_ENABLE();
   }
   return error;
}

static error_t stopPeripheral (Tperipheral_t * peripheral)
{
   error_t error = NO_ERROR;
   uint32_t gpioNum = peripheral->peripheralNum;

   HAL_GPIO_DeInit(inputPort[gpioNum], inputPin[gpioNum]);

   return error;
}

static void MX_ADC1_Init(void)
{
   /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
   hadc1.Instance = ADC1;
   hadc1.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
   hadc1.Init.Resolution = ADC_RESOLUTION12b;
   hadc1.Init.ScanConvMode = ENABLE;
   hadc1.Init.ContinuousConvMode = ENABLE;
   hadc1.Init.DiscontinuousConvMode = DISABLE;
   hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
   hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
   hadc1.Init.NbrOfConversion = 1;
   //   hadc1.Init.DMAContinuousRequests = ENABLE;
   hadc1.Init.DMAContinuousRequests = DISABLE;
   HAL_ADC_Init(&hadc1);
}

static clicType check_bit_map(uint8_t bt_result, uint8_t bt_cnt)
{
   clicType result;

   if (bt_result == 0b00 && bt_cnt == 1)
   {
      result= SingleShortClick;
   }
   else if (bt_result == 0b00 && bt_cnt ==2)
   {
      result = DoubleShortClick;
   }
   else if (bt_result == 0b01 && bt_cnt ==1)
   {
      result = SingleLongClick;
   }
   else if (bt_result == 0b10 && bt_cnt ==2)
   {
      result = ShotAndLongClick;
   }
   else if (bt_result == 0b11 && bt_cnt ==2)
   {
      result = DoubleLongClick;
   }
   else if(bt_result == 0b000 && bt_cnt == 3)
   {
      result = TripleShortClick;
   }
   else if(bt_result == 0b100 && bt_cnt == 3)
   {
      result = ShortShortLongClick;
   }
   else
   {
      result = NONE;
   }
   return result;
}

static clicType check_dimmervalue(uint8_t presentValue, uint8_t bt_direction, uint8_t duration)
{
   clicType result;
   if (presentValue == dimmer_min_value)
   {
      if (duration < plato_duration)
      {
         result = PlatoValue;
      }
      else
      {
         result = IncreaseValue;
      }
   }
   else if (presentValue > dimmer_min_value && presentValue < dimmer_max_value  && bt_direction == IncreaseValue)
   {
      result = IncreaseValue;
   }
   else if (presentValue == dimmer_max_value)
   {
      if (duration < plato_duration)
      {
         result = PlatoValue;
      }
      else
      {
         result = DecreaseValue;
      }
   }
   else if (presentValue > dimmer_min_value && presentValue < dimmer_max_value  && bt_direction ==DecreaseValue)
   {
      result = DecreaseValue;
   }
   else if (presentValue == dimmer_min_value && bt_direction ==DecreaseValue)
   {
      result = PlatoValue;
   }
   else
   {
      result = NONE;
   }
   return result;
}

static uint16_t GetADCValue(uint32_t Channel, int Count)
{
   uint32_t val = 0;
   ADC_ChannelConfTypeDef   sConfig;
   sConfig.Channel=Channel;
   sConfig.Rank=1;
   sConfig.SamplingTime=ADC_SMPR1_SMP10_0;
   HAL_ADC_ConfigChannel(&hadc1, &sConfig);

   for(int i = 0; i < Count; i++)
   {
      HAL_ADC_Start(&hadc1);
      HAL_ADC_PollForConversion(&hadc1, 1);
      val += HAL_ADC_GetValue(&hadc1);
   }
   return (uint16_t) (val / Count);
}

static void setVal(uint32_t inputNum, uint16_t value)
{
   double dValue;
   osAcquireMutex(&gpioMutexes[inputNum]);

   if (gpioContext[inputNum].formula != NULL)
   {
      dValue = parse_expression_with_callbacks(gpioContext[inputNum].formula , &gpioVarCallBack, NULL, &value);
      if (!isnan(dValue))
      {
         gpioContext[inputNum].value = (uint16_t) dValue;
      }

   }
   else
   {
      gpioContext[inputNum].value = value;
   }
   gpioStatus[inputNum] |= DEV_STAT_READBLE;

   osReleaseMutex(&gpioMutexes[inputNum]);
}

static void inputTask(void * pvParameters)
{
   (void) pvParameters;

   uint32_t i;
   clicType tempClickValue;
   uint16_t tempADCValue;
   uint16_t tempVal;
   uint16_t currentVal;

   mask_t * mask = (mask_t *) driver_gpio.mask;
   //   mask->driver = &driver_gpio;
   while (1)
   {
      for (i=0; i<arraysize(inputPin); i++)
      {
         if (gpioStatus[i] &  DEV_STAT_MANAGED)
         {
            if ((gpioStatus[i] & (DEV_STAT_ACTIVE )) && (gpioContext[i].inputMode == S_BINARY))
            {
               // Кнопень придавлена. Тупо считаем время придавки.
               if (*inputState[i])
               {
                  if (gpioContext[i].bt_time<max_value-1)
                  {
                     gpioContext[i].bt_time++;
                  }
                  if (gpioContext[i].bt_time>debounce)
                  {

                     setVal(i, 0x01);
                     //                     osAcquireMutex(&gpioMutexes[i]);
                     //
                     //                     gpioContext[i].value = 0x01;
                     //                     /* We can only read value */
                     //                     gpioStatus[i] |= DEV_STAT_READBLE;
                     //
                     //                     osReleaseMutex(&gpioMutexes[i]);
                     gpioContext[i].bt_release_time=0;
                  }
               }
               else
               {
                  if (gpioContext[i].bt_release_time<min_value-1 )
                  {
                     gpioContext[i].bt_release_time++; //  отсчета таймаута ожидания завершения комманды после последней нажатой кнопки.
                  }
                  if (gpioContext[i].bt_release_time>debounce)
                  {
                     setVal(i, 0x00);
                     //                     osAcquireMutex(&gpioMutexes[i]);
                     //
                     //                     gpioContext[i].value = 0x00;
                     //                     /* We can only read value*/
                     //                     gpioStatus[i] |= DEV_STAT_READBLE;
                     //
                     //                     osReleaseMutex(&gpioMutexes[i]);
                     gpioContext[i].bt_time=0;
                  }
               }

            }
            else if ((gpioStatus[i] & (DEV_STAT_ACTIVE )) && (gpioContext[i].inputMode == S_DIMMER))
            {
               /* We can read and write value*/
               gpioStatus[i] |= DEV_STAT_READBLE | DEV_STAT_WRITEBLE;

               // Кнопень придавлена. Тупо считаем время придавки.
               if (*inputState[i])
               {
                  if (gpioContext[i].bt_time<max_value-1) //Давно не нажимали кнопку
                  {
                     gpioContext[i].bt_time++;
                  }

                  if (gpioContext[i].bt_time>long_press)
                  {
                     osAcquireMutex(&gpioMutexes[i]);
                     currentVal = gpioContext[i].value;
                     osReleaseMutex(&gpioMutexes[i]);

                     tempClickValue = check_dimmervalue(currentVal, gpioContext[i].direction, gpioContext[i].bt_cnt);
                     gpioContext[i].bt_time=long_press - dimmer_long_press;

                     if (tempClickValue == IncreaseValue)

                     {
                        gpioContext[i].bt_cnt=1;
                        currentVal++;
                        gpioContext[i].direction = tempClickValue;
                     }
                     else if (tempClickValue == DecreaseValue)
                     {
                        gpioContext[i].bt_cnt=1;
                        currentVal--;
                        gpioContext[i].direction = tempClickValue;
                     }
                     else if (tempClickValue == PlatoValue)
                     {
                        gpioContext[i].bt_cnt++;
                        gpioContext[i].direction = tempClickValue;
                     }
                     setVal(i,currentVal );
                     //                     osAcquireMutex(&gpioMutexes[i]);

                     //                     gpioContext[i].value = currentVal;
                     //                  /* We can read and write value*/
                     //                  gpioStatus[i] |= DEV_STAT_READBLE | DEV_STAT_WRITEBLE;

                     //                     osReleaseMutex(&gpioMutexes[i]);

                     if (gpioContext[i].direction != PlatoValue && currentVal == 0 )
                     {
                        gpioContext[i].direction =PlatoValue;
                     }


                  }
               }
               //  Момент отпускания кнопки.
               else if (gpioContext[i].bt_time>debounce) //Если кнопка была нажата больше времени дребезга
               {
                  if (gpioContext[i].bt_cnt > 0)
                  {
                     // Отпустили после длинного нажатия
                     gpioContext[i].bt_time = 0;
                     gpioContext[i].bt_cnt = 0;

                  }
                  else if (gpioContext[i].bt_time>=short_press)
                  {
                     osAcquireMutex(&gpioMutexes[i]);
                     tempVal = gpioContext[i].value;
                     osReleaseMutex(&gpioMutexes[i]);
                     // Отпустили после короткого нажатия
                     if (tempVal>0)
                     {
                        //Сохраняем значение и выключаем
                        osAcquireMutex(&gpioMutexes[i]);

                        gpioContext[i].bt_result = gpioContext[i].value;
                        //                        gpioContext[i].value = dimmer_min_value;
                        //                     /* We can read and write value*/
                        //                     gpioStatus[i] |= DEV_STAT_READBLE | DEV_STAT_WRITEBLE;

                        osReleaseMutex(&gpioMutexes[i]);
                        setVal(i, dimmer_min_value);
                     }
                     else if(tempVal == 0 && (gpioContext[i].bt_result > 0))
                     {
                        //Если есть сохраненное значение, то восстанавливаем
                        setVal(i, gpioContext[i].bt_result);
                        //                        osAcquireMutex(&gpioMutexes[i]);

                        //                        gpioContext[i].value = gpioContext[i].bt_result;
                        //                     /* We can read and write value*/
                        //                     gpioStatus[i] |= DEV_STAT_READBLE | DEV_STAT_WRITEBLE;

                        //                        osReleaseMutex(&gpioMutexes[i]);
                     }
                     else
                     {
                        // Иначе включаем на всю
                        setVal(i, dimmer_max_value);
                        //                        osAcquireMutex(&gpioMutexes[i]);
                        //                        gpioContext[i].value = dimmer_max_value;

                        //                     /* We can read and write value*/
                        //                     gpioStatus[i] |= DEV_STAT_READBLE | DEV_STAT_WRITEBLE;

                        //                        osReleaseMutex(&gpioMutexes[i]);
                     }
                     gpioContext[i].bt_cnt = 0;
                     gpioContext[i].bt_time = 0;

                  }
               }

            }
            else if ((gpioStatus[i] & (DEV_STAT_ACTIVE )) && (gpioContext[i].inputMode == S_MORZE))
            {
               /* We can read and write value*/
               gpioStatus[i] |= DEV_STAT_READBLE | DEV_STAT_WRITEBLE;
               // Кнопень придавлена. Тупо считаем время придавки.
               if ((*inputState[i]))
               {
                  if (gpioContext[i].bt_time<max_value-1)
                  {
                     gpioContext[i].bt_time++;
                  }
               }
               //  Момент отпускания кнопки.
               else if (gpioContext[i].bt_time>debounce) //Если кнопка была нажата больше времени дребезга
               {
                  // Длинное нажатие. Ставим единичку в битовое поле результата.
                  if (gpioContext[i].bt_time>=long_press)
                  {
                     gpioContext[i].bt_result|=(1<<gpioContext[i].bt_cnt);
                     gpioContext[i].bt_cnt++; // Длинное нажатие. Пропускаем установку бита.
                     gpioContext[i].bt_time=0; // время придавки кнопки нам больше не нужно, сбрасываем.
                     gpioContext[i].bt_release_time=0;
                  }
                  else if (gpioContext[i].bt_time>=short_press)
                  {
                     gpioContext[i].bt_release_time=0;
                     gpioContext[i].bt_cnt++; // Короткое нажатие. Пропускаем установку бита.
                     gpioContext[i].bt_time=0; // время придавки кнопки нам больше не нужно, сбрасываем.
                  }

               }
               // Прошел ли таймаут отпускания кнопки
               else if (gpioContext[i].bt_cnt>0 && gpioContext[i].bt_release_time<release_timeout)
               {
                  // Отпущено, счетчик таймаута комманды(последовательности). Здесь мы еще ждем ввода нажатий для последовательности.
                  if (gpioContext[i].bt_release_time<min_value-1 )
                  {
                     gpioContext[i].bt_release_time++; //  отсчета таймаута ожидания завершения комманды после последней нажатой кнопки.
                  }
               }
               else if (gpioContext[i].bt_cnt>0 && gpioContext[i].bt_release_time>=release_timeout)
               { // Таймаут комманды, сброс состояний
                  tempClickValue = check_bit_map(gpioContext[i].bt_result, gpioContext[i].bt_cnt);
                  if (tempClickValue)
                  {
                     setVal(i, tempClickValue);
                     //                     osAcquireMutex(&gpioMutexes[i]);

                     //                     gpioContext[i].value = tempClickValue;
                     //                  /* We can read and write value*/
                     //                  gpioStatus[i] |= DEV_STAT_READBLE | DEV_STAT_WRITEBLE;

                     //                     osReleaseMutex(&gpioMutexes[i]);
                  }
                  gpioContext[i].bt_time=0;
                  gpioContext[i].bt_cnt=0;
                  gpioContext[i].bt_result=0;
                  gpioContext[i].bt_release_time=0;
               }
               else
               {
                  //               gpioStatus[i]|=DEV_STAT_READBLE;
                  gpioContext[i].bt_cnt=0; // Было ложное нажатие
                  gpioContext[i].bt_time=0;
                  gpioContext[i].bt_result=0;
                  gpioContext[i].bt_release_time=0;
               }
            }
            else if ((gpioStatus[i] & (DEV_STAT_ACTIVE )) && (gpioContext[i].inputMode == S_MULTIMETER))
            {
               tempADCValue = GetADCValue(adcChannel[i], 5);

               setVal(i, tempADCValue);
               //               osAcquireMutex(&gpioMutexes[i]);
               //               gpioContext[i].value = tempADCValue;
               /* We can only read value*/
               //               gpioStatus[i] |= DEV_STAT_READBLE;

               //               osReleaseMutex(&gpioMutexes[i]);
            }

            if (gpioContext[i].prevValue!=gpioContext[i].value)
            {
               (mask->mask) |= (1<<i);
            }
            else
            {
               (mask->mask) &= ~(1<<i);
            }
            gpioContext[i].prevValue = gpioContext[i].value;
         }
      }
      if (mask->mask != 0)
      {
         xQueueSendToBack(dataQueue, mask, 0);
      }
      osDelayTask(scan_interval);
   }
}

static error_t initInputs (void)
{
   error_t error = NO_ERROR;
   if (pInputTask == NULL)
   {
      pInputTask = osCreateTask("inputs", inputTask, NULL, configMINIMAL_STACK_SIZE*4, 3);
   }
   if (pInputTask == NULL)
   {
      error = ERROR_OUT_OF_MEMORY;
   }
   return error;
}
