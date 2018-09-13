//      TITLE("Context Swap")
//++
//
// Copyright (c) 1994  IBM Corporation
//
// Module Name:
//
//    ctxswap.s
//
// Abstract:
//
//    This module implements the PowerPC machine dependent code necessary to
//    field the dispatch interrupt and to perform kernel initiated context
//    switching.
//
// Author:
//
//    Peter L. Johnston (plj@vnet.ibm.com) August 1993
//    Adapted from code by David N. Cutler (davec) 1-Apr-1991
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//      plj    Apr-94    Upgraded to NT 3.5.
//
//--

#include "ksppc.h"

//  Module Constants

#define rPrcb           r.29
#define OTH             r.30
#define NTH             r.31

//  Global externals

        .extern ..KdPollBreakIn
        .extern ..KeFlushCurrentTb
        .extern ..KiActivateWaiterQueue
        .extern ..KiContinueClientWait
        .extern ..KiDeliverApc
        .extern ..KiQuantumEnd
        .extern ..KiReadyThread
        .extern ..KiWaitTest

        .extern KdDebuggerEnabled
        .extern KeTickCount
        .extern KiDispatcherReadyListHead
        .extern KiIdleSummary
        .extern KiReadySummary
        .extern KiWaitInListHead
        .extern KiWaitOutListHead
        .extern __imp_HalProcessorIdle
        .extern __imp_KeLowerIrql
#if DBG
        .extern ..DbgBreakPoint
        .extern ..DbgBreakPointWithStatus
#endif
#if !defined(NT_UP)

        .extern KeNumberProcessors
        .extern KiBarrierWait
        .extern KiContextSwapLock
        .extern KiDispatcherLock
        .extern KiProcessorBlock

#if SPINDBG
        .extern ..KiAcquireSpinLockDbg
        .extern ..KiTryToAcquireSpinLockDbg
#endif

#endif
        .extern KiMasterSequence
        .extern KiMasterPid

        .globl  KiScheduleCount
        .data
KiScheduleCount:
        .long   0

#if COLLECT_PAGING_DATA
        .globl  KiFlushOnProcessSwap
        .data
KiFlushOnProcessSwap:
        .long   0
#endif


//
// ThreadSwitchFrame
//
// This is the layout of the beginning of the stack frame that must be
// established by any routine that calls SwapContext.
//
// The caller of SwapContext must have saved r.14 and 26 thru 31.
// SwapContext will take care of r.15 thru r.25, f.14 thru f.31 and
// the condition register.
//
// Note: this is not a complete stack frame, the caller must allocate
//       additional space for any additional registers it needs to
//       save (eg Link Register).  (Also, the following has not been
//       padded to 8 bytes).
//
// WARNING: KiInitializeContextThread() is aware of the layout of
//          a ThreadSwitchFrame.
//

        .struct 0
        .space  StackFrameHeaderLength
swFrame:.space  SwapFrameLength
swFrameLength:


//      SBTTL("Switch To Thread")
//++
//
// NTSTATUS
// KiSwitchToThread (
//    IN PKTHREAD NextThread,
//    IN ULONG WaitReason,
//    IN ULONG WaitMode,
//    IN PKEVENT WaitObject
//    )
//
// Routine Description:
//
//    This function performs an optimal switch to the specified target thread
//    if possible. No timeout is associated with the wait, thus the issuing
//    thread will wait until the wait event is signaled or an APC is deliverd.
//
//    N.B. This routine is called with the dispatcher database locked.
//
//    N.B. The wait IRQL is assumed to be set for the current thread and the
//        wait status is assumed to be set for the target thread.
//
//    N.B. It is assumed that if a queue is associated with the target thread,
//        then the concurrency count has been incremented.
//
//    N.B. Control is returned from this function with the dispatcher database
//        unlocked.
//
// Arguments:
//
//    NextThread - Supplies a pointer to a dispatcher object of type thread.
//
//    WaitReason - supplies the reason for the wait operation.
//
//    WaitMode  - Supplies the processor wait mode.
//
//    WaitObject - Supplies a pointer to a dispatcher object of type event
//        or semaphore.
//
// Return Value:
//
//    The wait completion status. A value of STATUS_SUCCESS is returned if
//    the specified object satisfied the wait. A value of STATUS_USER_APC is
//    returned if the wait was aborted to deliver a user APC to the current
//    thread.
//--

                .struct 0
                .space  swFrameLength
sttLR:          .space  4
sttReason:      .space  4
sttMode:        .space  4
sttObject:      .space  4
                .align  3                       // ensure 8 byte alignment
sttFrameLength:

        SPECIAL_ENTRY_S(KiSwitchToThread,_TEXT$00)

        mflr    r.0                             // get return address
        stwu    r.sp, -sttFrameLength(r.sp)     // buy stack frame
        stw     r.14, swFrame + ExGpr14(r.sp)   // save gpr 14
        stw     r.26, swFrame + ExGpr26(r.sp)   // save gprs 26 through 31
        stw     r.27, swFrame + ExGpr27(r.sp)
        stw     r.28, swFrame + ExGpr28(r.sp)
        stw     r.29, swFrame + ExGpr29(r.sp)
        stw     r.30, swFrame + ExGpr30(r.sp)
        stw     r.31, swFrame + ExGpr31(r.sp)
        ori     NTH, r.3, 0
        stw     r.0, sttLR(r.sp)                // save return address
        li      r.0, 0

        PROLOGUE_END(KiSwitchToThread)

//
// Save the wait reason, the wait mode, and the wait object address.
//

        stw     r.4, sttReason(r.sp)            // save wait reason
        stw     r.5, sttMode(r.sp)              // save wait mode
        stw     r.6, sttObject(r.sp)            // save wait object address

//
// If the target thread's kernel stack is resident, the target thread's
// process is in the balance set, the target thread can run on the
// current processor, and another thread has not already been selected
// to run on the current processor, then do a direct dispatch to the
// target thread bypassing all the general wait logic, thread priorities
// permiting.
//

        lwz     r.7, ThApcState + AsProcess(NTH) // get target process address
        lbz     r.8, ThKernelStackResident(NTH) // get kernel stack resident
        lwz     rPrcb, KiPcr + PcPrcb(r.0)      // get address of PRCB
        lbz     r.10, PrState(r.7)              // get target process state
        lwz     OTH, KiPcr + PcCurrentThread(r.0) // get current thread address
        cmpwi   r.8, 0                          // kernel stack resident?
        beq     LongWay                         // if eq, kernel stack not resident
        cmpwi   r.10, ProcessInMemory           // process in memory?
        bne     LongWay                         // if ne, process not in memory

#if !defined(NT_UP)

        lwz     r.8, PbNextThread(rPrcb)        // get address of next thread
        lbz     r.10, ThNextProcessor(OTH)      // get current processor number
        lwz     r.14, ThAffinity(NTH)           // get target thread affinity
        lwz     r.26, KiPcr + PcSetMember(r.0)  // get processor set member
        cmpwi   r.8, 0                          // next thread selected?
        bne     LongWay                         // if ne, next thread selected
        and.    r.26, r.26, r.14                // check for compatible affinity
        beq     LongWay                         // if eq, affinity not compatible

#endif

//
// Compute the new thread priority.
//

        lbz     r.14, ThPriority(OTH)           // get client thread priority
        lbz     r.26, ThPriority(NTH)           // get server thread priority
        cmpwi   r.14, LOW_REALTIME_PRIORITY     // check if realtime client
        cmpwi   cr.7, r.26, LOW_REALTIME_PRIORITY // check if realtime server
        bge     stt60                           // if ge, realtime client
        lbz     r.27, ThPriorityDecrement(NTH)  // get priority decrement value
        lbz     r.28, ThBasePriority(NTH)       // get server base priority
        bge     cr.7, stt50                     // if ge, realtime server
        addi    r.9, r.28, 1                    // computed boosted priority
        cmpwi   r.27, 0                         // server boot active?
        bne     stt30                           // if ne, server boost active

//
// Both the client and the server are not realtime and a priority boost
// is not currently active for the server. Under these conditions an
// optimal switch to the server can be performed if the base priority
// of the server is above a minimum threshold or the boosted priority
// of the server is not less than the client priority.
//

        cmpw    r.9, r.14                       // check if high enough boost
        cmpwi   cr.7, r.9, LOW_REALTIME_PRIORITY // check if less than realtime
        blt     stt20                           // if lt, boosted priority less
        stb     r.9, ThPriority(NTH)            // asssume boosted priority is okay
        blt     cr.7, stt70                     // if lt, less than realtime
        li      r.9, LOW_REALTIME_PRIORITY - 1  // set high server priority
        stb     r.9, ThPriority(NTH)            //
        b       stt70

stt20:

//
// The boosted priority of the server is less than the current priority of
// the client. If the server base priority is above the required threshold,
// then a optimal switch to the server can be performed by temporarily
// raising the priority of the server to that of the client.
//

        cmpwi   r.28, BASE_PRIORITY_THRESHOLD   // check if above threshold
        sub     r.9, r.14, r.28                 // compute priority decrement value
        blt     LongWay                         // if lt, priority below threshold
        li      r.28, ROUND_TRIP_DECREMENT_COUNT // get system decrement count value
        stb     r.9, ThPriorityDecrement(NTH)   // set priority decrement value
        stb     r.14, ThPriority(NTH)           // set current server priority
        stb     r.28, ThDecrementCount(NTH)     // set server decrement count
        b       stt70

stt30:

//
// A server boost has previously been applied to the server thread. Count
// down the decrement count to determine if another optimal server switch
// is allowed.
//


        lbz     r.9, ThDecrementCount(NTH)      // decrement server count value
        subic.  r.9, r.9, 1                     //
        stb     r.9, ThDecrementCount(NTH)      // store updated decrement count
        beq     stt40                           // if eq, no more switches allowed

//
// Another optimal switch to the server is allowed provided that the
// server priority is not less than the client priority.
//

        cmpw    r.26, r.14                      // check if server lower priority
        bge     stt70                           // if ge, server not lower priority
        b       LongWay

stt40:

//
// The server has exhausted the number of times an optimal switch may
// be performed without reducing its priority. Reduce the priority of
// the server to its original unboosted value minus one.
//

        stb     r.0, ThPriorityDecrement(NTH)   // clear server priority decrement
        stb     r.28, ThPriority(NTH)           // set server priority to base
        b       LongWay

stt50:

//
// The client is not realtime and the server is realtime. An optimal switch
// to the server can be performed.
//

        lbz     r.9, PrThreadQuantum(r.7)       // get process quantum value
        b       stt65

stt60:

//
// The client is realtime. In order for an optimal switch to occur, the
// server must also be realtime and run at a high or equal priority.
//

        cmpw    r.26, r.14                      // check if server is lower priority
        lbz     r.9, PrThreadQuantum(r.7)       // get process quantum value
        blt     LongWay                         // if lt, server is lower priority

stt65:

        stb     r.9, ThQuantum(NTH)             // set server thread quantum

stt70:

//
// Set the next processor for the server thread.
//

#if !defined(NT_UP)

        stb     r.10, ThNextProcessor(NTH)      // set server next processor number

#endif

//
// Set the address of the wait block list in the client thread, initialize
// the event wait block, and insert the wait block in client event wait list.
//

        addi    r.8, OTH, EVENT_WAIT_BLOCK_OFFSET // compute wait block address
        stw     r.8, ThWaitBlockList(OTH)       // set address of wait block list
        stw     r.0, ThWaitStatus(OTH)          // set initial wait status
        stw     r.6, WbObject(r.8)              // set address of wait object
        stw     r.8, WbNextWaitBlock(r.8)       // set next wait block address
        lis     r.10, WaitAny                   // get wait type and wait key
        stw     r.10, WbWaitKey(r.8)            // set wait key and wait type
        addi    r.10, r.6, EvWaitListHead       // compute wait object listhead address
        lwz     r.14, LsBlink(r.10)             // get backward link of listhead
        addi    r.26, r.8, WbWaitListEntry      // compute wait block list entry address
        stw     r.26, LsBlink(r.10)             // set backward link of listhead
        stw     r.26, LsFlink(r.14)             // set forward link in last entry
        stw     r.10, LsFlink(r.26)             // set forward link in wait entry
        stw     r.14, LsBlink(r.26)             // set backward link in wait entry

