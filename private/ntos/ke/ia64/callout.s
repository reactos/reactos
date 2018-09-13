//++
//
// Copyright (c) 1994  Microsoft Corporation
//
// Module Name:
//
//    callout.s
//
// Abstract:
//
//    This module implements the code necessary to call out from kernel
//    mode to user mode.
//
// Author:
//
//    William K. Cheung (wcheung) 30-Oct-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksia64.h"

        PublicFunction(MmGrowKernelStack)
        PublicFunction(MmGrowKernelBackingStore)
        PublicFunction(KiUserServiceExit)
        PublicFunction(KiApcInterrupt)
        PublicFunction(RtlMoveMemory)

        .global     KeUserCallbackDispatcher

//++
//
// NTSTATUS
// KiCallUserMode (
//    IN PVOID *OutputBuffer,
//    IN PULONG OutputLength
//    )
//
// Routine Description:
//
//    This function calls a user mode function.
//
//    N.B. This function calls out to user mode and the NtCallbackReturn
//        function returns back to the caller of this function. Therefore,
//        the stack layout must be consistent between the two routines.
//
// Arguments:
//
//    OutputBuffer (a0) - Supplies a pointer to the variable that receivies
//        the address of the output buffer.
//
//    OutputLength (a1) - Supplies a pointer to a variable that receives
//        the length of the output buffer.
//
// Return Value:
//
//    The final status of the call out function is returned as the status
//    of the function.
//
//    N.B. This function does not return to its caller. A return to the
//        caller is executed when a NtCallbackReturn system service is
//        executed.
//
//    N.B. This function does return to its caller if a kernel stack
//         expansion is required and the attempted expansion fails.
//
//         The instruction that restores the sp (i.e. epilogue) must be
//         in the last bundle of the function.  This is a convention
//         by which this functin must abide or the stack unwinder in the
//         imagehlp DLL will not work.
//
//--


        NESTED_ENTRY(KiCallUserMode)

        .regstk  2, 8, 2, 0
        .prologue

        rT0         = t0
        rT1         = t1
        rT2         = t2
        rT3         = t3
        rT4         = t4
        rT5         = t5
        rT6         = t6
        rT7         = t7

        rpT0        = t10
        rpT1        = t11
        rpT2        = t12
        rpT3        = t13
        rpT4        = t14
        rpT5        = t15
        rpT6        = t16

        rpCurTh     = loc0
        rbsp        = loc1
        rbspInit    = loc2
        rpTF        = loc7
          
//
// allocate stack frame to save preserved floating point registers
//

        alloc       rT1 = ar.pfs, 2, 8, 2, 0
        mov         rT0 = ar.unat
        mov         rT2 = brp

        ARGPTR      (a0)
        ARGPTR      (a1)

