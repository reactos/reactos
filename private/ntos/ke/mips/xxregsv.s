//      TITLE("Register Save and Restore")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    xxregsv.s
//
// Abstract:
//
//    This module implements the code necessary to save and restore processor
//    registers during exception and interrupt processing.
//
// Author:
//
//    David N. Cutler (davec) 12-Aug-1990
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Save Volatile Floating Registers")
//++
//
// Routine Desription:
//
//    This routine is called to save the volatile floating registers.
//
//    N.B. This routine uses a special argument passing mechanism and destroys
//       no registers. It is assumed that floating register f0 is saved by the
//       caller.
//
// Arguments:
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiSaveVolatileFloatState)

        .set    noreorder
        .set    noat

#if defined(_EXTENDED_FLOAT)

        sdc1    f1,TrDblF1(s8)          // save odd floating registers
        sdc1    f3,TrDblF3(s8)          //
        sdc1    f5,TrDblF5(s8)          //
        sdc1    f7,TrDblF7(s8)          //
        sdc1    f9,TrDblF9(s8)          //
        sdc1    f11,TrDblF11(s8)        //
        sdc1    f13,TrDblF13(s8)        //
        sdc1    f15,TrDblF15(s8)        //
        sdc1    f17,TrDblF17(s8)        //
        sdc1    f19,TrDblF19(s8)        //
        sdc1    f21,TrDblF21(s8)        //
        sdc1    f23,TrDblF23(s8)        //
        sdc1    f25,TrDblF25(s8)        //
        sdc1    f27,TrDblF27(s8)        //
        sdc1    f29,TrDblF29(s8)        //
        sdc1    f31,TrDblF31(s8)        //

#endif

        sdc1    f2,TrFltF2(s8)          // save even floating registers
        sdc1    f4,TrFltF4(s8)          //
        sdc1    f6,TrFltF6(s8)          //
        sdc1    f8,TrFltF8(s8)          //
        sdc1    f10,TrFltF10(s8)        //
        sdc1    f12,TrFltF12(s8)        //
        sdc1    f14,TrFltF14(s8)        //
        sdc1    f16,TrFltF16(s8)        //
        j       ra                      // return
        sdc1    f18,TrFltF18(s8)        //
        .set    at
        .set    reorder

        .end    KiSaveVolatileFloatState)

        SBTTL("Restore Volatile Floating Registers")
//++
//
// Routine Desription:
//
//    This routine is called to restore the volatile floating registers.
//
//    N.B. This routine uses a special argument passing mechanism and destroys
//       no registers. It is assumed that floating register f0 is restored by
//       the caller.
//
// Arguments:
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRestoreVolatileFloatState)

        .set    noreorder
        .set    noat

#if defined(_EXTENDED_FLOAT)

        ldc1    f1,TrDblF1(s8)          // save odd floating registers
        ldc1    f3,TrDblF3(s8)          //
        ldc1    f5,TrDblF5(s8)          //
        ldc1    f7,TrDblF7(s8)          //
        ldc1    f9,TrDblF9(s8)          //
        ldc1    f11,TrDblF11(s8)        //
        ldc1    f13,TrDblF13(s8)        //
        ldc1    f15,TrDblF15(s8)        //
        ldc1    f17,TrDblF17(s8)        //
        ldc1    f19,TrDblF19(s8)        //
        ldc1    f21,TrDblF21(s8)        //
        ldc1    f23,TrDblF23(s8)        //
        ldc1    f25,TrDblF25(s8)        //
        ldc1    f27,TrDblF27(s8)        //
        ldc1    f29,TrDblF29(s8)        //
        ldc1    f31,TrDblF31(s8)        //

#endif

        ldc1    f2,TrFltF2(s8)          // restore floating registers f2 - f19
        ldc1    f4,TrFltF4(s8)          //
        ldc1    f6,TrFltF6(s8)          //
        ldc1    f8,TrFltF8(s8)          //
        ldc1    f10,TrFltF10(s8)        //
        ldc1    f12,TrFltF12(s8)        //
        ldc1    f14,TrFltF14(s8)        //
        ldc1    f16,TrFltF16(s8)        //
        j       ra                      // return
        ldc1    f18,TrFltF18(s8)        //
        .set    at
        .set    reorder

        .end    KiRestoreVolatileFloatState
