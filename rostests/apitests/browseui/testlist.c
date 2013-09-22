#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_SHExplorerParseCmdLine(void);

const struct test winetest_testlist[] =
{
    { "SHExplorerParseCmdLine", func_SHExplorerParseCmdLine },

    { 0, 0 }
};

