import csv
import asyncio
import io
import json
import os
import sqlite3
import threading
import time
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Any

from fastapi import Depends, FastAPI, Header, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, StreamingResponse
from pydantic import BaseModel, Field

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = Path(os.getenv("RATSENSE_DATA_DIR", ROOT / "data"))
DB_PATH = Path(os.getenv("RATSENSE_DB", DATA_DIR / "ratsense.sqlite3"))
DEVICE_TOKEN = os.getenv("RATSENSE_API_TOKEN", "")
MQTT_ENABLED = os.getenv("RATSENSE_MQTT_ENABLED", "0") == "1"
MQTT_HOST = os.getenv("RATSENSE_MQTT_HOST", "mosquitto")
MQTT_PORT = int(os.getenv("RATSENSE_MQTT_PORT", "1883"))

db_lock = threading.Lock()
listeners: set[WebSocket] = set()
mqtt_client: Any = None
event_loop: asyncio.AbstractEventLoop | None = None


def db() -> sqlite3.Connection:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    connection = sqlite3.connect(DB_PATH)
    connection.row_factory = sqlite3.Row
    return connection


def init_db() -> None:
    with db_lock, db() as connection:
        connection.executescript(
            """
            CREATE TABLE IF NOT EXISTS telemetry (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                device_id TEXT NOT NULL,
                kind TEXT NOT NULL,
                monotonic_ms INTEGER NOT NULL,
                wall_time_ms INTEGER,
                payload_json TEXT NOT NULL,
                unique_key TEXT UNIQUE
            );
            CREATE TABLE IF NOT EXISTS events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                device_id TEXT NOT NULL,
                event_id TEXT NOT NULL,
                session_id TEXT,
                sequence INTEGER,
                event_type TEXT NOT NULL,
                monotonic_ms INTEGER,
                payload_json TEXT NOT NULL,
                UNIQUE(device_id, event_id)
            );
            CREATE TABLE IF NOT EXISTS commands (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                device_id TEXT NOT NULL,
                cmd_id TEXT NOT NULL UNIQUE,
                command_json TEXT NOT NULL,
                status TEXT NOT NULL,
                created_at_ms INTEGER NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_telemetry_device_time
                ON telemetry(device_id, monotonic_ms);
            CREATE INDEX IF NOT EXISTS idx_events_session
                ON events(device_id, session_id, sequence);
            """
        )


async def broadcast(message: dict[str, Any]) -> None:
    stale: list[WebSocket] = []
    for socket in list(listeners):
        try:
            await socket.send_json(message)
        except Exception:
            stale.append(socket)
    for socket in stale:
        listeners.discard(socket)


def schedule_broadcast(message: dict[str, Any]) -> None:
    if event_loop is None:
        return
    try:
        asyncio.run_coroutine_threadsafe(broadcast(message), event_loop)
    except RuntimeError:
        pass


def require_token(x_ratsense_token: str | None = Header(default=None)) -> None:
    if DEVICE_TOKEN and x_ratsense_token != DEVICE_TOKEN:
        raise HTTPException(status_code=401, detail="invalid token")


def ingest_message(topic: str, payload: bytes | str) -> None:
    """Decode a versioned MQTT JSON payload and idempotently persist it."""
    if isinstance(payload, bytes):
        payload = payload.decode("utf-8")
    data = json.loads(payload)
    parts = topic.strip("/").split("/")
    if len(parts) < 4 or parts[0] != "ratsense" or parts[1] != "v1":
        raise ValueError("unsupported topic")
    device_id = parts[2]
    kind = "/".join(parts[3:])
    now_ms = int(time.time() * 1000)
    if kind == "events":
        event_id = str(data.get("event_id", ""))
        if not event_id:
            raise ValueError("event_id is required")
        with db_lock, db() as connection:
            connection.execute(
                """INSERT OR IGNORE INTO events
                   (device_id,event_id,session_id,sequence,event_type,monotonic_ms,payload_json)
                   VALUES (?,?,?,?,?,?,?)""",
                (
                    device_id,
                    event_id,
                    str(data.get("session_id", "")),
                    data.get("sequence"),
                    str(data.get("type", "unknown")),
                    data.get("monotonic_ms"),
                    json.dumps(data, separators=(",", ":")),
                ),
            )
        schedule_broadcast({"topic": topic, "kind": kind, "payload": data})
        return
    if kind.startswith("telemetry/"):
        unique_key = f"{device_id}:{kind}:{data.get('sequence', now_ms)}"
        with db_lock, db() as connection:
            connection.execute(
                """INSERT OR IGNORE INTO telemetry
                   (device_id,kind,monotonic_ms,wall_time_ms,payload_json,unique_key)
                   VALUES (?,?,?,?,?,?)""",
                (
                    device_id,
                    kind.removeprefix("telemetry/"),
                    int(data.get("monotonic_ms", 0)),
                    data.get("wall_time_ms"),
                    json.dumps(data, separators=(",", ":")),
                    unique_key,
                ),
            )
        schedule_broadcast({"topic": topic, "kind": kind, "payload": data})
        return
    if kind == "availability":
        unique_key = f"{device_id}:availability:{data.get('monotonic_ms', now_ms)}"
        with db_lock, db() as connection:
            connection.execute(
                """INSERT OR IGNORE INTO telemetry
                   (device_id,kind,monotonic_ms,wall_time_ms,payload_json,unique_key)
                   VALUES (?,?,?,?,?,?)""",
                (device_id, "availability", data.get("monotonic_ms", 0), now_ms, json.dumps(data), unique_key),
            )
        schedule_broadcast({"topic": topic, "kind": kind, "payload": data})
        return
    raise ValueError("unsupported MQTT kind")


