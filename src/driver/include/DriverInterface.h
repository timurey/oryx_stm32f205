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
#include "error.h"

/* Peripheral handles are void * for data hiding purposes. */
//typedef void * Peripheral_Descriptor_t;
typedef struct Tperipheral_t peripheral_t;

/**
 * @brief Device status
 **/

typedef enum
{
   DEV_STAT_MANAGED   = 0x01,
   DEV_STAT_ACTIVE      = 0x02,
   DEV_STAT_ONLINE      = 0x04,
   DEV_STAT_READBLE      = 0x08,
   DEV_STAT_WRITEBLE      = 0x10,

} devStatusAttributes;

/**
 * @brief Answer type
 **/
typedef enum
{
   FLOAT,
   UINT16,
   CHAR,
   PCHAR
}periphDataType;

/* Types that define valid read(), write() and ioctl() functions. */

typedef size_t ( *Peripheral_open_Function_t )( peripheral_t * const pxPeripheral);
typedef size_t ( *Peripheral_read_Function_t )( peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes );
typedef size_t ( *Peripheral_write_Function_t )( peripheral_t * const pxPeripheral, const void *pvBuffer, const size_t xBytes );
typedef size_t ( *Peripheral_ioctl_Function_t )( peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue );
typedef size_t ( *setPropertyFunction_t )( peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue );
typedef size_t ( *getPropertyFunction_t )( peripheral_t * const pxPeripheral, char* pcParameter, char *pcValue );

typedef struct {
   const char * property;
   const periphDataType dataType;
   const setPropertyFunction_t setFunction;
   const getPropertyFunction_t getFunction;
}property_t;

typedef struct {
   property_t * properties;
   size_t numOfProperies;
}properties_t;

typedef struct {
   const Peripheral_open_Function_t open; /*Function to ioctl peripheral*/
   const Peripheral_write_Function_t write; /*Function to read some bytes from peripheral*/
   const Peripheral_read_Function_t read;   /*Function to write some bytes to peripheral */
//   const Peripheral_ioctl_Function_t setProperty; /*Function to ioctl peripheral*/
}driver_functions_t;

typedef struct {
   const char * name; /*Internal name of the driver*/
   const char * path; /*Text name of the peripheral. For example /ONEWIRE/ or /GPIO/ or /somthing_else/ */
   const driver_functions_t *functions;
   const devStatusAttributes * status; /* Array of status register */
   const uint32_t countOfPerepherals;       /*Count of perepherals, that uses this driver*/
   const properties_t * propertyList;
   const periphDataType dataType;
}driver_t;


typedef struct Tperipheral_t {
   driver_t * driver;
   devStatusAttributes * status;
   uint32_t peripheralNum; /*Number of peripheral*/
   OsMutex mutex; /*Mutex for exclusive access*/
}peripheral_t;


/*
 * Function prototypes.
 */
error_t driver_open(peripheral_t * const pxPeripheral, const char * path, const uint16_t flags);
size_t driver_read(peripheral_t * const pxPeripheral, void * const pvBuffer, const size_t xBytes );
size_t driver_write(peripheral_t * const pxPeripheral, const void *pvBuffer, const size_t xBytes );
size_t driver_setproperty( peripheral_t * const pxPeripheral, char * pcRequest, char *pcValue );
size_t driver_getproperty( peripheral_t * const pxPeripheral, char * pcRequest, char *pcValue, const size_t xBytes );
void driver_close(peripheral_t  * pxPeripheral);

#define str(s) #s

#define register_driver(d_name, d_path, d_funcs, d_status_a, d_max_peripheral, d_prop_l, d_data_type) \
const properties_t properties_##d_name  = \
   { \
   .properties = &d_prop_l[0],\
   .numOfProperies = arraysize(d_prop_l),\
   }; \
const driver_t driver_##d_name __attribute__ ((section ("drivers"))) = \
   { \
   .name = str(d_name),\
   .path = d_path,\
   .functions = &d_funcs,\
   .status = &d_status_a[0],\
   .countOfPerepherals = d_max_peripheral,\
   .propertyList = &(properties_##d_name), \
   .dataType = d_data_type,\
   }

extern driver_t __start_drivers; //предоставленный линкером символ начала секции rest_functions
extern driver_t __stop_drivers; //предоставленный линкером символ конца секции rest_functions



//#undef str

#endif /* DRIVER_INCLUDE_DRIVERINTERFACE_H_ */
