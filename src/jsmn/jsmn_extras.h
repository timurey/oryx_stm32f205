/*
 * jsmn_extras.h
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */

#ifndef _JSMN_EXTRAS_H_
#define _JSMN_EXTRAS_H_

#include "jsmn.h"

#ifdef JSMN_PARENT_LINKS

typedef struct
{
   jsmn_parser *jSMNparser;
   jsmntok_t *jSMNtokens;
   unsigned int numOfTokens;
   const char * data;
   size_t lengthOfData;
   int resultCode;
}jsmnParserStruct;

int jsmn_get_value_token(jsmnParserStruct * jsonStruct,  char * pPath);
int jsmn_get_string(jsmnParserStruct * jsonStruct,  char * pPath, char * string, int maxlen);
int jsmn_get_bool(jsmnParserStruct * jsonStruct,  char * pPath, int* pointer);

#define xjsmn_parse(jsonParser) jsmn_parse(jsonParser->jSMNparser, jsonParser->data, jsonParser->lengthOfData, jsonParser->jSMNtokens, jsonParser->numOfTokens);
#endif /* JSMN_PARENT_LINKS */
#endif /* _JSMN_EXTRAS_H_ */
