/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    exfuncs.h

Abstract:

    Function definitions for the Executive.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _EXFUNCS_H
#define _EXFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <pstypes.h>
#include <extypes.h>

//
// Don't include WMI headers just for one define
//
typedef struct _EVENT_TRACE_HEADER *PEVENT_TRACE_HEADER;

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

#endif

//
// Native Calls
//
NTSTATUS
NTAPI
NtAddAtom(
    IN PWSTR AtomName,
    IN ULONG AtomNameLength,
    IN OUT PRTL_ATOM Atom
);

NTSTATUS
NTAPI
NtCancelTimer(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
);

NTSTATUS
NTAPI
NtClearEvent(
    IN HANDLE EventHandle
);

NTSTATUS
NTAPI
NtCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSTATUS
NTAPI
NtCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN InitialOwner
);

NTSTATUS
NTAPI
NtCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

NTSTATUS
NTAPI
NtCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
);

NTSTATUS
NTAPI
NtDeleteAtom(
    IN RTL_ATOM Atom
);

NTSTATUS
NTAPI
NtDisplayString(
    IN PUNICODE_STRING DisplayString
);

NTSTATUS
NTAPI
NtFindAtom(
    IN  PWSTR AtomName,
    IN  ULONG AtomNameLength,
    OUT PRTL_ATOM Atom OPTIONAL
);

NTSTATUS
NTAPI
NtOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAcces,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtPulseEvent(
    IN HANDLE EventHandle,
    IN PLONG PulseCount OPTIONAL
);

NTSTATUS
NTAPI
NtQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
);

NTSTATUS
NTAPI
NtQueryDefaultUILanguage(
    PLANGID LanguageId
);

NTSTATUS
NTAPI
NtQueryEvent(
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQueryInformationAtom(
    IN  RTL_ATOM Atom,
    IN  ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN  ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

NTSTATUS
NTAPI
NtQueryInstallUILanguage(
    PLANGID LanguageId
);

NTSTATUS
NTAPI
NtQueryMutant(
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtQuerySemaphore(
    IN HANDLE SemaphoreHandle,
    IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PWSTR Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQuerySystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

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

NTSTATUS
NTAPI
NtReleaseMutant(
    IN HANDLE MutantHandle,
    IN PLONG ReleaseCount OPTIONAL
);

NTSTATUS
NTAPI
NtReleaseSemaphore(
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount
);

NTSTATUS
NTAPI
NtResetEvent(
    IN HANDLE EventHandle,
    OUT PLONG NumberOfWaitingThreads OPTIONAL
);

NTSTATUS
NTAPI
NtSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
);

NTSTATUS
NTAPI
NtSetDefaultUILanguage(
    LANGID LanguageId
);

NTSTATUS
NTAPI
NtSetDefaultHardErrorPort(
    IN HANDLE PortHandle
);

NTSTATUS
NTAPI
NtSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
);

NTSTATUS
NTAPI
NtSetHighEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
NtSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
NtSetLowEventPair(
    HANDLE EventPair
);

NTSTATUS
NTAPI
NtSetLowWaitHighEventPair(
    HANDLE EventPair
);

NTSTATUS
NTAPI
NtSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING Value
);

NTSTATUS
NTAPI
NtSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
);

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

NTSTATUS
NTAPI
NtSetUuidSeed(
    IN PUCHAR UuidSeed
);

NTSTATUS
NTAPI
NtShutdownSystem(
    IN SHUTDOWN_ACTION Action
);

NTSTATUS
NTAPI
NtWaitHighEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
NtWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
NtTraceEvent(
    IN ULONG TraceHandle,
    IN ULONG Flags,
    IN ULONG TraceHeaderLength,
    IN PEVENT_TRACE_HEADER TraceHeader
);

NTSTATUS
NTAPI
ZwAddAtom(
    IN PWSTR AtomName,
    IN ULONG AtomNameLength,
    IN OUT PRTL_ATOM Atom
);

#ifdef NTOS_MODE_USER
NTSTATUS
NTAPI
ZwCancelTimer(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
);
#endif

NTSTATUS
NTAPI
ZwClearEvent(
    IN HANDLE EventHandle
);

NTSTATUS
NTAPI
ZwCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSTATUS
NTAPI
ZwCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
ZwCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN InitialOwner
);

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
NTSTATUS
NTAPI
ZwCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
);
#endif

NTSTATUS
NTAPI
ZwDeleteAtom(
    IN RTL_ATOM Atom
);

NTSTATUS
NTAPI
ZwDisplayString(
    IN PUNICODE_STRING DisplayString
);

NTSTATUS
NTAPI
ZwFindAtom(
    IN  PWSTR AtomName,
    IN  ULONG AtomNameLength,
    OUT PRTL_ATOM Atom OPTIONAL
);

NTSTATUS
NTAPI
ZwOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
ZwOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
ZwOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
ZwOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAcces,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

#ifdef NTOS_MODE_USER
NTSTATUS
NTAPI
ZwOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);
#endif

NTSTATUS
NTAPI
ZwPulseEvent(
    IN HANDLE EventHandle,
    IN PLONG PulseCount OPTIONAL
);

NTSTATUS
NTAPI
ZwQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
);

NTSTATUS
NTAPI
ZwQueryDefaultUILanguage(
    PLANGID LanguageId
);

NTSTATUS
NTAPI
ZwQueryEvent(
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
ZwQueryInformationAtom(
    IN  RTL_ATOM Atom,
    IN  ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN  ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

NTSTATUS
NTAPI
ZwQueryInstallUILanguage(
    PLANGID LanguageId
);

NTSTATUS
NTAPI
ZwQueryMutant(
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
ZwQuerySemaphore(
    IN HANDLE SemaphoreHandle,
    IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
ZwQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PWSTR Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSTATUS
NTAPI
ZwQuerySystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
ZwQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

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

NTSTATUS
NTAPI
ZwReleaseMutant(
    IN HANDLE MutantHandle,
    IN PLONG ReleaseCount OPTIONAL
);

NTSTATUS
NTAPI
ZwReleaseSemaphore(
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount
);

NTSTATUS
NTAPI
ZwResetEvent(
    IN HANDLE EventHandle,
    OUT PLONG NumberOfWaitingThreads OPTIONAL
);

NTSTATUS
NTAPI
ZwSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
);

NTSTATUS
NTAPI
ZwSetDefaultUILanguage(
    LANGID LanguageId
);

NTSTATUS
NTAPI
ZwSetDefaultHardErrorPort(
    IN HANDLE PortHandle
);

NTSTATUS
NTAPI
ZwSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
);

NTSTATUS
NTAPI
ZwSetHighEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
ZwSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
ZwSetLowEventPair(
    HANDLE EventPair
);

NTSTATUS
NTAPI
ZwSetLowWaitHighEventPair(
    HANDLE EventPair
);

NTSTATUS
NTAPI
ZwSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING Value
);

NTSTATUS
NTAPI
ZwSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
);

#ifdef NTOS_MODE_USER
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

NTSTATUS
NTAPI
ZwSetUuidSeed(
    IN PUCHAR UuidSeed
);

NTSTATUS
NTAPI
ZwShutdownSystem(
    IN SHUTDOWN_ACTION Action
);

NTSTATUS
NTAPI
ZwWaitHighEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
ZwWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
ZwTraceEvent(
    IN ULONG TraceHandle,
    IN ULONG Flags,
    IN ULONG TraceHeaderLength,
    IN PEVENT_TRACE_HEADER TraceHeader
);
#endif
