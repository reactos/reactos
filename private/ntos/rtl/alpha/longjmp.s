//      TITLE("Long Jump")
//++
//
// Copyright (c) 1993  Microsoft Corporation
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    longjmp.s
//
// Abstract:
//
//    This module implements the Alpha specific routine to perform a long
//    jump operation. Three jump buffer types are supported: unsafe, safe
//    acc-style (virtual frame pointer, PC mapped SEH scope), and safe
//    GEM-style (real frame pointer, SEB-based SEH context).
//
//    N.B. This function has been replaced by setjmp/setjmpex/longjmp in the
//         C runtime library. It remains here for backwards compatibility of
//         Beta 2 applications expecting setjmp and longjmp to be present in
//         ntdll, or for new acc or GEM compiled applications that link with
//         ntdll before libc.
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

//
// Define jump buffer types.
//
//  _JMPBUF_TYPE_ZERO was used by the Beta 2 ntdll setjmp and can be handled
//      the same as _JMPBUF_TYPE_ACC.
//
//  _JMPBUF_TYPE_FAST is for jump buffers containing the set of non-volatile
//      integer and floating registers. This form of setjmp/longjmp is not
//      compatible with SEH. It is an order of magnitude faster however.
//
//  _JMPBUF_TYPE_ACC is for setjmp/longjmp compatible with SEH. The Alpha
//      acc compiler uses a virtual frame pointer, and SEH scope is inferred
//      from the PC through passive PC-mapping scope tables.
//
//  _JMPBUF_TYPE_GEM is for setjmp/longjmp compatible with SEH. The Alpha
//      GEM C compiler uses a real frame pointer, and SEH scope is maintained
//      actively with an SEB pointer.
//

#define _JMPBUF_TYPE_ZERO 0
#define _JMPBUF_TYPE_FAST 1
#define _JMPBUF_TYPE_ACC  2
#define _JMPBUF_TYPE_GEM  3

        SBTTL("Long Jump")
//++
//
// int
// longjmp (
//    IN jmp_buf JumpBuffer,
//    IN int ReturnValue
//    )
//
// Routine Description:
//
//    This function performs a long jump to the context specified by the
//    jump buffer.
//
// Arguments:
//
//    JumpBuffer (a0) - Supplies the address of a jump buffer that contains
//       jump information.
//
//    N.B. This is an array of double's to force quadword alignment.
//
//    ReturnValue (a1) - Supplies the value that is to be returned to the
//       caller of set jump.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(longjmp)

        ldil    t0, 1                   // force nonzero value, if
        cmoveq  a1, t0, a1              //   given return value is zero

        ldl     t1, JbType(a0)          // get setjmp context type flag
        subq    t1, 1, t2               // if eq 1, fast, unsafe longjmp
        bne     t2, 10f                 // otherwise, provide safe longjmp

//
// Type 0x1: Provide unsafe handling of longjmp.
//

        mov     a1, v0                  // set return value
        .set    noreorder
        .set    noat
        ldt     f2, JbFltF2(a0)         // restore floating registers f2 - f9
        ldt     f3, JbFltF3(a0)         //
        ldt     f4, JbFltF4(a0)         //
        ldt     f5, JbFltF5(a0)         //
        ldt     f6, JbFltF6(a0)         //
        ldt     f7, JbFltF7(a0)         //
        ldt     f8, JbFltF8(a0)         //
        ldt     f9, JbFltF9(a0)         //

        ldq     s0, JbIntS0(a0)         // restore integer registers s0 - s6/fp
        ldq     s1, JbIntS1(a0)         //
        ldq     s2, JbIntS2(a0)         //
        ldq     s3, JbIntS3(a0)         //
        ldq     s4, JbIntS4(a0)         //
        ldq     s5, JbIntS5(a0)         //
        ldq     fp, JbIntS6(a0)         //
        .set    at
        .set    reorder

        ldq     a1, JbFir(a0)           // get setjmp return address
        ldq     sp, JbIntSp(a0)         // restore stack pointer
        jmp     zero, (a1)              // jump back to setjmp site

//
// Type 0x0: Provide safe handling of longjmp (idw 404 style).
// Type 0x2: Provide safe handling of longjmp (acc style).
//

10:     bic     t1, 0x2, t2             // if 0 or 2, safe acc longjmp
        bne     t2, longjmpRfp          // if not, safe GEM longjmp

        mov     a1, a3                  // set return value
        mov     zero, a2                // set exception record address
        LDP     a1, JbPc(a0)            // set target instruction address
        LDP     a0, JbFp(a0)            // set target virtual frame pointer
        br      zero, RtlUnwind         // finish in common code

        .end    longjmp

        SBTTL("Long Jump - GEM")

        .struct 0
LjRa:   .space  8                       // saved return address
        .space  8                       // padding for 16-byte stack alignment
LjEr:   .space  ExceptionRecordLength   // local exception record
LongjmpFrameLength:

//
// Type 0x3: Provide safe handling of longjmp (GEM style).
//

        NESTED_ENTRY(longjmpRfp, LongjmpFrameLength, ra)

        lda     sp, -LongjmpFrameLength(sp) // allocate stack frame
        stq     ra, LjRa(sp)            // save return address

        PROLOGUE_END

//
// The SEB is passed to the GEM exception handler through an exception
// record. Set up the following local exception record:
//
//     ExceptionRecord.ExceptionCode = STATUS_UNWIND;
//     ExceptionRecord.ExceptionFlags = EXCEPTION_UNWINDING;
//     ExceptionRecord.ExceptionRecord = NULL;
//     ExceptionRecord.ExceptionAddress = 0;
//     ExceptionRecord.NumberParameters = 1;
//     ExceptionRecord.ExceptionInformation[0] = Seb;
//

10:     mov     a1, a3                      // set return value
        lda     a2, LjEr(sp)                // set exception record address

        ldil    t0, STATUS_UNWIND           // get status code
        stl     t0, ErExceptionCode(a2)     // store in exception record
        ldil    t1, EXCEPTION_UNWINDING     // get exception flags
        stl     t1, ErExceptionFlags(a2)    // store in exception record
        STP     zero, ErExceptionRecord(a2) // clear indirect exception record
        STP     zero, ErExceptionAddress(a2) // clear exception address
        ldil    t2, 1                       // get number of parameters
        stl     t2, ErNumberParameters(a2)  // store in exception record
        LDP     t3, JbSeb(a0)               // get SEB pointer
        STP     t3, ErExceptionInformation(a2) // store in exception record
        LDP     a1, JbPc(a0)                // set target instruction address
        LDP     a0, JbFp(a0)                // set target real frame pointer

        bsr     ra, RtlUnwindRfp            // finish in common code

        .end    longjmpRfp
