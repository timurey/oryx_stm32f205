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
//#include "rest/input.h"
#include "rest/temperature.h"
#include "configs.h"
#include <ctype.h>
#include "debug.h"
#include "macros.h"
#include "../../expression_parser/variables_def.h"
char places[PLACES_CACHE_LENGTH];
char names[NAMES_CACHE_LENGTH];

char *pPlaces = &places[0];
char *pNames = &names[0];



// Type of sensor (used when presenting sensors)
 const mysensorSensorList_t sensorList[] =
 {
    {S_INPUT, "input"},
    {S_DOOR, ""}, // Door sensor, V_TRIPPED, V_ARMED
    {S_MOTION,  ""}, // Motion sensor, V_TRIPPED, V_ARMED
    {S_SMOKE,  ""}, // Smoke sensor, V_TRIPPED, V_ARMED
    {S_LIGHT, ""}, // Binary light or relay, V_STATUS (or V_LIGHT), V_WATT
    {S_BINARY, "digital"}, // Binary light or relay, V_STATUS (or V_LIGHT), V_WATT (same as S_LIGHT)
    {S_DIMMER, "dimmer"}, // Dimmable light or fan device, V_STATUS (on/off), V_DIMMER (dimmer level 0-100), V_WATT
    {S_COVER, ""}, // Blinds or window cover, V_UP, V_DOWN, V_STOP, V_DIMMER (open/close to a percentage)
    {S_TEMP, "temperature"}, // Temperature sensor, V_TEMP
    {S_HUM, ""}, // Humidity sensor, V_HUM
    {S_BARO, ""}, // Barometer sensor, V_PRESSURE, V_FORECAST
    {S_WIND, ""}, // Wind sensor, V_WIND, V_GUST
    {S_RAIN, ""}, // Rain sensor, V_RAIN, V_RAINRATE
    {S_UV, ""}, // Uv sensor, V_UV
    {S_WEIGHT, ""}, // Personal scale sensor, V_WEIGHT, V_IMPEDANCE
    {S_POWER, ""}, // Power meter, V_WATT, V_KWH
    {S_HEATER, ""}, // Header device, V_HVAC_SETPOINT_HEAT, V_HVAC_FLOW_STATE, V_TEMP
    {S_DISTANCE, ""}, // Distance sensor, V_DISTANCE
    {S_LIGHT_LEVEL, ""}, // Light level sensor, V_LIGHT_LEVEL (uncalibrated in percentage),  V_LEVEL (light level in lux)
    {S_ARDUINO_NODE, ""}, // Used (internally) for presenting a non-repeating Arduino node
    {S_ARDUINO_REPEATER_NODE, ""}, // Used (internally) for presenting a repeating Arduino node
    {S_LOCK, ""}, // Lock device, V_LOCK_STATUS
    {S_IR, ""}, // Ir device, V_IR_SEND, V_IR_RECEIVE
    {S_WATER, ""}, // Water meter, V_FLOW, V_VOLUME
    {S_AIR_QUALITY, ""}, // Air quality sensor, V_LEVEL
    {S_CUSTOM, ""}, // Custom sensor
    {S_MORZE, "sequential"},
    {S_DUST, ""}, // Dust sensor, V_LEVEL
    {S_SCENE_CONTROLLER, ""}, // Scene controller device, V_SCENE_ON, V_SCENE_OFF.
    {S_RGB_LIGHT, ""}, // RGB light. Send color component data using V_RGB. Also supports V_WATT
    {S_RGBW_LIGHT, ""}, // RGB light with an additional White component. Send data using V_RGBW. Also supports V_WATT
    {S_COLOR_SENSOR,  ""}, // Color sensor, send color information using V_RGB
    {S_HVAC, ""}, // Thermostat/HVAC device. V_HVAC_SETPOINT_HEAT, V_HVAC_SETPOINT_COLD, V_HVAC_FLOW_STATE, V_HVAC_FLOW_MODE, V_TEMP
    {S_MULTIMETER, "analog"}, // Multimeter device, V_VOLTAGE, V_CURRENT, V_IMPEDANCE
    {S_SPRINKLER,  ""}, // Sprinkler, V_STATUS (turn on/off), V_TRIPPED (if fire detecting device)
    {S_WATER_LEAK, ""}, // Water leak sensor, V_TRIPPED, V_ARMED
    {S_SOUND, ""}, // Sound sensor, V_TRIPPED, V_ARMED, V_LEVEL (sound level in dB)
    {S_VIBRATION, ""}, // Vibration sensor, V_TRIPPED, V_ARMED, V_LEVEL (vibration in Hz)
    {S_MOISTURE, ""}, // Moisture sensor, V_TRIPPED, V_ARMED, V_LEVEL (water content or moisture in percentage?)
    {S_ERROR, ""}, //Wrong sensor
 };

static char buf[]="00:00:00:00:00:00:00:00/0";

#define CURRELOFARRAY(element, array) (element > &array)?(element-&array):0

sensor_t sensors[MAX_NUM_SENSORS];

