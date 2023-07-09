//       TITLE("Fast Event Pair Support")
//++
//
// Copyright (c) 1993  IBM Corporation
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
//    Chuck Bauman 3-Sep-1993
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksppc.h"

//      SBTTL("Set Low Wait High Thread")
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
.extern         ..KiSetServerWaitClientEvent

        LEAF_ENTRY(NtSetLowWaitHighThread)

        lwz     r.3,KiPcr+PcCurrentThread(r.0)
        lwz     r.3,EtEventPair(r.3)            // get address of event pair object
        cmpwi   r.3,0
        addi    r.4,r.3,EpEventHigh             // compute address of high event
        beq     noevent_1                       // if eq, no event pair associated
        addi    r.3,r.3,EpEventLow              // compute address of low event
        li      r.5, 1                          // set user mode value
        b       ..KiSetServerWaitClientEvent   // finish in wait code

noevent_1:
        LWI(r.3,STATUS_NO_EVENT_PAIR)
        LEAF_EXIT(NtSetLowWaitHighThread)

//      .end    NtSetLowWaitHighThread

//      SBTTL("Set High Wait Low Thread")
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

        lwz     r.3,KiPcr+PcCurrentThread(r.0)
        lwz     r.3,EtEventPair(r.3)            // get address of event pair object
        cmpwi   r.3,0
        addi    r.4,r.3,EpEventLow              // compute address of low event
        beq     noevent_2                       // if eq, no event pair associated
        addi    r.3,r.3,EpEventHigh             // compute address of high event
        li      r.5,1                           // set user mode value
        b       ..KiSetServerWaitClientEvent   // finish in wait code

noevent_2:
        LWI(r.3,STATUS_NO_EVENT_PAIR)
        LEAF_EXIT(NtSetHighWaitLowThread)

//      .end    NtSetHighWaitLowThread
