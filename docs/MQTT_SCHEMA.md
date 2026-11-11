# MQTT schema

Development broker: Mosquitto on localhost, MQTT 3.1.1. Production must enable
TLS, broker authentication and a non-public network. No retained control command
is used. Only availability is retained.

Topic prefix: ratsense/v1/{device_id}

| Suffix | QoS | Retained | Payload |
|---|---:|---:|---|
| telemetry/status | 1 | no | firmware, uptime, fault, queue and storage state |
| telemetry/thermal | 0 or 1 | no | monotonic_ms, wall_time_ms, predicted_temp, quality, ambient, model_version |
| telemetry/training | 1 | no | press_count, motor_attempt_count, pellet_count, pressure summary |
| events | 1 | no | event_id, session_id, sequence, type, monotonic_ms, result |
| cmd/request | 1 | no | cmd_id, issued_at, expires_at, cmd_type, params, authorized_by |
| cmd/response | 1 | no | cmd_id, status, error_code, applied_version |
| config/reported | 1 | no | versioned actual device parameters |
| config/desired | 1 | no | versioned desired parameters |
| availability | 1 | yes | online/offline, firmware, monotonic_ms |

The backend validates event_id and cmd_id, stores events with a uniqueness
constraint, and exposes an API token. A repeated QoS1 message is safe. Commands
are restricted to session/config transitions and are sent to STM32 for local
state validation; MQTT never toggles motor GPIO.

