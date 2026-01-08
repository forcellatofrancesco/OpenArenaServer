# HTTP Logging Server Examples

This document provides example implementations of logging servers that can receive and process events from the OpenArena HTTP logging system.

## Node.js / Express.js Example

### Basic Implementation

```javascript
const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();

// Middleware
app.use(express.json());

// Log directory
const logDir = './logs';
if (!fs.existsSync(logDir)) {
    fs.mkdirSync(logDir);
}

// Logging endpoint
app.post('/api/game-logs', (req, res) => {
    const event = req.body;

    // Validate required fields
    if (!event.event_type || !event.timestamp) {
        return res.status(400).json({ error: 'Missing required fields' });
    }

    // Log to console
    console.log(`[${new Date(event.timestamp).toISOString()}] ${event.event_type}`, event);

    // Save to file (one file per day)
    const today = new Date(event.timestamp).toISOString().split('T')[0];
    const logFile = path.join(logDir, `game-logs-${today}.jsonl`);

    fs.appendFileSync(logFile, JSON.stringify(event) + '\n');

    // Send response
    res.status(200).json({ status: 'ok' });
});

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({ status: 'ok' });
});

// Statistics endpoint
app.get('/api/stats', (req, res) => {
    const stats = {
        uptime: process.uptime(),
        memory: process.memoryUsage(),
        logs_directory: logDir
    };
    res.json(stats);
});

// Error handling
app.use((err, req, res, next) => {
    console.error('Error:', err);
    res.status(500).json({ error: 'Internal server error' });
});

// Start server
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`Game logging server listening on port ${PORT}`);
    console.log(`Logs directory: ${logDir}`);
});
```

### Advanced Implementation with Database

