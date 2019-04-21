#define STANDALONE
#include <apitest.h>

extern void func_button(void);
extern void func_toolbar(void);
extern void func_MRUList(void);

const struct test winetest_testlist[] =
{
    { "buttonv6", func_button },
    { "MRUList", func_MRUList },
    { "toolbarv6", func_toolbar },
    { 0, 0 }
};
