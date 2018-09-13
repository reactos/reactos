/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ki.h

Abstract:

    This module contains the private (internal) header file for the
    kernel.

Author:

    David N. Cutler (davec) 28-Feb-1989

Revision History:

--*/

#ifndef _KI_
#define _KI_
#include "ntos.h"
#include "stdio.h"
#include "stdlib.h"
#include "zwapi.h"

//
// Private (internal) constant definitions.
//
// Priority increment value definitions
//

#define ALERT_INCREMENT 2           // Alerted unwait priority increment
#define BALANCE_INCREMENT 10        // Balance set priority increment
#define RESUME_INCREMENT 0          // Resume thread priority increment
#define TIMER_EXPIRE_INCREMENT 0    // Timer expiration priority increment

//
// Define time critical priority class base.
//

#define TIME_CRITICAL_PRIORITY_BOUND 14

//
// Define NIL pointer value.
//

#define NIL (PVOID)NULL             // Null pointer to void

//
// Define macros which are used in the kernel only
//
// Clear member in set
//

#define ClearMember(Member, Set) \
    Set = Set & (~(1 << (Member)))

//
// Set member in set
//

#define SetMember(Member, Set) \
    Set = Set | (1 << (Member))

#define FindFirstSetLeftMember(Set, Member) {                          \
    ULONG _Bit;                                                        \
    ULONG _Mask;                                                       \
    ULONG _Offset = 16;                                                \
    if ((_Mask = Set >> 16) == 0) {                                    \
        _Offset = 0;                                                   \
        _Mask = Set;                                                   \
    }                                                                  \
    if (_Mask >> 8) {                                                  \
        _Offset += 8;                                                  \
    }                                                                  \
    if ((_Bit = Set >> _Offset) & 0xf0) {                              \
        _Bit >>= 4;                                                    \
        _Offset += 4;                                                  \
    }                                                                  \
    *(Member) = KiFindLeftNibbleBitTable[_Bit] + _Offset;              \
}

//
// Lock and unlock APC queue lock.
//

#if defined(NT_UP)
#define KiLockApcQueue(Thread, OldIrql) \
    *(OldIrql) = KeRaiseIrqlToSynchLevel()
#else
#define KiLockApcQueue(Thread, OldIrql) \
    *(OldIrql) = KeAcquireSpinLockRaiseToSynch(&(Thread)->ApcQueueLock)
#endif

#if defined(NT_UP)
#define KiUnlockApcQueue(Thread, OldIrql) KeLowerIrql((OldIrql))
#else
#define KiUnlockApcQueue(Thread, OldIrql) KeReleaseSpinLock(&(Thread)->ApcQueueLock, (OldIrql))
#endif

//
// Lock and unlock context swap lock.
//

#if defined(_ALPHA_) || defined(_X86_)

#define KiLockContextSwap(OldIrql) \
    *(OldIrql) = KeAcquireQueuedSpinLockRaiseToSynch(LockQueueContextSwapLock)

#define KiUnlockContextSwap(OldIrql) \
    KeReleaseQueuedSpinLock(LockQueueContextSwapLock, OldIrql)

#else

#if defined(NT_UP)

#define KiLockContextSwap(OldIrql) \
    *(OldIrql) = KeRaiseIrqlToSynchLevel()

#define KiUnlockContextSwap(OldIrql) \
    KeLowerIrql((OldIrql))

#else

#define KiLockContextSwap(OldIrql) \
    *(OldIrql) = KeAcquireSpinLockRaiseToSynch(&KiContextSwapLock)

#define KiUnlockContextSwap(OldIrql) \
    KeReleaseSpinLock(&KiContextSwapLock, (OldIrql))

#endif
#endif

VOID
FASTCALL
KiUnlockDispatcherDatabase (
    IN KIRQL OldIrql
    );

LOGICAL
FASTCALL
KiTryToAcquireQueuedSpinLock (
    IN PKSPIN_LOCK_QUEUE QueuedLock
    );

