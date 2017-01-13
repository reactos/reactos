#define STANDALONE
#include <apitest.h>

extern void func_isuncpath(void);

const struct test winetest_testlist[] =
{
    { "PathIsUNC", func_isuncpath },
    { 0, 0 }
};
