//       TITLE("High Frequency Spin Locks")
//++
//
// Copyright (c) 1993  Microsoft Corporation
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    hifreqlk.s
//
// Abstract:
//
//    This module contains storage for high frequency spin locks. Each
//    is allocated to a separate cache line.
//
// Author:
//
//    David N. Cutler (davec) 25-Jun-1993
//    Joe Notarangelo  29-Nov-1993
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

//
// Define alignment for mp and up spinlocks.
//

#if defined(NT_UP)

#define ALIGN

#else

#define ALIGN .align 6

#endif

//
// Define spinlock size for mp and up systems.
//

#if defined(_AXP64_)

#define SPIN_LOCK .quad 0

#else

#define SPIN_LOCK .long 0

#endif

        .sdata
        ALIGN
        SPIN_LOCK

        .globl  CcMasterSpinLock
        ALIGN
CcMasterSpinLock:                       // cache manager master lock
        SPIN_LOCK                       //

        .globl  CcVacbSpinLock
        ALIGN
CcVacbSpinLock:                         // cache manager VACB lock
        SPIN_LOCK                       //

        .globl  ExpResourceSpinLock
        ALIGN
ExpResourceSpinLock:                    // resource package lock
        SPIN_LOCK                       //

        .globl  IopCancelSpinLock
        ALIGN
IopCancelSpinLock:                      // I/O cancel lock
        SPIN_LOCK                       //

        .globl  IopCompletionLock
        ALIGN
IopCompletionLock:                      // I/O completion lock
        SPIN_LOCK                       //

        .globl  IopDatabaseLock
        ALIGN
IopDatabaseLock:                        // I/O database lock
        SPIN_LOCK                       //

        .globl  IopVpbSpinLock
        ALIGN
IopVpbSpinLock:                         // I/O VPB lock
        SPIN_LOCK                       //

        .globl  IoStatisticsLock
        ALIGN
IoStatisticsLock:                       // I/O statistics lock
        SPIN_LOCK                       //

        .globl  IopFastLockSpinLock
        ALIGN
IopFastLockSpinLock:                    // fast I/O path lock
        SPIN_LOCK                       //

        .globl  KiContextSwapLock
        ALIGN
KiContextSwapLock:                      // context swap lock
        SPIN_LOCK                       //

        .globl  KiMasterSequence
KiMasterSequence:                       // master sequence number
        .quad   1                       //

        .globl  KiMasterAsn
KiMasterAsn:                            // master ASN number
        .long   0                       //

        .globl  KiMaximumAsn
KiMaximumAsn:                           //
        .long   0                       //

        .globl  KiTbiaFlushRequest      //
KiTbiaFlushRequest:                     // TB invalidate all mask
        .long   0xFFFFFFFF              //

        .globl  KiDispatcherLock
        ALIGN
KiDispatcherLock:                       // dispatcher database lock
        SPIN_LOCK                       //

        .globl  KiSynchIrql
KiSynchIrql:                            // synchronization IRQL
        .long   SYNCH_LEVEL             //

        .globl  MmPfnLock
        ALIGN
MmPfnLock:                              // page frame database lock
        SPIN_LOCK                       //

        .globl  MmChargeCommitmentLock
        ALIGN
MmChargeCommitmentLock:                 // charge commitment lock
        SPIN_LOCK                       //

        .globl  NonPagedPoolLock
        ALIGN
NonPagedPoolLock:                       // nonpage pool allocation lock
        SPIN_LOCK                       //

//
// IopLookasideIrpFloat - This is the number of IRPs that are currently
//      in progress that were allocated from a lookaside list.
//

        .globl  IopLookasideIrpFloat
        ALIGN
IopLookasideIrpFloat:                   //
        .long   0                       //

//
// IopLookasideIrpLimit - This is the maximum number of IRPs that can be
//      in progress that were allocated from a lookaside list.
//

        .globl  IopLookasideIrpLimit
IopLookasideIrpLimit:                   //
        .long   0                       //

//
// KeTickCount - This is the number of clock ticks that have occurred since
//      the system was booted. This count is used to compute a millisecond
//      tick counter.
//

        .align  6
        .globl  KeTickCount
KeTickCount:                            //
        .long   0, 0                    //

//
// KeMaximumIncrement - This is the maximum time between clock interrupts
//      in 100ns units that is supported by the host HAL.
//

        .globl  KeMaximumIncrement
KeMaximumIncrement:                     //
        .long   0                       //

//
// KeTimeAdjustment - This is the actual number of 100ns units that are to
//      be added to the system time at each interval timer interupt. This
//      value is copied from KeTimeIncrement at system start up and can be
//      later modified via the set system information service.
//      timer table entries.
//

        .globl  KeTimeAdjustment
KeTimeAdjustment:                       //
        .long   0                       //

//
// KiTickOffset - This is the number of 100ns units remaining before a tick
//      is added to the tick count and the system time is updated.
//

        .globl  KiTickOffset
KiTickOffset:                           //
        .long   0                       //

//
// KiMaximumDpcQueueDepth - This is used to control how many DPCs can be
//      queued before a DPC of medium importance will trigger a dispatch
//      interrupt.
//

        .globl  KiMaximumDpcQueueDepth
KiMaximumDpcQueueDepth:                 //
        .long   4                       //

//
// KiMinimumDpcRate - This is the rate of DPC requests per clock tick that
//      must be exceeded before DPC batching of medium importance DPCs
//      will occur.
//

        .globl  KiMinimumDpcRate
KiMinimumDpcRate:                       //
        .long   3                       //

//
// KiAdjustDpcThreshold - This is the threshold used by the clock interrupt
//      routine to control the rate at which the processor's DPC queue depth
//      is dynamically adjusted.
//

        .globl  KiAdjustDpcThreshold
KiAdjustDpcThreshold:                   //
        .long   20                      //

//
// KiIdealDpcRate - This is used to control the aggressiveness of the DPC
//      rate adjusting algorithm when decrementing the queue depth. As long
//      as the DPC rate for the last tick is greater than this rate, the
//      DPC queue depth will not be decremented.
//

        .globl  KiIdealDpcRate
KiIdealDpcRate:                         //
        .long   20                      //

//
// KiMbTimeStamp - This is the memory barrier time stamp value that is
//      incremented each time a global memory barrier is executed on an
//      MP system.
//

#if !defined(NT_UP)

        ALIGN
        .globl  KiMbTimeStamp
KiMbTimeStamp:                          //
        .long   0                       //

#endif
