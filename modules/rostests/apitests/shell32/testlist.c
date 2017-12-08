#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_AddCommas(void);
extern void func_Control_RunDLLW(void);
extern void func_CFSFolder(void);
extern void func_CMyComputer(void);
extern void func_CShellDesktop(void);
extern void func_CShellLink(void);
extern void func_menu(void);
extern void func_PathResolve(void);
extern void func_SHCreateFileExtractIconW(void);
extern void func_ShellExecuteEx(void);
extern void func_SHParseDisplayName(void);

const struct test winetest_testlist[] =
{
    { "AddCommas", func_AddCommas },
    { "Control_RunDLLW", func_Control_RunDLLW },
    { "CFSFolder", func_CFSFolder },
    { "CMyComputer", func_CMyComputer },
    { "CShellDesktop", func_CShellDesktop },
    { "CShellLink", func_CShellLink },
    { "menu", func_menu },
    { "PathResolve", func_PathResolve },
    { "SHCreateFileExtractIconW", func_SHCreateFileExtractIconW },
    { "ShellExecuteEx", func_ShellExecuteEx },
    { "SHParseDisplayName", func_SHParseDisplayName },
    { 0, 0 }
};
