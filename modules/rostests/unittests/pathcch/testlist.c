#define STANDALONE
#include <apitest.h>

extern void func_PathCchCompileTest(void);

const struct test winetest_testlist[] =
{
    { "PathCchCompileTest", func_PathCchCompileTest },
    { 0, 0 }
};