//
// Set the client thread wait parameters, set the thread state to Waiting,
// and insert the thread in the proper wait list.
//

        stb     r.0, ThAlertable(OTH)           // set alertable FALSE.
        stb     r.4, ThWaitReason(OTH)          // set wait reason
        stb     r.5, ThWaitMode(OTH)            // set the wait mode
        lbz     r.6, ThEnableStackSwap(OTH)     // get kernel stack swap enable
        lwz     r.10, [toc]KeTickCount(r.toc)   // get &KeTickCount
        lwz     r.10, 0(r.10)                   // get low part of tick count
        stw     r.10, ThWaitTime(OTH)           // set thread wait time
        li      r.8, Waiting                    // set thread state
        stb     r.8, ThState(OTH)               //
        lwz     r.8,[toc]KiWaitInListHead(r.toc) // get address of wait in listhead
        cmpwi   r.5, 0                          // is wait mode kernel?
        beq     stt75                           // if eq, wait mode is kernel
        cmpwi   r.6, 0                          // is kernel stack swap disabled?
        beq     stt75                           // if eq, kernel stack swap disabled
        cmpwi   r.14, LOW_REALTIME_PRIORITY + 9 // check if priority in range
        blt     stt76                           // if lt, thread priority in range
stt75:
        lwz     r.8,[toc]KiWaitOutListHead(r.toc) // get address of wait in listhead
stt76:
        lwz     r.14, LsBlink(r.8)              // get backlink of wait listhead
        addi    r.26, OTH, ThWaitListEntry      // compute wait list entry address
        stw     r.26, LsBlink(r.8)              // set backward link of listhead
        stw     r.26, LsFlink(r.14)             // set forward link in last entry
        stw     r.8, LsFlink(r.26)              // set forward link in wait entry
        stw     r.14, LsBlink(r.26)             // set backward link in wait entry

stt77:

//
// If the current thread is processing a queue entry, then attempt to
// activate another thread that is blocked on the queue object.
//
// N.B. The next thread address can change if the routine to activate
//      a queue waiter is called.
//

        lwz     r.3, ThQueue(OTH)               // get queue object address
        cmpwi   r.3, 0                          // queue object attached?
        beq     stt78                           // if eq, no queue object attached
        stw     NTH, PbNextThread(rPrcb)        // set next thread address
        bl      ..KiActivateWaiterQueue         // attempt to activate a blocked thread
        lwz     NTH, PbNextThread(rPrcb)        // get next thread address
        li      r.0, 0
        stw     r.0, PbNextThread(rPrcb)        // set next thread address to NULL
stt78:

#if !defined(NT_UP)

        lwz     r.27, [toc]KiContextSwapLock(r.2)// get &KiContextSwapLock
        lwz     r.28, [toc]KiDispatcherLock(r.2) // get &KiDispatcherLock

#endif

        stw     NTH, PbCurrentThread(rPrcb)     // set address of current thread object
        bl      ..SwapContext                   // swap context

//
// Lower IRQL to its previous level.
//
// N.B. SwapContext releases the dispatcher database lock.
//
// N.B. Register NTH (r.31) contains the address of the new thread on return.
//
// In the following, we could lower IRQL, isync then check for pending
// interrupts.  I believe it is faster to disable interrupts and get
// both loads going.  We need to avoid the situation where a DPC or
// APC could be queued between the time we load PcSoftwareInterrupt
// and actually lowering IRQL. (plj)
//
// We load the thread's WaitStatus in this block for scheduling
// reasons in the hope that the normal case will be there is NOT
// a DPC or APC pending.
//

        lbz     r.27, ThWaitIrql(NTH)           // get original IRQL

        DISABLE_INTERRUPTS(r.5, r.6)

        lhz     r.4, KiPcr+PcSoftwareInterrupt(r.0)
        lwz     r.26, ThWaitStatus(NTH)         // get wait completion status
        stb     r.27, KiPcr+PcCurrentIrql(r.0)  // set new IRQL

        ENABLE_INTERRUPTS(r.5)

        cmpw    r.4, r.27                       // check if new IRQL allows
        ble+    stt79                           //  APC/DPC and one is pending

        bl      ..KiDispatchSoftwareInterrupt   // process software interrupt

stt79:

//
// If the wait was not interrupted to deliver a kernel APC, then return the
// completion status.
//

        cmpwi   r.26, STATUS_KERNEL_APC         // check if awakened for kernel APC
        ori     r.3, r.26, 0                    // set return status
        bne     stt90                           // if ne, normal wait completion

//
// Disable interrupts and acquire the dispatcher database lock.
//

#if !defined(NT_UP)

        DISABLE_INTERRUPTS(r.5, r.6)

//
// WARNING:  The address of KiDispatcherLock was intentionally left in
//           r.28 by SwapContext for use here.
//

        ACQUIRE_SPIN_LOCK(r.28, NTH, r.7, stt80, stt82)

#endif

//
// Raise IRQL to synchronization level and save wait IRQL.
//

        li      r.7, SYNCH_LEVEL
        stb     r.7, KiPcr+PcCurrentIrql(r.0)

#if !defined(NT_UP)
        ENABLE_INTERRUPTS(r.5)
#endif

        stb     r.27, ThWaitIrql(NTH)           // set client wait IRQL

        b       ContinueWait

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.28, r.7, stt80, stt82, stt85, r.5, r.6)
#endif

LongWay:

//
// Ready the target thread for execution and wait on the specified wait
// object.
//

        bl      ..KiReadyThread                 // ready thread for execution

//
// Continue the wait and return the wait completion status.
//
// N.B. The wait continuation routine is called with the dispatcher
//      database locked.
//

ContinueWait:

        lwz     r.3, sttObject(r.sp)            // get wait object address
        lwz     r.4, sttReason(r.sp)            // get wait reason
        lwz     r.5, sttMode(r.sp)              // get wait mode
        bl      ..KiContinueClientWait          // continue client wait

stt90:
        lwz     r.0,  sttLR(r.sp)               // restore return address
        lwz     r.26, swFrame + ExGpr26(r.sp)   // restore gprs 26 thru 31
        lwz     r.27, swFrame + ExGpr27(r.sp)   //
        lwz     r.28, swFrame + ExGpr28(r.sp)   //
        lwz     r.29, swFrame + ExGpr29(r.sp)   //
        lwz     r.30, swFrame + ExGpr30(r.sp)   //
        mtlr    r.0                             // set return address
        lwz     r.31, swFrame + ExGpr31(r.sp)   //
        lwz     r.14, swFrame + ExGpr14(r.sp)   // restore gpr 14
        addi    r.sp, r.sp, sttFrameLength      // return stack frame

        blr

        DUMMY_EXIT(KiSwitchToThread)

        SBTTL("Dispatch Software Interrupt")
//++
//
// VOID
// KiDispatchSoftwareInterrupt (VOID)
//
// Routine Description:
//
//    This routine is called when the current irql drops below
//    DISPATCH_LEVEL and a software interrupt may be pending.  A
//    software interrupt is either a pending DPC or a pending APC.
//    If a DPC is pending, Irql is raised to DISPATCH_LEVEL and
//    KiDispatchInterrupt is called.  When KiDispatchInterrupt
//    returns, the Irql (before we raised it) is compared to
//    APC_LEVEL and if less and an APC interrupt is pending it
//    is processed.
//
//    Note: this routine manipulates PCR->CurrentIrql directly to
//    avoid recursion by using Ke[Raise|Lower]Irql.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        .text
        .align  5                               // cache block align

        LEAF_ENTRY_S(KiDispatchSoftwareInterrupt, _TEXT$00)

        li      r.3, 1                          // Flag to dispatch routines
                                                // to enable interrupts when
                                                // returning to IRQL 0
        DISABLE_INTERRUPTS(r.7, r.4)

        ALTERNATE_ENTRY(KiDispatchSoftwareIntDisabled)
        stw     r.3, -8(r.sp)                   // Flag to dispatch routines
                                                // indicating whether to enable
                                                // or not to enable interrupts
                                                // when returning to IRQL 0

        lhz     r.9, KiPcr+PcSoftwareInterrupt(r.0) // pending s/w interrupt?
        lbz     r.3, KiPcr+PcCurrentIrql(r.0)   // get current irql
        srwi.   r.4, r.9, 8                     // isolate DPC pending
        cmpw    cr.6, r.9, r.3                  // see if APC int and APC level
        cmpwi   cr.7, r.3, APC_LEVEL            // compare IRQL to APC LEVEL

//
// Possible values for SoftwareInterrupt (r.9) are
//
//   0x0101     DPC and APC interrupt pending
//   0x0100     DPC interrupt pending
//   0x0001     APC interrupt pending
//   0x0000     No software interrupt pending (unlikely but possible)
//
// Possible values for current IRQL are zero or one.  By comparing
// SoftwareInterrupt against current IQRL (above) we can quickly see
// if any software interrupts are valid at this time.
//
// Calculate correct IRQL for the interrupt we are processing.  If DPC
// then we need to be at DISPATCH_LEVEL which is one greater than APC_
// LEVEL.  r.4 contains one if we are going to run a DPC, so we add
// APC_LEVEL to r.4 to get the desired IRQL.
//

        addi    r.4, r.4, APC_LEVEL             // calculate new IRQL

        ble     cr.6,ExitEnabled                // return if no valid interrupt


#if DBG
        cmplwi  cr.6, r.3, DISPATCH_LEVEL       // sanity check, should only
        blt+    cr.6, $+12                      // below DISPATCH_LEVEL
        twi     31, 0, 0x16                     // BREAKPOINT
        blr                                     // return if wrong IRQL
#endif

//
// ..DispatchSoftwareInterrupt is an alternate entry used indirectly
// by KeReleaseSpinLock (via KxReleaseSpinLock).  KeReleaseSpinLock has
// been carefully written to construct the same conditions as apply if
// execution came from above.
//

..DispatchSoftwareInterrupt:

        stb     r.4, KiPcr+PcCurrentIrql(r.0)   // set IRQL
        ENABLE_INTERRUPTS(r.7)

        beq     cr.0, ..KiDispatchApc           // jif not DPC interrupt
        beq     cr.7, ..KiDispatchDpcOnly       // jif DPC and old IRQL APC LEV.
        b       ..KiDispatchDpc                 // DPC int, old IRQL < APC LEV.

ExitEnabled:
        ENABLE_INTERRUPTS(r.7)
        blr

        DUMMY_EXIT(KiDispatchSoftwareInterrupt)

        SBTTL("Unlock Dispatcher Database")
//++
//
// VOID
// KiUnlockDispatcherDatabase (
//    IN KIRQL OldIrql
//    )
//
// Routine Description:
//
//    This routine is entered at synchronization level with the dispatcher
//    database locked. Its function is to either unlock the dispatcher
//    database and return or initiate a context switch if another thread
//    has been selected for execution.
//
//    N.B. This code merges with the following swap context code.
//
//    N.B. A context switch CANNOT be initiated if the previous IRQL
//         is greater than or equal to DISPATCH_LEVEL.
//
//    N.B. This routine is carefully written to be a leaf function. If,
//        however, a context swap should be performed, the routine is
//        switched to a nested function.
//
// Arguments:
//
//    OldIrql (r.3) - Supplies the IRQL when the dispatcher database
//        lock was acquired.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiUnlockDispatcherDatabase, _TEXT$00)

//
// Check if a thread has been scheduled to execute on the current processor.
//

        cmpwi   cr.1, r.3, APC_LEVEL    // check if IRQL below dispatch level
        lwz     r.7, KiPcr+PcPrcb(r.0)  // get address of PRCB
        lhz     r.9, KiPcr+PcSoftwareInterrupt(r.0) // pending s/w interrupt?
        lwz     r.8, PbNextThread(r.7)  // get next thread address
        cmpw    cr.7, r.9, r.3          // compare pending against irql
        cmpwi   r.8, 0                  // check if next thread selected
        bne     uddSwitch               // jif new thread selected

//
// Not switching, release dispatcher database lock.
//

#if !defined(NT_UP)
        lwz     r.5, [toc]KiDispatcherLock(r.toc)
        li      r.6, 0
        RELEASE_SPIN_LOCK(r.5, r.6)
#endif

//
// If already at dispatch level, we're done.
//

        bgtlr   cr.1                    // return

//
// Dropping below DISPATCH_LEVEL, we may need to inspire a software
// interrupt if one is pending.
//
//
// Above we compared r.9 (SoftwareInterrupt) with r.3 (OldIrql) (result in
// cr.7).  See KiDispatchSoftwareInterrupt (above) for possible values.
// If we did not take the above branch then OldIrql is either zero or one.
// If SoftwareInterrupt is greater than OldIrql then a pending software
// interrupt can be taken at this time.
//

        bgt     cr.7, uddIntPending    // jif pending interrupt

        stb     r.3, KiPcr+PcCurrentIrql(r.0) // set new IRQL
        blr                             // return


