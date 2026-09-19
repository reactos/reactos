//
// ec_short_weierstrass.c   ECPOINT functions for short Weierstrass curves.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

//
// Scratch space requirements for each ECPOINT function.
//
// A function's requirements in scratch space consist of requirements for its own arithmetic
// operations and temporaries ("self" scratch space) and scratch space requirements for other
// ECPOINT functions it might call ("callee" scratch space).
//
// If the outer function does not need the temporaries after calling the inner ECPOINT
// function, then the total scratch space can be the maximum of both. Otherwise the scratch
// space of the outer function should be the concatenation of the "self" scratch space and
// the "callee" scratch space.
//
// The following table shows the scratch space requirements of each function with appropriate
// abbreviations. The calling sequence implies a directed graph that starting from the "leaves"
// (functions that do no call others) allows to calculate the total scratch space requirements.
//
//  #N Function                     Calls Function  Self Temporaries            Self Scratch
//   1 SetZero                      -               0                           COM_MOD(FMod)
//   2 SetDistinguishedPoint        -               0                           0
//   3 IsEqual                      -               4 ModEl                     COM_MOD(FMod)
//   4 IsZero                       -               1 ModEl                     COM_MOD(FMod)
//   5 OnCurve                      -               2 ModEl                     COM_MOD(FMod)
//   6 Double                       1,4,9           2 Ecp                       0
//   7 Add                          1,3,4,8,9       2 Ecp                       0
//   8 AddDiffNonZero               -               8 ModEl                     COM_MOD(FMod)
//   9 Double                       -               6 ModEl                     COM_MOD(FMod)
//
//  10 SetRandom                    11              0                           COM_MOD(GOrd)
//  11 ScalarMul                    4,5,7           1ModEl + (n+2)Ecp + 2Int    COM_MOD(GOrd)
//
// Since only 4 functions call others and to keep things simple, we will have 2
// types of scratch space: "ECURVE_COMMON" and "ECURVE_SCALAR"
//
// ---- All functions except 10 and 11 will use the "ECURVE_COMMON" scratch space. The size of it
//      depends only on parameters of the curve. Schematically it will be:
//          |----------COMMON------------------------------------------------------------------|
//          |------8 ModEl + 2 Ecpoint----||------COM_MOD(FMod)--------------------------------|
//
// ---- The SetRandom and ScalarMul have requirements that depend on temporaries for the pre-computation.
//      Also they depend on the "self" temporaries after calling the inner functions.
//      Therefore, these will require the "ECURVE_SCALAR" scratch space which
//      consists of two parts: The self space for the above two functions and the
//      common scratch space. These parts SHOULD NOT overlap. Schematically:
//
//          |--------------SCALAR---------------------------------------------------|
//          |----1ModEl + (n+2)Ecp + 2Int--------||---max(COMMON, COM_MOD(GOrd)----|

// The scratch space sizes are all calculated by the following function.
// *** Notice that almost all the curve parameters (exception is the distinguished point)
//     must have been initialized before calling this function.
VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassFillScratchSpaces( _In_ PSYMCRYPT_ECURVE pCurve )
{
    UINT32 nDigits = SymCryptDigitsFromBits( pCurve->FModBitsize );

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

    // Common
    pCurve->cbScratchCommon =
            8 * pCurve->cbModElement +
            2 * SymCryptSizeofEcpointFromCurve( pCurve ) +
            SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits );

    // Scalar (Overhead)
    pCurve->cbScratchScalar =
            pCurve->cbModElement +
            2 * SymCryptSizeofEcpointFromCurve( pCurve ) +
            2 * SymCryptSizeofIntFromDigits( pCurve->GOrdDigits ) +
            SYMCRYPT_MAX( pCurve->cbScratchCommon, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->GOrdDigits ));

    // Scalar dependent on precomp points (be careful to align the UINT32 arrays properly)
    pCurve->cbScratchScalarMulti =
            pCurve->info.sw.nPrecompPoints * SymCryptSizeofEcpointFromCurve( pCurve ) +
            ((2*pCurve->info.sw.nRecodedDigits * sizeof(UINT32) + SYMCRYPT_ASYM_ALIGN_VALUE - 1 )/SYMCRYPT_ASYM_ALIGN_VALUE) * SYMCRYPT_ASYM_ALIGN_VALUE;

    // GetSetValue
    pCurve->cbScratchGetSetValue =
            SymCryptSizeofEcpointEx( pCurve->cbModElement, SYMCRYPT_ECPOINT_FORMAT_MAX_LENGTH ) +
            2 * pCurve->cbModElement +
            SYMCRYPT_MAX(    SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ),
                    SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( pCurve->FModDigits ) );

    pCurve->cbScratchGetSetValue = SYMCRYPT_MAX( pCurve->cbScratchGetSetValue, SymCryptSizeofIntFromDigits( nDigits ) );

    // Eckey
    pCurve->cbScratchEckey =
            SYMCRYPT_MAX( pCurve->cbModElement + SymCryptSizeofIntFromDigits(SymCryptEcurveDigitsofScalarMultiplier(pCurve)),
                SymCryptSizeofEcpointFromCurve( pCurve ) ) +
            SYMCRYPT_MAX( pCurve->cbScratchScalar + pCurve->cbScratchScalarMulti, pCurve->cbScratchGetSetValue );
}

//
// The following function sets the point to (1:1:0) in Jacobian coordinates.
//
VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassSetZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;
    PSYMCRYPT_MODELEMENT peTmp = NULL;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) );

    // Getting handle to X
    peTmp = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst );

    // Setting the right value (always 1)
    SymCryptModElementSetValueUint32( 1, FMod, peTmp, pbScratch, cbScratch );

    // Getting handle to Y
    peTmp = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst );

    // Setting the right value (always 1)
    SymCryptModElementSetValueUint32( 1, FMod, peTmp, pbScratch, cbScratch );

    // Getting handle to Z
    peTmp = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst );

    // Setting the right value (always 0)
    SymCryptModElementSetValueUint32( 0, pCurve->FMod, peTmp, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassSetDistinguished(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poDst->pCurve) );

    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptEcpointCopy( pCurve, pCurve->G, poDst );
}

