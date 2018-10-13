#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_AddCommas(void);
extern void func_Control_RunDLLW(void);
extern void func_CFSFolder(void);
extern void func_CMyComputer(void);
extern void func_CShellDesktop(void);
extern void func_CShellLink(void);
extern void func_CUserNotification(void);
extern void func_IShellFolderViewCB(void);
extern void func_menu(void);
extern void func_OpenAs_RunDLL(void);
extern void func_PathResolve(void);
extern void func_SHCreateFileExtractIconW(void);
extern void func_ShellExecCmdLine(void);
extern void func_ShellExecuteEx(void);
extern void func_ShellState(void);
extern void func_SHParseDisplayName(void);

const struct test winetest_testlist[] =
{
    { "AddCommas", func_AddCommas },
    { "Control_RunDLLW", func_Control_RunDLLW },
    { "CFSFolder", func_CFSFolder },
    { "CMyComputer", func_CMyComputer },
    { "CShellDesktop", func_CShellDesktop },
    { "CShellLink", func_CShellLink },
    { "CUserNotification", func_CUserNotification },
    { "IShellFolderViewCB", func_IShellFolderViewCB },
    { "menu", func_menu },
    { "OpenAs_RunDLL", func_OpenAs_RunDLL },
    { "PathResolve", func_PathResolve },
    { "SHCreateFileExtractIconW", func_SHCreateFileExtractIconW },
    { "ShellExecCmdLine", func_ShellExecCmdLine },
    { "ShellExecuteEx", func_ShellExecuteEx },
    { "ShellState", func_ShellState },
    { "SHParseDisplayName", func_SHParseDisplayName },
    { 0, 0 }
};
