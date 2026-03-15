/*
 * PROJECT:     ReactOS llvm-mingw compatibility shims
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Provide Vista-era Win32 sync APIs when kernel32_vista is absent
 */

#include <psdk/windef.h>
#include <psdk/winnt.h>
#define _KERNEL32_
#include <psdk/winbase.h>
#undef _KERNEL32_
#undef NTSYSAPI
#undef NTSYSCALLAPI
#define NTSYSAPI
#define NTSYSCALLAPI
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#endif
#include <ndk/rtlfuncs.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <vcruntime/_mingw_mac.h>

#if (_WIN32_WINNT < 0x0600)
typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;
typedef RTL_CONDITION_VARIABLE CONDITION_VARIABLE, *PCONDITION_VARIABLE;
#endif

#if (_WIN32_WINNT < _WIN32_WINNT_VISTA) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize(
    _Out_ PRTL_RUN_ONCE RunOnce);

_Maybe_raises_SEH_exception_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ __inner_callback PRTL_RUN_ONCE_INIT_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID *Context);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _Outptr_opt_result_maybenull_ PVOID *Context);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context);
#endif

static inline PLARGE_INTEGER
llvmcompat_get_nt_timeout(PLARGE_INTEGER Time, DWORD Timeout)
{
    if (Timeout == INFINITE)
        return NULL;

    Time->QuadPart = (LONGLONG)Timeout * -10000;
    return Time;
}

static inline VOID
llvmcompat_set_last_error_from_status(NTSTATUS Status)
{
    SetLastError(RtlNtStatusToDosError(Status));
}

#ifdef _M_IX86
#define DEFINE_STDCALL_IMPORT_ALIAS(name, bytes) \
    extern const void * const llvmcompat_imp_##name __asm__("_imp__" #name "@" #bytes); \
    const void * const llvmcompat_imp_##name = (const void *)(ULONG_PTR)name
#else
#define DEFINE_STDCALL_IMPORT_ALIAS(name, bytes) \
    const void * const __MINGW_IMP_SYMBOL(name) = (const void *)(ULONG_PTR)name
#endif

VOID
WINAPI
AcquireSRWLockExclusive(PSRWLOCK Lock)
{
    RtlAcquireSRWLockExclusive((PRTL_SRWLOCK)Lock);
}
DEFINE_STDCALL_IMPORT_ALIAS(AcquireSRWLockExclusive, 4);

VOID
WINAPI
AcquireSRWLockShared(PSRWLOCK Lock)
{
    RtlAcquireSRWLockShared((PRTL_SRWLOCK)Lock);
}
DEFINE_STDCALL_IMPORT_ALIAS(AcquireSRWLockShared, 4);

VOID
WINAPI
InitializeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlInitializeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}
DEFINE_STDCALL_IMPORT_ALIAS(InitializeConditionVariable, 4);

VOID
WINAPI
InitializeSRWLock(PSRWLOCK Lock)
{
    RtlInitializeSRWLock((PRTL_SRWLOCK)Lock);
}
DEFINE_STDCALL_IMPORT_ALIAS(InitializeSRWLock, 4);

VOID
WINAPI
ReleaseSRWLockExclusive(PSRWLOCK Lock)
{
    RtlReleaseSRWLockExclusive((PRTL_SRWLOCK)Lock);
}
DEFINE_STDCALL_IMPORT_ALIAS(ReleaseSRWLockExclusive, 4);

VOID
WINAPI
ReleaseSRWLockShared(PSRWLOCK Lock)
{
    RtlReleaseSRWLockShared((PRTL_SRWLOCK)Lock);
}
DEFINE_STDCALL_IMPORT_ALIAS(ReleaseSRWLockShared, 4);

