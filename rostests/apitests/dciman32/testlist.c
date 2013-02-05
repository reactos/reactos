#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_DCICreatePrimary(void);

const struct test winetest_testlist[] =
{
    { "DCICreatePrimary", func_DCICreatePrimary },

    { 0, 0 }
};

