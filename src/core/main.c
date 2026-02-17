//------------------------------------------------------------------------------
// Main entry point
//
// Copyright (C)2026 Matt Davies, all rights reserved
//------------------------------------------------------------------------------

#include <core/core.h>

extern Mutex g_kore_output_mutex;

//------------------------------------------------------------------------------

int run(int argc, char** argv);

int main(int argc, char** argv)
{
    mutex_init(&g_kore_output_mutex);

#if OS_WINDOWS
    UINT old_cp = GetConsoleCP();
    SetConsoleCP(CP_UTF8);
    UINT old_output_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
#endif // OS_WINDOWS

    int result = run(argc, argv);

#if OS_WINDOWS
    SetConsoleCP(old_cp);
    SetConsoleOutputCP(old_output_cp);
#endif // OS_WINDOWS

#if CONFIG_DEBUG
    mem_print_leaks();
#endif // CONFIG_DEBUG
    mutex_done(&g_kore_output_mutex);
    return result;
}

void kill(cstr format, ...)
{
    va_list args;
    va_start(args, format);
    eprv(format, args);
    va_end(args);
    epr("\n");
    abort();
}
