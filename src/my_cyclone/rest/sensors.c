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
#include <math.h>
#include "debug.h"
#include "macros.h"
#include "../../expression_parser/variables_def.h"
char places[PLACES_CACHE_LENGTH];
char names[NAMES_CACHE_LENGTH];
char devices[DEVICES_CACHE_LENGTH];

char *pPlaces = &places[0];
char *pNames = &names[0];
char *pDevices = &devices[0];



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
   {S_WATER_LEVEL, "water level"}, // Water meter, V_FLOW, V_VOLUME
   {S_AIR_QUALITY, ""}, // Air quality sensor, V_LEVEL
   {S_CUSTOM, ""}, // Custom sensor
   {S_MORZE, "sequential"},
   {S_DUST, ""}, // Dust sensor, V_LEVEL
   {S_SCENE_CONTROLLER, "scene"}, // Scene controller device, V_SCENE_ON, V_SCENE_OFF.
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

//static char buf[]="00:00:00:00:00:00:00:00/0";

#define CURRELOFARRAY(element, array) (element > &array)?(element-&array):0

sensor_t sensors[MAX_NUM_SENSORS];

register_rest_function(sensors, "/sensors", &restInitSensors, &restDenitSensors, &restGetSensors,NULL,NULL,NULL);// &restPostSensors, &restPutSensors, &restDeleteSensors);
register_variables_functions(sensors, &sensorsGetValue);

static mysensor_sensor_t findTypeByName(const char * type);

static int sprintfSensor (char * bufer, int maxLen, mysensor_sensor_t curr_list_el, int restVersion)
{
   int p=0;
   if (restVersion == 1)
   {
      p+=snprintf(bufer+p, maxLen-p, "{\"name\":\"%s\",\"path\":\"%s/v1/sensors/%s\"]},\r\n", sensorList[curr_list_el].string, &restPrefix[0], sensorList[curr_list_el].string);

   }
   else if(restVersion ==2 )
   {
      p+=snprintf(bufer+p, maxLen-p, "\"%s\":{\"links\":{\"related\": \"%s/v2/sensors/%s\"}},\r\n",sensorList[curr_list_el].string, &restPrefix[0], sensorList[curr_list_el].string);
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

   for (mysensor_sensor_t cur_list_elem = 0; cur_list_elem < MYSENSOR_ENUM_LEN; cur_list_elem++)
   {
      if (*sensorList[cur_list_elem].string != '\0')
      {
         p+=sprintfSensor(restBuffer+p, maxLen - p, cur_list_elem, RestApi->restVersion);

      }
   }
   p--;
   p--;
   p--;
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


static int snprintfSensor(char * bufer, size_t maxLen, sensor_t * sensor, int restVersion)
{
   int p=0;
   int d1, d2;
   double f2, f_val;
   int i;

   if (restVersion == 1)
   {
      p+=snprintf(bufer+p, maxLen-p, "{\"id\":\"%s\",", sensor->device+1);
   }
   else if (restVersion ==2)
   {
      p+=snprintf(bufer+p, maxLen-p, "{\"type\":\"%s\",\"id\":\"%s\",\"attributes\":{",sensorList[sensor->type].string, sensor->device+1);
   }
   //   p+=snprintf(bufer+p, maxLen-p, "\"type\":\"%s\",", sensorList[sensors[i].subType].string);
   p+=snprintf(bufer+p, maxLen-p, "\"name\":\"%s\",", sensor->name);
   p+=snprintf(bufer+p, maxLen-p, "\"place\":\"%s\",", sensor->place);
   //
   switch (sensor->fd.driver->dataType)
   {
   case FLOAT:
      driver_read(&sensor->fd, &f_val, sizeof(f_val));
      d1 = f_val;            // Get the integer part (678).
      f2 = f_val - d1;     // Get fractional part (678.0123 - 678 = 0.0123).
      d2 = abs(trunc(f2 * 10));   // Turn into integer (123).
      p+=snprintf(bufer+p, maxLen-p, "\"value\":\"%d.%01d\",", d1, d2);
      break;
   case UINT16:
      //      p+=snprintf(bufer+p, maxLen-p, "\"value\":\"%u\",",sensorsGetValueUint16(&sensors[i]));
      break;
   case CHAR://Not supported yet
      break;
   case PCHAR:
      p+=snprintf(bufer+p, maxLen-p, "\"value\":\"");
      p+=driver_read(&sensor->fd, bufer+p, maxLen-p);
      p+=snprintf(bufer+p, maxLen-p, "\",");
      break;
   default:
      break;
   }
   //   p+=snprintf(bufer+p, maxLen-p, "\"serial\":\"%s\",",serialHexToString(sensors[i].serial, &buf[0], ONEWIRE_SERIAL_LENGTH));
   //   p+=snprintf(bufer+p, maxLen-p, "\"health\":%d,", sensorsHealthGetValue(&sensors[i]));
   if (restVersion == 1)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"online\":%s},", ((sensor->fd.status & DEV_STAT_ONLINE)?"true":"false"));
   }
   else if (restVersion == 2)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"online\":%s}},", ((sensor->fd.status & DEV_STAT_ONLINE)?"true":"false"));
   }

   return p;
}

