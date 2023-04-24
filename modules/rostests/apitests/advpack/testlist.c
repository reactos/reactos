#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_DelNode(void);

const struct test winetest_testlist[] =
{
    { "DelNode", func_DelNode },
    { 0, 0 }
};
