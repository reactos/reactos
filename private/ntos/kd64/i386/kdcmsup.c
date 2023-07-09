/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdcmsup.c

Abstract:

    Com support.  Code to init a com port, store port state, map
    portable procedures to x86 procedures.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-90

Revision History:

    Shielin Tzong (shielint) 10-Apr-91
                Add packet control protocol.

--*/

#include "kdp.h"

LARGE_INTEGER
KdpQueryPerformanceCounter (
    IN PKTRAP_FRAME TrapFrame
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpQueryPerformanceCounter)
#endif

LARGE_INTEGER
KdpQueryPerformanceCounter (
    IN PKTRAP_FRAME TrapFrame
    )
/*++

    Routine Description:

        This function optionaly calls KeQueryPerformanceCounter for
        the debugger.  If the trap had interrupts off, then no call
        to KeQueryPerformanceCounter is possible and a NULL is returned.

    Return Value:

        returns KeQueryPerformanceCounter if possible.
        otherwise 0
--*/
{

    if (!(TrapFrame->EFlags & EFLAGS_INTERRUPT_MASK)) {
        LARGE_INTEGER LargeIntegerZero;

        LargeIntegerZero.QuadPart = 0;
        return LargeIntegerZero;
    } else {
        return KeQueryPerformanceCounter(0);
    }
}
