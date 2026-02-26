//------------------------------------------------------------------------------
// Core module
//
// Copyright (C)2026 Matt Davies, all rights reserved
//------------------------------------------------------------------------------
//> def: _POSIX_C_SOURCE=200809L
//> def: _GNU_SOURCE

#pragma once

//------------------------------------------------------------------------------

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// INDEX
//
// [Config]             Configuration macros and settings
// [Macros]             Basic macros
// [Types]              Basic types and definitions
// [Library]            Library initialisation and shutdown
// [Memory]             Memory management functions
// [Array]              Dynamic array implementation
// [Mutex]              Simple locking for resource protection
// [Output]             Basic output to stdout and stderr
// [Arena]              Memory management via arenas and paging
// [Time]               Various cross-platform functions for handling time
// [Random]             Some simple routines for random number generation
// [Data]               Simple file-mapped routines
// [String]             String views and builder
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------[Config]

#define YES (1)
#define NO (0)

//
// Compiler detection
//

#define COMPILER_GCC NO
#define COMPILER_CLANG NO
#define COMPILER_MSVC NO

#if defined(__CLANG__)
#    undef COMPILER_CLANG
#    define COMPILER_CLANG YES
#elif defined(__GNUC__)
#    undef COMPILER_GCC
#    define COMPILER_GCC YES
#elif defined(_MSC_VER)
#    undef COMPILER_MSVC
#    define COMPILER_MSVC YES
#else
#    error "Unsupported compiler. Please use GCC, Clang, or MSVC."
#endif

//
// OS detection
//

#define OS_WINDOWS NO
#define OS_LINUX NO
#define OS_MACOS NO
#define OS_BSD NO
#define OS_POSIX NO

#if defined(_WIN32) || defined(_WIN64)
#    undef OS_WINDOWS
#    define OS_WINDOWS YES
#elif defined(__linux__)
#    undef OS_LINUX
#    define OS_LINUX YES
#elif defined(__APPLE__) || defined(__MACH__)
#    undef OS_MACOS
#    define OS_MACOS YES
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#    undef OS_BSD
#    define OS_BSD YES
#else
#    error "Unsupported OS. Please use Windows, Linux, macOS, or BSD."
#endif

#if OS_LINUX || OS_MACOS || OS_BSD
#    undef OS_POSIX
#    define OS_POSIX YES
#endif

//
// Architecture detection
//

#define ARCH_X86 NO
#define ARCH_X86_64 NO
#define ARCH_ARM NO
#define ARCH_ARM64 NO

#if defined(__i386__) || defined(_M_IX86)
#    undef ARCH_X86
#    define ARCH_X86 YES
#elif defined(__x86_64__) || defined(_M_X64)
#    undef ARCH_X86_64
#    define ARCH_X86_64 YES
#elif defined(__arm__) || defined(_M_ARM)
#    undef ARCH_ARM
#    define ARCH_ARM YES
#elif defined(__aarch64__) || defined(_M_ARM64)
#    undef ARCH_ARM64
#    define ARCH_ARM64 YES
#else
#    error "Unsupported architecture. Please use x86, x86_64, ARM, or ARM64."
#endif

//
// Build configuration detection
//

#define CONFIG_DEBUG NO
#define CONFIG_RELEASE NO
#define CONFIG_PROFILE NO

#if defined(NDEBUG)
#    undef CONFIG_RELEASE
#    define CONFIG_RELEASE YES
#elif defined(FINAL)
#    undef CONFIG_PROFILE
#    define CONFIG_PROFILE YES
#else
#    undef CONFIG_DEBUG
#    define CONFIG_DEBUG YES
#endif

//
// Debugger support
//

#if COMPILER_MSVC
#    define DEBUG_BREAK() __debugbreak()
#elif COMPILER_GCC || COMPILER_CLANG
#    define DEBUG_BREAK() __builtin_trap()
#else
#    error                                                                     \
        "Unsupported compiler for debug break. Please use GCC, Clang, or MSVC."
#endif

//
// Standard includes
//

#if OS_WINDOWS
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#endif // OS_WINDOWS

#if OS_LINUX
#    include <unistd.h>
#endif // OS_LINUX

