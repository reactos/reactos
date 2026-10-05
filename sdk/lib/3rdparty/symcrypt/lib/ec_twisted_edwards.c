//
// ec_twisted_edwards.c   Twisted Edwards Curve Implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsFillScratchSpaces( _In_ PSYMCRYPT_ECURVE pCurve )
{
    UINT32 nDigits = SymCryptDigitsFromBits( pCurve->FModBitsize );
    UINT32 cbModElement = pCurve->cbModElement;
    UINT32 nDigitsFieldLength = pCurve->FModDigits;

    //
    // All the scratch space computations are upper bounded by the SizeofXXX bound (2^19) and
    // the SCRATCH_BYTES_FOR_XXX bound (2^24) (see symcrypt_internal.h).
    //
    // One caveat is SymCryptSizeofEcpointFromCurve and SymCryptSizeofEcpointEx which calculate
    // the size of EcPoint with 4 coordinates (each one a modelement of max size 2^17). Thus upper
    // bounded by 2^20.
    //
    // Another is the precomp points computation where the nPrecompPoints are up to
    // 2^SYMCRYPT_ECURVE_SW_DEF_WINDOW = 2^6 and the nRecodedDigits are equal to the
    // GOrd bitsize < 2^20.
    //
    // Thus cbScratchScalarMulti is upper bounded by 2^6*2^20 + 2*2^20*2^4 ~ 2^26.
    //

    pCurve->cbScratchCommon = 8 * cbModElement + SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits );

    pCurve->cbScratchScalar =
            (pCurve->cbModElement) +
            2 * SymCryptSizeofEcpointFromCurve( pCurve ) +
            2 * SymCryptSizeofIntFromDigits( pCurve->GOrdDigits ) +
            SYMCRYPT_MAX( pCurve->cbScratchCommon, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->GOrdDigits ));

    pCurve->cbScratchScalarMulti =
            pCurve->info.sw.nPrecompPoints * SymCryptSizeofEcpointFromCurve( pCurve ) +
            ((2*pCurve->info.sw.nRecodedDigits * sizeof(UINT32) + SYMCRYPT_ASYM_ALIGN_VALUE - 1 )/SYMCRYPT_ASYM_ALIGN_VALUE) * SYMCRYPT_ASYM_ALIGN_VALUE;

    pCurve->cbScratchGetSetValue =
        SymCryptSizeofEcpointEx(cbModElement, SYMCRYPT_ECPOINT_FORMAT_MAX_LENGTH) +
        2 * cbModElement +
        SYMCRYPT_MAX(SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS(nDigitsFieldLength),
        SYMCRYPT_SCRATCH_BYTES_FOR_MODINV(nDigitsFieldLength));

    pCurve->cbScratchGetSetValue = SYMCRYPT_MAX( pCurve->cbScratchGetSetValue, SymCryptSizeofIntFromDigits( nDigits ) );

    pCurve->cbScratchEckey =
        SYMCRYPT_MAX( pCurve->cbModElement + SymCryptSizeofIntFromDigits(SymCryptEcurveDigitsofScalarMultiplier(pCurve)),
                SymCryptSizeofEcpointFromCurve( pCurve ) ) +
        SYMCRYPT_MAX( pCurve->cbScratchScalar + pCurve->cbScratchScalarMulti, pCurve->cbScratchGetSetValue );
}

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsSetDistinguished(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poDst->pCurve) );

    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptEcpointCopy( pCurve, pCurve->G, poDst );
}

UINT32
SYMCRYPT_CALL
SymCryptTwistedEdwardsIsZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch)
{
    PSYMCRYPT_MODULUS     pmMod = pCurve->FMod;
    UINT32 dResX = 0, dResY = 0;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) );

    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );

    PSYMCRYPT_MODELEMENT peSrcX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc );
    PSYMCRYPT_MODELEMENT peSrcY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc );
    PSYMCRYPT_MODELEMENT peSrcZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc );

    dResX = SymCryptModElementIsZero( pmMod, peSrcX );
    dResY = SymCryptModElementIsEqual( pmMod, peSrcY, peSrcZ );

    return ( dResX & dResY );
}

