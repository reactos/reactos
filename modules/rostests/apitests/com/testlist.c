
#define STANDALONE
#include <apitest.h>

extern void func_browseui(void);
extern void func_certmgr(void);
extern void func_combase(void);
extern void func_explorerframe(void);
extern void func_ieframe(void);
extern void func_interfaces(void);
extern void func_netcfgx(void);
extern void func_netshell(void);
extern void func_ole32(void);
extern void func_prnfldr(void);
extern void func_shdocvw(void);
extern void func_shell32(void);
extern void func_windows_storage(void);
extern void func_zipfldr(void);

const struct test winetest_testlist[] =
{
    { "browseui", func_browseui },
    { "certmgr", func_certmgr },
    { "combase", func_combase },
    { "explorerframe", func_explorerframe },
    { "ieframe", func_ieframe },
    { "interfaces", func_interfaces },
    { "netcfgx", func_netcfgx },
    { "netshell", func_netshell },
    { "ole32", func_ole32 },
    { "prnfldr", func_prnfldr },
    { "shdocvw", func_shdocvw },
    { "shell32", func_shell32 },
    { "windows_storage", func_windows_storage },
    { "zipfldr", func_zipfldr },

    { 0, 0 }
};
