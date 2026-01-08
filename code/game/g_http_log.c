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

#include "g_local.h"
#include "g_http_log.h"

#define HTTP_LOG_BUFFER_SIZE 4096
#define HTTP_LOG_URL_SIZE 512
#define HTTP_LOG_MAX_RETRIES 3

// Configuration
static char g_httpLogURL[HTTP_LOG_URL_SIZE] = "";
static qboolean g_httpLogEnabled = qfalse;

/*
=================
G_HTTPLog_EscapeJSON

Escape string for JSON output
=================
*/
static void G_HTTPLog_EscapeJSON(const char *src, char *dst, int dstSize)
{
    int srcPos = 0;
    int dstPos = 0;

    while (src[srcPos] && dstPos < dstSize - 2)
    {
        char c = src[srcPos];

        switch (c)
        {
        case '"':
            if (dstPos + 2 < dstSize)
            {
                dst[dstPos++] = '\\';
                dst[dstPos++] = '"';
            }
            break;
        case '\\':
            if (dstPos + 2 < dstSize)
            {
                dst[dstPos++] = '\\';
                dst[dstPos++] = '\\';
            }
            break;
        case '\n':
            if (dstPos + 2 < dstSize)
            {
                dst[dstPos++] = '\\';
                dst[dstPos++] = 'n';
            }
            break;
        case '\r':
            if (dstPos + 2 < dstSize)
            {
                dst[dstPos++] = '\\';
                dst[dstPos++] = 'r';
            }
            break;
        case '\t':
            if (dstPos + 2 < dstSize)
            {
                dst[dstPos++] = '\\';
                dst[dstPos++] = 't';
            }
            break;
        default:
            if (c >= 0x20 && c <= 0x7E)
            {
                dst[dstPos++] = c;
            }
            else if (dstPos + 6 < dstSize)
            {
                // Escape non-printable characters as \uXXXX
                Com_sprintf(&dst[dstPos], dstSize - dstPos, "\\u%04x", (unsigned char)c);
                dstPos += 6;
            }
            break;
        }

        srcPos++;
    }

    dst[dstPos] = '\0';
}

/*
=================
G_HTTPLog_BuildBaseJSON

Build the base JSON structure with common fields
=================
*/
static void G_HTTPLog_BuildBaseJSON(char *buffer, int bufferSize, httpLogEventType_t eventType)
{
    int gameTime = level.time;
    int min = gameTime / 60000;
    int sec = (gameTime % 60000) / 1000;

    const char *eventTypeStr = "UNKNOWN";

    switch (eventType)
    {
    case HTTP_LOG_KILL:
        eventTypeStr = "KILL";
        break;
    case HTTP_LOG_ITEM:
        eventTypeStr = "ITEM";
        break;
    case HTTP_LOG_CHAT:
        eventTypeStr = "CHAT";
        break;
    case HTTP_LOG_GAME_START:
        eventTypeStr = "GAME_START";
        break;
    case HTTP_LOG_GAME_END:
        eventTypeStr = "GAME_END";
        break;
    case HTTP_LOG_PLAYER_CONNECT:
        eventTypeStr = "PLAYER_CONNECT";
        break;
    case HTTP_LOG_PLAYER_DISCONNECT:
        eventTypeStr = "PLAYER_DISCONNECT";
        break;
    case HTTP_LOG_TEAM_CAPTURE:
        eventTypeStr = "TEAM_CAPTURE";
        break;
    case HTTP_LOG_ERROR:
        eventTypeStr = "ERROR";
        break;
    }

    Com_sprintf(buffer, bufferSize,
                "{"
                "\"timestamp\":%d,"
                "\"event_type\":\"%s\","
                "\"game_time\":\"%02d:%02d\","
                "\"level_name\":\"%s\","
                "\"game_type\":%d",
                level.time,
                eventTypeStr,
                min,
                sec,
                level.mapname,
                g_gametype.integer);
}

