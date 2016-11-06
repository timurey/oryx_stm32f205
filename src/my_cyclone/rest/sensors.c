/*
 * sensors.c
 *
 * Created on: 25 июня 2015 г.
 * Author: timurtaipov
 */

//Switch to the appropriate trace level
#define TRACE_LEVEL 2

#include "rest/sensors.h"
#include "rest/sensors_def.h"
//#include "rest/input.h"
#include "configs.h"
#include <ctype.h>
#include <math.h>
#include "debug.h"
#include "macros.h"
#include "../../expression_parser/variables_def.h"
#include "../../list/list.h"


static int Place_equal(void *a, void *b) {
   char * PlaceA = a;
   Place * PlaceB = b;
   return 0 == strcmp(PlaceA, PlaceB->place);
}
static int Name_equal(void *a, void *b) {
   char * NameA = a;
   Name * NameB = b;
   return (0 == strcmp(NameA, NameB->name));
}
static int Device_equal(void *a, void *b) {
   char * DeviceA = a;
   ID * DeviceB = b;
   return 0 == strcmp(DeviceA, DeviceB->id);
}

list_t *places;
list_t *names;
list_t *deviceIds;


// Type of sensor (used when presenting sensors)
const mysensorSensorList_t sensorList[] =
   {
      { S_INPUT, "generic" },
      { S_DOOR, "" }, // Door sensor, V_TRIPPED, V_ARMED
      { S_MOTION, "" }, // Motion sensor, V_TRIPPED, V_ARMED
      { S_SMOKE, "" }, // Smoke sensor, V_TRIPPED, V_ARMED
      { S_LIGHT, "" }, // Binary light or relay, V_STATUS (or V_LIGHT), V_WATT
      { S_BINARY, "digital" }, // Binary light or relay, V_STATUS (or V_LIGHT), V_WATT (same as S_LIGHT)
      { S_DIMMER, "dimmer" }, // Dimmable light or fan device, V_STATUS (on/off), V_DIMMER (dimmer level 0-100), V_WATT
      { S_COVER, "" }, // Blinds or window cover, V_UP, V_DOWN, V_STOP, V_DIMMER (open/close to a percentage)
      { S_TEMP, "temperature" }, // Temperature sensor, V_TEMP
      { S_HUM, "" }, // Humidity sensor, V_HUM
      { S_BARO, "" }, // Barometer sensor, V_PRESSURE, V_FORECAST
      { S_WIND, "" }, // Wind sensor, V_WIND, V_GUST
      { S_RAIN, "" }, // Rain sensor, V_RAIN, V_RAINRATE
      { S_UV, "" }, // Uv sensor, V_UV
      { S_WEIGHT, "" }, // Personal scale sensor, V_WEIGHT, V_IMPEDANCE
      { S_POWER, "" }, // Power meter, V_WATT, V_KWH
      { S_HEATER, "" }, // Header device, V_HVAC_SETPOINT_HEAT, V_HVAC_FLOW_STATE, V_TEMP
      { S_DISTANCE, "" }, // Distance sensor, V_DISTANCE
      { S_LIGHT_LEVEL, "" }, // Light level sensor, V_LIGHT_LEVEL (uncalibrated in percentage), V_LEVEL (light level in lux)
      { S_ARDUINO_NODE, "" }, // Used (internally) for presenting a non-repeating Arduino node
      { S_ARDUINO_REPEATER_NODE, "" }, // Used (internally) for presenting a repeating Arduino node
      { S_LOCK, "" }, // Lock device, V_LOCK_STATUS
      { S_IR, "" }, // Ir device, V_IR_SEND, V_IR_RECEIVE
      { S_WATER, "" }, // Water meter, V_FLOW, V_VOLUME
      { S_WATER_LEVEL, "waterlevel" }, // Water meter, V_FLOW, V_VOLUME
      { S_AIR_QUALITY, "" }, // Air quality sensor, V_LEVEL
      { S_CUSTOM, "" }, // Custom sensor
      { S_MORZE, "sequential" },
      { S_DUST, "" }, // Dust sensor, V_LEVEL
      { S_SCENE_CONTROLLER, "scene" }, // Scene controller device, V_SCENE_ON, V_SCENE_OFF.
      { S_RGB_LIGHT, "" }, // RGB light. Send color component data using V_RGB. Also supports V_WATT
      { S_RGBW_LIGHT, "" }, // RGB light with an additional White component. Send data using V_RGBW. Also supports V_WATT
      { S_COLOR_SENSOR, "" }, // Color sensor, send color information using V_RGB
      { S_HVAC, "" }, // Thermostat/HVAC device. V_HVAC_SETPOINT_HEAT, V_HVAC_SETPOINT_COLD, V_HVAC_FLOW_STATE, V_HVAC_FLOW_MODE, V_TEMP
      { S_MULTIMETER, "multimeter" }, // Multimeter device, V_VOLTAGE, V_CURRENT, V_IMPEDANCE
      { S_SPRINKLER, "" }, // Sprinkler, V_STATUS (turn on/off), V_TRIPPED (if fire detecting device)
      { S_WATER_LEAK, "" }, // Water leak sensor, V_TRIPPED, V_ARMED
      { S_SOUND, "" }, // Sound sensor, V_TRIPPED, V_ARMED, V_LEVEL (sound level in dB)
      { S_VIBRATION, "" }, // Vibration sensor, V_TRIPPED, V_ARMED, V_LEVEL (vibration in Hz)
      { S_MOISTURE, "" }, // Moisture sensor, V_TRIPPED, V_ARMED, V_LEVEL (water content or moisture in percentage?)
      { S_ERROR, "" }, //Wrong sensor
   };

//static char buf[]="00:00:00:00:00:00:00:00/0";

#define CURRELOFARRAY(element, array) (element > &array)?(element-&array):0

sensor_t sensors[MAX_NUM_SENSORS];

register_rest_function (sensor, "/sensor", &restInitSensors, &restDenitSensors, &restGetSensors, &restPostSensors, &restPutSensors, NULL); // &restPostSensors, &restPutSensors, &restDeleteSensors);
register_variables_functions (sensors, &sensorsGetValue);

static mysensor_sensor_t findTypeByName (const char * type);
static error_t parseJSONSensors (jsmnParserStruct * jsonParser, configMode mode);
static sensor_t *findSensor (char* pxdeviceName);

