/*
 * sensors_def.h
 *
 *  Created on: 11 сент. 2015 г.
 *      Author: timurtaipov
 */
#include "rest.h"

#ifndef MY_CYCLONE_REST_SENSORS_DEF_H_
#define MY_CYCLONE_REST_SENSORS_DEF_H_
typedef error_t (*tInitSensHandler)(const char * data, jsmntok_t *jSMNtokens, sensor_t ** pCurrentSensor, jsmnerr_t * resultCode, uint8_t * pos);
typedef error_t (*tDeinitSensHandler)(void);
typedef error_t (*tGetSensHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPostSensHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPutSensHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tDeleteSensHandler)(HttpConnection *connection, RestApi_t * rest);

typedef struct
{
   const char *sensClassPath;
   const mysensor_sensor_t sensorType;
   const tInitSensHandler sensInitClassHadler;
   const tDeinitSensHandler sensDenitClassHadler;
   const tGetSensHandler sensGetMethodHadler;
   const tPostSensHandler sensPostMethodHadler;
   const tPutSensHandler sensPutMethodHadler;
   const tDeleteSensHandler sensDeleteMethodHadler;
} sensFunctions;


#define register_sens_function(name, path, type, init_f, deinit_f, get_f, post_f, put_f, delete_f) \
   const sensFunctions handler_##name __attribute__ ((section ("sens_functions"))) = \
{ \
  .sensClassPath = path, \
  .sensorType = type, \
  .sensInitClassHadler = init_f, \
  .sensDenitClassHadler = deinit_f, \
  .sensGetMethodHadler = get_f, \
  .sensPostMethodHadler = post_f, \
  .sensPutMethodHadler = put_f, \
  .sensDeleteMethodHadler = delete_f \
}
extern sensFunctions __start_sens_functions; //предоставленный линкером символ начала секции rest_functions
extern sensFunctions __stop_sens_functions; //предоставленный линкером символ конца секции rest_functions

#endif /* MY_CYCLONE_REST_SENSORS_DEF_H_ */
