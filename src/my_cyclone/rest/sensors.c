/*
 * sensors.c
 *
 *  Created on: 25 июня 2015 г.
 *      Author: timurtaipov
 */
#include "rest/sensors.h"
#include "rest/sensors_def.h"

#include "rest/input.h"
#include "rest/temperature.h"
#include "configs.h"
#include <ctype.h>



char places[PLACES_CACHE_LENGTH];
char names[NAMES_CACHE_LENGTH];

char *pPlaces = &places[0];
char *pNames = &names[0];


sensor_t sensors[MAX_NUM_SENSORS];

register_rest_function(sensors, "/sensors", &restInitSensors, &restDenitSensors, &restGetSensors, &restPostSensors, &restPutSensors, &restDeleteSensors);

register_sens_function(test_sens, "/test", S_TEMP, NULL, NULL, NULL, NULL, NULL, NULL);

static int printfSensorMethods (char * bufer, int maxLen, sensFunctions * sensor)
{
   int p=0;
   int flag=0;
   p+=snprintf(bufer+p, maxLen-p, "{\r\n\"path\": \"%s/v1/sensor%s\",\r\n\"method\" : [", &restPrefix[0], sensor->sensClassPath);
   if (sensor->sensGetMethodHadler != NULL)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"GET\",");
      flag++;
   }
   if (sensor->sensPostMethodHadler != NULL)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"POST\",");
      flag++;
   }
   if (sensor->sensPutMethodHadler != NULL)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"PUT\",");
      flag++;

   }
   if (sensor->sensDeleteMethodHadler != NULL)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"DELETE\",");
      flag++;
   }
   if (flag>0)
   {
      p--;
   }

   p+=snprintf(bufer+p, maxLen-p, "]\r\n}");
   return p;
}

static int printfSensor (char * bufer, int maxLen)
{
   int p=0;
   p+=snprintf(bufer+p, maxLen-p, "{\"sensors\":[\r\n");
   for (sensFunctions *cur_sensor = &__start_sens_functions; cur_sensor < &__stop_sens_functions; cur_sensor++)
   {
      p+=printfSensorMethods(bufer+p, maxLen - p, cur_sensor);
      if ( cur_sensor != &__stop_sens_functions-1)
      {
         p+=snprintf(bufer+p, maxLen-p, ",\r\n");
      }
   }
   p+=snprintf(bufer+p, maxLen-p, "\r\n]\r\n}\r\n");
   return p;
}

error_t restInitSensors(void)
{
   error_t error = NO_ERROR;

   return error;
}

error_t restDenitSensors(void)
{
   error_t error = NO_ERROR;
   return error;
}

static sensFunctions * restFindSensor(RestApi_t* RestApi)
{
   for (sensFunctions *cur_sensor = &__start_sens_functions; cur_sensor < &__stop_sens_functions; cur_sensor++)
   {
      if (CLASS_EQU(RestApi, cur_sensor->sensClassPath))
      {
         return cur_sensor;
      }
   }
   return NULL;
}

