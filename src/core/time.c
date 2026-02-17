//------------------------------------------------------------------------------
// Time API implementation
//
// Copyright (C)2026 Matt Davies, all rights reserved
//------------------------------------------------------------------------------

#include <core/core.h>

//------------------------------------------------------------------------------

#if OS_WINDOWS

static u64 time_frequency(void)
{
    local_persist u64 frequency = 0;
    if (frequency == 0) {
        LARGE_INTEGER value;
        QueryPerformanceFrequency(&value);
        frequency = (u64)value.QuadPart;
    }
    return frequency;
}

TimePoint time_now(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (TimePoint)counter.QuadPart;
}

TimeDuration time_elapsed(TimePoint start, TimePoint end)
{
    return (TimeDuration)(end - start);
}

TimePoint time_add_duration(TimePoint time, TimeDuration duration)
{
    return time + duration;
}

void time_sleep_ms(u32 milliseconds) { Sleep(milliseconds); }

u64 time_duration_to_secs(TimeDuration duration)
{
    u64 frequency = time_frequency();
    return duration / frequency;
}

u64 time_duration_to_ms(TimeDuration duration)
{
    u64 frequency = time_frequency();
    return (duration * 1000) / frequency;
}

u64 time_duration_to_us(TimeDuration duration)
{
    u64 frequency = time_frequency();
    return (duration * 1000000) / frequency;
}

u64 time_duration_to_ns(TimeDuration duration)
{
    u64 frequency = time_frequency();
    return (duration * 1000000000) / frequency;
}

f64 time_secs(TimeDuration duration)
{
    return (f64)duration / (f64)time_frequency();
}

TimeDuration time_from_secs(u64 seconds)
{
    return (TimeDuration)(seconds * time_frequency());
}

TimeDuration time_from_ms(u64 milliseconds)
{
    return (TimeDuration)((milliseconds * time_frequency()) / 1000);
}

TimeDuration time_from_us(u64 microseconds)
{
    return (TimeDuration)((microseconds * time_frequency()) / 1000000);
}

TimeDuration time_from_ns(u64 nanoseconds)
{
    return (TimeDuration)((nanoseconds * time_frequency()) / 1000000000);
}

#elif OS_POSIX

static inline TimePoint _time_now_raw(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (TimePoint)ts.tv_sec * 1000000000ull + (TimePoint)ts.tv_nsec;
}

TimePoint time_now(void) { return _time_now_raw(); }

TimeDuration time_elapsed(TimePoint start, TimePoint end)
{
    return (TimeDuration)(end - start);
}

TimePoint time_add_duration(TimePoint time, TimeDuration duration)
{
    return time + duration;
}

void time_sleep_ms(u32 milliseconds)
{
    struct timespec req;
    req.tv_sec  = (time_t)(milliseconds / 1000);
    req.tv_nsec = (long)((milliseconds % 1000) * 1000000ul);
    nanosleep(&req, NULL);
}

u64 time_duration_to_secs(TimeDuration duration)
{
    return duration / 1000000000ull;
}

u64 time_duration_to_ms(TimeDuration duration) { return duration / 1000000ull; }

u64 time_duration_to_us(TimeDuration duration) { return duration / 1000ull; }

u64 time_duration_to_ns(TimeDuration duration) { return duration; }

f64 time_secs(TimeDuration duration) { return (f64)duration / 1000000000.0; }

TimeDuration time_from_secs(u64 seconds) { return seconds * 1000000000ull; }

TimeDuration time_from_ms(u64 milliseconds)
{
    return milliseconds * 1000000ull;
}

TimeDuration time_from_us(u64 microseconds) { return microseconds * 1000ull; }

TimeDuration time_from_ns(u64 nanoseconds) { return nanoseconds; }

#endif // OS_WINDOWS
