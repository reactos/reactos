/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Vista functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FIXME: Move these RTL declarations to the NDK */
NTSTATUS
NTAPI
RtlSleepConditionVariableCS(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                            IN OUT PRTL_CRITICAL_SECTION CriticalSection,
                            IN PLARGE_INTEGER TimeOut  OPTIONAL);

NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                             IN OUT PRTL_SRWLOCK SRWLock,
                             IN PLARGE_INTEGER TimeOut  OPTIONAL,
                             IN ULONG Flags);

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
SleepConditionVariableCS(IN OUT PCONDITION_VARIABLE ConditionVariable,
                         IN OUT PCRITICAL_SECTION CriticalSection,
                         IN DWORD dwMilliseconds)
{
    NTSTATUS Status = 0;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER TimeOutPtr = NULL;

    if (dwMilliseconds != INFINITE)
    {
        TimeOut.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimeOutPtr = &TimeOut;
    }

#if 0
    Status = RtlSleepConditionVariableCS((PRTL_CONDITION_VARIABLE)ConditionVariable,
                                         (PRTL_CRITICAL_SECTION)CriticalSection,
                                         TimeOutPtr);
#endif
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SleepConditionVariableSRW(IN OUT PCONDITION_VARIABLE ConditionVariable,
                          IN OUT PSRWLOCK SRWLock,
                          IN DWORD dwMilliseconds,
                          IN ULONG Flags)
{
    NTSTATUS Status = 0;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER TimeOutPtr = NULL;

    if (dwMilliseconds != INFINITE)
    {
        TimeOut.QuadPart = UInt32x32To64(-10000, dwMilliseconds);
        TimeOutPtr = &TimeOut;
    }

#if 0
    Status = RtlSleepConditionVariableSRW((PRTL_CONDITION_VARIABLE)ConditionVariable,
                                          (PRTL_SRWLOCK)SRWLock,
                                          TimeOutPtr,
                                          Flags);
#endif
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI InitializeCriticalSectionEx(OUT LPCRITICAL_SECTION lpCriticalSection,
                                       IN DWORD dwSpinCount,
                                       IN DWORD flags )
{
    NTSTATUS Status;

    /* FIXME: Flags ignored */

    /* Initialize the critical section */
    Status = RtlInitializeCriticalSectionAndSpinCount(
        (PRTL_CRITICAL_SECTION)lpCriticalSection,
        dwSpinCount);
    if (!NT_SUCCESS(Status))
    {
        /* Set failure code */
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}



/*
 * @implemented
 */
BOOL
WINAPI
QueryFullProcessImageNameW(HANDLE hProcess,
                           DWORD dwFlags,
                           LPWSTR lpExeName,
                           PDWORD pdwSize)
{
    BYTE Buffer[sizeof(UNICODE_STRING) + MAX_PATH * sizeof(WCHAR)];
    UNICODE_STRING *DynamicBuffer = NULL;
    UNICODE_STRING *Result = NULL;
    NTSTATUS Status;
    DWORD Needed;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       Buffer,
                                       sizeof(Buffer) - sizeof(WCHAR),
                                       &Needed);
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        DynamicBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Needed + sizeof(WCHAR));
        if (!DynamicBuffer)
        {
            SetLastErrorByStatus(STATUS_NO_MEMORY);
            return FALSE;
        }

        Status = NtQueryInformationProcess(hProcess,
                                           ProcessImageFileName,
                                           (LPBYTE)DynamicBuffer,
                                           Needed,
                                           &Needed);
        Result = DynamicBuffer;
    }
    else Result = (PUNICODE_STRING)Buffer;

    if (!NT_SUCCESS(Status)) goto Cleanup;

    if (Result->Length / sizeof(WCHAR) + 1 > *pdwSize)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Cleanup;
    }

    *pdwSize = Result->Length / sizeof(WCHAR);
    memcpy(lpExeName, Result->Buffer, Result->Length);
    lpExeName[*pdwSize] = 0;

Cleanup:
    RtlFreeHeap(RtlGetProcessHeap(), 0, DynamicBuffer);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
    }

    return !Status;
}


/*
 * @implemented
 */
BOOL
WINAPI
QueryFullProcessImageNameA(HANDLE hProcess,
                           DWORD dwFlags,
                           LPSTR lpExeName,
                           PDWORD pdwSize)
{
    DWORD pdwSizeW = *pdwSize;
    BOOL Result;
    LPWSTR lpExeNameW;

    lpExeNameW = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 *pdwSize * sizeof(WCHAR));
    if (!lpExeNameW)
    {
        SetLastErrorByStatus(STATUS_NO_MEMORY);
        return FALSE;
    }

    Result = QueryFullProcessImageNameW(hProcess, dwFlags, lpExeNameW, &pdwSizeW);

    if (Result)
        Result = (0 != WideCharToMultiByte(CP_ACP, 0,
                                           lpExeNameW,
                                           -1,
                                           lpExeName,
                                           *pdwSize,
                                           NULL, NULL));

    if (Result)
        *pdwSize = strlen(lpExeName);

    RtlFreeHeap(RtlGetProcessHeap(), 0, lpExeNameW);
    return Result;
}

/*
 * @unimplemented
 */
HRESULT
WINAPI
GetApplicationRecoveryCallback(IN HANDLE hProcess,
                               OUT APPLICATION_RECOVERY_CALLBACK* pRecoveryCallback,
                               OUT PVOID* ppvParameter,
                               PDWORD dwPingInterval,
                               PDWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT
WINAPI
GetApplicationRestart(IN HANDLE hProcess,
                      OUT PWSTR pwzCommandline  OPTIONAL,
                      IN OUT PDWORD pcchSize,
                      OUT PDWORD pdwFlags  OPTIONAL)
{
    UNIMPLEMENTED;
    return E_FAIL;
}


/*
 * @unimplemented
 */
VOID
WINAPI
RecoveryFinished(IN BOOL bSuccess)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
HRESULT
WINAPI
RecoveryInProgress(OUT PBOOL pbCancelled)
{
    UNIMPLEMENTED;
    return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT
WINAPI
RegisterApplicationRecoveryCallback(IN APPLICATION_RECOVERY_CALLBACK pRecoveyCallback,
                                    IN PVOID pvParameter  OPTIONAL,
                                    DWORD dwPingInterval,
                                    DWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_FAIL;
}


/*
 * @unimplemented
 */
HRESULT
WINAPI
RegisterApplicationRestart(IN PCWSTR pwzCommandline  OPTIONAL,
                           IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_FAIL;
}

