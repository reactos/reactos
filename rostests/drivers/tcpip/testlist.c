#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_InterfaceInfo(void);
extern void func_tcp_info(void);

const struct test winetest_testlist[] =
{
    { "InterfaceInfo", func_InterfaceInfo },
    { "tcp_info", func_tcp_info },

    { 0, 0 }
};

