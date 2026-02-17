//------------------------------------------------------------------------------
// String API implementation
//
// Copyright (C)2026 Matt Davies, all rights reserved
//------------------------------------------------------------------------------

#include <core/core.h>

#include <stdio.h>

//------------------------------------------------------------------------------

string string_from_cstr(const char* cstr)
{
    if (cstr == NULL) {
        return (string){0};
    }

    usize len = 0;
    while (cstr[len] != '\0') {
        len++;
    }

    return (string){.data = (u8*)cstr, .count = len};
}

string string_formatv(Arena* arena, cstr fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (len < 0) {
        return (string){0};
    }

    u8* data = (u8*)arena_alloc(arena, (usize)len + 1);

    vsnprintf((char*)data, (usize)len + 1, fmt, args);

    arena->cursor--; // Remove null terminator from arena allocation

    return (string){.data = data, .count = (usize)len};
}

string string_format(Arena* arena, cstr fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    string result = string_formatv(arena, fmt, args);
    va_end(args);
    return result;
}

string string_from(u8* data, usize size)
{
    return (string){.data = data, .count = size};
}

//------------------------------------------------------------------------------
// StringBuilder implementation

void sb_init(StringBuilder* sb, Arena* arena)
{
    sb->data  = (u8*)arena_alloc(arena, 0);
    sb->size  = 0;
    sb->arena = arena;
}

void sb_append_cstr(StringBuilder* sb, cstr str)
{
    string s = string_from_cstr(str);
    sb_append_string(sb, s);
}

void sb_append_string(StringBuilder* sb, string str)
{
    u8* dest = (u8*)arena_alloc(sb->arena, str.count);
    memcpy(dest, str.data, str.count);
    sb->size += str.count;
}

void sb_append_char(StringBuilder* sb, char c)
{
    u8* dest = (u8*)arena_alloc(sb->arena, 1);
    *dest    = (u8)c;
    sb->size += 1;
}

void sb_append_null(StringBuilder* sb) { sb_append_char(sb, '\0'); }

void sb_advance(StringBuilder* sb, usize count) { sb->size += count; }

void sb_formatv(StringBuilder* sb, cstr fmt, va_list args)
{
    va_list copy;
    va_copy(copy, args);
    int len = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);

    if (len < 0) {
        return;
    }

    u8* dest = (u8*)arena_alloc(sb->arena, (usize)len + 1);
    vsnprintf((char*)dest, (usize)len + 1, fmt, args);
    sb->arena->cursor--;
    sb->size += (usize)len;
}

void sb_format(StringBuilder* sb, cstr fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    sb_formatv(sb, fmt, args);
    va_end(args);
}

string sb_to_string(StringBuilder* sb)
{
    return (string){.data = sb->data, .count = sb->size};
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
