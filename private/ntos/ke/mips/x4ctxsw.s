//      TITLE("Context Swap")
//++
//
// Copyright (c) 1991 - 1993 Microsoft Corporation
//
// Module Name:
//
//    x4ctxswap.s
//
// Abstract:
//
//    This module implements the MIPS machine dependent code necessary to
//    field the dispatch interrupt and to perform kernel initiated context
//    switching.
//
// Author:
//
//    David N. Cutler (davec) 1-Apr-1991
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"
//#define _COLLECT_SWITCH_DATA_ 1

//
// Define external variables that can be addressed using GP.
//

        .extern KeNumberProcessIds 4
        .extern KeTickCount        3 * 4
        .extern KiContextSwapLock  4
        .extern KiDispatcherLock   4
        .extern KiIdleSummary      4
        .extern KiReadySummary     4
        .extern KiSynchIrql        4
        .extern KiWaitInListHead   2 * 4
        .extern KiWaitOutListHead  2 * 4

        SBTTL("Switch To Thread")
//++
//
// NTSTATUS
// KiSwitchToThread (
//    IN PKTHREAD NextThread
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

        NESTED_ENTRY(KiSwitchToThread, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate context frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s2
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //

        PROLOGUE_END

//
// Save the wait reason, the wait mode, and the wait object address.
//

        sw      a1,ExceptionFrameLength + (1 * 4)(sp) // save wait reason
        sw      a2,ExceptionFrameLength + (2 * 4)(sp) // save wait mode
        sw      a3,ExceptionFrameLength + (3 * 4)(sp) // save wait object address

//
// If the target thread's kernel stack is resident, the target thread's
// process is in the balance set, the target thread can can run on the
// current processor, and another thread has not already been selected
// to run on the current processor, then do a direct dispatch to the
// target thread bypassing all the general wait logic, thread priorities
// permiting.
//

        lw      t9,ThApcState + AsProcess(a0) // get target process address
        lbu     v0,ThKernelStackResident(a0) // get kernel stack resident
        lw      s0,KiPcr + PcPrcb(zero)   // get address of PRCB
        lbu     v1,PrState(t9)          // get target process state
        lw      s1,KiPcr + PcCurrentThread(zero) // get current thread address
        beq     zero,v0,LongWay         // if eq, kernel stack not resident
        xor     v1,v1,ProcessInMemory   // check if process in memory
        move    s2,a0                   // set target thread address
        bne     zero,v1,LongWay         // if ne, process not in memory

#if !defined(NT_UP)

        lw      t0,PbNextThread(s0)     // get address of next thread
        lbu     t1,ThNextProcessor(s1)  // get current processor number
        lw      t2,ThAffinity(s2)       // get target thread affinity
        lw      t3,KiPcr + PcSetMember(zero) // get processor set member
        bne     zero,t0,LongWay         // if ne, next thread selected
        and     t3,t3,t2                // check if for compatible affinity
        beq     zero,t3,LongWay         // if eq, affinity not compatible

#endif

//
// Compute the new thread priority.
//

        lbu     t4,ThPriority(s1)       // get client thread priority
        lbu     t5,ThPriority(s2)       // get server thread priority
        sltu    v0,t4,LOW_REALTIME_PRIORITY // check if realtime client
        sltu    v1,t5,LOW_REALTIME_PRIORITY // check if realtime server
        beq     zero,v0,60f             // if eq, realtime client
        lbu     t6,ThPriorityDecrement(s2) // get priority decrement value
        lbu     t7,ThBasePriority(s2)   // get client base priority
        beq     zero,v1,50f             // if eq, realtime server
        addu    t8,t7,1                 // computed boosted priority
        bne     zero,t6,30f             // if ne, server boost active

//
// Both the client and the server are not realtime and a priority boost
// is not currently active for the server. Under these conditions an
// optimal switch to the server can be performed if the base priority
// of the server is above a minimum threshold or the boosted priority
// of the server is not less than the client priority.
//

        sltu    v0,t8,t4                // check if high enough boost
        sltu    v1,t8,LOW_REALTIME_PRIORITY // check if less than realtime
        bne     zero,v0,20f             // if ne, boosted priority less
        sb      t8,ThPriority(s2)       // asssume boosted priority is okay
        bne     zero,v1,70f             // if ne, less than realtime
        li      t8,LOW_REALTIME_PRIORITY - 1 // set high server priority
        sb      t8,ThPriority(s2)       //
        b       70f                     //

//
// The boosted priority of the server is less than the current priority of
// the client. If the server base priority is above the required threshold,
// then a optimal switch to the server can be performed by temporarily
// raising the priority of the server to that of the client.
//

20:     sltu    v0,t7,BASE_PRIORITY_THRESHOLD // check if above threshold
        subu    t8,t4,t7                // compute priority decrement value
        bne     zero,v0,LongWay         // if ne, priority below threshold
        li      t7,ROUND_TRIP_DECREMENT_COUNT // get system decrement count value
        sb      t8,ThPriorityDecrement(s2) // set priority decrement value
        sb      t4,ThPriority(s2)       // set current server priority
        sb      t7,ThDecrementCount(s2) // set server decrement count
        b       70f                     //

//
// A server boost has previously been applied to the server thread. Count
// down the decrement count to determine if another optimal server switch
// is allowed.
//

30:     lbu     t8,ThDecrementCount(s2) // decrement server count value
        subu    t8,t8,1                 //
        sb      t8,ThDecrementCount(s2) // store updated decrement count
        beq     zero,t8,40f             // if eq, no more switches allowed

//
// Another optimal switch to the server is allowed provided that the
// server priority is not less than the client priority.
//

        sltu    v0,t5,t4                // check if server lower priority
        beq     zero,v0,70f             // if eq, server not lower priority
        b       LongWay                 //

//
// The server has exhausted the number of times an optimal switch may
// be performed without reducing it priority. Reduce the priority of
// the server to its original unboosted value minus one.
//

40:     sb      zero,ThPriorityDecrement(s2) // clear server priority decrement
        sb      t7,ThPriority(s2)       // set server priority to base
        b       LongWay                 //

//
// The client is not realtime and the server is realtime. An optimal switch
// to the server can be performed.
//

50:     lb      t8,PrThreadQuantum(t9)  // get process quantum value
        b       65f                     //

//
// The client is realtime. In order for an optimal switch to occur, the
// server must also be realtime and run at a high or equal priority.
//

60:     sltu    v0,t5,t4                // check if server is lower priority
        lb      t8,PrThreadQuantum(t9)  // get process quantum value
        bne     zero,v0,LongWay         // if ne, server is lower priority
65:     sb      t8,ThQuantum(s2)        // set server thread quantum

//
// Set the next processor for the server thread.
//

70:                                     //

#if !defined(NT_UP)

        sb      t1,ThNextProcessor(s2)  // set server next processor number

#endif

//
// Set the address of the wait block list in the client thread, initialization
// the event wait block, and insert the wait block in client event wait list.
//

        addu    t0,s1,EVENT_WAIT_BLOCK_OFFSET // compute wait block address
        sw      t0,ThWaitBlockList(s1)  // set address of wait block list
        sw      zero,ThWaitStatus(s1)   // set initial wait status
        sw      a3,WbObject(t0)         // set address of wait object
        sw      t0,WbNextWaitBlock(t0)  // set next wait block address
        lui     t1,WaitAny              // get wait type and wait key
        sw      t1,WbWaitKey(t0)        // set wait key and wait type
        addu    t1,a3,EvWaitListHead    // compute wait object listhead address
        lw      t2,LsBlink(t1)          // get backward link of listhead
        addu    t3,t0,WbWaitListEntry   // compute wait block list entry address
        sw      t3,LsBlink(t1)          // set backward link of listhead
        sw      t3,LsFlink(t2)          // set forward link in last entry
        sw      t1,LsFlink(t3)          // set forward link in wait entry
        sw      t2,LsBlink(t3)          // set backward link in wait entry

//
// Set the client thread wait parameters, set the thread state to Waiting,
// and insert the thread in the proper wait list.
//

        sb      zero,ThAlertable(s1)    // set alertable FALSE.
        sb      a1,ThWaitReason(s1)     //
        sb      a2,ThWaitMode(s1)       // set the wait mode
        lb      a3,ThEnableStackSwap(s1) // get kernel stack swap enable
        lw      t1,KeTickCount + 0      // get low part of tick count
        sw      t1,ThWaitTime(s1)       // set thread wait time
        li      t0,Waiting              // set thread state
        sb      t0,ThState(s1)          //
        la      t1,KiWaitInListHead     // get address of wait in listhead
        beq     zero,a2,75f             // if eq, wait mode is kernel
        beq     zero,a3,75f             // if eq, kernel stack swap disabled
        sltu    t0,t4,LOW_REALTIME_PRIORITY + 9 // check if priority in range
        bne     zero,t0,76f             // if ne, thread priority in range
75:     la      t1,KiWaitOutListHead     // get address of wait out listhead
76:     lw      t2,LsBlink(t1)          // get backlink of wait listhead
        addu    t3,s1,ThWaitListEntry   // compute wait list entry address
        sw      t3,LsBlink(t1)          // set backward link of listhead
        sw      t3,LsFlink(t2)          // set forward link in last entry
        sw      t1,LsFlink(t3)          // set forward link in wait entry
        sw      t2,LsBlink(t3)          // set backward link in wait entry

//
// If the current thread is processing a queue entry, then attempt to
// activate another thread that is blocked on the queue object.
//
// N.B. The next thread address can change if the routine to activate
//      a queue waiter is called.
//

77:     lw      a0,ThQueue(s1)          // get queue object address
        beq     zero,a0,78f             // if eq, no queue object attached
        sw      s2,PbNextThread(s0)     // set next thread address
        jal     KiActivateWaiterQueue   // attempt to activate a blocked thread
        lw      s2,PbNextThread(s0)     // get next thread address
        sw      zero,PbNextThread(s0)   // set next thread address to NULL
78:     sw      s2,PbCurrentThread(s0)  // set address of current thread object
        jal     SwapContext             // swap context

//
// Lower IRQL to its previous level.
//
// N.B. SwapContext releases the dispatcher database lock.
//
// N.B. The register s2 contains the address of the new thread on return.
//

        lw      v0,ThWaitStatus(s2)     // get wait completion status
        lbu     a0,ThWaitIrql(s2)       // get original IRQL
        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

//
// If the wait was not interrupted to deliver a kernel APC, then return the
// completion status.
//

        xor     v1,v0,STATUS_KERNEL_APC // check if awakened for kernel APC
        bne     zero,v1,90f             // if ne, normal wait completion

//
// Disable interrupts an attempt to acquire the dispatcher database lock.
//

        lw      s1,KiPcr + PcCurrentThread(zero) // get current thread address
        lbu	s2,KiSynchIrql          // get new IRQL level

79:     DISABLE_INTERRUPTS(t4)          // disable interrupts

#if !defined(NT_UP)

80:     ll      t0,KiDispatcherLock     // get current lock value
        move    t1,s1                   // set ownership value
        bne     zero,t0,85f             // if ne, spin lock owned
        sc      t1,KiDispatcherLock     // set spin lock owned
        beq     zero,t1,80b             // if eq, store conditional failure

#endif

//
// Raise IRQL to synchronization level and save wait IRQL.
//
// N.B. The raise IRQL code is duplicated here to avoid any extra overhead
//      since this is such a common operation.
//

        lbu     t1,KiPcr + PcIrqlTable(s2) // get translation table entry value
        li      t2,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t1,t1,PSR_INTMASK       // shift table entry into position
        lbu     t3,KiPcr + PcCurrentIrql(zero) // get current IRQL
        and     t4,t4,t2                // clear current interrupt enables
        or      t4,t4,t1                // set new interrupt enables
        sb      s2,KiPcr + PcCurrentIrql(zero) // set new IRQL level

        ENABLE_INTERRUPTS(t4)           // enable interrupts

        sb      t3,ThWaitIrql(s1)       // set client wait IRQL
        b       ContinueWait            //

#if !defined(NT_UP)

85:     ENABLE_INTERRUPTS(t4)           // enable interrupts

        b       79b                     // try again

#endif

//
// Ready the target thread for execution and wait on the specified wait
// object.
//

LongWay:                                //
        jal     KiReadyThread           // ready thread for execution

//
// Continue the and return the wait completion status.
//
// N.B. The wait continuation routine is called with the dispatcher
//      database locked.
//

ContinueWait:                           //
        lw      a0,ExceptionFrameLength + (3 * 4)(sp) // get wait object address
        lw      a1,ExceptionFrameLength + (1 * 4)(sp) // get wait reason
        lw      a2,ExceptionFrameLength + (2 * 4)(sp) // get wait mode
        jal     KiContinueClientWait    // continue client wait
90:     lw      s0,ExIntS0(sp)          // restore register s0 - s2
        lw      s1,ExIntS1(sp)          //
        lw      s2,ExIntS2(sp)          //
        lw      ra,ExIntRa(sp)          // get return address
        addu    sp,sp,ExceptionFrameLength // deallocate context frame
        j       ra                      // return

        .end    KiSwitchToThread

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
//        switched to a nested fucntion.
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
// Check if a thread has been scheduled to execute on the current processor.
//

        lw      t0,KiPcr + PcPrcb(zero) // get address of PRCB
        and     a0,a0,0xff              // isolate old IRQL
        sltu    t1,a0,DISPATCH_LEVEL    // check if IRQL below dispatch level
        lw      t2,PbNextThread(t0)     // get next thread address
        bne     zero,t2,30f             // if ne, a new thread selected

//
// A new thread has not been selected to run on the current processor.
// Release the dispatcher database lock and restore IRQL to its previous
// level.
//

10:                                     //

#if !defined(NT_UP)

        sw      zero,KiDispatcherLock   // set spin lock not owned

#endif

        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

//
// A new thread has been selected to run on the current processor, but
// the new IRQL is not below dispatch level. If the current processor is
// not executing a DPC, then request a dispatch interrupt on the current
// processor before releasing the dispatcher lock and restoring IRQL.
//


20:     bne     zero,t3,10b             // if ne, DPC routine active

#if !defined(NT_UP)

        sw      zero,KiDispatcherLock   // set spin lock not owned

#endif

        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t3,cause                // get exception cause register
        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        or      t3,t3,DISPATCH_INTERRUPT // set dispatch interrupt request
        mtc0    t3,cause                // set exception cause register
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

//
// A new thread has been selected to run on the current processor.
//
// If the new IRQL is less than dispatch level, then switch to the new
// thread.
//
// N.B. the jump to the switch to the next thread is required.
//

30:     lw      t3,PbDpcRoutineActive(t0) // get DPC active flag
        beq     zero,t1,20b             // if eq, IRQL not below dispatch
        j       KxUnlockDispatcherDatabase //

        .end    KiUnlockDispatcherDataBase

//
// N.B. This routine is carefully written as a nested function. Control
//      drops into this function from above.
//

        NESTED_ENTRY(KxUnlockDispatcherDatabase, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate context frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s2
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //

        PROLOGUE_END

        move    s0,t0                   // set address of PRCB
        lw      s1,KiPcr + PcCurrentThread(zero) // get current thread address
        move    s2,t2                   // set next thread address
        sb      a0,ThWaitIrql(s1)       // save previous IRQL
        sw      zero,PbNextThread(s0)   // clear next thread address

//
// Reready current thread for execution and swap context to the selected
// thread.
//
// N.B. The return from the call to swap context is directly to the swap
//      thread exit.
//

        move    a0,s1                   // set address of previous thread object
        sw      s2,PbCurrentThread(s0)  // set address of current thread object
        jal     KiReadyThread           // reready thread for execution
        la      ra,KiSwapThreadExit     // set return address
        j       SwapContext             // swap context

        .end    KxUnlockDispatcherDatabase

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
//    This routine is called to select and the next thread to run on the
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

        subu    sp,sp,ExceptionFrameLength // allocate context frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s2
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //

        PROLOGUE_END

        .set    noreorder
        .set    noat
        lw      s0,KiPcr + PcPrcb(zero)   // get address of PRCB
        lw      t0,KiReadySummary       // get ready summary
        lw      s1,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      s2,PbNextThread(s0)     // get address of next thread

#if !defined(NT_UP)

        lw      t1,KiPcr + PcSetMember(zero) // get processor affinity mask
        lbu     v0,PbNumber(s0)         // get current processor number
        lw      v1,KeTickCount + 0      // get low part of tick count

#endif

        srl     t3,t0,16                // isolate bits <31:16> of summary
        li      t2,16                   // set base bit number
        bnel    zero,s2,120f            // if ne, next thread selected
        sw      zero,PbNextThread(s0)   // zero address of next thread

//
// Find the highest nibble in the ready summary that contains a set bit
// and left justify so the nibble is in bits <31:28>.
//

        bne     zero,t3,10f             // if ne, bits <31:16> are nonzero
        srl     t3,t3,8                 // isolate bits <31:24> of summary
        li      t2,0                    // set base bit number
        srl     t3,t0,8                 // isolate bits <15:8> of summary
10:     bnel    zero,t3,20f             // if ne, bits <15:8> are nonzero
        addu    t2,t2,8                 // add bit offset to nonzero byte
20:     srl     t3,t0,t2                // isolate highest nonzero byte
        addu    t2,t2,3                 // adjust to high bit in nibble
        sltu    t4,t3,0x10              // check if high nibble nonzero
        xor     t4,t4,1                 // complement less than indicator
        sll     t4,t4,2                 // multiply by nibble width
        addu    t2,t2,t4                // compute ready queue priority
        la      t3,KiDispatcherReadyListHead // get ready listhead base address
        nor     t4,t2,zero              // compute left justify shift count
        sll     t4,t0,t4                // left justify ready summary to nibble

//
// If the next bit is set in the ready summary, then scan the corresponding
// dispatcher ready queue.
//

30:     bltz    t4,50f                  // if ltz, queue contains an entry
        sll     t4,t4,1                 // position next ready summary bit
        bne     zero,t4,30b             // if ne, more queues to scan
        subu    t2,t2,1                 // decrement ready queue priority

//
// All ready queues were scanned without finding a runnable thread so
// default to the idle thread and set the appropriate bit in idle summary.
//

40:                                     //

#if defined(_COLLECT_SWITCH_DATA_)

        la      t0,KeThreadSwitchCounters // get switch counters address
        lw      v0,TwSwitchToIdle(t0)   // increment switch to idle count
        addu    v0,v0,1                 //
        sw      v0,TwSwitchToIdle(t0)   //

#endif

#if defined(NT_UP)

        li      t0,1                    // get current idle summary
#else

        lw      t0,KiIdleSummary        // get current idle summary
        or      t0,t0,t1                // set member bit in idle summary

#endif

        sw      t0,KiIdleSummary        // set new idle summary
        b       120f                    //
        lw      s2,PbIdleThread(s0)     // set address of idle thread

//
// If the thread can execute on the current processor, then remove it from
// the dispatcher ready queue.
//

50:     sll     t5,t2,3                 // compute ready listhead offset
        addu    t5,t5,t3                // compute ready queue address
        lw      t6,LsFlink(t5)          // get address of first queue entry
        subu    s2,t6,ThWaitListEntry   // compute address of thread object

#if !defined(NT_UP)

60:     lw      t7,ThAffinity(s2)       // get thread affinity
        lw      t8,ThWaitTime(s2)       // get time of thread ready
        lbu     t9,ThNextProcessor(s2)  // get last processor number
        and     t7,t7,t1                // check for compatible thread affinity
        bne     zero,t7,70f             // if ne, thread affinity compatible
        subu    t8,v1,t8                // compute length of wait
        lw      t6,LsFlink(t6)          // get address of next entry
        bne     t5,t6,60b               // if ne, not end of list
        subu    s2,t6,ThWaitListEntry   // compute address of thread object
        bne     zero,t4,30b             // if ne, more queues to scan
        subu    t2,t2,1                 // decrement ready queue priority
        b       40b                     //
        nop                             // fill

//
// If the thread last ran on the current processor, the processor is the
// ideal processor for the thread, the thread has been waiting for longer
// than a quantum, ot its priority is greater than low realtime plus 9,
// then select the thread. Otherwise, an attempt is made to find a more
// appropriate candidate.
//

70:     lbu     a0,ThIdealProcessor(s2) // get ideal processor number
        beq     v0,t9,110f              // if eq, last processor number match
        sltu    t7,t2,LOW_REALTIME_PRIORITY + 9 // check if priority in range
        beq     v0,a0,100f              // if eq, ideal processor number match
        sltu    t8,t8,READY_SKIP_QUANTUM + 1 // check if wait time exceeded
        and     t8,t8,t7                // check if priority and time match
        beql    zero,t8,110f            // if eq, priority or time mismatch
        sb      v0,ThNextProcessor(s2)  // set next processor number

//
// Search forward in the ready queue until the end of the list is reached
// or a more appropriate thread is found.
//

        lw      t7,LsFlink(t6)          // get address of next entry
80:     beq     t5,t7,100f              // if eq, end of list
        subu    a1,t7,ThWaitListEntry   // compute address of thread object
        lw      a2,ThAffinity(a1)       // get thread affinity
        lw      t8,ThWaitTime(a1)       // get time of thread ready
        lbu     t9,ThNextProcessor(a1)  // get last processor number
        lbu     a0,ThIdealProcessor(a1) // get ideal processor number
        and     a2,a2,t1                // check for compatible thread affinity
        subu    t8,v1,t8                // compute length of wait
        beq     zero,a2,85f             // if eq, thread affinity not compatible
        sltu    t8,t8,READY_SKIP_QUANTUM + 1 // check if wait time exceeded
        beql    v0,t9,90f               // if eq, processor number match
        move    s2,a1                   // set thread address
        beql    v0,a0,90f               // if eq, processor number match
        move    s2,a1                   // set thread address
85:     bne     zero,t8,80b             // if ne, wait time not exceeded
        lw      t7,LsFlink(t7)          // get address of next entry
        b       110f                    //
        sb      v0,ThNextProcessor(s2)  // set next processor number

90:     move    t6,t7                   // set list entry address
100:    sb      v0,ThNextProcessor(s2)  // set next processor number
        .set    at
        .set    reorder

110:                                    //

#if defined(_COLLECT_SWITCH_DATA_)

        la      v1,KeThreadSwitchCounters + TwFindIdeal// get counter address
        lbu     a0,ThIdealProcessor(s2) // get ideal processor number
        lbu     t9,ThLastprocessor(s2)  // get last processor number
        beq     v0,a0,115f              // if eq, processor number match
        addu    v1,v1,TwFindLast - TwFindIdeal // compute counter address
        beq     v0,t9,115f              // if eq, processor number match
        addu    v1,v1,TwFindAny - TwFindLast // compute counter address
115:    lw      v0,0(v1)                // increment appropriate counter
        addu    v0,v0,1                 //
        sw      v0,0(v1)                //

#endif

#endif

//
// Remove the selected thread from the ready queue.
//

        lw      t7,LsFlink(t6)          // get list entry forward link
        lw      t8,LsBlink(t6)          // get list entry backward link
        li      t1,1                    // set bit for mask generation
        sw      t7,LsFlink(t8)          // set forward link in previous entry
        sw      t8,LsBlink(t7)          // set backward link in next entry
        bne     t7,t8,120f              // if ne, list is not empty
        sll     t1,t1,t2                // compute ready summary set member
        xor     t1,t1,t0                // clear ready summary bit
        sw      t1,KiReadySummary       //

//
// Swap context to the next thread.
//

        .set    noreorder
        .set    noat
120:    jal     SwapContext             // swap context
        sw      s2,PbCurrentThread(s0)  // set address of current thread object
        .set    at
        .set    reorder

//
// Lower IRQL, deallocate context frame, and return wait completion status.
//
// N.B. SwapContext releases the dispatcher database lock.
//
// N.B. The register v0 contains the kernel APC pending state on return.
//
// N.B. The register s2 contains the address of the new thread on return.
//

        ALTERNATE_ENTRY(KiSwapThreadExit)

        lw      s1,ThWaitStatus(s2)     // get wait completion status
        lbu     a0,ThWaitIrql(s2)       // get original wait IRQL
        sltu    v1,a0,APC_LEVEL         // check if wait IRQL is zero
        and     v1,v1,v0                // check if IRQL and APC pending set
        beq     zero,v1,10f             // if eq, IRQL or pending not set

//
// Lower IRQL to APC level and dispatch APC interrupt.
//

        .set    noreorder
        .set    noat
        li      a0,APC_LEVEL            // set new IRQL level
        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        li      t2,CU1_ENABLE           // get coprocessor 1 enable bits
        mfc0    t3,psr                  // get current PSR
        mtc0    t2,psr
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        mfc0    t4,cause                // get exception cause register
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        and     t4,t4,DISPATCH_INTERRUPT // clear APC interrupt pending
        mtc0    t4,cause                //
        mtc0    t3,psr                  // enable interrupts
        .set    at
        .set    reorder

        lw      t0,KiPcr + PcPrcb(zero) // get current processor block address
        lw      t1,PbApcBypassCount(t0) // increment the APC bypass count
        addu    t1,t1,1                 //
        sw      t1,PbApcBypassCount(t0) // store result
        move    a0,zero                 // set previous mode to kernel
        move    a1,zero                 // set exception frame address
        move    a2,zero                 // set trap frame addresss
        jal     KiDeliverApc            // deliver kernel mode APC
        move    a0,zero                 // set original wait IRQL

//
// Lower IRQL to wait level, set return status, restore registers, and
// return.
//

        .set    noreorder
        .set    noat
10:     lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        li      t2,CU1_ENABLE           // get coprocessor 1 enable bits
        mfc0    t3,psr                  // get current PSR
        mtc0    t2,psr
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL
        mtc0    t3,psr                  // enable interrupts
        .set    at
        .set    reorder

        move    v0,s1                   // set return status
        lw      s0,ExIntS0(sp)          // restore register s0 - s2
        lw      s1,ExIntS1(sp)          //
        lw      s2,ExIntS2(sp)          //
        lw      ra,ExIntRa(sp)          // get return address
        addu    sp,sp,ExceptionFrameLength // deallocate context frame
        j       ra                      // return

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
//    N.B. On entry to this routine all integer registers and the volatile
//         floating registers have been saved.
//
// Arguments:
//
//    s8 - Supplies a pointer to the base of a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiDispatchInterrupt, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate context frame
        sw      ra,ExIntRa(sp)          // save return address

        PROLOGUE_END

        lw      s0,KiPcr + PcPrcb(zero) // get address of PRCB

//
// Process the deferred procedure call list.
//

PollDpcList:                            //

        DISABLE_INTERRUPTS(s1)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t0,cause                // get exception cause register
        and     t0,t0,APC_INTERRUPT     // clear dispatch interrupt pending
        mtc0    t0,cause                // set exception cause register
        .set    at
        .set    reorder

        addu    a1,s0,PbDpcListHead     // compute DPC listhead address
        lw      a0,LsFlink(a1)          // get address of next entry
        beq     a0,a1,20f               // if eq, DPC list is empty

//
// Switch to interrupt stack to process the DPC list.
//

        lw      t1,KiPcr + PcInterruptStack(zero) // get interrupt stack address stack
        subu    t2,t1,ExceptionFrameLength // allocate exception frame
        sw      sp,ExIntS4(t2)          // save old stack pointer
        sw      zero,ExIntRa(t2)        // clear return address
        sw      t1,KiPcr + PcInitialStack(zero) // set initial stack address
        subu    t1,t1,KERNEL_STACK_SIZE // compute and set stack limit
        sw      t1,KiPcr + PcStackLimit(zero) //
        move    sp,t2                   // set new stack pointer
        sw      sp,KiPcr + PcOnInterruptStack(zero) // set stack indicator
        move    v0,s1                   // set previous PSR value
        jal     KiRetireDpcList         // process the DPC list

//
// Switch back to previous stack and restore the initial stack limit.
//

        lw      t1,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      t2,ThInitialStack(t1)   // get initial stack address
        lw      t3,ThStackLimit(t1)     // get stack limit
        lw      sp,ExIntS4(sp)          // restore stack pointer
        sw      t2,KiPcr + PcInitialStack(zero) // set initial stack address
        sw      t3,KiPcr + PcStackLimit(zero) // set stack limit
        sw      zero,KiPcr + PcOnInterruptStack(zero) // clear stack indicator

20:     ENABLE_INTERRUPTS(s1)           // enable interrupts

//
// Check to determine if quantum end has occured.
//
// N.B. If a new thread is selected as a result of processing a quantum
//      end request, then the new thread is returned with the dispatcher
//      database locked. Otherwise, NULL is returned with the dispatcher
//      database unlocked.
//

        lw      t0,KiPcr + PcQuantumEnd(zero) // get quantum end indicator
        bne     zero,t0,70f             // if ne, quantum end request

//
// Check to determine if a new thread has been selected for execution on
// this processor.
//

        lw      s2,PbNextThread(s0)     // get address of next thread object
        beq     zero,s2,50f             // if eq, no new thread selected

//
// Disable interrupts and attempt to acquire the dispatcher database lock.
//

        lbu     a0,KiSynchIrql          // get new IRQL value

        DISABLE_INTERRUPTS(t3)          // disable interrupts

#if !defined(NT_UP)

30:     ll      t0,KiDispatcherLock     // get current lock value
        move    t1,s2                   // set lock ownership value
        bne     zero,t0,60f             // if ne, spin lock owned
        sc      t1,KiDispatcherLock     // set spin lock owned
        beq     zero,t1,30b             // if eq, store conditional failed

#endif

//
// Raise IRQL to synchronization level.
//
// N.B. The raise IRQL code is duplicated here to avoid any extra overhead
//      since this is such a common operation.
//

        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t3)           // enable interrupts

40:     lw      s1,KiPcr + PcCurrentThread(zero) // get current thread object address
        lw      s2,PbNextThread(s0)     // get address of next thread object
        sw      zero,PbNextThread(s0)   // clear address of next thread object

//
// Reready current thread for execution and swap context to the selected thread.
//

        move    a0,s1                   // set address of previous thread object
        sw      s2,PbCurrentThread(s0)  // set address of current thread object
        jal     KiReadyThread           // reready thread for execution
        jal     SwapContext             // swap context

//
// Restore saved registers, deallocate stack frame, and return.
//

50:     lw      ra,ExIntRa(sp)          // get return address
        addu    sp,sp,ExceptionFrameLength // deallocate context frame
        j       ra                      // return

//
// Enable interrupts and check DPC queue.
//

#if !defined(NT_UP)

60:     ENABLE_INTERRUPTS(t3)           // enable interrupts

        j       PollDpcList             //

#endif

//
// Process quantum end event.
//
// N.B. If the quantum end code returns a NULL value, then no next thread
//      has been selected for execution. Otherwise, a next thread has been
//      selected and the dispatcher databased is locked.
//

70:     sw      zero,KiPcr + PcQuantumEnd(zero) // clear quantum end indicator
        jal     KiQuantumEnd            // process quantum end request
        bne     zero,v0,40b             // if ne, next thread selected
        lw      ra,ExIntRa(sp)          // get return address
        addu    sp,sp,ExceptionFrameLength // deallocate context frame
        j       ra                      // return

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
//    s0 - Address of Processor Control Block.
//    s1 - Address of previous thread object.
//    s2 - Address of next thread object.
//    sp - Pointer to a exception frame.
//
// Return value:
//
//    v0 - Kernel APC pending.
//    s0 - Address of Processor Control Block.
//    s2 - Address of current thread object.
//
//--

        NESTED_ENTRY(SwapContext, 0, zero)

//
// Set the thread state to running.
//

        li      t0,Running              // set thread state to running
        sb      t0,ThState(s2)          //

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

10:     ll      t0,KiContextSwapLock    // get current lock value
        move    t1,s2                   // set ownership value
        bne     zero,t0,10b             // if ne, lock already owned
        sc      t1,KiContextSwapLock    // set lock ownership value
        beq     zero,t1,10b             // if eq, store conditional failed
        sw      zero,KiDispatcherLock   // set lock not owned

#endif

//
// Save old thread nonvolatile context.
//

        sw      ra,ExSwapReturn(sp)     // save return address
        sw      s3,ExIntS3(sp)          // save integer registers s3 - s8.
        sw      s4,ExIntS4(sp)          //
        sw      s5,ExIntS5(sp)          //
        sw      s6,ExIntS6(sp)          //
        sw      s7,ExIntS7(sp)          //
        sw      s8,ExIntS8(sp)          //
        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f31
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

        PROLOGUE_END

//
// Accumlate the total time spent in a thread.
//

#if defined(PERF_DATA)

        addu    a0,sp,ExFltF20          // compute address of result
        move    a1,zero                 // set address of optional frequency
        jal     KeQueryPerformanceCounter // query performance counter
        lw      t0,ExFltF20(sp)         // get current cycle count
        lw      t1,ExFltF20 + 4(sp)     //
        lw      t2,PbStartCount(s0)     // get starting cycle count
        lw      t3,PbStartCount + 4(s0) //
        sw      t0,PbStartCount(s0)     // set starting cycle count
        sw      t1,PbStartCount + 4(s0) //
        lw      t4,EtPerformanceCountLow(s1) // get accumulated cycle count
        lw      t5,EtPerformanceCountHigh(s1) //
        subu    t6,t0,t2                // subtract low parts
        subu    t7,t1,t3                // subtract high parts
        sltu    v0,t0,t2                // generate borrow from high part
        subu    t7,t7,v0                // subtract borrow
        addu    t6,t6,t4                // add low parts
        addu    t7,t7,t5                // add high parts
        sltu    v0,t6,t4                // generate carry into high part
        addu    t7,t7,v0                // add carry
        sw      t6,EtPerformanceCountLow(s1)  // set accumulated cycle count
        sw      t7,EtPerformanceCountHigh(s1) //

#endif

//
// The following entry point is used to switch from the idle thread to
// another thread.
//

        ALTERNATE_ENTRY(SwapFromIdle)

#if DBG

        lw      t0,ThInitialStack(s1)   // get initial stack address
        lw      t1,ThStackLimit(s1)     // get stack limit
        sltu    t2,sp,t0                // stack within limits?
        sltu    t3,sp,t1                //
        xor     t3,t3,t2                //
        bne     zero,t3,5f              // if ne, stack within limits
        li      a0,PANIC_STACK_SWITCH   // set bug check code
        move    a1,t0                   // set initial stack address
        move    a2,t1                   // set stack limit
        move    a3,sp                   // set stack address
        jal     KeBugCheckEx            // bug check

#endif

//
// Get the old and new process object addresses.
//

5:      lw      s3,ThApcState + AsProcess(s2) // get new process address
        lw      s4,ThApcState + AsProcess(s1) // get old process address

//
// Save the processor state, swap stack pointers, and set the new stack
// limits.
//

        .set    noreorder
        .set    noat
        mfc0    s7,psr                  // save current PSR
        li      t1,CU1_ENABLE           // disable interrupts
        mtc0    t1,psr                  // 3 cycle hazzard
        lw      t2,ThInitialStack(s2)   // get new initial stack pointer
        lw      t3,ThStackLimit(s2)     // get new stack limit
        sw      sp,ThKernelStack(s1)    // save old kernel stack pointer
        lw      sp,ThKernelStack(s2)    // get new kernel stack pointer
        ld      t1,ThTeb(s2)            // get user TEB and TLS array addresses
        sw      t2,KiPcr + PcInitialStack(zero) // set initial stack pointer
        sw      t3,KiPcr + PcStackLimit(zero) // set stack limit
        sd      t1,KiPcr + PcTeb(zero)  // set user TEB and TLS array addresses

//
// If the new process is not the same as the old process, then swap the
// address space to the new process.
//
// N.B. The context swap lock cannot be dropped until all references to the
//      old process address space are complete. This includes any possible
//      TB Misses that could occur referencing the new address space while
//      still executing in the old address space.
//
// N.B. The process address space swap is executed with interrupts disabled.
//

#if defined(NT_UP)

        beq     s3,s4,20f               // if eq, old and new process match

#else

        beql    s3,s4,20f               // if eq, old and new process match
        sw      zero,KiContextSwapLock  // set spin lock not owned

//
// Update the processor set masks.
//

        lw      t0,KiPcr + PcSetMember(zero) // get processor set member
        lw      t2,PrActiveProcessors(s3) // get new active processor set
        lw      t1,PrActiveProcessors(s4) // get old active processor set
        or      t2,t2,t0                // set processor member in set
        xor     t1,t1,t0                // clear processor member in set
        sw      t2,PrActiveProcessors(s3) // set new active processor set
        sw      t1,PrActiveProcessors(s4) // set old active processor set
        sw      zero,KiContextSwapLock  // set spin lock not owned

#endif

        lw      s5,PrDirectoryTableBase(s3) // get page directory PDE
        lw      s6,PrDirectoryTableBase + 4(s3) // get hyper space PDE
        .set    at
        .set    reorder

//
// Allocate a new process PID. If the new PID number is greater than the
// number of PIDs supported on the host processor, then flush the entire
// TB and reset the PID number ot zero.
//

        lw      v1,KiPcr + PcCurrentPid(zero) // get current processor PID
        lw      t2,KeNumberProcessIds   // get number of process id's
        addu    v1,v1,1 << ENTRYHI_PID  // increment master system PID
        sltu    t2,v1,t2                // any more PIDs to allocate
        bne     zero,t2,10f             // if ne, more PIDs to allocate

//
// Flush the random part of the TB.
//

        jal     KiFlushRandomTb         // flush random part of TB
        move    v1,zero                 // set next PID value

//
// Swap address space to the specified process.
//

10:     sw      v1,KiPcr + PcCurrentPid(zero) // set current processor PID
        li      t3,PDE_BASE             // get virtual address of PDR
        or      t3,t3,v1                // merge process PID
        li      t4,PDR_ENTRY << INDEX_INDEX // set entry index for PDR

        .set    noreorder
        .set    noat
        mtc0    t3,entryhi              // set VPN2 and PID of TB entry
        mtc0    s5,entrylo0             // set first PDE value
        mtc0    s6,entrylo1             // set second PDE value
        mtc0    t4,index                // set index of PDR entry
        nop                             // 1 cycle hazzard
        tlbwi                           // write system PDR TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

//
// If the new thread has a kernel mode APC pending, then request an APC
// interrupt.
//

        .set    noreorder
        .set    noat
20:     lbu     v0,ThApcState + AsKernelApcPending(s2) // get kernel APC pending
        mfc0    t3,cause                // get cause register contents
        sll     t2,v0,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t3,t3,t2                // merge possible APC interrupt request
        mtc0    t3,cause                // write exception cause register
        mtc0    s7,psr                  // set new PSR
        .set    at
        .set    reorder

//
// Update the number of context switches for the current processor and the
// new thread and save the address of the new thread objhect in the PCR.
//

        lw      t0,PbContextSwitches(s0) // increment processor context switches
        addu    t0,t0,1                 //
        sw      t0,PbContextSwitches(s0) //
        lw      t1,ThContextSwitches(s2) // increment thread context switches
        addu    t1,t1,1                 //
        sw      t1,ThContextSwitches(s2) //
        sw      s2,KiPcr + PcCurrentThread(zero) // set address of new thread

//
// Restore new thread nonvolatile context.
//

        ldc1    f20,ExFltF20(sp)        // restore floating registers f20 - f31
        ldc1    f22,ExFltF22(sp)        //
        ldc1    f24,ExFltF24(sp)        //
        ldc1    f26,ExFltF26(sp)        //
        ldc1    f28,ExFltF28(sp)        //
        ldc1    f30,ExFltF30(sp)        //
        lw      s3,ExIntS3(sp)          // restore integer registers s3 - s8.
        lw      s4,ExIntS4(sp)          //
        lw      s5,ExIntS5(sp)          //
        lw      s6,ExIntS6(sp)          //
        lw      s7,ExIntS7(sp)          //
        lw      s8,ExIntS8(sp)          //

//
// Set address of current thread object and return.
//
// N.B. The register s2 contains the address of the new thread on return.
//

        lw      ra,ExSwapReturn(sp)     // get return address
        j       ra                      // return

        .end    SwapContext

        SBTTL("Swap Process")
//++
//
// BOOLEAN
// KiSwapProcess (
//    IN PKPROCESS NewProcess,
//    IN PKPROCESS OldProcess
//    )
//
// Routine Description:
//
//    This function swaps the address space from one process to another by
//    assigning a new process id, if necessary, and loading the fixed entry
//    in the TB that maps the process page directory page.
//
// Arguments:
//
//    NewProcess (a0) - Supplies a pointer to a control object of type process
//      which represents the new process that is switched to.
//
//    OldProcess (a1) - Supplies a pointer to a control object of type process
//      which represents the old process that is switched from.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
SpArg:  .space  4 * 4                   // argument register save area
        .space  4 * 3                   // fill for alignment
SpRa:   .space  4                       // saved return address
SpFrameLength:                          // length of stack frame
SpA0:   .space  4                       // saved argument register a0

        NESTED_ENTRY(KiSwapProcess, SpFrameLength, zero)

        subu    sp,sp,SpFrameLength     // allocate stack frame
        sw      ra,SpRa(sp)             // save return address

        PROLOGUE_END

//
// Acquire the context swap lock, clear the processor set member in he old
// process, set the processor member in the new process, and release the
// context swap lock.
//

#if !defined(NT_UP)

10:     ll      t0,KiContextSwapLock    // get current lock value
        move    t1,a0                   // set ownership value
        bne     zero,t0,10b             // if ne, lock already owned
        sc      t1,KiContextSwapLock    // set lock ownership value
        beq     zero,t1,10b             // if eq, store conditional failed
        lw      t0,KiPcr + PcSetMember(zero) // get processor set member
        lw      t2,PrActiveProcessors(a0) // get new active processor set
        lw      t1,PrActiveProcessors(a1) // get old active processor set
        or      t2,t2,t0                // set processor member in set
        xor     t1,t1,t0                // clear processor member in set
        sw      t2,PrActiveProcessors(a0) // set new active processor set
        sw      t1,PrActiveProcessors(a1) // set old active processor set
        sw      zero,KiContextSwapLock  // clear lock value

#endif

//
// Allocate a new process PID. If the new PID number is greater than the
// number of PIDs supported on the host processor, then flush the entire
// TB and reset the PID number ot zero.
//

        lw      v1,KiPcr + PcCurrentPid(zero) // get current processor PID
        lw      t2,KeNumberProcessIds   // get number of process id's
        addu    v1,v1,1 << ENTRYHI_PID  // increment master system PID
        sltu    t2,v1,t2                // any more PIDs to allocate
        bne     zero,t2,15f             // if ne, more PIDs to allocate

//
// Flush the random part of the TB.
//

        sw      a0,SpA0(sp)             // save process object address
        jal     KiFlushRandomTb         // flush random part of TB
        lw      a0,SpA0(sp)             // restore process object address
        move    v1,zero                 // set next PID value

//
// Swap address space to the specified process.
//

15:     sw      v1,KiPcr + PcCurrentPid(zero) // set current processor PID
        lw      t1,PrDirectoryTableBase(a0) // get page directory PDE
        lw      t2,PrDirectoryTableBase + 4(a0) // get hyper space PDE
        li      t3,PDE_BASE             // get virtual address of PDR
        or      t3,t3,v1                // merge process PID
        li      t4,PDR_ENTRY << INDEX_INDEX // set entry index for PDR

        DISABLE_INTERRUPTS(t5)          // disable interrupts

        .set    noreorder
        .set    noat
        mtc0    t3,entryhi              // set VPN2 and PID of TB entry
        mtc0    t1,entrylo0             // set first PDE value
        mtc0    t2,entrylo1             // set second PDE value
        mtc0    t4,index                // set index of PDR entry
        nop                             // 1 cycle hazzard
        tlbwi                           // write system PDR TB entry
        nop                             // 3 cycle hazzard
        nop                             //
        nop                             //
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t5)           // enable interrupts

        lw      ra,SpRa(sp)             // restore return address
        addu    sp,sp,SpFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KiSwapProcess
