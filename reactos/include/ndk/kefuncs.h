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
#include "ketypes.h"

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
