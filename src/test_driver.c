/*
 * test_driver.c
 *
 *  Created on: 19 мая 2016 г.
 *      Author: timurtaipov
 */


#include "DriverInterface.h"
#include "os_port.h"
#include <string.h>
char input[128];
char output[128];
char bufer[128];

devStatusAttributes testStatus[12];
float testValue[12];


OsTask *vTestTask;

devStatusAttributes gpioStatus[12];

static size_t test_write(peripheral_t * const pxPeripheral, const void * pvBuffer, const size_t xBytes);
static size_t test_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes);
static size_t gpio_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes);
static size_t test_open(peripheral_t * const pxPeripheral);
static size_t gpio_open(peripheral_t * const pxPeripheral);
static size_t set_active(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue);
static size_t get_active(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue, const size_t xBytes);

static size_t set_bufer( peripheral_t * const pxPeripheral, char * pcRequest, char *pcValue )
{
   return 0;
}

static size_t get_bufer(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue, const size_t xBytes)
{
   return 0;
}


static size_t get_active(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue, const size_t xBytes)
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result;
   /*Common comands*/
   if (peripheral->status[peripheral->peripheralNum] == DEV_STAT_ACTIVE)
   {
      strncpy(pcValue, "true", xBytes);
      result = strlen(pcValue);
   }
   else
   {
      strncpy(pcValue, "false", xBytes);
      result = strlen(pcValue);
   }
   return result;
}

//Array of properties. Conatins names of parameters and type of values.
static const property_t gpioPropList[] =
{
   { "mode", PCHAR, set_bufer, get_bufer},
   { "active_level", PCHAR, set_bufer, get_bufer},
   { "pull", PCHAR, set_bufer, get_bufer},
   { "formula", PCHAR, set_bufer, get_bufer},
};

static const property_t testPropList[] =
{
   { "active", PCHAR, set_active, get_active},
   { "set_bufer", PCHAR, set_bufer, get_bufer},
   { "time", PCHAR, set_bufer, get_bufer}
};

//Defined functions
static const driver_functions_t testFunctions =
{
   test_open,       //open
   test_write, //write
   test_read,  //read
};

static const driver_functions_t gpioFunctions = {gpio_open, NULL, gpio_read};

register_driver(test, "/test", testFunctions, testStatus, 12, testPropList, FLOAT);
register_driver(gpio, "/gpio", gpioFunctions, gpioStatus, 12, gpioPropList, PCHAR);


static void testTask(void * pvParameters)
{
   driver_t * driver = (driver_t *) pvParameters;
   mask_t * mask = (mask_t *) driver->mask;
   /* Преобразование типа void* к типу TaskParam* */
   //   mask = (struct mask_t *) pvParameters->;
   mask->driver = driver;
   int i;
   while (1)
   {
      for (i = 0; i< 12; i++)
      {
         testValue[i]++;
         if (i%2)
         {
            (mask->mask) |= (1<<i);
         }
         else
         {
            (mask->mask) &= ~(1<<i);
         }
      }
      xQueueSendToBack(dataQueue, mask, 0);
      osDelayTask(5000);
   }

}

static size_t set_active( peripheral_t * const pxPeripheral, char * pcRequest, char *pcValue )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result =0 ;

   if (strcmp(pcRequest, "active") == 0)
   {
      if (strcmp(pcValue, "true") == 0)
      {

         peripheral->status[peripheral->peripheralNum] |= DEV_STAT_ACTIVE;
         vTestTask = osCreateTask("testTask", testTask, peripheral->driver, configMINIMAL_STACK_SIZE, 2);
         result = 1;
      }
   }
   if (strcmp(pcRequest, "active") == 0)
   {
      if (strcmp(pcValue, "false") == 0)
      {
         osDeleteTask(vTestTask);
         peripheral->status[peripheral->peripheralNum] &= ~DEV_STAT_ACTIVE;
         result = 1;
      }

   }
   return result;
}

static size_t test_open(peripheral_t * const pxPeripheral)
{
   /* OK, boys and girls! Let's get read and write!!!*/
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   testStatus[peripheral->peripheralNum] |= DEV_STAT_READBLE|DEV_STAT_WRITEBLE;
   return 1;
}

static size_t gpio_open(peripheral_t * const pxPeripheral)
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   gpioStatus[peripheral->peripheralNum] |= DEV_STAT_READBLE|DEV_STAT_WRITEBLE;
   return 1;
}

static size_t test_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   double * value = (double *)pvBuffer;

   *value = testValue[peripheral->peripheralNum];

   return sizeof(double);
}

static size_t gpio_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   return snprintf(pvBuffer, xBytes, "23", &bufer[0]);

}

static size_t test_write(peripheral_t * const pxPeripheral, const void * pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   char * string = (char * ) pvBuffer;
   testStatus[peripheral->peripheralNum] = DEV_STAT_READBLE;
   return snprintf(&output[0], arraysize(output), "%s#%"PRIu32"!", string, peripheral->peripheralNum);

}



void driverTask (void *pvParameters)
{

   peripheral_t fp;
   volatile size_t readed, writed, setted;
   volatile error_t error;

   error = driver_open(&fp, "/test", 0);
   readed = driver_read(&fp, &input, arraysize(input) );
   driver_close(&fp);
   error = driver_open(&fp, "/test_1", 0);
   readed = driver_read(&fp, &input, arraysize(input) );
   writed = driver_write(&fp, "Hello from application to driver", 33);
   driver_close(&fp);
   error = driver_open(&fp, "/test_10", 0);
   readed = driver_read(&fp, &input, arraysize(input) );
   writed = driver_write(&fp, "Hello from application to driver", 33);
   driver_close(&fp);
   error = driver_open(&fp, "/check", 0);
   setted = driver_setproperty(&fp, "set_bufer", "Hello, aurora!");
   driver_close(&fp);
   error = driver_open(&fp, "/check", 0);
   readed = driver_read(&fp, &input, arraysize(input) );
   driver_close(&fp);
   osDeleteTask(NULL);
}
