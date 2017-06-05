#define STANDALONE
#include <apitest.h>

extern void func_button(void);
extern void func_toolbar(void);

const struct test winetest_testlist[] =
{
    { "buttonv6", func_button },
    { "toolbarv6", func_toolbar },
    { 0, 0 }
};
