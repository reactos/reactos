#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_DrawThemeParentBackground(void);

const struct test winetest_testlist[] =
{
    { "DrawThemeParentBackground", func_DrawThemeParentBackground },
    { 0, 0 }
};
