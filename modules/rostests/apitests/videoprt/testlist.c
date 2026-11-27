#define STANDALONE
#include <apitest.h>

extern void func_DeviceMap(void);

const struct test winetest_testlist[] =
{
    { "DeviceMap", func_DeviceMap },
    { 0, 0 }
};
