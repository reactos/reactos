//      TITLE("Manipulate Interrupt Request Level")
//++
//
// Copyright (c) 1995  Microsoft Corporation
//
// Module Name:
//
//    irql.s
//
// Abstract:
//
//    This module implements the code necessary to lower and raise the current
//    Interrupt Request Level (IRQL).  On PowerPC, hardware IRQL levels are
//    managed in the HAL.  This module implements routines that deal with
//    software-only IRQLs.
//
// Author:
//
//    Chuck Lenzmeier (chuckl) 12-Oct-1995
//        based on code by David N. Cutler (davec)
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksppc.h"

        SBTTL("Raise Interrupt Request Level to DPC Level")
//++
//
// VOID
// KeRaiseIrqlToDpcLevel (
//    PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH level and returns
//    the old IRQL value.
//
// Arguments:
//
//    OldIrql (r3) - Supplies a pointer to a variable that recieves the old
//       IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeRaiseIrqlToDpcLevel)

        li      r4,DISPATCH_LEVEL       // set new IRQL value
        RAISE_SOFTWARE_IRQL(r4,r3,r5)

        LEAF_EXIT(KeRaiseIrqlToDpcLevel)

        SBTTL("Raise Interrupt Request Level to DPC Level")
//++
//
// KIRQL
// KfRaiseIrqlToDpcLevel (
//    VOID
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH level and returns
//    the old IRQL value.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

        LEAF_ENTRY(KfRaiseIrqlToDpcLevel)

//
// On PPC, synchronization level is the same as dispatch level.
//

        ALTERNATE_ENTRY(KeRaiseIrqlToSynchLevel)

        li      r4,DISPATCH_LEVEL               // set new IRQL value
        lbz     r3,KiPcr+PcCurrentIrql(0)       // get current IRQL
        stb     r4,KiPcr+PcCurrentIrql(0)       // set new IRQL

        LEAF_EXIT(KfRaiseIrqlToDpcLevel)
