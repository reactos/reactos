/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//      TITLE("Large Integer Arithmetic")
//++
//
// Module Name:
//
//    largeint.s
//
// Abstract:
//
//    This module implements routines for performing extended integer
//    arithmetic.
//
// Author:
//
//    William K. Cheung (wcheung) 08-Feb-1996
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    09-Feb-1996    Updated to EAS 2.1
//
//--

#include "ksia64.h"

        .file    "largeint.s"


//++
//
// LARGE_INTEGER
// RtlConvertLongToLargeInteger (
//    IN LONG SignedInteger
//    )
//
// Routine Description:
//
//    This function converts a signed integer to a signed large integer
//    and returns the result.
//
// Arguments:
//
//    SignedInteger (a0) - Supplies the value to convert.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlConvertLongToLargeInteger)

        sxt4        v0 = a0                     // ensure canonical form
        LEAF_RETURN
        
        LEAF_EXIT(RtlConvertLongToLargeInteger)


//++
//
// LARGE_INTEGER
// RtlConvertUlongToLargeInteger (
//    IN ULONG UnsignedInteger
//    )
//
// Routine Description:
//
//    This function converts an unsigned integer to a signed large
//    integer and returns the result.
//
// Arguments:
//
//    UnsignedInteger (a0) - Supplies the value to convert.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlConvertUlongToLargeInteger)

        zxt4        v0 = a0                     // ensure canonical form
        LEAF_RETURN
        
        LEAF_EXIT(RtlConvertUlongToLargeInteger)

//++
//
// LARGE_INTEGER
// RtlLargeIntegerAdd (
//    IN LARGE_INTEGER Addend1,
//    IN LARGE_INTEGER Addend2
//    )
//
// Routine Description:
//
//    This function adds a signed large integer to a signed large integer and
//    returns the signed large integer result.
//
//    N.B. An overflow is possible, but no exception is generated.
//
// Arguments:
//
//    Addend1 (a0) - Supplies the first addend value.
//
//    Addend2 (a1) - Supplies the second addend value.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerAdd)

        add         v0 = a0, a1                 // add both quadword arguments
        LEAF_RETURN

        LEAF_EXIT(RtlLargeIntegerAdd)

//++
//
// LARGE_INTEGER
// RtlLargeIntegerArithmeticShift (
//    IN LARGE_INTEGER LargeInteger,
//    IN CCHAR ShiftCount
//    )
//
// Routine Description:
//
//    This function shifts a signed large integer right by an unsigned integer
//    modulo 64 and returns the shifted signed large integer result.
//
// Arguments:
//
//    LargeInteger (a0) - Supplies the large integer to be shifted.
//
//    ShiftCount (a1) - Supplies the right shift count.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerArithmeticShift)

        and         a1 = 0x3f, a1               // modulo 64 of shift count
        ;;
        shr         v0 = a0, a1                 // arithmetic shift the quadword right
        LEAF_RETURN

        LEAF_EXIT(RtlLargeIntegerArithmeticShift)


//++
//
// LARGE_INTEGER
// RtlLargeIntegerNegate (
//    IN LARGE_INTEGER Subtrahend
//    )
//
// Routine Description:
//
//    This function negates a signed large integer and returns the signed
//    large integer result.
//
//    N.B. An overflow is possible, but no exception is generated.
//
// Arguments:
//
//    Subtrahend (a0) - Supplies the subtrahend value.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerNegate)

        sub         v0 = zero, a0               // negate the quadword argument
        LEAF_RETURN

        LEAF_EXIT(RtlLargeIntegerNegate)

//++
//
// LARGE_INTEGER
// RtlLargeIntegerShiftLeft (
//    IN LARGE_INTEGER LargeInteger,
//    IN CCHAR ShiftCount
//    )
//
// Routine Description:
//
//    This function shifts a signed large integer left by an unsigned integer
//    modulo 64 and returns the shifted signed large integer result.
//
// Arguments:
//
//    LargeInteger (a0, a1) - Supplies the large integer to be shifted.
//
//    ShiftCount (a2) - Supplies the left shift count.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerShiftLeft)

        and         a1 = 0x3f, a1               // modulo 64 of shift count
        ;;
        shl         v0 = a0, a1                 // shift the quadword right
        LEAF_RETURN

        LEAF_EXIT(RtlLargeIntegerShiftLeft)

//++
//
// LARGE_INTEGER
// RtlLargeIntegerShiftRight (
//    IN LARGE_INTEGER LargeInteger,
//    IN CCHAR ShiftCount
//    )
//
// Routine Description:
//
//    This function shifts an unsigned large integer right by an unsigned
//    integer modulo 64 and returns the shifted unsigned large integer result.
//
// Arguments:
//
//    LargeInteger (a0) - Supplies the large integer to be shifted.
//
//    ShiftCount (a1) - Supplies the right shift count.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerShiftRight)
 
        and         a1 = 0x3f, a1               // modulo 64 of shift count
        ;;
        shr.u       v0 = a0, a1                 // logical shift the quadword right
        LEAF_RETURN

        LEAF_EXIT(RtlLargeIntegerShiftRight)

//++
//
// LARGE_INTEGER
// RtlLargeIntegerSubtract (
//    IN LARGE_INTEGER Minuend,
//    IN LARGE_INTEGER Subtrahend
//    )
//
// Routine Description:
//
//    This function subtracts a signed large integer from a signed large
//    integer and returns the signed large integer result.
//
//    N.B. An overflow is possible, but no exception is generated.
//
// Arguments:
//
//    Minuend (a0) - Supplies the minuend value.
//
//    Subtrahend (a1) - Supplies the subtrahend value.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerSubtract)

        sub         v0 = a0, a1                 // subtract the quadword args
        LEAF_RETURN

        LEAF_EXIT(RtlLargeIntegerSubtract)


//++
//
// LARGE_INTEGER
// RtlExtendedMagicDivide (
//    IN LARGE_INTEGER Dividend,
//    IN LARGE_INTEGER MagicDivisor,
//    IN CCHAR ShiftCount
//    )
//
// Routine Description:
//
//    This function divides a signed large integer by an unsigned large integer
//    and returns the signed large integer result. The division is performed
//    using reciprocal multiplication of a signed large integer value by an
//    unsigned large integer fraction which represents the most significant
//    64-bits of the reciprocal divisor rounded up in its least significant bit
//    and normalized with respect to bit 63. A shift count is also provided
//    which is used to truncate the fractional bits from the result value.
//
// Arguments:
//
//    Dividend (a0) - Supplies the dividend value.
//
//    MagicDivisor (a1) - Supplies the magic divisor value which
//      is a 64-bit multiplicative reciprocal.
//
//    Shiftcount (a2) - Supplies the right shift adjustment value.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlExtendedMagicDivide)

        cmp.gt      pt0, pt1 = r0, a0
        ;;
(pt0)   sub         a0 = r0, a0
        ;;
    
        setf.sig    ft0 = a0
        setf.sig    ft1 = a1
        ;;
        zxt1        a2 = a2
        xma.hu      ft2 = ft0, ft1, f0
        ;;
        getf.sig    v0 = ft2
        ;;
        shr         v0 = v0, a2
        ;;

(pt0)   sub         v0 = r0, v0
        LEAF_RETURN

        LEAF_EXIT(RtlExtendedMagicDivide)
