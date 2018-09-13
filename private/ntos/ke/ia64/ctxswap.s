//++
//
// Module Name:
//
//       ctxswap.s
//
// Abstract:
//
//
// Author:
//
//       Bernard Lint  Jul-12-1995
//
// Environment:
//
//       Kernel mode only
//
// Revision History:
//
//--

#include "ksia64.h"

        .file     "ctxswap.s"
        .text

//
// Globals imported:
//

        .global     KiReadySummary
        .global     KiIdleSummary
        .global     KiDispatcherReadyListHead
        .global     KeTickCount
        .global     KiMasterSequence
        .global     KiMasterRid
        .global     KiWaitInListHead
        .global     KiWaitOutListHead
#if !defined(NT_UP)
        .global     KiContextSwapLock
        .global     KiDispatcherLock
        .global     KiSwapContextNotifyRoutine
#endif // !defined(NT_UP)

        PublicFunction(KiDeliverApc)
        PublicFunction(KiSaveExceptionFrame)
        PublicFunction(KiRestoreExceptionFrame)
        PublicFunction(KiActivateWaiterQueue)
        PublicFunction(KiReadyThread)
        PublicFunction(KeFlushEntireTb)
        PublicFunction(KiQuantumEnd)
        PublicFunction(KiSyncNewRegionId)
        PublicFunction(KiCheckForSoftwareInterrupt)
        PublicFunction(KiSaveHigherFPVolatile)

#if DBG
        PublicFunction(KeBugCheckEx)
#endif // DBG


        SBTTL("Unlock Dispatcher Database")
//++
//--------------------------------------------------------------------
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
//        lock was acquired (in low order byte, not zero extended).
//
// Return Value:
//
//    None.
//
//--------------------------------------------------------------------
//--
        
        NESTED_ENTRY(KiUnlockDispatcherDatabase)
        NESTED_SETUP(1,3,1,0)

//
// Register aliases
//

        rDPC      = loc2                // DPC active flag

        rpT1      = t1                  // temp pointer
        rpT2      = t2                  // temp pointer
        rpT3      = t3                  // temp pointer
        rT1       = t5                  // temp regs
        rT2       = t6
        rpDPLock  = t7                  // pointer to dispatcher lock
        rPrcb     = t8                  // PRCB pointer

        pNotNl    = pt2                 // true if next thread not NULL
        pIRQGE    = pt3                 // true if DISPATCH_LEVEL <= old irql
        pIRQLT    = pt4                 // true if DISPATCH_LEVEL > old irql
        pDPC      = pt5                 // true if DPC active

        PROLOGUE_END

//
// Check if a thread has been scheduled to execute on the current processor
//

#if !defined(NT_UP)
        add       rpDPLock = @gprel(KiDispatcherLock), gp
#endif // !defined(NT_UP)
        movl      rPrcb = KiPcr + PcPrcb
        ;;

        LDPTR     (rPrcb, rPrcb)                // rPrcb -> PRCB
        ;;
        add       rpT1 = PbNextThread, rPrcb    // -> next thread
        add       rpT2 = PbDpcRoutineActive,rPrcb // -> DPC active flag
        ;;

        LDPTR     (v0, rpT1)                    // v0 = next thread
        ;;
        cmp.ne    pNotNl = zero, v0             // pNotNl = next thread is 0
        zxt1      a0 = a0                       // isolate old IRQL
        ;;

(pNotNl) cmp.leu.unc pIRQGE, pIRQLT = DISPATCH_LEVEL, a0
        mov       rDPC = 1                      // speculate that DPC is active
(pIRQLT) br.spnt   KxUnlockDispatcherDatabase
        ;;

//
// Case 1:
// Next thread is NULL:
// Release dispatcher database lock, restore IRQL to its previous level
// and return
//

//
// Case 2:
// A new thread has been selected to run on the current processor, but
// the new IRQL is not below dispatch level. Release the dispatcher 
// lock and restore IRQL. If the current processor is
// not executing a DPC, then request a dispatch interrupt on the current
// processor.
//
// At this point pNotNl = 1 if thread not NULL, 0 if NULL
//

(pIRQGE) ld4       rDPC = [rpT2]                // rDPC.4 = DPC active flag
#if !defined(NT_UP)
        RELEASE_SPINLOCK(rpDPLock)              // release dispatcher lock
#endif // !defined(NT_UP)
         ;;

        LOWER_IRQL(a0)
        cmp4.eq    pDPC = rDPC, zero            // pDPC = request DPC intr
        REQUEST_DISPATCH_INT(pDPC)              // request DPC interrupt

        NESTED_RETURN
        NESTED_EXIT(KiUnlockDispatcherDatabase)

//
// N.B. This routine is carefully written as a nested function.
//    Control only reaches this routine from above.
//
//    rPrcb contains the address of PRCB
//    v0 contains the next thread
//

        NESTED_ENTRY(KxUnlockDispatcherDatabase)
        PROLOGUE_BEGIN

        .regstk   1, 2, 1, 0
        alloc     t16 = ar.pfs, 1, 2, 1, 0
        .save     rp, loc0
        mov       loc0 = brp
        .fframe   SwitchFrameLength
        add       sp = -SwitchFrameLength, sp
        ;;

        .save     ar.unat, loc1
        mov       loc1 = ar.unat
        add       t0 = ExFltS19+SwExFrame+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;

        .save.gf  0x0, 0xC0000
        stf.spill [t0] = fs19, ExFltS17-ExFltS19
        stf.spill [t1] = fs18, ExFltS16-ExFltS18
        ;;

        .save.gf  0x0, 0x30000
        stf.spill [t0] = fs17, ExFltS15-ExFltS17
        stf.spill [t1] = fs16, ExFltS14-ExFltS16
        mov       t10 = bs4
        ;;

        .save.gf  0x0, 0xC000
        stf.spill [t0] = fs15, ExFltS13-ExFltS15
        stf.spill [t1] = fs14, ExFltS12-ExFltS14
        mov       t11 = bs3
        ;;

        .save.gf  0x0, 0x3000
        stf.spill [t0] = fs13, ExFltS11-ExFltS13
        stf.spill [t1] = fs12, ExFltS10-ExFltS12
        mov       t12 = bs2
        ;;

        .save.gf  0x0, 0xC00
        stf.spill [t0] = fs11, ExFltS9-ExFltS11
        stf.spill [t1] = fs10, ExFltS8-ExFltS10
        mov       t13 = bs1
        ;;

        .save.gf  0x0, 0x300
        stf.spill [t0] = fs9, ExFltS7-ExFltS9
        stf.spill [t1] = fs8, ExFltS6-ExFltS8
        mov       t14 = bs0
        ;;

        .save.gf  0x0, 0xC0
        stf.spill [t0] = fs7, ExFltS5-ExFltS7
        stf.spill [t1] = fs6, ExFltS4-ExFltS6
        mov       t15 = ar.lc
        ;;

        .save.gf  0x0, 0x30
        stf.spill [t0] = fs5, ExFltS3-ExFltS5
        stf.spill [t1] = fs4, ExFltS2-ExFltS4
        ;;

        .save.f   0xC
        stf.spill [t0] = fs3, ExFltS1-ExFltS3         // save fs3
        stf.spill [t1] = fs2, ExFltS0-ExFltS2         // save fs2
        ;;

        .save.f   0x3
        stf.spill [t0] = fs1, ExBrS4-ExFltS1          // save fs1
        stf.spill [t1] = fs0, ExBrS3-ExFltS0          // save fs0
        ;;

        .save.b   0x18
        st8       [t0] = t10, ExBrS2-ExBrS4           // save bs4
        st8       [t1] = t11, ExBrS1-ExBrS3           // save bs3
        ;;

        .save.b   0x6
        st8       [t0] = t12, ExBrS0-ExBrS2           // save bs2
        st8       [t1] = t13, ExIntS2-ExBrS1          // save bs1
        ;;

        .save.b   0x1
        st8       [t0] = t14, ExIntS3-ExBrS0          // save bs0
        ;;

        .save.gf  0xC, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s3, ExIntS1-ExIntS3          // save s3
        .mem.offset 8,0
        st8.spill [t1] = s2, ExIntS0-ExIntS2          // save s2
        ;;

        .save.gf  0x3, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s1, ExApLC-ExIntS1           // save s1
        .mem.offset 8,0
        st8.spill [t1] = s0, ExApEC-ExIntS0           // save s0
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        ;;

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3
        mov       s0 = rPrcb

        PROLOGUE_END

        mov       s2 = v0
        movl      rpT1 = KiPcr + PcCurrentThread
        ;;

        LDPTR     (s1, rpT1)                    // current thread
        add       rpT2 = PbNextThread, s0       // -> next thread
        ;;
        add       rpT3 = ThWaitIrql, s1         // -> previous IRQL

        STPTRINC  (rpT2, zero,PbCurrentThread-PbNextThread)  // clear next thread address
        ;;
        st1       [rpT3] = a0                   // save previous IRQL

