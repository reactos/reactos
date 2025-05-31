#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_MovSsBug(void);
extern void func_SystemCall(void);

const struct test winetest_testlist[] =
{
    { "MovSsBug",                       func_MovSsBug },
    { "SystemCall",                     func_SystemCall },

    { 0, 0 }
};
