#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_CommandLine(void);
extern void func_ieee(void);
extern void func_popen(void);
extern void func_splitpath(void);

const struct test winetest_testlist[] =
{
    { "CommandLine", func_CommandLine },
    { "ieee", func_ieee },
    { "popen", func_popen },
    { "splitpath", func_splitpath },

    { 0, 0 }
};
