## WEB-2026-001

- **Summary**: Added allocation-size guards before bitmap and SDF buffers are allocated from glyph bounding-box dimensions.
- **Root Cause**: `stbtt_GetGlyphBitmapSubpixel` and `stbtt_GetGlyphSDF` multiplied attacker-influenced dimensions (`gbm.w * gbm.h` and `w * h`) as signed `int` values without checking that both dimensions were positive and that the product fit in `int`. Malformed glyph bounding boxes could make the multiplication overflow and reach `STBTT_malloc` as a huge or otherwise invalid allocation size.
- **Fix Description**: Included `<limits.h>` for `INT_MAX`, changed the bitmap allocation path to require positive dimensions, and returned `NULL` before allocation when `width > INT_MAX / height`. Added the same positive-dimension and overflow guard to the SDF path before its `w * h` allocation.
- **Changed Code**: `stb_truetype.h`: added `<limits.h>` include; guarded `stbtt_GetGlyphBitmapSubpixel` allocation; guarded `stbtt_GetGlyphSDF` allocation.
- **Regression Risk**: Low. The fix only rejects malformed or impractically large glyph bitmaps/SDFs whose allocation size cannot be represented safely. Valid fonts with normal positive dimensions continue through the existing allocation and rendering paths.

## WEB-2026-002

- **Summary**: Added a recursion-depth guard to TrueType compound glyph shape resolution so self-referential/cyclic glyph graphs no longer recurse until stack overflow.
- **Root Cause**: `stbtt__GetGlyphShapeTT` handled compound glyph components by recursively resolving referenced glyphs without any depth limit or cycle break. Malicious fonts can point a component back to itself (or form cycles) and trigger unbounded recursion.
- **Fix Description**: Introduced an internal recursion depth parameter for the TrueType glyph-shape routine and enforced a conservative maximum depth (`STBTT_MAX_GLYPH_RECURSION_DEPTH = 32`). Recursive component resolution now calls the depth-tracked internal function (`depth + 1`) instead of the public wrapper, and returns failure (`0` vertices) when the depth limit is reached.
- **Changed Code**: `stb_truetype.h` — updated `stbtt__GetGlyphShapeTT` signature to include recursion depth, added depth-limit check, changed compound glyph recursive call to internal depth-tracked call, and updated `stbtt_GetGlyphShape` to seed depth `0` for TrueType paths.
- **Regression Risk**: Low. The change is localized to malformed/cyclic deep compound-glyph cases and preserves behavior for normal fonts with typical component depth.

## FUZZ-2026-001

- **Summary**: Hardened SFNT table-directory parsing by validating offset-table search parameters before iterating table records.
- **Root Cause**: `stbtt__find_table` trusted `numTables` and immediately iterated `data + tabledir + 16*i` without validating basic offset-table consistency (`searchRange`, `entrySelector`, `rangeShift`). Truncated fuzz inputs could set attacker-controlled header bytes and drive reads past the provided buffer.
- **Fix Description**: Added strict SFNT offset-table sanity checks in `stbtt__find_table`: reject `numTables == 0`, recompute expected `searchRange`, `entrySelector`, and `rangeShift` from `numTables`, and return failure when header values are inconsistent. Only then iterate table records.
- **Changed Code**: `stb_truetype.h` — updated `stbtt__find_table` to parse and validate offset-table fields before scanning. Updated malformed-font generators in `tests/WEB-2026-001/generate_malformed_font.py` and `tests/WEB-2026-002/generate_malicious_font.py` to emit spec-consistent offset-table search fields.
- **Regression Risk**: Low to medium. Valid SFNT fonts should already satisfy these invariants. The change may reject malformed but previously tolerated fonts with inconsistent directory-search metadata.

## FUZZ-2026-002

- **Summary**: Added an early font-signature gate in initialization to reject non-font payloads before table scans.
- **Root Cause**: `stbtt_InitFont_internal` proceeded to table discovery via repeated `stbtt__find_table` calls without first checking whether `data + fontstart` actually begins with a recognized SFNT/OTF signature. An attacker could provide truncated non-font bytes with internally consistent search fields and still drive table-directory reads out of bounds.
- **Fix Description**: Added a minimal early check in `stbtt_InitFont_internal` using `stbtt__isfont(data + fontstart)`. If the signature is not a recognized font container/header, initialization now fails immediately (`return 0`) before any table lookups.
- **Changed Code**: `stb_truetype.h` — inserted `stbtt__isfont` guard near the start of `stbtt_InitFont_internal`.
- **Regression Risk**: Low. This only tightens initialization for obviously invalid/non-font payloads and preserves behavior for valid TrueType/OpenType signatures.

## FUZZ-2026-003

- **Summary**: Added a minimum SFNT table-count sanity gate before any table-directory scanning.
- **Root Cause**: `stbtt_InitFont_internal` accepted any signature-valid SFNT header and then invoked `stbtt__find_table`, which iterates based on untrusted `numTables`. Extremely short/truncated payloads with `numTables` set below the minimum needed for supported fonts could still reach the directory scanner and trigger out-of-bounds reads.
- **Fix Description**: In `stbtt_InitFont_internal`, after the existing signature check, read `numTables` from the offset table and reject headers with fewer than 4 tables before any `stbtt__find_table` calls.
- **Changed Code**: `stb_truetype.h` — introduced local `sfnt_num_tables` in `stbtt_InitFont_internal` and added `if (sfnt_num_tables < 4) return 0;` prior to table lookups.
- **Regression Risk**: Low. Supported initialization paths already require at least `cmap`, `head`, `hhea`, and `hmtx`, so valid fonts should not be affected.

## WEB-2026-003

- **Summary**: Hardened TrueType contour-start handling to avoid one-past-end lookahead when an off-curve contour starts at the last point.
- **Root Cause**: In `stbtt__GetGlyphShapeTT`, when `next_move == i` and the start point was off-curve, code unconditionally read `vertices[off+i+1]`. For malformed glyphs where `i == n-1`, this became `vertices[off+n]` (one past the allocated vertex buffer).
- **Fix Description**: Added a bounds guard to only enter the off-curve start lookahead path when `(i+1) < n`. Otherwise, the code falls back to the non-lookahead start path and avoids out-of-bounds access.
- **Changed Code**: `stb_truetype.h` — changed `if (start_off)` to `if (start_off && (i+1) < n)` in `stbtt__GetGlyphShapeTT`.
- **Regression Risk**: Low. The change is localized to malformed edge cases at contour starts and preserves existing behavior for valid glyph point sequences where a next point exists.
