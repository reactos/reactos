//      TITLE("Floating Point Tests")
//++
//
// Copyright (c) 1991  Microsoft Corporation
//
// Module Name:
//
//    flptx.s
//
// Abstract:
//
//    This module implements the floating point tests.
//
// Author:
//
//    David N. Cutler (davec) 20-Jun-1991
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Add Double Test")
//++
//
// ULONG
// AddDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Addend1,
//    IN PULARGE_INTEGER Addend2,
//    OUT PULARGE_INTEGER Result
//    );
//
// Routine Description:
//
//    The following routine implements the add double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer the first addend value.
//    a2 - Supplies a pointer to the second addend value.
//    a3 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(AddDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first addend value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second addend value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first operand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second operand value
        mtc1    t3,f3                   //
        add.d   f4,f0,f2                // form sum
        mfc1    v0,f4                   // get result value
        mfc1    v1,f5                   //
        sw      v0,0(a3)                // store result value
        sw      v1,4(a3)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    AddDouble

        SBTTL("Divide Double Test")
//++
//
// ULONG
// DivideDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Dividend,
//    IN PULARGE_INTEGER Divisor,
//    OUT PULARGE_INTEGER Result
//    );
//
// Routine Description:
//
//    The following routine implements the divide double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer ot the dividend value.
//    a2 - Supplies a pointer ot the divisor value.
//    a3 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(DivideDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get dividend value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get divisor value
        lw      t3,4(a2)                //
        mtc1    t0,f10                  // set dividend value
        mtc1    t1,f11                  //
        mtc1    t2,f12                  // set divisor value
        mtc1    t3,f13                  //
        div.d   f14,f10,f12             // form quotient
        mfc1    v0,f14                  // get result value
        mfc1    v1,f15                  //
        sw      v0,0(a3)                // store result value
        sw      v1,4(a3)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    DivideDouble

        SBTTL("Multiply Double Test")
//++
//
// ULONG
// MultiplySingle (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Multiplicand,
//    IN PULARGE_INTEGER Multiplier,
//    OUT PULARGE_INTEGER Result
//    );
//
// Routine Description:
//
//    The following routine implements the multiply double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the multiplicand value.
//    a2 - Supplies a pointer to the multipler value.
//    a3 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(MultiplyDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get multiplicand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get multiplier value
        lw      t3,4(a2)                //
        mtc1    t0,f18                  // set multiplicand value
        mtc1    t1,f19                  //
        mtc1    t2,f24                  // set multiplier value
        mtc1    t3,f25                  //
        mul.d   f10,f18,f24             // form product
        mfc1    v0,f10                  // get result value
        mfc1    v1,f11                  //
        sw      v0,0(a3)                // store result value
        sw      v1,4(a3)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    MultiplyDouble

        SBTTL("Subtract Double Test")
//++
//
// ULONG
// SubtractDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Subtrahend,
//    IN PULARGE_INTEGER Minuend,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the subtract double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the subtrahend value.
//    a2 - Supplies a pointer to the minuend value.
//    a3 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(SubtractDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get subtrahend value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get minuend value
        lw      t3,4(a2)                //
        mtc1    t0,f20                  // set subtrahend value
        mtc1    t1,f21                  //
        mtc1    t2,f22                  // set minuend value
        mtc1    t3,f23                  //
        sub.d   f8,f20,f22              // form difference
        mfc1    v0,f8                   // get result value
        mfc1    v1,f9                   //
        sw      v0,0(a3)                // store result value
        sw      v1,4(a3)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    SubtractDouble

        SBTTL("Add Single Test")
//++
//
// ULONG
// AddSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Addend1,
//    IN ULONG Addend2,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the add single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the first addend value.
//    a2 - Supplies the second addend value.
//    a3 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(AddSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        add.s   f4,f0,f2                // form sum
        mfc1    v0,f4                   // get result value
        sw      v0,0(a3)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    AddSingle

        SBTTL("Divide Single Test")
//++
//
// ULONG
// DivideSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Dividend,
//    IN ULONG Divisor,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the divide single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the dividend value.
//    a2 - Supplies the divisor value.
//    a3 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(DivideSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f10                  // set dividend value
        mtc1    a2,f12                  // set divisor value
        div.s   f14,f10,f12             // form quotient
        mfc1    v0,f14                  // get result value
        sw      v0,0(a3)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    DivideSingle

        SBTTL("Multiply Single Test")