//
// Verify that
//   a * x^2 + y^2 = 1 + d * x^2 * y^2
//   x = X/Z, y = Y/Z,
// To avoid mod inv calculation which is expensive,
// we verify Z^2(aX^2 + Y^2) = Z^4 + d * X^2 * Y^2
//
UINT32
SYMCRYPT_CALL
SymCryptTwistedEdwardsOnCurve(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch)
{
    PSYMCRYPT_MODELEMENT  peTemp[4];
    PSYMCRYPT_MODULUS     pmMod = pCurve->FMod;
    SIZE_T nBytes;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );

    nBytes = SymCryptSizeofModElementFromModulus( pmMod );

    SYMCRYPT_ASSERT( cbScratch >= 4*nBytes );

    for (UINT32 i = 0; i < 4; ++i)
    {
       peTemp[i] = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
       pbScratch += nBytes;
       cbScratch -= nBytes;
    }

    PSYMCRYPT_MODELEMENT peSrcX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc );
    PSYMCRYPT_MODELEMENT peSrcY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc );
    PSYMCRYPT_MODELEMENT peSrcZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc );

    // peTemp[0] = X^2
    SymCryptModSquare( pmMod, peSrcX, peTemp[0], pbScratch, cbScratch);

    // peTemp[1] = Y^2
    SymCryptModSquare( pmMod, peSrcY, peTemp[1], pbScratch, cbScratch);

    // peTemp[2] = Z^2
    SymCryptModSquare( pmMod, peSrcZ, peTemp[2], pbScratch, cbScratch);

    // peTemp[3] = a * X^2
    SymCryptModMul( pmMod, pCurve->A, peTemp[0], peTemp[3], pbScratch, cbScratch );

    // peTemp[3] = a * X^2 + Y^2
    SymCryptModAdd( pmMod, peTemp[3], peTemp[1], peTemp[3], pbScratch, cbScratch );

    // peTemp[3] = Z^2 (a * X^2 + Y^2)
    SymCryptModMul( pmMod, peTemp[3], peTemp[2], peTemp[3], pbScratch, cbScratch );

    // peTemp[1] = X^2 * Y^2
    SymCryptModMul( pmMod, peTemp[0], peTemp[1], peTemp[1], pbScratch, cbScratch );

    // peTemp[1] = d * X^2 *Y^2
    SymCryptModMul( pmMod, pCurve->B, peTemp[1], peTemp[1], pbScratch, cbScratch );

    // peTemp[2] = Z^4
    SymCryptModMul( pmMod, peTemp[2], peTemp[2], peTemp[2], pbScratch, cbScratch );

    // peTemp[1] = Z^4 + d * X^2 * Y^2
    SymCryptModAdd( pmMod, peTemp[2], peTemp[1], peTemp[1], pbScratch, cbScratch );

    return SymCryptModElementIsEqual( pmMod, peTemp[1], peTemp[3] );
}