//
// Reready current thread for execution and swap context to the selected thread.
//
// N.B. The return from the call to swap context is directly to the swap
//      thread exit.
//

        STPTR     (rpT2, s2)                    // set current thread object
        mov       out0 = s1                     // out0 -> previous thread
        br.call.sptk brp = KiReadyThread
        ;;

        movl      rT1 = KiSwapThreadExit
        ;;

        mov       brp = rT1                     // set return address
        br.call.sptk   bt0 = SwapContext
        ;;

//
// N.B.: return to KiSwapThreadExit
//

        NESTED_EXIT(KxUnlockDispatcherDatabase)

        SBTTL("Swap Thread")
//++
//--------------------------------------------------------------------
//
// INT_PTR
// KiSwapThread (
//    VOID
//    )
//
// Routine Description:
//
//       This routine is called to select and the next thread to run on the
//       current processor and to perform a context switch to the thread.
//
// Arguments:
//
//       None.
//
// Return Value:
//
//       Wait completion status (v0).
//
// Notes:
//
//       GP valid on entry -- GP is not switched, just use kernel GP
//--------------------------------------------------------------------
//--
        NESTED_ENTRY(KiSwapThread)
        PROLOGUE_BEGIN

        .regstk   1, 2, 1, 0
        alloc     t16 = ar.pfs, 1, 2, 1, 0
        .save     rp, loc0
        mov       loc0 = brp
        .fframe   SwitchFrameLength
        add       sp = -SwitchFrameLength, sp
        ;;

        .save     ar.unat, loc1
        mov       loc1 = ar.unat
        add       t0 = ExFltS19+SwExFrame+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;

        .save.gf  0x0, 0xC0000
        stf.spill [t0] = fs19, ExFltS17-ExFltS19
        stf.spill [t1] = fs18, ExFltS16-ExFltS18
        ;;

        .save.gf  0x0, 0x30000
        stf.spill [t0] = fs17, ExFltS15-ExFltS17
        stf.spill [t1] = fs16, ExFltS14-ExFltS16
        mov       t10 = bs4
        ;;

        .save.gf  0x0, 0xC000
        stf.spill [t0] = fs15, ExFltS13-ExFltS15
        stf.spill [t1] = fs14, ExFltS12-ExFltS14
        mov       t11 = bs3
        ;;

        .save.gf  0x0, 0x3000
        stf.spill [t0] = fs13, ExFltS11-ExFltS13
        stf.spill [t1] = fs12, ExFltS10-ExFltS12
        mov       t12 = bs2
        ;;

        .save.gf  0x0, 0xC00
        stf.spill [t0] = fs11, ExFltS9-ExFltS11
        stf.spill [t1] = fs10, ExFltS8-ExFltS10
        mov       t13 = bs1
        ;;

        .save.gf  0x0, 0x300
        stf.spill [t0] = fs9, ExFltS7-ExFltS9
        stf.spill [t1] = fs8, ExFltS6-ExFltS8
        mov       t14 = bs0
        ;;

        .save.gf  0x0, 0xC0
        stf.spill [t0] = fs7, ExFltS5-ExFltS7
        stf.spill [t1] = fs6, ExFltS4-ExFltS6
        mov       t15 = ar.lc
        ;;

        .save.gf  0x0, 0x30
        stf.spill [t0] = fs5, ExFltS3-ExFltS5
        stf.spill [t1] = fs4, ExFltS2-ExFltS4
        ;;

        .save.f   0xC
        stf.spill [t0] = fs3, ExFltS1-ExFltS3         // save fs3
        stf.spill [t1] = fs2, ExFltS0-ExFltS2         // save fs2
        ;;

        .save.f   0x3
        stf.spill [t0] = fs1, ExBrS4-ExFltS1          // save fs1
        stf.spill [t1] = fs0, ExBrS3-ExFltS0          // save fs0
        ;;

        .save.b   0x18
        st8       [t0] = t10, ExBrS2-ExBrS4           // save bs4
        st8       [t1] = t11, ExBrS1-ExBrS3           // save bs3
        ;;

        .save.b   0x6
        st8       [t0] = t12, ExBrS0-ExBrS2           // save bs2
        st8       [t1] = t13, ExIntS2-ExBrS1          // save bs1
        ;;

        .save.b   0x1
        st8       [t0] = t14, ExIntS3-ExBrS0          // save bs0
        ;;

        .save.gf  0xC, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s3, ExIntS1-ExIntS3          // save s3
        .mem.offset 8,0
        st8.spill [t1] = s2, ExIntS0-ExIntS2          // save s2
        ;;

        .save.gf  0x3, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s1, ExApLC-ExIntS1           // save s1
        .mem.offset 8,0
        st8.spill [t1] = s0, ExApEC-ExIntS0           // save s0
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        ;;

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3

        PROLOGUE_END

//
// Register aliases
//

        //          s0                          // Prcb address
        //          s1                          // old thread address
        //          s2                          // new thread address

        rRdySum   = s3                          // ready summary
        rWstatus  = s3                          // wait status

        rpT1      = t0                          // temp pointer
        rpT2      = t1                          // temp pointer
        rProcNum  = t2                          // processor number
        rIdleSum    = t3                        // idle summary
        rpRdyHead   = t4                        // -> ready list head
        rQEntry     = t5                        // queue entry
        rQueNum     = t6                        // ready queue number
        rFlink      = t7                        // forward link
        rBlink      = t8                        // backward link
        rFirstByte  = t9                        // first non-zero byte
        rT1         = t10                       // temp regs
        rRdyShftd   = t11                       // current shifted ready summary bits
        rT3         = t12                       // temp regs
        rT4         = t13
        rpRdySum    = t14                       // pointer to KiReadySummary
        rAffmask    = t15                       // processor affinity mask
#if !defined(NT_UP)        
        rTickCount  = t16                       // tick count
        rThIdealProc = t17                      // thread ideal processor
        rThWaitTime = t18                       // time of thread ready
        rThNextProc = t19                       // thread's last good processor
        rTrialEntry = t20                       // next candidate entry in wait list
        rTrialThrd  = t21                       // next candidate thread in wait list
#endif // !defiend(NT_UP)
        pNotNl      = pt0                       // not null predicate
        pEmpty      = pt1                       // ready list empty
        pNoAPC      = pt2                       // do not dispatch APC
        pRdy        = pt3                       // test next ready queue
        pNoRdy      = pt4                       // no more ready queues
        pEq         = pt5                       // if cmp.eq is true
        pNotEq      = pt6                       // if cmp.eq is not true
        pMt1        = pt7                       // if match
        pMt2        = pt8                       // if match

        add       rpRdySum = @gprel(KiReadySummary), gp    // rpRdysum -> KiReadySummary
        movl      rpT1 = KiPcr + PcPrcb
        ;;

        LDPTRINC  (s0, rpT1, PcCurrentThread-PcPrcb)    // -> prcb
        ld4       rRdySum = [rpRdySum]          // rRdysum.4 = KiReadySummary
        ;;
        add       rpT2 = PbNextThread, s0

        LDPTRINC  (s1, rpT1, PcSetMember-PcCurrentThread) // current thread 
        ;;

        LDPTR     (s2, rpT2)                    // s2 -> new thread 
        STPTR     (rpT2, zero)                  // zero next thread
        ;;

#if !defined(NT_UP)

        ld4       rAffmask = [rpT1], PcNumber-PcSetMember // rAffmask.4 = processor affinity mask
        ;;
        ld1       rProcNum = [rpT1]             // processor number

#endif // !defined(NT_UP)

        cmp.ne    pNotNl = zero, s2             // if ne, next thread selected
