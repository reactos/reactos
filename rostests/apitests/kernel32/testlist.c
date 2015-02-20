#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_dosdev(void);
extern void func_FindFiles(void);
extern void func_GetCurrentDirectory(void);
extern void func_GetDriveType(void);
extern void func_GetModuleFileName(void);
extern void func_interlck(void);
extern void func_lstrcpynW(void);
extern void func_MultiByteToWideChar(void);
extern void func_SetCurrentDirectory(void);
extern void func_SetUnhandledExceptionFilter(void);
extern void func_TerminateProcess(void);

const struct test winetest_testlist[] =
{
    { "dosdev",                      func_dosdev },
    { "FindFiles",                   func_FindFiles },
    { "GetCurrentDirectory",         func_GetCurrentDirectory },
    { "GetDriveType",                func_GetDriveType },
    { "GetModuleFileName",           func_GetModuleFileName },
    { "interlck",                    func_interlck },
    { "lstrcpynW",                   func_lstrcpynW },
    { "MultiByteToWideChar",         func_MultiByteToWideChar },
    { "SetCurrentDirectory",         func_SetCurrentDirectory },
    { "SetUnhandledExceptionFilter", func_SetUnhandledExceptionFilter },
    { "TerminateProcess",            func_TerminateProcess },
    { 0, 0 }
};

