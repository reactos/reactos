#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_GetDeviceDriverFileName(void);

const struct test winetest_testlist[] =
{
    { "GetDeviceDriverFileName",                    func_GetDeviceDriverFileName },

    { 0, 0 }
};

