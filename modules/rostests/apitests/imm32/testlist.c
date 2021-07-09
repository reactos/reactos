
#define STANDALONE
#include <wine/test.h>

extern void func_imcc(void);

const struct test winetest_testlist[] =
{
    { "imcc", func_imcc },
    { 0, 0 }
};
