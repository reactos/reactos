//       TITLE("Fast Event Pair Support")
//++
//
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//    evpair.s
//
// Abstract:
//
//    This module contains the implementation for the fast event pair
//    system services that are used for client/server synchronization.
//
// Author:
//
//    David N. Cutler (davec) 1-Feb-1992
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Set Low Wait High Thread")
//++
//
// NTSTATUS
// NtSetLowWaitHighThread (
//    )
//
// Routine Description:
//
//    This function uses the prereferenced client/server event pair pointer
//    and sets the low event of the event pair and waits on the high event
//    of the event pair object.
//
//    N.B. This service assumes that it has been called from user mode.
//
//    N.B. This routine is highly optimized since this is a very heavily
//         used service.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    STATUS_NO_EVENT_PAIR is returned if no event pair is associated with
//    the current thread. Otherwise, the status of the wait operation is
//    returned as the function value.
//
//
//--

        LEAF_ENTRY(NtSetLowWaitHighThread)

        .set    noreorder
        .set    noat
        lw      a0,KiPcr + PcCurrentThread(zero) // get current thread address
        lui     v0,STATUS_NO_EVENT_PAIR >> 16 // set high half of error status
        lw      a0,EtEventPair(a0)      // get address of event pair object
        ori     v0,STATUS_NO_EVENT_PAIR & 0xffff // complete error status value
        addu    a1,a0,EpEventHigh       // compute address of high event
        beq     zero,a0,10f             // if eq, no event pair associated
        addu    a0,a0,EpEventLow        // compute address of low event
        j       KiSetServerWaitClientEvent // finish in wait code
        li      a2,1                    // set user mode value
        .set    at
        .set    reorder

10:     j       ra                      // return

        .end    NtSetLowWaitHighThread

        SBTTL("Set High Wait Low Thread")
//++
//
// NTSTATUS
// NtSetHighWaitLowThread (
//    )
//
// Routine Description:
//
//    This function uses the prereferenced client/server event pair pointer
//    and sets the High event of the event pair and waits on the low event
//    of the event pair object.
//
//    N.B. This service assumes that it has been called from user mode.
//
//    N.B. This routine is highly optimized since this is a very heavily
//         used service.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    STATUS_NO_EVENT_PAIR is returned if no event pair is associated with
//    the current thread. Otherwise, the status of the wait operation is
//    returned as the function value.
//
//
//--

        LEAF_ENTRY(NtSetHighWaitLowThread)

        .set    noreorder
        .set    noat
        lw      a0,KiPcr + PcCurrentThread(zero) // get current thread address
        lui     v0,STATUS_NO_EVENT_PAIR >> 16 // set high half of error status
        lw      a0,EtEventPair(a0)      // get address of event pair object
        ori     v0,STATUS_NO_EVENT_PAIR & 0xffff // complete error status value
        addu    a1,a0,EpEventLow        // compute address of low event
        beq     zero,a0,10f             // if eq, no event pair associated
        addu    a0,a0,EpEventHigh       // compute address of high event
        j       KiSetServerWaitClientEvent // finish in wait code
        li      a2,1                    // set user mode value
        .set    at
        .set    reorder

10:     j       ra                      // return

        .end    NtSetHighWaitLowThread
