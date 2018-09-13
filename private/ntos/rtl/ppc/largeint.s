//      TITLE("Large Integer Arithmetic")
//++
//
// Copyright (c) 1993  IBM Corporation
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
//    Converted to PowerPC by Walt Daniels and Norman Cohen Aug 93
//       (from MIPS based code)
//
// References:
//    See PowerPC Architecture book Appendix E.2 for 64-bit shifts
//    See "Hacker's Delight", Hank Warren, Nov. 91 for fancy divides
//
// Environment:
//
//    Any mode.
//
// Revision History:
//    Fixed RtlExtendedLargeIntegerDivide       (Steve Johns) 18-Feb-94
//       - if divisor >= 2^16 && dividend >= 2^32, quotient incorrect
//       - also, removed 6 uncessary occurrences of CMPI
//
//--

#include "ksppc.h"



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
//    Addend1 (r.5, r.6) - Supplies the first addend value.
//
//    Addend2 (r.7, r.8) - Supplies the second addend value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlLargeIntegerAdd)

        addc    r.5,r.5,r.7             // add low parts of large integer
        adde    r.6,r.6,r.8             // add high parts with carry
        stw     r.5,0(r.3)              // store low 32-bits
        stw     r.6,4(r.3)              // store high 32-bits
        LEAF_EXIT(RtlLargeIntegerAdd)   // return



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
//     SignedInteger (r.4) - Supplies the value to convert.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlConvertLongToLargeInteger)

        srawi   r.5,r.4,31              // compute high part of result
        stw     r.4,0(r.3)              // store low 32-bits
        stw     r.5,4(r.3)              // store high 32-bits
        LEAF_EXIT(RtlConvertLongToLargeInteger)  // return



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
//     UnsignedInteger (r.4) - Supplies the value to convert.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlConvertUlongToLargeInteger)

        li      r.5,0                   // clear high part
        stw     r.4,0(r.3)              // store low 32-bits
        stw     r.5,4(r.3)              // store high 32-bits
        LEAF_EXIT(RtlConvertUlongToLargeInteger) // return



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
//    Multiplicand (r.4) - Supplies the multiplicand value.
//
//    Multiplier (r.5) - Supplies the multiplier value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlEnlargedIntegerMultiply)

        mullw   r.6,r.4,r.5             // keep low 32-bits of result
        mulhw   r.7,r.4,r.5             // keep high 32-bits of result
        stw     r.6,0(r.3)              // store low 32-bits
        stw     r.7,4(r.3)              // store high 32-bits
        LEAF_EXIT(RtlEnlargedIntegerMultiply) // return



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
//    Multiplicand (r.4) - Supplies the multiplicand value.
//
//    Multiplier (r.5) - Supplies the multiplier value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlEnlargedUnsignedMultiply)

        mullw   r.6,r.4,r.5             // keep low-32-bits
        mulhwu  r.7,r.4,r.5             // keep high 32-bits
        stw     r.6,0(r.3)              // store low 32-bits
        stw     r.7,4(r.3)              // store high 32-bits
        LEAF_EXIT(RtlEnlargedUnsignedMultiply) // return



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
//    Dividend (r.3, r.4) - Supplies the dividend value.
//       (High-order bits in r.4, low-order bits in r.3)
//
//    Divisor (r.5) - Supplies the divisor value.
//
//    Remainder (r.6) - Supplies an optional pointer to a variable that
//        receives the remainder. Ptr is null if not needed.
//
// Return Value:
//
//    The unsigned long integer quotient is returned as the function value.
//
//--

        LEAF_ENTRY(RtlEnlargedUnsignedDivide)

        cmplw   r.4,r.5
        bge     overflow                // catch overflow or division by 0
        cmplwi  r.4,0                   // test high part for 0
        beq     only_32_bits            // 32-bit division suffices
// Normalize:  Shift divisor and dividend left to get rid of leading zeroes
// in the divisor.  Since r.4 < r.5, only zeroes are shifted out of the
// dividend.
        cntlzw  r.7,r.5                // number of bits to shift (N)
        slw     r.5,r.5,r.7            // shift divisor
        slw     r.4,r.4,r.7            // shift upper part of divisor
        subfic  r.9,r.7,32             // 32-N
        srw     r.9,r.3,r.9            // leftmost N bits of r.3, slid right
        or      r.4,r.4,r.9            //   and inserted into low end of r.4
        slw     r.3,r.3,r.7            // shift lower part of divisor
// Estimate high-order halfword of quotient.  If the dividend is
// A0 A1 A2 A3 and the divisor is B0 B1  (where each Ai or Bi is a halfword),
// then the estimate is A0 A1 0000 divided by B0 0000, or A0 A1 divided by B0.
// (r.4 holds A0 A1, r.3 holds A2 A3, and r.5 holds B0 B1.)
// The estimate may be too high because it does not account for B1; in rare
// cases, the estimate will not even fit in a halfword.  High estimates are
// corrected for later.
        srwi    r.8,r.5,16             // r.8 <- B0
        divwu   r.12,r.4,r.8           // r.12 <- floor([A0 A1]/B0)
