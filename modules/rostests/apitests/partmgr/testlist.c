#define STANDALONE
#include <apitest.h>

extern void func_StorDeviceNumber(void);

const struct test winetest_testlist[] =
{
    { "StorDeviceNumber", func_StorDeviceNumber },
    { 0, 0 }
};
