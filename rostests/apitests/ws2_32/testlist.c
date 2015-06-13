#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_getaddrinfo(void);
extern void func_ioctlsocket(void);
extern void func_recv(void);
extern void func_send(void);
extern void func_WSAStartup(void);
extern void func_nostartup(void);

const struct test winetest_testlist[] =
{
    { "getaddrinfo", func_getaddrinfo },
    { "ioctlsocket", func_ioctlsocket },
    { "nostartup", func_nostartup },
    { "recv", func_recv },
    { "send", func_send },
    { "WSAStartup", func_WSAStartup },
    { 0, 0 }
};