//++
//
// ULONG
// MultiplySingle (
//    IN ULONG RoundingMode,
//    IN ULONG Multiplicand,
//    IN ULONG Multiplier,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the multiply single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the multiplicand value.
//    a2 - Supplies the multipler value.
//    a3 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(MultiplySingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f18                  // set multiplicand value
        mtc1    a2,f24                  // set multiplier value
        mul.s   f10,f18,f24             // form product
        mfc1    v0,f10                  // get result value
        sw      v0,0(a3)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    MultiplySingle

        SBTTL("Subtract Single Test")
//++
//
// ULONG
// SubtractSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Subtrahend,
//    IN ULONG Minuend,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the subtract single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the subtrahend value.
//    a2 - Supplies the minuend value.
//    a3 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(SubtractSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f20                  // set subtrahend value
        mtc1    a2,f22                  // set minuend value
        sub.s   f8,f20,f22              // form difference
        mfc1    v0,f8                   // get result value
        sw      v0,0(a3)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    SubtractSingle

        SBTTL("Convert To Double From Single")
//++
//
// ULONG
// ConvertToDoubleFromSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Source,
//    OUT PULARGE_INTEGER Result
//    );
//
// Routine Description:
//
//    The following routine implements the convert to double from
//    single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(ConvertToDoubleFromSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set the source value
        cvt.d.s f2,f0                   // convert to double from single
        mfc1    v0,f2                   // get result value
        mfc1    v1,f3                   //
        sw      v0,0(a2)                // store result value
        sw      v1,4(a2)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    ConvertToDoubleFromSingle

        SBTTL("Convert To Longword From Double")
//++
//
// ULONG
// ConvertToLongwordFromDouble (
//    IN ULONG RoundingMode,
//    IN ULARGE_INTEGER Source,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the convert to longword from
//    double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(ConvertToLongwordFromDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get the source value
        lw      t1,4(a1)                //
        mtc1    t0,f0                   // set the source value
        mtc1    t1,f1                   //
        cvt.w.d f2,f0                   // convert to longword from double
        mfc1    v0,f2                   // get result value
        sw      v0,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    ConvertToLongwordFromDouble

        SBTTL("Convert To Longword From Single")
//++
//
// ULONG
// ConvertToLongwordFromSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Source,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the convert to longword from
//    single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(ConvertToLongwordFromSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set the source value
        cvt.w.s f2,f0                   // convert to longword from double
        mfc1    v0,f2                   // get result value
        sw      v0,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    ConvertToLongwordFromSingle

        SBTTL("Convert To Single From Double")
//++
//
// ULONG
// ConvertToSingleFromDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Source,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the convert to single from
//    double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(ConvertToSingleFromDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get the source value
        lw      t1,4(a1)                //
        mtc1    t0,f0                   // set the source value
        mtc1    t1,f1                   //
        cvt.s.d f2,f0                   // convert to single from double
        mfc1    v0,f2                   // get result value
        sw      v0,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    ConvertToSingleFromDouble

        SBTTL("Compare Double Test")
