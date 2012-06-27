#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_CreateService(void);
extern void func_QueryServiceConfig2(void);

const struct test winetest_testlist[] =
{
    { "CreateService", func_CreateService },
    { "QueryServiceConfig2", func_QueryServiceConfig2 },

    { 0, 0 }
};

