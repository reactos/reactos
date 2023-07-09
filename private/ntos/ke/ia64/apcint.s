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
//    William K. Cheung (wcheung) 1-Nov-1995
//
// Environment:
//
//    Kernel mode only, IRQL APC_LEVEL.
//
// Revision History:
//
//    08-Feb-1996    Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file "apcint.s"


//++
// Routine
//
//    VOID
//    KiApcInterrupt(PKINTERRUPT, PKTRAP_FRAME)
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
// Arguments:
//
//    a0 - Supplies a pointer to interrupt object.
//    
//    a1 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        .global     KiSaveExceptionFrame
        .type       KiSaveExceptionFrame, @function
        .global     KiDeliverApc
        .type       KiDeliverApc, @function


        NESTED_ENTRY(KiApcInterrupt)

        .regstk     2, 3, 3, 0
        .prologue   0xC, savedpfs

        alloc       savedpfs = ar.pfs, 2, 3, 3, 0
        mov         savedbrp = brp
        ;;

        .fframe     ExceptionFrameLength
        add         sp = -ExceptionFrameLength, sp
        nop.i       0
        ;;

        PROLOGUE_END

//
// Register definitions
//

        pKern       = pt0
        pUser       = pt1

        ARGPTR      (a1)

        cmp4.eq     pKern, pUser = a1, zero     // is this a kernel APC
        add         t0 = TrPreviousMode, a1
        ;;
(pUser) ld4         loc2 = [t0]                 // PreviousMode'size = 4-byte
(pKern) mov         loc2 = KernelMode
        ;;

//
// Save the nonvolatile machine state so a context record can be
// properly constructed to deliver an APC to user mode if required.
//

        cmp.eq      pUser = UserMode, loc2
        add         out0 = STACK_SCRATCH_AREA, sp
(pUser) br.call.sptk brp = KiSaveExceptionFrame
        ;;

        mov         out0 = loc2
        add         out1 = STACK_SCRATCH_AREA, sp   // -> exception frame
        mov         out2 = a1                   // -> trap frame
        br.call.sptk.many brp = KiDeliverApc

//
// deallocate exception frame and high floating pointer register save area
//

        .restore
        add         sp = ExceptionFrameLength, sp
        NESTED_RETURN

        NESTED_EXIT(KiApcInterrupt)
