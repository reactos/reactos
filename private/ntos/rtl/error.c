/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    error.c

Abstract:

    This module contains a routine for converting NT status codes
    to DOS/OS|2 error codes.

Author:

    David Treadwell (davidtr)   04-Apr-1991

Revision History:

--*/

#include <ntrtlp.h>
#include "winerror.h"
#include "error.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlNtStatusToDosError)
#endif

//
// Ensure that the Registry ERROR_SUCCESS error code and the
// NO_ERROR error code remain equal and zero.
//

#if ERROR_SUCCESS != 0 || NO_ERROR != 0
#error Invalid value for ERROR_SUCCESS.
#endif

ULONG
RtlNtStatusToDosError (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine converts an NT status code to its DOS/OS|2 equivalent.
    Remembers the Status code value in the TEB.

Arguments:

    Status - Supplies the status value to convert.

Return Value:

    The matching DOS/OS|2 error code.

--*/

{
    PTEB Teb;

    Teb = NtCurrentTeb();
    if ( Teb ) {
    Teb->LastStatusValue = Status;
        }

    return RtlNtStatusToDosErrorNoTeb( Status );
}

ULONG
RtlNtStatusToDosErrorNoTeb (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine converts an NT status code to its DOS/OS 2 equivalent
    and returns the translated value.

Arguments:

    Status - Supplies the status value to convert.

Return Value:

    The matching DOS/OS 2 error code.

--*/

{

    ULONG Offset;
    ULONG Entry;
    ULONG Index;

    //
    // Convert any HRESULTs to their original form of a NTSTATUS or a
    // WIN32 error
    //


    if (Status & 0x20000000) {

        //
        // The customer bit is set so lets just pass the
        // error code on thru
        //

        return Status;

    }
    else if ((Status & 0xffff0000) == 0x80070000) {

        //
        // The status code  was a win32 error already.
        //

        return(Status & 0x0000ffff);
    }
    else if ((Status & 0xf0000000) == 0xd0000000) {

        //
        // The status code is a HRESULT from NTSTATUS
        //

        Status &= 0xcfffffff;
    }

    //
    // Scan the run length table and compute the entry in the translation
    // table that maps the specified status code to a DOS error code.
    //

    Entry = 0;
    Index = 0;
    do {
        if ((ULONG)Status >= RtlpRunTable[Entry + 1].BaseCode) {
            Index += (RtlpRunTable[Entry].RunLength * RtlpRunTable[Entry].CodeSize);

        } else {
            Offset = (ULONG)Status - RtlpRunTable[Entry].BaseCode;
            if (Offset >= RtlpRunTable[Entry].RunLength) {
                break;

            } else {
                Index += (Offset * (ULONG)RtlpRunTable[Entry].CodeSize);
                if (RtlpRunTable[Entry].CodeSize == 1) {
                    return (ULONG)RtlpStatusTable[Index];

                } else {
                    return (((ULONG)RtlpStatusTable[Index + 1] << 16) |
                                                (ULONG)RtlpStatusTable[Index]);
                }
            }
        }

        Entry += 1;
    } while (Entry < (sizeof(RtlpRunTable) / sizeof(RUN_ENTRY)));

    //
    // The translation to a DOS error code failed.
    //
    // The redirector maps unknown OS/2 error codes by ORing 0xC001 into
    // the high 16 bits.  Detect this and return the low 16 bits if true.
    //

    if (((ULONG)Status >> 16) == 0xC001) {
        return ((ULONG)Status & 0xFFFF);
    }

#ifndef NTOS_KERNEL_RUNTIME
    DbgPrint("RTL: RtlNtStatusToDosError(0x%lx): No Valid Win32 Error Mapping\n",Status);
    DbgPrint("RTL: Edit ntos\\rtl\\generr.c to correct the problem\n");
    DbgPrint("RTL: ERROR_MR_MID_NOT_FOUND is being returned\n");

#if DBG
    DbgBreakPoint();
#endif // DBG

#endif // NTOS_KERNEL_RUNTIME

    return ERROR_MR_MID_NOT_FOUND;
}
