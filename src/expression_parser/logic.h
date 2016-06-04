/*
 * logic.h
 *
 *  Created on: 17 янв. 2016 г.
 *      Author: timurtaipov
 */

#ifndef EXPRESSION_PARSER_LOGIC_H_
#define EXPRESSION_PARSER_LOGIC_H_
#include "error.h"
#include"expression_parser.h"

#define EXPRESSIONS_LENGHT 128
#define EXPRESSION_MAX_COUNT 8
#define RESULT_LENGHT 64

error_t logicConfigure(void);
error_t logicStart(void);

#endif /* EXPRESSION_PARSER_LOGIC_H_ */
