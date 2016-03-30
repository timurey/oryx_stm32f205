/*
 * macros.h
 *
 *  Created on: 29 марта 2016 г.
 *      Author: timurtaipov
 */

#ifndef STUFF_MACROS_H_
#define STUFF_MACROS_H_


#define ISDIGIT(a) ((a>='0' && a<='9'))
#define ISUPPER(a) (a>='A' && a<= 'Z')
#define ISALPHA(a) ((a>='a' && a<='z')||(a>='A' && a<= 'Z'))
#define ISALNUM(a) (ISDIGIT(a)||ISALPHA(a))
#define ISSPACE(a) (a==' ' || a=='\t' || a== '\n')
#define ISDOT(a) (a=='.')
#endif /* STUFF_MACROS_H_ */