//
// save both preserved integer and float registers
//

        add         rpT1 = -CuFrameLength+CuBrRp+STACK_SCRATCH_AREA, sp
        add         rpT2 = -CuFrameLength+CuRsPFS+STACK_SCRATCH_AREA, sp
        add         sp = -CuFrameLength, sp
        ;;

        st8.nta     [rpT1] = rT2, CuPreds - CuBrRp          // save rp
        st8.nta     [rpT2] = rT1, CuApUNAT - CuRsPFS        // save pfs
        mov         rT3 = pr
        ;;

        st8.nta     [rpT1] = rT3, CuApLC - CuPreds          // save predicates
        st8.nta     [rpT2] = rT0, CuIntS0 - CuApUNAT        // save ar.unat
        mov         rT4 = ar.lc
        ;;

        st8.nta     [rpT1] = rT4, CuIntS1 - CuApLC          // save ar.lc
        st8.spill.nta [rpT2] = s0, CuIntS2 - CuIntS0        // save s0
        mov         rT0 = bs0
        ;;
        .mem.offset 0,0
        st8.spill.nta [rpT1] = s1, CuIntS3 - CuIntS1        // save s1
        .mem.offset 8,0
        st8.spill.nta [rpT2] = s2, CuBrS0 - CuIntS2         // save s2
        mov         rT1 = bs1
        ;;

        st8.spill.nta [rpT1] = s3, CuBrS1 - CuIntS3         // save s3
        st8.nta     [rpT2] = rT0, CuBrS2 - CuBrS0           // save bs0
        mov         rT2 = bs2
        ;;

        flushrs
        mov         ar.rsc = r0                             // put RSE in lazy mode
        mov         rT6 = ar.unat
        ;;

        mov         rT5 = ar.rnat
        mov         rT3 = bs3
        ;;

        st8.nta     [rpT1] = rT1, CuBrS3 - CuBrS1           // save bs1
        st8.nta     [rpT2] = rT2, CuBrS4 - CuBrS2           // save bs2
        mov         rT4 = bs4
        ;;

        st8.nta     [rpT1] = rT3, CuRsRNAT - CuBrS3         // save bs3
        st8.nta     [rpT2] = rT4, CuIntNats - CuBrS4        // save bs4
        ;;

        st8.nta     [rpT1] = rT5, CuFltS0 - CuRsRNAT        // save rnat
        st8.nta     [rpT2] = rT6, CuFltS1 - CuIntNats       // save NaTs
        ;;

        stf.spill.nta [rpT1] = fs0, CuFltS2 - CuFltS0       // save fs0
        stf.spill.nta [rpT2] = fs1, CuFltS3 - CuFltS1       // save fs1
        mov         v0 = zero                               // set v0 to 0
        ;;

        stf.spill.nta [rpT1] = fs2, CuFltS4 - CuFltS2       // save fs2
        stf.spill.nta [rpT2] = fs3, CuFltS5 - CuFltS3       // save fs3
        ;;

        stf.spill.nta [rpT1] = fs4, CuFltS6 - CuFltS4       // save fs4
        stf.spill.nta [rpT2] = fs5, CuFltS7 - CuFltS5       // save fs5
        nop.i       0
        ;;

        stf.spill.nta [rpT1] = fs6, CuFltS8 - CuFltS6       // save fs6
        stf.spill.nta [rpT2] = fs7, CuFltS9 - CuFltS7       // save fs7
        nop.i       0
        ;;

        stf.spill.nta [rpT1] = fs8, CuFltS10 - CuFltS8      // save fs8
        stf.spill.nta [rpT2] = fs9, CuFltS11 - CuFltS9      // save fs9
        nop.i       0
        ;;

        stf.spill.nta [rpT1] = fs10, CuFltS12 - CuFltS10    // save fs10
        stf.spill.nta [rpT2] = fs11, CuFltS13 - CuFltS11    // save fs11
        nop.i       0
        ;;

        stf.spill.nta [rpT1] = fs12, CuFltS14 - CuFltS12    // save fs12
        stf.spill.nta [rpT2] = fs13, CuFltS15 - CuFltS13    // save fs13
        nop.i       0
        ;;

        stf.spill.nta [rpT1] = fs14, CuFltS16 - CuFltS14    // save fs14
        stf.spill.nta [rpT2] = fs15, CuFltS17 - CuFltS15    // save fs15
        nop.i       0
        ;;

        stf.spill.nta [rpT1] = fs16, CuFltS18 - CuFltS16    // save fs16
        stf.spill.nta [rpT2] = fs17, CuFltS19 - CuFltS17    // save fs17
        nop.i       0
        ;;

        stf.spill.nta [rpT1] = fs18, CuA0 - CuFltS18        // save fs18
        stf.spill.nta [rpT2] = fs19, CuA1 - CuFltS19        // save fs19
        nop.i       0
        ;;

        PROLOGUE_END

