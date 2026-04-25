#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../Unity/src/unity.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../stb_truetype.h"

static unsigned char *g_font_data;

static void load_file_or_fail(const char *path)
{
    FILE *f = fopen(path, "rb");
    long size;

    TEST_ASSERT_NOT_NULL_MESSAGE(f, "failed to open generated malicious font");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_END), "fseek end failed");
    size = ftell(f);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, size, "ftell failed or empty font");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_SET), "fseek start failed");

    g_font_data = (unsigned char *) malloc((size_t) size);
    TEST_ASSERT_NOT_NULL_MESSAGE(g_font_data, "failed to allocate font buffer");
    TEST_ASSERT_EQUAL_UINT_MESSAGE((unsigned long) size, (unsigned long) fread(g_font_data, 1, (size_t) size, f), "failed to read font");
    fclose(f);
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_self_referencing_compound_glyph_is_rejected_without_recursion_crash(void)
{
    stbtt_fontinfo font;
    stbtt_vertex *vertices = NULL;
    int num_verts;

    memset(&font, 0, sizeof(font));
    TEST_ASSERT_TRUE_MESSAGE(stbtt_InitFont(&font, g_font_data, 0), "malicious font should initialize");

    num_verts = stbtt_GetGlyphShape(&font, 0, &vertices);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, num_verts, "self-referencing compound glyph should be rejected");
    TEST_ASSERT_NULL_MESSAGE(vertices, "rejected recursive glyph should not allocate vertices");
}

int main(void)
{
    load_file_or_fail("selfref_compound.ttf");

    UNITY_BEGIN();
    RUN_TEST(test_self_referencing_compound_glyph_is_rejected_without_recursion_crash);
    int failures = UNITY_END();

    free(g_font_data);
    g_font_data = NULL;
    return failures;
}
