# HTTP Logging System - Quick Start Guide

## Overview

The OpenArena server now includes an HTTP logging system that sends game events to a remote server in JSON format. This guide will get you up and running in minutes.

## 5-Minute Setup

### Step 1: Start a Logging Server

Choose your preferred implementation from `HTTP_LOGGING_SERVER_EXAMPLES.md`. The simplest is the Node.js Express version:

```bash
# Create a new directory
mkdir game-logs
cd game-logs

# Create server.js (copy from HTTP_LOGGING_SERVER_EXAMPLES.md - Node.js Basic section)
nano server.js

# Install dependencies
npm install express

# Run the server
node server.js
```

The server should output:
```
Game logging server listening on port 3000
```

### Step 2: Configure OpenArena Server

Edit your server configuration file (e.g., `server.cfg`):

```cfg
set g_httpLogURL "http://your-ip-address:3000/api/game-logs"
```

Or use the console command during gameplay:
```
g_httpLogURL http://your-ip-address:3000/api/game-logs
```

### Step 3: Verify It Works

1. Start the OpenArena server with the HTTP logging URL set
2. Join the server and perform actions (kills, chat, pick up items)
3. Check the logging server output - you should see events being logged

Example output:
```
[2024-01-08T10:30:45.123Z] PLAYER_CONNECT: { ... }
[2024-01-08T10:30:47.456Z] KILL: { ... }
[2024-01-08T10:30:52.789Z] ITEM: { ... }
```

## Common Tasks

### Enable Debug Output

See all HTTP events being logged to the game console:

```
g_debugAlloc 1
```

### View Event Logs

If using the Node.js server, logs are saved to disk:
```bash
tail -f logs/game-logs-*.jsonl
```

### Change Logging Server URL

```
g_httpLogURL "http://new-server.com:3000/api/game-logs"
```

### Disable HTTP Logging

Either clear the cvar:
```
g_httpLogURL ""
```

Or don't set it at all. The system will automatically disable if the URL is empty.

### Test Connectivity

From the OpenArena server:
```bash
curl http://your-logging-server:3000/health
```

Should return:
```json
{"status":"ok"}
```

## Event Types Reference

### Quick Reference

| Event | When Triggered | Key Fields |
|-------|----------------|-----------|
| KILL | Player kills another player | killer_num, victim_num, means_of_death |
| ITEM | Player picks up an item | client_num, item_name |
| CHAT | Player sends a message | client_num, message, is_team_chat |
| PLAYER_CONNECT | Player joins server | client_num, ip_address |
| PLAYER_DISCONNECT | Player leaves server | client_num |
| TEAM_CAPTURE | Flag capture (CTF) | client_num, team_name |

### View Sample Events

All events follow this structure:
```json
{
  "timestamp": 1704700245123,          // Unix timestamp in milliseconds
  "event_type": "KILL",                 // Event type
  "game_time": "05:30",                 // In-game time (MM:SS)
  "level_name": "q3dm1",                // Map name
  "game_type": 0,                       // Game mode
  "...event_specific_fields..."         // Additional fields vary by type
}
```

See `HTTP_LOGGING_README.md` for complete schema documentation.

## Troubleshooting

### Server Shows "HTTP Logging disabled"

**Problem:** Console shows: `HTTP Logging disabled (g_httpLogURL not set)`

**Solution:** Set the URL in your config or console:
```
g_httpLogURL "http://localhost:3000/api/game-logs"
```

### Events Not Appearing at Logging Server

**Problem:** Server is running but not receiving events

**Troubleshooting Steps:**

1. **Check network connectivity:**
   ```bash
   # From OpenArena server machine
   ping your-logging-server
   curl http://your-logging-server:3000/health
   ```

2. **Verify URL is correct:**
   ```
   # In OpenArena console
   g_httpLogURL
   ```

3. **Check firewall:**
   ```bash
   # Make sure port 3000 is accessible
   netstat -an | grep 3000
   ```

4. **Enable debug mode:**
   ```
   g_debugAlloc 1
   ```
   Look for `[HTTP]` messages in console output

5. **Check OpenArena log file:**
   ```bash
   tail -f games.log | grep HTTP
   ```

### Server Hangs or Slow Performance

**Problem:** Game feels laggy after setting HTTP logging

**Solution:** Network timeout is set to 5 seconds. Check:
1. Logging server is responsive
2. Network latency isn't excessive
3. Server CPU usage isn't maxed out

### JSON Parsing Errors

