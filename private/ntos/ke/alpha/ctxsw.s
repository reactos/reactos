//      TITLE("Context Swap")
//++
//
// Copyright (c) 1991  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    ctxsw.s
//
// Abstract:
//
//    This module implements the ALPHA machine dependent code necessary to
//    field the dispatch interrupt and to perform kernel initiated context
//    switching.
//
// Author:
//
//    David N. Cutler (davec) 1-Apr-1991
//    Joe Notarangelo 05-Jun-1992
//
// Environment:
//
//    Kernel mode only, IRQL DISPATCH_LEVEL.
//
// Revision History:
//
//--

#include "ksalpha.h"

// #define _COLLECT_SWITCH_DATA_ 1

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
//    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
//    database locked. Ifs function is to either unlock the dispatcher
//    database and return or initiate a context switch if another thread
//    has been selected for execution.
//
//    N.B. A context switch CANNOT be initiated if the previous IRQL
//         is DISPATCH_LEVEL.
//
//    N.B. This routine is carefully written to be a leaf function. If,
//        however, a context swap should be performed, the routine is
//        switched to a nested function.
//
// Arguments:
//
//    OldIrql (a0) - Supplies the IRQL when the dispatcher database
//        lock was acquired.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiUnlockDispatcherDatabase)

//
// Check if a thread has been scheduled to execute on the current processor
//

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get prcb address

        cmpult  a0, DISPATCH_LEVEL, t1  // check if IRQL below dispatch level
        LDP     t2, PbNextThread(v0)    // get next thread address
        bne     t2, 30f                 // if ne, next thread selected

//
// Release dispatcher database lock, restore IRQL to its previous level
// and return
//

10:                                     //

#if !defined(NT_UP)

        bis     a0, zero, a1            // set old IRQL value
        ldil    a0, LockQueueDispatcherLock // set lock queue number
        br      zero, KeReleaseQueuedSpinLock // release dispatcher lock

#else

        SWAP_IRQL                       // lower IRQL

        ret     zero, (ra)              // return

#endif

//
// A new thread has been selected to run on the current processor, but
// the new IRQL is not below dispatch level. If the current processor is
// not executing a DPC, then request a dispatch interrupt on the current
// processor before releasing the dispatcher lock and restoring IRQL.
//

20:     ldl     t2, PbDpcRoutineActive(v0) // check if DPC routine active
        bne     t2,10b                  // if ne, DPC routine active

#if !defined(NT_UP)

        bis     a0, zero, t0            // save old IRQL value
        ldil    a0, DISPATCH_LEVEL      // set interrupt request level

        REQUEST_SOFTWARE_INTERRUPT      // request DPC interrupt

        ldil    a0, LockQueueDispatcherLock // set lock queue number
        bis     t0, zero, a1            // set old IRQL value
        br      zero, KeReleaseQueuedSpinLock // release dispatcher lock

#else

        SWAP_IRQL                       // lower IRQL

        ldil    a0, DISPATCH_LEVEL      // set interrupt request level

        REQUEST_SOFTWARE_INTERRUPT      // request DPC interrupt

        ret     zero, (ra)              // return

#endif

//
// A new thread has been selected to run on the current processor.
//
// If the new IRQL is less than dispatch level, then switch to the new
// thread.
//

30:     beq     t1, 20b                         // if eq, not below dispatch level

        .end    KiUnlockDispatcherDatabase

