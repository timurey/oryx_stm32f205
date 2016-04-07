/*
 * input.c
 *
 *  Created on: 29 авг. 2015 г.
 *      Author: timurtaipov
 */
#include "input.h"
#include "os_port.h"
#include "rest/sensors_def.h"
#include "uuid.h"
#include "stm32f2xx_hal.h"

static OsTask *pInputTask;
static char buf[]="00:00:00:00:00:00:00:00/0";

static  uint16_t inputPin[]={IPNUT_PIN_0, IPNUT_PIN_1, IPNUT_PIN_2, IPNUT_PIN_3,
   IPNUT_PIN_4, IPNUT_PIN_5, IPNUT_PIN_6, IPNUT_PIN_7, IPNUT_PIN_8, IPNUT_PIN_9,
   IPNUT_PIN_10, IPNUT_PIN_11, IPNUT_PIN_12, IPNUT_PIN_13, IPNUT_PIN_14, IPNUT_PIN_15};

static  GPIO_TypeDef* inputPort[]={ IPNUT_PORT_0, IPNUT_PORT_1, IPNUT_PORT_2, IPNUT_PORT_3,
   IPNUT_PORT_4, IPNUT_PORT_5, IPNUT_PORT_6, IPNUT_PORT_7, IPNUT_PORT_8, IPNUT_PORT_9,
   IPNUT_PORT_10, IPNUT_PORT_11, IPNUT_PORT_12, IPNUT_PORT_13, IPNUT_PORT_14, IPNUT_PORT_15};

static  uint32_t adcChannel[]={INPUT_ADC_CHANNEL_0, INPUT_ADC_CHANNEL_1, INPUT_ADC_CHANNEL_2, INPUT_ADC_CHANNEL_3,
   INPUT_ADC_CHANNEL_4, INPUT_ADC_CHANNEL_5, INPUT_ADC_CHANNEL_6, INPUT_ADC_CHANNEL_7, INPUT_ADC_CHANNEL_8};

static ADC_HandleTypeDef hadc1;
#if 0
static error_t getRestInputs(HttpConnection *connection, RestApi_t* RestApi);
#endif
static void inputTask(void * pvParameters);
static error_t initInputs (const char * data, jsmntok_t *jSMNtokens, sensor_t ** pCurrentSensor, jsmnerr_t * resultCode, uint8_t * pos);

int input_snprintf(char * bufer, size_t max_len, int sensnum, int restVersion);

register_sens_function(inputs, "/inputs", S_INPUT, &initInputs, NULL, NULL, NULL, NULL, NULL, UINT16);
extern sensor_t sensors[MAX_NUM_SENSORS];

#define release_timeout   40 //Таймаут обработки кнопки после отпускания. Максимум 255 при использовании типа s08 для bt_release_time
#define max_value 255
#define min_value 255
#define long_press   50 // Время длительного нажатия. если держим кнопку нажатой столько циклов или больше - это длинное нажатие
#define short_press   5 // а это короткое
#define debounce 4

#define dimmer_long_press 5 // время ступени max 127
#define dimmer_max_value 100 // Число ступеней
#define plato_duration 20 //Продолжительность плато

#define command_max_len   2
#define scan_interval   10
typedef struct
{
   uint8_t bt_time;           // Переменная времени работы автомата
   uint8_t bt_release_time;   // Переменная времени работы автомата
   uint8_t bt_result;         // Переменная результат. Бит-карта нажатий.
   uint8_t bt_cnt;            // Переменная счетчик нажатий
} inputStatus;

#define direction bt_release_time

static inputStatus inputContex[MAX_INPUTS_COUNT];

volatile const uint32_t * inputState[]={
   GPIO_PIN_ISTATE(IPNUT_PORT_0, (IPNUT_PIN_0)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_1, (IPNUT_PIN_1)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_2, (IPNUT_PIN_2)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_3, (IPNUT_PIN_3)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_4, (IPNUT_PIN_4)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_5, (IPNUT_PIN_5)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_6, (IPNUT_PIN_6)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_7, (IPNUT_PIN_7)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_8, (IPNUT_PIN_8)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_9, (IPNUT_PIN_9)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_10, (IPNUT_PIN_10)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_11, (IPNUT_PIN_11)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_12, (IPNUT_PIN_12)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_13, (IPNUT_PIN_13)>>1), GPIO_PIN_ISTATE(IPNUT_PORT_14, (IPNUT_PIN_14)>>1),
   GPIO_PIN_ISTATE(IPNUT_PORT_15, (IPNUT_PIN_15)>>1)};

