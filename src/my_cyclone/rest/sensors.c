/*
 * sensors.c
 *
 *  Created on: 25 июня 2015 г.
 *      Author: timurtaipov
 */

//Switch to the appropriate trace level
#define TRACE_LEVEL 2

#include "rest/sensors.h"
#include "rest/sensors_def.h"
#include "rest/input.h"
#include "rest/temperature.h"
#include "configs.h"
#include <ctype.h>
#include "debug.h"


char places[PLACES_CACHE_LENGTH];
char names[NAMES_CACHE_LENGTH];

char *pPlaces = &places[0];
char *pNames = &names[0];

#define CURRELOFARRAY(element, array) (element > &array)?(element-&array):0

sensor_t sensors[MAX_NUM_SENSORS];

register_rest_function(sensors, "/sensors", &restInitSensors, &restDenitSensors, &restGetSensors, &restPostSensors, &restPutSensors, &restDeleteSensors);


static int printfSensorMethods (char * bufer, int maxLen, sensFunctions * sensor, int restVersion)
{
   int p=0;
   int flag=0;
   if (restVersion == 1)
   {
      p+=snprintf(bufer+p, maxLen-p, "{\r\n\"name\": \"%s\",\r\n\"path\": \"%s/v1/sensors%s\",\r\n\"method\" : [",sensor->sensClassName, &restPrefix[0], sensor->sensClassPath);
   }
   else if(restVersion ==2 )
   {
      p+=snprintf(bufer+p, maxLen-p, "\"%s\":{\"links\":{\"related\": \"%s/v2/sensors%s\"}}",sensor->sensClassName, &restPrefix[0], sensor->sensClassPath);
   }
   if (restVersion == 1)
   {
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
   }
   return p;
}

static int printfSensor (char * bufer, int maxLen, int restVersion)
{
   int p=0;
   if (restVersion == 1)
   {
      p+=snprintf(bufer+p, maxLen-p, "{\"sensors\":[\r\n");
   }
   else if (restVersion ==2 )
   {
      p+=snprintf(bufer+p, maxLen-p, "{\"data\":\r\n{\r\n\"type\": \"sensors\",\"id\":0,\"attributes\":{\"id\": 0,\"name\":\"sensors\"},\"relationships\": {");
   }
   for (sensFunctions *cur_sensor = &__start_sens_functions; cur_sensor < &__stop_sens_functions; cur_sensor++)
   {
      p+=printfSensorMethods(bufer+p, maxLen - p, cur_sensor, restVersion);
      if ( cur_sensor != &__stop_sens_functions-1)
      {
         p+=snprintf(bufer+p, maxLen-p, ",\r\n");
      }
   }
   if (restVersion ==2 )
   {
      p+=snprintf(bufer+p, maxLen-p, "}}\r\n");
   }
   p+=snprintf(bufer+p, maxLen-p, "\r\n}\r\n");
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

static error_t sensGetHandler(HttpConnection *connection, RestApi_t* RestApi, sensFunctions * sensor)
{
   int p=0;
   int i=0,j=0;
   error_t error = ERROR_NOT_FOUND;
   const size_t max_len = sizeof(restBuffer);
   if (RestApi->restVersion == 1)
   {
      p=sprintf(restBuffer,"{\"%s\":[ ", sensor->sensClassPath+1);
   }
   else if (RestApi->restVersion ==2)
   {
      p=sprintf(restBuffer,"{\"data\":[");
   }
   if (RestApi->objectId != NULL)
   {
      if (ISDIGIT(*(RestApi->objectId+1)))
      {
         j=atoi(RestApi->objectId+1);
         p+=sensor->sensGetMethodHadler(&restBuffer[p], max_len-p, j, RestApi->restVersion);
         error = NO_ERROR;
      }
      else
      {
         error = ERROR_UNSUPPORTED_REQUEST;
      }
   }
   else
   {
      for (i=0; i<= MAX_NUM_SENSORS; i++)
      {
         p+=sensor->sensGetMethodHadler(&restBuffer[p], max_len-p, i, RestApi->restVersion);
         error = NO_ERROR;
      }
   }
   p--;

   p+=snprintf(restBuffer+p, max_len-p,"]}\r\n");
   if (RestApi->restVersion == 1)
   {
      connection->response.contentType = mimeGetType(".json");
   }
   else if (RestApi->restVersion ==2)
   {
      connection->response.contentType = mimeGetType(".apijson");
   }


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
   return error;
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

         error = sensGetHandler(connection, RestApi, wantedSensor);
      }
      else
      {  //Если нет обработчика метода GET печатаем все доступные методы
         p = printfSensorMethods(restBuffer+p, max_len, wantedSensor, RestApi->restVersion);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден отдаем все сенсоры с методами
      printfSensor(&restBuffer[0], max_len, RestApi->restVersion);
      if (RestApi->restVersion == 1)
      {
         connection->response.contentType = mimeGetType(".json");
      }
      else if (RestApi->restVersion ==2)
      {
         connection->response.contentType = mimeGetType(".apijson");
      }
      rest_200_ok(connection, &restBuffer[0]);
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
         p = printfSensorMethods(restBuffer+p, max_len, wantedSensor, RestApi->restVersion);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден отдаем все сенсоры с методами
      printfSensor(&restBuffer[0], max_len, RestApi->restVersion);
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
         p = printfSensorMethods(restBuffer+p, max_len, wantedSensor, RestApi->restVersion);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден отдаем все сенсоры с методами
      printfSensor(&restBuffer[0], max_len, RestApi->restVersion);
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
         p = printfSensorMethods(restBuffer+p, max_len, wantedSensor, RestApi->restVersion);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден отдаем все сенсоры с методами
      printfSensor(&restBuffer[0], max_len, RestApi->restVersion);
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
      p = strchr(p,'\0');

      if (*p=='\0' && *(p+1) == '\0')
      {
         return NULL;
      }
      p++;
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

static error_t initSensor(sensor_t * cur_sensor)
{
   error_t error = NO_ERROR;
   //Create a mutex to protect critical sections
   if(!osCreateMutex(&cur_sensor->mutex))
   {
      //Failed to create mutex
      xprintf("Sensors: Can't create sensor#%d mutex.\r\n", cur_sensor->id);
      error= ERROR_OUT_OF_RESOURCES;
   }
   if (!error)
   {
      error = sensorsHealthInit (cur_sensor);
   }
   return error;
}

void sensorsConfigure(void)
{
   volatile error_t error = NO_ERROR;
   int i;

   memset (&sensors[0],0,sizeof(sensor_t)*MAX_NUM_SENSORS);
   for (i=0;i<MAX_NUM_SENSORS; i++)
   {
      initSensor(&sensors[i]);
   }
   if (!error)
   {
      error = read_config("/config/sensors.json",&parseSensors);
   }
   /*
    * todo: move this code to init section.
    */
   if (!error)
   {
      osCreateTask("oneWireTask",oneWireTask, NULL, configMINIMAL_STACK_SIZE*4, 1);
   }
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

error_t sensorsHealthInit (sensor_t * sensor)
{
   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsHealthInit: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);

   sensor->health.value =SENSORS_HEALTH_MAX_VALUE;
   sensor->health.counter =SENSORS_HEALTH_MIN_VALUE;

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsHealthInit: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

   return NO_ERROR;
}

int sensorsHealthGetValue(sensor_t * sensor)
{
   int result;

   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsHealthGetValue: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);

   result = sensor->health.value;

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsHealthGetValue: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

   return result;
}


void sensorsHealthIncValue(sensor_t * sensor)
{
   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsHealthIncValue: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);

   if (sensor->health.value<SENSORS_HEALTH_MAX_VALUE)
   {
      sensor->health.value++;
   }
   if (sensor->health.counter<SENSORS_HEALTH_MAX_VALUE)
   {
      sensor->health.counter++;
   }

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsHealthIncValue: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

}