#define KiQueuedSpinLockContext(n)  (&(KeGetCurrentPrcb()->LockQueue[n]))


// VOID
// KiBoostPriorityThread (
//    IN PKTHREAD Thread,
//    IN KPRIORITY Increment
//    )
//
//*++
//
// Routine Description:
//
//    This function boosts the priority of the specified thread using
//    the same algorithm used when a thread gets a boost from a wait
//    operation.
//
// Arguments:
//
//    Thread  - Supplies a pointer to a dispatcher object of type thread.
//
//    Increment - Supplies the priority increment that is to be applied to
//        the thread's priority.
//
// Return Value:
//
//    None.
//
//--*

#define KiBoostPriorityThread(Thread, Increment) {              \
    KPRIORITY NewPriority;                                      \
    PKPROCESS Process;                                          \
                                                                \
    if ((Thread)->Priority < LOW_REALTIME_PRIORITY) {           \
        if ((Thread)->PriorityDecrement == 0) {                 \
            NewPriority = (Thread)->BasePriority + (Increment); \
            if (NewPriority > (Thread)->Priority) {             \
                if (NewPriority >= LOW_REALTIME_PRIORITY) {     \
                    NewPriority = LOW_REALTIME_PRIORITY - 1;    \
                }                                               \
                                                                \
                Process = (Thread)->ApcState.Process;           \
                (Thread)->Quantum = Process->ThreadQuantum;     \
                KiSetPriorityThread((Thread), NewPriority);     \
            }                                                   \
        }                                                       \
    }                                                           \
}

// VOID
// KiInsertWaitList (
//    IN KPROCESSOR_MODE WaitMode,
//    IN PKTHREAD Thread
//    )
//
//*++
//
// Routine Description:
//
//    This function inserts the specified thread in the appropriate
//    wait list.
//
// Arguments:
//
//    WaitMode - Supplies the processor mode of the wait operation.
//
//    Thread - Supplies a pointer to a dispatcher object of type
//        thread.
//
// Return Value:
//
//    None.
//
//--*

#define KiInsertWaitList(_WaitMode, _Thread) {                  \
    PLIST_ENTRY _ListHead;                                      \
    _ListHead = &KiWaitInListHead;                              \
    if (((_WaitMode) == KernelMode) ||                          \
        ((_Thread)->EnableStackSwap == FALSE) ||                \
        ((_Thread)->Priority >= (LOW_REALTIME_PRIORITY + 9))) { \
        _ListHead = &KiWaitOutListHead;                         \
    }                                                           \
    InsertTailList(_ListHead, &(_Thread)->WaitListEntry);       \
}

//
// Private (internal) structure definitions.
//
// APC Parameter structure.
//

typedef struct _KAPC_RECORD {
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
} KAPC_RECORD, *PKAPC_RECORD;

//
// Executive initialization.
//

