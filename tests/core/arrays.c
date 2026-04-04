//> use: core

#include <core/core.h>
#include <test.h>

TEST_CASE(array, empty)
{
    Array(int) arr = 0;

    TEST_ASSERT_EQ(array_count(arr), 0);
}

TEST_CASE(array, push_pop)
{
    Array(int) arr = 0;

    array_push(arr, 1);
    array_push(arr, 2);
    array_push(arr, 3);

    TEST_ASSERT_EQ(array_count(arr), 3);
    TEST_ASSERT_EQ(arr[0], 1);
    TEST_ASSERT_EQ(arr[1], 2);
    TEST_ASSERT_EQ(arr[2], 3);

    int popped = array_pop(arr);
    TEST_ASSERT_EQ(popped, 3);
    TEST_ASSERT_EQ(array_count(arr), 2);

    array_free(arr);
    TEST_ASSERT_EQ(arr, NULL);
}
