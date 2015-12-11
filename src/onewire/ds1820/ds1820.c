/*
 * ds1820.c
 *
 *  Created on: 25 июня 2015 г.
 *      Author: timurtaipov
 */
#include "ds1820/ds1820.h"
#include "configs.h"
#include "rest/sensors.h"
#include "error.h"
#include <stdio.h>
#if (OW_DS1820_SUPPORT == ENABLE)

jsmn_parser parser;
jsmntok_t tokens[32]; // a number >= total number of tokens

static error_t scanDS1820(void);
static error_t grpoupDS1820(void);
static error_t individualDS1820(void);
static error_t ow_decode_temp(ds1820Scratchpad_t * ow_buf, uint8_t type, float *Temperature);


register_onewire_function (ds1820, &scanDS1820, &grpoupDS1820, &individualDS1820);


static error_t scanDS1820()
{
   int i;
   ds1820_t*  currentDS1820Sensor = &sensorsDS1820[0];
   memset(&sensorsDS1820, 0x00, sizeof(ds1820_t) * MAX_DS1820_COUNT);

   for (i=0; (i<MAX_ONEWIRE_COUNT); i++)
   {
      if (oneWireSensors[i].serial->type == 0x28)
      {
         *oneWireSensors[i].status |= MANAGED;
         currentDS1820Sensor->onewire = &oneWireSensors[i];
         currentDS1820Sensor++;
      }
   }
   return NO_ERROR;
}

static error_t grpoupDS1820(void)
{
   OW_Send(OW_SEND_RESET, (uint8_t *)"\xCC\x44", 2, 0, 0, OW_NO_READ); //0xCC,0x44 Skip Rom, Prepare temperature
   vTaskDelay(2000);
   return NO_ERROR;
}

static error_t individualDS1820(void)
{
   int i;
   ds1820Scratchpad_t buf;
   error_t error;

   for (i=0;i<MAX_DS1820_COUNT;i++)
   {
      if ((sensorsDS1820[i].onewire != NULL )&&(*(sensorsDS1820[i].onewire->status) & ONLINE ))
      {
         OW_Send(OW_SEND_RESET, (uint8_t *)"\x55", 1, NULL, 0, OW_NO_READ); //0x55 MatchRom
         OW_Send(OW_NO_RESET, &sensorsDS1820[i].onewire->serial->serial[0], 8, NULL, 0, OW_NO_READ); //Serial
         OW_Send(OW_NO_RESET, (uint8_t *)"\xBE", 1, NULL,0, OW_NO_READ); //0xBE - Выдать температуру
         OW_Send(OW_NO_RESET, (uint8_t *)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 9, &buf.lsb, 9 ,0); //read bytes
         *sensorsDS1820[i].onewire->status &= ~READEBLE;
         error = ow_decode_temp(&buf, sensorsDS1820[i].onewire->serial->type, sensorsDS1820[i].onewire->value );
         *sensorsDS1820[i].onewire->status |= READEBLE;
      }
   }
   return NO_ERROR;
}


error_t ow_decode_temp(ds1820Scratchpad_t * ow_buf, uint8_t type, float *Temperature)
{

   error_t error = NO_ERROR;
   short temp;
   // float Temperature=0;
   uint8_t  CRCR;
   CRCR = oneWireCRC(&(ow_buf->lsb), 8);

   if (ow_buf->crc == CRCR)
   {
      if (type==0x10)   //Если тип DS18S20 (0x10)
      {
         if (ow_buf->msb!=0xFF)
         {
            temp=ow_buf->lsb*5;
         }
         else
         {
            ow_buf->lsb=0xFF-ow_buf->lsb;
            temp =-1*(ow_buf->lsb*5);
         }
      }
      else if (type==0x28)   //Если тип DS18B20 (0x28)
      {

         if (ow_buf->msb & 0xF8)
         {                 //���� ����������� �������������
            temp=~(((unsigned int)ow_buf->msb<<8) | ow_buf->lsb) + 1;
            ow_buf->lsb = temp;
            ow_buf->msb = temp >> 8;
            temp=-1*(10*(((0x07&(short)ow_buf->msb)<<8)|((short )ow_buf->lsb))>>2)/4;
         }
         else
         {              //���� ����������� �������������
            temp=(10*(((0x07&(short)ow_buf->msb) << 8)|((short )ow_buf->lsb))>>2)/4;
         }
         //       *Temperature=(int)temp;

      }
      else
      {
         temp=255;
      }
      *Temperature=temp / 10.0;
   }
   else
   {
	   error = ERROR_BAD_CRC;
   }
   return error;
}

void setup_ds1820(void)
{
   //   error_t error;
   //   error = read_config("/config/sensors.json",&parseDS1820);

}


#endif
