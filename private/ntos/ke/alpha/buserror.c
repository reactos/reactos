/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    buserror.c

Abstract:

    This module implements the code necessary to process machine checks.

Author:

    Joe Notarangelo 11-Feb-1993

Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"



VOID
KiMachineCheck (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to process a machine check.  If the vendor
    has supplied a machine check handler with its HAL then the machine
    check handler is called.  If the routine returns TRUE indicating
    that the error has been handled then execution resumes, otherwise,
    a bugcheck is raised.

    If no machine check handler is registered or it does not indicate
    that the error has been handled, then this routine will attempt
    default handling.  Default handling consists of checking the
    machine check status in the exception record.  If the status indicates
    that the machine check is correctable or retryable then return and
    resume execution, otherwise a bugcheck is raised.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    if( ((ULONG_PTR)PCR->MachineCheckError != 0) &&
          (PCR->MachineCheckError)(ExceptionRecord,
                                   ExceptionFrame,
                                   TrapFrame) ) {

        //
        // The HAL has handled the error.
        //

        return;

    } else {

        //
        // Either there is no HAL handler, or it did not handle the
        // error.
        //

        if( ExceptionRecord->ExceptionInformation[0] != 0 ){

            //
            // The error is either correctable or retryable, resume
            // execution.
            //

#if DBG

            DbgPrint( "MCHK: resuming correctable or retryable error\n" );

#endif //DBG

            return;

        }
    }


    //
    // The error was not handled and is not correctable or retryable.
    //

    KeBugCheck(DATA_BUS_ERROR);
}

