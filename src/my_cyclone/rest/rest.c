#include "rest.h"
#include "clock.h"
#include "sntpd.h"
#include "sensors.h"

char restPrefix[]="/rest";

typedef struct{
   char* string;
   char* startAt;
   char* endAt;
   size_t lenght;
}
token_t;
//Mutex that protects critical sections
OsMutex restMutex;

register_rest_function(test_rest, "/test", NULL, NULL, NULL, NULL, NULL, NULL);


char restBuffer[4096];
static int printfRestClassMethods (char * bufer, int maxLen, restFunctions * cur_rest)
{
   int p=0;
   int flag=0;
   p+=snprintf(bufer+p, maxLen-p, "{\r\n\"path\": \"%s/v1%s\",\r\n\"method\" : [", &restPrefix[0], cur_rest->restClassPath);
   if (cur_rest->restGetClassHadler != NULL)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"GET\",");
      flag++;
   }
   if (cur_rest->restPostClassHadler != NULL)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"POST\",");
      flag++;
   }
   if (cur_rest->restPutClassHadler != NULL)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"PUT\",");
      flag++;

   }
   if (cur_rest->restDeleteClassHadler != NULL)
   {
      p+=snprintf(bufer+p, maxLen-p, "\"DELETE\",");
      flag++;
   }
   if (flag>0)
   {
      p--;
   }

   p+=snprintf(bufer+p, maxLen-p, "]\r\n}");
   return p;
}

static int printfRestClass (char * bufer, int maxLen)
{
   int p=0;
   p+=snprintf(bufer+p, maxLen-p, "{\"class\":[\r\n");
   for (restFunctions *cur_rest = &__start_rest_functions; cur_rest < &__stop_rest_functions; cur_rest++)
   {
      p+=printfRestClassMethods(bufer+p, maxLen - p, cur_rest);
      if ( cur_rest != &__stop_rest_functions-1)
      {
         p+=snprintf(bufer+p, maxLen-p, ",\r\n");
      }
   }
   p+=snprintf(bufer+p, maxLen-p, "\r\n]\r\n}\r\n");
   return p;
}

