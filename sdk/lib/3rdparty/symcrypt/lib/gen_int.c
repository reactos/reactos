//
// gen_int.c   Generic integer algorithms (not tied to low-level implementations)
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"


UINT64
SYMCRYPT_CALL
SymCryptUint64Gcd( UINT64 a, UINT64 b, UINT32 flags )
{
    UINT64 swap;
    UINT64 tmp;
    UINT64 a2;
    UINT64 b2;
    UINT32 i;

/*
    Algorithm outline:

    if( b even )
        swap (a,b)

 loop:
    { invariant: b is odd }
    if( a even )
        a = a/2
    else
        if a < b
            swap (a,b)
        a = (a - b) / 2

    We ignore the data_public flag as we currently always use a side-channel safe implementation

    to compute (a < b) on 64-bit values is hard if we want to avoid
*/
    SYMCRYPT_ASSERT( (flags & SYMCRYPT_FLAG_GCD_INPUTS_NOT_BOTH_EVEN) != 0 && ((a | b) & 1) != 0 );
    UNREFERENCED_PARAMETER( flags );

    // First we make sure that b is odd
    // If b even: swap (a,b)
    swap = ~(0 - (b & 1));
    tmp = (a ^ b) & swap;
    a ^= tmp;
    b ^= tmp;

    // Each loop iteration reduces len(a) + len(b) by at least 1, so looping 127 times is enough.
    // For inputs (2^63, 2^63 + 1) we get 63 iterations to reduce a to 1, and then another 63 to get
    // the other value to 1, plus one more to make it 0.
    for( i=0; i < 127; i++ )
    {
        // Compute the result of the 'else' part of the if( a even ) into (a2, b2)
        // First we evaluate (a < b), which is a bit tricky without access to the carry flag.
        // a < b =      (b>>63) if ((a^b) >> 63) == 1
        //              (a - b) >> 63 otherwise
        tmp = a ^ b;
        tmp = (tmp & b) | (~tmp & (a-b));
        swap = 0 - (tmp >> 63);

        // Now swap if a < b into (a2, b2)
        tmp = (a ^ b) & swap;
        a2 = a ^ tmp;
        b2 = b ^ tmp;

        //
        a2 = (a2 - b2) / 2;

        // Compute the (a is odd) condition
        tmp = 0 - (a & 1);

        // Assemble the final result
        a = (tmp & a2) | (~tmp & a/2);
        b = (tmp & b2) | (~tmp & b);
    }

    SYMCRYPT_ASSERT( a == 0 );
    return b;
}


