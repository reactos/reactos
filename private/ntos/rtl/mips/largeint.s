//      TITLE("Large Integer Arithmetic")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    largeint.s
//
// Abstract:
//
//    This module implements routines for performing extended integer
//    arithmtic.
//
// Author:
//
//    David N. Cutler (davec) 18-Apr-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Large Integer Add")
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
// Arguments:
//
//    Addend1 (a2, a3) - Supplies the first addend value.
//
//    Addend2 (4 * 4(sp), 4 * 5(sp)) - Supplies the second addend value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerAdd)

        lw      t0,4 * 4(sp)            // get low part of addend2 value
        lw      t1,4 * 5(sp)            // get high part of addend2 value
        addu    t0,t0,a2                // add low parts of large integer
        addu    t1,t1,a3                // add high parts of large integer
        sltu    t2,t0,a2                // generate carry from low part
        addu    t1,t1,t2                // add carry to high part
        sw      t0,0(a0)                // store low part of result
        sw      t1,4(a0)                // store high part of result
        move    v0,a0                   // set function return register
        j       ra                      // return

        .end    RtlLargeIntegerAdd

        SBTTL("Convert Long to Large Integer")
//++
//
// LARGE_INTEGER
// RtlConvertLongToLargeInteger (
//     IN LONG SignedInteger
//     )
//
// Routine Description:
//
//     This function converts the a signed integer to a signed large integer
//     and returns the result.
//
// Arguments:
//
//     SignedInteger (a1) - Supplies the value to convert.
//
// Return Value:
//
//     The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlConvertLongToLargeInteger)

        sra     a2,a1,31                // compute high part of result
        sw      a1,0(a0)                // store low part of result
        sw      a2,4(a0)                // store high part of result
        move    v0,a0                   // set function return register
        j       ra                      // return

        .end    RtlConvertLongToLargeInteger

        SBTTL("Convert Ulong to Large Integer")
//++
//
// LARGE_INTEGER
// RtlConvertUlongToLargeInteger (
//     IN LONG UnsignedInteger
//     )
//
// Routine Description:
//
//     This function converts the an unsigned integer to a signed large
//     integer and returns the result.
//
// Arguments:
//
//     UnsignedInteger (a1) - Supplies the value to convert.
//
// Return Value:
//
//     The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlConvertUlongToLargeInteger)

        sw      a1,0(a0)                // store low part of result
        sw      zero,4(a0)              // store high part of result
        move    v0,a0                   // set function return register
        j       ra                      // return

        .end    RtlConvertUlongToLargeInteger

        SBTTL("Enlarged Signed Integer Multiply")
//++
//
// LARGE_INTEGER
// RtlEnlargedIntegerMultiply (
//    IN LONG Multiplicand,
//    IN LONG Multiplier
//    )
//
// Routine Description:
//
//    This function multiplies a signed integer by an signed integer and
//    returns a signed large integer result.
//
// Arguments:
//
//    Multiplicand (a1) - Supplies the multiplicand value.
//
//    Multiplier (a2) - Supplies the multiplier value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlEnlargedIntegerMultiply)

        mult    a1,a2                  // multiply longword value
        mflo    t0                     // get low 32-bits of result
        mfhi    t1                     // get high 32-bits of result
        sw      t0,0(a0)               // set low part of result
        sw      t1,4(a0)               // set high part of result
        move    v0,a0                  // set function return register
        j       ra                     // return

        .end    RtlEnlargedIntegerMultiply)

        SBTTL("Enlarged Unsigned Integer Multiply")
//++
//
// LARGE_INTEGER
// RtlEnlargedUnsignedMultiply (
//    IN ULONG Multiplicand,
//    IN ULONG Multiplier
//    )
//
// Routine Description:
//
//    This function multiplies an unsigned integer by an unsigned integer
//    and returns a signed large integer result.
//
// Arguments:
//
//    Multiplicand (a1) - Supplies the multiplicand value.
//
//    Multiplier (a2) - Supplies the multiplier value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlEnlargedUnsignedMultiply)

        multu   a1,a2                  // multiply longword value
        mflo    t0                     // get low 32-bits of result
        mfhi    t1                     // get high 32-bits of result
        sw      t0,0(a0)               // set low part of result
        sw      t1,4(a0)               // set high part of result
        move    v0,a0                  // set function return register
        j       ra                     // return

        .end    RtlEnlargedUnsignedMultiply)

        SBTTL("Enlarged Unsigned Divide")
