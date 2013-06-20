#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_cache(void);
extern void func_name(void);

const struct test winetest_testlist[] =
{
    { "cache", func_cache },
    { "name", func_name },
    { 0, 0 }
};