//
//  Point doubling: dbl-2008-hwcd, 5Mul + 4Square + 2Add + 5Sub
//
//  poDst (X, Y, Z, T) = 2 * poSrc(X, Y, Z, T)
//  1.  A = X1 ^ 2
//  2.  B = Y1 ^ 2
//  3.  C = 2 * Z1 ^ 2
//  4.  D = a * A
//  5.  E = (X1 + Y1) ^ 2 - A - B
//  6.  G = D + B
//  7.  F = G - C
//  8.  H = D - B
//  9.  X3 = E * F
//  10. Y3 = G * H
//  11. T3 = E * H
//  12. Z3 = F * G
//
VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsDouble(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch)
{
    PSYMCRYPT_MODELEMENT  peTemp[8];
    PSYMCRYPT_MODULUS     pmMod = pCurve->FMod;
    SIZE_T nBytes;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );

    UNREFERENCED_PARAMETER( flags );

    nBytes = SymCryptSizeofModElementFromModulus( pmMod );

    SYMCRYPT_ASSERT( cbScratch >= 8*nBytes );

    for (UINT32 i = 0; i < 8; ++i)
    {
       peTemp[i] = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
       pbScratch += nBytes;
       cbScratch -= nBytes;
    }

    PSYMCRYPT_MODELEMENT peSrcX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc );
    PSYMCRYPT_MODELEMENT peSrcY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc );
    PSYMCRYPT_MODELEMENT peSrcZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc );

    PSYMCRYPT_MODELEMENT peDstX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstT = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 3, pCurve, poDst );

    PSYMCRYPT_MODELEMENT peA = peTemp[0];
    PSYMCRYPT_MODELEMENT peB = peTemp[1];
    PSYMCRYPT_MODELEMENT peC = peTemp[2];
    PSYMCRYPT_MODELEMENT peD = peTemp[3];
    PSYMCRYPT_MODELEMENT peE = peTemp[4];
    PSYMCRYPT_MODELEMENT peF = peTemp[5];
    PSYMCRYPT_MODELEMENT peG = peTemp[6];
    PSYMCRYPT_MODELEMENT peH = peTemp[7];


    // A = X1^2
    SymCryptModSquare( pmMod, peSrcX, peA, pbScratch, cbScratch );

    // B = Y1^2
    SymCryptModSquare( pmMod, peSrcY, peB, pbScratch, cbScratch );

    // C1 = Z1^2
    SymCryptModSquare( pmMod, peSrcZ, peC, pbScratch, cbScratch );

    // C = C1 + C1 = Z1^2 + Z1^2 = 2 * Z1^2
    SymCryptModAdd( pmMod, peC, peC, peC, pbScratch, cbScratch );

    // D = a * A
    SymCryptModMul( pmMod, pCurve->A, peA, peD, pbScratch, cbScratch );

    // E1 = X1 + Y1
    SymCryptModAdd( pmMod, peSrcX, peSrcY, peE, pbScratch, cbScratch );

    // E2 = E1^2 = (X1 + Y1)^2
    SymCryptModSquare( pmMod, peE, peE, pbScratch, cbScratch );

    // E3 = E2 - A = (X1 + Y1)^2 - A
    SymCryptModSub( pmMod, peE, peA, peE, pbScratch, cbScratch );

    // E = E3 - B = (X1 + Y1)^2 - A - B
    SymCryptModSub( pmMod, peE, peB, peE, pbScratch, cbScratch );

    // G = D + B
    SymCryptModAdd( pmMod, peD, peB, peG, pbScratch, cbScratch );

    // F = G - C
    SymCryptModSub( pmMod, peG, peC, peF, pbScratch, cbScratch );

    // H = D - B
    SymCryptModSub( pmMod, peD, peB, peH, pbScratch, cbScratch );

    // X3 = E * F
    SymCryptModMul( pmMod, peE, peF, peDstX, pbScratch, cbScratch );

    // Y3 = G * H
    SymCryptModMul( pmMod, peG, peH, peDstY, pbScratch, cbScratch );

    // T3 = E * H
    SymCryptModMul( pmMod, peE, peH, peDstT, pbScratch, cbScratch );

    // Z3 = F * G
    SymCryptModMul( pmMod, peF, peG, peDstZ, pbScratch, cbScratch );
}


