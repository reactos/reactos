//      TITLE("Get Interrupt Request Level")
//++
//
// Module Name:
//
//    irql2.s
//
// Abstract:
//
//    This module implements the code to read the tpr register and convert
//    the result to Interrupt Request Level (IRQL).
//
//
// Author:
//
//    William K. Cheung (wcheung)
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksia64.h"

         .file    "irql2.s"

//++
//
// KIRQL
// KeGetCurrentIrql(
//      VOID
//      )
//
// Routine Description:
//
//    This function returns the current irql of the processor.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Current processor irql.
//
//--

        LEAF_ENTRY(KeGetCurrentIrql)
        LEAF_SETUP(0,0,0,0)

        GET_IRQL(v0)
        
        LEAF_RETURN
        LEAF_EXIT(KeGetCurrentIrql)
