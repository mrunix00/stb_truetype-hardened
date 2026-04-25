#!/usr/bin/env python3
"""Generate truncated but structurally-consistent SFNT header payload for FUZZ-2026-002."""

from pathlib import Path

OUT = Path(__file__).with_name("truncated_consistent_tabledir.ttf")
PAYLOAD = bytes([0x4C, 0x4C, 0x4C, 0x14, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00])

OUT.write_bytes(PAYLOAD)
print(f"wrote {OUT} ({len(PAYLOAD)} bytes)")
