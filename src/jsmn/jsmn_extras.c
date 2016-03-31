/*
 * jsmn_extras.c
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */


#include "jsmn_extras.h"
#include <string.h>
#define ISDIGIT(a) ((a>='0' && a<='9'))
#define ISUPPER(a) (a>='A' && a<= 'Z')
#define ISALPHA(a) ((a>='a' && a<='z')||(a>='A' && a<= 'Z'))
#define ISALNUM(a) (ISDIGIT(a)||ISALPHA(a))
#define ISSPACE(a) (a==' ' || a=='\t' || a== '\n')
#define ISDOT(a) (a=='.')

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifdef JSMN_PARENT_LINKS
/*
 * Implement of atoi function
 * This implementation do not support negative numbers
 */

static int jsmn_atoi ( char *s)
{

   int i =0;
   int sign = 1;
   while (ISSPACE(*s))
   {
      s++;
   }

   while (ISDIGIT(*s))
   {
      i=i*10+(*s-'0');
      s++;
   }
   return(i*sign);

}
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

/*
 * todo check every iteration on '\0'
 */
static char * nextNode(char * previousNode)
{
   char * next = previousNode;



   if (*next == '.')
   {
      next++;
   }
   else if (*next == '[')
   {
      next++;
   }
   else if (ISDIGIT(*next))
   {
      while(ISDIGIT(*next))
      {
         next++;
      }
      if (*next == ']')
      {
         next++;
         if (*next == '.')
         {
            next++;
         }
      }
   }
   else if (ISALNUM(*next))
   {
      while(ISALNUM(*next) || ISSPACE(*next))
      {
         next++;
      }
   }
   else if (*next == '$')
   {
      next++;
   }
   else
   {
      next = NULL;
   }

   return next;
}

static int lenOFNode(char * node)
{
   int len = 0;
   if (*node == '$')//если узел -корень
   {
      len = 1;
   }
   else if(*node == '.')
   {
      len = 1;

   }
   else if (*node == '[')
   {
      len = 1;
   }
   else if (ISALNUM(*node) || ISSPACE(*node))
   {
      while(ISALNUM(*node) || ISSPACE(*node))
      {
         len++;
         node++;
      }
   }
   else
   {
      len = 0;
   }
   return len;
}

/**
 * Find a key, specified by path and return a number of token in tokens array
 */
static int findTokenByParentAndName(const char *pJSON, jsmntok_t * pTok, int par_token, char * path, int numOfTokens)
{
   int result =-1;
   int token = 0;
   int childEl = 0;
   int arrayEl;

   if ((*path == '.') || (*path == '[')) //Assume what we searh key of object
   {
      for (token = par_token+1; token<= numOfTokens; token++)
      {
         if ((pTok+token)->parent == par_token) //Перебираем токены, которые являются дочерними к par_token
         {
            result = token;
            break;
         }

      }
   }
   else if (ISDIGIT(*path)) //Assume what we searh element of array
   {
      if ((pTok+par_token)->type == JSMN_ARRAY) //Check parrent type
      {
         arrayEl = jsmn_atoi(path);
         if (arrayEl < (pTok+par_token)->size)
         {
            for (token = par_token+1; token < numOfTokens && childEl <= arrayEl; token++)
            {
               if ((pTok+token)->parent == par_token) //Перебираем токены, которые являются дочерними к par_token
               {
                  if (childEl == arrayEl)
                  {
                     result=token;
                     break;
                  }
                  childEl++;
               }
            }
         }
      }
   }
   else if (ISALNUM(*path))
   {

      for (token = par_token+1; token < numOfTokens && childEl < (pTok+par_token)->size; token++)
      {
         if ((pTok+token)->parent == par_token) //Перебираем токены, которые являются дочерними к par_token
         {
            if (lenOFNode(path) == (pTok+token)->end-((pTok+token)->start)) //Быстрое сравнение длины
            {
               if (jsmn_strncmp((pJSON+(pTok+token)->start), path, ((pTok+token)->end-((pTok+token)->start))) == 0)
               {
                  result=token;
                  break;
               }
            }
            childEl++;
         }

      }
   }
   else if (*path == '$')
   {
      result = -1;
   }

   return result;
}


int jsmn_get_value(const char *js, jsmntok_t *tokens, unsigned int num_tokens,  char * pPath)
{
   char *path = pPath;
   int token =-1;
   if (*path == '$')
   {
      path = nextNode(path);
      if (path)
      {
         token = findTokenByParentAndName(js, tokens, token, path, num_tokens); //Must be 0
      }
      path = nextNode(path);
      while (*path && token>=0 )
      {

         token = findTokenByParentAndName(js, tokens, token, path, num_tokens);
         path = nextNode(path);

      }
      if (token>=0)
      {
         jsmntok_t * parent = tokens+((tokens +token)->parent);
         if (parent->type == JSMN_OBJECT)
         {
            token = findTokenByParentAndName(js, tokens, token, ".", num_tokens);
         }
      }
   }
   else
   {
      token = -2;
   }
   return token;

}

int jsmn_get_string(const char *js, jsmntok_t *tokens, unsigned int num_tokens,  char * pPath, char * string, int maxlen)
{
   int tokNum;
   int length =0;
   tokNum = jsmn_get_value(js, tokens, num_tokens, pPath);

   if (tokNum>0)
   {

      length = tokens[tokNum].end - tokens[tokNum].start;
      if (length < maxlen)
      {
         memcpy(string, &js[tokens[tokNum].start], length);
         string[length] = '\0';
      }
      else
      {
         length = 0;
      }
   }

   return length;
}
int jsmn_get_bool(const char *js, jsmntok_t *tokens, unsigned int num_tokens,  char * pPath)
{
   int result = FALSE;
   int tokNum;

   tokNum = jsmn_get_value(js, tokens, num_tokens, pPath);

   if (tokNum > 0)
   {
      if (strncmp (&js[tokens[tokNum].start], "true" ,4) == 0)
      {
         result = TRUE;
      }
   }
   return result;
}
#endif
