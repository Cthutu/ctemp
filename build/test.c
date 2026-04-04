#include "test.h"

int test_total_tests      = 0;
int test_passed_tests     = 0;
int test_failed_tests     = 0;
int test_total_assertions = 0;
int test_total_failures   = 0;
int test_current_failures = 0;

TestCategory test_categories[MAX_CATEGORIES];
int          test_category_count = 0;

RegisteredTest test_registry[MAX_REGISTERED_TESTS];
int            test_registry_count = 0;

int test_verbose_output            = TEST_VERBOSE;

static const char* test_current_category = NULL;
static const char* test_current_name     = NULL;

static void test_write_json_string(FILE* stream, const char* value)
{
    fputc('"', stream);
    if (value) {
        for (const unsigned char* p = (const unsigned char*)value; *p; ++p) {
            switch (*p) {
            case '\\':
                fputs("\\\\", stream);
                break;
            case '"':
                fputs("\\\"", stream);
                break;
            case '\n':
                fputs("\\n", stream);
                break;
            case '\r':
                fputs("\\r", stream);
                break;
            case '\t':
                fputs("\\t", stream);
                break;
            default:
                if (*p < 0x20) {
                    fprintf(stream, "\\u%04x", *p);
                } else {
                    fputc(*p, stream);
                }
                break;
            }
        }
    }
    fputc('"', stream);
}

void test_register(void (*test_func)(void),
                   const char* category,
                   const char* name)
{
    if (test_registry_count >= MAX_REGISTERED_TESTS) {
        return;
    }

    RegisteredTest* test = &test_registry[test_registry_count];
    test->test_func      = test_func;

    size_t category_len  = strlen(category);
    size_t copy_len      = (category_len < MAX_CATEGORY_NAME_LENGTH - 1)
                               ? category_len
                               : MAX_CATEGORY_NAME_LENGTH - 1;
    memcpy(test->category, category, copy_len);
    test->category[copy_len] = '\0';

    size_t name_len          = strlen(name);
    copy_len                 = (name_len < MAX_TEST_NAME_LENGTH - 1) ? name_len
                                                                     : MAX_TEST_NAME_LENGTH - 1;
    memcpy(test->name, name, copy_len);
    test->name[copy_len] = '\0';

    test_registry_count++;
}

void test_parse_args(int argc, char* argv[], TestOptions* options)
{
    options->filter_scope   = NULL;
    options->filter_name    = NULL;
    options->help_requested = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            options->help_requested = 1;
        } else if (strcmp(argv[i], "--test") == 0 ||
                   strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                char* filter = argv[++i];
                char* colon  = strchr(filter, ':');
                if (colon) {
                    *colon               = '\0';
                    options->filter_scope = filter;
                    options->filter_name  = colon + 1;
                } else {
                    options->filter_scope = filter;
                }
            }
        }
    }
}

void test_print_help(const char* program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -t, --test <filter>     Run only tests matching <category> or "
           "<category>:<name>\n");
}

int test_should_run(const char*        category,
                    const char*        name,
                    const TestOptions* options)
{
    if (options->filter_scope && strcmp(category, options->filter_scope) != 0) {
        return 0;
    }

    if (options->filter_name) {
        return strcmp(name, options->filter_name) == 0;
    }

    return 1;
}

void test_init(void)
{
    test_total_tests      = 0;
    test_passed_tests     = 0;
    test_failed_tests     = 0;
    test_total_assertions = 0;
    test_total_failures   = 0;
    test_current_failures = 0;
    test_category_count   = 0;
    test_current_category = NULL;
    test_current_name     = NULL;

    for (int i = 0; i < MAX_CATEGORIES; i++) {
        test_categories[i].name[0]      = '\0';
        test_categories[i].passed_tests = 0;
        test_categories[i].failed_tests = 0;
        test_categories[i].total_tests  = 0;
    }
}

