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

#include "configs.h"
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include "debug.h"
#include "macros.h"
#include "../../expression_parser/variables_def.h"
#include "../../list/list.h"


static int Place_equal(void *a, void *b) {
   char * PlaceA = a;
   PlaceListT * PlaceB = b;
   return 0 == strcmp(PlaceA, PlaceB->place);
}
static void Place_remove(void *val) {
   PlaceListT * placePtr = val;
   osFreeMem(placePtr);
}

static void Name_remove(void *val) {
   NameListT * namePtr = val;
   osFreeMem(namePtr);
}

static int Name_equal(void *a, void *b) {
   char * NameA = a;
   NameListT * NameB = b;
   return (0 == strcmp(NameA, NameB->name));
}
static int Device_equal(void *a, void *b) {
   char * DeviceA = a;
   IDListT * DeviceB = b;
   return 0 == strcmp(DeviceA, DeviceB->id);
}

static int Sensor_equal(void *a, void *b) {

   SensorListT * SensorB = b;
   searchSensorBy_t * searchBy = a;
   char * chWanted = (char *)(searchBy->value);
   mysensor_sensor_t mysensWanted = *((mysensor_sensor_t *) (searchBy->value));
   switch (searchBy->by)
   {
   case SEARCH_BY_TYPE:
      return (mysensWanted == SensorB->sensor.type);
      break;

   case SEARCH_BY_PLACE:
      return 0 == strcmp(chWanted, SensorB->sensor.place);
      break;

   case SEARCH_BY_NAME:
      return 0 == strcmp(chWanted, SensorB->sensor.name);
      break;

   case SEARCH_BY_ID:
      return 0 == strcmp(chWanted, SensorB->sensor.id);
      break;

   case SEARCH_BY_HEALTH:
      break;
   default:
      break;
   }
   return 0;
}

list_t *places;
list_t *names;
list_t *deviceIds;
list_t *sensors;


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


register_rest_function (sensor, "/sensor", &restInitSensors, &restDenitSensors, &restGetSensors, &restPostSensors, &restPutSensors, NULL); // &restPostSensors, &restPutSensors, &restDeleteSensors);
register_variables_functions (sensors, &sensorsGetValue);

static mysensor_sensor_t findTypeByName (const char * type);
static error_t parseJSONSensors (jsmnParserStruct * jsonParser, configMode mode);
static sensor_t * createSensorWithId(char * ID);

