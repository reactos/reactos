#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_CreateService(void);
extern void func_LockDatabase(void);
extern void func_QueryServiceConfig2(void);
extern void func_SaferIdentifyLevel(void);

const struct test winetest_testlist[] =
{
    { "CreateService", func_CreateService },
    { "LockDatabase" , func_LockDatabase },
    { "QueryServiceConfig2", func_QueryServiceConfig2 },
    { "SaferIdentifyLevel", func_SaferIdentifyLevel },

    { 0, 0 }
};