//------------------------------------------------------------------------------[Macros]

#define internal static
#define global_variable static
#define local_persist static

#define UNUSED(x) (void)(x)

#define KB(x) ((x) * 1024ull)
#define MB(x) (KB(x) * 1024ull)
#define GB(x) (MB(x) * 1024ull)

#define ALIGN_UP(value, alignment)                                             \
    (((value) + ((alignment) - 1)) & ~((alignment) - 1))

#define ALIGN_PTR_UP(type, ptr, alignment)                                     \
    (type*)ALIGN_UP((usize)(ptr), (alignment))

#define PI 3.14159265358979323846
#define TAU (PI * 2.0)

#define ASSERT(condition, ...)                                                 \
    do {                                                                       \
        if (!(condition)) {                                                    \
            eprn("ASSERTION FAILED: " #condition);                             \
            eprn(__VA_ARGS__);                                                 \
            DEBUG_BREAK();                                                     \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) MAX((min), MIN((x), (max)))

#define DEF_SLICE(type)                                                        \
    typedef struct {                                                           \
        type* data;                                                            \
        usize count;                                                           \
    }

//------------------------------------------------------------------------------[Types]

typedef uint8_t  u8;  // Unsigned 8-bit integer
typedef uint16_t u16; // Unsigned 16-bit integer
typedef uint32_t u32; // Unsigned 32-bit integer
typedef uint64_t u64; // Unsigned 64-bit integer

typedef int8_t  i8;  // Signed 8-bit integer
typedef int16_t i16; // Signed 16-bit integer
typedef int32_t i32; // Signed 32-bit integer
typedef int64_t i64; // Signed 64-bit integer

typedef size_t    usize; // Unsigned size type (platform-dependent)
typedef ptrdiff_t isize; // Signed size type (platform-dependent)

typedef float  f32; // 32-bit floating point
typedef double f64; // 64-bit floating point

typedef const char* cstr; // Constant string type

//------------------------------------------------------------------------------[Library]

void kill(cstr format, ...);

//------------------------------------------------------------------------------[Memory]

void* mem_alloc(usize size, cstr file, int line);
void* mem_realloc(void* ptr, usize size, cstr file, int line);
void* mem_free(void* ptr, cstr file, int line);

usize mem_size(const void* ptr);
void  mem_leak(void* ptr);

void mem_check(void* ptr);

// Memory debugging utilities
#if CONFIG_DEBUG
void  mem_print_leaks(void);
usize mem_get_allocation_count(void);
usize mem_get_total_allocated(void);
#endif // CONFIG_DEBUG

#define KORE_ALLOC(size) mem_alloc((size), __FILE__, __LINE__)
#define KORE_REALLOC(ptr, size) mem_realloc((ptr), (size), __FILE__, __LINE__)
#define KORE_FREE(ptr) ptr = mem_free((ptr), __FILE__, __LINE__), (ptr) = NULL

#define KORE_ARRAY_ALLOC(type, count)                                          \
    (type*)mem_alloc(sizeof(type) * (count), __FILE__, __LINE__)
#define KORE_ARRAY_REALLOC(ptr, type, count)                                   \
    (type*)mem_realloc((ptr), sizeof(type) * (count), __FILE__, __LINE__)
#define KORE_ARRAY_FREE(ptr) KORE_FREE(ptr)

void mem_break_on_alloc(u64 index);

//------------------------------------------------------------------------------[Array]

#define Array(T) T*

typedef struct ArrayHeader_t {
    usize count;
} ArrayHeader;

void* array_maybe_grow(void* array,
                       usize element_size,
                       usize required_capacity,
                       cstr  file,
                       int   line);

// Level 0 accessor macros - assumes that (a) is non-NULL and is a valid array
#define __array_info(a) ((ArrayHeader*)(a) - 1)
#define __array_bytes_capacity(a)                                              \
    (mem_size(__array_info(a)) - sizeof(ArrayHeader))
#define __array_count(a) (__array_info(a)->count)
#define __array_bytes_size(a) (__array_count(a) * sizeof(*(a)))
#define __array_safe(a, op) ((a) ? (op) : 0)

// Level 1 accessor macros - handles a NULL array
#define array_size(a) __array_safe((a), __array_bytes_size(a))
#define array_capacity(a)                                                      \
    __array_safe((a), __array_bytes_capacity(a) / sizeof(*(a)))
#define array_count(a) __array_safe((a), __array_count(a))

#define array_push(a, ...)                                                     \
    do {                                                                       \
        typeof(*(a)) __array_tmp[] = {__VA_ARGS__};                            \
        usize        __array_n = sizeof(__array_tmp) / sizeof(__array_tmp[0]); \
        (a)                    = array_maybe_grow((a),                         \
                               sizeof(*(a)),                \
                               array_count(a) + __array_n,  \
                               __FILE__,                    \
                               __LINE__);                   \
        memcpy((a) + __array_count(a), __array_tmp, __array_n * sizeof(*(a))); \
        __array_count(a) += __array_n;                                         \
    } while (0)

#define array_pop(a) ((a)[__array_count(a)-- - 1])

// Array free function - deallocates array memory
#define array_free(a)                                                          \
    do {                                                                       \
        if ((a)) {                                                             \
            ArrayHeader* header = __array_info(a);                             \
            KORE_FREE(header);                                                 \
            (a) = NULL;                                                        \
        }                                                                      \
    } while (0)

#define array_delete(a, index)                                                 \
    do {                                                                       \
        usize __array_index = (index);                                         \
        if (__array_index < array_count(a)) {                                  \
            memmove(&(a)[__array_index],                                       \
                    &(a)[__array_index + 1],                                   \
                    (array_count(a) - __array_index - 1) * sizeof(*(a)));      \
            __array_count(a)--;                                                \
        }                                                                      \
    } while (0)

#define array_clear(a)                                                         \
    do {                                                                       \
        if (a) {                                                               \
            __array_count(a) = 0;                                              \
        }                                                                      \
    } while (0)

// Array ensure capacity macro
#define array_requires(a, required_capacity)                                   \
    (a) = (typeof(*(a))*)array_maybe_grow(                                     \
        (a), sizeof(*(a)), (required_capacity), __FILE__, __LINE__)

// Ensure capacity incrementally
#define array_needs(a, additional_capacity)                                    \
    (a) = (typeof(*(a))*)array_maybe_grow((a),                                 \
                                          sizeof(*(a)),                        \
                                          array_count(a) +                     \
                                              (additional_capacity),           \
                                          __FILE__,                            \
                                          __LINE__)

// Array ensure size and capacity macro
#define array_reserve(a, required_size)                                        \
    do {                                                                       \
        (a) = (typeof(*(a))*)array_maybe_grow(                                 \
            (a), sizeof(*(a)), (required_size), __FILE__, __LINE__);           \
        __array_count(a) = (required_size);                                    \
    } while (0)

#define array_leak(a) mem_leak(__array_info(a))

//------------------------------------------------------------------------------[Arena]

#define ARENA_DEFAULT_NUM_PAGES_GROW 16

// OS-based arena with reserved memory pages
typedef struct {
    u8*   memory;         // Base pointer to arena - never changes
    usize cursor;         // Current allocation cursor
    usize committed_size; // Number of bytes currently committed
    usize reserved_size;  // Total number of bytes reserved (maximum capacity)
    usize alloc_granularity; // OS allocation granularity (page size)
    usize grow_rate;         // Number of pages to grow by when expanding
} Arena;

// Used to build arrays within an arena
typedef struct {
    Arena* arena;        // Arena being used
    usize  count;        // Number of elements currently in array
    usize  alignment;    // Alignment of each element
    usize  element_size; // Size of each element
    void*  start;        // Start of the array in the arena
} ArenaSession;

//
// Arena Lifetime
//

typedef struct {
    usize reserved_size;
    usize grow_rate;
} ArenaDefaultParams;

void _arena_init(Arena* arena, ArenaDefaultParams params);

#define arena_init(arena, ...)                                                 \
    _arena_init((arena), (ArenaDefaultParams){__VA_ARGS__})

void arena_done(Arena* arena);

//
// Arena allocation
//

void* arena_alloc(Arena* arena, usize size);
void  arena_align(Arena* arena, usize align);
void* arena_alloc_align(Arena* arena, usize size, usize align);

u8*  arena_formatv(Arena* arena, cstr fmt, va_list args);
u8*  arena_format(Arena* arena, cstr fmt, ...);
void arena_null_terminate(Arena* arena);

//
// Arena marks
//

void* arena_store(Arena* arena);
void  arena_restore(Arena* arena, void* mark);
void  arena_reset(Arena* arena);

//
// Arena state
//

u32 arena_offset(Arena* arena, void* p);

//
// Arena sessions
//

void arena_session_init(ArenaSession* session,
                        Arena*        arena,
                        usize         alignment,
                        usize         element_size);
void arena_session_undo(ArenaSession* session);

void* arena_session_alloc(ArenaSession* session, usize count);
usize arena_session_count(ArenaSession* session);
void* arena_session_address(ArenaSession* session);

//------------------------------------------------------------------------------[Mutex]

#if OS_WINDOWS
typedef CRITICAL_SECTION Mutex;
#elif OS_POSIX
#    include <pthread.h>
typedef pthread_mutex_t Mutex;
#else
#    error "Mutex not implemented for this OS."
#endif

void mutex_init(Mutex* mutex);
void mutex_done(Mutex* mutex);
void mutex_lock(Mutex* mutex);
void mutex_unlock(Mutex* mutex);

//------------------------------------------------------------------------------[Output]

void prv(const char* format, va_list args);
void pr(const char* format, ...);
void prn(const char* format, ...);
void eprv(const char* format, va_list args);
void epr(const char* format, ...);
void eprn(const char* format, ...);

#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_FAINT "\033[2m"
#define ANSI_UNDERLINE "\033[4m"
#define ANSI_INVERSED "\033[7m"

#define ANSI_BLACK "\033[30m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"

#define ANSI_BOLD_BLACK "\033[1;30m"
#define ANSI_BOLD_RED "\033[1;31m"
#define ANSI_BOLD_GREEN "\033[1;32m"
#define ANSI_BOLD_YELLOW "\033[1;33m"
#define ANSI_BOLD_BLUE "\033[1;34m"
#define ANSI_BOLD_MAGENTA "\033[1;35m"
#define ANSI_BOLD_CYAN "\033[1;36m"
#define ANSI_BOLD_WHITE "\033[1;37m"

#define ANSI_FAINT_BLACK "\033[2;30m"
#define ANSI_FAINT_RED "\033[2;31m"
#define ANSI_FAINT_GREEN "\033[2;32m"
#define ANSI_FAINT_YELLOW "\033[2;33m"
#define ANSI_FAINT_BLUE "\033[2;34m"
#define ANSI_FAINT_MAGENTA "\033[2;35m"
#define ANSI_FAINT_CYAN "\033[2;36m"
#define ANSI_FAINT_WHITE "\033[2;37m"

#define ANSI_BG_BLACK "\033[40m"
#define ANSI_BG_RED "\033[41m"
#define ANSI_BG_GREEN "\033[42m"
#define ANSI_BG_YELLOW "\033[43m"
#define ANSI_BG_BLUE "\033[44m"
#define ANSI_BG_MAGENTA "\033[45m"
#define ANSI_BG_CYAN "\033[46m"
#define ANSI_BG_WHITE "\033[47m"

#define ANSI_BG_BOLD_BLACK "\033[1;40m"
#define ANSI_BG_BOLD_RED "\033[1;41m"
#define ANSI_BG_BOLD_GREEN "\033[1;42m"
#define ANSI_BG_BOLD_YELLOW "\033[1;43m"
#define ANSI_BG_BOLD_BLUE "\033[1;44m"
#define ANSI_BG_BOLD_MAGENTA "\033[1;45m"
#define ANSI_BG_BOLD_CYAN "\033[1;46m"
#define ANSI_BG_BOLD_WHITE "\033[1;47m"

#define ANSI_BG_FAINT_BLACK "\033[2;40m"
#define ANSI_BG_FAINT_RED "\033[2;41m"
#define ANSI_BG_FAINT_GREEN "\033[2;42m"
#define ANSI_BG_FAINT_YELLOW "\033[2;43m"
#define ANSI_BG_FAINT_BLUE "\033[2;44m"
#define ANSI_BG_FAINT_MAGENTA "\033[2;45m"
#define ANSI_BG_FAINT_CYAN "\033[2;46m"
#define ANSI_BG_FAINT_WHITE "\033[2;47m"

#define UNICODE_TREE_BRANCH "├─ "
#define UNICODE_TREE_LAST_BRANCH "└─ "
#define UNICODE_TREE_VERTICAL "│  "

#define UNICODE_TABLE_TOP_LEFT "┌"
#define UNICODE_TABLE_TOP_RIGHT "┐"
#define UNICODE_TABLE_BOTTOM_LEFT "└"
#define UNICODE_TABLE_BOTTOM_RIGHT "┘"
#define UNICODE_TABLE_HORIZONTAL "─"
#define UNICODE_TABLE_VERTICAL "│"
#define UNICODE_TABLE_T_LEFT "├"
#define UNICODE_TABLE_T_RIGHT "┤"
#define UNICODE_TABLE_T_TOP "┬"
#define UNICODE_TABLE_T_BOTTOM "┴"
#define UNICODE_TABLE_CROSS "┼"

//------------------------------------------------------------------------------[Time]

typedef u64 TimePoint;
typedef u64 TimeDuration;

TimePoint    time_now(void);
TimeDuration time_elapsed(TimePoint start, TimePoint end);
TimePoint    time_add_duration(TimePoint time, TimeDuration duration);

void time_sleep_ms(u32 milliseconds);

u64 time_duration_to_secs(TimeDuration duration);
u64 time_duration_to_ms(TimeDuration duration);
u64 time_duration_to_us(TimeDuration duration);
u64 time_duration_to_ns(TimeDuration duration);

f64 time_secs(TimeDuration duration);

TimeDuration time_from_secs(u64 seconds);
TimeDuration time_from_ms(u64 milliseconds);
TimeDuration time_from_us(u64 microseconds);
TimeDuration time_from_ns(u64 nanoseconds);

//------------------------------------------------------------------------------[Random]

void random_seed(u64 seed);
u64  random_u64(void);
u64  random_range_u64(u64 min, u64 max);
i64  random_range_i64(i64 min, i64 max);

//------------------------------------------------------------------------------[Data]

typedef struct {
    u8*   data;
    usize size;

#if OS_WINDOWS
// Windows-specific file-mapping state
#elif OS_POSIX
// POSIX-specific file-mapping state
#endif
} Data;

bool data_load(cstr path, Data* data);
void data_unload(Data* data);

//------------------------------------------------------------------------------[String]

DEF_SLICE(u8) string;

// Macro to create pr string parameters from a string.
#define STRINGV(s) (int)((s).count), (s).data

// Macro to insert into a format string that needs to be paired with a STRINGV
#define STRINGP "%.*s"

#define SLICE_SET(slice_name, slice_type, ...)                                  \
    static slice_type slice_name##_data[] = {__VA_ARGS__};                      \
    DEF_SLICE(slice_type) slice_name##_t;                                        \
    slice_name##_t slice_name = {                                                \
        slice_name##_data,                                                       \
        sizeof(slice_name##_data) / sizeof(slice_name##_data[0])                 \
    }

//------------------------------------------------------------------------------
// String construction

string string_from_cstr(const char* cstr);
string string_formatv(Arena* arena, cstr fmt, va_list args);
string string_format(Arena* arena, cstr fmt, ...);
string string_from(u8* data, usize size);

//------------------------------------------------------------------------------
// StringBuilder API

typedef struct {
    u8*    data;
    usize  size;
    Arena* arena;
} StringBuilder;

void sb_init(StringBuilder* sb, Arena* arena);
void sb_append_cstr(StringBuilder* sb, cstr str);
void sb_append_string(StringBuilder* sb, string str);
void sb_append_char(StringBuilder* sb, char c);
void sb_append_null(StringBuilder* sb);
void sb_advance(StringBuilder* sb, usize count);
void sb_formatv(StringBuilder* sb, cstr fmt, va_list args);
void sb_format(StringBuilder* sb, cstr fmt, ...);

string sb_to_string(StringBuilder* sb);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