//++
//
// ULONG
// RtlEnlargedUnsignedDivide (
//    IN ULARGE_INTEGER Dividend,
//    IN ULONG Divisor,
//    IN PULONG Remainder.
//    )
//
// Routine Description:
//
//    This function divides an unsigned large integer by an unsigned long
//    and returns the resultant quotient and optionally the remainder.
//
//    N.B. It is assumed that no overflow will occur.
//
// Arguments:
//
//    Dividend (a0, a1) - Supplies the dividend value.
//
//    Divisor (a2) - Supplies the divisor value.
//
//    Remainder (a3) - Supplies an optional pointer to a variable that
//        receives the remainder.
//
// Return Value:
//
//    The unsigned long integer quotient is returned as the function value.
//
//--

        LEAF_ENTRY(RtlEnlargedUnsignedDivide)

        sltu    v1,a1,a2                // check if overflow will occur
        beq     zero,a2,20f             // if eq, attempted division by zero
        dsll    a1,a1,32                // left justify hihg part of dividend
        beq     zero,v1,30f             // if eq, overflow will occur
        dsll    a0,a0,32                // zero extend low part of dividend
        dsrl    a0,a0,32                //
        or      a0,a0,a1                // merge high and low part of dividend
        ddivu   a0,a2                   // compute quotient value
        mflo    v0                      // set quotient value
        beq     zero,a3,10f             // if eq, remainder not requested
        mfhi    a0                      // load remainder
        sw      a0,0(a3)                // store longword remainder
10:     j       ra                      // return


20:     break   DIVIDE_BY_ZERO_BREAKPOINT // attempted division by zero
        j       ra                      //

30:     break   DIVIDE_OVERFLOW_BREAKPOINT // division value overflows result
        j       ra                      //

        .end    RtlEnlargedUnsignedDivide

        SBTTL("Extended Large Integer Divide")
//++
//
// LARGE_INTEGER
// RtlExtendedLargeIntegerDivide (
//    IN LARGE_INTEGER Dividend,
//    IN ULONG Divisor,
//    IN PULONG Remainder.
//    )
//
// Routine Description:
//
//    This function divides an unsigned large integer by an unsigned long
//    and returns the resultant quotient and optionally the remainder.
//
// Arguments:
//
//    Dividend (a2, a3) - Supplies the dividend value.
//
//    Divisor (4 * 4(sp)) - Supplies the divisor value.
//
//    Remainder (4 * 5(sp)- Supplies an optional pointer to a variable
//      that receives the remainder.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlExtendedLargeIntegerDivide)

        .set    noreorder
        .set    noat
        move    v0,a0                   // set function return register
        lw      a1,4 * 4(sp)            // get divisor value
        lw      t0,4 * 5(sp)            // get address to store remainder
        beq     zero,a1,30f             // if eq, attempted division by zero
        li      t1,63                   // set loop count
        move    t2,zero                 // clear partial remainder
10:     sra     t3,t2,31                // replicate partial remainder high bit
        sll     t2,t2,1                 // shift next dividend bit
        srl     t4,a3,31                // into the partial remainder
        or      t2,t2,t4                //
        sll     a3,a3,1                 // double left shift dividend
        srl     t4,a2,31                //
        or      a3,a3,t4                //
        sltu    t4,t2,a1                // check if partial remainder less
        subu    t4,t4,1                 // convert to 0 or -1
        or      t4,t4,t3                // merge with partial remainder high bit
        and     t5,t4,a1                // select divisor or 0
        sll     a2,a2,1                 //
        subu    a2,a2,t4                // merge quotient bit
        subu    t2,t2,t5                // subtract out divisor
        bne     zero,t1,10b             // if ne, more iterations to go
        subu    t1,t1,1                 // decrement iteration count
        beq     zero,t0,20f             // if eq, remainder not requested
        sw      a2,0(a0)                // store low part of quotient
        sw      t2,0(t0)                // store longword remainder
20:     j       ra                      // return
        sw      a3,4(a0)                // store high part of quotient
        .set    at
        .set    reorder

