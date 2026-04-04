//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(time, unit_conversions_round_trip)
{
    TEST_ASSERT_EQ(time_from_secs(2), 2000000000ull);
    TEST_ASSERT_EQ(time_from_ms(3), 3000000ull);
    TEST_ASSERT_EQ(time_from_us(4), 4000ull);
    TEST_ASSERT_EQ(time_from_ns(5), 5ull);

    TEST_ASSERT_EQ(time_duration_to_secs(time_from_secs(2)), 2);
    TEST_ASSERT_EQ(time_duration_to_ms(time_from_ms(3)), 3);
    TEST_ASSERT_EQ(time_duration_to_us(time_from_us(4)), 4);
    TEST_ASSERT_EQ(time_duration_to_ns(time_from_ns(5)), 5);
}

TEST_CASE(time, elapsed_and_add_duration)
{
    TimePoint start = time_now();
    TimePoint end   = time_add_duration(start, time_from_ms(5));

    TEST_ASSERT_EQ(time_elapsed(start, end), time_from_ms(5));
    TEST_ASSERT_GE(end, start);
}
