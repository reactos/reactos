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
    PFAST_MUTEX FastMutex
);

VOID
FASTCALL
ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(
    PFAST_MUTEX FastMutex
);

//
// Pushlock functions
//
VOID
FASTCALL
ExfAcquirePushLockExclusive(
    PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfAcquirePushLockShared(
    PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfReleasePushLock(
    PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfReleasePushLockExclusive(
    PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfReleasePushLockShared(
    PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfTryToWakePushLock(
    PEX_PUSH_LOCK PushLock
);

VOID
FASTCALL
ExfUnblockPushLock(
    PEX_PUSH_LOCK PushLock,
    PVOID CurrentWaitBlock
);

//
// Handle Table Functions
//
NTKERNELAPI
BOOLEAN
NTAPI
ExEnumHandleTable(
    IN PHANDLE_TABLE HandleTable,
    IN PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
    IN OUT PVOID Context,
    OUT PHANDLE Handle OPTIONAL
);

//
// Resource Functions
//
PVOID
NTAPI
ExEnterCriticalRegionAndAcquireResourceExclusive(
    IN PERESOURCE Resource
);

PVOID
NTAPI
ExEnterCriticalRegionAndAcquireResourceShared(
    IN PERESOURCE Resource
);

PVOID
NTAPI
ExEnterCriticalRegionAndAcquireSharedWaitForExclusive(
    IN PERESOURCE Resource
);

VOID
FASTCALL
ExReleaseResourceAndLeaveCriticalRegion(
    IN PERESOURCE Resource
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAddAtom(
    IN PWSTR AtomName,
    IN ULONG AtomNameLength,
    IN OUT PRTL_ATOM Atom
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelTimer(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtClearEvent(
    IN HANDLE EventHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKeyedEvent(
    OUT PHANDLE KeyedEventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Flags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN InitialOwner
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteAtom(
    IN RTL_ATOM Atom
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDisplayString(
    IN PUNICODE_STRING DisplayString
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateSystemEnvironmentValuesEx(
    IN ULONG InformationClass,
    IN PVOID Buffer,
    IN ULONG BufferLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFindAtom(
    IN  PWSTR AtomName,
    IN  ULONG AtomNameLength,
    OUT PRTL_ATOM Atom OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyedEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAcces,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPulseEvent(
    IN HANDLE EventHandle,
    IN PLONG PulseCount OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
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
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationAtom(
    IN  RTL_ATOM Atom,
    IN  ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN  ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
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
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySemaphore(
    IN HANDLE SemaphoreHandle,
    IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PWSTR Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValueEx(
    IN PUNICODE_STRING VariableName,
    IN LPGUID VendorGuid,
    IN PVOID Value,
    IN OUT PULONG ReturnLength,
    IN OUT PULONG Attributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseMutant(
    IN HANDLE MutantHandle,
    IN PLONG ReleaseCount OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseKeyedEvent(
    IN HANDLE EventHandle,
    IN PVOID Key,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseSemaphore(
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResetEvent(
    IN HANDLE EventHandle,
    OUT PLONG NumberOfWaitingThreads OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
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
    IN HANDLE PortHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEventBoostPriority(
    IN HANDLE EventHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetHighEventPair(
    IN HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLowEventPair(
    HANDLE EventPair
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLowWaitHighEventPair(
    HANDLE EventPair
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING Value
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValueEx(
    IN PUNICODE_STRING VariableName,
    IN LPGUID VendorGuid
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimer(
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine,
    IN PVOID TimerContext,
    IN BOOLEAN WakeTimer,
    IN LONG Period OPTIONAL,
    OUT PBOOLEAN PreviousState OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetUuidSeed(
    IN PUCHAR UuidSeed
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtShutdownSystem(
    IN SHUTDOWN_ACTION Action
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForKeyedEvent(
    IN HANDLE EventHandle,
    IN PVOID Key,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitHighEventPair(
    IN HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceEvent(
    IN ULONG TraceHandle,
    IN ULONG Flags,
    IN ULONG TraceHeaderLength,
    IN PEVENT_TRACE_HEADER TraceHeader
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAddAtom(
    IN PWSTR AtomName,
    IN ULONG AtomNameLength,
    IN OUT PRTL_ATOM Atom
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwCancelTimer(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent(
    IN HANDLE EventHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN InitialOwner
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteAtom(
    IN RTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
    IN PUNICODE_STRING DisplayString
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFindAtom(
    IN  PWSTR AtomName,
    IN  ULONG AtomNameLength,
    OUT PRTL_ATOM Atom OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAcces,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent(
    IN HANDLE EventHandle,
    IN PLONG PulseCount OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
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
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationAtom(
    IN  RTL_ATOM Atom,
    IN  ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN  ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
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
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySemaphore(
    IN HANDLE SemaphoreHandle,
    IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PWSTR Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN SIZE_T Length,
    OUT PSIZE_T ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseMutant(
    IN HANDLE MutantHandle,
    IN PLONG ReleaseCount OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseSemaphore(
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent(
    IN HANDLE EventHandle,
    OUT PLONG NumberOfWaitingThreads OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
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
    IN HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighEventPair(
    IN HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowEventPair(
    HANDLE EventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowWaitHighEventPair(
    HANDLE EventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING Value
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN SIZE_T SystemInformationLength
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwSetTimer(
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine,
    IN PVOID TimerContext,
    IN BOOLEAN WakeTimer,
    IN LONG Period OPTIONAL,
    OUT PBOOLEAN PreviousState OPTIONAL
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwSetUuidSeed(
    IN PUCHAR UuidSeed
);

NTSYSAPI
NTSTATUS
NTAPI
ZwShutdownSystem(
    IN SHUTDOWN_ACTION Action
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitHighEventPair(
    IN HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTraceEvent(
    IN ULONG TraceHandle,
    IN ULONG Flags,
    IN ULONG TraceHeaderLength,
    IN PEVENT_TRACE_HEADER TraceHeader
);

#ifdef __cplusplus
}
#endif

#endif