(pNotNl) br.sptk   Kst_SwapContext              // br if thread not null

// Search KiReadySummary to find highest priority queue with at least one
// thread ready to run

        pcmp1.eq  rT1 = rRdySum, zero           // parallel cmp to zero
        ;;
        czx1.l    rFirstByte = rT1              // index of byte with first one
        ;;
        add       rFirstByte = -4, rFirstByte   // adjust for 32-bit value
        ;;
        shl       rQueNum = rFirstByte, 3       // shift for bit index
        ;;
        shl       rRdyShftd = rRdySum, rQueNum  // shift first byte to MSB
        sub       rQueNum = 31, rQueNum         // change bit index to queue #
        ;;

Kst_Loop:
        cmp.gt    pNoRdy = 0, rQueNum           // Check for last queue (0)
(pNoRdy) br.sptk  Kst_NoRdy                     // br if no more queues to check
        ;;

        tbit.nz   pRdy = rRdyShftd, 31          // test high order bit
(pRdy)  br.sptk   Kst_Ready                     // br if find queue not empty
        ;;

//
// If the next bit is set in the ready summary, then scan the corresponding
// dispatcher ready queue.
//

Kst50:
        sub       rQueNum = rQueNum, zero, 1    // decrement queue #
        shl       rRdyShftd = rRdyShftd, 1      // get next bit
        br.sptk   Kst_Loop                      // check next queue
        ;;

//
// All ready queues were scanned without finding a runnable thread so
// default to the idle thread and set the appropriate bit in idle summary.
//

Kst_NoRdy:

#if defined(_COLLECT_SWITCH_DATA_)

        add         rpT1 = @gprel(KeThreadSwitchCounters), gp
        ;;
        add         rpT1 = TwSwitchToIdle, rpT1
        ;;
        ld4         rT1 = [rpT1]
        ;;
        add         rT1 = rT1, zero, 1          // increment count
        ;;
        st4         [rpT1] = rT1

#endif // defined(_COLLECT_SWITCH_DATA_)

        add       rpT1 = @gprel(KiIdleSummary), gp // -> idle summary
        add       rpT2 = PbIdleThread, s0       // -> PbIdleThread
        ;;

#if defined(NT_UP)

        mov       rIdleSum = 1                  // get current idle summary
#else

        ld4       rIdleSum= [rpT1]              // get current idle summary
        ;;
        or        rIdleSum = rIdleSum, rAffmask // set member bit in idle summary

#endif // !defined(NT_UP)

        ;;
        st4       [rpT1] = rIdleSum             // set new idle summary
        LDPTR     (s2, rpT2)                    // address of idle thread
        br.sptk   Kst_SwapContext               // branch to do swap context

//
// If the thread can execute on the current processor, then remove it from
// the dispatcher ready queue. At this point rQueNum has number of highest
// priority non-empty queue.
//

Kst_Ready:
        add       rpRdyHead = @gprel(KiDispatcherReadyListHead),gp  // get ready list head base address
        ;;
#ifdef _WIN64
        shladd    rpRdyHead=rQueNum,4,rpRdyHead  // compute ready queue address
#else
        shladd    rpRdyHead=rQueNum,3,rpRdyHead  // compute ready queue address
#endif
        ;;
        add       rpT1 = LsFlink, rpRdyHead      // -> addr of first queue entry
        ;;
        LDPTR     (rQEntry,rpT1)                 // rQEntry.4 = address of first queue entry
        ;;

Kst70:
        add       s2 = -ThWaitListEntry, rQEntry // -> thread object
        ;;

#if !defined(NT_UP)

//
// If the thread can execute on the current processor, then remove it from
// the dispatcher ready queue.
//

        add       rpT1 = ThAffinity, s2
        ;;
        ld4       rT3 = [rpT1]                   // rT3 = ThAffinity
        ;;
        and       rT1 = rAffmask, rT3            // the current processor?
        ;;
        cmp4.ne   pt0 = zero, rT1                // if ne, thread affinity compatible
        add       rQEntry = LsFlink, rQEntry     // t4 -> LsFlink(rQEntry)
(pt0)   br.cond.sptk Kst80
        ;;

        LDPTR     (rQEntry, rQEntry)             // get address of next entry
        ;;
        cmp.ne    pt0 = rQEntry, rpRdyHead       // check for end of list
 (pt0)  br.cond.sptk Kst70                       // if ne, not end of list
        br          Kst50
        ;;

//
// If the thread last ran on the current processor, the processor is the
// ideal processor for the thread, the thread has been waiting for longer
// than a quantum, ot its priority is greater than (or equal) low realtime plus 9,
// then select the thread. Otherwise, an attempt is made to find a more
// appropriate candidate.
//

Kst80:
        add         rpT1 = ThNextProcessor, s2
        add         rpT2 = ThIdealProcessor, s2
        ;;
        ld1         rThNextProc = [rpT1]
        ld1         rThIdealProc = [rpT2]
        ;;
        add         rpT1 = ThWaitTime-ThNextProcessor, rpT1
        add         rpT2 = @gprel(KeTickCount),gp // -> KeTickCount
        ;;
        ld4         rThWaitTime = [rpT1]          // get time of thread ready
//
// ld4 or ld8???
//
        ld4         rTickCount = [rpT2]           // rTickCount = KeTickCount
        cmp.eq      pMt1 = rThNextProc, rProcNum  // check last processor
(pMt1)  br.cond.spnt Kst120                       // if eq, last processor match
        ;;

        sub         rT1 = rTickCount, rThWaitTime // rT1 = TickCount-WaitTime 
        cmp.eq      pMt2= rThIdealProc,rProcNum   // check ideal processor
(pMt2)  br.cond.spnt Kst120                       // if eq ideal processor match
        ;;

        cmp.leu     pMt1 = LOW_REALTIME_PRIORITY+9, rQueNum // choose thread if LOW+9 <= priority
        cmp.leu     pMt2 = READY_SKIP_QUANTUM+1, rT1 // choose thread if QUANT+1 <= time delta

        add         rpT1 = LsFlink, rQEntry
(pMt1)  br.cond.spnt Kst120
(pMt2)  br.cond.spnt Kst120                     
        ;;

//
// search forward in the ready queue until the end of the list is reached
// or a more appropriate thread is found
//
        
Kst90:
        LDPTR       (rTrialEntry, rpT1)         // get address of next entry
        ;;
        cmp.eq      pEq= rTrialEntry,rpRdyHead  // if eq, at end of list
(pEq)   br.spnt     Kst120                      // select original thread           
        ;;
        add         rTrialThrd = -ThWaitListEntry, rTrialEntry // compute address of new thread object
        ;;
        add         rpT1= ThAffinity,rTrialThrd // address of affinity
        add         rpT2= ThWaitTime,rTrialThrd // address of WaitTime
        ;;        
        ld4         rT3 = [rpT1], ThNextProcessor-ThAffinity
        ;;
        and         rT1 = rAffmask, rT3         // the current processor?
        ;;
        cmp4.eq     pEq = zero, rT1             // if eq, thread affinity not compatible
        ;;
(pEq)   br.cond.spnt Kst100                     // br if not compatible 
        ;;
        ld1         rThNextProc = [rpT1]
        ;;
        cmp.eq      pMt1 = rThNextProc,rProcNum // check processor match
        ;;
(pMt1)  br.cond.spnt Kst110                     // if eq, processor match
        ;;
        add         rpT1 = ThIdealProcessor-ThNextProcessor, rpT1
        ;;
        ld1         rThIdealProc = [rpT1]
        ;;
        cmp.eq      pMt2 = rThIdealProc,rProcNum // check ideal processor
        ;;
(pMt2)  br.cond.spnt Kst110                     // if eq, ideal processor match
        ;;
Kst100:        
        ld4         rThWaitTime = [rpT2]        // get time of thread ready
        ;;
        sub         rT1 = rTickCount,rThWaitTime // rT1 = TickCount-WaitTime 
        ;;
        cmp.leu     pMt1 = READY_SKIP_QUANTUM+1, rT1 // choose original thread if QUANT+1 <= time delta
(pMt1)  br.cond.spnt Kst120                     
        ;;

        add         rpT1 = LsFlink, rTrialEntry // get address of Flink
        br.sptk     Kst90
        ;;