class CommandRequest(BaseModel):
    device_id: str = Field(min_length=1, max_length=64, pattern=r"^[A-Za-z0-9_.-]+$")
    cmd_id: str = Field(min_length=1, max_length=80)
    cmd_type: str = Field(min_length=1, max_length=40)
    params: dict[str, Any] = Field(default_factory=dict)
    expires_at: int | None = None


def start_mqtt() -> None:
    global mqtt_client
    if not MQTT_ENABLED:
        return
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        return
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="ratsense-backend")

    def on_connect(client, userdata, flags, reason_code, properties=None):
        del userdata, flags, reason_code, properties
        client.subscribe("ratsense/v1/+/telemetry/#", qos=1)
        client.subscribe("ratsense/v1/+/events", qos=1)
        client.subscribe("ratsense/v1/+/availability", qos=1)

    def on_message(client, userdata, message):
        del client, userdata
        try:
            ingest_message(message.topic, message.payload)
        except (ValueError, json.JSONDecodeError):
            pass

    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect(MQTT_HOST, MQTT_PORT, keepalive=30)
    mqtt_client.loop_start()


def stop_mqtt() -> None:
    if mqtt_client is not None:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()


@asynccontextmanager
async def lifespan(_: FastAPI):
    global event_loop
    event_loop = asyncio.get_running_loop()
    init_db()
    start_mqtt()
    yield
    stop_mqtt()
    event_loop = None


app = FastAPI(title="RatSense local backend", version="1.0", lifespan=lifespan)


@app.get("/", response_class=HTMLResponse)
def index() -> str:
    return (ROOT / "web" / "index.html").read_text(encoding="utf-8")


@app.get("/health")
def health() -> dict[str, Any]:
    return {"ok": True, "mqtt_enabled": MQTT_ENABLED, "db": str(DB_PATH)}


@app.get("/api/status", dependencies=[Depends(require_token)])
def status() -> dict[str, Any]:
    with db_lock, db() as connection:
        rows = connection.execute(
            """SELECT device_id, payload_json, monotonic_ms FROM telemetry
               WHERE kind='status' ORDER BY id DESC LIMIT 100"""
        ).fetchall()
    return {"devices": [dict(row) | {"payload": json.loads(row["payload_json"])} for row in rows]}


@app.get("/api/thermal", dependencies=[Depends(require_token)])
def thermal(device_id: str, limit: int = 500) -> dict[str, Any]:
    limit = max(1, min(limit, 5000))
    with db_lock, db() as connection:
        rows = connection.execute(
            """SELECT monotonic_ms, wall_time_ms, payload_json FROM telemetry
               WHERE device_id=? AND kind='thermal' ORDER BY monotonic_ms DESC LIMIT ?""",
            (device_id, limit),
        ).fetchall()
    return {"device_id": device_id, "samples": [json.loads(row["payload_json"]) for row in reversed(rows)]}


@app.get("/api/events", dependencies=[Depends(require_token)])
def events(device_id: str, session_id: str | None = None, limit: int = 500) -> dict[str, Any]:
    limit = max(1, min(limit, 5000))
    query = "SELECT payload_json FROM events WHERE device_id=?"
    values: list[Any] = [device_id]
    if session_id:
        query += " AND session_id=?"
        values.append(session_id)
    query += " ORDER BY sequence ASC, id ASC LIMIT ?"
    values.append(limit)
    with db_lock, db() as connection:
        rows = connection.execute(query, values).fetchall()
    return {"device_id": device_id, "events": [json.loads(row["payload_json"]) for row in rows]}


@app.get("/api/events.csv", dependencies=[Depends(require_token)])
def events_csv(device_id: str, session_id: str | None = None) -> StreamingResponse:
    result = events(device_id, session_id, 5000)
    output = io.StringIO()
    writer = csv.DictWriter(output, fieldnames=["event_id", "session_id", "type", "sequence", "monotonic_ms", "result"])
    writer.writeheader()
    for event in result["events"]:
        writer.writerow({field: event.get(field, "") for field in writer.fieldnames})
    return StreamingResponse(iter([output.getvalue()]), media_type="text/csv")


@app.post("/api/commands", dependencies=[Depends(require_token)])
def command(request: CommandRequest) -> dict[str, Any]:
    if request.cmd_type not in {"START_SESSION", "PAUSE_SESSION", "RESUME_SESSION", "STOP_SESSION", "CONFIG_SET"}:
        raise HTTPException(status_code=400, detail="unsupported command")
    now_ms = int(time.time() * 1000)
    if request.expires_at is not None and request.expires_at < now_ms:
        raise HTTPException(status_code=400, detail="expired command")
    command_json = request.model_dump_json()
    with db_lock, db() as connection:
        try:
            connection.execute(
                """INSERT INTO commands(device_id,cmd_id,command_json,status,created_at_ms)
                   VALUES(?,?,?,?,?)""",
                (request.device_id, request.cmd_id, command_json, "queued", now_ms),
            )
        except sqlite3.IntegrityError:
            row = connection.execute("SELECT status FROM commands WHERE cmd_id=?", (request.cmd_id,)).fetchone()
            return {"cmd_id": request.cmd_id, "status": row["status"] if row else "duplicate"}
    topic = f"ratsense/v1/{request.device_id}/cmd/request"
    if mqtt_client is not None:
        mqtt_client.publish(topic, command_json, qos=1)
    return {"cmd_id": request.cmd_id, "status": "queued", "topic": topic}


@app.websocket("/ws/events")
async def websocket_events(websocket: WebSocket) -> None:
    await websocket.accept()
    listeners.add(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        listeners.discard(websocket)

