//
// ec_montgomery.c   Montgomery Implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

VOID
SYMCRYPT_CALL
SymCryptMontgomeryFillScratchSpaces(_In_ PSYMCRYPT_ECURVE pCurve)
{
    UINT32 nDigits = SymCryptDigitsFromBits( pCurve->FModBitsize );
    UINT32 nBytes = SymCryptSizeofModElementFromModulus( pCurve->FMod );
    UINT32 nCommon = SYMCRYPT_MAX( SymCryptSizeofIntFromDigits( nDigits ), SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ), SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( nDigits ) ) );
    UINT32 cbModElement = pCurve->cbModElement;
    UINT32 nDigitsFieldLength = pCurve->FModDigits;

    //
    // All the scratch space computations are upper bounded by the SizeofXXX bound (2^19) and
    // the SCRATCH_BYTES_FOR_XXX bound (2^24) (see symcrypt_internal.h).
    //
    // One caveat is SymCryptSizeofEcpointFromCurve and SymCryptSizeofEcpointEx which calculate the
    // size of EcPoint with 4 coordinates (each one a modelement of max size 2^17). Thus upper
    // bounded by 2^20.
    //

    pCurve->cbScratchCommon = nCommon;
    pCurve->cbScratchScalar =
        SymCryptSizeofIntFromDigits(nDigits) +
        6 * nBytes +
        nCommon;

    pCurve->cbScratchScalarMulti = 0;
    pCurve->cbScratchGetSetValue =
        SymCryptSizeofEcpointEx( cbModElement, SYMCRYPT_ECPOINT_FORMAT_MAX_LENGTH ) +
        2 * cbModElement +
        SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsFieldLength ),
             SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( nDigitsFieldLength ) );

    pCurve->cbScratchGetSetValue = SYMCRYPT_MAX( pCurve->cbScratchGetSetValue, SymCryptSizeofIntFromDigits( nDigits ) );

    pCurve->cbScratchEckey =
        SYMCRYPT_MAX( cbModElement + SymCryptSizeofIntFromDigits(SymCryptEcurveDigitsofScalarMultiplier(pCurve)),
            SymCryptSizeofEcpointFromCurve( pCurve ) ) +
        SYMCRYPT_MAX( pCurve->cbScratchScalar, pCurve->cbScratchGetSetValue );
}

VOID
SYMCRYPT_CALL
SymCryptMontgomerySetDistinguished(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_MONTGOMERY_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poDst->pCurve) );

    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptEcpointCopy( pCurve, pCurve->G, poDst );
}

//
//  Verify poSrc1(X1, Z1) = poSrc2(X2, Z2)
//  To avoid ModInv for 1/Z, we do
//     X1 * Z2 = X2 * Z1
//
//  This function currently ignores the flags parameter as there is no distinction between equal and
//  negative equal case in Single Projective Coordinates used in Montgomery curves. We accept the flags
//  to maintain the same API as for other curves.
//
UINT32
SYMCRYPT_CALL
SymCryptMontgomeryIsEqual(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch)
{
    PSYMCRYPT_MODELEMENT  peTemp[2];
    PSYMCRYPT_MODELEMENT  peSrc1X, peSrc1Z;
    PSYMCRYPT_MODELEMENT  peSrc2X, peSrc2Z;
    PSYMCRYPT_MODULUS     pmMod = pCurve->FMod;
    SIZE_T nBytes;

    SYMCRYPT_ASSERT( (flags & ~(SYMCRYPT_FLAG_ECPOINT_EQUAL|SYMCRYPT_FLAG_ECPOINT_NEG_EQUAL)) == 0 );
    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_MONTGOMERY_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc1->pCurve) && SymCryptEcurveIsSame(pCurve, poSrc2->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );

    UNREFERENCED_PARAMETER( flags );

    nBytes = SymCryptSizeofModElementFromModulus( pmMod );

    SYMCRYPT_ASSERT( cbScratch >= 2 * nBytes );

    for (UINT32 i = 0; i < 2; ++i)
    {
        peTemp[i] = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
        pbScratch += nBytes;
        cbScratch -= nBytes;
    }

    peSrc1X = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc1 );
    peSrc1Z = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc1 );

    peSrc2X = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc2 );
    peSrc2Z = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc2 );

    // peTemp[0] = X1 * Z2
    SymCryptModMul( pmMod, peSrc1X, peSrc2Z, peTemp[0], pbScratch, cbScratch );

    // peTemp[1] = X2 * Z1
    SymCryptModMul( pmMod, peSrc2X, peSrc1Z, peTemp[1], pbScratch, cbScratch );

    return SymCryptModElementIsEqual( pmMod, peTemp[0], peTemp[1] );
}