Kst110:
        mov         rQEntry = rTrialEntry       // set list entry address
        mov         s2 = rTrialThrd             // set new thread address
        ;;
//
// rProcNum = current processor and processor for new thread
//

Kst120:

#if defined(_COLLECT_SWITCH_DATA_)

        add         rpT1 = @gprel(KeThreadSwitchCounters), gp
        cmp.eq      pt0, pt1 = rThNextProc, rProcNum // if eq, last processor match
        ;;
        add         rpT1 = TwFindAny, rpT1      // computer address of Any counter
(pt1)   cmp.eq.unc  pt2 = rThIdealProc,rProcNum // if eq, ideal processor match
        ;;
(pt0)   add         rpT2 = TwFindLast-TwFindAny, rpT1       // address of Last counter
(pt2)   add         rpT2 = TwFindIdeal-TwFindAny, rpT1      // address of Ideal counter
        ;;
        ld4         rT4 = [rpT2]                // get appropriate counter
        ;;
        add         rT4 = 1, rT4                // increment counter
        ;;
        st4         [rpT2] = rT4                // store counter
        
#endif // defined(_COLLECT_SWITCH_DATA_)

        add         rpT1 = ThNextProcessor, s2
        ;;
        st1         [rpT1] = rProcNum           // update next processor
        ;;
#endif   // !defined(NT_UP)

//
// Remove the selected thread from the ready queue.
//

        add       rpT1 = LsFlink, rQEntry       // rpT1 -> forward link
        add       rpT2 = LsBlink, rQEntry       // rpT2 -> backward link
        ;;

        LDPTR     (rFlink, rpT1)                // rFlink = forward link
        LDPTR     (rBlink, rpT2)                // rBlink = backward link
        ;;

        add       rpT1 = LsFlink, rBlink        // rpT1 -> previous entry
        add       rpT2 = LsBlink, rFlink        // rpT2 -> next entry
        ;;

        STPTR     (rpT1, rFlink)                // set forward link in previous entry
        STPTR     (rpT2, rBlink)                // set backward link in next entry

        mov       rT1 = 1                       // set bit for mask generation
        cmp.eq    pEmpty = rFlink, rBlink       // if eq, list is empty
        ;;


(pEmpty) shl       rT1 = rT1, rQueNum            // ready summary set member
        ;;
(pEmpty) xor       rT1 = rT1, rRdySum            // clear ready summary bit
        ;;
(pEmpty) st4       [rpRdySum] = rT1              // store updated KiReadySummary

//
// Swap context to the next thread.
//

Kst_SwapContext:
        add       rpT1 = PbCurrentThread, s0
        ;;

        STPTR     (rpT1, s2)                    // set addr of current thread
        br.call.sptk brp = SwapContext          // call SwapContext(prcb, OldTh, NewTh)
        ;;

//
// Lower IRQL, deallocate exception/switch frame, and return wait completion status.
//
// N.B. SwapContext releases the dispatcher database lock.
//
// N.B. v0 contains the kernel APC pending state on return.
//
// N.B. s2 contains the address of the new thread on return.
//

        ALTERNATE_ENTRY(KiSwapThreadExit)

        add       rpT1 = ThWaitStatus, s2      // -> ThWaitStatus
        add       rpT2 = ThWaitIrql, s2        // -> ThWaitIrql
        zxt1      v0 = v0
        ;;

        LDPTR     (rWstatus, rpT1)             // wait completion status
        ld1       a0 = [rpT2]                  // a0 = original wait IRQL
        ;;
        or        rT1 = a0, v0
        ;;

        cmp.eq    pNoAPC = zero, rT1           // APC pending and IRQL == 0
        add       rpT2 = PbApcBypassCount, s0
(pNoAPC) br.spnt  Kst_Exit
        ;;

        .regstk   1, 2, 3, 0
        alloc     t16 = ar.pfs, 1, 2, 3, 0
        SET_IRQL(APC_LEVEL)
        movl      rpT1 = KiPcr+PcApcInterrupt
        ;;

        ld4       rT1 = [rpT2]
        st1       [rpT1] = zero
        mov       out0 = KernelMode
        ;;

        add       rT1 = 1, rT1
        mov       out1 = zero
        mov       out2 = zero
        ;;
        
        st4       [rpT2] = rT1
        br.call.sptk brp = KiDeliverApc
        ;;

//
// Lower IRQL to wait level, set return status, restore registers, and
// return.
//

Kst_Exit:

        LOWER_IRQL(a0)                          // a0 = new irql

        add       out0 = STACK_SCRATCH_AREA+SwExFrame, sp
        mov       v0 = rWstatus                 // return wait status
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

        add       rpT1 = ExApEC+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;
        ld8       rT1 = [rpT1]
        mov       brp = loc0
        ;;

        mov       ar.unat = loc1
        nop.f     0
        mov       ar.pfs = rT1

        .restore
        add       sp = SwitchFrameLength, sp
        nop.i     0
        br.ret.sptk brp
        ;;

        NESTED_EXIT(KiSwapThread)

        SBTTL("Swap Context to Next Thread")
//++
//--------------------------------------------------------------------
// Routine:
//
//       SwapContext
//
// Routine Description:
//
//       This routine is called to swap context from one thread to the next.
//
// Arguments:
//
//       s0 - Address of Processor Control Block (PRCB).
//       s1 - Address of previous thread object.
//       s2 - Address of next thread object.
//
// Return value:
//
//       v0 - Kernel APC pending flag
//       s0 - Address of Processor Control Block (PRCB).
//       s1 - Address of previous thread object.
//       s2 - Address of current thread object.
//
// Note:
//       Kernel GP is not saved and restored across context switch
//
//--------------------------------------------------------------------
//--

        NESTED_ENTRY(SwapContext)

//
// Register aliases
//

        rT1       = t1                          // temp
        rT2       = t2                          // temp
        rT3       = t3                          // temp
        rNewproc  = t4                          // next process object
        rOldproc  = t5                          // previous process object
        rpThBSL   = t6                          // pointer to new thread backing store limit
        rpT1      = t7                          // temp pointer
        rpT2      = t8                          // temp pointer
        rpT3      = t9                          // temp pointer
        rRSEDis   = t10                         // RSE disable
        rNewIKS   = t11                         // new initial kernel stack
        rRSEEn    = t12                         // RSE enable
        rNewKSL   = t13                         // new kernel stack limit
        rpPcrBSL  = t14                         // pointer to new kernel backing store limit
        rpNewBSP  = t15                         // pointer to new thread BSP
        rpNewRNAT = t16                         // pointer to new thread RNAT
        rNewBSP   = t17                         // new thread BSP/BSPSTORE
        rOldBSP   = t18                         // old thread BSP
        rOldRNAT  = t19                         // old thread RNAT
        rNewRNAT  = t20                         // new thread RNAT
        rOldIKS   = t21                         // old thread initial kstack

        rOldPsrL  = s3                          // old psr interrupt flag

        pApc      = pt3                         // APC pending
        pUsTh     = pt4                         // is user thread?
        pKrTh     = pt5                         // is user thread?
        pIdle     = pt6                         // true if SwapContext not called from Idle
        pSave     = pt7                         // is high fp set dirty?
        pDiff     = ps1                         // if new and old process different
        pSame     = ps2                         // if new and old process same

//
// Set new thread's state to running. Note this must be done
// under the dispatcher lock so that KiSetPriorityThread sees
// the correct state.
//

        PROLOGUE_BEGIN

        alloc     rT2 = ar.pfs, 0, 0, 2, 0
        add       rpT1 = SwRp+STACK_SCRATCH_AREA, sp
        add       rpT2 = SwPFS+STACK_SCRATCH_AREA, sp
        ;;

        mov       rOldBSP = ar.bsp
        mov       rT1 = brp
        add       rpT3 = ThState, s2
        ;;

        .savesp   brp, SwRp+STACK_SCRATCH_AREA
        st8.nta   [rpT1] = rT1, SwFPSR-SwRp         // save return link
        .savesp   ar.pfs, SwPFS+STACK_SCRATCH_AREA
        st8.nta   [rpT2] = rT2, SwPreds-SwPFS       // save pfs
        mov       rT3 = Running
        ;;

        flushrs
        st1.nta   [rpT3] = rT3
        mov       rT1 = pr
        cmp.ne    pUsTh = zero, teb
        ;;

        mov       ar.rsc = r0                       // disable RSE
        mov       out0 = ar.fpsr
        add       rpT3 = ThStackBase, s1
        ;;

        LDPTR     (rT3, rpT3)
        st8       [rpT1] = out0                     // save kernel FPSR
        st8       [rpT2] = rT1                      // save preserved predicates
        ;;

