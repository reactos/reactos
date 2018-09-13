//      TITLE("Get Current TEB Pointer")
//++
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    ntcurteb.s
//
// Abstract:
//
//    This module implements the function to retrieve the current TEB pointer.
//
// Author:
//
//    Joe Notarangelo 29-Jul-1992
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

//++
//
// PTEB
// NtCurrentTeb(
//     VOID
//     )
//
// Routine Description:
//
//    This function returns the current TEB pointer retrieved via an unprivileged
//    call pal.  Since the call pal is unprivileged this routine is appropriate in
//    any mode.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Current TEB pointer.
//
//--

        LEAF_ENTRY(NtCurrentTeb)

	GET_THREAD_ENVIRONMENT_BLOCK    // (PALcode) result in v0

	ret	zero, (ra)		// return

        .end    NtCurrentTeb