//
// The following function checks if
//      - X1*Z2^2 = X2*Z1^2 and Y1*Z2^3 = Y2*Z1^3 (Equal case)
//      - X1*Z2^2 = X2*Z1^2 and Y1*Z2^3 = -Y2*Z1^3 (Negative case)
//
// Remark: The case where Z1 = Z2 = 0 is covered above (the zero point
//         is equal to its negative).
//
UINT32
SYMCRYPT_CALL
SymCryptShortWeierstrassIsEqual(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;

    PSYMCRYPT_MODELEMENT peX1 = NULL;    // Pointer to X1
    PSYMCRYPT_MODELEMENT peY1 = NULL;    // Pointer to Y1
    PSYMCRYPT_MODELEMENT peZ1 = NULL;    // Pointer to Z1
    PSYMCRYPT_MODELEMENT peX2 = NULL;    // Pointer to X2
    PSYMCRYPT_MODELEMENT peY2 = NULL;    // Pointer to Y2
    PSYMCRYPT_MODELEMENT peZ2 = NULL;    // Pointer to Z2

    UINT32  dResX = 0;
    UINT32  dResY = 0;
    UINT32  dResYN = 0;

    PSYMCRYPT_MODELEMENT peT[4] = { 0 };  // Temporaries

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc1->pCurve) && SymCryptEcurveIsSame(pCurve, poSrc2->pCurve) );
    SYMCRYPT_ASSERT( (flags & ~(SYMCRYPT_FLAG_ECPOINT_EQUAL|SYMCRYPT_FLAG_ECPOINT_NEG_EQUAL)) == 0 );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) + 4 * pCurve->cbModElement );

    // Creating temporaries
    for (UINT32 i=0; i<4; i++)
    {
        peT[i] = SymCryptModElementCreate(
                pbScratch,
                pCurve->cbModElement,
                FMod );

        SYMCRYPT_ASSERT( peT[i] != NULL);

        pbScratch += pCurve->cbModElement;
    }

    // Fixing remaining scratch space size
    cbScratch -= 4 * pCurve->cbModElement;

    // Getting pointers to x and y of the source point
    peX1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc1 );
    peY1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc1 );
    peZ1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc1 );
    peX2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc2 );
    peY2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc2 );
    peZ2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc2 );

    // Setting the default flag if flags == 0
    flags |= ( SYMCRYPT_MASK32_ZERO( flags ) & SYMCRYPT_FLAG_ECPOINT_EQUAL );

    // Calculation
    SymCryptModSquare( FMod, peZ1, peT[0], pbScratch, cbScratch );            // T0 := Z1 * Z1 = Z1^2
    SymCryptModSquare( FMod, peZ2, peT[1], pbScratch, cbScratch );            // T1 := Z2 * Z2 = Z2^2
    SymCryptModMul( FMod, peX1, peT[1], peT[2], pbScratch, cbScratch );       // T2 := X1 * T1 = X1*Z2^2
    SymCryptModMul( FMod, peX2, peT[0], peT[3], pbScratch, cbScratch );       // T3 := X2 * T0 = X2*Z1^2

    dResX = SymCryptModElementIsEqual( FMod, peT[2], peT[3] );

    SymCryptModMul( FMod, peZ1, peT[0], peT[0], pbScratch, cbScratch );       // T0 := Z1 * T0 = Z1^3
    SymCryptModMul( FMod, peZ2, peT[1], peT[1], pbScratch, cbScratch );       // T1 := Z2 * T1 = Z2^3
    SymCryptModMul( FMod, peY1, peT[1], peT[2], pbScratch, cbScratch );       // T2 := Y1 * T1 = Y1*Z2^3
    SymCryptModMul( FMod, peY2, peT[0], peT[3], pbScratch, cbScratch );       // T3 := Y2 * T0 = Y2*Z1^3

    dResY = SymCryptModElementIsEqual( FMod, peT[2], peT[3] );

    SymCryptModNeg( FMod, peT[3], peT[3], pbScratch, cbScratch );             // T3 := -T3 = -Y2*Z1^3

    dResYN = SymCryptModElementIsEqual( FMod, peT[2], peT[3] );

    return (SYMCRYPT_MASK32_NONZERO(flags & SYMCRYPT_FLAG_ECPOINT_EQUAL) & dResX & dResY) |
           (SYMCRYPT_MASK32_NONZERO(flags & SYMCRYPT_FLAG_ECPOINT_NEG_EQUAL) & dResX & dResYN);
}

UINT32
SYMCRYPT_CALL
SymCryptShortWeierstrassIsZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;
    PSYMCRYPT_MODELEMENT peZ = NULL;    // Pointer to Z

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) );

    UNREFERENCED_PARAMETER( pbScratch );
    UNREFERENCED_PARAMETER( cbScratch );

    // Getting pointer to Z of the source point
    peZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc );

    // Setting temporary to 0
    return SymCryptModElementIsZero( FMod, peZ );
}

