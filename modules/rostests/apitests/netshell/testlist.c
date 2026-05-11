#define STANDALONE
#include <apitest.h>

extern void func_NcIsValidConnectionName(void);

const struct test winetest_testlist[] =
{
    { "NcIsValidConnectionName", func_NcIsValidConnectionName },
    { 0, 0 }
};
