#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_VerQueryValue(void);

const struct test winetest_testlist[] =
{
    { "VerQueryValue",                 func_VerQueryValue },

    { 0, 0 }
};