//
// The following function verifies if the point (X:Y:Z) in Jacobian
// coordinates satisfies the equation Y^2 = X^3 + aXZ^4+bZ^6 .
//
UINT32
SYMCRYPT_CALL
SymCryptShortWeierstrassOnCurve(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;

    PSYMCRYPT_MODELEMENT peX = NULL;    // Pointer to X
    PSYMCRYPT_MODELEMENT peY = NULL;    // Pointer to Y
    PSYMCRYPT_MODELEMENT peZ = NULL;    // Pointer to Z

    PSYMCRYPT_MODELEMENT peT[2] = { 0 }; // Temporaries

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) + 2 * pCurve->cbModElement );

    // Creating temporaries
    for (UINT32 i=0; i<2; i++)
    {
        peT[i] = SymCryptModElementCreate(
                pbScratch,
                pCurve->cbModElement,
                FMod );

        SYMCRYPT_ASSERT( peT[i] != NULL);

        pbScratch += pCurve->cbModElement;
    }

    // Fixing remaining scratch space size
    cbScratch -= 2*pCurve->cbModElement;

    // Getting pointers to coordinates of the source point
    peX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc );
    peY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc );
    peZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc );

    // Calculation
    SymCryptModSquare( FMod, peZ, peT[0], pbScratch, cbScratch );               // T1 := Z  * Z  = Z^2
    SymCryptModSquare( FMod, peT[0], peT[1], pbScratch, cbScratch );            // T2 := T1 * T1 = Z^4
    SymCryptModMul( FMod, peT[0], peT[1], peT[0], pbScratch, cbScratch );       // T1 := T1 * T2 = Z^6

    SymCryptModMul( FMod, peT[0], pCurve->B, peT[0], pbScratch, cbScratch );    // T1 := T1 * b  = bZ^6

    SymCryptModMul( FMod, peT[1], peX, peT[1], pbScratch, cbScratch );          // T2 := T2 * X  = XZ^4
    SymCryptModMul( FMod, peT[1], pCurve->A, peT[1], pbScratch, cbScratch );    // T2 := T2 * a  = aXZ^4

    SymCryptModAdd( FMod, peT[0], peT[1], peT[1], pbScratch, cbScratch );       // T2 := T1 + T2 = aXZ^4 + bZ^6

    SymCryptModSquare( FMod, peX, peT[0], pbScratch, cbScratch );               // T1 := X  * X  = X^2
    SymCryptModMul( FMod, peT[0], peX, peT[0], pbScratch, cbScratch );          // T1 := T1 * X  = X^3
    SymCryptModAdd( FMod, peT[0], peT[1], peT[1], pbScratch, cbScratch );       // T2 := T1 + T2 = X^3 + aXZ^4 + bZ^6

    SymCryptModSquare( FMod, peY, peT[0], pbScratch, cbScratch );               // T1 := Y  * Y  = Y^2

    return SymCryptModElementIsEqual( FMod, peT[0], peT[1] );
}

//
// based on dbl-2007-bl formula
// but tweaked by saml to
//   a) remove overeager conversions from modular multiplication to modular squaring which introduce
//      more addition/subtraction. With current implementations (based on montgomery reduction),
//      the cost of [a square and an add/sub] is greater than the cost of [a multiplication]
//   b) share intermediate results of producing 8YYYY. [add/sub] is ~10% of cost of mul, so reducing
//      count of these operation has a real impact
//
//      2Y = 2*Y1
//      2YY = 2Y*Y1
//      4YY = 2*2YY
//      8YYYY = 2YY*4YY
//      S = X1*4YY
//      XX = X1^2
//      ZZ = Z1^2
//      ZZZZ = ZZ^2
//      M = 3*XX+a*ZZZZ
//      T = M^2-2*S
//      X3 = T
//      Y3 = M*(S-T)-8YYYY
//      Z3 = Z1*2Y
//
// Total cost:
//      6 Mul (1 by a)
//      4 Sqr
//      2 Add
//      4 Sub
//      3 Dbl
//
// Special Case:
//      If the source point is equal to the identity
//      point of the curve (i.e. Z1 = 0 in Jacobian
//      coordinates) then the resulting point has
//      Z3 = Z1*2Y1 = 0. Thus, this formula is
//      complete (it works for all points).
//
VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassDouble(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;
    PSYMCRYPT_MODELEMENT peT[3] = { 0 };    // Temporaries

    PCSYMCRYPT_MODELEMENT peX1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc );
    PCSYMCRYPT_MODELEMENT peY1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc );
    PCSYMCRYPT_MODELEMENT peZ1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc );

    PSYMCRYPT_MODELEMENT  peX3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst );
    PSYMCRYPT_MODELEMENT  peY3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst );
    PSYMCRYPT_MODELEMENT  peZ3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst );

    SYMCRYPT_ASSERT( pCurve->type == SYMCRYPT_INTERNAL_ECURVE_TYPE_SHORT_WEIERSTRASS );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) + 3 * pCurve->cbModElement );

    UNREFERENCED_PARAMETER( flags );

    // Creating temporaries
    for (UINT32 i=0; i<3; i++)
    {
        peT[i] = SymCryptModElementCreate(
                pbScratch,
                pCurve->cbModElement,
                FMod );

        SYMCRYPT_ASSERT( peT[i] != NULL);

        pbScratch += pCurve->cbModElement;
    }

    // Fixing remaining scratch space size
    cbScratch -= 3*pCurve->cbModElement;

    // Calculate the points
    SymCryptModAdd( FMod, peY1, peY1, peT[0], pbScratch, cbScratch );           /* T0 := Y1 + Y1 = 2Y */
    SymCryptModSquare( FMod, peZ1, peT[1], pbScratch, cbScratch );              /* T1 := Z1 * Z1 = ZZ */
    SymCryptModMul( FMod, peT[0], peZ1, peZ3, pbScratch, cbScratch );           /* Z3 := 2Y * Z1 = 2YZ */

    SymCryptModMul( FMod, peY1, peT[0], peY3, pbScratch, cbScratch );           /* Y3 := 2Y * Y1 = 2YY */
    SymCryptModAdd( FMod, peY3, peY3, peT[0], pbScratch, cbScratch );           /* T0 := 2YY + 2YY = 4YY */
    SymCryptModMul( FMod, peT[0], peY3, peY3, pbScratch, cbScratch );           /* Y3 := 2YY * 4YY = 8YYYY */

    SymCryptModMul( FMod, peT[0], peX1, peT[0], pbScratch, cbScratch );         /* T0 := X1 * 4YY = 4XYY = S */

    SymCryptModSquare( FMod, peT[1], peT[1], pbScratch, cbScratch );            /* T1 := T1 * T1 = ZZZZ */
    SymCryptModSquare( FMod, peX1, peT[2], pbScratch, cbScratch );              /* T2 := X1 * X1 = XX */
    SymCryptModMul( FMod, peT[1], pCurve->A, peT[1], pbScratch, cbScratch );    /* T1 := T1 * a  = a*ZZZZ */
    SymCryptModAdd( FMod, peT[2], peT[1], peT[1], pbScratch, cbScratch );       /* T1 := T2 + T1 = XX + a*ZZZZ */
    SymCryptModAdd( FMod, peT[2], peT[2], peT[2], pbScratch, cbScratch );       /* T2 := T2 + T2 = 2*XX */
    SymCryptModAdd( FMod, peT[0], peT[0], peX3, pbScratch, cbScratch );         /* X3 := 2*S */
    SymCryptModAdd( FMod, peT[2], peT[1], peT[1], pbScratch, cbScratch );       /* T1 := T2 + T1 = 3*XX + a*ZZZZ = M */

    SymCryptModSquare( FMod, peT[1], peT[2], pbScratch, cbScratch );            /* T2 := M^2 */
    SymCryptModSub( FMod, peT[2], peX3, peX3, pbScratch, cbScratch );           /* X3 := M^2 - 2*S = T */

    SymCryptModSub( FMod, peT[0], peX3, peT[0], pbScratch, cbScratch );         /* T0 := S - T */
    SymCryptModMul( FMod, peT[1], peT[0], peT[0], pbScratch, cbScratch );       /* T0 := M * (S - T) */
    SymCryptModSub( FMod, peT[0], peY3, peY3, pbScratch, cbScratch );           /* Y3 := M * (S - T) - 8*YYYY */
}


