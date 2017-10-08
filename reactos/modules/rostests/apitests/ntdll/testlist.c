#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_LdrEnumResources(void);
extern void func_NtAcceptConnectPort(void);
extern void func_NtAllocateVirtualMemory(void);
extern void func_NtApphelpCacheControl(void);
extern void func_NtContinue(void);
extern void func_NtCreateFile(void);
extern void func_NtCreateKey(void);
extern void func_NtCreateThread(void);
extern void func_NtDeleteKey(void);
extern void func_NtFreeVirtualMemory(void);
extern void func_NtLoadUnloadKey(void);
extern void func_NtMapViewOfSection(void);
extern void func_NtMutant(void);
extern void func_NtOpenKey(void);
extern void func_NtOpenProcessToken(void);
extern void func_NtOpenThreadToken(void);
extern void func_NtProtectVirtualMemory(void);
extern void func_NtQueryInformationProcess(void);
extern void func_NtQueryKey(void);
extern void func_NtQuerySystemEnvironmentValue(void);
extern void func_NtQueryVolumeInformationFile(void);
extern void func_NtReadFile(void);
extern void func_NtSaveKey(void);
extern void func_NtSetValueKey(void);
extern void func_NtSystemInformation(void);
extern void func_NtWriteFile(void);
extern void func_RtlAllocateHeap(void);
extern void func_RtlBitmap(void);
extern void func_RtlCopyMappedMemory(void);
extern void func_RtlDeleteAce(void);
extern void func_RtlDetermineDosPathNameType(void);
extern void func_RtlDosApplyFileIsolationRedirection_Ustr(void);
extern void func_RtlDoesFileExists(void);
extern void func_RtlDosPathNameToNtPathName_U(void);
extern void func_RtlDosSearchPath_U(void);
extern void func_RtlDosSearchPath_Ustr(void);
extern void func_RtlFirstFreeAce(void);
extern void func_RtlGenerate8dot3Name(void);
extern void func_RtlGetFullPathName_U(void);
extern void func_RtlGetFullPathName_Ustr(void);
extern void func_RtlGetFullPathName_UstrEx(void);
extern void func_RtlGetLengthWithoutTrailingPathSeperators(void);
extern void func_RtlGetLongestNtPathLength(void);
extern void func_RtlHandle(void);
extern void func_RtlImageRvaToVa(void);
extern void func_RtlInitializeBitMap(void);
extern void func_RtlIsNameLegalDOS8Dot3(void);
extern void func_RtlMemoryStream(void);
extern void func_RtlNtPathNameToDosPathName(void);
extern void func_RtlpEnsureBufferSize(void);
extern void func_RtlReAllocateHeap(void);
extern void func_RtlUpcaseUnicodeStringToCountedOemString(void);
extern void func_StackOverflow(void);
extern void func_TimerResolution(void);

const struct test winetest_testlist[] =
{
    { "LdrEnumResources",               func_LdrEnumResources },
    { "NtAcceptConnectPort",            func_NtAcceptConnectPort },
    { "NtAllocateVirtualMemory",        func_NtAllocateVirtualMemory },
    { "NtApphelpCacheControl",          func_NtApphelpCacheControl },
    { "NtContinue",                     func_NtContinue },
    { "NtCreateFile",                   func_NtCreateFile },
    { "NtCreateKey",                    func_NtCreateKey },
    { "NtCreateThread",                 func_NtCreateThread },
    { "NtDeleteKey",                    func_NtDeleteKey },
    { "NtFreeVirtualMemory",            func_NtFreeVirtualMemory },
    { "NtLoadUnloadKey",                func_NtLoadUnloadKey },
    { "NtMapViewOfSection",             func_NtMapViewOfSection },
    { "NtMutant",                       func_NtMutant },
    { "NtOpenKey",                      func_NtOpenKey },
    { "NtOpenProcessToken",             func_NtOpenProcessToken },
    { "NtOpenThreadToken",              func_NtOpenThreadToken },
    { "NtProtectVirtualMemory",         func_NtProtectVirtualMemory },
    { "NtQueryInformationProcess",      func_NtQueryInformationProcess },
    { "NtQueryKey",                     func_NtQueryKey },
    { "NtQuerySystemEnvironmentValue",  func_NtQuerySystemEnvironmentValue },
    { "NtQueryVolumeInformationFile",   func_NtQueryVolumeInformationFile },
    { "NtReadFile",                     func_NtReadFile },
    { "NtSaveKey",                      func_NtSaveKey},
    { "NtSetValueKey",                  func_NtSetValueKey},
    { "NtSystemInformation",            func_NtSystemInformation },
    { "NtWriteFile",                    func_NtWriteFile },
    { "RtlAllocateHeap",                func_RtlAllocateHeap },
    { "RtlBitmapApi",                   func_RtlBitmap },
    { "RtlCopyMappedMemory",            func_RtlCopyMappedMemory },
    { "RtlDeleteAce",                   func_RtlDeleteAce },
    { "RtlDetermineDosPathNameType",    func_RtlDetermineDosPathNameType },
    { "RtlDosApplyFileIsolationRedirection_Ustr", func_RtlDosApplyFileIsolationRedirection_Ustr },
    { "RtlDoesFileExists",              func_RtlDoesFileExists },
    { "RtlDosPathNameToNtPathName_U",   func_RtlDosPathNameToNtPathName_U },
    { "RtlDosSearchPath_U",             func_RtlDosSearchPath_U },
    { "RtlDosSearchPath_Ustr",          func_RtlDosSearchPath_Ustr },
    { "RtlFirstFreeAce",                func_RtlFirstFreeAce },
    { "RtlGenerate8dot3Name",           func_RtlGenerate8dot3Name },
    { "RtlGetFullPathName_U",           func_RtlGetFullPathName_U },
    { "RtlGetFullPathName_Ustr",        func_RtlGetFullPathName_Ustr },
    { "RtlGetFullPathName_UstrEx",      func_RtlGetFullPathName_UstrEx },
    { "RtlGetLengthWithoutTrailingPathSeperators", func_RtlGetLengthWithoutTrailingPathSeperators },
    { "RtlGetLongestNtPathLength",      func_RtlGetLongestNtPathLength },
    { "RtlHandle",                      func_RtlHandle },
    { "RtlImageRvaToVa",                func_RtlImageRvaToVa },
    { "RtlInitializeBitMap",            func_RtlInitializeBitMap },
    { "RtlIsNameLegalDOS8Dot3",         func_RtlIsNameLegalDOS8Dot3 },
    { "RtlMemoryStream",                func_RtlMemoryStream },
    { "RtlNtPathNameToDosPathName",     func_RtlNtPathNameToDosPathName },
    { "RtlpEnsureBufferSize",           func_RtlpEnsureBufferSize },
    { "RtlReAllocateHeap",              func_RtlReAllocateHeap },
    { "RtlUpcaseUnicodeStringToCountedOemString", func_RtlUpcaseUnicodeStringToCountedOemString },
    { "StackOverflow",                  func_StackOverflow },
    { "TimerResolution",                func_TimerResolution },

    { 0, 0 }
};
