#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ANSI colour codes for output
#define TEST_COLOUR_RESET "\033[0m"
#define TEST_COLOUR_RED "\033[31m"
#define TEST_COLOUR_GREEN "\033[32m"
#define TEST_COLOUR_YELLOW "\033[33m"
#define TEST_COLOUR_BLUE "\033[34m"
#define TEST_COLOUR_MAGENTA "\033[35m"
#define TEST_COLOUR_CYAN "\033[36m"
#define TEST_COLOUR_WHITE "\033[37m"
#define TEST_COLOUR_BOLD "\033[1m"

// Test output verbosity control (can be overridden by TEST_VERBOSE environment
// variable)
#ifndef TEST_VERBOSE
#    define TEST_VERBOSE                                                       \
        0 // 0 = quiet (only show failures), 1 = verbose (show all assertions)
#endif

// Test category structure
#define MAX_CATEGORIES 50
#define MAX_CATEGORY_NAME_LENGTH 64

typedef struct {
    char name[MAX_CATEGORY_NAME_LENGTH];
    int  passed_tests;
    int  failed_tests;
    int  total_tests;
} TestCategory;

// Test registry for auto-discovery
#define MAX_REGISTERED_TESTS 500
#define MAX_TEST_NAME_LENGTH 64

typedef struct {
    void (*test_func)(void);
    char category[MAX_CATEGORY_NAME_LENGTH];
    char name[MAX_TEST_NAME_LENGTH];
} RegisteredTest;

// Test filtering options
typedef struct {
    const char* filter_category;
    const char* filter_test;
    int         help_requested;
} TestOptions;

