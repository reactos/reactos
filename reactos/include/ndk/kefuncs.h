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
    _In_ PKAPC Apc,
    _In_ PKTHREAD Thread,
    _In_ KAPC_ENVIRONMENT TargetEnvironment,
    _In_ PKKERNEL_ROUTINE KernelRoutine,
    _In_opt_ PKRUNDOWN_ROUTINE RundownRoutine,
    _In_ PKNORMAL_ROUTINE NormalRoutine,
    _In_ KPROCESSOR_MODE Mode,
    _In_ PVOID Context
);

BOOLEAN
NTAPI
KeInsertQueueApc(
    _In_ PKAPC Apc,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2,
    _In_ KPRIORITY PriorityBoost
);

VOID
NTAPI
KiDeliverApc(
    _In_ KPROCESSOR_MODE PreviousMode,
#ifdef _M_AMD64
    _In_ PKEXCEPTION_FRAME ExceptionFrame,
#else
    _Reserved_ PKEXCEPTION_FRAME ExceptionFrame,
#endif
    _In_ PKTRAP_FRAME TrapFrame
);

//
// Process/Thread Functions
//
VOID
NTAPI
KeTerminateThread(
    _In_ KPRIORITY Increment
);

BOOLEAN
NTAPI
KeIsAttachedProcess(
    VOID
);

VOID
NTAPI
KeSetEventBoostPriority(
    _In_ PKEVENT Event,
    _In_opt_ PKTHREAD *Thread
);

KAFFINITY
NTAPI
KeSetAffinityThread(
    _Inout_ PKTHREAD Thread,
    _In_ KAFFINITY Affinity
);

PKPROCESS
NTAPI
KeGetCurrentProcess(
    VOID
);

BOOLEAN
NTAPI
KeAddSystemServiceTable(
    _In_ PULONG_PTR Base,
    _In_opt_ PULONG Count,
    _In_ ULONG Limit,
    _In_ PUCHAR Number,
    _In_ ULONG Index
);

//
// Spinlock Functions
//
VOID
FASTCALL
KiAcquireSpinLock(
    _Inout_ PKSPIN_LOCK SpinLock
);

VOID
FASTCALL
KiReleaseSpinLock(
    _Inout_ PKSPIN_LOCK SpinLock
);

KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch(
    _In_ KSPIN_LOCK_QUEUE_NUMBER LockNumber
);

BOOLEAN
FASTCALL
KeTryToAcquireQueuedSpinLockRaiseToSynch(
    _In_ KSPIN_LOCK_QUEUE_NUMBER LockNumber,
    _In_ PKIRQL OldIrql
);

VOID
FASTCALL
KeAcquireInStackQueuedSpinLockRaiseToSynch(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PKLOCK_QUEUE_HANDLE LockHandle
);


//
// Interrupt Functions
//
VOID
NTAPI
KeInitializeInterrupt(
    _Out_ PKINTERRUPT InterruptObject,
    _In_ PKSERVICE_ROUTINE ServiceRoutine,
    _In_ PVOID ServiceContext,
    _In_ PKSPIN_LOCK SpinLock,
    _In_ ULONG Vector,
    _In_ KIRQL Irql,
    _In_ KIRQL SynchronizeIrql,
    _In_ KINTERRUPT_MODE InterruptMode,
    _In_ BOOLEAN ShareVector,
    _In_ CHAR ProcessorNumber,
    _In_ BOOLEAN FloatingSave
);

BOOLEAN
NTAPI
KeConnectInterrupt(
    _Inout_ PKINTERRUPT InterruptObject
);