//++
//
// ULONG
// CompareXyDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Comparand1,
//    IN PULARGE_INTEGER Comparand2
//    );
//
// Routine Description:
//
//    The following routines implements the compare double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the first comparand value.
//    a2 - Supplies a pointer to the second comparand value.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(CompareFDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.f.d   f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareFDouble

        LEAF_ENTRY(CompareUnDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.un.d  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareUnDouble

        LEAF_ENTRY(CompareEqDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.eq.d  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareEqDouble

        LEAF_ENTRY(CompareUeqDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.ueq.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareUeqDouble

        LEAF_ENTRY(CompareOltDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.olt.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareOltDouble

        LEAF_ENTRY(CompareUltDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.ult.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareUltDouble

        LEAF_ENTRY(CompareOleDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.ole.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareOleDouble

        LEAF_ENTRY(CompareUleDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.ule.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareUleDouble

        LEAF_ENTRY(CompareSfDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.sf.d  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareSfDouble

        LEAF_ENTRY(CompareNgleDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.ngle.d f0,f2                  // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareNgleDouble

        LEAF_ENTRY(CompareSeqDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.seq.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareSeqDouble

        LEAF_ENTRY(CompareNglDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.ngl.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareNglDouble

        LEAF_ENTRY(CompareLtDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.lt.d  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareLtDouble

        LEAF_ENTRY(CompareNgeDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.nge.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareNgeDouble

        LEAF_ENTRY(CompareLeDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.le.d  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareLeDouble

        LEAF_ENTRY(CompareNgtDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get first comparand value
        lw      t1,4(a1)                //
        lw      t2,0(a2)                // get second comparand value
        lw      t3,4(a2)                //
        mtc1    t0,f0                   // set first comparand value
        mtc1    t1,f1                   //
        mtc1    t2,f2                   // set second comparand value
        mtc1    t3,f3                   //
        c.ngt.d f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareNgtDouble

        SBTTL("Compare Single Test")
//++
//
// ULONG
// CompareXySingle (
//    IN ULONG RoundingMode,
//    IN ULONG Comparand1,
//    IN ULONG Comparand2
//    );
//
// Routine Description:
//
//    The following routine implements the compare single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the first comparand value.
//    a2 - Supplies the second comparand value.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(CompareFSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.f.s   f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareFSingle

        LEAF_ENTRY(CompareUnSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.un.s  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareUnSingle

        LEAF_ENTRY(CompareEqSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.eq.s  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareEqSingle

        LEAF_ENTRY(CompareUeqSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.ueq.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareUeqSingle

        LEAF_ENTRY(CompareOltSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.olt.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareOltSingle

        LEAF_ENTRY(CompareUltSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.ult.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareUltSingle

        LEAF_ENTRY(CompareOleSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.ole.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareOleSingle

        LEAF_ENTRY(CompareUleSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.ule.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareUleSingle

        LEAF_ENTRY(CompareSfSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.sf.s  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareSfSingle

        LEAF_ENTRY(CompareNgleSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.ngle.s f0,f2                  // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareNgleSingle

        LEAF_ENTRY(CompareSeqSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.seq.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareSeqSingle

        LEAF_ENTRY(CompareNglSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.ngl.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareNglSingle

        LEAF_ENTRY(CompareLtSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.lt.s  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareLtSingle

        LEAF_ENTRY(CompareNgeSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.nge.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareNgeSingle

        LEAF_ENTRY(CompareLeSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.le.s  f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareLeSingle

        LEAF_ENTRY(CompareNgtSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mtc1    a2,f2                   // set second operand value
        c.ngt.s f0,f2                   // compare operands
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    CompareNgtSingle

        SBTTL("Absolute Double Test")
//++
//
// ULONG
// AbsoluteDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Operand,
//    OUT PULARGE_INTEGER Result
//    );
//
// Routine Description:
//
//    The following routine implements the absolute double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the operand value.
//    a2 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(AbsoluteDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get double operand value
        lw      t1,4(a1)                //
        mtc1    t0,f0                   // set double operand value
        mtc1    t1,f1                   //
        abs.d   f4,f0                   // form absolute value
        mfc1    v0,f4                   // get double result value
        mfc1    v1,f5                   //
        sw      v0,0(a2)                // store double result value
        sw      v1,4(a2)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    AbsoluteDouble

        SBTTL("Move Double Test")
//++
//
// ULONG
// MoveDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Operand,
//    OUT PULARGE_INTEGER Result
//    );
//
// Routine Description:
//
//    The following routine implements the move double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the operand value.
//    a2 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(MoveDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get double operand value
        lw      t1,4(a1)                //
        mtc1    t0,f0                   // set double operand value
        mtc1    t1,f1                   //
        mov.d   f4,f0                   // move value
        mfc1    v0,f4                   // get double result value
        mfc1    v1,f5                   //
        sw      v0,0(a2)                // store double result value
        sw      v1,4(a2)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    MoveDouble

        SBTTL("Negate Double Test")
//++
//
// ULONG
// NegateDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Operand,
//    OUT PULARGE_INTEGER Result
//    );
//
// Routine Description:
//
//    The following routine implements the negate double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the operand value.
//    a2 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(NegateDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get double operand value
        lw      t1,4(a1)                //
        mtc1    t0,f0                   // set double operand value
        mtc1    t1,f1                   //
        neg.d   f4,f0                   // form negative value
        mfc1    v0,f4                   // get double result value
        mfc1    v1,f5                   //
        sw      v0,0(a2)                // store double result value
        sw      v1,4(a2)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    NegateDouble

        SBTTL("Absolute Single Test")
//++
//
// ULONG
// AbsoluteSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Operand,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the absolute single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the operand value.
//    a2 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(AbsoluteSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        abs.s   f4,f0                   // form absolute value
        mfc1    v0,f4                   // get result value
        sw      v0,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    AbsoluteSingle

        SBTTL("Move Single Test")
//++
//
// ULONG
// MoveSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Operand,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the move single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the operand value.
//    a2 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(MoveSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        mov.s   f4,f0                   // move value
        mfc1    v0,f4                   // get result value
        sw      v0,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    MoveSingle

        SBTTL("Negate Single Test")
//++
//
// ULONG
// NegateSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Operand,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the negate single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the operand value.
//    a2 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(NegateSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set first operand value
        neg.s   f4,f0                   // form negative value
        mfc1    v0,f4                   // get result value
        sw      v0,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    NegateSingle

        SBTTL("Round To Longword From Double")
//++
//
// ULONG
// RoundToLongwordFromDouble (
//    IN ULONG RoundingMode,
//    IN ULARGE_INTEGER Source,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the round to longword from double
//    test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(RoundToLongwordFromDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lw      t0,0(a1)                // get the source value
        lw      t1,4(a1)                //
        mtc1    t0,f0                   // set the source value
        mtc1    t1,f1                   //
        round.w.d f2,f0                 // convert to longword from double
        mfc1    v0,f2                   // get result value
        sw      v0,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    RoundToLongwordFromDouble

        SBTTL("Round To Longword From Single")
//++
//
// ULONG
// RoundToLongwordFromSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Source,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the round to longword from single
//    test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(RoundToLongwordFromSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set the source value
        round.w.s f2,f0                 // convert to longword from double
        mfc1    v0,f2                   // get result value
        sw      v0,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    RoundToLongwordFromSingle

        SBTTL("Truncate To Longword From Double")
//++
//
// ULONG
// TruncateToLongwordFromDouble (
//    IN ULONG RoundingMode,
//    IN ULARGE_INTEGER Source,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the truncate to longword from double
//    test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(TruncateToLongwordFromDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lwc1    f0,0(a1)                // get the source value
        lwc1    f1,4(a1)                //
        trunc.w.d f2,f0                 // convert to longword from double
        swc1    f2,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    TruncateToLongwordFromDouble

        SBTTL("Truncate To Longword From Single")
//++
//
// ULONG
// TruncateToLongwordFromSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Source,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the truncate to longword from single
//    test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(TruncateToLongwordFromSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set the source value
        trunc.w.s f2,f0                 // convert to longword from double
        swc1    f2,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    TruncateToLongwordFromSingle

        SBTTL("Square Root Double Test")
//++
//
// ULONG
// SquareRootDouble (
//    IN ULONG RoundingMode,
//    IN PULARGE_INTEGER Operand,
//    OUT PULARGE_INTEGER Result
//    );
//
// Routine Description:
//
//    The following routine implements the negate double test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies a pointer to the operand value.
//    a2 - Supplies a pointer to a variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(SquareRootDouble)

        ctc1    a0,fsr                  // set rounding mode and enables
        lwc1    f0,0(a1)                // get double operand value
        lwc1    f1,4(a1)                //
        sqrt.d  f4,f0                   // form square root double
        swc1    f4,0(a2)                // store double result value
        swc1    f5,4(a2)                //
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    SquareRootDouble

        SBTTL("Square Root Single")
//++
//
// ULONG
// SquareRootSingle (
//    IN ULONG RoundingMode,
//    IN ULONG Source,
//    OUT PULONG Result
//    );
//
// Routine Description:
//
//    The following routine implements the square root single test.
//
// Arguments:
//
//    a0 - Supplies the rounding mode.
//    a1 - Supplies the source operand value.
//    a2 - Supplies a pointer to the variable that receives the result.
//
// Return Value:
//
//    The resultant floating status is returned as the function value.
//
//--

        LEAF_ENTRY(SquareRootSingle)

        ctc1    a0,fsr                  // set rounding mode and enables
        mtc1    a1,f0                   // set the source value
        sqrt.s  f2,f0                   // compute square root single
        swc1    f2,0(a2)                // store result value
        cfc1    v0,fsr                  // get floating status
        j       ra                      // return

        .end    SquareRootSingle
