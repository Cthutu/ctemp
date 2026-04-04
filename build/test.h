#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TEST_VERBOSE
#    define TEST_VERBOSE 0
#endif

#define MAX_CATEGORIES 50
#define MAX_CATEGORY_NAME_LENGTH 64

typedef struct {
    char name[MAX_CATEGORY_NAME_LENGTH];
    int  passed_tests;
    int  failed_tests;
    int  total_tests;
} TestCategory;

#define MAX_REGISTERED_TESTS 500
#define MAX_TEST_NAME_LENGTH 64

typedef struct {
    void (*test_func)(void);
    char category[MAX_CATEGORY_NAME_LENGTH];
    char name[MAX_TEST_NAME_LENGTH];
} RegisteredTest;

typedef struct {
    const char* filter_scope;
    const char* filter_name;
    int         help_requested;
} TestOptions;

#define TEST_ASSERT(condition)                                                 \
    do {                                                                       \
        test_total_assertions++;                                               \
        if (!(condition)) {                                                    \
            test_record_failure(__FILE__, __LINE__, #condition, NULL);         \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_EQ(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        long long   _cmp_a      = (long long)_test_val_a;                      \
        long long   _cmp_b      = (long long)_test_val_b;                      \
        if (_cmp_a != _cmp_b) {                                                \
            char _detail[128];                                                 \
            snprintf(_detail, sizeof(_detail), "%lld != %lld", _cmp_a, _cmp_b);\
            test_record_failure(__FILE__, __LINE__, #a " == " #b, _detail);    \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_STR_EQ(a, b)                                               \
    do {                                                                       \
        test_total_assertions++;                                               \
        if (strcmp((a), (b)) != 0) {                                           \
            char _detail[256];                                                 \
            snprintf(_detail, sizeof(_detail), "\"%s\" != \"%s\"", (a), (b)); \
            test_record_failure(__FILE__, __LINE__, #a " == " #b, _detail);    \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_NULL(ptr)                                                  \
    do {                                                                       \
        test_total_assertions++;                                               \
        if ((ptr) != NULL) {                                                   \
            test_record_failure(__FILE__, __LINE__, #ptr " == NULL", NULL);    \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr)                                              \
    do {                                                                       \
        test_total_assertions++;                                               \
        if ((ptr) == NULL) {                                                   \
            test_record_failure(__FILE__, __LINE__, #ptr " != NULL", NULL);    \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_GT(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        long long   _cmp_a      = (long long)_test_val_a;                      \
        long long   _cmp_b      = (long long)_test_val_b;                      \
        if (_cmp_a <= _cmp_b) {                                                \
            char _detail[128];                                                 \
            snprintf(_detail, sizeof(_detail), "%lld <= %lld", _cmp_a, _cmp_b);\
            test_record_failure(__FILE__, __LINE__, #a " > " #b, _detail);     \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_LT(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        long long   _cmp_a      = (long long)_test_val_a;                      \
        long long   _cmp_b      = (long long)_test_val_b;                      \
        if (_cmp_a >= _cmp_b) {                                                \
            char _detail[128];                                                 \
            snprintf(_detail, sizeof(_detail), "%lld >= %lld", _cmp_a, _cmp_b);\
            test_record_failure(__FILE__, __LINE__, #a " < " #b, _detail);     \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_GE(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        long long   _cmp_a      = (long long)_test_val_a;                      \
        long long   _cmp_b      = (long long)_test_val_b;                      \
        if (_cmp_a < _cmp_b) {                                                 \
            char _detail[128];                                                 \
            snprintf(_detail, sizeof(_detail), "%lld < %lld", _cmp_a, _cmp_b);\
            test_record_failure(__FILE__, __LINE__, #a " >= " #b, _detail);    \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_LE(a, b)                                                   \
    do {                                                                       \
        test_total_assertions++;                                               \
        __auto_type _test_val_a = (a);                                         \
        __auto_type _test_val_b = (b);                                         \
        long long   _cmp_a      = (long long)_test_val_a;                      \
        long long   _cmp_b      = (long long)_test_val_b;                      \
        if (_cmp_a > _cmp_b) {                                                 \
            char _detail[128];                                                 \
            snprintf(_detail, sizeof(_detail), "%lld > %lld", _cmp_a, _cmp_b);\
            test_record_failure(__FILE__, __LINE__, #a " <= " #b, _detail);    \
            test_current_failures++;                                           \
            test_total_failures++;                                             \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_MEM_EQ(p1, p2, size)                                       \
    do {                                                                       \
        test_total_assertions++;                                               \
        if (memcmp((p1), (p2), (size)) != 0) {                                 \
            char _detail[128];                                                 \
            snprintf(_detail, sizeof(_detail),                                 \
                     "memory blocks differ for %zu bytes", (size_t)(size));    \
            test_record_failure(__FILE__, __LINE__, "memory equality", _detail);\
            test_current_failures++;                                           \
            test_total_failures++;                                             \
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
        test_emit_suite_start(&options);

#define RUN_ALL_TESTS()                                                        \
    do {                                                                       \
        for (int i = 0; i < test_registry_count; i++) {                        \
            RegisteredTest* test = &test_registry[i];                          \
            if (test_should_run(test->category, test->name, &options)) {       \
                test_begin_test(test->category, test->name);                   \
                test->test_func();                                             \
                test_end_test(test->category, test->name);                     \
            }                                                                  \
        }                                                                      \
    } while (0)

#define TEST_SUITE_END()                                                       \
    test_emit_summary();                                                       \
    return test_failed_tests > 0 ? 1 : 0;                                      \
    }

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
void test_record_failure(const char* file,
                         int         line,
                         const char* expected,
                         const char* detail);
void test_emit_suite_start(const TestOptions* options);
void test_begin_test(const char* category, const char* name);
void test_end_test(const char* category, const char* name);
void test_emit_summary(void);
int  test_selected_count(const TestOptions* options);

extern int test_total_tests;
extern int test_passed_tests;
extern int test_failed_tests;
extern int test_total_assertions;
extern int test_total_failures;
extern int test_current_failures;

extern TestCategory test_categories[MAX_CATEGORIES];
extern int          test_category_count;

extern RegisteredTest test_registry[MAX_REGISTERED_TESTS];
extern int            test_registry_count;

extern int test_verbose_output;