(pUsTh) mov       rT1 = ar21
(pUsTh) mov       rT2 = ar24
        ;;

(pUsTh) add       rpT1 = -ThreadStateSaveAreaLength+TsAppRegisters+TsAr21, rT3
(pUsTh) add       rpT2 = -ThreadStateSaveAreaLength+TsAppRegisters+TsAr24, rT3
        ;;

(pUsTh) st8       [rpT1] = rT1, TsAr25-TsAr21
(pUsTh) st8       [rpT2] = rT2, TsAr26-TsAr24

(pUsTh) mov       rT1 = ar25
(pUsTh) mov       rT2 = ar26
        ;;

(pUsTh) st8       [rpT1] = rT1, TsAr27-TsAr25
(pUsTh) st8       [rpT2] = rT2, TsAr28-TsAr26

(pUsTh) mov       rT1 = ar27
(pUsTh) mov       rT2 = ar28
        ;;

(pUsTh) st8       [rpT1] = rT1, TsAr29-TsAr27
(pUsTh) st8       [rpT2] = rT2, TsAr30-TsAr28

(pUsTh) mov       rT1 = ar29
(pUsTh) mov       rT2 = ar30
        ;;

(pUsTh) st8       [rpT1] = rT1
(pUsTh) st8       [rpT2] = rT2
        add       rpT3 = ThInitialStack, s1
        ;;

        LDPTRINC  (rOldIKS, rpT3, ThKernelBStore-ThInitialStack)
        mov       rOldRNAT = ar.rnat
        mov       rT2 = psr
        ;;

        st8.nta   [rpT3] = rOldBSP
        tbit.nz   pSave = rT2, PSR_MFH        // check mfh bit
        add       rpT2 = SwBsp+STACK_SCRATCH_AREA, sp
        ;;

        st8.nta   [rpT2] = rOldBSP, SwRnat-SwBsp
(pSave) add       rpT1 = -ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR, rOldIKS
        ;;

(pSave) ld8.nta   rT2 = [rpT1]
        st8.nta   [rpT2] = rOldRNAT
        ;;
(pSave) dep       rT2 = 0, rT2, PSR_MFH, 1     // Clear user trap frame mfh bit
        ;;
    
(pSave) st8.nta   [rpT1] = rT2
(pSave) add       out0 = -TrStIPSR+TrapFrameLength+TsHigherFPVolatile, rpT1
(pSave) br.call.sptk brp = KiSaveHigherFPVolatile
        ;;

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

       add       rpT1 = @gprel(KiContextSwapLock), gp
       add       rpT2 = @gprel(KiDispatcherLock), gp
       add       rpT3 = @gprel(KiSwapContextNotifyRoutine), gp
       ;;

       ld8       rpT3 = [rpT3]
       ACQUIRE_SPINLOCK(rpT1, s1, Sc_Lock1)

//
// Release KiDispatcherLock after setting thread state to Running
//

       RELEASE_SPINLOCK(rpT2)
       add       rpT1 = EtCid+CidUniqueThread, s1  // -> old thread unique id
       add       rpT2 = EtCid+CidUniqueThread, s2  // -> new thread unique id
       ;;

       ld8.nta   out0 = [rpT1]
       ld8.nta   out1 = [rpT2]
       cmp.ne    pt1 = zero, rpT3
       ;;

(pt1)  ld8.nta   rT1 = [rpT3], 8
       ;;
(pt1)  ld8.nta   gp = [rpT3]
       mov       bt0 = rT1
(pt1)  br.call.spnt brp = bt0                      // call notify routine
       ;;
       movl      gp = _gp

#endif // !defined(NT_UP)

       PROLOGUE_END

//
// ***** TBD ****** Save preformance counters? (user vs. kernel)
//

//
// Accumlate the total time spent in a thread.
//

#if defined(PERF_DATA)
         **** TBD  **** MIPS code

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

#endif // defined(PERF_DATA)

//
// The following entry point is used to switch from the idle thread to
// another thread.
//

        ;;
        ALTERNATE_ENTRY(SwapFromIdle)

//
// Get address of old and new process objects.
//

        alloc     rT1 = ar.pfs, 2, 0, 1, 0
        add       rpT2 = ThApcState+AsProcess,s2 // -> new thread AsProcess
        add       rpT1 = ThApcState+AsProcess,s1 // -> old thread AsProcess
        ;;

        LDPTR     (rOldproc, rpT1)               // old process
        LDPTR     (rNewproc, rpT2)               // new process
        ;;

        flushrs
        add       rpT1 = ThInitialStack, s2
        add       rpT2 = ThKernelStack, s1

        DISABLE_INTERRUPTS(rOldPsrL)
        ;;
        mov       ar.rsc = r0                    // disable RSE

//
// Store the kernel stack pointer in the previous thread object,
// load the new kernel stack pointer from the new thread object,
// switch basking store pointers, select new process id and swap 
// to the new process.
//

        LDPTRINC  (rNewIKS, rpT1, ThKernelStack-ThInitialStack)
        STPTR     (rpT2, sp)                             // save current sp
        ;;

        LDPTRINC  (sp, rpT1, ThStackLimit-ThKernelStack) // new kernel stack
        movl      rpT2 = KiPcr + PcInitialStack
        ;;

        LDPTR     (rNewKSL, rpT1)                        // new stack limit
        st8       [rpT2] = rNewIKS, PcStackLimit-PcInitialStack
        ;;

        st8       [rpT2] = rNewKSL
        add       rpNewBSP = SwBsp+STACK_SCRATCH_AREA, sp
        add       rpNewRNAT = SwRnat+STACK_SCRATCH_AREA, sp
        ;;

        LDPTR     (rNewBSP, rpNewBSP)        // get new BSP/RNAT
        ld8       rNewRNAT = [rpNewRNAT]
        ;;

        alloc     rT1 = 0,0,0,0              // make current frame 0 size
        invala
        ;;

//
// Setup PCR intial kernel BSP and BSTORE limit
//
                
        loadrs                               // invalidate RSE and ALAT
        movl      rpT2 = KiPcr + PcInitialBStore    // -> PCR initial BSP
        ;;

        mov       ar.bspstore = rNewBSP      // load new bspstore
        add       rpT1 = ThInitialBStore,s2  // -> new initial BSP
        add       rpThBSL = ThBStoreLimit,s2 // -> new bstore limit
        ;;

        mov       ar.rnat = rNewRNAT         // load new RNATs
        LDPTR     (rT1, rpT1)                // rT2 = new initial kernel BSP
        ;;

        LDPTR     (rT2, rpThBSL)             // rT1 = new kernel BStore limit
        st8       [rpT2] = rT1, PcBStoreLimit-PcInitialBStore  // save in PCR
        ;;

        mov       ar.rsc = RSC_KERNEL        // enable RSE
        st8       [rpT2] = rT2               // save in PCR
        ;;

//
// If the new process is not the same as the old process, then swap the
// address space to the new process.
//
// N.B. The dispatcher lock cannot be dropped until all references to the
//      old process address space are complete. This includes any possible
//      TB Misses that could occur referencing the new address space while
//      still executing in the old address space.
//
// N.B. The process address space swap is executed with interrupts disabled.
//

        alloc     rT1 = 0,1,2,0
        cmp.ne    pDiff,pSame=rOldproc,rNewproc // if ne, switch process
        ;;

        mov       out0 = rNewproc               // set address of new process
        mov       out1 = rOldproc               // set address of old process
(pDiff) br.call.sptk brp = KxSwapProcess        // call swap address space(NewProc, OldProc)
        ;;
//
// In new address space, if changed.
//

#if !defined(NT_UP)

//
// Release the context swap lock
// N.B. KiContextSwapLock is always released in KxSwapProcess, if called
//

(pSame) add       rpT1 = @gprel(KiContextSwapLock), gp 
        ;;
        PRELEASE_SPINLOCK(pSame, rpT1)

