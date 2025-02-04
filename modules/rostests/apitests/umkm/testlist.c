#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_SystemCall(void);
extern void func_XState(void);

const struct test winetest_testlist[] =
{
    { "SystemCall",                     func_SystemCall },
    { "XState",                         func_XState },

    { 0, 0 }
};
