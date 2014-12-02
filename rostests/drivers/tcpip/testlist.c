#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_InterfaceInfo(void);

const struct test winetest_testlist[] =
{
    { "InterfaceInfo", func_InterfaceInfo },

    { 0, 0 }
};

