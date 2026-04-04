//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(random, seed_repeats_sequence)
{
    random_seed(12345);
    u64 first_a  = random_u64();
    u64 second_a = random_u64();

    random_seed(12345);
    u64 first_b  = random_u64();
    u64 second_b = random_u64();

    TEST_ASSERT_EQ(first_a, first_b);
    TEST_ASSERT_EQ(second_a, second_b);
}

TEST_CASE(random, ranges_stay_bounded)
{
    random_seed(67890);

    for (int i = 0; i < 32; i++) {
        u64 value_u = random_range_u64(10, 20);
        i64 value_i = random_range_i64(-5, 5);

        TEST_ASSERT_GE(value_u, 10);
        TEST_ASSERT_LE(value_u, 20);
        TEST_ASSERT_GE(value_i, -5);
        TEST_ASSERT_LE(value_i, 5);
    }
}
