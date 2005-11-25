/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    psfuncs.h

Abstract:

    Function definitions for the Process Manager

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _PSFUNCS_H
#define _PSFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <pstypes.h>

#ifndef NTOS_MODE_USER

//
// Win32K Process/Thread Functions
//
struct _W32THREAD*
NTAPI
PsGetWin32Thread(
    VOID
);

struct _W32PROCESS*
NTAPI
PsGetWin32Process(
    VOID
);

PVOID
NTAPI
PsGetProcessWin32Process(
    PEPROCESS Process
);

VOID
NTAPI
PsSetProcessWin32Process(
    PEPROCESS Process,
    PVOID Win32Process
);

VOID
NTAPI
PsSetThreadWin32Thread(
    PETHREAD Thread,
    PVOID Win32Thread
);

PVOID
NTAPI
PsGetThreadWin32Thread(
    PETHREAD Thread
);

VOID 
NTAPI
PsEstablishWin32Callouts(
    PW32_CALLOUT_DATA CalloutData
);

//
// Process Impersonation Functions
//
VOID
NTAPI
PsRevertThreadToSelf(
    IN PETHREAD Thread
);

//
// Misc. Functions
//
HANDLE
NTAPI
PsGetProcessId(PEPROCESS Process);

#endif

//
// Native Calls
//
NTSTATUS
NTAPI
NtAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
NTAPI
NtAlertThread(
    IN HANDLE ThreadHandle
);

NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    HANDLE JobHandle,
    HANDLE ProcessHandle
);

NTSTATUS
NTAPI
NtCreateJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

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

NTSTATUS
NTAPI
NtImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ThreadToImpersonate,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

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

NTSTATUS
NTAPI
NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
    HANDLE TerminationPort
);

NTSTATUS
NTAPI
NtResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
NTAPI
NtResumeProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
NTAPI
NtSetInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength
);

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

NTSTATUS
NTAPI
NtSuspendProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
NTAPI
NtSuspendThread(
    IN HANDLE ThreadHandle,
    IN PULONG PreviousSuspendCount
);

NTSTATUS
NTAPI
NtTerminateProcess(
    IN HANDLE ProcessHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
NtTerminateThread(
    IN HANDLE ThreadHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
NtTerminateJobObject(
    HANDLE JobHandle,
    NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
ZwAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
NTAPI
ZwAlertThread(
    IN HANDLE ThreadHandle
);

NTSTATUS
NTAPI
ZwAssignProcessToJobObject(
    HANDLE JobHandle,
    HANDLE ProcessHandle
);

NTSTATUS
NTAPI
ZwCreateJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

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

NTSTATUS
NTAPI
ZwImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ThreadToImpersonate,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSTATUS
NTAPI
ZwIsProcessInJob(
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSTATUS
NTAPI
ZwOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenThreadTokenEx(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

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

NTSTATUS
NTAPI
ZwQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
ZwRegisterThreadTerminatePort(
    HANDLE TerminationPort
);

NTSTATUS
NTAPI
ZwResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
NTAPI
ZwResumeProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
NTAPI
ZwSetInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength
);

NTSTATUS
NTAPI
ZwSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
);

NTSTATUS
NTAPI
ZwSuspendProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
NTAPI
ZwSuspendThread(
    IN HANDLE ThreadHandle,
    IN PULONG PreviousSuspendCount
);

NTSTATUS
NTAPI
ZwTerminateProcess(
    IN HANDLE ProcessHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
ZwTerminateThread(
    IN HANDLE ThreadHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
ZwTerminateJobObject(
    HANDLE JobHandle,
    NTSTATUS ExitStatus
);

#endif
