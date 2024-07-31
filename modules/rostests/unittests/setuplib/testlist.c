#define STANDALONE
#include <apitest.h>

extern void func_IsValidInstallDirectory(void);

const struct test winetest_testlist[] =
{
    { "IsValidInstallDirectory", func_IsValidInstallDirectory },
    { 0, 0 }
};
