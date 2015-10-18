#define STANDALONE
#include <apitest.h>

extern void func_CComHeapPtr(void);

const struct test winetest_testlist[] =
{
    { "CComHeapPtr", func_CComHeapPtr },
    { 0, 0 }
};
