#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_InternetOpen(void);

const struct test winetest_testlist[] =
{
    { "InternetOpen", func_InternetOpen },

    { 0, 0 }
};
