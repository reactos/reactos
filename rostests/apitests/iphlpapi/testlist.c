#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_SendARP(void);

const struct test winetest_testlist[] =
{
    { "SendARP",              func_SendARP },

    { 0, 0 }
};

