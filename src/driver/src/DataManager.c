/*
 * DataManager.c
 *
 *  Created on: 02 июня 2016 г.
 *      Author: timurtaipov
 */
#include "DataManager.h"
#include "../../expression_parser/logic.h"
#include "error.h"
#include <stdint.h>

QueueHandle_t exprQueue;
QueueHandle_t dataQueue;

OsTask * dataManagerTask;

uint8_t depedencesArr[EXPRESSION_MAX_COUNT][3][16]; //32 - magic number (driver count & periph max count)


/* Build depedency matrix */
static void storeDependence(uint8_t depExpr, peripheral_t * depPeriph)
{
   uint32_t driverNumber = (depPeriph->driver-&__start_drivers);
   int peripheralNumber = depPeriph->peripheralNum;
   depedencesArr[depExpr][driverNumber][peripheralNumber] = 1;
}

void buildQueue(mask_t * mask)
{
   uint8_t exprNum;
   uint32_t periphNum;
   portBASE_TYPE xStatus;
   driver_t * driver= mask->driver;
   uint32_t driverNumber = (driver-&__start_drivers);

   for (exprNum = 0; exprNum < EXPRESSION_MAX_COUNT; exprNum++ )
   {
      for (periphNum = 0; periphNum < driver->countOfPerepherals; periphNum++ )
      {
         if ((depedencesArr[exprNum][driverNumber][periphNum] > 0) && (mask->mask & (1<<periphNum)))
         {
            xStatus = xQueueSendToBack(exprQueue, &exprNum, 0);
            if (xStatus == pdPASS)
            {
               //               depedencesArr[exprNum][driverNumber][periphNum] =0;
            }
         }
      }
   }
}

void dataManager_task(void * pvParameters)
{
   portBASE_TYPE xStatus;
   mask_t mask;
   while(1)
   {
      xStatus = xQueueReceive(dataQueue, &mask, INFINITE_DELAY);
      if (xStatus == pdPASS) {
         buildQueue(&mask);
      }
   }
}

int userVarFake( void *user_data, const char *name, double *value )
{
   peripheral_t fd; //file descriptor
   uint8_t expressionNumber = *( uint8_t *)(user_data);
   char device[32];
   snprintf(&device[0], 32, "/%s", name);

   memset(&fd, 0, sizeof(fd));

   if (driver_open(&fd, &device[0], POPEN_READ) == NO_ERROR)
   {
      storeDependence(expressionNumber, &fd);
      driver_close(&fd);
   }


   *value = 1.0;

   return PARSER_TRUE;

}