static error_t sensCommonGetHandler(HttpConnection *connection, RestApi_t* RestApi, mysensor_sensor_t sens_type)
{
   int p=0;
   error_t error = ERROR_NOT_FOUND;
   const size_t maxLen = sizeof(restBuffer);

   if (RestApi->restVersion == 1)
   {
      p=sprintf(&restBuffer[0],"{\"%s\":[\r\n", sensorList[sens_type].string);
   }

   else if (RestApi->restVersion ==2)
   {
      p=sprintf(restBuffer,"{\"data\":[\r\n");
   }


   if (RestApi->objectId != NULL)
   {
      for (sensor_t * curr_sensor = &sensors[0]; curr_sensor < &sensors[arraysize(sensors)]; curr_sensor++)
      {
         if ((curr_sensor->type == sens_type) && (strncmp(curr_sensor->device, RestApi->objectId, RestApi->objectIdLen) == 0))
         {

            error = driver_open(&curr_sensor->fd, RestApi->objectId, 0);

            if (error)
               return error;

            p+=snprintfSensor(&restBuffer[p], maxLen-p, curr_sensor, RestApi->restVersion);
            error = NO_ERROR;
            driver_close(&curr_sensor->fd);
            break;
         }
      }
   }
   else
   {
      for (sensor_t * curr_sensor = &sensors[0]; curr_sensor < &sensors[arraysize(sensors)]; curr_sensor++)
      {
         if (curr_sensor->type == sens_type)
         {

            error = driver_open(&curr_sensor->fd, curr_sensor->device, 0);

            if (error)
               return error;

            p+=snprintfSensor(&restBuffer[p], maxLen-p, curr_sensor, RestApi->restVersion);
            error = NO_ERROR;
            driver_close(&curr_sensor->fd);
         }
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

   error_t error = ERROR_FILE_NOT_FOUND;

   /* Check, what sensor type is defined in request  */
   if (RestApi->classNameLen > 0)
   {
      /* Проверяем, соответствует ли тип датчиака в RestApi->className*/
      for (mysensor_sensor_t i =0;i<MYSENSOR_ENUM_LEN; i++) // Contain length of enum
      {
         if (strncmp(sensorList[i].string, (RestApi->className)+1, RestApi->classNameLen-1) ==0) // Exclude '/' in className
         {
            return sensCommonGetHandler(connection, RestApi, i);

         }
      }
   }
   else
   {
      /* Print all avalible sensors*/
      sprintfListSensors(connection, RestApi);
   }
   return error;
}

error_t restPostSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t maxLen = sizeof(restBuffer);
   error_t error = NO_ERROR;
   //   sensFunctions * wantedSensor;
   //   wantedSensor = openPeripheral(RestApi);
   //   if (wantedSensor != NULL)
   //   {//Если сенсор найден
   //      if (wantedSensor->sensPostMethodHadler != NULL)
   //      {//Если есть обработчик метода POST
   //         error = wantedSensor->sensPostMethodHadler(connection, RestApi);
   //      }
   //      else
   //      {  //Если нет обработчика метода POST печатаем все доступные методы
   //         p = sprintfSensor(restBuffer+p, maxLen, wantedSensor, RestApi->restVersion);
   //         p+=snprintf(restBuffer+p, maxLen-p, "\r\n");
   //         rest_501_not_implemented(connection, &restBuffer[0]);
   //         error = ERROR_NOT_IMPLEMENTED;
   //      }
   //   }
   //   else
   //   {//Если сенсор не найден
   //      rest_404_not_found(connection, "Not found\r\n");
   //      error = ERROR_NOT_FOUND;
   //   }
   return error;
}

error_t restPutSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   const size_t maxLen = arraysize(restBuffer);
   error_t error = NO_ERROR;
   //   sensFunctions * wantedSensor;
   //   wantedSensor = openPeripheral(RestApi);
   //   if (wantedSensor != NULL)
   //   {//Если сенсор найден
   //      if (wantedSensor->sensPutMethodHadler != NULL)
   //      {//Если есть обработчик метода PUT
   //         error = wantedSensor->sensPutMethodHadler(connection, RestApi);
   //      }
   //      else
   //      {  //Если нет обработчика метода PUT печатаем все доступные методы
   //         p = sprintfSensor(restBuffer+p, maxLen, wantedSensor, RestApi->restVersion);
   //         p+=snprintf(restBuffer+p, maxLen-p, "\r\n");
   //         rest_501_not_implemented(connection, &restBuffer[0]);
   //         error = ERROR_NOT_IMPLEMENTED;
   //      }
   //   }
   //   else
   //   {//Если сенсор не найден
   //      rest_404_not_found(connection, "Not found\r\n");
   //      error = ERROR_NOT_FOUND;
   //   }
   return error;
}

