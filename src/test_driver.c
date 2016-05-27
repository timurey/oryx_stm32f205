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

static size_t test_read(Peripheral_Descriptor_t const pxPeripheral, void * const pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   return snprintf(pvBuffer, xBytes, "Hello from driver #%"PRIu32"!", peripheral->peripheralNum);

}

static size_t test_read1(Peripheral_Descriptor_t const pxPeripheral, void * const pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   return snprintf(pvBuffer, xBytes, "Hello from driver #%s!", &bufer[0]);

}

static size_t test_write(Peripheral_Descriptor_t const pxPeripheral, const void * pvBuffer, const size_t xBytes)
{

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   char * string = (char * ) pvBuffer;
   return snprintf(&output[0], arraysize(output), "%s#%"PRIu32"!", string, peripheral->peripheralNum);

}

static size_t test_ioctl(Peripheral_Descriptor_t const pxPeripheral, char* pcParameter, char *pcValue)
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

register_driver(test_driver, "/test", NULL, test_write, test_read, test_ioctl, 12);
register_driver(check_driver, "/check", NULL, NULL, test_read1, test_ioctl, 12);


void driverTask (void *pvParameters)
{

   Peripheral_Descriptor_t * fp;
   volatile size_t readed, writed, setted;
   fp = driver_open("/test", 0);
   readed = driver_read(fp, &input, arraysize(input) );
   driver_close(fp);

   fp = driver_open("/test1", 0);
   readed = driver_read(fp, &input, arraysize(input) );
   writed = driver_write(fp, "Hello from application to driver", 33);
   driver_close(fp);
   fp = driver_open("/test10", 0);
   readed = driver_read(fp, &input, arraysize(input) );
   writed = driver_write(fp, "Hello from application to driver", 33);
   driver_close(fp);
   fp = driver_open("/check", 0);
   setted = driver_ioctl(fp, "set_bufer", "Hello, aurora!");
   readed = driver_read(fp, &input, arraysize(input) );
   driver_close(fp);
   osDeleteTask(NULL);
}
