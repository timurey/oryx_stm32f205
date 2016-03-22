/*
 * jsmn_extras.c
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */


#include "jsmn_extras.h"
#include <stdlib.h>
#ifdef JSMN_PARENT_LINKS
#define ISDIGIT(a) (((a)>='0') && ((a)<='9'))

/**
 * Implement of strncmp function.
 */

static int jsmn_strncmp(const char *s1, const char *s2, size_t len)
{
   for ( ; (*s1 == *s2)&&(len--); s1++, s2++)
   {
      if (!len)
         return(0);
   }
   return(*s1-*s2);
}
/**
 * Find an element of array, specified by parent token id and element number
 * and return a number of token in tokens array
 */
static int jsmn_get_array_value_by_tokens (jsmntok_t * pTok, int numOfTokens, int parentToken, int numOfElement)
{
   int crntToken;
   int cur_num=0;

   if (parentToken < 0) return -1; // Incorrect path or path not found
   if (pTok->type != JSMN_ARRAY) return -1; //Path is not array
   if (pTok->size < 1) return -1; //Path is not ended by array or array is empty

   for (crntToken=parentToken; crntToken<numOfTokens; crntToken++) // перебираем все токены, начиная с родительского
   {
      if (pTok->parent == parentToken)
      {
         if (cur_num++== numOfElement)
            return crntToken;
      }
      pTok++;
   }

   return -1; //Array's element not reached...
}
/**
 * Find a key, specified by path and return a number of token in tokens array
 */
/*
 * todo некорректно пропускается пустой элемент массива,
 * рассмотреть такой вариант адресации элементов
 * let obj = json?["workplan"]?["presets"]?[1]?["id"] as? Int
 */
static int jsmn_find_key(const char *pJSON, jsmntok_t * pTok, int numOfTokens, char * pPath)
{
   char *p = pPath;
   int level=0;
   int crnLevel = 0;
   int crntToken=0;
   jsmntok_t * pDTok = pTok;
   if (*(p)!='/')
      return -1; // Incorrect path
   int parent = -1;
   while (*p)
   {
      if (*(p++) == '/')
         level++;
   }
   p = pPath;

   for (crntToken=0; crntToken<numOfTokens; crntToken++)
   {
      pDTok = pTok + crntToken;
      //Found token with needed parrent and name
      if(
         (jsmn_strncmp((pJSON+(pDTok)->start), p, (pDTok)->end - (pDTok)->start) == 0 \
            && ((*(p+ ((pDTok)->end - (pDTok)->start)) == '/' )\
               || (*(p+ ((pDTok)->end - (pDTok)->start)) == '\0' ))\
         ) \
         || ( (*p=='\\') && ( *(p+1)=='[') && ISDIGIT(*(p+2)) )\
         || ((*(pJSON+(pDTok)->start) == '{') || (*(pJSON+(pDTok)->start) == '}'))  \
      )
      {
         if (parent == pDTok->parent)
         {
            parent = crntToken;

            if (*(pJSON+(pDTok)->start) == '{')
            {
               crnLevel++;
               p++;
            }
            else if (*(pJSON+(pDTok)->start) == '}')
            {
               crnLevel--;
               p++;
            }
            else if (jsmn_strncmp((pJSON+(pDTok)->start), p, (pDTok)->end - (pDTok)->start) == 0)
            {
               if (crnLevel == level)
                  return crntToken;
               while (*++p!='/' || *p == '\0');
            }
            else
            {
               if (( (*p=='\\') && ( *(p+1)=='[') ) && ISDIGIT(*(p+2)))
               {
                  crntToken = jsmn_get_array_value_by_tokens(pDTok, numOfTokens, crntToken, atoi(p+2));
                  if (crntToken<0)
                  {
                     return -1; //wrong path or
                  }
               }
            }
         }
      }

   }


   return -1; // Path not found

}

/**
 * Find a value, specified by path and return a number of token in tokens array
 */
int jsmn_get_value (const char *pJSON, jsmntok_t * pTok, int numOfTokens, char * pPath)
{
   int crntToken;
   int parentToken = jsmn_find_key(pJSON, pTok, numOfTokens, pPath);

   if (parentToken < 1) return -1; // Incorrect path or path not found

   if ((pTok+parentToken)->size!=1) return -1; //Path is not ended by key

   for (crntToken=parentToken; crntToken<numOfTokens; crntToken++)
   {
      if ((pTok+crntToken)->parent == parentToken)
         return crntToken;
   }

   return -1; //Something was wrong...
}





/**
 * Find an element of array, specified by path and element number
 * and return a number of token in tokens array
 */
int jsmn_get_array_value (const char *pJSON, jsmntok_t * pTok, int numOfTokens, char * pPath, int numOfElement)
{
   int crntToken;
   int cur_num=0;
   int parentToken = jsmn_get_value(pJSON, pTok, numOfTokens, pPath);

   if (parentToken < 0) return -1; // Incorrect path or path not found
   if ((pTok+parentToken)->type != JSMN_ARRAY) return -1; //Path is not array
   if ((pTok+parentToken)->size < 1) return -1; //Path is not ended by array or array is empty

   for (crntToken=parentToken; crntToken<numOfTokens; crntToken++)
   {
      if ((pTok+crntToken)->parent == parentToken)
      {
         if (cur_num++== numOfElement)
            return crntToken;
      }
   }

   return -1; //Array's element not reached...
}
#endif