/*
=================
G_HTTPLog_SendHTTPRequest

Send the JSON data to the remote server via HTTP POST
=================
*/
static void G_HTTPLog_SendHTTPRequest(const char *jsonData)
{
    // This is a placeholder for actual HTTP sending functionality.
    // In a real implementation, this would use libcurl or similar library
    // to send HTTP POST requests. For now, we'll log to file as well.

    if (!jsonData || !jsonData[0])
    {
        return;
    }

    // Log to game console if in debug mode
    if (g_debugAlloc.integer)
    {
        G_Printf("HTTP Log: %s\n", jsonData);
    }

    // Log to file if enabled
    if (level.logFile)
    {
        char logEntry[HTTP_LOG_BUFFER_SIZE];
        Com_sprintf(logEntry, sizeof(logEntry), "[HTTP] %s\n", jsonData);
        trap_FS_Write(logEntry, strlen(logEntry), level.logFile);
    }

    // TODO: Integrate with libcurl to send actual HTTP POST request
    // Example code for libcurl (would require linking with libcurl):
    /*
    CURL *curl = curl_easy_init();
    if ( curl ) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append( headers, "Content-Type: application/json" );

        curl_easy_setopt( curl, CURLOPT_URL, g_httpLogURL );
        curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDS, jsonData );
        curl_easy_setopt( curl, CURLOPT_TIMEOUT, 5L );

        CURLcode res = curl_easy_perform( curl );
        if ( res != CURLE_OK ) {
            G_Printf( "HTTP Log Error: %s\n", curl_easy_strerror( res ) );
        }

        curl_slist_free_all( headers );
        curl_easy_cleanup( curl );
    }
    */
}

/*
=================
G_InitHTTPLogging

Initialize the HTTP logging system with configuration
=================
*/
void G_InitHTTPLogging(void)
{
    char *httpLogURL;

    // Get the HTTP logging URL from cvar (if available)
    httpLogURL = trap_Cvar_VariableStringBuffer("g_httpLogURL", g_httpLogURL, sizeof(g_httpLogURL));

    if (httpLogURL && httpLogURL[0])
    {
        Q_strncpyz(g_httpLogURL, httpLogURL, sizeof(g_httpLogURL));
        g_httpLogEnabled = qtrue;

        if (g_dedicated.integer)
        {
            G_Printf("HTTP Logging enabled: %s\n", g_httpLogURL);
        }
    }
    else
    {
        g_httpLogEnabled = qfalse;
        if (g_dedicated.integer)
        {
            G_Printf("HTTP Logging disabled (g_httpLogURL not set)\n");
        }
    }
}

/*
=================
G_ShutdownHTTPLogging

Shutdown the HTTP logging system
=================
*/
void G_ShutdownHTTPLogging(void)
{
    g_httpLogEnabled = qfalse;
}

/*
=================
G_HTTPLog_SendEvent

Generic function to send an event with custom JSON data
=================
*/
void G_HTTPLog_SendEvent(httpLogEventType_t eventType, const char *jsonData)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), eventType);

    if (jsonData && jsonData[0])
    {
        Q_strcat(buffer, sizeof(buffer), ",");
        Q_strcat(buffer, sizeof(buffer), jsonData);
    }

    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}

/*
=================
G_HTTPLog_Kill

Log a kill event
=================
*/
void G_HTTPLog_Kill(int killerNum, int victimNum, int meansOfDeath, const char *killerName, const char *victimName)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];
    char escapedKillerName[256];
    char escapedVictimName[256];
    char eventData[1024];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_EscapeJSON(killerName ? killerName : "UNKNOWN", escapedKillerName, sizeof(escapedKillerName));
    G_HTTPLog_EscapeJSON(victimName ? victimName : "UNKNOWN", escapedVictimName, sizeof(escapedVictimName));

    Com_sprintf(eventData, sizeof(eventData),
                "\"killer_num\":%d,"
                "\"killer_name\":\"%s\","
                "\"victim_num\":%d,"
                "\"victim_name\":\"%s\","
                "\"means_of_death\":%d",
                killerNum,
                escapedKillerName,
                victimNum,
                escapedVictimName,
                meansOfDeath);

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), HTTP_LOG_KILL);
    Q_strcat(buffer, sizeof(buffer), ",");
    Q_strcat(buffer, sizeof(buffer), eventData);
    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}

/*
=================
G_HTTPLog_Item

Log an item pickup event
=================
*/
void G_HTTPLog_Item(int clientNum, const char *clientName, const char *itemName)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];
    char escapedClientName[256];
    char escapedItemName[256];
    char eventData[1024];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_EscapeJSON(clientName ? clientName : "UNKNOWN", escapedClientName, sizeof(escapedClientName));
    G_HTTPLog_EscapeJSON(itemName ? itemName : "UNKNOWN", escapedItemName, sizeof(escapedItemName));

    Com_sprintf(eventData, sizeof(eventData),
                "\"client_num\":%d,"
                "\"client_name\":\"%s\","
                "\"item_name\":\"%s\"",
                clientNum,
                escapedClientName,
                escapedItemName);

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), HTTP_LOG_ITEM);
    Q_strcat(buffer, sizeof(buffer), ",");
    Q_strcat(buffer, sizeof(buffer), eventData);
    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}