//error_t restDeleteSensors(HttpConnection *connection, RestApi_t* RestApi)
//{
//   int p=0;
//   const size_t maxLen = sizeof(restBuffer);
//   error_t error = NO_ERROR;
//   sensFunctions * wantedSensor;
//   wantedSensor = restFindSensor(RestApi);
//   if (wantedSensor != NULL)
//   {//Если сенсор найден
//      if (wantedSensor->sensDeleteMethodHadler != NULL)
//      {//Если есть обработчик метода DELETE
//         error = wantedSensor->sensDeleteMethodHadler(connection, RestApi);
//      }
//      else
//      {  //Если нет обработчика метода DELETE печатаем все доступные методы
//         p = sprintfSensor(restBuffer+p, maxLen, wantedSensor, RestApi->restVersion);
//         p+=snprintf(restBuffer+p, maxLen-p, "\r\n");
//         rest_501_not_implemented(connection, &restBuffer[0]);
//         error = ERROR_NOT_IMPLEMENTED;
//      }
//   }
//   else
//   {//Если сенсор не найден
//      rest_404_not_found(connection, "Not found\r\n");
//      error = ERROR_NOT_FOUND;
//   }
//   return error;
//}

char* sensorsFindDevice(const char * device, size_t length)
{
   char * p = &devices[0];
   while (p < &(devices[DEVICES_CACHE_LENGTH-1]))
   {
      if (strncmp(device, p, length)==0)
      {
         return p;
      }
      p = strchr(++p,'\0');
   }
   return NULL;
}

