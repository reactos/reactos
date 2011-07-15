#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_splitpath(void);

const struct test winetest_testlist[] =
{
    { "splitpath", func_splitpath },

    { 0, 0 }
};

