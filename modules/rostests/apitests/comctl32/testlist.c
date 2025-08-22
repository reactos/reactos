#define STANDALONE
#include <apitest.h>

extern void func_button(void);
extern void func_ImageListApi(void);
extern void func_propsheet(void);
extern void func_toolbar(void);
extern void func_tooltip(void);

const struct test winetest_testlist[] =
{
    { "buttonv6", func_button },
    { "ImageListApi", func_ImageListApi },
    { "propsheetv6", func_propsheet },
    { "toolbarv6", func_toolbar },
    { "tooltipv6", func_tooltip },
    { 0, 0 }
};