#endif // !defined(NT_UP)

//
// ***** TBD ***** if process swap -- save/restore debug regs?
//

//
// Update TEB
//

        add       rpT1 = ThTeb, s2              // -> new Teb
        movl      rpT2 = KiPcr+PcInitialStack
        ;;

        LDPTR     (teb, rpT1)
        LDPTRINC  (rT1, rpT2, PcCurrentThread-PcInitialStack)
        ;;

//
// ** TBD *** Restore performance counters (user vs. kernel, #, in use flag)
// Exception frame or thead object
//

        mov       kteb = teb                    // update kernel TEB
        STPTRINC  (rpT2, s2, PcInitialStack-PcCurrentThread)
        cmp.ne    pUsTh = zero, teb

        RESTORE_INTERRUPTS(rOldPsrL)
        add       rpT1 = -ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR,rT1
        add       loc0 = ThApcState+AsKernelApcPending, s2
        ;;

//
// If the new thread has a kernel mode APC pending, then request an APC
// interrupt.
//

(pUsTh) ld8       rT1 = [rpT1]
        ld1       loc0 = [loc0]                 // load the ApcPending flag
        ;;
(pUsTh) dep       rT1 = 1, rT1, PSR_DFH, 1
        ;;

(pUsTh) st8       [rpT1] = rT1
        cmp4.ne   pApc = zero, loc0
        REQUEST_APC_INT(pApc)                   // request APC, if pending

//
// Increment context switch counters
//

        add       rpT1 = PbContextSwitches, s0
        add       rpT2 = ThContextSwitches, s2
        add       rpT3 = PbIdleThread, s0  // -> IdleThread address
        ;;

        ld4       rT1 = [rpT1]
        ld4       rT2 = [rpT2]

        LDPTR     (rT3, rpT3)
        ;;
        add       rT1 = rT1, zero, 1
        add       rT2 = rT2, zero, 1
        ;;

        st4       [rpT1] = rT1             // increment # of context switches
        add       rpT3 = ThStackBase, s2

        st4       [rpT2] = rT2             // increment # of context switches
        cmp.eq    pIdle = s2, rT3          // is new thread idle thread?
 (pt1)  br.spnt   Sc20
        ;;

        ld8.nta   rT3 = [rpT3]
        cmp.ne    pUsTh, pKrTh = zero, teb
(pKrTh) br.spnt   Sc20
        ;;

(pUsTh) add       rpT1 = -ThreadStateSaveAreaLength+TsAppRegisters+TsAr21, rT3
(pUsTh) add       rpT2 = -ThreadStateSaveAreaLength+TsAppRegisters+TsAr24, rT3
        ;;

(pUsTh) ld8.nta   rT1 = [rpT1], TsAr25-TsAr21
(pUsTh) ld8.nta   rT2 = [rpT2], TsAr26-TsAr24
        ;;

(pUsTh) mov       ar21 = rT1
(pUsTh) mov       ar24 = rT2

(pUsTh) ld8.nta   rT1 = [rpT1], TsAr27-TsAr25
(pUsTh) ld8.nta   rT2 = [rpT2], TsAr28-TsAr26
        ;;

(pUsTh) mov       ar25 = rT1
(pUsTh) mov       ar26 = rT2
     
(pUsTh) ld8.nta   rT1 = [rpT1], TsAr29-TsAr27
(pUsTh) ld8.nta   rT2 = [rpT2], TsAr30-TsAr28
        ;;

(pUsTh) mov       ar27 = rT1
(pUsTh) mov       ar28 = rT2
     
(pUsTh) ld8.nta   rT1 = [rpT1]
(pUsTh) ld8.nta   rT2 = [rpT2]
        ;;

(pUsTh) mov       ar29 = rT1
(pUsTh) mov       ar30 = rT2

Sc20:
        add       rpT1 = SwRp+STACK_SCRATCH_AREA, sp
        add       rpT2 = SwPFS+STACK_SCRATCH_AREA, sp
        ;;

        ld8       out0 = [rpT1],SwFPSR-SwRp // restore brp and pfs
        ld8       out1 = [rpT2],SwPreds-SwPFS
        ;;

        ld8       rT3 = [rpT1]
        ld8       rT2 = [rpT2]
        ;;

//
// Temporary Serialize and synchronize at the end of task switch for 
// taking care of KeFlushIoBuffer
//
        sync.i
        ;; 
        srlz.i
        ;;
  
//
// Note: at this point s0 = Prcb, s1 = previous thread, s2 = current thread
//

        mov       ar.fpsr = rT3
        mov       pr = rT2                      // Restore preserved preds
        mov       brp = out0
        mov       v0 = loc0                     // set v0 = apc pending
        mov       ar.pfs = out1
        br.ret.sptk brp

        NESTED_EXIT(SwapContext)

        SBTTL("Swap Process")
//++
//--------------------------------------------------------------------
//
// VOID
// KiSwapProcess (
//    IN PKPROCESS NewProcess,
//    IN PKPROCESS OldProcess
//    )
//
// Routine Description:
//
//    This function swaps the address space from one process to antoher by
//    assigning a new region id, if necessary, and loading the fixed entry
//    in the TB that maps the process page directory page. This routine follows
//    the PowerPC design for handling RID wrap.
//
// On entry:
//
//    Interrupt state unknown.
//
// Arguments:
//
//    NewProcess (a0) - Supplies a pointer to a control object of type process
//      which represents the new process that is switched to (32-bit address).
//
//    OldProcess (a1) - Supplies a pointer to a control object of type process
//      which represents the old process that is switched from (32-bit address).
//
// Return Value:
//
//    None.
//
//--------------------------------------------------------------------
//--
        NESTED_ENTRY(KiSwapProcess)
        NESTED_SETUP(2,3,3,0)

        PROLOGUE_END

//
// Register aliases
//

         rNewProc  = a0
         rOldProc  = a1

         rpCSLock  = loc2

         rpT1      = t0
         rpT2      = t1
         rProcSet  = t2
         rNewActive= t3
         rOldActive= t4
         rMasterSeq= t5
         rNewSeq   = t6
         rOldPsrL  = t7
         rVa       = t8
         rPDE0     = t9                          // PDE for page directory page 0
         rVa2      = t10                         
         rSessionBase = t11                   
         rSessionInfo = t12      
         rT1       = t13                         
         rT2       = t14

//
// KiSwapProcess must get the context swap lock
// KxSwapProcess is called from SwapContext with the lock held
//

#if !defined(NT_UP)
        add         rpCSLock = @gprel(KiContextSwapLock), gp
        ;;
        ACQUIRE_SPINLOCK(rpCSLock, rpCSLock, Ksp_Lock1)
        br.sptk     Ksp_Continue
#endif // !defined(NT_UP)
        ;;

        ALTERNATE_ENTRY(KxSwapProcess)
        NESTED_SETUP(2,3,3,0)

#if !defined(NT_UP)
        add         rpCSLock = @gprel(KiContextSwapLock), gp
#endif // !defined(NT_UP)

        PROLOGUE_END
//
// Clear the processor set member number in the old process and set the
// processor member number in the new process.
//

Ksp_Continue:
        
        ARGPTR     (rNewProc)
        ARGPTR     (rOldProc)

#if !defined(NT_UP)

        add       rpT2 = PrActiveProcessors, rOldProc     // -> old active processor set
        movl      rpT1 = KiPcr + PcSetMember              // -> processor set member
        ;;

        ld4       rProcSet= [rpT1]                        // rProcSet.4 =  processor set member
        add       rpT1 = PrActiveProcessors, rNewProc     // -> new active processor set
        ;;

        ld4       rNewActive = [rpT1]                     // rNewActive.4 = new active processor set
        ld4       rOldActive = [rpT2]                     // rOldActive.4 = old active processor set
        ;;

        or        rNewActive = rNewActive,rProcSet        // set processor member in new set
        xor       rOldActive = rOldActive,rProcSet        // clear processor member in old set
        ;;

        st4       [rpT1] = rNewActive           // set new active processor set
        st4       [rpT2] = rOldActive           // set old active processor set

#endif // !defined(NT_UP)

