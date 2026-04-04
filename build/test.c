#include "test.h"

// Global test counters
int test_total_tests      = 0;
int test_passed_tests     = 0;
int test_failed_tests     = 0;
int test_total_assertions = 0;
int test_total_failures   = 0;
int test_current_failures = 0;

// Global category tracking
TestCategory test_categories[MAX_CATEGORIES];
int          test_category_count = 0;

// Global test registry
RegisteredTest test_registry[MAX_REGISTERED_TESTS];
int            test_registry_count = 0;

// Global verbosity flag (set at runtime from environment variable)
int test_verbose_output            = TEST_VERBOSE;

void test_register(void (*test_func)(void),
                   const char* category,
                   const char* name)
{
    if (test_registry_count >= MAX_REGISTERED_TESTS) {
        return; // Registry full
    }

    RegisteredTest* test = &test_registry[test_registry_count];
    test->test_func      = test_func;

    // Safely copy category name
    size_t category_len  = strlen(category);
    size_t copy_len      = (category_len < MAX_CATEGORY_NAME_LENGTH - 1)
                               ? category_len
                               : MAX_CATEGORY_NAME_LENGTH - 1;
    memcpy(test->category, category, copy_len);
    test->category[copy_len] = '\0';

    // Safely copy test name
    size_t name_len          = strlen(name);
    copy_len                 = (name_len < MAX_TEST_NAME_LENGTH - 1) ? name_len
                                                                     : MAX_TEST_NAME_LENGTH - 1;
    memcpy(test->name, name, copy_len);
    test->name[copy_len] = '\0';

    test_registry_count++;
}

void test_parse_args(int argc, char* argv[], TestOptions* options)
{
    options->filter_category = NULL;
    options->filter_test     = NULL;
    options->help_requested  = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            options->help_requested = 1;
        } else if (strcmp(argv[i], "--category") == 0 ||
                   strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) {
                options->filter_category = argv[++i];
            }
        } else if (strcmp(argv[i], "--test") == 0 ||
                   strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                options->filter_test = argv[++i];
            }
        }
    }
}

void test_print_help(const char* program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf(
        "  -c, --category <name>   Run only tests in the specified category\n");
    printf("  -t, --test <name>       Run only the test with the specified "
           "name\n");
    printf("\n");
    printf("Environment Variables:\n");
    printf("  TEST_VERBOSE=1          Enable verbose output (show all "
           "assertions)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                      Run all tests\n", program_name);
    printf("  %s -c memory            Run only memory tests\n", program_name);
    printf("  %s -t simple            Run only the 'simple' test\n",
           program_name);
    printf("  TEST_VERBOSE=1 %s       Run all tests with verbose output\n",
           program_name);
}

int test_should_run(const char*        category,
                    const char*        name,
                    const TestOptions* options)
{
    // If a specific test is requested, check if this is it
    if (options->filter_test) {
        return strcmp(name, options->filter_test) == 0;
    }

    // If a category filter is set, check if this test matches
    if (options->filter_category) {
        return strcmp(category, options->filter_category) == 0;
    }

    // No filters, run all tests
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

    // Initialise categories
    for (int i = 0; i < MAX_CATEGORIES; i++) {
        test_categories[i].name[0]      = '\0';
        test_categories[i].passed_tests = 0;
        test_categories[i].failed_tests = 0;
        test_categories[i].total_tests  = 0;
    }

    // Check for TEST_VERBOSE environment variable
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4996) // Suppress deprecated function warning
#endif
    const char* verbose_env = getenv("TEST_VERBOSE");
#ifdef _MSC_VER
#    pragma warning(pop)
#endif
    if (verbose_env != NULL) {
        test_verbose_output = (strcmp(verbose_env, "1") == 0 ||
                               strcmp(verbose_env, "true") == 0 ||
                               strcmp(verbose_env, "yes") == 0);
    } else {
        // Fall back to compile-time default
        test_verbose_output = TEST_VERBOSE;
    }
}

void test_track_category(const char* category)
{
    // Check if category already exists
    for (int i = 0; i < test_category_count; i++) {
        if (strcmp(test_categories[i].name, category) == 0) {
            return; // Category already exists
        }
    }

    // Add new category if we have space
    if (test_category_count < MAX_CATEGORIES) {
        // Safer string copy
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
            test_categories[i].total_tests += (passed + failed);
            return;
        }
    }
}

void test_print_category_summary(void)
{
    if (test_category_count == 0) {
        return; // No categories to display
    }

    printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE
           "\n=== Test Categories Summary ===" TEST_COLOUR_RESET "\n");

    for (int i = 0; i < test_category_count; i++) {
        TestCategory* cat = &test_categories[i];
        if (cat->failed_tests > 0) {
            printf(TEST_COLOUR_RED "✗" TEST_COLOUR_RESET " %s: %d/%d passed\n",
                   cat->name,
                   cat->passed_tests,
                   cat->total_tests);
        } else {
            printf(TEST_COLOUR_GREEN "✓" TEST_COLOUR_RESET
                                     " %s: %d/%d passed\n",
                   cat->name,
                   cat->passed_tests,
                   cat->total_tests);
        }
    }
}

