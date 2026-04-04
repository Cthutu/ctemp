//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(arena, simple)
{
    Arena arena;
    arena_init(&arena, .reserved_size = 1024, .grow_rate = 1);

    TEST_ASSERT_EQ(arena_store(&arena), 0);
    arena_done(&arena);
}

TEST_CASE(arena, alloc_restore_reset)
{
    Arena arena;
    arena_init(&arena, .reserved_size = 4096, .grow_rate = 1);

    u8* first = arena_alloc(&arena, 8);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQ(arena_offset(&arena, first), 0);

    u64   mark = arena_store(&arena);
    u8*   next = arena_alloc(&arena, 16);
    TEST_ASSERT_EQ(arena_offset(&arena, next), 8);

    arena_restore(&arena, mark);
    TEST_ASSERT_EQ(arena_store(&arena), 8);

    arena_reset(&arena);
    TEST_ASSERT_EQ(arena_store(&arena), 0);

    arena_done(&arena);
}

TEST_CASE(arena, session_tracks_count_and_undoes)
{
    Arena arena;
    arena_init(&arena, .reserved_size = 4096, .grow_rate = 1);

    ArenaSession session;
    arena_session_init(&session, &arena, sizeof(u32), sizeof(u32));

    u32* values = arena_session_alloc(&session, 3);
    TEST_ASSERT_NOT_NULL(values);
    TEST_ASSERT_EQ(arena_session_count(&session), 3);
    TEST_ASSERT_EQ(arena_session_address(&session), arena.memory);

    values[0] = 11;
    values[1] = 22;
    values[2] = 33;

    TEST_ASSERT_EQ(values[0], 11);
    TEST_ASSERT_EQ(values[1], 22);
    TEST_ASSERT_EQ(values[2], 33);

    arena_session_undo(&session);
    TEST_ASSERT_EQ(arena_session_count(&session), 0);
    TEST_ASSERT_EQ(arena_store(&arena), 0);

    arena_done(&arena);
}
