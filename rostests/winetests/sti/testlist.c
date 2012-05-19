#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_sti(void);

const struct test winetest_testlist[] =
{
    { "sti", func_sti },
    { 0, 0 }
};
