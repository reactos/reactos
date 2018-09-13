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
//    This module implements the PowerPC machine dependent code necessary to
//    startup a thread in kernel mode.
//
// Author:
//
//    Peter L. Johnston (plj@vnet.ibm.com) 20-Sep-1993
//    Based on code by David N. Cutler (davec) 28-Mar-1990
//
// Environment:
//
//    Kernel mode only, IRQL APC_LEVEL.
//
// Revision History:
//
//--

#include "ksppc.h"

        .extern ..KeBugCheck
        .extern ..KiExceptionExit
        .extern __imp_KeLowerIrql

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

                .text                           // resume .text section


        FN_TABLE(KiThreadDispatch,0,0)

        DUMMY_ENTRY(KiThreadDispatch)

        stwu    r.sp, -STACK_DELTA (r.sp)
        stw     r.0, TrGpr0 + TF_BASE (r.sp)
        mflr    r.0
        stw     r.0, TrLr + TF_BASE (r.sp)
        mflr    r.0
        stw     r.0, EfLr (r.sp)
        mfcr    r.0
        stw     r.0, EfCr (r.sp)

        stw     r.2, TrGpr2 + TF_BASE(r.sp)
        stw     r.3, TrGpr3 + TF_BASE(r.sp)
        stw     r.4, TrGpr4 + TF_BASE(r.sp)
        stw     r.5, TrGpr5 + TF_BASE(r.sp)
        stw     r.6, TrGpr6 + TF_BASE(r.sp)
        stw     r.7, TrGpr7 + TF_BASE(r.sp)
        stw     r.8, TrGpr8 + TF_BASE(r.sp)
        stw     r.9, TrGpr9 + TF_BASE(r.sp)
        stw     r.10, TrGpr10 + TF_BASE(r.sp)
        stw     r.11, TrGpr11 + TF_BASE(r.sp)
        stw     r.12, TrGpr12 + TF_BASE(r.sp)

        mfctr   r.6                            //   Fixed Point Exception
        mfxer   r.7                            //   registers

        stfd    f.0, TrFpr0 + TF_BASE(r.sp)    // save volatile FPRs
        stfd    f.1, TrFpr1 + TF_BASE(r.sp)
        stfd    f.2, TrFpr2 + TF_BASE(r.sp)
        stfd    f.3, TrFpr3 + TF_BASE(r.sp)
        stfd    f.4, TrFpr4 + TF_BASE(r.sp)
        stfd    f.5, TrFpr5 + TF_BASE(r.sp)
        stfd    f.6, TrFpr6 + TF_BASE(r.sp)
        stfd    f.7, TrFpr7 + TF_BASE(r.sp)
        stfd    f.8, TrFpr8 + TF_BASE(r.sp)
        stfd    f.9, TrFpr9 + TF_BASE(r.sp)
        stfd    f.10, TrFpr10 + TF_BASE(r.sp)
        stfd    f.11, TrFpr11 + TF_BASE(r.sp)
        stfd    f.12, TrFpr12 + TF_BASE(r.sp)
        stfd    f.13, TrFpr13 + TF_BASE(r.sp)
        mffs    f.0                            // get Floating Point Status
                                               //   and Control Register (FPSCR)

        stw     r.6, TrCtr + TF_BASE(r.sp)     //      Count,
        stw     r.7, TrXer + TF_BASE(r.sp)     //      Fixed Point Exception,
        stfd    f.0, TrFpscr + TF_BASE(r.sp)   //  and FPSCR registers.

        stw     r.13, ExGpr13 + EF_BASE(r.sp)  // save non-volatile GPRs
        stw     r.14, ExGpr14 + EF_BASE(r.sp)
        stw     r.15, ExGpr15 + EF_BASE(r.sp)
        stw     r.16, ExGpr16 + EF_BASE(r.sp)
        stw     r.17, ExGpr17 + EF_BASE(r.sp)
        stw     r.18, ExGpr18 + EF_BASE(r.sp)
        stw     r.19, ExGpr19 + EF_BASE(r.sp)
        stw     r.20, ExGpr20 + EF_BASE(r.sp)
        stw     r.21, ExGpr21 + EF_BASE(r.sp)
        stw     r.22, ExGpr22 + EF_BASE(r.sp)
        stw     r.23, ExGpr23 + EF_BASE(r.sp)
        stw     r.24, ExGpr24 + EF_BASE(r.sp)
        stw     r.25, ExGpr25 + EF_BASE(r.sp)
        stw     r.26, ExGpr26 + EF_BASE(r.sp)
        stw     r.27, ExGpr27 + EF_BASE(r.sp)
        stw     r.28, ExGpr28 + EF_BASE(r.sp)
        stw     r.29, ExGpr29 + EF_BASE(r.sp)
        stw     r.30, ExGpr30 + EF_BASE(r.sp)
        stw     r.31, ExGpr31 + EF_BASE(r.sp)

        stfd    f.14, ExFpr14 + EF_BASE(r.sp)  // save non-volatile FPRs
        stfd    f.15, ExFpr15 + EF_BASE(r.sp)
        stfd    f.16, ExFpr16 + EF_BASE(r.sp)
        stfd    f.17, ExFpr17 + EF_BASE(r.sp)
        stfd    f.18, ExFpr18 + EF_BASE(r.sp)
        stfd    f.19, ExFpr19 + EF_BASE(r.sp)
        stfd    f.20, ExFpr20 + EF_BASE(r.sp)
        stfd    f.21, ExFpr21 + EF_BASE(r.sp)
        stfd    f.22, ExFpr22 + EF_BASE(r.sp)
        stfd    f.23, ExFpr23 + EF_BASE(r.sp)
        stfd    f.24, ExFpr24 + EF_BASE(r.sp)
        stfd    f.25, ExFpr25 + EF_BASE(r.sp)
        stfd    f.26, ExFpr26 + EF_BASE(r.sp)
        stfd    f.27, ExFpr27 + EF_BASE(r.sp)
        stfd    f.28, ExFpr28 + EF_BASE(r.sp)
        stfd    f.29, ExFpr29 + EF_BASE(r.sp)
        stfd    f.30, ExFpr30 + EF_BASE(r.sp)
        stfd    f.31, ExFpr31 + EF_BASE(r.sp)

        PROLOGUE_END(KiThreadDispatch)

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
//    When this thread was created, a stack frame for this routine was
//    pushed onto the top of the thread's stack.  Then a stack frame
//    for SwapContext was pushed onto the stack and initialized such
//    that SwapContext will return to the first instruction of this routine
//    when this thread is first switched to.
//
// Arguments:
//
//    r.16  A boolean value that specifies whether a user mode thread
//          context was established when the thread was initialized.
//
//    r.17  Starting context parameter for the initial thread.
//
//    r.18  Starting address of the initial thread routine.
//
//    r.19  Starting address of the initial system routine.
//
//    r.20  Address of the (user mode) Trap Frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiThreadStartup)

        lwz     r.sp, 0(r.sp)                   // unlink SwapContext's frame

