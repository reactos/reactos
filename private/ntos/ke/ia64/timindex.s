//      TITLE("Compute Timer Table Index")
//++
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

#include "ksia64.h"

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

        .global  KiTimeIncrementReciprocal
        .global  KiTimeIncrementShiftCount

        LEAF_ENTRY(KiComputeTimerTableIndex)

        add      t2 = @gprel(KiTimeIncrementReciprocal), gp
        add      t3 = @gprel(KiTimeIncrementShiftCount), gp
        ;;

//
// Capture global values for magic divide, the reciprocal multiply value
// and the shift count.
//

        ARGPTR   (a2)

        ld8      t2 = [t2]
        add      t0 = TiDueTime, a2
        ;;

        ld1      t3 = [t3] 
        setf.sig ft1 = t2
        ;;

//
// Compute the due time and store in the timer object.
//

        sub      t4 = a1, a0
        ;;
        setf.sig ft0 = t4
        ;;

//
// Do the reciprocal multiply and capture the upper 64 bits of the
// 128 bit product with xma.h instruction.
//

        st8      [t0] = t4
        xma.hu   ft2 = ft0, ft1, f0
        ;;

        getf.sig v0 = ft2
        movl     t4 = TIMER_TABLE_SIZE - 1
        ;;

//
// Right shift the result by the specified shift count and mask off extra
// bits.
//

        shr      v0 = v0, t3
        ;;
        and      v0 = v0, t4
 
        LEAF_RETURN

        LEAF_EXIT(KiComputeTimerTableIndex)