//
// based on dbl-2007-bl / dbl-2001-b formulae
// but tweaked by saml to
//   a) remove overeager conversions from modular multiplication to modular squaring which introduce
//      more addition/subtraction. With current implementations (based on montgomery reduction),
//      the cost of [a square and an add/sub] is greater than the cost of [a multiplication]
//   b) share intermediate results of producing 8YYYY. [add/sub] is ~10% of cost of mul, so reducing
//      count of these operation has a real impact
//   c) make use of knowledge that curve has a == -3, so M can be calculated more efficiently
//
//      2Y = 2*Y1
//      2YY = 2Y*Y1
//      4YY = 2*2YY
//      8YYYY = 2YY*4YY
//      ZZ = Z1^2
//      S = X1*4YY
//      M = 3*(X1+ZZ)*(X1-ZZ) = 3*(XX - ZZZZ)
//      T = M^2-2*S
//      X3 = T
//      Y3 = M*(S-T)-8YYYY
//      Z3 = 2Y*Z1
//
// Total cost:
//      6 Mul
//      2 Sqr
//      2 Add
//      4 Sub
//      4 Dbl
//
// Special Case:
//      If the source point is equal to the identity
//      point of the curve (i.e. Z1 = 0 in Jacobian
//      coordinates) then the resulting point has
//      Z3 = Z1*2Y1 = 0. Thus, this formula is
//      complete (it works for all points).
//
VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassDoubleSpecializedAm3(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;
    PSYMCRYPT_MODELEMENT peT[3] = { 0 };    // Temporaries

    PCSYMCRYPT_MODELEMENT peX1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc );
    PCSYMCRYPT_MODELEMENT peY1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc );
    PCSYMCRYPT_MODELEMENT peZ1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc );

    PSYMCRYPT_MODELEMENT  peX3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst );
    PSYMCRYPT_MODELEMENT  peY3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst );
    PSYMCRYPT_MODELEMENT  peZ3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst );

    SYMCRYPT_ASSERT( pCurve->type == SYMCRYPT_INTERNAL_ECURVE_TYPE_SHORT_WEIERSTRASS_AM3 );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) + 3 * pCurve->cbModElement );

    UNREFERENCED_PARAMETER( flags );

    // Creating temporaries
    for (UINT32 i=0; i<3; i++)
    {
        peT[i] = SymCryptModElementCreate(
                pbScratch,
                pCurve->cbModElement,
                FMod );

        SYMCRYPT_ASSERT( peT[i] != NULL);

        pbScratch += pCurve->cbModElement;
    }

    // Fixing remaining scratch space size
    cbScratch -= 3*pCurve->cbModElement;

    // Calculate the points
    SymCryptModAdd( FMod, peY1, peY1, peT[0], pbScratch, cbScratch );       /* T0 := Y1 + Y1 = 2Y */
    SymCryptModSquare( FMod, peZ1, peT[1], pbScratch, cbScratch );          /* T1 := Z1 * Z1 = ZZ */
    SymCryptModMul( FMod, peY1, peT[0], peY3, pbScratch, cbScratch );       /* Y3 := 2Y * Y1 = 2YY */

    SymCryptModMul( FMod, peT[0], peZ1, peZ3, pbScratch, cbScratch );       /* Z3 := 2Y * Z1 = 2YZ */

    SymCryptModAdd( FMod, peY3, peY3, peT[0], pbScratch, cbScratch );       /* T0 := 2YY + 2YY = 4YY */
    SymCryptModAdd( FMod, peX1, peT[1], peT[2], pbScratch, cbScratch );     /* T2 := X1 + ZZ */
    SymCryptModMul( FMod, peT[0], peY3, peY3, pbScratch, cbScratch );       /* Y3 := 2YY * 4YY = 8YYYY */

    SymCryptModSub( FMod, peX1, peT[1], peT[1], pbScratch, cbScratch );     /* T1 := X1 - ZZ */
    SymCryptModMul( FMod, peT[0], peX1, peT[0], pbScratch, cbScratch );     /* T0 := X1 * 4YY = 4XYY = S */

    SymCryptModMul( FMod, peT[2], peT[1], peT[2], pbScratch, cbScratch );   /* T2 := (X1 + ZZ)*(X1 - ZZ) = XX - ZZZZ */
    SymCryptModAdd( FMod, peT[2], peT[2], peT[1], pbScratch, cbScratch );   /* T1 := 2*(XX - ZZZZ) */
    SymCryptModAdd( FMod, peT[0], peT[0], peX3, pbScratch, cbScratch );     /* X3 := 2*S */
    SymCryptModAdd( FMod, peT[1], peT[2], peT[1], pbScratch, cbScratch );   /* T1 := 3*(XX - ZZZZ) = M */

    SymCryptModSquare( FMod, peT[1], peT[2], pbScratch, cbScratch );        /* T2 := M^2 */
    SymCryptModSub( FMod, peT[2], peX3, peX3, pbScratch, cbScratch );       /* X3 := M^2 - 2*S = T */

    SymCryptModSub( FMod, peT[0], peX3, peT[0], pbScratch, cbScratch );     /* T0 := S - T */
    SymCryptModMul( FMod, peT[1], peT[0], peT[0], pbScratch, cbScratch );   /* T0 := M * (S - T) */
    SymCryptModSub( FMod, peT[0], peY3, peY3, pbScratch, cbScratch );       /* Y3 := M * (S - T) - 8*YYYY */
}

