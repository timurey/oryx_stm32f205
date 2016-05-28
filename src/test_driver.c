/*
 * test_driver.c
 *
 *  Created on: 19 мая 2016 г.
 *      Author: timurtaipov
 */


#include "DriverInterface.h"
#include "os_port.h"
#include <stdio.h>
#include <string.h>
char input[128];
char output[128];
char bufer[128];

static size_t test_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   return snprintf(pvBuffer, xBytes, "24", peripheral->peripheralNum);

}

static size_t test_read1(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   return snprintf(pvBuffer, xBytes, "23", &bufer[0]);

}

static size_t test_write(peripheral_t * const pxPeripheral, const void * pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   char * string = (char * ) pvBuffer;
   return snprintf(&output[0], arraysize(output), "%s#%"PRIu32"!", string, peripheral->peripheralNum);

}

static size_t test_ioctl(peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue)
{
   size_t lenParameter;
   size_t lenValue;
//   lenParameter = strlen(pcParameter);
   if (strcmp(pcParameter, "set_bufer") == 0)
   {
      strncpy(&bufer[0], pcValue, arraysize(bufer));
   }
   lenValue = strlen(bufer);
   return lenValue;
}

register_driver(test, "/test", NULL, test_write, test_read, test_ioctl, 12, PCHAR);
register_driver(gpio, "/gpio", NULL, NULL, test_read1, test_ioctl, 12, PCHAR);


void driverTask (void *pvParameters)
{

   peripheral_t fp;
   volatile size_t readed, writed, setted;
   volatile error_t error;

   error = driver_open(&fp, "/test", 0);
   readed = driver_read(&fp, &input, arraysize(input) );
   driver_close(&fp);
   error = driver_open(&fp, "/test/1", 0);
   readed = driver_read(&fp, &input, arraysize(input) );
   writed = driver_write(&fp, "Hello from application to driver", 33);
   driver_close(&fp);
   error = driver_open(&fp, "/test/10", 0);
   readed = driver_read(&fp, &input, arraysize(input) );
   writed = driver_write(&fp, "Hello from application to driver", 33);
   driver_close(&fp);
   error = driver_open(&fp, "/check", 0);
   setted = driver_ioctl(&fp, "set_bufer", "Hello, aurora!");
   driver_close(&fp);
   error = driver_open(&fp, "/check", 0);
   readed = driver_read(&fp, &input, arraysize(input) );
   driver_close(&fp);
   osDeleteTask(NULL);
}
