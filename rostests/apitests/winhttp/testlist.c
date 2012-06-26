#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_WinHttpOpen(void);

const struct test winetest_testlist[] =
{
    { "WinHttpOpen", func_WinHttpOpen },

    { 0, 0 }
};
