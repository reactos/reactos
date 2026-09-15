//
// ec_mul.c   Generic multiplication algorithms for elliptic curves
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

//
// Most of the following algorithms were presented in the paper
// "Selecting Elliptic Curves for Cryptography: An Efficiency and
//  Security Analysis" by Bos, Costello, Longa, and Naehrig
//

//
// The following is an adaptation of algorithm 4: "Precomputation
// scheme for Weierstrass curves"
//
// Input:   Point P and number of precomputed points nPoints (=2^(w-2))
//
// Output:  P[i] = (2*i+1)P for 0<=i<2^(w-2)
//
// Remarks:
//          1. We store each point in an array of 4*2^(w-2) = 2^w modelements where
//             each point is represented with X,Y,Z Jacobian coordinates and the W=-Y
//             negated Y coordinate (so that we can get the negative of a point easily)
//          2. The source point P is already in the 0'th position of the array.
//
VOID
SYMCRYPT_CALL
SymCryptPrecomputation(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
            UINT32                  nPoints,
    _In_reads_( SYMCRYPT_ECURVE_SW_MAX_NPRECOMP_POINTS )
            PSYMCRYPT_ECPOINT *     poPIs,
    _Out_   PSYMCRYPT_ECPOINT       poQ,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poQ->pCurve) );
    // Calculation for Q = 2*P
    SymCryptEcpointDouble( pCurve, poPIs[0], poQ, 0, pbScratch, cbScratch );

    for (UINT32 i=1; i<nPoints; i++)
    {
        // Calculation for (2i+1)*P = i*Q + P
        SymCryptEcpointAddDiffNonZero( pCurve, poQ, poPIs[i-1], poPIs[i], pbScratch, cbScratch );
    }
}

VOID
SYMCRYPT_CALL
SymCryptOfflinePrecomputation(
    _In_    PSYMCRYPT_ECURVE pCurve,
    _Out_writes_bytes_( cbScratch )
            PBYTE         pbScratch,
            SIZE_T        cbScratch )
{
    PSYMCRYPT_ECPOINT poQ = NULL;

    UINT32 cbEcpoint = SymCryptSizeofEcpointFromCurve( pCurve );

    SYMCRYPT_ASSERT( cbScratch >= cbEcpoint + SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pCurve->FModDigits ) );

    poQ = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
    SYMCRYPT_ASSERT( poQ != NULL );
    pbScratch += cbEcpoint;
    cbScratch -= cbEcpoint;

    SymCryptPrecomputation(
                pCurve,
                pCurve->info.sw.nPrecompPoints,
                pCurve->info.sw.poPrecompPoints,
                poQ,
                pbScratch,
                cbScratch );
}

// Mask which is 0xffffffff only when _index == _target
#define DELTA_MASK( _index, _target)    SYMCRYPT_MASK32_ZERO( (_index) ^ (_target) )

