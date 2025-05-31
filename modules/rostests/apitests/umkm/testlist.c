#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_SystemCall(void);
extern void func_XStateConfig(void);
extern void func_XStateSave(void);

const struct test winetest_testlist[] =
{
    { "SystemCall",                     func_SystemCall },
    { "XStateConfig",                   func_XStateConfig },
    { "XStateSave",                     func_XStateSave },

    { 0, 0 }
};
