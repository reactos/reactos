#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_GetCurrentDirectory(void);
extern void func_GetDriveType(void);
extern void func_SetCurrentDirectory(void);

const struct test winetest_testlist[] =
{
    { "GetCurrentDirectory",    func_GetCurrentDirectory },
    { "GetDriveType",           func_GetDriveType },
    { "SetCurrentDirectory",    func_SetCurrentDirectory },

    { 0, 0 }
};