// Subtract partial quotient times divisor from dividend: If Q0 is the quotient
// computed above, this means that Q0 0000 times B0 B1 is subtracted from
// A0 A1 A2 A3.  We compute Q0 times B0 B1 and then shift the two-word
// product left 16 bits.
        mullw   r.9,r.12,r.5           // low word of Q0 times B0 B1
        mulhwu  r.10,r.12,r.5          // high word of Q0 times B0 B1
        slwi    r.10,r.10,16           // shift high word left 16 bits
        inslwi  r.10,r.9,16,16         // move 16 bits from left of low word to
                                       //   right of high word
        slwi    r.9,r.9,16             // shift low word left 16 bits
        subfc   r.3,r.9,r.3            // low word of difference
        subfe   r.4,r.10,r.4           // high word of difference
// If the estimate for Q0 was too high, the difference will be negative.
// While A0 A1 A2 A3 is negative, repeatedly add B0 B1 0000 to A0 A1 A2 A3
// and decrement Q0 by one to correct for the overestimate.
        cmpwi   r.4,0                  // A0 A1 A2 A3 is negative iff A0 A1 is
        bge     Q0_okay                // no correction needed
        inslwi  r.10,r.5,16,16         // high word of B0 B1 0000 (= 0000 B0)
        slwi    r.9,r.5,16             // low word of B0 B1 0000 (= B1 0000)
adjust_Q0:
        addc   r.3,r.3,r.9             // add B0 B1 0000 to A0 A1 A2 A3 (low)
        adde   r.4,r.4,r.10            // add B0 B1 0000 to A0 A1 A2 A3 (high)
        cmpwi  r.4,0                   // Is A0 A1 A2 A3 now nonnegative?
        addi   r.12,r.12,-1            // decrement Q0
        blt    adjust_Q0               // if A0 A1 A2 A3 still negative, repeat
Q0_okay:
// Estimate low-order halfword of quotient.  A0 is necessarily 0000 at this
// point, so if the remaining part of the dividend is A0 A1 A2 A3 then the
// estimate is A1 A2 0000 divided by B0 0000, or A1 A2 divided by B0.
// (r.4 holds A0 A1, r.3 holds A2 A3, and r.8 holds B0.)
        slwi    r.9,r.4,16             // r.9 <- A1 0000
        inslwi  r.9,r.3,16,16          // r.9 <- A1 A2
        divwu   r.11,r.9,r.8           // r.11 <- floor([A1 A2]/B0)
// Subtract partial quotient times divisor from remaining part of dividend:
// If Q1 is the quotient computed above, this means
// that Q1 times B0 B1 is subtracted from A0 A1 A2 A3.  We compute
        mullw   r.9,r.11,r.5           // low word of Q1 times B0 B1
        mulhwu  r.10,r.11,r.5          // high word of Q1 times B0 B1
        subfc   r.3,r.9,r.3            // low word of difference
        subfe   r.4,r.10,r.4           // high word of difference
// If the estimate for Q1 was too high, the difference will be negative.
// While A0 A1 A2 A3 is negative, repeatedly add B0 B1 to A0 A1 A2 A3
// and decrement Q1 by one to correct for the overestimate.
        cmpwi   r.4,0                  // A0 A1 A2 A3 is negative iff A0 A1 is
        bge     Q1_okay                // no correction needed
adjust_Q1:
        addc   r.3,r.3,r.5             // add B0 B1 to A0 A1 A2 A3 (low)
        addze  r.4,r.4                 // add B0 B1 to A0 A1 A2 A3 (high)
        cmpwi  r.4,0                   // Is A0 A1 A2 A3 now nonnegative?
        addi   r.11,r.11,-1            // decrement Q1
        blt    adjust_Q1               // if A0 A1 A2 A3 still negative, repeat
Q1_okay:
// Build the results.  The desired quotient is Q0 Q1.
// The desired remainder is obtained by shifting A2 A3 right by the number
// of bits by which the dividend and divisor were shifted left in the
// normalization step.  The number of bits shifted is still in r.7.
        cmplwi r.6,0                   // remainder needed?
        bne    rem1                    // if so, go compute it
        slwi   r.3,r.12,16             // r.3 <- Q0 0000
        or     r.3,r.3,r.11            // r.3 <- Q0 Q1
        blr
rem1:
        srw    r.8,r.3,r.7             // remainder <- [A2 A3] >> (r.7)
        slwi   r.3,r.12,16             // r.3 <- Q0 0000
        stw    r.8,0(r.6)              // store remainder
        or     r.3,r.3,r.11            // r.3 <- Q0 Q1
        blr
//
// End of normal case
//
// The case of a 32-bit dividend:
only_32_bits:
        cmplwi  r.6,0                   // remainder needed?
        bne     rem2                    // if so, go compute quotient+remainder
        divwu   r.3,r.3,r.5             // result <- dividend/divisor
        blr