void test_track_category(const char* category)
{
    for (int i = 0; i < test_category_count; i++) {
        if (strcmp(test_categories[i].name, category) == 0) {
            return;
        }
    }

    if (test_category_count < MAX_CATEGORIES) {
        size_t category_len = strlen(category);
        size_t copy_len     = (category_len < MAX_CATEGORY_NAME_LENGTH - 1)
                                  ? category_len
                                  : MAX_CATEGORY_NAME_LENGTH - 1;
        memcpy(test_categories[test_category_count].name, category, copy_len);
        test_categories[test_category_count].name[copy_len] = '\0';
        test_categories[test_category_count].passed_tests   = 0;
        test_categories[test_category_count].failed_tests   = 0;
        test_categories[test_category_count].total_tests    = 0;
        test_category_count++;
    }
}

void test_update_category_stats(const char* category, int passed, int failed)
{
    for (int i = 0; i < test_category_count; i++) {
        if (strcmp(test_categories[i].name, category) == 0) {
            test_categories[i].passed_tests += passed;
            test_categories[i].failed_tests += failed;
            test_categories[i].total_tests += passed + failed;
            return;
        }
    }
}

void test_print_category_summary(void) {}

void test_record_failure(const char* file,
                         int         line,
                         const char* expected,
                         const char* detail)
{
    fprintf(stderr, "{\"event\":\"assertion_failed\",\"category\":");
    test_write_json_string(stderr, test_current_category);
    fprintf(stderr, ",\"name\":");
    test_write_json_string(stderr, test_current_name);
    fprintf(stderr, ",\"file\":");
    test_write_json_string(stderr, file);
    fprintf(stderr, ",\"line\":%d,\"expected\":", line);
    test_write_json_string(stderr, expected);
    fprintf(stderr, ",\"detail\":");
    test_write_json_string(stderr, detail ? detail : "");
    fputs("}\n", stderr);
    fflush(stderr);
}

int test_selected_count(const TestOptions* options)
{
    int count = 0;
    for (int i = 0; i < test_registry_count; i++) {
        RegisteredTest* test = &test_registry[i];
        if (test_should_run(test->category, test->name, options)) {
            count++;
        }
    }
    return count;
}

void test_emit_suite_start(const TestOptions* options)
{
    fprintf(stdout, "{\"event\":\"suite_start\",\"selected_tests\":%d}\n",
            test_selected_count(options));
    fflush(stdout);
}

void test_begin_test(const char* category, const char* name)
{
    test_current_failures = 0;
    test_current_category = category;
    test_current_name     = name;
    test_track_category(category);

    fprintf(stdout, "{\"event\":\"test_start\",\"category\":");
    test_write_json_string(stdout, category);
    fprintf(stdout, ",\"name\":");
    test_write_json_string(stdout, name);
    fputs("}\n", stdout);
    fflush(stdout);
}

void test_end_test(const char* category, const char* name)
{
    int passed = test_current_failures == 0;

    if (passed) {
        test_passed_tests++;
        test_update_category_stats(category, 1, 0);
    } else {
        test_failed_tests++;
        test_update_category_stats(category, 0, 1);
    }
    test_total_tests++;

    fprintf(stdout, "{\"event\":\"test_end\",\"category\":");
    test_write_json_string(stdout, category);
    fprintf(stdout, ",\"name\":");
    test_write_json_string(stdout, name);
    fprintf(stdout, ",\"passed\":%s,\"failures\":%d}\n",
            passed ? "true" : "false",
            test_current_failures);
    fflush(stdout);
}

void test_emit_summary(void)
{
    fprintf(stdout,
            "{\"event\":\"suite_end\",\"total_tests\":%d,"
            "\"passed_tests\":%d,\"failed_tests\":%d,"
            "\"total_assertions\":%d,\"failed_assertions\":%d,"
            "\"categories\":[",
            test_total_tests,
            test_passed_tests,
            test_failed_tests,
            test_total_assertions,
            test_total_failures);

    for (int i = 0; i < test_category_count; i++) {
        if (i > 0) {
            fputc(',', stdout);
        }
        TestCategory* cat = &test_categories[i];
        fputs("{\"name\":", stdout);
        test_write_json_string(stdout, cat->name);
        fprintf(stdout,
                ",\"tests\":%d,\"passed\":%d,\"failed\":%d}",
                cat->total_tests,
                cat->passed_tests,
                cat->failed_tests);
    }

    fputs("]}\n", stdout);
    fflush(stdout);
}

void test_summary(void) { test_emit_summary(); }

TEST_SUITE_BEGIN()
RUN_ALL_TESTS();
TEST_SUITE_END()
