#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_CFSFolder(void);
extern void func_CMyComputer(void);
extern void func_CShellDesktop(void);
extern void func_CShellLink(void);
extern void func_menu(void);
extern void func_ShellExecuteEx(void);
extern void func_SHParseDisplayName(void);

const struct test winetest_testlist[] =
{
    { "CFSFolder", func_CFSFolder },
    { "CMyComputer", func_CMyComputer },
    { "CShellDesktop", func_CShellDesktop },
    { "CShellLink", func_CShellLink },
    { "menu", func_menu },
    { "ShellExecuteEx", func_ShellExecuteEx },
    { "SHParseDisplayName", func_SHParseDisplayName },
    { 0, 0 }
};