//
// Check if sufficient rooms are available on both the kernel
// memory stack and backing store for another system call.
// Call the MM functions to grow them if necessary.
//

        mov         rbsp = ar.bsp
        movl        loc3 = KiPcr + PcCurrentThread

        st8.nta     [rpT1] = a0
        st8.nta     [rpT2] = a1
        mov         rT2 = 0x1ff
        ;;

        add         rbspInit = 0x200, rbsp
        mov         rT0 = sp
        ;;
        andcm       rbspInit = rbspInit, rT2
        ;;

        LDPTRINC    (rpCurTh, loc3, PcStackLimit - PcCurrentThread)
        add         rT0 = -KERNEL_LARGE_STACK_COMMIT, rT0
        mov         rT1 = rbspInit
        ;;

        add         loc4 = ThStackLimit, rpCurTh
        add         loc5 = ThBStoreLimit, rpCurTh
        add         rT1 = KERNEL_LARGE_BSTORE_COMMIT, rT1
        ;;

//
// check if it is necessary to grow the kernel stack and backing store
//

        LDPTR       (rT2, loc4)                   // Get current stack limit.
        LDPTR       (rT3, loc5)                   // Get current bstore limit
        mov         out0 = sp
        ;;

        cmp.ge      ps0 = rT2, rT0
        cmp.ge      ps1 = rT1, rT3
(ps0)   br.call.spnt.many brp = MmGrowKernelStack

//
// Get expanded stack limit from thread object and set the stack limit
// in PCR if the growth of kernel stack is successful.
//

        LDPTR       (rT0, loc4)
        cmp4.ne     pt2, pt1 = zero, v0
        ;;

        nop.m       0
(pt1)   st8         [loc3] = rT0, PcBStoreLimit - PcStackLimit
(pt2)   br.spnt     Kcum10
        ;;

        mov         out0 = rbspInit
(ps1)   br.call.spnt.many brp = MmGrowKernelBackingStore
        ;;

//
// Get expanded bstore limit from thread object and set the bstore limit
// in PCR if the growth of kernel backing store is successful.
//

        LDPTR       (rT0, loc5)
        cmp4.ne     pt2, pt1 = zero, v0
        add         loc4 = ThCallbackStack - ThStackLimit, loc4
        add         loc5 = ThCallbackBStore - ThBStoreLimit, loc5
        ;;


        rPcInStack = rT0
        rPcInBStore = rT1
        rThCbStack = rT2
        rThCbBStore = rT3
        rpLabel = rT4


        PLDPTRINC   (pt1, rThCbStack,loc4, ThTrapFrame - ThCallbackStack)
(pt1)   st8         [loc3] = rT0, PcInitialStack - PcBStoreLimit
(pt2)   br.spnt     Kcum10
        ;;


//
// Get the address of the current thread and save the previous trap
// frame and callback stack addresses in the current frame.  Also
// save the new callback stack address in the thread object.
//
// Get initial and callback stack & backing store addresses
//

        ld8.nt1     rPcInStack = [loc3], PcInitialBStore - PcInitialStack
        LDPTRINC    (rpTF, loc4, ThCallbackStack - ThTrapFrame)
        add         rpT1 = @gprel(KeUserCallbackDispatcher), gp
        ;;

        ld8.nt1     rPcInBStore = [loc3], PcInitialStack-PcInitialBStore
        LDPTR       (rThCbBStore, loc5)
        add         rpT2 = CuInStack+STACK_SCRATCH_AREA, sp
        ;;

        LDPTR       (rpLabel, rpT1)
        add         rpT5 = CuInBStore+STACK_SCRATCH_AREA, sp
        add         loc6 = PcInitialBStore - PcInitialStack, loc3
        ;;

        STPTR       (loc4, sp)                       // set callback stack addr
        STPTR       (loc5, rbsp)                     // set callback bstore addr
        add         loc4 = ThInitialStack - ThCallbackStack, loc4