// Test framework macros
#define TEST_ASSERT(condition)                                                 \
    do {                                                                       \
        test_total_assertions++;                                               \
        if (!(condition)) {                                                    \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: %s\n", #condition);                          \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET " %s\n",       \
                   #condition);                                                \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_EQ(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        /* Convert both to the wider type to avoid sign comparison */          \
        long long _cmp_a      = (long long)_test_val_a;                        \
        long long _cmp_b      = (long long)_test_val_b;                        \
        if (_cmp_a != _cmp_b) {                                                \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: %s == %s\n", #a, #b);                        \
            printf("    Values: %lld != %lld\n", _cmp_a, _cmp_b);              \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET " %s == %s\n", \
                   #a,                                                         \
                   #b);                                                        \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_STR_EQ(a, b)                                               \
    do {                                                                       \
        test_total_assertions++;                                               \
        if (strcmp((a), (b)) != 0) {                                           \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: \"%s\" == \"%s\"\n", (a), (b));              \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET                \
                   " \"%s\" == \"%s\"\n",                                      \
                   (a),                                                        \
                   (b));                                                       \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_NULL(ptr)                                                  \
    do {                                                                       \
        test_total_assertions++;                                               \
        if ((ptr) != NULL) {                                                   \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: %s to be NULL\n", #ptr);                     \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET                \
                   " %s is NULL\n",                                            \
                   #ptr);                                                      \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr)                                              \
    do {                                                                       \
        test_total_assertions++;                                               \
        if ((ptr) == NULL) {                                                   \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: %s to be non-NULL\n", #ptr);                 \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET                \
                   " %s is not NULL\n",                                        \
                   #ptr);                                                      \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_GT(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        /* Convert both to the wider type to avoid sign comparison */          \
        long long _cmp_a      = (long long)_test_val_a;                        \
        long long _cmp_b      = (long long)_test_val_b;                        \
        if (_cmp_a <= _cmp_b) {                                                \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: %s > %s\n", #a, #b);                         \
            printf("    Values: %lld <= %lld\n", _cmp_a, _cmp_b);              \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET " %s > %s\n",  \
                   #a,                                                         \
                   #b);                                                        \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_LT(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        /* Convert both to the wider type to avoid sign comparison */          \
        long long _cmp_a      = (long long)_test_val_a;                        \
        long long _cmp_b      = (long long)_test_val_b;                        \
        if (_cmp_a >= _cmp_b) {                                                \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: %s < %s\n", #a, #b);                         \
            printf("    Values: %lld >= %lld\n", _cmp_a, _cmp_b);              \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET " %s < %s\n",  \
                   #a,                                                         \
                   #b);                                                        \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_GE(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        /* Convert both to the wider type to avoid sign comparison */          \
        long long _cmp_a      = (long long)_test_val_a;                        \
        long long _cmp_b      = (long long)_test_val_b;                        \
        if (_cmp_a < _cmp_b) {                                                 \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: %s >= %s\n", #a, #b);                        \
            printf("    Values: %lld < %lld\n", _cmp_a, _cmp_b);               \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET " %s >= %s\n", \
                   #a,                                                         \
                   #b);                                                        \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_LE(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        auto      _test_val_a = (a);                                           \
        auto      _test_val_b = (b);                                           \
        /* Convert both to the wider type to avoid sign comparison */          \
        long long _cmp_a      = (long long)_test_val_a;                        \
        long long _cmp_b      = (long long)_test_val_b;                        \
        if (_cmp_a > _cmp_b) {                                                 \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Expected: %s <= %s\n", #a, #b);                        \
            printf("    Values: %lld > %lld\n", _cmp_a, _cmp_b);               \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET " %s <= %s\n", \
                   #a,                                                         \
                   #b);                                                        \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_MEM_EQ(p1, p2, size)                                       \
    do {                                                                       \
        test_total_assertions++;                                               \
        if (memcmp((p1), (p2), (size)) != 0) {                                 \
            printf("  " TEST_COLOUR_RED "✗ ASSERTION FAILED" TEST_COLOUR_RESET \
                   " at %s:%d\n",                                              \
                   __FILE__,                                                   \
                   __LINE__);                                                  \
            printf("    Memory blocks are not equal for %zu bytes\n", (size)); \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        } else if (test_verbose_output) {                                      \
            printf("  " TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET                \
                   " Memory blocks are equal for %zu bytes\n",                 \
                   (size));                                                    \
        }                                                                      \
    } while (0)

#define TEST_CASE(category, name)                                              \
    void test_##category##_##name(void);                                       \
    __attribute__((constructor)) static void                                   \
    register_test_##category##_##name(void)                                    \
    {                                                                          \
        test_register(test_##category##_##name, #category, #name);             \
    }                                                                          \
    void test_##category##_##name(void)

#define RUN_TEST(category, name)                                               \
    do {                                                                       \
        test_current_failures = 0;                                             \
        test_track_category(#category);                                        \
        if (test_verbose_output) {                                             \
            printf(TEST_COLOUR_CYAN TEST_COLOUR_BOLD                           \
                   "Running test: %s::%s" TEST_COLOUR_RESET "\n",              \
                   #category,                                                  \
                   #name);                                                     \
        }                                                                      \
        test_##category##_##name();                                            \
        if (test_current_failures == 0) {                                      \
            if (test_verbose_output) {                                         \
                printf(TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET " %s::%s\n",    \
                       #category,                                              \
                       #name);                                                 \
            }                                                                  \
            test_passed_tests++;                                               \
            test_update_category_stats(#category, 1, 0);                       \
        } else {                                                               \
            printf(TEST_COLOUR_RED                                             \
                   "✗ %s::%s FAILED (%d assertions failed)" TEST_COLOUR_RESET  \
                   "\n",                                                       \
                   #category,                                                  \
                   #name,                                                      \
                   test_current_failures);                                     \
            test_failed_tests++;                                               \
            test_update_category_stats(#category, 0, 1);                       \
        }                                                                      \
        test_total_tests++;                                                    \
    } while (0)

#ifdef TEST
#    define TEST_SUITE_ENTRY main
#else
#    define TEST_SUITE_ENTRY run
#endif

#define TEST_SUITE_BEGIN()                                                     \
    int TEST_SUITE_ENTRY(int argc, char* argv[])                               \
    {                                                                          \
        TestOptions options = {0};                                             \
        test_parse_args(argc, argv, &options);                                 \
        if (options.help_requested) {                                          \
            test_print_help(argv[0]);                                          \
            return 0;                                                          \
        }                                                                      \
        test_init();                                                           \
        printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE                               \
               "=== Test Suite Started ===" TEST_COLOUR_RESET "\n");

#define RUN_ALL_TESTS()                                                        \
    do {                                                                       \
        for (int i = 0; i < test_registry_count; i++) {                        \
            RegisteredTest* test = &test_registry[i];                          \
            if (test_should_run(test->category, test->name, &options)) {       \
                test_current_failures = 0;                                     \
                test_track_category(test->category);                           \
                if (test_verbose_output) {                                     \
                    printf(TEST_COLOUR_CYAN TEST_COLOUR_BOLD                   \
                           "Running test: %s::%s" TEST_COLOUR_RESET "\n",      \
                           test->category,                                     \
                           test->name);                                        \
                }                                                              \
                test->test_func();                                             \
                if (test_current_failures == 0) {                              \
                    if (test_verbose_output) {                                 \
                        printf(TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET         \
                                                 " %s::%s\n",                  \
                               test->category,                                 \
                               test->name);                                    \
                    }                                                          \
                    test_passed_tests++;                                       \
                    test_update_category_stats(test->category, 1, 0);          \
                } else {                                                       \
                    printf(TEST_COLOUR_RED "✗ %s::%s FAILED (%d assertions "   \
                                           "failed)" TEST_COLOUR_RESET "\n",   \
                           test->category,                                     \
                           test->name,                                         \
                           test_current_failures);                             \
                    test_failed_tests++;                                       \
                    test_update_category_stats(test->category, 0, 1);          \
                }                                                              \
                test_total_tests++;                                            \
            }                                                                  \
        }                                                                      \
    } while (0)

#define TEST_SUITE_END()                                                       \
    if (!test_verbose_output) {                                                \
        test_print_category_summary();                                         \
    }                                                                          \
    printf("\n");                                                              \
    test_summary();                                                            \
    return test_failed_tests > 0 ? 1 : 0;                                      \
    }

// Function declarations
void test_init(void);
void test_summary(void);
void test_track_category(const char* category);
void test_update_category_stats(const char* category, int passed, int failed);
void test_print_category_summary(void);
void test_register(void (*test_func)(void),
                   const char* category,
                   const char* name);
void test_parse_args(int argc, char* argv[], TestOptions* options);
void test_print_help(const char* program_name);
int  test_should_run(const char*        category,
                     const char*        name,
                     const TestOptions* options);

// Global test counters (declared here, defined in implementation)
extern int test_total_tests;
extern int test_passed_tests;
extern int test_failed_tests;
extern int test_total_assertions;
extern int test_total_failures;
extern int test_current_failures;

// Global category tracking
extern TestCategory test_categories[MAX_CATEGORIES];
extern int          test_category_count;

// Global test registry
extern RegisteredTest test_registry[MAX_REGISTERED_TESTS];
extern int            test_registry_count;

// Global verbosity flag (set at runtime from environment variable)
extern int test_verbose_output;
