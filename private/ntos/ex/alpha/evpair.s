//       TITLE("Fast Event Pair Support")
//++
//
// Copyright (c) 1992  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
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
//    Thomas Van Baak (tvb) 20-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

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
//    N.B. This is a very heavily used service.
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

        GET_CURRENT_THREAD              // v0 = current thread

        ldl     a0, EtEventPair(v0)     // get address of event pair object
        beq     a0, 10f                 // if eq, no event pair associated
        addl    a0, EpEventHigh, a1     // compute client event address
        addl    a0, EpEventLow, a0      // comput server event address
        ldil    a2, 1                   // set user mode value
        br      zero, KiSetServerWaitClientEvent // finish in wait code

10:     ldil    v0, STATUS_NO_EVENT_PAIR        // set error status return value
        ret     zero, (ra)              // return

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
//    N.B. This is a very heavily used service.
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

        GET_CURRENT_THREAD              // v0 = current thread

        ldl     a0, EtEventPair(v0)     // get address of event pair object
        beq     a0, 10f                 // if eq, no event pair associated
        addl    a0, EpEventLow, a1      // compute client event address
        addl    a0, EpEventHigh, a0     // compute server event address
        ldil    a2, 1                   // set user mode value
        br      zero, KiSetServerWaitClientEvent // finish in wait code

10:     ldil    v0, STATUS_NO_EVENT_PAIR        // set error status return value
        ret     zero, (ra)              // return

        .end    NtSetHighWaitLowThread