rem2:
        divwu   r.7,r.3,r.5             // quotient <- dividend / divisor
        mullw   r.8,r.7,r.5             // r.8 <- quotient * divisor
        subf    r.8,r.8,r.3             // remainder<-dividend-quotient*divisor
        mr      r.3,r.7                 // result <- quotient
        stw     r.8,0(r.6)              // store remainder
        blr
// The error cases:
overflow:
        twi     6,r.5,0                 // trap if divide by zero
        twi     0x1b,r.5,0              // trap on overflow

        LEAF_EXIT(RtlEnlargedUnsignedDivide)


//++
//
// ULARGE_INTEGER
// RtlExtendedLargeIntegerDivide (
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
// Arguments:
//
//    Dividend (r.5, r.6) - Supplies the dividend value.
//
//    Divisor (r.7) - Supplies the divisor value.
//
//    Remainder (r.8)- Supplies an optional pointer to a variable
//      that receives the remainder.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlExtendedLargeIntegerDivide)

        cmplwi  r.7,0                   // zero divisor?
        beq     div_zero_s              // if so, branch to error exit
        cmpwi   r.6,0                   // check sign of dividend high word
        bne     big_dividend

// The high-order word of the dividend is zero, so 32-bit unsigned division
// can be used.
        li      r.12,0                  // upper word of quotient is zero
        stw     r.12,4(r.3)             // store upper word of quotient
        divwu   r.11,r.5,r.7            // compute lower word of quotient
        stw     r.11,0(r.3)             // store lower word of quotient
        cmplwi  r.8,0                   // remainder needed?
        beqlr                           // if not, return
        mullw   r.10,r.11,r.7           // quotient * divisor
        subf    r.9,r.10,r.5            // dividend - quotient * divisor
        stw     r.9,0(r.8)              // store remainder
        blr                             // return

big_dividend:
        srwi.   r.0,r.7,16              // upper 16 bits of divisor
        bne     long_division           // if not, must use long division

// The divisor is only one 16-bit digit long, so use short division:
        srwi    r.0,r.6,16              // first 16-bit digit of dividend
        divwu   r.4,r.0,r.7             // first 16-bit digit of quotient
        mullw   r.10,r.4,r.7            // amount to subtract for remainder
        subf    r.9,r.10,r.0            // remainder from first digit
        insrwi  r.6,r.9,16,0            // combine rmndr with 2nd digit of dvdnd
        divwu   r.12,r.6,r.7            // second digit of quotient
        insrwi  r.12,r.4,16,0           // high two quotient digits in one word
        mullw   r.10,r.12,r.7           // amount to subtract for remainder
        subf    r.9,r.10,r.6            // remainder from second digit
        srwi    r.0,r.5,16              // third digit of dividend
        insrwi  r.0,r.9,16,0            // combine rmndr with 3rd digit of dvdnd
        divwu   r.4,r.0,r.7             // third digit of quotient
        mullw   r.10,r.4,r.7            // amount to subtract for remainder
        subf    r.9,r.10,r.0            // remainder from third digit
        insrwi  r.5,r.9,16,0            // combine rmndr with 4th digit of dvdnd
        divwu   r.11,r.5,r.7            // fourth digit of quotient
        mullw   r.10,r.11,r.7           // amount to subtract for remainder
        insrwi  r.11,r.4,16,0           // low two quotient digits in one word
        subf    r.9,r.10,r.5            // remainder from fourth digit
        b       store_results

long_division:
// Since the divisor is more than one 16-bit digit long, the quotient will
// be of the form 0x0000 Q2 Q3 Q4, where each of Q2, Q3, and Q4 is a 16-bit
// digit.
//
// Normalize the divisor and dividend so that the high-order bit of the
// divisor is 1.  This normalization must be undone after the division to
// compute the remainder.  Let U1 U2 U3 U4 be the 16-bit digits of the
// unnormalized dividend.  Each digit Ui consists of an S-bit high-order part
// UiH and a (16-S)-bit low-order part UiL, where S is the number of leading
// zeroes in the divisor.  Thus R6 holds U1H U1L U2H U2L and R5 holds
// U3H U3L U4H U4L.  Let N0 N1 N2 N3 N4 be the 16-bit digits of the  normalized
// dividend: N0 = U1H, N1 = U1L U2H, N2 = U2L U3H, N3 = U3L U4H, N4 = U4L 0...0.
// Let D1 D2 be the normalized divisor.
        cntlzw  r.0,r.7                 // number of bits to shift left (S)
        subfic  r.12,r.0,16             // 16-S
        subfic  r.11,r.0,32             // 32-S
        srw     r.9,r.6,r.12            // U1H U1L U2H = N0 N1
        srw     r.4,r.5,r.11            // U3H
        slw     r.10,r.6,r.0            // U1L U2H U2L 0...0
        or      r.6,r.4,r.10            // U1L U2H U2L U3H = N1 N2
        slw     r.4,r.5,r.0             // U3L U4H U4L 0...0 = N3 N4
        slw     r.7,r.7,r.0             // normalized divisor (D1 D2)
        srwi    r.11,r.7,16             // D1