```javascript
const express = require('express');
const sqlite3 = require('sqlite3').verbose();
const app = express();

app.use(express.json());

// SQLite database
const db = new sqlite3.Database(':memory:');

// Create tables
db.serialize(() => {
    // Events table
    db.run(`
        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            event_type TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            game_time TEXT,
            level_name TEXT,
            game_type INTEGER,
            data TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    `);

    // Players table
    db.run(`
        CREATE TABLE IF NOT EXISTS players (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            client_num INTEGER UNIQUE,
            client_name TEXT NOT NULL,
            ip_address TEXT,
            first_seen DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_seen DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    `);

    // Kills table (for quick stats)
    db.run(`
        CREATE TABLE IF NOT EXISTS kills (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            killer_num INTEGER,
            killer_name TEXT,
            victim_num INTEGER,
            victim_name TEXT,
            means_of_death INTEGER,
            timestamp INTEGER,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    `);
});

// Log endpoint
app.post('/api/game-logs', (req, res) => {
    const event = req.body;

    // Insert into events table
    db.run(
        `INSERT INTO events (event_type, timestamp, game_time, level_name, game_type, data)
         VALUES (?, ?, ?, ?, ?, ?)`,
        [
            event.event_type,
            event.timestamp,
            event.game_time,
            event.level_name,
            event.game_type,
            JSON.stringify(event)
        ],
        function(err) {
            if (err) {
                console.error('DB Error:', err);
                return res.status(500).json({ error: 'Database error' });
            }
        }
    );

    // Process specific event types
    switch (event.event_type) {
        case 'PLAYER_CONNECT':
            db.run(
                `INSERT OR IGNORE INTO players (client_num, client_name, ip_address)
                 VALUES (?, ?, ?)`,
                [event.client_num, event.client_name, event.ip_address]
            );
            break;

        case 'KILL':
            db.run(
                `INSERT INTO kills (killer_num, killer_name, victim_num, victim_name, means_of_death, timestamp)
                 VALUES (?, ?, ?, ?, ?, ?)`,
                [event.killer_num, event.killer_name, event.victim_num, event.victim_name, event.means_of_death, event.timestamp]
            );
            break;
    }

    console.log(`Logged: ${event.event_type} at ${new Date(event.timestamp).toISOString()}`);
    res.json({ status: 'ok' });
});

// Statistics endpoints
app.get('/api/stats/kills', (req, res) => {
    db.all(
        `SELECT killer_name, COUNT(*) as kill_count
         FROM kills
         GROUP BY killer_name
         ORDER BY kill_count DESC
         LIMIT 10`,
        (err, rows) => {
            if (err) {
                return res.status(500).json({ error: err.message });
            }
            res.json({ top_killers: rows });
        }
    );
});

app.get('/api/stats/players', (req, res) => {
    db.all(
        `SELECT * FROM players ORDER BY last_seen DESC`,
        (err, rows) => {
            if (err) {
                return res.status(500).json({ error: err.message });
            }
            res.json({ players: rows });
        }
    );
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`Advanced game logging server listening on port ${PORT}`);
});
```

## Python Flask Example

### Basic Implementation

```python
from flask import Flask, request, jsonify
from datetime import datetime
import json
import os

app = Flask(__name__)

LOG_DIR = './logs'
if not os.path.exists(LOG_DIR):
    os.makedirs(LOG_DIR)

@app.route('/api/game-logs', methods=['POST'])
def log_event():
    try:
        event = request.get_json()
        
        # Validate
        if not event or 'event_type' not in event:
            return jsonify({'error': 'Missing event_type'}), 400
        
        # Log to console
        timestamp = datetime.fromtimestamp(event['timestamp'] / 1000)
        print(f"[{timestamp.isoformat()}] {event['event_type']}: {event}")
        
        # Save to file
        date_str = timestamp.strftime('%Y-%m-%d')
        log_file = os.path.join(LOG_DIR, f'game-logs-{date_str}.jsonl')
        
        with open(log_file, 'a') as f:
            f.write(json.dumps(event) + '\n')
        
        return jsonify({'status': 'ok'}), 200
    
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/health', methods=['GET'])
def health():
    return jsonify({'status': 'ok'}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=3000, debug=False)
```

### Advanced Implementation with SQLAlchemy

```python
from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import json
import os

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///game_logs.db'
db = SQLAlchemy(app)

class GameEvent(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    event_type = db.Column(db.String(50), nullable=False)
    timestamp = db.Column(db.Integer, nullable=False)
    game_time = db.Column(db.String(10))
    level_name = db.Column(db.String(100))
    game_type = db.Column(db.Integer)
    data = db.Column(db.JSON)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)

class Kill(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    killer_num = db.Column(db.Integer)
    killer_name = db.Column(db.String(100))
    victim_num = db.Column(db.Integer)
    victim_name = db.Column(db.String(100))
    means_of_death = db.Column(db.Integer)
    timestamp = db.Column(db.Integer)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)

class Player(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    client_num = db.Column(db.Integer, unique=True)
    client_name = db.Column(db.String(100))
    ip_address = db.Column(db.String(50))
    first_seen = db.Column(db.DateTime, default=datetime.utcnow)
    last_seen = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

@app.route('/api/game-logs', methods=['POST'])
def log_event():
    try:
        event = request.get_json()
        
        # Store event
        game_event = GameEvent(
            event_type=event.get('event_type'),
            timestamp=event.get('timestamp'),
            game_time=event.get('game_time'),
            level_name=event.get('level_name'),
            game_type=event.get('game_type'),
            data=event
        )
        db.session.add(game_event)
        
        # Process specific events
        if event['event_type'] == 'KILL':
            kill = Kill(
                killer_num=event.get('killer_num'),
                killer_name=event.get('killer_name'),
                victim_num=event.get('victim_num'),
                victim_name=event.get('victim_name'),
                means_of_death=event.get('means_of_death'),
                timestamp=event.get('timestamp')
            )
            db.session.add(kill)
        
        elif event['event_type'] == 'PLAYER_CONNECT':
            player = Player.query.filter_by(client_num=event.get('client_num')).first()
            if not player:
                player = Player(
                    client_num=event.get('client_num'),
                    client_name=event.get('client_name'),
                    ip_address=event.get('ip_address')
                )
                db.session.add(player)
            else:
                player.last_seen = datetime.utcnow()
        
        db.session.commit()
        return jsonify({'status': 'ok'}), 200
    
    except Exception as e:
        db.session.rollback()
        return jsonify({'error': str(e)}), 500

@app.route('/api/stats/top-killers', methods=['GET'])
def top_killers():
    limit = request.args.get('limit', 10, type=int)
    kills = db.session.query(
        Kill.killer_name,
        db.func.count(Kill.id).label('count')
    ).group_by(Kill.killer_name).order_by(db.desc('count')).limit(limit).all()
    
    return jsonify({
        'top_killers': [{'name': k[0], 'kills': k[1]} for k in kills]
    })

@app.route('/api/stats/active-players', methods=['GET'])
def active_players():
    players = Player.query.order_by(Player.last_seen.desc()).all()
    return jsonify({
        'players': [{
            'client_num': p.client_num,
            'client_name': p.client_name,
            'ip_address': p.ip_address,
            'first_seen': p.first_seen.isoformat(),
            'last_seen': p.last_seen.isoformat()
        } for p in players]
    })

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    app.run(host='0.0.0.0', port=3000, debug=False)
```

## Go Implementation

```go
package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"time"
)

type GameEvent struct {
	Timestamp  int64       `json:"timestamp"`
	EventType  string      `json:"event_type"`
	GameTime   string      `json:"game_time"`
	LevelName  string      `json:"level_name"`
	GameType   int         `json:"game_type"`
	Data       interface{} `json:"data"`
}

func logEventHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var event GameEvent
	if err := json.NewDecoder(r.Body).Decode(&event); err != nil {
		http.Error(w, "Invalid JSON", http.StatusBadRequest)
		return
	}

	// Log to console
	timestamp := time.Unix(event.Timestamp/1000, 0)
	fmt.Printf("[%s] %s: %+v\n", timestamp.Format(time.RFC3339), event.EventType, event)

	// Write to file
	file, err := os.OpenFile("game-logs.txt", os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
	if err != nil {
		http.Error(w, "Internal server error", http.StatusInternalServerError)
		return
	}
	defer file.Close()

	data, _ := json.Marshal(event)
	file.WriteString(string(data) + "\n")

	// Response
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{"status": "ok"})
}

func healthHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{"status": "ok"})
}

func main() {
	http.HandleFunc("/api/game-logs", logEventHandler)
	http.HandleFunc("/health", healthHandler)

	fmt.Println("Game logging server listening on port 3000")
	log.Fatal(http.ListenAndServe(":3000", nil))
}
```

## Docker Deployment

### Node.js Dockerfile

```dockerfile
FROM node:18-alpine

WORKDIR /app

COPY package.json .
RUN npm install

COPY server.js .

EXPOSE 3000

CMD ["node", "server.js"]
```

### Docker Compose

```yaml
version: '3.8'

services:
  game-logs:
    build: .
    ports:
      - "3000:3000"
    volumes:
      - ./logs:/app/logs
    environment:
      - NODE_ENV=production
      - PORT=3000
    restart: unless-stopped
```

## Testing the Logging Server

### Using curl

```bash
# Test with a kill event
curl -X POST http://localhost:3000/api/game-logs \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp": 1234567890,
    "event_type": "KILL",
    "game_time": "05:30",
    "level_name": "q3dm1",
    "game_type": 0,
    "killer_num": 1,
    "killer_name": "Player1",
    "victim_num": 2,
    "victim_name": "Player2",
    "means_of_death": 7
  }'

# Check health
curl http://localhost:3000/health
```

### Using Python

```python
import requests
import json
import time

event = {
    "timestamp": int(time.time() * 1000),
    "event_type": "KILL",
    "game_time": "05:30",
    "level_name": "q3dm1",
    "game_type": 0,
    "killer_num": 1,
    "killer_name": "Player1",
    "victim_num": 2,
    "victim_name": "Player2",
    "means_of_death": 7
}

response = requests.post(
    'http://localhost:3000/api/game-logs',
    json=event
)
print(response.json())
```

## Production Considerations

1. **Authentication** - Add API keys or OAuth for production
2. **Rate Limiting** - Implement request rate limiting
3. **Data Retention** - Implement archival/cleanup policies
4. **Monitoring** - Add health checks and alerts
5. **Scalability** - Use load balancing for multiple servers
6. **Security** - Use HTTPS/TLS for production
7. **Backup** - Regular database backups
8. **Analytics** - Add real-time dashboards and reports
