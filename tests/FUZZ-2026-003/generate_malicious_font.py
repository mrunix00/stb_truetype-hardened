#!/usr/bin/env python3
"""Generate a truncated, signature-valid SFNT payload for FUZZ-2026-003."""

from pathlib import Path

OUT = Path(__file__).with_name("truncated_valid_sfnt_tabledir.ttf")
PAYLOAD = bytes([
    0x00, 0x01, 0x00, 0x00,  # SFNT version (valid TrueType signature)
    0x00, 0x02,              # numTables = 2
    0x00, 0x20,              # searchRange = 32 (valid for numTables=2)
    0x00, 0x01,              # entrySelector = 1
    0x00, 0x00,              # rangeShift = 0
    0x02, 0x00, 0x20, 0x00, 0x01, 0x00, 0x01,  # truncated/incomplete directory bytes
])

OUT.write_bytes(PAYLOAD)
print(f"wrote {OUT} ({len(PAYLOAD)} bytes)")