//
// save initial stack and backing store addresses
//

        st8.nta     [rpT2] = rPcInStack, CuCbStk - CuInStack
        st8.nta     [rpT5] = rPcInBStore, CuCbBStore - CuInBStore
        add         loc5 = ThInitialBStore - ThCallbackBStore, loc5
        ;;

//
// save callback stack and backing store addresses
//

        st8.nta     [rpT2] = rThCbStack, CuTrFrame - CuCbStk
        st8.nta     [rpT5] = rThCbBStore, CuTrStIIP - CuCbBStore
        ;;

//
// register aliases
//

        rpEntry=rT0
        rIPSR=rT1
        rIIP=rT2


        rsm         1 << PSR_I                  // disable interrupts
        movl        rT5 = MM_EPC_VA
        ;;

        ld8.nt1     rpEntry = [rpLabel], 8      // get continuation IP
        st8.nta     [rpT2] = rpTF               // save trap frame address
        add         rpT3 = TrStIIP, rpTF
        ;;

        ld8.nt1     gp = [rpLabel]              // get continuation GP
        STPTR       (loc4, sp)                  // reset initial stack addr
        mov         rT6 = @secrel(KiUserServiceExit)
        ;;

        ld8         rIIP = [rpT3], TrStIPSR-TrStIIP  // get trap IIP
        STPTR       (loc5, rbspInit)            // reset initial bstore addr
        add         rT6 = rT6, rT5
        ;;

        ld8         rIPSR = [rpT3], TrStIIP-TrStIPSR
        st8.nta     [loc3] = sp                 // reset initial stack addr
        add         rpT1 = -ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR, sp
        ;;

        st8.nta     [loc6] = rbspInit           // reset initial bstore addr
        st8.nta     [rpT5] = rIIP               // save original IIP
        mov         bt0 = rT6
        ;;

        st8         [rpT1] = rIPSR
        st8         [rpT3] = rpEntry            // set trap IIP
        cmp4.eq     ps1, ps2 = 0, r0            // preset predicates for
                                                // KiUserServiceExit
                                                // N.B. ps1 -- pUser
                                                //      ps2 -- pKrnl
        ;;

        add         rpT1 = ThApcState+AsUserApcPending, rpCurTh
        ;;
        ld1         rT2 = [rpT1], ThAlerted-ThApcState-AsUserApcPending
        mov         t0 = rpTF                   // set t0 -> trap frame address
        ;;                                      // per system call convention

        st1         [rpT1] = zero
        cmp4.eq     pt0 = zero, rT2
(pt0)   br.sptk     bt0                         // no user apc pending,
        ;;                                      // branch to KiUserServiceExit

        FAST_ENABLE_INTERRUPTS
        SET_IRQL    (APC_LEVEL)
        ;;

        mov         out1 = rpTF
        br.call.sptk brp = KiApcInterrupt
        ;;

        FAST_DISABLE_INTERRUPTS
        SET_IRQL    (zero)

        mov         t0 = rpTF                   // set t0 -> trap frame address
        br          bt0                         // per system call convention

//
// An attempt to grow the kernel stack or backing store failed.
//

Kcum10:

        nop.m       0
        add         rpT1 = CuBrRp+STACK_SCRATCH_AREA, sp
        add         rpT2 = CuRsPFS+STACK_SCRATCH_AREA, sp
        ;;

        ld8.nt1     rT1 = [rpT1], CuPreds-CuBrRp
        ;;
        ld8.nt1     rT2 = [rpT2]
        mov         brp = rT1
        ;;

        ld8.nt1     rT3 = [rpT1]
        mov         ar.pfs = rT2
        ;;
        mov         pr = rT3, -1
        ;;

        .restore
        add         sp = CuFrameLength, sp
        nop.i       0
        br.ret.sptk.clr brp

        NESTED_EXIT(KiCallUserMode)


