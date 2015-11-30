/*
 * uuid.c
 *
 *  Created on: 30 авг. 2015 г.
 *      Author: timurtaipov
 */

#include "uuid.h"
#include <stdio.h>

#define STM32_UUID ((void *)0x1FFF7A10)

uuid_t * uuid = STM32_UUID;
static char buffer[37];

/**
 * @brief Get pointer to string, containing UUID.
 * @param[in] Num of bytes to print from the end of UUID.
 * @return Pointer to formatted string
 **/
char * get_uuid (int * bytes_to_print)
{
   int p=0;
   int byte;
   int order_of_bytes[]={3,2,1,0,7,6,5,4,11,10,9,8};
   if (bytes_to_print == NULL || *bytes_to_print > 12)
   {
      byte = 0; //Start bit = 0
   }
   else
   {
      byte = 12 - *bytes_to_print;
   }
   while (byte<12)
   {
      p+=sprintf(&buffer[0]+p,"%02X:",(unsigned int)uuid->b[order_of_bytes[byte]] );
      byte++;
   }
   buffer[p-1] = '\0';
   return &buffer[0];
}
