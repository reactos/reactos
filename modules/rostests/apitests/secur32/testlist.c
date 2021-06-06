#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_ClientServer(void);
extern void func_Auth(void);

const struct test winetest_testlist[] =
{
    { "ClientServer",    func_ClientServer },
    { "Auth",            func_Auth },
    { 0, 0 }
};
