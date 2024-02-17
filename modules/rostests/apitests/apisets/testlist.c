
#define STANDALONE
#include <apitest.h>

extern void func_apisets(void);

const struct test winetest_testlist[] =
{
    { "apisets", func_apisets },
    { 0, 0 }
};