30:     break   DIVIDE_BY_ZERO_BREAKPOINT // attempted division by zero
        j       ra                      //

        .end    RtlExtendedLargeIntegerDivide

        SBTTL("Extended Magic Divide")
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
//    Dividend (a2, a3) - Supplies the dividend value.
//
//    MagicDivisor (4 * 4(sp), 4 * 5(sp)) - Supplies the magic divisor value
//       which is a 64-bit multiplicative reciprocal.
//
//    Shiftcount (4 * 6(sp)) - Supplies the right shift adjustment value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlExtendedMagicDivide)

        move    t0,a2                   // assume dividend is positive
        move    t1,a3                   //
        bgez    a3,10f                  // if gez, positive dividend
        subu    t0,zero,t0              // negate low part of dividend
        subu    t1,zero,t1              // negate high part of dividend
        sltu    t2,zero,t0              // set borrow from high part
        subu    t1,t1,t2                // subtract out out borrow
10:     lw      a1,4 * 4(sp)            // get low part of magic dividor
        lw      t2,4 * 5(sp)            // get high part of magic divisor
        lbu     v0,4 * 6(sp)            // get shift count

//
// Compute low 32-bits of dividend times low 32-bits of divisor.
//

        multu   t0,a1                   //
        mfhi    t3                      // save high 32-bits of product

//
// Compute low 32-bits of dividend time high 32-bits of divisor.
//

        multu   t0,t2                   //
        mflo    t4                      // save low 32-bits of product
        mfhi    t5                      // save high 32-bits of product

//
// Compute high 32-bits of dividend times low 32-bits of divisor.
//

        multu   t1,a1                   //
        mflo    t6                      // save loow 32-bits of product
        mfhi    t7                      // save high 32-bits of product

//
// Compute high 32-bits of dividend times high 32-bits of divisor.
//

        multu   t1,t2                   //
        mflo    t8                      // save low 32-bits of product
        mfhi    t9                      // save high 32-bits of product

//
// Add partial results to form high 64-bits of result.
//

        addu    t0,t3,t4                //
        sltu    t1,t0,t4                // generate carry
        addu    t0,t0,t6                //
        sltu    t2,t0,t6                // generate carry
        addu    t2,t1,t2                // combine carries
        addu    t1,t2,t5                //
        sltu    t2,t1,t5                // generate carry
        addu    t1,t1,t7                //
        sltu    t3,t1,t7                // generate carry
        addu    t2,t2,t3                // combine carries
        addu    t1,t1,t8                //
        sltu    t3,t1,t8                // generate carry
        addu    t2,t2,t3                // combine carries
        addu    t2,t2,t9                //

//
// Right shift the result by the specified shift count and negate result
// if necessary.
//

        li      v1,32                   // compute left shift count
        subu    v1,v1,v0                //
        bgtz    v1,20f                  // if gtz, shift less that 32-bits

//
// Shift count is greater than or equal 32 bits - high half of result is zero,
// low half is the high half shifted right by remaining count.
//

        move    t1,zero                 // set high half of result
        srl     t0,t2,v0                // set low half of result
        b       30f                     //

//
// Shift count is less than 32-bits - high half of result is the high half
// of product shifted right by count, low half of result is the shifted out
// bits of the high half combined with the rigth shifted low half of the
// product.
//

20:     srl     t0,t1,v0                // shift low half right count bits
        srl     t1,t2,v0                // shift high half right count bits
        beq     zero,v0,30f             // if eq, no more shifts necessary
        sll     t2,t2,v1                // isolate shifted out bits of high half
        or      t0,t0,t2                // combine bits for low half of result

//
// Negate result if neccessary.
//

30:     bgez    a3,40f                  // if gez, positive result
        subu    t0,zero,t0              // negate low half of result
        subu    t1,zero,t1              // negate high half of result
        beq     zero,t0,40f             // if eq, negation complete
        subu    t1,t1,1                 // convert high part to ones complement
40:     sw      t0,0(a0)                // store low half of result
        sw      t1,4(a0)                // store high half of result
        move    v0,a0                   // set function return register
        j       ra                      // return

        .end    RtlExtendedMagicDivide

        SBTTL("Large Integer Divide")
