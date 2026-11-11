#!/usr/bin/env python3
"""Create and verify a signed-image manifest; signing remains an external key operation."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("--target-id", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--output", type=Path, default=Path("out/firmware_manifest.json"))
    parser.add_argument("--signature-hex", default="")
    args = parser.parse_args()
    if not args.image.is_file():
        raise SystemExit(f"missing image: {args.image}")
    digest = hashlib.sha256(args.image.read_bytes()).hexdigest()
    manifest = {
        "magic": "RSFW",
        "format_version": 1,
        "target_id": args.target_id,
        "image_version": args.version,
        "image_size": args.image.stat().st_size,
        "sha256": digest,
        "signature": args.signature_hex,
        "signature_required": True,
        "safe_window_required": True,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