// Set Q2 = [N0 N1 N2] / [D1 D2].  Start by guessing Q2 = [N0 N1] / D1, then
// adjust if necessary.  This guess will occasionally be one too high and
// very rarely two too high, but never higher than that and never too low.
// (See Theorem B in Section 4.3.1 of Knuth, Vol. II, pp. 256-7.)
// Let C N0' N1' N2' be the partial remainder [N0 N1 N2] - Q2 * [D1 D2].
        divwu   r.12,r.9,r.11           // guess Q2 = [N0 N1] / D1
        srwi    r.10,r.9,16             // 0x0000 N0
        mullw   r.5,r.12,r.7            // low word of Q2 * [D1 D2]
        subfc   r.9,r.5,r.6             // low word of C N0' N1' N2' (N1' N2')
        mulhwu  r.5,r.12,r.7            // high word of Q2 * [D1 D2]
        subfe.  r.10,r.5,r.10           // high word of C N0' N1' N2' (C N0')
        bge     Q2_okay                 // if difference is >= 0, Q2 was not too high
adjust_Q2:
        addc    r.9,r.9,r.7             // low word of [C N0' N1' N2']+[D1 D2]
        addze.  r.10,r.10               // high word of [C N0' N1' N2']+[D1 D2]
        addi    r.12,r.12,-1            // Q2 - 1
        blt     adjust_Q2               // try again if still negative
Q2_okay:
// At this point 0 <= [C N0' N1' N2'] < [D1 D2], so [C N0'] = 0x00000000.
// r.12 holds the upper word of the quotient, 0x0000 Q2.
// Set Q3 = [N1' N2' N3] / [D1 D2] by guessing and adjusting as above.
// Let [N0" N1" N2" N3"] be the partial remainder [N1' N2' N3] - Q3 * [D1 D2].
        divwu   r.6,r.9,r.11            // guess Q3 = [N1' N2'] / D1
        srwi    r.10,r.9,16             // 0x0000 N1'
        slwi    r.9,r.9,16              // N2' 0x0000
        inslwi  r.9,r.4,16,16           // N2' N3
        mullw   r.5,r.6,r.7             // low word of Q3 * [D1 D2]
        subfc   r.9,r.5,r.9             // N2" N3"
        mulhwu  r.5,r.6,r.7             // high word of Q3 * [D1 D2]
        subfe.  r.10,r.5,r.10           // N0" N1"
        bge     Q3_okay                 // if difference >= 0, Q3 was not too high
adjust_Q3:
        addc    r.9,r.9,r.7             // low word of [N0" N1" N2" N3"]+[D1 D2]
        addze.  r.10,r.10               // high word [N0" N1" N2" N3"]+[D1 D2]
        addi    r.6,r.6,-1              // Q3 - 1
        blt     adjust_Q3               // try again if difference still negative
Q3_okay:
// At this point 0 <= [N0" N1" N2" N3"] < [D1 D2], so [N0" N1"] = 0x00000000.
// Set Q4 = [N2" N3" N4] / [D1 D2] by guessing and adjusting as above.
// Let [R1 R2 R3 R4] be the partial remainder [N2" N3" N4] - Q4 * [D1 D2].
        divwu   r.11,r.9,r.11           // guess Q4 = [N2" N3"] / D1
        insrwi  r.4,r.9,16,0            // N3" N4
        srwi    r.10,r.9,16             // 0x0000 N2"
        mullw   r.5,r.11,r.7            // low word of Q4 * [D1 D2]
        subfc   r.9,r.5,r.4             // R3 R4
        mulhwu  r.5,r.11,r.7            // high word of Q4 * [D1 D2]
        subfe.  r.10,r.5,r.10           // R1 R2
        bge     Q4_okay                 // if difference < 0, Q4 was not too high
adjust_Q4:
        addc    r.9,r.9,r.7             // low word of [R1 R2 R3 R4]+[D1 D2]
        addze.  r.10,r.10               // high word of [R1 R2 R3 R4]+[D1 D2]
        addi    r.11,r.11,-1            // Q4 - 1
        blt     adjust_Q4               // try again if partial remainder still negative
Q4_okay:
// At this point 0 <= [R1 R2 R3 R4] < [D1 D2], so [R3 R4] is the remainder.
        insrwi  r.11,r.6,16,0           // low word of quotient: Q3 Q4
        srw     r.9,r.9,r.0             // unnormalize remainder

store_results:
        stw     r.11,0(r.3)             // store low word of quotient
        stw     r.12,4(r.3)             // store high word of quotient
        cmplwi  r.8,0                   // remainder needed?
        beqlr                           // if not, return
        stw     r.9,0(r.8)              // store remainder
        blr                             // return

div_zero_s:
        twi     6,r.7,0                 // Trap on divide by zero

        LEAF_EXIT(RtlExtendedLargeIntegerDivide)


