#define STANDALONE
#include <apitest.h>

extern void func_isvalidname(void);

const struct test winetest_testlist[] =
{
    { "NcIsValidConnectionName", func_isvalidname },
    { 0, 0 }
};