//order of bytes in UUID int order_of_bytes[]={3,2,1,0,7,6,5,4,11,10,9,8};
#define IsLocal(serial) (((serial[0] == uuid->b[6] && serial[1]==uuid->b[5] && serial[2]==uuid->b[4] && serial[3]==uuid->b[11] && serial[4]==uuid->b[10] && serial[5]==uuid->b[9] && serial[6]==uuid->b[8] )?TRUE:FALSE))
#define HexToDec(a) (10*((a & 0xF0) > 4) + (a&0x0f))

typedef enum
{
   NONE = 0x00,
   //Sequence
   SingleShortClick, //0x01 Single short click
   DoubleShortClick, //0x02 Double short click
   SingleLongClick,  //0x03 Singl long click
   ShotAndLongClick, //0x04 Short and long click
   DoubleLongClick,  //0x05
   TripleShortClick, //0x06
   ShortShortLongClick, //0x07
   //Dimmer
   IncreaseValue = 0x11,   //0x11 17
   DecreaseValue,          //0x12 18
   PlatoValue,             //0x13 19
} clicType;

static void initInput(sensor_t * sensor)
{
   GPIO_InitTypeDef GPIO_InitStruct;
   switch ((sensor->subType))
   {
   case S_BINARY:
   case S_MORZE:
   case S_DIMMER:
      GPIO_InitStruct.Pin = inputPin[HexToDec(sensor->serial[7])];
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLDOWN;
      GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
      HAL_GPIO_Init(inputPort[HexToDec(sensor->serial[7])], &GPIO_InitStruct);
      (sensor->status) |= MANAGED;
      break;
   case S_MULTIMETER:
      GPIO_InitStruct.Pin = inputPin[HexToDec(sensor->serial[7])];
      GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      HAL_GPIO_Init(inputPort[HexToDec(sensor->serial[7])], &GPIO_InitStruct);
      (sensor->status) |= MANAGED;

      break;
   default:

      break;
   }
   sensor->status  |= ONLINE;
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
   if (presentValue == 0)
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
   else if (presentValue > 0 && presentValue < dimmer_max_value  && bt_direction == IncreaseValue)
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
   else if (presentValue > 0 && presentValue < dimmer_max_value  && bt_direction ==DecreaseValue)
   {
      result = DecreaseValue;
   }
   else if (presentValue == 0 && bt_direction ==DecreaseValue)
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
   uint16_t val = 0;
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
   return val / Count;
}

