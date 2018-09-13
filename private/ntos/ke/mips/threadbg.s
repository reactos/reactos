//      TITLE("Thread Startup")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    threadbg.s
//
// Abstract:
//
//    This module implements the MIPS machine dependent code necessary to
//    startup a thread in kernel mode.
//
// Author:
//
//    David N. Cutler (davec) 28-Mar-1990
//
// Environment:
//
//    Kernel mode only, IRQL APC_LEVEL.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Thread Startup")
//++
//
// RoutineDescription:
//
//    The following code is never executed. It's purpose is to allow the
//    kernel debugger to walk call frames backwards through thread startup
//    and to support get/set user context.
//
//--

        NESTED_ENTRY(KiThreadDispatch, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate exception frame
        sw      ra,ExIntRa(sp)          // save return address
        sw      s0,ExIntS0(sp)          // save integer registers s0 - s7
        sw      s1,ExIntS1(sp)          //
        sw      s2,ExIntS2(sp)          //
        sw      s3,ExIntS3(sp)          //
        sw      s4,ExIntS4(sp)          //
        sw      s5,ExIntS5(sp)          //
        sw      s6,ExIntS6(sp)          //
        sw      s7,ExIntS7(sp)          //
        sw      s8,ExIntS8(sp)          //
        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f30
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

        PROLOGUE_END

//++
//
// Routine Description:
//
//    This routine is called at thread startup. Its function is to call the
//    initial thread procedure. If control returns from the initial thread
//    procedure and a user mode context was established when the thread
//    was initialized, then the user mode context is restored and control
//    is transfered to user mode. Otherwise a bug check will occur.
//
//
// Arguments:
//
//    s0 (saved) - Supplies a boolean value that specified whether a user
//       mode thread context was established when the thread was initialized.
//
//    s1 (saved) - Supplies the starting context parameter for the initial
//       thread procedure.
//
//    s2 (saved) - Supplies the starting address of the initial thread routine.
//
//    s3 - Supplies the starting address of the initial system routine.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiThreadStartup)

        lw      s0,ExIntS0(sp)          // get user context flag
        lw      s1,ExIntS1(sp)          // get context parameter value
        lw      s2,ExIntS2(sp)          // get initial routine address
        addu    sp,sp,ExceptionFrameLength // deallocate context frame
        bne     zero,s0,10f             // if ne, user context specified
        subu    sp,sp,TrapFrameArguments // allocate argument space

        .set    noreorder
        .set    noat
10:     ctc1    zero,fsr                // clear floating status
        .set    at
        .set    reorder

        li      a0,APC_LEVEL            // lower IRQL to APC level
        jal     KeLowerIrql             //
        move    a0,s2                   // set address of thread routine
        move    a1,s1                   // set startup context parameter
        jal     s3                      // call system startup routine
        beq     zero,s0,20f             // if eq, no user context

//
// Finish in common exception exit code which will restore the nonvolatile
// registers and exit to user mode.
//

        j       KiExceptionExit         // finish in exception exit code

//
// An attempt was made to enter user mode for a thread that has no user mode
// context. Generate a bug check.
//

20:     li      a0,NO_USER_MODE_CONTEXT // set bug check code
        jal     KeBugCheck              // call bug check routine

        .end    KiThreadDispatch