//++
//
// PVOID
// KeSwitchKernelStack (
//    IN PVOID StackBase,
//    IN PVOID StackLimit,
//    IN PVOID BStoreLimit
//    )
//
// Routine Description:
//
//    This function switches to the specified large kernel stack.
//
//    N.B. This function can ONLY be called when there are no variables
//        in the stack that refer to other variables in the stack, i.e.,
//        there are no pointers into the stack.
//
// Arguments:
//
//    StackBase (a0) - Supplies a pointer to the base of the new kernel
//        stack.
//
//    StackLimit (a1) - supplies a pointer to the limit of the new kernel
//        stack.
//
//    BStoreLimit (a2) - supplies a pointer to the limit of the new kernel
//        backing store.
//
// Return Value:
//
//    The old kernel stack/backing store base is returned as the function value.
//
//--


        NESTED_ENTRY(KeSwitchKernelStack)

        .regstk   3, 4, 3, 0
        .prologue

        //
        // register aliases
        //

        rpCurTh     = loc0
        rThStackBase= loc1
        rbsp        = loc2
        rT0         = t0
        rT1         = t1
        rT2         = t2
        rT3         = t3
        rT4         = t4
        rT5         = t5
        rT6         = t6
        rT7         = t7
        rpT0        = t10
        rpT1        = t11
        rpT2        = t12
        rpT3        = t13
        rpT4        = t14
        rpT5        = t15
        rpT6        = t16


        alloc       rT1 = ar.pfs, 3, 3, 3, 0
        mov         rpT3 = sp
        mov         rT0 = brp
        ;;

        mov         rbsp = ar.bsp
        movl        rpT0 = KiPcr+PcCurrentThread
        ;;

        flushrs
        add         sp = -16, sp                // allocate space for brp, pfs
        ;;

        LDPTR       (rpCurTh, rpT0)
        st8         [rpT3] = rT0, 8     // save brp, pfs in old scratch area 
        ;;

        st8         [rpT3] = rT1
        add         rpT1 = ThStackBase, rpCurTh
        add         rpT2 = ThTrapFrame, rpCurTh
        ;;

        PROLOGUE_END

        LDPTR       (rThStackBase, rpT1)
        LDPTR       (rT2, rpT2)                 // get trap frame address
        mov         out1 = sp
        ;;

//
// relocate trap frame.
// copy memory stack and backing store.
//

        sub         rT1 = rThStackBase, rT2
        ;;
        sub         out2 = rThStackBase, sp     // compute the copy size
        sub         rT2 = a0, rT1
        ;;

        STPTR       (rpT2, rT2)                 // save new trap frame address
        sub         out0 = a0, out2
 (p0)   br.call.sptk.many brp = RtlMoveMemory
        ;;
         
        sub         out2 = rbsp, rThStackBase
        mov         out0 = a0
        mov         out1 = rThStackBase
        ;;

        mov         ar.rsc = r0                 // put RSE in lazy mode
        mov         rbsp = out2
 (p0)   br.call.sptk.many brp = RtlMoveMemory
        ;;
         
        rsm         1 << PSR_I                  // disable interrupts
        mov         rpT0 = rpCurTh
        sub         rT1 = rThStackBase, sp

        add         rpT1 = ThInitialStack, rpCurTh
        add         rpT2 = ThStackLimit, rpCurTh
        ;;

//
// interrupt disabled, then update kernel stack/bstore base,
// then switch kernel stack and backing store
//

//
// set new initial stack and stack base addresses in the kernel thread object.
// set the return value to the old stack base address.
//

        STPTRINC    (rpT1, a0, ThInitialBStore - ThInitialStack)
        STPTRINC    (rpT2, a1, ThBStoreLimit - ThStackLimit)
        add         rpT5 = 16, sp
        ;;

        STPTR       (rpT1, a0)
        STPTR       (rpT2, a2)
        add         rpT6 = 24, sp
        ;;

        add         rpT1 = ThStackBase, rpCurTh
        add         rpT2 = ThLargeStack, rpCurTh
        add         rT5 = 1, r0
        ;;

        STPTR       (rpT1, a0)                  // save new stack/bstore base
        st1         [rpT2] = rT5                // large stack indicator
        nop.i       0

        ld8         rT3 = [rpT5]                // restore return pointer
        movl        rpT3 = KiPcr+PcInitialStack
        ;;

        ld8         rT4 = [rpT6]                // restore pfs
        add         rpT4 = PcStackLimit - PcInitialStack, rpT3
        mov         v0 = rThStackBase
        ;;

