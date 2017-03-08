#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_DrawThemeParentBackground(void);
extern void func_CloseThemeData(void);

const struct test winetest_testlist[] =
{
    { "DrawThemeParentBackground", func_DrawThemeParentBackground },
    { "CloseThemeData", func_CloseThemeData },
    { 0, 0 }
};
