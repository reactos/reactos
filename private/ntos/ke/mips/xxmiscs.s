//      TITLE("Miscellaneous Kernel Functions")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    misc.s
//
// Abstract:
//
//    This module implements machine dependent miscellaneous kernel functions.
//    Functions are provided to request a software interrupt, continue thread
//    execution, flush the write buffer, and perform last chance exception
//    processing.
//
// Author:
//
//    David N. Cutler (davec) 31-Mar-1990
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Request Software Interrupt")
//++
//
// VOID
// KiRequestSoftwareInterrupt (
//    ULONG RequestIrql
//    )
//
// Routine Description:
//
//    This function requests a software interrupt at the specified IRQL
//    level.
//
// Arguments:
//
//    RequestIrql (a0) - Supplies the request IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRequestSoftwareInterrupt)

        li      t0,1 << (CAUSE_INTPEND - 1) // get partial request mask value

        DISABLE_INTERRUPTS(t1)          // disable interrupts

        .set    noreorder
        .set    noat
        mfc0    t2,cause                // get exception cause register
        sll     t0,t0,a0                // shift request mask into position
        or      t2,t2,t0                // merge interrupt request mask
        mtc0    t2,cause                // set exception cause register
        .set    at
        .set    reorder

        ENABLE_INTERRUPTS(t1)           // enable interrupts

        j       ra                      // return

        .end    KiRequestSoftwareInterrupt

        SBTTL("Continue Execution System Service")
//++
//
// NTSTATUS
// NtContinue (
//    IN PCONTEXT ContextRecord,
//    IN BOOLEAN TestAlert
//    )
//
// Routine Description:
//
//    This routine is called as a system service to continue execution after
//    an exception has occurred. Its functions is to transfer information from
//    the specified context record into the trap frame that was built when the
//    system service was executed, and then exit the system as if an exception
//    had occurred.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies a pointer to a context record.
//
//    TestAlert (a1) - Supplies a boolean value that specifies whether alert
//       should be tested for the previous processor mode.
//
//    N.B. Register s8 is assumed to contain the address of a trap frame.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record is misaligned or is not accessible, then the appropriate
//    status code is returned.
//
//--

        NESTED_ENTRY(NtContinue, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate exception frame
        sw      ra,ExIntRa(sp)          // save return address

        PROLOGUE_END

//
// Save the nonvolatile machine state so that it can be restored by exception
// exit if it is not overwritten by the specified context record.
//

        sd      s0,TrXIntS0(s8)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(s8)         //
        sd      s2,TrXIntS2(s8)         //
        sd      s3,TrXIntS3(s8)         //
        sd      s4,TrXIntS4(s8)         //
        sd      s5,TrXIntS5(s8)         //
        sd      s6,TrXIntS6(s8)         //
        sd      s7,TrXIntS7(s8)         //
        li      t0,TRUE                 // set saved s-registers flag
        sb      t0,TrSavedFlag(s8)      //

        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f31
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

//
// Transfer information from the context frame to the exception and trap
// frames.
//

        sb      a1,ExceptionFrameLength + 4(sp) // save test alert argument
        move    a1,sp                   // set address of exception frame
        move    a2,s8                   // set address of trap frame
        jal     KiContinue              // transfer context to kernel frames

//
// If the kernel continuation routine returns success, then exit via the
// exception exit code. Otherwise return to the system service dispatcher.
//

        bne     zero,v0,20f             // if ne, transfer failed

//
// Check to determine if alert should be tested for the previous processor
// mode and restore the previous mode in the thread object.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lbu     t1,ExceptionFrameLength + 4(sp) // get test alert argument
        lw      t2,TrTrapFrame(s8)      // get old trap frame address
        lbu     t3,TrPreviousMode(s8)   // get old previous mode
        lbu     a0,ThPreviousMode(t0)   // get current previous mode
        sw      t2,ThTrapFrame(t0)      // restore old trap frame address
        sb      t3,ThPreviousMode(t0)   // restore old previous mode
        beq     zero,t1,10f             // if eq, don't test for alert
        jal     KeTestAlertThread       // test alert for current thread

//
// Exit the system via exception exit which will restore the nonvolatile
// machine state.
//

10:     j       KiExceptionExit         // finish in exception exit

//
// Context record is misaligned or not accessible.
//

20:     lw      ra,ExIntRa(sp)          // restore return address
        addu    sp,sp,ExceptionFrameLength // deallocate stack frame
        j       ra                      // return

        .end    NtContinue

        SBTTL("Raise Exception System Service")
//++
//
// NTSTATUS
// NtRaiseException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PCONTEXT ContextRecord,
//    IN BOOLEAN FirstChance
//    )
//
// Routine Description:
//
//    This routine is called as a system service to raise an exception.
//    The exception can be raised as a first or second chance exception.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    ContextRecord (a1) - Supplies a pointer to a context record.
//
//    FirstChance (a2) - Supplies a boolean value that determines whether
//       this is the first (TRUE) or second (FALSE) chance for dispatching
//       the exception.
//
//    N.B. Register s8 is assumed to contain the address of a trap frame.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record or exception record is misaligned or is not accessible,
//    then the appropriate status code is returned.
//
//--

        NESTED_ENTRY(NtRaiseException, ExceptionFrameLength, zero)

        subu    sp,sp,ExceptionFrameLength // allocate exception frame
        sw      ra,ExIntRa(sp)          // save return address

        PROLOGUE_END

//
// Save the nonvolatile machine state so that it can be restored by exception
// exit if it is not overwritten by the specified context record.
//

        sd      s0,TrXIntS0(s8)         // save integer registers s0 - s7
        sd      s1,TrXIntS1(s8)         //
        sd      s2,TrXIntS2(s8)         //
        sd      s3,TrXIntS3(s8)         //
        sd      s4,TrXIntS4(s8)         //
        sd      s5,TrXIntS5(s8)         //
        sd      s6,TrXIntS6(s8)         //
        sd      s7,TrXIntS7(s8)         //
        li      t0,TRUE                 // set saved s-registers flag
        sb      t0,TrSavedFlag(s8)      //

        sdc1    f20,ExFltF20(sp)        // save floating registers f20 - f31
        sdc1    f22,ExFltF22(sp)        //
        sdc1    f24,ExFltF24(sp)        //
        sdc1    f26,ExFltF26(sp)        //
        sdc1    f28,ExFltF28(sp)        //
        sdc1    f30,ExFltF30(sp)        //

//
// Call the raise exception kernel routine which will marshall the arguments
// and then call the exception dispatcher.
//

        sw     a2,ExArgs + 16(sp)       // set first chance argument
        move   a2,sp                    // set address of exception frame
        move   a3,s8                    // set address of trap frame
        jal    KiRaiseException         // call raise exception routine

//
// If the raise exception routine returns success, then exit via the exception
// exit code. Otherwise return to the system service dispatcher.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      t1,TrTrapFrame(s8)      // get old trap frame address
        bne     zero,v0,10f             // if ne, dispatch not successful
        sw      t1,ThTrapFrame(t0)      // restore old trap frame address

//
// Exit the system via exception exit which will restore the nonvolatile
// machine state.
//

        j       KiExceptionExit         // finish in exception exit

//
// The context or exception record is misaligned or not accessible, or the
// exception was not handled.
//

10:     lw      ra,ExIntRa(sp)          // restore return address
        addu    sp,sp,ExceptionFrameLength // deallocate stack frame
        j       ra                      // return

        .end    NtRaiseException
