//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(arena, simple)
{
    Arena arena;
    arena_init(&arena, .reserved_size = 1024, .grow_rate = 1);

    u32 offset = arena_offset(&arena, arena_store(&arena));
    TEST_ASSERT_EQ(offset, 0);
}
