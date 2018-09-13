/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    trace.c

Abstract:

    This module contains the code for debug tracing.

Author:

    Michael Tsang (MikeTs) 24-Sep-1998

Environment:

    Kernel mode

Revision History:


--*/

#include "pch.h"

#ifdef TRACING
//
// Constants
//
#define TF_CHECKING_TRACE       0x00000001

//
// Local Data
//
int IoepTraceLevel = 0;
int IoepIndentLevel = 0;
ULONG IoepTraceFlags = 0;

BOOLEAN
IsTraceOn(
    IN UCHAR   n,
    IN PSZ     ProcName
    )
/*++

Routine Description:
    This routine determines if the given procedure should be traced.

Arguments:
    n - trace level of the procedure
    ProcName - points to the procedure name string

Return Value:
    Success - returns TRUE
    Failure - returns FALSE

--*/
{
    BOOLEAN rc = FALSE;

    if (!(IoepTraceFlags & TF_CHECKING_TRACE))
    {
        IoepTraceFlags |= TF_CHECKING_TRACE;

        if (IoepTraceLevel >= n)
        {
            int i;

            KdPrint((MODNAME ": "));

            for (i = 0; i < IoepIndentLevel; ++i)
            {
                KdPrint(("| "));
            }

            KdPrint((ProcName));

            rc = TRUE;
        }

        IoepTraceFlags &= ~TF_CHECKING_TRACE;
    }

    return rc;
}       //IsTraceOn

#endif


