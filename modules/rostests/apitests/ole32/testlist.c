#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_initializespy(void);

const struct test winetest_testlist[] =
{
    { "initializespy", func_initializespy },

    { 0, 0 }
};