//
// A new thread has been selected to run on the current processor, but
// the new IRQL is not below dispatch level. Release the dispatcher lock.
// If the current processor is not executing a DPC, then request a dispatch
// interrupt on the current.  IRQL is already at the right level.
//

uddCantSwitch:

#if !defined(NT_UP)
        lwz     r.5, [toc]KiDispatcherLock(r.toc)
        li      r.6, 0
        RELEASE_SPIN_LOCK(r.5, r.6)
#endif

        lwz     r.6, PbDpcRoutineActive(r.7)
        li      r.9, 1
        cmplwi  r.6, 0
        bnelr                           // return if DPC already active

        lwz     r.5, [toc]KiScheduleCount(r.toc)
        stb     r.9, KiPcr+PcDispatchInterrupt(r.0) // request dispatch interrupt
        lwz     r.6, 0(r.5)
        addi    r.6, r.6, 1             // bump schedule count
        stw     r.6, 0(r.5)

        blr                             // return

//
// A software interrupt is pending with higher priority than OldIrql.
//
// cr.1 is the result of comparing OldIrql with APC_LEVEL.
//

uddIntPending:

//
// Set flag to enable interrupts after dispatching software interrupts.
// r.9 must be non-zero because PcSoftwareInt(r.9) > OldIrql. Only
// needs to be set once no matter which dispatch routine is called.
//
        stw     r.9, -8(r.sp)

        beq     cr.1, ..KiDispatchDpcOnly  // new IRQL doesn't allow APCs

        srwi.   r.3, r.9, 8                // isolate DPC pending
        addi    r.3, r.3, APC_LEVEL        // calculate correct IRQL
        stb     r.3, KiPcr+PcCurrentIrql(r.0) // set new IRQL

        bne     ..KiDispatchDpc            // jif DPC at APC_LEVEL

//
// IRQL dropped from DISPATCH_LEVEL to APC_LEVEL, make sure no DPCs
// were queued while we were checking.  We are now at APC level so
// any new DPCs will happen without our having to check again.
//

        lbz     r.4, KiPcr+PcDispatchInterrupt(r.0)
        cmpwi   r.4, 0                     // new DPCs?
        beq     ..KiDispatchApc            // jif not

        li      r.3, DISPATCH_LEVEL        // re-raise to DISPATCH_LEVEL
        stb     r.3, KiPcr+PcCurrentIrql(r.0) // set new IRQL

        b       ..KiDispatchDpc

        DUMMY_EXIT(KiUnlockDispatcherDatabase)

//
// A new thread has been selected to run on the current processor.
//
// If the new IRQL is less than dispatch level, then switch to the new
// thread.
//

uddSwitch:
        bgt     cr.1, uddCantSwitch     // jif new IRQL > apc level

//
// N.B. This routine is carefully written as a nested function. Control
//      drops into this function from above.
//

        SPECIAL_ENTRY_S(KxUnlockDispatcherDatabase, _TEXT$00)

        mflr    r.0                             // get return address
        stwu    r.sp, -kscFrameLength(r.sp)     // buy stack frame
        stw     r.29, swFrame + ExGpr29(r.sp)   // save gpr 29
        stw     r.30, swFrame + ExGpr30(r.sp)   // save gpr 30
        ori     rPrcb, r.7, 0                   // copy PRCB address
        stw     r.31, swFrame + ExGpr31(r.sp)   // save gpr 31
        lwz     OTH, KiPcr+PcCurrentThread(r.0) // get current thread address
        stw     r.27, swFrame + ExGpr27(r.sp)   // save gpr 27
        ori     NTH, r.8, 0                     // thread to switch to
        stw     r.14, swFrame + ExGpr14(r.sp)   // save gpr 14
        li      r.11,0
        stw     r.28, swFrame + ExGpr28(r.sp)   // save gpr 28
        stw     r.26, swFrame + ExGpr26(r.sp)   // save gpr 26
        stw     r.0,  kscLR(r.sp)               // save return address

        PROLOGUE_END(KxUnlockDispatcherDatabase)

        stw     r.11, PbNextThread(rPrcb)       // clear next thread address
        stb     r.3, ThWaitIrql(OTH)            // save previous IRQL

//
// Reready current thread for execution and swap context to the selected thread.
//

        ori     r.3, OTH, 0                     // set address of previous thread object
        stw     NTH, PbCurrentThread(rPrcb)     // set address of current thread object

#if !defined(NT_UP)

        lwz     r.27,[toc]KiContextSwapLock(r.2)// get &KiContextSwapLock
        lwz     r.28,[toc]KiDispatcherLock(r.2) // get &KiDispatcherLock

#endif

        bl      ..KiReadyThread                 // reready thread for execution
        b       ksc130                          // join common code

        DUMMY_EXIT(KxUnlockDispatcherDatabase)

//++
//
// VOID
// KxReleaseSpinLock(VOID)
//
// Routine Description:
//
//    This routine is entered when a call to KeReleaseSpinLock lowers
//    IRQL to a level sufficiently low for a pending software interrupt
//    to be deliverable.
//
//    Although this routine has no arguments, the following entry conditions
//    apply.
//
//      Interrupts are disabled.
//
//      r.7     MSR prior to disabling interrupts.
//      r.4     IRQL to be raised to (PCR->CurrentIrql has been lowered
//              even though interrupts are currently disabled).
//
//      cr.0    ne if DPC pending
//      cr.7    eq if ONLY DPCs can run (ie PCR->CurrentIrql == APC_LEVEL)
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--
        .struct 0
        .space  StackFrameHeaderLength
sp31:   .space  4                       // r.31 save
        .align  3                       // ensure 8 byte alignment
spLength:

        SPECIAL_ENTRY_S(KxReleaseSpinLock, _TEXT$01)

        stw     r.31, sp31-spLength(r.sp)       // save r.31
        mflr    r.31                            // save return address (in r.31)
        stwu    r.sp, -spLength(r.sp)           // buy stack frame

        PROLOGUE_END(KxReleaseSpinLock)

        li      r.3, 1
        stw     r.3, -8(r.sp)                   // flag to dispatch routines
                                                // to enable interrupts when
                                                // returning to irql 0.
        bl      ..DispatchSoftwareInterrupt

        mtlr    r.31                            // set return address
        lwz     r.31, sp31(r.sp)                // restore r.31
        addi    r.sp, r.sp, spLength            // release stack frame

        SPECIAL_EXIT(KxReleaseSpinLock)


//++
//
// VOID
// KiDispatchDpcOnly (VOID)
//
// Routine Description:
//
//    This routine is entered as a result of lowering IRQL to
//    APC_LEVEL with a DPC interrupt pending.  IRQL is currently
//    at DISPATCH_LEVEL, the dispatcher database is unlocked.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--
        .set kdsiFrameLength, STK_MIN_FRAME+8

        NESTED_ENTRY_S(KiDispatchDpcOnly, kdsiFrameLength, 0, 0, _TEXT$00)
        PROLOGUE_END(KiDispatchDpcOnly)

kddo10: bl      ..KiDispatchInterrupt

        li      r.3, APC_LEVEL                  // get new IRQL

        DISABLE_INTERRUPTS(r.8, r.4)

        lbz     r.4, KiPcr+PcDispatchInterrupt(r.0) // more DPCs pending?
        lwz     r.5, KiPcr+PcPrcb(r.0)          // get address of PRCB
        cmpwi   r.4, 0
        lwz     r.6, PbInterruptCount(r.5)      // bump interrupt count
        addi    r.4, r.4, APC_LEVEL             // calc new IRQL
        addi    r.6, r.6, 1
        stw     r.6, PbInterruptCount(r.5)
        lwz     r.6, STK_MIN_FRAME(r.sp)        // parameter to enable
        stb     r.4, KiPcr+PcCurrentIrql(r.0)   // set new IRQL
        beq+    kddo20

        ENABLE_INTERRUPTS(r.8)

        b       kddo10                          // jif more DPCs to run

kddo20: cmpwi   r.6, 0                          // o.k to enable interrupts?
        beq     kddo25                          // return if not
        ENABLE_INTERRUPTS(r.8)                  // reenable interrupts and exit

kddo25:
        NESTED_EXIT(KiDispatchDpcOnly, kdsiFrameLength, 0, 0)

//++
//
// VOID
// KiDispatchDpc (VOID)
//
// Routine Description:
//
//    This routine is entered as a result of lowering IRQL below
//    APC_LEVEL with a DPC interrupt pending.  IRQL is currently
//    at DISPATCH_LEVEL, the dispatcher database is unlocked.
//
//    Once DPC processing is complete, APC processing may be required.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY_S(KiDispatchDpc, kdsiFrameLength, 0, 0, _TEXT$00)
        PROLOGUE_END(KiDispatchDpc)

kdd10:  bl      ..KiDispatchInterrupt

        DISABLE_INTERRUPTS(r.8, r.4)

        lwz     r.5, KiPcr+PcPrcb(r.0)          // get address of PRCB
        lhz     r.4, KiPcr+PcSoftwareInterrupt(r.0) // more DPCs or APCs pending?
        lwz     r.6, PbInterruptCount(r.5)      // bump interrupt count
        cmpwi   r.4, APC_LEVEL
        addi    r.6, r.6, 1
        stw     r.6, PbInterruptCount(r.5)
        bgt-    kdd20                           // jif more DPCs
        lwz     r.6, STK_MIN_FRAME(r.sp)        // parameter to enable
                                                // interrupts
        stb     r.4, KiPcr+PcCurrentIrql(r.0)   // set new IRQL
        blt     kdd30                           // honor parameter to exit
                                                // enabled or disabled when
                                                // returning to IRQL 0

        ENABLE_INTERRUPTS(r.8)                  // equal

        b       kda10                           // jif APCs to run

kdd20:  ENABLE_INTERRUPTS(r.8)                  // greater than

        b       kdd10                           // jif more DPCs to run

kdd30:  cmpwi   r.6, 0                          // o.k to enable interrupts?
        beq-    kdd35                           // return if not
        ENABLE_INTERRUPTS(r.8)                  // reenable interrupts and exit

kdd35:
        NESTED_EXIT(KiDispatchDpc, kdsiFrameLength, 0, 0)

//++
//
// VOID
// KiDispatchApc (VOID)
//
// Routine Description:
//
//    This routine is entered as a result of lowering IRQL below
//    APC_LEVEL with an APC interrupt pending.  IRQL is currently
//    at APC_LEVEL.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY_S(KiDispatchApc, kdsiFrameLength, 0, 0, _TEXT$00)
        PROLOGUE_END(KiDispatchApc)

kda10:
        lwz     r.6, KiPcr+PcPrcb(r.0)          // get address of PRCB
        li      r.3, 0                          // PreviousMode = Kernel
        lwz     r.7, PbApcBypassCount(r.6)      // get APC bypass count
        li      r.4, 0                          // TrapFrame = 0
        li      r.5, 0                          // ExceptionFrame = 0
        addi    r.7, r.7, 1                     // increment APC bypass count
        stb     r.3, KiPcr+PcApcInterrupt(r.0)  // clear APC pending
        stw     r.7, PbApcBypassCount(r.6)      // store new APC bypass count
        bl      ..KiDeliverApc

        li      r.3, 0                          // get new IRQL

#if !defined(NT_UP)

        DISABLE_INTERRUPTS(r.8, r.4)

#else

        stb     r.3, KiPcr+PcCurrentIrql(r.0)   // lower IRQL
        sync

#endif

        lwz     r.5, KiPcr+PcPrcb(r.0)          // get address of PRCB
        lbz     r.4, KiPcr+PcApcInterrupt(r.0)  // more APCs pending?
        lwz     r.6, PbInterruptCount(r.5)      // bump interrupt count
        cmpwi   r.4, 0
        addi    r.6, r.6, 1
        stw     r.6, PbInterruptCount(r.5)
        stb     r.4, KiPcr+PcCurrentIrql(r.0)   // Raise IRQL if more APCs

#if !defined(NT_UP)

        ENABLE_INTERRUPTS(r.8)

#endif

        bne-    kda10                           // jif more APCs to run

        NESTED_EXIT(KiDispatchApc, kdsiFrameLength, 0, 0)

        SBTTL("Swap Thread")