error_t restGetSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t max_len = sizeof(restBuffer);
   error_t error = NO_ERROR;
   sensFunctions * wantedSensor;
   wantedSensor = restFindSensor(RestApi);
   if (wantedSensor != NULL)
   {//Если сенсор найден
      if (wantedSensor->sensGetMethodHadler != NULL)
      {//Если есть обработчик метода GET
         error = wantedSensor->sensGetMethodHadler(connection, RestApi);
      }
      else
      {  //Если нет обработчика метода GET печатаем все доступные методы
         p = printfSensorMethods(restBuffer+p, max_len, wantedSensor);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден отдаем все сенсоры с методами
      printfSensor(&restBuffer[0], max_len);
      rest_404_not_found(connection, &restBuffer[0]);
      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restPostSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t max_len = sizeof(restBuffer);
   error_t error = NO_ERROR;
   sensFunctions * wantedSensor;
   wantedSensor = restFindSensor(RestApi);
   if (wantedSensor != NULL)
   {//Если сенсор найден
      if (wantedSensor->sensPostMethodHadler != NULL)
      {//Если есть обработчик метода POST
         error = wantedSensor->sensPostMethodHadler(connection, RestApi);
      }
      else
      {  //Если нет обработчика метода POST печатаем все доступные методы
         p = printfSensorMethods(restBuffer+p, max_len, wantedSensor);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден отдаем все сенсоры с методами
      printfSensor(&restBuffer[0], max_len);
      rest_404_not_found(connection, &restBuffer[0]);
      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restPutSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t max_len = sizeof(restBuffer);
   error_t error = NO_ERROR;
   sensFunctions * wantedSensor;
   wantedSensor = restFindSensor(RestApi);
   if (wantedSensor != NULL)
   {//Если сенсор найден
      if (wantedSensor->sensPutMethodHadler != NULL)
      {//Если есть обработчик метода PUT
         error = wantedSensor->sensPutMethodHadler(connection, RestApi);
      }
      else
      {  //Если нет обработчика метода PUT печатаем все доступные методы
         p = printfSensorMethods(restBuffer+p, max_len, wantedSensor);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден отдаем все сенсоры с методами
      printfSensor(&restBuffer[0], max_len);
      rest_404_not_found(connection, &restBuffer[0]);
      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restDeleteSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t max_len = sizeof(restBuffer);
   error_t error = NO_ERROR;
   sensFunctions * wantedSensor;
   wantedSensor = restFindSensor(RestApi);
   if (wantedSensor != NULL)
   {//Если сенсор найден
      if (wantedSensor->sensDeleteMethodHadler != NULL)
      {//Если есть обработчик метода DELETE
         error = wantedSensor->sensDeleteMethodHadler(connection, RestApi);
      }
      else
      {  //Если нет обработчика метода DELETE печатаем все доступные методы
         p = printfSensorMethods(restBuffer+p, max_len, wantedSensor);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден отдаем все сенсоры с методами
      printfSensor(&restBuffer[0], max_len);
      rest_404_not_found(connection, &restBuffer[0]);
      error = ERROR_NOT_FOUND;
   }
   return error;
}
char* sensorsFindName(const char * name, size_t length)
{
   char * p = &names[0];
   while (p < &(names[NAMES_CACHE_LENGTH-1]))
   {
      if (strncmp(name, p, length)==0)
      {
         return p;
      }
      p = strchr(++p,'\0');
   }
   return NULL;
}

char* sensorsAddName(const char * name, size_t length)
{
   if (length+pNames<&(names[NAMES_CACHE_LENGTH-1]))
   {
      memcpy(pNames, name, length);
      pNames+=length;
      *pNames = '\0';
      pNames++;
      return pNames-(length+1);
   }
   return NULL;
}

char* sensorsFindPlace(const char * place, size_t length)
{
   char * p = &places[0];
   while (p < &(places[NAMES_CACHE_LENGTH-1]))
   {
      if (strncmp(place, p, length)==0)
      {
         return p;
      }
      p = strchr(++p,'\0');
   }
   return NULL;
}

char* sensorsAddPlace(const char * place, size_t length)
{
   if (length+pPlaces<&(places[NAMES_CACHE_LENGTH-1]))
   {
      memcpy(pPlaces, place, length);
      pPlaces+=length;
      *pPlaces = '\0';
      pPlaces++;
      return pPlaces-(length+1);
   }
   return NULL;
}


static error_t parseSensors (char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   jsmnerr_t resultCode;
   jsmn_init(jSMNparser);
   uint8_t pos;
   sensor_t * currentSensor = &sensors[0];

   resultCode = jsmn_parse(jSMNparser, data, len, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);

   if(resultCode)
   {
      for (sensFunctions *cur_sensor = &__start_sens_functions; cur_sensor < &__stop_sens_functions; cur_sensor++)
      {
         if (cur_sensor->sensInitClassHadler != NULL)
         {
            cur_sensor->sensInitClassHadler(data, jSMNtokens, &currentSensor, &resultCode, &pos);
         }
      }
   }
   else
   {
      //    ntp_use_default_servers();
   }

   return NO_ERROR;
}

void sensorsConfigure(void)
{
   error_t error;
   memset (&sensors[0],0,sizeof(sensor_t)*MAX_NUM_SENSORS);
   error = read_config("/config/sensors.json",&parseSensors);
   osCreateTask("oneWireTask",oneWireTask, NULL, configMINIMAL_STACK_SIZE*4, 1);
   //   osCreateTask("input",inputTask, NULL, configMINIMAL_STACK_SIZE*4, 1);
}

/**
 * @brief Convert a string representation of an serial number to a binary serial address
 * @param[in] str string representing the serial address
 * @param[in] length size of string
 * @param[out] OwSerial_t Binary representation of the serial address
 * @return Error code
 **/

error_t serialStringToHex(const char *str, size_t length, uint8_t *serial, size_t expectedLength)
{
   error_t error;
   size_t i = 0;
   int value = -1;

   //Parse input string
   while(length--)
   {
      //Hexadecimal digit found?
      if(isxdigit((uint8_t) *str))
      {
         //First digit to be decoded?
         if(value < 0) value = 0;
         //Update the value of the current byte
         if(isdigit((uint8_t) *str))
            value = (value * 16) + (*str - '0');
         else if(isupper((uint8_t) *str))
            value = (value * 16) + (*str - 'A' + 10);
         else
            value = (value * 16) + (*str - 'a' + 10);
         //Check resulting value
         if(value > 0xFF)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }
      }
      //Dash or colon separator found?
      else if((*str == '-' || *str == ':') && i < 8)
      {
         //Each separator must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }

         //Save the current byte
         serial[i++] = value;
         //Prepare to decode the next byte
         value = -1;
      }

      //Invalid character...
      else
      {
         //The conversion failed
         error = ERROR_INVALID_SYNTAX;
         break;
      }

      //Point to the next character
      str++;
   }

   //End of string reached?
   if(i == expectedLength - 1)
   {
      //The NULL character must be preceded by a valid number
      if(value < 0)
      {
         //The conversion failed
         error = ERROR_INVALID_SYNTAX;
      }
      else
      {
         //Save the last byte of the MAC address
         serial[i] = value;
         //The conversion succeeded
         error = NO_ERROR;
      }

   }

   //Return status code
   return error;
}


char *serialHexToString(const uint8_t *serial, char *str, int length)
{
   static char buffer[MAX_LEN_SERIAL*3-1];
   int p=0;
   int i;
   //The str parameter is optional
   if(!str) str = buffer;
   if (length>0)
   {
      p=sprintf(str, "%02X",serial[0]);
   }
   //Format serial number
   for (i=1;i<length;i++)
   {
      p+=sprintf(str+p, ":%02X",serial[i]);
   }

   //Return a pointer to the formatted string
   return str;
}
