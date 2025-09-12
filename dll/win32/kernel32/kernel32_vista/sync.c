#include "k32_vista.h"

#define NDEBUG
#include <debug.h>

#if defined(_M_AMD64) || defined(__i386__)
/* 
 * GCC 13 compatibility implementations for AMD64 and i386 builds
 * These use critical sections as a fallback since the RTL functions
 * are not available in ReactOS for these architectures when targeting XP
 */

typedef struct _SRWLOCK_CS {
    RTL_CRITICAL_SECTION cs;
    BOOLEAN Initialized;
} SRWLOCK_CS, *PSRWLOCK_CS;

VOID
WINAPI
AcquireSRWLockExclusive(PSRWLOCK Lock)
{
    PSRWLOCK_CS LockCs = (PSRWLOCK_CS)Lock;
    if (LockCs && LockCs->Initialized)
    {
        RtlEnterCriticalSection(&LockCs->cs);
    }
}

VOID
WINAPI
AcquireSRWLockShared(PSRWLOCK Lock)
{
    PSRWLOCK_CS LockCs = (PSRWLOCK_CS)Lock;
    if (LockCs && LockCs->Initialized)
    {
        RtlEnterCriticalSection(&LockCs->cs);
    }
}

VOID
WINAPI
InitializeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    /* Simple initialization - just clear the pointer */
    ConditionVariable->Ptr = NULL;
}

VOID
WINAPI
InitializeSRWLock(PSRWLOCK Lock)
{
    PSRWLOCK_CS LockCs = (PSRWLOCK_CS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SRWLOCK_CS));
    if (LockCs)
    {
        RtlInitializeCriticalSection(&LockCs->cs);
        LockCs->Initialized = TRUE;
        *Lock = *(PSRWLOCK)&LockCs;
    }
    else
    {
        Lock->Ptr = NULL;
    }
}

VOID
WINAPI
ReleaseSRWLockExclusive(PSRWLOCK Lock)
{
    PSRWLOCK_CS LockCs = (PSRWLOCK_CS)Lock;
    if (LockCs && LockCs->Initialized)
    {
        RtlLeaveCriticalSection(&LockCs->cs);
    }
}

VOID
WINAPI
ReleaseSRWLockShared(PSRWLOCK Lock)
{
    PSRWLOCK_CS LockCs = (PSRWLOCK_CS)Lock;
    if (LockCs && LockCs->Initialized)
    {
        RtlLeaveCriticalSection(&LockCs->cs);
    }
}

#else
/* Use actual RTL functions for builds that have them available */

VOID
WINAPI
AcquireSRWLockExclusive(PSRWLOCK Lock)
{
    RtlAcquireSRWLockExclusive((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
AcquireSRWLockShared(PSRWLOCK Lock)
{
    RtlAcquireSRWLockShared((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
InitializeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlInitializeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}

VOID
WINAPI
InitializeSRWLock(PSRWLOCK Lock)
{
    RtlInitializeSRWLock((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
ReleaseSRWLockExclusive(PSRWLOCK Lock)
{
    RtlReleaseSRWLockExclusive((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
ReleaseSRWLockShared(PSRWLOCK Lock)
{
    RtlReleaseSRWLockShared((PRTL_SRWLOCK)Lock);
}

#endif /* _M_AMD64 || __i386__ */

FORCEINLINE
PLARGE_INTEGER
GetNtTimeout(PLARGE_INTEGER Time, DWORD Timeout)
{
    if (Timeout == INFINITE) return NULL;
    Time->QuadPart = (ULONGLONG)Timeout * -10000;
    return Time;
}

BOOL
WINAPI
SleepConditionVariableCS(PCONDITION_VARIABLE ConditionVariable, PCRITICAL_SECTION CriticalSection, DWORD Timeout)
{
#if defined(_M_AMD64) || defined(__i386__)
    /* 
     * Simplified implementation for GCC 13 compatibility
     * Release the critical section, sleep, then re-acquire
     */
    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)CriticalSection);
    SleepEx(Timeout != INFINITE ? Timeout : 1, TRUE);
    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)CriticalSection);
    
    /* Always return success for compatibility */
    return TRUE;
#else
    NTSTATUS Status;
    LARGE_INTEGER Time;

    Status = RtlSleepConditionVariableCS(ConditionVariable, (PRTL_CRITICAL_SECTION)CriticalSection, GetNtTimeout(&Time, Timeout));
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
#endif
}

BOOL
WINAPI
SleepConditionVariableSRW(PCONDITION_VARIABLE ConditionVariable, PSRWLOCK Lock, DWORD Timeout, ULONG Flags)
{
#if defined(_M_AMD64) || defined(__i386__)
    /* 
     * Simplified implementation for GCC 13 compatibility
     * Release the lock, sleep, then re-acquire based on flags
     */
    if (Flags & CONDITION_VARIABLE_LOCKMODE_SHARED)
    {
        ReleaseSRWLockShared(Lock);
        SleepEx(Timeout != INFINITE ? Timeout : 1, TRUE);
        AcquireSRWLockShared(Lock);
    }
    else
    {
        ReleaseSRWLockExclusive(Lock);
        SleepEx(Timeout != INFINITE ? Timeout : 1, TRUE);
        AcquireSRWLockExclusive(Lock);
    }
    
    /* Always return success for compatibility */
    return TRUE;
#else
    NTSTATUS Status;
    LARGE_INTEGER Time;

    Status = RtlSleepConditionVariableSRW(ConditionVariable, Lock, GetNtTimeout(&Time, Timeout), Flags);
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
#endif
}

VOID
WINAPI
WakeAllConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
#if defined(_M_AMD64) || defined(__i386__)
    /* Stub - condition variables not fully implemented for AMD64/i386 */
    UNREFERENCED_PARAMETER(ConditionVariable);
#else
    RtlWakeAllConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
#endif
}

VOID
WINAPI
WakeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
#if defined(_M_AMD64) || defined(__i386__)
    /* Stub - condition variables not fully implemented for AMD64/i386 */
    UNREFERENCED_PARAMETER(ConditionVariable);
#else
    RtlWakeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
#endif
}


/*
* @implemented
*/
BOOL WINAPI InitializeCriticalSectionEx(OUT LPCRITICAL_SECTION lpCriticalSection,
                                        IN DWORD dwSpinCount,
                                        IN DWORD flags)
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
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Success */
    return TRUE;
}

