/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ketypes.h (ARM)

Abstract:

    ARM Type definitions for the Kernel services.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _ARM_KETYPES_H
#define _ARM_KETYPES_H

//
// Dependencies
//

//
// IPI Types
//
#define IPI_APC                 1
#define IPI_DPC                 2
#define IPI_FREEZE              4
#define IPI_PACKET_READY        8
#define IPI_SYNCH_REQUEST       16

//
// PRCB Flags
//
#define PRCB_MAJOR_VERSION      1
#define PRCB_BUILD_DEBUG        1
#define PRCB_BUILD_UNIPROCESSOR 2

//
// No LDTs on ARM
//
#define LDT_ENTRY              ULONG

//
// HAL Variables
//
#define INITIAL_STALL_COUNT     0x64

//
// Static Kernel-Mode Address start (use MM_KSEG0_BASE for actual)
//
#define KSEG0_BASE              0x80000000

//
// Trap Frame Definition
//
typedef struct _KTRAP_FRAME
{
    ULONG OldIrql;
    //  UCHAR PreviousMode;
    //    ULONG Fpscr;
    //    ULONG FpExc;
    //    ULONG S[33];
    //    ULONG FpExtra[8];    
    ULONG Spsr;
    ULONG R0;
    ULONG R1;
    ULONG R2;
    ULONG R3;
    ULONG R4;
    ULONG R5;
    ULONG R6;
    ULONG R7;
    ULONG R8;
    ULONG R9;
    ULONG R10;
    ULONG R11;
    ULONG R12;
    ULONG UserSp;
    ULONG UserLr;
    ULONG SvcSp;
    ULONG SvcLr;
    ULONG Pc;
} KTRAP_FRAME, *PKTRAP_FRAME;

#ifndef NTOS_MODE_USER

//
// Exception Frame Definition
//
typedef struct _KEXCEPTION_FRAME
{
    ULONG R4;
    ULONG R5;
    ULONG R6;
    ULONG R7;
    ULONG R8;
    ULONG R9;
    ULONG R10;
    ULONG R11;
//    ULONG R12;
//    ULONG Sp;
    ULONG Psr;
    ULONG Lr;
//    ULONG SwapReturn;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

//
// Processor State
//
typedef struct _KPROCESSOR_STATE
{
    struct _CONTEXT ContextFrame;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

//
// Processor Region Control Block
//
typedef struct _KPRCB
{
    USHORT MinorVersion;
    USHORT MajorVersion;
    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;
    UCHAR Number;
    UCHAR Reserved;
    USHORT BuildType;
    KAFFINITY SetMember;
    KPROCESSOR_STATE ProcessorState;
    ULONG KernelReserved[16];
    ULONG HalReserved[16];
    KSPIN_LOCK_QUEUE LockQueue[LockQueueMaximumLock];
    struct _KTHREAD *NpxThread;
    ULONG InterruptCount;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG DpcTime;
    ULONG DebugDpcTime;
    ULONG InterruptTime;
    ULONG AdjustDpcThreshold;
    ULONG PageColor;
    UCHAR SkipTick;
    UCHAR DebuggerSavedIRQL;
    UCHAR NodeColor;
    UCHAR Spare1;
    ULONG NodeShiftedColor;
    ULONG PcrPage;
    struct _KNODE *ParentNode;
    ULONG MultiThreadProcessorSet;
    struct _KPRCB *MultiThreadSetMaster;
    ULONG SecondaryColorMask;
    LONG Sleeping;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG KeAlignmentFixupCount;
    ULONG SpareCounter0;
    ULONG KeDcacheFlushCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeFirstLevelTbFills;
    ULONG KeIcacheFlushCount;
    ULONG KeSecondLevelTbFills;
    ULONG KeSystemCalls;
    volatile ULONG IoReadOperationCount;
    volatile ULONG IoWriteOperationCount;
    volatile ULONG IoOtherOperationCount;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    PP_LOOKASIDE_LIST PPLookasideList[16];
    PP_LOOKASIDE_LIST PPNPagedLookasideList[32];
    PP_LOOKASIDE_LIST PPPagedLookasideList[32];
    volatile ULONG PacketBarrier;
    volatile ULONG ReverseStall;
    PVOID IpiFrame;
    volatile PVOID CurrentPacket[3];
    volatile ULONG TargetSet;
    volatile PKIPI_WORKER WorkerRoutine;
    volatile ULONG IpiFrozen;
    volatile ULONG RequestSummary;
    volatile struct _KPRCB *SignalDone;
    struct _KDPC_DATA DpcData[2];
    PVOID DpcStack;
    ULONG MaximumDpcQueueDepth;
    ULONG DpcRequestRate;
    ULONG MinimumDpcRate;
    volatile UCHAR DpcInterruptRequested;
    volatile UCHAR DpcThreadRequested;
    volatile UCHAR DpcRoutineActive;
    volatile UCHAR DpcThreadActive;
    ULONG PrcbLock;
    ULONG DpcLastCount;
    volatile ULONG TimerHand;
    volatile ULONG TimerRequest;
    PVOID DpcThread;
    KEVENT DpcEvent;
    UCHAR ThreadDpcEnable;
    volatile BOOLEAN QuantumEnd;
    volatile UCHAR IdleSchedule;
    LONG DpcSetEventRequest;
    LONG TickOffset;
    KDPC CallDpc;
    LIST_ENTRY WaitListHead;
    ULONG ReadySummary;
    ULONG QueueIndex;
    LIST_ENTRY DispatcherReadyListHead[32];
    SINGLE_LIST_ENTRY DeferredReadyListHead;
    PVOID ChainedInterruptList;
    LONG LookasideIrpFloat;
    volatile LONG MmPageFaultCount;
    volatile LONG MmCopyOnWriteCount;
    volatile LONG MmTransitionCount;
    volatile LONG MmCacheTransitionCount;
    volatile LONG MmDemandZeroCount;
    volatile LONG MmPageReadCount;
    volatile LONG MmPageReadIoCount;
    volatile LONG MmCacheReadCount;
    volatile LONG MmCacheIoCount;
    volatile LONG MmDirtyPagesWriteCount;
    volatile LONG MmDirtyWriteIoCount;
    volatile LONG MmMappedPagesWriteCount;
    volatile LONG MmMappedWriteIoCount;
    CHAR VendorString[13];
    ULONG MHz;
    ULONG FeatureBits;
    volatile LARGE_INTEGER IsrTime;
    PROCESSOR_POWER_STATE PowerState;
} KPRCB, *PKPRCB;

//
// Macro to get current KPRCB
//
FORCEINLINE
struct _KPRCB *
KeGetCurrentPrcb(VOID)
{
    return PCR->Prcb;
}

#endif
#endif
