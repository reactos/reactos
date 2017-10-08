#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_power(void);

const struct test winetest_testlist[] =
{
    { "power", func_power },
    { 0, 0 }
};
