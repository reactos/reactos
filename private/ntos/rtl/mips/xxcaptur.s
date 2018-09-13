//      TITLE("Capture and Restore Context")
//++
//
// Copyright (c) 1990  Microsoft Corporation
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
//--

#include "ksmips.h"

//
// Define local symbols.
//

#define UserPsr (CU1_ENABLE) | (0xff << PSR_INTMASK) | (1 << PSR_UX) | \
                (2 << PSR_KSU) | (1 << PSR_IE) // constant user PSR value

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
// Save integer registers zero, at - t9, s8, gp, sp, ra, lo, and hi.
//

        .set    noreorder
        .set    noat
        sd      zero,CxXIntZero(a0)     //
        sd      AT,CxXIntAt(a0)         //
        sd      v0,CxXIntV0(a0)         //
        mflo    v0                      //
        sd      v1,CxXIntV1(a0)         //
        mfhi    v1                      //
        sd      v0,CxXIntLo(a0)         //
        sd      v1,CxXIntHi(a0)         //
        sd      a0,CxXIntA0(a0)         //
        sd      a1,CxXIntA1(a0)         //
        sd      a2,CxXIntA2(a0)         //
        sd      a3,CxXIntA3(a0)         //
        sd      t0,CxXIntT0(a0)         //
        sd      t1,CxXIntT1(a0)         //
        sd      t2,CxXIntT2(a0)         //
        sd      t3,CxXIntT3(a0)         //
        sd      t4,CxXIntT4(a0)         //
        sd      t5,CxXIntT5(a0)         //
        sd      t6,CxXIntT6(a0)         //
        sd      t7,CxXIntT7(a0)         //
        sd      s0,CxXIntS0(a0)         //
        sd      s1,CxXIntS1(a0)         //
        sd      s2,CxXIntS2(a0)         //
        sd      s3,CxXIntS3(a0)         //
        sd      s4,CxXIntS4(a0)         //
        sd      s5,CxXIntS5(a0)         //
        sd      s6,CxXIntS6(a0)         //
        sd      s7,CxXIntS7(a0)         //
        sd      t8,CxXIntT8(a0)         //
        sd      t9,CxXIntT9(a0)         //
        sd      zero,CxXIntK0(a0)       //
        sd      zero,CxXIntK1(a0)       //
        sd      s8,CxXIntS8(a0)         //
        sd      gp,CxXIntGp(a0)         //
        sd      sp,CxXIntSp(a0)         //
        sd      ra,CxXIntRa(a0)         //

//
// Save floating status and floating registers f0 - f31.
//

        cfc1    v0,fsr                  //
        sdc1    f0,CxFltF0(a0)          //
        sdc1    f2,CxFltF2(a0)          //
        sdc1    f4,CxFltF4(a0)          //
        sdc1    f6,CxFltF6(a0)          //
        sdc1    f8,CxFltF8(a0)          //
        sdc1    f10,CxFltF10(a0)        //
        sdc1    f12,CxFltF12(a0)        //
        sdc1    f14,CxFltF14(a0)        //
        sdc1    f16,CxFltF16(a0)        //
        sdc1    f18,CxFltF18(a0)        //
        sdc1    f20,CxFltF20(a0)        //
        sdc1    f22,CxFltF22(a0)        //
        sdc1    f24,CxFltF24(a0)        //
        sdc1    f26,CxFltF26(a0)        //
        sdc1    f28,CxFltF28(a0)        //
        sdc1    f30,CxFltF30(a0)        //
        .set    at
        .set    reorder

//
// Save control information and set context flags.
//

        sw      v0,CxFsr(a0)            // save floating status
        sw      ra,CxFir(a0)            // set continuation address
        li      v0,UserPsr              // set constant user processor status
        bgez    sp,10f                  // if gez, called from user mode

        .set    noreorder
        .set    noat
        mfc0    v0,psr                  // get current processor status
        nop                             //
        .set    at
        .set    reorder

10:     sw      v0,CxPsr(a0)            // set processor status
        li      t0,CONTEXT_FULL         // set context control flags
        sw      t0,CxContextFlags(a0)   //
        j       ra                      // return

        .end    RtlCaptureContext

        SBTTL("Restore Context")
