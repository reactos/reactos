/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    psfuncs.h

Abstract:

    Function definitions for the Process Manager

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _PSFUNCS_H
#define _PSFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <pstypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NTOS_MODE_USER

//
// Win32K Process/Thread Functions
//
NTKERNELAPI
struct _W32THREAD*
NTAPI
PsGetCurrentThreadWin32Thread(
    VOID
);

NTKERNELAPI
struct _W32PROCESS*
NTAPI
PsGetCurrentProcessWin32Process(
    VOID
);

NTKERNELAPI
PVOID
NTAPI
PsGetProcessWin32Process(
    PEPROCESS Process
);

NTKERNELAPI
VOID
NTAPI
PsSetProcessWin32Process(
    PEPROCESS Process,
    PVOID Win32Process
);

NTKERNELAPI
VOID
NTAPI
PsSetThreadWin32Thread(
    PETHREAD Thread,
    PVOID Win32Thread
);

NTKERNELAPI
PVOID
NTAPI
PsGetThreadWin32Thread(
    PETHREAD Thread
);

NTKERNELAPI
PTEB
NTAPI
PsGetThreadTeb(
    IN PETHREAD Thread
);

NTKERNELAPI
BOOLEAN
NTAPI
PsGetThreadHardErrorsAreDisabled(
    PETHREAD Thread
);

NTKERNELAPI
VOID
NTAPI
PsSetThreadHardErrorsAreDisabled(
    PETHREAD Thread,
    IN BOOLEAN Disabled
);

NTKERNELAPI
VOID
NTAPI
PsEstablishWin32Callouts(
    PWIN32_CALLOUTS_FPNS CalloutData
);

NTKERNELAPI
VOID
NTAPI
PsReturnProcessNonPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T    Amount
);

//
// Process Impersonation Functions
//
NTKERNELAPI
VOID
NTAPI
PsRevertThreadToSelf(
    IN PETHREAD Thread
);

//
// Misc. Functions
//
NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessThreadByCid(
    IN PCLIENT_ID Cid,
    OUT PEPROCESS *Process OPTIONAL,
    OUT PETHREAD *Thread
);

BOOLEAN
NTAPI
PsIsProtectedProcess(
    IN PEPROCESS Process
);

NTKERNELAPI
BOOLEAN
NTAPI
PsIsSystemProcess(
    IN PEPROCESS Process
);

//
// Quota Functions
//
NTKERNELAPI
VOID
NTAPI
PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN SIZE_T    Amount
);

NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessNonPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T    Amount
);

NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T    Amount
);

NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN SIZE_T    Amount
);

NTKERNELAPI
VOID
NTAPI
PsReturnPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN SIZE_T    Amount
);

NTKERNELAPI
VOID
NTAPI
PsReturnProcessNonPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T    Amount
);

NTKERNELAPI
VOID
NTAPI
PsReturnProcessPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T    Amount
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

typedef ULONG APPHELPCACHESERVICECLASS;
NTSYSCALLAPI
NTSTATUS
NTAPI
NtApphelpCacheControl(
    IN APPHELPCACHESERVICECLASS Service,
    IN PVOID ServiceData
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThread(
    IN HANDLE ThreadHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    HANDLE JobHandle,
    HANDLE ProcessHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtCreateJobSet(
    IN ULONG NumJob,
    IN PJOB_SET_ARRAY UserJobSet,
    IN ULONG Flags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcessEx(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN ULONG Flags,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    IN BOOLEAN InJob
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB UserStack,
    IN BOOLEAN CreateSuspended
);

#ifndef NTOS_MODE_USER
#if defined(_M_IX86)
FORCEINLINE
PTEB
NtCurrentTeb(VOID)
{
#ifndef __GNUC__
    return (PTEB)(ULONG_PTR)__readfsdword(0x18);
#else
    struct _TEB *ret;

    __asm__ __volatile__ (
        "movl %%fs:0x18, %0\n"
        : "=r" (ret)
        : /* no inputs */
    );

    return ret;
#endif
}
#elif defined (_M_AMD64)
FORCEINLINE struct _TEB * NtCurrentTeb(VOID)
{
    return (struct _TEB *)__readgsqword(FIELD_OFFSET(NT_TIB, Self));
}
#endif
#else
struct _TEB * NtCurrentTeb(void);
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ThreadToImpersonate,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtIsProcessInJob(
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadTokenEx(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength,
    PULONG ReturnLength
);

#ifndef _NTDDK_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
    HANDLE TerminationPort
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeProcess(
    IN HANDLE ProcessHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendProcess(
    IN HANDLE ProcessHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendThread(
    IN HANDLE ThreadHandle,
    IN PULONG PreviousSuspendCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateProcess(
    IN HANDLE ProcessHandle,
    IN NTSTATUS ExitStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateThread(
    IN HANDLE ThreadHandle,
    IN NTSTATUS ExitStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateJobObject(
    HANDLE JobHandle,
    NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread(
    IN HANDLE ThreadHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAssignProcessToJobObject(
    HANDLE JobHandle,
    HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB UserStack,
    IN BOOLEAN CreateSuspended
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ThreadToImpersonate,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSYSAPI
NTSTATUS
NTAPI
ZwIsProcessInJob(
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadTokenEx(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength,
    PULONG ReturnLength
);

#ifndef _NTDDK_
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRegisterThreadTerminatePort(
    HANDLE TerminationPort
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResumeProcess(
    IN HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSuspendProcess(
    IN HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSuspendThread(
    IN HANDLE ThreadHandle,
    IN PULONG PreviousSuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess(
    IN HANDLE ProcessHandle,
    IN NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateThread(
    IN HANDLE ThreadHandle,
    IN NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateJobObject(
    HANDLE JobHandle,
    NTSTATUS ExitStatus
);

#ifdef __cplusplus
}
#endif

#endif
