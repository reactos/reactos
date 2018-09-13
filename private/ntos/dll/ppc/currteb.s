//      TITLE("LdrInitializeThunk")
//++
//
//  Copyright (c) 1993  IBM Corporation
//
//  Module Name:
//
//     currteb.s
//
//  Abstract:
//
//     This module creates the function descriptor for the fast path
//     service call to get the current TEB address. This is isolated
//     because only positive service numbers go in the service table
//     and NtCurrentTeb uses a negative service number for system
//     call optimization.
//
//
//  Author:
//
//    Chuck Bauman 29-Sep-1993
//
//  Environment:
//
//     User mode.
//
//  Revision History:
//
//--

#include "ksppc.h"

        LEAF_ENTRY(NtCurrentTeb)
        mr      r.3, r.13       // Return the TEB which should be in r.13
        LEAF_EXIT(NtCurrentTeb)
