/*
 * DriverInterface.c
 *
 *  Created on: 19 мая 2016 г.
 *      Author: timurtaipov
 */

#include "DriverInterface.h"
#include <stdlib.h>
#include <string.h>
#include "os_port.h"

Peripheral_Descriptor_t * driver_open(const char * path, const uint16_t flags){

   peripheral_t * peripheral = NULL;
   (void) flags; //Unused variables.

   size_t len;// Lenght of registerd path
   uint32_t numOfPeripherals;
   size_t result;
   /*First, allocate memory for perepheral descriptor*/
   peripheral = osAllocMem( sizeof( peripheral_t ) );

   if (peripheral == NULL)
   {
      return NULL;
   }
   memset(peripheral, 0 , sizeof( peripheral_t ));
   /*Second, create mutex for peripheral access*/
   if(!osCreateMutex(&peripheral->mutex))
   {
      osFreeMem(peripheral);
      peripheral = NULL;
      return NULL;
   }

   /*Third, search driver by device name*/
   for (driver_t *curr_driver = &__start_drivers; curr_driver < &__stop_drivers; curr_driver++)
   {
      len = strlen(curr_driver->path);

      if (strncmp(curr_driver->path, path, len) == 0)
      {

         /*Third, find number of peripheral*/
         if ((*(path+len) == '_') && (*(path+len+1) >= '0') && (*(path+len+1) <= '9'))
         {
            /*Next charcter in requered path is a numeric,
             *  convert it from srting to int */

            /*It can't be a nigative, and must be less,
             * than count of peripherals, defined by driver */
            numOfPeripherals = (uint32_t) atoi (path+len+1);
            if (numOfPeripherals>=curr_driver->countOfPerepherals)
            {
               break;
            }
            peripheral->peripheralNum = numOfPeripherals;
            peripheral->driver = curr_driver;
            break;
         }
         /*If number of perepheral is not defined in requred path,
          * use default value 0 */
         else if (*(path+len) == '/' || *(path+len) == '\0' )
         {
            peripheral->driver = curr_driver;
            peripheral->peripheralNum = 0;
            break;
         }

      }
   }

   /*If driver not found for current path, free memory and return*/
   if (peripheral->driver)
   {
      /*Initialize peripheral specific part*/
      if (peripheral->driver->open != NULL)
      {
         result = peripheral->driver->open(peripheral);
         if (!result) //Check result of
         {
            peripheral->driver = NULL; //Error
         }
      }
   }

   if (!peripheral->driver)
   {
      osFreeMem(peripheral);
      peripheral = NULL;
   }

   return (Peripheral_Descriptor_t) peripheral;
}

size_t driver_read(Peripheral_Descriptor_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result = 0;
   /*Check, that all pointers is valid*/
   if ( (peripheral != NULL)
      && (peripheral->driver != NULL)
      && (peripheral->driver->read != NULL)
   )
   {
      osAcquireMutex(&peripheral->mutex);

      result = peripheral->driver->read(pxPeripheral, pvBuffer, xBytes);

      osReleaseMutex(&peripheral->mutex);
   }
   else
   {
      /*If error is ocured, return 0*/
   }
   return result;
}

size_t driver_write(Peripheral_Descriptor_t * const pxPeripheral, const void * pvBuffer, const size_t xBytes )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result = 0;
   /*Check, that all pointers is valid*/
   if ( (peripheral != NULL)
      && (peripheral->driver != NULL)
      && (peripheral->driver->write != NULL)
   )
   {
      osAcquireMutex(&peripheral->mutex);

      result =  peripheral->driver->write(pxPeripheral, pvBuffer, xBytes);

      osReleaseMutex(&peripheral->mutex);
   }
   else
   {
      /*If error is ocured, return 0*/
   }
   return result;
}

size_t driver_ioctl( Peripheral_Descriptor_t * const pxPeripheral, char * pcRequest, char *pcValue )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t len;
   size_t result =0 ;
   len = strlen(pcRequest);

   /*If request is common*/
   if (strncmp(pcRequest, "start", len) == 0)
   {
      /*Common comands*/
      result = 1;
   }
   else
   {
      /*Check, that all pointers is valid*/
      if ( (peripheral != NULL)
         && (peripheral->driver != NULL)
         && (peripheral->driver->ioctl != NULL)
      )
      {
         result =  peripheral->driver->ioctl(pxPeripheral, pcRequest, pcValue);
      }
      else
      {
         /*If error is ocured, return 0*/
      }
   }
   return result;
}

void driver_close(Peripheral_Descriptor_t  * pxPeripheral)
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;

   if (peripheral != NULL)
   {
      osDeleteMutex(&peripheral->mutex);

      peripheral->driver = NULL;

      osFreeMem(peripheral);
      peripheral = NULL;
   }
}
