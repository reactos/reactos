#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_usp10(void);


const struct test winetest_testlist[] =
{
    { "usp10", func_usp10 },
    { 0, 0 }
};