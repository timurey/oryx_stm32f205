/*
 * temperature.h
 *
 *  Created on: 02 июля 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_SENSORS_TEMPERATURE_H_
#define MY_CYCLONE_REST_SENSORS_TEMPERATURE_H_

//#include "rest/sensors.h"
#include "onewire.h"

#define MAX_TEMPERATURE_SENSORS_COUNT MAX_DS1820_COUNT

error_t getRestTemperature(HttpConnection *connection, RestApi_t* RestApi);
error_t postRestTemperature(HttpConnection *connection, RestApi_t* RestApi);
error_t putRestTemperature(HttpConnection *connection, RestApi_t* RestApi);
error_t deleteRestTemperature(HttpConnection *connection, RestApi_t* RestApi);
//error_t initTemperature (const char * data, jsmntok_t *jSMNtokens, sensor_t ** currentSensor, jsmnerr_t *  resultCode, uint8_t * pos);
error_t deinitTemperature (void);
error_t initTemperature (const char * data, jsmntok_t *jSMNtokens, sensor_t ** pCurrentSensor, int resultCode, uint8_t * pos);


#endif /* MY_CYCLONE_REST_SENSORS_TEMPERATURE_H_ */
