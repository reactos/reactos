#define STANDALONE
#include <apitest.h>

extern void func_MRUList(void);
extern void func_WinList(void);

const struct test winetest_testlist[] =
{
    { "MRUList", func_MRUList },
    { "WinList", func_WinList },
    { 0, 0 }
};
