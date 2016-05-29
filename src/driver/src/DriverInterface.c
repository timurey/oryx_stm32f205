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

error_t driver_open(peripheral_t * const pxPeripheral, const char * path, const uint16_t flags){

   (void) flags; //Unused variables.
   size_t len;// Lenght of registerd path
   uint32_t numOfPeripherals;

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;

   /* Check fd*/
   if (peripheral->driver != NULL)
   {
      return ERROR_FILE_OPENING_FAILED;
   }
   /* Check path*/
   if (path == NULL || *path == '\0')
   {
      return ERROR_INVALID_PATH;
   }

   memset(peripheral, 0 , sizeof( peripheral_t ));
   /*Second, create mutex for peripheral access*/

   if(!osCreateMutex(&peripheral->mutex))
   {
      return ERROR_OUT_OF_MEMORY;
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
               return ERROR_FILE_NOT_FOUND;
            }
            peripheral->peripheralNum = numOfPeripherals;
            peripheral->driver = curr_driver;
            peripheral->status = (devStatusAttributes *) curr_driver->status;
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
         else
         {
            return ERROR_FILE_NOT_FOUND;
         }

      }
   }

   if (peripheral->driver)
   {
      /*Initialize peripheral specific part*/
      if (peripheral->driver->functions->open != NULL)
      {
         if (!peripheral->driver->functions->open(peripheral))
         {
            return ERROR_FILE_OPENING_FAILED;
         }
      }
   }
   else
   {
      return ERROR_FILE_NOT_FOUND;
   }


   return NO_ERROR;
}

size_t driver_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result = 0;
   /*Check, that all pointers is valid*/
   if ( (peripheral != NULL)
      && (peripheral->driver != NULL)
      && (peripheral->driver->functions->read != NULL)
   )
   {
      osAcquireMutex(&peripheral->mutex);

      if (peripheral->status[peripheral->peripheralNum] & DEV_STAT_READBLE)
      {
         result = peripheral->driver->functions->read(pxPeripheral, pvBuffer, xBytes);
      }
      osReleaseMutex(&peripheral->mutex);
   }
   else
   {
      /*If error is ocured, return 0*/
   }
   return result;
}

size_t driver_write(peripheral_t * const pxPeripheral, const void * pvBuffer, const size_t xBytes )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result = 0;
   /*Check, that all pointers is valid*/
   if ( (peripheral != NULL)
      && (peripheral->driver != NULL)
      && (peripheral->driver->functions->write != NULL)
   )
   {
      osAcquireMutex(&peripheral->mutex);
      if (peripheral->status[peripheral->peripheralNum] & DEV_STAT_WRITEBLE)
      {
         result =  peripheral->driver->functions->write(pxPeripheral, pvBuffer, xBytes);
      }
      osReleaseMutex(&peripheral->mutex);
   }
   else
   {
      /*If error is ocured, return 0*/
   }
   return result;
}

static property_t * findProperty(driver_t * driver, char * pcRequest)
{
   size_t i;
   char const * propertyName;
   for (i=0; i< driver->propertyList->numOfProperies; i++)
   {
      propertyName = (driver->propertyList->properties+i)->property;
      if (strcmp(propertyName, pcRequest) == 0)
      {
         return (driver->propertyList->properties + i);
      }
   }
   return NULL;
}

size_t driver_setproperty( peripheral_t * const pxPeripheral, char * pcRequest, char *pcValue )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result =0 ;

   if (!pxPeripheral->driver)
   {
      return 0;
   }
   property_t * property = findProperty(peripheral->driver, pcRequest);
   if (property != NULL)
   {
      if (property->setFunction != NULL)
      {
         result = property->setFunction(pxPeripheral, pcRequest, pcValue);
      }
   }
   /* Если нет специального обработчика */
   else
   {
      /*If request is common*/
      if (strcmp(pcRequest, "active") == 0)
      {
         if (strcmp(pcValue, "true") == 0)
         {
            /*Common comands*/
            peripheral->status[peripheral->peripheralNum] = DEV_STAT_ACTIVE;
            result = 1;
         }
      }
      if (strcmp(pcRequest, "active") == 0)
      {
         if (strcmp(pcValue, "false") == 0)
         {
            /*Common comands*/
            peripheral->status[peripheral->peripheralNum] = DEV_STAT_ACTIVE;
            result = 1;
         }
      }
   }
   return result;
}

size_t driver_getproperty( peripheral_t * pxPeripheral, char * pcRequest, char *pcValue, const size_t xBytes )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result =0 ;

   if (!pxPeripheral->driver)
   {
      return 0;
   }

   property_t * getproperty = findProperty(peripheral->driver, pcRequest);
   if (getproperty != NULL)
   {
      if (getproperty->getFunction != NULL)
      {
         result = getproperty->getFunction(pxPeripheral, pcRequest, pcValue);
      }
   }
   /* Если нет специального обработчика */
   else
   {
      /*If request is common*/
      if (strcmp(pcRequest, "active") == 0)
      {

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

      }
   }
   return result;
}

void driver_close(peripheral_t * const pxPeripheral)
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;

   if (peripheral != NULL)
   {
      osDeleteMutex(&peripheral->mutex);

      peripheral->driver = NULL;
   }
}
