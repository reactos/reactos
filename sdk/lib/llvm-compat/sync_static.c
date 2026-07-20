/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Statically linked SRW lock, condition variable and init-once surface for llvm-mingw runtimes
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

/*
 * libc++ references the Win7 SRW lock APIs (directly and through dllimport slots) and the Vista condition
 * variable and one-time initialization APIs (through dllimport slots). Bind them to the RTL implementation
 * linked statically from rtl_vista (sdk/lib/rtl's srw.c, condvar.c and runonce.c) instead of exporting them
 * from kernel32_vista.dll: modules get one self-contained, consistent synchronization implementation
 * (ReactOS' lock layout is not Windows-compatible, mixing implementations on the same lock would be fatal)
 * and no import that real Windows cannot satisfy.
 */

/* This TU defines the kernel32 Win7 surface itself: make the headers declare it, non-dllimport */
#define _KERNEL32_
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601

/* Bind the Rtl references to the static rtl_vista implementation, not to ntdll(_vista).dll imports */
#define _NTSYSTEM_

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <imp_alias.h>

/* The kernel32 names, for direct references */

VOID
WINAPI
InitializeSRWLock(PSRWLOCK SRWLock)
{
    RtlInitializeSRWLock((PRTL_SRWLOCK)SRWLock);
}

VOID
WINAPI
AcquireSRWLockExclusive(PSRWLOCK SRWLock)
{
    RtlAcquireSRWLockExclusive((PRTL_SRWLOCK)SRWLock);
}

VOID
WINAPI
AcquireSRWLockShared(PSRWLOCK SRWLock)
{
    RtlAcquireSRWLockShared((PRTL_SRWLOCK)SRWLock);
}

VOID
WINAPI
ReleaseSRWLockExclusive(PSRWLOCK SRWLock)
{
    RtlReleaseSRWLockExclusive((PRTL_SRWLOCK)SRWLock);
}

VOID
WINAPI
ReleaseSRWLockShared(PSRWLOCK SRWLock)
{
    RtlReleaseSRWLockShared((PRTL_SRWLOCK)SRWLock);
}

BOOLEAN
WINAPI
TryAcquireSRWLockExclusive(PSRWLOCK SRWLock)
{
    return RtlTryAcquireSRWLockExclusive((PRTL_SRWLOCK)SRWLock);
}

BOOLEAN
WINAPI
TryAcquireSRWLockShared(PSRWLOCK SRWLock)
{
    return RtlTryAcquireSRWLockShared((PRTL_SRWLOCK)SRWLock);
}

VOID
WINAPI
WakeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlWakeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}

VOID
WINAPI
WakeAllConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlWakeAllConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}

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
SleepConditionVariableSRW(PCONDITION_VARIABLE ConditionVariable, PSRWLOCK SRWLock, DWORD Timeout, ULONG Flags)
{
    NTSTATUS Status;
    LARGE_INTEGER Time;

    Status = RtlSleepConditionVariableSRW(ConditionVariable, SRWLock, GetNtTimeout(&Time, Timeout), Flags);
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
InitOnceExecuteOnce(PINIT_ONCE InitOnce, PINIT_ONCE_FN InitFn, PVOID Parameter, LPVOID *Context)
{
    return NT_SUCCESS(RtlRunOnceExecuteOnce(InitOnce,
                                            (PRTL_RUN_ONCE_INIT_FN)InitFn,
                                            Parameter,
                                            Context));
}

/* The dllimport slots, bound directly to the RTL functions where the signatures match */

IMP_ALIAS_STDCALL(InitializeSRWLock, 4, RtlInitializeSRWLock);
IMP_ALIAS_STDCALL(AcquireSRWLockExclusive, 4, RtlAcquireSRWLockExclusive);
IMP_ALIAS_STDCALL(AcquireSRWLockShared, 4, RtlAcquireSRWLockShared);
IMP_ALIAS_STDCALL(ReleaseSRWLockExclusive, 4, RtlReleaseSRWLockExclusive);
IMP_ALIAS_STDCALL(ReleaseSRWLockShared, 4, RtlReleaseSRWLockShared);
IMP_ALIAS_STDCALL(TryAcquireSRWLockExclusive, 4, RtlTryAcquireSRWLockExclusive);
IMP_ALIAS_STDCALL(TryAcquireSRWLockShared, 4, RtlTryAcquireSRWLockShared);
IMP_ALIAS_STDCALL(WakeConditionVariable, 4, RtlWakeConditionVariable);
IMP_ALIAS_STDCALL(WakeAllConditionVariable, 4, RtlWakeAllConditionVariable);
IMP_ALIAS_STDCALL(SleepConditionVariableSRW, 16, SleepConditionVariableSRW);
IMP_ALIAS_STDCALL(InitOnceExecuteOnce, 16, InitOnceExecuteOnce);
