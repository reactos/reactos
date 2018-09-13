//      TITLE("Set Jump")
//++
//
// Copyright (c) 1993  Microsoft Corporation
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    setjmp.s
//
// Abstract:
//
//    This module implements the Alpha acc compiler specific routine to
//    provide SAFE handling of setjmp/longjmp with respect to structured
//    exception handling.
//
//    N.B. This function has been replaced by setjmp/setjmpex/longjmp in the
//         C runtime library. It remains here for backwards compatibility of
//         Beta 2 applications expecting setjmp and longjmp to be present in
//         ntdll, or for new acc compiled applications that link with ntdll
//         before libc.
//
// Author:
//
//    David N. Cutler (davec) 2-Apr-1993
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 22-Apr-1993
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Set Jump")
//++
//
// int
// setjmp (
//    IN jmp_buf JumpBuffer
//    )
//
// Routine Description:
//
//    This function implements a safe setjmp.
//
// Arguments:
//
//    JumpBuffer (a0) - Supplies the address of a jump buffer to store the
//       jump information.
//
//    N.B. This is an array of double's to force quadword alignment.
//
// Return Value:
//
//    A value of zero is returned.
//
//--

        .struct 0
SjRa:   .space  8                       // saved return address
SjS0:   .space  8                       // saved integer register s0
SjFl:   .space  8                       // InFunction flag variable
SjEf:   .space  8                       // EstablisherFrame(s) structure
SjCx:   .space  ContextFrameLength      // context frame
SetjmpFrameLength:

        NESTED_ENTRY(setjmp, SetjmpFrameLength, zero)

        lda     sp, -SetjmpFrameLength(sp) // allocate stack frame
        stq     ra, SjRa(sp)            // save return address
        stq     s0, SjS0(sp)            // save integer register s0

        PROLOGUE_END

//
// Save the nonvolatile machine state.
//

        lda     t0, SjCx(sp)            // address of context record

        .set    noreorder
        .set    noat
        stt     f2, CxFltF2(t0)         // save floating registers f2 - f9
        stt     f3, CxFltF3(t0)         //
        stt     f4, CxFltF4(t0)         //
        stt     f5, CxFltF5(t0)         //
        stt     f6, CxFltF6(t0)         //
        stt     f7, CxFltF7(t0)         //
        stt     f8, CxFltF8(t0)         //
        stt     f9, CxFltF9(t0)         //

        stq     s0, CxIntS0(t0)         // save integer registers s0 - fp/s6
        stq     s1, CxIntS1(t0)         //
        stq     s2, CxIntS2(t0)         //
        stq     s3, CxIntS3(t0)         //
        stq     s4, CxIntS4(t0)         //
        stq     s5, CxIntS5(t0)         //
        stq     fp, CxIntFp(t0)         //
        .set    at
        .set    reorder

        stq     gp, CxIntGp(t0)         // save integer register gp
        lda     v0, SetjmpFrameLength(sp) // compute stack pointer of caller
        stq     v0, CxIntSp(t0)         // save caller stack pointer
        stq     ra, CxIntRa(t0)         // save return address
        stq     ra, CxFir(t0)           // save continuation address
        ldil    t1, 2                   // get acc safe setjmp flag
        stl     t1, JbType(a0)          // set jump buffer context type
        STP     ra, JbPc(a0)            // save target instruction address
        mov     a0, s0                  // preserve jump buffer address

//
// Perform unwind to determine the virtual frame pointer of the caller.
//

        SUBP    ra, 4, a0               // compute control PC address
        bsr     RtlLookupFunctionEntry  // lookup function table address

        ldq     a0, SjRa(sp)            // get return address
        SUBP    a0, 4, a0               // compute control PC address
        mov     v0, a1                  // set address of function entry
        lda     a2, SjCx(sp)            // address of context record
        lda     a3, SjFl(sp)            // set address of in function variable
        lda     a4, SjEf(sp)            // set frame pointers address
        mov     zero, a5                // set context pointer array address
        bsr     RtlVirtualUnwind        // compute virtual frame pointer value

//
// Set return value, restore registers, deallocate stack frame, and return.
//

        LDP     t0, SjEf(sp)            // get virtual frame pointer
        STP     t0, JbFp(s0)            // save virtual frame pointer address
        mov     zero, v0                // set return value

        ldq     s0, SjS0(sp)            // restore integer register s0
        ldq     ra, SjRa(sp)            // restore return address
        lda     sp, SetjmpFrameLength(sp) // deallocate stack frame
        ret     zero, (ra)              // return

        .end    setjmp