//
// The following is an adaptation of algorithm 1: "Variable-base scalar multiplication
// using the fixed-window method"
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointScalarMulFixedWindow(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _In_    PCSYMCRYPT_INT          piScalar,
    _In_opt_
            PCSYMCRYPT_ECPOINT      poSrc,
            UINT32                  flags,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;

    PCSYMCRYPT_MODULUS FMod = pCurve->FMod;

    UINT32  i, j;

    UINT32  w = pCurve->info.sw.window;
    UINT32  nPrecompPoints = pCurve->info.sw.nPrecompPoints;
	// dcl - assuming that nRecodedDigits has some reasonably small range - please document
	// so that we can know usage of this variable will not cause problems
	// Also, documentation of inputs, notes, etc at the function definition would be quite helpful
    UINT32  nRecodedDigits = ((pCurve->GOrdBitsize + w - 2) / (w-1)) + 1;

    // Masks
    UINT32  fZero = 0;
    UINT32  fEven = 0;
    UINT32  indexMask = 0;

    BOOLEAN bPrecompOffline = FALSE;

    // ====================================================
    // Temporaries
    PSYMCRYPT_MODELEMENT    peT = NULL;
    PSYMCRYPT_ECPOINT       poPIs[SYMCRYPT_ECURVE_SW_MAX_NPRECOMP_POINTS] = { 0 };
    PSYMCRYPT_ECPOINT       poQ = NULL;
    PSYMCRYPT_ECPOINT       poTmp = NULL;
    PSYMCRYPT_INT           piRem = NULL;
    PSYMCRYPT_INT           piTmp = NULL;
    PUINT32                 absofKIs = NULL;
    PUINT32                 sigofKIs = NULL;
    // ===================================================

    PSYMCRYPT_MODELEMENT    peQX = NULL;
    PSYMCRYPT_MODELEMENT    peQY = NULL;
    PSYMCRYPT_MODELEMENT    peQZ = NULL;

    SIZE_T cbEcpoint = SymCryptSizeofEcpointFromCurve( pCurve );
    SIZE_T cbScalar = SymCryptSizeofIntFromDigits( pCurve->GOrdDigits );

    // Make sure we only specify the correct flags
    if ((flags & ~SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL) != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto exit;
    }

    // Check if poSrc is NULL and if yes set it to G
    if (poSrc == NULL)
    {
        poSrc = pCurve->G;
        bPrecompOffline = TRUE;
    }

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) ||
                     SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrc->pCurve) && SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS(pCurve, 1) );

    SYMCRYPT_ASSERT( cbScratch >=
                        pCurve->cbModElement +
                        (nPrecompPoints+2)*cbEcpoint +
                        2*cbScalar +
                        ((2*nRecodedDigits*sizeof(UINT32) + SYMCRYPT_ASYM_ALIGN_VALUE - 1)/SYMCRYPT_ASYM_ALIGN_VALUE )*SYMCRYPT_ASYM_ALIGN_VALUE +
                        SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ) );

    // Creating temporary modelement
    peT = SymCryptModElementCreate( pbScratch, pCurve->cbModElement, FMod );
    SYMCRYPT_ASSERT( peT != NULL );
    pbScratch += pCurve->cbModElement;

    // Creating temporary precomputed points (if needed)
    SYMCRYPT_ASSERT( nPrecompPoints <= SYMCRYPT_ECURVE_SW_MAX_NPRECOMP_POINTS );
    for (i=0; i<nPrecompPoints; i++)
    {
        if (bPrecompOffline)
        {
            poPIs[i] = pCurve->info.sw.poPrecompPoints[i];
        }
        else
        {
            poPIs[i] = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
            SYMCRYPT_ASSERT( poPIs[i] != NULL );
            pbScratch += cbEcpoint;
        }
    }

    // Creating temporary points
    poQ = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
    SYMCRYPT_ASSERT( poQ != NULL );
    pbScratch += cbEcpoint;

    poTmp = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
    SYMCRYPT_ASSERT( poTmp != NULL );
    pbScratch += cbEcpoint;

    // Creating temporary scalar for the remainder
    piRem = SymCryptIntCreate( pbScratch, cbScalar, pCurve->GOrdDigits );
    SYMCRYPT_ASSERT( piRem != NULL);
    pbScratch += cbScalar;

    piTmp = SymCryptIntCreate( pbScratch, cbScalar, pCurve->GOrdDigits );
    SYMCRYPT_ASSERT( piTmp != NULL);
    pbScratch += cbScalar;

    // Fixing pointers to recoded digits (be careful that the remaining space is SYMCRYPT_ASYM_ALIGNed)
    absofKIs = (PUINT32) pbScratch;
    pbScratch += nRecodedDigits * sizeof(UINT32);
    sigofKIs = (PUINT32) pbScratch;
    pbScratch += nRecodedDigits * sizeof(UINT32);
    pbScratch = (PBYTE) ( ((SIZE_T)pbScratch + SYMCRYPT_ASYM_ALIGN_VALUE - 1) & ~(SYMCRYPT_ASYM_ALIGN_VALUE - 1) );

    // Fixing remaining scratch space size
    cbScratch -= ( pCurve->cbModElement + (nPrecompPoints+2)*cbEcpoint + 2*cbScalar );
    cbScratch -= (((2*nRecodedDigits*sizeof(UINT32) + SYMCRYPT_ASYM_ALIGN_VALUE - 1)/SYMCRYPT_ASYM_ALIGN_VALUE )*SYMCRYPT_ASYM_ALIGN_VALUE);

    //
    // Main algorithm
    //

    // It is the caller's responsibility to ensure that the provided piScalar <= GOrd, double check this in debug mode
    SYMCRYPT_ASSERT( !SymCryptIntIsLessThan( SymCryptIntFromModulus( pCurve->GOrd ), piScalar ) );

    // Store k into an int
    SymCryptIntCopy( piScalar, piRem );

    // Check if k is 0
    fZero = SymCryptIntIsEqualUint32( piRem, 0 );

    // Or if the src point is zero
    fZero |= SymCryptEcpointIsZero( pCurve, poSrc, pbScratch, cbScratch );

    // Check if k is even and convert it to r-k if true
    fEven = SYMCRYPT_MASK32_ZERO(SymCryptIntGetBit( piRem, 0 ));
    SymCryptIntSubSameSize( SymCryptIntFromModulus(pCurve->GOrd), piRem, piTmp);
    SymCryptIntMaskedCopy( piTmp, piRem, fEven );

    // Recoding stage
    SymCryptFixedWindowRecoding( w, piRem, piTmp, absofKIs, sigofKIs, nRecodedDigits );

    // Precomputation stage
    if (!bPrecompOffline)
    {
        // Copy the first point in the start of the poPIs array
        SymCryptEcpointCopy( pCurve, poSrc, poPIs[0] );

        SymCryptPrecomputation( pCurve, nPrecompPoints, poPIs, poQ, pbScratch, cbScratch );
    }


    // Get the pointers to Q
    peQX = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 0, pCurve, poQ );
    peQY = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 1, pCurve, poQ );
    peQZ = SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( 2, pCurve, poQ );

    // Q = P[ (|k_t|-1)/2 ] in memory access side-channel safe way
    // That is, we touch all the precomputed points. The access pattern of KIs is fixed.
    for (j=0; j<nPrecompPoints; j++)
    {
        indexMask = DELTA_MASK( j, absofKIs[nRecodedDigits-1] );
        SymCryptEcpointMaskedCopy( pCurve, poPIs[j], poQ, indexMask);
    }

    for (i=nRecodedDigits - 2; i>0; i--)
    {
        // Q = 2^(w-1) * Q
        for (j=0; j<w-1; j++)
        {
            SymCryptEcpointDouble( pCurve, poQ, poQ, 0, pbScratch, cbScratch );
        }

        // Copy the required precomputed point into poTmp (touch all points)
        for (j=0; j<nPrecompPoints; j++)
        {
            indexMask = DELTA_MASK( j, absofKIs[i] );
            SymCryptEcpointMaskedCopy( pCurve, poPIs[j], poTmp, indexMask);
        }

        // Negate if needed
        SymCryptEcpointNegate( pCurve, poTmp, sigofKIs[i], pbScratch, cbScratch );

        // Do the addition Q + s_i P[k_i]
        SymCryptEcpointAddDiffNonZero( pCurve, poQ, poTmp, poQ, pbScratch, cbScratch );
    }

    // Q = 2^(w-1) * Q
    for (j=0; j<w-1; j++)
    {
        SymCryptEcpointDouble( pCurve, poQ, poQ, 0, pbScratch, cbScratch );
    }

    // Copy the point s_0 P[k_0] into poTmp
    for (j=0; j<nPrecompPoints; j++)
    {
        indexMask = DELTA_MASK( j, absofKIs[0] );
        SymCryptEcpointMaskedCopy( pCurve, poPIs[j], poTmp, indexMask);
    }

    // Negate if needed
    SymCryptEcpointNegate( pCurve, poTmp, sigofKIs[0], pbScratch, cbScratch );

    // Complete addition routine
    SymCryptEcpointAdd( pCurve, poQ, poTmp, poQ, 0, pbScratch, cbScratch );

    // If even invert
    SymCryptEcpointNegate( pCurve, poQ, fEven, pbScratch, cbScratch );

    // Multiply by the cofactor (if needed) by continuing the doubling
    if ((pCurve->coFactorPower!=0) && ((flags & SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL) != 0))
    {
        for (j=0; j<pCurve->coFactorPower; j++)
        {
            SymCryptEcpointDouble( pCurve, poQ, poQ, 0, pbScratch, cbScratch );
        }
    }

    // If the resultant point is zero, ensure it will be set to the canonical zero point
    fZero |= SymCryptEcpointIsZero( pCurve, poQ, pbScratch, cbScratch );

    // Set the zero point
    SymCryptEcpointSetZero( pCurve, poTmp, pbScratch, cbScratch );
    SymCryptEcpointMaskedCopy( pCurve, poTmp, poQ, fZero );

    // Output the result (normalized flag == FALSE)
    SymCryptEcpointCopy( pCurve, poQ, poDst );

    scError = SYMCRYPT_NO_ERROR;

