/*
 * temperature.c
 *
 *  Created on: 02 июля 2015 г.
 *      Author: timurtaipov
 */
#include "rest/sensors.h"
#include "rest/sensors_def.h"
#include "temperature.h"
#include "jsmn_extras.h"
#include "os_port.h"

#include <math.h>
#if 0
int temperature_snprintf(char * bufer, size_t max_len, int sens_num, int restVersion);
#endif
register_sens_function(temperature, "/temperature", S_TEMP, &initTemperature, &deinitTemperature, NULL, NULL, NULL, NULL, FLOAT);

#if (OW_DS1820_SUPPORT == ENABLE)



error_t deinitTemperature (void)
{
   return NO_ERROR;
}

error_t initTemperature (const char * data, jsmntok_t *jSMNtokens, sensor_t ** pCurrentSensor, jsmnerr_t * resultCode, uint8_t * pos)
{
#define MAXLEN 64
   char tmp_str[MAXLEN];
   char * str = &tmp_str[0];
   int len;
   int i;
   char path[64];
   uint8_t flag = 0;
   error_t error;
   sensor_t * currentSensor = *pCurrentSensor;

   for (i=0;i<MAX_ONEWIRE_COUNT;i++)
   {

      sprintf(&path[0],"$.sensors.temperature.onewire[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, *resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         error = serialStringToHex( str, len, &currentSensor->serial[0], ONEWIRE_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.sensors.temperature.onewire[%d].name",i);
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
               xprintf("error parsing ds1820 name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.sensors.temperature.onewire[%d].place",i);
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
               xprintf("error parsing ds1820 place: no avalible memory for saving place");
            }
         }
      }
      if (flag >0)
      {
         currentSensor->id=i;
         currentSensor->type = S_TEMP;
         currentSensor->subType = S_TEMP;
         currentSensor->driver = D_ONEWIRE;
         currentSensor->valueType = FLOAT;
         flag=0;
         currentSensor++;
         (*pos)++;
      }

   }
   *pCurrentSensor=currentSensor;
   return NO_ERROR;
}
#if 0
static char buf[]="00:00:00:00:00:00:00:00/0";
extern sensor_t sensors[MAX_ONEWIRE_COUNT];
int temperature_snprintf(char * bufer, size_t max_len, int sens_num, int restVersion)
{
   int p=0;
   int d1, d2;
   float f2, temp_val;
   int counter=0, i;

   for(i=0; i <= MAX_NUM_SENSORS; i++)
   {
      if (sensors[i].type == S_TEMP && sensors[i].subType == S_TEMP && sensors[i].id == sens_num)
      {
         while ((!(sensors[i].status & READEBLE) ) & (sensors[i].status & MANAGED))
         {
            osDelayTask(1);
            if (++counter == 3000)
            {
               break;
            }
         }
         temp_val = sensorsGetValueFloat(&sensors[i]);
         d1 = temp_val;            // Get the integer part (678).
         f2 = temp_val - d1;     // Get fractional part (678.0123 - 678 = 0.0123).
         d2 = abs(trunc(f2 * 10));   // Turn into integer (123).
         if (restVersion == 1)
         {
            p+=snprintf(bufer+p, max_len-p, "{\"id\":%d,",sensors[i].id);
         }
         else if (restVersion ==2)
         {
            p+=snprintf(bufer+p, max_len-p, "{\"type\":\"temperature\",\"id\":%d,\"attributes\":{",sensors[i].id);
         }
         p+=snprintf(bufer+p, max_len-p, "\"name\":\"%s\",", sensors[i].name);
         p+=snprintf(bufer+p, max_len-p, "\"place\":\"%s\",", sensors[i].place);
         p+=snprintf(bufer+p, max_len-p, "\"value\":\"%d.%01d\",", d1, d2);//sensorsDS1820[i].value
         p+=snprintf(bufer+p, max_len-p, "\"serial\":\"%s\",",serialHexToString(sensors[i].serial, &buf[0], ONEWIRE_SERIAL_LENGTH));
         p+=snprintf(bufer+p, max_len-p, "\"health\":%d,", sensorsHealthGetValue(&sensors[i]));
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
#endif

#endif
#if 0
error_t getRestTemperature(HttpConnection *connection, RestApi_t* RestApi)
{
   int p=0;
   int i=0,j=0;
   error_t error = ERROR_NOT_FOUND;
   const size_t max_len = sizeof(restBuffer);
   p=sprintf(restBuffer,"{\"temperature\":[ ");
   if (RestApi->objectId != NULL)
   {
      if (ISDIGIT(*(RestApi->objectId+1)))
      {
         j=atoi(RestApi->objectId+1);
         for (i=0; i< MAX_NUM_SENSORS; i++)
         {
            if (sensors[i].id == j)
            {
               p+=temperature_snprintf(&restBuffer[p], max_len-p, i);
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

         p+=temperature_snprintf(&restBuffer[p], max_len-p, i);
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
error_t postRestTemperature(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection, "You can't create new clock...");
}

error_t putRestTemperature(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_200_ok(connection,"all right");
}

error_t deleteRestTemperature(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;
   return rest_400_bad_request(connection,"You can't delete any clock...");
}

