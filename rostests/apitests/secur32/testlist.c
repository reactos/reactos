#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_ClientServer(void);

const struct test winetest_testlist[] =
{
    { "ClientServer",    func_ClientServer },
    { 0, 0 }
};
