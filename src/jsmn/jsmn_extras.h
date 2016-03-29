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

int jsmn_get_value(const char *js, jsmntok_t *tokens, unsigned int num_tokens,  char * pPath);

#endif /* JSMN_PARENT_LINKS */
#endif /* _JSMN_EXTRAS_H_ */