//
// If the process sequence number matches the system sequence number, then
// use the process RID. Otherwise, allocate a new process RID.
//
// N.B. KiMasterRid, KiMasterSequence are changed only when holding the
//      KiContextSwapLock.
//

        add       rT2 = PrSessionMapInfo, rNewProc
        add       out0 = PrProcessRegion, rNewProc
        ;;
        ld8       out1 = [rT2]
        add       rT1  = PrSessionRegion, rNewProc
        ;;

        cmp.eq    pt0 = out1, r0
        ;;
(pt0)   st8      [rT2] = rT1
(pt0)   mov      out1 = rT1
               
        br.call.sptk brp = KiSyncNewRegionId

#if !defined(NT_UP)
//
// Can now release the context swap lock
//

        RELEASE_SPINLOCK(rpCSLock)

#endif // !defined(NT_UP)


//
// Switch address space to new process
// v0 = rRid = new process rid
//

        fwb                                     // hint to flush write buffers

        DISABLE_INTERRUPTS(rOldPsrL)

        add       rpT1 = PrDirectoryTableBase, rNewProc
        add       rpT2 = PrSessionParentBase, rNewProc
        ;;

        ld8.nta   rPDE0 = [rpT1]                // rPDE0 = Page directory page 0
        ld8.nta   rSessionBase = [rpT2]
        ;;

//
// To access IFA, ITDR registers, PSR.ic bit must be 0. Otherwise,
// it causes an illegal operation fault. While PSR.ic=0, any
// interruption can not be afforded. Make sure there will be no
// TLB miss and no interrupt coming in during this period.
//

        rsm       1 << PSR_IC                   // PSR.ic=0
        ;;
        srlz.d                                  // must serialize

        mov       rT1 = PAGE_SHIFT << IDTR_PS   // load page size field for IDTR
        ;;

        mov       cr.itir = rT1                 // set up IDTR for dirbase

        movl      rVa = PDE_UTBASE              // load vaddr for parent dirtable
        ;;

        mov       cr.ifa = rVa                  // set up IFA for dirbase vaddr

        mov       rT2   = DTR_UTBASE_INDEX
        ;;

        itr.d     dtr[rT2] = rPDE0              // insert PDE0 to DTR
        movl      rVa = PDE_STBASE
        ;;

        mov       cr.ifa = rVa
        mov       rT2 = DTR_STBASE_INDEX        
        ;;

        itr.d     dtr[rT2] = rSessionBase       // insert the root for session space
        ;;        

        ssm       1 << PSR_IC                   // PSR.ic=1
        ;;

        srlz.i                                  // must I serialize
        ;;
        RESTORE_INTERRUPTS(rOldPsrL)            // restore PSR.i

        NESTED_RETURN
        NESTED_EXIT(KiSwapProcess)

        SBTTL("Retire Deferred Procedure Call List")
//++
// Routine:
//
//    VOID
//    KiRetireDpcList (
//      PKPRCB Prcb,
//      )
//
// Routine Description:
//
//    This routine is called to retire the specified deferred procedure
//    call list. DPC routines are called using the idle thread (current)
//    stack.
//
//    N.B. Interrupts must be disabled on entry to this routine. Control is returned 
//         to the caller with the same conditions true.
//
// Arguments:
//
//    a0 - Address of the current PRCB.
//
// Return value:
//
//    None.
//
//--

        NESTED_ENTRY(KiRetireDpcList)
        NESTED_SETUP(1,2,4,0)

        PROLOGUE_END


Krdl_Restart:

        add       t0 = PbDpcQueueDepth, a0
        add       t1 = PbDpcRoutineActive, a0
        add       t2 = PbDpcLock, a0
        ;;

        ld4       t4 = [t0]
        add       t3 = PbDpcListHead+LsFlink, a0
        ;;

Krdl_Restart2:

        cmp4.eq   pt1 = zero, t4
        st4       [t1] = t4
 (pt1)  br.spnt   Krdl_Exit
        ;;

#if !defined(NT_UP)
        ACQUIRE_SPINLOCK(t2, a0, Krdl_20)
#endif  // !defined(NT_UP)

        ld4       t4 = [t0]
        LDPTR     (t5, t3)             // -> first DPC entry
        ;;
        cmp4.eq   pt1, pt2 = zero, t4
        ;;

 (pt2)  add       t10 = LsFlink, t5
 (pt2)  add       out0 = -DpDpcListEntry, t5
 (pt1)  br.spnt   Krdl_Unlock
        ;;

        LDPTR     (t6, t10)
        add       t11 = DpDeferredRoutine, out0
        add       t12 = DpSystemArgument1, out0
        ;;

//
// Setup call to DPC routine
//
// arguments are:
//      dpc object address (out0)
//      deferred context   (out1)
//      system argument 1  (out2)
//      system argument 2  (out3)
//
// N.B. the arguments must be loaded from the DPC object BEFORE
//      the inserted flag is cleared to prevent the object being
//      overwritten before its time.
//

        ld8.nt1   t13 = [t11], DpDeferredContext-DpDeferredRoutine
        ld8.nt1   out2 = [t12], DpSystemArgument2-DpSystemArgument1
        ;;

        ld8.nt1   out1 = [t11], DpLock-DpDeferredContext
        ld8.nt1   out3 = [t12]
        add       t4 = -1, t4

        STPTRINC  (t3, t6, -LsFlink)
        ld8.nt1   t14 = [t13], 8
        add       t15 = LsBlink, t6
        ;;

        ld8.nt1   gp = [t13]
        STPTR     (t15, t3)
  
        STPTR     (t11, zero)
        st4       [t0] = t4

#if !defined(NT_UP)
        RELEASE_SPINLOCK(t2)             // set spin lock not owned
#endif //!defined(NT_UP)

        FAST_ENABLE_INTERRUPTS
        mov       bt0 = t14
        br.call.sptk.few.clr brp = bt0          // call DPC routine
        ;;

//
// Check to determine if any more DPCs are available to process.
//

        FAST_DISABLE_INTERRUPTS
        br        Krdl_Restart
        ;;

//
// The DPC list became empty while we were acquiring the DPC queue lock.
// Clear DPC routine active.  The race condition mentioned above doesn't
// exist here because we hold the DPC queue lock.
//

Krdl_Unlock:

#if !defined(NT_UP)
        add       t2 = PbDpcLock, a0
        ;;
        RELEASE_SPINLOCK(t2)
#endif // !defined(NT_UP)

Krdl_Exit:

        add       t0 = PbDpcQueueDepth, a0
        add       t1 = PbDpcRoutineActive, a0
        add       out0 = PbDpcInterruptRequested, a0
        ;;

        st4.nta   [t1] = zero
        st4.rel.nta [out0] = zero
        add       t2 = PbDpcLock, a0

        ld4       t4 = [t0]
        add       t3 = PbDpcListHead+LsFlink, a0
        ;;

        cmp4.eq   pt1, pt2 = zero, t4
 (pt2)  br.spnt   Krdl_Restart2
        ;;

        NESTED_RETURN
        NESTED_EXIT(KiRetireDpcList)

        SBTTL("Dispatch Interrupt")
