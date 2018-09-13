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
//    Sep 19, 1993    plj    Conversion to IBM PowerPC
//
//--

#include "ksppc.h"

//
// Define external vsriables that can be addressed using GP.
//

        .extern KiTimeIncrementReciprocal
        .extern KiTimeIncrementShiftCount

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
//    Interval (r.3, r.4) - Supplies the relative time at which the timer is
//        to expire.
//
//    CurrentTime (r.5, r.6) - Supplies the current interrupt time.
//
//    Timer (r.7) - Supplies a pointer to a dispatch object of type timer.
//
// Return Value:
//
//    The time table index is returned as the function value and the due
//    time is stored in the timer object.
//
//--

        LEAF_ENTRY(KiComputeTimerTableIndex)

// Get addresses of KiTimeIncrementReciprocal and KiTimeIncrementShiftCounter

	lwz	r.11, [toc]KiTimeIncrementReciprocal(r.toc)
	lwz	r.10, [toc]KiTimeIncrementShiftCount(r.toc)

	lwz	r.12, 4(r.11)		// get high part of magic divisor
	lwz	r.11, 0(r.11)		// get low part of magic divisor

// Calculate DueTime = CurrentTime - Interval, result in {r.3,r.4}

	subfc	r.3, r.3, r.5           // subtract low parts (with carry)
	subfe	r.4, r.4, r.6           // subtract high parts (using carry)

	lbz	r.10, 0(r.10)		// get shift count

        stw	r.3, TiDueTime(r.7)     // set due time of timer object
        stw	r.4, TiDueTime+4(r.7)

//
// Compute low 32-bits of dividend times low 32-bits of divisor.
//
	mulhwu	r.0, r.3, r.11

//
// Compute low 32-bits of dividend times high 32-bits of divisor.
//
	mullw	r.6, r.3, r.12		// low 32-bits to r.6
	mulhwu	r.7, r.3, r.12		// high 32-bits to r.7

//
// Compute high 32-bits of dividend times low 32-bits of divisor.
//
	mullw	r.8, r.4, r.11		// low 32-bits to r.8
	mulhwu	r.9, r.4, r.11		// high 32-bits to r.9

//
// Compute high 32-bits of dividend times high 32-bits of divisor.
//
	mullw	r.3, r.4, r.12          // low 32-bits to r.3
        mulhwu  r.4, r.4, r.12          // high 32-bits to r.4

//
// On the grounds that I can't do more than double precision arithmetic
// without visual aids,  I will attempt to draw a picture of what is
// going on here,....  my apologies to those who don't need the pic.
//
//
//      _________________________________________________________________
//      |               |               |               |               |
//      |               |               |  Due Time Low * Recip Low     |
//      |               |               |       r.0             -       |
//      |---------------------------------------------------------------|
//      |               |  Due Time Low * Recip High    |               |
//      |               |       r.7             r.6     |               |
//      |------------------------------------------------               |
//      |               | Due Time High * Recip Low     |               |
//      |               |       r.9             r.8     |               |
//      |------------------------------------------------               |
//      | Due Time High * Recip High    |               |               |
//      |       r.4             r.3     |               |               |
//      |---------------------------------------------------------------|
//      |               |               |               |               |
//      |       x       |       y       |       z       |               |
//      |----------------------------------------------------------------
//

//
// Add partial results to form high 64-bits of result.
//
// (add 3 low parts together (r.0, r.6 and r.8) generating carries.
// the carries are added to the sum of the high parts (r.7, r.9 and r.3),
// the sum of the low parts is discarded).
//
	addc	r.0, r.6, r.0		// z = r.0 + r.6
	adde	r.7, r.9, r.7		// y = r.7 + r.9 + carry from z
        addze   r.4, r.4                // x += carry from y
	addc	r.0, r.8, r.0		// z += r.8
	adde	r.3, r.3, r.7		// y += r.3 + carry from z
        addze   r.4, r.4                // x += carry from y

//
// Combine low part of x with high part of y
//
// N.B. It is assumed that the shift count is less than 32-bits and not zero.
//
        subfic  r.11, r.10, 32          // compute left shift count
	srw	r.3, r.3, r.10		// get high part of low part
        slw     r.4, r.4, r.11          // get low part of high word
        or      r.3, r.4, r.3           // combine upper low and lower high
        rlwinm  r.3, r.3, 0, TIMER_TABLE_SIZE - 1

        LEAF_EXIT(KiComputeTimerTableIndex)