register_rest_function(sensors, "/sensors", &restInitSensors, &restDenitSensors, &restGetSensors, &restPostSensors, &restPutSensors, &restDeleteSensors);
register_variables_functions(sensors, &sensorsGetValue);

static int sprintfSensor (char * bufer, int maxLen, sensFunctions * sensor, int restVersion)
{
   int p=0;
   int flag=0;
   if (restVersion == 1)
   {
      p+=snprintf(bufer+p, maxLen-p, "{\"name\":\"%s\",\"path\":\"%s/v1/sensors%s\",\"method\":[", sensor->sensClassName, &restPrefix[0], sensor->sensClassPath);

//      if (sensor->sensGetMethodHadler != NULL)
//      {
         p+=snprintf(bufer+p, maxLen-p, "\"GET\",");
         flag++;
//      }
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

      p+=snprintf(bufer+p, maxLen-p, "]}");
   }
   else if(restVersion ==2 )
   {
      p+=snprintf(bufer+p, maxLen-p, "\"%s\":{\"links\":{\"related\": \"%s/v2/sensors%s\"}}",sensor->sensClassName, &restPrefix[0], sensor->sensClassPath);
   }
   return p;
}



static error_t sprintfListSensors (HttpConnection *connection, RestApi_t* RestApi)
{
   int p =0;
   const size_t maxLen = sizeof(restBuffer);
   error_t error = NO_ERROR;
   if (RestApi->restVersion == 1)
   {
      p+=snprintf(&restBuffer[p], maxLen-p, "{\"sensors\":[\r\n");
   }
   else if (RestApi->restVersion ==2 )
   {
      p+=snprintf(&restBuffer[p], maxLen-p, "{\"data\":{\"type\":\"sensors\",\"id\":0,\r\n\"relationships\":{\r\n");
   }

   for (sensFunctions *cur_sensor = &__start_sens_functions; cur_sensor < &__stop_sens_functions; cur_sensor++)
   {
      p+=sprintfSensor(restBuffer+p, maxLen - p, cur_sensor, RestApi->restVersion);
      if ( cur_sensor != &__stop_sens_functions-1)
      {
         p+=snprintf(&restBuffer[p], maxLen-p, ",\r\n");
      }
   }

   if (RestApi->restVersion == 1)
   {
      p+=snprintf(&restBuffer[p], maxLen-p, "\r\n]}\r\n");
   }
   else if (RestApi->restVersion ==2 )
   {
      p+=snprintf(&restBuffer[p], maxLen-p, "\r\n}}}\r\n");
   }
   if (RestApi->restVersion == 1)
   {
      connection->response.contentType = mimeGetType(".json");
      error = rest_300_multiple_choices(connection, &restBuffer[0]);
   }
   else if (RestApi->restVersion == 2)
   {
      connection->response.contentType = mimeGetType(".apijson");
      error = rest_200_ok(connection, &restBuffer[0]);
   }

   return error;
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
      if (REST_CLASS_EQU(RestApi, cur_sensor->sensClassPath))
      {
         return cur_sensor;
      }
   }
   return NULL;
}


static int snprintfSensor(char * bufer, size_t maxLen, int sens_num, sensFunctions * sensor, int restVersion)
{
   int p=0;
   int d1, d2;
   float f2, f_val;
   int i;

   for(i=0; i <= MAX_NUM_SENSORS; i++)
   {
      if (sensors[i].type == sensor->sensorType && sensors[i].id == sens_num)
      {
         if (restVersion == 1)
         {
            p+=snprintf(bufer+p, maxLen-p, "{\"id\":%d,",sensors[i].id);
         }
         else if (restVersion ==2)
         {
            p+=snprintf(bufer+p, maxLen-p, "{\"type\":\"%s\",\"id\":%d,\"attributes\":{",sensorList[sensors[i].type].string, sensors[i].id);
         }
         p+=snprintf(bufer+p, maxLen-p, "\"type\":\"%s\",", sensorList[sensors[i].subType].string);
         p+=snprintf(bufer+p, maxLen-p, "\"name\":\"%s\",", sensors[i].name);
         p+=snprintf(bufer+p, maxLen-p, "\"place\":\"%s\",", sensors[i].place);

         switch (sensors[i].valueType)
         {
         case FLOAT:
            f_val = sensorsGetValueFloat(&sensors[i]);
            d1 = f_val;            // Get the integer part (678).
            f2 = f_val - d1;     // Get fractional part (678.0123 - 678 = 0.0123).
            d2 = abs(trunc(f2 * 10));   // Turn into integer (123).
            p+=snprintf(bufer+p, maxLen-p, "\"value\":\"%d.%01d\",", d1, d2);
            break;
         case UINT16:
            p+=snprintf(bufer+p, maxLen-p, "\"value\":\"%u\",",sensorsGetValueUint16(&sensors[i]));
            break;
         case CHAR://Not supported yet
         case PCHAR:
         default:
            break;
         }
         p+=snprintf(bufer+p, maxLen-p, "\"serial\":\"%s\",",serialHexToString(sensors[i].serial, &buf[0], ONEWIRE_SERIAL_LENGTH));
         p+=snprintf(bufer+p, maxLen-p, "\"health\":%d,", sensorsHealthGetValue(&sensors[i]));
         if (restVersion == 1)
         {
            p+=snprintf(bufer+p, maxLen-p, "\"online\":%s},", ((sensors[i].status & ONLINE)?"true":"false"));
         }
         else if (restVersion == 2)
         {
            p+=snprintf(bufer+p, maxLen-p, "\"online\":%s}},", ((sensors[i].status & ONLINE)?"true":"false"));
         }
         break;
      }
   }
   return p;
}

