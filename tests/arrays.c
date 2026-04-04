//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(array, empty)
{
    Array(int) arr = 0;

    TEST_ASSERT_EQ(array_count(arr), 0);
}
