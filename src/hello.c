//> use: core

#include <core/core.h>

int run(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    prn(ANSI_RED "Hello" ANSI_RESET ", " ANSI_GREEN "World!" ANSI_RESET);

    return 0;
}