static int sprintfSensor (char * bufer, int maxLen, mysensor_sensor_t curr_list_el, int restVersion)
{
   int p = 0;
   if (restVersion == 1)
   {
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel2"{"PRIlevel3"\"name\":\"%s\","PRIlevel3"\"path\":\"%s/v1/sensors/%s\""PRIlevel2"},", sensorList[curr_list_el].string, &restPrefix[0], sensorList[curr_list_el].string);
   }
   else if (restVersion == 2)
   {
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"%s\":{"PRIlevel4"\"links\":{"PRIlevel5"\"related\":\"%s/v2/sensors/%s\""PRIlevel4"}"PRIlevel3"},", sensorList[curr_list_el].string, &restPrefix[0], sensorList[curr_list_el].string);
   }
   return p;
}

static error_t sprintfListSensors (HttpConnection *connection, RestApi_t* RestApi)
{
   int p = 0;
   const size_t maxLen = arraysize(restBuffer);
   error_t error = NO_ERROR;
   if (RestApi->restVersion == 1)
   {
      p += snprintf(&restBuffer[p], maxLen - p, "{"PRIlevel1"\"sensors\":[");
   }
   else if (RestApi->restVersion == 2)
   {
      p +=snprintf(&restBuffer[p], maxLen - p, "{"PRIlevel1"\"data\":{"PRIlevel2"\"type\":\"sensors\","PRIlevel2"\"id\":0,"PRIlevel2"\"relationships\":{");
   }

   for (mysensor_sensor_t cur_list_elem = 0; cur_list_elem < MYSENSOR_ENUM_LEN; cur_list_elem++)
   {
      if (*sensorList[cur_list_elem].string != '\0')
      {
         p += sprintfSensor(restBuffer + p, maxLen - p, cur_list_elem, RestApi->restVersion);

      }
   }
   p--;
   if (RestApi->restVersion == 1)
   {
      p += snprintf(&restBuffer[p], maxLen - p, ""PRIlevel1"]"PRIlevel0"}\r\n");
   }
   else if (RestApi->restVersion == 2)
   {
      p += snprintf(&restBuffer[p], maxLen - p, ""PRIlevel2"}"PRIlevel1"}"PRIlevel0"}\r\n");
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

error_t restInitSensors(void) {
   error_t error = NO_ERROR;
   return error;
}

error_t restDenitSensors(void) {
   error_t error = NO_ERROR;
   //Destroy lists
   list_destroy(places);
   list_destroy(names);
   list_destroy(deviceIds);

   return error;
}

static int snprintfSensor(char * bufer, size_t maxLen, sensor_t * sensor, int restVersion)
{
   int p = 0;
   int saved_p, changed_p;

   uint16_t u16;
   int d1, d2;
   double f2, f_val;

   char * property;
   size_t propNum;
   if (restVersion == 1)
   {
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel2"{");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"id\":\"%s\",", sensor->device + 1);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"name\":\"%s\",", sensor->name);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"place\":\"%s\",", sensor->place);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"value\":");
   }
   else if (restVersion == 2)
   {
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel2"{");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"type\":\"sensor\",");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"id\":\"%s\",", sensor->device + 1);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"attributes\":{");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"type\":\"%s\",", sensorList[sensor->type].string);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"name\":\"%s\",", sensor->name);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"place\":\"%s\",", sensor->place);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"value\":");
   }

   switch (sensor->fd->driver->dataType)
   {
   case FLOAT:
      driver_read(sensor->fd, &f_val, sizeof(f_val));
      d1 = f_val; // Get the integer part (678).
      f2 = f_val - d1; // Get fractional part (678.0123 - 678 = 0.0123).
      d2 = abs(trunc(f2 * 10)); // Turn into integer (123).
      p += snprintf(bufer + p, maxLen - p, "%d.%01d", d1, d2);
      break;
   case UINT16:
      if (driver_read(sensor->fd, &u16, sizeof(uint16_t)) == sizeof(uint16_t))
      {
         p += snprintf(bufer + p, maxLen - p, "%"PRIu16"", u16);
      }
      break;
   case CHAR: //Not supported yet
      break;
   case PCHAR:
      p += snprintf(bufer + p, maxLen - p, "\"");
      p += driver_read(sensor->fd, bufer + p, maxLen - p);
      p += snprintf(bufer + p, maxLen - p, "\"");
      break;
   default:
      break;
   }
   p += snprintf(bufer + p, maxLen - p, ",");

   if (restVersion == 1)
   {
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"online\":");
      p += driver_getproperty(sensor->fd, "active", bufer + p, maxLen - p);
      p += snprintf(bufer + p, maxLen - p, ",");
      saved_p = p;

      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"min\":");
      changed_p = driver_getproperty(sensor->fd, "min", bufer + p, maxLen - p);
      if (changed_p > 0)
      {
         p += changed_p;
         p += snprintf(bufer + p, maxLen - p, ",");
      }
      else
      {
         p = saved_p;
      }
      saved_p = p;

      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"max\":");
      changed_p = driver_getproperty(sensor->fd, "max", bufer + p, maxLen - p);
      if (changed_p > 0)
      {
         p += changed_p;
         p += snprintf(bufer + p, maxLen - p, ",");
      }
      else
      {
         p = saved_p;
      }

      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"parameters\":[");
      for (propNum = 0; propNum < sensor->fd->driver->propertyList->numOfProperies; propNum++)
      {
         property = (char*) sensor->fd->driver->propertyList->properties[propNum].property;
         if (! ((strcmp(property, "min") == 0) || strcmp(property, "max")==0) )
         {
            saved_p = p;
            p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"{"PRIlevel5"\"name\":\"%s\",\"value\":", property);
            changed_p = driver_getproperty(sensor->fd, property, bufer + p, maxLen - p);
            if (changed_p > 0)
            {
               p += changed_p;
               p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"},");
            }
            else
            {
               p = saved_p;
            }
         }
      }
      p--;
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"]");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel2"},");
   }
   else if (restVersion == 2)
   {
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"online\":");
      p += driver_getproperty(sensor->fd, "active", bufer + p, maxLen - p);
      p += snprintf(bufer + p, maxLen - p, ",");

      saved_p = p;

      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"min\":");
      changed_p = driver_getproperty(sensor->fd, "min", bufer + p, maxLen - p);
      if (changed_p > 0)
      {
         p += changed_p;
         p += snprintf(bufer + p, maxLen - p, ",");
      }
      else
      {
         p = saved_p;
      }
      saved_p = p;

      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"max\":");
      changed_p = driver_getproperty(sensor->fd, "max", bufer + p, maxLen - p);
      if (changed_p > 0)
      {
         p += changed_p;
         p += snprintf(bufer + p, maxLen - p, ",");
      }
      else
      {
         p = saved_p;
      }

      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"parameters\":[");

      for (propNum = 0; propNum < sensor->fd->driver->propertyList->numOfProperies; propNum++)
      {
         property = (char*) sensor->fd->driver->propertyList->properties[propNum].property;
         if (! ((strcmp(property, "min") == 0) || strcmp(property, "max")==0) )
         {
            saved_p = p;
            p += snprintf(bufer + p, maxLen - p, ""PRIlevel5"{"PRIlevel6"\"name\":\"%s\",\"value\":", property);
            changed_p = driver_getproperty(sensor->fd, property, bufer + p, maxLen - p);
            if (changed_p > 0)
            {
               p += changed_p;
               p += snprintf(bufer + p, maxLen - p, ""PRIlevel5"},");
            }
            else
            {
               p = saved_p;
            }
         }
      }
      p--;
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"]");

      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"}"PRIlevel2"},");
   }

   return p;
}
#if 0
static error_t sensCommonGetHandler(HttpConnection *connection, RestApi_t* RestApi, mysensor_sensor_t sens_type)
{
   int p = 0;
   error_t error = ERROR_NOT_FOUND;
   const size_t maxLen = sizeof(restBuffer);

   if (RestApi->restVersion == 1)
   {
      p = sprintf(&restBuffer[0], "{"PRIlevel1"\"%s\":[", sensorList[sens_type].string);
   }

   else if (RestApi->restVersion == 2)
   {
      p = sprintf(restBuffer, "{"PRIlevel1"\"data\":[");
   }

   if (RestApi->objectId != NULL)
   {
      for (sensor_t * curr_sensor = &sensors[0]; curr_sensor < &sensors[arraysize(sensors)]; curr_sensor++)
      {
         if ((curr_sensor->type == sens_type) && (strncmp(curr_sensor->id, RestApi->objectId, RestApi->objectIdLen) == 0))
         {
            error = driver_open(&curr_sensor->fd, RestApi->objectId, POPEN_READ);

            if (error == NO_ERROR)
            {
               p += snprintfSensor(&restBuffer[p], maxLen - p, curr_sensor, RestApi->restVersion);
               driver_close(&curr_sensor->fd);
               break;
            }
         }
      }
   }
   else
   {
      for (sensor_t * curr_sensor = &sensors[0]; curr_sensor < &sensors[arraysize(sensors)]; curr_sensor++)
      {
         if (curr_sensor->type == sens_type)
         {

            error = driver_open(&curr_sensor->fd, curr_sensor->id, POPEN_READ);

            if (error == NO_ERROR)
            {
               p += snprintfSensor(&restBuffer[p], maxLen - p, curr_sensor, RestApi->restVersion);
               driver_close(&curr_sensor->fd);
            }
         }
      }
   }
   if (error == NO_ERROR)
   {
      p--;
   }
   p += snprintf(restBuffer + p, maxLen - p, ""PRIlevel1"]"PRIlevel0"}\r\n");
   if (RestApi->restVersion == 1)
   {
      connection->response.contentType = mimeGetType(".json");
   }
   else if (RestApi->restVersion == 2)
   {
      connection->response.contentType = mimeGetType(".apijson");
   }

   switch (error)
   {
   case NO_ERROR:
      return rest_200_ok(connection, &restBuffer[0]);
      break;
   case ERROR_UNSUPPORTED_REQUEST:
      connection->response.contentType = mimeGetType(".txt");
      return rest_400_bad_request(connection, "400 Bad Request.\r\n");
      break;
   case ERROR_NOT_FOUND:
   default:
      connection->response.contentType = mimeGetType(".txt");
      return rest_404_not_found(connection, "404 Not Found.\r\n");
      break;
   }

   return error;
}
#endif