//++
//
// VOID
// RtlpRestoreContext (
//    IN PCONTEXT ContextRecord,
//    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL
//    )
//
// Routine Description:
//
//    This function restores the context of the caller to the specified
//    context.
//
//    N.B. This is a special routine that is used by RtlUnwind to restore
//       context in the current mode. PSR, t0, and t1 are not restored.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies the address of a context record.
//
//    ExceptionRecord (a1) - Supplies an optional pointer to an exception
//       record.
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
// If an exception record is specified and the exception status is
// STATUS_LONGJUMP, then restore the nonvolatile registers to their
// state at the call to setjmp before restoring the context record.
//

        li      t0,STATUS_LONGJUMP      // get long jump status code
        beq     zero,a1,5f              // if eq, no exception record
        lw      t1,ErExceptionCode(a1)  // get exception code
        lw      a2,ErExceptionInformation(a1) // get address of jump buffer
        bne     t0,t1,5f                // if ne, not a long jump
        ldc1    f20,JbFltF20(a2)        // move floating registers f20 - f31
        ldc1    f22,JbFltF22(a2)        //
        ldc1    f24,JbFltF24(a2)        //
        ldc1    f26,JbFltF26(a2)        //
        ldc1    f28,JbFltF28(a2)        //
        ldc1    f30,JbFltF30(a2)        //
        sdc1    f20,CxFltF20(a0)        //
        sdc1    f22,CxFltF22(a0)        //
        sdc1    f24,CxFltF24(a0)        //
        sdc1    f26,CxFltF26(a0)        //
        sdc1    f28,CxFltF28(a0)        //
        sdc1    f30,CxFltF30(a0)        //
        lw      s0,JbIntS0(a2)          // move integer registers s0 - s8
        lw      s1,JbIntS1(a2)          //
        lw      s2,JbIntS2(a2)          //
        lw      s3,JbIntS3(a2)          //
        lw      s4,JbIntS4(a2)          //
        lw      s5,JbIntS5(a2)          //
        lw      s6,JbIntS6(a2)          //
        lw      s7,JbIntS7(a2)          //
        lw      s8,JbIntS8(a2)          //
        sd      s0,CxXIntS0(a0)         //
        sd      s1,CxXIntS1(a0)         //
        sd      s2,CxXIntS2(a0)         //
        sd      s3,CxXIntS3(a0)         //
        sd      s4,CxXIntS4(a0)         //
        sd      s5,CxXIntS5(a0)         //
        sd      s6,CxXIntS6(a0)         //
        sd      s7,CxXIntS7(a0)         //
        sd      s8,CxXIntS8(a0)         //
        lw      v0,JbFir(a2)            // move fir and sp
        lw      v1,JbIntSp(a2)          //
        sw      v0,CxFir(a0)            //
        sd      v0,CxXIntRa(a0)         //
        sd      v1,CxXIntSp(a0)         //

//
// If the call is from user mode, then use the continue system service to
// continue execution. Otherwise, restore the context directly since the
// current mode is kernel and threads can't be arbitrarily interrupted.
//

5:      bltz    ra,10f                  // if lt, kernel mode restore
        move    a1,zero                 // set test alert argument
        jal     ZwContinue              // continue execution

//
// Save the address of the context record and contuation address in
// registers t0 and t1. These registers are not restored.
//

10:     move    t0,a0                   // save context record address
        lw      t1,CxFir(t0)            // get continuation address

//
// Restore floating status and floating registers f0 - f31.
//

        .set    noreorder
        .set    noat
        lw      v0,CxFsr(t0)            // restore floating status
        ctc1    v0,fsr                  //
        ldc1    f0,CxFltF0(t0)          // restore floating registers f0 - f31
        ldc1    f2,CxFltF2(t0)          //
        ldc1    f4,CxFltF4(t0)          //
        ldc1    f6,CxFltF6(t0)          //
        ldc1    f8,CxFltF8(t0)          //
        ldc1    f10,CxFltF10(t0)        //
        ldc1    f12,CxFltF12(t0)        //
        ldc1    f14,CxFltF14(t0)        //
        ldc1    f16,CxFltF16(t0)        //
        ldc1    f18,CxFltF18(t0)        //
        ldc1    f20,CxFltF20(t0)        //
        ldc1    f22,CxFltF22(t0)        //
        ldc1    f24,CxFltF24(t0)        //
        ldc1    f26,CxFltF26(t0)        //
        ldc1    f28,CxFltF28(t0)        //
        ldc1    f30,CxFltF30(t0)        //

//
// Restore integer registers and continue execution.
//

        ld      v0,CxXIntLo(t0)         // restore multiply/divide registers
        ld      v1,CxXIntHi(t0)         //
        mtlo    v0                      //
        mthi    v1                      //
        ld      AT,CxXIntAt(t0)         // restore integer registers at - a3
        ld      v0,CxXIntV0(t0)         //
        ld      v1,CxXIntV1(t0)         //
        ld      a0,CxXIntA0(t0)         //
        ld      a1,CxXIntA1(t0)         //
        ld      a2,CxXIntA2(t0)         //
        ld      a3,CxXIntA3(t0)         //
        ld      t2,CxXIntT2(t0)         // restore integer registers t2 - t7
        ld      t3,CxXIntT3(t0)         //
        ld      t4,CxXIntT4(t0)         //
        ld      t5,CxXIntT5(t0)         //
        ld      t6,CxXIntT6(t0)         //
        ld      t7,CxXIntT7(t0)         //
        ld      s0,CxXIntS0(t0)         // restore integer registers s0 - s7
        ld      s1,CxXIntS1(t0)         //
        ld      s2,CxXIntS2(t0)         //
        ld      s3,CxXIntS3(t0)         //
        ld      s4,CxXIntS4(t0)         //
        ld      s5,CxXIntS5(t0)         //
        ld      s6,CxXIntS6(t0)         //
        ld      s7,CxXIntS7(t0)         //
        ld      t8,CxXIntT8(t0)         // restore integer registers t8 and t9
        ld      t9,CxXIntT9(t0)         //
        ld      s8,CxXIntS8(t0)         // restore integer register s8
        ld      gp,CxXIntGp(t0)         // restore integer register gp
        ld      sp,CxXIntSp(t0)         //
        j       t1                      // continue execution
        ld      ra,CxXIntRa(t0)         //
        .set    at
        .set    reorder

        .end    RtlpRestoreContext
