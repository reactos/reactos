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
//    Chuck Bauman 20-Mar-1993
//
//  Environment:
//
//     Any mode.
//
//  Revision History:
//
//    Port NT product1 source to PowerPC 12-Aug-1993
//    Changed to nested function and added conversion of function
//    descriptor.   Peter Johnston (plj@vnet.ibm.com) 18-Aug-1994
//
//--

#include "ksppc.h"


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
//    This routine is called via APC and is the first code executed in
//    user mode for every user mode thread.
//
//    On entry to this routine, the current stack frame is immediately
//    preceeded by a context record containing the initial state of the
//    new thread.
//
//    This function computes a pointer to the context record on the stack
//    and calls LdrpInitialize with that pointer as its parameter.
//
//    On return from LdrpInitialize, we convert the function descriptor
//    whose address is in the Context record Iar field from a pointer to
//    a function descriptor to the actual TOC and entry point values for
//    the thread.
//
// Arguments:
//
//    NormalContext   (r.3) - User Mode APC context parameter (ignored).
//
//    SystemArgument1 (r.4) - User Mode APC system argument 1 (ignored).
//
//    SystemArgument2 (r.5) - User Mode APC system argument 2 (ignored).
//
// Return Value:
//
//    None.
//
//--

        .extern ..LdrpInitialize

        .struct 0
        .space  STK_MIN_FRAME
LitLr:  .space  4
        .align  3
LitFr1: .space  STK_MIN_FRAME
LitCx:  .space  ContextFrameLength
        .align  3
LitFrameLen:

        SPECIAL_ENTRY(LdrInitializeThunk)
        mflr    r.0
        stwu    r.sp, -LitFr1(r.sp)             // buy stack frame
        addi    r.3, r.sp, LitCx                // compute context record addr
        stw     r.0, LitLr(r.sp)                // save return address
        PROLOGUE_END(LdrInitializeThunk)

        bl      ..LdrpInitialize                // Jump to LdrpInitialize
        lwz     r.11, LitCx + CxGpr2(r.sp)      // TOC set
        cmpwi   r.11, 0                         // jif true context passed
                                                // (e.g: fork)
        lwz     r.11, LitCx + CxIar(r.sp)       // read fn descr from Cr Iar
        lwz     r.0,  LitLr(r.sp)               // get return address
        bne     truectx
        lwz     r.12, 4(r.11)                   // get toc
        lwz     r.11, 0(r.11)                   // get fn entry
        stw     r.12, LitCx + CxGpr2(r.sp)      // set initial toc
        stw     r.11, LitCx + CxIar(r.sp)       // set entry point

truectx:
        mtlr    r.0                             // restore return address

        addi    r.sp, r.sp, LitFr1              // free out stack frame
        SPECIAL_EXIT(LdrInitializeThunk)

