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
PVOID
NTAPI
PsGetCurrentThreadWin32Thread(
    VOID
);

NTKERNELAPI
PVOID
NTAPI
PsGetCurrentProcessWin32Process(
    VOID
);

NTKERNELAPI
PVOID
NTAPI
PsGetProcessWin32Process(
    _In_ PEPROCESS Process
);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetProcessWin32Process(
    _Inout_ PEPROCESS Process,
    _In_opt_ PVOID Win32Process,
    _In_opt_ PVOID OldWin32Process
);

NTKERNELAPI
PVOID
NTAPI
PsSetThreadWin32Thread(
    _Inout_ PETHREAD Thread,
    _In_opt_ PVOID Win32Thread,
    _In_opt_ PVOID OldWin32Thread
);

NTKERNELAPI
PVOID
NTAPI
PsGetThreadWin32Thread(
    _In_ PETHREAD Thread
);

NTKERNELAPI
PVOID
NTAPI
PsGetProcessWin32WindowStation(
    _In_ PEPROCESS Process
);

NTKERNELAPI
VOID
NTAPI
PsSetProcessWindowStation(
    _Inout_ PEPROCESS Process,
    _In_opt_ PVOID WindowStation
);

NTKERNELAPI
PTEB
NTAPI
PsGetThreadTeb(
    _In_ PETHREAD Thread
);

NTKERNELAPI
HANDLE
NTAPI
PsGetThreadId(
    _In_ PETHREAD Thread
);

NTKERNELAPI
PEPROCESS
NTAPI
PsGetThreadProcess(
    _In_ PETHREAD Thread
);

NTKERNELAPI
ULONG
NTAPI
PsGetThreadFreezeCount(
    _In_ PETHREAD Thread
);

NTKERNELAPI
BOOLEAN
NTAPI
PsGetThreadHardErrorsAreDisabled(
    _In_ PETHREAD Thread
);

NTKERNELAPI
VOID
NTAPI
PsSetThreadHardErrorsAreDisabled(
    _Inout_ PETHREAD Thread,
    _In_ BOOLEAN Disabled
);

NTKERNELAPI
VOID
NTAPI
PsEstablishWin32Callouts(
    _In_ PWIN32_CALLOUTS_FPNS CalloutData
);

NTKERNELAPI
VOID
NTAPI
PsReturnProcessNonPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T    Amount
);

NTKERNELAPI
ULONG
NTAPI
PsGetCurrentProcessSessionId(
    VOID
);

//
// Process Impersonation Functions
//
NTKERNELAPI
BOOLEAN
NTAPI
PsIsThreadImpersonating(
    _In_ PETHREAD Thread
);

NTKERNELAPI
VOID
NTAPI
PsRevertThreadToSelf(
    _Inout_ PETHREAD Thread
);

//
// Misc. Functions
//
NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessThreadByCid(
    _In_ PCLIENT_ID Cid,
    _Out_opt_ PEPROCESS *Process,
    _Out_ PETHREAD *Thread
);

BOOLEAN
NTAPI
PsIsProtectedProcess(
    _In_ PEPROCESS Process
);

NTKERNELAPI
BOOLEAN
NTAPI
PsIsSystemProcess(
    _In_ PEPROCESS Process
);

VOID
NTAPI
PsSetProcessPriorityByClass(
    _In_ PEPROCESS Process,
    _In_ PSPROCESSPRIORITYMODE Type
);

HANDLE
NTAPI
PsGetProcessInheritedFromUniqueProcessId(
    _In_ PEPROCESS Process
);

NTKERNELAPI
NTSTATUS
NTAPI
PsGetProcessExitStatus(
    _In_ PEPROCESS Process
);

NTKERNELAPI
ULONG
NTAPI
PsGetProcessSessionId(
    _In_ PEPROCESS Process
);

NTKERNELAPI
BOOLEAN
NTAPI
PsGetProcessExitProcessCalled(
    _In_ PEPROCESS Process
);

//
// Quota Functions
//
NTKERNELAPI
VOID
NTAPI
PsChargePoolQuota(
    _In_ PEPROCESS Process,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T    Amount
);

NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessNonPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T    Amount
);

NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T    Amount
);

NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessPoolQuota(
    _In_ PEPROCESS Process,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T    Amount
);

NTKERNELAPI
VOID
NTAPI
PsReturnPoolQuota(
    _In_ PEPROCESS Process,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T    Amount
);

NTKERNELAPI
VOID
NTAPI
PsReturnProcessNonPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T    Amount
);

NTKERNELAPI
VOID
NTAPI
PsReturnProcessPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T    Amount
);

