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

/*
 * todo: too many return in code;
 * add error_t error and osDeleteMutex in the end of routine
 */
error_t driver_open(peripheral_t * const pxPeripheral, const char * path, const periphOpenType flags){

   (void) flags; //Unused variables.
   driver_t *curr_driver;
   size_t len;// Lenght of registerd path
   uint32_t numOfPeripherals;
   error_t error = NO_ERROR;

   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;

   /* Check fd*/
   if (peripheral->driver != NULL)
   {
      error = ERROR_FILE_OPENING_FAILED;
   }
   if (error==NO_ERROR)
   {
      /* Check path*/
      if (path == NULL || *path == '\0')
      {
         error =  ERROR_INVALID_PATH;
      }
   }
   if (error==NO_ERROR)
   {
      memset(peripheral, 0 , sizeof( peripheral_t ));
      /*Second, create mutex for peripheral access*/

      if(!osCreateMutex(&peripheral->mutex))
      {
         error = ERROR_OUT_OF_MEMORY;
      }
      do
      {
         /*Third, search driver by device name*/
         for (curr_driver = &__start_drivers; ((curr_driver < &__stop_drivers)&&(error==NO_ERROR)); curr_driver++)
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
                     error = ERROR_FILE_NOT_FOUND;
                     break;
                  }
                  peripheral->peripheralNum = numOfPeripherals;
                  peripheral->driver = curr_driver;
                  peripheral->status = (devStatusAttributes *) (curr_driver->status+numOfPeripherals);
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
                  error = ERROR_FILE_NOT_FOUND;
                  break;
               }

            }
         }
         if (curr_driver == &__stop_drivers)
         {
            error = ERROR_FILE_NOT_FOUND;
            break;
         }
         if (error!=NO_ERROR)
         {
            break;
         }
         /* Если просто читаем, то проверяем, можем ли его прочесть*/
         if (flags == POPEN_READ)
         {
            if (!(*(peripheral->status) & (DEV_STAT_MANAGED | DEV_STAT_READBLE)))
            {
               error = ERROR_FILE_READING_FAILED;
               break;
            }
         }
         else if (flags == POPEN_WRITE)
         {
            if (!(*(peripheral->status) & (DEV_STAT_MANAGED | DEV_STAT_WRITEBLE)))
            {
               error = ERROR_NOT_WRITABLE;
               break;
            }
         }
         else if (flags == POPEN_CREATE)
         {
            if (*(peripheral->status) & (DEV_STAT_ACTIVE))
            {
               error=  ERROR_INVALID_RECIPIENT;
               break;
            }
         }
         else
         {
            error = ERROR_INVALID_PARAMETER;
            break;
         }
         if (peripheral->driver)
         {
            /*Initialize peripheral specific part*/
            if (peripheral->driver->functions->open != NULL)
            {
               if (!peripheral->driver->functions->open(peripheral))
               {
                  error = ERROR_FILE_OPENING_FAILED;
                  break;
               }
            }
         }
         else
         {
            error = ERROR_FILE_NOT_FOUND;
            break;
         }

         peripheral->mode = flags;
      } while (0);
      if (error!=NO_ERROR)
      {
         peripheral->driver = NULL;
         osDeleteMutex(&peripheral->mutex);
      }
   }

   return error;
}

size_t driver_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes )
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;
   size_t result = 0;
   /*Check, that all pointers is valid, file open mode is read*/
   if ((peripheral->mode == POPEN_READ)
      && (peripheral != NULL)
      && (peripheral->driver != NULL)
      && (peripheral->driver->functions->read != NULL)
   )
   {
      osAcquireMutex(&peripheral->mutex);

      if (*(peripheral->status) & DEV_STAT_READBLE)
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
      if (*(peripheral->status) & DEV_STAT_WRITEBLE)
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
   if (pxPeripheral->mode != POPEN_CREATE)
   {
      return 0;
   }
   property_t * property = findProperty(peripheral->driver, pcRequest);
   if (property != NULL)
   {
      if (property->setFunction != NULL)
      {
         result = property->setFunction(pxPeripheral, pcValue);
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
            *(peripheral->status) |= DEV_STAT_ACTIVE;
            result = 1;
         }
         else if (strcmp(pcValue, "false") == 0)
         {
            /*Common comands*/
            *(peripheral->status) &= ~DEV_STAT_ACTIVE;
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
         result = getproperty->getFunction(pxPeripheral, pcValue, xBytes);
      }
   }
   /* Если нет специального обработчика */
   else
   {
      /*If request is common*/

      /*Common comands*/
      if (peripheral->status == DEV_STAT_ACTIVE)
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
   return result;
}

void driver_close(peripheral_t * const pxPeripheral)
{
   peripheral_t * peripheral = (peripheral_t *) pxPeripheral;

   if (peripheral->driver != NULL)
   {
      osDeleteMutex(&peripheral->mutex);

      peripheral->driver = NULL;
      peripheral->mode = 0;
   }
}
