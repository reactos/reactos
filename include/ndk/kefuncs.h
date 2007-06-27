/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    kefuncs.h

Abstract:

    Functions definitions for the Kernel services.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _KEFUNCS_H
#define _KEFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <ketypes.h>

#ifndef NTOS_MODE_USER

//
// APC Functions
//
VOID
NTAPI
KeInitializeApc(
    IN PKAPC Apc,
    IN PKTHREAD Thread,
    IN KAPC_ENVIRONMENT TargetEnvironment,
    IN PKKERNEL_ROUTINE KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN KPROCESSOR_MODE Mode,
    IN PVOID Context
);

BOOLEAN
NTAPI
KeInsertQueueApc(
    IN PKAPC Apc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN KPRIORITY PriorityBoost
);

VOID
NTAPI
KiDeliverApc(
    IN KPROCESSOR_MODE PreviousMode,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
);

//
// Process/Thread Functions
//
VOID
NTAPI
KeTerminateThread(
    IN KPRIORITY Increment
);

BOOLEAN
NTAPI
KeIsAttachedProcess(
    VOID
);

VOID
NTAPI
KeSetEventBoostPriority(
    IN PKEVENT Event,
    IN PKTHREAD *Thread OPTIONAL
);

KAFFINITY
NTAPI
KeSetAffinityThread(
    PKTHREAD Thread,
    KAFFINITY Affinity
);

PKPROCESS
NTAPI
KeGetCurrentProcess(
    VOID
);

BOOLEAN
NTAPI
KeAddSystemServiceTable(
    PULONG_PTR Base,
    PULONG Count OPTIONAL,
    ULONG Limit,
    PUCHAR Number,
    ULONG Index
);

//
// Spinlock Functions
//
VOID
FASTCALL
KiAcquireSpinLock(
    PKSPIN_LOCK SpinLock
);

VOID
FASTCALL
KiReleaseSpinLock(
    PKSPIN_LOCK SpinLock
);

KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch(
    IN KSPIN_LOCK_QUEUE_NUMBER LockNumber
);

VOID
FASTCALL
KeAcquireInStackQueuedSpinLockRaiseToSynch(
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
);


//
// Interrupt Functions
//
VOID
NTAPI
KeInitializeInterrupt(
    PKINTERRUPT InterruptObject,
    PKSERVICE_ROUTINE ServiceRoutine,
    PVOID ServiceContext,
    PKSPIN_LOCK SpinLock,
    ULONG Vector,
    KIRQL Irql,
    KIRQL SynchronizeIrql,
    KINTERRUPT_MODE InterruptMode,
    BOOLEAN ShareVector,
    CHAR ProcessorNumber,
    BOOLEAN FloatingSave
);

BOOLEAN
NTAPI
KeConnectInterrupt(
    PKINTERRUPT InterruptObject
);

BOOLEAN
NTAPI
KeDisconnectInterrupt(
    PKINTERRUPT InterruptObject
);

VOID
NTAPI
KiDispatchInterrupt(
    VOID
);

VOID
NTAPI
KiCoprocessorError(
    VOID
);

VOID
NTAPI
KiUnexpectedInterrupt(
    VOID
);

VOID
NTAPI
KeEnterKernelDebugger(
    VOID
);

BOOLEAN
NTAPI
KeIsExecutingDpc(
    VOID
);

VOID
NTAPI
KeFlushQueuedDpcs(
    VOID
);

BOOLEAN
NTAPI
KiIpiServiceRoutine(
    IN PKTRAP_FRAME TrapFrame,
    IN PVOID ExceptionFrame
);

//
// ARC Configuration Functions. Only enabled if you have ARC Support
//
#ifdef _ARC_
PCONFIGURATION_COMPONENT_DATA
NTAPI
KeFindConfigurationNextEntry(
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG ComponentKey OPTIONAL,
    IN PCONFIGURATION_COMPONENT_DATA *NextLink
);

PCONFIGURATION_COMPONENT_DATA
NTAPI
KeFindConfigurationEntry(
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG ComponentKey OPTIONAL
);
#endif

//
// Low-level Hardware/CPU Control Functions
//
VOID
NTAPI
KeFlushEntireTb(
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
);

VOID
NTAPI
KeUpdateSystemTime(
    PKTRAP_FRAME TrapFrame,
    KIRQL Irql,
    ULONG Increment
);

VOID
NTAPI
KeUpdateRunTime(
    PKTRAP_FRAME TrapFrame,
    KIRQL Irql
);

VOID
NTAPI
KeSetDmaIoCoherency(
    IN ULONG Coherency
);

VOID
KeSetGdtSelector(
    ULONG Entry,
    ULONG Value1,
    ULONG Value2
);

VOID
NTAPI
KeSetProfileIrql(
    IN KIRQL ProfileIrql
);

