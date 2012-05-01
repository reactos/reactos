#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_NtAllocateVirtualMemory(void);
extern void func_NtFreeVirtualMemory(void);
extern void func_NtSystemInformation(void);
extern void func_RtlDetermineDosPathNameType(void);
extern void func_RtlDoesFileExists(void);
extern void func_RtlDosSearchPath_U(void);
extern void func_RtlDosSearchPath_Ustr(void);
extern void func_RtlGetFullPathName_U(void);
extern void func_RtlGetFullPathName_Ustr(void);
extern void func_RtlGetFullPathName_UstrEx(void);
extern void func_RtlGetLongestNtPathLength(void);
extern void func_RtlInitializeBitMap(void);
extern void func_ZwContinue(void);

const struct test winetest_testlist[] =
{
    { "NtAllocateVirtualMemory",        func_NtAllocateVirtualMemory },
    { "NtFreeVirtualMemory",            func_NtFreeVirtualMemory },
    { "NtSystemInformation",            func_NtSystemInformation },
    { "RtlDetermineDosPathNameType",    func_RtlDetermineDosPathNameType },
    { "RtlDoesFileExists",              func_RtlDoesFileExists },
    { "RtlDosSearchPath_U",             func_RtlDosSearchPath_U },
    { "RtlDosSearchPath_Ustr",          func_RtlDosSearchPath_Ustr },
    { "RtlGetFullPathName_U",           func_RtlGetFullPathName_U },
    { "RtlGetFullPathName_Ustr",        func_RtlGetFullPathName_Ustr },
    { "RtlGetFullPathName_UstrEx",      func_RtlGetFullPathName_UstrEx },
    { "RtlGetLongestNtPathLength",      func_RtlGetLongestNtPathLength },
    { "RtlInitializeBitMap",            func_RtlInitializeBitMap },
    { "ZwContinue",                     func_ZwContinue },

    { 0, 0 }
};
