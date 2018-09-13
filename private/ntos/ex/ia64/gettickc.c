//++
//
// Module Name:
//
//     gettickc.c
//
// Abstract:
//
//    This module implements the system service that returns the number
//    of milliseconds since the system was booted.
//
// Author:
//
//    Bernard Lint
//
// Revision History:
//
//    Based on gettick.s (wcheung)
//
//--

#include "exp.h"
#undef NtGetTickCount

ULONG
NtGetTickCount (
   VOID
   )

/*++

Routine Description:

    This function computes the number of milliseconds since the system
    was booted. The computation is performed by multiplying the clock
    interrupt count by a scaled fixed binary multiplier and then right
    shifting the 64-bit result to extract the 32-bit millisecond count.

    The multiplier fraction is scaled by 24 bits. Thus for a 100 Hz clock
    rate, there are 10 ticks per millisecond, and the multiplier is
    0x0a000000 (10 << 24). For a 128 Hz clock rate, there are 7.8125, or
    7 13/16 ticks per millisecond, and so the multiplier is 0x07d00000.

    This effectively replaces a (slow) divide instruction with a (fast)
    multiply instruction. The multiplier value is only calculated once
    based on the TimeIncrement value (clock tick interval in 100ns units).

    N.B. The tick count value wraps every 2^32 milliseconds (49.71 days).

Arguments:

    None.

Return Value:

    The number of milliseconds since the system was booted is returned
    as the function value.

--*/

{
    ULONGLONG Product;

    //
    // compute unsigned 64-bit product
    //

    Product = (ULONGLONG)KeTickCount * ExpTickCountMultiplier;

    //
    // shift off 24-bit fraction part and 
    // return the 32-bit canonical ULONG integer part.
    //
            
    return ((ULONG)(Product >> 24));
}


