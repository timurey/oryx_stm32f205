/*
 * output.c
 *
 *  Created on: 03 нояб. 2015 г.
 *      Author: timurtaipov
 */
#include "rest/rest.h"

#include "output.h"
#include "sensors.h"
#include "executor_def.h"
#include "executor.h"
#include "uuid.h"

static OsTask *pOutputTask;

error_t InitOutputs(jsmnParserStruct * jsonParser, executor_t ** pCurrentExecutor, uint8_t * pos);
error_t restDenitOutput(void);
error_t restGetOutput(HttpConnection *connection, RestApi_t * rest);
error_t restPostOutput(HttpConnection *connection, RestApi_t * rest);
error_t restPutOutput(HttpConnection *connection, RestApi_t * rest);
error_t restDeleteOutput(HttpConnection *connection, RestApi_t * rest);

void outputTask (void * pvParameters);

#define HexToDec(a) (10*((a & 0xF0) > 4) + (a&0x0f))

register_executor_function(output, "/output", O_DIGITAL, &InitOutputs, &restDenitOutput, &restGetOutput, &restPostOutput, &restPutOutput, &restDeleteOutput);


static void initOutput(executor_t * currentExecutor)
{
   (void) currentExecutor;

}
error_t InitOutputs(const char * data, jsmntok_t *jSMNtokens, executor_t ** pCurrentExecutor, uint8_t * pos)
{
#define MAXLEN 64
   char tmp_str[MAXLEN];
   char * str = &tmp_str[0];
   int len;
   int i, j=0;
   char path[64];
   uint8_t dac_used =0;
   uint8_t flag = 0;
   error_t error;
   executor_t * currentExecutor = *pCurrentExecutor;

   for (i=0;i<MAX_NUM_OUTPUTS;i++)
   {
      /*
       * Code for digital outputs
       */
      sprintf(&path[0],"$.executors.output.digital[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         error = serialStringToHex(str, len, &currentExecutor->serial[0], OUTPUT_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.executors.output.digital[%d].name",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {
         currentExecutor->name = sensorsFindName(str, len);

         if (currentExecutor->name == 0)
         {
            currentExecutor->name = sensorsAddName(str, len);
            flag++;
            if (currentExecutor->name == 0)
            {
               xprintf("error parsing output name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.executors.output.digital[%d].place",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {

         currentExecutor->place = sensorsFindPlace(str, len);

         if (currentExecutor->place==0)
         {
            currentExecutor->place = sensorsAddPlace(str, len);
            flag++;
            if (currentExecutor->place == 0)
            {
               xprintf("error parsing output place: no avalible memory for saving place");
            }
         }

      }
      if (flag >0)
      {
         currentExecutor->id=j++;
         currentExecutor->type = O_DIGITAL;
         flag=0;
         if (IsLocal(currentExecutor->serial))
         {
            initOutput(currentExecutor);
         }
         currentExecutor++;
         (*pos)++;
      }

   }
   for (i=0;i<MAX_NUM_OUTPUTS;i++)
   {
      /*
       * Code for analog outnputs
       */
      sprintf(&path[0],"$.executors.output.analog[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {

         error = serialStringToHex( str, len, &currentExecutor->serial[0], OUTPUT_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.executors.output.analog[%d].name",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {


         currentExecutor->name = sensorsFindName(str, len);

         if (currentExecutor->name == 0)
         {
            currentExecutor->name = sensorsAddName(str, len);
            flag++;
            if (currentExecutor->name == 0)
            {
               xprintf("error parsing output name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.executors.output.analog[%d].place",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {


         currentExecutor->place = sensorsFindPlace(str, len);

         if (currentExecutor->place==0)
         {
            currentExecutor->place = sensorsAddPlace(str, len);
            flag++;
            if (currentExecutor->place == 0)
            {
               xprintf("error parsing output place: no avalible memory for saving place");
            }
         }

      }
      if (flag >0)
      {
         currentExecutor->id=j++;
         currentExecutor->type = O_ANALOG;
         flag=0;
         if (IsLocal(currentExecutor->serial))
         {
            initOutput(currentExecutor);
         }
         currentExecutor++;
         (*pos)++;
      }
   }
   for (i=0;i<MAX_NUM_OUTPUTS;i++)
   {
      /*
       * Code for dimmers outputs
       */
      sprintf(&path[0],"$.executors.output.dimmer[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {

         error = serialStringToHex( str, len, &currentExecutor->serial[0], OUTPUT_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.executors.output.dimmer[%d].name",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {


         currentExecutor->name = sensorsFindName(str, len);

         if (currentExecutor->name == 0)
         {
            currentExecutor->name = sensorsAddName(str, len);
            flag++;
            if (currentExecutor->name == 0)
            {
               xprintf("error parsing input name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.executors.output.dimmer[%d].place",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {


         currentExecutor->place = sensorsFindPlace(str, len);

         if (currentExecutor->place==0)
         {
            currentExecutor->place = sensorsAddPlace(str, len);
            flag++;
            if (currentExecutor->place == 0)
            {
               xprintf("error parsing input place: no avalible memory for saving place");
            }
         }

      }
      if (flag >0)
      {
         currentExecutor->id=j++;
         currentExecutor->type = S_DIMMER;
         flag=0;
         if (IsLocal(currentExecutor->serial))
         {
            initOutput(currentExecutor);
         }
         currentExecutor++;
         (*pos)++;
      }
   }
   for (i=0;i<MAX_NUM_OUTPUTS;i++)
   {
      /*
       * Code for HTTP commands
       */
      sprintf(&path[0],"$.executors.output.http[%d].serial",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {

         error = serialStringToHex( str, len, &currentExecutor->serial[0], OUTPUT_SERIAL_LENGTH);
         if (error == NO_ERROR)
         {
            flag++;
         }
      }
      sprintf(&path[0],"$.executors.output.http[%d].name",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {


         currentExecutor->name = sensorsFindName(str, len);

         if (currentExecutor->name == 0)
         {
            currentExecutor->name = sensorsAddName(str, len);
            flag++;
            if (currentExecutor->name == 0)
            {
               xprintf("error parsing input name: no avalible memory for saving name");
            }
         }
      }
      sprintf(&path[0],"$.executors.output.http[%d].place",i);
      len = jsmn_get_string(data, jSMNtokens, resultCode, &path[0], str, MAXLEN);
      if(len>0)
      {


         currentExecutor->place = sensorsFindPlace(str, len);

         if (currentExecutor->place==0)
         {
            currentExecutor->place = sensorsAddPlace(str, len);
            flag++;
            if (currentExecutor->place == 0)
            {
               xprintf("error parsing input place: no avalible memory for saving place");
            }
         }

      }
      if (flag >0)
      {
         currentExecutor->id=j++;
         //               currentExecutor->type = S_MULTIMETER;
         //               if (dac_used == 0)
         //               {
         //                  MX_ADC1_Init();
         //                  __ADC1_CLK_ENABLE();
         //               }
         dac_used++;
         flag=0;
         if (IsLocal(currentExecutor->serial))
         {
            initOutput(currentExecutor);
         }
         currentExecutor++;
         (*pos)++;
      }
   }
   pOutputTask = osCreateTask("outputs", outputTask, NULL, configMINIMAL_STACK_SIZE, 3);
   *pCurrentExecutor=currentExecutor;
   return error;
}

void outputTask (void * pvParameters)
{
   (void) pvParameters;
   while (1)
   {
      vTaskDelay(1000);
   }
}
error_t restDenitOutput(void)
{
   error_t error = NO_ERROR;
   return error;
}


error_t restGetOutput(HttpConnection *connection, RestApi_t * rest)
{
   error_t error = NO_ERROR;

   (void) connection;
   (void) rest;


   return error;
}

error_t restPostOutput(HttpConnection *connection, RestApi_t * rest)
{
   error_t error = NO_ERROR;

   (void) connection;
   (void) rest;

   return error;
}

error_t restPutOutput(HttpConnection *connection, RestApi_t * rest)
{
   error_t error = NO_ERROR;

   (void) connection;
   (void) rest;


   (void) connection;
   (void) rest;

   return error;
}

error_t restDeleteOutput(HttpConnection *connection, RestApi_t * rest)
{
   error_t error = NO_ERROR;

   (void) connection;
   (void) rest;

   return error;
}


