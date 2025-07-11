/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     API definitions for api-ms-win-core-processthreads-l1
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI
BOOL
WINAPI
SetThreadStackGuarantee(
    _Inout_ PULONG StackSizeInBytes);

#if (_WIN32_WINNT >= 0x600)
WINBASEAPI
BOOL
WINAPI
InitializeProcThreadAttributeList(
    _Out_writes_bytes_to_opt_(*lpSize,*lpSize) LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    _In_ DWORD dwAttributeCount,
    _Reserved_ DWORD dwFlags,
    _When_(lpAttributeList == nullptr,_Out_) _When_(lpAttributeList != nullptr,_Inout_) PSIZE_T lpSize);

WINBASEAPI
BOOL
WINAPI
UpdateProcThreadAttribute(
    _Inout_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    _In_ DWORD dwFlags,
    _In_ DWORD_PTR Attribute,
    _In_reads_bytes_opt_(cbSize) PVOID lpValue,
    _In_ SIZE_T cbSize,
    _Out_writes_bytes_opt_(cbSize) PVOID lpPreviousValue,
    _In_opt_ PSIZE_T lpReturnSize);
#endif //(_WIN32_WINNT >= 0x600)

#if (_WIN32_WINNT >= 0x602) || defined(__REACTOS__)
FORCEINLINE
HANDLE
GetCurrentProcessToken(
    VOID)
{
    return (HANDLE)(LONG_PTR)-4;
}

FORCEINLINE
HANDLE
GetCurrentThreadToken(
    VOID)
{
    return (HANDLE)(LONG_PTR)-5;
}

FORCEINLINE
HANDLE
GetCurrentThreadEffectiveToken(
    VOID)
{
    return (HANDLE)(LONG_PTR)-6;
}
#endif // (_WIN32_WINNT >= 0x602) || defined(__REACTOS__)

typedef enum _PROCESS_INFORMATION_CLASS
{
    ProcessMemoryPriority,
    ProcessMemoryExhaustionInfo,
    ProcessAppMemoryInfo,
    ProcessInPrivateInfo,
    ProcessPowerThrottling,
    ProcessReservedValue1,
    ProcessTelemetryCoverageInfo,
    ProcessProtectionLevelInfo,
    ProcessLeapSecondInfo,
    ProcessMachineTypeInfo,
    ProcessOverrideSubsequentPrefetchParameter,
    ProcessMaxOverridePrefetchParameter,
    ProcessInformationClassMax
} PROCESS_INFORMATION_CLASS;

#ifdef __cplusplus
} // extern "C"
#endif
