#define STANDALONE
#include <apitest.h>

extern void func_DisplayModes(void);

const struct test winetest_testlist[] =
{
    { "DisplayModes", func_DisplayModes },
    { 0, 0 }
};