//++
//
// VOID
// KiSwapThread (
//    VOID
//    )
//
// Routine Description:
//
//    This routine is called to select the next thread to run on the
//    current processor and to perform a context switch to the thread.
//
// Arguments:
//
//    None.
//
// Outputs:     ( for call to SwapContext )
//
//    r.31  NTH    pointer to new thread
//    r.30  OTH    pointer to old thread
//    r.29  rPrcb  pointer to processor control block
//    r.28         pointer to dispatcher database lock
//    r.27         pointer to context swap lock
//
// Return Value:
//
//    Wait completion status (r.3).
//
//--

        .struct 0
        .space  swFrameLength
kscLR:  .space  4
        .align  3               // ensure 8 byte alignment
kscFrameLength:

        .align  6                               // cache line align

        SPECIAL_ENTRY_S(KiSwapThread,_TEXT$00)

        mflr    r.0                             // get return address
        stwu    r.sp, -kscFrameLength(r.sp)     // buy stack frame
        stw     r.29, swFrame + ExGpr29(r.sp)   // save gpr 29
        stw     r.30, swFrame + ExGpr30(r.sp)   // save gpr 30
        stw     r.31, swFrame + ExGpr31(r.sp)   // save gpr 31
        lwz     OTH, KiPcr+PcCurrentThread(r.0) // get current thread addr
        lwz     rPrcb, KiPcr+PcPrcb(r.0)        // get Processor Control Block
        stw     r.27, swFrame + ExGpr27(r.sp)   // save gpr 27
        stw     r.28, swFrame + ExGpr28(r.sp)   // save gpr 28
        stw     r.14, swFrame + ExGpr14(r.sp)   // save gpr 14
        lwz     r.27, [toc]KiReadySummary(r.toc) // get &KiReadySummary
        lwz     NTH, PbNextThread(rPrcb)        // get address of next thread
        stw     r.26, swFrame + ExGpr26(r.sp)   // save gpr 26
        li      r.28, 0                         // load a 0
        lwz     r.26, 0(r.27)                   // get ready summary
        cmpwi   NTH, 0                          // next thread selected?
        stw     r.0,  kscLR(r.sp)               // save return address

        PROLOGUE_END(KiSwapThread)

        stw     r.28, PbNextThread(rPrcb)       // zero address of next thread
        bne     ksc120                          // if ne, next thread selected

#if !defined(NT_UP)

        lwz     r.5, [toc]KeTickCount(r.toc)    // get &KeTickCount
        lwz     r.3, KiPcr+PcSetMember(r.0)     // get processor affinity mask
        lbz     r.4, PbNumber(rPrcb)            // get current processor number
        lwz     r.5, 0(r.5)                     // get low part of tick count

#endif

//
// Find the highest priority ready thread.
//

        cntlzw  r.6, r.26                       // count zero bits from left
        lwz     r.8, [toc]KiDispatcherReadyListHead(r.toc) // get ready listhead base address
        slw.    r.7, r.26, r.6                  // shift first set bit into sign bit
        subfic  r.6, r.6, 31                    // convert shift count to priority

        beq     kscIdle                         // if mask is zero, no ready threads

kscReadyScan:

//
// If the thread can execute on the current processor, then remove it from
// the dispatcher ready queue.
//

        slwi    r.9, r.6, 3                     // compute ready listhead offset
        slwi    r.7, r.7, 1                     // position next ready summary bit
        add     r.9, r.9, r.8                   // compute ready queue address
        lwz     r.10, LsFlink(r.9)              // get address of first entry
        subi    NTH, r.10, ThWaitListEntry      // compute address of thread object

#if !defined(NT_UP)

kscAffinityScan:

        lwz     r.11, ThAffinity(NTH)           // get thread affinity
        lbz     r.0, ThNextProcessor(NTH)       // get last processor number
        and.    r.11, r.11, r.3                 // check for compatible thread affinity
        cmpw    cr.6, r.0, r.4                  // compare last processor with current
        bne     kscAffinityOk                   // if ne, thread affinity compatible

        lwz     r.10, LsFlink(r.10)             // get address of next entry
        cmpw    r.10, r.9                       // compare with queue address
        subi    NTH, r.10, ThWaitListEntry      // compute address of thread object
        bne     kscAffinityScan                 // if ne, not end of list
ksc70:
        cmpwi   r.7, 0                          // more ready queues to scan?
        subi    r.6, r.6, 1                     // decrement ready queue priority
        blt     kscReadyScan                    // if lt, queue contains an entry
        beq     kscIdle                         // if eq, no ready threads

        slwi    r.7, r.7, 1                     // position next ready summary bit
        b       ksc70                           // check next bit

kscAffinityOk:

//
// If the thread last ran on the current processor, has been waiting for
// longer than a quantum, or its priority is greater than low realtime
// plus 9, then select the thread. Otherwise, an attempt is made to find
// a more appropriate candidate.
//

        beq     cr.6, kscReadyFound             // if eq, processor number match
        lbz     r.0, ThIdealProcessor(NTH)      // get ideal processor number
        cmpwi   r.6, LOW_REALTIME_PRIORITY + 9  // check if priority in range
        cmpw    cr.6, r.0, r.4                  // compare ideal processor with current
        beq     cr.6, ksc100                    // if eq, processor number match
        bge     ksc100                          // if ge, priority high enough
        lwz     r.12, ThWaitTime(NTH)           // get time of thread ready
        sub     r.12, r.5, r.12                 // compute length of wait
        cmpwi   cr.7, r.12, READY_SKIP_QUANTUM + 1 // check if wait time exceeded
        bge     cr.7, ksc100                    // if ge, waited long enough

//
// Search forward in the ready queue until the end of the list is reached
// or a more appropriate thread is found.
//

        lwz     r.14, LsFlink(r.10)             // get address of next entry
ksc80:
        cmpw    r.14, r.9                       // compare with queue address
        subi    r.28, r.14, ThWaitListEntry     // compute address of thread object
        beq     ksc100                          // if eq, end of list

        lwz     r.11, ThAffinity(r.28)          // get thread affinity
        lbz     r.0, ThNextProcessor(r.28)      // get last processor number
        and.    r.11, r.11, r.3                 // check for compatible thread affinity
        cmpw    cr.6, r.0, r.4                  // compare last processor with current
        beq     ksc85                           // if eq, thread affinity not compatible
        beq     cr.6, ksc90                     // if eq, processor number match
        lbz     r.0, ThIdealProcessor(NTH)      // get ideal processor number
        cmpw    cr.6, r.0, r.4                  // compare ideal processor with current
        beq     cr.6, ksc90                     // if eq, processor number match
ksc85:
        lwz     r.12, ThWaitTime(r.28)          // get time of thread ready
        lwz     r.14, LsFlink(r.14)             // get address of next entry
        sub     r.12, r.5, r.12                 // compute length of wait
        cmpwi   cr.7, r.12, READY_SKIP_QUANTUM + 1 // check if wait time exceeded
        blt     cr.7, ksc80                     // if lt, wait time not exceeded
        b       ksc100                          // wait time exceeded -- switch to
                                                //   first matching thread in ready queue
ksc90:
        ori     NTH, r.28, 0                    // set thread address
        ori     r.10, r.14, 0                   // set list entry address
ksc100:
        stb     r.4, ThNextProcessor(NTH)       // set next processor number

kscReadyFound:

#endif

//
// Remove the selected thread from the ready queue.
//

        lwz     r.11, LsFlink(r.10)             // get list entry forward link
        lwz     r.12, LsBlink(r.10)             // get list entry backward link
        li      r.0, 1                          // set bit for mask generation
        slw     r.0, r.0, r.6                   // compute ready summary set member
        cmpw    r.11, r.12                      // check for list empty
        stw     r.11, LsFlink(r.12)             // set forward link in previous entry
        stw     r.12, LsBlink(r.11)             // set backward link in next entry
        bne     ksc120                          // if ne, list is not empty
        xor     r.26, r.26, r.0                 // clear ready summary bit
        stw     r.26, 0(r.27)                   // update ready summary
ksc120:

//
// Swap context to the next thread.
//


#if !defined(NT_UP)

        lwz     r.27,[toc]KiContextSwapLock(r.2)// get &KiContextSwapLock
        lwz     r.28,[toc]KiDispatcherLock(r.2) // get &KiDispatcherLock

#endif

        stw     NTH, PbCurrentThread(rPrcb)     // set new thread current
ksc130:
        bl      ..SwapContext                   // swap context


//
// Lower IRQL and return wait completion status.
//
// N.B. SwapContext releases the dispatcher database lock.
//
        lbz     r.4, ThWaitIrql(NTH)            // get original wait IRQL

        DISABLE_INTERRUPTS(r.6, r.7)

        lhz     r.5, KiPcr+PcSoftwareInterrupt(r.0) // check for pending s/w ints
        lwz     r.14, kscLR(r.sp)               // get return address
        lwz     r.31, ThWaitStatus(NTH)         // get wait completion status
        lwz     r.26, swFrame + ExGpr26(r.sp)   // restore gprs 26 thru 30
        lwz     r.27, swFrame + ExGpr27(r.sp)   //
        lwz     r.28, swFrame + ExGpr28(r.sp)   //
        lwz     r.29, swFrame + ExGpr29(r.sp)   //
        lwz     r.30, swFrame + ExGpr30(r.sp)   //
        stb     r.4, KiPcr+PcCurrentIrql(r.0)   // set new IRQL

        ENABLE_INTERRUPTS(r.6)

        cmpw    r.5, r.4                        // see if s/w int could now run
        bgtl-   ..KiDispatchSoftwareInterrupt   // jif pending int can run
        mtlr    r.14                            // set return address
        ori     r.3,  r.31, 0                   // set return status
        lwz     r.31, swFrame + ExGpr31(r.sp)   // restore r.31
        lwz     r.14, swFrame + ExGpr14(r.sp)   // restore r.14
        addi    r.sp, r.sp, kscFrameLength      // return stack frame

        SPECIAL_EXIT(KiSwapThread)

kscIdle:

//
// All ready queues were scanned without finding a runnable thread so
// default to the idle thread and set the appropriate bit in idle summary.
//

        lwz     r.5, [toc]KiIdleSummary(r.toc) // get &KiIdleSummary

#if defined(NT_UP)

        li      r.4, 1                  // set current idle summary
#else

        lwz     r.4, 0(r.5)             // get current idle summary
        or      r.4, r.4, r.3           // set member bit in idle summary

#endif

        stw     r.4, 0(r.5)             // set new idle summary

        lwz     NTH,PbIdleThread(rPrcb) // set address of idle thread
        b       ksc120                  // switch to idle thread


        SBTTL("Swap Context to Next Thread")
//++
//
// Routine Description:
//
//    This routine is called to swap context from one thread to the next.
//
// Arguments:
//
//    r.sp         Pointer to { StackFrameHeader, ExceptionFrame }
//    r.31  NTH    Address of next thread object
//    r.30  OTH    Address of previous thread object
//    r.29  rPrcb  Address of processor control block
//    r.28         Address of KiDispatcherLock
//    r.27         Address of KiContextSwapLock
//
// Return value:
//
//    r.31  NTH    Address of current thread object.
//    r.29  rPrcb  Address of processor control block
//
// Note that the TOC register is neither saved nor restored across a
// thread switch.  This is because we are in NTOSKRNL (actually in the
// routine SwapContext) in both threads (ie the TOC does not change).
//
// GPR 13 is set to the address of the TEB where it will remain.
//
//--

        SPECIAL_ENTRY_S(SwapContext,_TEXT$00)

        mfcr    r.3                             // get condition register

//
// Acquire the context swap lock so the address space of the old process
// cannot be deleted and then release the dispatcher database lock.
//
// N.B. This lock is used to protect the address space until the context
//    switch has sufficiently progressed to the point where the address
//    space is no longer needed. This lock is also acquired by the reaper
//    thread before it finishes thread termination.
//

#if !defined(NT_UP)
        b        LkCtxSw                 // skip spin code

        SPIN_ON_SPIN_LOCK(r.27,r.4,LkCtxSw,LkCtxSwSpin) // spin on context swap lock

        ACQUIRE_SPIN_LOCK(r.27,r.31,r.4,LkCtxSw,LkCtxSwSpin) // acquire context swap lock
#endif

//
// Set the new thread's state to Running before releasing the dispatcher lock.
//

        li      r.8, Running                    // set state of new thread
        stb     r.8, ThState(NTH)               // to running.

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.28,r.4)      // release dispatcher lock
#endif

