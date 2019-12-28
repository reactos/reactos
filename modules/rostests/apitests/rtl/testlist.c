#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_RtlAllocateHeap(void);
extern void func_RtlBitmap(void);
extern void func_RtlComputePrivatizedDllName_U(void);
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
extern void func_RtlIsNameLegalDOS8Dot3(void);
extern void func_RtlMemoryStream(void);
extern void func_RtlNtPathNameToDosPathName(void);
extern void func_RtlpEnsureBufferSize(void);
extern void func_RtlQueryTimeZoneInformation(void);
extern void func_RtlReAllocateHeap(void);
extern void func_RtlUnicodeStringToAnsiString(void);
extern void func_RtlUpcaseUnicodeStringToCountedOemString(void);

const struct test winetest_testlist[] =
{
    { "RtlAllocateHeap",                func_RtlAllocateHeap },
    { "RtlBitmapApi",                   func_RtlBitmap },
    { "RtlComputePrivatizedDllName_U",  func_RtlComputePrivatizedDllName_U },
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
    { "RtlIsNameLegalDOS8Dot3",         func_RtlIsNameLegalDOS8Dot3 },
    { "RtlMemoryStream",                func_RtlMemoryStream },
    { "RtlNtPathNameToDosPathName",     func_RtlNtPathNameToDosPathName },
    { "RtlpEnsureBufferSize",           func_RtlpEnsureBufferSize },
    { "RtlQueryTimeZoneInformation",    func_RtlQueryTimeZoneInformation },
    { "RtlReAllocateHeap",              func_RtlReAllocateHeap },
    { "RtlUnicodeStringToAnsiString",   func_RtlUnicodeStringToAnsiString },
    { "RtlUpcaseUnicodeStringToCountedOemString", func_RtlUpcaseUnicodeStringToCountedOemString },

    { 0, 0 }
};
