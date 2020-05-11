
#define STANDALONE
#include <apitest.h>

extern void func_stacktrace(void);

const struct test winetest_testlist[] =
{
    { "stacktrace",                     func_stacktrace },

    { 0, 0 }
};