//
// Save old thread non-volatile context.
//

        mflr    r.0                             // get return address

        stw     r.15, swFrame + ExGpr15(r.sp)   // save gprs 15 thru 25
        stw     r.16, swFrame + ExGpr16(r.sp)   //
        stw     r.17, swFrame + ExGpr17(r.sp)   //
        stw     r.18, swFrame + ExGpr18(r.sp)   //
        stw     r.19, swFrame + ExGpr19(r.sp)   //
        stw     r.20, swFrame + ExGpr20(r.sp)   //
        stw     r.21, swFrame + ExGpr21(r.sp)   //
        stw     r.22, swFrame + ExGpr22(r.sp)   //
        stw     r.23, swFrame + ExGpr23(r.sp)   //
        stw     r.24, swFrame + ExGpr24(r.sp)   //
        stw     r.25, swFrame + ExGpr25(r.sp)   //

        stfd    f.14, swFrame + ExFpr14(r.sp)   // save non-volatile
        stfd    f.15, swFrame + ExFpr15(r.sp)   // floating point regs
        stfd    f.16, swFrame + ExFpr16(r.sp)   //
        stfd    f.17, swFrame + ExFpr17(r.sp)   //
        stfd    f.18, swFrame + ExFpr18(r.sp)   //
        stfd    f.19, swFrame + ExFpr19(r.sp)   //
        stfd    f.20, swFrame + ExFpr20(r.sp)   //
        stfd    f.21, swFrame + ExFpr21(r.sp)   //
        stfd    f.22, swFrame + ExFpr22(r.sp)   //
        stfd    f.23, swFrame + ExFpr23(r.sp)   //
        stfd    f.24, swFrame + ExFpr24(r.sp)   //
        stfd    f.25, swFrame + ExFpr25(r.sp)   //
        stfd    f.26, swFrame + ExFpr26(r.sp)   //
        stfd    f.27, swFrame + ExFpr27(r.sp)   //
        stfd    f.28, swFrame + ExFpr28(r.sp)   //
        stfd    f.29, swFrame + ExFpr29(r.sp)   //
        stfd    f.30, swFrame + ExFpr30(r.sp)   //
        stfd    f.31, swFrame + ExFpr31(r.sp)   //

        stw     r.3, swFrame + SwConditionRegister(r.sp)// save CR
        stw     r.0, swFrame + SwSwapReturn(r.sp)       // save return address

        PROLOGUE_END(SwapContext)

//
// The following entry point is used to switch from the idle thread to
// another thread.
//

..SwapFromIdle:

#if DBG
        cmpw    NTH, OTH
        bne     th_ok
        twi     31, 0, 0x16
th_ok:
#endif

        stw     NTH, KiPcr+PcCurrentThread(r.0) // set new thread current

//
// Get the old and new process object addresses.
//

#define NPROC   r.3
#define OPROC   r.4

        lwz     NPROC, ThApcState + AsProcess(NTH) // get new process object
        lwz     OPROC, ThApcState + AsProcess(OTH) // get old process object

        DISABLE_INTERRUPTS(r.15, r.6)

        lwz     r.13, ThTeb(NTH)                // get addr of user TEB
        lwz     r.24, ThStackLimit(NTH)         // get stack limit
        lwz     r.23, ThInitialStack(NTH)       // get initial kernel stk ptr
        lwz     r.22, ThKernelStack(NTH)        // get new thread stk ptr
        cmpw    cr.0, NPROC, OPROC              // same process ?

        stw     r.sp, ThKernelStack(OTH)        // save current kernel stk ptr
        stw     r.13, KiPcr+PcTeb(r.0)          // set addr of user TEB
//
// Although interrupts are disabled, someone may be attempting to
// single step thru the following.  I can't see anyway to perform
// the two operations atomically so I am inserting a label that is
// known externally and can be checked against the exception address
// if we fail stack validation in common_exception_entry (in real0.s)
// in which case it's really ok.  This has no performance impact.
//
// *** WARNING ****** WARNING ****** WARNING ****** WARNING ***
//
//      (1) these two instructions MUST stay together,
//      (2) the stack validation code in common_exception_entry
//          KNOWS that the second instruction is a 'ori r.sp, r.22, 0'
//          and will perform such an instruction in line to correct
//          the problem.  If you change this sequence you will need
//          to make an equivalent change in real0.s and the correct-
//          ability is dependent on the second instruction destroying
//          the stack pointer.
// (plj).
//

        stw     r.24, KiPcr+PcStackLimit(r.0)   // set stack limit
        stw     r.23, KiPcr+PcInitialStack(r.0) // set initial kernel stack ptr
        .globl  KepSwappingContext
KepSwappingContext:
        ori     r.sp, r.22, 0                   // switch stacks

#if !defined(NT_UP)

//
// Old process address space is no longer required.  Ensure all
// stores are done prior to releasing the ContextSwap lock.
// N.B. SwapContextLock is still needed to ensure KiMasterPid
// integrity.
//

        li      r.16, 0

        eieio
        bne     cr.0, ksc10
        stw     r.16, 0(r.27)                   // release Context Swap lock
        b       ksc20

ksc10:

#else

        beq     cr.0, ksc20

#endif

//
// If the process sequence number matches the system sequence number, then
// use the process PID. Otherwise, allocate a new process PID.
//
// N.B. The following code is duplicated from KiSwapProcess and will
//      join KiSwapProcess at SwapProcessSlow if sequence numbers
//      don't match.  Register usage from here to the branch should
//      match KiSwapProcess.
//
        lwz     r.10,[toc]KiMasterSequence(r.toc) // get &KiMasterSequence
        lwz     r.9,PrProcessSequence(NPROC) // get process sequence number
        lwz     r.11,0(r.10)            // get master sequence number
        lwz     r.7,PrProcessPid(NPROC) // get process PID
        cmpw    r.11,r.9                // master sequence == process sequence?
        bne     ksc15                   // jif not equal, go the slow path

#if !defined(NT_UP)

        stw     r.16, 0(r.27)           // release Context Swap lock

#endif

//
// Swap address space to the specified process.
//

        lwz     r.5,PrDirectoryTableBase(r.3) // get page dir page real addr

        mtsr    0,r.7                   // set sreg 0
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    1,r.7                   // set sreg 1
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    2,r.7                   // set sreg 2
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    3,r.7                   // set sreg 3
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    4,r.7                   // set sreg 4
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    5,r.7                   // set sreg 5
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    6,r.7                   // set sreg 6
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    7,r.7                   // set sreg 7
        addi    r.7,r.7,12-7            // add 5 to VSID
        mtsr    12,r.7                  // set sreg 12
        isync                           // context synchronize
        stw     r.5,KiPcr+PcPgDirRa(r.0) // store page dir page ra in PCR

#if COLLECT_PAGING_DATA
        lwz     r.10,[toc]KiFlushOnProcessSwap(r.toc)
        lwz     r.10,0(r.10)
        cmpwi   r.10,0
        bnel    ..KeFlushCurrentTb
#endif

        b       ksc20

ksc15:
        bl      SwapProcessSlow

ksc20:
        lbz     r.5, ThApcState + AsKernelApcPending(NTH)
        lbz     r.16, ThDebugActive(NTH)        // get the active debug register
                                                // mask
        stb     r.5, KiPcr+PcApcInterrupt(r.0)  // set APC pending appropriately
        stb     r.16, KiPcr+PcDebugActive(r.0)  // set the active debug register
                                                // mask for the new thread
        lwz     r.5, PbContextSwitches(rPrcb)   // get context switch count
        lwz     r.7, ThContextSwitches(NTH)
        addi    r.5, r.5, 1                     // bump context switch count
        stw     r.5, PbContextSwitches(rPrcb)   // for processor.
        addi    r.7, r.7, 1                     // bump context switch count
        stw     r.7, ThContextSwitches(NTH)     // for this thread

        ENABLE_INTERRUPTS(r.15)

        lwz     r.0, swFrame + SwSwapReturn(r.sp)       // get return address
        lwz     r.5, swFrame + SwConditionRegister(r.sp)// get CR

        lwz     r.15, swFrame + ExGpr15(r.sp)   // restore gprs 15 thru 25
        lwz     r.16, swFrame + ExGpr16(r.sp)   //
        lwz     r.17, swFrame + ExGpr17(r.sp)   //
        lwz     r.18, swFrame + ExGpr18(r.sp)   //
        lwz     r.19, swFrame + ExGpr19(r.sp)   //
        lwz     r.20, swFrame + ExGpr20(r.sp)   //
        lwz     r.21, swFrame + ExGpr21(r.sp)   //
        lwz     r.22, swFrame + ExGpr22(r.sp)   //
        lwz     r.23, swFrame + ExGpr23(r.sp)   //
        lwz     r.24, swFrame + ExGpr24(r.sp)   //
        lwz     r.25, swFrame + ExGpr25(r.sp)   //

        lfd     f.14, swFrame + ExFpr14(r.sp)   // restore non-volatile
        lfd     f.15, swFrame + ExFpr15(r.sp)   // floating point regs
        lfd     f.16, swFrame + ExFpr16(r.sp)   //
        lfd     f.17, swFrame + ExFpr17(r.sp)   //
        lfd     f.18, swFrame + ExFpr18(r.sp)   //
        lfd     f.19, swFrame + ExFpr19(r.sp)   //
        lfd     f.20, swFrame + ExFpr20(r.sp)   //
        lfd     f.21, swFrame + ExFpr21(r.sp)   //
        lfd     f.22, swFrame + ExFpr22(r.sp)   //
        lfd     f.23, swFrame + ExFpr23(r.sp)   //
        lfd     f.24, swFrame + ExFpr24(r.sp)   //
        lfd     f.25, swFrame + ExFpr25(r.sp)   //
        lfd     f.26, swFrame + ExFpr26(r.sp)   //
        lfd     f.27, swFrame + ExFpr27(r.sp)   //
        mtlr    r.0                             // set return address
        mtcrf   0xff, r.5                       // set condition register
        lfd     f.28, swFrame + ExFpr28(r.sp)   //
        lfd     f.29, swFrame + ExFpr29(r.sp)   //
        lfd     f.30, swFrame + ExFpr30(r.sp)   //
        lfd     f.31, swFrame + ExFpr31(r.sp)   //

        SPECIAL_EXIT(SwapContext)

#undef  NTH
#undef  OTH

//++
//
// VOID
// KiSwapProcess (
//    IN PKPROCESS NewProcess,
//    IN PKPROCESS OldProcess
//    )
//
// Routine Description:
//
//    This function swaps the address space from one process to another by
//    moving to the PCR the real address of the process page directory page
//    and loading segment registers 0-7 and 12 with VSIDs derived therefrom.
//
//    The fast path below is duplicated inline in SwapContext for speed.
//    SwapContext joins this code at SwapProcessSlow if sequence numbers
//    differ.
//
// Arguments:
//
//    NewProcess (r3) - Supplies a pointer to a control object of type process
//      which represents the new process that is switched to.
//
//    OldProcess (r4) - Supplies a pointer to a control object of type process
//      which represents the old process that is switched from.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KiSwapProcess,_TEXT$00)

//
// Get the Context Swap lock.  This lock is used to protect a
// processes memory space, it serves double duty to protect access
// to KiMasterSequence.
//
// N.B. It is already held if entry is via SwapProcessSlow, the
// lock is ALWAYS released by this routine.
//

#if !defined(NT_UP)

        lwz     r.6, [toc]KiContextSwapLock(r.2)

        ACQUIRE_SPIN_LOCK(r.6,r.3,r.5,LkCtxSw2,LkCtxSw2Spin)

#endif

//
// If the process sequence number matches the system sequence number, then
// use the process PID. Otherwise, allocate a new process PID.
//
// WARNING: if you change register usage in the following be sure to make
//          the same changes in SwapContext.
//

        lwz     r.10,[toc]KiMasterSequence(r.toc) // get &KiMasterSequence
        lwz     r.9,PrProcessSequence(r.3) // get process sequence number
        lwz     r.11,0(r.10)            // get master sequence number
        lwz     r.7,PrProcessPid(r.3)   // get process PID
        cmpw    r.11,r.9                // master sequence == process sequence?
        bne     SwapProcessSlow         // jif not equal, out of line


//
// Swap address space to the specified process.
//

spup:   lwz     r.5,PrDirectoryTableBase(r.3) // get page dir page real addr

        DISABLE_INTERRUPTS(r.8,r.0)     // disable interrupts

#if !defined(NT_UP)

        sync
        li      r.10, 0
        stw     r.10, 0(r.6)            // release KiContextSwapLock

