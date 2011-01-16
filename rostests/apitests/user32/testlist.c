#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_InitializeLpkHooks(void);
extern void func_RealGetWindowClass(void);
extern void func_ScrollDC(void);
extern void func_ScrollWindowEx(void);
extern void func_GetSystemMetrics(void);
extern void func_GetIconInfo(void);

const struct test winetest_testlist[] =
{
    { "InitializeLpkHooks", func_InitializeLpkHooks },
    { "RealGetWindowClass", func_RealGetWindowClass },
    { "ScrollDC", func_ScrollDC },
    { "ScrollWindowEx", func_ScrollWindowEx },
    { "GetSystemMetrics", func_GetSystemMetrics },
    { "GetIconInfo", func_GetIconInfo },

    { 0, 0 }
};