static mysensor_sensor_t findSensorType(char * name, size_t len)
{
   mysensor_sensor_t i;
   for ( i = 0; i < MYSENSOR_ENUM_LEN; i++) // Contain length of enum
   {
      if (*sensorList[i].string != '\0')
      {
         if (strlen(sensorList[i].string) == len)
         {
            if (strncmp(sensorList[i].string, name, len) == 0) // Exclude '/' in className
            {
               break;
            }
         }
      }
   }
   return i;
}
static int sprintf_single_header(char * buffer, int maxLen, mysensor_sensor_t sensorType, int restVersion)
{
   int length = 0;
   if (restVersion == 1)
   {
      length = snprintf(buffer, maxLen, "{"PRIlevel1"\"%s\":{", sensorList[sensorType].string);
   }

   else if (restVersion == 2)
   {
      length = snprintf(buffer, maxLen, "{"PRIlevel1"\"data\":");
   }
   return length;
}
static int sprintf_header(char * buffer, int maxLen, mysensor_sensor_t sensorType, int restVersion)
{
   int length = 0;
   if (restVersion == 1)
   {
      length = snprintf(buffer, maxLen, "{"PRIlevel1"\"%s\":[", sensorList[sensorType].string);
   }

   else if (restVersion == 2)
   {
      length = snprintf(buffer, maxLen, "{"PRIlevel1"\"data\":[");
   }
   return length;
}

static int sprintf_single_footer(char * buffer, int maxLen, mysensor_sensor_t sensorType __attribute__((unused)), int restVersion __attribute__((unused)))
{
   int length ;
   length = snprintf(buffer, maxLen, PRIlevel1"}\r\n");
   return length;
}
static int sprintf_footer(char * buffer, int maxLen, mysensor_sensor_t sensorType __attribute__((unused)), int restVersion __attribute__((unused)))
{
   int length ;
   length = snprintf(buffer, maxLen, ""PRIlevel1"]"PRIlevel0"}\r\n");
   return length;
}

/*
 * continue here:
 */
static error_t sensor_open (sensor_t **sensor, const char * sensorPath, const size_t sensorPathLength, const periphOpenType mode)
{
   error_t sError = ERROR_FILE_NOT_FOUND;

   for (sensor_t * curr_sensor = &sensors[0]; curr_sensor < &sensors[arraysize(sensors)]; curr_sensor++)
   {
      if (curr_sensor->device != NULL)
      {
         if (strncmp(curr_sensor->device, sensorPath, sensorPathLength) == 0)
         {
            curr_sensor->fd = driver_open(sensorPath, mode);
            if (curr_sensor->fd != NULL)
            {

               *sensor = curr_sensor;
               break;
            }
         }
      }
      //
   }
   return sError;
}