#endif

        stw     r.5,KiPcr+PcPgDirRa(r.0) // store page dir page ra in PCR
        mtsr    0,r.7                   // set sreg 0
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    1,r.7                   // set sreg 1
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    2,r.7                   // set sreg 2
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    3,r.7                   // set sreg 3
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    4,r.7                   // set sreg 4
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    5,r.7                   // set sreg 5
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    6,r.7                   // set sreg 6
        addi    r.7,r.7,1               // add 1 to VSID
        mtsr    7,r.7                   // set sreg 7
        addi    r.7,r.7,12-7            // add 5 to VSID
        mtsr    12,r.7                  // set sreg 12
        isync                           // context synchronize

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

#if COLLECT_PAGING_DATA
        lwz     r.10,[toc]KiFlushOnProcessSwap(r.toc)
        lwz     r.10,0(r.10)
        cmpwi   r.10,0
        bne     ..KeFlushCurrentTb
#endif

        ALTERNATE_EXIT(KiSwapProcess)   // return

//
// We need a new PID, the dispatcher database lock is still held so
// we can update KiMasterPid without further protection.
//

SwapProcessSlow:
        lwz     r.8,[toc]KiMasterPid(r.toc) // get &KiMasterPid
        lwz     r.7,0(r.8)              // get KiMasterPid
        addi    r.7,r.7,16              // bump master pid
        rlwinm. r.7,r.7,0,0x007ffff0    // detect PID wrap
        beq     ..KxSwapProcess         // jif PID wrap

        stw     r.11,PrProcessSequence(r.3) // save new process sequence
//
// control returns here from KxSwapProcess
//

spnp:

#if !defined(NT_UP)

        lwz     r.6, [toc]KiContextSwapLock(r.2)

#endif

        stw     r.7,0(r.8)              // save new master PID
        stw     r.7,PrProcessPid(r.3)   // save new process PID
        b       spup                    // continue with main line code

#if !defined(NT_UP)

       SPIN_ON_SPIN_LOCK(r.6,r.5,LkCtxSw2,LkCtxSw2Spin)

#endif

        DUMMY_EXIT(KiSwapProcess)

//++
//
// VOID
// KxSwapProcess (
//    IN PKPROCESS NewProcess,
//    )
//
// Routine Description:
//
//    This function is called (only) from KiSwapProcess when PID wrap has
//    occured.  KiSwapProcess is a LEAF function.  The purpose of this
//    function is to alloacte a stack frame and save data that needs to
//    be restored for KiSwapProcess.  This routine is called aproximately
//    once every 16 million new processes.  The emphasis in KiSwapProcess
//    is to handle the other 16 million - 1 cases as fast as possible.
//
// Arguments:
//
//    NewProcess        (r3)  - Supplies a pointer to a control object of
//      type process which represents the new process being switched to.
//      This must be saved and restored for KiSwapProcess.
//
//    &KiMasterPid      (r8)  - Address of system global KiMasterPid
//      This must be restored for KiSwapProcess.
//
//    &KiMasterSequence (r10) - Address of system global KiMasterSequence.
//
//    KiMasterSequence  (r11) - Current Value of the above variable.
//
// Return Value:
//
//    None.
//
//    Registers r3, r8 and the Link Register are restored.  r7 contains
//    the new PID which will be 16.
//
//--

        .struct 0
        .space  StackFrameHeaderLength
spLR:   .space  4                       // link register save
spR3:   .space  4                       // new process address save
        .align  3                       // ensure correct alignment
spFrameLength:

        SPECIAL_ENTRY_S(KxSwapProcess,_TEXT$00)

        mflr    r.0                     // get link register
        stwu    r.sp,-spFrameLength(r.sp) // buy stack frame
        stw     r.3,spR3(r.sp)          // save new process address
        stw     r.0,spLR(r.sp)          // save swap process' return address

        PROLOGUE_END(KxSwapProcess)

//
// PID wrap has occured.  On PowerPC we do not need to lock the process
// id wrap lock because tlb synchronization is handled by hardware.
//

        addic.  r.11,r.11,1             // bump master sequence number
        bne+    spnsw                   // jif sequence number did not wrap

//
// The master sequence number has wrapped, this is 4 billion * 16 million
// processes,... not too shabby.  We start the sequence again at 2 in case
// there are system processes that have been running since the system first
// started.
//

        li      r.11,2                  // start again at 2

spnsw:  stw     r.11,0(r.10)            // save new master sequence number
        stw     r.11,PrProcessSequence(r.3) // save new process sequence num

        bl      ..KeFlushCurrentTb      // flush entire HPT (and all processor
                                        // TLBs)

        lwz     r.0,spLR(r.sp)          // get swap process' return address
        lwz     r.3,spR3(r.sp)          // get new process address
        lwz     r.8,[toc]KiMasterPid(r.toc) // get &KiMasterPid
        addi    r.sp,r.sp,spFrameLength // return stack frame
        li      r.7,16                  // set new PID
        mtlr    r.0
        b       spnp                    // continue in KiSwapProcess

        DUMMY_EXIT(KxSwapProcess)

//++
//
// VOID
// KiIdleLoop (
//    VOID
//    )
//
// Routine Description:
//
//    This is the idle loop for NT.  This code runs in a thread for
//    each processor in the system.  The idle thread runs at IRQL
//    DISPATCH_LEVEL and polls for work.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.  (This routine never returns).
//
//  Non-volatile register usage is as follows.
//
//      r.14    --unused - available --
//      r.15    Address of KdDebuggerEnabled
//      r.16    Kernel TOC (backup)
//      r.17    Idle loop MSR with Interrupts DISABLED
//      r.18    Idle loop MSR with Interrupts ENABLED
//      r.19    HalProcessorIdle entry point
//      r.20    HAL's TOC
//      r.21    Debugger poll count
//      r.22    Address of KeTickCount
//      r.23    Zero
//      r.24    Address of dispatcher database lock (MP) (backup for r28)
//      r.25    DpcListHead
//      r.26    --unused - available --
//      r.27    Address of Context Swap lock
//      r.28    Address of dispatcher database lock (MP)
//      r.29    Address of Processor Control Block
//
//  When another thread is selected to run, SwapContext is called.
//  Normally, callers of SwapContext are responsible for saving and
//  restoring non-volatile regs r.14 and r.26 thru r.31.  SwapContext
//  saves/restores gprs r.15 thru r.25. The idle loop never returns so
//  previous contents of r.14 and r.26 thru r.31 are not saved.  The
//  idle loop pre-initializes the storage area where SwapContext would
//  normally save r.15 thru r.25 with values that the idle loop needs
//  in those registers upon return from SwapContext and skips over the
//  register save on the way into SwapContext (alternate entry point
//  SwapFromIdle).  All callers to SwapContext pass the following
//  arguments-
//
//    r.27      Address of Context Swap lock (&KiContextSwapLock)
//    r.28      Address of dispatcher database lock (&KiDispatcherLock)
//    r.29      Address of PRCB
//    r.30      Address of OLD thread object
//    r.31      Address of NEW thread object
//
//  The idle loop does not have a fixed use for regs r.30 and r.31.
//  r.29 contains the correct value for this processor.   r.14 and
//  r.26 contents are unknown and must be regenerated upon return
//  from SwapContext.  The assignment of function to these registers
//  was chosen for easy regeneration of content.
//
//  Note also that r.21 was assigned in the range of registers
//  restored by SwapContext so that it is reset to its initial
//  values whenever SwapContext is called.
//
//--

#define rDbg            r.15
#define rKTOC           r.16
#define rIntOff         r.17
#define rIntOn          r.18
#define rHalIdle        r.19
#define rHalToc         r.20
#define rDbgCount       r.21
#define rTickP          r.22
#define rZero           r.23
#define rDispLkSave     r.24
#define rDPCHEAD        r.25

#define rDispLk         r.28
#define rPrcb           r.29


        SPECIAL_ENTRY_S(KiIdleLoop,_TEXT$00)

        mflr    r.0                             // get return address
        stwu    r.sp, -kscFrameLength(r.sp)     // buy stack frame
        stw     r.0,  kscLR(r.sp)               // save return address

        PROLOGUE_END(KiIdleLoop)


//
//  Setup initial global register values
//

        ori     rKTOC, r.toc, 0                 // backup kernel's TOC
        lwz     rPrcb,  KiPcr+PcPrcb(r.0)       // Address of PCB to rPrcb
        lwz     rTickP, [toc]KeTickCount(r.toc) // Address of KeTickCount
        lwz     rDbg,   [toc]KdDebuggerEnabled(r.toc)// Addr KdDebuggerEnabled
        lwz     rHalToc,[toc]__imp_HalProcessorIdle(r.toc)
        lwz     rHalToc,0(rHalToc)
        lwz     rHalIdle,0(rHalToc)             // HalProcessorIdle entry
        lwz     rHalToc,4(rHalToc)              // HAL's TOC
        li      rZero, 0                        // Keep zero around, we use it
        mfmsr   rIntOff                         // get current machine state
        rlwinm  rIntOff, rIntOff, 0, 0xffff7fff // clear interrupt enable
        ori     rIntOn,  rIntOff,    0x00008000 // set interrupt enable

#if !defined(NT_UP)

        lwz     rDispLk, [toc]KiDispatcherLock(r.toc)// get &KiDispatcherLock
        lwz     r.27, [toc]KiContextSwapLock(r.toc)  // get &KiContextSwapLock

#endif

        addi    rDPCHEAD, rPrcb, PbDpcListHead  // compute DPC listhead address
        li      rDbgCount, 0                    // Clear breakin loop counter

#if !defined(NT_UP)
        ori     rDispLkSave, rDispLk, 0         // copy &KiDispatcherLock
#endif

//
// Registers 15 thru 25 are normally saved by SwapContext but the idle
// loop uses an alternate entry that skips the save by SwapContext.
// SwapContext will still restore them so we set up the stack so what
// we want is what gets restored.  This is especially useful for things
// whose values need to be reset after SwapContext is called, eg rDbgCount.
//

        lwz     r.4, [toc]__imp_KeLowerIrql(r.toc) // &&fd(KeLowerIrql)
        stw     r.15, swFrame + ExGpr15(r.sp)
        stw     r.16, swFrame + ExGpr16(r.sp)
        stw     r.17, swFrame + ExGpr17(r.sp)
        lwz     r.4, 0(r.4)                     // &fd(KeLowerIrql)
        stw     r.18, swFrame + ExGpr18(r.sp)
        stw     r.19, swFrame + ExGpr19(r.sp)
        stw     r.20, swFrame + ExGpr20(r.sp)
        lwz     r.5, 0(r.4)                     // &KeLowerIrql
        stw     r.21, swFrame + ExGpr21(r.sp)
        stw     r.22, swFrame + ExGpr22(r.sp)
        stw     r.23, swFrame + ExGpr23(r.sp)
        stw     r.24, swFrame + ExGpr24(r.sp)
        stw     r.25, swFrame + ExGpr25(r.sp)

//
//  Control reaches here with IRQL at HIGH_LEVEL.  Lower IRQL to
//  DISPATCH_LEVEL and set wait IRQL of idle thread.
//

        mtctr   r.5
        lwz     r.toc, 4(r.4)                   // HAL's TOC
        lwz     r.11, KiPcr+PcCurrentThread(r.0) // Lower thread and processor
        li      r.3, DISPATCH_LEVEL             // IRQL to DISPATCH_LEVEL.
        stb     r.3, ThWaitIrql(r.11)
        bctrl
        ori     r.toc, rKTOC, 0                 // restore our TOC

//
//  In a multiprocessor system the boot processor proceeds directly into
//  the idle loop. As other processors start executing, however, they do
//  not directly enter the idle loop and spin until all processors have
//  been started and the boot master allows them to proceed.
//

#if !defined(NT_UP)

        lwz     r.4, [toc]KiBarrierWait(r.toc)

BarrierWait:
        lwz     r.3, 0(r.4)                     // get barrier wait value
        cmpwi   r.3, 0                          // if ne spin until allowed
        bne     BarrierWait                     // to proceed.

        lbz     r.3,  PbNumber(rPrcb)           // get processor number
        cmpwi   cr.4, r.3, 0                    // save "processor == 0 ?"

#endif

//
// Set condition register and swap return values in the swap frame.
//

        mfcr    r.3                             // save condition register
        stw     r.3, swFrame + SwConditionRegister(r.sp)

        bl      FindIdleReturn
FindIdleReturn:
        mflr    r.3
        addi    r.3, r.3, KiIdleReturn - FindIdleReturn
        stw     r.3, swFrame + SwSwapReturn(r.sp)// save return address

//
//  The following loop is executed for the life of the system.
//

