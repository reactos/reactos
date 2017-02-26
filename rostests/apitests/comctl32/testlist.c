#define STANDALONE
#include <apitest.h>

extern void func_button(void);

const struct test winetest_testlist[] =
{
    { "buttonv6", func_button },
    { 0, 0 }
};
