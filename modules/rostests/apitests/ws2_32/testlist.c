#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_bind(void);
extern void func_close(void);
extern void func_getaddrinfo(void);
extern void func_gethostname(void);
extern void func_getnameinfo(void);
extern void func_getservbyname(void);
extern void func_getservbyport(void);
extern void func_ioctlsocket(void);
extern void func_nonblocking(void);
extern void func_nostartup(void);
extern void func_open_osfhandle(void);
extern void func_recv(void);
extern void func_send(void);
extern void func_WSAAsync(void);
extern void func_WSAIoctl(void);
extern void func_WSARecv(void);
extern void func_WSAStartup(void);

const struct test winetest_testlist[] =
{
    { "bind", func_bind },
    { "close", func_close },
    { "getaddrinfo", func_getaddrinfo },
    { "gethostname", func_gethostname },
    { "getnameinfo", func_getnameinfo },
    { "getservbyname", func_getservbyname },
    { "getservbyport", func_getservbyport },
    { "ioctlsocket", func_ioctlsocket },
    { "nonblocking", func_nonblocking },
    { "nostartup", func_nostartup },
    { "open_osfhandle", func_open_osfhandle },
    { "recv", func_recv },
    { "send", func_send },
    { "WSAAsync", func_WSAAsync },
    { "WSAIoctl", func_WSAIoctl },
    { "WSARecv", func_WSARecv },
    { "WSAStartup", func_WSAStartup },
    { 0, 0 }
};