error_t restGetSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   enum filteringType {
      None,
      Full,
      Path,
      Type,
      Empty
   } request;
   error_t error = ERROR_FILE_NOT_FOUND;
   error_t dError = ERROR_FILE_NOT_FOUND;
   mysensor_sensor_t sensType = S_INPUT;
   char * sensorType = NULL;
   size_t sensorTypeLength = 0;

   char * sensorPath = NULL;
   size_t sensorPathLength = 0;

   sensor_t *currSensor = NULL;

   int p = 0, tp = 0;
   size_t maxLen = arraysize(restBuffer);

   request = None;

   /*
    * Try to recognize, what RestApi contains
    * where is sensor type and sensor path
    * /rest/v2/sensors/temperature/test_7  - full request
    * /rest/v2/sensors/test_7 - request contain path only
    * /rest/v2/sensors/temperature - request cotain type only
    * /rest/v2/sensors - request is empty
    *
    *
    */
   if (RestApi->objectIdLen > 1)
   {
      sensorType = RestApi->className + 1;
      sensorTypeLength = RestApi->classNameLen - 1;
      sensType = findSensorType(sensorType, sensorTypeLength);

      sensorPath = RestApi->objectId;
      sensorPathLength = RestApi->objectIdLen;
      request = Full;
   }
   else if (RestApi->classNameLen > 1)
   {
      //try to recognize sensor type
      sensorType = RestApi->className + 1;
      sensorTypeLength = RestApi->classNameLen - 1;
      sensType = findSensorType(sensorType, sensorTypeLength);

      if (sensType < MYSENSOR_ENUM_LEN)
      {
         sensorPath = RestApi->objectId;
         sensorPathLength = RestApi->objectIdLen;
         request = Type;
      }
      else
      {
         sensorPath = RestApi->className;
         sensorPathLength = RestApi->classNameLen;
         request = Path;
      }
   }
   else
   {
      request = Empty;
   }
   //Try to open sensor by path

   if (request == Full || request == Path)
   {
      dError = sensor_open(&currSensor, sensorPath, sensorPathLength, POPEN_READ);
      if (dError == NO_ERROR)
      {
         if ((request == Path) || (request == Full && sensType == currSensor->type))
         {
            error = NO_ERROR;
            p += sprintf_single_header(&restBuffer[0], arraysize(restBuffer) - p, sensType, RestApi->restVersion);
            p += snprintfSensor(&restBuffer[p], maxLen - p, currSensor, RestApi->restVersion);
            p --; //Remove last ',' symbol
            p+=sprintf_single_footer(&restBuffer[p], arraysize(restBuffer) - p, sensType, RestApi->restVersion);
         }

         driver_close(currSensor->fd);
      }
      else
      {
         error = dError;
      }
   }
   else if (request == Type)
   {
      p += sprintf_header(&restBuffer[0], arraysize(restBuffer) - p, sensType, RestApi->restVersion);
      tp = p;
      for (sensor_t * curr_sensor = &sensors[0]; curr_sensor < &sensors[arraysize(sensors)]; curr_sensor++)
      {
         if (curr_sensor->device != NULL)
         {
            if (curr_sensor->type == sensType)
            {
               curr_sensor->fd = driver_open(curr_sensor->device, POPEN_READ);
               if (curr_sensor->fd != NULL)
               {
                  error = NO_ERROR;
                  p += snprintfSensor(&restBuffer[p], maxLen - p, curr_sensor, RestApi->restVersion);
                  driver_close(curr_sensor->fd);
               }
            }
         }
      }
      if (tp != p) //If any sensor has been printed
      {
         p--;
      }
      p+=sprintf_footer(&restBuffer[p], arraysize(restBuffer) - p, sensType, RestApi->restVersion);
   }
   else if (request == Empty)
   {
      //Not supported by rest v1
      if (RestApi->restVersion != 1)
      {
         p += sprintf_header(&restBuffer[0], arraysize(restBuffer) - p, sensType, RestApi->restVersion);
         tp = p;
         for (sensor_t * curr_sensor = &sensors[0]; curr_sensor < &sensors[arraysize(sensors)]; curr_sensor++)
         {
            if (curr_sensor->device != NULL)
            {

               curr_sensor->fd = driver_open(curr_sensor->device, POPEN_READ);
               if (curr_sensor->fd != NULL)
               {
                  error = NO_ERROR;
                  p += snprintfSensor(&restBuffer[p], maxLen - p, curr_sensor, RestApi->restVersion);
                  driver_close(curr_sensor->fd);
               }
            }
         }
         if (tp != p) //If any sensor has been printed
         {
            p--;
         }
         p+=sprintf_footer(&restBuffer[p], arraysize(restBuffer) - p, sensType, RestApi->restVersion);
         //         if (RestApi->restVersion == 1)
         //         {
         //            p += snprintf(&restBuffer[p], maxLen - p, "{"PRIlevel1"\"sensors\":[");
         //         }
         //         else if (RestApi->restVersion == 2)
         //         {
         //            p +=snprintf(&restBuffer[p], maxLen - p, "{"PRIlevel1"\"data\":{"PRIlevel2"\"type\":\"sensors\","PRIlevel2"\"id\":0,"PRIlevel2"\"relationships\":{");
         //         }
         //
         //         for (mysensor_sensor_t cur_list_elem = 0; cur_list_elem < MYSENSOR_ENUM_LEN; cur_list_elem++)
         //         {
         //            if (*sensorList[cur_list_elem].string != '\0')
         //            {
         //               p += sprintfSensor(restBuffer + p, maxLen - p, cur_list_elem, RestApi->restVersion);
         //               error = NO_ERROR;
         //            }
         //         }
         //         p--;
         //         if (RestApi->restVersion == 1)
         //         {
         //            p += snprintf(&restBuffer[p], maxLen - p, ""PRIlevel1"]"PRIlevel0"}\r\n");
         //         }
         //         else if (RestApi->restVersion == 2)
         //         {
         //            p += snprintf(&restBuffer[p], maxLen - p, ""PRIlevel2"}"PRIlevel1"}"PRIlevel0"}\r\n");
         //         }
      }
   }
   switch (error)
   {
   case NO_ERROR:
      if (RestApi->restVersion == 1)
      {
         connection->response.contentType = mimeGetType(".json");
      }
      else if (RestApi->restVersion == 2)
      {
         connection->response.contentType = mimeGetType(".apijson");
      }
      error = rest_200_ok(connection, &restBuffer[0]);
      break;
   case ERROR_UNSUPPORTED_REQUEST:
      connection->response.contentType = mimeGetType(".txt");
      error = rest_400_bad_request(connection, "400 Bad Request.\r\n");
      break;
   case ERROR_NOT_FOUND:
   default:
      connection->response.contentType = mimeGetType(".txt");
      error = rest_404_not_found(connection, "404 Not Found.\r\n");
      break;
   }
   return error;
}

