#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_AddCommas(void);
extern void func_Control_RunDLLW(void);
extern void func_CFSFolder(void);
extern void func_CheckEscapes(void);
extern void func_CIDLData(void);
extern void func_CMyComputer(void);
extern void func_CommandLineToArgvW(void);
extern void func_CShellDesktop(void);
extern void func_CShellLink(void);
extern void func_CUserNotification(void);
extern void func_DragDrop(void);
extern void func_ExtractIconEx(void);
extern void func_FindExecutable(void);
extern void func_GetDisplayNameOf(void);
extern void func_GUIDFromString(void);
extern void func_ILCreateFromPath(void);
extern void func_ILIsEqual(void);
extern void func_Int64ToString(void);
extern void func_IShellFolderViewCB(void);
extern void func_menu(void);
extern void func_OpenAs_RunDLL(void);
extern void func_PathIsEqualOrSubFolder(void);
extern void func_PathIsTemporary(void);
extern void func_PathResolve(void);
extern void func_PIDL(void);
extern void func_RealShellExecuteEx(void);
extern void func_SHAppBarMessage(void);
extern void func_SHChangeNotify(void);
extern void func_SHCreateDataObject(void);
extern void func_SHCreateFileDataObject(void);
extern void func_SHCreateFileExtractIconW(void);
extern void func_She(void);
extern void func_ShellExecCmdLine(void);
extern void func_ShellExecuteEx(void);
extern void func_ShellExecuteW(void);
extern void func_ShellHook(void);
extern void func_ShellState(void);
extern void func_SHGetAttributesFromDataObject(void);
extern void func_SHGetFileInfo(void);
extern void func_SHLimitInputEdit(void);
extern void func_SHParseDisplayName(void);
extern void func_SHSimpleIDListFromPath(void);
extern void func_SHRestricted(void);

const struct test winetest_testlist[] =
{
    { "AddCommas", func_AddCommas },
    { "Control_RunDLLW", func_Control_RunDLLW },
    { "CFSFolder", func_CFSFolder },
    { "CheckEscapes", func_CheckEscapes },
    { "CIDLData", func_CIDLData },
    { "CMyComputer", func_CMyComputer },
    { "CommandLineToArgvW", func_CommandLineToArgvW },
    { "CShellDesktop", func_CShellDesktop },
    { "CShellLink", func_CShellLink },
    //{ "CUserNotification", func_CUserNotification }, // Test is broken on Win 2003
    { "DragDrop", func_DragDrop },
    { "ExtractIconEx", func_ExtractIconEx },
    { "FindExecutable", func_FindExecutable },
    { "GetDisplayNameOf", func_GetDisplayNameOf },
    { "GUIDFromString", func_GUIDFromString },
    { "ILCreateFromPath", func_ILCreateFromPath },
    { "ILIsEqual", func_ILIsEqual },
    { "Int64ToString", func_Int64ToString },
    { "IShellFolderViewCB", func_IShellFolderViewCB },
    { "menu", func_menu },
    { "OpenAs_RunDLL", func_OpenAs_RunDLL },
    { "PathIsEqualOrSubFolder", func_PathIsEqualOrSubFolder },
    { "PathIsTemporary", func_PathIsTemporary },
    { "PathResolve", func_PathResolve },
    { "PIDL", func_PIDL },
    { "RealShellExecuteEx", func_RealShellExecuteEx },
    { "SHAppBarMessage", func_SHAppBarMessage },
    { "SHChangeNotify", func_SHChangeNotify },
    { "SHCreateDataObject", func_SHCreateDataObject },
    { "SHCreateFileDataObject", func_SHCreateFileDataObject },
    { "SHCreateFileExtractIconW", func_SHCreateFileExtractIconW },
    { "She", func_She },
    { "ShellExecCmdLine", func_ShellExecCmdLine },
    { "ShellExecuteEx", func_ShellExecuteEx },
    { "ShellExecuteW", func_ShellExecuteW },
    { "ShellHook", func_ShellHook },
    { "ShellState", func_ShellState },
    { "SHGetAttributesFromDataObject", func_SHGetAttributesFromDataObject },
    { "SHGetFileInfo", func_SHGetFileInfo },
    { "SHLimitInputEdit", func_SHLimitInputEdit },
    { "SHParseDisplayName", func_SHParseDisplayName },
    { "SHSimpleIDListFromPath", func_SHSimpleIDListFromPath },
    { "SHRestricted", func_SHRestricted },

    { 0, 0 }
};
