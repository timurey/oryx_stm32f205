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
devStatusAttributes gpioStatus[12];

static size_t test_write(peripheral_t * const pxPeripheral, const void * pvBuffer, const size_t xBytes);
static size_t test_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes);
static size_t gpio_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes);
static size_t test_ioctl(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue);
static size_t test_open(peripheral_t * const pxPeripheral);
static size_t gpio_open(peripheral_t * const pxPeripheral);
static size_t set_bufer(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue)
{
   return 0;
}

static size_t get_bufer(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue)
{
   return 0;
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
   { "set_bufer", PCHAR, set_bufer, get_bufer},
   { "time", PCHAR, set_bufer, get_bufer}
};

//Defined functions
static const driver_functions_t testFunctions =
{
   test_open,       //open
   test_write, //write
   test_read,  //read
   test_ioctl  //ioctl
};

static const driver_functions_t gpioFunctions = {gpio_open, NULL, gpio_read, test_ioctl};

register_driver(test, "/test", testFunctions, testStatus, 12, testPropList, PCHAR);
register_driver(gpio, "/gpio", gpioFunctions, gpioStatus, 12, gpioPropList, PCHAR);

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
   return snprintf(pvBuffer, xBytes, "24", peripheral->peripheralNum);

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

static size_t test_ioctl(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue)
{
   size_t lenValue = 0;
   //   lenParameter = strlen(pcParameter);
   if (strcmp(pcParameter, "set_bufer") == 0)
   {
      strncpy(&bufer[0], pcValue, arraysize(bufer));
      lenValue = strlen(bufer);
   }
   return lenValue;
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
