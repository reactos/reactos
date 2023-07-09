//      TITLE("Miscellaneous Kernel Functions")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    miscs.s
//
// Abstract:
//
//    This module implements machine dependent miscellaneous kernel functions.
//    Functions are provided to request a software interrupt, continue thread
//    execution, and perform last chance exception processing.
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
//    Thomas Van Baak (tvb) 29-Jul-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Request Software Interrupt")
//++
//
// VOID
// KiRequestSoftwareInterrupt (
//    KIRQL RequestIrql
//    )
//
// Routine Description:
//
//    This function requests a software interrupt at the specified IRQL
//    level.
//
// Arguments:
//
//    RequestIrql (a0) - Supplies the requested IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRequestSoftwareInterrupt)

//
// If an interrupt routine is active, do not request an interrupt from the
// PAL. Indicate the interrupt has been requested in the PRCB. The interrupt
// exit code will dispatch the software interrupt directly.
//

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        LDP     t0, PbInterruptTrapFrame(v0) // get interrupt trap frame
        beq     t0, 10f                 // if eq, no interrupt active
        blbs    a0, 10f                 // if lbs, APC interrupt requested
        stl     a0, PbSoftwareInterrupts(v0) // set interrupt request bit in PRCB
        ret     zero, (ra)              //

//
// Request software interrupt.
//

