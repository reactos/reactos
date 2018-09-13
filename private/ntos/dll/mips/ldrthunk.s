//      TITLE("LdrInitializeThunk")
//++
//
//  Copyright (c) 1989  Microsoft Corporation
//
//  Module Name:
//
//     ldrthunk.s
//
//  Abstract:
//
//     This module implements the thunk for the LdrpInitialize APC routine.
//
//  Author:
//
//     Steven R. Wood (stevewo) 27-Apr-1990
//
//  Environment:
//
//     Any mode.
//
//  Revision History:
//
//--

#include "ksmips.h"

//++
//
// VOID
// LdrInitializeThunk(
//    IN PVOID NormalContext,
//    IN PVOID SystemArgument1,
//    IN PVOID SystemArgument2
//    )
//
// Routine Description:
//
//    This function computes a pointer to the context record on the stack
//    and jumps to the LdrpInitialize function with that pointer as its
//    parameter.
//
// Arguments:
//
//    NormalContext (a0) - User Mode APC context parameter (ignored).
//
//    SystemArgument1 (a1) - User Mode APC system argument 1 (ignored).
//
//    SystemArgument2 (a2) - User Mode APC system argument 2 (ignored).
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(LdrInitializeThunk)

        .set    noreorder
        .set    noat
        j       LdrpInitialize          // Jump to LdrpInitialize
        move    a0,sp                   // Get address of context record
        .set    at
        .set    reorder

        .end LdrInitializeThunk

//++
//
// VOID
// LdrpSetGp(
//    IN ULONG GpValue
//    )
//
// Routine Description:
//
//    This function sets the value of the Gp register.
//
// Arguments:
//
//    GpValue (a0) - Supplies the value for Gp
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(LdrpSetGp)

        .set    noreorder
        .set    noat
        j       ra
        move    gp,a0
        .set    at
        .set    reorder

        .end LdrpSetGp
