/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/kefuncs.h
 * PURPOSE:         Prototypes for Kernel Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KEFUNCS_H
#define _KEFUNCS_H

/* DEPENDENCIES **************************************************************/
#include "ketypes.h"

/* PROTOTYPES ****************************************************************/

BOOLEAN
NTAPI
KeAddSystemServiceTable(
    PULONG_PTR Base,
    PULONG Count OPTIONAL,
    ULONG Limit,
    PUCHAR Number,
    ULONG Index
);

VOID
NTAPI
KeInitializeApc(
    IN PKAPC  Apc,
    IN PKTHREAD  Thread,
    IN KAPC_ENVIRONMENT  TargetEnvironment,
    IN PKKERNEL_ROUTINE  KernelRoutine,
    IN PKRUNDOWN_ROUTINE  RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE  NormalRoutine,
    IN KPROCESSOR_MODE  Mode,
    IN PVOID  Context
);

VOID
NTAPI
KeEnterKernelDebugger(VOID);

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

VOID
NTAPI
KiDeliverApc(
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID Reserved,
    IN PKTRAP_FRAME TrapFrame
);

VOID
NTAPI
KiDispatchInterrupt(VOID);

VOID
NTAPI
KeTerminateThread(
    IN KPRIORITY        Increment
);

BOOLEAN
NTAPI
KeIsAttachedProcess(VOID);

BOOLEAN
NTAPI
KeIsExecutingDpc(
    VOID
);

VOID
NTAPI
KeSetEventBoostPriority(
    IN PKEVENT Event,
    IN PKTHREAD *Thread OPTIONAL
);

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

VOID
NTAPI
KeFlushEntireTb(
    IN BOOLEAN Unknown,
    IN BOOLEAN CurrentCpuOnly
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
KeSetDmaIoCoherency(
    IN ULONG Coherency
);

VOID
NTAPI
KeSetProfileIrql(
    IN KIRQL ProfileIrql
);

NTSTATUS
NTAPI
KeSetAffinityThread(
    PKTHREAD Thread,
    KAFFINITY Affinity
);

NTSTATUS
NTAPI
KeUserModeCallback(
    IN ULONG FunctionID,
    IN PVOID InputBuffer,
    IN ULONG InputLength,
    OUT PVOID *OutputBuffer,
    OUT PULONG OutputLength
);

VOID
NTAPI
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement
);

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

PKPROCESS
NTAPI
KeGetCurrentProcess(
    VOID
);

VOID
KeSetGdtSelector(
    ULONG Entry,
    ULONG Value1,
    ULONG Value2
);

NTSTATUS
NTAPI
KeRaiseUserException(
    IN NTSTATUS ExceptionCode
    );

#endif
