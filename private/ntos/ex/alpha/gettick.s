//       TITLE("Get Tick Count")
//++
//
// Copyright (c) 1992  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    gettick.s
//
// Abstract:
//
//    This module contains the implementation for the get tick count
//    system service that returns the number of milliseconds since the
//    system was booted.
//
// Author:
//
//    David N. Cutler (davec) 10-Sep-1992
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 5-Oct-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Get Tick Count")
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

        ldl     t0, KeTickCount         // get current tick count value
        zap     t0, 0xf0, t0            // convert to unsigned longword
        ldl     t1, ExpTickCountMultiplier  // get tick count multiplier
        zap     t1, 0xf0, t1            // convert to unsigned longword
        mulq    t0, t1, v0              // compute 64-bit product
        srl     v0, 24, v0              // shift off 24-bit fraction part
        addl    v0, 0, v0               // keep 32-bit canonical ULONG integer part

        ret     zero, (ra)              // return

        .end    NtGetTickCount
