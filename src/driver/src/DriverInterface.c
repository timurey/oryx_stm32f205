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
Tperipheral_t * driver_open(const char * path, const periphOpenType flags)
{

   (void) flags; //Unused variables.
   driver_t *curr_driver;
   size_t len; // Lenght of registerd path
   uint32_t numOfPeripherals;
   error_t error = NO_ERROR;
   Tperipheral_t * pxPeripheralControl = NULL;

   pxPeripheralControl = osAllocMem(sizeof(Tperipheral_t));

   /* Check path*/
   if (path == NULL || *path == '\0')
   {
      error = ERROR_INVALID_PATH;
   }

   if (error == NO_ERROR)
   {

      memset(pxPeripheralControl, 0, sizeof(Tperipheral_t));
      /*Second, create mutex for peripheral access*/

      do
      {
         /*Third, search driver by device name*/
         for (curr_driver = (driver_t *) &__start_drivers;
            ((curr_driver < (driver_t *) &__stop_drivers) && (error == NO_ERROR)); curr_driver++)
         {
            len = strlen(curr_driver->path);

            if (strncmp(curr_driver->path, path, len) == 0)
            {

               /*Third, find number of peripheral*/
               if ((*(path + len) == '_') && (*(path + len + 1) >= '0') && (*(path + len + 1) <= '9'))
               {
                  /*Next charcter in requered path is a numeric,
                   *  convert it from srting to int */

                  /*It can't be a nigative, and must be less,
                   * than count of peripherals, defined by driver */
                  numOfPeripherals = (uint32_t) atoi(path + len + 1);
                  if (numOfPeripherals >= curr_driver->countOfPerepherals)
                  {
                     error = ERROR_FILE_NOT_FOUND;
                     break;
                  }
                  pxPeripheralControl->peripheralNum = numOfPeripherals;
                  pxPeripheralControl->driver = curr_driver;
                  pxPeripheralControl->status = (curr_driver->status + (pxPeripheralControl->peripheralNum));
                  pxPeripheralControl->mutex =
                     &pxPeripheralControl->driver->status[pxPeripheralControl->peripheralNum].mutex;
                  /*Create mutex for device*/
                  if (pxPeripheralControl->mutex->handle == NULL)
                  {
                     if (!osCreateMutex(pxPeripheralControl->mutex))
                     {
                        error = ERROR_OUT_OF_MEMORY;
                        break;
                     }
                  }
                  break;
               }

               /*If number of perepheral is not defined in requred path,
                * use default value 0 */
               else if (*(path + len) == '/' || *(path + len) == '\0')
               {
                  pxPeripheralControl->peripheralNum = 0;
                  pxPeripheralControl->driver = curr_driver;
                  pxPeripheralControl->status = (curr_driver->status + (pxPeripheralControl->peripheralNum));
                  pxPeripheralControl->mutex =
                     &pxPeripheralControl->driver->status[pxPeripheralControl->peripheralNum].mutex;
                  /*Create mutex for device*/
                  if (pxPeripheralControl->mutex->handle == NULL)
                  {
                     if (!osCreateMutex(pxPeripheralControl->mutex))
                     {
                        error = ERROR_OUT_OF_MEMORY;
                        break;
                     }
                  }
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
         if (error != NO_ERROR)
         {
            break;
         }
         if (pxPeripheralControl->status->initialized != TRUE)
         {
            if (pxPeripheralControl->driver)
            {
               /*Initialize peripheral specific part*/
               if (pxPeripheralControl->driver->functions->open != NULL)
               {
                  if (!pxPeripheralControl->driver->functions->open(pxPeripheralControl))
                  {
                     error = ERROR_FILE_OPENING_FAILED;
                     break;
                  }
                  pxPeripheralControl->mode = flags;
                  pxPeripheralControl->status->initialized = TRUE;
               }
            }
            else
            {
               error = ERROR_FILE_NOT_FOUND;
               break;
            }
         }

      } while (0);

      if (error != NO_ERROR)
      {
         pxPeripheralControl->driver = NULL;
         if (pxPeripheralControl->mutex != NULL)
         {
            osDeleteMutex(pxPeripheralControl->mutex);
         }
         osFreeMem(pxPeripheralControl);
         pxPeripheralControl = NULL;
      }
   }

   return pxPeripheralControl;
}

size_t driver_read(Tperipheral_t * pxPeripheral, void * const pvBuffer, const size_t xBytes)
{
   Tperipheral_t * pxPeripheralControl = pxPeripheral;
   size_t result = 0;
   /*Check, that all pointers is valid, file open mode is read*/
   if ((pxPeripheral != NULL) && (pxPeripheralControl->driver != NULL)
      && (pxPeripheralControl->driver->functions->read != NULL))
   {
      osAcquireMutex(pxPeripheralControl->mutex);

      result = pxPeripheralControl->driver->functions->read((peripheral_t) pxPeripheralControl, pvBuffer, xBytes);

      osReleaseMutex(pxPeripheralControl->mutex);
   }
   else
   {
      /*If error is ocured, return 0*/
   }
   return result;
}

size_t driver_write(Tperipheral_t * pxPeripheral, const void * pvBuffer, const size_t xBytes)
{
   Tperipheral_t * pxPeripheralControl = pxPeripheral;
   size_t result = 0;
   /*Check, that all pointers is valid*/
   if ((pxPeripheralControl != NULL) && (pxPeripheralControl->driver != NULL)
      && (pxPeripheralControl->driver->functions->write != NULL))
   {
      osAcquireMutex(pxPeripheralControl->mutex);

      result = pxPeripheralControl->driver->functions->write((peripheral_t) pxPeripheralControl, pvBuffer, xBytes);

      osReleaseMutex(pxPeripheralControl->mutex);
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
   for (i = 0; i < driver->propertyList->numOfProperies; i++)
   {
      propertyName = (driver->propertyList->properties + i)->property;
      if (strcmp(propertyName, pcRequest) == 0)
      {
         return (property_t *) (driver->propertyList->properties + i);
      }
   }
   return NULL;
}

size_t driver_setproperty(Tperipheral_t * pxPeripheral, char * pcRequest, char *pcValue)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;
   size_t result = 0;

   if (!peripheral->driver)
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
   /* Если нет собственного обработчика */
   else
   {
      /*If request is common*/
      if (strcmp(pcRequest, "active") == 0)
      {
         if (strcmp(pcValue, "true") == 0)
         {
            /*Common comands*/
            peripheral->status->statusAttribute |= DEV_STAT_ACTIVE;
            result = 1;
         }
         else if (strcmp(pcValue, "false") == 0)
         {
            /*Common comands*/
            peripheral->status->statusAttribute &= ~DEV_STAT_ACTIVE;
            result = 1;
         }
      }
   }
   return result;
}

size_t driver_getproperty(Tperipheral_t * pxPeripheral, char * pcRequest, char *pcValue, const size_t xBytes)
{
   Tperipheral_t * peripheral = (Tperipheral_t *) pxPeripheral;
   size_t result = 0;

   if (!peripheral->driver)
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
      if (strcmp(pcRequest, "active") == 0)
      {
         /*Common comands*/
         if (peripheral->status->statusAttribute == DEV_STAT_ACTIVE)
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

void driver_close(Tperipheral_t * pxPeripheral)
{
   Tperipheral_t * pxPeripheralControl = pxPeripheral;
   if (pxPeripheralControl != NULL)
   {
      pxPeripheralControl->driver = NULL;
      pxPeripheralControl->status = NULL;
      pxPeripheralControl->peripheralNum = 0;
      pxPeripheralControl->mode = POPEN_UNDEFINED;
      osFreeMem(pxPeripheralControl);
      //      pxPeripheral = NULL;
   }
}
