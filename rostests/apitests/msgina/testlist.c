#define STANDALONE
#include <apitest.h>

extern void func_ShellDimScreen(void);

const struct test winetest_testlist[] =
{
    { "ShellDimScreen", func_ShellDimScreen },
    { 0, 0 }
};
