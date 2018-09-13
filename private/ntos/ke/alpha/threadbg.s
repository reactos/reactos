//      TITLE("Thread Startup")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992, 1993  Digital Equipment Corporation
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
//    Joe Notarangelo  21-Apr-1992
//
// Environment:
//
//    Kernel mode only, IRQL APC_LEVEL.
//
// Revision History:
//
//--

#include "ksalpha.h"

//++
//
// RoutineDescription:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through thread startup
//    and to support get/set user context.
//
//--

        NESTED_ENTRY(KiThreadDispatch, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate exception frame
        stq     ra, ExIntRa(sp)         // save return address
        stq     s0, ExIntS0(sp)         // save integer regs s0-s5
        stq     s1, ExIntS1(sp)         //
        stq     s2, ExIntS2(sp)         //
        stq     s3, ExIntS3(sp)         //
        stq     s4, ExIntS4(sp)         //
        stq     s5, ExIntS5(sp)         //

        stt     f2, ExFltF2(sp)         // save floating regs f2 - f9
        stt     f3, ExFltF3(sp)         //
        stt     f4, ExFltF4(sp)         //
        stt     f5, ExFltF5(sp)         //
        stt     f6, ExFltF6(sp)         //
        stt     f7, ExFltF7(sp)         //
        stt     f8, ExFltF8(sp)         //
        stt     f9, ExFltF9(sp)         //

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
//    sp - Supplies a pointer to the exception frame which contains the
//         startup parameters.
//
//    Within Exception frame:
//
//    s0 - Supplies a boolean value that specified whether a user mode
//       thread context was established when the thread was initialized.
//
//    s1 - Supplies the starting context parameter for the initial thread
//       procedure.
//
//    s2 - Supplies the starting address of the initial thread routine.
//
//    s3 - Supplies the starting address of the initial system routine.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiThreadStartup)

//
// Capture the arguments for startup from the exception frame.
// After the arguments are captured, deallocate the exception frame.
//

        ldq     s0, ExIntS0(sp)         // capture user context boolean
        ldq     s1, ExIntS1(sp)         // set startup context parameter
        ldq     s2, ExIntS2(sp)         // set address of thread routine
        ldq     s3, ExIntS3(sp)         // capture startup routine address
        ldq     s4, ExIntS4(sp)         // restore s4
        ldq     s5, ExIntS5(sp)         // restore s5
        ldq     fp, ExIntFp(sp)         // restore trap frame pointer
        lda     sp, ExceptionFrameLength(sp) // deallocate exception frame

//
// Lower Irql to APC level.
//

        ldil    a0, APC_LEVEL           // set IRQL to APC level

        SWAP_IRQL                       // lower IRQL

//
// Jump to the startup routine with the address of the thread routine and
// the startup context parameter.
//

        bis     s2, zero, a0            // set address of thread routine
        bis     s1, zero, a1            // set startup context parameter
        jsr     ra, (s3)                // call system startup routine

//
// If we return and no user context was supplied then we have trouble.
//

        beq     s0, 20f                 // if eq, no user context

//
// Finish in common exception exit code which will restore the nonvolatile
// registers and exit to user mode.
//

        br      zero, KiExceptionExit   // finish in exception exit code

//
// An attempt was made to enter user mode for a thread that has no user mode
// context. Generate a bug check.
//

20:     ldil    a0, NO_USER_MODE_CONTEXT // set bug check code
        bsr     ra, KeBugCheck          // call bug check routine

        .end    KiThreadDispatch
