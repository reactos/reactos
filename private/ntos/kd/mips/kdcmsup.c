/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    kdcmsup.c

Abstract:

    The module implements code to poll for a kernel debugger breakin attempt.

Author:

    Bryan M. Willman (bryanwi) 19-Jan-92

Revision History:

--*/

#include "kdp.h"

LARGE_INTEGER
KdpQueryPerformanceCounter (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function returns the current value of the system performance
    counter.

Arguments:

    None.

Return Value:

    The value returned by KeQueryPerformanceCounter is returned as the
    function value.

--*/

{

    return KeQueryPerformanceCounter(0);
}