UINT32
SYMCRYPT_CALL
SymCryptMontgomeryIsZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;
    PSYMCRYPT_MODELEMENT peZ = NULL;    // Pointer to Z

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_MONTGOMERY_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) );

    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );

    // Getting pointer to Z of the source point
    peZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1,  pCurve, poSrc );

    return SymCryptModElementIsZero( FMod, peZ );
}

VOID
SymCryptMontgomeryDoubleAndAdd(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peX1,
    _In_opt_                        PCSYMCRYPT_MODELEMENT   peZ1,
    _In_                            PCSYMCRYPT_MODELEMENT   peA24,
    _Inout_                         PSYMCRYPT_MODELEMENT    peX2,
    _Inout_                         PSYMCRYPT_MODELEMENT    peZ2,
    _Inout_                         PSYMCRYPT_MODELEMENT    peX3,
    _Inout_                         PSYMCRYPT_MODELEMENT    peZ3,
    _Inout_                         PSYMCRYPT_MODELEMENT    peTemp1,
    _Inout_                         PSYMCRYPT_MODELEMENT    peTemp2,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch)
/*
We use the notation of ladd-1987-m-3, this is a generic Montgomery ladder implementation.
This is similar to RFC7748 for TLS use of curve25519, however, unlike in the RFC, we support the case when Z1 != 1.

When it is statically known that Z1 == 1 the caller can set peZ1 to NULL to skip one redundant modular multiplication.
   Note that this will be revealed through timing, so peZ1 can only be set to NULL it is not secret that Z1 == 1.
   Z1 == 1 is statically known for points which have just been imported into SymCrypt (and for the distinguished point of the
   curve), and this knowledge is tracked in an ecPoint's normalized flag.

The (X,Z) values represent an x-coordinate (X/Z) but it avoids the modular division.

The value a24 is such that 4*a24 = a+2 where a is one of the Montgomery curve parameters.
Thus, a24 = (a+2)/4. For curve25519, A = 486662, so a24 = 121666 (=0x01db42)

Algorithm (ladd-1987-m-3), with all operations expanded
   A  = X2 + Z2
   AA = A^2
   B  = X2 - Z2
   BB = B^2
   E  = AA - BB
   C  = X3 + Z3
   D  = X3 - Z3
   DA = D * A
   CB = C * B
   X5 = (DA + CB)^2
        DApCB = DA + CB
        X5 = DApCB^2
   if peZ1 != NULL:
        X5 = Z1 * X5
   Z5 = X1 * (DA - CB)^2
        DAmCB = DA - CB
        DAmCB2 = DAmCB ^ 2
        Z5 = X1 * DAmCB2
   X4 = AA * BB
   Z4 = E * (BB + a24 * E)
        A24E = A24 * E
        BAE = BB + A24 * E
        Z4 = E * BAE

If we write a = (X2,Z2) and b = (X3,Z3), and a-b = (X1,Z1), then this algorithm computes
(2*a) and (a+b) into (X4, Z4) and (X5,Z5) respectively.
The Montgomery ladder uses this as follows:
- Store xP and (x+1)P
- To process a 0 bit in the scalar, apply the DoubleAndAdd to (xP,(x+1)P) to get (2xP, (2x+1)P)
- To process a 1 bit in the scalar, apply the DoubleAndAdd to ((x+1)P, xP) to get ((2x+2)P, (2x+1)P)
This updates the state to either (2xP, (2x+1)P) or to ((2x+1)P, (2x+2)P) and corresponds to updating
x to either 2x or 2x+1.

The starting value is (0,P), represented as ((1,0),(P_x,P_z)
The algorithm above, when applied to (1, 0, X, Z) produces:
    A = 1, AA = 1, B = 1, BB = 1, E = 0,
    C = X+Z, D = X-Z, DA = X-Z, CB = X+Z,
    X5 = 4(X^2)Z, Z5 = 4X(Z^2)
    X4 = 1, Z4 = 0
for an output of (1, 0, 4(X^2)Z, 4X(Z^2))
But (4(X^2)Z, 4X(Z^2)) is just another representation of (X,Z) as only the quotient of the two numbers is significant.
So even if an exponent starts with a bunch of 0 bits, the DoubleAndAdd-based function computes the right result in constant time.

*/
{
    // Temp1 =          A = X2 + Z2
    SymCryptModAdd( pmMod, peX2, peZ2, peTemp1, pbScratch, cbScratch );

    // Z2 =             B = X2 - Z2
    SymCryptModSub( pmMod, peX2, peZ2, peZ2, pbScratch, cbScratch );

    // Temp2 =          C = X3 + Z3
    SymCryptModAdd( pmMod, peX3, peZ3, peTemp2, pbScratch, cbScratch );

    // Z3 =             D = X3 - Z3
    SymCryptModSub( pmMod, peX3, peZ3, peZ3, pbScratch, cbScratch );

    // X3 =             CB = C * B      = Temp2 * Z2
    SymCryptModMul( pmMod, peTemp2, peZ2, peX3, pbScratch, cbScratch );

    // Z3 =             DA = D * A      = Z3 * Temp1
    SymCryptModMul( pmMod, peZ3, peTemp1, peZ3, pbScratch, cbScratch );

    // From this point on, the outputs (X5,Z5) depend only on (X3,Z3) and (X1,Z1)
    // and the outputs (X4,Z4) only on (Temp1,Z2) and A24
    // We'll do the (X4,Z4) first

    // X2 =             AA = A * A      = Temp1 * Temp1
    SymCryptModSquare( pmMod, peTemp1, peX2, pbScratch, cbScratch );

    // Temp1 =          BB = B * B      = Z2 * Z2
    SymCryptModSquare( pmMod, peZ2, peTemp1, pbScratch, cbScratch );

    // Temp2 =          E = AA - BB     = X2 - Temp1
    SymCryptModSub( pmMod, peX2, peTemp1, peTemp2, pbScratch, cbScratch );

    // X2 =             X4 = AA * BB    = X2 * Temp1
    SymCryptModMul( pmMod, peX2, peTemp1, peX2, pbScratch, cbScratch );

    // Z2 =             A24E = A24 * E    = A24 * Temp2
    SymCryptModMul( pmMod, peA24, peTemp2, peZ2, pbScratch, cbScratch );

    // Z2 =             BAE = (BB + a24 * E) = BB + A24E = Temp1 + Z2
    SymCryptModAdd( pmMod, peTemp1, peZ2, peZ2, pbScratch, cbScratch );

    // Z2 =             Z4 = E * BAE        = Temp2 + Z2
    SymCryptModMul( pmMod, peTemp2, peZ2, peZ2, pbScratch, cbScratch );

    // Now we compute (X5, Z5)

    // Temp1 =          DApCB = DA + CB     = Z3 + X3
    SymCryptModAdd( pmMod, peZ3, peX3, peTemp1, pbScratch, cbScratch );

    // Z3 =             DAmCB = DA - CB     = Z3 - X3
    SymCryptModSub( pmMod, peZ3, peX3, peZ3, pbScratch, cbScratch );

    // X3 =             DApCB^2 = Temp1 ^ 2 ( = X5 when (peZ1 == NULL) => Z1 == 1)
    SymCryptModSquare( pmMod, peTemp1, peX3, pbScratch, cbScratch );

    if (peZ1 != NULL) // source point is not normalized
    {
    // X3 =             X5 = Z1 * DApCB^2   = Z1 * X3
    SymCryptModMul( pmMod, peZ1, peX3, peX3, pbScratch, cbScratch );
    }

    // Z3 =             DAmCB2 = DAmCB ^ 2  = Z3 ^ 2
    SymCryptModSquare( pmMod, peZ3, peZ3, pbScratch, cbScratch );

    // Z3 =             Z5 = X1 * DAmCB2        = X1 * Z3
    SymCryptModMul( pmMod, peX1, peZ3, peZ3, pbScratch, cbScratch );
}

