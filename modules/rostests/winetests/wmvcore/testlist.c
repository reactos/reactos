#define STANDALONE
#include <wine/test.h>

extern void func_wmvcore(void);

const struct test winetest_testlist[] =
{
    { "wmvcore", func_wmvcore },
    { 0, 0 }
};
