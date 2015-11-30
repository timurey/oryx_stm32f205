/**
 * @file web_socket_misc.h
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

#ifndef _WEB_SOCKET_MISC_H
#define _WEB_SOCKET_MISC_H

//Dependencies
#include "core/net.h"
#include "web_socket/web_socket.h"

//WebSocket related functions
void webSocketChangeState(WebSocket *webSocket, WebSocketState newState);

error_t webSocketInitTls(WebSocket *webSocket);

error_t webSocketSendData(WebSocket *webSocket, const void *data,
   size_t length, size_t *written, uint_t flags);

error_t webSocketReceiveData(WebSocket *webSocket, void *data,
   size_t size, size_t *received, uint_t flags);

error_t webSocketParseHandshake(WebSocket *webSocket);
error_t webSocketParseRequestLine(WebSocket *webSocket, char_t *line);
error_t webSocketParseStatusLine(WebSocket *webSocket, char_t *line);
error_t webSocketParseHeaderField(WebSocket *webSocket, char_t *line);

error_t webSocketFormatClientHandshake(WebSocket *webSocket);
error_t webSocketFormatServerHandshake(WebSocket *webSocket);

error_t webSocketVerifyClientHandshake(WebSocket *webSocket);
error_t webSocketVerifyServerHandshake(WebSocket *webSocket);

error_t webSocketFormatFrameHeader(WebSocket *webSocket,
   WebSocketFrameType type, size_t payloadLen);

error_t webSocketParseFrameHeader(WebSocket *webSocket,
   const WebSocketFrame *frame, WebSocketFrameType *type);

error_t webSocketFormatCloseFrame(WebSocket *webSocket);

bool_t webSocketCheckStatusCode(uint16_t statusCode);

bool_t webSocketCheckUtf8Stream(WebSocketUtf8Context *context,
   const uint8_t *data, size_t length, size_t remaining);

#endif