static void inputTask(void * pvParameters)
{
   int i;
   int inputNum=0;
   clicType tempvalue;
   uint16_t currentVal;
   (void) pvParameters;
   memset (&inputContex, 0x00, sizeof(inputStatus) * MAX_INPUTS_COUNT);
   while (1)
   {
      inputNum=0;
      for (i=0; i<MAX_NUM_SENSORS; i++)
      {
         if (sensors[i].type == S_INPUT && sensors[i].subType == S_BINARY)
         {
            if (IsLocal(sensors[i].serial))
            {
               // Кнопень придавлена. Тупо считаем время придавки.
               if ((*inputState[sensors[i].serial[MAX_LEN_SERIAL-1]]))
               {
                  if (inputContex[inputNum].bt_time<max_value-1)
                  {
                     inputContex[inputNum].bt_time++;
                  }
                  if (inputContex[inputNum].bt_time>debounce)
                  {
                     sensorsSetValueUint16(&sensors[i], 0x01);
                     inputContex[inputNum].bt_release_time=0;
                  }
               }
               else
               {
                  if (inputContex[inputNum].bt_release_time<min_value-1 )
                  {
                     inputContex[inputNum].bt_release_time++; //  отсчета таймаута ожидания завершения комманды после последней нажатой кнопки.
                  }
                  if (inputContex[inputNum].bt_release_time>debounce)
                  {
                     sensorsSetValueUint16(&sensors[i], 0x00);
                     inputContex[inputNum].bt_time=0;
                  }
               }

            }
            inputNum++;

         }
         if (sensors[i].type == S_INPUT && sensors[i].subType == S_DIMMER)
         {
            if (IsLocal(sensors[i].serial))
            {
               // Кнопень придавлена. Тупо считаем время придавки.
               if ((*inputState[sensors[i].serial[MAX_LEN_SERIAL-1]]))
               {
                  if (inputContex[inputNum].bt_time<max_value-1) //Давно не нажимали кнопку
                  {
                     inputContex[inputNum].bt_time++;
                  }

                  if (inputContex[inputNum].bt_time>long_press)
                  {
                     currentVal = sensorsGetValueUint16(&sensors[i]);
                     tempvalue = check_dimmervalue(currentVal, inputContex[inputNum].direction, inputContex[inputNum].bt_cnt);
                     inputContex[inputNum].bt_time=long_press-dimmer_long_press;

                     if (tempvalue == IncreaseValue)

                     {
                        inputContex[inputNum].bt_cnt=1;
                        currentVal++;
                        inputContex[inputNum].direction = tempvalue;
                     }
                     else if (tempvalue == DecreaseValue)
                     {
                        inputContex[inputNum].bt_cnt=1;
                        currentVal--;
                        inputContex[inputNum].direction = tempvalue;
                     }
                     else if (tempvalue == PlatoValue)
                     {
                        inputContex[inputNum].bt_cnt++;
                        inputContex[inputNum].direction = tempvalue;
                     }
                     sensorsSetValueUint16(&sensors[i], currentVal);
                     if (inputContex[inputNum].direction != PlatoValue && currentVal == 0 )
                     {
                        inputContex[inputNum].direction =PlatoValue;
                     }


                  }
               }
               //  Момент отпускания кнопки.
               else if (inputContex[inputNum].bt_time>debounce) //Если кнопка была нажата больше времени дребезга
               {
                  if (inputContex[inputNum].bt_cnt > 0)
                  {
                     // Отпустили после длинного нажатия
                     inputContex[inputNum].bt_time = 0;
                     inputContex[inputNum].bt_cnt = 0;

                  }
                  else if (inputContex[inputNum].bt_time>=short_press)
                  {
                     // Отпустили после короткого нажатия
                     if (sensorsGetValueUint16(&sensors[i])>0)
                     {
                        //Сохраняем значение и выключаем
                        inputContex[inputNum].bt_result = sensorsGetValueUint16(&sensors[i]);
                        sensorsSetValueUint16(&sensors[i], 0);
                     }
                     else if(sensorsGetValueUint16(&sensors[i]) == 0 && (inputContex[inputNum].bt_result > 0))
                     {
                        //Если есть сохраненное значение, то восстанавливаем
                        sensorsSetValueUint16(&sensors[i], inputContex[inputNum].bt_result);
                     }
                     else
                     {
                        // Иначе включаем на всю
                        sensorsSetValueUint16(&sensors[i], dimmer_max_value);
                     }
                     inputContex[inputNum].bt_cnt = 0;
                     inputContex[inputNum].bt_time = 0;

                  }
               }

            }
            inputNum++;

         }
         else if (sensors[i].type == S_INPUT && sensors[i].subType == S_MORZE)
         {
            if (IsLocal(sensors[i].serial))
            {
               // Кнопень придавлена. Тупо считаем время придавки.
               if ((*inputState[sensors[i].serial[MAX_LEN_SERIAL-1]]))
               {
                  if (inputContex[inputNum].bt_time<max_value-1)
                  {
                     inputContex[inputNum].bt_time++;
                  }
               }
               //  Момент отпускания кнопки.
               else if (inputContex[inputNum].bt_time>debounce) //Если кнопка была нажата больше времени дребезга
               {
                  // Длинное нажатие. Ставим единичку в битовое поле результата.
                  if (inputContex[inputNum].bt_time>=long_press)
                  {
                     inputContex[inputNum].bt_result|=(1<<inputContex[inputNum].bt_cnt);
                     inputContex[inputNum].bt_cnt++; // Длинное нажатие. Пропускаем установку бита.
                     inputContex[inputNum].bt_time=0; // время придавки кнопки нам больше не нужно, сбрасываем.
                     inputContex[inputNum].bt_release_time=0;
                  }
                  else if (inputContex[inputNum].bt_time>=short_press)
                  {
                     inputContex[inputNum].bt_release_time=0;
                     inputContex[inputNum].bt_cnt++; // Короткое нажатие. Пропускаем установку бита.
                     inputContex[inputNum].bt_time=0; // время придавки кнопки нам больше не нужно, сбрасываем.
                  }

               }
               // Прошел ли таймаут отпускания кнопки
               else if (inputContex[inputNum].bt_cnt>0 && inputContex[inputNum].bt_release_time<release_timeout)
               {
                  // Отпущено, счетчик таймаута комманды(последовательности). Здесь мы еще ждем ввода нажатий для последовательности.
                  if (inputContex[inputNum].bt_release_time<min_value-1 )
                  {
                     inputContex[inputNum].bt_release_time++; //  отсчета таймаута ожидания завершения комманды после последней нажатой кнопки.
                  }
               }
               else if (inputContex[inputNum].bt_cnt>0 && inputContex[inputNum].bt_release_time>=release_timeout)
               { // Таймаут комманды, сброс состояний
                  tempvalue = check_bit_map(inputContex[inputNum].bt_result, inputContex[inputNum].bt_cnt);
                  if (tempvalue)
                  {
                     sensorsSetValueUint16(&sensors[i],tempvalue);
                  }
                  inputContex[inputNum].bt_time=0;
                  inputContex[inputNum].bt_cnt=0;
                  inputContex[inputNum].bt_result=0;
                  inputContex[inputNum].bt_release_time=0;
               }
               else
               {
                  inputContex[inputNum].bt_cnt=0; // Было ложное нажатие
                  inputContex[inputNum].bt_time=0;
                  inputContex[inputNum].bt_result=0;
                  inputContex[inputNum].bt_release_time=0;
               }
            }
            inputNum++;
         }
         else if (sensors[i].type == S_INPUT && sensors[i].subType == S_MULTIMETER)
         {
            if (IsLocal(sensors[i].serial))
            {

               sensorsSetValueUint16(&sensors[i], GetADCValue(adcChannel[HexToDec(sensors[i].serial[7])], 5));
            }
            inputNum++;
         }
      }

      osDelayTask(scan_interval);
   }
}
#if 0
int input_snprintf(char * bufer, size_t max_len, int sens_num, int restVersion)
{
   int p=0;
   int i;
   for(i=0; i <= MAX_NUM_SENSORS; i++)
   {
      if (sensors[i].type == S_INPUT && (sensors[sens_num].subType == S_BINARY || sensors[sens_num].subType == S_MORZE || sensors[sens_num].subType == S_DIMMER || sensors[sens_num].subType == S_MULTIMETER) && sensors[i].id == sens_num)
      {
         if (restVersion == 1)

         {
            p+=snprintf(bufer+p, max_len-p, "{\"id\":%d,",sensors[i].id);
         }
         else if (restVersion ==2)
         {
            p+=snprintf(bufer+p, max_len-p, "{\"type\":\"input\",\"id\":%d,\"attributes\":{",sensors[i].id);
         }
         p+=snprintf(bufer+p, max_len-p, "\"name\":\"%s\",", sensors[i].name);
         p+=snprintf(bufer+p, max_len-p, "\"place\":\"%s\",", sensors[i].place);
         switch (sensors[sens_num].subType)
         {
         case S_BINARY:
            p+=snprintf(bufer+p, max_len-p, "\"type\":\"digital\",");
            p+=snprintf(bufer+p, max_len-p, "\"value\":%s,",(sensorsGetValueUint16(&sensors[i]) & 1?"true":"false"));
            break;
         case S_MORZE:
            p+=snprintf(bufer+p, max_len-p, "\"type\":\"sequential\",");
            p+=snprintf(bufer+p, max_len-p, "\"value\":\"0x%02x\",",sensorsGetValueUint16(&sensors[i]));
            break;
         case S_DIMMER:
            p+=snprintf(bufer+p, max_len-p, "\"type\":\"dimmer\",");
            p+=snprintf(bufer+p, max_len-p, "\"value\":%u,",sensorsGetValueUint16(&sensors[i]));
            break;
         case S_MULTIMETER:
            p+=snprintf(bufer+p, max_len-p, "\"type\":\"analog\",");
            p+=snprintf(bufer+p, max_len-p, "\"value\":%u,",sensorsGetValueUint16(&sensors[i]));
            break;
         default:
            break;
         }
         p+=snprintf(bufer+p, max_len-p,"\"serial\":\"%s\",",serialHexToString(sensors[sens_num].serial, &buf[0], ONEWIRE_SERIAL_LENGTH));
         if (restVersion == 1)
         {
            p+=snprintf(bufer+p, max_len-p, "\"online\":%s},", ((sensors[i].status & ONLINE)?"true":"false"));
         }
         else if (restVersion == 2)
         {
            p+=snprintf(bufer+p, max_len-p, "\"online\":%s}},", ((sensors[i].status & ONLINE)?"true":"false"));
         }
         break;
      }
   }
   return p;
}