//
//  Point addition: add-2008-hwcd  11Mul + 3add + 4sub
//
//  poDst(X, Y, Z, T) = poSrc(X, Y, Z, T) + poSrc2(X, Y, Z, T)
//  1.  A = X1 * X2
//  2.  B = Y1 * Y2
//  3.  C = d * T1 * T2
//  4.  D = Z1 * Z2
//  5.  E = (X1 + Y1) * (X2 + Y2) - A - B
//  6.  F = D - C
//  7.  G = D + C
//  8.  H = B - a * A
//  9.  X3 = E * F
//  10. Y3 = G * H
//  11. T3 = E * H
//  12. Z3 = F * G
//
VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsAdd(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PSYMCRYPT_MODELEMENT  peTemp[8];
    PSYMCRYPT_MODULUS     pmMod = pCurve->FMod;
    SIZE_T nBytes;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc1->pCurve) && SymCryptEcurveIsSame(pCurve, poSrc2->pCurve) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );

    UNREFERENCED_PARAMETER( flags );

    nBytes = SymCryptSizeofModElementFromModulus( pmMod );

    SYMCRYPT_ASSERT( cbScratch >= 8*nBytes );

    for (UINT32 i = 0; i < 8; ++i)
    {
       peTemp[i] = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
       pbScratch += nBytes;
       cbScratch -= nBytes;
    }

    PSYMCRYPT_MODELEMENT peSrc1X = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc1 );
    PSYMCRYPT_MODELEMENT peSrc1Y = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc1 );
    PSYMCRYPT_MODELEMENT peSrc1Z = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc1 );
    PSYMCRYPT_MODELEMENT peSrc1T = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 3, pCurve, poSrc1 );

    PSYMCRYPT_MODELEMENT peSrc2X = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc2 );
    PSYMCRYPT_MODELEMENT peSrc2Y = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc2 );
    PSYMCRYPT_MODELEMENT peSrc2Z = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc2 );
    PSYMCRYPT_MODELEMENT peSrc2T = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 3, pCurve, poSrc2 );

    PSYMCRYPT_MODELEMENT peDstX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstT = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 3, pCurve, poDst );

    PSYMCRYPT_MODELEMENT peA = peTemp[0];
    PSYMCRYPT_MODELEMENT peB = peTemp[1];
    PSYMCRYPT_MODELEMENT peC = peTemp[2];
    PSYMCRYPT_MODELEMENT peD = peTemp[3];
    PSYMCRYPT_MODELEMENT peE = peTemp[4];
    PSYMCRYPT_MODELEMENT peF = peTemp[5];
    PSYMCRYPT_MODELEMENT peG = peTemp[6];
    PSYMCRYPT_MODELEMENT peH = peTemp[7];

    // A = X1 * X2
    SymCryptModMul( pmMod, peSrc1X, peSrc2X, peA, pbScratch, cbScratch );

    // B = Y1 * Y2
    SymCryptModMul( pmMod, peSrc1Y, peSrc2Y, peB, pbScratch, cbScratch );

    // C1 = T1 * T2
    SymCryptModMul( pmMod, peSrc1T, peSrc2T, peC, pbScratch, cbScratch );

    // C = d * C1 = d * T1 * T2
    SymCryptModMul( pmMod, pCurve->B, peC, peC, pbScratch, cbScratch );

    // D = Z1 * Z2
    SymCryptModMul( pmMod, peSrc1Z, peSrc2Z, peD, pbScratch, cbScratch );

    // E1 = X1 + Y1
    SymCryptModAdd( pmMod, peSrc1X, peSrc1Y, peE, pbScratch, cbScratch );

    // E2 = X2 + Y2
    SymCryptModAdd( pmMod, peSrc2X, peSrc2Y, peF, pbScratch, cbScratch );

    // E = E * F
    SymCryptModMul( pmMod, peE, peF, peE, pbScratch, cbScratch );

    // E = E - A
    SymCryptModSub( pmMod, peE, peA, peE, pbScratch, cbScratch );

    // E = E - B
    SymCryptModSub( pmMod, peE, peB, peE, pbScratch, cbScratch );

    // F = D - C
    SymCryptModSub( pmMod, peD, peC, peF, pbScratch, cbScratch );

    // G = D + C
    SymCryptModAdd( pmMod, peD, peC, peG, pbScratch, cbScratch );

    // H = a * A
    SymCryptModMul( pmMod, pCurve->A, peA, peH, pbScratch, cbScratch );

    // H = B - a * A
    SymCryptModSub( pmMod, peB, peH, peH, pbScratch, cbScratch );

    // X3 = E * F
    SymCryptModMul( pmMod, peE, peF, peDstX, pbScratch, cbScratch );

    // Y3 = G * H
    SymCryptModMul( pmMod, peG, peH, peDstY, pbScratch, cbScratch );

    // T3 = E * H
    SymCryptModMul( pmMod, peE, peH, peDstT, pbScratch, cbScratch );

    // Y3 = F * G
    SymCryptModMul( pmMod, peF, peG, peDstZ, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsAddDiffNonZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_(cbScratch)
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SymCryptTwistedEdwardsAdd( pCurve, poSrc1, poSrc2, poDst, 0, pbScratch, cbScratch );
}

