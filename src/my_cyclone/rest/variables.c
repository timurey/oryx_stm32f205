/*
 * variables.c
 *
 *  Created on: 05 февр. 2016 г.
 *      Author: timurtaipov
 */


#include "variables.h"
#include <math.h>

register_rest_function(variables, "/variables", &restInitVariables, NULL, &restGetVariable, &restPostVariable, &restPutVariable, &restDeleteVariable);

variables pVariables[NUM_VARIABLES];

#define ISDIGIT(a) (((a)>='0') && ((a)<='9'))


error_t restInitVariables(void)
{
   int i;
   error_t error = NO_ERROR;
   for (i=0; i< NUM_VARIABLES; i++)
   {
      if(!osCreateMutex(&(pVariables[i].mutex)))
      {
         //Failed to create mutex
         xprintf("Sensors: Can't create variables#%d mutex.\r\n", i);
         error= ERROR_OUT_OF_RESOURCES;
      }
      pVariables[i].name=NULL;
      pVariables[i].value=0;
   }
   return error;
}

static variables * findFreeVariable (void)
{
   int i;
   variables * pVariable = NULL;

   for (i=0; i< NUM_VARIABLES; i++)
   {
      if (pVariables[i].name == NULL)
      {
         pVariable = &pVariables[i];
         //Leave
         break;
      }
   }
   return pVariable;
}

static variables * findVariable (char * name)
{
   int i;
   variables * pVariable = NULL;

   for (i=0; i< NUM_VARIABLES; i++)
   {
      if (strcmp(pVariables[i].name, name) == 0)
      {
         pVariable = &pVariables[i];
         //Leave
         break;
      }
   }

   return pVariable;
}

error_t getVariable(char *name, double * value)
{
   error_t error = NO_ERROR;
   variables * pVariable;

   pVariable = findVariable(name);

   if (pVariable != NULL)
   {
      //Enter critical section
      osAcquireMutex(&pVariable->mutex);

      *value = pVariable->value;

      //Leave critical section
      osReleaseMutex(&pVariable->mutex);
   }
   else
   {
      error = ERROR_OBJECT_NOT_FOUND;
   }
   return error;
}

/*
 * Create variable with name. Value is not required
 */
/*
 * todo: may be better to return * pVariable
 */
error_t createVariable(char *name, double value)
{
   error_t error = NO_ERROR;
   variables * pVariable;
   size_t len = strlen(name);

   pVariable = findVariable(name);

   if (pVariable == NULL)
   {
      pVariable = findFreeVariable();

      if (pVariable != NULL)
      {
         //Enter critical section
         osAcquireMutex(&pVariable->mutex);

         //allocate system memory for variable name
         pVariable->name = osAllocMem(len+1);

         if (pVariable->name ==NULL)
         {
            error = ERROR_OUT_OF_RESOURCES;
         }
         //No error
         if (!error)
         {
            //must be faster than strcpy
            memcpy(pVariable->name, name,  len);
            pVariable->name[len]='\0';

            //If value is set
            pVariable->value = value;
         }
         //Leave critical section
         osReleaseMutex(&pVariable->mutex);
      }
      else
      {
         //Sorry, we have't free variables for you
         error = ERROR_OUT_OF_RESOURCES;
      }
   }
   else
   {
      //Sorry, variable already exist
      error = ERROR_FAILURE;
   }
   return error;
}

error_t setVariable(char *name, double value)
{
   error_t error = NO_ERROR;
   variables * pVariable;

   pVariable = findVariable(name);

   if (pVariable != NULL)
   {
      //Enter critical section
      osAcquireMutex(&pVariable->mutex);

      pVariable->value = value;

      //Leave critical section
      osReleaseMutex(&pVariable->mutex);
   }
   else
   {
      error = ERROR_OBJECT_NOT_FOUND;
   }
   return error;
}

error_t deleteVariable(char *name)
{
   error_t error = NO_ERROR;
   variables * pVariable = NULL;

   pVariable = findVariable(name);

   if (pVariable != NULL)
   {
      //Enter critical section
      osAcquireMutex(&pVariable->mutex);

      pVariable->value = 0;
      osFreeMem(pVariable->name);

      pVariable->name = NULL;

      //Leave critical section
      osReleaseMutex(&pVariable->mutex);
   }
   return error;
}

static size_t snprintfVariable(char * pChar, size_t maxSize, variables * pVariable)
{
   size_t len;
   int d1, d2;
   double f2;

   //Enter critical section
   osAcquireMutex(&pVariable->mutex);
   d1 = pVariable->value;            // Get the integer part (678).
   f2 = pVariable->value-d1;     // Get fractional part (678.0123 - 678 = 0.0123).
   d2 = abs(trunc(f2 * 10));   // Turn into integer (123).

   len = snprintf(pChar, maxSize, "{name:\"%s\",value:\"%d.%01d\"}",pVariable->name, d1, d2);
   //Leave critical section
   osReleaseMutex(&pVariable->mutex);
   return len;
}

error_t restGetVariable(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = ERROR_NOT_FOUND;
   int p=0;
   int i=0,j=0;
   const size_t max_len = sizeof(restBuffer);

   p=sprintf(restBuffer,"{\"variable\":[ ");
   if (RestApi->objectId != NULL)
   {
      if (ISDIGIT(*(RestApi->objectId+1)))
      {
         j=atoi(RestApi->objectId+1);
         if (j<NUM_VARIABLES)
         {
            if (pVariables[j].name!=NULL)
            {
               p+=snprintfVariable(&restBuffer[p], max_len-p, &pVariables[j]);
               error = NO_ERROR;
            }
         }
      }
      else
      {
         error = ERROR_UNSUPPORTED_REQUEST;
      }

   }
   else
   {
      for (i=0; i< NUM_VARIABLES; i++)
      {
         if (pVariables[i].name!=NULL)
         {
            p+=snprintfVariable(&restBuffer[p], max_len-p, &pVariables[i]);
            p+=snprintf(&restBuffer[p], max_len-p, ",");
            error = NO_ERROR;
         }

      }
   }
   p--;
   createVariable("a1",0);
   p+=snprintf(restBuffer+p, max_len-p,"]}\r\n");
   connection->response.contentType = mimeGetType(".json");
   switch (error)
   {
   case NO_ERROR:
      return rest_200_ok(connection, &restBuffer[0]);
      break;
   case ERROR_UNSUPPORTED_REQUEST:
      return rest_400_bad_request(connection, "400 Bad Request.\r\n");
      break;
   case ERROR_NOT_FOUND:
   default:
      return rest_404_not_found(connection, "404 Not Found.\r\n");
      break;

   }

}

error_t restPostVariable(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;

   return error;

}

error_t restPutVariable(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;

   return error;

}

error_t restDeleteVariable(HttpConnection *connection, RestApi_t* RestApi)
{
   error_t error = NO_ERROR;

   return error;

}



