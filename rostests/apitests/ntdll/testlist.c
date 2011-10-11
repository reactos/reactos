#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_RtlInitializeBitMap(void);
extern void func_ZwContinue(void);
extern void func_NtFreeVirtualMemory(void);
extern void func_NtSystemInformation(void);

const struct test winetest_testlist[] =
{
    { "RtlInitializeBitMap", func_RtlInitializeBitMap },
    { "ZwContinue", func_ZwContinue },
    { "NtFreeVirtualMemory", func_NtFreeVirtualMemory },
    { "NtSystemInformation", func_NtSystemInformation },

    { 0, 0 }
};

