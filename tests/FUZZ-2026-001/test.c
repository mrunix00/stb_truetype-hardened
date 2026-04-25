#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../../Unity/src/unity.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../stb_truetype.h"

static unsigned char *g_payload;
static size_t g_payload_size;

static void load_payload_or_fail(const char *path)
{
    FILE *f = fopen(path, "rb");
    long sz;

    TEST_ASSERT_NOT_NULL_MESSAGE(f, "failed to open malformed font payload");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_END), "fseek end failed");
    sz = ftell(f);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, sz, "payload must be non-empty");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_SET), "fseek start failed");

    g_payload = (unsigned char *)malloc((size_t)sz);
    TEST_ASSERT_NOT_NULL_MESSAGE(g_payload, "malloc failed for payload");

    TEST_ASSERT_EQUAL_UINT_MESSAGE((unsigned long)sz,
                                   (unsigned long)fread(g_payload, 1, (size_t)sz, f),
                                   "failed to read payload");
    fclose(f);
    g_payload_size = (size_t)sz;
}

void setUp(void) {}
void tearDown(void) {}

void test_truncated_tabledir_is_rejected_without_oob_read(void)
{
    stbtt_fontinfo info;
    long page_size = sysconf(_SC_PAGESIZE);
    size_t map_size;
    unsigned char *mapping;
    unsigned char *font_ptr;
    int ok;

    TEST_ASSERT_GREATER_THAN_MESSAGE(0, page_size, "sysconf(_SC_PAGESIZE) failed");

    map_size = (size_t)page_size * 2u;
    mapping = (unsigned char *)mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(MAP_FAILED, mapping, "mmap failed");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0,
                                  mprotect(mapping + (size_t)page_size, (size_t)page_size, PROT_NONE),
                                  "mprotect guard page failed");

    font_ptr = mapping + (size_t)page_size - g_payload_size;
    memcpy(font_ptr, g_payload, g_payload_size);

    memset(&info, 0, sizeof(info));
    ok = stbtt_InitFont(&info, font_ptr, 0);

    /* Fixed behavior: malformed/truncated input is rejected cleanly. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ok, "malformed truncated table directory must be rejected");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, munmap(mapping, map_size), "munmap failed");
}

int main(void)
{
    load_payload_or_fail("truncated_tabledir.ttf");

    UNITY_BEGIN();
    RUN_TEST(test_truncated_tabledir_is_rejected_without_oob_read);
    int failures = UNITY_END();

    free(g_payload);
    g_payload = NULL;
    g_payload_size = 0;
    return failures;
}
