#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_LdrEnumResources(void);
extern void func_NtAllocateVirtualMemory(void);
extern void func_NtContinue(void);
extern void func_NtCreateFile(void);
extern void func_NtCreateThread(void);
extern void func_NtDeleteKey(void);
extern void func_NtFreeVirtualMemory(void);
extern void func_NtMapViewOfSection(void);
extern void func_NtMutant(void);
extern void func_NtProtectVirtualMemory(void);
extern void func_NtQueryKey(void);
extern void func_NtQuerySystemEnvironmentValue(void);
extern void func_NtQueryVolumeInformationFile(void);
extern void func_NtSaveKey(void);
extern void func_NtSystemInformation(void);
extern void func_RtlBitmap(void);
extern void func_RtlDetermineDosPathNameType(void);
extern void func_RtlDoesFileExists(void);
extern void func_RtlDosPathNameToNtPathName_U(void);
extern void func_RtlDosSearchPath_U(void);
extern void func_RtlDosSearchPath_Ustr(void);
extern void func_RtlGetFullPathName_U(void);
extern void func_RtlGetFullPathName_Ustr(void);
extern void func_RtlGetFullPathName_UstrEx(void);
extern void func_RtlGetLengthWithoutTrailingPathSeperators(void);
extern void func_RtlGetLongestNtPathLength(void);
extern void func_RtlInitializeBitMap(void);
extern void func_RtlMemoryStream(void);
extern void func_StackOverflow(void);
extern void func_TimerResolution(void);

const struct test winetest_testlist[] =
{
    { "LdrEnumResources",               func_LdrEnumResources },
    { "NtAllocateVirtualMemory",        func_NtAllocateVirtualMemory },
    { "NtContinue",                     func_NtContinue },
    { "NtCreateFile",                   func_NtCreateFile },
    { "NtCreateThread",                 func_NtCreateThread },
    { "NtDeleteKey",                    func_NtDeleteKey },
    { "NtFreeVirtualMemory",            func_NtFreeVirtualMemory },
    { "NtMapViewOfSection",             func_NtMapViewOfSection },
    { "NtMutant",                       func_NtMutant },
    { "NtProtectVirtualMemory",         func_NtProtectVirtualMemory },
    { "NtQueryKey",                     func_NtQueryKey },
    { "NtQuerySystemEnvironmentValue",  func_NtQuerySystemEnvironmentValue },
    { "NtQueryVolumeInformationFile",   func_NtQueryVolumeInformationFile },
    { "NtSaveKey",                      func_NtSaveKey},
    { "NtSystemInformation",            func_NtSystemInformation },
    { "RtlBitmapApi",                   func_RtlBitmap },
    { "RtlDetermineDosPathNameType",    func_RtlDetermineDosPathNameType },
    { "RtlDoesFileExists",              func_RtlDoesFileExists },
    { "RtlDosPathNameToNtPathName_U",   func_RtlDosPathNameToNtPathName_U },
    { "RtlDosSearchPath_U",             func_RtlDosSearchPath_U },
    { "RtlDosSearchPath_Ustr",          func_RtlDosSearchPath_Ustr },
    { "RtlGetFullPathName_U",           func_RtlGetFullPathName_U },
    { "RtlGetFullPathName_Ustr",        func_RtlGetFullPathName_Ustr },
    { "RtlGetFullPathName_UstrEx",      func_RtlGetFullPathName_UstrEx },
    { "RtlGetLengthWithoutTrailingPathSeperators", func_RtlGetLengthWithoutTrailingPathSeperators },
    { "RtlGetLongestNtPathLength",      func_RtlGetLongestNtPathLength },
    { "RtlInitializeBitMap",            func_RtlInitializeBitMap },
    { "RtlMemoryStream",                func_RtlMemoryStream },
    { "StackOverflow",                  func_StackOverflow },
    { "TimerResolution",                func_TimerResolution },

    { 0, 0 }
};