//++
//
// LARGE_INTEGER
// RtlExtendedMagicDivide (
//    IN LARGE_INTEGER Dividend,
//    IN ULARGE_INTEGER MagicDivisor,
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
//    The value returned is the most significant 64 bits of the product
//    Dividend*MagicDivisor, shifted right ShiftCount bits.
//
// Arguments:
//
//    Dividend (r.5, r.6) - Supplies the dividend value.
//
//    MagicDivisor (r.7, r.8) - Supplies the magic divisor value
//       which is a 64-bit multiplicative reciprocal.
//
//    Shiftcount (r.9) - Supplies the right shift adjustment value,
//       assumed to be in the range 0 to 63.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--
// Let Dividend = A B and MagicDivisor = C D, where each of A, B, C, and D is
// a 32-bit word.  Then Dividend*MagicDivisor is a 128-bit product, computed
// as follows:
//                                                A              B
//                                   x            C              D
//            ==========================================================
//                                          high_word(B*D) low_word(B*D)
//                           high_word(A*D)  low_word(A*D)
//                           high_word(B*C)  low_word(B*C)
//            high_word(A*C)  low_word(A*C)
//            ==========================================================
//                 P1             P2            P3              P4
//
// Since the return value is [P1 P2] >> Shift_Count, P3 and P4 need not be
// computed, but the carry out of the P3 column must be computed to compute P2.

        LEAF_ENTRY(RtlExtendedMagicDivide)

// If the dividend is negative, negate it and record the fact by setting
// cr7 to LT.
        crclr   4*cr.7+0                // clear cr.7 LT bit
        cmpwi   r.6,0                   // is high-order word of divisor < 0?
        bge     divisor_nonnegative     // if not, we are ready to compute
        crset   4*cr.7+0                // set cr.7 LT bit to mark negation
        subfic  r.5,r.5,0               // negate lower half of dividend
        subfze  r.6,r.6                 // negate upper half of dividend
divisor_nonnegative:
// To avoid pipeline delays, produce partial products first, in the order they
// will be consumed by the addc and addze instructions below.
        mulhwu  r.11,r.6,r.7            // high(A*D)
        mulhwu  r.0,r.5,r.8             // high(B*C)
        mulhwu  r.12,r.6,r.8            // high(A*C)
        mullw   r.4,r.6,r.8             // low(A*C)
        mulhwu  r.10,r.5,r.7            // high(B*D)
        mullw   r.6,r.6,r.7             // low(A*D)
        mullw   r.7,r.5,r.8             // low(B*C)
// Now combine the partial products, forming P1 in r.12, P2 in r.11, P3 in r.10:
        addc    r.11,r.11,r.0           // high(A*D)+high(B*C)
        addze   r.12,r.12               // high(A*C)+partial carry
        addc    r.11,r.11,r.4           // high(A*D)+high(B*C)+low(A*C)
        addze   r.12,r.12               // high(A*C)+partial carry
        addc    r.10,r.10,r.6           // high(B*D)+low(A*D)
        addze   r.11,r.11               // hi(A*D)+hi(B*C)+low(A*C)+part. carry
        addze   r.12,r.12               // high(A*C)+partial carry
        addc    r.10,r.10,r.7           // high(B*D)+low(A*D)+low(B*C) = P3
        addze   r.11,r.11               // hi(A*D)+hi(B*C)+low(A*C)+carry = P2
        addze   r.12,r.12               // high(A*C)+carry = P1
// Shift the 64-bit value whose high half is in r.12 and whose low half is in
// r.11 right by (r.7) bits.  The sequence below depends on the fact that
// shift amounts are interpreted mod 64.  In particular, a shift amount of
// -N bits, where 0 < N <= 32, specifies a shift of 64-N bits, where
// 32 <= 64-N < 64, and a shift of 32 or more bits always yields zero.
// Let s = (r.9) be the amount to shift right.  The sequence below behaves
// differently when s<32, when s=32, and when s>32.  When s<32, we view the
// 64-bit value to be shifted as follows:
//
//    |             r.12             |             r.11             |
//    +-----------------+------------+-----------------+------------+
//    |        T        |     U      |        V        |     W      |
//    +-----------------+------------+-----------------+------------+
//    |<- (32-s) bits ->|<- s bits ->|<- (32-s) bits ->|<- s bits ->|
//
// When s >= 32, we view the 64-bit value to be shifted as follows:
//
//    |             r.12             |             r.11             |
//    +-----------------+------------+------------------------------+
//    |        X        |     Y      |              Z               |
//    +-----------------+------------+------------------------------+
//    |                 |<- (s-32) ->|<------------ 32 ------------>|
//    |<- (64-s) bits ->|<--------------- s bits ------------------>|
//
                                // When s < 32: | When s = 32: | When s > 32:
                                // -------------+--------------+-------------
        subfic  r.7,r.9,32      // 32-s (> 0)   |   0          | 32-s (< 0)
        addi    r.8,r.9,-32     // s-32 (< 0)   |   0          | s-32 (> 0)
        srw     r.11,r.11,r.9   // 0...0 V      |   0          | 0
        slw     r.4,r.12,r.7    // U 0...0      |   X (= X Y)  | 0
        or      r.11,r.11,r.4   // U V          |   X          | 0
        srw     r.4,r.12,r.8    // 0            |   X          | 0...0 X
        or      r.11,r.11,r.4   // U V          |   X          | 0...0 X
        srw     r.12,r.12,r.9   // 0...0 T      |   0          | 0