void test_summary(void)
{
    // Calculate column widths for proper alignment
    const int label_width = 20;
    const int value_width = 10;
    const int total_width =
        label_width + value_width + 1; // +1 for separator inside

    // Top border
    printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE "┌");
    for (int i = 0; i < total_width; i++) {
        printf("─");
    }
    printf("┐" TEST_COLOUR_RESET "\n");

    // Header
    printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE "│");
    int header_padding =
        (total_width - 18) / 2; // "Test Suite Summary" is 18 chars
    for (int i = 0; i < header_padding; i++) {
        printf(" ");
    }
    printf("Test Suite Summary");
    for (int i = 0; i < total_width - header_padding - 18; i++) {
        printf(" ");
    }
    printf("│" TEST_COLOUR_RESET "\n");

    // Separator
    printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE "├");
    for (int i = 0; i < label_width; i++) {
        printf("─");
    }
    printf("┬");
    for (int i = 0; i < value_width; i++) {
        printf("─");
    }
    printf("┤" TEST_COLOUR_RESET "\n");

    // Column headers
    printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE "│ %-*s │ %-*s │" TEST_COLOUR_RESET
                                             "\n",
           label_width - 2,
           "Metric",
           value_width - 2,
           "Value");

    // Separator
    printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE "├");
    for (int i = 0; i < label_width; i++) {
        printf("─");
    }
    printf("┼");
    for (int i = 0; i < value_width; i++) {
        printf("─");
    }
    printf("┤" TEST_COLOUR_RESET "\n");

    // Data rows
    printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET " %-*s " TEST_COLOUR_BLUE
                            "│" TEST_COLOUR_RESET " %*d " TEST_COLOUR_BLUE
                            "│" TEST_COLOUR_RESET "\n",
           label_width - 2,
           "Total tests run",
           value_width - 2,
           test_total_tests);

    printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET " %-*s " TEST_COLOUR_BLUE
                            "│" TEST_COLOUR_RESET " " TEST_COLOUR_GREEN
                            "%*d" TEST_COLOUR_RESET " " TEST_COLOUR_BLUE
                            "│" TEST_COLOUR_RESET "\n",
           label_width - 2,
           "Tests passed",
           value_width - 2,
           test_passed_tests);

    if (test_failed_tests > 0) {
        printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET " %-*s " TEST_COLOUR_BLUE
                                "│" TEST_COLOUR_RESET " " TEST_COLOUR_RED
                                "%*d" TEST_COLOUR_RESET " " TEST_COLOUR_BLUE
                                "│" TEST_COLOUR_RESET "\n",
               label_width - 2,
               "Tests failed",
               value_width - 2,
               test_failed_tests);
    }

    printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET " %-*s " TEST_COLOUR_BLUE
                            "│" TEST_COLOUR_RESET " %*d " TEST_COLOUR_BLUE
                            "│" TEST_COLOUR_RESET "\n",
           label_width - 2,
           "Total assertions",
           value_width - 2,
           test_total_assertions);

    if (test_total_failures > 0) {
        printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET " %-*s " TEST_COLOUR_BLUE
                                "│" TEST_COLOUR_RESET " " TEST_COLOUR_RED
                                "%*d" TEST_COLOUR_RESET " " TEST_COLOUR_BLUE
                                "│" TEST_COLOUR_RESET "\n",
               label_width - 2,
               "Failed assertions",
               value_width - 2,
               test_total_failures);
    }

    // Status section separator
    printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE "├");
    for (int i = 0; i < label_width; i++) {
        printf("─");
    }
    printf("┴");
    for (int i = 0; i < value_width; i++) {
        printf("─");
    }
    printf("┤" TEST_COLOUR_RESET "\n");

    // Status message
    if (test_failed_tests == 0) {
        const char* status_msg = "🎉 ALL TESTS PASSED! 🎉";
        int         status_len =
            23; // Length without emoji (emojis count as different widths)
        int status_padding_left = (total_width - status_len) / 2;
        int status_padding_right =
            total_width - status_len - status_padding_left;

        printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET);
        for (int i = 0; i < status_padding_left; i++) {
            printf(" ");
        }
        printf(TEST_COLOUR_GREEN TEST_COLOUR_BOLD "%s" TEST_COLOUR_RESET,
               status_msg);
        for (int i = 0; i < status_padding_right; i++) {
            printf(" ");
        }
        printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET "\n");
    } else {
        const char* status_msg          = "❌ SOME TESTS FAILED ❌";
        int         status_len          = 23; // Length without emoji
        int         status_padding_left = (total_width - status_len) / 2;
        int         status_padding_right =
            total_width - status_len - status_padding_left;

        printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET);
        for (int i = 0; i < status_padding_left; i++) {
            printf(" ");
        }
        printf(TEST_COLOUR_RED TEST_COLOUR_BOLD "%s" TEST_COLOUR_RESET,
               status_msg);
        for (int i = 0; i < status_padding_right; i++) {
            printf(" ");
        }
        printf(TEST_COLOUR_BLUE "│" TEST_COLOUR_RESET "\n");
    }

    // Bottom border
    printf(TEST_COLOUR_BOLD TEST_COLOUR_BLUE "└");
    for (int i = 0; i < total_width; i++) {
        printf("─");
    }
    printf("┘" TEST_COLOUR_RESET "\n");
}

TEST_SUITE_BEGIN()
RUN_ALL_TESTS();
TEST_SUITE_END()
