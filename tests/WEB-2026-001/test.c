#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../Unity/src/unity.h"

static size_t g_large_allocation_request;
static int g_abort_on_large_allocation;
static jmp_buf g_large_allocation_jmp;
static unsigned char *g_font_data;
static size_t g_font_size;

static void *web_2026_001_malloc(size_t size)
{
    /* The malformed font should make the vulnerable code request nearly
       SIZE_MAX bytes after signed int overflow in width * height. The fixed
       code should detect the overflow and return NULL before calling malloc. */
    if (size > (size_t) (16 * 1024 * 1024)) {
        g_large_allocation_request = size;
        if (g_abort_on_large_allocation)
            longjmp(g_large_allocation_jmp, 1);
        return NULL;
    }
    return malloc(size ? size : 1);
}

static void web_2026_001_free(void *ptr)
{
    free(ptr);
}

#define STBTT_malloc(sz, userdata) web_2026_001_malloc(sz)
#define STBTT_free(ptr, userdata) web_2026_001_free(ptr)
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../stb_truetype.h"

static void load_file_or_fail(const char *path)
{
    FILE *f = fopen(path, "rb");
    long size;

    TEST_ASSERT_NOT_NULL_MESSAGE(f, "failed to open generated malformed font");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_END), "fseek end failed");
    size = ftell(f);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, size, "ftell failed or empty font");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_SET), "fseek start failed");

    g_font_data = (unsigned char *) malloc((size_t) size);
    TEST_ASSERT_NOT_NULL_MESSAGE(g_font_data, "failed to allocate font buffer");
    TEST_ASSERT_EQUAL_UINT_MESSAGE((unsigned long) size, (unsigned long) fread(g_font_data, 1, (size_t) size, f), "failed to read font");
    fclose(f);
    g_font_size = (size_t) size;
}

void setUp(void)
{
    g_large_allocation_request = 0;
    g_abort_on_large_allocation = 0;
}

void tearDown(void)
{
}

static void init_font_or_fail(stbtt_fontinfo *font)
{
    memset(font, 0, sizeof(*font));
    TEST_ASSERT_TRUE_MESSAGE(stbtt_InitFont(font, g_font_data, 0), "malformed test font should initialize enough to exercise glyph allocation");
}

void test_glyph_bitmap_rejects_overflow_dimensions_before_allocation(void)
{
    stbtt_fontinfo font;
    unsigned char *bitmap;
    int width = 0;
    int height = 0;

    init_font_or_fail(&font);

    bitmap = stbtt_GetGlyphBitmapSubpixel(&font, 1.0f, 1.0f, 0.0f, 0.0f, 0, &width, &height, NULL, NULL);
    if (bitmap)
        STBTT_free(bitmap, font.userdata);

    TEST_ASSERT_EQUAL_INT_MESSAGE(65535, width, "generated font should produce overflow-width bbox at scale 1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(65535, height, "generated font should produce overflow-height bbox at scale 1");
    TEST_ASSERT_NULL_MESSAGE(bitmap, "overflow-sized bitmap should not be allocated");
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, (unsigned long long) g_large_allocation_request,
        "vulnerable code reached malloc with unchecked width*height overflow");
}

void test_glyph_sdf_rejects_overflow_dimensions_before_allocation(void)
{
    stbtt_fontinfo font;
    unsigned char *sdf = NULL;
    int width = 0;
    int height = 0;
    int jumped;

    init_font_or_fail(&font);

    g_abort_on_large_allocation = 1;
    jumped = setjmp(g_large_allocation_jmp);
    if (jumped == 0) {
        sdf = stbtt_GetGlyphSDF(&font, 1.0f, 0, 0, 180, 36.0f, &width, &height, NULL, NULL);
        if (sdf)
            stbtt_FreeSDF(sdf, font.userdata);
    }
    g_abort_on_large_allocation = 0;

    TEST_ASSERT_EQUAL_INT_MESSAGE(65535, width, "generated font should produce overflow-width SDF bbox at scale 1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(65535, height, "generated font should produce overflow-height SDF bbox at scale 1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, jumped, "vulnerable code reached SDF malloc with unchecked width*height overflow");
    TEST_ASSERT_NULL_MESSAGE(sdf, "overflow-sized SDF should not be allocated");
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, (unsigned long long) g_large_allocation_request,
        "vulnerable code reached SDF malloc with unchecked width*height overflow");
}

int main(void)
{
    load_file_or_fail("malformed_overflow.ttf");

    UNITY_BEGIN();
    RUN_TEST(test_glyph_bitmap_rejects_overflow_dimensions_before_allocation);
    RUN_TEST(test_glyph_sdf_rejects_overflow_dimensions_before_allocation);
    int failures = UNITY_END();

    free(g_font_data);
    g_font_data = NULL;
    g_font_size = 0;
    return failures;
}
