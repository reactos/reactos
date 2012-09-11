#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_GetCurrentDirectory(void);
extern void func_GetDriveType(void);
extern void func_GetModuleFileName(void);
extern void func_lstrcpynW(void);
extern void func_SetCurrentDirectory(void);
extern void func_SetUnhandledExceptionFilter(void);

const struct test winetest_testlist[] =
{
    { "GetCurrentDirectory",         func_GetCurrentDirectory },
    { "GetDriveType",                func_GetDriveType },
    { "GetModuleFileName",           func_GetModuleFileName },
    { "lstrcpynW",                   func_lstrcpynW },
    { "SetCurrentDirectory",         func_SetCurrentDirectory },
    { "SetUnhandledExceptionFilter", func_SetUnhandledExceptionFilter},

    { 0, 0 }
};