//Create a new sensor
// POST method
error_t restPostSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   jsmn_parser parser;
   // jsmntok_t tokens[CONFIG_JSMN_NUM_TOKENS]; // a number >= total number of tokens
   jsmnParserStruct jsonParser;
   // sensor_t * sensor;
   char path[64];
   int p = 0;
   // const size_t maxLen = sizeof(restBuffer);
   size_t received;
   error_t error = NO_ERROR;

   error = httpReadStream(connection, connection->buffer, connection->request.contentLength, &received, HTTP_FLAG_BREAK_CRLF);
   if (error)
   {
      return error;
   }
   if (received == connection->request.contentLength)
   {
      connection->buffer[received] = '\0';
   }

   jsonParser.jSMNparser = &parser;
   jsonParser.jSMNtokens = osAllocMem(sizeof(jsmntok_t) * CONFIG_JSMN_NUM_TOKENS);
   if (!jsonParser.jSMNtokens)
   {
      return ERROR_OUT_OF_MEMORY;
   }
   jsonParser.numOfTokens = CONFIG_JSMN_NUM_TOKENS;
   jsonParser.data = connection->buffer;
   jsonParser.lengthOfData = connection->request.contentLength;
   jsonParser.resultCode = 0;

   if (RestApi->restVersion == 1)
   {
      error = parseJSONSensors(&jsonParser, RESTv1POST);
   }
   else if (RestApi->restVersion == 2)
   {
      error = parseJSONSensors(&jsonParser, RESTv2POST);
   }
   //return 201 created
   if (error == NO_ERROR)
   {
      p+=snprintf(&path[0], arraysize(path), "/sensors/");
      p+= jsmn_get_string(&jsonParser, "$.data[0].type", &path[p], arraysize(path)-p);
      path[p++]='/';
      p+= jsmn_get_string(&jsonParser, "$.data[0].id", &path[p], arraysize(path)-p);

      connection->response.location= &path[0];
   }
   osFreeMem(jsonParser.jSMNtokens);

   switch (error)
   {
   case NO_ERROR:
      return rest_201_created(connection, &restBuffer[0]);
      break;
   case ERROR_INVALID_RECIPIENT:
      return rest_409_conflict(connection, "409 Conflict.\r\n");
      break;
   case ERROR_UNSUPPORTED_REQUEST:
      connection->response.contentType = mimeGetType(".txt");
      return rest_400_bad_request(connection, "400 Bad Request.\r\n");
      break;
   case ERROR_NOT_FOUND:
   default:
      connection->response.contentType = mimeGetType(".txt");
      return rest_404_not_found(connection, "404 Not Found.\r\n");
      break;
   }

   return error;
}

//Edit existing sensor
//For PUT or PATCH methods
error_t restPutSensors(HttpConnection *connection, RestApi_t* RestApi) {

   error_t error = NO_ERROR;
   char path[64];
   size_t received;
   jsmn_parser parser;
   jsmnParserStruct jsonParser;

   if (RestApi->className != NULL)
   {

      error = httpReadStream(connection, connection->buffer, connection->request.contentLength, &received, HTTP_FLAG_BREAK_CRLF);
      if (error)
      {
         return error;
      }
      if (received == connection->request.contentLength)
      {
         connection->buffer[received] = '\0';
      }

      jsonParser.jSMNparser = &parser;
      jsonParser.jSMNtokens = osAllocMem( sizeof(jsmntok_t) * CONFIG_JSMN_NUM_TOKENS);
      if (!jsonParser.jSMNtokens)
      {
         return ERROR_OUT_OF_MEMORY;
      }
      jsonParser.numOfTokens = CONFIG_JSMN_NUM_TOKENS;
      jsonParser.data = connection->buffer;
      jsonParser.lengthOfData = connection->request.contentLength;
      jsonParser.resultCode = 0;

      snprintf(&path[0], arraysize(path), "%.*s", RestApi->objectIdLen, RestApi->objectId );

      if (RestApi->restVersion == 1)
      {
         error = parseJSONSensors(&jsonParser, RESTv1PUT);
      }
      else if (RestApi->restVersion == 2)
      {
         error = parseJSONSensors(&jsonParser, RESTv2PUT);
      }
      osFreeMem(jsonParser.jSMNtokens);
   }
   switch (error)
   {
   case NO_ERROR:
      return rest_200_ok(connection, &restBuffer[0]);
      break;
   case ERROR_UNSUPPORTED_REQUEST:
      connection->response.contentType = mimeGetType(".txt");
      return rest_400_bad_request(connection, "400 Bad Request.\r\n");
      break;
   case ERROR_NOT_FOUND:
   default:
      connection->response.contentType = mimeGetType(".txt");
      return rest_404_not_found(connection, "404 Not Found.\r\n");
      break;
   }

   return error;
}

//error_t restDeleteSensors(HttpConnection *connection, RestApi_t* RestApi)
//{
// int p=0;
// const size_t maxLen = sizeof(restBuffer);
// error_t error = NO_ERROR;
// sensFunctions * wantedSensor;
// wantedSensor = restFindSensor(RestApi);
// if (wantedSensor != NULL)
// {//Если сенсор найден
// if (wantedSensor->sensDeleteMethodHadler != NULL)
// {//Если есть обработчик метода DELETE
// error = wantedSensor->sensDeleteMethodHadler(connection, RestApi);
// }
// else
// { //Если нет обработчика метода DELETE печатаем все доступные методы
// p = sprintfSensor(restBuffer+p, maxLen, wantedSensor, RestApi->restVersion);
// p+=snprintf(restBuffer+p, maxLen-p, "\r\n");
// rest_501_not_implemented(connection, &restBuffer[0]);
// error = ERROR_NOT_IMPLEMENTED;
// }
// }
// else
// {//Если сенсор не найден
// rest_404_not_found(connection, "Not found\r\n");
// error = ERROR_NOT_FOUND;
// }
// return error;
//}


