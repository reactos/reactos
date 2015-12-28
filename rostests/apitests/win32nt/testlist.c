#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_NtGdiDdCreateDirectDrawObject(void);

const struct test winetest_testlist[] =
{
    { "NtGdiDdCreateDirectDrawObject", func_NtGdiDdCreateDirectDrawObject },

    { 0, 0 }
};
