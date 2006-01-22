/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    kefuncs.h

Abstract:

    Functions definitions for the Kernel services.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

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

VOID
NTAPI
KiDeliverApc(
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID Reserved,
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

NTSTATUS
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

//
// ARC Configuration Functions
//
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

//
// Low-level Hardware/CPU Control Functions
//
VOID
NTAPI
KeFlushEntireTb(
    IN BOOLEAN Unknown,
    IN BOOLEAN CurrentCpuOnly
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
NTSTATUS
NTAPI
NtContinue(
    IN PCONTEXT Context,
    IN BOOLEAN TestAlert
);

NTSTATUS
NTAPI
NtCallbackReturn(
    PVOID Result,
    ULONG ResultLength,
    NTSTATUS Status
);

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

NTSTATUS
NTAPI
NtDelayExecution(
    IN BOOLEAN Alertable,
    IN LARGE_INTEGER *Interval
);

NTSTATUS
NTAPI
NtFlushInstructionCache(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytesToFlush
);

NTSTATUS
NTAPI
NtGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
);

ULONG
NTAPI
NtGetTickCount(
    VOID
);

NTSTATUS
NTAPI
NtQueryIntervalProfile(
    IN  KPROFILE_SOURCE ProfileSource,
    OUT PULONG Interval
);

NTSTATUS
NTAPI
NtQueryPerformanceCounter(
    IN PLARGE_INTEGER Counter,
    IN PLARGE_INTEGER Frequency
);

NTSTATUS
NTAPI
NtQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
);

NTSTATUS
NTAPI
NtQueryTimerResolution(
    OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG ActualResolution
);

NTSTATUS
NTAPI
NtQueueApcThread(
    HANDLE ThreadHandle,
    PKNORMAL_ROUTINE ApcRoutine,
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

NTSTATUS
NTAPI
NtRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames
);

NTSTATUS
NTAPI
NtSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
);

NTSTATUS
NTAPI
NtSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ClockSource
);

NTSTATUS
NTAPI
NtSetLdtEntries(
    ULONG Selector1,
    LDT_ENTRY LdtEntry1,
    ULONG Selector2,
    LDT_ENTRY LdtEntry2
);

NTSTATUS
NTAPI
NtSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    IN PLARGE_INTEGER NewSystemTime OPTIONAL
);

NTSTATUS
NTAPI
NtSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOLEAN SetOrUnset,
    OUT PULONG ActualResolution
);

NTSTATUS
NTAPI
NtStartProfile(
    IN HANDLE ProfileHandle
);

NTSTATUS
NTAPI
NtStopProfile(
    IN HANDLE ProfileHandle
);

NTSTATUS
NTAPI
NtTestAlert(
    VOID
);

NTSTATUS
NTAPI
NtVdmControl(
    ULONG ControlCode,
    PVOID ControlData
);

NTSTATUS
NTAPI
NtW32Call(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID* Result,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtYieldExecution(
    VOID
);

NTSTATUS
NTAPI
ZwContinue(
    IN PCONTEXT Context,
    IN BOOLEAN TestAlert
);

NTSTATUS
NTAPI
ZwCallbackReturn(
    PVOID Result,
    ULONG ResultLength,
    NTSTATUS Status
);

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

NTSTATUS
NTAPI
ZwDelayExecution(
    IN BOOLEAN Alertable,
    IN LARGE_INTEGER *Interval
);

NTSTATUS
NTAPI
ZwFlushInstructionCache(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytesToFlush
);

NTSTATUS
NTAPI
ZwGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
);

ULONG
NTAPI
ZwGetTickCount(
    VOID
);

NTSTATUS
NTAPI
ZwQueryIntervalProfile(
    IN  KPROFILE_SOURCE ProfileSource,
    OUT PULONG Interval
);

NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
    IN PLARGE_INTEGER Counter,
    IN PLARGE_INTEGER Frequency
);

NTSTATUS
NTAPI
ZwQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
);

NTSTATUS
NTAPI
ZwQueryTimerResolution(
    OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG ActualResolution
);

NTSTATUS
NTAPI
ZwQueueApcThread(
    HANDLE ThreadHandle,
    PKNORMAL_ROUTINE ApcRoutine,
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

NTSTATUS
NTAPI
ZwRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames
);

NTSTATUS
NTAPI
ZwSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
);

NTSTATUS
NTAPI
ZwSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ClockSource
);

NTSTATUS
NTAPI
ZwSetLdtEntries(
    ULONG Selector1,
    LDT_ENTRY LdtEntry1,
    ULONG Selector2,
    LDT_ENTRY LdtEntry2
);

NTSTATUS
NTAPI
ZwSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    IN PLARGE_INTEGER NewSystemTime OPTIONAL
);

NTSTATUS
NTAPI
ZwSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOLEAN SetOrUnset,
    OUT PULONG ActualResolution
);

NTSTATUS
NTAPI
ZwStartProfile(
    IN HANDLE ProfileHandle
);

NTSTATUS
NTAPI
ZwStopProfile(
    IN HANDLE ProfileHandle
);

NTSTATUS
NTAPI
ZwTestAlert(
    VOID
);

NTSTATUS
NTAPI
ZwVdmControl(
    ULONG ControlCode,
    PVOID ControlData
);

NTSTATUS
NTAPI
ZwW32Call(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID* Result OPTIONAL,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
NTAPI
ZwYieldExecution(
    VOID
);
#endif