char* sensorsAddDeviceId(const char * deviceId, size_t length)
{
   ID * deviceIdPtr = osAllocMem(sizeof(ID) + length + 1);

   if (deviceIdPtr)
   {
      memcpy(deviceIdPtr->id, deviceId, length);
      deviceIdPtr->ptr = &(deviceIdPtr->id[0]);
      *((deviceIdPtr->id) + length) = '\0';
      list_node_t * ptr = list_rpush(deviceIds, list_node_new(deviceIdPtr));
      if (ptr)
      {
         return &(deviceIdPtr->id[0]);
      }
      else
      {
         osFreeMem(deviceIdPtr);
      }
   }
   return NULL;
}

char* sensorsFindDeviceId( char * deviceId, size_t length)
{
   (void) length;

   list_node_t *a = list_find(deviceIds, deviceId);
   if (a)
   {
      ID * devicePtr = a->val;
      return &(devicePtr->id[0]);
   }
   return NULL;
}

/* todo: if allocator suppotrs, try to allocate static memory.
 * We'll never free this block, try to allocate it without fragmentation*/
char* sensorsAddName(const char * name, size_t length)
{
   Name * namePtr = osAllocMem(sizeof(Name) + length + 1);

   if (namePtr)
   {
      memcpy(namePtr->name, name, length);
      namePtr->ptr = &(namePtr->name[0]);
      *((namePtr->name) + length) = '\0';
      list_node_t * ptr = list_rpush(names, list_node_new(namePtr));
      if (ptr)
      {
         return &(namePtr->name[0]);
      }
      else
      {
         osFreeMem(namePtr);
      }
   }
   return NULL;
}

char* sensorsFindName(char * name, size_t length)
{
   (void) length;

   list_node_t *a = list_find(names, name);
   if (a)
   {
      Name * namePtr = a->val;
      return &(namePtr->name[0]);
   }
   return NULL;
}

/* todo: if allocator suppotrs, try to allocate static memory.
 * We'll never free this block, try to allocate it without fragmentation*/
char * sensorsAddPlace(const char * place, size_t length)
{
   Place * placePtr = osAllocMem(sizeof(Place) + length + 1);

   if (placePtr)
   {
      memcpy(placePtr->place, place, length);
      placePtr->ptr=&(placePtr->place[0]);
      *((placePtr->place) + length) = '\0';
      list_node_t * ptr = list_rpush(places, list_node_new(placePtr));
      if (ptr)
      {
         return &(placePtr->place[0]);
      }
      else
      {
         osFreeMem(placePtr);
      }
   }
   return NULL;
}

char * sensorsFindPlace(char * place, size_t length)
{
   (void) length;

   list_node_t *a = list_find(places, place);
   if (a)
   {
      Place * placePtr = a->val;
      return &(placePtr->place[0]);
   }
   return NULL;
}


static sensor_t *findSensor(char* pxdeviceName)
{
   sensor_t * curr_sensor = &sensors[0];
   /* If deviceName is defined, try to find it*/

   if (pxdeviceName != NULL && *pxdeviceName != '\0')
   {
      while (curr_sensor < &sensors[MAX_NUM_SENSORS])
      {
         if (strcmp(curr_sensor->device, pxdeviceName) == 0)
         {
            return curr_sensor;
         }
         curr_sensor++;
      }
   }

   curr_sensor = &sensors[0];
   /* If deviceName is not found in sensor list or it not defined*/
   while (curr_sensor < &sensors[MAX_NUM_SENSORS])
   {
      if (curr_sensor->device == NULL)
      {
         return curr_sensor;
      }
      curr_sensor++;
   }
   return NULL; //No sensors found
}

static char * storeDevice(char * device)
{
   char *ptr = NULL;
   size_t deviceLen = strlen(device);
   ptr = sensorsFindDeviceId(device, deviceLen);
   if (ptr == NULL)
   {
      ptr = sensorsAddDeviceId(device, deviceLen);
   }
   return ptr;

}

static char * storeName(char * name)
{
   char *ptr = NULL;
   size_t nameLen = strlen(name);
   ptr = sensorsFindName(name, nameLen);
   if (ptr == NULL)
   {
      ptr = sensorsAddName(name, nameLen);
   }
   return ptr;

}

static char * storePlace(char * place)
{
   size_t placeLen = strlen(place);
   char * ptr = sensorsFindPlace(place, placeLen);
   if (ptr == NULL)
   {
      ptr = sensorsAddPlace(place, placeLen);
   }
   return ptr;
}

static mysensor_sensor_t storeType(char * type)
{
   return findTypeByName(type);
}

