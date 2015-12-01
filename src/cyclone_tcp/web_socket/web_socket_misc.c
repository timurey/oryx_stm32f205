/**
 * @file web_socket_misc.c
 * @brief Helper functions for WebSockets
 *
 * @section License
 *
 * Copyright (C) 2010-2015 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 1.6.5
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL WEB_SOCKET_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include "core/net.h"
#include "web_socket/web_socket.h"
#include "web_socket/web_socket_auth.h"
#include "web_socket/web_socket_misc.h"
#include "base64.h"
#include "sha1.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (WEB_SOCKET_SUPPORT == ENABLED)

//WebSocket GUID
const char_t webSocketGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


/**
 * @brief Update WebSocket state
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] newState New state to switch to
 **/

void webSocketChangeState(WebSocket *webSocket, WebSocketState newState)
{
   //Switch to the new state
   webSocket->state = newState;
   //Save current time;
   webSocket->timestamp = osGetSystemTime();

   //Reset sub-state
   webSocket->txContext.state = WS_SUB_STATE_INIT;
   webSocket->rxContext.state = WS_SUB_STATE_INIT;
}


/**
 * @brief Send data (low-level function)
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 **/

error_t webSocketSendData(WebSocket *webSocket, const void *data,
   size_t length, size_t *written, uint_t flags)
{
   error_t error;

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(webSocket->tlsContext != NULL)
   {
      //Use SSL/TLS to transmit data to the client
      error = tlsWrite(webSocket->tlsContext, data, length, flags);
      //Number of bytes that have been written
      *written = error ? 0 : length;
   }
   else
#endif
   {
      //Transmit data
      error = socketSend(webSocket->socket, data, length, written, flags);
   }

   //Return status code
   return error;
}