void sensorsHealthDecValue(sensor_t * sensor)
{

   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsHealthDecValue: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);


   if (sensor->health.value>SENSORS_HEALTH_MIN_VALUE)
   {
      sensor->health.value--;
   }
   if (sensor->health.counter<SENSORS_HEALTH_MAX_VALUE)
   {
      sensor->health.counter++;
   }

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsHealthDecValue: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

}

void sensorsHealthSetValue(sensor_t * sensor, int value)
{

   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsHealthSetValue: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);


   if (value>SENSORS_HEALTH_MAX_VALUE)
   {
      sensor->health.value=SENSORS_HEALTH_MAX_VALUE;
   }
   else if (value<SENSORS_HEALTH_MIN_VALUE)
   {
      sensor->health.value=SENSORS_HEALTH_MIN_VALUE;
   }
   else
   {
      sensor->health.value = value;
   }

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsHealthSetValue: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

}

void sensorsSetValueUint16(sensor_t * sensor, uint16_t value)
{

   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsSetValueUint16: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);

   sensor->value.uVal = value;

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsSetValueUint16: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

}

void sensorsSetValueFloat(sensor_t * sensor, float value)
{

   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsSetValueFloat: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);

   sensor->value.fVal = value;

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsSetValueFloat: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

}

uint16_t sensorsGetValueUint16(sensor_t * sensor)
{
   uint16_t value;

   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsGetValueUint16: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);

   value = sensor->value.uVal;

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsGetValueUint16: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

   return value;

}

float sensorsGetValueFloat(sensor_t * sensor)
{
   float value;

   //Enter critical section
   //Debug message
   TRACE_INFO("sensorsGetValueFloat: Enter critical section sensor#%d.", sensor->id);
   osAcquireMutex(&sensor->mutex);

   value = sensor->value.fVal;

   //Leave critical section
   //Debug message
   TRACE_INFO("sensorsGetValueFloat: Leave critical section sensor#%d.\r\n", sensor->id);
   osReleaseMutex(&sensor->mutex);

   return value;

}