exit:

    return scError;
}

//
// The following is an adaptation of algorithm 9: "Double-scalar multiplication using the
// width-w NAF with interleaving"
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointMultiScalarMulWnafWithInterleaving(
    _In_                            PCSYMCRYPT_ECURVE       pCurve,
    _In_reads_( nPoints )           PCSYMCRYPT_INT *        piSrcScalarArray,
    _In_reads_( nPoints )           PCSYMCRYPT_ECPOINT *    poSrcEcpointArray,
    _In_                            UINT32                  nPoints,
    _In_                            UINT32                  flags,
    _Out_                           PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;

    UINT32  i, j;

    UINT32  w = pCurve->info.sw.window;
    UINT32  nPrecompPoints = pCurve->info.sw.nPrecompPoints;            // One table for each base
    UINT32  nRecodedDigits = pCurve->GOrdBitsize + 1;                   // Notice the difference with the fixed window

    // Masks
    UINT32  fZero[SYMCRYPT_ECURVE_MULTI_SCALAR_MUL_MAX_NPOINTS] = { 0 };
    UINT32  fZeroTot = 0xffffffff;

    BOOLEAN bPrecompOffline = FALSE;

    // ====================================================
    // Temporaries
    PSYMCRYPT_ECPOINT       poPIs[SYMCRYPT_ECURVE_SW_MAX_NPRECOMP_POINTS] = { 0 };
    PSYMCRYPT_ECPOINT       poQ = NULL;
    PSYMCRYPT_ECPOINT       poTmp = NULL;
    PSYMCRYPT_INT           piRem = NULL;
    PSYMCRYPT_INT           piTmp = NULL;

    PUINT32                 absofKIs = NULL;
    PUINT32                 sigofKIs = NULL;
    // ===================================================

    SIZE_T cbEcpoint = SymCryptSizeofEcpointFromCurve( pCurve );
    SIZE_T cbScalar = SymCryptSizeofIntFromDigits( pCurve->GOrdDigits );

    PBYTE pbScratchEnd = pbScratch + cbScratch;
    UNREFERENCED_PARAMETER( pbScratchEnd ); // Used in asserts

    // Make sure we only specify the correct flags
    if ((flags & ~(SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL)) != 0)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto exit;
    }

    // Check the maximum number of points
    if (nPoints > SYMCRYPT_ECURVE_MULTI_SCALAR_MUL_MAX_NPOINTS)
    {
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto exit;
    }

    // Check if the first point is NULL
    if (poSrcEcpointArray[0] == NULL)
    {
        poSrcEcpointArray[0] = pCurve->G;
        bPrecompOffline = TRUE;
    }

    // Make sure that the non side-channel flag is specified
    if ((flags & SYMCRYPT_FLAG_DATA_PUBLIC) == 0 )
    {
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto exit;
    }

    SYMCRYPT_ASSERT( SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE(pCurve) ||
                     SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE(pCurve) );
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS(pCurve, nPoints) );

    // Creating temporary precomputed points (if needed for the first point)
    for (i=0; i<nPoints*nPrecompPoints; i++)
    {
        if ((i<nPrecompPoints) && bPrecompOffline)
        {
            poPIs[i] = pCurve->info.sw.poPrecompPoints[i];
        }
        else
        {
            SYMCRYPT_ASSERT( pbScratch + cbEcpoint <= pbScratchEnd );
            poPIs[i] = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
            SYMCRYPT_ASSERT( poPIs[i] != NULL );
            pbScratch += cbEcpoint;
        }
    }

    SYMCRYPT_ASSERT( pbScratch + 2*cbEcpoint + 2*cbScalar + 2*nPoints*nRecodedDigits*sizeof(UINT32) <= pbScratchEnd );
    // Creating temporary points
    poQ = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
    SYMCRYPT_ASSERT( poQ != NULL );
    pbScratch += cbEcpoint;

    poTmp = SymCryptEcpointCreate( pbScratch, cbEcpoint, pCurve );
    SYMCRYPT_ASSERT( poTmp != NULL );
    pbScratch += cbEcpoint;

    // Creating temporary scalar for the remainder
    piRem = SymCryptIntCreate( pbScratch, cbScalar, pCurve->GOrdDigits );
    SYMCRYPT_ASSERT( piRem != NULL);
    pbScratch += cbScalar;

    piTmp = SymCryptIntCreate( pbScratch, cbScalar, pCurve->GOrdDigits );
    SYMCRYPT_ASSERT( piTmp != NULL);
    pbScratch += cbScalar;

    // Fixing pointers to recoded digits (be careful that the remaining space is SYMCRYPT_ASYM_ALIGNed)
    absofKIs = (PUINT32) pbScratch;
    pbScratch += nPoints * nRecodedDigits * sizeof(UINT32);
    sigofKIs = (PUINT32) pbScratch;
    pbScratch += nPoints * nRecodedDigits * sizeof(UINT32);
    pbScratch = (PBYTE) ( ((SIZE_T)pbScratch + SYMCRYPT_ASYM_ALIGN_VALUE - 1) & ~(SYMCRYPT_ASYM_ALIGN_VALUE - 1) );

    // Fixing remaining scratch space size
	// dcl - my guess is that the values here are small enough that there should not be a problem, but
	// would be better if that were documented.
    cbScratch -= ( (nPoints*nPrecompPoints+2)*cbEcpoint + 2*cbScalar );
    cbScratch -= (((2*nPoints*nRecodedDigits*sizeof(UINT32) + SYMCRYPT_ASYM_ALIGN_VALUE - 1)/SYMCRYPT_ASYM_ALIGN_VALUE )*SYMCRYPT_ASYM_ALIGN_VALUE);

    //
    // Main algorithm
    //
    for (j = 0; j<nPoints; j++)
    {
        SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poSrcEcpointArray[j]->pCurve) );

        // Check if k is 0 or if the src point is zero
        fZero[j] = ( SymCryptIntIsEqualUint32( piSrcScalarArray[j], 0 ) | SymCryptEcpointIsZero( pCurve, poSrcEcpointArray[j], pbScratch, cbScratch ) );
        fZeroTot &= fZero[j];

        // Skip the recoding stage (and all remaining steps) if this point will give result zero
        if (!fZero[j])
        {
            SymCryptIntCopy( piSrcScalarArray[j], piRem );

            // Recoding stage
            SymCryptWidthNafRecoding( w, piRem, &absofKIs[j*nRecodedDigits], &sigofKIs[j*nRecodedDigits], nRecodedDigits );

            // Precomputation stage
            if ((j>0) || !bPrecompOffline)
            {
                // Copy the first point in the start of the poPIs array
                SymCryptEcpointCopy( pCurve, poSrcEcpointArray[j], poPIs[j*nPrecompPoints] );

                SymCryptPrecomputation( pCurve, nPrecompPoints, &poPIs[j*nPrecompPoints], poQ, pbScratch, cbScratch );
            }
        }
    }

    // Set poQ to zero point
    SymCryptEcpointSetZero( pCurve, poQ, pbScratch, cbScratch );

    if (!fZeroTot)
    {
        // Main loop
        for (INT32 i = nRecodedDigits-1; i>-1; i--)
        {
            SymCryptEcpointDouble( pCurve, poQ, poQ, 0, pbScratch, cbScratch );

            for (j = 0; j<nPoints; j++)
            {
                if (!fZero[j] && sigofKIs[j*nRecodedDigits + i] != 0)
                {
                    SymCryptEcpointCopy( pCurve, poPIs[j*nPrecompPoints + absofKIs[j*nRecodedDigits + i]/2], poTmp );

                    if (sigofKIs[j*nRecodedDigits + i] == 0xffffffff)
                    {
                        SymCryptEcpointNegate( pCurve, poTmp, 0xffffffff, pbScratch, cbScratch );
                    }

                    SymCryptEcpointAdd( pCurve, poQ, poTmp, poQ, SYMCRYPT_FLAG_DATA_PUBLIC, pbScratch, cbScratch );
                }
            }
        }
    }

    // Multiply by the cofactor (if needed) by continuing the doubling
    if ((pCurve->coFactorPower!=0) && ((flags & SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL) != 0))
    {
        for (j=0; j<pCurve->coFactorPower; j++)
        {
            SymCryptEcpointDouble( pCurve, poQ, poQ, 0, pbScratch, cbScratch );
        }
    }

    // If the resultant point is zero, ensure it will be set to the canonical zero point
    if ( SymCryptEcpointIsZero( pCurve, poQ, pbScratch, cbScratch ) )
    {
        // Set poQ to zero point
        SymCryptEcpointSetZero( pCurve, poQ, pbScratch, cbScratch );
    }

    // Copy the result to the destination (normalized flag == FALSE)
    SymCryptEcpointCopy( pCurve, poQ, poDst );

    scError = SYMCRYPT_NO_ERROR;

