/*
 * DriverInterface.h
 *
 *  Created on: 19 мая 2016 г.
 *      Author: timurtaipov
 */

#ifndef DRIVER_INCLUDE_DRIVERINTERFACE_H_
#define DRIVER_INCLUDE_DRIVERINTERFACE_H_

#include <stddef.h>
#include <stdint.h>
#include "os_port.h"
/* Peripheral handles are void * for data hiding purposes. */
typedef void * Peripheral_Descriptor_t;

/* Types that define valid read(), write() and ioctl() functions. */

typedef size_t ( *Peripheral_open_Function_t )( Peripheral_Descriptor_t const pxPeripheral);
typedef size_t ( *Peripheral_read_Function_t )( Peripheral_Descriptor_t const pxPeripheral, void * const pvBuffer, const size_t xBytes );
typedef size_t ( *Peripheral_write_Function_t )( Peripheral_Descriptor_t const pxPeripheral, const void *pvBuffer, const size_t xBytes );
typedef size_t ( *Peripheral_ioctl_Function_t )( Peripheral_Descriptor_t const pxPeripheral, char* pcParameter, char *pcValue );

typedef struct {
   const char * name; /*Internal name of the driver*/
   const char * path; /*Text name of the peripheral. For example /ONEWIRE/ or /GPIO/ or /somthing_else/ */
   const Peripheral_open_Function_t open; /*Function to ioctl peripheral*/
   const Peripheral_write_Function_t write; /*Function to read some bytes from peripheral*/
   const Peripheral_read_Function_t read;   /*Function to write some bytes to peripheral */
   const Peripheral_ioctl_Function_t ioctl; /*Function to ioctl peripheral*/
   const uint32_t countOfPerepherals;       /*Count of perepherals, that uses this driver*/
}driver_t;

typedef struct {
   driver_t * driver;
   uint32_t peripheralNum; /*Number of peretheral*/
   OsMutex mutex; /*Mutex for exclusive access*/
}peripheral_t;


/*
 * Function prototypes.
 */
Peripheral_Descriptor_t * driver_open(const char * path, const uint16_t flags);

size_t driver_read(Peripheral_Descriptor_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes );
size_t driver_write(Peripheral_Descriptor_t * const pxPeripheral, const void *pvBuffer, const size_t xBytes );
size_t driver_ioctl( Peripheral_Descriptor_t * const pxPeripheral, char * pcRequest, char *pcValue );
void driver_close(Peripheral_Descriptor_t  * pxPeripheral);

#define str(s) #s

#define register_driver(d_name, d_path,d_open_f, d_write_f, d_read_f, d_ioctl_f, d_max_peripheral) \
   const driver_t driver_##d_name __attribute__ ((section ("drivers"))) = \
{ \
   .name = str(d_name),\
   .path = d_path,\
   .open = d_open_f,\
   .write = d_write_f,\
   .read = d_read_f,\
   .ioctl = d_ioctl_f,\
   .countOfPerepherals = d_max_peripheral,\
}

extern driver_t __start_drivers; //предоставленный линкером символ начала секции rest_functions
extern driver_t __stop_drivers; //предоставленный линкером символ конца секции rest_functions

//#undef str

#endif /* DRIVER_INCLUDE_DRIVERINTERFACE_H_ */
