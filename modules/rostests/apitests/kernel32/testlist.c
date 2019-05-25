#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_ConsoleCP(void);
extern void func_CreateProcess(void);
extern void func_DefaultActCtx(void);
extern void func_DeviceIoControl(void);
extern void func_dosdev(void);
extern void func_FindActCtxSectionStringW(void);
extern void func_FindFiles(void);
extern void func_FLS(void);
extern void func_FormatMessage(void);
extern void func_GetComputerNameEx(void);
extern void func_GetCurrentDirectory(void);
extern void func_GetDriveType(void);
extern void func_GetModuleFileName(void);
extern void func_GetVolumeInformation(void);
extern void func_interlck(void);
extern void func_IsDBCSLeadByteEx(void);
extern void func_JapaneseCalendar(void);
extern void func_LoadLibraryExW(void);
extern void func_lstrcpynW(void);
extern void func_lstrlen(void);
extern void func_Mailslot(void);
extern void func_MultiByteToWideChar(void);
extern void func_PrivMoveFileIdentityW(void);
extern void func_SetComputerNameExW(void);
extern void func_SetConsoleWindowInfo(void);
extern void func_SetCurrentDirectory(void);
extern void func_SetUnhandledExceptionFilter(void);
extern void func_SystemFirmware(void);
extern void func_TerminateProcess(void);
extern void func_TunnelCache(void);
extern void func_WideCharToMultiByte(void);

const struct test winetest_testlist[] =
{
    { "ConsoleCP",                   func_ConsoleCP },
    { "CreateProcess",               func_CreateProcess },
    { "DefaultActCtx",               func_DefaultActCtx },
    { "DeviceIoControl",             func_DeviceIoControl },
    { "dosdev",                      func_dosdev },
    { "FindActCtxSectionStringW",    func_FindActCtxSectionStringW },
    { "FindFiles",                   func_FindFiles },
    { "FLS",                         func_FLS },
    { "FormatMessage",               func_FormatMessage },
    { "GetComputerNameEx",           func_GetComputerNameEx },
    { "GetCurrentDirectory",         func_GetCurrentDirectory },
    { "GetDriveType",                func_GetDriveType },
    { "GetModuleFileName",           func_GetModuleFileName },
    { "GetVolumeInformation",        func_GetVolumeInformation },
    { "interlck",                    func_interlck },
    { "IsDBCSLeadByteEx",            func_IsDBCSLeadByteEx },
    { "JapaneseCalendar",            func_JapaneseCalendar },
    { "LoadLibraryExW",              func_LoadLibraryExW },
    { "lstrcpynW",                   func_lstrcpynW },
    { "lstrlen",                     func_lstrlen },
    { "MailslotRead",                func_Mailslot },
    { "MultiByteToWideChar",         func_MultiByteToWideChar },
    { "PrivMoveFileIdentityW",       func_PrivMoveFileIdentityW },
    { "SetComputerNameExW",          func_SetComputerNameExW },
    { "SetConsoleWindowInfo",        func_SetConsoleWindowInfo },
    { "SetCurrentDirectory",         func_SetCurrentDirectory },
    { "SetUnhandledExceptionFilter", func_SetUnhandledExceptionFilter },
    { "SystemFirmware",              func_SystemFirmware },
    { "TerminateProcess",            func_TerminateProcess },
    { "TunnelCache",                 func_TunnelCache },
    { "WideCharToMultiByte",         func_WideCharToMultiByte },
    { 0, 0 }
};