//++
//
// LARGE_INTEGER
// RtlLargeIntegerDivide (
//    IN LARGE_INTEGER Dividend,
//    IN LARGE_INTEGER Divisor,
//    IN PLARGE_INTEGER Remainder.
//    )
//
// Routine Description:
//
//    This function divides an unsigned large integer by an unsigned
//    large and returns the resultant quotient and optionally the remainder.
//
// Arguments:
//
//    Dividend (a2, a3) - Supplies the dividend value.
//
//    Divisor (4 * 4(sp), 4 * 5(sp)) - Supplies the divisor value.
//
//    Remainder (4 * 6(sp)- Supplies an optional pointer to a variable
//      that receives the remainder.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerDivide)

        .set    noreorder
        .set    noat
        move    v0,a0                   // set function return register
        lw      a1,4 * 4(sp)            // get low part of divisor
        lw      t0,4 * 5(sp)            // get high part of divisor
        lw      t1,4 * 6(sp)            // get address to store remainder
        or      v1,t0,a1                // combine low and high parts
        beq     zero,v1,60f             // if eq, attempted division by zero
        li      t2,63                   // set loop count
        move    t3,zero                 // clear partial remainder
        move    t4,zero                 //
10:     sll     t4,t4,1                 // shift next dividend bit
        srl     t5,t3,31                // into the partial remainder
        or      t4,t4,t5                //
        sll     t3,t3,1                 //
        srl     t5,a3,31                //
        or      t3,t3,t5                //
        sll     a3,a3,1                 // double left shift dividend
        srl     t5,a2,31                //
        or      a3,a3,t5                //
        sltu    t5,t4,t0                // check if partial remainder less
        beq     zero,t5,20f             // if eq, partial remainder not less
        sll     a2,a2,1                 //
        bne     zero,t2,10b             // if ne, more iterations to go
        subu    t2,t2,1                 // decrement iteration count
        beq     zero,t1,50f             // if eq, remainder not requested
        sw      a2,0(a0)                // store low part of quotient
        sw      t3,0(t1)                // store large integer remainder
        sw      t4,4(t1)                //
        j       ra                      // return
        sw      a3,4(a0)                // store high part of quotient

20:     bne     t0,t4,30f               // if ne, partial remainder greater
        sltu    t5,t3,a1                // check is partial remainder less
        bne     zero,t5,40f             // if ne, partial remainder less
        nop                             //
30:     or      a2,a2,1                 // merge quotient bit
        subu    t4,t4,t0                // subtract out divisor high part
        sltu    t5,t3,a1                // set borrow from high part
        subu    t4,t4,t5                // subtract borrow from high part
        subu    t3,t3,a1                // subtract out divisor low
40:     bne     zero,t2,10b             // if ne, more iterations to go
        subu    t2,t2,1                 // decrement iteration count
        beq     zero,t1,50f             // if eq, remainder not requested
        sw      a2,0(a0)                // store low part of quotient
        sw      t3,0(t1)                // store large integer remainder
        sw      t4,4(t1)                //
50:     j       ra                      // return
        sw      a3,4(a0)                // store high part of quotient
        .set    at
        .set    reorder

60:     break   DIVIDE_BY_ZERO_BREAKPOINT // attempted division by zero
        j       ra                      //

        .end    RtlLargeIntegerDivide

        SBTTL("Extended Integer Multiply")
//++
//
// LARGE_INTEGER
// RtlExtendedIntegerMultiply (
//    IN LARGE_INTEGER Multiplicand,
//    IN LONG Multiplier
//    )
//
// Routine Description:
//
//    This function multiplies a signed large integer by a signed integer and
//    returns the signed large integer result.
//
// Arguments:
//
//    Multiplicand (a2, a3) - Supplies the multiplicand value.
//
//    Multiplier (4 * 4(sp)) - Supplies the multiplier value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlExtendedIntegerMultiply)

        lw      a1,4 * 4(sp)            // get multiplier value
        xor     t9,a1,a3                // compute sign of result
        move    t0,a1                   // assume multiplier positive
        bgez    a1,10f                  // if gez, positive multiplier
        subu    t0,zero,t0              // negate multiplier
10:     move    t1,a2                   // assume multiplicand positive
        move    t2,a3                   //
        bgez    a3,20f                  // if gez, positive multiplicand
        subu    t1,zero,t1              // negate multiplicand
        subu    t2,zero,t2              //
        sltu    t3,zero,t1              // compute borrow from high part
        subu    t2,t2,t3                // subtract out borrow

