/*
 * logic_def.h
 *
 *  Created on: 07 апр. 2016 г.
 *      Author: timurtaipov
 */

#ifndef EXPRESSION_PARSER_LOGIC_DEF_H_
#define EXPRESSION_PARSER_LOGIC_DEF_H_

#include "jsmn_extras.h"
#include "error.h"

typedef error_t (*tGetVarHandler)(const char * path, double * value);

typedef struct
{
   const char *varClassName;
   const tGetVarHandler varGetMethodHadler;
} varFunctions;

#define str(s) #s

#define register_variables_functions(name, get_f) \
   const varFunctions handler_var_##name __attribute__ ((section ("var_functions"))) = \
{ \
  .varClassName = str(name), \
  .varGetMethodHadler = get_f\
}
extern varFunctions __start_var_functions; //предоставленный линкером символ начала секции rest_functions
extern varFunctions __stop_var_functions; //предоставленный линкером символ конца секции rest_functions


#endif /* EXPRESSION_PARSER_LOGIC_DEF_H_ */
