#!/usr/bin/env python3
"""Export a checked ten-feature model manifest without inventing training data."""

from __future__ import annotations

import argparse
import json
import struct
import zlib
from pathlib import Path

FEATURE_ORDER = [
    "mean",
    "max",
    "min",
    "std",
    "center_mean",
    "edge_mean",
    "hot_fraction",
    "ambient",
    "humidity",
    "quality",
]


def crc(parameters: dict) -> int:
    raw = bytearray(struct.pack("<I", int(parameters["version"])))
    for key in ("mean", "scale", "support", "weights"):
        raw.extend(struct.pack("<10f", *[float(value) for value in parameters[key]]))
    raw.extend(struct.pack("<3f", float(parameters["alpha"]), float(parameters["output_mean"]),
                           float(parameters["output_scale"])))
    return zlib.crc32(raw) & 0xFFFFFFFF


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path)
    parser.add_argument("--output", type=Path, default=Path("out/model_manifest.json"))
    args = parser.parse_args()
    if args.input:
        model = json.loads(args.input.read_text(encoding="utf-8"))
    else:
        model = {
            "version": 1,
            "mean": [0.0] * 10,
            "scale": [1.0] * 10,
            "support": [0.0] * 10,
            "weights": [0.55, 0.10, -0.08, 0.04, 0.18, -0.10, 0.05, 0.02, 0.01, 0.06],
            "alpha": 0.05,
            "output_mean": 36.5,
            "output_scale": 1.0,
        }
    if any(len(model.get(key, [])) != 10 for key in ("mean", "scale", "support", "weights")):
        raise SystemExit("all model vectors must have ten values")
    if any(float(value) == 0.0 for value in model["scale"]):
        raise SystemExit("model scale values must be non-zero")
    model["feature_order"] = FEATURE_ORDER
    model["crc32_ieee"] = crc(model)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(model, indent=2) + "\n", encoding="utf-8")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