VOID
ExpInitializeExecutive (
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

//
// Kernel executive object function definitions.
//

BOOLEAN
KiChannelInitialization (
    VOID
    );

VOID
KiRundownChannel (
    VOID
    );

//
// Interprocessor interrupt function definitions.
//
// Define immediate interprocessor commands.
//

#define IPI_APC 1                       // APC interrupt request
#define IPI_DPC 2                       // DPC interrupt request
#define IPI_FREEZE 4                    // freeze execution request
#define IPI_PACKET_READY 8              // packet ready request
#define IPI_SYNCH_REQUEST 0x10          // synchronous Reverse Stall packet

//
// Define interprocess interrupt types.
//

typedef ULONG KIPI_REQUEST;

typedef
ULONG_PTR
(*PKIPI_BROADCAST_WORKER)(
    IN ULONG_PTR Argument
    );

#if NT_INST

#define IPI_INSTRUMENT_COUNT(a,b) KiIpiCounts[a].b++;

#else

#define IPI_INSTRUMENT_COUNT(a,b)

#endif

//
// Define interprocessor interrupt function prototypes.
//

ULONG_PTR
KiIpiGenericCall (
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG_PTR Context
    );

#if defined(_ALPHA_) || defined(_IA64_)

ULONG
KiIpiProcessRequests (
    VOID
    );

#endif

VOID
FASTCALL
KiIpiSend (
    IN KAFFINITY TargetProcessors,
    IN KIPI_REQUEST Request
    );

VOID
KiIpiSendPacket (
    IN KAFFINITY TargetProcessors,
    IN PKIPI_WORKER WorkerFunction,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

// begin_nthal

BOOLEAN
KiIpiServiceRoutine (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame
    );

// end_nthal

VOID
FASTCALL
KiIpiSignalPacketDone (
    IN PKIPI_CONTEXT SignalDone
    );

VOID
KiIpiStallOnPacketTargets (
    KAFFINITY TargetSet
    );

#if defined(_X86_)

//
// VOID
// KiIpiSendSynchronousPacket (
//   IN PKPRCB Prcb,
//   IN KAFFINITY TargetProcessors,
//   IN PKIPI_WORKER WorkerFunction,
//   IN PVOID Parameter1,
//   IN PVOID Parameter2,
//   IN PVOID Parameter3
//   )
//
// Routine Description:
//
//   Similar to KiIpiSendPacket except that the pointer to the
//   originating PRCB (SignalDone) is kept in the global variable
//   KiSynchPacket and is protected by the context swap lock.  The
//   actual IPI is sent via KiIpiSend with a request type of
//   IPI_SYNCH_REQUEST.  This mechanism is used to send IPI's that
//   (reverse) stall until released by the originator.   This avoids
//   a deadlock that can occur if two processors are trying to deliver
//   IPI packets at the same time and one of them is a reverse stall.
//

#define KiIpiSendSynchronousPacket(Prcb,Target,Function,P1,P2,P3)       \
    {                                                                   \
        extern PKPRCB KiSynchPacket;                                    \
                                                                        \
        Prcb->CurrentPacket[0] = (PVOID)(P1);                           \
        Prcb->CurrentPacket[1] = (PVOID)(P2);                           \
        Prcb->CurrentPacket[2] = (PVOID)(P3);                           \
        Prcb->TargetSet = (Target);                                     \
        Prcb->WorkerRoutine = (Function);                               \
        KiSynchPacket = (Prcb);                                         \
        KiIpiSend((Target),IPI_SYNCH_REQUEST);                          \
    }

#endif

//
// Private (internal) function definitions.
//

VOID
FASTCALL
KiActivateWaiterQueue (
    IN PRKQUEUE Queue
    );

BOOLEAN
KiAdjustInterruptTime (
    IN LONGLONG TimeDelta
    );

VOID
KiApcInterrupt (
    VOID
    );

NTSTATUS
KiCallUserMode (
    IN PVOID *OutputBuffer,
    IN PULONG OutputLength
    );

typedef struct {
    ULONGLONG               Adjustment;
    LARGE_INTEGER           NewCount;
    volatile LONG           KiNumber;
    volatile LONG           HalNumber;
    volatile LONG           Barrier;
} ADJUST_INTERRUPT_TIME_CONTEXT, *PADJUST_INTERRUPT_TIME_CONTEXT;

VOID
KiCalibrateTimeAdjustment (
    PADJUST_INTERRUPT_TIME_CONTEXT Adjust
    );

VOID
KiChainedDispatch (
    VOID
    );

#if DBG

VOID
KiCheckTimerTable (
    IN ULARGE_INTEGER SystemTime
    );

#endif

LARGE_INTEGER
KiComputeReciprocal (
    IN LONG Divisor,
    OUT PCCHAR Shift
    );

ULONG
KiComputeTimerTableIndex (
    IN LARGE_INTEGER Interval,
    IN LARGE_INTEGER CurrentCount,
    IN PRKTIMER Timer
    );

PLARGE_INTEGER
FASTCALL
KiComputeWaitInterval (
    IN PLARGE_INTEGER OriginalTime,
    IN PLARGE_INTEGER DueTime,
    IN OUT PLARGE_INTEGER NewTime
    );

NTSTATUS
KiContinue (
    IN PCONTEXT ContextRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

VOID
KiDeliverApc (
    IN KPROCESSOR_MODE PreviousMode,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

BOOLEAN
KiDisableInterrupts (
    VOID
    );

VOID
KiRestoreInterrupts (
    IN BOOLEAN Enable
    );

VOID
KiDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    );

KCONTINUE_STATUS
KiSetDebugProcessor (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN KPROCESSOR_MODE PreviousMode
    );

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    );

VOID
KiDispatchInterrupt (
    VOID
    );

PKTHREAD
FASTCALL
KiFindReadyThread (
    IN ULONG Processor,
    KPRIORITY LowPriority
    );

VOID
KiFloatingDispatch (
    VOID
    );

#if !defined(_IA64_)

VOID
FASTCALL
KiFlushSingleTb (
    IN BOOLEAN Invalid,
    IN PVOID Virtual
    );

#endif // !_IA64_

VOID
KiFlushMultipleTb (
    IN BOOLEAN Invalid,
    IN PVOID *Virtual,
    IN ULONG Count
    );

#if defined(_ALPHA_)

VOID
KiFlushMultipleTb64 (
    IN BOOLEAN Invalid,
    IN PULONG_PTR Virtual,
    IN ULONG Count
    );

VOID
FASTCALL
KiFlushSingleTb64 (
    IN BOOLEAN Invalid,
    IN ULONG_PTR Virtual
    );

#endif

PULONG
KiGetUserModeStackAddress (
    VOID
    );

VOID
KiInitializeContextThread (
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL,
    IN PCONTEXT ContextFrame OPTIONAL
    );

VOID
KiInitializeKernel (
    IN PKPROCESS Process,
    IN PKTHREAD Thread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
KiInitSystem (
    VOID
    );

BOOLEAN
KiInitMachineDependent (
    VOID
    );

VOID
KiInitializeUserApc (
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

LONG
FASTCALL
KiInsertQueue (
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    IN BOOLEAN Head
    );

BOOLEAN
FASTCALL
KiInsertQueueApc (
    IN PKAPC Apc,
    IN KPRIORITY Increment
    );

LOGICAL
FASTCALL
KiInsertTreeTimer (
    IN PRKTIMER Timer,
    IN LARGE_INTEGER Interval
    );

VOID
KiInterruptDispatch (
    VOID
    );

VOID
KiInterruptDispatchRaise (
    IN PKINTERRUPT Interrupt
    );

VOID
KiInterruptDispatchSame (
    IN PKINTERRUPT Interrupt
    );

#if defined(i386)

VOID
KiInitializePcr (
    IN ULONG Processor,
    IN PKPCR    Pcr,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt,
    IN PKTSS Tss,
    IN PKTHREAD Thread,
    IN PVOID DpcStack
    );

VOID
KiFlushNPXState (
    PFLOATING_SAVE_AREA
    );

VOID
Ke386ConfigureCyrixProcessor (
    VOID
    );

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    );

VOID
KiSetHardwareTrigger (
    VOID
    );

#ifdef DBGMP
VOID
KiPollDebugger (
    VOID
    );
#endif

VOID
FASTCALL
KiIpiSignalPacketDoneAndStall (
    IN PKIPI_CONTEXT Signaldone,
    IN ULONG volatile *ReverseStall
    );

#endif


KIRQL
KiLockDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    );

VOID
KiPassiveRelease (
    VOID
    );

PRKTHREAD
KiQuantumEnd (
    VOID
    );

NTSTATUS
KiRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN BOOLEAN FirstChance
    );

VOID
FASTCALL
KiReadyThread (
    IN PRKTHREAD Thread
    );

LOGICAL
FASTCALL
KiReinsertTreeTimer (
    IN PRKTIMER Timer,
    IN ULARGE_INTEGER DueTime
    );

#if DBG

#define KiRemoveTreeTimer(Timer)               \
    (Timer)->Header.Inserted = FALSE;          \
    RemoveEntryList(&(Timer)->TimerListEntry); \
    (Timer)->TimerListEntry.Flink = NULL;      \
    (Timer)->TimerListEntry.Blink = NULL

#else

#define KiRemoveTreeTimer(Timer)               \
    (Timer)->Header.Inserted = FALSE;          \
    RemoveEntryList(&(Timer)->TimerListEntry)

#endif

#if defined(NT_UP)

#define KiRequestApcInterrupt(Processor) KiRequestSoftwareInterrupt(APC_LEVEL)

#else

#define KiRequestApcInterrupt(Processor)                  \
    if (KeGetCurrentPrcb()->Number == (CCHAR)Processor) { \
        KiRequestSoftwareInterrupt(APC_LEVEL);            \
    } else {                                              \
        KiIpiSend((KAFFINITY)(1 << Processor), IPI_APC);  \
    }

#endif

#if defined(NT_UP)

#define KiRequestDispatchInterrupt(Processor)

#else

#define KiRequestDispatchInterrupt(Processor)             \
    if (KeGetCurrentPrcb()->Number != (CCHAR)Processor) { \
        KiIpiSend((KAFFINITY)(1 << Processor), IPI_DPC);  \
    }

#endif

PRKTHREAD
FASTCALL
KiSelectNextThread (
    IN PRKTHREAD Thread
    );

VOID
KiSetSystemTime (
    IN PLARGE_INTEGER NewTime,
    OUT PLARGE_INTEGER OldTime
    );

VOID
KiSuspendNop (
    IN struct _KAPC *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    );

VOID
KiSuspendThread (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

BOOLEAN
KiSwapProcess (
    IN PKPROCESS NewProcess,
    IN PKPROCESS OldProcess
    );

LONG_PTR
FASTCALL
KiSwapThread (
    VOID
    );

VOID
KiThreadStartup (
    IN PVOID StartContext
    );

VOID
KiTimerExpiration (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
FASTCALL
KiTimerListExpire (
    IN PLIST_ENTRY ExpiredListHead,
    IN KIRQL OldIrql
    );

VOID
KiUnexpectedInterrupt (
    VOID
    );

VOID
KiUnlockDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN KIRQL OldIrql
    );

VOID
FASTCALL
KiUnwaitThread (
    IN PRKTHREAD Thread,
    IN LONG_PTR WaitStatus,
    IN KPRIORITY Increment
    );

VOID
KiUserApcDispatcher (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN PKNORMAL_ROUTINE NormalRoutine
    );

VOID
KiUserExceptionDispatcher (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextFrame
    );

VOID
FASTCALL
KiWaitSatisfyAll (
    IN PRKWAIT_BLOCK WaitBlock
    );

//
// VOID
// FASTCALL
// KiWaitSatisfyAny (
//    IN PKMUTANT Object,
//    IN PKTHREAD Thread
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for any type of object and performs
//    any side effects that are necessary.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
//    Thread - Supplies a pointer to a dispatcher object of type thread.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyAny(_Object_, _Thread_) {                               \
    if (((_Object_)->Header.Type & DISPATCHER_OBJECT_TYPE_MASK) == EventSynchronizationObject) { \
        (_Object_)->Header.SignalState = 0;                                  \
                                                                             \
    } else if ((_Object_)->Header.Type == SemaphoreObject) {                 \
        (_Object_)->Header.SignalState -= 1;                                 \
                                                                             \
    } else if ((_Object_)->Header.Type == MutantObject) {                    \
        (_Object_)->Header.SignalState -= 1;                                 \
        if ((_Object_)->Header.SignalState == 0) {                           \
            (_Thread_)->KernelApcDisable -= (_Object_)->ApcDisable;          \
            (_Object_)->OwnerThread = (_Thread_);                            \
            if ((_Object_)->Abandoned == TRUE) {                             \
                (_Object_)->Abandoned = FALSE;                               \
                (_Thread_)->WaitStatus = STATUS_ABANDONED;                   \
            }                                                                \
                                                                             \
            InsertHeadList((_Thread_)->MutantListHead.Blink,                 \
                           &(_Object_)->MutantListEntry);                    \
        }                                                                    \
    }                                                                        \
}

//
// VOID
// FASTCALL
// KiWaitSatisfyMutant (
//    IN PKMUTANT Object,
//    IN PKTHREAD Thread
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for a mutant object.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
//    Thread - Supplies a pointer to a dispatcher object of type thread.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyMutant(_Object_, _Thread_) {                            \
    (_Object_)->Header.SignalState -= 1;                                     \
    if ((_Object_)->Header.SignalState == 0) {                               \
        (_Thread_)->KernelApcDisable -= (_Object_)->ApcDisable;              \
        (_Object_)->OwnerThread = (_Thread_);                                \
        if ((_Object_)->Abandoned == TRUE) {                                 \
            (_Object_)->Abandoned = FALSE;                                   \
            (_Thread_)->WaitStatus = STATUS_ABANDONED;                       \
        }                                                                    \
                                                                             \
        InsertHeadList((_Thread_)->MutantListHead.Blink,                     \
                       &(_Object_)->MutantListEntry);                        \
    }                                                                        \
}

