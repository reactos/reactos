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
// IRQLs
//
#define PASSIVE_LEVEL           0
#define LOW_LEVEL               0
#define APC_LEVEL               1
#define DISPATCH_LEVEL          2
#define IPI_LEVEL               7
#define POWER_LEVEL             7
#define PROFILE_LEVEL           8
#define HIGH_LEVEL              8
#define SYNCH_LEVEL             (IPI_LEVEL - 1)

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
// HAL Variables
//
#define INITIAL_STALL_COUNT     0x64

//
// Static Kernel-Mode Address start (use MM_KSEG0_BASE for actual)
//
#define KSEG0_BASE              0x80000000

//
// FIXME: mmtypes.h?
//
#define KIPCR                   0xFFFFF000
#define USPCR                   0x7FFF0000
#define PCR                     ((volatile KPCR * const)USPCR)
#define USERPCR                 ((volatile KPCR * const)KIPCR)

//
// Synchronization-level IRQL
//
#define SYNCH_LEVEL             DISPATCH_LEVEL

//
// Trap Frame Definition
//
typedef struct _KTRAP_FRAME
{
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
    ULONG Sp;
    ULONG Lr;
    ULONG Pc;
    ULONG Psr;   
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 7) & (~7)];
    UCHAR OldIrql;
    UCHAR PreviousMode;
    ULONG Fpscr;
    ULONG FpExc;
    ULONG S[33];
    ULONG FpExtra[8];
} KTRAP_FRAME, *PKTRAP_FRAME;

//
// Processor Control Region
// On ARM, it's actually readable from user-mode, much like KUSER_SHARED_DATA
//
#ifdef NTOS_MODE_USER
#define PKINTERRUPT_ROUTINE PVOID // Hack!
#endif
typedef struct _KPCR
{
    ULONG MinorVersion;
    ULONG MajorVersion;
    PKINTERRUPT_ROUTINE InterruptRoutine[64];
    PVOID XcodeDispatch;
    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;
    ULONG SecondLevelDcacheSize;
    ULONG SecondLevelDcacheFillSize;
    ULONG SecondLevelIcacheSize;
    ULONG SecondLevelIcacheFillSize;
    struct _KPRCB *Prcb;
    struct _TEB *Teb;
    PVOID TlsArray;
    ULONG DcacheFillSize;
    ULONG IcacheAlignment;
    ULONG IcacheFillSize;
    ULONG ProcessorId;
    ULONG ProfileInterval;
    ULONG ProfileCount;
    ULONG StallExecutionCount;
    ULONG StallScaleFactor;
    CCHAR Number;
    PVOID DataBusError;
    PVOID InstructionBusError;
    ULONG CachePolicy;
    UCHAR IrqlMask[64];
    UCHAR IrqlTable[64];
    UCHAR CurrentIrql;
    KAFFINITY SetMember;
    struct _KTHREAD *CurrentThread;
    KAFFINITY NotMember;
    ULONG SystemReserved[6];
    ULONG DcacheAlignment;
    ULONG HalReserved[64];
    BOOLEAN FirstLevelActive;
    BOOLEAN DpcRoutineActive;
    ULONG CurrentPid;
    BOOLEAN OnInterruptStack;
    PVOID SavedInitialStack;
    PVOID SavedStackLimit;
    PVOID SystemServiceDispatchStart;
    PVOID SystemServiceDispatchEnd;
    PVOID InterruptStack;
    PVOID PanicStack;
    PVOID BadVaddr;
    PVOID InitialStack;
    PVOID StackLimit;
    ULONG QuantumEnd;
} KPCR, *PKPCR;

#ifndef NTOS_MODE_USER
//
// Stub
//
typedef struct _KFLOATING_SAVE
{
    ULONG Reserved;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

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
    //
    // TODO
    //
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

//
// Macro to get current CPU
//
FORCEINLINE
ULONG
DDKAPI
KeGetCurrentProcessorNumber(VOID)
{
    return PCR->Number;
}

#endif
#endif