//
// set new initial stack/bstore and their limits in the PCR
//

        st8         [rpT3] = a0, PcInitialBStore - PcInitialStack
        st8         [rpT4] = a1, PcBStoreLimit - PcStackLimit
        sub         sp = a0, rT1                // switch to new kernel stack
        ;;

        st8         [rpT3] = a0
        st8         [rpT4] = a2
        add         rT2 = rbsp, a0
        ;;

        alloc       rT5 = ar.pfs, 0, 0, 0, 0
        ;;
        mov         rT6 = ar.rnat
        mov         brp = rT3
        ;;

        loadrs
        ;;
        mov         ar.bspstore = rT2           // switch to new backing store
        mov         ar.pfs = rT4
        ;;

        mov         ar.rnat = rT6
        ssm         1 << PSR_I                  // enable interrupt
        .restore
        add         sp = 16, sp                 // deallocate stack frame
        ;;

        invala
        mov         ar.rsc = RSC_KERNEL
        ;;

        br.ret.sptk.clr brp

        NESTED_EXIT(KeSwitchKernelStack)


//++
//
// NTSTATUS
// NtCallbackReturn (
//    IN PVOID OutputBuffer OPTIONAL,
//    IN ULONG OutputLength,
//    IN NTSTATUS Status
//    )
//
// Routine Description:
//
//    This function returns from a user mode callout to the kernel
//    mode caller of the user mode callback function.
//
//    N.B. This function returns to the function that called out to user
//        mode and the KiCallUserMode function calls out to user mode.
//        Therefore, the stack layout must be consistent between the
//        two routines.
//
//        t0 - current trap frame address
//
// Arguments:
//
//    OutputBuffer - Supplies an optional pointer to an output buffer.
//
//    OutputLength - Supplies the length of the output buffer.
//
//    Status - Supplies the status value returned to the caller of the
//        callback function.
//
// Return Value:
//
//    If the callback return cannot be executed, then an error status is
//    returned. Otherwise, the specified callback status is returned to
//    the caller of the callback function.
//
//    N.B. This function returns to the function that called out to user
//         mode is a callout is currently active.
//
//--

        LEAF_ENTRY(NtCallbackReturn)

        .regstk     3, 1, 0, 0

        rT0         = t1
        rT1         = t2
        rT2         = t3
        rT3         = t4
        rT4         = t5
        rT5         = t6
        rT6         = t7
        rT7         = t8
        rT8         = t9
        rT9         = t10
        rpT0        = t11
        rpT1        = t12
        rpT2        = t13
        rpT3        = t14
        rpT4        = t15
        rpT5        = t16
        rpT6        = t17

        rIPSR       = t18
        rpCurTh     = t19
        rThCbStack  = t20
        rThCbBStore = t21
        rRnat       = loc0


        alloc       rT6 = ar.pfs, 3, 1, 0, 0
        movl        rpT0 = KiPcr+PcCurrentThread
        ;;

        LDPTR       (rpCurTh, rpT0)
        add         rIPSR = TrStIPSR, t0
        ;;

        ld8         rIPSR = [rIPSR]
        movl        rT1 = 1 << PSR_DB | 1 << PSR_TB | 1 << PSR_SS | 1 << PSR_LP

        add         rpT0 = ThCallbackStack, rpCurTh
        add         rpT3 = ThCallbackBStore, rpCurTh
        ;;

        LDPTR       (rThCbStack, rpT0)          // get callback stack address
        LDPTR       (rThCbBStore, rpT3)         // get callback bstore address
        and         rIPSR = rIPSR, rT1          // capture db, tb, ss, lp bits
        ;;

        cmp.eq      pt1, pt2 = zero, rThCbStack
        add         rpT1 = CuIntNats+STACK_SCRATCH_AREA, rThCbStack
        add         rpT2 = CuApLC+STACK_SCRATCH_AREA, rThCbStack
        ;;

  (pt2) ld8.nt1     rT0 = [rpT1], CuPreds - CuIntNats
  (pt1) movl        v0 = STATUS_NO_CALLBACK_ACTIVE

  (pt2) ld8.nt1     rT1 = [rpT2], CuBrRp - CuApLC
        nop.m       0
  (pt1) br.ret.sptk.clr brp
        ;;

        ld8.nt1     rT2 = [rpT1], CuRsPFS - CuPreds
        ld8.nt1     rT3 = [rpT2], CuBrS0 - CuBrRp
        mov         v0 = a2                     // set callback service status
        ;;

        ld8.nt1     rT4 = [rpT1], CuBrS1 - CuRsPFS
        ld8.nt1     rT5 = [rpT2], CuBrS2 - CuBrS0
        nop.i       0
        ;;

        mov         ar.unat = rT0
        mov         ar.lc = rT1
        nop.i       0
          
        ld8.nt1     rT6 = [rpT1], CuBrS3 - CuBrS1
        ld8.nt1     rT7 = [rpT2], CuBrS4 - CuBrS2
        mov         pr = rT2, -1
        ;;

        ld8.nt1     rT8 = [rpT1], CuIntS0 - CuBrS3
        ld8.nt1     rT9 = [rpT2], CuIntS1 - CuBrS4
        mov         brp = rT3
        ;;

        ld8.fill.nt1 s0 = [rpT1], CuIntS2 - CuIntS0
        ld8.fill.nt1 s1 = [rpT2], CuIntS3 - CuIntS1
        mov         ar.pfs = rT4
        ;;

        ld8.fill.nt1 s2 = [rpT1], CuApUNAT - CuIntS2
        ld8.fill.nt1 s3 = [rpT2], CuRsRNAT - CuIntS3
        mov         bs0 = rT5
        ;;
          
        ld8.nt1     rT0 = [rpT1], CuFltS0 - CuApUNAT
        ld8.nt1     rRnat = [rpT2], CuFltS1 - CuRsRNAT
        mov         bs1 = rT6
        ;;
          
        ldf.fill.nt1 fs0 = [rpT1], CuFltS2 - CuFltS0
        ldf.fill.nt1 fs1 = [rpT2], CuFltS3 - CuFltS1
        mov         bs2 = rT7
        ;;

        ldf.fill.nt1 fs2 = [rpT1], CuFltS4 - CuFltS2
        ldf.fill.nt1 fs3 = [rpT2], CuFltS5 - CuFltS3
        mov         bs3 = rT8
        ;;

        ldf.fill.nt1 fs4 = [rpT1], CuFltS6 - CuFltS4
        ldf.fill.nt1 fs5 = [rpT2], CuFltS7 - CuFltS5
        nop.i       0
        ;;

        ldf.fill.nt1 fs6 = [rpT1], CuFltS8 - CuFltS6
        ldf.fill.nt1 fs7 = [rpT2], CuFltS9 - CuFltS7
        nop.i       0
        ;;

        ldf.fill.nt1 fs8 = [rpT1], CuFltS10 - CuFltS8
        ldf.fill.nt1 fs9 = [rpT2], CuFltS11 - CuFltS9
        nop.i       0
        ;;

        ldf.fill.nt1 fs10 = [rpT1], CuFltS12 - CuFltS10
        ldf.fill.nt1 fs11 = [rpT2], CuFltS13 - CuFltS11
        nop.i       0
        ;;

        ldf.fill.nt1 fs12 = [rpT1], CuFltS14 - CuFltS12
        ldf.fill.nt1 fs13 = [rpT2], CuFltS15 - CuFltS13
        nop.i       0
        ;;

        ldf.fill.nt1 fs14 = [rpT1], CuFltS16 - CuFltS14
        ldf.fill.nt1 fs15 = [rpT2], CuFltS17 - CuFltS15
        nop.i       0
        ;;

        ldf.fill.nt1 fs16 = [rpT1], CuFltS18 - CuFltS16
        ldf.fill.nt1 fs17 = [rpT2], CuFltS19 - CuFltS17
        nop.i       0
        ;;

        ldf.fill.nt1 fs18 = [rpT1], CuA0 - CuFltS18
        ldf.fill.nt1 fs19 = [rpT2], CuA1 - CuFltS19
        nop.i       0
        ;;

        ld8.nt1     rpT5 = [rpT1], CuCbStk - CuA0       // load value of A0
        mov         ar.unat = rT0
        mov         bs4 = rT9
        ;;

        ld8.nt1     rT0 = [rpT1], CuTrFrame - CuCbStk   // load callback stack
        ld8.nt1     rpT6 = [rpT2], CuCbBStore - CuA1    // load value of A1
        add         rpT3 = ThCallbackStack, rpCurTh
        ;;

        ld8.nt1     rT1 = [rpT1], CuInStack - CuTrFrame // load trap frame addr
        ld8.nt1     rT2 = [rpT2], CuTrStIIP-CuCbBStore  // load callback bstore
        add         rpT4 = ThCallbackBStore, rpCurTh
        ;;
          
        ld8.nt1     rT5 = [rpT1]                // get previous initial stack
        ld8.nt1     rT4 = [rpT2], CuInBStore-CuTrStIIP  // load trap frame IIP
        add         rpT0 = ThTrapFrame, rpCurTh

        STPTR       (rpT5, a0)                  // store buffer address in A0
        st4         [rpT6] = a1                 // store buffer length in A1
        add         rpT6 = TrStIPSR, rT1

        STPTR       (rpT3, rT0)                 // restore callback stack addr
        STPTR       (rpT4, rT2)                 // restore callback bstore addr
        add         rpT3 =  ThInitialStack, rpCurTh
        ;;

        ld8.nt1     rT6 = [rpT2]                // get previous initial bstore
        ld8         rT0 = [rpT6], TrStIIP-TrStIPSR
        add         rpT5 = ThInitialBStore, rpCurTh
        ;;

        st8.nta     [rpT6] = rT4, TrStIPSR-TrStIIP  // restore trap IIP
        rsm         1 << PSR_I                  // disable interrupts
        or          rIPSR = rIPSR, rT0
        ;;

        st8         [rpT6] = rIPSR              // propagate ss, db, tb, lp bits
        movl        rpT4 = KiPcr+PcInitialStack
        ;;

        alloc       rT0 = ar.pfs, 0, 0, 0, 0
        mov         ar.rsc = r0                // put RSE in lazy mode
        mov         rT7 = rRnat
        ;;

        loadrs
        STPTR       (rpT0, rT1)                 // restore trap frame address
        nop.i       0
        ;;

        mov         ar.bspstore = rThCbBStore   // rThCbBStore
        st8.nta     [rpT4] = rT5, PcInitialBStore - PcInitialStack
        nop.i       0
        ;;

//
// restore initial stack and bstore.
//

        mov         ar.rnat = rT7
        STPTR       (rpT3, rT5)
        nop.i       0
        ;;

        mov         ar.rsc = RSC_KERNEL         // restore RSC
        st8.nta     [rpT4] = rT6
        add         sp = CuFrameLength, rThCbStack

        STPTR       (rpT5, rT6)
        ssm         1 << PSR_I                  // enable interrupts

        invala
        br.ret.sptk.clr brp

        LEAF_EXIT(NtCallbackReturn)



