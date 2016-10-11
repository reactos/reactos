#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_getaddrinfo(void);
extern void func_getnameinfo(void);
extern void func_getservbyname(void);
extern void func_getservbyport(void);
extern void func_ioctlsocket(void);
extern void func_nostartup(void);
extern void func_recv(void);
extern void func_send(void);
extern void func_WSARecv(void);
extern void func_WSAStartup(void);

const struct test winetest_testlist[] =
{
    { "getaddrinfo", func_getaddrinfo },
    { "getnameinfo", func_getnameinfo },
    { "getservbyname", func_getservbyname },
    { "getservbyport", func_getservbyport },
    { "ioctlsocket", func_ioctlsocket },
    { "nostartup", func_nostartup },
    { "recv", func_recv },
    { "send", func_send },
    { "WSARecv", func_WSARecv },
    { "WSAStartup", func_WSAStartup },
    { 0, 0 }
};