static error_t getRestInputs(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   int i=0,j=0;
   error_t error = ERROR_NOT_FOUND;
   const size_t max_len = sizeof(restBuffer);
   p=sprintf(restBuffer,"{\"inputs\":[ ");
   if (RestApi->objectId != NULL)
   {
      if (ISDIGIT(*(RestApi->objectId+1)))
      {
         j=atoi(RestApi->objectId+1);
         for (i=0; i< MAX_NUM_SENSORS; i++)
         {
            if (sensors[i].id == j)
            {
               p+=input_snprintf(&restBuffer[p], max_len-p, i);
               error = NO_ERROR;
               break;
            }

         }
      }
      else
      {
         error = ERROR_UNSUPPORTED_REQUEST;
      }

   }
   else
   {
      for (i=0; i< MAX_NUM_SENSORS; i++)
      {

         p+=input_snprintf(&restBuffer[p], max_len-p, i);
         error = NO_ERROR;
      }
   }
   p--;

   p+=snprintf(restBuffer+p, max_len-p,"]}\r\n");
   connection->response.contentType = mimeGetType(".json");

   switch (error)
   {
   case NO_ERROR:
      return rest_200_ok(connection, &restBuffer[0]);
      break;
   case ERROR_UNSUPPORTED_REQUEST:
      return rest_400_bad_request(connection, "400 Bad Request.\r\n");
      break;
   case ERROR_NOT_FOUND:
   default:
      return rest_404_not_found(connection, "404 Not Found.\r\n");
      break;

   }


}
#endif

