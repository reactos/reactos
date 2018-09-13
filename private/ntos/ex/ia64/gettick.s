//++
//
// Module Name:
//
//     gettick.s
//
// Abstract:
//
//    This module implements the system service that returns the number
//    of milliseconds since the system was booted.
//
// Author:
//
//    William K. Cheung (wcheung) 25-Sep-95
//
// Revision History:
//
//    02-Feb-96    Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file       "gettick.s"

        .global    KeTickCount
        .global    ExpTickCountMultiplier

//++
//
// ULONG
// NtGetTickCount (
//    VOID
//    )
//
// Routine Description:
//
//    This function computes the number of milliseconds since the system
//    was booted. The computation is performed by multiplying the clock
//    interrupt count by a scaled fixed binary multiplier and then right
//    shifting the 64-bit result to extract the 32-bit millisecond count.
//
//    The multiplier fraction is scaled by 24 bits. Thus for a 100 Hz clock
//    rate, there are 10 ticks per millisecond, and the multiplier is
//    0x0a000000 (10 << 24). For a 128 Hz clock rate, there are 7.8125, or
//    7 13/16 ticks per millisecond, and so the multiplier is 0x07d00000.
//
//    This effectively replaces a (slow) divide instruction with a (fast)
//    multiply instruction. The multiplier value is only calculated once
//    based on the TimeIncrement value (clock tick interval in 100ns units).
//
//    N.B. The tick count value wraps every 2^32 milliseconds (49.71 days).
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The number of milliseconds since the system was booted is returned
//    as the function value.
//
//--

        LEAF_ENTRY(NtGetTickCount)

//
// load zero-extended operands
//

        add         t0 = @gprel(KeTickCount), gp
        add         t1 = @gprel(ExpTickCountMultiplier), gp
        ;;

        ld4         t2 = [t0]
        ld4         t3 = [t1]
        ;;

//
// compute 64-bit product
//

        setf.sig    fs2 = t2
        setf.sig    fs3 = t3
        ;;
        xmpy.lu     fs1 = fs2, fs3
        ;;

//
// shift off 24-bit fraction part and 
// the 32-bit canonical ULONG integer part.
//

        getf.sig    v0 = fs1
        ;;
        extr.u      v0 = v0, 24, 32

        br.ret.sptk.clr brp                    // return

        LEAF_EXIT(NtGetTickCount)