// If the original dividend was negated, we now negate the result:
        bnl     cr.7,sign_is_right
        subfic  r.11,r.11,0             // negate low word
        subfze  r.12,r.12               // negate high word
sign_is_right:
// Store the result:
        stw     r.11,0(r.3)             // low word of result
        stw     r.12,4(r.3)             // high word of result
        blr                             // return

        LEAF_EXIT(RtlExtendedMagicDivide)


//++
//
// ULARGE_INTEGER
// RtlLargeIntegerDivide (
//    IN ULARGE_INTEGER Dividend,
//    IN ULARGE_INTEGER Divisor,
//    IN PLARGE_INTEGER Remainder.
//    )
//
// Routine Description:
//
//    This function divides an unsigned large integer by an unsigned large
//    integer and returns the resultant quotient and optionally the remainder.
//
// Arguments:
//
//    Dividend (r.5, r.6) - Supplies the dividend value.
//
//    Divisor (r.7, r.8) - Supplies the divisor value.
//
//    Remainder (r.9)- Supplies an optional pointer to a variable
//      that receives the remainder.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlLargeIntegerDivide)

        or.     r.0,r.7,r.8             // combine low and high parts of divisor
        beq     div0                    // if 0, then attempted division by zero
        li      r.0,64                  // set loop count
        mtctr   r.0                     // in the count register
        li      r.10,0                  // clear partial remainder
        li      r.11,0                  //

// Invariants for the following loop:
//    1. (q<<(CTR)*Divisor) + [r.11 r.10 r.6 r.5]>>(64-(CTR)) = Dividend
//    2. [r.11 r.10] < Divisor
// where q is the rightmost (64-(CTR)) bits of [r.6 r.5]
// Initially, (CTR)=64 and [r.11 r.10] = 0,, so q=0 and
//    [r.11 r.10 r.6 r.5]>>(64-(CTR)) = [0 0 r.6 r.5]>>0 = [r.6 r.5], reducing
//    the loop invariants to:
//       1. [r.6 r.5] = Dividend
//       2. 0 < Divisor
// At the end of the loop, (CTR)=0, so q=[r.6 r.5] and the loop invariants
//    reduce to:
//       1. [r.6 r.5]*Divisor + [r.11 r.10] = Dividend
//       2. [r.11 r.10] < Divisor
//    That is, [r.6 r.5] holds the quotient and [r.11 r.10] holds the remainder.
// During execution of the loop, [r.11 r.10] holds the partial remainder, the
//    leftmost (CTR) bits of [r.6 r.5] hold the bits of the dividend not yet
//    appended to the partial remainder, and the remaining bits of [r.6 r.5]
//    hold the leftmost (64-(CTR)) bits of the quotient.

divl:
// Shift the 128-bit quantity [r.11 r.10 r.6 r.5] left one bit.  This has the
// effect of dropping the leftmost bit of the partial remainder (necessarily
// zero), "bringing down" the next bit of the dividend to the right end of the
// partial remainder, and shifting the partial quotient left one bit.
        slwi    r.11,r.11,1             // shift r.11 left one bit
        inslwi  r.11,r.10,1,31          // left bit of r.10 to right bit of r.11
        slwi    r.10,r.10,1             // shift r.10 left one bit
        inslwi  r.10,r.6,1,31           // left bit of r.6 to right bit of r.10
        slwi    r.6,r.6,1               // shift r.6 left one bit
        inslwi  r.6,r.5,1,31            // left bit of r.5 to right bit of r.6
        slwi    r.5,r.5,1               // shift r.5 left one bit
// If [r.11 r.10] >= Divisor, increment the quotient and subtract the divisor
// from the partial remainder:
        cmplw   cr.0,r.11,r.8           // high(partial_rem) vs. high(divisor)
        cmplw   cr.1,r.10,r.7           // low(partial_rem) vs. low(divisor)
        bgt     cr.0,PR_greater         // high(part_rem) > high(divisor)
        blt     cr.0,endl               // high(part_rem) < high(divisor)
        blt     cr.1,endl               // highs =, low(part_rem) < low(divisor)

PR_greater:
        ori     r.5,r.5,1               // increment shifted quotient
        subfc   r.10,r.7,r.10           // low(part_rem-divisor)
        subfe   r.11,r.8,r.11           // high(part_rem-divisor)

