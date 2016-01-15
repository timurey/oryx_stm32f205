/*
 * onewire_rest.c
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: timurtaipov
 */
#include "onewire_rest.h"

static error_t restGetOneWire (HttpConnection *connection, RestApi_t* RestApi);

/*
 * todo: caution! restGetOneWire is not atomic!
 * oneWireFoundedDevices may been changed while function is running
 */
register_rest_function(onewire, "/onewire", NULL, NULL, &restGetOneWire, NULL, NULL, NULL);

static error_t restGetOneWire (HttpConnection *connection, RestApi_t* RestApi)
{
	(void) RestApi;
	int i;
	int p =0;
	error_t error = NO_ERROR;

	p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "{\"onewire\":{\"status\":\"ok\",\"health\":%d,", onewireHealth);
	p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "\"founded devices\":{\"count\":%d,\"serials\":[", oneWireFoundedDevices);

	for (i=0; i<oneWireFoundedDevices; i++ )
	{
		p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "\"%s\",", serialHexToString(foundedSerial[i].serial, NULL, ONEWIRE_SERIAL_LENGTH));
	}
	if (oneWireFoundedDevices>0)
	{
		p--;
	}
	p+=snprintf(restBuffer+p, sizeof(restBuffer)-p, "]}}}\r\n");

	connection->response.contentType = mimeGetType(".json");
	connection->response.noCache = TRUE;

	error=rest_200_ok(connection, &restBuffer[0]);
	//Any error to report?

	return error;
}