/*
Extended GCD notes.

A side-channel safe implementation cannot effectively use Euclid's algorithm.
The quotient is typically very small, but it can be very large. An SCS implementation
would require the quotient to always be treated as a full-sized number, which would kill performance.
Instead we use the binary algorithm which is easier to adapt to side-channel safety.

Basic algorithm for inputs S1 and S2:
    Eliminate the joint factors of two. These are added later to the result
    For now we assume that both S1 and S2 are non-zero and S2 is odd.

Invariant:
    A = A1 * S1 (mod S2)
    B = B1 * S1 (mod S2)
    B is odd

Initial values:
    A = S1; A1 = 1;
    B = S2; B1 = 0;

Main loop:

    t = len(A) + len(B) - 1     // Careful of overflows, use a SIZE_T

    repeat t times:
        1. if A odd and A < B:
                Swap (A, A1) with (B, B1)
        2. if A odd:
                A -= B;
                A1 -= B1 (mod S2);
        3. A /= 2;
           A1 /= 2 (mod S2);

Proof of the invariant:
    It is easy to see that initially the invariant holds (S2 is odd).

    Assume the invariant holds at the start of the loop's iteration.
    Step 1 of the main loop preserves the invariant since the first 2
    equations of the invariant are the same for A's and B's and
    the swapping happens only if A is odd. Therefore, B is odd
    after step 1.
    Step 2 essentially subtracts the second equation of the invariant
    from the first (modulo S2). This preserves the invariant since step
    1 ensured that A >= B (when A odd), so the operation A = A-B holds
    modulo S2.
    Step 3 essentially multiplies the first equation of the invariant
    with the inverse of 2 modulo S2. Since S2 is odd we know that the
    inverse exists. Also the operation A = A/2 is correct modulo S2
    because steps 1 and 2 ensured that A is even at this point.
    (To see this, consider 2*a = x (mod S2) => a = x*2^{-1} (mod S2)
     where a is an integer and 2^{-1} is the inverse of 2 modulo S2)

Termination/Results:
    Each iteration reduces len(A) + len(B) by at least one until A=0.
    When A=0 the loop does nothing except churn by dividing A and A1
    by 2 every time.
    After len(A)+len(B)-1 iterations, A must be zero. At that point
    we have

        B = GCD
        B1 * S1 = GCD (mod S2)

    The LCM is calculated as S1*S2 / GCD.

    InvS1ModS2 is defined as the smallest value X such that
    X*S1 = GCD (mod S2), but B1 might not be the smallest solution.
    Let P2 = S2/GCD.
    Any two solutions to X*S1 = GCD (mod S2) has (X1-X2)*S1 mod S2 = 0,
    so X1-X2 is a multiple of P2. Therefore we need to reduce B1 modulo P2
    to get the smallest solution for InvS1ModS2.

    ** Notice that if B1 is a multiple of S2 (or 0), which means that GCD is equal to S2,
    then the above result is 0. In that case InvS1ModS2 is undefined.

    Similarly, InvS2ModS1 is defined as the smallest value Y such that
    Y*S2 = GCD (mod S1). We have that for some integer q:

        q*S2 = B1*S1 - GCD       =>      (-q mod S1) * S2 = GCD (mod S1)

    As above, if B1 is 0, then InvS2ModS1 is undefined. Therefore we ignore this case.
    For the defined case, B1>=1 and S1>=GCD which implies that q >= 0. This
    allows us to divide (B1*S1 - GCD) by S2.
    Therefore InvS2ModS1 can be computed as -((B1*S1 - GCD)/S2) mod S1.

For simplicity, our generic implementation works with all values the same size.
This can be less efficient if one input is much larger than the other, for
example for RSA key generation when one input is 1000+ bits and the other 17 bits.
However, that is not a high-performance path. If it is, a dedicated GCD with one
input a UINT32 or UINT64 would be the solution to a much faster extended GCD.
*/
VOID
SYMCRYPT_CALL
SymCryptIntExtendedGcd(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
                                    UINT32          flags,
    _Out_opt_                       PSYMCRYPT_INT   piGcd,
    _Out_opt_                       PSYMCRYPT_INT   piLcm,
    _Out_opt_                       PSYMCRYPT_INT   piInvSrc1ModSrc2,
    _Out_opt_                       PSYMCRYPT_INT   piInvSrc2ModSrc1,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch )
{
    UINT32 nDigits  = SYMCRYPT_MAX( SymCryptIntDigitsizeOfObject( piSrc1 ), SymCryptIntDigitsizeOfObject( piSrc2 ));
    PSYMCRYPT_INT       piA;        // size nDigits
    PSYMCRYPT_INT       piB;        // size nDigits, NOT ALLOCATED (part of the pdGcd divisor)
    PSYMCRYPT_INT       piTmp;      // size nDigits
    PSYMCRYPT_INT       piA1;       // size nDigits
    PSYMCRYPT_INT       piB1;       // size nDigits
    PSYMCRYPT_INT       piTmpDbl;   // size 2*nDigits
    PSYMCRYPT_DIVISOR   pdGcd;      // size nDigits
    PSYMCRYPT_DIVISOR   pdTmp;      // size nDigits
    UINT32              cbInt;
    UINT32              cbWideInt;
    UINT32              cbDivisor;
    SIZE_T              cbFnScratch;
    UINT32              t;
    UINT32              c;
    UINT32              d;

    UNREFERENCED_PARAMETER( flags );    // Currently not used to improve performance.

    // Compute how much scratch space we need for the functions we call
    cbFnScratch = SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( 2 * nDigits, nDigits );
    cbFnScratch = SYMCRYPT_MAX( cbFnScratch, SYMCRYPT_SCRATCH_BYTES_FOR_INT_MUL( 2*nDigits ) );
    cbFnScratch = SYMCRYPT_MAX( cbFnScratch, SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( nDigits ) );

    cbInt = SymCryptSizeofIntFromDigits( nDigits );
    cbWideInt = SymCryptSizeofIntFromDigits( 2*nDigits );
    cbDivisor = SymCryptSizeofDivisorFromDigits( nDigits );

    SYMCRYPT_ASSERT( cbWideInt != 0 );
    SYMCRYPT_ASSERT( cbScratch >=   4 * cbInt +
                                    1 * cbWideInt +
                                    2 * cbDivisor +
                                    cbFnScratch );

    piA = SymCryptIntCreate( pbScratch, cbInt, nDigits );
    pbScratch += cbInt; cbScratch -= cbInt;
    // piB is stored inside the pdGcd object created later
    piTmp = SymCryptIntCreate( pbScratch, cbInt, nDigits );
    pbScratch += cbInt; cbScratch -= cbInt;
    piA1 = SymCryptIntCreate( pbScratch, cbInt, nDigits );
    pbScratch += cbInt; cbScratch -= cbInt;
    piB1 = SymCryptIntCreate( pbScratch, cbInt, nDigits );
    pbScratch += cbInt; cbScratch -= cbInt;

    piTmpDbl = SymCryptIntCreate( pbScratch, cbWideInt, 2 * nDigits );
    pbScratch += cbWideInt; cbScratch -= cbWideInt;

    pdGcd = SymCryptDivisorCreate( pbScratch, cbDivisor, nDigits );
    pbScratch += cbDivisor; cbScratch -= cbDivisor;
    piB = SymCryptIntFromDivisor( pdGcd );

    pdTmp = SymCryptDivisorCreate( pbScratch, cbDivisor, nDigits );
    pbScratch += cbDivisor; cbScratch -= cbDivisor;

    SymCryptIntCopyMixedSize( piSrc1, piA );    // Ignore the error return value here as we know
    SymCryptIntCopyMixedSize( piSrc2, piB );    // that the destination integers are large enough.

    SymCryptIntSetValueUint32( 1, piA1 );
    SymCryptIntSetValueUint32( 0, piB1 );

    // Currently not supported: Src1 to be 0 or Src2 to be even
    SYMCRYPT_ASSERT( !SymCryptIntIsEqualUint32( piA, 0 ) );
    SYMCRYPT_ASSERT( (SymCryptIntGetValueLsbits32( piB ) & 1) != 0 );
    if ( SymCryptIntIsEqualUint32( piA, 0 ) ||
        ((SymCryptIntGetValueLsbits32( piB ) & 1) == 0) )
    {
        goto cleanup;
    }

    // Currently not supported: piInvSrc2ModSrc1 != NULL and max( Src1.nDigits, Src2.nDigits ) * 2 > SymCryptDigitsFromBits(SYMCRYPT_INT_MAX_BITS)
    if( (piInvSrc2ModSrc1 != NULL) && (piTmpDbl == NULL) )
    {
        goto cleanup;
    }

    t = SymCryptIntBitsizeOfObject( piSrc1 ) + SymCryptIntBitsizeOfObject( piSrc2 ) - 1;
    while( t > 0 )
    {
        t--;

        //if A odd and A < B:
        //    Swap (A, A1) with (B, B1)
        c = 1 & (SymCryptIntGetValueLsbits32( piA ) & SymCryptIntSubSameSize( piA, piB, piTmp ) );
        SymCryptIntConditionalSwap( piA, piB, c );
        SymCryptIntConditionalSwap( piA1, piB1, c );

        //if A odd:
        //    A -= B;     A1 -= B1 (mod S2);
        c = 1 & SymCryptIntGetValueLsbits32( piA );
        SymCryptIntSubSameSize( piA, piB, piTmp );          // Never a carry due to the previous conditional swap
        SymCryptIntConditionalCopy( piTmp, piA, c );

        d = SymCryptIntSubSameSize( piA1, piB1, piTmp );
        SymCryptIntConditionalCopy( piTmp, piA1, c );
        SymCryptIntAddMixedSize( piA1, piSrc2, piTmp );
        SymCryptIntConditionalCopy( piTmp, piA1, c & d );

        // A /= 2;      A1 /= 2 (mod S2);
        SYMCRYPT_ASSERT( (SymCryptIntGetValueLsbits32( piA ) & 1) == 0 );
        SymCryptIntShr1( 0, piA, piA );
        c = SymCryptIntGetValueLsbits32( piA1 ) & 1;
        d = SymCryptIntAddMixedSize( piA1, piSrc2, piTmp );
        SymCryptIntConditionalCopy( piTmp, piA1, c );
        SymCryptIntShr1( c & d, piA1, piA1 );

    }

    // B = GCD, B1 * S1 = GCD (mod S2)
    // A = 0, A1 is scratch
    //
    // Algorithm from here:
    // GCD as divisor
    // LCM = S1 * S2 / GCD.
    // P2 = S2 / GCD, as divisor (only for InvS1ModS2)
    // InvS1ModS2 = B1 mod P2
    // InvS2ModS1 = -((B1*S1 - GCD) div S2) mod S1

    if( piGcd != NULL )
    {
        SymCryptIntCopyMixedSize( piB, piGcd );
    }

    if( piLcm == NULL && piInvSrc1ModSrc2 == NULL && piInvSrc2ModSrc1 == NULL )
    {
        // Only GCD needed; don't do the other work
        goto cleanup;
    }

    SymCryptIntCopyMixedSize( piB, SymCryptIntFromDivisor( pdGcd ) );                // copy into INT of the right size

    // IntToDivisor requirement:
    //      Gcd !=0
    SymCryptIntToDivisor( SymCryptIntFromDivisor( pdGcd ), pdGcd, 3, 0, pbScratch, cbScratch );

    if( piLcm != NULL )
    {
        // LCM = S1 * S2 / GCD
        SymCryptIntMulMixedSize( piSrc1, piSrc2, piLcm, pbScratch, cbScratch );
        SymCryptIntDivMod( piLcm, pdGcd, piLcm, NULL, pbScratch, cbScratch );
    }

    if( piInvSrc1ModSrc2 != NULL )
    {
        // Future optimization: if GCD == 1 then we can just copy B1.
        SymCryptIntDivMod( piSrc2, pdGcd, SymCryptIntFromDivisor( pdTmp ), NULL, pbScratch, cbScratch );

        // IntToDivisor requirement:
        //      Src2 / pdGcd > 0
        SymCryptIntToDivisor( SymCryptIntFromDivisor( pdTmp ), pdTmp, 1, 0, pbScratch, cbScratch );
        SymCryptIntDivMod( piB1, pdTmp, NULL, piInvSrc1ModSrc2, pbScratch, cbScratch );
    }

    if( piInvSrc2ModSrc1 != NULL )
    {
        //  InvS2ModS1 = - ( (B1*S1 - GCD)/S2 ) mod S1

        // S2 as divisor
        SymCryptIntCopyMixedSize( piSrc2, SymCryptIntFromDivisor( pdTmp ) );

        // IntToDivisor requirement:
        //      Src2 is odd --> Src2 != 0
        SymCryptIntToDivisor( SymCryptIntFromDivisor( pdTmp ), pdTmp, 1, 0, pbScratch, cbScratch );

        SymCryptIntMulMixedSize( piB1, piSrc1, piTmpDbl, pbScratch, cbScratch );
        SymCryptIntSubMixedSize( piTmpDbl, piB, piTmpDbl );     // Never a borrow if B1 >= 1
        SymCryptIntDivMod( piTmpDbl, pdTmp, piTmpDbl, NULL, pbScratch, cbScratch );

        // and reduce modulo S1
        SymCryptIntCopyMixedSize( piSrc1, SymCryptIntFromDivisor( pdTmp ) );

        // IntToDivisor requirement:
        //      Src1 > 0
        SymCryptIntToDivisor( SymCryptIntFromDivisor( pdTmp ), pdTmp, 1, 0, pbScratch, cbScratch );
        SymCryptIntDivMod( piTmpDbl, pdTmp, NULL, piInvSrc2ModSrc1, pbScratch, cbScratch );

        // Negative modulo S1
        SymCryptIntSubMixedSize( SymCryptIntFromDivisor( pdTmp ), piInvSrc2ModSrc1, piInvSrc2ModSrc1 );   // Never a borrow as piInvSrc2ModSrc1 < S1
    }

cleanup:
    return; // Need a statement after a label...
}
