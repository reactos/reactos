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
//    Joe Notarangelo 20-Jul-1993  (Alpha AXP version)
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

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
//    This function computes the timer table index for the specified timer
//    object and stores the due time in the timer object.
//
//    N.B. The interval parameter is guaranteed to be negative since it is
//         expressed as relative time.
//
//    The formula for due time calculation is:
//
//    Due Time = Current Time - Interval
//
//    The formula for the index calculation is:
//
//    Index = (Due Time / Maximum Time) & (Table Size - 1)
//
//    The index division is performed using reciprocal multiplication.
//
// Arguments:
//
//    Interval (a0) - Supplies the relative time at which the timer is
//        to expire.
//
//    CurrentTime (a1) - Supplies the current interrupt time.
//
//    Timer (a2) - Supplies a pointer to a dispatch object of type timer.
//
// Return Value:
//
//    The time table index is returned as the function value and the due
//    time is stored in the timer object.
//
//--

        LEAF_ENTRY(KiComputeTimerTableIndex)

//
// Compute the due time and store in the timer object.
//

        subq    a1, a0, t0              // compute due time
        stq     t0, TiDueTime(a2)       // set due time of timer object

//
// Capture global values for magic divide, the reciprocal multiply value
// and the shift count.
//

	lda	t2, KiTimeIncrementReciprocal // get address of reciprocal
	ldq	t1, 0(t2)		// get timer reciprocal for magic divide
	lda	t2, KiTimeIncrementShiftCount // get address of shift count
	ldq_u	t10, 0(t2)		// read surrounding quadword
	extbl	t10, t2, t10		// extract shift count for magic divide

//
// Do the reciprocal multiply and capture the upper 64 bits of the
// 128 bit product with umulh instruction.
//

	umulh	t0, t1, t11		// t11 = upper 64 bits of product

//
// Right shift the result by the specified shift count and mask off extra
// bits.
//

	srl	t11, t10, t0		// t0 = division result
	ldil	t3, TIMER_TABLE_SIZE - 1 // get mask value
	and	t0, t3, v0		// v0 = mask result
	ret	zero, (ra)		// return
	
        .end    KiComputeTimerTableIndex
