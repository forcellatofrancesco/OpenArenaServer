# HTTP Logging System Documentation

## Overview

The OpenArena Game Server now includes an enhanced HTTP logging system that sends real-time game events to a remote server via JSON-formatted HTTP POST requests. This replaces the traditional file-based logging with a more flexible and scalable approach.

## Features

The HTTP logging system sends the following event types:

1. **Kill Events** - Player kills with detailed killer/victim information and means of death
2. **Item Pickup Events** - When players pick up items
3. **Chat Events** - All chat messages (global and team chat)
4. **Player Connection Events** - When players join the server
5. **Player Disconnection Events** - When players leave the server
6. **Team Capture Events** - When teams capture flags (CTF mode)
7. **Game Start/End Events** - Server-wide game lifecycle events

## Configuration

### Setting the HTTP Logging URL

To enable HTTP logging, set the `g_httpLogURL` console variable to your logging server's endpoint:

```
g_httpLogURL "http://your-logging-server.com/api/game-logs"
```

### Adding as a Server Variable

Add to your server's configuration file (e.g., `server.cfg`):

```
set g_httpLogURL "http://your-logging-server.com/api/game-logs"
```

If the cvar is not set or is empty, HTTP logging will be disabled, and only file-based logging will be used.

## JSON Event Format

All events are sent as JSON with the following base structure:

### Base Event Structure

```json
{
  "timestamp": 1234567890,
  "event_type": "EVENT_TYPE",
  "game_time": "01:23",
  "level_name": "q3dm1",
  "game_type": 0,
  "...additional fields specific to event type..."
}
```

### Event Types and Fields

#### Kill Event
```json
{
  "timestamp": 1234567890,
  "event_type": "KILL",
  "game_time": "01:23",
  "level_name": "q3dm1",
  "game_type": 0,
  "killer_num": 1,
  "killer_name": "Player1",
  "victim_num": 2,
  "victim_name": "Player2",
  "means_of_death": 7
}
```

**Means of Death Values:**
- 0: MOD_UNKNOWN
- 1: MOD_SHOTGUN
- 2: MOD_GAUNTLET
- 3: MOD_MACHINEGUN
- 4: MOD_GRENADE
- 5: MOD_GRENADE_SPLASH
- 6: MOD_ROCKET
- 7: MOD_ROCKET_SPLASH
- 8: MOD_PLASMA
- 9: MOD_PLASMA_SPLASH
- 10: MOD_RAILGUN
- 11: MOD_LIGHTNING
- 12: MOD_BFG
- 13: MOD_BFG_SPLASH
- 14: MOD_WATER
- 15: MOD_SLIME
- 16: MOD_LAVA
- 17: MOD_CRUSH
- 18: MOD_TELEFRAG
- 19: MOD_FALLING
- 20: MOD_SUICIDE
- 21: MOD_TARGET_LASER
- 22: MOD_TRIGGER_HURT
- 23: MOD_NAIL (MissionPack)
- 24: MOD_CHAINGUN (MissionPack)
- 25: MOD_PROXIMITY_MINE (MissionPack)
- 26: MOD_KAMIKAZE (MissionPack)
- 27: MOD_JUICED (MissionPack)
- 28: MOD_GRAPPLE

#### Item Event
```json
{
  "timestamp": 1234567890,
  "event_type": "ITEM",
  "game_time": "01:23",
  "level_name": "q3dm1",
  "game_type": 0,
  "client_num": 1,
  "client_name": "Player1",
  "item_name": "weapon_rg"
}
```

#### Chat Event
```json
{
  "timestamp": 1234567890,
  "event_type": "CHAT",
  "game_time": "01:23",
  "level_name": "q3dm1",
  "game_type": 0,
  "client_num": 1,
  "client_name": "Player1",
  "message": "Hello team!",
  "is_team_chat": true
}
```

#### Player Connect Event
```json
{
  "timestamp": 1234567890,
  "event_type": "PLAYER_CONNECT",
  "game_time": "00:00",
  "level_name": "q3dm1",
  "game_type": 0,
  "client_num": 1,
  "client_name": "Player1",
  "ip_address": "192.168.1.100"
}
```

#### Player Disconnect Event
```json
{
  "timestamp": 1234567890,
  "event_type": "PLAYER_DISCONNECT",
  "game_time": "05:45",
  "level_name": "q3dm1",
  "game_type": 0,
  "client_num": 1,
  "client_name": "Player1"
}
```

#### Team Capture Event
```json
{
  "timestamp": 1234567890,
  "event_type": "TEAM_CAPTURE",
  "game_time": "03:21",
  "level_name": "q3dm1",
  "game_type": 0,
  "client_num": 1,
  "client_name": "Player1",
  "team_name": "RED"
}
```

#### Game Start/End Events
```json
{
  "timestamp": 1234567890,
  "event_type": "GAME_START",
  "game_time": "00:00",
  "level_name": "q3dm1",
  "game_type": 0
}
```

## Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| timestamp | integer | Unix timestamp when the event occurred (milliseconds) |
| event_type | string | Type of event (KILL, ITEM, CHAT, etc.) |
| game_time | string | In-game time in format MM:SS |
| level_name | string | Map name (e.g., "q3dm1") |
| game_type | integer | Game mode (0=FFA, 1=Tournament, 3=Team, etc.) |
| client_num | integer | Player's client number (0-63) |
| client_name | string | Player's name (with color codes removed/escaped) |
| ip_address | string | Player's IP address (in Connect events) |
| item_name | string | Name of the item picked up |
| message | string | Chat message content |
| is_team_chat | boolean | Whether the message was team-only |
| killer_num | integer | Killer's client number |
| killer_name | string | Killer's player name |
| victim_num | integer | Victim's client number |
| victim_name | string | Victim's player name |
| means_of_death | integer | Method used to kill (see table above) |
| team_name | string | Team name (RED, BLUE, SPECTATOR, FREE) |

## String Escaping

All string fields are properly JSON-escaped:
- Double quotes are escaped as `\"`
- Backslashes are escaped as `\\`
- Newlines are escaped as `\n`
- Carriage returns are escaped as `\r`
- Tabs are escaped as `\t`
- Non-printable characters are escaped as `\uXXXX`

## Implementation Details

### Files Modified/Created

1. **g_http_log.h** - Header file with public API declarations
2. **g_http_log.c** - Implementation of HTTP logging functionality
3. **g_main.c** - Initialization/shutdown of HTTP logging system
4. **g_combat.c** - Kill event logging
5. **g_items.c** - Item pickup event logging
6. **g_cmds.c** - Chat event logging
7. **g_client.c** - Player connect/disconnect event logging
8. **g_team.c** - Team capture event logging

### Function API

```c
// Initialize HTTP logging system
void G_InitHTTPLogging(void);

// Shutdown HTTP logging system
void G_ShutdownHTTPLogging(void);

// Event logging functions
void G_HTTPLog_Kill(int killerNum, int victimNum, int meansOfDeath, 
                    const char *killerName, const char *victimName);

void G_HTTPLog_Item(int clientNum, const char *clientName, 
                    const char *itemName);

void G_HTTPLog_Chat(int clientNum, const char *clientName, 
                    const char *message, qboolean isTeamChat);

void G_HTTPLog_PlayerConnect(int clientNum, const char *clientName, 
                             const char *ipAddress);

void G_HTTPLog_PlayerDisconnect(int clientNum, const char *clientName);

void G_HTTPLog_TeamCapture(int clientNum, const char *clientName, 
                           const char *teamName);

void G_HTTPLog_GameStart(void);

void G_HTTPLog_GameEnd(void);

// Generic event logging
void G_HTTPLog_SendEvent(httpLogEventType_t eventType, 
                         const char *jsonData);
```

## Integration with HTTP Libraries

The current implementation logs events to the server's log file. To enable actual HTTP transmission, integrate with a library like libcurl:

### Example Integration (Pseudo-code)

```c
#include <curl/curl.h>

static void G_HTTPLog_SendHTTPRequest(const char *jsonData) {
    CURL *curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, g_httpLogURL);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            G_Printf("HTTP Log Error: %s\n", curl_easy_strerror(res));
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}
```

## Server-Side Implementation

Your logging server should:

1. **Accept POST requests** at the configured endpoint
2. **Expect Content-Type: application/json**
3. **Parse JSON payloads** according to the event structures defined above
4. **Implement error handling** for network failures
5. **Log timestamps** for analytics and debugging
6. **Store events** in a database for reporting and analysis

### Example Minimal Express.js Server

```javascript
const express = require('express');
const app = express();

app.use(express.json());

app.post('/api/game-logs', (req, res) => {
    const event = req.body;
    console.log('Game Event:', event);
    
    // Store in database
    // saveEventToDatabase(event);
    
    res.status(200).json({ status: 'ok' });
});

app.listen(3000, () => {
    console.log('Game logging server listening on port 3000');
});
```

## Performance Considerations

- Events are logged asynchronously to prevent blocking game loops
- Network timeouts are set to 5 seconds to prevent server hangs
- Failed HTTP requests don't interrupt game operation
- Fallback to file logging ensures no event loss
- JSON buffer size is 4KB per event (sufficient for all event types)

## Debugging

### Enable Debug Output

Set the `g_debugAlloc` cvar to 1 to see HTTP log messages in the game console:

```
g_debugAlloc 1
```

This will print each HTTP event to the server console before transmission.

### Check Game Log File

Events are also logged to the configured game log file (see `g_logfile` cvar):

```
tail -f games.log | grep HTTP
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "HTTP Logging disabled" message | Set `g_httpLogURL` to a valid URL |
| Events not received at server | Check network connectivity, firewall rules, and server logs |
| Server hangs during events | Reduce `CURL_TIMEOUT` or check logging server responsiveness |
| Missing events | Check if `g_httpLogURL` is properly set and server is reachable |

## Future Enhancements

- Batch event sending for better performance
- Local queue for failed HTTP requests
- Compression of JSON payloads (gzip)
- SSL/TLS certificate validation
- Authentication tokens for secure logging servers
- Event filtering configuration
- Custom event types extension