10:     REQUEST_SOFTWARE_INTERRUPT      // request software interrupt

        ret     zero, (ra)              // return

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
//    an exception has occurred. Its function is to transfer information from
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
//    N.B. Register fp is assumed to contain the address of a trap frame.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record is misaligned or is not accessible, then the appropriate
//    status code is returned.
//
//--

        NESTED_ENTRY(NtContinue, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate exception frame
        stq     ra, ExIntRa(sp)         // save return address

        PROLOGUE_END

//
// Save the nonvolatile machine state so that it can be restored by exception
// exit if it is not overwritten by the specified context record.
//

        stq     s0, ExIntS0(sp)         // save nonvolatile integer state
        stq     s1, ExIntS1(sp)         //
        stq     s2, ExIntS2(sp)         //
        stq     s3, ExIntS3(sp)         //
        stq     s4, ExIntS4(sp)         //
        stq     s5, ExIntS5(sp)         //

        stt     f2, ExFltF2(sp)         // save nonvolatile floating state
        stt     f3, ExFltF3(sp)         //
        stt     f4, ExFltF4(sp)         //
        stt     f5, ExFltF5(sp)         //
        stt     f6, ExFltF6(sp)         //
        stt     f7, ExFltF7(sp)         //
        stt     f8, ExFltF8(sp)         //
        stt     f9, ExFltF9(sp)         //

//
// Transfer information from the context frame to the exception and trap
// frames.
//

        mov     a1, s0                  // preserve test alert argument in s0
        mov     sp, a1                  // set address of exception frame
        mov     fp, a2                  // set address of trap frame
        bsr     ra, KiContinue          // transfer context to kernel frames

//
// If the kernel continuation routine returns success, then exit via the
// exception exit code. Otherwise return to the system service dispatcher.
//

        bne     v0, 20f                 // if ne, transfer failed

//
// Check to determine if alert should be tested for the previous processor
// mode and restore the previous mode in the thread object.
//

        GET_CURRENT_THREAD              // get current thread address

        LDP     t3, TrTrapFrame(fp)     // get old trap frame address
        ldl     t2, TrPreviousMode(fp)  // get old previous mode
        LoadByte(a0, ThPreviousMode(v0)) // get current previous mode
        STP     t3, ThTrapFrame(v0)     // restore old trap frame address
        StoreByte(t2, ThPreviousMode(v0)) // restore old previous mode
        beq     s0, 10f                 // if eq, don't test for alert
        bsr     ra, KeTestAlertThread   // test alert for current thread

//
// Exit the system via exception exit which will restore the nonvolatile
// machine state.
//

10:     br      zero, KiExceptionExit   // finish in exception exit

//
// Context record is misaligned or not accessible.
//

20:     ldq     ra, ExIntRa(sp)         // restore return address
        lda     sp, ExceptionFrameLength(sp) // deallocate stack frame
        ret     zero, (ra)              // return

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
//    N.B. Register fp is assumed to contain the address of a trap frame.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record or exception record is misaligned or is not accessible,
//    then the appropriate status code is returned.
//
//--

        NESTED_ENTRY(NtRaiseException, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate exception frame
        stq     ra, ExIntRa(sp)         // save return address
        stq     s0, ExIntS0(sp)         // save S0 and S1 in the prologue
        stq     s1, ExIntS1(sp)         // so that Get/SetContext can find
                                        // the right ones.
        PROLOGUE_END

//
// Save the nonvolatile machine state so that it can be restored by exception
// exit if it is not overwritten by the specified context record.
//

        stq     s2, ExIntS2(sp)         //
        stq     s3, ExIntS3(sp)         //
        stq     s4, ExIntS4(sp)         //
        stq     s5, ExIntS5(sp)         //

        stt     f2, ExFltF2(sp)         // save nonvolatile floating state
        stt     f3, ExFltF3(sp)         //
        stt     f4, ExFltF4(sp)         //
        stt     f5, ExFltF5(sp)         //
        stt     f6, ExFltF6(sp)         //
        stt     f7, ExFltF7(sp)         //
        stt     f8, ExFltF8(sp)         //
        stt     f9, ExFltF9(sp)         //

//
// Call the raise exception kernel routine which will marshall the arguments
// and then call the exception dispatcher.
//
// KiRaiseException requires five arguments: the first two are the same as
// the first two of this function and the other three are set here.
//

        mov     a2, a4                  // set first chance argument
        mov     sp, a2                  // set address of exception frame
        mov     fp, a3                  // set address of trap frame
        bsr     ra, KiRaiseException    // call raise exception routine

//
// If the raise exception routine returns success, then exit via the exception
// exit code. Otherwise return to the system service dispatcher.
//

        bis     v0, zero, t0            // save return status
        LDP     t1, TrTrapFrame(fp)     // get old trap frame address

        GET_CURRENT_THREAD              // get current thread address

        bne     t0, 10f                 // if ne, dispatch not successful
        STP     t1, ThTrapFrame(v0)     // restore old trap frame address

//
// Exit the system via exception exit which will restore the nonvolatile
// machine state.
//

        br      zero, KiExceptionExit   // finish in exception exit

//
// The context or exception record is misaligned or not accessible, or the
// exception was not handled.
//

10:     ldq     ra, ExIntRa(sp)         // restore return address
        lda     sp, ExceptionFrameLength(sp) // deallocate stack frame
        ret     zero, (ra)              // return

        .end    NtRaiseException

        SBTTL("Instruction Memory Barrier")
//++
//
// VOID
// KiImb (
//     VOID
//     )
//
// Routine Description:
//
//    This routine is called to flush the instruction cache on the
//    current processor.
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

        LEAF_ENTRY(KiImb)

        IMB                     // flush the icache via PALcode

        ret     zero, (ra)      // return

        .end    KiImb

        SBTTL("Memory Barrier")
//++
//
// VOID
// KiImb (
//     VOID
//     )
//
// Routine Description:
//
//    This routine is called to issue a memory barrier on the current
//    processor.
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

        LEAF_ENTRY(KiMb)

        mb                      // memory barrier

        ret     zero, (ra)      // return

        .end    KiMb

//++
//
// VOID
// KiEnablePALAlignmentFixups(
//     VOID	
//     )
//
// Routine Description:
//
//     Enable PAL fixups and PAL counting of alignment faults.
//
// Arguments:
//
//     None
//
// Return Value:
//
//     None.
//
//--

	LEAF_ENTRY(KiEnablePALAlignmentFixups)


	ENABLE_ALIGNMENT_FIXUPS         // enable alignment fixups

	ret	zero, (ra)		// return

	.end	KiEnablePALAlignmentFixups

//++
//
// VOID
// KiDisablePALAlignmentFixups(
//     VOID	
//     )
//
// Routine Description:
//
//     Disable PAL fixups and force all alignment faults to
//     the kernel.
//
// Arguments:
//
//     None
//
// Return Value:
//
//     None.
//
//--

	LEAF_ENTRY(KiDisablePALAlignmentFixups)

	DISABLE_ALIGNMENT_FIXUPS        // disable alignment fixups

	ret	zero, (ra)		// return

	.end	KiDisablePALAlignmentFixups
