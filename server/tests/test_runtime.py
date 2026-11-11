import importlib
import json


def test_ingest_is_idempotent(tmp_path, monkeypatch):
    monkeypatch.setenv("RATSENSE_DATA_DIR", str(tmp_path))
    monkeypatch.setenv("RATSENSE_DB", str(tmp_path / "test.sqlite3"))
    module = importlib.import_module("server.app.main")
    module.DATA_DIR = tmp_path
    module.DB_PATH = tmp_path / "test.sqlite3"
    module.init_db()
    thermal = {"sequence": 1, "monotonic_ms": 10, "predicted_temp": 36.5}
    event = {"event_id": "e1", "session_id": "s1", "sequence": 1, "type": "PELLET_DETECTED"}
    module.ingest_message("ratsense/v1/test/telemetry/thermal", json.dumps(thermal))
    module.ingest_message("ratsense/v1/test/events", json.dumps(event))
    module.ingest_message("ratsense/v1/test/events", json.dumps(event))
    assert len(module.thermal("test")["samples"]) == 1
    assert len(module.events("test")["events"]) == 1

