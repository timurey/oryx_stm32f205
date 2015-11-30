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
/**
 * Finds a value of a key in parsed JSON data string. Returns number of element of array of tokens.
 * Path must be absolute ("/path/to/key")
 */
int jsmn_get_value(const char *pJSON, jsmntok_t * pTok, int numOfTokens, char * pPath);

/**
 * Finds a value of element in array in parsed JSON data string. Returns number of element of array of tokens.
 * Path must be absolute ("/path/to/array")
 */
int jsmn_get_array_value (const char *pJSON, jsmntok_t * pTok, int numOfTokens, char * pPath, int numOfElement);
#endif /* JSMN_PARENT_LINKS */
#endif /* _JSMN_EXTRAS_H_ */
