//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(core, sizes_and_clamp)
{
    TEST_ASSERT_EQ(KB(2), 2048ull);
    TEST_ASSERT_EQ(MB(2), 2097152ull);
    TEST_ASSERT_EQ(GB(1), 1073741824ull);

    TEST_ASSERT_EQ(MIN(3, 7), 3);
    TEST_ASSERT_EQ(MAX(3, 7), 7);
    TEST_ASSERT_EQ(CLAMP(5, 0, 4), 4);
    TEST_ASSERT_EQ(CLAMP(-3, 0, 4), 0);
    TEST_ASSERT_EQ(CLAMP(2, 0, 4), 2);
}

TEST_CASE(core, align_up_and_ptr)
{
    TEST_ASSERT_EQ(ALIGN_UP(13, 8), 16);
    TEST_ASSERT_EQ(ALIGN_UP(16, 8), 16);

    u8   bytes[32] = {0};
    u8*  raw       = bytes + 1;
    u32* aligned   = ALIGN_PTR_UP(u32, raw, 8);

    TEST_ASSERT_EQ(((usize)aligned) % 8, 0);
    TEST_ASSERT_GE((usize)aligned, (usize)raw);
}
