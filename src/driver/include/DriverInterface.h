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
#include "DataManager.h"

#define MAX_OF_PERIPHERALS 32
/* Peripheral handles are void * for data hiding purposes. */
//typedef void * Peripheral_Descriptor_t;
typedef void * peripheral_t;
//typedef struct Tperipheral_t peripheral_t;
typedef struct Tmask_t mask_t;

/**
 * @brief Device status
 **/

typedef enum
{
   DEV_STAT_MANAGED = 0x01,
   DEV_STAT_ACTIVE = 0x02,
   DEV_STAT_ONLINE = 0x04,
   DEV_STAT_READBLE = 0x08,
   DEV_STAT_WRITEBLE = 0x10,

} devStatusAttributes;

typedef struct
{
   devStatusAttributes statusAttribute;
   bool_t initialized;
   OsMutex mutex;
} devState;
/**
 * @brief Device opening type
 **/
typedef enum
{
   POPEN_UNDEFINED, POPEN_READ = 0x01, POPEN_WRITE = 0x02, POPEN_CONFIGURE = 0x04, POPEN_INFO = 0x08,
} periphOpenType;

/**
 * @brief Answer type
 **/
typedef enum
{
   FLOAT, UINT16, CHAR, PCHAR
} periphDataType;

/* Types that define valid read(), write() and ioctl() functions. */

typedef size_t (*Peripheral_open_Function_t)(peripheral_t const pxPeripheral);
typedef size_t (*Peripheral_read_Function_t)(peripheral_t pxPeripheral, void * const pvBuffer, const size_t xBytes);
typedef size_t (*Peripheral_write_Function_t)(peripheral_t const pxPeripheral, const void *pvBuffer,
   const size_t xBytes);
typedef size_t (*setPropertyFunction_t)(peripheral_t const pxPeripheral, char *pcValue);
typedef size_t (*getPropertyFunction_t)(peripheral_t const pxPeripheral, char *pcValue, const size_t xBytes);

typedef struct
{
   const char * property;
   const periphDataType dataType;
   const setPropertyFunction_t setFunction;
   const getPropertyFunction_t getFunction;
} property_t;

typedef struct
{
   const property_t * properties;
   size_t numOfProperies;
} properties_t;

typedef struct
{
   const Peripheral_open_Function_t open; /*Function to ioctl peripheral*/
   const Peripheral_write_Function_t write; /*Function to read some bytes from peripheral*/
   const Peripheral_read_Function_t read; /*Function to write some bytes to peripheral */
} driver_functions_t;

typedef struct
{
   const char * name; /*Internal name of the driver*/
   const char * path; /*Text name of the peripheral. For example /ONEWIRE/ or /GPIO/ or /somthing_else/ */
   const driver_functions_t *functions;
   devState * status; /* Array of status register */
   const uint32_t countOfPerepherals; /*Count of perepherals, that uses this driver*/
   const properties_t * propertyList;
   const periphDataType dataType;
   mask_t * mask;
} driver_t;

typedef struct Tmask_t
{
   const driver_t * driver;
   unsigned mask : MAX_OF_PERIPHERALS;
} mask_t;

typedef struct
{
   driver_t * driver;
   devState * status;
   uint8_t peripheralNum; /*Number of peripheral*/
   OsMutex * mutex; /*Mutex for exclusive access*/
   periphOpenType mode;
} Tperipheral_t;

/*
 * Function prototypes.
 */
Tperipheral_t * driver_open(const char * path, const periphOpenType flags);
size_t driver_read(Tperipheral_t * pxPeripheral, void * const pvBuffer, const size_t xBytes);
size_t driver_write(Tperipheral_t * pxPeripheral, const void * pvBuffer, const size_t xBytes);
size_t driver_setproperty(Tperipheral_t * pxPeripheral, char * pcRequest, char *pcValue);
size_t driver_getproperty(Tperipheral_t * pxPeripheral, char * pcRequest, char *pcValue, const size_t xBytes);
void driver_close(Tperipheral_t * pxPeripheral);

#define str(s) #s

#define register_driver(d_name, d_path, d_funcs, d_status_a, d_max_peripheral, d_prop_l, d_data_type) \
   const driver_t driver_##d_name;\
   \
   mask_t d_name##_mask =  \
   {\
   .driver = &(driver_##d_name), \
   };\
   \
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
      .mask = &(d_name##_mask),\
   };\


extern const driver_t __start_drivers; //предоставленный линкером символ начала секции rest_functions
extern const driver_t __stop_drivers; //предоставленный линкером символ конца секции rest_functions

#define drivers_count (&__stop_drivers - &__start_drivers)

//#undef str

#endif /* DRIVER_INCLUDE_DRIVERINTERFACE_H_ */
