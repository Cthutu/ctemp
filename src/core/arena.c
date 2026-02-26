//------------------------------------------------------------------------------
// Arena implementation
//
// Copyright (C)2026 Matt Davies, all rights reserved
//------------------------------------------------------------------------------

#include <core/core.h>

#include <stdio.h>

#if OS_POSIX
#    include <sys/mman.h>
#endif

//------------------------------------------------------------------------------

typedef struct {
    usize alloc_granularity;
    usize reserve_granularity;
} ArenaMemoryInfo;

internal ArenaMemoryInfo get_arena_memory_info(void)
{
#if OS_WINDOWS
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return (ArenaMemoryInfo){
        .alloc_granularity   = (usize)sys_info.dwPageSize,
        .reserve_granularity = (usize)sys_info.dwAllocationGranularity,
    };
#elif OS_POSIX
    usize page_size = (usize)sysconf(_SC_PAGESIZE);
    return (ArenaMemoryInfo){
        .alloc_granularity   = page_size,
        .reserve_granularity = page_size,
    };
#else
#    error "Arena memory info not implemented for this OS."
#endif
}

void _arena_init(Arena* arena, ArenaDefaultParams params)
{
    ArenaMemoryInfo mem_info = get_arena_memory_info();

    if (params.grow_rate == 0) {
        params.grow_rate = ARENA_DEFAULT_NUM_PAGES_GROW;
    }
    if (params.reserved_size == 0) {
        params.reserved_size = GB(4);
    }

    params.reserved_size =
        ALIGN_UP(params.reserved_size, mem_info.reserve_granularity);
    usize initial_alloc_size = mem_info.alloc_granularity * params.grow_rate;

    ASSERT(params.reserved_size >= initial_alloc_size,
           "Arena reserved size must be at least %zu bytes",
           initial_alloc_size);

#if OS_WINDOWS
    // Reserve the full range.
    u8* memory = (u8*)VirtualAlloc(nullptr,
                                   params.reserved_size,
                                   MEM_RESERVE | MEM_COMMIT,
                                   PAGE_READWRITE);

    // Allocate the first block.
    mem_check(
        VirtualAlloc(memory, initial_alloc_size, MEM_COMMIT, PAGE_READWRITE));

#elif OS_POSIX
    // Reserve the full range.
    u8* memory = (u8*)mmap(nullptr,
                           params.reserved_size,
                           PROT_NONE,
                           MAP_PRIVATE | MAP_ANONYMOUS,
                           -1,
                           0);
    mem_check(memory);

    // Allocate the first block.
    if (mprotect(memory, initial_alloc_size, PROT_READ | PROT_WRITE) != 0) {
        perror("mprotect");
        exit(1);
    }
#else
#    error "Arena creation not implemented for this OS."
#endif // OS_WINDOWS

    arena->memory            = memory;
    arena->cursor            = 0;
    arena->committed_size    = initial_alloc_size;
    arena->reserved_size     = params.reserved_size;
    arena->alloc_granularity = mem_info.alloc_granularity;
    arena->grow_rate         = params.grow_rate;
}

void arena_done(Arena* arena)
{
#if OS_WINDOWS
    VirtualFree(arena->memory, 0, MEM_RELEASE);
#elif OS_POSIX
    munmap(arena->memory, arena->reserved_size);
#else
#    error "Arena destruction not implemented for this OS."
#endif // OS_WINDOWS

    memset(arena, 0, sizeof(Arena));
}

internal void _arena_ensure_room(Arena* arena, usize size)
{
    usize new_cursor = arena->cursor + size;
    if (new_cursor > arena->reserved_size) {
        eprn("Arena overflow: requested %zu bytes, but only %zu bytes "
             "available.",
             new_cursor,
             arena->reserved_size - arena->cursor);
        exit(1);
    }

    if (new_cursor > arena->committed_size) {
        // Need to commit more memory.
        usize commit_size =
            ALIGN_UP(new_cursor - arena->committed_size,
                     arena->alloc_granularity * arena->grow_rate);

#if OS_WINDOWS
        mem_check(VirtualAlloc(arena->memory + arena->committed_size,
                               commit_size,
                               MEM_COMMIT,
                               PAGE_READWRITE));
#elif OS_POSIX
        if (mprotect(arena->memory + arena->committed_size,
                     commit_size,
                     PROT_READ | PROT_WRITE) != 0) {
            perror("mprotect");
            exit(1);
        }
#else
#    error "Arena memory commit not implemented for this OS."
#endif // OS_WINDOWS

        arena->committed_size += commit_size;
    }
}

void* arena_alloc(Arena* arena, usize size)
{
    _arena_ensure_room(arena, size);

    void* ptr = arena->memory + arena->cursor;
    arena->cursor += size;
    return ptr;
}

void arena_align(Arena* arena, usize align)
{
    usize aligned_cursor = ALIGN_UP(arena->cursor, align);
    _arena_ensure_room(arena, aligned_cursor - arena->cursor);
    arena->cursor = aligned_cursor;
}

void* arena_alloc_align(Arena* arena, usize size, usize align)
{
    arena_align(arena, align);
    return arena_alloc(arena, size);
}

u8* arena_formatv(Arena* arena, cstr fmt, va_list args)
{
    // Get the size of the formatted string.
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    // Allocate space in the arena.
    u8* buffer = (u8*)arena_alloc(arena, (usize)size + 1);

    // Format the string into the buffer.
    vsnprintf((char*)buffer, (usize)size + 1, fmt, args);
    arena->cursor--; // Remove null terminator from arena allocation

    return buffer;
}

u8* arena_format(Arena* arena, cstr fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    u8* result = arena_formatv(arena, fmt, args);
    va_end(args);
    return result;
}

void arena_null_terminate(Arena* arena)
{
    u8* ptr = (u8*)arena_alloc(arena, 1);
    *ptr    = '\0';
}

void* arena_store(Arena* arena) { return arena->memory + arena->cursor; }

void arena_restore(Arena* arena, void* mark)
{
    usize offset = (usize)((u8*)mark - arena->memory);
    ASSERT(offset <= arena->cursor, "Invalid arena restore point.");
    arena->cursor = offset;
}

void arena_reset(Arena* arena) { arena->cursor = 0; }

u32 arena_offset(Arena* arena, void* p)
{
    return (u32)((u8*)p - arena->memory);
}

//------------------------------------------------------------------------------

void arena_session_init(ArenaSession* session,
                        Arena*        arena,
                        usize         alignment,
                        usize         element_size)
{
    session->arena        = arena;
    session->count        = 0;
    session->alignment    = alignment;
    session->element_size = element_size;
    session->start        = arena_store(arena);
}

void arena_session_undo(ArenaSession* session)
{
    arena_restore(session->arena, session->start);
    session->count = 0;
}

void* arena_session_alloc(ArenaSession* session, usize count)
{
    void* ptr = arena_alloc_align(
        session->arena, count * session->element_size, session->alignment);
    session->count += count;
    return ptr;
}

usize arena_session_count(ArenaSession* session) { return session->count; }

void* arena_session_address(ArenaSession* session) { return session->start; }
