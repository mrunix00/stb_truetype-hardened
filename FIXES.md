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