//
// Compute low 32-bits of multiplier times the low 32-bit of multiplicand.
//

20:     multu   t0,t1                   //
        mflo    t4                      // save low 32-bits of product
        mfhi    t5                      // save high 32-bits of product

//
// Compute low 32-bits of multiplier times the high 32-bits of multiplicand.
//

        multu   t0,t2                   //
        mflo    t6                      // save low 32-bits of product
        mfhi    t7                      // save high 32-bits of product

//
// Add partial results to form high 64-bits of result.
//

        addu    t5,t5,t6                //
        sltu    t3,t5,t6                // generate carry
        addu    t6,t3,t7                //

//
// Negate result if neccessary.
//

        bgez    t9,40f                  // if gez, positive result
        subu    t4,zero,t4              // negate low half of result
        subu    t5,zero,t5              // negate high half of result
        subu    t6,zero,t6              // negate extended part of result
        beq     zero,t4,30f             // if eq, negation complete
        subu    t5,t5,1                 // convert high part to ones complement
        subu    t6,t6,1                 // convert extended part to ones complement
        b       40f                     //

30:     beq     zero,t5,40f             // if eq, negation complete
        subu    t6,t6,1                 // convert extended part to ones complement

//
// Store final result.
//

40:     sw      t4,0(a0)                // store low half of result
        sw      t5,4(a0)                // store high half of result
        move    v0,a0                   // set function return register
        j       ra                      // return

        .end    RtlExtendedIntegerMultiply

        SBTTL("Large Integer Negate")
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
// Arguments:
//
//    Subtrahend (a2, a3) - Supplies the subtrahend value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerNegate)

        subu    a2,zero,a2              // negate low part of subtrahend
        subu    a3,zero,a3              // negate high part of subtrahend
        sltu    t0,zero,a2              // compute borrow from high part
        subu    a3,a3,t0                // subtract borrow from high part
        sw      a2,0(a0)                // store low part of result
        sw      a3,4(a0)                // store high part of result
        move    v0,a0                   // set function return register
        j       ra                      // return

        .end    RtlLargeIntegerNegate

        SBTTL("Large Integer Subtract")
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
// Arguments:
//
//    Minuend (a2, a3) - Supplies the minuend value.
//
//    Subtrahend (4 * 4(sp), 4 * 5(sp)) - Supplies the subtrahend value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerSubtract)

        lw      a1,4 * 4(sp)            // get low part of subtrahend
        lw      t2,4 * 5(sp)            // get high part of subtrahend
        subu    t0,a2,a1                // subtract low parts
        subu    t1,a3,t2                // subtract high parts
        sltu    t3,a2,a1                // generate borrow from high part
        subu    t1,t1,t3                // subtract borrow
        sw      t0,0(a0)                // store low part of result
        sw      t1,4(a0)                // store high part of result
        move    v0,a0                   // set function return register
        j       ra                      // return

        .end    RtlLargeIntegerSubtract

        SBTTL("Large Integer Shift Left")
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
//    This function shifts a signed large integer left by an unsigned
//    integer modulo 64 and returns the shifted signed large integer
//    result.
//
//    N.B. No test is made for significant bits shifted out of the result.
//
// Arguments:
//
//    LargeInteger (a2, a3) - Supplies the large integer to be shifted.
//
//    ShiftCount (4 * 4(sp)) - Supplies the left shift count.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerShiftLeft)

        lbu     a1,4 * 4(sp)            // get shift count
        move    v0,a0                   // set function return register
        and     a1,a1,0x3f              // truncate shift count mod 64

//
// Left shift the operand by the specified shift count.
//

        li      v1,32                   // compute right shift count
        subu    v1,v1,a1                //
        bgtz    v1,10f                  // if gtz, shift less that 32-bits

//
// Shift count is greater than or equal 32 bits - low half of result is zero,
// high half is the low half shifted left by remaining count.
//

        sll     a3,a2,a1                // set high half of result
        sw      zero,0(a0)              // store low part of reuslt
        sw      a3,4(a0)                // store high part of result
        j       ra                      // return