VOID
NTAPI
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement
);

NTSTATUS
NTAPI
Ke386CallBios(
    IN ULONG BiosCommand,
    IN OUT PCONTEXT BiosArguments
);

//
// Misc. Functions
//
NTSTATUS
NTAPI
KeUserModeCallback(
    IN ULONG FunctionID,
    IN PVOID InputBuffer,
    IN ULONG InputLength,
    OUT PVOID *OutputBuffer,
    OUT PULONG OutputLength
);

NTSTATUS
NTAPI
KeRaiseUserException(
    IN NTSTATUS ExceptionCode
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtContinue(
    IN PCONTEXT Context,
    IN BOOLEAN TestAlert
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCallbackReturn(
    PVOID Result,
    ULONG ResultLength,
    NTSTATUS Status
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProfile(
    OUT PHANDLE ProfileHandle,
    IN HANDLE ProcessHandle,
    IN PVOID ImageBase,
    IN ULONG ImageSize,
    IN ULONG Granularity,
    OUT PVOID Buffer,
    IN ULONG ProfilingSize,
    IN KPROFILE_SOURCE Source,
    IN KAFFINITY ProcessorMask
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDelayExecution(
    IN BOOLEAN Alertable,
    IN LARGE_INTEGER *Interval
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushInstructionCache(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytesToFlush
);

ULONG
NTAPI
NtGetCurrentProcessorNumber(
    VOID
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
);

NTSYSCALLAPI
ULONG
NTAPI
NtGetTickCount(
    VOID
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryIntervalProfile(
    IN  KPROFILE_SOURCE ProfileSource,
    OUT PULONG Interval
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPerformanceCounter(
    IN PLARGE_INTEGER Counter,
    IN PLARGE_INTEGER Frequency
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryTimerResolution(
    OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG ActualResolution
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThread(
    HANDLE ThreadHandle,
    PKNORMAL_ROUTINE ApcRoutine,
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ClockSource
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLdtEntries(
    ULONG Selector1,
    LDT_ENTRY LdtEntry1,
    ULONG Selector2,
    LDT_ENTRY LdtEntry2
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    IN PLARGE_INTEGER NewSystemTime OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOLEAN SetOrUnset,
    OUT PULONG ActualResolution
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtStartProfile(
    IN HANDLE ProfileHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtStopProfile(
    IN HANDLE ProfileHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTestAlert(
    VOID
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtVdmControl(
    ULONG ControlCode,
    PVOID ControlData
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtW32Call(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID* Result OPTIONAL,
    OUT PULONG ResultLength OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtYieldExecution(
    VOID
);

NTSYSAPI
NTSTATUS
NTAPI
ZwContinue(
    IN PCONTEXT Context,
    IN BOOLEAN TestAlert
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCallbackReturn(
    PVOID Result,
    ULONG ResultLength,
    NTSTATUS Status
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProfile(
    OUT PHANDLE ProfileHandle,
    IN HANDLE ProcessHandle,
    IN PVOID ImageBase,
    IN ULONG ImageSize,
    IN ULONG Granularity,
    OUT PVOID Buffer,
    IN ULONG ProfilingSize,
    IN KPROFILE_SOURCE Source,
    IN KAFFINITY ProcessorMask
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDelayExecution(
    IN BOOLEAN Alertable,
    IN LARGE_INTEGER *Interval
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytesToFlush
);

NTSYSAPI
NTSTATUS
NTAPI
ZwGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
);

NTSYSAPI
ULONG
NTAPI
ZwGetTickCount(
    VOID
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryIntervalProfile(
    IN  KPROFILE_SOURCE ProfileSource,
    OUT PULONG Interval
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
    IN PLARGE_INTEGER Counter,
    IN PLARGE_INTEGER Frequency
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryTimerResolution(
    OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG ActualResolution
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueueApcThread(
    HANDLE ThreadHandle,
    PKNORMAL_ROUTINE ApcRoutine,
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ClockSource
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLdtEntries(
    ULONG Selector1,
    LDT_ENTRY LdtEntry1,
    ULONG Selector2,
    LDT_ENTRY LdtEntry2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    IN PLARGE_INTEGER NewSystemTime OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOLEAN SetOrUnset,
    OUT PULONG ActualResolution
);

NTSYSAPI
NTSTATUS
NTAPI
ZwStartProfile(
    IN HANDLE ProfileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwStopProfile(
    IN HANDLE ProfileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTestAlert(
    VOID
);

NTSYSAPI
NTSTATUS
NTAPI
ZwVdmControl(
    ULONG ControlCode,
    PVOID ControlData
);

NTSYSAPI
NTSTATUS
NTAPI
ZwW32Call(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID* Result OPTIONAL,
    OUT PULONG ResultLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution(
    VOID
);
#endif