NTKERNELAPI
PVOID
NTAPI
PsGetProcessSecurityPort(
    _In_ PEPROCESS Process
);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetProcessSecurityPort(
    _Inout_ PEPROCESS Process,
    _In_ PVOID SecurityPort
);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentThreadProcessId(
    VOID
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG SuspendCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtApphelpCacheControl(
    _In_ APPHELPCACHESERVICECLASS Service,
    _In_opt_ PAPPHELP_CACHE_SERVICE_LOOKUP ServiceData
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThread(
    _In_ HANDLE ThreadHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    _In_ HANDLE JobHandle,
    _In_ HANDLE ProcessHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtCreateJobSet(
    _In_ ULONG NumJob,
    _In_ PJOB_SET_ARRAY UserJobSet,
    _In_ ULONG Flags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ParentProcess,
    _In_ BOOLEAN InheritObjectTable,
    _In_opt_ HANDLE SectionHandle,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE ExceptionPort
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcessEx(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ParentProcess,
    _In_ ULONG Flags,
    _In_opt_ HANDLE SectionHandle,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE ExceptionPort,
    _In_ BOOLEAN InJob
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _Out_ PCLIENT_ID ClientId,
    _In_ PCONTEXT ThreadContext,
    _In_ PINITIAL_TEB UserStack,
    _In_ BOOLEAN CreateSuspended
);

#ifndef NTOS_MODE_USER
FORCEINLINE struct _TEB * NtCurrentTeb(VOID)
{
#if defined(_M_IX86)
    return (PTEB)__readfsdword(0x18);
#elif defined (_M_AMD64)
    return (struct _TEB *)__readgsqword(FIELD_OFFSET(NT_TIB, Self));
#elif defined (_M_ARM)
    return (struct _TEB *)KeGetPcr()->Used_Self;
#endif
}
#else
struct _TEB * NtCurrentTeb(void);
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateThread(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ThreadToImpersonate,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtIsProcessInJob(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE JobHandle
);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PCLIENT_ID ClientId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadTokenEx(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _In_ ULONG HandleAttributes,
    _Out_ PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationJobObject(
    _In_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobInformationClass,
    _Out_bytecap_(JobInformationLength) PVOID JobInformation,
    _In_ ULONG JobInformationLength,
    _Out_ PULONG ReturnLength
);

#ifndef _NTDDK_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
);
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_ PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
    _In_ HANDLE TerminationPort
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG SuspendCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeProcess(
    _In_ HANDLE ProcessHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationJobObject(
    _In_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobInformationClass,
    _In_bytecount_(JobInformationLength) PVOID JobInformation,
    _In_ ULONG JobInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendProcess(
    _In_ HANDLE ProcessHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendThread(
    _In_ HANDLE ThreadHandle,
    _In_ PULONG PreviousSuspendCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateThread(
    _In_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateJobObject(
    _In_ HANDLE JobHandle,
    _In_ NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG SuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread(
    _In_ HANDLE ThreadHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAssignProcessToJobObject(
    _In_ HANDLE JobHandle,
    _In_ HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ParentProcess,
    _In_ BOOLEAN InheritObjectTable,
    _In_opt_ HANDLE SectionHandle,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE ExceptionPort
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _Out_ PCLIENT_ID ClientId,
    _In_ PCONTEXT ThreadContext,
    _In_ PINITIAL_TEB UserStack,
    _In_ BOOLEAN CreateSuspended
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateThread(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ThreadToImpersonate,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSYSAPI
NTSTATUS
NTAPI
ZwIsProcessInJob(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE JobHandle
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessTokenEx(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _Out_ PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PCLIENT_ID ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadTokenEx(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _In_ ULONG HandleAttributes,
    _Out_ PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationJobObject(
    _In_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobInformationClass,
    _Out_bytecap_(JobInformationLength) PVOID JobInformation,
    _In_ ULONG JobInformationLength,
    _Out_ PULONG ReturnLength
);

#ifndef _NTDDK_
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_ PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRegisterThreadTerminatePort(
    _In_ HANDLE TerminationPort
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG SuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResumeProcess(
    _In_ HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationJobObject(
    _In_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobInformationClass,
    _In_ PVOID JobInformation,
    _In_ ULONG JobInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSuspendProcess(
    _In_ HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSuspendThread(
    _In_ HANDLE ThreadHandle,
    _In_ PULONG PreviousSuspendCount
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess (
    _In_opt_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateThread(
    _In_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateJobObject(
    _In_ HANDLE JobHandle,
    _In_ NTSTATUS ExitStatus
);

#ifdef __cplusplus
}
#endif

#endif
