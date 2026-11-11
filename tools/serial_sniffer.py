#!/usr/bin/env python3
"""Decode a captured COBS UART stream for offline review."""

from __future__ import annotations

import argparse
from pathlib import Path


def cobs_decode(encoded: bytes) -> bytes:
    out = bytearray()
    index = 0
    while index < len(encoded):
        code = encoded[index]
        if code == 0:
            raise ValueError("zero inside encoded frame")
        index += 1
        end = index + code - 1
        if end > len(encoded):
            raise ValueError("short COBS block")
        out.extend(encoded[index:end])
        index = end
        if code != 0xFF and index < len(encoded):
            out.append(0)
    return bytes(out)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path, help="binary capture with 0x00 frame delimiters")
    args = parser.parse_args()
    data = args.capture.read_bytes()
    for number, encoded in enumerate(data.split(b"\0")):
        if not encoded:
            continue
        try:
            decoded = cobs_decode(encoded)
            print(f"frame={number} decoded_len={len(decoded)} hex={decoded.hex()}")
        except ValueError as error:
            print(f"frame={number} error={error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

