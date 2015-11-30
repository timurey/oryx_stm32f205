/*
 * configs.h
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */

#ifndef STUFF_CONFIGS_H_
#define STUFF_CONFIGS_H_
#include "error.h"
#include "../jsmn/jsmn_extras.h"
#define CONFIG_JSMN_NUM_TOKENS 256
typedef error_t (*tConfigParser)(char * data, size_t len, jsmn_parser *jSMNparser, jsmntok_t *jSMNtokens);

error_t configInit(void);
void configDeinit(void);

error_t read_config(char * path, tConfigParser parser);
error_t save_config (char * path, const char*	fmt, ...);
typedef error_t (* tConfigHandler)(char *data, size_t len, jsmn_parser* jSMNparser, jsmntok_t *jSMNtokens);
typedef struct
{
	const char *configFilename;
	const tConfigHandler configParsingFunction;
} configParsingFunctions;

#define register_config_function(name, filename, func) const configParsingFunctions handler_##name __attribute__ ((section ("config_functions"))) = \
{ \
  .configFilename = filename, \
  .configParsingFunction = func \
}
extern configParsingFunctions __start_config_functions; //предоставленный линкером символ начала секции rest_functions
extern configParsingFunctions __stop_config_functionss; //предоставленный линкером символ конца секции rest_functions
#endif /* STUFF_CONFIGS_H_ */