IdleLoop:

#if DBG

#if !defined(NT_UP)
        bne     cr.4, CheckDpcList              // if ne, not processor zero
#endif

//
// Check if the debugger is enabled,  and whether it is time to poll
// for a debugger breakin.  (This check is only performed on cpu 0).
//

        subic.  rDbgCount, rDbgCount, 1         // decrement poll counter
        bge+    CheckDpcList                    // jif not time yet.
        lbz     r.3, 0(rDbg)                    // check if debugger enabled
        li      rDbgCount, 20 * 1000            // set breakin loop counter
        cmpwi   r.3, 0
        beq+    CheckDpcList                    // jif debugger not enabled
        bl      ..KdPollBreakIn                 // check if breakin requested
        cmpwi   r.3, 0
        beq+    CheckDpcList                    // jif no breakin request
        li      r.3, DBG_STATUS_CONTROL_C       // send status to debugger
        bl      ..DbgBreakPointWithStatus

#endif

//
// Disable interrupts and check if there is any work in the DPC list.
//

CheckDpcList:

        mtmsr   rIntOn                          // give interrupts a chance
        isync                                   //  to interrupt spinning
        mtmsr   rIntOff                         // disable interrupts
	cror	0,0,0				// N.B. 603e/ev Errata 15


//
// Process the deferred procedure call list for the current processor.
//

        lwz     r.3,   LsFlink(rDPCHEAD)        // get address of first entry
        cmpw    r.3,   rDPCHEAD                 // is list empty?
        beq     CheckNextThread                 // if eq, DPC list is empty

        ori     r.31, rDPCHEAD, 0
        bl      ..KiProcessDpcList              // process the DPC list

//
// Clear dispatch interrupt pending.
//

        stb     rZero, KiPcr+PcDispatchInterrupt(r.0) // clear pending DPC interrupt

#if DBG
        li      rDbgCount, 0                    // clear breakin loop counter
#endif

//
//  Check if a thread has been selected to run on this processor
//

CheckNextThread:
        lwz     r.31, PbNextThread(rPrcb)       // get address of next thread
        cmpwi   r.31, 0
        beq     Idle                            // jif no thread to execute

//
// A thread has been selected for execution on this processor. Acquire
// dispatcher database lock, get the thread address again (it may have
// changed), clear the address of the next thread in the processor block,
// and call swap context to start execution of the selected thread.
//
// N.B. If the dispatcher database lock cannot be obtained immediately,
//      then attempt to process another DPC rather than spinning on the
//      dispatcher database lock.
//

#if !defined(NT_UP)

        TRY_TO_ACQUIRE_SPIN_LOCK(rDispLk, rDispLk, r.11, LkDisp, CheckDpcList)

#endif

        mtmsr   rIntOn                          // enable interrupts
	cror	0,0,0				// N.B. 603e/ev Errata 15

#if !defined(NT_UP)
        lwz     r.31, PbNextThread(rPrcb)       // get next thread address
#endif

        lwz     r.30, PbCurrentThread(rPrcb)    // get current thread address
        stw     rZero, PbNextThread(rPrcb)      // clear address of next thread
        stw     r.31, PbCurrentThread(rPrcb)    // set new thread current

//
// Acquire the context swap lock so the address space of the old process
// cannot be deleted and then release the dispatcher database lock. In
// this case the old process is the system process, but the context swap
// code releases the context swap lock so it must be acquired.
//
// N.B. This lock is used to protect the address space until the context
//    switch has sufficiently progressed to the point where the address
//    space is no longer needed. This lock is also acquired by the reaper
//    thread before it finishes thread termination.
//

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.27,r.31,r.3,LkCtxSw1,LkCtxSw1Spin)

#endif

//
// Set the new thread's state to Running before releasing the dispatcher lock.
//

        li      r.3, Running                    // set state of new thread
        stb     r.3, ThState(r.31)              // to running.

#if !defined(NT_UP)
       RELEASE_SPIN_LOCK(r.28,rZero)
#endif

        bl      ..SwapFromIdle                  // swap context to new thread

KiIdleReturn:

#if !defined(NT_UP)

        ori     rDispLk, rDispLkSave, 0         // restore &KiDispatcherLock

//
// rDbgCount (r.21) will have been reset to 0 by the register restore
// at the end of SwapContext.
//
// If processor 0, check for debugger breakin, otherwise just check for
// DPCs again.
//

#endif

        b       IdleLoop

//
// There are no entries in the DPC list and a thread has not been selected
// for execution on this processor. Call the HAL so power managment can be
// performed.
//
// N.B. The HAL is called with interrupts disabled. The HAL will return
//      with interrupts enabled.
//

Idle:
        mtctr   rHalIdle                        // set entry point
        ori     r.toc, rHalToc, 0               // set HAL's TOC
        bctrl                                   // call HalProcessorIdle
        isync                                   // give HAL's interrupt enable
                                                //  a chance to take effect
        ori     r.toc, rKTOC, 0                 // restore ntoskrnl's TOC

        b       IdleLoop

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK(r.27,r.3,LkCtxSw1,LkCtxSw1Spin)
#endif

        DUMMY_EXIT(KiIdleLoop)

#undef rDPCHEAD

        SBTTL("Process Deferred Procedure Call List")
//++
//
// Routine Description:
//
//    This routine is called to process the given deferred procedure call
//    list.
//
//    If called from KiDispatchInterrupt, we will have been switched to
//    the interrupt stack, the new stack pointer is in r.sp and entry is
//    at ..KiProcessDpcList.alt.
//
//    If called from the idle loop, we run on the idle loop thread's
//    stack and no special action is needed.   However, the idle loop
//    does not pass r.9 as we expect it and only passes r.0 = 0 on MP
//    systems.  We take advantage of the separate entry points to set
//    these registers appropriately.
//
//    N.B. Interrupts must be disabled on entry to this routine. Control
//         is returned to the caller with the same conditions true.
//
// Arguments:
//
//    None.
//
// On entry:
//
//              r.0  - Zero
//              r.9  - Machine State Register prior to disabling interrupts.
//    rPrcb     r.29 - address of processor control block.
//              r.31 - address of DPC list head.
//
// On exit:
//
//              r.0  - Zero
//              r.9  - Machine State Register prior to disabling interrupts.
//    rPrcb     r.29 - address of processor control block.
//              r.31 - address of DPC list head.
//
// Return value:
//
//    None.
//--

        .struct 0
        .space  StackFrameHeaderLength
dp.lr:  .space  4
dp.toc: .space  4

#if DBG

dp.func:.space  4
dp.strt:.space  4
dp.cnt: .space  4
dp.time:.space  4

#endif

        .align  3       // ensure stack frame length is multile of 8
dp.framelen:

        .text

#if DBG

#  define       rDpStart        r.22
#  define       rDpCount        r.23
#  define       rDpTime         r.24

#endif


        SPECIAL_ENTRY_S(KiProcessDpcList,_TEXT$00)

        stwu    r.sp, -dp.framelen(r.sp)        // buy stack frame

        // see routine description for why we do the following.

        ori     r.9, rIntOn, 0                  // get MSR interrupts enabled

#if defined(NT_UP)
        li      r.0, 0
#endif


..KiProcessDpcList.alt:

        mflr    r.7                             // save return address

        // save regs we will use,... don't need to save 29 and 31 as they
        // were saved by our caller and currently contain the values we want.

        stw     r.toc, dp.toc(r.sp)

#if DBG

        stw     rDpTime,  dp.time(r.sp)
        stw     rDpCount, dp.cnt(r.sp)
        stw     rDpStart, dp.strt(r.sp)

#endif

        stw     r.7, dp.lr(r.sp)                // save Link Register

        PROLOGUE_END(KiProcessDpcList)

DpcCallRestart:

        stw     r.sp, PbDpcRoutineActive(rPrcb) // set DPC routine active

//
// Process the DPC list.
//

DpcCall:

#if !defined(NT_UP)

        addi    r.7, rPrcb, PbDpcLock           // compute DPC lock address

        ACQUIRE_SPIN_LOCK(r.7, r.7, r.0, spinlk2, spinlk2spin)

#endif

        lwz     r.3, LsFlink(r.31)              // get address of first entry
        lwz     r.12, LsFlink(r.3)              // get addr of next entry
        cmpw    r.3, r.31                       // is list empty?
        subi    r.3, r.3, DpDpcListEntry        // subtract DpcListEntry offset
        beq-    UnlkDpc0                        // if yes, release the lock.

//
// Get deferred routine address, this is done early as what
// we actually have is a function descriptor's address and we
// need to get the entry point address.
//

        lwz     r.11, DpDeferredRoutine(r.3)
        lwz     r.8, PbDpcQueueDepth(rPrcb)     // get DPC queue depth

//
// remove entry from list
//

        stw     r.12, LsFlink(r.31)             // set addr of next in header
        stw     r.31, LsBlink(r.12)             // set addr of previous in next

        lwz     r.10, 0(r.11)                   // get DPC code address

//
// entry removed, set up arguments for DPC proc
//
// args are-
//      dpc object address (r.3)
//      deferred context   (r.4)
//      system argument 1  (r.5)
//      system argument 2  (r.6)
//
// note, the arguments must be loaded from the DPC object BEFORE
//       the inserted flag is cleared to prevent the object being
//       overwritten before its time.
//

        lwz     r.4, DpDeferredContext(r.3)
        lwz     r.5, DpSystemArgument1(r.3)
        lwz     r.6, DpSystemArgument2(r.3)

        subi    r.8, r.8, 1                     // decrement DPC queue depth
        stw     r.8, PbDpcQueueDepth(rPrcb)     //
        stw     r.0, DpLock(r.3)                // clear DPC inserted state

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.7, r.0)
#endif

        mtctr   r.10                            // ready address for branch

        ENABLE_INTERRUPTS(r.9)

        lwz     r.toc, 4(r.11)                  // get DPC toc pointer

#if DBG
        lwz     rDpStart, KiPcr2 + Pc2TickCountLow(r.0) // get current time
        lwz     rDpCount, PbInterruptCount(rPrcb)// get current interrupt count
        lwz     rDpTime, PbInterruptTime(rPrcb) // get current interrupt time
        stw     r.10, dp.func(r.sp)
#endif

        bctrl                                   // call DPC routine

        li      r.0, 0                          // reset zero constant

#if DBG
        lbz     r.10, KiPcr+PcCurrentIrql(r.0)  // get current IRQL
        cmplwi  r.10, DISPATCH_LEVEL            // check if < DISPATCH_LEVEL
        blt     DpcBadIrql                      // jif IRQL < DISPATCH_LEVEL

DpcIrqlOk:

        lwz     r.12, KiPcr2 + Pc2TickCountLow(r.0) // calculate time spent in
        sub     r.12, r.12, rDpStart            // r.12 = time
        cmpwi   r.12, 100
        bge     DpcTookTooLong                  // jif >= 1 second
DpcTimeOk:
#endif

//
// Check to determine if any more DPCs are available to process.
//

        DISABLE_INTERRUPTS(r.9, r.10)

        lwz     r.3, LsFlink(r.31)              // get address of first entry
        cmpw    r.3, r.31                       // is list empty?
        bne-    DpcCall                         // if no, process it

//
// Clear DpcRoutineActive, then check one last time that the DPC queue is
// empty.  This is required to close a race condition with the DPC queueing
// code where it appears that a DPC routine is active (and thus an
// interrupt is not requested), but this code has decided that the queue
// is empty and is clearing DpcRoutineActive.
//

        stw     r.0, PbDpcRoutineActive(rPrcb)
        stw     r.0, PbDpcInterruptRequested(rPrcb) // clear DPC interrupt requested
        eieio                                   // force writes out

        lwz     r.3, LsFlink(r.31)
        cmpw    r.3, r.31
        bne-    DpcCallRestart

DpcDone:

//
// List is empty, restore non-volatile registers we have used.
//

        lwz     r.10, dp.lr(r.sp)               // get link register

#if DBG
        lwz     rDpTime,  dp.time(r.sp)
        lwz     rDpCount, dp.cnt(r.sp)
        lwz     rDpStart, dp.strt(r.sp)
#endif


//
// Return to caller.
//

        lwz     r.toc, dp.toc(r.sp)             // restore kernel toc
        mtlr    r.10                            // set return address
        lwz     r.sp, 0(r.sp)                   // release stack frame

        blr                                     // return

UnlkDpc0:

