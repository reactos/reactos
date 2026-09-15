//
// dlgroup.c   Dlgroup functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

// Miller-Rabin iterations for prime generation
#define DLGROUP_MR_ITERATIONS       (64)

// Default size for Q according to FIPS 186-3
static const struct _DSA_NBITSOFQ_CUTOFFS {
    UINT32 nBitsOfP;
    UINT32 nBitsOfQ;
} g_nBitsOfQ_Cutoffs[] = {
    { 1024, 160 },
    { 2048, 256 },
    { UINT32_MAX, 256 },
};

// Const label for the generation of generator G according to FIPS 186-3
static const BYTE ggen[] = { 'g', 'g', 'e', 'n' };

UINT32
SYMCRYPT_CALL
SymCryptDlgroupCalculateBitsizeOfQ( UINT32 nBitsOfP )
{
    UINT32 i = 0;
    while ( (i<SYMCRYPT_ARRAY_SIZE(g_nBitsOfQ_Cutoffs) - 1) &&
            (g_nBitsOfQ_Cutoffs[i].nBitsOfP < nBitsOfP) )
    {
        i++;
    };

    return g_nBitsOfQ_Cutoffs[i].nBitsOfQ;
}

PSYMCRYPT_DLGROUP
SYMCRYPT_CALL
SymCryptDlgroupAllocate( UINT32  nBitsOfP, UINT32  nBitsOfQ )
{
    PVOID               p;
    SIZE_T              cb;
    PSYMCRYPT_DLGROUP   res = NULL;

    // Invalid parameters
    if ( (nBitsOfP < SYMCRYPT_DLGROUP_MIN_BITSIZE_P) ||
         ((nBitsOfQ > 0) && (nBitsOfQ < SYMCRYPT_DLGROUP_MIN_BITSIZE_Q)) ||
         (nBitsOfP < nBitsOfQ) )
    {
        goto cleanup;
    }

    cb = SymCryptSizeofDlgroupFromBitsizes( nBitsOfP, nBitsOfQ );

    p = SymCryptCallbackAlloc( cb );

    if ( p==NULL )
    {
        goto cleanup;
    }

    res = SymCryptDlgroupCreate( p, cb, nBitsOfP, nBitsOfQ );

cleanup:
    return res;
}

VOID
SYMCRYPT_CALL
SymCryptDlgroupFree( _Out_ PSYMCRYPT_DLGROUP pgObj )
{
    SYMCRYPT_CHECK_MAGIC( pgObj );
    SymCryptDlgroupWipe( pgObj );
    SymCryptCallbackFree( pgObj );
}

UINT32
SYMCRYPT_CALL
SymCryptSizeofDlgroupFromBitsizes( UINT32 nBitsOfP, UINT32 nBitsOfQ )
{
    UINT32 cbSeed = 0;

    if (nBitsOfQ == 0)
    {
        nBitsOfQ = nBitsOfP-1;        // Default to the maximum possible size for Q
    }

    // Invalid parameters
    if ( (nBitsOfP < SYMCRYPT_DLGROUP_MIN_BITSIZE_P) ||
         (nBitsOfQ < SYMCRYPT_DLGROUP_MIN_BITSIZE_Q) ||
         (nBitsOfP < nBitsOfQ) )
    {
        return 0;
    }

    if ( nBitsOfP == nBitsOfQ )
    {
        nBitsOfQ--;
    }

    // Calculate the (tight) bytesize of the seed
    cbSeed = (nBitsOfQ+7)/8;

    return  sizeof(SYMCRYPT_DLGROUP) +
            SYMCRYPT_SIZEOF_MODULUS_FROM_BITS( nBitsOfP ) +
            SYMCRYPT_SIZEOF_MODULUS_FROM_BITS( nBitsOfQ ) +
            SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( nBitsOfP ) +
            ((cbSeed + SYMCRYPT_ASYM_ALIGN_VALUE - 1)/SYMCRYPT_ASYM_ALIGN_VALUE)*SYMCRYPT_ASYM_ALIGN_VALUE;    // Make sure that the entire structure is ASYM_ALIGNED.
}

PSYMCRYPT_DLGROUP
SYMCRYPT_CALL
SymCryptDlgroupCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
                                    UINT32              nBitsOfP,
                                    UINT32              nBitsOfQ )
{
    PSYMCRYPT_DLGROUP       pDlgroup = NULL;

    UINT32                  cbModP;
    UINT32                  cbModQ;
    UINT32                  cbModElement;

    SYMCRYPT_ASSERT( cbBuffer >= SymCryptSizeofDlgroupFromBitsizes( nBitsOfP, nBitsOfQ ) );
    UNREFERENCED_PARAMETER( cbBuffer );     // only referenced in ASSERTs...
    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );

    // Invalid parameters
    if ( (nBitsOfP < SYMCRYPT_DLGROUP_MIN_BITSIZE_P) ||
         ((nBitsOfQ > 0) && (nBitsOfQ < SYMCRYPT_DLGROUP_MIN_BITSIZE_Q)) ||
         (nBitsOfP < nBitsOfQ) )
    {
        goto cleanup;
    }

    if ( nBitsOfP == nBitsOfQ )
    {
        nBitsOfQ--;
    }

    pDlgroup = (PSYMCRYPT_DLGROUP) pbBuffer;

    SYMCRYPT_ASSERT( cbBuffer > sizeof(SYMCRYPT_DLGROUP) );

    // DLGROUP parameters
    pDlgroup->cbTotalSize = SymCryptSizeofDlgroupFromBitsizes( nBitsOfP, nBitsOfQ );
    pDlgroup->fHasPrimeQ = FALSE;

    pDlgroup->nBitsOfP = nBitsOfP;
    pDlgroup->cbPrimeP = (nBitsOfP+7)/8;
    pDlgroup->nDigitsOfP = SymCryptDigitsFromBits( nBitsOfP );
    pDlgroup->nMaxBitsOfP = nBitsOfP;

    pDlgroup->nBitsOfQ = nBitsOfQ;                                             // 0 value possible
    pDlgroup->cbPrimeQ = (nBitsOfQ+7)/8;                                       // 0 value possible
    pDlgroup->nDigitsOfQ = (nBitsOfQ>0)?SymCryptDigitsFromBits( nBitsOfQ ):0;  // 0 value possible
    pDlgroup->nMaxBitsOfQ = (nBitsOfQ==0)?(nBitsOfP-1):nBitsOfQ;

    pDlgroup->isSafePrimeGroup = FALSE;
    pDlgroup->nMinBitsPriv = 0;
    pDlgroup->nDefaultBitsPriv = nBitsOfQ;                                     // 0 value possible

    pDlgroup->nBitsOfSeed = nBitsOfQ;                                          // 0 value possible
    pDlgroup->cbSeed = (pDlgroup->nBitsOfSeed+7)/8;                            // 0 value possible

    pDlgroup->eFipsStandard = SYMCRYPT_DLGROUP_FIPS_NONE;       // This will be set either on generate or import
    pDlgroup->pHashAlgorithm = NULL;            // Like-wise
    pDlgroup->dwGenCounter = 0;                 // Like-wise
    pDlgroup->bIndexGenG = 1;                   // Default: 1

    // Create SymCrypt objects
    pbBuffer += sizeof(SYMCRYPT_DLGROUP);

    cbModP = SymCryptSizeofModulusFromDigits( pDlgroup->nDigitsOfP );
    SYMCRYPT_ASSERT( cbBuffer > sizeof(SYMCRYPT_DLGROUP) + cbModP );
    pDlgroup->pmP = SymCryptModulusCreate( pbBuffer, cbModP, pDlgroup->nDigitsOfP );
    pbBuffer += cbModP;

    //
    // **** Always defer the creation of the Q modulus until the group generation or
    // import of the modulus. This way it is always the fastest possible even when the caller
    // specified nBitsOfQ = 0.
    //
    if (nBitsOfQ>0)
    {
        cbModQ = SymCryptSizeofModulusFromDigits( pDlgroup->nDigitsOfQ );
    }
    else
    {
        cbModQ = cbModP;
    }
    SYMCRYPT_ASSERT( cbBuffer > sizeof(SYMCRYPT_DLGROUP) + cbModP + cbModQ );
    pDlgroup->pbQ = pbBuffer;      // Set the aligned buffer
    pDlgroup->pmQ = NULL;
    pbBuffer += cbModQ;

    cbModElement = SymCryptSizeofModElementFromModulus( pDlgroup->pmP );
    SYMCRYPT_ASSERT( cbBuffer > sizeof(SYMCRYPT_DLGROUP) + cbModP + cbModQ + cbModElement );
    pDlgroup->peG = SymCryptModElementCreate( pbBuffer, cbModElement, pDlgroup->pmP );
    pbBuffer += cbModElement;

    pDlgroup->pbSeed = pbBuffer;

    // Setting the magic
    SYMCRYPT_SET_MAGIC( pDlgroup );

cleanup:
    return pDlgroup;
}

VOID
SYMCRYPT_CALL
SymCryptDlgroupWipe( _Out_ PSYMCRYPT_DLGROUP pgDst )
{
    SymCryptWipe( (PBYTE) pgDst, pgDst->cbTotalSize );
}

