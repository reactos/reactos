#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_LoadUserProfile(void);

const struct test winetest_testlist[] =
{
    { "LoadUserProfile", func_LoadUserProfile },
    { 0, 0 }
};