//
// based on add-2007-bl formula
// but tweaked by saml to
//      remove overeager conversions from modular multiplication to modular squaring which introduce
//      more addition/subtraction.
//
//      Z1Z1 = Z1^2
//      Z2Z2 = Z2^2
//      U1 = X1*Z2Z2
//      U2 = X2*Z1Z1
//      S1 = Y1*Z2*Z2Z2
//      S2 = Y2*Z1*Z1Z1
//      H = U2-U1
//      2H = 2*H
//      I = (2H)^2
//      J = H*I
//      r = 2*(S2-S1)
//      V = U1*I
//      X3 = r^2-J-2*V
//      Y3 = r*(V-X3)-2*S1*J
//      Z3 = (Z1*Z2)*2H
//
// Total cost:
//     12 Mul
//      4 Sqr
//      0 Add
//      7 Sub
//      3 Dbl
//
// Special Case:
//      If the two source points are opposite (X1 / Z1^2 == X2 / Z2^2),
//      then H = U2-U1 = 0. Thus Z3 = 0 and the result is correct.
//
VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassAddDiffNonZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;

    PCSYMCRYPT_MODELEMENT peX1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc1 );
    PCSYMCRYPT_MODELEMENT peY1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc1 );
    PCSYMCRYPT_MODELEMENT peZ1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc1 );

    PCSYMCRYPT_MODELEMENT peX2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc2 );
    PCSYMCRYPT_MODELEMENT peY2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc2 );
    PCSYMCRYPT_MODELEMENT peZ2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc2 );

    PSYMCRYPT_MODELEMENT  peX3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst );
    PSYMCRYPT_MODELEMENT  peY3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst );
    PSYMCRYPT_MODELEMENT  peZ3 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst );

    PSYMCRYPT_MODELEMENT peT[7] = { 0 };  // Temporaries

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc1->pCurve) && SymCryptEcurveIsSame(pCurve, poSrc2->pCurve) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) + 7 * pCurve->cbModElement );

    // Creating temporaries
    for (UINT32 i=0; i<7; i++)
    {
        peT[i] = SymCryptModElementCreate(
                pbScratch,
                pCurve->cbModElement,
                FMod );

        SYMCRYPT_ASSERT( peT[i] != NULL);

        pbScratch += pCurve->cbModElement;
    }

    // Fixing remaining scratch space size
    cbScratch -= 7*pCurve->cbModElement;

    // Calculation

    SymCryptModSquare( FMod, peZ1, peT[0], pbScratch, cbScratch );          /* T0 := Z1 * Z1 = Z1Z1 */
    SymCryptModMul( FMod, peZ1, peT[0], peT[1], pbScratch, cbScratch );     /* T1 := Z1*Z1Z1 */

    SymCryptModSquare( FMod, peZ2, peT[6], pbScratch, cbScratch );          /* T6 := Z2 * Z2 = Z2Z2 */
    SymCryptModMul( FMod, peX1, peT[6], peT[2], pbScratch, cbScratch );     /* T2 := X1 * T6 = X1*Z2Z2 = U1 */
    SymCryptModMul( FMod, peX2, peT[0], peT[3], pbScratch, cbScratch );     /* T3 := X2 * Z1Z1 = U2 */
    SymCryptModSub( FMod, peT[3], peT[2], peT[5], pbScratch, cbScratch );   /* T5 := T3 - T2 = U2 - U1 = H */
    SymCryptModAdd( FMod, peT[5], peT[5], peT[3], pbScratch, cbScratch );   /* T3 := T5 + T5 = 2H */

    SymCryptModMul( FMod, peZ1, peZ2, peT[4], pbScratch, cbScratch );       /* T4 := Z1 * Z2 */

    SymCryptModMul( FMod, peZ2, peT[6], peT[6], pbScratch, cbScratch );     /* T6 := Z2 * T6 = Z2*Z2Z2 */
    SymCryptModMul( FMod, peT[4], peT[3], peZ3, pbScratch, cbScratch );     /* Z3 := T4 * T3 = Z1*Z2*2H */

    SymCryptModMul( FMod, peY1, peT[6], peT[6], pbScratch, cbScratch );     /* T6 := Y1 * T6 = Y1*Z2*Z2Z2 = S1 */
    SymCryptModMul( FMod, peY2, peT[1], peT[4], pbScratch, cbScratch );     /* T4 := Y2*Z1*Z1Z1 = S2 */
    SymCryptModSub( FMod, peT[4], peT[6], peT[4], pbScratch, cbScratch );   /* T4 := T4 - T6 = S2-S1 */
    SymCryptModAdd( FMod, peT[4], peT[4], peT[4], pbScratch, cbScratch );   /* T4 := T4 + T4 = 2*(S2-S1) = r */

    SymCryptModSquare( FMod, peT[3], peT[3], pbScratch, cbScratch );        /* T3 := T3 * T3 = (2*H)^2 = I */
    SymCryptModMul( FMod, peT[3], peT[5], peT[5], pbScratch, cbScratch );   /* T5 := T3 * T5 = H*I = J */
    SymCryptModMul( FMod, peT[2], peT[3], peT[3], pbScratch, cbScratch );   /* T3 := T2 * T3 = U1*I = V */

    SymCryptModSquare( FMod, peT[4], peT[2], pbScratch, cbScratch );        /* T2 := T4 * T4 = r^2 */
    SymCryptModSub( FMod, peT[2], peT[5], peT[2], pbScratch, cbScratch );   /* T2 := T2 - T5 = r^2 - J */
    SymCryptModSub( FMod, peT[2], peT[3], peT[2], pbScratch, cbScratch );   /* T2 := T2 - T3 = r^2 - J - V */
    SymCryptModSub( FMod, peT[2], peT[3], peX3, pbScratch, cbScratch );     /* T2 := T2 - T3 = r^2 - J - 2*V = X3 */

    SymCryptModSub( FMod, peT[3], peX3, peT[3], pbScratch, cbScratch );     /* T3 := T3 - T2 = V - X3 */
    SymCryptModMul( FMod, peT[3], peT[4], peT[3], pbScratch, cbScratch );   /* T3 := T3 * T4 = r*(V-X3) */
    SymCryptModMul( FMod, peT[6], peT[5], peT[6], pbScratch, cbScratch );   /* T6 := T6 * T5 = S1*J */
    SymCryptModAdd( FMod, peT[6], peT[6], peT[6], pbScratch, cbScratch );   /* T6 := T6 + T6 = 2*S1*J */
    SymCryptModSub( FMod, peT[3], peT[6], peY3, pbScratch, cbScratch );     /* Y3 := T6 - T3 = r*(V-X3) - 2*S1*J */
}

