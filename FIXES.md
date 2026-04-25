## WEB-2026-001

- **Summary**: Added allocation-size guards before bitmap and SDF buffers are allocated from glyph bounding-box dimensions.
- **Root Cause**: `stbtt_GetGlyphBitmapSubpixel` and `stbtt_GetGlyphSDF` multiplied attacker-influenced dimensions (`gbm.w * gbm.h` and `w * h`) as signed `int` values without checking that both dimensions were positive and that the product fit in `int`. Malformed glyph bounding boxes could make the multiplication overflow and reach `STBTT_malloc` as a huge or otherwise invalid allocation size.
- **Fix Description**: Included `<limits.h>` for `INT_MAX`, changed the bitmap allocation path to require positive dimensions, and returned `NULL` before allocation when `width > INT_MAX / height`. Added the same positive-dimension and overflow guard to the SDF path before its `w * h` allocation.
- **Changed Code**: `stb_truetype.h`: added `<limits.h>` include; guarded `stbtt_GetGlyphBitmapSubpixel` allocation; guarded `stbtt_GetGlyphSDF` allocation.
- **Regression Risk**: Low. The fix only rejects malformed or impractically large glyph bitmaps/SDFs whose allocation size cannot be represented safely. Valid fonts with normal positive dimensions continue through the existing allocation and rendering paths.
