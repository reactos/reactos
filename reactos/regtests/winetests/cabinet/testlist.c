#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_extract(void);

const struct test winetest_testlist[] =
{
    { "extract", func_extract },
    { 0, 0 }
};
