//      TITLE("Capture and Restore Context")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    capture.s
//
// Abstract:
//
//    This module implements the code necessary to capture and restore
//    the context of the caller.
//
// Author:
//
//    David N. Cutler (davec) 14-Sep-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 13-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Capture Context")
//++
//
// VOID
// RtlCaptureContext (
//    OUT PCONTEXT ContextRecord
//    )
//
// Routine Description:
//
//    This function captures the context of the caller in the specified
//    context record.
//
//    The context record is assumed to be quadword aligned (since all the
//    members of the record are themselves quadwords).
//
//    N.B. The stored value of registers a0 and ra will be a side effect of
//        having made this call. All other registers will be stored as they
//        were when the call to this function was made.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies the address of a context record.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlCaptureContext)

//
// Save all the integer registers.
//

        .set    noreorder
        .set    noat
        stq     v0, CxIntV0(a0)         //  0: store integer register v0
        stq     t0, CxIntT0(a0)         //  1: store integer registers t0 - t7
        stq     t1, CxIntT1(a0)         //  2:
        stq     t2, CxIntT2(a0)         //  3:
        stq     t3, CxIntT3(a0)         //  4:
        stq     t4, CxIntT4(a0)         //  5:
        stq     t5, CxIntT5(a0)         //  6:
        stq     t6, CxIntT6(a0)         //  7:
        stq     t7, CxIntT7(a0)         //  8:

        stq     s0, CxIntS0(a0)         //  9: store integer registers s0 - s5
        stq     s1, CxIntS1(a0)         // 10:
        stq     s2, CxIntS2(a0)         // 11:
        stq     s3, CxIntS3(a0)         // 12:
        stq     s4, CxIntS4(a0)         // 13:
        stq     s5, CxIntS5(a0)         // 14:
        stq     fp, CxIntFp(a0)         // 15: store integer register fp/s6

        stq     a0, CxIntA0(a0)         // 16: store integer registers a0 - a5
        stq     a1, CxIntA1(a0)         // 17:
        stq     a2, CxIntA2(a0)         // 18:
        stq     a3, CxIntA3(a0)         // 19:
        stq     a4, CxIntA4(a0)         // 20:
        stq     a5, CxIntA5(a0)         // 21:

        stq     t8, CxIntT8(a0)         // 22: store integer registers t8 - t11
        stq     t9, CxIntT9(a0)         // 23:
        stq     t10, CxIntT10(a0)       // 24:
        stq     t11, CxIntT11(a0)       // 25:

        stq     ra, CxIntRa(a0)         // 26: store integer register ra
        stq     t12, CxIntT12(a0)       // 27: store integer register t12
        stq     AT, CxIntAt(a0)         // 28: store integer register at
        stq     gp, CxIntGp(a0)         // 29: store integer register gp
        stq     sp, CxIntSp(a0)         // 30: store integer register sp
        stq     zero, CxIntZero(a0)     // 31: store integer register zero

//
// Save all the floating registers, and the floating control register.
//

        stt     f0, CxFltF0(a0)         // store floating registers f0 - f31
        stt     f1, CxFltF1(a0)         //
        stt     f2, CxFltF2(a0)         //
        stt     f3, CxFltF3(a0)         //
        stt     f4, CxFltF4(a0)         //
        stt     f5, CxFltF5(a0)         //
        stt     f6, CxFltF6(a0)         //
        stt     f7, CxFltF7(a0)         //
        stt     f8, CxFltF8(a0)         //
        stt     f9, CxFltF9(a0)         //
        stt     f10, CxFltF10(a0)       //
        stt     f11, CxFltF11(a0)       //
        stt     f12, CxFltF12(a0)       //
        stt     f13, CxFltF13(a0)       //
        stt     f14, CxFltF14(a0)       //
        stt     f15, CxFltF15(a0)       //
        stt     f16, CxFltF16(a0)       //
        stt     f17, CxFltF17(a0)       //
        stt     f18, CxFltF18(a0)       //
        stt     f19, CxFltF19(a0)       //
        stt     f20, CxFltF20(a0)       //
        stt     f21, CxFltF21(a0)       //
        stt     f22, CxFltF22(a0)       //
        stt     f23, CxFltF23(a0)       //
        stt     f24, CxFltF24(a0)       //
        stt     f25, CxFltF25(a0)       //
        stt     f26, CxFltF26(a0)       //
        stt     f27, CxFltF27(a0)       //
        stt     f28, CxFltF28(a0)       //
        stt     f29, CxFltF29(a0)       //
        stt     f30, CxFltF30(a0)       //
        stt     f31, CxFltF31(a0)       //

//
// Save control information and set context flags.
//

        mf_fpcr f0, f0, f0              // get floating point control register
        stt     f0, CxFpcr(a0)          // store it
        ldt     f0, CxFltF0(a0)         // restore original f0 value
        stq     ra, CxFir(a0)           // set continuation address
        .set    at
        .set    reorder

//
// If called from user mode set a known fixed PSR value for user mode. If
// called from kernel mode we are able to execute the privileged call pal
// to get the actual PSR.
//

        ldil    v0, PSR_USER_MODE       // set constant user processor status
        bgt     sp, 10f                 // if sp > 0, call came from user mode

        GET_CURRENT_PROCESSOR_STATUS_REGISTER // get contents of PSR in v0

