#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_WinHttpOpen(void);

const struct test winetest_testlist[] =
{
    { "WinHttpOpen", func_WinHttpOpen },

    { 0, 0 }
};