//
// Pickup arguments - as this routine wasn't actually called by anything
// we can use the non-volatile registers as we please.
//

        ori     r.31, r.toc, 0                  // save our TOC
        li      r.3, APC_LEVEL                  // lower IRQL to APC level
        lwz     r.4, [toc]__imp_KeLowerIrql(r.toc) // &&function descriptor
        lwz     r.4, 0(r.4)                     // &function descriptor
        lwz     r.5, 0(r.4)                     // &KeLowerIrql
        lwz     r.toc, 4(r.4)                   // HAL's TOC
        mtctr   r.5
        bctrl
        ori     r.toc, r.31, 0                  // restore our TOC

        mtctr   r.19                            // set address of system routine
        ori     r.3, r.18, 0                    // set address of thread routine
        ori     r.4, r.17, 0                    // set startup context parameter
        bctrl                                   // call system startup routine
        cmpwi   r.16, 0                         // check if user context
        beq     kts10                           // jif none

//
// Finish in common exception exit code which will restore the nonvolatile
// registers and exit to user mode.
//

        ori     r.4, r.20, 0                    // set trap frame address
        addi    r.3, r.20, TrapFrameLength      // deduce exception frame addr

        b       ..KiExceptionExit               // finish in exception exit code

//
// An attempt was made to enter user mode for a thread that has no user mode
// context. Generate a bug check.
//
kts10:
        li      r.3,NO_USER_MODE_CONTEXT        // set bug check code
        bl      ..KeBugCheck                    // call bug check routine

KiThreadDispatch.end:


