//
// modexp.c   Modular exponentiation functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// The windowed modular exponentiation algorithm works by generating a
// side-channel table of all the powers of the base from 0 up to 2^W - 1
// where W is the window size:
//      scsPrecomp = { 1, base, base^2, ..., base^(2^W-1) }
//
// TODO: To mitigate power analysis attacks when multiplying by 1 (which might
//       contain a lot of zeros in non-Montgomery moduli), future work is to
//       get rid of the 1 in the table. The leak is limited since now we always
//       have Montgomery moduli.
//
// Then it slices the exponent into chunks of W bits and goes through
// each chunk of the exponent starting from the most significant
// chunk. For each chunk c_i it squares a temporary modelement
// W times and then multiplies it by scsPrecomp[c_i]. The starting
// value of the temporary modelement is scsPrecomp[c_0] i.e. the one
// corresponding to the most significant chunk.
//
// Denote by M and SQ the multiplications and squarings and by B = nBitsExp
// number of bits of the exponent. Then the algorithm does
//      (2^W - 2)*M + (B-1)/W*(W*SQ + M) =
//      (2^W + (B-1)/W -2) multiplications and (B-1) squarings
//
// It is beneficial to change the window size from W to W+1 when
//      2^(W+1) + (B-1)/(W+1) < 2^W + (B-1)/W =>
//      B > 2^W*W(W+1)+1
// A simple table that calculates the optimal values for the window size
// is shown below.
//
// The minimum value of W is W=4 as 2^W should be a multiple
// of the groupsize of the scsTable, which is 4 by default.

#define MIN_WINDOW_SIZE     (4)

static const UINT32 cutoffs[] =
{
    // 5,      // W should be 2 for 5 < B <= 25
    // 25,     // W should be 3 for 25 < B <= 97
    // 97,     // W should be 4 for 97 < B <= 321
    321,    // W should be 5 for 321 < B <= 961
    // 961,    // W should be 6 for 961 < B
};

static const UINT32 nCuttoffs = sizeof(cutoffs) / sizeof(cutoffs[0]);

VOID
SYMCRYPT_CALL
SymCryptModExpWindowed(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peBase,
    _In_                            PCSYMCRYPT_INT          piExp,
                                    UINT32                  nBitsExp,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 W = 0;
    UINT32 nTableElements = 0;

    SYMCRYPT_SCSTABLE scsPrecomp = { 0 };
    UINT32 cbScsPrecomp = 0;

    UINT32 cbModElement = SymCryptSizeofModElementFromModulus( pmMod );

    PSYMCRYPT_MODELEMENT peT1 = NULL;
    PSYMCRYPT_MODELEMENT peT2 = NULL;

    UINT32 nIterations = 0;
    UINT32 iBit = 0;
    UINT32 nBits = 0;
    UINT32 index = 0;

    // Truncate the nBitsExp if above the object size
    nBitsExp = SYMCRYPT_MIN( nBitsExp, SymCryptIntBitsizeOfObject(piExp) );

    // Calculate the window size
    W = MIN_WINDOW_SIZE;
    while ((W-MIN_WINDOW_SIZE < nCuttoffs) && (cutoffs[W-MIN_WINDOW_SIZE]<nBitsExp))
    {
        W++;
    }
    nTableElements = (1<<W);

    // Initialize the table of temporary modelements
    cbScsPrecomp = SymCryptScsTableInit( &scsPrecomp, nTableElements, cbModElement );

    SYMCRYPT_ASSERT( cbScratch >= cbScsPrecomp + 2*cbModElement + SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pmMod->nDigits ) );

    SymCryptScsTableSetBuffer( &scsPrecomp, pbScratch, cbScsPrecomp );
    pbScratch += cbScsPrecomp;
    cbScratch -= cbScsPrecomp;

    // Create the temporary modelement
    peT1 = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
    SYMCRYPT_ASSERT( peT1 != NULL );
    pbScratch += cbModElement;
    cbScratch -= cbModElement;
    peT2 = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
    SYMCRYPT_ASSERT( peT2 != NULL );
    pbScratch += cbModElement;
    cbScratch -= cbModElement;

    // Fill the first element with 1 (**note: this will cause 0^0 = 1)
    // and the second with peBase
    SYMCRYPT_ASSERT( nTableElements >= 2 );

    SymCryptModElementSetValueUint32( 1, pmMod, peT1, pbScratch, cbScratch );
    SymCryptScsTableStore( &scsPrecomp, 0, (PBYTE)peT1, cbModElement );

    SymCryptModElementCopy( pmMod, peBase, peT1 );
    SymCryptScsTableStore( &scsPrecomp, 1, (PBYTE)peT1, cbModElement );

    // Fill the table with the powers of peBase
    for (UINT32 i=2; i<nTableElements; i++)
    {
        // TODO: Future improvement, use squarings for this table.
        SymCryptModMul( pmMod, peT1, peBase, peT1, pbScratch, cbScratch );
        SymCryptScsTableStore( &scsPrecomp, i, (PBYTE)peT1, cbModElement );
    }

    // Find the number of iterations (minus one) and the starting position bit
    SYMCRYPT_ASSERT( nBitsExp != 0 );
    nIterations = (nBitsExp - 1) / W;
    iBit = nIterations * W;

    // Do the first chunk (it might be smaller than W bits)
    nBits = nBitsExp - iBit;
    index = SymCryptIntGetBits( piExp, iBit, nBits );
    SymCryptScsTableLoad( &scsPrecomp, index, (PBYTE)peT1, cbModElement );

    // Work in batches of W bits in the exponent
    for (UINT32 i=0; i<nIterations; i++)
    {
        // Square W times
        for (UINT32 j=0; j<W; j++)
        {
            SymCryptModSquare( pmMod, peT1, peT1, pbScratch, cbScratch );
        }

        iBit -= W;
        index = SymCryptIntGetBits( piExp, iBit, W );
        SymCryptScsTableLoad( &scsPrecomp, index, (PBYTE)peT2, cbModElement );

        SymCryptModMul( pmMod, peT1, peT2, peT1, pbScratch, cbScratch );
    }

    SYMCRYPT_ASSERT( iBit == 0 );

    SymCryptModElementCopy( pmMod, peT1, peDst );
}

VOID
SYMCRYPT_CALL
SymCryptModExpSquareAndMultiply32(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peBase,
    _In_                            PCSYMCRYPT_INT          piExp,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 cbModElement = SymCryptSizeofModElementFromModulus( pmMod );

    PSYMCRYPT_MODELEMENT peT1 = NULL;
    PSYMCRYPT_MODELEMENT peT2 = NULL;

    // The bits of the exponent when this function is called are
    // always less than 32.
    UINT32 exp = SymCryptIntGetValueLsbits32( piExp );

    SYMCRYPT_ASSERT( cbScratch >= 2*cbModElement + SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pmMod->nDigits ) );

    // Create the temporary modelements
    peT1 = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
    SYMCRYPT_ASSERT( peT1 != NULL );
    pbScratch += cbModElement;
    cbScratch -= cbModElement;
    peT2 = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
    SYMCRYPT_ASSERT( peT2 != NULL );
    pbScratch += cbModElement;
    cbScratch -= cbModElement;

    if (exp == 0)
    {
        SymCryptModElementSetValueUint32( 1, pmMod, peDst, pbScratch, cbScratch );
    }
    else
    {
        SymCryptModElementSetValueUint32( 1, pmMod, peT1, pbScratch, cbScratch );
        SymCryptModElementCopy( pmMod, peBase, peT2 );

        while (exp>1)
        {
            if (exp%2 == 1)
            {
                SymCryptModMul( pmMod, peT1, peT2, peT1, pbScratch, cbScratch );
            }

            SymCryptModSquare( pmMod, peT2, peT2, pbScratch, cbScratch );
            exp /= 2;
        }

        SymCryptModMul( pmMod, peT1, peT2, peDst, pbScratch, cbScratch );
    }
}

VOID
SYMCRYPT_CALL
SymCryptModExpGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peBase,
    _In_                            PCSYMCRYPT_INT          piExp,
                                    UINT32                  nBitsExp,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    if ( ((flags & SYMCRYPT_FLAG_DATA_PUBLIC)!=0) && (nBitsExp <= sizeof(UINT32)*8) )
    {
        SymCryptModExpSquareAndMultiply32( pmMod, peBase, piExp, peDst, pbScratch, cbScratch );
    }
    else
    {
        SymCryptModExpWindowed( pmMod, peBase, piExp, nBitsExp, peDst, pbScratch, cbScratch );  // This is the default
    }
}

//
// MultiExponentiation
//

// SYMCRYPT_MODMULTIEXP_MAX_NPRECOMP: The maximum number of precomputed powers of the
// base point allowed for the multi-exponentiation operation.
//  It should be equal to 2^(SYMCRYPT_FDEF_MAX_WINDOW_MODEXP-1)
#define SYMCRYPT_MODMULTIEXP_MAX_NPRECOMP       (1<<(SYMCRYPT_FDEF_MAX_WINDOW_MODEXP-1))

// SYMCRYPT_MODMULTIEXP_WINDOW_SIZE: Fixed window size for the WnafWithInterleaving
// implementation. It is found to give the faster running times for sizes
// 512 - 2048 bits.
#define SYMCRYPT_MODMULTIEXP_WINDOW_SIZE        (5)

C_ASSERT( (1 << (SYMCRYPT_MODMULTIEXP_WINDOW_SIZE-1)) <= SYMCRYPT_MODMULTIEXP_MAX_NPRECOMP );

//
// The following function fills the table with odd powers
// of the base point B.
//
// The first value must be filled by the caller.
VOID
SYMCRYPT_CALL
SymCryptModExpPrecomputation(
    _In_    PCSYMCRYPT_MODULUS      pmP,
            UINT32                  nPrecomputedPowers,
    _In_reads_( SYMCRYPT_MODMULTIEXP_MAX_NPRECOMP )
            PSYMCRYPT_MODELEMENT *  pePIs,
            PSYMCRYPT_MODELEMENT    peTemp,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch
)
{
    SYMCRYPT_ASSERT(nPrecomputedPowers>=2);

    // Calculate B^2
    SymCryptModSquare( pmP, pePIs[0], peTemp, pbScratch, cbScratch );

    for (UINT32 i=1; i<nPrecomputedPowers; i++)
    {
        // B[i] = B^2*B[i-1]
        SymCryptModMul( pmP, peTemp, pePIs[i-1], pePIs[i], pbScratch, cbScratch );
    }
}

//
// The following is a similar algorithm to SymCryptEcpointMultiScalarMulWnafWithInterleaving.
// It is a NON SIDE-CHANNEL SAFE algorithm.
//
VOID
SYMCRYPT_CALL
SymCryptModMultiExpWnafWithInterleaving(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_reads_( nBases )            PCSYMCRYPT_MODELEMENT * peBaseArray,
    _In_reads_( nBases )            PCSYMCRYPT_INT *        piExpArray,
                                    UINT32                  nBases,
                                    UINT32                  nBitsExp,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32  i, j;

    UINT32  w = 0;
    UINT32  nPrecompPoints = 0;
    UINT32  nRecodedDigits = 0;

    // Masks
    UINT32  fOne[SYMCRYPT_MODMULTIEXP_MAX_NBASES] = { 0 };
    UINT32  fOneTot = 0xffffffff;       // Final result 1

    UINT32  fZeroExp = 0;               // Zero exponent
    UINT32  fZeroTot = 0;               // Final result 0

    UINT32 cbModElement = SymCryptSizeofModElementFromModulus( pmMod );

    // ====================================================
    // Temporaries
    PSYMCRYPT_MODELEMENT    pePIs[SYMCRYPT_MODMULTIEXP_MAX_NBASES*SYMCRYPT_MODMULTIEXP_MAX_NPRECOMP] = { 0 };
    PSYMCRYPT_MODELEMENT    peTemp = NULL;
    PSYMCRYPT_MODELEMENT    peOne = NULL;

    PUINT32                 absofKIs = NULL;
    // ===================================================

    // Calculate the window size
    w = SYMCRYPT_MODMULTIEXP_WINDOW_SIZE;
    nPrecompPoints = (1 << (w-1));          // We only store odd powers of the base point

    // Number of recoded digits
    nRecodedDigits = nBitsExp;

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    //      - nBases, nPrecompPoints, and nRecodedDigits are bounded by SYMCRYPT_MODMULTIEXP_MAX_NBASES,
    //        SYMCRYPT_MODMULTIEXP_MAX_NBITSEXP, and SYMCRYPT_MODMULTIEXP_MAX_NPRECOMP, respectively.
    // Thus the following calculation does not overflow cbScratch.
    //
    SYMCRYPT_ASSERT( SYMCRYPT_MODMULTIEXP_MAX_NBASES >= nBases );
    SYMCRYPT_ASSERT( SYMCRYPT_MODMULTIEXP_MAX_NPRECOMP >= nPrecompPoints );

    // Creating temporary precomputed modelements
    for (i=0; i<nBases*nPrecompPoints; i++)
    {
        SYMCRYPT_ASSERT( cbScratch >= cbModElement );
        pePIs[i] = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
        SYMCRYPT_ASSERT( pePIs[i] != NULL );
        pbScratch += cbModElement;
        cbScratch -= cbModElement;
    }

    SYMCRYPT_ASSERT( cbScratch >=
            2*cbModElement +
            ((nBases*nRecodedDigits*sizeof(UINT32) + SYMCRYPT_ASYM_ALIGN_VALUE - 1)/SYMCRYPT_ASYM_ALIGN_VALUE)*SYMCRYPT_ASYM_ALIGN_VALUE +
            SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( SymCryptModulusDigitsizeOfObject( pmMod ) ) );

    // Creating temporary points
    peTemp = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
    SYMCRYPT_ASSERT( peTemp != NULL );
    pbScratch += cbModElement;
    cbScratch -= cbModElement;

    peOne = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
    SYMCRYPT_ASSERT( peOne != NULL );
    pbScratch += cbModElement;
    cbScratch -= cbModElement;

    // Fixing pointers to recoded digits (be careful that the remaining space is SYMCRYPT_ASYM_ALIGNed)
    absofKIs = (PUINT32) pbScratch;
    pbScratch += nBases * nRecodedDigits * sizeof(UINT32);
    cbScratch -= nBases * nRecodedDigits * sizeof(UINT32);

    // Update cbScratch first using pbScratch, as the amount of scratch skipped for alignment depends upon the alignment of pbScratch
    cbScratch -=        ( ((SIZE_T)pbScratch + SYMCRYPT_ASYM_ALIGN_VALUE - 1) & ~(SYMCRYPT_ASYM_ALIGN_VALUE - 1) ) - (SIZE_T)pbScratch;
    pbScratch = (PBYTE) ( ((SIZE_T)pbScratch + SYMCRYPT_ASYM_ALIGN_VALUE - 1) & ~(SYMCRYPT_ASYM_ALIGN_VALUE - 1) );


    //
    // Main algorithm
    //

    // Set peOne to 1
    SymCryptModElementSetValueUint32( 1, pmMod, peOne, pbScratch, cbScratch );

    // Zero-out all recoded digits
    SymCryptWipe( (PBYTE)absofKIs, nBases*nRecodedDigits*sizeof(UINT32) );

    for (j = 0; j<nBases; j++)
    {
        // Check if the exponent is zero
        fZeroExp = SymCryptIntIsEqualUint32( piExpArray[j], 0 );

        // Check if the result is 0 (i.e. 0^e with e!=0)
        if( !fZeroExp && SymCryptModElementIsZero(pmMod, peBaseArray[j]) )
        {
            fZeroTot = 0xffffffff;
            break;
        }

        // Check if the exponent is 0 or if the base point is 1
        fOne[j] = ( fZeroExp | SymCryptModElementIsEqual( pmMod, peBaseArray[j], peOne ) );
        fOneTot &= fOne[j];

        // Skip the recoding stage (and all remaining steps) if this point will give result 1
        if (!fOne[j])
        {
            // Recoding stage
            SymCryptPositiveWidthNafRecoding( w, piExpArray[j], nBitsExp, &absofKIs[j*nRecodedDigits], nRecodedDigits );

            // Copy the base in the start of the pePIs array
            SymCryptModElementCopy( pmMod, peBaseArray[j], pePIs[j*nPrecompPoints] );

            // Precomputation stage
            SymCryptModExpPrecomputation( pmMod, nPrecompPoints, &pePIs[j*nPrecompPoints], peTemp, pbScratch, cbScratch );
        }
    }

    if (fZeroTot)
    {
        SymCryptModElementSetValueUint32( 0, pmMod, peDst, pbScratch, cbScratch );
    }
    else
    {
        SymCryptModElementSetValueUint32( 1, pmMod, peTemp, pbScratch, cbScratch );

        if (!fOneTot)
        {
            // Main loop
            for (INT32 i = nRecodedDigits-1; i>-1; i--)
            {
                SymCryptModSquare( pmMod, peTemp, peTemp, pbScratch, cbScratch );

                for (j = 0; j<nBases; j++)
                {
                    if (absofKIs[j*nRecodedDigits + i] != 0)
                    {
                        SymCryptModMul( pmMod, peTemp, pePIs[j*nPrecompPoints + absofKIs[j*nRecodedDigits + i]/2], peTemp, pbScratch, cbScratch );
                    }
                }
            }
        }

        // Copy the result into the destination
        SymCryptModElementCopy( pmMod, peTemp, peDst );
    }
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModMultiExpGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_reads_( nBases )            PCSYMCRYPT_MODELEMENT * peBaseArray,
    _In_reads_( nBases )            PCSYMCRYPT_INT *        piExpArray,
                                    UINT32                  nBases,
                                    UINT32                  nBitsExp,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if ( (nBases > SYMCRYPT_MODMULTIEXP_MAX_NBASES) ||
         (nBitsExp > SYMCRYPT_MODMULTIEXP_MAX_NBITSEXP) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if ((flags & SYMCRYPT_FLAG_DATA_PUBLIC)!=0)
    {
        SymCryptModMultiExpWnafWithInterleaving( pmMod, peBaseArray, piExpArray, nBases, nBitsExp, peDst, pbScratch, cbScratch );
    }
    else
    {
        UINT32 cbModElement = 0;
        PSYMCRYPT_MODELEMENT peTemp = NULL;
        PSYMCRYPT_MODELEMENT peAcc = NULL;

        // Use two temporary modelements to store the results
        // *** Make sure that the scratch space is enough i.e. the scratch space of ModMultiExp is
        //  at least 2 modelements bigger than the scratch space of ModExp
        cbModElement = SymCryptSizeofModElementFromModulus( pmMod );

        SYMCRYPT_ASSERT( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP(SymCryptModulusDigitsizeOfObject(pmMod)) + 2*cbModElement <=
                         SYMCRYPT_SCRATCH_BYTES_FOR_MODMULTIEXP( SymCryptModulusDigitsizeOfObject(pmMod), nBases, nBitsExp ) );
        SYMCRYPT_ASSERT( cbScratch >= 2*cbModElement + SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP(SymCryptModulusDigitsizeOfObject(pmMod)) );

        peTemp = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
        pbScratch += cbModElement; cbScratch -= cbModElement;

        peAcc = SymCryptModElementCreate( pbScratch, cbModElement, pmMod );
        pbScratch += cbModElement; cbScratch -= cbModElement;

        // Set peAcc to 1
        SymCryptModElementSetValueUint32( 1, pmMod, peAcc, pbScratch, cbScratch );

        for (UINT32 i=0; i<nBases; i++)
        {
            SymCryptModExpWindowed( pmMod, peBaseArray[i], piExpArray[i], nBitsExp, peTemp, pbScratch, cbScratch );

            SymCryptModMul( pmMod, peAcc, peTemp, peAcc, pbScratch, cbScratch );
        }

        // Copy the result into the destination
        SymCryptModElementCopy( pmMod, peAcc, peDst );
    }

cleanup:
    return scError;
}
