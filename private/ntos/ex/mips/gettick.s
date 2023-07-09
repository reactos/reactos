//       TITLE("Get Tick Count")
//++
//
// Copyright (c) 1992  Microsoft Corporation
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
//--

#include "ksmips.h"

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
//    N.B. The tick count value wraps every 46.29 days.
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

        lw      t0,KeTickCount          // get current tick count value
        lw      t1,ExpTickCountMultiplier // get tick count multiplier
        multu   t0,t1                   // compute 64-bit unsigned product
        mflo    v0                      // get low 32-bits of product
        mfhi    v1                      // get high 32-bit of product
        srl     v0,v0,24                // extract 32-bit integer part
        sll     v1,v1,32 - 24           //
        or      v0,v0,v1                //

        j       ra                      // return

        .end    NtGetTickCount