error_t restTry (HttpConnection *connection, const char_t *uri)
{
   int p=0;
   const size_t max_len = sizeof(restBuffer);

   RestApi_t restAPI;
   volatile error_t error;

   error = restParsePath(connection, &restAPI);
   //Not implemented
   switch (error)
   {
   case NO_ERROR:
      break;
   case ERROR_UNEXPECTED_MESSAGE:
      return ERROR_NOT_FOUND;
   case ERROR_INVALID_CLASS:
      p = printfRestClass(restBuffer+p, max_len);
      p+=snprintf(restBuffer+p, max_len-p, "\r\n");
      rest_404_not_found(connection, &restBuffer[0]);
      return NO_ERROR;
      break;
   default:
      p = printfRestClass(restBuffer+p, max_len);
      p+=snprintf(restBuffer+p, max_len-p, "\r\n");
      rest_404_not_found(connection, &restBuffer[0]);
      return NO_ERROR;
   }

   error = findRestHandler(&restAPI);
   if (error == ERROR_NOT_FOUND)
   {
      printfRestClass(&restBuffer[0], max_len);
      rest_404_not_found(connection, &restBuffer[0]);
      return error;
   }


   //Enter critical section
   osAcquireMutex(&restMutex);
   switch (restAPI.method)
   {
   case METHOD_GET:
      if (restAPI.restClassHadlers->restGetClassHadler)
      {
         error = restAPI.restClassHadlers->restGetClassHadler(connection, &restAPI);
      }
      else
      {  //Если нет обработчика метода GET печатаем все доступные методы
         p = printfRestClassMethods(restBuffer+p, max_len, restAPI.restClassHadlers);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
      break;
   case METHOD_POST:
      if (restAPI.restClassHadlers->restPostClassHadler)
      {
         error = restAPI.restClassHadlers->restPostClassHadler(connection, &restAPI);
      }
      else
      {  //Если нет обработчика метода POST печатаем все доступные методы
         p = printfRestClassMethods(restBuffer+p, max_len, restAPI.restClassHadlers);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
      break;
   case METHOD_PUT:
      if (restAPI.restClassHadlers->restPutClassHadler)
      {
         error = restAPI.restClassHadlers->restPutClassHadler(connection, &restAPI);
      }
      else
      {  //Если нет обработчика метода PUT печатаем все доступные методы
         p = printfRestClassMethods(restBuffer+p, max_len, restAPI.restClassHadlers);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
      break;
   case METHOD_DELETE:
      if (restAPI.restClassHadlers->restDeleteClassHadler)
      {
         error = restAPI.restClassHadlers->restDeleteClassHadler(connection, &restAPI);
      }
      else
      {  //Если нет обработчика метода DELETE печатаем все доступные методы
         p = printfRestClassMethods(restBuffer+p, max_len, restAPI.restClassHadlers);
         p+=snprintf(restBuffer+p, max_len-p, "\r\n");
         rest_501_not_implemented(connection, &restBuffer[0]);
         error = ERROR_NOT_IMPLEMENTED;
      }
      break;

   default:
      p = printfRestClassMethods
      (restBuffer+p, max_len, restAPI.restClassHadlers);
      p+=snprintf(restBuffer+p, max_len-p, "\r\n");
      rest_501_not_implemented(connection, &restBuffer[0]);
      error = ERROR_NOT_IMPLEMENTED;
   }

   //   if (error == ERROR_NOT_FOUND)
   //   {
   //      error = rest_404_not_found(connection, "404 Not Found.\r\n");
   //   }
   //
   //   else if (error == ERROR_NOT_IMPLEMENTED)
   //   {
   //      error = rest_501_not_implemented(connection, "501 Not Implemented.\r\n");
   //   }
   //Exit critical section
   osReleaseMutex(&restMutex);

   return error;
}
error_t restInit(void)
{
   error_t error;
   if(!osCreateMutex(&restMutex))
   {
      //Failed to create mutex
      return ERROR_OUT_OF_RESOURCES;
   }
   for (restFunctions *cur_rest = &__start_rest_functions; cur_rest < &__stop_rest_functions; cur_rest++)
   {
      if (cur_rest->restInitClassHadler!=NULL)
      {
         error = cur_rest->restInitClassHadler();
         if (error)
         {
            return error;
         }
      }
   }
   return NO_ERROR;
}


error_t rest_answer(HttpConnection *connection, char * message, uint_t error_code)
{
   error_t error;
   size_t p, n;
   if (*message)
      connection->response.contentLength = strlen(message);

   p = connection->response.contentLength;

   connection->response.statusCode = error_code;
   connection->response.chunkedEncoding = FALSE;
   connection->response.noCache = TRUE;

   error = httpWriteHeader(connection);
   if (error)
   {
      return error;
   }

   if (connection->response.contentLength)
   {
      error = httpWriteStream(connection, message, connection->response.contentLength);
   }

   if (error)
   {
      return error;
   }

   while(p > 0)
   {
      //Limit the number of bytes to read at a time
      n = MIN(p, HTTP_SERVER_BUFFER_SIZE);

      //Send data to the client
      error = httpWriteStream(connection, connection->buffer, n);
      //Any error to report?
      if(error) break;

      //Decrement the count of remaining bytes to be transferred
      p -= n;
   }
   if (error)
   {
      return error;
   }
   error = httpCloseStream(connection);
   return error;
}


static error_t restFindToken (token_t *token, char firstChar, char lastChar)
{
   error_t error;
   token->startAt = token->string;
   token->endAt=token->string;
   token->lenght = 0;

   if (strlen(token->string))
   {
      token->startAt=strchr(token->string, firstChar);
      token->endAt=strchr(token->startAt+1, lastChar);
      if ((token->startAt == NULL) || (token->endAt == NULL))
      {
         error = ERROR_UNEXPECTED_MESSAGE;
         return error;
      }
      else
      {
         token->lenght = (token->endAt)-(token->startAt);
         return NO_ERROR;
      }
   }
   else
   {
      //String is too short
      error = ERROR_UNEXPECTED_MESSAGE;
      return error;
   }
}
/**
 * @brief Parse URI and fill RestApi fields
 * @param[in] HttpConnection Pointer connection context
 * @return Error code NO_ERROR, ERROR_UNEXPECTED_MESSAGE, ERROR_INVALID_VERSION, ERROR_INVALID_CLASS
 **/
error_t restParsePath(HttpConnection *connection, RestApi_t *RestApi)
{
   char *_uri = connection->request.uri;
   error_t error=NO_ERROR;
   size_t leftChars;
   token_t token;
   uint_t i;

   RestApi->restVersion = 0;
   RestApi->class = NULL;
   RestApi->className = NULL;
   RestApi->objectId = NULL;
   RestApi->classLen = 0;
   RestApi->classNameLen = 0;
   RestApi->objectIdLen = 0;
   RestApi->restClassHadlers = NULL;
   RestApi->method = 0;

   for(i = 0; i < arraysize(methodCodeList); i++)
   {
      if (!strcmp(methodCodeList[i].message, connection->request.method))
      {
         RestApi->method = methodCodeList[i].method;
         break;
      }
   }
   if (!RestApi->method)
   {
      error = ERROR_UNEXPECTED_MESSAGE;
      return error;
   }
   //
   if (!_uri)
      error = ERROR_UNEXPECTED_MESSAGE;
   if (error)
      return error;

   token.string = _uri;

   leftChars =strlen(_uri);

   error = restFindToken(&token, '/', '/');
   if (error != NO_ERROR)
   {
      // Maybe it is an last token, which can be ended by '?' or 0x00
      error = restFindToken(&token, '/', '?');
      if (error != NO_ERROR)
      {
         error = restFindToken(&token, '/', 0x00);
      }
   }

   if (error != NO_ERROR)
      return error;

   if (token.lenght ==strlen(restPrefix))
   {
      if (strncmp(_uri, restPrefix, token.lenght) ==  0)
      {
         error = NO_ERROR;
      }
      else
      {
         error = ERROR_UNEXPECTED_MESSAGE;
      }
   }
   else
   {
      error = ERROR_UNEXPECTED_MESSAGE;
   }
   if (error)
      return error;
   token.string = token.endAt;

   leftChars-=token.lenght;

   if (leftChars<=1)
   {
      error = ERROR_INVALID_VERSION;
      return error;
   }
   error = restFindToken(&token, '/', '/');
   if (error != NO_ERROR)
   {
      // Maybe it is an last token, which can be ended by '?' or 0x00
      error = restFindToken(&token, '/', '?');
      if (error != NO_ERROR)
      {
         error = restFindToken(&token, '/', 0x00);
      }
   }

   if (error != NO_ERROR)
      return error;

   if(token.lenght < 3) // должен быть как минимум "/v1"
   {
      error = ERROR_INVALID_VERSION;
   }
   else
   {
      if (*(token.startAt+1) != 'v')
         error = ERROR_INVALID_VERSION;
      else
      {
         RestApi->restVersion=atoi(token.startAt+2);
         if (RestApi->restVersion)
            error = NO_ERROR;
         else
            error = ERROR_INVALID_VERSION;
      }
   }

   if (error != NO_ERROR)
      return error;

   token.string = token.endAt;

   leftChars-=token.lenght;
   if (leftChars<=1)
   {
      error = ERROR_INVALID_CLASS;
      return error;
   }


   error = restFindToken(&token, '/', '/');
   if (error != NO_ERROR)
   {
      // Maybe it is an last token, which can be ended by '?' or 0x00
      error = restFindToken(&token, '/', '?');
      if (error != NO_ERROR)
      {
         error = restFindToken(&token, '/', 0x00);
      }
   }


   if (error != NO_ERROR)
      return error;

   if (token.lenght > 1) //Must be longer than just "/"
   {
      RestApi->class = token.startAt;
      RestApi->classLen = token.lenght;

   }
   else
      error = ERROR_INVALID_CLASS;
   if (error != NO_ERROR)
      return error;


   //Next fields are optional - no error code, if it is an empty
   token.string = token.endAt;
   leftChars-=token.lenght;
   if (leftChars<=1)
   { //Nothing to parse anymore
      error = NO_ERROR;
      return error;
   }

   error = restFindToken(&token, '/', '/');
   if (error != NO_ERROR)
   {
      // Maybe it is an last token, which can be ended by '?' or 0x00
      error = restFindToken(&token, '/', '?');
      if (error != NO_ERROR)
      {
         error = restFindToken(&token, '/', 0x00);
      }
   }

   if (token.lenght > 1)  //Must be longer than just "/"
   {
      RestApi->className = token.startAt;
      RestApi->classNameLen = token.lenght;
   }
   else
   {
      error = NO_ERROR; //NO error if not
      return error;
   }

   token.string = token.endAt;
   leftChars-=token.lenght;
   if (leftChars<=1)
   { //Nothing to parse anymore
      error = NO_ERROR;
      return error;
   }

   error = restFindToken(&token, '/', '/');
   if (error != NO_ERROR)
   {
      // Maybe it is an last token, which can be ended by '?' or 0x00
      error = restFindToken(&token, '/', '?');
      if (error != NO_ERROR)
      {
         error = restFindToken(&token, '/', 0x00);
      }
   }

   if (token.lenght > 1)  //Must be longer than just "/"
   {
      RestApi->objectId = token.startAt;
      RestApi->objectIdLen = token.lenght;
   }
   else
   {
      error = NO_ERROR; //NO error if not
      return error;
   }

   return error;
}

error_t findRestHandler(RestApi_t* RestApi)
{
   volatile error_t error = NO_ERROR;
   for (restFunctions *cur_rest = &__start_rest_functions; cur_rest < &__stop_rest_functions; cur_rest++)
   {
      if (strncmp(RestApi->class, cur_rest->restClassPath, RestApi->classLen ) == 0)
      {
         RestApi->restClassHadlers = cur_rest;
         return error;
      }
   }
   return ERROR_NOT_FOUND;
}
