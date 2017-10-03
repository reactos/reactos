#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_GetProfileDirs(void);
extern void func_LoadUserProfile(void);

const struct test winetest_testlist[] =
{
    { "GetProfileDirs", func_GetProfileDirs },
    { "LoadUserProfile", func_LoadUserProfile },
    { 0, 0 }
};
