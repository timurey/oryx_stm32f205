/*
 * logic.h
 *
 *  Created on: 17 янв. 2016 г.
 *      Author: timurtaipov
 */

#ifndef EXPRESSION_PARSER_LOGIC_H_
#define EXPRESSION_PARSER_LOGIC_H_
#include "error.h"
void parser_task (void *pvParameters);
error_t logicConfigure(void);

#define EXPRESSIONS_LENGHT 128
#define EXPRESSION_MAX_COUNT 8
#define RESULT_LENGHT 64
char expressions[EXPRESSIONS_LENGHT];
char * pExpression[EXPRESSION_MAX_COUNT];

char rules[RESULT_LENGHT];
char * pRules[EXPRESSION_MAX_COUNT];

#endif /* EXPRESSION_PARSER_LOGIC_H_ */
