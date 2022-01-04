/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    exfuncs.h

Abstract:

    Function definitions for the Executive.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _EXFUNCS_H
#define _EXFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <pstypes.h>
#include <extypes.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Don't include WMI headers just for one define
//
#ifndef PEVENT_TRACE_HEADER_DEFINED
#define PEVENT_TRACE_HEADER_DEFINED
typedef struct _EVENT_TRACE_HEADER *PEVENT_TRACE_HEADER;
#endif

#ifndef NTOS_MODE_USER
//
// Fast Mutex functions
//
VOID
FASTCALL
ExEnterCriticalRegionAndAcquireFastMutexUnsafe(
    _Inout_ PFAST_MUTEX FastMutex
);

VOID
FASTCALL
ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(
    _Inout_ PFAST_MUTEX FastMutex
);

//
// Pushlock functions
//
VOID
FASTCALL
ExfAcquirePushLockExclusive(
    _Inout_ PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfAcquirePushLockShared(
    _Inout_ PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfReleasePushLock(
    _Inout_ PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfReleasePushLockExclusive(
    _Inout_ PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfReleasePushLockShared(
    _Inout_ PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfTryToWakePushLock(
    _Inout_ PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfUnblockPushLock(
    _Inout_ PEX_PUSH_LOCK PushLock,
    _Inout_ PVOID CurrentWaitBlock
);

//
// Handle Table Functions
//
NTKERNELAPI
BOOLEAN
NTAPI
ExEnumHandleTable(
    _In_ PHANDLE_TABLE HandleTable,
    _In_ PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
    _Inout_ PVOID Context,
    _Out_opt_ PHANDLE Handle
);

//
// HardError Functions
//
NTSTATUS
NTAPI
ExRaiseHardError(
    _In_ NTSTATUS ErrorStatus,
    _In_ ULONG NumberOfParameters,
    _In_ ULONG UnicodeStringParameterMask,
    _In_ PULONG_PTR Parameters,
    _In_ ULONG ValidResponseOptions,
    _Out_ PULONG Response
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAddAtom(
    _In_ PWSTR AtomName,
    _In_ ULONG AtomNameLength,
    _Inout_ PRTL_ATOM Atom
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelTimer(
    _In_ HANDLE TimerHandle,
    _Out_opt_ PBOOLEAN CurrentState
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtClearEvent(
    _In_ HANDLE EventHandle
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ EVENT_TYPE EventType,
    _In_ BOOLEAN InitialState
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEventPair(
    _Out_ PHANDLE EventPairHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKeyedEvent(
    _Out_ PHANDLE OutHandle,
    _In_ ACCESS_MASK AccessMask,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateMutant(
    _Out_ PHANDLE MutantHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ BOOLEAN InitialOwner
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSemaphore(
    _Out_ PHANDLE SemaphoreHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ LONG InitialCount,
    _In_ LONG MaximumCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTimer(
    _Out_ PHANDLE TimerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TIMER_TYPE TimerType
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteAtom(
    _In_ RTL_ATOM Atom
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDisplayString(
    _In_ PUNICODE_STRING DisplayString
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateSystemEnvironmentValuesEx(
    _In_ ULONG InformationClass,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFindAtom(
    _In_  PWSTR AtomName,
    _In_  ULONG AtomNameLength,
    _Out_opt_ PRTL_ATOM Atom
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyedEvent(
    _Out_ PHANDLE OutHandle,
    _In_ ACCESS_MASK AccessMask,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEventPair(
    _Out_ PHANDLE EventPairHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenMutant(
    _Out_ PHANDLE MutantHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSemaphore(
    _Out_ PHANDLE SemaphoreHandle,
    _In_ ACCESS_MASK DesiredAcces,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTimer(
    _Out_ PHANDLE TimerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPulseEvent(
    _In_ HANDLE EventHandle,
    _In_opt_ PLONG PulseCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDefaultLocale(
    _In_ BOOLEAN UserProfile,
    _Out_ PLCID DefaultLocaleId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDefaultUILanguage(
    LANGID* LanguageId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryEvent(
    _In_ HANDLE EventHandle,
    _In_ EVENT_INFORMATION_CLASS EventInformationClass,
    _Out_ PVOID EventInformation,
    _In_ ULONG EventInformationLength,
    _Out_ PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationAtom(
    _In_  RTL_ATOM Atom,
    _In_  ATOM_INFORMATION_CLASS AtomInformationClass,
    _Out_ PVOID AtomInformation,
    _In_  ULONG AtomInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInstallUILanguage(
    LANGID* LanguageId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryMutant(
    _In_ HANDLE MutantHandle,
    _In_ MUTANT_INFORMATION_CLASS MutantInformationClass,
    _Out_ PVOID MutantInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySemaphore(
    _In_ HANDLE SemaphoreHandle,
    _In_ SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    _Out_ PVOID SemaphoreInformation,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValue(
    _In_ PUNICODE_STRING Name,
    _Out_ PWSTR Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValueEx(
    _In_ PUNICODE_STRING VariableName,
    _In_ LPGUID VendorGuid,
    _In_ PVOID Value,
    _Inout_ PULONG ReturnLength,
    _Inout_ PULONG Attributes
);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryTimer(
    _In_ HANDLE TimerHandle,
    _In_ TIMER_INFORMATION_CLASS TimerInformationClass,
    _Out_ PVOID TimerInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseHardError(
    _In_ NTSTATUS ErrorStatus,
    _In_ ULONG NumberOfParameters,
    _In_ ULONG UnicodeStringParameterMask,
    _In_ PULONG_PTR Parameters,
    _In_ ULONG ValidResponseOptions,
    _Out_ PULONG Response
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseMutant(
    _In_ HANDLE MutantHandle,
    _In_opt_ PLONG ReleaseCount
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseKeyedEvent(
    _In_opt_ HANDLE EventHandle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseSemaphore(
    _In_ HANDLE SemaphoreHandle,
    _In_ LONG ReleaseCount,
    _Out_opt_ PLONG PreviousCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG NumberOfWaitingThreads
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultLocale(
    _In_ BOOLEAN UserProfile,
    _In_ LCID DefaultLocaleId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultUILanguage(
    LANGID LanguageId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultHardErrorPort(
    _In_ HANDLE PortHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEventBoostPriority(
    _In_ HANDLE EventHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetHighEventPair(
    _In_ HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetHighWaitLowEventPair(
    _In_ HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLowEventPair(
    _In_ HANDLE EventPair
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLowWaitHighEventPair(
    _In_ HANDLE EventPair
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValue(
    _In_ PUNICODE_STRING VariableName,
    _In_ PUNICODE_STRING Value
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValueEx(
    _In_ PUNICODE_STRING VariableName,
    _In_ LPGUID VendorGuid,
    _In_ PVOID Value,
    _Inout_ PULONG ReturnLength,
    _Inout_ PULONG Attributes
);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_reads_bytes_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimer(
    _In_ HANDLE TimerHandle,
    _In_ PLARGE_INTEGER DueTime,
    _In_ PTIMER_APC_ROUTINE TimerApcRoutine,
    _In_ PVOID TimerContext,
    _In_ BOOLEAN WakeTimer,
    _In_opt_ LONG Period,
    _Out_opt_ PBOOLEAN PreviousState
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetUuidSeed(
    _In_ PUCHAR UuidSeed
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtShutdownSystem(
    _In_ SHUTDOWN_ACTION Action
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForKeyedEvent(
    _In_opt_ HANDLE EventHandle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitHighEventPair(
    _In_ HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitLowEventPair(
    _In_ HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceEvent(
    _In_ ULONG TraceHandle,
    _In_ ULONG Flags,
    _In_ ULONG TraceHeaderLength,
    _In_ PEVENT_TRACE_HEADER TraceHeader
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAddAtom(
    _In_ PWSTR AtomName,
    _In_ ULONG AtomNameLength,
    _Inout_ PRTL_ATOM Atom
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwCancelTimer(
    _In_ HANDLE TimerHandle,
    _Out_opt_ PBOOLEAN CurrentState
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent(
    _In_ HANDLE EventHandle
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ EVENT_TYPE EventType,
    _In_ BOOLEAN InitialState
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEventPair(
    _Out_ PHANDLE EventPairHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKeyedEvent(
    _Out_ PHANDLE OutHandle,
    _In_ ACCESS_MASK AccessMask,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateMutant(
    _Out_ PHANDLE MutantHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ BOOLEAN InitialOwner
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSemaphore(
    _Out_ PHANDLE SemaphoreHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ LONG InitialCount,
    _In_ LONG MaximumCount
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateTimer(
    _Out_ PHANDLE TimerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TIMER_TYPE TimerType
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteAtom(
    _In_ RTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
    _In_ PUNICODE_STRING DisplayString
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFindAtom(
    _In_  PWSTR AtomName,
    _In_  ULONG AtomNameLength,
    _Out_opt_ PRTL_ATOM Atom
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEventPair(
    _Out_ PHANDLE EventPairHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenMutant(
    _Out_ PHANDLE MutantHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSemaphore(
    _Out_ PHANDLE SemaphoreHandle,
    _In_ ACCESS_MASK DesiredAcces,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenTimer(
    _Out_ PHANDLE TimerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent(
    _In_ HANDLE EventHandle,
    _In_opt_ PLONG PulseCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale(
    _In_ BOOLEAN UserProfile,
    _Out_ PLCID DefaultLocaleId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultUILanguage(
    LANGID* LanguageId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEvent(
    _In_ HANDLE EventHandle,
    _In_ EVENT_INFORMATION_CLASS EventInformationClass,
    _Out_ PVOID EventInformation,
    _In_ ULONG EventInformationLength,
    _Out_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationAtom(
    _In_  RTL_ATOM Atom,
    _In_  ATOM_INFORMATION_CLASS AtomInformationClass,
    _Out_ PVOID AtomInformation,
    _In_  ULONG AtomInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInstallUILanguage(
    LANGID* LanguageId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMutant(
    _In_ HANDLE MutantHandle,
    _In_ MUTANT_INFORMATION_CLASS MutantInformationClass,
    _Out_ PVOID MutantInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySemaphore(
    _In_ HANDLE SemaphoreHandle,
    _In_ SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    _Out_ PVOID SemaphoreInformation,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemEnvironmentValue(
    _In_ PUNICODE_STRING Name,
    _Out_ PWSTR Value,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryTimer(
    _In_ HANDLE TimerHandle,
    _In_ TIMER_INFORMATION_CLASS TimerInformationClass,
    _Out_ PVOID TimerInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseHardError(
    _In_ NTSTATUS ErrorStatus,
    _In_ ULONG NumberOfParameters,
    _In_ ULONG UnicodeStringParameterMask,
    _In_ PULONG_PTR Parameters,
    _In_ ULONG ValidResponseOptions,
    _Out_ PULONG Response
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseMutant(
    _In_ HANDLE MutantHandle,
    _In_opt_ PLONG ReleaseCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseSemaphore(
    _In_ HANDLE SemaphoreHandle,
    _In_ LONG ReleaseCount,
    _Out_opt_ PLONG PreviousCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG NumberOfWaitingThreads
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale(
    _In_ BOOLEAN UserProfile,
    _In_ LCID DefaultLocaleId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage(
    LANGID LanguageId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultHardErrorPort(
    _In_ HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighEventPair(
    _In_ HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighWaitLowEventPair(
    _In_ HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowEventPair(
    _In_ HANDLE EventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowWaitHighEventPair(
    _In_ HANDLE EventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemEnvironmentValue(
    _In_ PUNICODE_STRING VariableName,
    _In_ PUNICODE_STRING Value
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_reads_bytes_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwSetTimer(
    _In_ HANDLE TimerHandle,
    _In_ PLARGE_INTEGER DueTime,
    _In_ PTIMER_APC_ROUTINE TimerApcRoutine,
    _In_ PVOID TimerContext,
    _In_ BOOLEAN WakeTimer,
    _In_opt_ LONG Period,
    _Out_opt_ PBOOLEAN PreviousState
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwSetUuidSeed(
    _In_ PUCHAR UuidSeed
);

NTSYSAPI
NTSTATUS
NTAPI
ZwShutdownSystem(
    _In_ SHUTDOWN_ACTION Action
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitHighEventPair(
    _In_ HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitLowEventPair(
    _In_ HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTraceEvent(
    _In_ ULONG TraceHandle,
    _In_ ULONG Flags,
    _In_ ULONG TraceHeaderLength,
    _In_ PEVENT_TRACE_HEADER TraceHeader
);

#ifdef __cplusplus
}
#endif

#endif
