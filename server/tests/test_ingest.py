import json
from pathlib import Path


def test_file_is_present():
    assert Path(__file__).resolve().parents[1].joinpath("app/main.py").exists()


def test_payload_is_json():
    payload = {"event_id": "e1", "type": "PELLET_DETECTED", "sequence": 1}
    assert json.loads(json.dumps(payload)) == payload