//
// VOID
// FASTCALL
// KiWaitSatisfyOther (
//    IN PKMUTANT Object
//    )
//
//
// Routine Description:
//
//    This function satisfies a wait for any type of object except a mutant
//    and performs any side effects that are necessary.
//
// Arguments:
//
//    Object - Supplies a pointer to a dispatcher object.
//
// Return Value:
//
//    None.
//

#define KiWaitSatisfyOther(_Object_) {                                       \
    if (((_Object_)->Header.Type & DISPATCHER_OBJECT_TYPE_MASK) == EventSynchronizationObject) { \
        (_Object_)->Header.SignalState = 0;                                  \
                                                                             \
    } else if ((_Object_)->Header.Type == SemaphoreObject) {                 \
        (_Object_)->Header.SignalState -= 1;                                 \
                                                                             \
    }                                                                        \
}

VOID
FASTCALL
KiWaitTest (
    IN PVOID Object,
    IN KPRIORITY Increment
    );

VOID
KiFreezeTargetExecution (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiPollFreezeExecution (
    VOID
    );

VOID
KiSaveProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiSaveProcessorControlState (
    IN PKPROCESSOR_STATE ProcessorState
    );

VOID
KiRestoreProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiRestoreProcessorControlState (
    IN PKPROCESSOR_STATE ProcessorState
    );

#if defined(_ALPHA_)

VOID
KiSynchronizeProcessIds (
    VOID
    );

#endif

BOOLEAN
KiTryToAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#if defined(_ALPHA_)

//
// Prototypes for memory barrier instructions
//

VOID
KiImb(
    VOID
    );

VOID
KiMb(
    VOID
    );

//
// Functions for enabling/disabling alignment exceptions
//
extern ULONG KiEnableAlignmentFaultExceptions;

VOID
KiEnableAlignmentExceptions(
    VOID
    );

VOID
KiDisableAlignmentExceptions(
    VOID
    );

#else

#define KiEnableAlignmentExceptions()
#define KiDisableAlignmentExceptions()

#endif

#endif // _KI_

//
// External references to private kernel data structures
//

#if DEVL
extern PMESSAGE_RESOURCE_DATA  KiBugCodeMessages;
#endif

extern ULONG KiDmaIoCoherency;
extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern KSPIN_LOCK KiContextSwapLock;
extern PKDEBUG_ROUTINE KiDebugRoutine;
extern PKDEBUG_SWITCH_ROUTINE KiDebugSwitchRoutine;
extern KSPIN_LOCK KiDispatcherLock;
extern LIST_ENTRY KiDispatcherReadyListHead[MAXIMUM_PRIORITY];
extern CCHAR KiFindFirstSetLeft[256];
extern CALL_PERFORMANCE_DATA KiFlushSingleCallData;
extern ULONG_PTR KiHardwareTrigger;
extern KAFFINITY KiIdleSummary;
extern UCHAR KiFindLeftNibbleBitTable[];
extern KEVENT KiSwapEvent;
extern LIST_ENTRY KiProcessInSwapListHead;
extern LIST_ENTRY KiProcessOutSwapListHead;
extern LIST_ENTRY KiStackInSwapListHead;
extern LIST_ENTRY KiProfileSourceListHead;
extern BOOLEAN KiProfileAlignmentFixup;
extern ULONG KiProfileAlignmentFixupInterval;
extern ULONG KiProfileAlignmentFixupCount;
extern ULONG KiProfileInterval;
extern LIST_ENTRY KiProfileListHead;
extern KSPIN_LOCK KiProfileLock;
extern ULONG KiReadySummary;
extern UCHAR KiArgumentTable[];
extern ULONG KiServiceLimit;
extern ULONG_PTR KiServiceTable[];
extern CALL_PERFORMANCE_DATA KiSetEventCallData;
extern ULONG KiTickOffset;
extern LARGE_INTEGER KiTimeIncrementReciprocal;
extern CCHAR KiTimeIncrementShiftCount;
extern LIST_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
extern KAFFINITY KiTimeProcessor;
extern KDPC KiTimerExpireDpc;
extern KSPIN_LOCK KiFreezeExecutionLock;
extern BOOLEAN KiSlavesStartExecution;
extern PSWAP_CONTEXT_NOTIFY_ROUTINE KiSwapContextNotifyRoutine;
extern PTHREAD_SELECT_NOTIFY_ROUTINE KiThreadSelectNotifyRoutine;
extern PTIME_UPDATE_NOTIFY_ROUTINE KiTimeUpdateNotifyRoutine;
extern LIST_ENTRY KiWaitInListHead;
extern LIST_ENTRY KiWaitOutListHead;
extern CALL_PERFORMANCE_DATA KiWaitSingleCallData;
extern ULONG KiEnableTimerWatchdog;

#if defined(_IA64_)

extern ULONG KiGlobalRid;
extern ULONG KiMasterRid;
extern ULONG KiMasterSequence;
extern ULONG KiIdealDpcRate;

#if !defined(UP_NT)

extern KSPIN_LOCK KiMasterRidLock;

#endif

VOID
KiSaveEmDebugContext (
    IN OUT PCONTEXT Context
    );

VOID
KiLoadEmDebugContext (
    IN PCONTEXT Context
    );

VOID
KiFlushRse (
    VOID
    );

VOID
KiInvalidateStackedRegisters (
    VOID
    );

VOID
KiSetNewRid (
    ULONG NewGlobalRid,
    ULONG NewProcessRid
    );

NTSTATUS
Ki386CheckDivideByZeroTrap(
    IN PKTRAP_FRAME Frame
    );

#endif // defined(_IA64_)

#if defined(i386)

extern KIRQL KiProfileIrql;

BOOLEAN
KeInvalidateAllCaches (
    IN BOOLEAN AllProcessors
    );

#endif

#if defined(_ALPHA_)

extern ULONG KiMaximumAsn;
extern ULONGLONG KiMasterSequence;
extern LONG KiMbTimeStamp;
extern ULONG KiSynchIrql;

#endif

#if defined(_ALPHA_) || defined(_IA64_)

extern KINTERRUPT KxUnexpectedInterrupt;

#endif

#if NT_INST

extern KIPI_COUNTS KiIpiCounts[MAXIMUM_PROCESSORS];

#endif

extern KSPIN_LOCK KiFreezeLockBackup;
extern ULONG KiFreezeFlag;
extern volatile ULONG KiSuspendState;

#if DBG

extern ULONG KiMaximumSearchCount;

#endif