static error_t parseJSONSensors(jsmnParserStruct * jsonParser, configMode mode)
{
   sensor_t * currentSensor;
   char jsonPATH[64]; //

   char deviceId[64];

   char parameter[64];
   char value[64];

   char * name = &parameter[0]; /* Use bufer for copying name. */
   char * place = &parameter[0]; /* And place too. */
   char * type = &parameter[0]; /* And type %-) */


   error_t error = NO_ERROR;
   int parameters;

   uint8_t path_type = (mode) & (JSONPATH);

   uint32_t parameterNum;
   size_t len;

   int sensorsToken;
   int countOfSensors = 0;
   int result;
   uint32_t sensorNum;

   uint32_t configured =0;
   jsmn_init(jsonParser->jSMNparser);

   jsonParser->resultCode = xjsmn_parse(jsonParser);

   if (jsonParser->resultCode > 0)
   {
      switch (path_type)
      {
      case CONFIGURE_Path: sensorsToken = jsmn_get_value_token(jsonParser, "$.sensors"); break;
      case RESTv2Path: sensorsToken = jsmn_get_value_token(jsonParser, "$.data"); break;
      default:
      {
         sensorsToken = 0;
         error = ERROR_UNSUPPORTED_CONFIGURATION;
         break;
      }
      }
      if (error == NO_ERROR)
      {
         if ((jsonParser->jSMNtokens+sensorsToken)->type == JSMN_ARRAY)
         {
            if (sensorsToken > 0 && sensorsToken < jsonParser->resultCode)
            {
               countOfSensors = (jsonParser->jSMNtokens+sensorsToken)->size;
            }
         }
         else if ((jsonParser->jSMNtokens+sensorsToken)->type == JSMN_OBJECT)
         {
            countOfSensors = 1;
         }
      }
      else
      {
         countOfSensors = 0;
         error = ERROR_UNSUPPORTED_CONFIGURATION;
      }

      for (sensorNum = 0; sensorNum< (uint32_t) countOfSensors; sensorNum++)
      {
         parameters = 1;
         parameterNum =0;
         error = NO_ERROR;

         switch (path_type)
         {
         case CONFIGURE_Path:
            snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].device", sensorNum);
            result = jsmn_get_string(jsonParser, &jsonPATH[0], &deviceId[0], arraysize(deviceId)-1);
            break;

         case RESTv2Path:
#if 0
            snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.data[%"PRIu32"].id", sensorNum);
            deviceId[0] = '/';
            result = jsmn_get_string(jsonParser, &jsonPATH[0], &deviceId[1], arraysize(deviceId)-1);
#endif
            snprintf(&jsonPATH[0], arraysize(jsonPATH), "%s", "$.data.id");
            deviceId[0] = '/';
            result = jsmn_get_string(jsonParser, &jsonPATH[0], &deviceId[1], arraysize(deviceId)-1);
            break;
         default: result = 0;
         }


         if (result)
         {
            currentSensor = findSensor(&deviceId[0]);

            if (!currentSensor) {
               error = ERROR_OUT_OF_MEMORY;
               break;
            }

            /* If sensor exist, and we can't edit it */
            if ((!(mode & EDITEBLE)) && (currentSensor->device != NULL))
            {
               error = ERROR_INVALID_RECIPIENT; //Sensor already exist. Check config file.
               continue;
            }

            currentSensor->fd = driver_open(&deviceId[0], POPEN_CONFIGURE);

            if (currentSensor->fd != NULL)
            {
               /* Если устройство уже запущено */
               if (currentSensor->fd->status->statusAttribute & DEV_STAT_ACTIVE)
               {
                  /* Stop device here for reconfigure */
                  driver_setproperty(currentSensor->fd, "active", "false");
               }
               /*Setting up parameters*/
               while (parameters)
               {
#if 0
                  /* Damn!!! We can't find name of the odject by XJOSNPATH
                   * try to get it manually.
                   * It's not clean, but it should work */
                  switch (path_type)
                  {
                  case CONFIGURE_Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].parameters[%"PRIu32"]", sensorNum, parameterNum); break;
                  case RESTv2Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.data[%"PRIu32"].attributes.parameters[%"PRIu32"]", sensorNum, parameterNum); break;
                  default: jsonPATH[0]='\0';
                  }
#endif
                  switch (path_type)
                  {
                  case CONFIGURE_Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].parameters[%"PRIu32"].name", sensorNum, parameterNum); break;
                  case RESTv2Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.data.attributes.parameters[%"PRIu32"].name", parameterNum); break;
                  default: jsonPATH[0]='\0';
                  }
                  parameters = jsmn_get_string(jsonParser, &jsonPATH[0], &parameter[0], arraysize(parameter));
                  if (parameters)
                  {
#if 0
                     pcParameter = strchr(parameter, '\"'); /*Find opening quote */

                     if (pcParameter)
                     {
                        pcParameter++;
                        strtok(pcParameter, "\""); /*Truncate string by closing quote*/
                     }
                     switch (path_type)
                     {
                     case CONFIGURE_Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].parameters[%"PRIu32"].%s", sensorNum, parameterNum, pcParameter);break;
                     case RESTv2Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.data.attributes.parameters[%"PRIu32"].%s", sensorNum, parameterNum, pcParameter); break;
                     default: jsonPATH[0]='\0';
                     }
#endif
                     switch (path_type)
                     {
                     case CONFIGURE_Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].parameters[%"PRIu32"].value", sensorNum, parameterNum);break;
                     case RESTv2Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.data.attributes.parameters[%"PRIu32"].value", parameterNum); break;
                     default: jsonPATH[0]='\0';
                     }
                     parameters = jsmn_get_string(jsonParser, &jsonPATH[0], &value[0], arraysize(value));
                     if (parameters)
                     {
                        driver_setproperty(currentSensor->fd, &parameter[0], &value[0]);
                     }
                     parameterNum++;
                  }
               }
               /*Store device*/
               if (driver_setproperty(currentSensor->fd, "active", "true") == 1)
               {
                  /* Let's store deviceId*/
                  currentSensor->device = storeDevice(&deviceId[0]);

                  /* Let's store name*/
                  switch (path_type)
                  {
                  case CONFIGURE_Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].name", sensorNum); break;
                  case RESTv2Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "%s", "$.data.attributes.name"); break;
                  default: jsonPATH[0]='\0';
                  }

                  len = jsmn_get_string(jsonParser, &jsonPATH[0], name, arraysize(parameter));

                  /* default name */
                  if (len <= 0)
                  {
                     snprintf(name, arraysize(parameter), "sensor%"PRIu32"", sensorNum);
                  }

                  currentSensor->name = storeName(name);

                  if (currentSensor->name == NULL)
                  {
                     error = ERROR_OUT_OF_MEMORY;
                  }



                  /* Let's store place*/
                  switch (path_type)
                  {
                  case CONFIGURE_Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].place", sensorNum); break;
                  case RESTv2Path: snprintf(&jsonPATH[0], arraysize(jsonPATH),"%s", "$.data.attributes.place"); break;
                  default: jsonPATH[0]='\0';
                  }

                  len = jsmn_get_string(jsonParser, &jsonPATH[0], place, arraysize(parameter));
                  if (len > 0)
                  {
                     currentSensor->place = storePlace(place);
                     if (currentSensor->place == NULL)
                     {
                        error = ERROR_OUT_OF_MEMORY;
                     }
                  }

                  /* Let's store type*/
                  switch (path_type)
                  {
                  case CONFIGURE_Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].type", sensorNum); break;
                  case RESTv2Path: snprintf(&jsonPATH[0], arraysize(jsonPATH),"%s", "$.data.attributes.type"); break;
                  default: jsonPATH[0]='\0';
                  }

                  len = jsmn_get_string(jsonParser, &jsonPATH[0], type, arraysize(parameter));
                  if (len > 0)
                  {
                     currentSensor->type = storeType(type);
                  }
               }
               else
               {
                  error = ERROR_NO_ACK; //Can't activate device
               }
               driver_close(currentSensor->fd);
               if (error == NO_ERROR)
               {
                  configured++;
               }
            }
            else
            {
               error = ERROR_INVALID_FILE;
            }
         }
         else
         {
            error = ERROR_UNSUPPORTED_CONFIGURATION;
         }
      }
   }

   if (configured > 0)
   {
      return NO_ERROR;
   }
   else
   {
      return error;
   }
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
 // error = sensorsHealthInit (cur_sensor);
 }
 return error;
 }
 */