//
// Shift count is less than 32-bits - high half of result is the high half
// of operand shifted left by count combined with the low half of the operand
// shifted right, low half of result is the low half shifted left.
//

10:     sll     a3,a3,a1                // shift high half left count bits
        beq     zero,a1,20f             // if eq, no more shifts necessary
        srl     t0,a2,v1                // isolate shifted out bits of low half
        sll     a2,a2,a1                // shift low half left count bits
        or      a3,a3,t0                // combine bits for high half of result
20:     sw      a2,0(a0)                // store low part of reuslt
        sw      a3,4(a0)                // store high part of result
        j       ra                      // return

        .end    RtlLargeIntegerShiftLeft

        SBTTL("Large Integer Logical Shift Right")
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
//    integer modulo 64 and returns the shifted unsigned large integer
//    result.
//
// Arguments:
//
//    LargeInteger (a2, a3) - Supplies the large integer to be shifted.
//
//    ShiftCount (4 * 4(sp)) - Supplies the right shift count.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerShiftRight)

        lbu     a1,4 * 4(sp)            // get shift count
        move    v0,a0                   // set function return register
        and     a1,a1,0x3f              // truncate shift count mod 64

//
// Right shift the operand by the specified shift count.
//

        li      v1,32                   // compute left shift count
        subu    v1,v1,a1                //
        bgtz    v1,10f                  // if gtz, shift less that 32-bits

//
// Shift count is greater than or equal 32 bits - high half of result is
// zero, low half is the high half shifted right by remaining count.
//

        srl     a2,a3,a1                // set low half of result
        sw      a2,0(a0)                // store low part of reuslt
        sw      zero,4(a0)              // store high part of result
        j       ra                      // return

//
// Shift count is less than 32-bits - high half of result is the high half
// of operand shifted right by count, low half of result is the shifted out
// bits of the high half combined with the right shifted low half of the
// operand.
//

10:     srl     a2,a2,a1                // shift low half right count bits
        beq     zero,a1,20f             // if eq, no more shifts necessary
        sll     t0,a3,v1                // isolate shifted out bits of high half
        srl     a3,a3,a1                // shift high half right count bits
        or      a2,a2,t0                // combine bits for low half of result
20:     sw      a2,0(a0)                // store low part of reuslt
        sw      a3,4(a0)                // store high part of result
        j       ra                      // return

        .end    RtlLargeIntegerShiftRight

        SBTTL("Large Integer Arithmetic Shift Right")
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
//    This function shifts a signed large integer right by an unsigned
//    integer modulo 64 and returns the shifted signed large integer
//    result.
//
// Arguments:
//
//    LargeInteger (a1, a2) - Supplies the large integer to be shifted.
//
//    ShiftCount (a3) - Supplies the right shift count.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by a0.
//
//--

        LEAF_ENTRY(RtlLargeIntegerArithmeticShift)

        lbu     a1,4 * 4(sp)            // get shift count
        move    v0,a0                   // set function return register
        and     a1,a1,0x3f              // truncate shift count mod 64

//
// Right shift the operand by the specified shift count.
//

        li      v1,32                   // compute left shift count
        subu    v1,v1,a1                //
        bgtz    v1,10f                  // if gtz, shift less that 32-bits

//
// Shift count is greater than or equal 32 bits - high half of result is
// zero, low half is the high half shifted right by remaining count.
//

        sra     a2,a3,a1                // set low half of result
        sra     a3,a3,31                // set high half of result
        sw      a2,0(a0)                // store low part of reuslt
        sw      a3,4(a0)                // store high part of result
        j       ra                      // return

//
// Shift count is less than 32-bits - high half of result is the high half
// of operand shifted right by count, low half of result is the shifted out
// bits of the high half combined with the right shifted low half of the
// operand.
//

10:     srl     a2,a2,a1                // shift low half right count bits
        beq     zero,a1,20f             // if eq, no more shifts necessary
        sll     t0,a3,v1                // isolate shifted out bits of high half
        sra     a3,a3,a1                // shift high half right count bits
        or      a2,a2,t0                // combine bits for low half of result
20:     sw      a2,0(a0)                // store low part of reuslt
        sw      a3,4(a0)                // store high part of result
        j       ra                      // return

        .end    RtlLargeIntegerArithmeticShift