exit:
    return scError;
}

VOID
SYMCRYPT_CALL
SymCryptEcpointGenericSetRandom(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _Out_   PSYMCRYPT_INT           piScalar,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch )
{
    PSYMCRYPT_MODELEMENT peScalar = NULL;
    SYMCRYPT_ASSERT( SymCryptEcurveIsSame(pCurve, poDst->pCurve) );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS(pCurve, 1) );
    SYMCRYPT_ASSERT( cbScratch >= pCurve->cbModElement );

    peScalar = SymCryptModElementCreate( pbScratch, pCurve->cbModElement, pCurve->GOrd );
    SYMCRYPT_ASSERT( peScalar != NULL );

    // Setting a random mod element in the [1, SubgroupOrder-1] set
    SymCryptModSetRandom( pCurve->GOrd, peScalar, (SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE|SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE), pbScratch + pCurve->cbModElement, cbScratch - pCurve->cbModElement );

    // Setting the integer
    SymCryptModElementToInt( pCurve->GOrd, peScalar, piScalar, pbScratch + pCurve->cbModElement, cbScratch - pCurve->cbModElement );

    // Do the multiplication (pass over the entire scratch space as it is not needed anymore)
    // !! Explicitly not checking the error return here as the only error is from specifying invalid flags !!
    SymCryptEcpointScalarMul( pCurve, piScalar, NULL, 0, poDst, pbScratch, cbScratch );
}