/**
 * @brief Receive data (low-level function)
 * @param[in] webSocket Handle to a WebSocket
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t webSocketReceiveData(WebSocket *webSocket, void *data,
   size_t size, size_t *received, uint_t flags)
{
   error_t error;

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(webSocket->tlsContext != NULL)
   {
      //Use SSL/TLS to receive data from the client
      error = tlsRead(webSocket->tlsContext, data, size, received, flags);
   }
   else
#endif
   {
      //Receive data
      error = socketReceive(webSocket->socket, data, size, received, flags);
   }

   //Return status code
   return error;
}


/**
 * @brief SSL/TLS initialization
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketInitTls(WebSocket *webSocket)
{
#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   error_t error;
   TlsConnectionEnd connectionEnd;

   //Debug message
   TRACE_INFO("Initializing SSL/TLS session...\r\n");

   //Allocate SSL/TLS context
   webSocket->tlsContext = tlsInit();
   //Initialization failed?
   if(webSocket->tlsContext == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Start of exception handling block
   do
   {
      //WebSocket client or server?
      if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
         connectionEnd = TLS_CONNECTION_END_CLIENT;
      else
         connectionEnd = TLS_CONNECTION_END_SERVER;

      //Select the relevant operation mode
      error = tlsSetConnectionEnd(webSocket->tlsContext, connectionEnd);
      //Any error to report?
      if(error)
         break;

      //Bind TLS to the relevant socket
      error = tlsSetSocket(webSocket->tlsContext, webSocket->socket);
      //Any error to report?
      if(error)
         break;

      //Invoke user-defined callback, if any
      if(webSocket->tlsInitCallback != NULL)
      {
         //Perform SSL/TLS related initialization
         error = webSocket->tlsInitCallback(webSocket, webSocket->tlsContext);
         //Any error to report?
         if(error)
            break;
      }

      //End of exception handling block
   } while(0);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      tlsFree(webSocket->tlsContext);
   }

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Parse client or server handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketParseHandshake(WebSocket *webSocket)
{
   error_t error;
   size_t n;
   WebSocketFrameContext *rxContext;

   //Point to the RX context
   rxContext = &webSocket->rxContext;

   //Initialize status code
   error = NO_ERROR;

   //Wait for the handshake to complete
   while(webSocket->state != WS_STATE_OPEN)
   {
      //Check current sub-state
      if(rxContext->state == WS_SUB_STATE_INIT)
      {
         //Initialize status code
         webSocket->statusCode = WS_STATUS_CODE_NO_STATUS_RCVD;

         //Initialize FIN flag
         rxContext->fin = TRUE;

         //Flush the receive buffer
         rxContext->bufferPos = 0;
         rxContext->bufferLen = 0;

         //Initialize variables
         webSocket->handshakeContext.statusCode = 0;
         webSocket->handshakeContext.upgradeWebSocket = FALSE;
         webSocket->handshakeContext.connectionUpgrade = FALSE;
         webSocket->handshakeContext.closingFrameSent = FALSE;
         webSocket->handshakeContext.closingFrameReceived = FALSE;

#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
         webSocket->authContext.requiredAuthMode = WS_AUTH_MODE_NONE;
#endif

#if (WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
         strcpy(webSocket->authContext.nonce, "");
         strcpy(webSocket->authContext.opaque, "");
         webSocket->authContext.stale = FALSE;
#endif

         //Client or server operation?
         if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
            strcpy(webSocket->handshakeContext.serverKey, "");
         else
            strcpy(webSocket->handshakeContext.clientKey, "");

         //Debug message
         TRACE_DEBUG("WebSocket: server handshake\r\n");

         //Decode the leading line
         rxContext->state = WS_SUB_STATE_HANDSHAKE_LEADING_LINE;
      }
      else if(rxContext->state == WS_SUB_STATE_HANDSHAKE_LEADING_LINE)
      {
         //Check whether more data is required
         if(rxContext->bufferLen < 2)
         {
            //Limit the number of characters to read at a time
            n = WEB_SOCKET_BUFFER_SIZE - 1 - rxContext->bufferLen;

            //Read data until a CLRF character is encountered
            error = webSocketReceiveData(webSocket, rxContext->buffer +
               rxContext->bufferLen, n, &n, SOCKET_FLAG_BREAK_CRLF);

            //Update the length of the buffer
            rxContext->bufferLen += n;
         }
         else if(rxContext->bufferLen >= (WEB_SOCKET_BUFFER_SIZE - 1))
         {
            //Report an error
            error = ERROR_INVALID_REQUEST;
         }
         else if(strncmp((char_t *) rxContext->buffer + rxContext->bufferLen - 2, "\r\n", 2))
         {
            //Limit the number of characters to read at a time
            n = WEB_SOCKET_BUFFER_SIZE - 1 - rxContext->bufferLen;

            //Read data until a CLRF character is encountered
            error = webSocketReceiveData(webSocket, rxContext->buffer +
               rxContext->bufferLen, n, &n, SOCKET_FLAG_BREAK_CRLF);

            //Update the length of the buffer
            rxContext->bufferLen += n;
         }
         else
         {
            //Properly terminate the string with a NULL character
            rxContext->buffer[rxContext->bufferLen] = '\0';

            //Client or server operation?
            if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
            {
               //The leading line from the server follows the Status-Line format
               error = webSocketParseStatusLine(webSocket, (char_t *) rxContext->buffer);
            }
            else
            {
               //The leading line from the client follows the Request-Line format
               error = webSocketParseRequestLine(webSocket, (char_t *) rxContext->buffer);
            }

            //Flush the receive buffer
            rxContext->bufferPos = 0;
            rxContext->bufferLen = 0;

            //Parse the header fields of the handshake
            rxContext->state = WS_SUB_STATE_HANDSHAKE_HEADER_FIELD;
         }
      }
      else if(rxContext->state == WS_SUB_STATE_HANDSHAKE_HEADER_FIELD)
      {
         //Check whether more data is required
         if(rxContext->bufferLen < 2)
         {
            //Limit the number of characters to read at a time
            n = WEB_SOCKET_BUFFER_SIZE - 1 - rxContext->bufferLen;

            //Read data until a CLRF character is encountered
            error = webSocketReceiveData(webSocket, rxContext->buffer +
               rxContext->bufferLen, n, &n, SOCKET_FLAG_BREAK_CRLF);

            //Update the length of the buffer
            rxContext->bufferLen += n;
         }
         else if(rxContext->bufferLen >= (WEB_SOCKET_BUFFER_SIZE - 1))
         {
            //Report an error
            error = ERROR_INVALID_REQUEST;
         }
         else if(strncmp((char_t *) rxContext->buffer + rxContext->bufferLen - 2, "\r\n", 2))
         {
            //Limit the number of characters to read at a time
            n = WEB_SOCKET_BUFFER_SIZE - 1 - rxContext->bufferLen;

            //Read data until a CLRF character is encountered
            error = webSocketReceiveData(webSocket, rxContext->buffer +
               rxContext->bufferLen, n, &n, SOCKET_FLAG_BREAK_CRLF);

            //Update the length of the buffer
            rxContext->bufferLen += n;
         }
         else
         {
            //Properly terminate the string with a NULL character
            rxContext->buffer[rxContext->bufferLen] = '\0';

            //An empty line indicates the end of the header fields
            if(!strcmp((char_t *) rxContext->buffer, "\r\n"))
            {
               //Client or server operation?
               if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
               {
                  //Verify server's handshake
                  error = webSocketVerifyServerHandshake(webSocket);
               }
               else
               {
                  //Verify client's handshake
                  error = webSocketVerifyClientHandshake(webSocket);
               }
            }
            else
            {
               //Read the next character to detect if the CRLF is immediately
               //followed by a LWSP character
               rxContext->state = WS_SUB_STATE_HANDSHAKE_LWSP;
            }
         }
      }
      else if(rxContext->state == WS_SUB_STATE_HANDSHAKE_LWSP)
      {
         char_t nextChar;

         //Read the next character
         error = webSocketReceiveData(webSocket, &nextChar, sizeof(char_t), &n, 0);

         //Successful read operation?
         if(!error && n == sizeof(char_t))
         {
            //LWSP character found?
            if(nextChar == ' ' || nextChar == '\t')
            {
               //Unfolding is accomplished by regarding CRLF immediately
               //followed by a LWSP as equivalent to the LWSP character
               if(rxContext->bufferLen >= 2)
               {
                  //Remove trailing CRLF sequence
                  rxContext->bufferLen -= 2;
               }

               //The header field spans multiple line
               rxContext->state = WS_SUB_STATE_HANDSHAKE_HEADER_FIELD;
            }
            else
            {
               //Parse header field
               error = webSocketParseHeaderField(webSocket, (char_t *) rxContext->buffer);

               //Restore the very first character of the header field
               rxContext->buffer[0] = nextChar;
               //Adjust the length of the receive buffer
               rxContext->bufferLen = sizeof(char_t);

               //Decode the next header field
               rxContext->state = WS_SUB_STATE_HANDSHAKE_HEADER_FIELD;
            }
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }

      //Any error to report?
      if(error)
         break;
   }

   //Return status code
   return error;
}


/**
 * @brief Parse the Request-Line of the client's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] line NULL-terminated string that contains the Request-Line
 * @return Error code
 **/

