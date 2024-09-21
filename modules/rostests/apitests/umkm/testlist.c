#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_Exceptions(void);
extern void func_SystemCall(void);

const struct test winetest_testlist[] =
{
    { "Exceptions",                     func_Exceptions },
    { "SystemCall",                     func_SystemCall },

    { 0, 0 }
};
