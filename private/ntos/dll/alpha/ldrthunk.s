//      TITLE("LdrInitializeThunk")
//++
//
// Copyright (c) 1989  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    ldrthunk.s
//
// Abstract:
//
//    This module implements the thunk for the LdrpInitialize APC routine.
//
// Author:
//
//    Steven R. Wood (stevewo) 27-Apr-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 18-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

//++
//
// VOID
// LdrInitializeThunk (
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

        mov     sp, a0                  // get address of context record
        br      zero, LdrpInitialize    // jump to LdrpInitialize

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
//    GpValue (a0) - Supplies the value for Gp.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(LdrpSetGp)

        mov     a0, gp                  // set global pointer register
        ret     zero, (ra)              // return

        .end LdrpSetGp
