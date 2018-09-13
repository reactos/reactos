/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    kdlock.c

Abstract:

    This module contains code to synchronize the usage of the port
    used by the kernel debugger.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-90

Revision History:

--*/

#include "kdp.h"


VOID
KdpPortLock(
    VOID
    )

/*++

Routine Description:

    Acquire the spinlock for the debug port.

    Note that user must call this explicitly, the get/put routines
    do NOT make any use of the lock.

    CALLER MUST HAVE SET PROPER IRQL BEFORE CALLING US.

    We use KiAcquireSpinLock and NOT Ke... because our IRQL may
    be above DISPATCH_LEVEL.

Arguments:

    None.

Return value:

    None.

--*/

{
    KiAcquireSpinLock(&KdpDebuggerLock);
}


VOID
KdpPortUnlock(
    VOID
    )

/*++

Routine Description:

    Release the spinlock for the debug port.

    Note that user must call this explicitly, the get/put routines
    do NOT make any use of the lock.

    CALLER MUST HAVE SET PROPER IRQL BEFORE CALLING US.

    We use KiReleaseSpinLock and NOT Ke... because our IRQL may
    be above DISPATCH_LEVEL.

Arguments:

    None.

Return value:

    None.

--*/

{
    KiReleaseSpinLock(&KdpDebuggerLock);
}

BOOLEAN
KdPollBreakIn(
    VOID
    )

/*++

Routine Description:

    This procedure raises IRQL to high_level, seizes the Debug port
    spinlock, and checks to see if a breakin packet is pending.
    If a packet is present, return TRUE, else FALSE.

    A packet is present if:

    There is a valid character which matches BREAK_CHAR.

    N.B.    Interrupts must be OFF around this call

Return Value:

    TRUE if breakin sequence present, caller should execute int-3.
    FALSE if no breakin seen.

--*/

{
    BOOLEAN BreakIn;
    BOOLEAN Enable;
    UCHAR   Input;
    KIRQL   OldIrql;
    ULONG   Status;

    //
    // If the debugger is enabled, see if a breakin by the kernel
    // debugger is pending.
    //

    BreakIn = FALSE;
    if (KdDebuggerEnabled != FALSE) {
        Enable = KiDisableInterrupts();
#ifndef _X86_
        KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#endif
        if (KdpControlCPending != FALSE) {
            KdpControlCPressed = TRUE;
            BreakIn = TRUE;
            KdpControlCPending = FALSE;

        } else {
            if (KiTryToAcquireSpinLock(&KdpDebuggerLock) != FALSE) {
                Status = KdPortPollByte(&Input);
                if ((Status == CP_GET_SUCCESS) &&
                    (Input == BREAKIN_PACKET_BYTE)) {
                    BreakIn = TRUE;
                    KdpControlCPressed = TRUE;
                }

                KdpPortUnlock();
            }
        }

#ifndef _X86_
        KeLowerIrql(OldIrql);
#endif
        KiRestoreInterrupts(Enable);
    }

    return BreakIn;
}


BOOLEAN
KdpPollBreakInWithPortLock(
    VOID
    )

/*++

Routine Description:

    This procedure same as KdPollBreakIn, but assumes the caller
    already holds the port lock.  Returns TRUE if a breakin packet
    is pending.

    A packet is present if:

    There is a valid character which matches BREAK_CHAR.

    N.B.    Interrupts must be OFF around this call

Return Value:

    TRUE if breakin sequence present, caller should execute int-3.
    FALSE if no breakin seen.

--*/

{

    BOOLEAN BreakIn;
    BOOLEAN Enable;
    UCHAR Input;
    ULONG Status;

    //
    // If the debugger is enabled, see if a breakin by the kernel
    // debugger is pending.
    //

    BreakIn = FALSE;
    if (KdDebuggerEnabled != FALSE) {
        if (KdpControlCPending != FALSE) {
            BreakIn = TRUE;
            KdpControlCPending = FALSE;

        } else {
            Status = KdPortPollByte(&Input);
            if ((Status == CP_GET_SUCCESS) &&
                (Input == BREAKIN_PACKET_BYTE)) {
                BreakIn = TRUE;
            }
        }
    }

    return BreakIn;
}
