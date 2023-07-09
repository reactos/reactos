//       TITLE("Get Tick Count")
//++
//
// Copyright (c) 1993  IBM Corporation
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

//      SBTTL("Get Tick Count")
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
        .extern KeTickCount
        .extern ExpTickCountMultiplier

        LEAF_ENTRY(NtGetTickCount)

        lwz     r.8,KiPcr2 + Pc2TickCountLow(r.0)// get current tick count value
        lwz     r.9,KiPcr2 + Pc2TickCountMultiplier(r.0)// get tick count multiplier
        mulhwu  r.3,r.8,r.9             // compute 64-bit unsigned product
        mullw   r.4,r.8,r.9
        rlwinm	r.3,r.3,8,0xFFFFFF00	// extract 32-bit integer part
        rlwimi	r.3,r.4,8,0x000000FF	//   "      "        "     "

        LEAF_EXIT(NtGetTickCount)       // return

