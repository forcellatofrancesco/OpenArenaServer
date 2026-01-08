/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef G_HTTP_LOG_H
#define G_HTTP_LOG_H

// HTTP logging module for sending game events to a remote server via JSON

typedef enum {
	HTTP_LOG_KILL,
	HTTP_LOG_ITEM,
	HTTP_LOG_CHAT,
	HTTP_LOG_GAME_START,
	HTTP_LOG_GAME_END,
	HTTP_LOG_PLAYER_CONNECT,
	HTTP_LOG_PLAYER_DISCONNECT,
	HTTP_LOG_TEAM_CAPTURE,
	HTTP_LOG_ERROR
} httpLogEventType_t;

// Initialize HTTP logging system
void G_InitHTTPLogging( void );

// Shutdown HTTP logging system
void G_ShutdownHTTPLogging( void );

// Send a kill event to the remote server
void G_HTTPLog_Kill( int killerNum, int victimNum, int meansOfDeath, const char *killerName, const char *victimName );

// Send an item pickup event
void G_HTTPLog_Item( int clientNum, const char *clientName, const char *itemName );

// Send a chat message event
void G_HTTPLog_Chat( int clientNum, const char *clientName, const char *message, qboolean isTeamChat );

// Send a game start event
void G_HTTPLog_GameStart( void );

// Send a game end event
void G_HTTPLog_GameEnd( void );

// Send a player connect event
void G_HTTPLog_PlayerConnect( int clientNum, const char *clientName, const char *ipAddress );

// Send a player disconnect event
void G_HTTPLog_PlayerDisconnect( int clientNum, const char *clientName );

// Send a team capture event (CTF)
void G_HTTPLog_TeamCapture( int clientNum, const char *clientName, const char *teamName );

// Generic HTTP log function with custom event data
void G_HTTPLog_SendEvent( httpLogEventType_t eventType, const char *jsonData );

#endif // G_HTTP_LOG_H