BOOL
WINAPI
SleepConditionVariableCS(
    PCONDITION_VARIABLE ConditionVariable,
    PCRITICAL_SECTION CriticalSection,
    DWORD Timeout)
{
    LARGE_INTEGER Time;
    NTSTATUS Status;

    Status = RtlSleepConditionVariableCS((PRTL_CONDITION_VARIABLE)ConditionVariable,
                                         (PRTL_CRITICAL_SECTION)CriticalSection,
                                         llvmcompat_get_nt_timeout(&Time, Timeout));
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        llvmcompat_set_last_error_from_status(Status);
        return FALSE;
    }

    return TRUE;
}
DEFINE_STDCALL_IMPORT_ALIAS(SleepConditionVariableCS, 12);

BOOL
WINAPI
SleepConditionVariableSRW(
    PCONDITION_VARIABLE ConditionVariable,
    PSRWLOCK Lock,
    DWORD Timeout,
    ULONG Flags)
{
    LARGE_INTEGER Time;
    NTSTATUS Status;

    Status = RtlSleepConditionVariableSRW((PRTL_CONDITION_VARIABLE)ConditionVariable,
                                          (PRTL_SRWLOCK)Lock,
                                          llvmcompat_get_nt_timeout(&Time, Timeout),
                                          Flags);
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        llvmcompat_set_last_error_from_status(Status);
        return FALSE;
    }

    return TRUE;
}
DEFINE_STDCALL_IMPORT_ALIAS(SleepConditionVariableSRW, 16);

VOID
WINAPI
WakeAllConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlWakeAllConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}
DEFINE_STDCALL_IMPORT_ALIAS(WakeAllConditionVariable, 4);

VOID
WINAPI
WakeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlWakeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}
DEFINE_STDCALL_IMPORT_ALIAS(WakeConditionVariable, 4);

VOID
WINAPI
InitOnceInitialize(PINIT_ONCE InitOnce)
{
    RtlRunOnceInitialize((PRTL_RUN_ONCE)InitOnce);
}
DEFINE_STDCALL_IMPORT_ALIAS(InitOnceInitialize, 4);

BOOL
WINAPI
InitOnceBeginInitialize(
    LPINIT_ONCE lpInitOnce,
    DWORD dwFlags,
    PBOOL fPending,
    LPVOID *lpContext)
{
    NTSTATUS Status;

    Status = RtlRunOnceBeginInitialize((PRTL_RUN_ONCE)lpInitOnce, dwFlags, lpContext);
    if (!NT_SUCCESS(Status))
    {
        llvmcompat_set_last_error_from_status(Status);
        return FALSE;
    }

    *fPending = (Status == STATUS_PENDING);
    return TRUE;
}
DEFINE_STDCALL_IMPORT_ALIAS(InitOnceBeginInitialize, 16);

BOOL
WINAPI
InitOnceComplete(
    LPINIT_ONCE lpInitOnce,
    DWORD dwFlags,
    LPVOID lpContext)
{
    NTSTATUS Status;

    Status = RtlRunOnceComplete((PRTL_RUN_ONCE)lpInitOnce, dwFlags, lpContext);
    if (!NT_SUCCESS(Status))
    {
        llvmcompat_set_last_error_from_status(Status);
        return FALSE;
    }

    return TRUE;
}
DEFINE_STDCALL_IMPORT_ALIAS(InitOnceComplete, 12);

BOOL
WINAPI
InitOnceExecuteOnce(
    PINIT_ONCE InitOnce,
    PINIT_ONCE_FN InitFn,
    PVOID Parameter,
    LPVOID *Context)
{
    NTSTATUS Status;

    Status = RtlRunOnceExecuteOnce((PRTL_RUN_ONCE)InitOnce,
                                   (PRTL_RUN_ONCE_INIT_FN)InitFn,
                                   Parameter,
                                   Context);
    if (!NT_SUCCESS(Status))
    {
        llvmcompat_set_last_error_from_status(Status);
        return FALSE;
    }

    return TRUE;
}
DEFINE_STDCALL_IMPORT_ALIAS(InitOnceExecuteOnce, 16);
