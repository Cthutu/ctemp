//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(memory, alloc_realloc_free)
{
    u8* buffer = KORE_ARRAY_ALLOC(u8, 8);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_EQ(mem_size(buffer), 8);

    memset(buffer, 0x2a, 8);
    buffer = KORE_ARRAY_REALLOC(buffer, u8, 16);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_EQ(mem_size(buffer), 16);
    TEST_ASSERT_EQ(buffer[0], 0x2a);

    KORE_ARRAY_FREE(buffer);
    TEST_ASSERT_EQ(buffer, NULL);
}

#if CONFIG_DEBUG
TEST_CASE(memory, debug_counts_track_live_allocations)
{
    usize before_count = mem_get_allocation_count();
    usize before_total = mem_get_total_allocated();

    void* a = KORE_ALLOC(32);
    void* b = KORE_ALLOC(48);

    TEST_ASSERT_EQ(mem_get_allocation_count(), before_count + 2);
    TEST_ASSERT_EQ(mem_get_total_allocated(), before_total + 80);

    KORE_FREE(a);
    KORE_FREE(b);

    TEST_ASSERT_EQ(mem_get_allocation_count(), before_count);
    TEST_ASSERT_EQ(mem_get_total_allocated(), before_total);
}
#endif