10:     stl     v0, CxPsr(a0)           // store processor status
        ldil    v0, CONTEXT_FULL        // set context control flags
        stl     v0, CxContextFlags(a0)  // store it
        ret     zero, (ra)              // return

        .end    RtlCaptureContext

        SBTTL("Restore Context")
//++
//
// VOID
// RtlpRestoreContext (
//    IN PCONTEXT ContextRecord
//    )
//
// Routine Description:
//
//    This function restores the context of the caller to the specified
//    context.
//
//    The context record is assumed to be quadword aligned (since all the
//    members of the record are themselves quadwords).
//
//    N.B. This is a special routine that is used by RtlUnwind to restore
//       context in the current mode. PSR, t0, and t1 are not restored.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies the address of a context record.
//
// Return Value:
//
//    None.
//
//    N.B. There is no return from this routine.
//
//--

        LEAF_ENTRY(RtlpRestoreContext)

//
// If the call is from user mode, then use the continue system service to
// continue execution. Otherwise, restore the context directly since the
// current mode is kernel and threads can't be arbitrarily interrupted.
//

        blt     ra, 10f                 // if ra < 0, kernel mode restore
        ldil    a1, FALSE               // set test alert argument false
        bsr     ra, ZwContinue          // continue execution

//
// Save the address of the context record and continuation address in
// registers t0 and t1. These registers are not restored.
//

10:     mov     a0, t0                  // save context record address
        ldq     t1, CxFir(t0)           // get continuation address

//
// Restore floating control and floating registers f0 - f30.
//

        .set    noreorder
        .set    noat
        ldt     f0, CxFpcr(t0)          // get floating point control register
        mt_fpcr f0, f0, f0              // load register

        ldt     f0, CxFltF0(t0)         // restore floating registers f0 - f30
        ldt     f1, CxFltF1(t0)         //
        ldt     f2, CxFltF2(t0)         //
        ldt     f3, CxFltF3(t0)         //
        ldt     f4, CxFltF4(t0)         //
        ldt     f5, CxFltF5(t0)         //
        ldt     f6, CxFltF6(t0)         //
        ldt     f7, CxFltF7(t0)         //
        ldt     f8, CxFltF8(t0)         //
        ldt     f9, CxFltF9(t0)         //
        ldt     f10, CxFltF10(t0)       //
        ldt     f11, CxFltF11(t0)       //
        ldt     f12, CxFltF12(t0)       //
        ldt     f13, CxFltF13(t0)       //
        ldt     f14, CxFltF14(t0)       //
        ldt     f15, CxFltF15(t0)       //
        ldt     f16, CxFltF16(t0)       //
        ldt     f17, CxFltF17(t0)       //
        ldt     f18, CxFltF18(t0)       //
        ldt     f19, CxFltF19(t0)       //
        ldt     f20, CxFltF20(t0)       //
        ldt     f21, CxFltF21(t0)       //
        ldt     f22, CxFltF22(t0)       //
        ldt     f23, CxFltF23(t0)       //
        ldt     f24, CxFltF24(t0)       //
        ldt     f25, CxFltF25(t0)       //
        ldt     f26, CxFltF26(t0)       //
        ldt     f27, CxFltF27(t0)       //
        ldt     f28, CxFltF28(t0)       //
        ldt     f29, CxFltF29(t0)       //
        ldt     f30, CxFltF30(t0)       //

//
// Restore integer registers and continue execution.
// N.B. Integer registers t0 and t1 cannot be restored by this function.
//

        ldq     v0, CxIntV0(t0)         // restore integer register v0
        ldq     t2, CxIntT2(t0)         // restore integer registers t2 - t7
        ldq     t3, CxIntT3(t0)         //
        ldq     t4, CxIntT4(t0)         //
        ldq     t5, CxIntT5(t0)         //
        ldq     t6, CxIntT6(t0)         //
        ldq     t7, CxIntT7(t0)         // 

        ldq     s0, CxIntS0(t0)         // restore integer registers s0 - s5
        ldq     s1, CxIntS1(t0)         //
        ldq     s2, CxIntS2(t0)         //
        ldq     s3, CxIntS3(t0)         //
        ldq     s4, CxIntS4(t0)         //
        ldq     s5, CxIntS5(t0)         //
        ldq     fp, CxIntFp(t0)         // restore integer register fp/s6

        ldq     a0, CxIntA0(t0)         // restore integer registers a0 - a5
        ldq     a1, CxIntA1(t0)         //
        ldq     a2, CxIntA2(t0)         //
        ldq     a3, CxIntA3(t0)         //
        ldq     a4, CxIntA4(t0)         //
        ldq     a5, CxIntA5(t0)         //

        ldq     t8, CxIntT8(t0)         // restore integer registers t8 - t11
        ldq     t9, CxIntT9(t0)         //
        ldq     t10, CxIntT10(t0)       // 
        ldq     t11, CxIntT11(t0)       //

        ldq     ra, CxIntRa(t0)         // restore integer register ra
        ldq     t12, CxIntT12(t0)       // restore integer register t12
        ldq     AT, CxIntAt(t0)         // restore integer register at
        ldq     gp, CxIntGp(t0)         // restore integer register gp
        ldq     sp, CxIntSp(t0)         // restore integer register sp

        jmp     zero, (t1)              // continue execution
        .set    at
        .set    reorder

        .end    RtlpRestoreContext
