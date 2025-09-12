#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_GetDeviceDriverFileName(void);
extern void func_GetDeviceDriverBaseName(void);

const struct test winetest_testlist[] =
{
    { "GetDeviceDriverFileName",                    func_GetDeviceDriverFileName },
    { "GetDeviceDriverBaseName",                    func_GetDeviceDriverBaseName },

    { 0, 0 }
};

