#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_wlanapi(void);

const struct test winetest_testlist[] =
{
    { "wlanapi", func_wlanapi },
    { 0, 0 }
};
