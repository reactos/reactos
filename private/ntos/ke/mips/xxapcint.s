//      TITLE("Asynchronous Procedure Call (APC) Interrupt")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    xxapcint.s
//
// Abstract:
//
//    This module implements the code necessary to field and process the
//    Asynchronous Procedure Call (APC) interrupt.
//
// Author:
//
//    David N. Cutler (davec) 3-Apr-1990
//
// Environment:
//
//    Kernel mode only, IRQL APC_LEVEL.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Asynchronous Procedure Call Interrupt")
//++
//
// Routine Description:
//
//    This routine is entered as the result of a software interrupt generated
//    at APC_LEVEL. Its function is to allocate an exception frame and call
//    the kernel APC delivery routine to deliver kernel mode APCs and to check
//    if a user mode APC should be delivered. If a user mode APC should be
//    delivered, then the kernel APC delivery routine constructs a context
//    frame on the user stack and alters the exception and trap frames so that
//    control will be transfered to the user APC dispatcher on return from the
//    interrupt.
//
//    N.B. On entry to this routine all integer registers and the volatile
//       floating registers have been saved. The remainder of the machine
//       state is saved if and only if the previous mode was user mode.
//
// Arguments:
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiApcInterrupt, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate exception frame
        sw      ra,ExIntRa(sp)          // save return address

        PROLOGUE_END

//
// Determine the previous mode.
//

        lw      t0,TrPsr(s8)            // get saved processor status
        srl     t0,t0,PSR_KSU + 1       // isolate previous mode
        and     a0,t0,0x1               //
        beq     zero,a0,20f             // if eq, kernel mode

//
// The previous mode was user.
//
// Save the nonvolatile floating state so a context record can be
// properly constructed to deliver an APC to user mode if required.
// It is also necessary to save the volatile floating state for
// suspend/resume operations.
//

        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f31
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

//
// Clear APC interrupt.
//

20:     DISABLE_INTERRUPTS(t0)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t1,cause                // get exception cause register
        li      t2,~APC_INTERRUPT       // clear APC interrupt pending
        and     t1,t1,t2                //
        mtc0    t1,cause                //
        nop                             //
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t0)           // enable interrupts

//
// Attempt to deliver an APC.
//

        move    a1,sp                   // set address of exception frame
        move    a2,s8                   // set address of trap frame
        jal     KiDeliverApc            // call APC delivery routine

//
// Deallocate stack frame and return.
//

        lw      ra,ExIntRa(sp)          // restore return address
        addu    sp,sp,ExceptionFrameLength // deallocate exception frame
        j       ra                      // return

        .end    KiApcInterrupt