char* sensorsAddDevice(const char * device, size_t length)
{
   if (length+pDevices<&(devices[DEVICES_CACHE_LENGTH-1]))
   {
      memcpy(pDevices, device, length);
      pDevices+=length;
      *pDevices = '\0';
      pDevices++;
      return pDevices-(length+1);
   }
   return NULL;
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

static sensor_t *findFreeSensor(void)
{
   sensor_t * curr_sensor = &sensors[0];
   while ( curr_sensor < &sensors[MAX_NUM_SENSORS])
   {
      if (curr_sensor->device == NULL)
      {
         return curr_sensor;
      }
      curr_sensor++;
   }
   return NULL; //No free sensors
}


static error_t parseSensors (char *data, size_t length, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens)
{
   sensor_t * currentSensor;
   char jsonPATH[64]; //

   char device[64];

   char parameter[64];

   char * pcParameter;

   char * name = &parameter[0];   /* Use bufer for copying name. */
   char * place = &parameter[0];  /* And place too. */
   char * type = &parameter[0];  /* And type %-) */

   char value[64];
   error_t error;
   int result = 1;
   int parameters;

   uint32_t input_num;
   uint32_t parameter_num;
   size_t len;
   volatile int resultCode = 0;

   jsmn_init(jSMNparser);

   resultCode = jsmn_parse(jSMNparser, data, length, jSMNtokens, CONFIG_JSMN_NUM_TOKENS);

   input_num = 0;
   if(resultCode > 0)
   {
      while (result)
      {
         currentSensor = findFreeSensor();
         if (!currentSensor)
         {
            break;
         }
         snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.inputs[%"PRIu32"].device", input_num);
         result = jsmn_get_string(data, jSMNtokens, resultCode, &jsonPATH[0], &device[0], arraysize(device));

         if (result)
         {
            parameters = 1;
            parameter_num = 0;
            error = driver_open(&(currentSensor->fd), &device[0], 0);
            if (!error)
            {
               /*Setting up parameters*/
               while (parameters)
               {
                  /* Damn!!! We can't find name of the odject by XJOSNPATH
                   * try to get it manually.
                   * It's not clean, but it should work */
                  snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.inputs[%"PRIu32"].parameters[%"PRIu32"]", input_num, parameter_num);
                  parameters = jsmn_get_string(data, jSMNtokens, resultCode, &jsonPATH[0], &parameter[0], arraysize(parameter));
                  if (parameters)
                  {
                     pcParameter = strchr(parameter, '\"'); /*Find opening quote */

                     if (pcParameter)
                     {
                        pcParameter++;
                        strtok(pcParameter, "\"");  /*Truncate string by closing quote*/
                     }

                     snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.inputs[%"PRIu32"].parameters[%"PRIu32"]%s", input_num, parameter_num, pcParameter);
                     parameters = jsmn_get_string(data, jSMNtokens, resultCode, &jsonPATH[0], &value[0], arraysize(value));
                     if (parameters)
                     {
                        driver_ioctl(&(currentSensor->fd), pcParameter, &value[0]);
                     }
                     parameter_num++;
                  }
               }
               /*Store device*/

               currentSensor->device = sensorsFindDevice(&device[0], strlen(&device[0]));

               if (currentSensor->device == 0)
               {
                  currentSensor->device = sensorsAddDevice(&device[0], strlen(&device[0]));

                  if (currentSensor->device == 0)
                  {
                     xprintf("error saving device: no avalible memory for saving device");
                  }
               }

               /*Setting up name and place*/
               snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.inputs[%"PRIu32"].name", input_num);

               len = jsmn_get_string(data, jSMNtokens, resultCode, &jsonPATH[0], name, arraysize(parameter));
               if(len>0)
               {
                  currentSensor->name = sensorsFindName(name, len);

                  if (currentSensor->name == 0)
                  {
                     currentSensor->name = sensorsAddName(name, len);

                     if (currentSensor->name == 0)
                     {
                        xprintf("error parsing input name: no avalible memory for saving name");
                     }
                  }
               }

               snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.inputs[%"PRIu32"].place", input_num);

               len = jsmn_get_string(data, jSMNtokens, resultCode, &jsonPATH[0], place, arraysize(parameter));
               if(len>0)
               {
                  currentSensor->place = sensorsFindPlace(place, len);

                  if (currentSensor->place == 0)
                  {
                     currentSensor->place = sensorsAddPlace(place, len);

                     if (currentSensor->place == 0)
                     {
                        xprintf("error parsing input place: no avalible memory for saving place");
                     }
                  }
               }
               /* Setting up type */
               snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.inputs[%"PRIu32"].type", input_num);

               len = jsmn_get_string(data, jSMNtokens, resultCode, &jsonPATH[0], type, arraysize(parameter));

               if(len>0)
               {
                  currentSensor->type = findTypeByName(type);
                  /*todo: check for undefined type*/
               }

               driver_ioctl(&(currentSensor->fd), "start", "");
            }
            /*If device configured and we did not reach end of devices*/
            if (&(currentSensor->fd) && currentSensor <= &sensors[MAX_NUM_SENSORS-1])
            {

               driver_close(&(currentSensor->fd));

            }
            else
            {
               driver_close(&(currentSensor->fd));

            }
         }
         input_num++;
      }
   }
   else
   {
      //    ntp_use_default_servers();
   }

   return NO_ERROR;
}
/*
static error_t initSensor(sensor_t * cur_sensor)
{
   error_t error = NO_ERROR;
   //Create a mutex to protect critical sections
   if(!osCreateMutex(&cur_sensor->mutex))
   {
      //Failed to create mutex
      xprintf("Sensors: Can't create sensor#%s mutex.\r\n", cur_sensor->device);
      error= ERROR_OUT_OF_RESOURCES;
   }
   if (!error)
   {
      //      error = sensorsHealthInit (cur_sensor);
   }
   return error;
}
*/
void sensorsConfigure(void)
{
   volatile error_t error = NO_ERROR;
   int i;

   memset (&sensors[0], 0, sizeof(sensor_t)*MAX_NUM_SENSORS);
//   for (i=0;i<MAX_NUM_SENSORS; i++)
//   {
//      initSensor(&sensors[i]);
//   }
   if (!error)
   {
      error = read_config("/config/sensors_draft.json", &parseSensors);
   }
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

//error_t sensorsHealthInit (sensor_t * sensor)
//{
//   //Enter critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthInit: Enter critical section sensor#%s.", sensor->device);
//   osAcquireMutex(&sensor->mutex);
//
//   sensor->health.value =SENSORS_HEALTH_MAX_VALUE;
//   sensor->health.counter =SENSORS_HEALTH_MIN_VALUE;
//
//   //Leave critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthInit: Leave critical section sensor#%s.\r\n", sensor->device);
//   osReleaseMutex(&sensor->mutex);
//
//   return NO_ERROR;
//}
//
//int sensorsHealthGetValue(sensor_t * sensor)
//{
//   int result;
//
//   //Enter critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthGetValue: Enter critical section sensor#%s.", sensor->device);
//   osAcquireMutex(&sensor->mutex);
//
//   result = sensor->health.value;
//
//   //Leave critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthGetValue: Leave critical section sensor#%s.\r\n", sensor->device);
//   osReleaseMutex(&sensor->mutex);
//
//   return result;
//}
//
//
//void sensorsHealthIncValue(sensor_t * sensor)
//{
//   //Enter critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthIncValue: Enter critical section sensor#%s.", sensor->device);
//   osAcquireMutex(&sensor->mutex);
//
//   if (sensor->health.value<SENSORS_HEALTH_MAX_VALUE)
//   {
//      sensor->health.value++;
//   }
//   if (sensor->health.counter<SENSORS_HEALTH_MAX_VALUE)
//   {
//      sensor->health.counter++;
//   }
//
//   //Leave critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthIncValue: Leave critical section sensor#%s.\r\n", sensor->device);
//   osReleaseMutex(&sensor->mutex);
//
//}
//
//void sensorsHealthDecValue(sensor_t * sensor)
//{
//
//   //Enter critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthDecValue: Enter critical section sensor#%s.", sensor->device);
//   osAcquireMutex(&sensor->mutex);
//
//
//   if (sensor->health.value>SENSORS_HEALTH_MIN_VALUE)
//   {
//      sensor->health.value--;
//   }
//   if (sensor->health.counter<SENSORS_HEALTH_MAX_VALUE)
//   {
//      sensor->health.counter++;
//   }
//
//   //Leave critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthDecValue: Leave critical section sensor#%s.\r\n", sensor->device);
//   osReleaseMutex(&sensor->mutex);
//
//}
//
//void sensorsHealthSetValue(sensor_t * sensor, int value)
//{
//
//   //Enter critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthSetValue: Enter critical section sensor#%s.", sensor->device);
//   osAcquireMutex(&sensor->mutex);
//
//
//   if (value>SENSORS_HEALTH_MAX_VALUE)
//   {
//      sensor->health.value=SENSORS_HEALTH_MAX_VALUE;
//   }
//   else if (value<SENSORS_HEALTH_MIN_VALUE)
//   {
//      sensor->health.value=SENSORS_HEALTH_MIN_VALUE;
//   }
//   else
//   {
//      sensor->health.value = value;
//   }
//
//   //Leave critical section
//   //Debug message
//   TRACE_INFO("sensorsHealthSetValue: Leave critical section sensor#%s.\r\n", sensor->device);
//   osReleaseMutex(&sensor->mutex);
//
//}
//
//
//

error_t sensorsGetValue(const char *name, double * value)
{
   error_t error = ERROR_OBJECT_NOT_FOUND;
//   sensor_t * sensor = findSensorByName(name);
//
//   if(sensor)
//   {
//      {
//         switch (sensor->valueType)
//         {
//         case FLOAT:
//            *value =  (double) sensorsGetValueFloat(sensor);
//            error = NO_ERROR;
//            break;
//         case UINT16:
//            *value = sensorsGetValueUint16(sensor);
//            error = NO_ERROR;
//            break;
//         case CHAR://Not supported yet
//         case PCHAR:
//         default:
//            break;
//         }
//      }
//   }
   return error;
}


static mysensor_sensor_t findTypeByName(const char * type)
{
   mysensor_sensor_t result = S_INPUT; /* Default value */
   mysensor_sensor_t num =0;
   while (num++ <= MYSENSOR_ENUM_LEN)
   {
      if (strcmp(sensorList[num].string, type) == 0)
      {
         result = num;
         break;
      }
   }

   return result;
}
