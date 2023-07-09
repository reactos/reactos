//      TITLE("Asynchronous Procedure Call (APC) Interrupt")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    apcint.s
//
// Abstract:
//
//    This module implements the code necessary to field and process the
//    Asynchronous Procedure Call (APC) interrupt.
//
// Author:
//
//    David N. Cutler (davec) 3-Apr-1990
//    Joe Notarangelo 15-Jul-1992  alpha version
//
// Environment:
//
//    Kernel mode only, IRQL APC_LEVEL.
//
// Revision History:
//
//--

#include "ksalpha.h"

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
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved. The remainder of the machine state is saved if and only
//       if the previous mode was user mode. It is assumed that none of the
//       APC delivery code, nor any of the kernel mode APC routines themselves
//       use any floating point instructions.
//
// Arguments:
//
//    s6/fp - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiApcInterrupt, ExceptionFrameLength, zero)

	lda	sp, -ExceptionFrameLength(sp) // allocate exception frame
	stq	ra, ExIntRa(sp)		// save return address

        PROLOGUE_END

//
// Save the volatile floating state and determine the previous mode.
//

	bsr	ra, KiSaveVolatileFloatState // save volatile floats
	ldl	t0, TrPsr(fp)		// get saved processor status
	and	t0, PSR_MODE_MASK, a0	// isolate previous mode
        beq     a0, 10f			// if eq, kernel mode

//
// The previous mode was user.
//
// Save the nonvolatile machine state so a context record can be
// properly constructed to deliver an APC to user mode if required.
// It is also necessary to save the volatile floating state for
// suspend/resume operations.
//

	stq	s0, ExIntS0(sp)		// save nonvolatile integer state
	stq	s1, ExIntS1(sp)		//
	stq	s2, ExIntS2(sp)		//
	stq	s3, ExIntS3(sp)		//
	stq	s4, ExIntS4(sp)		//
	stq	s5, ExIntS5(sp)		//
	bsr	ra, KiSaveNonVolatileFloatState // save floating state

//
// Attempt to deliver an APC.
//

10:     bis	sp, zero, a1		// set address of exception frame
	bis	fp, zero, a2		// set address of trap frame
	bsr	ra, KiDeliverApc	// call APC delivery routine

//
// Restore the volatile floating state and return from the interrupt.
//

	bsr	ra, KiRestoreVolatileFloatState // restore floating state
	ldq	ra, ExIntRa(sp)		// restore return address
	lda	sp, ExceptionFrameLength(sp) // deallocate exception frame
	ret	zero, (ra)		// return

        .end    KiApcInterrupt
