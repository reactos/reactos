#define STANDALONE
#include <apitest.h>

extern void func_isuncpath(void);
extern void func_isuncpathserver(void);
extern void func_isuncpathservershare(void);
extern void func_PathUnExpandEnvStrings(void);

const struct test winetest_testlist[] =
{
    { "PathIsUNC", func_isuncpath },
    { "PathIsUNCServer", func_isuncpathserver },
    { "PathIsUNCServerShare", func_isuncpathservershare },
    { "PathUnExpandEnvStrings", func_PathUnExpandEnvStrings },
    { 0, 0 }
};