static error_t initInputs (const char * data, jsmntok_t *jSMNtokens, sensor_t ** pCurrentSensor, jsmnerr_t * resultCode, uint8_t *pos)
{
#define MAXLEN 64
   int len;
   int i, j=0;
   char path[64];
   char tmp_str[MAXLEN];
   char * str = &tmp_str[0];
   //   int length;
   error_t error = NO_ERROR;
   uint8_t flag = 0;
   uint8_t adc_used=0;
   sensor_t * currentSensor = *pCurrentSensor;

   for (i=0;i<TOTAL_INPUTS;i++)
   {
      /*
       * Code for digital inputs
       */
      sprintf(&path[0],"$.sensors.inputs.digital[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {

         error = serialStringToHex( str, len, &currentSensor->serial[0], INPUT_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.sensors.inputs.digital[%d].name",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentSensor->name = sensorsFindName(str, len);

         if (currentSensor->name == 0)
         {
            currentSensor->name = sensorsAddName(str, len);
            flag++;
            if (currentSensor->name == 0)
            {
               xprintf("error parsing input name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.sensors.inputs.digital[%d].place",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentSensor->place = sensorsFindPlace(str, len);

         if (currentSensor->place==0)
         {
            currentSensor->place = sensorsAddPlace(str, len);
            flag++;
            if (currentSensor->place == 0)
            {
               xprintf("error parsing input place: no avalible memory for saving place");
            }
         }

      }
      if (flag >0)
      {
         currentSensor->id=j++;
         currentSensor->type = S_INPUT;
         currentSensor->subType =S_BINARY;
         flag=0;
         if (IsLocal(currentSensor->serial))
         {
            initInput(currentSensor);
         }
         currentSensor++;
         (*pos)++;
      }

   }
   for (i=0;i<TOTAL_INPUTS;i++)
   {
      /*
       * Code for sequential inputs
       */
      sprintf(&path[0],"$.sensors.inputs.sequential[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         error = serialStringToHex(str, len, &currentSensor->serial[0], INPUT_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.sensors.inputs.sequential[%d].name",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentSensor->name = sensorsFindName(str, len);

         if (currentSensor->name == 0)
         {
            currentSensor->name = sensorsAddName(str, len);
            flag++;
            if (currentSensor->name == 0)
            {
               xprintf("error parsing input name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.sensors.inputs.sequential[%d].place",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentSensor->place = sensorsFindPlace(str, len);

         if (currentSensor->place==0)
         {
            currentSensor->place = sensorsAddPlace(str, len);
            flag++;
            if (currentSensor->place == 0)
            {
               xprintf("error parsing input place: no avalible memory for saving place");
            }
         }

      }
      if (flag >0)
      {
         currentSensor->id=j++;
         currentSensor->type = S_INPUT;
         currentSensor->subType = S_MORZE;
         flag=0;
         if (IsLocal(currentSensor->serial))
         {
            initInput(currentSensor);
         }
         currentSensor++;
         (*pos)++;
      }
   }
   for (i=0;i<TOTAL_INPUTS;i++)
   {
      /*
       * Code for dimmers inputs
       */
      sprintf(&path[0],"$.sensors.inputs.dimmer[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         error = serialStringToHex(str, len, &currentSensor->serial[0], INPUT_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.sensors.inputs.dimmer[%d].name",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentSensor->name = sensorsFindName(str, len);

         if (currentSensor->name == 0)
         {
            currentSensor->name = sensorsAddName(str, len);
            flag++;
            if (currentSensor->name == 0)
            {
               xprintf("error parsing input name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.sensors.inputs.dimmer[%d].place",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentSensor->place = sensorsFindPlace(str, len);

         if (currentSensor->place==0)
         {
            currentSensor->place = sensorsAddPlace(str, len);
            flag++;
            if (currentSensor->place == 0)
            {
               xprintf("error parsing input place: no avalible memory for saving place");
            }
         }

      }
      if (flag >0)
      {
         currentSensor->id=j++;
         currentSensor->type = S_INPUT;
         currentSensor->subType = S_DIMMER;
         flag=0;
         if (IsLocal(currentSensor->serial))
         {
            initInput(currentSensor);
         }
         currentSensor++;
         (*pos)++;
      }
   }
   for (i=0;i<TOTAL_INPUTS;i++)
   {
      /*
       * Code for analog inputs
       */
      sprintf(&path[0],"$.sensors.inputs.analog[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         error = serialStringToHex(str, len, &currentSensor->serial[0], INPUT_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.sensors.inputs.analog[%d].name",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentSensor->name = sensorsFindName(str, len);

         if (currentSensor->name == 0)
         {
            currentSensor->name = sensorsAddName(str, len);
            flag++;
            if (currentSensor->name == 0)
            {
               xprintf("error parsing input name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.sensors.inputs.analog[%d].place",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentSensor->place = sensorsFindPlace(str, len);

         if (currentSensor->place==0)
         {
            currentSensor->place = sensorsAddPlace(str, len);
            flag++;
            if (currentSensor->place == 0)
            {
               xprintf("error parsing input place: no avalible memory for saving place");
            }
         }

      }
      if (flag >0)
      {
         currentSensor->id=j++;
         currentSensor->type = S_INPUT;
         currentSensor->subType = S_MULTIMETER;
         if (adc_used == 0)
         {
            MX_ADC1_Init();
            __ADC1_CLK_ENABLE();
         }
         adc_used++;
         flag=0;
         if (IsLocal(currentSensor->serial))
         {
            initInput(currentSensor);
         }
         currentSensor++;
         (*pos)++;
      }
   }
   pInputTask = osCreateTask("inputs", inputTask, NULL, configMINIMAL_STACK_SIZE, 3);
   *pCurrentSensor=currentSensor;
   return error;
}
