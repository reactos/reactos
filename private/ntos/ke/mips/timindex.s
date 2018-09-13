//      TITLE("Compute Timer Table Index")
//++
//
// Copyright (c) 1993  Microsoft Corporation
//
// Module Name:
//
//    timindex.s
//
// Abstract:
//
//    This module implements the code necessary to compute the timer table
//    index for a timer.
//
// Author:
//
//    David N. Cutler (davec) 17-May-1993
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define external vsriables that can be addressed using GP.
//

        .extern KiTimeIncrementReciprocal 2 * 4
        .extern KiTimeIncrementShiftCount 1

        SBTTL("Compute Timer Table Index")
//++
//
// ULONG
// KiComputeTimerTableIndex (
//    IN LARGE_INTEGER Interval,
//    IN LARGE_INTEGER CurrentTime,
//    IN PKTIMER Timer
//    )
//
// Routine Description:
//
//    This function compute the timer table index for the specified timer
//    object and stores the due time in the timer object.
//
//    N.B. The interval parameter is guaranteed to be negative since it is
//         expressed as relative time.
//
//    The formula for due time calculation is:
//
//    Due Time = Current time - Interval
//
//    The formula for the index calculation is:
//
//    Index = (Due Time / Maximum Time) & (Table Size - 1)
//
//    The due time division is performed using reciprocal multiplication.
//
// Arguments:
//
//    Interval (a0, a1) - Supplies the relative time at which the timer is
//        to expire.
//
//    CurrentTime (a2, a3) - Supplies the current interrupt time.
//
//    Timer (10(sp)) - Supplies a pointer to a dispatch object of type timer.
//
// Return Value:
//
//    The time table index is returned as the function value and the due
//    time is stored in the timer object.
//
//--

        LEAF_ENTRY(KiComputeTimerTableIndex)

        subu    t0,a2,a0                // subtract low parts
        subu    t1,a3,a1                // subtract high parts
        sltu    t2,a2,a0                // generate borrow from high part
        subu    t1,t1,t2                // subtract borrow
        lw      a0,4 * 4(sp)            // get address of timer object
        ld      t2,KiTimeIncrementReciprocal // get 64-bit magic divisor
        dsll    t0,t0,32                // isolate low 32-bits of due time
        dsrl    t0,t0,32                //
        dsll    t1,t1,32                // isolate high 32-bits of due time
        or      t3,t1,t0                // merge low and high parts of due time
        sd      t3,TiDueTime(a0)        // set due time of timer object

//
// Compute the product of the due time with the magic divisor.
//

        dmultu  t2,t3                   // compute 128-bit product
        lbu     v1,KiTimeIncrementShiftCount // get shift count
        mfhi    v0                      // get high 32-bits of product

//
// Right shift the result by the specified shift count and isolate the timer
// table index.
//

        dsrl    v0,v0,v1                // shift low half right count bits
        and     v0,v0,TIMER_TABLE_SIZE - 1 // compute index value
        j       ra                      // return

        .end    KiComputeTimerTableIndex
