#define STANDALONE
#include <apitest.h>

extern void func_MRUList(void);

const struct test winetest_testlist[] =
{
    { "MRUList", func_MRUList },
    { 0, 0 }
};
