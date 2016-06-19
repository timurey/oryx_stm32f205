/*
 * sensors_def.h
 *
 *  Created on: 11 сент. 2015 г.
 *      Author: timurtaipov
 */
#include "rest.h"

#ifndef MY_CYCLONE_REST_SENSORS_DEF_H_
#define MY_CYCLONE_REST_SENSORS_DEF_H_
typedef error_t (*tInitSensHandler)(const char * data, jsmntok_t *jSMNtokens, sensor_t ** pCurrentSensor, int resultCode, uint8_t * pos);
typedef error_t (*tDeinitSensHandler)(void);
typedef int (*tGetSensHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPostSensHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPutSensHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tDeleteSensHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPrintfSensHandler)(char * bufer, size_t max_len, int sens_num);
typedef struct
{
   const char *sensClassName;
   const char *sensClassPath;
   const mysensor_sensor_t sensorType;
   const tInitSensHandler sensInitClassHadler;
   const tDeinitSensHandler sensDenitClassHadler;
   const tGetSensHandler sensGetMethodHadler;
   const tPostSensHandler sensPostMethodHadler;
   const tPutSensHandler sensPutMethodHadler;
   const tDeleteSensHandler sensDeleteMethodHadler;
//   const sensValueType_t sensValueType;
} sensFunctions;

#define str(s) #s

#define register_sens_function(name, path, type, init_f, deinit_f, get_f, post_f, put_f, delete_f, valueType) \
   const sensFunctions handler_##name __attribute__ ((section ("sens_functions"))) = \
{ \
  .sensClassName = str(name), \
  .sensClassPath = path, \
  .sensorType = type, \
  .sensInitClassHadler = init_f, \
  .sensDenitClassHadler = deinit_f, \
  .sensGetMethodHadler = get_f, \
  .sensPostMethodHadler = post_f, \
  .sensPutMethodHadler = put_f, \
  .sensDeleteMethodHadler = delete_f, \
}
extern sensFunctions __start_sens_functions; //предоставленный линкером символ начала секции rest_functions
extern sensFunctions __stop_sens_functions; //предоставленный линкером символ конца секции rest_functions

#endif /* MY_CYCLONE_REST_SENSORS_DEF_H_ */
