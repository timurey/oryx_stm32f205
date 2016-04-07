/*
 * rest.h
 *
 *  Created on: 06 мая 2015 г.
 *      Author: timurtaipov
 */

#ifndef MY_CYCLONE_REST_REST_H_
#define MY_CYCLONE_REST_REST_H_

#include "httpd.h"
#include "jsmn_extras.h"
#define SIZE_OF_REST_BUFFER 4096

//#define REST_JSON_TYPE JSON
//#define REST_JSON_TYPE JSON_API
extern char restBuffer[SIZE_OF_REST_BUFFER];
extern OsMutex restMutex;

extern char restPrefix[];
error_t restInit(void);
error_t rest_answer(HttpConnection *connection, char * message, uint_t error_code);
error_t restTry (HttpConnection *connection, const char_t *uri);
typedef struct TRestApi RestApi_t;

typedef error_t (*tInitRestHandler)(void);
typedef error_t (*tDeinitRestHandler)(void);

//todo: exclude RestApi_t from these functions:
typedef error_t (*tGetRestHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPostRestHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tPutRestHandler)(HttpConnection *connection, RestApi_t * rest);
typedef error_t (*tDeleteRestHandler)(HttpConnection *connection, RestApi_t * rest);

#define rest_200_ok(a, b) rest_answer(a, b, 200)
#define rest_300_multiple_choices(a, b) rest_answer(a, b, 300)
#define rest_400_bad_request(a, b) rest_answer(a, b, 400)
#define rest_404_not_found(a, b) rest_answer(a, b, 404)
#define rest_500_int_server_error(a, b) rest_answer(a, b, 500)
#define rest_501_not_implemented(a, b) rest_answer(a, b, 501)

#define REST_CLASS_EQU(rest , class) ( (rest->classNameLen == strlen(class) && (strncmp(rest->className, class, strlen(class)) == 0) ))
#define NAME_EQU(name1, len, name2) ( (len == strlen(name2) && (strncmp(name1, name2, len) == 0) ))

/**
 * @brief HTTP version numbers
 **/

typedef enum
{
	METHOD_GET = 0x01,
	METHOD_POST = 0x02,
	METHOD_PUT = 0x03,
	METHOD_PATCH = 0x04,
	METHOD_DELETE = 0x05
} HttpMethod_t;

typedef struct
{
	HttpMethod_t method;
	const char_t message[7];
} HttpMethodCodeDesc;

static const HttpMethodCodeDesc methodCodeList[] =
{
		//Success
		{METHOD_GET, "GET"},
		{METHOD_POST, "POST"},
		{METHOD_PUT, "PUT"},
		{METHOD_PATCH, "PATCH"},
		{METHOD_DELETE, "DELETE"}

};
typedef struct
{
   const char *restClassName;
   const char *restClassPath;
   const tInitRestHandler restInitClassHadler;
   const tDeinitRestHandler restDenitClassHadler;
   const tGetRestHandler restGetClassHadler;
   const tPostRestHandler restPostClassHadler;
   const tPutRestHandler restPutClassHadler;
   const tDeleteRestHandler restDeleteClassHadler;
} restFunctions;

typedef struct TRestApi
{
	int restVersion;
	char* class;
	char* className;
	char* objectId;
	HttpMethod_t method;
	size_t classLen;
	size_t classNameLen;
	size_t objectIdLen;
	restFunctions* restClassHadlers;
} RestApi_t;

error_t restParsePath(HttpConnection *connection, RestApi_t *RestApi);
error_t findRestHandler(RestApi_t* RestApi);

#define str(s) #s

#define register_rest_function(name, path, init_f, deinit_f, get_f, post_f, put_f, delete_f) \
   const restFunctions handler_##name __attribute__ ((section ("rest_functions"))) = \
{ \
  .restClassName = str(name), \
  .restClassPath = path, \
  .restInitClassHadler = init_f, \
  .restDenitClassHadler = deinit_f, \
  .restGetClassHadler = get_f, \
  .restPostClassHadler = post_f, \
  .restPutClassHadler = put_f, \
  .restDeleteClassHadler = delete_f \
}
extern restFunctions __start_rest_functions; //предоставленный линкером символ начала секции rest_functions
extern restFunctions __stop_rest_functions; //предоставленный линкером символ конца секции rest_functions

#endif /* MY_CYCLONE_REST_REST_H_ */