//
// N.B. This routine is carefully written as a nested function.
//    Control only reaches this routine from above.
//
//    v0 contains the address of PRCB
//    t2 contains the next thread
//

        NESTED_ENTRY(KxUnlockDispatcherDatabase, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate context frame
        stq     ra, ExIntRa(sp)         // save return address
        stq     s0, ExIntS0(sp)         // save integer registers
        stq     s1, ExIntS1(sp)         //
        stq     s2, ExIntS2(sp)         //
        stq     s3, ExIntS3(sp)         //
        stq     s4, ExIntS4(sp)         //
        stq     s5, ExIntS5(sp)         //
        stq     fp, ExIntFp(sp)         //

        PROLOGUE_END

        bis     v0, zero, s0            // set address of PRCB

        GET_CURRENT_THREAD              // get current thread address

        bis     v0, zero, s1            // set current thread address
        bis     t2, zero, s2            // set next thread address
        StoreByte(a0, ThWaitIrql(s1))   // save previous IRQL
        STP     zero, PbNextThread(s0)  // clear next thread address

//
// Reready current thread for execution and swap context to the selected thread.
//
// N.B. The return from the call to swap context is directly to the swap
//      thread exit.
//

        bis     s1, zero, a0            // set previous thread address
        STP     s2, PbCurrentThread(s0) // set address of current thread object
        bsr     ra, KiReadyThread       // reready thread for execution
        lda     ra, KiSwapThreadExit    // set return address
        jmp     SwapContext             // swap context

        .end    KxUnlockDispatcherDatabase

        SBTTL("Swap Thread")
//++
//
// INT_PTR
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
// Return Value:
//
//    Wait completion status (v0).
//
//--

        NESTED_ENTRY(KiSwapThread, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate context frame
        stq     ra, ExIntRa(sp)         // save return address
        stq     s0, ExIntS0(sp)         // save integer registers s0 - s5
        stq     s1, ExIntS1(sp)         //
        stq     s2, ExIntS2(sp)         //
        stq     s3, ExIntS3(sp)         //
        stq     s4, ExIntS4(sp)         //
        stq     s5, ExIntS5(sp)         //
        stq     fp, ExIntFp(sp)         // save fp

        PROLOGUE_END

        GET_PROCESSOR_CONTROL_REGION_BASE // get pcr address

        bis     v0, zero, s3            // save PCR address
        LDP     s0, PcPrcb(s3)          // get address of PRCB
        ldl     s5, KiReadySummary      // get ready summary
        zapnot  s5, 0x0f, t0            // clear high 32 bits

        GET_CURRENT_THREAD              // get current thread address

        bis     v0, zero, s1            // set current thread address
        LDP     s2, PbNextThread(s0)    // get next thread address

#if !defined(NT_UP)

        ldl     fp, PcSetMember(s3)     // get processor affinity mask

#endif

        STP     zero, PbNextThread(s0)  // zero next thread address
        bne     s2, 120f                // if ne, next thread selected

//
// Find the highest nibble in the ready summary that contains a set bit
// and left justify so the nibble is in bits <63:60>.
//

        cmpbge  zero, t0, s4            // generate mask of clear bytes
        ldil    t2, 7                   // set initial bit number
        srl     s4, 1, t5               // check bits <15:8>
        cmovlbc t5, 15, t2              // if bit clear, bit number = 15
        srl     s4, 2, t6               // check bits <23:16>
        cmovlbc t6, 23, t2              // if bit clear, bit number = 23
        srl     s4, 3, t7               // check bits <31:24>
        cmovlbc t7, 31, t2              // if bit clear, bit number = 31
        bic     t2, 7, t3               // get byte shift from priority
        srl     t0, t3, s4              // isolate highest nonzero byte
        and     s4, 0xf0, t4            // check if high nibble nonzero
        subq    t2, 4, t1               // compute bit number if high nibble zero
        cmoveq  t4, t1, t2              // if eq, high nibble zero
10:     ornot   zero, t2, t4            // compute left justify shift count
        sll     t0, t4, t0              // left justify ready summary to nibble

//
// If the next bit is set in the ready summary, then scan the corresponding
// dispatcher ready queue.
//

30:     blt     t0, 50f                 // if ltz, queue contains an entry
31:     sll     t0, 1, t0               // position next ready summary bit
        subq    t2, 1, t2               // decrement ready queue priority
        bne     t0, 30b                 // if ne, more queues to scan

//
// All ready queues were scanned without finding a runnable thread so
// default to the idle thread and set the appropirate bit in idle summary.
//

#if defined(_COLLECT_SWITCH_DATA_)

        lda     t0, KeThreadSwitchCounters // get switch counters address
        ldl     v0, TwSwitchToIdle(t0)  // increment switch to idle count
        addl    v0, 1, v0               //
        stl     v0, TwSwitchToIdle(t0)  //

#endif

#if defined(NT_UP)

        ldil    t0, 1                   // get current idle summary

#else

        ldl     t0, KiIdleSummary       // get current idle summary
        bis     t0, fp, t0              // set member bit in idle summary

#endif

        stl     t0, KiIdleSummary       // set new idle summary
        LDP     s2, PbIdleThread(s0)    // set address of idle thread

        br      zero, 120f              // swap context

//
// Compute address of ready list head and scan reday queue for a runnable
// thread.
//

50:     lda     t1, KiDispatcherReadyListHead // get ready ready base address

#if defined(_AXP64_)

        sll     t2, 4, s4               // compute ready queue address
        addq    s4, t1, s4              //

#else

        s8addl  t2, t1, s4              // compute ready queue address

#endif

        LDP     t4, LsFlink(s4)         // get address of next queue entry
55:     SUBP    t4, ThWaitListEntry, s2 // compute address of thread object

//
// If the thread can execute on the current processor, then remove it from
// the dispatcher ready queue.
//

#if !defined(NT_UP)

        ldl     t5, ThAffinity(s2)      // get thread affinity
        and     t5, fp, t6              // the current processor
        bne     t6, 60f                 // if ne, thread affinity compatible
        LDP     t4, LsFlink(t4)         // get address of next entry
        cmpeq   t4, s4, t1              // check for end of list
        beq     t1, 55b                 // if eq, not end of list
        br      zero, 31b               //

//
// If the thread last ran on the current processor, the processor is the
// ideal processor for the thread, the thread has been waiting for longer
// than a quantum, ot its priority is greater than low realtime plus 9,
// then select the thread. Otherwise, an attempt is made to find a more
// appropriate candidate.
//

60:     ldq_u   t1, PcNumber(s3)        // get current processor number
        extbl   t1, PcNumber % 8, t12   //
        ldq_u   t11, ThNextProcessor(s2) // get last processor number
        extbl   t11, ThNextProcessor % 8, t9 //
        cmpeq   t9, t12, t5             // check thread's last processor
        bne     t5, 110f                // if eq, last processor match
        ldq_u   t6, ThIdealProcessor(s2) // get thread's ideal processor number
        extbl   t6, ThIdealProcessor % 8, a3 //
        cmpeq   a3, t12, t8             // check thread's ideal processor
        bne     t8, 100f                // if eq, ideal processor match
        ldl     t6, KeTickCount         // get low part of tick count
        ldl     t7, ThWaitTime(s2)      // get time of thread ready
        subq    t6, t7, t8              // compute length of wait
        cmpult  t8, READY_SKIP_QUANTUM + 1, t1 // check if wait time exceeded
        cmpult  t2, LOW_REALTIME_PRIORITY + 9, t3 // check if priority in range
        and     t1, t3, v0              // check if priority and time match
        beq     v0, 100f                // if eq, select this thread

//
// Search forward in the ready queue until the end of the list is reached
// or a more appropriate thread is found.
//

        LDP     t7, LsFlink(t4)         // get address of next entry
80:     cmpeq   t7, s4, t1              // if eq, end of list
        bne     t1, 100f                // select original thread
        SUBP    t7, ThWaitListEntry, a0 // compute address of thread object
        ldl     a2, ThAffinity(a0)      // get thread affinity
        and     a2, fp, t1              // check for compatibile thread affinity
        beq     t1, 85f                 // if eq, thread affinity not compatible
        ldq_u   t5, ThNextProcessor(a0) // get last processor number
        extbl   t5, ThNextProcessor % 8, t9 //
        cmpeq   t9, t12, t10            // check if last processor number match
        bne     t10, 90f                // if ne, last processor match
        ldq_u   a1, ThIdealProcessor(a0) // get ideal processor number
        extbl   a1, ThIdealProcessor % 8, a3 //
        cmpeq   a3, t12, t10            // check if ideal processor match
        bne     t10, 90f                // if ne, ideal processor match
85:     ldl     t8, ThWaitTime(a0)      // get time of thread ready
        LDP     t7, LsFlink(t7)         // get address of next entry
        subq    t6, t8, t8              // compute length of wait
        cmpult  t8, READY_SKIP_QUANTUM + 1, t5 //  check if wait time exceeded
        bne     t5, 80b                 // if ne, wait time not exceeded
        br      zero, 100f              // select original thread

//
// Last processor or ideal processor match.
//

90:     bis     a0, zero, s2            // set thread address
        bis     t7, zero, t4            // set list entry address
        bis     t5, zero, t11           // copy last processor data
100:    insbl   t12, ThNextProcessor % 8, t8 // move next processor into position
        mskbl   t11, ThNextProcessor % 8, t5 // mask next processor position
        bis     t8, t5, t6              // merge
        stq_u   t6, ThNextProcessor(s2) // update next processor

110:                                    //

#if defined(_COLLECT_SWITCH_DATA_)

        ldq_u   t5, ThNextProcessor(s2) // get last processor number
        extbl   t5, ThNextProcessor % 8, t9 //
        ldq_u   a1, ThIdealProcessor(s2) // get ideal processor number
        extbl   a1, ThIdealProcessor % 8, a3 //
        lda     t0, KeThreadSwitchCounters + TwFindAny // compute address of Any counter
        ADDP    t0, TwFindIdeal-TwFindAny, t1 // compute address of Ideal counter
        cmpeq   t9, t12, t7             // if eq, last processor match
        ADDP    t0, TwFindLast-TwFindAny, t6 // compute address of Last counter
        cmpeq   a3, t12, t5             // check if ideal processor match
        cmovne  t7, t6, t0              // if last match, use last counter
        cmovne  t5, t1, t0              // if ideal match, use ideal counter
        ldl     v0, 0(t0)               // increment counter
        addl    v0, 1, v0               //
        stl     v0, 0(t0)               //

#endif

#endif

        LDP     t5, LsFlink(t4)         // get list entry forward link
        LDP     t6, LsBlink(t4)         // get list entry backward link
        STP     t5, LsFlink(t6)         // set forward link in previous entry
        STP     t6, LsBlink(t5)         // set backward link in next entry
        cmpeq   t6, t5, t7              // if eq, list is empty
        beq     t7, 120f                //
        ldil    t1, 1                   // compute ready summary set member
        sll     t1, t2, t1              //
        xor     t1, s5, t1              // clear member bit in ready summary
        stl     t1, KiReadySummary      //

//
// Swap context to the next thread
//

120:    STP     s2, PbCurrentThread(s0) // set address of current thread object
        bsr     ra, SwapContext         // swap context

//
// Lower IRQL, deallocate context frame, and return wait completion status.
//
// N.B. SwapContext releases the dispatcher database lock.
//
// N.B. The register v0 contains the complement of the kernel APC pending state.
//
// N.B. The register s2 contains the address of the new thread.
//

        ALTERNATE_ENTRY(KiSwapThreadExit)

        LDP     s1, ThWaitStatus(s2)    // get wait completion status
        ldq_u   t1, ThWaitIrql(s2)      // get original IRQL
        extbl   t1, ThWaitIrql % 8, a0  //
        bis     v0, a0, t3              // check if APC pending and IRQL is zero
        bne     t3, 10f                 // if ne, APC not pending or IRQL not zero

//
// Lower IRQL to APC level and dispatch APC interrupt.
//

        ldil    a0, APC_LEVEL           // lower IRQL to APC level

        SWAP_IRQL                       //

        ldil    a0, APC_LEVEL           // clear software interrupt

        DEASSERT_SOFTWARE_INTERRUPT     //

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        ldl     t1, PbApcBypassCount(v0) // increment the APC bypass count
        addl    t1, 1, t2               //
        stl     t2, PbApcBypassCount(v0) //
        bis     zero, zero, a0          // set previous mode to kernel
        bis     zero, zero, a1          // set exception frame address
        bis     zero, zero, a2          // set trap frame address
        bsr     ra, KiDeliverApc        // deliver kernel mode APC
        bis     zero, zero, a0          // set original wait IRQL

//
// Lower IRQL to wait level, set return status, restore registers, and
// return.
//

10:     SWAP_IRQL                       // lower IRQL to wait level

        bis     s1, zero, v0            // set return status value
        ldq     ra, ExIntRa(sp)         // restore return address
        ldq     s0, ExIntS0(sp)         // restore int regs S0-S5
        ldq     s1, ExIntS1(sp)         //
        ldq     s2, ExIntS2(sp)         //
        ldq     s3, ExIntS3(sp)         //
        ldq     s4, ExIntS4(sp)         //
        ldq     s5, ExIntS5(sp)         //
        ldq     fp, ExIntFp(sp)         // restore fp

        lda     sp, ExceptionFrameLength(sp) // deallocate context frame
        ret     zero, (ra)              // return

        .end    KiSwapThread

        SBTTL("Dispatch Interrupt")
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
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved. The volatile floating point registers have not been saved.
//
// Arguments:
//
//    fp - Supplies a pointer to the base of a trap frame.
//
// Return Value:
//
//    None.
//
//--
        .struct 0
DpSp:   .space  8                       // saved stack pointer
DpBs:   .space  8                       // base of previous stack
DpLim:  .space  8                       // limit of previous stack
        .space  8                       // pad to octaword
DpcFrameLength:                         // DPC frame length

        NESTED_ENTRY(KiDispatchInterrupt, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate exception frame
        stq     ra, ExIntRa(sp)         // save return address

//
// Save the saved registers in case we context switch to a new thread.
//
// N.B. - If we don't context switch then we need only restore those
//        registers that we use in this routine, currently those registers
//        are s0, s1
//

        stq     s0, ExIntS0(sp)          // save integer registers s0-s6
        stq     s1, ExIntS1(sp)          //
        stq     s2, ExIntS2(sp)          //
        stq     s3, ExIntS3(sp)          //
        stq     s4, ExIntS4(sp)          //
        stq     s5, ExIntS5(sp)          //
        stq     fp, ExIntFp(sp)          //

        PROLOGUE_END

//
// Increment the dispatch interrupt count
//

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        bis     v0, zero, s0             // save current prcb address
        ldl     t2, PbDispatchInterruptCount(s0) // increment dispatch interrupt count
        addl    t2, 1, t3                //
        stl     t3, PbDispatchInterruptCount(s0) //

//
// Process the DPC List with interrupts off.
//

        ldl     t0, PbDpcQueueDepth(s0) // get current queue depth
        beq     t0, 20f                 // if eq, no DPCs pending

//
// Save current initial kernel stack address and set new initial kernel stack
// address.
//

PollDpcList:                            //

        DISABLE_INTERRUPTS              // disable interrupts

        GET_PROCESSOR_CONTROL_REGION_BASE // get current PCR address

        LDP     a0, PcDpcStack(v0)      // get address of DPC stack
        lda     t0, -DpcFrameLength(a0) // allocate DPC frame
        LDP     t4, PbCurrentThread(s0) // get current thread
        LDP     t5, ThStackLimit(t4)    // get current stack limit
        stq     sp, DpSp(t0)            // save old stack pointer
        stq     t5, DpLim(t0)           // save old stack limit
        SUBP    a0, KERNEL_STACK_SIZE, t5 // compute new stack limit
        STP     t5, ThStackLimit(t4)    // store new stack limit
        bis     t0, t0, sp              // set new stack pointer

        SET_INITIAL_KERNEL_STACK        // set new initial kernel stack

        stq     v0, DpBs(sp)            // save previous initial stack
        bsr     ra, KiRetireDpcList     // process the DPC list

//
// Switch back to previous stack and restore the initial stack limit.
//

        ldq     a0, DpBs(sp)            // get previous initial stack address
        ldq     t5, DpLim(sp)           // get old stack limit

        SET_INITIAL_KERNEL_STACK        // reset current initial stack

        ldq     sp, DpSp(sp)            // restore stack pointer
        LDP     t4, PbCurrentThread(s0) // get current thread
        STP     t5, ThStackLimit(t4)    // restore stack limit

        ENABLE_INTERRUPTS               // enable interrupts

//
// Check to determine if quantum end has occured.
//

20:     ldl     t0, PbQuantumEnd(s0)    // get quantum end indicator
        beq     t0, 25f                 // if eq, no quantum end request
        stl     zero, PbQuantumEnd(s0)  // clear quantum end indicator
        bsr     ra, KiQuantumEnd        // process quantum end request
        beq     v0, 50f                 // if eq, no next thread, return
        bis     v0, zero, s2            // set next thread
        br      zero, 40f               // else restore interrupts and return

//
// Determine if a new thread has been selected for execution on
// this processor.
//

25:     LDP     v0, PbNextThread(s0)    // get address of next thread object
        beq     v0, 50f                 // if eq, no new thread selected

//
// Lock dispatcher database and reread address of next thread object
//      since it is possible for it to change in mp sysytem.
//
// N.B. This is a very special acquire of the dispatcher lock in that it
//      will not be acquired unless it is free. Therefore, it is known
//      that there cannot be any queued lock requests.
//

#if !defined(NT_UP)

        lda     s1, KiDispatcherLock    // get dispatcher base lock address
        ldil    s2, LockQueueDispatcherLock * 2 // compute per processor
        SPADDP  s2, s0, s2              // lock queue entry address
        lda     s2, PbLockQueue(s2)     //

#endif

30:     ldl     a0, KiSynchIrql         // raise IRQL to synch level

        SWAP_IRQL                       //

#if !defined(NT_UP)

        LDP_L   t0, 0(s1)               // get current lock value
        bis     s2, zero, t1            // t1 = lock ownership value
        bne     t0, 45f                 // ne => spin lock owned
        STP_C   t1, 0(s1)               // set lock to owned
        beq     t1, 45f                 // if eq, conditional store failed
        mb                              // synchronize memory access
        bis     s1, LOCK_QUEUE_OWNER, t0 // set lock owner bit in lock entry
        STP     t0, LqLock(s2)          //

#endif

//
// Reready current thread for execution and swap context to the selected thread.
//

        LDP     s2, PbNextThread(s0)    // get addr of next thread

40:     GET_CURRENT_THREAD              // get current thread address

        bis     v0, zero, s1            // save current thread address

        STP     zero, PbNextThread(s0)  // clear address of next thread
        STP     s2, PbCurrentThread(s0) // set address of current thread
        bis     s1, zero, a0            // set address of previous thread
        bsr     ra, KiReadyThread       // reready thread for execution
        bsr     ra, KiSaveVolatileFloatState // save floating state
        bsr     ra, SwapContext         // swap context

//
// Restore the saved integer registers that were changed for a context
// switch only.
//
// N.B. - The frame pointer must be restored before the volatile floating
//        state because it is the pointer to the trap frame.
//

        ldq     s2, ExIntS2(sp)         // restore s2 - s5
        ldq     s3, ExIntS3(sp)         //
        ldq     s4, ExIntS4(sp)         //
        ldq     s5, ExIntS5(sp)         //
        ldq     fp, ExIntFp(sp)         // restore the frame pointer
        bsr     ra, KiRestoreVolatileFloatState // restore floating state

//
// Restore the remaining saved integer registers and return.
//

50:     ldq     s0, ExIntS0(sp)         // restore s0 - s1
        ldq     s1, ExIntS1(sp)         //
        ldq     ra, ExIntRa(sp)         // get return address
        lda     sp, ExceptionFrameLength(sp) // deallocate context frame
        ret     zero, (ra)              // return

//
// Dispatcher lock is owned, spin on both the the dispatcher lock and
// the DPC queue going not empty.
//

#if !defined(NT_UP)

45:     bis     v0, zero, a0            // lower IRQL to wait for locks

        SWAP_IRQL                       //

48:     LDP     t0, 0(s1)               // read current dispatcher lock value
        beq     t0, 30b                 // lock available.  retry spinlock
        ldl     t1, PbDpcQueueDepth(s0) // get current DPC queue depth
        bne     t1, PollDpcList         // if ne, list not empty
        br      zero, 48b               // loop in cache until lock available

#endif

        .end   KiDispatchInterrupt

        SBTTL("Swap Context to Next Thread")
//++
//
// Routine Description:
//
//    This routine is called to swap context from one thread to the next.
//
// Arguments:
//
//    s0 - Address of Processor Control Block (PRCB).
//    s1 - Address of previous thread object.
//    s2 - Address of next thread object.
//    sp - Pointer to a exception frame.
//
// Return value:
//
//    v0 - complement of Kernel APC pending.
//    s2 - Address of current thread object.
//
//--

        NESTED_ENTRY(SwapContext, 0, zero)

        stq     ra, ExSwapReturn(sp)    // save return address

        PROLOGUE_END

//
// Set new thread's state to running. Note this must be done
// under the dispatcher lock so that KiSetPriorityThread sees
// the correct state.
//

        ldil    t0, Running             // set state of new thread to running
        StoreByte( t0, ThState(s2) )    //

//
// Acquire the context swap lock so the address space of the old thread
// cannot be deleted and then release the dispatcher database lock.
//
// N.B. This lock is used to protect the address space until the context
//    switch has sufficiently progressed to the point where the address
//    space is no longer needed. This lock is also acquired by the reaper
//    thread before it finishes thread termination.
//

#if !defined(NT_UP)

        ldil    a0, LockQueueContextSwapLock * 2 // compute per processor
        SPADDP  a0, s0, a0              // lock queue entry address
        lda     a0, PbLockQueue(a0)     //
        bsr     ra, KiAcquireQueuedSpinLock // acquire context swap lock
        ldil    a0, LockQueueDispatcherLock * 2 // compute per processor
        SPADDP  a0, s0, a0              // lock queue entry address
        lda     a0, PbLockQueue(a0)     //
        bsr     ra, KiReleaseQueuedSpinLock // release dispatcher lock

#endif

//
// Accumulate the total time spent in a thread.
//

#if defined(PERF_DATA)

        bis     zero,zero,a0            // optional frequency not required
        bsr     ra, KeQueryPerformanceCounter // 64-bit cycle count in v0
        ldq     t0, PbStartCount(s0)    // get starting cycle count
        stq     v0, PbStartCount(s0)    // set starting cycle count
        ldl     t1, EtPerformanceCountHigh(s1) // get accumulated cycle count high
        sll     t1, 32, t2              //
        ldl     t3, EtPerformanceCountLow(s1) // get accumulated cycle count low
        zap     t3, 0xf0, t4            // zero out high dword sign extension
        bis     t2, t4, t3              //
        subq    v0, t0, t5              // compute elapsed cycle count
        addq    t5, t3, t4              // compute new cycle count
        stl     t4, EtPerformanceCountLow(s1) // set new cycle count in thread
        srl     t4, 32, t2              //
        stl     t2, EtPerformanceCountHigh(s1) //

#endif

        bsr     ra, KiSaveNonVolatileFloatState // save floating state

//
// The following entry point is used to switch from the idle thread to
// another thread.
//

        ALTERNATE_ENTRY(SwapFromIdle)

//
// Check if an attempt is being made to swap context while executing a DPC.
//

        ldl     v0, PbDpcRoutineActive(s0) // get DPC routine active flag
        beq     v0, 10f                 //
        ldil    a0, ATTEMPTED_SWITCH_FROM_DPC // set bug check code
        bsr     ra, KeBugCheck          // call bug check routine

//
// Get address of old and new process objects.
//

10:     LDP     s5, ThApcState + AsProcess(s1) // get address of old process
        LDP     s4, ThApcState + AsProcess(s2) // get address of new process

//
// Save the current PSR in the context frame, store the kernel stack pointer
// in the previous thread object, load the new kernel stack pointer from the
// new thread object, load the ptes for the new kernel stack in the DTB
// stack, select and new process id and swap to the new process, and restore
// the previous PSR from the context frame.
//

        DISABLE_INTERRUPTS              // disable interrupts

        LDP     a0, ThInitialStack(s2)  // get initial kernel stack pointer
        STP     sp, ThKernelStack(s1)   // save old kernel stack pointer
        bis     s2, zero, a1            // new thread address
        LDP     a2, ThTeb(s2)           // get address of user TEB

//
// On uni-processor systems keep the global current thread address
// up to date.
//

#ifdef  NT_UP

        STP     a1, KiCurrentThread     // save new current thread

#endif  //NT_UP

//
// If the old process is the same as the new process, then there is no need
// to change the address space.  The a3 parameter indicates that the address
// space is not to be swapped if it is less than zero.  Otherwise, a3 will
// contain the pfn of the PDR for the new address space.
//

        ldil    a3, -1                  // assume no address space change
        cmpeq   s5, s4, t0              // old process = new process?
        bne     t0, 50f                 // if ne, no address space swap

//
// Update the processor set masks. Clear the processor set member number in
// the old process and set the processor member number in the new process.
//

#if !defined(NT_UP)

        ldl     t0, PbSetMember(s0)     // get processor set member mask
        ldq     t1, PrActiveProcessors(s5) // get old processor sets
        ldq     t2, PrActiveProcessors(s4) // get new processor sets
        bic     t1, t0, t3              // clear member in old active set
        bis     t2, t0, t4              // set member in new active set
        sll     t0, 32, t5              // set member in new run on set
        bis     t4, t5, t4              //
        stq     t3, PrActiveProcessors(s5)  // set old processor sets
        stq     t4, PrActiveProcessors(s4)  // set new processor sets

#endif

        LDP     a3, PrDirectoryTableBase(s4) // get page directory PDE
        srl     a3, PTE_PFN, a3         // isolate page frame number

//
// If the maximum address space number is zero, then force a TB invalidate.
//

        ldl     a4, KiMaximumAsn        // get maximum ASN number
        bis     zero, 1, a5             // set ASN wrap indicator
        beq     a4, 50f                 // if eq, only one ASN
        bis     a4, zero, t3            // save maximum ASN number

//
// Check if a pending TB invalidate is pending on the current processor.
//

#if !defined(NT_UP)

        lda     t8, KiTbiaFlushRequest  // get TBIA flush request mask address
        ldl     t1, 0(t8)               // get TBIA flush request mask
        and     t1, t0, t2              // check if current processor request
        beq     t2, 20f                 // if eq, no pending flush request
        bic     t1, t0, t1              // clear processor member in mask
        stl     t1, 0(t8)               // set TBIA flush request mask

#endif

//
// If the process sequence number matches the master sequence number then
// use the process ASN. Otherwise, allocate a new ASN and check for wrap.
// If ASN wrap occurs, then also increment the master sequence number.
//

20:     lda     t9, KiMasterSequence    // get master sequence number address
        ldq     t4, 0(t9)               // get master sequence number
        ldq     t5, PrProcessSequence(s4) // get process sequence number
        ldl     a4, PrProcessAsn(s4)    // get process ASN
        xor     t4, t5, a5              // check if sequence number matches
        beq     a5, 40f                 // if eq, sequence number match
        lda     t10, KiMasterAsn        // get master ASN number address
        ldl     a4, 0(t10)              // get master ASN number
        addl    a4, 1, a4               // increment master ASN number
        cmpult  t3, a4, a5              // check for ASN wrap
        beq     a5, 30f                 // if eq, ASN did not wrap
        addq    t4, 1, t4               // increment master sequence number
        stq     t4, 0(t9)               // set master sequence number

#if !defined(NT_UP)

        ldl     t5, KeActiveProcessors  // get active processor mask
        bic     t5, t0, t5              // clear current processor member
        stl     t5, 0(t8)               // request flush on other processors

#endif

        bis     zero, zero, a4          // reset master ASN
30:     stl     a4, 0(t10)              // set master ASN number
        stl     a4, PrProcessAsn(s4)    // set process ASN number
        stq     t4, PrProcessSequence(s4) // set process sequence number

#if !defined(NT_UP)

        ldq     t5, PrActiveProcessors(s4) // get new processor sets
        zapnot  t5, 0xf, t5             // clear run on processor set
        sll     t5, 32, t3              // set run on set equal to active set
        bis     t5, t3, t5              //
        stq     t5, PrActiveProcessors(s4) // set new processor sets

#endif

//
// Merge TBIA flush request with ASN wrap indicator.
//

40:                                     //

#if !defined(NT_UP)

        bis     t2, a5, a5              // merge TBIA indicators

#endif

//
// a0 = initial ksp of new thread
// a1 = new thread address
// a2 = new TEB
// a3 = PDR of new address space or -1
// a4 = new ASN
// a5 = ASN wrap indicator
//

50:     SWAP_THREAD_CONTEXT             // swap thread

        LDP     sp, ThKernelStack(s2)   // get new kernel stack pointer

        ENABLE_INTERRUPTS               // enable interrupts

//
// Release the context swap lock.
//

#if !defined(NT_UP)

        ldil    a0, LockQueueContextSwapLock * 2 // compute per processor
        SPADDP  a0, s0, a0              // lock queue entry address
        lda     a0, PbLockQueue(a0)     //
        bsr     ra, KiReleaseQueuedSpinLock // release context swap lock

#endif

//
// If the new thread has a kernel mode APC pending, then request an APC
// interrupt.
//

        ldil    v0, 1                   // set no apc pending
        LoadByte(t0, ThApcState + AsKernelApcPending(s2)) // get kernel APC pendng
        ldl     t2, ExPsr(sp)           // get previous processor status
        beq     t0, 50f                 // if eq no apc pending
        ldil    a0, APC_INTERRUPT       // set APC level value

        REQUEST_SOFTWARE_INTERRUPT      // request an apc interrupt

        bis     zero, zero, v0          // set APC pending

//
// Count number of context switches.
//

50:     ldl     t1, PbContextSwitches(s0) // increment number of switches
        addl    t1, 1, t1               //
        stl     t1, PbContextSwitches(s0) //
        ldl     t0, ThContextSwitches(s2) // increment thread switches
        addl    t0, 1, t0               //
        stl     t0, ThContextSwitches(s2) //

//
// Restore the nonvolatile floating state.
//

        bsr     ra, KiRestoreNonVolatileFloatState //

//
// load RA and return with address of current thread in s2
//

        ldq     ra, ExSwapReturn(sp)    // get return address
        ret     zero, (ra)              // return

        .end    SwapContext

        SBTTL("Swap Process")
//++
//
// BOOLEAN
// KiSwapProcess (
//    IN PKPROCESS NewProcess
//    IN PKPROCESS OldProcess
//    )
//
// Routine Description:
//
//    This function swaps the address space from one process to another by
//    assigning a new ASN if necessary and calling the palcode to swap
//    the privileged portion of the process context (the page directory
//    base pointer and the ASN).  This function also maintains the processor
//    set for both processes in the switch.
//
// Arguments:
//
//    NewProcess (a0) - Supplies a pointer to a control object of type process
//        which represents the new process to switch to.
//
//    OldProcess (a1) - Supplies a pointer to a control object of type process
//        which represents the old process to switch from..
//
// Return Value:
//
//    None.
//
//--

        .struct 0
SwA0:   .space  8                       // saved new process address
SwA1:   .space  8                       // saved old process address
SwRa:   .space  8                       // saved return address
        .space  8                       // unused
SwapFrameLength:                        // swap frame length

        NESTED_ENTRY(KiSwapProcess, SwapFrameLength, zero)

        lda     sp, -SwapFrameLength(sp) // allocate stack frame
        stq     ra, SwRa(sp)            // save return address

        PROLOGUE_END

//
// Acquire the context swap lock, clear the processor set member in he old
// process, set the processor member in the new process, and release the
// context swap lock.
//

#if !defined(NT_UP)

        stq     a0, SwA0(sp)            // save new process address
        stq     a1, SwA1(sp)            // save old process address
        ldil    a0, LockQueueContextSwapLock // set lock queue number
        bsr     ra, KeAcquireQueuedSpinLock // acquire context swap lock
        bis     v0, zero, t6            // save old IRQL

        GET_PROCESSOR_CONTROL_REGION_BASE // get PCR address

        ldq     a3, SwA0(sp)            // restore new process address
        ldq     a1, SwA1(sp)            // restore old process address
        ldl     t0, PcSetMember(v0)     // get processor set member mask
        ldq     t1, PrActiveProcessors(a1) // get old processor sets
        ldq     t2, PrActiveProcessors(a3) // get new processor sets
        bic     t1, t0, t3              // clear member in old active set
        bis     t2, t0, t4              // set member in new active set
        sll     t0, 32, t5              // set member in new run on set
        bis     t4, t5, t4              //
        stq     t3, PrActiveProcessors(a1) // set old processor sets
        stq     t4, PrActiveProcessors(a3) // set new processor sets

#else

        bis     a0, zero, a3            // copy new process address

#endif

        LDP     a0, PrDirectoryTableBase(a3) // get page directory PDE
        srl     a0, PTE_PFN, a0         // isloate page frame number

//
// If the maximum address space number is zero, then assign ASN zero to
// the new process.
//

        ldl     a1, KiMaximumAsn        // get maximum ASN number
        ldil    a2, TRUE                // set ASN wrap indicator
        beq     a1, 40f                 // if eq, only one ASN
        bis     a1, zero, t3            // save maximum ASN number

//
// Check if a pending TB invalidate all is pending on the current processor.
//

#if !defined(NT_UP)

        lda     t8, KiTbiaFlushRequest  // get TBIA flush request mask address
        ldl     t1, 0(t8)               // get TBIA flush request mask
        and     t1, t0, t2              // check if current processor request
        beq     t2, 10f                 // if eq, no pending flush request
        bic     t1, t0, t1              // clear processor member in mask
        stl     t1, 0(t8)               // set TBIA flush request mask

#endif

//
// If the process sequence number matches the master sequence number then
// use the process ASN. Otherwise, allocate a new ASN and check for wrap.
// If ASN wrap occurs, then also increment the master sequence number.
//

10:     lda     t9, KiMasterSequence    // get master sequence number address
        ldq     t4, 0(t9)               // get master sequence number
        ldq     t5, PrProcessSequence(a3) // get process sequence number
        ldl     a1, PrProcessAsn(a3)    // get process ASN
        xor     t4, t5, a2              // check if sequence number matches
        beq     a2, 30f                 // if eq, sequence number match
        lda     t10, KiMasterAsn        // get master ASN number address
        ldl     a1, 0(t10)              // get master ASN number
        addl    a1, 1, a1               // increment master ASN number
        cmpult  t3, a1, a2              // check for ASN wrap
        beq     a2, 20f                 // if eq, ASN did not wrap
        addq    t4, 1, t4               // increment master sequence number
        stq     t4, 0(t9)               // set master sequence number

#if !defined(NT_UP)

        ldl     t5, KeActiveProcessors  // get active processor mask
        bic     t5, t0, t5              // clear current processor member
        stl     t5, 0(t8)               // request flush on other processors

#endif

        bis     zero, zero, a1          // reset master ASN
20:     stl     a1, 0(t10)              // set master ASN number
        stl     a1, PrProcessAsn(a3)    // set process ASN number
        stq     t4, PrProcessSequence(a3) // set process sequence number

#if !defined(NT_UP)

        ldq     t5, PrActiveProcessors(a3) // get new processor sets
        zapnot  t5, 0xf, t5             // clear run on processor set
        sll     t5, 32, t3              // set run on set equal to active set
        bis     t5, t3, t5              //
        stq     t5, PrActiveProcessors(a3) // set new processor sets

#endif

//
// Merge TBIA flush request with ASN wrap indicator.
//

30:                                     //

#if !defined(NT_UP)

        bis     t2, a2, a2              // merge TBIA indicators

#endif

//
// a0 = pfn of new page directory base
// a1 = new address space number
// a2 = tbiap indicator
//

40:     SWAP_PROCESS_CONTEXT            // swap address space

//
// Release context swap lock.
//

#if !defined(NT_UP)

        ldil    a0, LockQueueContextSwapLock // set lock queue number
        bis     t6, zero, a1            // set old IRQL value
        bsr     ra, KeReleaseQueuedSpinLock // release dispatcher lock

#endif

        ldq     ra, SwRa(sp)            // restore return address
        lda     sp, SwapFrameLength(sp) // deallocate stack frame
        ret     zero, (ra)              // return

        .end    KiSwapProcess
