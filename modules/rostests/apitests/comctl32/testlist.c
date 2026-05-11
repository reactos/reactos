#define STANDALONE
#include <apitest.h>

extern void func_buttonv6(void);
extern void func_ImageListApi(void);
extern void func_propsheetv6(void);
extern void func_toolbarv6(void);
extern void func_tooltipv6(void);

const struct test winetest_testlist[] =
{
    { "buttonv6", func_buttonv6 },
    { "ImageListApi", func_ImageListApi },
    { "propsheetv6", func_propsheetv6 },
    { "toolbarv6", func_toolbarv6 },
    { "tooltipv6", func_tooltipv6 },
    { 0, 0 }
};
