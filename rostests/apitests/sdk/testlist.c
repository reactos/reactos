#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_delayimp(void);

const struct test winetest_testlist[] =
{
    { "delayimp", func_delayimp },
    { 0, 0 }
};