//++
//--------------------------------------------------------------------
// Routine:
//
//     KiDispatchInterrupt
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
//    N.B. On entry to this routine the volatile states (excluding high 
//         floating point register set) have been saved.
//
// On entry:
//
//    sp - points to stack scratch area.
//
// Arguments:
//
//    None
//
// Return Value:
//
//    None.
//--------------------------------------------------------------------
//--
        NESTED_ENTRY(KiDispatchInterrupt)
        PROLOGUE_BEGIN

        .regstk   0, 4, 1,0
        alloc     t16 = ar.pfs, 0, 4, 1, 0
        .save     rp, loc0
        mov       loc0 = brp
        .fframe   SwitchFrameLength
        add       sp = -SwitchFrameLength, sp
        ;;

        .save     ar.unat, loc1
        mov       loc1 = ar.unat
        add       t0 = ExFltS19+SwExFrame+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;

        .save.gf  0x0, 0xC0000
        stf.spill [t0] = fs19, ExFltS17-ExFltS19
        stf.spill [t1] = fs18, ExFltS16-ExFltS18
        ;;

        .save.gf  0x0, 0x30000
        stf.spill [t0] = fs17, ExFltS15-ExFltS17
        stf.spill [t1] = fs16, ExFltS14-ExFltS16
        mov       t10 = bs4
        ;;

        .save.gf  0x0, 0xC000
        stf.spill [t0] = fs15, ExFltS13-ExFltS15
        stf.spill [t1] = fs14, ExFltS12-ExFltS14
        mov       t11 = bs3
        ;;

        .save.gf  0x0, 0x3000
        stf.spill [t0] = fs13, ExFltS11-ExFltS13
        stf.spill [t1] = fs12, ExFltS10-ExFltS12
        mov       t12 = bs2
        ;;

        .save.gf  0x0, 0xC00
        stf.spill [t0] = fs11, ExFltS9-ExFltS11
        stf.spill [t1] = fs10, ExFltS8-ExFltS10
        mov       t13 = bs1
        ;;

        .save.gf  0x0, 0x300
        stf.spill [t0] = fs9, ExFltS7-ExFltS9
        stf.spill [t1] = fs8, ExFltS6-ExFltS8
        mov       t14 = bs0
        ;;

        .save.gf  0x0, 0xC0
        stf.spill [t0] = fs7, ExFltS5-ExFltS7
        stf.spill [t1] = fs6, ExFltS4-ExFltS6
        mov       t15 = ar.lc
        ;;

        .save.gf  0x0, 0x30
        stf.spill [t0] = fs5, ExFltS3-ExFltS5
        stf.spill [t1] = fs4, ExFltS2-ExFltS4
        ;;

        .save.f   0xC
        stf.spill [t0] = fs3, ExFltS1-ExFltS3         // save fs3
        stf.spill [t1] = fs2, ExFltS0-ExFltS2         // save fs2
        ;;

        .save.f   0x3
        stf.spill [t0] = fs1, ExBrS4-ExFltS1          // save fs1
        stf.spill [t1] = fs0, ExBrS3-ExFltS0          // save fs0
        ;;

        .save.b   0x18
        st8       [t0] = t10, ExBrS2-ExBrS4           // save bs4
        st8       [t1] = t11, ExBrS1-ExBrS3           // save bs3
        ;;

        .save.b   0x6
        st8       [t0] = t12, ExBrS0-ExBrS2           // save bs2
        st8       [t1] = t13, ExIntS2-ExBrS1          // save bs1
        ;;

        .save.b   0x1
        st8       [t0] = t14, ExIntS3-ExBrS0          // save bs0
        ;;

        .save.gf  0xC, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s3, ExIntS1-ExIntS3          // save s3
        .mem.offset 8,0
        st8.spill [t1] = s2, ExIntS0-ExIntS2          // save s2
        ;;

        .save.gf  0x3, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s1, ExApLC-ExIntS1           // save s1
        .mem.offset 8,0
        st8.spill [t1] = s0, ExApEC-ExIntS0           // save s0
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        ;;

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3

        PROLOGUE_END

//
// Register aliases
//

        rPrcb     = loc2
        rKerGP    = loc3

        rpT1      = t0
        rpT2      = t1
        rT1       = t2
        rT2       = t3
        rpDPLock  = t4                          // pointer to dispatcher lock

        pNoTh     = pt1                         // No next thread to run
        pNext     = pt2                         // next thread not null
        pNull     = pt3                         // no thread available
        pOwned    = pt4                         // dispatcher lock already owned
        pNotOwned = pt5
        pQEnd     = pt6                         // quantum end request pending
        pNoQEnd   = pt7                         // no quantum end request pending

//
// Increment the dispatch interrupt count
//

        mov       rKerGP = gp                   // save gp
        movl      rPrcb = KiPcr + PcPrcb
        ;;

        LDPTR     (rPrcb, rPrcb)                 // rPrcb -> Prcb
        ;;
        add       rpT1 = PbDispatchInterruptCount, rPrcb
        ;;
        ld4       rT1 = [rpT1]
        ;;
        add       rT1 = rT1, zero, 1
        ;;
        st4       [rpT1] = rT1

// **** TBD **** use alpha optimization to first check Dpc Q depth


//
// Process the DPC list
//

Kdi_PollDpcList:

//
// Process the deferred procedure call list.
//

        FAST_ENABLE_INTERRUPTS
        ;;
        srlz.d
        ;;
        
//
// **** TBD ***** No stack switch as in alpha, mips...
// Save current initial stack address and set new initial stack address.
//

        FAST_DISABLE_INTERRUPTS
        mov      out0 = rPrcb
        br.call.sptk brp = KiRetireDpcList
        ;;
        

//
// Check to determine if quantum end has occured.
//
// N.B. If a new thread is selected as a result of processing a quantum
//      end request, then the new thread is returned with the dispatcher
//      database locked. Otherwise, NULL is returned with the dispatcher
//      database unlocked.
//

        FAST_ENABLE_INTERRUPTS
        add       rpT1 = PbQuantumEnd, rPrcb
        ;;

        ld4       rT1 = [rpT1]                  // get quantum end indicator
        ;;
        cmp4.ne   pQEnd, pNoQEnd = rT1, zero    // if zero, no quantum end reqs
        mov       gp = rKerGP                   // restore gp
        ;;

(pQEnd) st4       [rpT1] = zero                 // clear quantum end indicator
(pNoQEnd) br.cond.sptk Kdi_NoQuantumEnd
(pQEnd) br.call.spnt brp = KiQuantumEnd         // call KiQuantumEnd (C code)
        ;;

        cmp4.eq   pNoTh, pNext = v0, zero       // pNoTh = no next thread
(pNoTh) br.dpnt   Kdi_Exit                      // br to exit if no next thread
(pNext) br.dpnt   Kdi_Swap                      // br to swap to next thread

//
// If no quantum end requests:
// Check to determine if a new thread has been selected for execution on
// this processor.
//

Kdi_NoQuantumEnd:
        add       rpT2 = PbNextThread, rPrcb
        ;;
        LDPTR     (rT1, rpT2)                   // rT1 = address of next thread object
        ;;

        cmp.eq    pNull = rT1, zero             // pNull => no thread selected
(pNull) br.dpnt   Kdi_Exit                      // exit if no thread selected

#if !defined(NT_UP)

//
// Disable interrupts and try to acquire the dispatcher database lock.
//

        FAST_DISABLE_INTERRUPTS
        add       rpDPLock = @gprel(KiDispatcherLock), gp
        ;;

        xchg8.nt1 rT2 = [rpDPLock], rpDPLock    // try to obtain lock
        ;;

        cmp.ne    pOwned, pNotOwned = zero, rT2 // pOwned = 1 if not free
        ;;
        PSET_IRQL(pNotOwned, SYNCH_LEVEL)
(pOwned) br.dpnt   Kdi_PollDpcList              // br out if owned
        ;;

//
// Lock obtained -- raise IRQL to synchronization level
//

        FAST_ENABLE_INTERRUPTS

#endif // !defined(NT_UP)

//
// Reread address of next thread object since it is possible for it to
// change in a multiprocessor system.
//

Kdi_Swap:

        add       rpT2 = PbNextThread, rPrcb    // -> next thread
        movl      rpT1 = KiPcr + PcCurrentThread
        ;;

        LDPTR     (s1, rpT1)                    // current thread object
        LDPTR     (s2, rpT2)                    // next thread object
        add       rpT1 = PbCurrentThread, rPrcb
        ;;


//
// Reready current thread for execution and swap context to the selected thread.
//

        STPTR     (rpT2, zero)                  // clear addr of next thread
        STPTR     (rpT1, s2)                    // set addr of current thread
        mov       out0 = s1                     // set addr of previous thread
        mov       gp =rKerGP                    // restore gp
        br.call.sptk brp = KiReadyThread        // call KiReadyThread(OldTh)
        ;;

        mov       gp = rKerGP                   // restore gp
        mov       s0 = rPrcb                    // setup call
        br.call.sptk brp = SwapContext          // call SwapContext(Prcb, OldTh, NewTh)
        ;;

//
// Restore saved registers, and return.
//

        add       out0 = STACK_SCRATCH_AREA+SwExFrame, sp
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

Kdi_Exit:

        add       rpT1 = ExApEC+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;
        ld8       rT1 = [rpT1]
        mov       brp = loc0
        ;;

        mov       ar.unat = loc1
        mov       ar.pfs = rT1
        .restore
        add       sp = SwitchFrameLength, sp
        br.ret.sptk brp

        NESTED_EXIT(KiDispatchInterrupt)