VOID
SYMCRYPT_CALL
SymCryptDlgroupCopy(
    _In_    PCSYMCRYPT_DLGROUP   pgSrc,
    _Out_   PSYMCRYPT_DLGROUP    pgDst )
{
    //
    // in-place copy is somewhat common...
    //
    if( pgSrc != pgDst )
    {
        pgDst->cbTotalSize = pgSrc->cbTotalSize;
        pgDst->fHasPrimeQ = pgSrc->fHasPrimeQ;

        pgDst->nBitsOfP = pgSrc->nBitsOfP;
        pgDst->cbPrimeP = pgSrc->cbPrimeP;
        pgDst->nDigitsOfP = pgSrc->nDigitsOfP;
        pgDst->nMaxBitsOfP = pgSrc->nMaxBitsOfP;

        pgDst->nBitsOfQ = pgSrc->nBitsOfQ;
        pgDst->cbPrimeQ = pgSrc->cbPrimeQ;
        pgDst->nDigitsOfQ = pgSrc->nDigitsOfQ;
        pgDst->nMaxBitsOfQ = pgSrc->nMaxBitsOfQ;

        pgDst->isSafePrimeGroup = pgSrc->isSafePrimeGroup;
        pgDst->nMinBitsPriv = pgSrc->nMinBitsPriv;
        pgDst->nDefaultBitsPriv = pgSrc->nDefaultBitsPriv;

        pgDst->nBitsOfSeed = pgSrc->nBitsOfSeed;
        pgDst->cbSeed = pgSrc->cbSeed;

        pgDst->eFipsStandard = pgSrc->eFipsStandard;
        pgDst->pHashAlgorithm = pgSrc->pHashAlgorithm;
        pgDst->dwGenCounter = pgSrc->dwGenCounter;
        pgDst->bIndexGenG = pgSrc->bIndexGenG;
        pgDst->pbQ = pgSrc->pbQ;

        memcpy( (PBYTE)pgDst + sizeof(SYMCRYPT_DLGROUP), (PCBYTE)pgSrc + sizeof(SYMCRYPT_DLGROUP), pgSrc->cbTotalSize - sizeof(SYMCRYPT_DLGROUP) );
    }
}


// DLGROUP-specific functions

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupGeneratePrimeQ_FIPS(
    _In_    PSYMCRYPT_DLGROUP   pDlgroup,
    _In_    PCSYMCRYPT_TRIALDIVISION_CONTEXT
                                pTrialDivisionContext,
    _Out_   PUINT32             pfPrimeQFound,
    _Out_   PSYMCRYPT_INT       piQ,
    _Out_   PSYMCRYPT_DIVISOR   pdDivTwoQ,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_HASH hashAlgorithm = pDlgroup->pHashAlgorithm;
    UINT32 nBitsOfQ = pDlgroup->nBitsOfQ;
    UINT32 cbPrimeQ = pDlgroup->cbPrimeQ;
    PBYTE pbSeed = pDlgroup->pbSeed;
    UINT32 cbSeed = pDlgroup->cbSeed;

    PSYMCRYPT_INT piDivTwoQ = SymCryptIntFromDivisor(pdDivTwoQ);

    SIZE_T cbHash = SymCryptHashResultSize( hashAlgorithm );
    PBYTE pbTrHash = NULL;          // Pointer to the truncated hash value
    PBYTE pbHashExtra = NULL;       // Needed as temp buffer for 186-2

    UINT32 dwShiftBits = (8-nBitsOfQ%8)%8;              // When nBitsOfQ is a multiple of 8 -> dwShiftBits = 0;

    UINT32 carry = 0;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_DIVISOR(SymCryptDigitsFromBits(nBitsOfQ+1)),
                                  SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_IS_PRIME(pDlgroup->nDigitsOfQ),
                                       2 * cbHash )) );
    SYMCRYPT_ASSERT( cbHash >= cbPrimeQ );

    // Hash the seed according to the standard specified
    if (pDlgroup->eFipsStandard == SYMCRYPT_DLGROUP_FIPS_186_2)
    {
        SYMCRYPT_ASSERT( hashAlgorithm == SymCryptSha1Algorithm );
        SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_MAX(2*cbHash, cbSeed) );

        // Hash buffers
        pbTrHash = pbScratch;
        pbHashExtra = pbTrHash + cbHash;

        // Prepare an int for SEED + 1
        scError = SymCryptIntSetValue( pbSeed, cbSeed, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piDivTwoQ );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // Add 1
        carry = SymCryptIntAddUint32( piDivTwoQ, 1, piDivTwoQ );
        if (carry > 0)
        {
            // This should never happen as the size of piDivTwoQ is at least one bit bigger than Q
            scError = SYMCRYPT_FIPS_FAILURE;
            goto cleanup;
        }

        // (SEED+1) Mod 2^nBitsOfSeed
        SymCryptIntModPow2( piDivTwoQ, nBitsOfQ, piDivTwoQ );

        // Get the value into pbTrHash (Notice the cbSeed size)
        scError = SymCryptIntGetValue( piDivTwoQ, pbTrHash, cbSeed, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // Hash it into pbHashExtra
        SymCryptHash( hashAlgorithm, pbTrHash, cbPrimeQ, pbHashExtra, cbHash );

        // Hash the seed
        SymCryptHash( hashAlgorithm, pbSeed, cbSeed, pbTrHash, cbHash );

        // Xor the two
        SymCryptXorBytes( pbTrHash, pbHashExtra, pbTrHash, cbHash );

    }
    else if (pDlgroup->eFipsStandard == SYMCRYPT_DLGROUP_FIPS_186_3)
    {
        SYMCRYPT_ASSERT( cbScratch >= cbHash );
        pbTrHash = pbScratch;
        SymCryptHash( hashAlgorithm, pbSeed, cbSeed, pbTrHash, cbHash );
    }
    else
    {
        scError = SYMCRYPT_FIPS_FAILURE;
        goto cleanup;
    }

    // Convert it to (2^{N-1} + (Hash mod 2^{N-1})) | 1
    pbTrHash += (cbHash-cbPrimeQ);                      // Skip any leading zero bytes
    pbTrHash[0] &= ((BYTE)0xff >> (dwShiftBits));       // Cut off top bits in the most significant byte
    pbTrHash[0] |= ((BYTE)0x01 << (7 - dwShiftBits));   // Set the (N-1)-th bit
    pbTrHash[cbPrimeQ-1] |= ((BYTE)0x01);               // Make the entire number odd

    // Set the value
    scError = SymCryptIntSetValue( pbTrHash, cbPrimeQ, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piQ );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Assume not a prime
    *pfPrimeQFound = 0;

    // Fast compositeness check
    if (SymCryptIntFindSmallDivisor( pTrialDivisionContext, piQ, NULL, 0 ))
    {
        goto cleanup;
    }

    // IntMillerRabinPrimalityTest requirement:
    //      piQ > 3 since nBitsOfQ is bounded by SYMCRYPT_DLGROUP_MIN_BITSIZE_Q
    *pfPrimeQFound = SymCryptIntMillerRabinPrimalityTest(
                        piQ,
                        nBitsOfQ,
                        DLGROUP_MR_ITERATIONS,
                        SYMCRYPT_FLAG_DATA_PUBLIC,      // q and p will be public
                        pbScratch,
                        cbScratch );

    // Set pdDivTwoQ
    if (*pfPrimeQFound)
    {
        scError = SymCryptIntCopyMixedSize( piQ, piDivTwoQ );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        SymCryptIntMulPow2( piDivTwoQ, 1, piDivTwoQ );

        // IntToDivisor requirement:
        //      Q is non-zero as prime --> 2*Q != 0
        SymCryptIntToDivisor(
            piDivTwoQ,
            pdDivTwoQ,
            4*pDlgroup->nBitsOfP,              // 4*L
            SYMCRYPT_FLAG_DATA_PUBLIC,
            pbScratch,
            cbScratch );
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupGeneratePrimeP_FIPS(
    _In_    PSYMCRYPT_DLGROUP   pDlgroup,
    _In_    PSYMCRYPT_DIVISOR   pdDivTwoQ,
    _In_    UINT32              dwMaxCounter,       // Maximum value of counter (used in validation)
    _In_    PCSYMCRYPT_TRIALDIVISION_CONTEXT
                                pTrialDivisionContext,
    _Out_   PUINT32             pfPrimePFound,
    _Out_   PSYMCRYPT_INT       piP,
    _Out_   PUINT32             pdwCounter,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_HASH hashAlgorithm = pDlgroup->pHashAlgorithm;
    UINT32 nBitsOfP = pDlgroup->nBitsOfP;
    PBYTE pbSeed = pDlgroup->pbSeed;
    UINT32 cbSeed = pDlgroup->cbSeed;
    UINT32 nBitsOfSeed = pDlgroup->nBitsOfSeed;

    SIZE_T cbHash = SymCryptHashResultSize( hashAlgorithm );

    UINT32 counter = 0;

    UINT32 ndDivTwoQ = SymCryptDivisorDigitsizeOfObject( pdDivTwoQ );
    UINT32 cbIntTwoQ = SymCryptSizeofIntFromDigits( ndDivTwoQ );

    PSYMCRYPT_INT piPersistent = NULL;
    PSYMCRYPT_INT piRemainder = NULL;

    PBYTE pbHashOutput = NULL;
    PBYTE pbTempSeed = NULL;

    PBYTE pbW = NULL;
    UINT32 cbW = pDlgroup->cbPrimeP;

    PBYTE pbWCurr = NULL;
    SIZE_T cbWBytesLeft = 0;

    UINT32 carry = 0;

    // We will use internal scratch space at the start of pbScratch
    // because cbHash, cbSeed and cbW are not necessarily aligned according
    // to SYMCRYPT_ASYM_ALIGN_VALUE
    PBYTE pbScratchInternal = 0;
    SIZE_T cbScratchInternal = 0;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= 2*cbIntTwoQ + cbHash + cbSeed + cbW +
                                  SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( pDlgroup->nDigitsOfP, ndDivTwoQ ),
                                       SYMCRYPT_SCRATCH_BYTES_FOR_INT_IS_PRIME( pDlgroup->nDigitsOfP )) );

    // Create temporaries
    pbScratchInternal = pbScratch;
    cbScratchInternal = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( pDlgroup->nDigitsOfP, ndDivTwoQ ),
                             SYMCRYPT_SCRATCH_BYTES_FOR_INT_IS_PRIME( pDlgroup->nDigitsOfP ) );
    pbScratch += cbScratchInternal;

    piPersistent = SymCryptIntCreate( pbScratch, cbIntTwoQ, ndDivTwoQ );
    pbScratch += cbIntTwoQ;

    piRemainder = SymCryptIntCreate( pbScratch, cbIntTwoQ, ndDivTwoQ );
    pbScratch += cbIntTwoQ;

    pbHashOutput = pbScratch;
    pbScratch += cbHash;

    pbTempSeed = pbScratch;
    pbScratch += cbSeed;

    pbW = pbScratch;

    // Set the value for the expression "domain_parameter_seed + offset + j"
    scError = SymCryptIntSetValue( pbSeed, cbSeed, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piPersistent );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // If the standard is 186-2 add 1 since the starting offset is 2
    if (pDlgroup->eFipsStandard == SYMCRYPT_DLGROUP_FIPS_186_2)
    {
        carry = SymCryptIntAddUint32( piPersistent, 1, piPersistent );
        if (carry!=0)
        {
            // This should never happen as piPersistent has at least one more bit than
            // seedLen == nBitsOfQ
            scError = SYMCRYPT_FIPS_FAILURE;
            goto cleanup;
        }

        // Mod 2^seedlen
        SymCryptIntModPow2( piPersistent, nBitsOfSeed, piPersistent );
    }

    *pfPrimePFound = 0;

    for (counter = 0; counter < dwMaxCounter+1; counter++)
    {
        cbWBytesLeft = cbW;                         // Bytes left to write
        pbWCurr = pbW + cbW - SYMCRYPT_MIN(cbW,cbHash);      // Position of the first hash chunk to write (if cbW < cbHash then we write only 1 chunk)

        while (cbWBytesLeft > 0)
        {
            // Add 1 to piPersistent
            // This can never generate a carry as piPersistent has at least one more bit than
            // seedLen == nBitsOfQ and in the next step we always do mod 2^seedlen.
            carry = SymCryptIntAddUint32( piPersistent, 1, piPersistent );
            if (carry!=0)
            {
                scError = SYMCRYPT_FIPS_FAILURE;
                goto cleanup;
            }

            // Mod 2^seedlen
            SymCryptIntModPow2( piPersistent, nBitsOfSeed, piPersistent );

            // Extract piPersistent into a byte array (this will always be equal to domain_parameter_seed + offset + j)
            scError = SymCryptIntGetValue( piPersistent, pbTempSeed, cbSeed, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST );
            if (scError != SYMCRYPT_NO_ERROR)
            {
                goto cleanup;
            }

            // Hash it
            SymCryptHash( hashAlgorithm, pbTempSeed, cbSeed, pbHashOutput, cbHash );

            if (cbWBytesLeft >= cbHash)
            {
                // Move the entire hash output to the correct location in the pbW buffer
                memcpy(pbWCurr, pbHashOutput, cbHash );
            }
            else
            {
                // Move only the last bytes of the hash output
                memcpy(pbWCurr, pbHashOutput + cbHash - cbWBytesLeft, cbWBytesLeft );
            }

            // Update the positions on the W buffer
            cbWBytesLeft -= SYMCRYPT_MIN(cbHash,cbWBytesLeft);
            pbWCurr -= SYMCRYPT_MIN(cbHash,cbWBytesLeft);
        }

        // Import the W buffer into P
        scError = SymCryptIntSetValue( pbW, cbW, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piP );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // Zero-out the top bits of the integer
        SymCryptIntModPow2( piP, nBitsOfP, piP );

        // Set the most significant bit
        SymCryptIntSetBits( piP, 1, nBitsOfP-1, 1);

        // At this point piP = X = W + 2^{L-1}

        // Calculate c = X mod 2Q
        SymCryptIntDivMod( piP, pdDivTwoQ, NULL, piRemainder, pbScratchInternal, cbScratchInternal );

        if (SymCryptIntIsEqualUint32(piRemainder, 0))
        {
            // Just add one to X
            // We can never get a carry here because the remainder X mod 2Q
            // is 0. Therefore X is even.
            carry = SymCryptIntAddUint32( piP, 1, piP );
            SYMCRYPT_ASSERT( carry==0 );
        }
        else
        {
            // Subtract 1 from c
            // We can never get a borrow here because the remainder is not 0.
            carry = SymCryptIntSubUint32( piRemainder, 1, piRemainder );
            SYMCRYPT_ASSERT( carry==0 );

            // X-(c-1)
            // We can never get a borrow here because c is smaller
            // or equal to X.
            carry = SymCryptIntSubMixedSize( piP, piRemainder, piP );
            SYMCRYPT_ASSERT( carry==0 );
        }

        // Check if smaller than 2^{L-1} by checking the L-1 bit
        if (SymCryptIntGetBit( piP, nBitsOfP-1 ) == 0)
        {
            continue;
        }

        // Fast compositeness check
        if (SymCryptIntFindSmallDivisor( pTrialDivisionContext, piP, NULL, 0 ))
        {
            continue;
        }

        // IntMillerRabinPrimalityTest requirement:
        //      piP > 3 since nBitsOfP is bounded by SYMCRYPT_DLGROUP_MIN_BITSIZE_P
        *pfPrimePFound = SymCryptIntMillerRabinPrimalityTest(
                        piP,
                        nBitsOfP,
                        DLGROUP_MR_ITERATIONS,
                        SYMCRYPT_FLAG_DATA_PUBLIC,      // q and p will be public
                        pbScratchInternal,
                        cbScratchInternal );

        if (*pfPrimePFound)
        {
            *pdwCounter = counter;
            break;
        }
    }
cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupGenerateGenG_FIPS(
    _In_    PSYMCRYPT_DLGROUP       pDlgroup,
    _Out_   PSYMCRYPT_MODELEMENT    peG,
    _Out_writes_bytes_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_HASH hashAlgorithm = pDlgroup->pHashAlgorithm;
    PCSYMCRYPT_MODULUS pmP = pDlgroup->pmP;
    UINT32 nDigitsOfP = pDlgroup->nDigitsOfP;
    UINT32 nBitsOfP = pDlgroup->nBitsOfP;
    PCSYMCRYPT_MODULUS pmQ = pDlgroup->pmQ;
    UINT32 nDigitsOfQ = pDlgroup->nDigitsOfQ;
    PBYTE pbSeed = pDlgroup->pbSeed;
    UINT32 cbSeed = pDlgroup->cbSeed;
    BYTE bIndexGenG = pDlgroup->bIndexGenG;

    SIZE_T cbHash = SymCryptHashResultSize( hashAlgorithm );
    SYMCRYPT_ASSERT( cbHash == hashAlgorithm->resultSize );
    SIZE_T cbState = SymCryptHashStateSize( hashAlgorithm );
    SYMCRYPT_ASSERT( cbState == hashAlgorithm->stateSize );

    UINT16 count = 0;
    BYTE bTmp = 0;

    PSYMCRYPT_INT piExp = NULL;
    PSYMCRYPT_INT piRem = NULL;
    PSYMCRYPT_MODELEMENT peOne = NULL;
    PBYTE pbState = NULL;
    PBYTE pbW = NULL;

    UINT32 cbExp = SymCryptSizeofIntFromDigits( nDigitsOfP );
    UINT32 cbRem = SymCryptSizeofIntFromDigits( nDigitsOfQ );
    UINT32 cbModElement = SymCryptSizeofModElementFromModulus( pmP );

    UINT32 borrow = 0;

    // We will use internal scratch space at the start of pbScratch
    // because cbHash is not necessarily aligned according
    // to SYMCRYPT_ASYM_ALIGN_VALUE
    PBYTE pbScratchInternal = 0;
    SIZE_T cbScratchInternal = 0;

    UNREFERENCED_PARAMETER( cbScratch );
    UNREFERENCED_PARAMETER( nDigitsOfQ );

    // Create temporaries
    pbScratchInternal = pbScratch;
    cbScratchInternal = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( nDigitsOfP ),
                        SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( nDigitsOfP, nDigitsOfQ ),
                             SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsOfP ) ));
    SYMCRYPT_ASSERT( cbScratch >= cbScratchInternal + cbExp + cbRem );
    SYMCRYPT_ASSERT( cbScratch >= cbScratchInternal + cbExp + cbModElement + cbHash + cbState );
    pbScratch += cbScratchInternal;

    piExp = SymCryptIntCreate( pbScratch, cbExp, nDigitsOfP );
    pbScratch += cbExp;

    piRem = SymCryptIntCreate( pbScratch, cbRem, nDigitsOfQ );

    // Calculate the exponent e = (p-1)/q
    borrow = SymCryptIntSubUint32( SymCryptIntFromModulus((PSYMCRYPT_MODULUS)pmP), 1, piExp );
    if (borrow!=0)
    {
        // The only way to get a borrow here is if the imported prime P
        // is zero and we generate a G from P and Q.
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptIntDivMod(
            piExp,
            SymCryptDivisorFromModulus( (PSYMCRYPT_MODULUS)pmQ ),
            piExp,
            piRem,
            pbScratchInternal,
            cbScratchInternal );

    if ( !SymCryptIntIsEqualUint32(piRem, 0) )
    {
        // The only way to get a non-zero remainder is if Q does not divide P-1
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // To reach here we have guaranteed that P and Q are odd, with bitlength >= 32b, and Q divides P-1.
    // It follows that piExp >= 2, as it must be even and non-zero.

    peOne = SymCryptModElementCreate( pbScratch, cbModElement, pmP);
    pbScratch += cbModElement;

    pbState = pbScratch;
    pbScratch += cbState;

    pbW = pbScratch;

    // Initialize the hash state
    SymCryptHashInit( hashAlgorithm, pbState );

    // Set the modelement equal to one
    SymCryptModElementSetValueUint32( 1, pmP, peOne, pbScratchInternal, cbScratchInternal );

    do
    {
        count += 1;

        if (count == 0)
        {
            scError = SYMCRYPT_FIPS_FAILURE;
            goto cleanup;
        }

        // Hash the seed
        SymCryptHashAppend( hashAlgorithm, pbState, pbSeed, cbSeed );

        // Hash the "ggen" string
        SymCryptHashAppend( hashAlgorithm, pbState, ggen, sizeof(ggen) );

        // Hash the index
        SymCryptHashAppend( hashAlgorithm, pbState, &bIndexGenG, sizeof(bIndexGenG) );

        // Hash the count (in MSB)
        bTmp = (BYTE)(count >> 8);
        SymCryptHashAppend( hashAlgorithm, pbState, &bTmp, sizeof(bTmp) );
        bTmp = (BYTE)count;
        SymCryptHashAppend( hashAlgorithm, pbState, &bTmp, sizeof(bTmp) );

        // Result into W
        SymCryptHashResult( hashAlgorithm, pbState, pbW, cbHash );

        // Set this into G
        scError = SymCryptModElementSetValue(
                        pbW,
                        cbHash,
                        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                        pmP,
                        peG,
                        pbScratchInternal,
                        cbScratchInternal );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // ModExp G in place
        SymCryptModExp(
            pmP,
            peG,
            piExp,
            nBitsOfP,
            SYMCRYPT_FLAG_DATA_PUBLIC,
            peG,
            pbScratchInternal,
            cbScratchInternal );

    } while (SymCryptModElementIsZero( pmP, peG ) || SymCryptModElementIsEqual( pmP, peG, peOne ));

cleanup:
    return scError;
}

// Scratch space requirements for the entire FIPS standards generation of P,Q,G
UINT32
SYMCRYPT_CALL
SymCryptDlgroupScratchSpace_FIPS( UINT32 nBitsOfP, UINT32 nBitsOfQ, PCSYMCRYPT_HASH pHashAlgorithm )
{
    UINT32 nDigitsOfP = SymCryptDigitsFromBits( nBitsOfP );
    UINT32 nDigitsOfQ = SymCryptDigitsFromBits( nBitsOfQ );
    UINT32 ndDivTwoQ = SymCryptDigitsFromBits(nBitsOfQ + 1);

    UINT32 cbPrimeP = (nBitsOfP+7)/8;   // Note: The upper bound for nBitsOfP is enforced by SymCryptDigitsFromBits
    UINT32 cbDivTwoQ = SymCryptSizeofDivisorFromDigits(ndDivTwoQ);
    UINT32 cbIntTwoQ = SymCryptSizeofIntFromDigits( ndDivTwoQ );
    UINT32 cbSeed = (nBitsOfQ+7)/8;     // Note: The upper bound for nBitsOfP is enforced by SymCryptDigitsFromBits

    UINT32 cbExp = SymCryptSizeofIntFromDigits( nDigitsOfP );
    UINT32 cbRem = SymCryptSizeofIntFromDigits( nDigitsOfQ );
    UINT32 cbModElement = SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( nBitsOfP );

    UINT32 cbHash = (UINT32)SymCryptHashResultSize( pHashAlgorithm );
    UINT32 cbState = (UINT32) SymCryptHashStateSize( pHashAlgorithm );

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    // Thus the following calculation does not overflow the result and is bounded by 2^28.
    //
    return SYMCRYPT_MAX( cbDivTwoQ + SYMCRYPT_MAX(
                            // Generate Q
                            SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( ndDivTwoQ ),
                            SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_IS_PRIME( nDigitsOfQ ),
                                 2 * cbHash)),
                            // Generate P
                            2*cbIntTwoQ + cbHash + cbSeed + cbPrimeP +
                            SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( nDigitsOfP, ndDivTwoQ ),
                                 SYMCRYPT_SCRATCH_BYTES_FOR_INT_IS_PRIME( nDigitsOfP )) ),
           SYMCRYPT_MAX(
                // Convert P and Q to moduli
                SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS( nDigitsOfP ),
                // Generate GenG
                cbExp + SYMCRYPT_MAX(cbRem, cbModElement + cbState + cbHash) +
                SYMCRYPT_MAX(SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( nDigitsOfP ),
                SYMCRYPT_MAX(SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( nDigitsOfP, nDigitsOfQ ),
                    SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigitsOfP ) )) ));
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupGenerate(
    _In_    PCSYMCRYPT_HASH         hashAlgorithm,
    _In_    SYMCRYPT_DLGROUP_FIPS   fipsStandard,
    _Inout_ PSYMCRYPT_DLGROUP       pDlgroup )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;
    PBYTE pbScratchInternal = NULL;
    SIZE_T cbScratchInternal = 0;

    UINT32 fPrimeQFound = 0;
    UINT32 fPrimePFound = 0;

    // A divisor equal to 2*Q will be needed for the generation of P
    PSYMCRYPT_DIVISOR pdDivTwoQ = NULL;
    UINT32 cbDivTwoQ = 0;
    UINT32 ndDivTwoQ = 0;

    UINT32 nBitsOfP = 0;
    UINT32 nDigitsOfP = 0;
    UINT32 nBitsOfQ = 0;
    UINT32 nDigitsOfQ = 0;

    PCSYMCRYPT_TRIALDIVISION_CONTEXT pTrialDivisionContext = NULL;

    if (fipsStandard == SYMCRYPT_DLGROUP_FIPS_NONE)
    {
        fipsStandard = SYMCRYPT_DLGROUP_FIPS_LATEST;
    }

    // Numbered comments refer to the steps in the FIPS standard
    // 1. Check that L,N is in the list of acceptable pairs
    //      => Skipped as SymCrypt supports more sizes

    // 2. Check that seedlen >= N
    //      => Skipped as we always have seedlen == N (see below)


    // Make sure that a hash algorithm is passed (if needed)
    // and set the FIPS standard
    if (fipsStandard == SYMCRYPT_DLGROUP_FIPS_186_2)
    {
        if (hashAlgorithm != NULL)
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        pDlgroup->eFipsStandard = fipsStandard;
        hashAlgorithm = SymCryptSha1Algorithm;
    }
    else
    {
        if (hashAlgorithm == NULL)
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        pDlgroup->eFipsStandard = fipsStandard;
    }

    // If during allocation the caller didn't know the size of Q
    // and set it to 0, pick the default bitsize here
    // and fix all the zero parameters.
    if (pDlgroup->nBitsOfQ == 0)
    {
        pDlgroup->nBitsOfQ = SymCryptDlgroupCalculateBitsizeOfQ(pDlgroup->nBitsOfP);

        if (pDlgroup->nBitsOfQ > pDlgroup->nMaxBitsOfQ)
        {
            scError = SYMCRYPT_FIPS_FAILURE;        // This hits when nMaxBitsOfQ = (nBitsOfP-1) <= 160
            goto cleanup;
        }

        pDlgroup->cbPrimeQ = (pDlgroup->nBitsOfQ + 7)/8;
        pDlgroup->nDigitsOfQ = SymCryptDigitsFromBits( pDlgroup->nBitsOfQ );
        pDlgroup->nDefaultBitsPriv = pDlgroup->nBitsOfQ;
        pDlgroup->nBitsOfSeed = pDlgroup->nBitsOfQ;
        pDlgroup->cbSeed = (pDlgroup->nBitsOfSeed+7)/8;
    }

    // Helper variables
    nBitsOfP = pDlgroup->nBitsOfP;
    nDigitsOfP = pDlgroup->nDigitsOfP;
    nBitsOfQ = pDlgroup->nBitsOfQ;
    nDigitsOfQ = pDlgroup->nDigitsOfQ;

    // Create the modulus Q
    pDlgroup->pmQ = SymCryptModulusCreate( pDlgroup->pbQ, SymCryptSizeofModulusFromDigits( nDigitsOfQ ), nDigitsOfQ );

    // Conditions on the hash function output size
    // The second condition is needed for generation of G in SymCrypt
    // since it allows even very small sizes of P.
    if ( (8*((UINT32)SymCryptHashResultSize( hashAlgorithm )) < nBitsOfQ) ||
         (8*((UINT32)SymCryptHashResultSize( hashAlgorithm )) > nBitsOfP) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Set the group's hash algorithm
    pDlgroup->pHashAlgorithm = hashAlgorithm;

    // Calculate sizes for the 2*Q divisor
    ndDivTwoQ = SymCryptDigitsFromBits(nBitsOfQ + 1);
    cbDivTwoQ = SymCryptSizeofDivisorFromDigits(ndDivTwoQ);

    // Scratch space
    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    //      - SymCryptDlgroupScratchSpace_FIPS is bounded by 2^28.
    //
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = SymCryptDlgroupScratchSpace_FIPS( nBitsOfP, nBitsOfQ, hashAlgorithm );
    pbScratch = SymCryptCallbackAlloc(cbScratch);
    if (pbScratch==NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Create a divisor 2*Q (needed for the generation of P)
    pdDivTwoQ = SymCryptDivisorCreate( pbScratch, cbDivTwoQ, ndDivTwoQ );
    pbScratchInternal = pbScratch + cbDivTwoQ;
    cbScratchInternal = cbScratch - cbDivTwoQ;

    // Create a trial division context for both P and Q
    pTrialDivisionContext = SymCryptCreateTrialDivisionContext( pDlgroup->nDigitsOfP );
    if (pTrialDivisionContext == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    do
    {
        do
        {
            // Fill the seed buffer in the DLGroup with seedlen bits
            scError = SymCryptCallbackRandom( pDlgroup->pbSeed, pDlgroup->cbSeed );
            if (scError != SYMCRYPT_NO_ERROR)
            {
                goto cleanup;
            }

            // Zero-out the top bits if needed
            if ((pDlgroup->nBitsOfSeed)%8 != 0)
            {
                pDlgroup->pbSeed[0] &= ((BYTE)0xff >> (8 - (pDlgroup->nBitsOfSeed)%8));
            }

            scError = SymCryptDlgroupGeneratePrimeQ_FIPS(
                            pDlgroup,
                            pTrialDivisionContext,
                            &fPrimeQFound,
                            SymCryptIntFromModulus(pDlgroup->pmQ),
                            pdDivTwoQ,
                            pbScratchInternal,
                            cbScratchInternal );
            if (scError != SYMCRYPT_NO_ERROR)
            {
                goto cleanup;
            }
        }
        while (fPrimeQFound == 0);

        scError = SymCryptDlgroupGeneratePrimeP_FIPS(
                        pDlgroup,
                        pdDivTwoQ,
                        4*nBitsOfP - 1,
                        pTrialDivisionContext,
                        &fPrimePFound,
                        SymCryptIntFromModulus(pDlgroup->pmP),
                        &(pDlgroup->dwGenCounter),
                        pbScratchInternal,
                        cbScratchInternal );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }
    while (fPrimePFound == 0);

    // Specify that we have a Q
    pDlgroup->fHasPrimeQ = TRUE;

    // Convert both of P and Q to moduli
    // IntToModulus requirement:
    //      Both P,Q > 0 since they are primes
    SymCryptIntToModulus(
        SymCryptIntFromModulus( pDlgroup->pmP ),
        pDlgroup->pmP,
        1000*nBitsOfP,        // Average operations
        SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
        pbScratch,
        cbScratch );

    SymCryptIntToModulus(
        SymCryptIntFromModulus( pDlgroup->pmQ ),
        pDlgroup->pmQ,
        1000*nBitsOfP,        // Average operations
        SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
        pbScratch,
        cbScratch );

    // Generate G
    scError = SymCryptDlgroupGenerateGenG_FIPS( pDlgroup, pDlgroup->peG, pbScratch, cbScratch );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:
    if (pTrialDivisionContext!=NULL)
    {
        SymCryptFreeTrialDivisionContext( pTrialDivisionContext );
    }

    if (pbScratch!=NULL)
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupSetValueSafePrime(
            SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE   dhSafePrimeType,
    _Inout_ PSYMCRYPT_DLGROUP                   pDlgroup )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;

    PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS safePrimeParams = NULL;

    UINT32 i;
    UINT32 nBitsOfQ;

    // Given we know nBitsOfP = nBitsOfQ+1 for all safe-prime groups, this specifies a tight bound when selecting a group
    UINT32 nMaxBitsOfP = SYMCRYPT_MIN(pDlgroup->nMaxBitsOfP, pDlgroup->nMaxBitsOfQ+1);
    UINT32 nMaxDigitsOfP;

    if ( dhSafePrimeType == SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE_NONE )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Iterate through all named safe-prime groups until we find one which fits the requested parameters
    // We can definitely do something smarter here, but we have only 10 values to check so do the dumb thing for now
    // Relies on the fact the SymCryptNamedSafePrimeGroups is ordered from largest to smallest
    for ( i=0; i<SYMCRYPT_DH_SAFEPRIME_GROUP_COUNT; i++ )
    {
        if ( SymCryptNamedSafePrimeGroups[i]->eDhSafePrimeType == dhSafePrimeType &&
             SymCryptNamedSafePrimeGroups[i]->nBitsOfP <= nMaxBitsOfP )
        {
            safePrimeParams = SymCryptNamedSafePrimeGroups[i];
            break;
        }
    }

    if (safePrimeParams == NULL)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    nMaxDigitsOfP = SymCryptDigitsFromBits(safePrimeParams->nBitsOfP);

    // Scratch space
    //
    // From symcrypt_internal.h we have:
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    //
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS(nMaxDigitsOfP),
        SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS(nMaxDigitsOfP) );
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if (pbScratch==NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Set fields marking the Dlgroup as being a named safe-prime group
    pDlgroup->isSafePrimeGroup  = TRUE;
    pDlgroup->eFipsStandard     = SYMCRYPT_DLGROUP_FIPS_NONE;
    pDlgroup->nMinBitsPriv      = safePrimeParams->nMinBitsPriv;
    pDlgroup->nDefaultBitsPriv  = safePrimeParams->nDefaultBitsPriv;

    // Ensure that fields which don't apply to named safe-prime groups are cleared
    pDlgroup->pHashAlgorithm = NULL;
    pDlgroup->dwGenCounter = 0;

    pDlgroup->nBitsOfSeed = 0;
    pDlgroup->pbSeed = NULL;
    pDlgroup->cbSeed = 0;

    // Set the bitsize and bytesize of P
    pDlgroup->nBitsOfP = safePrimeParams->nBitsOfP;
    pDlgroup->cbPrimeP = (safePrimeParams->nBitsOfP + 7)/ 8;
    pDlgroup->nDigitsOfP = SymCryptDigitsFromBits(safePrimeParams->nBitsOfP);

    // Set the bitsize and bytesize of Q
    nBitsOfQ = pDlgroup->nBitsOfP - 1;
    pDlgroup->nBitsOfQ = nBitsOfQ;
    pDlgroup->cbPrimeQ = (nBitsOfQ + 7)/8;
    pDlgroup->nDigitsOfQ = SymCryptDigitsFromBits(nBitsOfQ);
    pDlgroup->fHasPrimeQ = TRUE;

    //
    // Prime P
    //

    // Recreate the modulus P
    // (this will set nDigits in the modulus object appropriately, which is necessary for use of SymCryptIntShr1 below)
    pDlgroup->pmP = SymCryptModulusCreate( (PBYTE) pDlgroup->pmP, SymCryptSizeofModulusFromDigits( pDlgroup->nDigitsOfP ), pDlgroup->nDigitsOfP );

    scError = SymCryptIntSetValue( safePrimeParams->pcbPrimeP, pDlgroup->cbPrimeP, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, SymCryptIntFromModulus(pDlgroup->pmP) );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // IntToModulus requirement:
    //      nBitsOfP >= SYMCRYPT_DLGROUP_MIN_BITSIZE_P --> P > 0
    SymCryptIntToModulus(
        SymCryptIntFromModulus( pDlgroup->pmP ),
        pDlgroup->pmP,
        1000*pDlgroup->nBitsOfP,        // Average operations
        SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
        pbScratch,
        cbScratch );

    //
    // Prime Q
    //

    // Create the modulus Q
    pDlgroup->pmQ = SymCryptModulusCreate( pDlgroup->pbQ, SymCryptSizeofModulusFromDigits( pDlgroup->nDigitsOfQ ), pDlgroup->nDigitsOfQ );

    // Q = floor( P / 2 )
    SymCryptIntShr1( 0, SymCryptIntFromModulus(pDlgroup->pmP), SymCryptIntFromModulus(pDlgroup->pmQ) );

    // IntToModulus requirement:
    //      nBitsOfQ >= SYMCRYPT_DLGROUP_MIN_BITSIZE_Q --> Q > 0
    SymCryptIntToModulus(
        SymCryptIntFromModulus( pDlgroup->pmQ ),
        pDlgroup->pmQ,
        1000*nBitsOfQ,        // Average operations
        SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
        pbScratch,
        cbScratch );

    //
    // Generator G
    //

    // G to 2
    SymCryptModElementSetValueUint32( 2, pDlgroup->pmP, pDlgroup->peG, pbScratch, cbScratch );

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}

BOOLEAN
SYMCRYPT_CALL
SymCryptDlgroupIsSame(
    _In_    PCSYMCRYPT_DLGROUP  pDlgroup1,
    _In_    PCSYMCRYPT_DLGROUP  pDlgroup2 )
{
    BOOLEAN fIsSameGroup = FALSE;

    if ( pDlgroup1 == pDlgroup2 )
    {
        fIsSameGroup = TRUE;
        goto cleanup;
    }

    if ( (pDlgroup1->nBitsOfP != pDlgroup2->nBitsOfP) ||
         (pDlgroup1->nDigitsOfP != pDlgroup2->nDigitsOfP) ||
         !SymCryptIntIsEqual ( SymCryptIntFromModulus(pDlgroup1->pmP), SymCryptIntFromModulus(pDlgroup2->pmP) ) ||
         !SymCryptModElementIsEqual ( pDlgroup1->pmP, pDlgroup1->peG, pDlgroup2->peG ))
    {
        goto cleanup;
    }

    fIsSameGroup = TRUE;

cleanup:
    return fIsSameGroup;
}

VOID
SYMCRYPT_CALL
SymCryptDlgroupGetSizes(
    _In_    PCSYMCRYPT_DLGROUP      pDlgroup,
    _Out_   SIZE_T*                 pcbPrimeP,
    _Out_   SIZE_T*                 pcbPrimeQ,
    _Out_   SIZE_T*                 pcbGenG,
    _Out_   SIZE_T*                 pcbSeed )
{
    if (pcbPrimeP!=NULL)
    {
        *pcbPrimeP = pDlgroup->cbPrimeP;
    }

    if (pcbPrimeQ!=NULL)
    {
        *pcbPrimeQ = pDlgroup->cbPrimeQ;     // This returns 0 if the group does not have a prime Q
    }

    if (pcbGenG!=NULL)
    {
        *pcbGenG = pDlgroup->cbPrimeP;
    }

    if (pcbSeed!=NULL)
    {
        *pcbSeed = pDlgroup->cbSeed;        // This returns 0 if the group does not have a prime Q
    }
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupAutoCompleteNamedSafePrimeGroup(
    _Inout_                         PSYMCRYPT_DLGROUP   pDlgroup,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE  pbScratchInternal;
    SIZE_T cbScratchInternal;

    PSYMCRYPT_INT piTemp = NULL;
    UINT32 cbTemp;
    UINT32 i;
    UINT32 nBitsOfQ;
    PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS safePrimeParams = NULL;

    // Check whether bottom 64b of P all 1 - as first cheap check
    if ( SymCryptIntGetValueLsbits64( SymCryptIntFromModulus(pDlgroup->pmP) ) != ((UINT64) -1) )
    {
        goto cleanup; // Not a named safe-prime group
    }

    cbTemp = SymCryptSizeofIntFromDigits( pDlgroup->nDigitsOfP );
    SYMCRYPT_ASSERT( cbScratch >= cbTemp );

    // Create an integer piTemp
    piTemp = SymCryptIntCreate( pbScratch, cbTemp, pDlgroup->nDigitsOfP );
    pbScratchInternal = pbScratch + cbTemp;
    cbScratchInternal = cbScratch - cbTemp;

    // Set piTemp to the generator G (this will fail if the number cannot fit in the object)
    SymCryptModElementToInt( pDlgroup->pmP, pDlgroup->peG, piTemp, pbScratchInternal, cbScratchInternal );

    // Generator must be 2 mod P
    if ( !SymCryptIntIsEqualUint32( piTemp, 2 ) )
    {
        goto cleanup; // Not a named safe-prime group
    }

    // Iterate through all named safe-prime groups and check whether any of them have matching Prime P
    // We can definitely do something smarter here, but we have only 10 values to check so do the dumb thing for now
    for ( i=0; i<SYMCRYPT_DH_SAFEPRIME_GROUP_COUNT; i++ )
    {
        if ( SymCryptNamedSafePrimeGroups[i]->nBitsOfP == pDlgroup->nBitsOfP )
        {
            // Set piTemp to the named safe-prime group's P (this will fail if the number cannot fit in the object)
            SymCryptIntSetValue( SymCryptNamedSafePrimeGroups[i]->pcbPrimeP, pDlgroup->cbPrimeP, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, piTemp );

            if ( SymCryptIntIsEqual( piTemp, SymCryptIntFromModulus(pDlgroup->pmP) ) )
            {
                safePrimeParams = SymCryptNamedSafePrimeGroups[i];
                break;
            }
        }
    }

    // If we found a match in the previous loop, auto-populate appropriate fields in pDlGroup
    if (safePrimeParams != NULL)
    {
        if ( pDlgroup->eFipsStandard == SYMCRYPT_DLGROUP_FIPS_186_2 ||
             pDlgroup->eFipsStandard == SYMCRYPT_DLGROUP_FIPS_186_3 )
        {
            // Inappropriate use of named safe-prime groups
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        // Set fields marking the Dlgroup as being a named safe-prime group
        pDlgroup->isSafePrimeGroup  = TRUE;
        pDlgroup->eFipsStandard     = SYMCRYPT_DLGROUP_FIPS_NONE;
        pDlgroup->nMinBitsPriv      = safePrimeParams->nMinBitsPriv;
        pDlgroup->nDefaultBitsPriv  = safePrimeParams->nDefaultBitsPriv;

        // Ensure that fields which don't apply to named safe-prime groups are cleared
        pDlgroup->pHashAlgorithm = NULL;
        pDlgroup->dwGenCounter = 0;

        pDlgroup->nBitsOfSeed = 0;
        pDlgroup->pbSeed = NULL;
        pDlgroup->cbSeed = 0;

        // Set the bitsize and bytesize of Q
        nBitsOfQ = pDlgroup->nBitsOfP - 1;
        pDlgroup->nBitsOfQ = nBitsOfQ;
        pDlgroup->cbPrimeQ = (nBitsOfQ + 7)/8;
        pDlgroup->nDigitsOfQ = SymCryptDigitsFromBits(nBitsOfQ);

        // Create the modulus Q
        pDlgroup->pmQ = SymCryptModulusCreate( pDlgroup->pbQ, SymCryptSizeofModulusFromDigits( pDlgroup->nDigitsOfQ ), pDlgroup->nDigitsOfQ );

        // piTemp still has the value of P, and Q = floor( P / 2 )
        SymCryptIntShr1( 0, piTemp, piTemp );

        // Set the prime Q
        scError = SymCryptIntCopyMixedSize( piTemp, SymCryptIntFromModulus(pDlgroup->pmQ) );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // IntToModulus requirement:
        //      nBitsOfQ >= SYMCRYPT_DLGROUP_MIN_BITSIZE_Q --> Q > 0
        SymCryptIntToModulus(
            SymCryptIntFromModulus( pDlgroup->pmQ ),
            pDlgroup->pmQ,
            1000*nBitsOfQ,        // Average operations
            SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
            pbScratch,
            cbScratch );

        pDlgroup->fHasPrimeQ = TRUE;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupSetValue(
    _In_reads_bytes_( cbPrimeP )    PCBYTE                  pbPrimeP,
                                    SIZE_T                  cbPrimeP,
    _In_reads_bytes_( cbPrimeQ )    PCBYTE                  pbPrimeQ,
                                    SIZE_T                  cbPrimeQ,
    _In_reads_bytes_( cbGenG )      PCBYTE                  pbGenG,
                                    SIZE_T                  cbGenG,
                                    SYMCRYPT_NUMBER_FORMAT  numFormat,
    _In_opt_                        PCSYMCRYPT_HASH         pHashAlgorithm,
    _In_reads_bytes_( cbSeed )      PCBYTE                  pbSeed,
                                    SIZE_T                  cbSeed,
                                    UINT32                  genCounter,
                                    SYMCRYPT_DLGROUP_FIPS   fipsStandard,
    _Inout_                         PSYMCRYPT_DLGROUP       pDlgroup )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;
    SIZE_T cbScratchVerify = 0;

    PSYMCRYPT_INT piTemp = NULL;

    UINT32 nBitsOfP = 0;
    UINT32 nBitsOfQ = 0;

    UINT32 nMaxDigitsOfP = SymCryptDigitsFromBits(pDlgroup->nMaxBitsOfP);
    UINT32 nMaxDigitsOfQ = SymCryptDigitsFromBits(pDlgroup->nMaxBitsOfQ);

    PCSYMCRYPT_TRIALDIVISION_CONTEXT pTrialDivisionContext = NULL;

    // Make sure that the inputs make sense
    if ( (pbPrimeP==NULL) || (cbPrimeP==0) ||       // Prime P is needed
         ((pbGenG==NULL)&&(cbGenG>0)) ||
         ((pbPrimeQ==NULL)&&(cbPrimeQ>0)) ||
         ((pbGenG==NULL)&&(pbPrimeQ==NULL)) ||      // We can't have both Q and G missing
         ((pbSeed==NULL)&&(cbSeed>0)) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // FIPS 186-4 verification is needed
    if (fipsStandard != SYMCRYPT_DLGROUP_FIPS_NONE)
    {
        // Make sure we have what we need
        if ((pbPrimeQ == NULL)||
            (cbPrimeQ == 0) ||
            (pbSeed == NULL) ||
            (cbSeed == 0) ||
            ((fipsStandard == SYMCRYPT_DLGROUP_FIPS_186_2) && (pHashAlgorithm != NULL)) ||
            ((fipsStandard != SYMCRYPT_DLGROUP_FIPS_186_2) && (pHashAlgorithm == NULL)) )
        {
            scError = SYMCRYPT_AUTHENTICATION_FAILURE;
            goto cleanup;
        }
    }

    // Set the hashAlgorithm
    if ( (fipsStandard == SYMCRYPT_DLGROUP_FIPS_186_2) ||
         ((pHashAlgorithm==NULL) && (pbGenG == NULL)) )
    {
        // This hits either when:
        //  - The FIPS standard is 186-2
        //  - When we don't specify an algorithm or generator G (thus we need a hash algorithm to generate it
        //    ourselves)
        pDlgroup->pHashAlgorithm = SymCryptSha1Algorithm;
    }
    else
    {
        pDlgroup->pHashAlgorithm = pHashAlgorithm;
    }

    if ( (fipsStandard != SYMCRYPT_DLGROUP_FIPS_NONE) || (pbGenG == NULL))
    {
        // The following is the scratch space for generation / verification
        // Notice that we take the maximum size possible so it can get relatively big.
        // Also, we will need some additional space for the computed parameters:
        //      computedP, computedQ, and computedG.
        cbScratchVerify = SymCryptDlgroupScratchSpace_FIPS( pDlgroup->nMaxBitsOfP, pDlgroup->nMaxBitsOfQ, pDlgroup->pHashAlgorithm ) +
                          SYMCRYPT_MAX( SymCryptSizeofIntFromDigits(nMaxDigitsOfP),
                          SYMCRYPT_MAX( SymCryptSizeofIntFromDigits(nMaxDigitsOfQ),
                               2*SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS(nMaxDigitsOfP)));
    }

    // Scratch space
    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    //      - SymCryptDlgroupScratchSpace_FIPS is bounded by 2^28.
    //
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS(nMaxDigitsOfP) +
                    SYMCRYPT_MAX( SymCryptSizeofIntFromDigits(nMaxDigitsOfQ),
                    SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS(nMaxDigitsOfP) ),
                     cbScratchVerify );
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if (pbScratch==NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    //
    // Prime P
    //

    // Set the prime P (this will fail if the number cannot fit in the object)
    scError = SymCryptIntSetValue( pbPrimeP, cbPrimeP, numFormat, SymCryptIntFromModulus(pDlgroup->pmP) );
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Check the bitsize of value
    nBitsOfP = SymCryptIntBitsizeOfValue(SymCryptIntFromModulus(pDlgroup->pmP));
    if ( nBitsOfP > pDlgroup->nMaxBitsOfP)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (nBitsOfP < SYMCRYPT_DLGROUP_MIN_BITSIZE_P)
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    // FIPS 186-4 verification is needed
    // Check genCounter is not too big
    if (fipsStandard != SYMCRYPT_DLGROUP_FIPS_NONE &&
        genCounter > 4*nBitsOfP-1 )
    {
        scError = SYMCRYPT_AUTHENTICATION_FAILURE;
        goto cleanup;
    }

    if( (SymCryptIntGetValueLsbits32( SymCryptIntFromModulus( pDlgroup->pmP ) ) & 1) == 0 )
    {
        // P is even, when it should be a prime of at least 32 bits
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Set the bitsize and bytesize of the value
    pDlgroup->nBitsOfP = nBitsOfP;
    pDlgroup->cbPrimeP = (nBitsOfP + 7)/8;

    // IntToModulus requirement:
    //      nBitsOfP >= SYMCRYPT_DLGROUP_MIN_BITSIZE_P --> P > 0
    SymCryptIntToModulus(
        SymCryptIntFromModulus( pDlgroup->pmP ),
        pDlgroup->pmP,
        1000*nBitsOfP,        // Average operations
        SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
        pbScratch,
        cbScratch );

    //
    // Prime Q
    //

    // Wiping of previous (optional) parameters related to Q
    if (pDlgroup->pmQ != NULL)
    {
        SymCryptModulusWipe( pDlgroup->pmQ );
    }
    if (pDlgroup->cbSeed != 0)
    {
        SymCryptWipe( pDlgroup->pbSeed, pDlgroup->cbSeed);
    }

    if (pbPrimeQ != NULL)
    {
        // Create an integer piTemp
        piTemp = SymCryptIntCreate( pbScratch, cbScratch, nMaxDigitsOfQ );

        // Set the prime Q (this will fail if the number cannot fit in the object)
        scError = SymCryptIntSetValue( pbPrimeQ, cbPrimeQ, numFormat, piTemp );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // Check the bitsize of value
        nBitsOfQ = SymCryptIntBitsizeOfValue(piTemp);
        if ( nBitsOfQ > pDlgroup->nMaxBitsOfQ)
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        if (nBitsOfQ < SYMCRYPT_DLGROUP_MIN_BITSIZE_Q)
        {
            scError = SYMCRYPT_WRONG_KEY_SIZE;
            goto cleanup;
        }

        if( (SymCryptIntGetValueLsbits32( piTemp ) & 1) == 0 )
        {
            // Some of our modinv algorithms require odd inputs, and Q should be odd as it
            // claims to be a prime.
            // (Q can't be 2 as it must be at least 32 bits long.)
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        // Set the bitsize and bytesize of the value
        pDlgroup->nBitsOfQ = nBitsOfQ;
        pDlgroup->cbPrimeQ = (nBitsOfQ + 7)/8;
        pDlgroup->nDigitsOfQ = SymCryptDigitsFromBits(nBitsOfQ);
        pDlgroup->nDefaultBitsPriv = nBitsOfQ;
        pDlgroup->nBitsOfSeed = nBitsOfQ;
        pDlgroup->cbSeed = (nBitsOfQ+7)/8;

        // Create the modulus Q
        pDlgroup->pmQ = SymCryptModulusCreate( pDlgroup->pbQ, SymCryptSizeofModulusFromDigits( pDlgroup->nDigitsOfQ ), pDlgroup->nDigitsOfQ );

        // Set the prime Q
        scError = SymCryptIntCopyMixedSize( piTemp, SymCryptIntFromModulus(pDlgroup->pmQ) );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // piTemp is not needed any more so we are free to re-use the scratch space

        // IntToModulus requirement:
        //      nBitsOfQ >= SYMCRYPT_DLGROUP_MIN_BITSIZE_Q --> Q > 0
        SymCryptIntToModulus(
            SymCryptIntFromModulus( pDlgroup->pmQ ),
            pDlgroup->pmQ,
            1000*nBitsOfP,        // Average operations
            SYMCRYPT_FLAG_DATA_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
            pbScratch,
            cbScratch );

        pDlgroup->fHasPrimeQ = TRUE;
    }
    else
    {
        // Clear all info about Q
        pDlgroup->cbPrimeQ = 0;
        pDlgroup->nBitsOfQ = 0;
        pDlgroup->nDigitsOfQ = 0;

        pDlgroup->nDefaultBitsPriv = 0;
        pDlgroup->nBitsOfSeed = 0;
        pDlgroup->cbSeed = 0;

        pDlgroup->pmQ = NULL;
        pDlgroup->fHasPrimeQ = FALSE;
    }

    pDlgroup->isSafePrimeGroup = FALSE;
    pDlgroup->nMinBitsPriv = 0;

    //
    // Provided Generator G
    //
    if (pbGenG != NULL)
    {
        // Set the generator G (this will fail if the number cannot fit in the object)
        scError = SymCryptModElementSetValue( pbGenG, cbGenG, numFormat, pDlgroup->pmP, pDlgroup->peG, pbScratch, cbScratch );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        scError = SymCryptDlgroupAutoCompleteNamedSafePrimeGroup( pDlgroup, pbScratch, cbScratch );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // Successfully detected, validated and autocompleted named safe-prime group
        if (pDlgroup->isSafePrimeGroup)
        {
            goto cleanup;
        }
    }

    //
    //  Verification data (this has to be done before possibly generating G)
    //

    // Set the FIPS standard
    pDlgroup->eFipsStandard = fipsStandard;

    // Set the seed
    if (pbSeed != NULL)
    {
        if (cbSeed != pDlgroup->cbSeed)
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
        memcpy( pDlgroup->pbSeed, pbSeed, cbSeed );
    }

    // Set the genCounter
    pDlgroup->dwGenCounter = genCounter;

    //
    // Generator G
    //

    if (pbGenG == NULL)
    {
        // Let's generate G here since none was given

        // // We need Q (check at the beginning)
        // if (pbPrimeQ==NULL)
        // {
            // scError = SYMCRYPT_INVALID_ARGUMENT;
            // goto cleanup;
        // }

        // If no seed was given let's generate our own
        if (pbSeed==NULL)
        {
            SymCryptCallbackRandom(pDlgroup->pbSeed, pDlgroup->cbSeed);
        }

        scError = SymCryptDlgroupGenerateGenG_FIPS(
                        pDlgroup,
                        pDlgroup->peG,
                        pbScratch,
                        cbScratch );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }


    // Verification
    if (fipsStandard != SYMCRYPT_DLGROUP_FIPS_NONE)
    {
        // Verification
        PBYTE pbScratchInternal = pbScratch;
        SIZE_T cbScratchInternal = cbScratch;

        UINT32 ndDivTwoQ = 0;
        UINT32 cbDivTwoQ = 0;
        PSYMCRYPT_DIVISOR pdDivTwoQ = NULL;

        UINT32 cbComputed = 0;
        PSYMCRYPT_INT piComputed = NULL;
        UINT32 fPrimeComputed = 0;
        UINT32 dwComputedCounter = 0;

        PSYMCRYPT_MODELEMENT peComputed = NULL;
        PSYMCRYPT_MODELEMENT peOne = NULL;

        // Step 3: Acceptable pairs of L,N => skipped

        // Step 6: nBitsOfSeed < nBitsOfQ => skipped

        // Create the divisor object
        ndDivTwoQ = SymCryptDigitsFromBits(pDlgroup->nBitsOfQ + 1);
        cbDivTwoQ = SymCryptSizeofDivisorFromDigits( ndDivTwoQ );
        pdDivTwoQ = SymCryptDivisorCreate( pbScratchInternal, cbDivTwoQ, ndDivTwoQ );
        pbScratchInternal += cbDivTwoQ;
        cbScratchInternal -= cbDivTwoQ;

        // Create the temporary integer of size Q
        cbComputed = SymCryptSizeofIntFromDigits( pDlgroup->nDigitsOfQ );
        piComputed = SymCryptIntCreate( pbScratchInternal, cbComputed, pDlgroup->nDigitsOfQ );
        pbScratchInternal += cbComputed;
        cbScratchInternal -= cbComputed;

        // Create a trial division context for both P and Q
        pTrialDivisionContext = SymCryptCreateTrialDivisionContext( pDlgroup->nDigitsOfP );
        if (pTrialDivisionContext == NULL)
        {
            scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }

        // Steps 8,9: Check if computed_q is prime and equal to q
        scError = SymCryptDlgroupGeneratePrimeQ_FIPS(
                            pDlgroup,
                            pTrialDivisionContext,
                            &fPrimeComputed,
                            piComputed,
                            pdDivTwoQ,
                            pbScratchInternal,
                            cbScratchInternal );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            scError = SYMCRYPT_AUTHENTICATION_FAILURE;  // Overwrite any possible error
            goto cleanup;
        }

        if ((!fPrimeComputed)||(!SymCryptIntIsEqual( piComputed, SymCryptIntFromModulus(pDlgroup->pmQ))))
        {
            scError = SYMCRYPT_AUTHENTICATION_FAILURE;
            goto cleanup;
        }

        // Create the temporary integer of size P
        pbScratchInternal -= cbComputed;
        cbScratchInternal += cbComputed;
        cbComputed = SymCryptSizeofIntFromDigits( pDlgroup->nDigitsOfP );
        piComputed = SymCryptIntCreate( pbScratchInternal, cbComputed, pDlgroup->nDigitsOfP );
        pbScratchInternal += cbComputed;
        cbScratchInternal -= cbComputed;

        // Steps 10-14: Check if computed_p is prime and equal to p
        scError = SymCryptDlgroupGeneratePrimeP_FIPS(
                        pDlgroup,
                        pdDivTwoQ,
                        pDlgroup->dwGenCounter,     // Go up to this
                        pTrialDivisionContext,
                        &fPrimeComputed,
                        piComputed,
                        &dwComputedCounter,
                        pbScratchInternal,
                        cbScratchInternal );
        if (scError != SYMCRYPT_NO_ERROR)
        {
            scError = SYMCRYPT_AUTHENTICATION_FAILURE;  // Overwrite any possible error
            goto cleanup;
        }

        if ((!fPrimeComputed)||(dwComputedCounter!=pDlgroup->dwGenCounter)||(!SymCryptIntIsEqual( piComputed, SymCryptIntFromModulus(pDlgroup->pmP))))
        {
            scError = SYMCRYPT_AUTHENTICATION_FAILURE;
            goto cleanup;
        }

        // Validation of G

        // Create the temporary modelement mod P
        pbScratchInternal -= cbComputed;
        cbScratchInternal += cbComputed;
        cbComputed = SymCryptSizeofModElementFromModulus( pDlgroup->pmP );
        peOne = SymCryptModElementCreate( pbScratchInternal, cbComputed, pDlgroup->pmP );
        pbScratchInternal += cbComputed;
        cbScratchInternal -= cbComputed;
        peComputed = SymCryptModElementCreate( pbScratchInternal, cbComputed, pDlgroup->pmP );
        pbScratchInternal += cbComputed;
        cbScratchInternal -= cbComputed;

        // Step 2: Verify that 2<= G <= p-1
        SymCryptModElementSetValueUint32( 1, pDlgroup->pmP, peOne, pbScratchInternal, cbScratchInternal );     // Set the temporary to 1

        if ((SymCryptModElementIsZero(pDlgroup->pmP, pDlgroup->peG)) || (SymCryptModElementIsEqual(pDlgroup->pmP, pDlgroup->peG, peOne)))
        {
            scError = SYMCRYPT_AUTHENTICATION_FAILURE;
            goto cleanup;
        }

        // Step 3: Verify that G^Q == 1
        SymCryptModExp(
            pDlgroup->pmP,
            pDlgroup->peG,
            SymCryptIntFromModulus(pDlgroup->pmQ),
            nBitsOfQ,
            SYMCRYPT_FLAG_DATA_PUBLIC,
            peComputed,
            pbScratchInternal,
            cbScratchInternal );

        if (!SymCryptModElementIsEqual(pDlgroup->pmP, peComputed, peOne))
        {
            scError = SYMCRYPT_AUTHENTICATION_FAILURE;
            goto cleanup;
        }

    }

cleanup:
    if (pTrialDivisionContext!=NULL)
    {
        SymCryptFreeTrialDivisionContext( pTrialDivisionContext );
    }

    if (pbScratch!=NULL)
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupGetValue(
    _In_                            PCSYMCRYPT_DLGROUP      pDlgroup,
    _Out_writes_bytes_( cbPrimeP )  PBYTE                   pbPrimeP,
                                    SIZE_T                  cbPrimeP,
    _Out_writes_bytes_( cbPrimeQ )  PBYTE                   pbPrimeQ,
                                    SIZE_T                  cbPrimeQ,
    _Out_writes_bytes_( cbGenG )    PBYTE                   pbGenG,
                                    SIZE_T                  cbGenG,
                                    SYMCRYPT_NUMBER_FORMAT  numFormat,
    _Out_                           PCSYMCRYPT_HASH *       ppHashAlgorithm,
    _Out_writes_bytes_( cbSeed )    PBYTE                   pbSeed,
                                    SIZE_T                  cbSeed,
    _Out_                           PUINT32                 pGenCounter )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;

    if ( ((pbPrimeP==NULL)&&(cbPrimeP>0)) ||
         ((pbPrimeQ==NULL)&&(cbPrimeQ>0)) ||
         ((pbGenG==NULL)&&(cbGenG>0)) ||
         ((pbSeed==NULL)&&(cbSeed>0)) ||
         ((pbSeed!=NULL)&&(cbSeed!=pDlgroup->cbSeed)) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if ((pbPrimeQ!=NULL) && (!pDlgroup->fHasPrimeQ))
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    if (pbPrimeP!=NULL)
    {
        scError = SymCryptIntGetValue(
                        SymCryptIntFromModulus(pDlgroup->pmP),
                        pbPrimeP,
                        cbPrimeP,
                        numFormat );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }

    if (pbPrimeQ!=NULL)
    {
        scError = SymCryptIntGetValue(
                        SymCryptIntFromModulus(pDlgroup->pmQ),
                        pbPrimeQ,
                        cbPrimeQ,
                        numFormat );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }

    if (pbGenG!=NULL)
    {
        // Scratch space is needed
        cbScratch = SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pDlgroup->nDigitsOfP);
        pbScratch = SymCryptCallbackAlloc( cbScratch );
        if (pbScratch==NULL)
        {
            scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }

        scError = SymCryptModElementGetValue(
                        pDlgroup->pmP,
                        pDlgroup->peG,
                        pbGenG,
                        cbGenG,
                        numFormat,
                        pbScratch,
                        cbScratch );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }

    if (ppHashAlgorithm!=NULL)
    {
        if (pDlgroup->eFipsStandard == SYMCRYPT_DLGROUP_FIPS_186_2)
        {
            *ppHashAlgorithm = NULL;
        }
        else
        {
            *ppHashAlgorithm = pDlgroup->pHashAlgorithm;
        }
    }

    if (pbSeed!=NULL && pDlgroup->pbSeed!=NULL)
    {
        memcpy( pbSeed, pDlgroup->pbSeed, pDlgroup->cbSeed);
    }

    if (pGenCounter!=NULL)
    {
        *pGenCounter = pDlgroup->dwGenCounter;
    }

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}
