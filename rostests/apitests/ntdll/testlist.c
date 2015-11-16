#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_LdrEnumResources(void);
extern void func_NtAllocateVirtualMemory(void);
extern void func_NtApphelpCacheControl(void);
extern void func_NtContinue(void);
extern void func_NtCreateFile(void);
extern void func_NtCreateThread(void);
extern void func_NtDeleteKey(void);
extern void func_NtFreeVirtualMemory(void);
extern void func_NtMapViewOfSection(void);
extern void func_NtMutant(void);
extern void func_NtOpenProcessToken(void);
extern void func_NtOpenThreadToken(void);
extern void func_NtProtectVirtualMemory(void);
extern void func_NtQueryKey(void);
extern void func_NtQuerySystemEnvironmentValue(void);
extern void func_NtQueryVolumeInformationFile(void);
extern void func_NtSaveKey(void);
extern void func_NtSystemInformation(void);
extern void func_RtlAllocateHeap(void);
extern void func_RtlBitmap(void);
extern void func_RtlCopyMappedMemory(void);
extern void func_RtlDetermineDosPathNameType(void);
extern void func_RtlDoesFileExists(void);
extern void func_RtlDosPathNameToNtPathName_U(void);
extern void func_RtlDosSearchPath_U(void);
extern void func_RtlDosSearchPath_Ustr(void);
extern void func_RtlGenerate8dot3Name(void);
extern void func_RtlGetFullPathName_U(void);
extern void func_RtlGetFullPathName_Ustr(void);
extern void func_RtlGetFullPathName_UstrEx(void);
extern void func_RtlGetLengthWithoutTrailingPathSeperators(void);
extern void func_RtlGetLongestNtPathLength(void);
extern void func_RtlImageRvaToVa(void);
extern void func_RtlInitializeBitMap(void);
extern void func_RtlMemoryStream(void);
extern void func_RtlReAllocateHeap(void);
extern void func_StackOverflow(void);
extern void func_TimerResolution(void);

const struct test winetest_testlist[] =
{
    { "LdrEnumResources",               func_LdrEnumResources },
    { "NtAllocateVirtualMemory",        func_NtAllocateVirtualMemory },
    { "NtApphelpCacheControl",          func_NtApphelpCacheControl },
    { "NtContinue",                     func_NtContinue },
    { "NtCreateFile",                   func_NtCreateFile },
    { "NtCreateThread",                 func_NtCreateThread },
    { "NtDeleteKey",                    func_NtDeleteKey },
    { "NtFreeVirtualMemory",            func_NtFreeVirtualMemory },
    { "NtMapViewOfSection",             func_NtMapViewOfSection },
    { "NtMutant",                       func_NtMutant },
    { "NtOpenProcessToken",             func_NtOpenProcessToken },
    { "NtOpenThreadToken",              func_NtOpenThreadToken },
    { "NtProtectVirtualMemory",         func_NtProtectVirtualMemory },
    { "NtQueryKey",                     func_NtQueryKey },
    { "NtQuerySystemEnvironmentValue",  func_NtQuerySystemEnvironmentValue },
    { "NtQueryVolumeInformationFile",   func_NtQueryVolumeInformationFile },
    { "NtSaveKey",                      func_NtSaveKey},
    { "NtSystemInformation",            func_NtSystemInformation },
    { "RtlAllocateHeap",                func_RtlAllocateHeap },
    { "RtlBitmapApi",                   func_RtlBitmap },
    { "RtlCopyMappedMemory",            func_RtlCopyMappedMemory },
    { "RtlDetermineDosPathNameType",    func_RtlDetermineDosPathNameType },
    { "RtlDoesFileExists",              func_RtlDoesFileExists },
    { "RtlDosPathNameToNtPathName_U",   func_RtlDosPathNameToNtPathName_U },
    { "RtlDosSearchPath_U",             func_RtlDosSearchPath_U },
    { "RtlDosSearchPath_Ustr",          func_RtlDosSearchPath_Ustr },
    { "RtlGenerate8dot3Name",           func_RtlGenerate8dot3Name },
    { "RtlGetFullPathName_U",           func_RtlGetFullPathName_U },
    { "RtlGetFullPathName_Ustr",        func_RtlGetFullPathName_Ustr },
    { "RtlGetFullPathName_UstrEx",      func_RtlGetFullPathName_UstrEx },
    { "RtlGetLengthWithoutTrailingPathSeperators", func_RtlGetLengthWithoutTrailingPathSeperators },
    { "RtlGetLongestNtPathLength",      func_RtlGetLongestNtPathLength },
    { "RtlImageRvaToVa",                func_RtlImageRvaToVa },
    { "RtlInitializeBitMap",            func_RtlInitializeBitMap },
    { "RtlMemoryStream",                func_RtlMemoryStream },
    { "RtlReAllocateHeap",              func_RtlReAllocateHeap },
    { "StackOverflow",                  func_StackOverflow },
    { "TimerResolution",                func_TimerResolution },

    { 0, 0 }
};
