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

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

VOID 
STDCALL
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
STDCALL
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
STDCALL
KiDeliverApc(
    IN KPROCESSOR_MODE  PreviousMode,
    IN PVOID  Reserved,
    IN PKTRAP_FRAME  TrapFrame
);

VOID
STDCALL
KiDispatchInterrupt(VOID);


BOOLEAN
STDCALL
KeAreApcsDisabled(
    VOID
    );

VOID
STDCALL
KeFlushQueuedDpcs(
    VOID
    );

ULONG
STDCALL
KeGetRecommendedSharedDataAlignment(
    VOID
    );

ULONG
STDCALL
KeQueryRuntimeThread(
    IN PKTHREAD Thread,
    OUT PULONG UserTime
    );    

BOOLEAN
STDCALL
KeSetKernelStackSwapEnable(
    IN BOOLEAN Enable
    );

BOOLEAN
STDCALL
KeDeregisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord
    );

BOOLEAN
STDCALL
KeRegisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PUCHAR Component
    );

VOID 
STDCALL
KeTerminateThread(
    IN KPRIORITY        Increment       
);

BOOLEAN
STDCALL
KeIsAttachedProcess(VOID);

BOOLEAN
STDCALL
KeIsExecutingDpc(
    VOID
);

VOID
STDCALL
KeSetEventBoostPriority(
    IN PKEVENT Event,
    IN PKTHREAD *Thread OPTIONAL
);

PCONFIGURATION_COMPONENT_DATA 
STDCALL
KeFindConfigurationNextEntry(
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG ComponentKey OPTIONAL,
    IN PCONFIGURATION_COMPONENT_DATA *NextLink
);
                             
PCONFIGURATION_COMPONENT_DATA 
STDCALL
KeFindConfigurationEntry(
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG ComponentKey OPTIONAL
);

VOID
STDCALL
KeFlushEntireTb(
    IN BOOLEAN Unknown,
    IN BOOLEAN CurrentCpuOnly
);

VOID
STDCALL
KeRevertToUserAffinityThread(
    VOID
);

VOID
STDCALL
KiCoprocessorError(
    VOID
);

VOID
STDCALL
KiUnexpectedInterrupt(
    VOID
);

VOID
STDCALL
KeSetDmaIoCoherency(
    IN ULONG Coherency
);

VOID
STDCALL
KeSetProfileIrql(
    IN KIRQL ProfileIrql
);

NTSTATUS
STDCALL
KeSetAffinityThread(
    PKTHREAD Thread,
    KAFFINITY Affinity
);
            
VOID
STDCALL
KeSetSystemAffinityThread(
    IN KAFFINITY Affinity
);

NTSTATUS
STDCALL
KeUserModeCallback(
    IN ULONG    FunctionID,
    IN PVOID    InputBuffer,
    IN ULONG    InputLength,
    OUT PVOID    *OutputBuffer,
    OUT PULONG    OutputLength
);

VOID
STDCALL
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement
);

VOID
STDCALL
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
STDCALL 
KeConnectInterrupt(
    PKINTERRUPT InterruptObject
);

BOOLEAN
STDCALL 
KeDisconnectInterrupt(
    PKINTERRUPT InterruptObject
);

PKPROCESS
STDCALL
KeGetCurrentProcess(
    VOID
);

VOID
KeSetGdtSelector(
    ULONG Entry,
    ULONG Value1,
    ULONG Value2
);

LONG
STDCALL
KeReadStateMutant(
    IN PKMUTANT Mutant
);

VOID
STDCALL
KeInitializeMutant(
    IN PKMUTANT Mutant,
    IN BOOLEAN InitialOwner
);
                
LONG
STDCALL
KeReleaseMutant(
    IN PKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandon,
    IN BOOLEAN Wait
);

NTSTATUS
STDCALL
KeRaiseUserException(
    IN NTSTATUS ExceptionCode
    );

VOID 
STDCALL
KeFlushWriteBuffer(VOID);
    
#endif
