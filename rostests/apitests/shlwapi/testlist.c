#define STANDALONE
#include <apitest.h>

extern void func_isuncpath(void);
extern void func_PathUnExpandEnvStrings(void);

const struct test winetest_testlist[] =
{
    { "PathIsUNC", func_isuncpath },
    { "PathUnExpandEnvStrings", func_PathUnExpandEnvStrings },
    { 0, 0 }
};
