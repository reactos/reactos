#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_SystemCall(void);

const struct test winetest_testlist[] =
{
    { "SystemCall",                     func_SystemCall },

    { 0, 0 }
};
