#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_NtAllocateVirtualMemory(void);
extern void func_NtFreeVirtualMemory(void);
extern void func_NtSystemInformation(void);
extern void func_RtlInitializeBitMap(void);
extern void func_ZwContinue(void);

const struct test winetest_testlist[] =
{
    { "NtAllocateVirtualMemory",    func_NtAllocateVirtualMemory },
    { "NtFreeVirtualMemory",        func_NtFreeVirtualMemory },
    { "NtSystemInformation",        func_NtSystemInformation },
    { "RtlInitializeBitMap",        func_RtlInitializeBitMap },
    { "ZwContinue",                 func_ZwContinue },

    { 0, 0 }
};

