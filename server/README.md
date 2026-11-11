# Local backend

The backend stores versioned MQTT telemetry and idempotent events in SQLite. It
accepts only the topic shape documented in docs/MQTT_SCHEMA.md, ignores duplicate
event IDs, and exposes a small REST/WebSocket UI for local experiments.

Start it with Docker Compose from this directory. The default broker is intentionally
an anonymous development broker bound to localhost; production deployment must use
TLS, broker authentication, an API token, and a non-public network. The compose
volume directories are ignored by Git.

Useful endpoints:

- GET /health
- GET /api/status
- GET /api/thermal?device_id=...
- GET /api/events?device_id=...
- GET /api/events.csv?device_id=...
- POST /api/commands
- WebSocket /ws/events

Set RATSENSE_API_TOKEN before exposing the API. The API only accepts state/config
commands and never contains a direct motor GPIO operation.