//
// The following function is a complete **SIDE-CHANNEL-UNSAFE**
// addition of points that detects as fast as possible the special cases
// and merges the two previous calls.
//
VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassAddSideChannelUnsafe(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;

    PCSYMCRYPT_MODELEMENT peX1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc1 );
    PCSYMCRYPT_MODELEMENT peY1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc1 );
    PCSYMCRYPT_MODELEMENT peZ1 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc1 );

    PCSYMCRYPT_MODELEMENT peX2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poSrc2 );
    PCSYMCRYPT_MODELEMENT peY2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc2 );
    PCSYMCRYPT_MODELEMENT peZ2 = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poSrc2 );

    PSYMCRYPT_MODELEMENT peT[8] = { 0 };  // Temporaries

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc1->pCurve) && SymCryptEcurveIsSame(pCurve, poSrc2->pCurve) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) + 8 * pCurve->cbModElement );

    // Check if one of the points is zero
    if (SymCryptModElementIsZero( FMod, peZ1 ))
    {
        SymCryptEcpointCopy( pCurve, poSrc2, poDst);
        return;
    }

    if (SymCryptModElementIsZero( FMod, peZ2 ))
    {
        SymCryptEcpointCopy( pCurve, poSrc1, poDst);
        return;
    }

    // Creating temporaries
    for (UINT32 i=0; i<8; i++)
    {
        peT[i] = SymCryptModElementCreate(
                pbScratch,
                pCurve->cbModElement,
                FMod );

        SYMCRYPT_ASSERT( peT[i] != NULL);

        pbScratch += pCurve->cbModElement;
    }

    // Fixing remaining scratch space size
    cbScratch -= 8*pCurve->cbModElement;

    // Calculation

    SymCryptModSquare( FMod, peZ1, peT[0], pbScratch, cbScratch );          /* T0 := Z1 * Z1 = Z1Z1 */
    SymCryptModMul( FMod, peZ1, peT[0], peT[1], pbScratch, cbScratch );     /* T1 := Z1*Z1Z1 */

    SymCryptModSquare( FMod, peZ2, peT[6], pbScratch, cbScratch );          /* T6 := Z2 * Z2 = Z2Z2 */
    SymCryptModMul( FMod, peX1, peT[6], peT[2], pbScratch, cbScratch );     /* T2 := X1 * T6 = X1*Z2Z2 = U1 */
    SymCryptModMul( FMod, peX2, peT[0], peT[3], pbScratch, cbScratch );     /* T3 := X2 * Z1Z1 = U2 */
    SymCryptModSub( FMod, peT[3], peT[2], peT[5], pbScratch, cbScratch );   /* T5 := T3 - T2 = U2 - U1 = H */

    SymCryptModMul( FMod, peY2, peT[1], peT[7], pbScratch, cbScratch );     /* T7 := Y2 * T1 = Y2*Z1*Z1Z1 = S2 */
    SymCryptModMul( FMod, peZ2, peT[6], peT[1], pbScratch, cbScratch );     /* T1 := Z2 * T6 = Z2*Z2Z2 */
    SymCryptModMul( FMod, peY1, peT[1], peT[1], pbScratch, cbScratch );     /* T1 := Y1 * T1 = Y1*Z2*Z2Z2 = S1 */
    SymCryptModSub( FMod, peT[7], peT[1], peT[7], pbScratch, cbScratch );   /* T7 := T7 - T1 = S2-S1 */

    if (SymCryptModElementIsZero( FMod, peT[5] ) & SymCryptModElementIsZero( FMod, peT[7] ))
    {
        // Points are equal - run double on poSrc1

        SymCryptModElementCopy( FMod, peT[0], peT[4] );                             /* Move Z1Z1 for later */

        SymCryptModSquare( FMod, peX1, peT[0], pbScratch, cbScratch );              /* T0 := X1 * X1 = XX */
        SymCryptModSquare( FMod, peY1, peT[3], pbScratch, cbScratch );              /* T3 := Y1 * Y1 = YY */
        SymCryptModSquare( FMod, peT[3], peT[5], pbScratch, cbScratch );            /* T5 := T3 * T3 = YYYY */

        SymCryptModAdd( FMod, peX1, peT[3], peT[1], pbScratch, cbScratch );         /* T1 := X1 + T3 = X + YY */
        SymCryptModSquare( FMod, peT[1], peT[1], pbScratch, cbScratch );            /* T1 := T1 * T1 = (X + YY)^2 */
        SymCryptModSub( FMod, peT[1], peT[0], peT[1], pbScratch, cbScratch );       /* T1 := T1 - T0 = (X + YY)^2 - XX */
        SymCryptModSub( FMod, peT[1], peT[5], peT[1], pbScratch, cbScratch );       /* T1 := T1 - T5 = (X + YY)^2 - XX - YYYY */
        SymCryptModAdd( FMod, peT[1], peT[1], peT[1], pbScratch, cbScratch );       /* T1 := T1 + T1 = 2*((X + YY)^2 - XX - YYYY) = S */

        //SymCryptModSquare( FMod, peZ1, peT[4], pbScratch, cbScratch );              /* T4 := Z1 * Z1 = ZZ */

        SymCryptModSquare( FMod, peT[4], peT[2], pbScratch, cbScratch );            /* T2 := T4 * T4 = ZZ^2 */
        SymCryptModMul( FMod, peT[2], pCurve->A, peT[2], pbScratch, cbScratch );    /* T2 := T2 * a  = a*ZZ^2 */
        SymCryptModAdd( FMod, peT[2], peT[0], peT[2], pbScratch, cbScratch );       /* T2 := T2 + T0 = XX + a*ZZ^2 */
        SymCryptModAdd( FMod, peT[0], peT[0], peT[0], pbScratch, cbScratch );       /* T0 := T0 + T0 = 2*XX */
        SymCryptModAdd( FMod, peT[2], peT[0], peT[2], pbScratch, cbScratch );       /* T2 := T2 + T0 = 3*XX + a*ZZ^2 = M */

        SymCryptModSquare( FMod, peT[2], peT[0], pbScratch, cbScratch );            /* T0 := T2 * T2 = M^2 */
        SymCryptModSub( FMod, peT[0], peT[1], peT[0], pbScratch, cbScratch );       /* T0 := T0 - T1 = M^2 - S */
        SymCryptModSub( FMod, peT[0], peT[1], peT[0], pbScratch, cbScratch );       /* T0 := T0 - T1 = M^2 - 2*S = T = X3 */

        SymCryptModSub( FMod, peT[1], peT[0], peT[1], pbScratch, cbScratch );       /* T1 := T1 - T0 = S - T */
        SymCryptModMul( FMod, peT[2], peT[1], peT[1], pbScratch, cbScratch );       /* T1 := T2 * T1 = M * (S - T) */
        SymCryptModAdd( FMod, peT[5], peT[5], peT[5], pbScratch, cbScratch );       /* T5 := T5 + T5 = 2*YYYY */
        SymCryptModAdd( FMod, peT[5], peT[5], peT[5], pbScratch, cbScratch );       /* T5 := T5 + T5 = 4*YYYY */
        SymCryptModAdd( FMod, peT[5], peT[5], peT[5], pbScratch, cbScratch );       /* T5 := T5 + T5 = 8*YYYY */
        SymCryptModSub( FMod, peT[1], peT[5], peT[1], pbScratch, cbScratch );       /* T1 := T1 - T5 = M * (S - T) - 8*YYYY = Y3 */

        SymCryptModAdd( FMod, peY1, peZ1, peT[2], pbScratch, cbScratch );           /* T2 := Y1 + Z1 */
        SymCryptModSquare( FMod, peT[2], peT[2], pbScratch, cbScratch );            /* T2 := T2 * T2 = (Y + Z )^2 */
        SymCryptModSub( FMod, peT[2], peT[3], peT[2], pbScratch, cbScratch );       /* T2 := T2 - T3 = (Y + Z )^2 - YY */
        SymCryptModSub( FMod, peT[2], peT[4], peT[2], pbScratch, cbScratch );       /* T2 := T2 - T4 = (Y + Z )^2 - YY - ZZ = Z3 */

        // Setting the result
        SymCryptModElementCopy( FMod, peT[0], SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst ) );
        SymCryptModElementCopy( FMod, peT[1], SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst ) );
        SymCryptModElementCopy( FMod, peT[2], SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst ) );
    }
    else
    {
        // Continue the addition

        SymCryptModAdd( FMod, peZ1, peZ2, peT[4], pbScratch, cbScratch );       /* T4 := Z1 + Z2 */
        SymCryptModSquare( FMod, peT[4], peT[4], pbScratch, cbScratch );        /* T4 := T4 * T4 = (Z1 + Z2)^2 */
        SymCryptModSub( FMod, peT[4], peT[0], peT[4], pbScratch, cbScratch );   /* T4 := T4 - Z1Z1 = (Z1 + Z2)^2 - Z1Z1 */
        SymCryptModSub( FMod, peT[4], peT[6], peT[4], pbScratch, cbScratch );   /* T4 := T4 - T6 = (Z1 + Z2)^2 - Z1Z1 - Z2Z2 */
        SymCryptModMul( FMod, peT[4], peT[5], peT[4], pbScratch, cbScratch );   /* T4 := T4 * T5 = ((Z1 + Z2)^2 - Z1Z1 - Z2Z2)*H = Z3 */

        SymCryptModAdd( FMod, peT[7], peT[7], peT[7], pbScratch, cbScratch );   /* T7 := T7 + T7 = 2*(S2-S1) = r */

        SymCryptModAdd( FMod, peT[5], peT[5], peT[3], pbScratch, cbScratch );   /* T3 := T5 + T5 = 2*H */
        SymCryptModSquare( FMod, peT[3], peT[3], pbScratch, cbScratch );        /* T3 := T3 * T3 = (2*H)^2 = I */
        SymCryptModMul( FMod, peT[3], peT[5], peT[5], pbScratch, cbScratch );   /* T5 := T3 * T5 = H*I = J */
        SymCryptModMul( FMod, peT[2], peT[3], peT[3], pbScratch, cbScratch );   /* T3 := T2 * T3 = U1*I = V */

        SymCryptModSquare( FMod, peT[7], peT[2], pbScratch, cbScratch );        /* T2 := T7 * T7 = r^2 */
        SymCryptModSub( FMod, peT[2], peT[5], peT[2], pbScratch, cbScratch );   /* T2 := T2 - T5 = r^2 - J */
        SymCryptModSub( FMod, peT[2], peT[3], peT[2], pbScratch, cbScratch );   /* T2 := T2 - T3 = r^2 - J - V */
        SymCryptModSub( FMod, peT[2], peT[3], peT[2], pbScratch, cbScratch );   /* T2 := T2 - T3 = r^2 - J - 2*V = X3 */

        SymCryptModSub( FMod, peT[3], peT[2], peT[3], pbScratch, cbScratch );   /* T3 := T3 - T2 = V - X3 */
        SymCryptModMul( FMod, peT[3], peT[7], peT[3], pbScratch, cbScratch );   /* T3 := T3 * T7 = r*(V-X3) */
        SymCryptModMul( FMod, peT[1], peT[5], peT[6], pbScratch, cbScratch );   /* T6 := T1 * T5 = S1*J */
        SymCryptModAdd( FMod, peT[6], peT[6], peT[6], pbScratch, cbScratch );   /* T6 := T6 + T6 = 2*S1*J */
        SymCryptModSub( FMod, peT[3], peT[6], peT[3], pbScratch, cbScratch );   /* T3 := T6 - T3 = r*(V-X3) - 2*S1*J = Y3 */

        // Setting the result
        SymCryptModElementCopy( FMod, peT[2], SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poDst ) );
        SymCryptModElementCopy( FMod, peT[3], SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poDst ) );
        SymCryptModElementCopy( FMod, peT[4], SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poDst ) );
    }
}

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassAdd(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    UINT32 dSrc1Zero = 0;
    UINT32 dSrc2Zero = 0;
    UINT32 dSrcEqual = 0;

    // Temporary points
    PSYMCRYPT_ECPOINT   poQ0 = NULL;
    PSYMCRYPT_ECPOINT   poQ1 = NULL;

    SIZE_T cbEcpoint = SymCryptSizeofEcpointFromCurve( pCurve );

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc1->pCurve) && SymCryptEcurveIsSame(pCurve, poSrc2->pCurve) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );     // We will need the entire scratch space

    SYMCRYPT_ASSERT( cbScratch > 2*cbEcpoint );

    if ((flags & SYMCRYPT_FLAG_DATA_PUBLIC) != 0)
    {
        SymCryptShortWeierstrassAddSideChannelUnsafe( pCurve, poSrc1, poSrc2, poDst, pbScratch, cbScratch );
    }
    else
    {
        // Creating temporary points
        poQ0 = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
        SYMCRYPT_ASSERT( poQ0 != NULL);
        pbScratch += cbEcpoint;

        poQ1 = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
        SYMCRYPT_ASSERT( poQ1 != NULL);
        pbScratch += cbEcpoint;

        // Fixing remaining scratch space size
        cbScratch -= 2*cbEcpoint;

        // Calculate the masks
        dSrc1Zero = SymCryptShortWeierstrassIsZero( pCurve, poSrc1, pbScratch, cbScratch );
        dSrc2Zero = SymCryptShortWeierstrassIsZero( pCurve, poSrc2, pbScratch, cbScratch );
        dSrcEqual = SymCryptShortWeierstrassIsEqual( pCurve, poSrc1, poSrc2, SYMCRYPT_FLAG_ECPOINT_EQUAL, pbScratch, cbScratch );

        // Side-channel safe computations
        SymCryptShortWeierstrassAddDiffNonZero( pCurve, poSrc1, poSrc2, poQ0, pbScratch, cbScratch );   // This covers the cases where Src1 != Src2 or Src1 = -Src2

        SymCryptEcpointDouble( pCurve, poSrc1, poQ1, 0, pbScratch, cbScratch );     // Dispatch to Double function; enables type assertion on SymCryptShortWeierstrassDouble to be specific
        SymCryptEcpointMaskedCopy( pCurve, poQ1, poQ0, dSrcEqual );                                     // (Masked) copy if the points are equal

        SymCryptEcpointMaskedCopy( pCurve, poSrc1, poQ0, dSrc2Zero );                                   // (Masked) copy if Src2 = 0
        SymCryptEcpointMaskedCopy( pCurve, poSrc2, poQ0, dSrc1Zero );                                   // (Masked) copy if Src1 = 0

        SymCryptEcpointCopy( pCurve, poQ0, poDst );                                 // Copy the final result to destination
    }
}

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassNegate(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Inout_ PSYMCRYPT_ECPOINT   poSrc,
            UINT32              mask,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;
    PSYMCRYPT_MODELEMENT peY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poSrc);

    PSYMCRYPT_MODELEMENT peTmp = NULL;

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) + pCurve->cbModElement);

    peTmp = SymCryptModElementCreate(
                pbScratch,
                pCurve->cbModElement,
                FMod );
    SYMCRYPT_ASSERT( peTmp != NULL);

    pbScratch += pCurve->cbModElement;
    cbScratch -= pCurve->cbModElement;

    SymCryptModNeg( FMod, peY, peTmp, pbScratch, cbScratch );
    SymCryptModElementMaskedCopy( FMod, peTmp, peY, mask );
}
