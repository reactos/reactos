#define STANDALONE
#include <apitest.h>

extern void func_load(void);

const struct test winetest_testlist[] =
{
    { "load", func_load },
    { 0, 0 }
};
