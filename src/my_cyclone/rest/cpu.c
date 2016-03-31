/*
 * cpu.c
 *
 *  Created on: 22 окт. 2015 г.
 *      Author: timurtaipov
 */
#include "cpu.h"


#if (configUSE_IDLE_HOOK == 1) //Don't forget to define configUSE_IDLE_HOOK in FreeRTOSConfig.h

#include "portable.h"
#include "task.h"
#include "uuid.h"

static portTickType LastTick;
static unsigned long count;             //наш трудяга счетчик
static unsigned long max_count ;                //максимальное значение счетчика, вычисляется при калибровке и соответствует 100% CPU idle


#define _GET_DIFF(a ,b) (a-b)

register_rest_function(cpu, "/cpu", NULL, NULL, &restGetCpuStatus, NULL, NULL, NULL);

error_t restGetCpuStatus(HttpConnection *connection, RestApi_t* RestApi)
{
   (void) RestApi;

   int p =0;
   error_t error = NO_ERROR;

   volatile size_t free = xPortGetFreeHeapSize();
   if (RestApi->restVersion == 1)
   {
   p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "{\"cpu\": {\"load\" : %u, \"clock\" : %u, \"temperature\" : %u}, ", GetCPU_Load(), (unsigned int)SystemCoreClock, 38);
   p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "\"memory\" : {\"total\" : %lu, \"heap\" : %lu, \"free\" : %lu, \"usage\" : %lu}, ", 128*1024ul, (unsigned long int)configTOTAL_HEAP_SIZE, (unsigned long)free, (unsigned long int)configTOTAL_HEAP_SIZE-(unsigned long)free);
   p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "\"uid\" : \"%s\" }\r\n",  get_uuid(NULL) ) ; //Print full UUID

   connection->response.contentType = mimeGetType(".json");
   }
   else    if (RestApi->restVersion == 2)
   {
      p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "{\"data\":{\"type:\":\"cpu\",\"id\":0,\"attributes\": {\"load\" : %u, \"clock\" : %u, \"temperature\" : %u, ", GetCPU_Load(), (unsigned int)SystemCoreClock, 38);
         p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "\"total\" : %lu, \"heap\" : %lu, \"free\" : %lu, \"usage\" : %lu,", 128*1024ul, (unsigned long int)configTOTAL_HEAP_SIZE, (unsigned long)free, (unsigned long int)configTOTAL_HEAP_SIZE-(unsigned long)free);
         p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "\"uid\" : \"%s\" }}}\r\n",  get_uuid(NULL) ) ; //Print full UUID

   }
   connection->response.noCache = TRUE;

   error=rest_200_ok(connection, &restBuffer[0]);
   //Any error to report?

   return error;
}
//---------------------------------
static volatile unsigned char CPU_IDLE = 0;

//---------------------------------
unsigned char GetCPU_Load(void)     //Возвращает % загрузки
{
   return (100 - CPU_IDLE);
}

unsigned char GetCPU_Idle(void)      //Возвращает % простоя
{
   return CPU_IDLE;
}

//----------------------------------
void vApplicationIdleHook( void ) {     //это и есть поток с минимальным приоритетом

   count++;                                                  //приращение счетчика

   if(_GET_DIFF(xTaskGetTickCount(), LastTick ) > 1000)    //если прошло 1000 тиков (1 сек для моей платфрмы)
   {
      LastTick = xTaskGetTickCount();
      if(count > max_count) max_count = count;          //это калибровка
      CPU_IDLE = 100 * count / max_count;               //вычисляем текущую загрузку
      count = 0;                                        //обнуляем счетчик
   }
}
#endif