#if 0
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
#endif
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
   list_destroy(sensors);

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

   Tperipheral_t * fd;
   if (restVersion == 1)
   {
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel2"{");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"id\":\"%s\",", sensor->id + 1);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"name\":\"%s\",", sensor->name);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"place\":\"%s\",", sensor->place);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"value\":");
   }
   else if (restVersion == 2)
   {
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel2"{");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"type\":\"sensor\",");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"id\":\"%s\",", sensor->id + 1);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"attributes\":{");
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"type\":\"%s\",", sensorList[sensor->type].string);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"name\":\"%s\",", sensor->name);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"place\":\"%s\",", sensor->place);
      p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"value\":");
   }
   fd = driver_open(sensor->id, POPEN_READ);
   if (fd!=NULL)
   {
      switch (fd->driver->dataType)
      {
      case FLOAT:
         driver_read(fd, &f_val, sizeof(f_val));
         d1 = f_val; // Get the integer part (678).
         f2 = f_val - d1; // Get fractional part (678.0123 - 678 = 0.0123).
         d2 = abs(trunc(f2 * 10)); // Turn into integer (123).
         p += snprintf(bufer + p, maxLen - p, "%d.%01d", d1, d2);
         break;
      case UINT16:
         if (driver_read(fd, &u16, sizeof(uint16_t)) == sizeof(uint16_t))
         {
            p += snprintf(bufer + p, maxLen - p, "%"PRIu16"", u16);
         }
         break;
      case CHAR: //Not supported yet
         break;
      case PCHAR:
         p += snprintf(bufer + p, maxLen - p, "\"");
         p += driver_read(fd, bufer + p, maxLen - p);
         p += snprintf(bufer + p, maxLen - p, "\"");
         break;
      default:
         break;
      }
      p += snprintf(bufer + p, maxLen - p, ",");

      if (restVersion == 1)
      {
         p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"online\":");
         p += driver_getproperty(fd, "active", bufer + p, maxLen - p);
         p += snprintf(bufer + p, maxLen - p, ",");
         saved_p = p;

         p += snprintf(bufer + p, maxLen - p, ""PRIlevel3"\"min\":");
         changed_p = driver_getproperty(fd, "min", bufer + p, maxLen - p);
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
         changed_p = driver_getproperty(fd, "max", bufer + p, maxLen - p);
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
         for (propNum = 0; propNum < fd->driver->propertyList->numOfProperies; propNum++)
         {
            property = (char*) fd->driver->propertyList->properties[propNum].property;
            if (! ((strcmp(property, "min") == 0) || strcmp(property, "max")==0) )
            {
               saved_p = p;
               p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"{"PRIlevel5"\"name\":\"%s\",\"value\":", property);
               changed_p = driver_getproperty(fd, property, bufer + p, maxLen - p);
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
         p += driver_getproperty(fd, "active", bufer + p, maxLen - p);
         p += snprintf(bufer + p, maxLen - p, ",");

         saved_p = p;

         p += snprintf(bufer + p, maxLen - p, ""PRIlevel4"\"min\":");
         changed_p = driver_getproperty(fd, "min", bufer + p, maxLen - p);
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
         changed_p = driver_getproperty(fd, "max", bufer + p, maxLen - p);
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

         for (propNum = 0; propNum < fd->driver->propertyList->numOfProperies; propNum++)
         {
            property = (char*) fd->driver->propertyList->properties[propNum].property;
            if (! ((strcmp(property, "min") == 0) || strcmp(property, "max")==0) )
            {
               saved_p = p;
               p += snprintf(bufer + p, maxLen - p, ""PRIlevel5"{"PRIlevel6"\"name\":\"%s\",\"value\":", property);
               changed_p = driver_getproperty(fd, property, bufer + p, maxLen - p);
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
   }
   else
   {
      p=0;
   }
   driver_close(fd);
   return p;
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


error_t restGetSensors(HttpConnection *connection, RestApi_t* RestApi)
{
   enum filteringType {
      None,
      Full,
      PathOrType,
      //      Type,
      Empty
   } request;
   error_t error = ERROR_FILE_NOT_FOUND;

   char buffer[32];
   unsigned int numOfSensor;
   mysensor_sensor_t type = S_INPUT;
   list_node_t *sensListNode;
   searchSensorBy_t searchParams;
   SensorListT * sensorPtr;

   char * sensorType = NULL;
   size_t sensorTypeLength;

   char * sensorPath = NULL;
   size_t sensorPathLength;

   sensor_t *currSensor = NULL;

   int p = 0, tp = 0;
   size_t maxLen = arraysize(restBuffer);

   request = None;

   /*
    * Try to recognize, what RestApi contains
    * where is sensor type and sensor path
    * /rest/v2/sensors/temperature/test_7  - full request **will not support**
    * /rest/v2/sensors/test_7 - request contain path only
    * /rest/v2/sensors/temperature - request cotain type only
    * /rest/v2/sensors - request is empty
    */
   if (RestApi->objectIdLen > 1)
   {
      sensorType = RestApi->className + 1;
      sensorTypeLength = RestApi->classNameLen - 1;

      sensorPath = RestApi->objectId;
      sensorPathLength = RestApi->objectIdLen;
      request = Full;
   }
   else if (RestApi->classNameLen > 1)
   {
      sensorType = RestApi->className + 1;
      sensorTypeLength = RestApi->classNameLen - 1;
      sensorPath = RestApi->className + 1;
      sensorPathLength = RestApi->classNameLen - 1;
      request = PathOrType;
   }
   else
   {
      request = Empty;
   }
   //Try to open sensor by path
   if (request == PathOrType)
   {
      snprintf (&buffer[0], arraysize(buffer), "/%s", sensorPath);

      if (buffer[sensorPathLength] == '/')
      {
         buffer[sensorPathLength] = '\0';
      }

      //find sensor by path
      searchParams.by = SEARCH_BY_ID;
      searchParams.value =  &buffer[0];
      sensListNode = list_find(sensors, &searchParams);
      //If sensor is founded by path
      if (sensListNode)
      {
         sensorPtr = sensListNode->val;
         currSensor = &sensorPtr->sensor;
         p += sprintf_single_header(&restBuffer[0], arraysize(restBuffer) - p, currSensor->type, RestApi->restVersion);
         p += snprintfSensor(&restBuffer[p], maxLen - p, currSensor, RestApi->restVersion);
         p --; //Remove last ',' symbol
         p+=sprintf_single_footer(&restBuffer[p], arraysize(restBuffer) - p, currSensor->type, RestApi->restVersion);
         error = NO_ERROR;
      }
      else
         //Try to found all sensors by type
      {
         snprintf (&buffer[0], arraysize(buffer), "%s", sensorType);
         if (buffer[sensorTypeLength] == '/')
         {
            buffer[sensorTypeLength] = '\0';
         }
         type = findTypeByName(&buffer[0]);
         //find sensor by path
         searchParams.by = SEARCH_BY_TYPE;
         searchParams.value =  &type;
         sensListNode = list_find(sensors, &searchParams);
         p += sprintf_header(&restBuffer[0], arraysize(restBuffer) - p, type, RestApi->restVersion);
         tp = p;
         while (sensListNode!=NULL) {
            sensorPtr = sensListNode->val;
            currSensor = &sensorPtr->sensor;
            p += snprintfSensor(&restBuffer[p], maxLen - p, currSensor, RestApi->restVersion);
            sensListNode = list_find_next(sensors, sensListNode, &searchParams);
         };
         if (tp != p) //If any sensor has been printed
         {
            p--;
         }
         p+=sprintf_footer(&restBuffer[p], arraysize(restBuffer) - p, currSensor->type, RestApi->restVersion);

         error = NO_ERROR;
      }
   }
   else if (request == Empty)
   {
      if (RestApi->restVersion != 1)
      {
         type = S_INPUT;
         p += sprintf_header(&restBuffer[0], arraysize(restBuffer) - p, type, RestApi->restVersion);
         tp = p;
         for (numOfSensor = 0; numOfSensor < sensors->len; numOfSensor++)
         {
            sensListNode = list_at(sensors, numOfSensor);
            sensorPtr = sensListNode->val;
            currSensor = &sensorPtr->sensor;
            p += snprintfSensor(&restBuffer[p], maxLen - p, currSensor, RestApi->restVersion);
         }
         if (tp != p) //If any sensor has been printed
         {
            p--;
         }
         p+=sprintf_footer(&restBuffer[p], arraysize(restBuffer) - p, currSensor->type, RestApi->restVersion);
         error = NO_ERROR;
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
      if (jsonParser.jSMNtokens == NULL)
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



char* addDeviceId(const char * deviceId)
{
   size_t length = strlen(deviceId);

   IDListT * idPtr = osAllocMem(sizeof(IDListT) + length + 1);

   if (idPtr)
   {
      memcpy(idPtr->id, deviceId, length);
      idPtr->ptr = &(idPtr->id[0]);
      *((idPtr->id) + length) = '\0';
      list_node_t * ptr = list_rpush(deviceIds, list_node_new(idPtr));
      if (ptr)
      {
         return &(idPtr->id[0]);
      }
      else
      {
         osFreeMem(idPtr);
      }
   }
   else
   {
      osFreeMem(idPtr);
   }
   return NULL;
}

char* sensorsFindDeviceId( char * deviceId)
{

   list_node_t *a = list_find(deviceIds, deviceId);
   if (a)
   {
      IDListT * devicePtr = a->val;
      return &(devicePtr->id[0]);
   }
   return NULL;
}

/* todo: if allocator suppotrs, try to allocate static memory.
 * We'll never free this block, try to allocate it without fragmentation*/
NameListT * sensorsAddName(const char * name)
{
   size_t length = strlen(name);
   NameListT * namePtr = osAllocMem(sizeof(NameListT) + length + 1);

   if (namePtr)
   {
      memcpy(namePtr->name, name, length);
      namePtr->ptr = &(namePtr->name[0]);
      *((namePtr->name) + length) = '\0';
      list_node_t * ptr = list_rpush(names, list_node_new(namePtr));
      if (ptr)
      {
         return namePtr;
      }
      else
      {
         osFreeMem(namePtr);
      }
   }
   return NULL;
}

NameListT * sensorsFindName(char * name)
{

   list_node_t *a = list_find(names, name);
   if (a)
   {
      NameListT * namePtr = a->val;
      return namePtr;
   }
   return NULL;
}

/* todo: if allocator suppotrs, try to allocate static memory.*/
PlaceListT * sensorsAddPlace(const char * place)
{
   size_t length = strlen(place);
   PlaceListT * placePtr = osAllocMem(sizeof(PlaceListT) + length + 1);

   if (placePtr)
   {
      memcpy(placePtr->place, place, length);
      placePtr->ptr=&(placePtr->place[0]);
      *((placePtr->place) + length) = '\0';
      list_node_t * ptr = list_rpush(places, list_node_new(placePtr));
      if (ptr)
      {
         return placePtr;
      }
      else
      {
         osFreeMem(placePtr);
      }
   }
   return NULL;
}


PlaceListT * sensorsFindPlace(char * place)
{
   list_node_t *a = list_find(places, place);
   if (a)
   {
      PlaceListT * placePtr = a->val;
      return placePtr;
   }
   return NULL;
}



static char * findOrStoreName(char * name)
{
   NameListT *ptr = NULL;
   ptr = sensorsFindName(name);
   if (ptr == NULL)
   {
      ptr = sensorsAddName(name);
      ptr->counter = 0;
   }
   if (ptr->counter<INT_MAX)
   {
      ptr->counter++;
   }
   return ptr->name;
}

static char * findOrStorePlace(char * place)
{
   PlaceListT *ptr = NULL;
   ptr = sensorsFindPlace(place);
   if (ptr == NULL)
   {
      ptr = sensorsAddPlace(place);
      ptr->counter = 0;
   }
   if (ptr->counter<INT_MAX)
   {
      ptr->counter++;
   }
   return ptr->place;
}

static sensor_t * findSensorById(char * ID)
{
   sensor_t * currSensor = NULL;

   searchSensorBy_t searchParams = {SEARCH_BY_ID, ID};

   list_node_t *sensListNode = list_find(sensors, &searchParams);

   if (sensListNode)
   {
      SensorListT * sensorPtr = sensListNode->val;
      currSensor = &sensorPtr->sensor;
   }
   return currSensor;
}

static sensor_t * createSensorWithId(char * ID)
{
   SensorListT * sensorPtr = osAllocMem(sizeof(SensorListT));
   memset(sensorPtr, 0, sizeof(SensorListT));
   char * id = NULL;
   if (sensorPtr)
   {
      list_node_t * ptr = list_rpush(sensors, list_node_new(sensorPtr));
      if (ptr)
      {
         id = addDeviceId(ID);
         if (id != NULL)
         {
            sensorPtr->sensor.id =id;
         }
         return &(sensorPtr->sensor);
      }
      else
      {
         osFreeMem(sensorPtr);
      }
   }
   return NULL;
}

static mysensor_sensor_t storeType(char * type)
{
   return findTypeByName(type);
}

static void forgotName(char *name)
{
   NameListT *ptr = NULL;
   ptr = sensorsFindName(name);
   if (ptr != NULL)
   {
      if (ptr->counter>0)
      {
         ptr->counter--;
      }
      if (ptr->counter <=0)
      {
         list_remove(names, list_find(names, name));
      }
   }
}

static void forgotPlace(char *place)
{
   PlaceListT *ptr = NULL;
   ptr = sensorsFindPlace(place);
   if (ptr != NULL)
   {
      if (ptr->counter>0)
      {
         ptr->counter--;
      }
      if (ptr->counter <=0)
      {
         list_remove(places, list_find(places,place));
      }
   }
}

static error_t parseJSONSensors(jsmnParserStruct * jsonParser, configMode mode)
{
   sensor_t * currentSensor = NULL;
   char jsonPATH[64]; //
   char idBuf[32];
   char nameBuf[64];
   char placeBuf[64];
   char typeBuf[32];
   char parameter[64];
   char value[64];

   char * name = NULL; /* Use bufer for copying name. */
   char * place = NULL; /* And place too. */
   mysensor_sensor_t type = S_INPUT; /* And type %-) */

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

   Tperipheral_t * fd;

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

      if (error == NO_ERROR)
      {
         for (sensorNum = 0; sensorNum< (uint32_t) countOfSensors; sensorNum++)
         {
            parameters = 1;
            parameterNum =0;
            error = NO_ERROR;

            //Let's store sensor by ID
            switch (path_type)
            {
            case CONFIGURE_Path:
               snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].device", sensorNum);
               result = jsmn_get_string(jsonParser, &jsonPATH[0], idBuf, arraysize(idBuf)-1);
               break;
            case RESTv2Path:
#if 0
               snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.data[%"PRIu32"].id", sensorNum);
               deviceId[0] = '/';
               result = jsmn_get_string(jsonParser, &jsonPATH[0], &deviceId[1], arraysize(deviceId)-1);
#endif
               snprintf(&jsonPATH[0], arraysize(jsonPATH), "%s", "$.data.id");
               *idBuf = '/';
               result = jsmn_get_string(jsonParser, &jsonPATH[0], idBuf+1, arraysize(idBuf)-2);
               break;
            default: result = 0;
            }

            if (result)
            {
               //Try to find sensor by ID
               currentSensor = findSensorById(&idBuf[0]);

               if (!currentSensor) {
                  //If sensor is not found, create new sensor with ID
                  currentSensor = createSensorWithId(&idBuf[0]);

                  if (!currentSensor) {

                     error = ERROR_OUT_OF_MEMORY;
                     break;
                  }
               }
            }

            /* Let's store name*/
            switch (path_type)
            {
            case CONFIGURE_Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "$.sensors[%"PRIu32"].name", sensorNum); break;
            case RESTv2Path: snprintf(&jsonPATH[0], arraysize(jsonPATH), "%s", "$.data.attributes.name"); break;
            default: jsonPATH[0]='\0';
            }

            len = jsmn_get_string(jsonParser, &jsonPATH[0], nameBuf, arraysize(nameBuf));

            /* default name */
            if (len <= 0)
            {
               snprintf(nameBuf, arraysize(nameBuf), "sensor%"PRIu32"", sensorNum);
            }

            name = findOrStoreName(&nameBuf[0]);

            if (name == NULL)
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

            len = jsmn_get_string(jsonParser, &jsonPATH[0], placeBuf, arraysize(placeBuf));

            if (len > 0)
            {
               place = findOrStorePlace(&placeBuf[0]);
               if (place == NULL)
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

            len = jsmn_get_string(jsonParser, &jsonPATH[0], typeBuf, arraysize(typeBuf));
            if (len > 0)
            {
               type = storeType(&typeBuf[0]);
            }

            if ((currentSensor->name != NULL) & (currentSensor->name != name))
            {
               forgotName(currentSensor->name);
            }
            currentSensor->name = name;

            if ((currentSensor->place != NULL) & (currentSensor->place != place))
            {
               forgotPlace(currentSensor->place);
            }
            currentSensor->place = place;

            currentSensor->type = type;

            //            garbageCollection();
            //Try to configure device

            fd = driver_open(currentSensor->id, POPEN_CONFIGURE);

            if (fd != NULL)
            {
               /* Если устройство уже запущено */
               if (fd->status->statusAttribute & DEV_STAT_ACTIVE)
               {
                  /* Stop device here for reconfigure */
                  driver_setproperty(fd, "active", "false");
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
                     pcParameter = strchr(buffer, '\"'); /*Find opening quote */

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
                        driver_setproperty(fd, &parameter[0], &value[0]);
                     }
                     parameterNum++;
                  }
               }
               /*Store device*/
               if (driver_setproperty(fd, "active", "true") != 1)
               {
                  error = ERROR_NO_ACK; //Can't activate device
               }
               driver_close(fd);

               if (error == NO_ERROR)
               {
                  configured++;
               }
            }
            else
            {
               forgotName(name);
               forgotPlace(place);
               error = ERROR_INVALID_FILE;

            }
         }
      }
      else
      {
         error = ERROR_UNSUPPORTED_CONFIGURATION;
      }

   }

   if (configured > 0)
   {
      error = NO_ERROR;
   }

   return error;
}

void sensorsConfigure(void) {
   volatile error_t error = NO_ERROR;
   //Create lists
   places = list_new();
   names = list_new();
   deviceIds = list_new();
   sensors = list_new();
   places->match = Place_equal;
   places->free = Place_remove;

   names->match = Name_equal;
   names->free = Name_remove;

   deviceIds->match = Device_equal;
   sensors->match = Sensor_equal;

   if (!error)
   {
      error = read_config("/config/sensors_draft.json", &parseJSONSensors);
   }
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