//
// Montgomery point multiplication only works on X-coordinates.
// We ignore the Y-coordinates.
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMontgomeryPointScalarMul(
    _In_    PCSYMCRYPT_ECURVE      pCurve,
    _In_    PCSYMCRYPT_INT         piScalar,
    _In_opt_
            PCSYMCRYPT_ECPOINT     poSrc,
            UINT32                 flags,
    _Out_   PSYMCRYPT_ECPOINT      poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE                  pbScratch,
            SIZE_T                 cbScratch)
{
    SYMCRYPT_ERROR        scError = SYMCRYPT_NO_ERROR;

    PSYMCRYPT_MODULUS     pmMod;
    PSYMCRYPT_MODELEMENT  peX1, peZ1, peA24, peX2, peZ2, peX3, peZ3, peTemp1, peTemp2, peResult;
    UINT32                i, nBytes, nDigits, cond, newcond, nCommon;
    PBYTE                 pBegin;
    SIZE_T                cbAllScratch;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_MONTGOMERY_TYPE(pCurve) );
    SYMCRYPT_ASSERT( (poSrc == NULL || SymCryptEcurveIsSame(pCurve, poSrc->pCurve)) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );

    // Make sure we only specify the correct flags
    if ((flags & ~SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL) != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (poSrc == NULL)
    {
        poSrc = pCurve->G;
    }

    //
    // Set up structure for X2, Z2, X3, Z3, Temp1, and Temp2, and the scratch space.
    //
    pmMod = pCurve->FMod;

    nDigits = SymCryptDigitsFromBits( pCurve->FModBitsize );
    nBytes = SymCryptSizeofModElementFromModulus( pmMod );
    nCommon = SYMCRYPT_MAX( SymCryptSizeofIntFromDigits(nDigits), SYMCRYPT_MAX(SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS(nDigits), SYMCRYPT_SCRATCH_BYTES_FOR_MODINV(nDigits)));

    SYMCRYPT_ASSERT( cbScratch >= 6 * nBytes + nCommon );

    cbAllScratch = cbScratch;
    pBegin = pbScratch;

    //
    // Create mod elements
    //
    peX2 = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;

    peZ2 = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;

    peX3 = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;

    peZ3 = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;

    peTemp1 = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;

    peTemp2 = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
    pbScratch += nBytes;

    cbScratch = nCommon;

    //
    // Set up values
    //

    peA24 = pCurve->A;

    // X1 = X, Z1 = Z
    peX1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc);
    peZ1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc);

    // X2 = 1, Z2 = 0, X3 = X, Z3 = Z
    SymCryptModElementSetValueUint32( 1, pmMod, peX2, pbScratch, cbScratch );
    SymCryptModElementSetValueUint32( 0, pmMod, peZ2, pbScratch, cbScratch );
    SymCryptModElementCopy( pmMod, peX1, peX3 );
    SymCryptModElementCopy( pmMod, peZ1, peZ3 );

    if ( poSrc->normalized )
    {
        // Set peZ1 to NULL to avoid redundant multiplications in SymCryptMontgomeryDoubleAndAdd
        peZ1 = NULL;
    }

    //
    //  Montgomery ladder scalar multiplication
    //

    i = (pCurve->GOrdBitsize + pCurve->coFactorPower);
    cond = 0;
    while ( i != 0 )
    {
        // If cond = 0, we have (X2, Z2, X3, Z3)
        // if cond = 1, we have (X3, Z3, X2, Z2)
        i--;
        newcond = SymCryptIntGetBit( piScalar, i );
        cond ^= newcond;

        SymCryptModElementConditionalSwap( pmMod, peX2, peX3, cond);
        SymCryptModElementConditionalSwap( pmMod, peZ2, peZ3, cond);

        cond = newcond;

        SymCryptMontgomeryDoubleAndAdd( pmMod, peX1, peZ1, peA24, peX2, peZ2, peX3, peZ3, peTemp1, peTemp2, pbScratch, cbScratch );
    }

    // Now put them back in the normal order
    SymCryptModElementConditionalSwap( pmMod, peX2, peX3, cond);
    SymCryptModElementConditionalSwap( pmMod, peZ2, peZ3, cond);

    // Multiply by the cofactor (if needed) by continuing the doubling
    if ((flags & SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL) != 0)
    {
        i = pCurve->coFactorPower;
        while (i!=0)
        {
            i--;
            // We only use the doubling output here, so we definitely don't need to provide Z1
            // We could refactor to have a separate SymCryptMontgomeryDouble function but for Curve25519 this loop is ~1% of runtime
            SymCryptMontgomeryDoubleAndAdd( pmMod, peX1, NULL, peA24, peX2, peZ2, peX3, peZ3, peTemp1, peTemp2, pbScratch, cbScratch );
        }
    }

    // Set X coordinate
    peResult = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst);
    SymCryptModElementCopy( pCurve->FMod, peX2, peResult );

    // Set Z coordinate
    peResult = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst);
    SymCryptModElementCopy( pCurve->FMod, peZ2, peResult );

    poDst->normalized = FALSE;

    scError = SYMCRYPT_NO_ERROR;

cleanup:
    return scError;
}