BOOLEAN
NTAPI
KeDisconnectInterrupt(
    _Inout_ PKINTERRUPT InterruptObject
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

BOOLEAN
NTAPI
KiIpiServiceRoutine(
    _In_ PKTRAP_FRAME TrapFrame,
#ifdef _M_AMD64
    _In_ PKEXCEPTION_FRAME ExceptionFrame
#else
    _Reserved_ PKEXCEPTION_FRAME ExceptionFrame
#endif
);

//
// Generic DPC Routines
//
VOID
NTAPI
KeGenericCallDpc(
    _In_ PKDEFERRED_ROUTINE Routine,
    _In_ PVOID Context
);

VOID
NTAPI
KeSignalCallDpcDone(
    _In_ PVOID SystemArgument1
);

BOOLEAN
NTAPI
KeSignalCallDpcSynchronize(
    _In_ PVOID SystemArgument2
);

//
// ARC Configuration Functions. Only enabled if you have ARC Support
//
#ifdef _ARC_
PCONFIGURATION_COMPONENT_DATA
NTAPI
KeFindConfigurationNextEntry(
    _In_ PCONFIGURATION_COMPONENT_DATA Child,
    _In_ CONFIGURATION_CLASS Class,
    _In_ CONFIGURATION_TYPE Type,
    _In_opt_ PULONG ComponentKey,
    _In_ PCONFIGURATION_COMPONENT_DATA *NextLink
);

PCONFIGURATION_COMPONENT_DATA
NTAPI
KeFindConfigurationEntry(
    _In_ PCONFIGURATION_COMPONENT_DATA Child,
    _In_ CONFIGURATION_CLASS Class,
    _In_ CONFIGURATION_TYPE Type,
    _In_opt_ PULONG ComponentKey
);
#endif

//
// Low-level Hardware/CPU Control Functions
//
VOID
NTAPI
KeFlushEntireTb(
    _In_ BOOLEAN Invalid,
    _In_ BOOLEAN AllProcessors
);

VOID
NTAPI
KeSetDmaIoCoherency(
    _In_ ULONG Coherency
);

VOID
KeSetGdtSelector(
    _In_ ULONG Entry,
    _In_ ULONG Value1,
    _In_ ULONG Value2
);

VOID
NTAPI
KeProfileInterrupt(
    _In_ PKTRAP_FRAME TrapFrame
);

VOID
NTAPI
KeProfileInterruptWithSource(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ KPROFILE_SOURCE Source
);

VOID
NTAPI
KeSetProfileIrql(
    _In_ KIRQL ProfileIrql
);

VOID
NTAPI
KeSetTimeIncrement(
    _In_ ULONG MaxIncrement,
    _In_ ULONG MinIncrement
);

NTSTATUS
NTAPI
Ke386CallBios(
    _In_ ULONG BiosCommand,
    _Inout_ PCONTEXT BiosArguments
);

//
// Misc. Functions
//
NTSTATUS
NTAPI
KeUserModeCallback(
    _In_ ULONG FunctionID,
    _In_reads_opt_(InputLength) PVOID InputBuffer,
    _In_ ULONG InputLength,
    _Outptr_result_buffer_(*OutputLength) PVOID *OutputBuffer,
    _Out_ PULONG OutputLength
);

NTSTATUS
NTAPI
KeRaiseUserException(
    _In_ NTSTATUS ExceptionCode
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtContinue(
    _In_ PCONTEXT Context,
    _In_ BOOLEAN TestAlert
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCallbackReturn(
    _In_ PVOID Result,
    _In_ ULONG ResultLength,
    _In_ NTSTATUS Status
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProfile(
    _Out_ PHANDLE ProfileHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ ULONG Granularity,
    _Out_ PVOID Buffer,
    _In_ ULONG ProfilingSize,
    _In_ KPROFILE_SOURCE Source,
    _In_ KAFFINITY ProcessorMask
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProfileEx(
    _Out_ PHANDLE ProfileHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ ULONG Granularity,
    _Out_ PVOID Buffer,
    _In_ ULONG ProfilingSize,
    _In_ KPROFILE_SOURCE Source,
    _In_ USHORT GroupCount,
    _In_reads_(GroupCount) PGROUP_AFFINITY Affinity
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDelayExecution(
    _In_ BOOLEAN Alertable,
    _In_ LARGE_INTEGER *Interval
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushInstructionCache(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG NumberOfBytesToFlush
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
    _In_ HANDLE ThreadHandle,
    _Out_ PCONTEXT Context
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
    _In_  KPROFILE_SOURCE ProfileSource,
    _Out_ PULONG Interval
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER Counter,
    _Out_opt_ PLARGE_INTEGER Frequency
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemTime(
    _Out_ PLARGE_INTEGER CurrentTime
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryTimerResolution(
    _Out_ PULONG MinimumResolution,
    _Out_ PULONG MaximumResolution,
    _Out_ PULONG ActualResolution
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThread(
    _In_ HANDLE ThreadHandle,
    _In_ PKNORMAL_ROUTINE ApcRoutine,
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT Context,
    _In_ BOOLEAN SearchFrames
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetContextThread(
    _In_ HANDLE ThreadHandle,
    _In_ PCONTEXT Context
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetIntervalProfile(
    _In_ ULONG Interval,
    _In_ KPROFILE_SOURCE ClockSource
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLdtEntries(
    _In_ ULONG Selector1,
    _In_ LDT_ENTRY LdtEntry1,
    _In_ ULONG Selector2,
    _In_ LDT_ENTRY LdtEntry2
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemTime(
    _In_ PLARGE_INTEGER SystemTime,
    _In_opt_ PLARGE_INTEGER NewSystemTime
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimerResolution(
    _In_ ULONG RequestedResolution,
    _In_ BOOLEAN SetOrUnset,
    _Out_ PULONG ActualResolution
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtStartProfile(
    _In_ HANDLE ProfileHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtStopProfile(
    _In_ HANDLE ProfileHandle
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
    _In_ ULONG ControlCode,
    _In_ PVOID ControlData
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtW32Call(
    _In_ ULONG RoutineIndex,
    _In_ PVOID Argument,
    _In_ ULONG ArgumentLength,
    _Out_opt_ PVOID* Result,
    _Out_opt_ PULONG ResultLength
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
    _In_ PCONTEXT Context,
    _In_ BOOLEAN TestAlert
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCallbackReturn(
    _In_ PVOID Result,
    _In_ ULONG ResultLength,
    _In_ NTSTATUS Status
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProfile(
    _Out_ PHANDLE ProfileHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ ULONG Granularity,
    _Out_ PVOID Buffer,
    _In_ ULONG ProfilingSize,
    _In_ KPROFILE_SOURCE Source,
    _In_ KAFFINITY ProcessorMask
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDelayExecution(
    _In_ BOOLEAN Alertable,
    _In_ LARGE_INTEGER *Interval
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG NumberOfBytesToFlush
);

NTSYSAPI
NTSTATUS
NTAPI
ZwGetContextThread(
    _In_ HANDLE ThreadHandle,
    _Out_ PCONTEXT Context
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
    _In_  KPROFILE_SOURCE ProfileSource,
    _Out_ PULONG Interval
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER Counter,
    _Out_opt_ PLARGE_INTEGER Frequency
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemTime(
    _Out_ PLARGE_INTEGER CurrentTime
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryTimerResolution(
    _Out_ PULONG MinimumResolution,
    _Out_ PULONG MaximumResolution,
    _Out_ PULONG ActualResolution
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueueApcThread(
    _In_ HANDLE ThreadHandle,
    _In_ PKNORMAL_ROUTINE ApcRoutine,
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT Context,
    _In_ BOOLEAN SearchFrames
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetContextThread(
    _In_ HANDLE ThreadHandle,
    _In_ PCONTEXT Context
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetIntervalProfile(
    _In_ ULONG Interval,
    _In_ KPROFILE_SOURCE ClockSource
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLdtEntries(
    _In_ ULONG Selector1,
    _In_ LDT_ENTRY LdtEntry1,
    _In_ ULONG Selector2,
    _In_ LDT_ENTRY LdtEntry2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime(
    _In_ PLARGE_INTEGER SystemTime,
    _In_opt_ PLARGE_INTEGER NewSystemTime
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetTimerResolution(
    _In_ ULONG RequestedResolution,
    _In_ BOOLEAN SetOrUnset,
    _Out_ PULONG ActualResolution
);

NTSYSAPI
NTSTATUS
NTAPI
ZwStartProfile(
    _In_ HANDLE ProfileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwStopProfile(
    _In_ HANDLE ProfileHandle
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
    _In_ ULONG ControlCode,
    _In_ PVOID ControlData
);

NTSYSAPI
NTSTATUS
NTAPI
ZwW32Call(
    _In_ ULONG RoutineIndex,
    _In_ PVOID Argument,
    _In_ ULONG ArgumentLength,
    _Out_opt_ PVOID* Result,
    _Out_opt_ PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution(
    VOID
);
#endif