**Problem:** Logging server can't parse events

**Solution:** Ensure your server handler expects `Content-Type: application/json` and properly decodes the JSON payload.

## Database Queries

### Example Queries (SQLite)

If using a database-backed logging server:

```sql
-- Top killers in the last 24 hours
SELECT killer_name, COUNT(*) as kills
FROM kills
WHERE timestamp > (strftime('%s','now')-86400)*1000
GROUP BY killer_name
ORDER BY kills DESC
LIMIT 10;

-- Most picked up items
SELECT item_name, COUNT(*) as count
FROM events
WHERE event_type = 'ITEM'
GROUP BY item_name
ORDER BY count DESC;

-- Active players
SELECT DISTINCT client_name
FROM events
WHERE event_type IN ('KILL', 'CHAT', 'ITEM')
ORDER BY timestamp DESC
LIMIT 20;
```

## Docker Quick Start

If you have Docker installed:

```bash
# Create docker-compose.yml (see HTTP_LOGGING_SERVER_EXAMPLES.md)
docker-compose up -d

# Logs are in ./logs directory
tail -f logs/game-logs-*.jsonl
```

## Integration with Analytics

### Store in Elasticsearch

```bash
# Forward logs to Elasticsearch for analysis
cat logs/game-logs-*.jsonl | \
  jq -R 'fromjson' | \
  jq '.timestamp = (.timestamp | todate)' | \
  curl -X POST http://localhost:9200/game-logs/_doc -H 'Content-Type: application/json' -d @-
```

### Create Grafana Dashboard

Use a database-backed server and connect Grafana to your database to create real-time dashboards.

### Slack Notifications

Add to your logging server to notify Slack on notable events:

```javascript
// Example: Notify on first blood
if (event.event_type === 'KILL' && kills_in_game === 1) {
    slack.send({
        text: `First blood! ${event.killer_name} killed ${event.victim_name}`
    });
}
```

## Advanced Configuration

### Multiple Logging Servers

Set up multiple servers for redundancy:

```javascript
// Modify g_http_log.c to support multiple URLs
const servers = [
    'http://primary-logger:3000/api/game-logs',
    'http://backup-logger:3000/api/game-logs'
];

// Send to all servers (requires code modification)
for (let url of servers) {
    sendToServer(url, json_data);
}
```

### Custom Events

Extend the logging system for custom game events:

1. Add event type to `httpLogEventType_t` enum in `g_http_log.h`
2. Create logging function in `g_http_log.c`
3. Call from appropriate game logic

### Event Filtering

Disable logging for specific event types by modifying the implementation to skip certain events.

## Performance Tips

1. **Batch Events** - Modify server to batch multiple events into single requests
2. **Async Processing** - Use worker queues on server side for processing
3. **Event Sampling** - Log only 10% of low-priority events in high-traffic scenarios
4. **Local Caching** - Queue events locally if server is unavailable

## Security Considerations

1. **Use HTTPS** - For production, use TLS/SSL
2. **API Keys** - Add authentication headers to requests
3. **Rate Limiting** - Implement on server side to prevent abuse
4. **Input Validation** - Validate all JSON fields on server
5. **IP Whitelisting** - Only allow OpenArena servers to POST events

## File Locations

| File | Purpose |
|------|---------|
| `code/game/g_http_log.h` | Public API header |
| `code/game/g_http_log.c` | Implementation |
| `HTTP_LOGGING_README.md` | Complete documentation |
| `HTTP_LOGGING_SERVER_EXAMPLES.md` | Server implementations |
| `HTTP_LOGGING_IMPLEMENTATION.md` | Technical details |

## Next Steps

1. ✅ Start a logging server
2. ✅ Configure OpenArena with `g_httpLogURL`
3. ✅ Verify events are being received
4. ✅ Build analytics on top of the event stream
5. ✅ Create dashboards and reports

## Getting Help

- Check logs for error messages
- Review event JSON format in `HTTP_LOGGING_README.md`
- Test server connectivity with curl
- Enable `g_debugAlloc 1` for verbose output
- Check game console output for HTTP log messages

## Example Workflow

```bash
# 1. Start logging server (in separate terminal)
node server.js

# 2. Watch logs in real-time
tail -f logs/game-logs-*.jsonl

# 3. Start OpenArena server
./quake3 +set g_httpLogURL "http://localhost:3000/api/game-logs"

# 4. Connect and play
# Events should appear in both terminals
```

Enjoy real-time game event logging! 🎮📊