/*
=================
G_HTTPLog_Chat

Log a chat message event
=================
*/
void G_HTTPLog_Chat(int clientNum, const char *clientName, const char *message, qboolean isTeamChat)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];
    char escapedClientName[256];
    char escapedMessage[2048];
    char eventData[2560];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_EscapeJSON(clientName ? clientName : "UNKNOWN", escapedClientName, sizeof(escapedClientName));
    G_HTTPLog_EscapeJSON(message ? message : "", escapedMessage, sizeof(escapedMessage));

    Com_sprintf(eventData, sizeof(eventData),
                "\"client_num\":%d,"
                "\"client_name\":\"%s\","
                "\"message\":\"%s\","
                "\"is_team_chat\":%s",
                clientNum,
                escapedClientName,
                escapedMessage,
                isTeamChat ? "true" : "false");

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), HTTP_LOG_CHAT);
    Q_strcat(buffer, sizeof(buffer), ",");
    Q_strcat(buffer, sizeof(buffer), eventData);
    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}

/*
=================
G_HTTPLog_GameStart

Log a game start event
=================
*/
void G_HTTPLog_GameStart(void)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), HTTP_LOG_GAME_START);
    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}

/*
=================
G_HTTPLog_GameEnd

Log a game end event
=================
*/
void G_HTTPLog_GameEnd(void)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), HTTP_LOG_GAME_END);
    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}

/*
=================
G_HTTPLog_PlayerConnect

Log a player connection event
=================
*/
void G_HTTPLog_PlayerConnect(int clientNum, const char *clientName, const char *ipAddress)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];
    char escapedClientName[256];
    char escapedIPAddress[256];
    char eventData[512];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_EscapeJSON(clientName ? clientName : "UNKNOWN", escapedClientName, sizeof(escapedClientName));
    G_HTTPLog_EscapeJSON(ipAddress ? ipAddress : "UNKNOWN", escapedIPAddress, sizeof(escapedIPAddress));

    Com_sprintf(eventData, sizeof(eventData),
                "\"client_num\":%d,"
                "\"client_name\":\"%s\","
                "\"ip_address\":\"%s\"",
                clientNum,
                escapedClientName,
                escapedIPAddress);

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), HTTP_LOG_PLAYER_CONNECT);
    Q_strcat(buffer, sizeof(buffer), ",");
    Q_strcat(buffer, sizeof(buffer), eventData);
    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}

/*
=================
G_HTTPLog_PlayerDisconnect

Log a player disconnection event
=================
*/
void G_HTTPLog_PlayerDisconnect(int clientNum, const char *clientName)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];
    char escapedClientName[256];
    char eventData[256];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_EscapeJSON(clientName ? clientName : "UNKNOWN", escapedClientName, sizeof(escapedClientName));

    Com_sprintf(eventData, sizeof(eventData),
                "\"client_num\":%d,"
                "\"client_name\":\"%s\"",
                clientNum,
                escapedClientName);

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), HTTP_LOG_PLAYER_DISCONNECT);
    Q_strcat(buffer, sizeof(buffer), ",");
    Q_strcat(buffer, sizeof(buffer), eventData);
    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}

/*
=================
G_HTTPLog_TeamCapture

Log a team flag capture event (CTF)
=================
*/
void G_HTTPLog_TeamCapture(int clientNum, const char *clientName, const char *teamName)
{
    char buffer[HTTP_LOG_BUFFER_SIZE];
    char escapedClientName[256];
    char escapedTeamName[256];
    char eventData[512];

    if (!g_httpLogEnabled)
    {
        return;
    }

    G_HTTPLog_EscapeJSON(clientName ? clientName : "UNKNOWN", escapedClientName, sizeof(escapedClientName));
    G_HTTPLog_EscapeJSON(teamName ? teamName : "UNKNOWN", escapedTeamName, sizeof(escapedTeamName));

    Com_sprintf(eventData, sizeof(eventData),
                "\"client_num\":%d,"
                "\"client_name\":\"%s\","
                "\"team_name\":\"%s\"",
                clientNum,
                escapedClientName,
                escapedTeamName);

    G_HTTPLog_BuildBaseJSON(buffer, sizeof(buffer), HTTP_LOG_TEAM_CAPTURE);
    Q_strcat(buffer, sizeof(buffer), ",");
    Q_strcat(buffer, sizeof(buffer), eventData);
    Q_strcat(buffer, sizeof(buffer), "}");

    G_HTTPLog_SendHTTPRequest(buffer);
}