//
// The DPC list became empty while we were acquiring the DPC queue lock.
// Clear DPC routine active.  The race condition mentioned above doesn't
// exist here because we hold the DPC queue lock.
//

        stw     r.0, PbDpcRoutineActive(rPrcb)

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.7, r.0)
#endif

        b       DpcDone

//
//  DpcTookTooLong, DpcBadIrql
//
//  Come here is it took >= 1 second to execute a DPC routine.  This is way
//  too long, assume something is wrong and breakpoint.
//
//  This code is out of line to avoid wasting cache space for something that
//  (hopefully) never happens.
//

#if DBG

DpcTookTooLong:
        lwz     r.toc, dp.toc(r.sp)             // restore kernel's TOC
        lwz     r.11, PbInterruptCount(rPrcb)   // get current interrupt count
        lwz     r.10, PbInterruptTime(rPrcb)    // get current interrupt time
        lwz     r.toc, dp.toc(r.sp)             // restore our toc
        sub     r.11, r.11, rDpCount            // compute number of interrupts
        sub     r.10, r.10, rDpTime             // compute interrupt time
        bl      ..DbgBreakPoint                 // execute debug breakpoint
        b       DpcTimeOk                       // continue

DpcBadIrql:
        lwz     r.toc, dp.toc(r.sp)             // restore kernel's TOC
        bl      ..DbgBreakPoint                 // breakpoint
        b       DpcIrqlOk                       // continue

#endif

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK(r.7, r.11, spinlk2, spinlk2spin)
#endif

        DUMMY_EXIT(KiProcessDpcList)

        SBTTL("Dispatch Interrupt")

#define rDPCHEAD        r.31

//++
//
// Routine Description:
//
//    This routine is entered as the result of a software interrupt generated
//    at DISPATCH_LEVEL. Its function is to process the Deferred Procedure Call
//    (DPC) list, and then perform a context switch if a new thread has been
//    selected for execution on the processor.
//
//    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
//    database unlocked. When a return to the caller finally occurs, the
//    IRQL remains at DISPATCH_LEVEL, and the dispatcher database is still
//    unlocked.
//
// Arguments:
//
//    None.
//
// Outputs:
// ( for call to KiProcessDpcList )
//              r.3  - address of first dpc in list
//              r.0  - Zero
//              r.9  - Machine State Register prior to disabling interrupts.
//    rPrcb     r.29 - address of processor control block.
//    rDPCHEAD  r.31 - address of DPC listhead.
//    r.9   Machine State Register prior to disabling interrupts.
//
// ( for call to KiDispIntSwapContext )
//
//    r.28         pointer to Dispatcher Database Lock
//    r.29  rPrcb  pointer to the processor control block
//    r.31  NTH    pointer to new thread
//
// Return Value:
//
//    None.
//
//--

        .struct 0
        .space  StackFrameHeaderLength
di.lr:  .space  4
di.28:  .space  4
di.29:  .space  4
di.30:  .space  4
di.31:  .space  4

        .align  3       // ensure stack frame length is multile of 8
di.framelen:

        SPECIAL_ENTRY_S(KiDispatchInterrupt,_TEXT$00)

        mflr    r.0
        stwu    r.sp, -di.framelen(r.sp)
        stw     r.29, di.29(r.sp)
        lwz     rPrcb,KiPcr+PcPrcb(r.0)
        stw     r.30, di.30(r.sp)
        stw     r.31, di.31(r.sp)
        stw     r.0,  di.lr(r.sp)
        stw     r.28, di.28(r.sp)

        PROLOGUE_END(KiDispatchInterrupt)

//
// Setup commonly used constants
//

        lwz     r.3, PbDpcBypassCount(rPrcb)    // get DPC bypass count
        li      r.0, 0                          // zero
        addi    rDPCHEAD, rPrcb, PbDpcListHead  // compute DPC listhead address
        addi    r.3, r.3, 1                     // increment DPC bypass count
        stw     r.3, PbDpcBypassCount(rPrcb)    // store new DPC bypass count

//
// Process the deferred procedure call list.
//

PollDpcList:

        DISABLE_INTERRUPTS(r.9, r.8)
        lwz     r.3, LsFlink(rDPCHEAD)          // get address of first entry
        stb     r.0, KiPcr+PcDispatchInterrupt(r.0)
        cmpw    r.3, rDPCHEAD                   // list has entries?
        beq-    di.empty                        // jif list is empty

//
// Switch to the interrupt stack
//

        lwz     r.6, KiPcr+PcInterruptStack(r.0)// get addr of interrupt stack
        lwz     r.28, KiPcr+PcInitialStack(r.0)  // get current stack base
        lwz     r.30, KiPcr+PcStackLimit(r.0)    // get current stack limit
        subi    r.4, r.6, KERNEL_STACK_SIZE      // compute stack limit
        stw     r.sp,KiPcr+PcOnInterruptStack(r.0)// flag ON interrupt stack
        stw     r.sp, -dp.framelen(r.6)          // save new back pointer

//
// N.B. Can't step thru the next two instructions.
//

        stw     r.4,  KiPcr+PcStackLimit(r.0)   // set stack limit
        stw     r.6,  KiPcr+PcInitialStack(r.0) // set current base to int stk
        subi    r.sp, r.6, dp.framelen          // calc new sp

        bl      ..KiProcessDpcList.alt          // process all DPCs for this
                                                // processor.

//
// N.B. KiProcessDpcList left r.0, r.9 intact.
//
// Return from KiProcessDpcList switched back to the proper stack,
// update PCR to reflect this.
//

        stw     r.30, KiPcr+PcStackLimit(r.0)   // restore stack limit
        stw     r.28, KiPcr+PcInitialStack(r.0) // set old stack current
        stw     r.0,  KiPcr+PcOnInterruptStack(r.0)// clear ON interrupt stack

di.empty:

        ENABLE_INTERRUPTS(r.9)

//
// Check to determine if quantum end has occurred.
//
// N.B. If a new thread is selected as a result of processing a quantum
//      end request, then the new thread is returned with the dispatcher
//      database locked. Otherwise, NULL is returned with the dispatcher
//      database unlocked.
//
        lwz     r.3, KiPcr+PcQuantumEnd(r.0)    // get quantum end indicator
        cmpwi   r.3, 0                          // if 0, no quantum end request
        beq     di.CheckForNewThread
        stw     r.0, KiPcr+PcQuantumEnd(r.0)    // clear quantum end indicator

        bl      ..KiQuantumEnd                  // process quantum end
        cmpwi   r.3, 0                          // new thread selected?
        li      r.0, 0                          // reset r.0 to zero
//
// If KiQuantumEnd returned no new thread to run, the dispatcher
// database is unlocked, get out.
//

        beq+    di.exit

#if !defined(NT_UP)

//
// Even though the dispatcher database is already locked, we are expected
// to pass the address of the lock in r.28.
//

        lwz     r.28, [toc]KiDispatcherLock(r.toc)// get &KiDispatcherLock

#endif

        b       di.Switch                       // switch to new thread

//
// Check to determine if a new thread has been selected for execution on
// this processor.
//

di.CheckForNewThread:
        lwz     r.3, PbNextThread(rPrcb)        // get address of next thread
        cmpwi   r.3, 0                          // is there a new thread?
        beq     di.exit                         // no, branch.

#if !defined(NT_UP)

        lwz     r.28, [toc]KiDispatcherLock(r.toc)// get &KiDispatcherLock


//
// Lock dispatcher database and reread address of next thread object since it
// is possible for it to change in a multiprocessor system. (leave address
// of lock in r.28).
//

        TRY_TO_ACQUIRE_SPIN_LOCK(r.28, r.28, r.11, di.spinlk, PollDpcList)

#endif

di.Switch:
        lwz     r.31, PbNextThread(rPrcb)       // get thread address (again)
        stw     r.0,  PbNextThread(rPrcb)       // clear addr of next thread obj

//
// Reready current thread for execution and swap context to the
// selected thread.  We do this indirectly thru KiDispIntSwapContext
// to avoid saving and restoring so many registers for the cases
// when KiDispatchInterrupt does not thread switch.
//

        bl      ..KiDispIntSwapContext          // swap to new thread

di.exit:

        lwz     r.0,  di.lr(r.sp)
        lwz     r.31, di.31(r.sp)
        lwz     r.30, di.30(r.sp)
        mtlr    r.0
        lwz     r.29, di.29(r.sp)
        lwz     r.28, di.28(r.sp)
        addi    r.sp, r.sp, di.framelen

        SPECIAL_EXIT(KiDispatchInterrupt)

//++
//
// VOID
// KiDispIntSwapContext (
//    IN PKTHREAD Thread
//    )
//
// Routine Description:
//
//    This routine is called to perform a context switch to the specified
//    thread. The current (new previous) thread is re-readied for execution.
//
//    Since this routine is called as subroutine all volatile registers are
//    considered free.
//
//    Our caller has saved and will restore gprs 28 thru 31 and does not
//    care if we trash them.
//
//    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
//    database locked. When a return to the caller finally occurs, the
//    dispatcher database is unlocked.
//
// Arguments:
//
//    r.28         pointer to Dispatcher Database Lock
//    r.29  rPrcb  pointer to the processor control block
//    r.31  NTH    pointer to new thread
//
// Outputs:     ( for call to SwapContext )
//
//    r.27         pointer to KiContextSwapLock
//    r.28         pointer to Dispatcher Database Lock
//    r.29  rPrcb  pointer to processor control block
//    r.30  OTH    pointer to old thread
//    r.31  NTH    pointer to new thread
//
// Return Value:
//
//    Wait completion status (r.3).
//
//--

        .struct 0
        .space  swFrameLength
kdiscLR:.space  4
        .align  3                       // ensure 8 byte alignment
kdiscFrameLength:

        .align  6                       // cache line alignment

        SPECIAL_ENTRY_S(KiDispIntSwapContext,_TEXT$00)

        mflr    r.0                             // get return address
        lwz     r.30, KiPcr+PcCurrentThread(r.0) // get current (old) thread
        stwu    r.sp, -kdiscFrameLength(r.sp)   // buy stack frame
        stw     r.14, swFrame + ExGpr14(r.sp)   // save gpr 14
        stw     r.26, swFrame + ExGpr26(r.sp)   // save gprs 26 and 27
        stw     r.27, swFrame + ExGpr27(r.sp)   //

//
//  Gprs 28, 29, 30 and 31 saved/restored by KiDispatchInterrupt
//

        stw     r.0,  kdiscLR(r.sp)             // save return address

        PROLOGUE_END(KiDispIntSwapContext)

        stw     r.31, PbCurrentThread(rPrcb)    // set new thread current

//
// Reready current thread and swap context to the selected thread.
//

        ori     r.3, r.30, 0

#if !defined(NT_UP)

        lwz     r.27, [toc]KiContextSwapLock(r.2)

#endif

        bl      ..KiReadyThread                 // reready current thread
        bl      ..SwapContext                   // switch threads

//
// Restore registers and return.
//

        lwz     r.0,  kdiscLR(r.sp)             // restore return address
        lwz     r.26, swFrame + ExGpr26(r.sp)   // restore gpr 26 and 27
        mtlr    r.0                             // set return address
        lwz     r.27, swFrame + ExGpr27(r.sp)   //

//
//  Gprs 28, 29, 30 and 31 saved/restored by KiDispatchInterrupt
//

        lwz     r.14, swFrame + ExGpr14(r.sp)   // restore gpr 14
        addi    r.sp, r.sp, kdiscFrameLength    // return stack frame

        SPECIAL_EXIT(KiDispIntSwapContext)

//++
//
// VOID
// KiRequestSoftwareInterrupt (KIRQL RequestIrql)
//
// Routine Description:
//
//    This function requests a software interrupt at the specified IRQL
//    level.
//
// Arguments:
//
//    RequestIrql (r.3) - Supplies the request IRQL value.
//                        Allowable values are APC_LEVEL (1)
//                                             DPC_LEVEL (2)
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRequestSoftwareInterrupt)

        lbz     r.6, KiPcr+PcCurrentIrql(r.0)   // get current IRQL
        rlwinm  r.4, r.3, 31, 0x1               // transform 1 or 2 to 0 or 1
        li      r.5, 1                          // non-zero value
        cmpw    r.6, r.3                        // is current IRQL < requested IRQL?
        stb     r.5, KiPcr+PcSoftwareInterrupt(r.4) // set interrupt pending
        blt     ..KiDispatchSoftwareInterrupt   // jump to dispatch interrupt if
                                                //  current IRQL low enough (note
                                                //  that this is a jump, not a call)

        LEAF_EXIT(KiRequestSoftwareInterrupt)
