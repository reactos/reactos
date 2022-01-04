#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_browseui(void);
extern void func_certmgr(void);
extern void func_ieframe(void);
extern void func_interfaces(void);
extern void func_netcfgx(void);
extern void func_netshell(void);
extern void func_ole32(void);
extern void func_shdocvw(void);
extern void func_shell32(void);
extern void func_zipfldr(void);

const struct test winetest_testlist[] =
{
    { "browseui", func_browseui },
    { "certmgr", func_certmgr },
    { "ieframe", func_ieframe },
    { "interfaces", func_interfaces },
    { "netcfgx", func_netcfgx },
    { "netshell", func_netshell },
    { "ole32", func_ole32 },
    { "shdocvw", func_shdocvw },
    { "shell32", func_shell32 },
    { "zipfldr", func_zipfldr },

    { 0, 0 }
};
