//      TITLE("PCR access")
//++
//
// Copyright (c) 1995  Microsoft Corporation
//
// Module Name:
//
//    pcr.s
//
// Abstract:
//
//    This module implements the routines for accessing PCR fields.
//    Specifically, routines that need multiple-instruction access
//    to the PCR and its related structures, and need to run with
//    interrupts disabled.
//
// Author:
//
//    Chuck Lenzmeier (chuckl) 5-Apr-95
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksppc.h"


        SBTTL("KeIsExecutingDpc")
//++
//
// BOOLEAN
// KeIsExecutingDpc (
//    VOID
//    )
//
// Routine Description:
//
//    This function returns the DPC Active flag on the current processor.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Current DPC Active flag.  This flag indicates if a DPC routine is
//    currently running on this processor.
//
//--

        LEAF_ENTRY(KeIsExecutingDpc)

#if !defined(NT_UP)
        DISABLE_INTERRUPTS(r9, r8)
#endif

//
// Get the DPC active indicator and return a BOOLEAN.  Note that
// DpcRoutineActive holds 0 or some nonzero value (usually the
// stack pointer), so it does not conform to the C TRUE/FALSE values.
//

        lwz     r4, KiPcr+PcPrcb(r0)            // get PRCB address
        li      r3, FALSE                       // assume DPC not active
        lwz     r4, PbDpcRoutineActive(r4)      // get DPC active indicator
        cmpwi   r4, 0                           // DPC active?
        beq     kied10                          // branch if not
        li      r3, TRUE                        // indicate DPC active
kied10:

#if !defined(NT_UP)
        ENABLE_INTERRUPTS(r9)
#endif

        LEAF_EXIT(KeIsExecutingDpc) // return

