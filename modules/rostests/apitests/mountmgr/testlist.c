#define STANDALONE
#include <apitest.h>

extern void func_QueryPoints(void);

const struct test winetest_testlist[] =
{
    { "QueryPoints", func_QueryPoints },
    { 0, 0 }
};

