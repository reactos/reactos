#define STANDALONE
#include <apitest.h>

extern void func_zipfldr(void);

const struct test winetest_testlist[] =
{
    { "zipfldr", func_zipfldr },
    { 0, 0 }
};