//
//  Verify poSrc1(X1, Y1, Z1, T1) = poSrc2(X2, Y2, Z2, T2)
//  To avoid ModInv for 1/Z, we do
//     X1 * Z2 = X2 * Z1, and
//     Y1 * Z2 = Y2 * Z1
//
//  This function also do poSrc1 = -1 * poSrc check as flags indicates
//
UINT32
SYMCRYPT_CALL
SymCryptTwistedEdwardsIsEqual(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_(cbScratch)
            PBYTE               pbScratch,
            SIZE_T              cbScratch)
{
    PSYMCRYPT_MODELEMENT  peTemp[2];
    PSYMCRYPT_MODELEMENT  peSrc1X, peSrc1Y, peSrc1Z;
    PSYMCRYPT_MODELEMENT  peSrc2X, peSrc2Y, peSrc2Z;
    PSYMCRYPT_MODULUS     pmMod = pCurve->FMod;
    SIZE_T nBytes;
    UINT32  dResX = 0;
    UINT32  dResXN = 0;
    UINT32  dResY = 0;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc1->pCurve) && SymCryptEcurveIsSame(pCurve, poSrc2->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );

    nBytes = SymCryptSizeofModElementFromModulus( pmMod );

    SYMCRYPT_ASSERT( cbScratch >= 2*nBytes );

    for (UINT32 i = 0; i < 2; ++i)
    {
        peTemp[i] = SymCryptModElementCreate( pbScratch, nBytes, pmMod );
        pbScratch += nBytes;
        cbScratch -= nBytes;
    }

    peSrc1X = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc1 );
    peSrc1Y = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc1 );
    peSrc1Z = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc1 );

    peSrc2X = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc2 );
    peSrc2Y = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc2 );
    peSrc2Z = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc2 );

    // Setting the default flag if flags == 0
    flags |= (SYMCRYPT_MASK32_ZERO(flags) & SYMCRYPT_FLAG_ECPOINT_EQUAL);

    // peTemp[0] = X1 * Z2
    SymCryptModMul( pmMod, peSrc1X, peSrc2Z, peTemp[0], pbScratch, cbScratch );

    // peTemp[1] = X2 * Z1
    SymCryptModMul( pmMod, peSrc2X, peSrc1Z, peTemp[1], pbScratch, cbScratch );

    dResX = SymCryptModElementIsEqual( pmMod, peTemp[0], peTemp[1] );

    // Neg peTemp[1]
    SymCryptModNeg(pmMod, peTemp[1], peTemp[1], pbScratch, cbScratch);
    dResXN = SymCryptModElementIsEqual(pmMod, peTemp[0], peTemp[1]);

    // peTemp[0] = Y1 * Z2
    SymCryptModMul( pmMod, peSrc1Y, peSrc2Z, peTemp[0], pbScratch, cbScratch );

    // peTemp[1] = Y2 * Z1
    SymCryptModMul( pmMod, peSrc2Y, peSrc1Z, peTemp[1], pbScratch, cbScratch );

    dResY = SymCryptModElementIsEqual( pmMod, peTemp[0], peTemp[1] );

    return (SYMCRYPT_MASK32_NONZERO( flags & SYMCRYPT_FLAG_ECPOINT_EQUAL ) & dResX & dResY ) |
           (SYMCRYPT_MASK32_NONZERO( flags & SYMCRYPT_FLAG_ECPOINT_NEG_EQUAL ) & dResXN & dResY );
}

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsSetZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_(cbScratch)
            PBYTE               pbScratch,
            SIZE_T              cbScratch)
{
    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );

    PSYMCRYPT_MODULUS  pmMod = pCurve->FMod;

    PSYMCRYPT_MODELEMENT peDstX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst );
    PSYMCRYPT_MODELEMENT peDstT = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 3, pCurve, poDst );

    SymCryptModElementSetValueUint32( 0, pmMod, peDstX, pbScratch, cbScratch );
    SymCryptModElementSetValueUint32( 1, pmMod, peDstY, pbScratch, cbScratch );
    SymCryptModElementSetValueUint32( 1, pmMod, peDstZ, pbScratch, cbScratch );
    SymCryptModElementSetValueUint32( 0, pmMod, peDstT, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsNegate(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Inout_ PSYMCRYPT_ECPOINT   poSrc,
            UINT32              mask,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;
    PSYMCRYPT_MODELEMENT peX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0,pCurve, poSrc);
    PSYMCRYPT_MODELEMENT peT = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 3,pCurve, poSrc);

    PSYMCRYPT_MODELEMENT peTmp = NULL;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) + pCurve->cbModElement );

    peTmp = SymCryptModElementCreate(
                pbScratch,
                pCurve->cbModElement,
                FMod );
    SYMCRYPT_ASSERT( peTmp != NULL);

    pbScratch += pCurve->cbModElement;
    cbScratch -= pCurve->cbModElement;

    SymCryptModNeg( FMod, peX, peTmp, pbScratch, cbScratch );
    SymCryptModElementMaskedCopy( FMod, peTmp, peX, mask );

    SymCryptModNeg( FMod, peT, peTmp, pbScratch, cbScratch );
    SymCryptModElementMaskedCopy( FMod, peTmp, peT, mask );
}