error_t webSocketParseRequestLine(WebSocket *webSocket, char_t *line)
{
   //Debug message
   TRACE_DEBUG("%s", line);

   return NO_ERROR;
}


/**
 * @brief Parse the Status-Line of the server's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] line NULL-terminated string that contains the Status-Line
 * @return Error code
 **/

error_t webSocketParseStatusLine(WebSocket *webSocket, char_t *line)
{
   char_t *p;
   char_t *token;

   //Debug message
   TRACE_DEBUG("%s", line);

   //Retrieve the HTTP-Version field
   token = strtok_r(line, " ", &p);
   //Any parsing error?
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //Retrieve the Status-Code field
   token = strtok_r(NULL, " ", &p);
   //Any parsing error?
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //Convert the status code
   webSocket->handshakeContext.statusCode = strtoul(token, &p, 10);
   //Any parsing error?
   if(*p != '\0')
      return ERROR_INVALID_SYNTAX;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse a header field
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] line NULL-terminated string that contains the header field
 * @return Error code
 **/

error_t webSocketParseHeaderField(WebSocket *webSocket, char_t *line)
{
   char_t *separator;
   char_t *name;
   char_t *value;
   WebSocketHandshakeContext *handshakeContext;

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //Debug message
   TRACE_DEBUG("%s", line);

   //Check whether a separator is present
   separator = strchr(line, ':');

   //Separator found?
   if(separator != NULL)
   {
      //Split the line
      *separator = '\0';

      //Get field name and value
      name = strTrimWhitespace(line);
      value = strTrimWhitespace(separator + 1);

      //Upgrade header field found?
      if(!strcasecmp(name, "Upgrade"))
      {
         if(!strcasecmp(value, "websocket"))
            handshakeContext->upgradeWebSocket = TRUE;

      }
      //Connection header field found?
      else if(!strcasecmp(name, "Connection"))
      {
         if(!strcasecmp(value, "Upgrade"))
            handshakeContext->connectionUpgrade = TRUE;
      }
      //Sec-WebSocket-Accept header field found?
      else if(!strcasecmp(name, "Sec-WebSocket-Accept"))
      {
         //Save the contents of the Sec-WebSocket-Accept header field
         strSafeCopy(handshakeContext->serverKey,
            value, WEB_SOCKET_SERVER_KEY_SIZE + 1);
      }
#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
      //WWW-Authenticate header field found?
      else if(!strcasecmp(name, "WWW-Authenticate"))
      {
         //Parse WWW-Authenticate header field
         webSocketParseAuthenticateField(webSocket, value);
      }
#endif
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format client's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketFormatClientHandshake(WebSocket *webSocket)
{
   char_t *p;
   WebSocketFrameContext *txContext;

   //Point to the TX context
   txContext = &webSocket->txContext;
   //Point to the buffer where to format the client's handshake
   p = (char_t *) txContext->buffer;

   //The Request-Line begins with a method token, followed by the
   //Request-URI and the protocol version, and ending with CRLF
   p += sprintf(p, "GET %s HTTP/1.1\r\n", webSocket->uri);

   //Add Host header field
   if(webSocket->host[0] != '\0')
   {
      //The Host header field specifies the Internet host and port number of
      //the resource being requested
      p += sprintf(p, "Host: %s:%d\r\n", webSocket->host, webSocket->remotePort);
   }
   else
   {
      //If the requested URI does not include a host name for the service being
      //requested, then the Host header field must be given with an empty value
      p += sprintf(p, "Host:\r\n");
   }

#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
   //Check whether authorization is required
   if(webSocket->authContext.selectedAuthMode != WS_AUTH_MODE_NONE)
   {
      //Add Authorization header field
      p += webSocketAddAuthorizationField(webSocket, p);
   }
#endif

   //Add Origin header field
   if(webSocket->origin[0] != '\0')
      p += sprintf(p, "Origin: %s\r\n", webSocket->origin);
   else
      p += sprintf(p, "Origin: null\r\n");

   //Add Upgrade header field
   p += sprintf(p, "Upgrade: websocket\r\n");
   //Add Connection header field
   p += sprintf(p, "Connection: Upgrade\r\n");

   //Add Sec-WebSocket-Key header field
   p += sprintf(p, "Sec-WebSocket-Key: %s\r\n",
      webSocket->handshakeContext.clientKey);

   //Add Sec-WebSocket-Version header field
   p += sprintf(p, "Sec-WebSocket-Version: 13\r\n");
   //An empty line indicates the end of the header fields
   p += sprintf(p, "\r\n");

   //Debug message
   TRACE_DEBUG("\r\n");
   TRACE_DEBUG("WebSocket: client handshake\r\n");
   TRACE_DEBUG("%s", txContext->buffer);

   //Rewind to the beginning of the buffer
   txContext->bufferPos = 0;
   //Update the number of data buffered but not yet sent
   txContext->bufferLen = strlen((char_t *) txContext->buffer);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format server's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketFormatServerHandshake(WebSocket *webSocket)
{
   return ERROR_NOT_IMPLEMENTED;
}


/**
 * @brief Verify client's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketVerifyClientHandshake(WebSocket *webSocket)
{
   //webSocketChangeState(webSocket, WS_STATE_SERVER_HANDSHAKE);
   return ERROR_NOT_IMPLEMENTED;
}


/**
 * @brief Verify server's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketVerifyServerHandshake(WebSocket *webSocket)
{
   size_t n;
   WebSocketHandshakeContext *handshakeContext;
   Sha1Context sha1Context;

   //Debug message
   TRACE_DEBUG("WebSocket: verifying server handshake\r\n");

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //If the status code received from the server is not 101, the client
   //handles the response per HTTP procedures
   if(handshakeContext->statusCode == 401)
   {
      //Authorization required
      return ERROR_AUTH_REQUIRED;
   }
   else if(handshakeContext->statusCode != 101)
   {
      //Unknown status code
      return ERROR_INVALID_STATUS;
   }

   //If the response lacks an Upgrade header field or the Upgrade header field
   //contains a value that is not an ASCII case-insensitive match for the
   //value "websocket", the client must fail the WebSocket connection
   if(!handshakeContext->upgradeWebSocket)
      return ERROR_INVALID_SYNTAX;

   //If the response lacks a Connection header field or the Connection header
   //field doesn't contain a token that is an ASCII case-insensitive match for
   //the value "Upgrade", the client must fail the WebSocket connection
   if(!handshakeContext->connectionUpgrade)
      return ERROR_INVALID_SYNTAX;

   //If the response lacks a Sec-WebSocket-Accept header field, the client
   //must fail the WebSocket connection
   if(strlen(handshakeContext->serverKey) == 0)
      return ERROR_INVALID_SYNTAX;

   //Concatenate the Sec-WebSocket-Key with the GUID string and digest the
   //resulting string using SHA-1
   sha1Init(&sha1Context);
   sha1Update(&sha1Context, handshakeContext->clientKey, strlen(handshakeContext->clientKey));
   sha1Update(&sha1Context, webSocketGuid, strlen(webSocketGuid));
   sha1Final(&sha1Context, NULL);

   //Encode the result using Base64
   base64Encode(sha1Context.digest, SHA1_DIGEST_SIZE,
      (char_t *) webSocket->txContext.buffer, &n);

   //Debug message
   TRACE_DEBUG("  Client key: %s\r\n", handshakeContext->clientKey);
   TRACE_DEBUG("  Server key: %s\r\n", handshakeContext->serverKey);
   TRACE_DEBUG("  Calculated key: %s\r\n", webSocket->txContext.buffer);

   //Check the Sec-WebSocket-Accept header field
   if(strcmp(handshakeContext->serverKey, (char_t *) webSocket->txContext.buffer))
      return ERROR_INVALID_KEY;

   //If the server's response is validated as provided for above, it is
   //said that the WebSocket Connection is Established and that the
   //WebSocket connection is in the OPEN state
   webSocketChangeState(webSocket, WS_STATE_OPEN);

   //Successful processing
   return NO_ERROR;
}

/**
 * @brief Format WebSocket frame header
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] type Frame type
 * @param[in] payloadLen Length of the payload data
 * @return Error code
 **/

error_t webSocketFormatFrameHeader(WebSocket *webSocket,
   WebSocketFrameType type, size_t payloadLen)
{
   error_t error;
   WebSocketFrameContext *txContext;
   WebSocketFrame *frame;

   //Point to the TX context
   txContext = &webSocket->txContext;

   //Flush the transmit buffer
   txContext->bufferPos = 0;
   txContext->bufferLen = 0;

   //The endpoint must encapsulate the data in a WebSocket frame
   frame = (WebSocketFrame *) txContext->buffer;

   //The frame needs to be formatted according to the WebSocket framing
   //format
   frame->fin = TRUE;
   frame->reserved = 0;
   frame->opcode = type;

   //All frames sent from the client to the server are masked by a 32-bit
   //value that is contained within the frame
   if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
   {
      //All frames sent from client to server have the Mask bit set to 1
      frame->mask = TRUE;

      //Make sure that the RNG callback function has been registered
      if(webSockRandCallback != NULL)
      {
         //Generate a random masking key
         error = webSockRandCallback(txContext->maskingKey, sizeof(uint32_t));
         //Any error to report?
         if(error)
            return error;
      }
      else
      {
         //A cryptographically strong random number generator
         //must be used to generate the masking key
         return ERROR_PRNG_NOT_READY;
      }
   }
   else
   {
      //Clear the Mask bit
      frame->mask = FALSE;
   }

   //Size of the frame header
   txContext->bufferLen = sizeof(WebSocketFrame);

   //Compute the number of application data to be transmitted
   txContext->payloadLen = payloadLen;

   //Check the length of the payload
   if(payloadLen <= 125)
   {
      //Payload length
      frame->payloadLen = payloadLen;
   }
   else if(payloadLen <= 65535)
   {
      //If the Payload Length field is set to 126, then the following
      //2 bytes are interpreted as a 16-bit unsigned integer
      frame->payloadLen = 126;

      //Save the length of the payload data
      STORE16BE(payloadLen, frame->extPayloadLen);

      //Adjust the length of the frame header
      txContext->bufferLen += sizeof(uint16_t);
   }
   else
   {
      //If the Payload Length field is set to 127, then the following
      //8 bytes are interpreted as a 64-bit unsigned integer
      frame->payloadLen = 127;

      //Save the length of the payload data
      STORE64BE(payloadLen, frame->extPayloadLen);

      //Adjust the length of the frame header
      txContext->bufferLen += sizeof(uint64_t);
   }

   //Debug message
   TRACE_DEBUG("WebSocket: Sending frame\r\n");
   TRACE_DEBUG("  FIN = %u\r\n", frame->fin);
   TRACE_DEBUG("  Reserved = %u\r\n", frame->reserved);
   TRACE_DEBUG("  Opcode = %u\r\n", frame->opcode);
   TRACE_DEBUG("  Mask = %u\r\n", frame->mask);
   TRACE_DEBUG("  Payload Length = %u\r\n", txContext->payloadLen);

   //The Masking Key field is present the mask bit is set to 1
   if(frame->mask)
   {
      //Debug message
      TRACE_DEBUG_ARRAY("  Masking Key = ", txContext->maskingKey, sizeof(uint32_t));

      //Copy the masking key
      memcpy(txContext->buffer + txContext->bufferLen,
         txContext->maskingKey, sizeof(uint32_t));

      //Adjust the length of the frame header
      txContext->bufferLen += sizeof(uint32_t);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse WebSocket frame header
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] frame Pointer to the WebSocket frame header
 * @param[out] type Frame type
 * @return Error code
 **/

error_t webSocketParseFrameHeader(WebSocket *webSocket,
   const WebSocketFrame *frame, WebSocketFrameType *type)
{
   size_t k;
   size_t n;
   uint16_t statusCode;
   WebSocketFrameContext *rxContext;

   //Point to the RX context
   rxContext = &webSocket->rxContext;

   //Point to the Extended Payload Length
   n = sizeof(WebSocketFrame);

   //Check the Payload Length field
   if(frame->payloadLen == 126)
   {
      //If the Payload Length field is set to 126, then the following
      //2 bytes are interpreted as a 16-bit unsigned integer
      rxContext->payloadLen = LOAD16BE(frame->extPayloadLen);

      //Point to the next field
      n += sizeof(uint16_t);
   }
   else if(frame->payloadLen == 127)
   {
      //If the Payload Length field is set to 127, then the following
      //8 bytes are interpreted as a 64-bit unsigned integer
      rxContext->payloadLen = LOAD64BE(frame->extPayloadLen);

      //Point to the next field
      n += sizeof(uint64_t);
   }
   else
   {
      //Retrieve the length of the payload data
      rxContext->payloadLen = frame->payloadLen;
   }

   //Debug message
   TRACE_DEBUG("WebSocket: frame received...\r\n");
   TRACE_DEBUG("  FIN = %u\r\n", frame->fin);
   TRACE_DEBUG("  Reserved = %u\r\n", frame->reserved);
   TRACE_DEBUG("  Opcode = %u\r\n", frame->opcode);
   TRACE_DEBUG("  Mask = %u\r\n", frame->mask);
   TRACE_DEBUG("  Payload Length = %u\r\n", rxContext->payloadLen);

   //Check whether the payload data is masked
   if(frame->mask)
   {
      //Save the masking key
      memcpy(rxContext->maskingKey, (uint8_t *) frame + n, sizeof(uint32_t));
      //Debug message
      TRACE_DEBUG_ARRAY("  Masking Key = ", rxContext->maskingKey, sizeof(uint32_t));

      //Point to the payload data
      n += sizeof(uint32_t);
   }

   //Text or Close frame received?
   if(frame->opcode == WS_FRAME_TYPE_TEXT ||
      frame->opcode == WS_FRAME_TYPE_CLOSE)
   {
      //Reinitialize UTF-8 decoding context
      webSocket->utf8Context.utf8CharSize = 0;
      webSocket->utf8Context.utf8CharIndex = 0;
      webSocket->utf8Context.utf8CodePoint = 0;
   }

   //If the RSV field is a nonzero value and none of the negotiated extensions
   //defines the meaning of such a nonzero value, the receiving endpoint must
   //fail the WebSocket connection
   if(frame->reserved != 0)
   {
      //Report a protocol error
      webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
      //Terminate the WebSocket connection
      return ERROR_INVALID_FRAME;
   }

   //The Opcode field defines the interpretation of the payload data
   if(frame->opcode == WS_FRAME_TYPE_CONTINUATION)
   {
      //A Continuation frame cannot be the first frame of a fragmented message
      if(rxContext->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
   }
   else if(frame->opcode == WS_FRAME_TYPE_TEXT)
   {
      //The Opcode must be 0 in subsequent fragmented frames
      if(!rxContext->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      //Save the Opcode field
      rxContext->type = (WebSocketFrameType) frame->opcode;
   }
   else if(frame->opcode == WS_FRAME_TYPE_BINARY)
   {
      //The Opcode must be 0 in subsequent fragmented frames
      if(!rxContext->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      //Save the Opcode field
      rxContext->type = (WebSocketFrameType) frame->opcode;
   }
   else if(frame->opcode == WS_FRAME_TYPE_CLOSE)
   {
      //Check the length of the payload data
      if(rxContext->payloadLen == 0)
      {
         //The Close frame does not contain any body
         webSocket->statusCode = WS_STATUS_CODE_NORMAL_CLOSURE;
      }
      else if(rxContext->payloadLen == 1)
      {
         //Report a protocol error
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
      }
      else
      {
         //If there is a body, the first two bytes of the body must be
         //a 2-byte unsigned integer representing a status code
         statusCode = LOAD16BE((uint8_t *) frame + n);

         //Debug message
         TRACE_DEBUG("  Status Code = %u\r\n", statusCode);

         //When sending a Close frame in response, the endpoint typically
         //echos the status code it received
         if(webSocketCheckStatusCode(statusCode))
            webSocket->statusCode = statusCode;
         else
            webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

         //The body may contain UTF-8-encoded data
         if(rxContext->payloadLen > 2)
         {
            //Compute the length of the data
            k = rxContext->payloadLen - 2;

            //Invalid UTF-8 sequence?
            if(!webSocketCheckUtf8Stream(&webSocket->utf8Context,
               (uint8_t *) frame + n + 2, k, k))
            {
               //The received data is not consistent with the type of the message
               webSocket->statusCode = WS_STATUS_CODE_INVALID_PAYLOAD_DATA;
            }
         }
      }

      //A Close frame has been received
      webSocket->handshakeContext.closingFrameReceived = TRUE;
      //Exit immediately
      return ERROR_END_OF_STREAM;
   }
   else if(frame->opcode == WS_FRAME_TYPE_PING)
   {
      //Save the Opcode field
      rxContext->type = (WebSocketFrameType) frame->opcode;

      //Control frames must not be fragmented
      if(!frame->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      //All control frames must have a payload length of 125 bytes or less
      if(frame->payloadLen > 125)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
   }
   else if(frame->opcode == WS_FRAME_TYPE_PONG)
   {
      //Save the Opcode field
      rxContext->type = (WebSocketFrameType) frame->opcode;

      //Control frames must not be fragmented
      if(!frame->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      //All control frames must have a payload length of 125 bytes or less
      if(frame->payloadLen > 125)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
   }
   else
   {
      //If an unknown opcode is received, the receiving endpoint must fail
      //the WebSocket connection
      webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
   }

   //Save the FIN flag
   rxContext->fin = frame->fin;
   //Save the Mask flag
   rxContext->mask = frame->mask;

   //Return frame type
   if(type != NULL)
      *type = rxContext->type;

   //Check status code
   if(webSocket->statusCode == WS_STATUS_CODE_NO_STATUS_RCVD)
      return NO_ERROR;
   else if(webSocket->statusCode == WS_STATUS_CODE_NORMAL_CLOSURE)
      return ERROR_END_OF_STREAM;
   else if(webSocket->statusCode == WS_STATUS_CODE_PROTOCOL_ERROR)
      return ERROR_INVALID_FRAME;
   else
      return ERROR_FAILURE;
}


/**
 * @brief Format a Close frame
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketFormatCloseFrame(WebSocket *webSocket)
{
   error_t error;
   uint8_t *p;

   //Format Close frame
   error = webSocketFormatFrameHeader(webSocket,
      WS_FRAME_TYPE_CLOSE, sizeof(uint16_t));

   //Check status code
   if(!error)
   {
      //1005 is a reserved value and must not be set as a status code in
      //a Close control frame by an endpoint
      if(webSocket->statusCode == WS_STATUS_CODE_NO_STATUS_RCVD)
         webSocket->statusCode = WS_STATUS_CODE_NORMAL_CLOSURE;

      //Debug message
      TRACE_DEBUG("  Status Code = %u\r\n", webSocket->statusCode);

      //Point to end of the WebSocket frame header
      p = webSocket->txContext.buffer + webSocket->txContext.bufferLen;

      //Write status code
      p[0] = MSB(webSocket->statusCode);
      p[1] = LSB(webSocket->statusCode);

      //All frames sent from the client to the server are masked
      if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
      {
         //Apply masking
         p[0] ^= webSocket->txContext.maskingKey[0];
         p[1] ^= webSocket->txContext.maskingKey[1];
      }

      //Adjust the length of the frame
      webSocket->txContext.bufferLen += sizeof(uint16_t);
   }

   //Return status code
   return error;
}


/**
 * @brief Check whether a status code is valid
 * @param[in] statusCode Status code
 * @return The function returns TRUE is the specified status code is
 *   valid. Otherwise, FALSE is returned
 **/

bool_t webSocketCheckStatusCode(uint16_t statusCode)
{
   bool_t valid;

   //Check status code
   if(statusCode == WS_STATUS_CODE_NORMAL_CLOSURE ||
      statusCode == WS_STATUS_CODE_GOING_AWAY ||
      statusCode == WS_STATUS_CODE_PROTOCOL_ERROR ||
      statusCode == WS_STATUS_CODE_UNSUPPORTED_DATA ||
      statusCode == WS_STATUS_CODE_INVALID_PAYLOAD_DATA ||
      statusCode == WS_STATUS_CODE_POLICY_VIOLATION ||
      statusCode == WS_STATUS_CODE_MESSAGE_TOO_BIG ||
      statusCode == WS_STATUS_CODE_MANDATORY_EXT ||
      statusCode == WS_STATUS_CODE_INTERNAL_ERROR)
   {
      valid = TRUE;
   }
   else if(statusCode >= 3000)
   {
      valid = TRUE;
   }
   else
   {
      valid = FALSE;
   }

   //The function returns TRUE is the specified status code is valid
   return valid;
}


/**
 * @brief Check whether a an UTF-8 stream is valid
 * @param[in] context UTF-8 decoding context
 * @param[in] data Pointer to the chunk of data to be processed
 * @param[in] length Data chunk length
 * @param[in] remaining number of remaining bytes in the UTF-8 stream
 * @return The function returns TRUE is the specified UTF-8 stream is
 *   valid. Otherwise, FALSE is returned
 **/

bool_t webSocketCheckUtf8Stream(WebSocketUtf8Context *context,
   const uint8_t *data, size_t length, size_t remaining)
{
   size_t i;
   bool_t valid;

   //Initialize flag
   valid = TRUE;

   //Interpret the byte stream as UTF-8
   for(i = 0; i < length && valid; i++)
   {
      //Leading or continuation byte?
      if(context->utf8CharIndex == 0)
      {
         //7-bit code point?
         if((data[i] & 0x80) == 0x00)
         {
            //The code point consist of a single byte
            context->utf8CharSize = 1;
            //Decode the first byte of the sequence
            context->utf8CodePoint = data[i] & 0x7F;
         }
         //11-bit code point?
         else if((data[i] & 0xE0) == 0xC0)
         {
            //The code point consist of a 2 bytes
            context->utf8CharSize = 2;
            //Decode the first byte of the sequence
            context->utf8CodePoint = (data[i] & 0x1F) << 6;
         }
         //16-bit code point?
         else if((data[i] & 0xF0) == 0xE0)
         {
            //The code point consist of a 3 bytes
            context->utf8CharSize = 3;
            //Decode the first byte of the sequence
            context->utf8CodePoint = (data[i] & 0x0F) << 12;
         }
         //21-bit code point?
         else if((data[i] & 0xF8) == 0xF0)
         {
            //The code point consist of a 3 bytes
            context->utf8CharSize = 4;
            //Decode the first byte of the sequence
            context->utf8CodePoint = (data[i] & 0x07) << 18;
         }
         else
         {
            //The UTF-8 stream is not valid
            valid = FALSE;
         }

         //This test only applies to frames that are not fragmented
         if(length <= remaining)
         {
            //Make sure the UTF-8 stream is properly terminated
            if((i + context->utf8CharSize) > remaining)
            {
               //The UTF-8 stream is not valid
               valid = FALSE;
            }
         }

         //Decode the next byte of the sequence
         context->utf8CharIndex = context->utf8CharSize - 1;
      }
      else
      {
         //Continuation bytes all have 10 in the high-order position
         if((data[i] & 0xC0) == 0x80)
         {
            //Decode the multi-byte sequence
            context->utf8CharIndex--;
            //All continuation bytes contain exactly 6 bits from the code point
            context->utf8CodePoint |= (data[i] & 0x3F) << (context->utf8CharIndex * 6);

            //The correct encoding of a code point use only the minimum number
            //of bytes required to hold the significant bits of the code point
            if(context->utf8CharSize == 2)
            {
               //Overlong encoding is not supported
               if((context->utf8CodePoint & ~0x7F) == 0)
                  valid = FALSE;
            }
            if(context->utf8CharSize == 3 && context->utf8CharIndex < 2)
            {
               //Overlong encoding is not supported
               if((context->utf8CodePoint & ~0x7FF) == 0)
                  valid = FALSE;
            }
            if(context->utf8CharSize == 4 && context->utf8CharIndex < 3)
            {
               //Overlong encoding is not supported
               if((context->utf8CodePoint & ~0xFFFF) == 0)
                  valid = FALSE;
            }

            //According to the UTF-8 definition (RFC 3629) the high and low
            //surrogate halves used by UTF-16 (U+D800 through U+DFFF) are not
            //legal Unicode values, and their UTF-8 encoding should be treated
            //as an invalid byte sequence
            if(context->utf8CodePoint >= 0xD800 && context->utf8CodePoint < 0xE000)
               valid = FALSE;

            //Code points greater than U+10FFFF are not valid
            if(context->utf8CodePoint >= 0x110000)
               valid = FALSE;
         }
         else
         {
            //The start byte is not followed by enough continuation bytes
            valid = FALSE;
         }
      }
   }

   //The function returns TRUE is the specified UTF-8 stream is valid
   return valid;
}

#endif
