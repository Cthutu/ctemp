//------------------------------------------------------------------------------
// Random number generation API implementation
//
// Copyright (C)2026 Matt Davies, all rights reserved
//------------------------------------------------------------------------------

#include <core/core.h>

global_variable u64 g_random_state = 0;

//------------------------------------------------------------------------------

internal inline u64 random_step(void)
{
    if (g_random_state == 0) {
        random_seed(time_now() | 1ull);
    }

    u64 x = g_random_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    g_random_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

void random_seed(u64 seed)
{
    if (seed == 0) {
        seed = 0x9e3779b97f4a7c15ull;
    }
    g_random_state = seed;
}

u64 random_u64(void) { return random_step(); }

u64 random_range_u64(u64 min, u64 max)
{
    ASSERT(min <= max, "random_range_u64: min must be <= max");
    u64 span = max - min + 1ull;
    return min + (random_step() % span);
}

i64 random_range_i64(i64 min, i64 max)
{
    ASSERT(min <= max, "random_range_i64: min must be <= max");
    u64 span = (u64)(max - min) + 1ull;
    return min + (i64)(random_step() % span);
}
