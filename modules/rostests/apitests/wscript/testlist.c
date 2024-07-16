#define STANDALONE
#include <apitest.h>

extern void func_wsf(void);

const struct test winetest_testlist[] =
{
    { "wsf", func_wsf },
    { 0, 0 }
};