endl:   bdnz    divl                    // decrement CTR and loop

        cmplwi  r.9,0                   // remainder requested?
        beq     norem                   // no remainder
        stw     r.10,0(r.9)             // store low part of remainder
        stw     r.11,4(r.9)             // store high part of remainder
norem:  stw     r.5,0(r.3)              // store low part of quotient
        stw     r.6,4(r.3)              // store high part of quotient
        blr

div0:
        twi     6,r.0,0                 // Trap on divide by zero

        LEAF_EXIT(RtlLargeIntegerDivide) //



//++
//
// LARGE_INTEGER
// Rtl     ExtendedIntegerMultiply (
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
//    Multiplicand (r.5, r.6) - Supplies the multiplicand value.
//
//    Multiplier (r.7) - Supplies the multiplier value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlExtendedIntegerMultiply)

// If A B is the multiplicand and C is the multiplier (where A, B, and C are
// each 32 bits long), the product is computed as follows:
//
//                           A         B
//      *                              C
//         =============================
//                   high(B*C)  low(B*C)
//      +  high(A*C)  low(A*C)
//         =============================
//                P1        P2        P3
//
// Since the high-order bit of B is not a sign bit but the high-order bit of
// C is, we have no multiplication instruction appropriate for computing
// high(B*C) directly.  Instead, we negate any negative operand before
// doing the multiplication, multiply using unsigned arithmetic, and then
// negate the product if we had negated exactly one operand.  If P1 is
// nonzero before negating the product, the multiplication has overflowed.
// We use the LT bit of cr.7 to track whether exactly one operand has
// been negated.

        crclr   4*cr.7+0                // clear LT bit of cr.7
        cmpwi   r.6,0                   // test sign of multiplicand
        bge     multiplicand_adjusted   // if nonnegative, proceed
        subfic  r.5,r.5,0               // negate low part of multiplicand
        subfze  r.6,r.6                 // negate high part of multiplicand
        crset   4*cr.7+0                // set LT bit of cr.7
multiplicand_adjusted:
        cmpwi   r.7,0                   // test sign of multiplier
        bge     multiplier_adjusted     // if nonnegative, proceed
        neg     r.7,r.7                 // negate multiplier
        crnot   4*cr.7+0,4*cr.7+0       // invert LT bit of cr.7
multiplier_adjusted:
        mulhwu  r.9,r.5,r.7             // high(B*C)
        mullw   r.10,r.6,r.7            // low(A*C)
        mulhwu  r.11,r.6,r.7            // high(A*C)
        mullw   r.8,r.5,r.7             // P3 = low(B*C)
        addc    r.9,r.9,r.10            // P2 = high(B*C)+low(A*C)
        addze   r.11,r.11               // P1 = high(A*C)+[carry out of P2]
        cmpwi   r.11,0                  // check for overflow
        bne     mull_over
        bnl     cr.7,product_adjusted   // was exactly one operand negated?
        subfic  r.8,r.8,0               // negate low part of product
        subfze  r.9,r.9                 // negate high part of product
product_adjusted:
        stw     r.8,0(r.3)              // store low word of product
        stw     r.9,4(r.3)              // store high word of product
        blr
mull_over:
        twi     0x1b,r.11,0             // Trap on overflow

        LEAF_EXIT(RtlExtendedIntegerMultiply)



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
//    Subtrahend (r.5, r.6) - Supplies the subtrahend value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlLargeIntegerNegate)

        subfic  r.5,r.5,0               // double precision subtract from 0
        subfze  r.6,r.6
        stw     r.5,0(r.3)              // store low part of result
        stw     r.6,4(r.3)              // store high part of result
        LEAF_EXIT(RtlLargeIntegerNegate) // return



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
//    Minuend (r.5, r.6) - Supplies the minuend value.
//
//    Subtrahend (r.7, r.8) - Supplies the subtrahend value.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlLargeIntegerSubtract)

        subfc   r.5,r.7,r.5             // double precision subtract
        subfe   r.6,r.8,r.6
        stw     r.5,0(r.3)              // store low part of result
        stw     r.6,4(r.3)              // store high part of result
        LEAF_EXIT(RtlLargeIntegerSubtract) // return



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
//    LargeInteger (r.5, r.6) - Supplies the large integer to be shifted.
//
//    ShiftCount (r.7) - Supplies the left shift count.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlLargeIntegerShiftLeft)

        andi.   r.7,r.7,0x3f            // mod 64 of shift count
        subfic  r.8,r.7,32
        slw     r.6,r.6,r.7
        srw     r.0,r.5,r.8
        or      r.6,r.6,r.0
        addic   r.8,r.7,-32
        slw     r.0,r.5,r.8
        or      r.6,r.6,r.0
        slw     r.5,r.5,r.7
        stw     r.5,0(r.3)              // store low result
        stw     r.6,4(r.3)              // store high result
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
//    integer modulo 64 and returns the shifted unsigned large integer
//    result.
//
// Arguments:
//
//    LargeInteger (r.5, r.6) - Supplies the large integer to be shifted.
//
//    ShiftCount (r.7) - Supplies the right shift count.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlLargeIntegerShiftRight)

        andi.   r.7,r.7,0x3f            // mod 64 of shift count

