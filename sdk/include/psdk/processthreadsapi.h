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

typedef struct _STARTUPINFOA
{
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    PBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _STARTUPINFOW
{
    DWORD cb;
    LPWSTR lpReserved;
    LPWSTR lpDesktop;
    LPWSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    PBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOW,*LPSTARTUPINFOW;

#ifdef UNICODE
typedef STARTUPINFOW STARTUPINFO, *LPSTARTUPINFO;
#else
typedef STARTUPINFOA STARTUPINFO, *LPSTARTUPINFO;
#endif // UNICODE

typedef struct _PROCESS_INFORMATION
{
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION,*PPROCESS_INFORMATION,*LPPROCESS_INFORMATION;

typedef struct _PROC_THREAD_ATTRIBUTE_LIST *PPROC_THREAD_ATTRIBUTE_LIST, *LPPROC_THREAD_ATTRIBUTE_LIST;

WINBASEAPI
BOOL
WINAPI
SetThreadStackGuarantee(
    _Inout_ PULONG StackSizeInBytes);

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
VOID
WINAPI
FlushProcessWriteBuffers(
    VOID);

WINBASEAPI
_Success_(return != FALSE)
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

WINBASEAPI
VOID
WINAPI
DeleteProcThreadAttributeList(
    _Inout_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList);

#endif // (_WIN32_WINNT >= 0x0600) || defined(__REACTOS__)

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

#ifdef __cplusplus
} // extern "C"
#endif
