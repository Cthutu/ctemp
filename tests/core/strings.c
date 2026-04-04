//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(string, from_cstr_and_from_bytes)
{
    string hello = string_from_cstr("hello");
    TEST_ASSERT_EQ(hello.count, 5);
    TEST_ASSERT_EQ(hello.data[0], 'h');

    u8 raw[]     = {'a', 'b', 'c'};
    string bytes = string_from(raw, 3);
    TEST_ASSERT_EQ(bytes.count, 3);
    TEST_ASSERT_EQ(bytes.data[2], 'c');
}

TEST_CASE(string, format_uses_arena_storage)
{
    Arena arena;
    arena_init(&arena, .reserved_size = 4096, .grow_rate = 1);

    string text = string_format(&arena, "%s %d", "value", 42);
    TEST_ASSERT_EQ(text.count, 8);
    TEST_ASSERT_STR_EQ((char*)text.data, "value 42");

    arena_done(&arena);
}

TEST_CASE(string, builder_appends_and_formats)
{
    Arena arena;
    arena_init(&arena, .reserved_size = 4096, .grow_rate = 1);

    StringBuilder sb;
    sb_init(&sb, &arena);
    sb_append_cstr(&sb, "ab");
    sb_append_char(&sb, 'c');
    sb_append_string(&sb, string_from_cstr("de"));
    sb_format(&sb, "%d", 12);
    sb_append_null(&sb);

    string built = sb_to_string(&sb);
    TEST_ASSERT_EQ(built.count, 8);
    TEST_ASSERT_EQ(built.data[7], '\0');
    TEST_ASSERT_STR_EQ((char*)built.data, "abcde12");

    arena_done(&arena);
}