// The sequence below depends on the fact that shift amounts are interpreted
// mod 64.  In particular, a shift amount of -N bits, where 0 < N <= 32,
// specifies a shift of 64-N bits, where 32 <= 64-N < 64, and a shift of 32 or
// more bits always yields zero.  Let s = (r.7) be the amount to shift right.
// The sequence below behaves differently when s<32, when s=32, and when s>32.
// When s<32, we view the 64-bit value to be shifted as follows:
//
//
//    |             r.6              |             r.5              |
//    +-----------------+------------+-----------------+------------+
//    |        T        |     U      |        V        |     W      |
//    +-----------------+------------+-----------------+------------+
//    |<- (32-s) bits ->|<- s bits ->|<- (32-s) bits ->|<- s bits ->|
//
// When s >= 32, we view the 64-bit value to be shifted as follows:
//
//    |             r.6              |             r.5              |
//    +-----------------+------------+------------------------------+
//    |        X        |     Y      |              Z               |
//    +-----------------+------------+------------------------------+
//    |                 |<- (s-32) ->|<------------ 32 ------------>|
//    |<- (64-s) bits ->|<--------------- s bits ------------------>|
//
                                // When s < 32: | When s = 32: | When s > 32:
                                // -------------+--------------+-------------
        subfic  r.8,r.7,32      // 32-s (> 0)   |   0          | 32-s (< 0)
        addi    r.9,r.7,-32     // s-32 (< 0)   |   0          | s-32 (> 0)
        srw     r.5,r.5,r.7     // 0...0 V      |   0          | 0
        slw     r.0,r.6,r.8     // U 0...0      |   X (= X Y)  | 0
        or      r.5,r.5,r.0     // U V          |   X          | 0
        srw     r.0,r.6,r.9     // 0            |   X          | 0...0 X
        or      r.5,r.5,r.0     // U V          |   X          | 0...0 X
        srw     r.6,r.6,r.7     // 0...0 T      |   0          | 0

        stw     r.5,0(r.3)      // store low result
        stw     r.6,4(r.3)      // store high result
        LEAF_EXIT(RtlLargeIntegerShiftRight)



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
//    LargeInteger (r.5, r.6) - Supplies the large integer to be shifted.
//
//    ShiftCount (r.7) - Supplies the right shift count.
//
// Return Value:
//
//    The large integer result is stored at the address supplied by r.3.
//
//--

        LEAF_ENTRY(RtlLargeIntegerArithmeticShift)

        andi.   r.7,r.7,0x3f            // mod 64 of shift count

// The sequence below depends on the fact that shift amounts are interpreted
// mod 64.  In particular, a shift amount of -N bits, where 0 < N <= 32,
// specifies a shift of 64-N bits, where 32 <= 64-N < 64, and an arithmetic
// shift of 32 or more bits always yields 32 copies of the original sign bit.
// Let s = (r.7) be the amount to shift right.  The sequence below behaves
// differently when s<=32 and when s>32.  When s<=32, we view the 64-bit value
// to be shifted as follows:
//
//
//    |             r.6              |             r.5              |
//    +-----------------+------------+-----------------+------------+
//    |        T        |     U      |        V        |     W      |
//    +-----------------+------------+-----------------+------------+
//    |<- (32-s) bits ->|<- s bits ->|<- (32-s) bits ->|<- s bits ->|
//
// When s > 32, we view the 64-bit value to be shifted as follows:
//
//    |             r.6              |             r.5              |
//    +-----------------+------------+------------------------------+
//    |        X        |     Y      |              Z               |
//    +-----------------+------------+------------------------------+
//    |                 |<- (s-32) ->|<------------ 32 ------------>|
//    |<- (64-s) bits ->|<--------------- s bits ------------------>|
//

                              // When s <= 32:         | When s > 32:
                              // ----------------------+----------------------
        subfic  r.8,r.7,32    // 32-s (>= 0)           | 32-s (< 0)
        srw     r.5,r.5,r.7   // 0...0 V               | 0
        slw     r.0,r.6,r.8   // U 0...0               | 0
        or      r.5,r.5,r.0   // U V                   | 0
        addic.  r.9,r.7,-32   // s-32 (<= 0)           | s-32 (> 0)
        sraw    r.10,r.6,r.9  // sign(TU)...sign(TU)   | sign(XY)...sign(XY) X
        ble     more_than_32  // (CR0 set by addic. two instructions earlier.)
        mr      r.5,r.10      // [instruction skipped] | sign(XY)...sign(XY) X
more_than_32:                 //                       |
        sraw    r.6,r.6,r.7   // sign(T)...sign(T) T   | sign(XY)...sign(XY)

        stw     r.5,0(r.3)    // store low result
        stw     r.6,4(r.3)    // store high result
        LEAF_EXIT(RtlLargeIntegerArithmeticShift)