void sensorsConfigure(void) {
   volatile error_t error = NO_ERROR;
   //Create lists
   places = list_new();
   names = list_new();
   deviceIds = list_new();
   places->match = Place_equal;
   names->match = Name_equal;
   deviceIds->match = Device_equal;
   memset(&sensors[0], 0, sizeof(sensor_t) * MAX_NUM_SENSORS);
   // for (i=0;i<MAX_NUM_SENSORS; i++)
   // {
   // initSensor(&sensors[i]);
   // }
   if (!error)
   {
      error = read_config("/config/sensors_draft.json", &parseJSONSensors);
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
   while (length--)
   {
      //Hexadecimal digit found?
      if (isxdigit((uint8_t ) *str))
      {
         //First digit to be decoded?
         if (value < 0)
            value = 0;
         //Update the value of the current byte
         if (isdigit((uint8_t ) *str))
            value = (value * 16) + (*str - '0');
         else if (isupper((uint8_t ) *str))
            value = (value * 16) + (*str - 'A' + 10);
         else
            value = (value * 16) + (*str - 'a' + 10);
         //Check resulting value
         if (value > 0xFF)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }
      }
      //Dash or colon separator found?
      else if ((*str == '-' || *str == ':') && i < 8)
      {
         //Each separator must be preceded by a valid number
         if (value < 0)
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
   if (i == expectedLength - 1)
   {
      //The NULL character must be preceded by a valid number
      if (value < 0)
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
   static char buffer[MAX_LEN_SERIAL * 3 - 1];
   int p = 0;
   int i;
   //The str parameter is optional
   if (!str)
      str = buffer;
   if (length > 0)
   {
      p = sprintf(str, "%02X", serial[0]);
   }
   //Format serial number
   for (i = 1; i < length; i++)
   {
      p += sprintf(str + p, ":%02X", serial[i]);
   }

   //Return a pointer to the formatted string
   return str;
}

//error_t sensorsHealthInit (sensor_t * sensor)
//{
// //Enter critical section
// //Debug message
// TRACE_INFO("sensorsHealthInit: Enter critical section sensor#%s.", sensor->device);
// osAcquireMutex(&sensor->mutex);
//
// sensor->health.value =SENSORS_HEALTH_MAX_VALUE;
// sensor->health.counter =SENSORS_HEALTH_MIN_VALUE;
//
// //Leave critical section
// //Debug message
// TRACE_INFO("sensorsHealthInit: Leave critical section sensor#%s.\r\n", sensor->device);
// osReleaseMutex(&sensor->mutex);
//
// return NO_ERROR;
//}
//
//int sensorsHealthGetValue(sensor_t * sensor)
//{
// int result;
//
// //Enter critical section
// //Debug message
// TRACE_INFO("sensorsHealthGetValue: Enter critical section sensor#%s.", sensor->device);
// osAcquireMutex(&sensor->mutex);
//
// result = sensor->health.value;
//
// //Leave critical section
// //Debug message
// TRACE_INFO("sensorsHealthGetValue: Leave critical section sensor#%s.\r\n", sensor->device);
// osReleaseMutex(&sensor->mutex);
//
// return result;
//}
//
//
//void sensorsHealthIncValue(sensor_t * sensor)
//{
// //Enter critical section
// //Debug message
// TRACE_INFO("sensorsHealthIncValue: Enter critical section sensor#%s.", sensor->device);
// osAcquireMutex(&sensor->mutex);
//
// if (sensor->health.value<SENSORS_HEALTH_MAX_VALUE)
// {
// sensor->health.value++;
// }
// if (sensor->health.counter<SENSORS_HEALTH_MAX_VALUE)
// {
// sensor->health.counter++;
// }
//
// //Leave critical section
// //Debug message
// TRACE_INFO("sensorsHealthIncValue: Leave critical section sensor#%s.\r\n", sensor->device);
// osReleaseMutex(&sensor->mutex);
//
//}
//
//void sensorsHealthDecValue(sensor_t * sensor)
//{
//
// //Enter critical section
// //Debug message
// TRACE_INFO("sensorsHealthDecValue: Enter critical section sensor#%s.", sensor->device);
// osAcquireMutex(&sensor->mutex);
//
//
// if (sensor->health.value>SENSORS_HEALTH_MIN_VALUE)
// {
// sensor->health.value--;
// }
// if (sensor->health.counter<SENSORS_HEALTH_MAX_VALUE)
// {
// sensor->health.counter++;
// }
//
// //Leave critical section
// //Debug message
// TRACE_INFO("sensorsHealthDecValue: Leave critical section sensor#%s.\r\n", sensor->device);
// osReleaseMutex(&sensor->mutex);
//
//}
//
//void sensorsHealthSetValue(sensor_t * sensor, int value)
//{
//
// //Enter critical section
// //Debug message
// TRACE_INFO("sensorsHealthSetValue: Enter critical section sensor#%s.", sensor->device);
// osAcquireMutex(&sensor->mutex);
//
//
// if (value>SENSORS_HEALTH_MAX_VALUE)
// {
// sensor->health.value=SENSORS_HEALTH_MAX_VALUE;
// }
// else if (value<SENSORS_HEALTH_MIN_VALUE)
// {
// sensor->health.value=SENSORS_HEALTH_MIN_VALUE;
// }
// else
// {
// sensor->health.value = value;
// }
//
// //Leave critical section
// //Debug message
// TRACE_INFO("sensorsHealthSetValue: Leave critical section sensor#%s.\r\n", sensor->device);
// osReleaseMutex(&sensor->mutex);
//
//}
//
//
//

error_t sensorsGetValue(const char *name, double * value) {
   error_t error = ERROR_OBJECT_NOT_FOUND;
   // sensor_t * sensor = findSensorByName(name);
   //
   // if(sensor)
   // {
   // {
   // switch (sensor->valueType)
   // {
   // case FLOAT:
   // *value = (double) sensorsGetValueFloat(sensor);
   // error = NO_ERROR;
   // break;
   // case UINT16:
   // *value = sensorsGetValueUint16(sensor);
   // error = NO_ERROR;
   // break;
   // case CHAR://Not supported yet
   // case PCHAR:
   // default:
   // break;
   // }
   // }
   // }
   return error;
}

static mysensor_sensor_t findTypeByName(const char * type) {
   mysensor_sensor_t result = S_INPUT; /* Default value */
   mysensor_sensor_t num = 0;
   while (num < MYSENSOR_ENUM_LEN) {
      if (*sensorList[num].string != '\0')
      {
         if (strcmp(sensorList[num].string, type) == 0) {
            result = num;
            break;
         }
      }
      num++;
   }
   return result;
}