static error_t sensCommonGetHandler(HttpConnection *connection, RestApi_t* RestApi, sensFunctions * sensor)
{
   int p=0;
   int i=0,j=0;
   error_t error = ERROR_NOT_FOUND;
   const size_t maxLen = sizeof(restBuffer);
   if (RestApi->restVersion == 1)
   {
      p=sprintf(restBuffer,"{\"%s\":[\r\n", sensor->sensClassName);
   }
   else if (RestApi->restVersion ==2)
   {
      p=sprintf(restBuffer,"{\"data\":[\r\n");
   }
   if (RestApi->objectId != NULL)
   {
      if (ISDIGIT(*(RestApi->objectId+1)))
      {
         j=atoi(RestApi->objectId+1);
         p+=snprintfSensor(&restBuffer[p], maxLen-p, j, sensor, RestApi->restVersion);
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
         p+=snprintfSensor(&restBuffer[p], maxLen-p, i, sensor, RestApi->restVersion);
         error = NO_ERROR;
      }
   }
   p--;

   p+=snprintf(restBuffer+p, maxLen-p,"]}\r\n");
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

   error_t error = NO_ERROR;
   sensFunctions * wantedSensor;
   if (RestApi->className!=NULL)
   {
      wantedSensor = restFindSensor(RestApi);
      if (wantedSensor != NULL)
      {//Если сенсор найден
         if (wantedSensor->sensGetMethodHadler != NULL)
         {//Если есть обработчик метода GET, используем специальный
            error = wantedSensor->sensGetMethodHadler(connection, RestApi);
         }
         else
         {  //Если нет обработчика метода GET используем общий
            error = sensCommonGetHandler(connection, RestApi, wantedSensor);
         }
      }
      else//Если сенсор не найден возвращяем ошибку
      {
         rest_404_not_found(connection, "sensor not found\r\n");
      }
   }
   else
   { //
      sprintfListSensors(connection, RestApi);

      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restPostSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t maxLen = sizeof(restBuffer);
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
         p = sprintfSensor(restBuffer+p, maxLen, wantedSensor, RestApi->restVersion);
         p+=snprintf(restBuffer+p, maxLen-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден
      rest_404_not_found(connection, "Not found\r\n");
      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restPutSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t maxLen = sizeof(restBuffer);
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
         p = sprintfSensor(restBuffer+p, maxLen, wantedSensor, RestApi->restVersion);
         p+=snprintf(restBuffer+p, maxLen-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден
      rest_404_not_found(connection, "Not found\r\n");
      error = ERROR_NOT_FOUND;
   }
   return error;
}

error_t restDeleteSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t maxLen = sizeof(restBuffer);
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
         p = sprintfSensor(restBuffer+p, maxLen, wantedSensor, RestApi->restVersion);
         p+=snprintf(restBuffer+p, maxLen-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
   }
   else
   {//Если сенсор не найден
      rest_404_not_found(connection, "Not found\r\n");
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
static sensor_t * findSensorByName(const char * name)
{
   int sens_num;
   int i;
   size_t len=0;
   char * p = (char *) name;
   sensFunctions * sensor =NULL;
   sensor_t * result = NULL;
   while (ISALPHA(*p))
   {
      p++;
      len++;
   }
   if (*p=='_')
   {
      p++;
      sens_num = atoi(p);
      for (sensFunctions *cur_sensor = &__start_sens_functions; cur_sensor < &__stop_sens_functions; cur_sensor++)
      {
         if (NAME_EQU(name, len, cur_sensor->sensClassName))

         {
            sensor = cur_sensor;
            break;
         }
      }
      if (sensor)
      {
         for(i=0; i <= MAX_NUM_SENSORS; i++)
         {
            if (sensors[i].type == sensor->sensorType && sensors[i].id == sens_num)
            {
               result = &(sensors[i]);
               break;
            }

         }
      }
   }
   return result;
}

error_t sensorsGetValue(const char *name, double * value)
{
   error_t error = ERROR_OBJECT_NOT_FOUND;
   sensor_t * sensor = findSensorByName(name);

   if(sensor)
   {
      {
         switch (sensor->valueType)
         {
         case FLOAT:
            *value =  (double) sensorsGetValueFloat(sensor);
            error = NO_ERROR;
            break;
         case UINT16:
            *value = sensorsGetValueUint16(sensor);
            error = NO_ERROR;
            break;
         case CHAR://Not supported yet
         case PCHAR:
         default:
            break;
         }
      }
   }
   return error;
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
