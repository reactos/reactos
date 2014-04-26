#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_devclass(void);

const struct test winetest_testlist[] =
{
    { "devclass", func_devclass },
    { 0, 0 }
};
