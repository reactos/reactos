//
// rsakey.c   RSA keys' related algorithms
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define RSA_DEFAULT_PUBLIC_EXPONENT        (65537)

PSYMCRYPT_RSAKEY
SYMCRYPT_CALL
SymCryptRsakeyAllocate(
    _In_    PCSYMCRYPT_RSA_PARAMS   pParams,
    _In_    UINT32                  flags )
{
    PVOID               p;
    SIZE_T              cb;
    PSYMCRYPT_RSAKEY    res = NULL;

    UNREFERENCED_PARAMETER( flags );

    SYMCRYPT_ASSERT( pParams != NULL );

    cb = SymCryptSizeofRsakeyFromParams( pParams );

    p = SymCryptCallbackAlloc( cb );

    if ( p==NULL )
    {
        goto cleanup;
    }

    res = SymCryptRsakeyCreate( p, cb, pParams );

cleanup:
    return res;
}

VOID
SYMCRYPT_CALL
SymCryptRsakeyFree( _Out_ PSYMCRYPT_RSAKEY pkObj )
{
    SYMCRYPT_CHECK_MAGIC( pkObj );
    SymCryptRsakeyWipe( pkObj );
    SymCryptCallbackFree( pkObj );
}

UINT32
SYMCRYPT_CALL
SymCryptSizeofRsakeyFromParams( _In_ PCSYMCRYPT_RSA_PARAMS pParams )
{
    UINT32 nModulusDigits;
    UINT32 res;

    SYMCRYPT_ASSERT( pParams != NULL );

    nModulusDigits = SymCryptDigitsFromBits( pParams->nBitsOfModulus );

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    //      - nPrimes and nPubExps are bounded by SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES = 2 and
    //        SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS = 1
    // Thus the following calculation does not overflow the result.
    //
    res =  sizeof(SYMCRYPT_RSAKEY) +
           SymCryptSizeofModulusFromDigits( nModulusDigits ) +                                              // For Modulus
           pParams->nPrimes * SymCryptSizeofModulusFromDigits( nModulusDigits ) +                           // For Primes
           pParams->nPrimes * SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( pParams->nBitsOfModulus ) +             // For CrtInverses
           pParams->nPubExp * SymCryptSizeofIntFromDigits( nModulusDigits ) +                               // For PrivExps
           pParams->nPubExp * pParams->nPrimes * SymCryptSizeofIntFromDigits( nModulusDigits );             // For CrtPrivExps

    // Consistency check with the static macro (optimized away in production)
    SYMCRYPT_ASSERT( res <= SYMCRYPT_SIZEOF_RSAKEY_FROM_PARAMS( pParams->nBitsOfModulus, pParams->nPrimes, pParams->nPubExp ) );

    return res;
}

PSYMCRYPT_RSAKEY
SYMCRYPT_CALL
SymCryptRsakeyCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE                   pbBuffer,
                                    SIZE_T                  cbBuffer,
    _In_                            PCSYMCRYPT_RSA_PARAMS   pParams )
{
    PSYMCRYPT_RSAKEY pkObj = NULL;

    PBYTE pbCurr = pbBuffer;
    SIZE_T cbNeeded;
    SIZE_T itemSize;

    SYMCRYPT_ASSERT( pParams != NULL );

    cbNeeded = SymCryptSizeofRsakeyFromParams( pParams );

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbCurr );

    if (( cbBuffer < cbNeeded ) ||
        ( pParams->nBitsOfModulus < SYMCRYPT_RSAKEY_MIN_BITSIZE_MODULUS ) ||
        ( pParams->nBitsOfModulus > SYMCRYPT_RSAKEY_MAX_BITSIZE_MODULUS ) ||
        ( pParams->nPubExp < 1 ) ||
        ( pParams->nPubExp > SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS ) ||
        ( pParams->nPrimes == 1 ) ||
        ( pParams->nPrimes > SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES ) )
    {
        goto cleanup;
    }
    SYMCRYPT_ASSERT( cbBuffer >= sizeof( SYMCRYPT_RSAKEY ) );

    pkObj = (PSYMCRYPT_RSAKEY) pbCurr;

    // Set all the parameters to 0
    SymCryptWipe( pbBuffer, cbBuffer );

    // Main parameters of the RSAKEY
    // Everything is 0 until created

    pkObj->cbTotalSize = (UINT32) cbNeeded;
    // The result should always be within 4 GB, but we check to avoid security bugs
    SYMCRYPT_ASSERT( pkObj->cbTotalSize == cbNeeded );

    pkObj->hasPrivateKey = FALSE;

    pkObj->nSetBitsOfModulus = pParams->nBitsOfModulus;
    pkObj->nDigitsOfModulus = SymCryptDigitsFromBits( pkObj->nSetBitsOfModulus );   // The modulus object has always this number of digits

    pkObj->nPrimes = pParams->nPrimes;
    pkObj->nPubExp = pParams->nPubExp;

    pbCurr += sizeof( SYMCRYPT_RSAKEY );

    // Modulus
    itemSize = SymCryptSizeofModulusFromDigits( pkObj->nDigitsOfModulus );
    SYMCRYPT_ASSERT( cbBuffer >= sizeof( SYMCRYPT_RSAKEY ) + itemSize
                                 + (pkObj->nPrimes*SymCryptSizeofModulusFromDigits( pkObj->nDigitsOfModulus ))
                                 + (pkObj->nPrimes*SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( pParams->nBitsOfModulus ))
                                 + (pkObj->nPubExp*SymCryptSizeofIntFromDigits( pkObj->nDigitsOfModulus ))
                                 + (pkObj->nPubExp*pkObj->nPrimes*SymCryptSizeofIntFromDigits( pkObj->nDigitsOfModulus )) );
    pkObj->pmModulus = SymCryptModulusCreate(
                        pbCurr,
                        itemSize,
                        pkObj->nDigitsOfModulus );
    SYMCRYPT_ASSERT( pkObj->pmModulus != NULL );
    pbCurr += itemSize;

    // For the remaining objects
    // defer creation until SymCryptRsakeyGenerate or
    // SymCryptRsakeySetValue

    // Primes
    for (UINT32 i=0; i<pkObj->nPrimes; i++)
    {
        pkObj->pbPrimes[i] = pbCurr;
        pbCurr += SymCryptSizeofModulusFromDigits( pkObj->nDigitsOfModulus );
    }

    // CRT Inverses of primes
    for (UINT32 i=0; i<pkObj->nPrimes; i++)
    {
        pkObj->pbCrtInverses[i] = pbCurr;
        pbCurr += SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( pParams->nBitsOfModulus );
    }

    // Private exponents
    for (UINT32 i=0; i<pkObj->nPubExp; i++)
    {
        pkObj->pbPrivExps[i] = pbCurr;
        pbCurr += SymCryptSizeofIntFromDigits( pkObj->nDigitsOfModulus );
    }

    // Private exponents modulo each prime (minus 1)
    for (UINT32 i=0; i<pkObj->nPubExp*pkObj->nPrimes; i++)
    {
        pkObj->pbCrtPrivExps[i] = pbCurr;
        pbCurr += SymCryptSizeofIntFromDigits( pkObj->nDigitsOfModulus );
    }

    // Setting the magic
    SYMCRYPT_SET_MAGIC( pkObj );

cleanup:
    return pkObj;
}

VOID
SYMCRYPT_CALL
SymCryptRsakeyWipe( _Out_ PSYMCRYPT_RSAKEY pkDst )
{
    // Wipe the whole structure in one go.
    SymCryptWipe( pkDst, pkDst->cbTotalSize );
}

#if 0
VOID
SYMCRYPT_CALL
SymCryptRsakeyCopy(
    _In_    PCSYMCRYPT_RSAKEY  pkSrc,
    _Out_   PSYMCRYPT_RSAKEY   pkDst )
{
    SymCryptFatal( 'rsac' );
    // This function doesn't work correctly because subobjects might
    // not have been created yet.
    // Future: fix this

    //
    // in-place copy is somewhat common...
    //
    if( pkSrc != pkDst )
    {
        pkDst->fAlgorithmInfo = pkSrc->fAlgorithmInfo;
        pkDst->cbTotalSize = pkSrc->cbTotalSize;
        pkDst->hasPrivateKey = pkSrc->hasPrivateKey;
        pkDst->nSetBitsOfModulus = pkSrc->nSetBitsOfModulus;

        pkDst->nBitsOfModulus = pkSrc->nBitsOfModulus;
        pkDst->nDigitsOfModulus = pkSrc->nDigitsOfModulus;

        pkDst->nPubExp = pkSrc->nPubExp;
        for (UINT32 i=0; i<SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS; i++)
        {
            pkDst->au64PubExp[i] = pkSrc->au64PubExp[i];
        }

        pkDst->nPrimes = pkSrc->nPrimes;
        for (UINT32 i=0; i<SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES; i++)
        {
            pkDst->nBitsOfPrimes[i] = pkSrc->nBitsOfPrimes[i];
            pkDst->nDigitsOfPrimes[i] = pkSrc->nDigitsOfPrimes[i];
        }

        // Copy the objects
        SymCryptModulusCopy( pkSrc->pmModulus, pkDst->pmModulus );

        for (UINT32 i=0; i< pkSrc->nPrimes; i++)
        {
            SymCryptModulusCopy( pkSrc->pmPrimes[i], pkDst->pmPrimes[i] );
            SymCryptModElementCopy( pkSrc->pmPrimes[i], pkSrc->peCrtInverses[i], pkDst->peCrtInverses[i] );
        }

        for (UINT32 i=0; i< pkSrc->nPubExp; i++)
        {
            SymCryptIntCopy( pkSrc->piPrivExps[i], pkDst->piPrivExps[i] );
        }

        for (UINT32 i=0; i< pkSrc->nPubExp*pkSrc->nPrimes; i++)
        {
            SymCryptIntCopy( pkSrc->piCrtPrivExps[i], pkDst->piCrtPrivExps[i] );
        }
    }
}
#endif

BOOLEAN
SYMCRYPT_CALL
SymCryptRsakeyHasPrivateKey( _In_ PCSYMCRYPT_RSAKEY pkRsakey )
{
    return pkRsakey->hasPrivateKey;
}

UINT32
SYMCRYPT_CALL
SymCryptRsakeySizeofModulus( _In_ PCSYMCRYPT_RSAKEY pkRsakey )
{
    return (pkRsakey->nBitsOfModulus + 7)/8;
}

UINT32
SYMCRYPT_CALL
SymCryptRsakeyModulusBits( _In_ PCSYMCRYPT_RSAKEY pkRsakey )
{
    return pkRsakey->nBitsOfModulus;
}

UINT32
SYMCRYPT_CALL
SymCryptRsakeySizeofPublicExponent(
    _In_    PCSYMCRYPT_RSAKEY pRsakey,
            UINT32            index )
{
    SYMCRYPT_ASSERT( index == 0 );
    UNREFERENCED_PARAMETER( index );
    return SymCryptUint64Bytesize( pRsakey->au64PubExp[0] );
}

UINT32
SYMCRYPT_CALL
SymCryptRsakeySizeofPrime(
    _In_    PCSYMCRYPT_RSAKEY pkRsakey,
            UINT32            index )
{
    return (pkRsakey->nBitsOfPrimes[index] + 7)/8;
}

UINT32
SYMCRYPT_CALL
SymCryptRsakeyGetNumberOfPublicExponents( _In_ PCSYMCRYPT_RSAKEY pkRsakey )
{
    return pkRsakey->nPubExp;
}

UINT32
SYMCRYPT_CALL
SymCryptRsakeyGetNumberOfPrimes( _In_ PCSYMCRYPT_RSAKEY pkRsakey )
{
    return pkRsakey->nPrimes;
}

VOID
SYMCRYPT_CALL
SymCryptRsakeyCreateAllObjects( _Inout_ PSYMCRYPT_RSAKEY  pkRsakey )
{
    // Primes
    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        pkRsakey->pmPrimes[i] = SymCryptModulusCreate(
                                pkRsakey->pbPrimes[i],
                                SymCryptSizeofModulusFromDigits( pkRsakey->nDigitsOfPrimes[i] ),
                                pkRsakey->nDigitsOfPrimes[i] );
        SYMCRYPT_ASSERT( pkRsakey->pmPrimes[i] != NULL );
    }

    // CRT Inverses of primes
    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        pkRsakey->peCrtInverses[i] = SymCryptModElementCreate(
                                pkRsakey->pbCrtInverses[i],
                                SymCryptSizeofModElementFromModulus( pkRsakey->pmPrimes[i] ),
                                pkRsakey->pmPrimes[i] );
        SYMCRYPT_ASSERT( pkRsakey->peCrtInverses[i] != NULL );
    }

    // Private exponents
    for( UINT32 i=0; i<pkRsakey->nPubExp; i++ )
    {
        pkRsakey->piPrivExps[i] = SymCryptIntCreate(
                                    pkRsakey->pbPrivExps[i],
                                    SymCryptSizeofIntFromDigits( pkRsakey->nDigitsOfModulus ),
                                    pkRsakey->nDigitsOfModulus );
        SYMCRYPT_ASSERT( pkRsakey->piPrivExps[i] != NULL );
    }

    // Private exponents modulo each prime (minus 1)
    for (UINT32 i=0; i<pkRsakey->nPubExp*pkRsakey->nPrimes; i++)
    {
        pkRsakey->piCrtPrivExps[i] = SymCryptIntCreate(
                                pkRsakey->pbCrtPrivExps[i],
                                SymCryptSizeofIntFromDigits( pkRsakey->nDigitsOfPrimes[i] ),
                                pkRsakey->nDigitsOfPrimes[i] );
        SYMCRYPT_ASSERT( pkRsakey->piCrtPrivExps[i] != NULL );
    }
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyCalculatePrivateFields(
    _Inout_ PSYMCRYPT_RSAKEY  pkRsakey,
    _Out_   PSYMCRYPT_DIVISOR pdTmp,    // Temporary of nMaxDigitsOfPrimes
    _Out_   PSYMCRYPT_INT     piPhi,    // Temporary of nDigitsOfModulus
    _Out_   PSYMCRYPT_INT     piAcc,    // Temporary of nMaxDigitsOfPrimes + nDigitsOfModulus
    _Out_writes_bytes_( cbScratch )
            PBYTE             pbScratch,
            SIZE_T            cbScratch,
            UINT32            flags
)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    BYTE tmpGcdBuf[ SYMCRYPT_SIZEOF_INT_FROM_BITS( 64 ) + SYMCRYPT_ASYM_ALIGN_VALUE];
    PSYMCRYPT_INT piTmpGcd;

    // Use pdTmp as int scratch
    PSYMCRYPT_INT piScr = SymCryptIntFromDivisor(pdTmp);

    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION;

    if ( ( flags & ~allowedFlags ) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // We need a 1-digit tmp value to store the GCD in.
    // Simpler to put it on the stack than to add full scratch size computation support to this function
    piTmpGcd = SymCryptIntCreate( SYMCRYPT_ASYM_ALIGN_UP( tmpGcdBuf ), sizeof( tmpGcdBuf ) - SYMCRYPT_ASYM_ALIGN_VALUE, SymCryptDigitsFromBits( 64 ) );

    // Run the CRT generation
    scError = SymCryptCrtGenerateInverses( pkRsakey->nPrimes, pkRsakey->pmPrimes, 0, pkRsakey->peCrtInverses, pbScratch, cbScratch);
    if (scError!=SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Calculate Phi
    SymCryptIntSetValueUint32( 1, piPhi );
    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        // piScr can have the different number of digits than each prime
        scError = SymCryptIntCopyMixedSize( SymCryptIntFromModulus( pkRsakey->pmPrimes[i] ), piScr );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
             goto cleanup;
        }
        SymCryptIntSubUint32( piScr, 1, piScr );         // p-1
        SymCryptIntMulMixedSize( piScr, piPhi, piAcc, pbScratch, cbScratch );
        scError = SymCryptIntCopyMixedSize( piAcc, piPhi );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }

    // Calculate the private exponents
    for (UINT32 i=0; i<pkRsakey->nPubExp; i++)
    {
        // IntExtendedGcd requirements:
        //      - First argument > 0: piPhi as the product of p-1's
        //      - Second argument: odd, verified below
        // We also reject public exponent 1, as that is obviously unsafe.
        if( /* pkRsakey->au64PubExp[i] == 1 || */ (pkRsakey->au64PubExp[i] & 1) != 1)
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        // Calculate D
        SymCryptIntSetValueUint64( pkRsakey->au64PubExp[i], piScr );

        // Calculate D
        SymCryptIntExtendedGcd(
            piPhi,
            piScr,
            SYMCRYPT_FLAG_GCD_INPUTS_NOT_BOTH_EVEN,
            piTmpGcd,   // Gcd
            NULL,   // Lcm
            NULL,   // InvSrc1ModSrc2
            pkRsakey->piPrivExps[i],
            pbScratch,
            cbScratch);

        if( !SymCryptIntIsEqualUint32( piTmpGcd, 1 ) )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
    }

    //Calculate the private exponents modulo each prime minus 1
    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        scError = SymCryptIntCopyMixedSize( SymCryptIntFromModulus(pkRsakey->pmPrimes[i]), SymCryptIntFromDivisor(pdTmp) );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // IntToDivisor requirement:
        //      Each prime has at least SYMCRYPT_RSAKEY_MIN_BITSIZE_PRIME bits --> P-1 > 0
        SymCryptIntSubUint32( SymCryptIntFromDivisor(pdTmp), 1, SymCryptIntFromDivisor(pdTmp) );
        SymCryptIntToDivisor(
            SymCryptIntFromDivisor(pdTmp),
            pdTmp,
            pkRsakey->nPubExp,
            0,
            pbScratch,
            cbScratch );

        for (UINT32 j=0; j<pkRsakey->nPubExp; j++)
        {
            SymCryptIntDivMod(
                pkRsakey->piPrivExps[j],
                pdTmp,
                NULL,
                piPhi,      // Set it to Phi as each private exponent might have different size
                pbScratch,
                cbScratch );

            scError = SymCryptIntCopyMixedSize( piPhi, pkRsakey->piCrtPrivExps[ j*pkRsakey->nPrimes + i ]);
            if (scError!=SYMCRYPT_NO_ERROR)
            {
                goto cleanup;
            }
        }
    }

    // Check that the product of the primes is in fact the modulus
    if( (flags & SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION) == 0 )
    {
        if( pkRsakey->nPrimes != 2 )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        SymCryptIntMulMixedSize(
            SymCryptIntFromModulus(pkRsakey->pmPrimes[0]),
            SymCryptIntFromModulus(pkRsakey->pmPrimes[1]),
            piAcc,
            pbScratch, cbScratch );

        if( !SymCryptIntIsEqual( piAcc, SymCryptIntFromModulus( pkRsakey->pmModulus ) ) )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyGenerate(
    _Inout_                     PSYMCRYPT_RSAKEY    pkRsakey,
    _In_reads_opt_( nPubExp )   PCUINT64            pu64PubExp,
                                UINT32              nPubExp,
    _In_                        UINT32              flags )
{
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    // 3 sizes of temporary elements:
    //  - ndPrimes = number of digit size of each prime (we choose it to be the same for all primes)
    //  - ndMod = pkRsakey->nDigitsOfModulus
    //  - ndLarge = ndPrimes + ndMod

    UINT32 ndPrimes = 0;

    UINT32 cbPrimes = 0;
    PSYMCRYPT_INT piLow = NULL;
    PSYMCRYPT_INT piHigh = NULL;

    UINT32 cbDivisor = 0;
    PSYMCRYPT_DIVISOR pdTmp = NULL;

    UINT32 ndMod = pkRsakey->nDigitsOfModulus;
    UINT32 cbMod = 0;
    PSYMCRYPT_INT piPhi = NULL;

    UINT32 ndLarge = 0;
    UINT32 cbLarge = 0;
    PSYMCRYPT_INT piAcc = NULL;

    PBYTE           pbScratch = NULL;
    UINT32          cbScratch = 0;
    PBYTE           pbFnScratch = NULL;
    UINT32          cbFnScratch = 0;

    UINT32 maxTries = 0;                    // For the prime generation (and the modulus operations ?)
    UINT32 primeBits = 0;

    const UINT64 defaultExponent = RSA_DEFAULT_PUBLIC_EXPONENT;

    // Ensure caller has specified what algorithm(s) the key will be used with
    UINT32 algorithmFlags = SYMCRYPT_FLAG_RSAKEY_SIGN | SYMCRYPT_FLAG_RSAKEY_ENCRYPT;
    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS | algorithmFlags;

    if ( ( ( flags & ~allowedFlags ) != 0 ) ||
         ( ( flags & algorithmFlags ) == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // SymCryptRsaSignVerifyPct requires the generated key to be at least 496 bits to avoid fatal
    // Require caller to specify NO_FIPS for up to 1024 bits as running FIPS tests on too-small keys
    // does not make it FIPS certifiable and gives the wrong impression to callers
    if ( ( (flags & SYMCRYPT_FLAG_KEY_NO_FIPS) == 0 ) &&
         ( pkRsakey->nSetBitsOfModulus < SYMCRYPT_RSAKEY_FIPS_MIN_BITSIZE_MODULUS ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Handle the default exponent case
    if( pu64PubExp == NULL && nPubExp == 0 )
    {
        pu64PubExp = &defaultExponent;
        nPubExp = 1;
    }

    // Make sure we have:
    // - exactly 2 primes
    // - the right number of public exponents
    // - exactly 1 public exponent
    if (pkRsakey->nPrimes != 2 || nPubExp != pkRsakey->nPubExp || nPubExp != 1 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Copy the public exponent into the key
    pkRsakey->au64PubExp[0] = pu64PubExp[0];

    // Before doing anything calculate all the needed sizes
    // The size limits were checked in SymCryptRsakeyCreate which is the only way to create an Rsakey object.
    pkRsakey->nBitsOfModulus = pkRsakey->nSetBitsOfModulus;             // This will be the exact bit size of our modulus

    pkRsakey->nBitsOfPrimes[0] = (pkRsakey->nBitsOfModulus + 1)/2;
    pkRsakey->nBitsOfPrimes[1] = pkRsakey->nBitsOfModulus/2;            // The second prime is one bit smaller for odd-length moduli

    pkRsakey->nDigitsOfPrimes[0] = SymCryptDigitsFromBits(pkRsakey->nBitsOfPrimes[0]);
    pkRsakey->nDigitsOfPrimes[1] = SymCryptDigitsFromBits(pkRsakey->nBitsOfPrimes[1]);

    pkRsakey->nMaxDigitsOfPrimes = SYMCRYPT_MAX(pkRsakey->nDigitsOfPrimes[0], pkRsakey->nDigitsOfPrimes[1]);

    ndPrimes = pkRsakey->nMaxDigitsOfPrimes;
    ndLarge = ndPrimes + ndMod;

    primeBits = SYMCRYPT_MAX(pkRsakey->nBitsOfPrimes[0],pkRsakey->nBitsOfPrimes[1]);
    maxTries = 100 * primeBits;

    // Create all the SymCryptObjects
    SymCryptRsakeyCreateAllObjects( pkRsakey );

    // Allocate the temp integers and the scratch space
    // All sizes are limited by the modulus sizes verified in SymCryptRsakeyCreate
    cbPrimes = SymCryptSizeofIntFromDigits( ndPrimes );
    cbMod = SymCryptSizeofIntFromDigits( ndMod );
    cbLarge = SymCryptSizeofIntFromDigits( ndLarge );
    cbDivisor = SymCryptSizeofDivisorFromDigits( ndPrimes );

    cbScratch = 2*cbPrimes + cbMod + cbLarge + cbDivisor +
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_PRIME_GEN(ndPrimes),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS(ndMod),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_MUL(ndMod),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_CRT_GENERATION(ndPrimes),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_EXTENDED_GCD(ndMod),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_DIVISOR(ndPrimes),
                     SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( ndMod, ndPrimes )
                    ))))));

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    pbFnScratch = pbScratch;
    cbFnScratch = cbScratch;

    // Create temporaries
	// dcl - this would be easier to review with one statement per line
    piLow = SymCryptIntCreate( pbFnScratch, cbPrimes, ndPrimes ); pbFnScratch += cbPrimes; cbFnScratch -= cbPrimes;
    piHigh = SymCryptIntCreate( pbFnScratch, cbPrimes, ndPrimes ); pbFnScratch += cbPrimes; cbFnScratch -= cbPrimes;

    piPhi = SymCryptIntCreate( pbFnScratch, cbMod, ndMod ); pbFnScratch += cbMod; cbFnScratch -= cbMod;

    piAcc = SymCryptIntCreate( pbFnScratch, cbLarge, ndLarge ); pbFnScratch += cbLarge; cbFnScratch -= cbLarge;

    pdTmp = SymCryptDivisorCreate( pbFnScratch, cbDivisor, ndPrimes ); pbFnScratch += cbDivisor; cbFnScratch -= cbDivisor;

    // ***Prime generation limits***
    //
    // If nBitsOfModulus is even (main case)
    //  Low limit   = 2^{primeBits-1} + 2^{primeBits - 2}
    //  High limit  = 2^primeBits - 1
    //
    // If nBitsOfModulus is odd we use different
    // limits for the two primes (until we have an integer sqrt function)
    //
    // For the first
    //      Low limit   = 2^{primeBits-1} + 2^{primeBits - 2}
    //      High limit  = 2^primeBits - 1
    // For the second
    //      Low limit   = 2^{primeBits-2} + 2^{primeBits - 3}
    //      High limit  = 2^{primeBits-1} - 1
    //
    // Notice that nBitsOfModulus is a public value.
    //
    // *** TODO: This works only for 2 primes to give modulus
    //           of exactly nBitsOfModulus bits.

    SymCryptIntSetValueUint32( 3, piLow );
    SymCryptIntMulPow2( piLow, primeBits - 2, piLow );

    SymCryptIntSetValueUint32( 1, piHigh );
    SymCryptIntMulPow2( piHigh, primeBits, piHigh );
    SymCryptIntSubUint32( piHigh, 1, piHigh );

    // Generate primes and at the same time accumulate their product into piPhi
    SymCryptIntSetValueUint32( 1, piPhi );
    for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
    {
        if ( ((pkRsakey->nBitsOfModulus % 2)==1) && (i>0) )
        {
            SymCryptIntDivPow2( piLow, 1, piLow );
            SymCryptIntDivPow2( piHigh, 1, piHigh );
        }

        // IntGenerateRandomPrime requirement:
        //      piLow > 3 since nBitsOfModulus is bounded by
        //      SYMCRYPT_RSAKEY_MIN_BITSIZE_MODULUS.
        scError = SymCryptIntGenerateRandomPrime(
                    piLow,
                    piHigh,
                    pu64PubExp,
                    nPubExp,
                    maxTries,
                    0,
                    SymCryptIntFromModulus( pkRsakey->pmPrimes[i] ),
                    pbFnScratch,
                    cbFnScratch);
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }

        // IntToModulus requirement:
        //      piLow > 0 --> pkRsakey->pmPrimes[i] > 0
        SymCryptIntToModulus(
                SymCryptIntFromModulus( pkRsakey->pmPrimes[i] ),
                pkRsakey->pmPrimes[i],
                pkRsakey->nBitsOfModulus,       // Average number of operations
                SYMCRYPT_FLAG_MODULUS_PARITY_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
                pbFnScratch,
                cbFnScratch );

        SymCryptIntMulMixedSize( SymCryptIntFromModulus( pkRsakey->pmPrimes[i] ), piPhi, piAcc, pbFnScratch, cbFnScratch );   // P_i * Product
        scError = SymCryptIntCopyMixedSize( piAcc, piPhi );     // Move the result to piPhi
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }

    // IntToModulus requirement:
    //      piPhi product of non-zero primes --> piPhi > 0
    SymCryptIntCopy( piPhi, SymCryptIntFromModulus( pkRsakey->pmModulus ) );
    SymCryptIntToModulus(
                SymCryptIntFromModulus( pkRsakey->pmModulus ),
                pkRsakey->pmModulus,
                pkRsakey->nBitsOfModulus,       // Average number of operations
                SYMCRYPT_FLAG_DATA_PUBLIC,
                pbFnScratch,
                cbFnScratch );

    if ( SymCryptIntBitsizeOfValue( piPhi ) != pkRsakey->nBitsOfModulus)
    {
        scError = SYMCRYPT_EXTERNAL_FAILURE;    // This should never happen (make it assert)
        goto cleanup;
    }

    // Calculate the rest of the fields
    scError = SymCryptRsakeyCalculatePrivateFields( pkRsakey, pdTmp, piPhi, piAcc, pbFnScratch, cbFnScratch, 0 );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    pkRsakey->hasPrivateKey = TRUE;

    pkRsakey->fAlgorithmInfo = flags; // We want to track all of the flags in the Rsakey

    if ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 )
    {
        // Ensure RSA algorithm selftest is run before first use of RSA algorithm
        // Per FIPS 140-3 IG, this selftest cannot be a PCT
        SYMCRYPT_RUN_SELFTEST_ONCE(
            SymCryptRsaSelftest,
            SYMCRYPT_SELFTEST_ALGORITHM_RSA);

        // Run SignVerify PCT on generated keypair
        // Our current understanding is that this PCT is sufficient for both RSA_SIGN and RSA_ENCRYPT

        // Unconditionally set the sign flag to enable SignVerify PCT on encrypt-only keypair
        pkRsakey->fAlgorithmInfo |= SYMCRYPT_FLAG_RSAKEY_SIGN;

        SYMCRYPT_RUN_KEY_GEN_PCT(
            SymCryptRsaSignVerifyPct,
            pkRsakey,
            SYMCRYPT_PCT_RSA_SIGN );

        // Unset the sign flag before returning encrypt-only keypair
        if ( ( flags & SYMCRYPT_FLAG_RSAKEY_SIGN ) == 0 )
        {
            pkRsakey->fAlgorithmInfo ^= SYMCRYPT_FLAG_RSAKEY_SIGN;
        }
    }

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

// The maximum number of iterations we use in probabilistic prime recovery method
// If n, e, d are valid then successful prime recover for each iteration should
// occur with probability ~1/2; with 100 iterations we fail for a valid private
// exponent with probability ~2^-100
#define SYMCRYPT_MAX_PRIME_RECOVERY_ITERATIONS  (100)

#define SYMCRYPT_SCRATCH_BYTES_FOR_PRIME_RECOVERY( _ndMod, _ndPubExp, _nBitsMod ) \
    SymCryptSizeofIntFromDigits( _ndMod ) + /* Space for piPrivExp*/ \
    SymCryptSizeofIntFromDigits( _ndPubExp ) + /* Space for piPubExp*/ \
    SymCryptSizeofIntFromDigits( _ndMod + _ndPubExp ) + /* Space for piExpProd*/ \
    (4*SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( _nBitsMod )) + /* Space for peTmpY, peTmpX, peOne, peNegOne */\
    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_MUL( _ndMod + _ndPubExp ), /* Space for SymCryptIntMulMixedSize */ \
    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _ndMod ), /* Space for other SymCryptMod* */ \
    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( _ndMod ), /* Space for SymCryptModExp */ \
    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_EXTENDED_GCD( _ndMod ), /* Space for SymCryptIntExtendedGcd */ \
    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS( _ndMod ), /* Space for SymCryptIntToModulus */ \
            SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( _ndMod, _ndMod ) /* Space for SymCryptIntDivMod */ \
        )))))

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyCalculatePrimesFromPrivateExponent(
    _Inout_ PSYMCRYPT_RSAKEY        pkRsakey, // must already have modulus and public exponent set
    _In_reads_bytes_( cbPrivateExponent )
            PCBYTE                  pbPrivateExponent,
            SIZE_T                  cbPrivateExponent,
            SYMCRYPT_NUMBER_FORMAT  numFormat,
    _Out_writes_bytes_( cbScratch )
            PBYTE                   pbScratch,
            UINT32                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    // 3 digit sizes of temporary integers:
    //  - ndMod = pkRsakey->nDigitsOfModulus
    //  - ndPubExp = digits for a UINT64 public exponent
    //  - ndExpProd = ndMod + ndPubExp

    UINT32 ndMod = pkRsakey->nDigitsOfModulus;
    UINT32 cbMod = SymCryptSizeofIntFromDigits( ndMod );

    UINT32 ndPubExp = SymCryptDigitsFromBits( 64 );
    UINT32 cbPubExp = SymCryptSizeofIntFromDigits( ndPubExp );

    UINT32 nBitsExpProd = 0; // we compute this later before use
    UINT32 ndExpProd = ndMod + ndPubExp;
    UINT32 cbExpProd = SymCryptSizeofIntFromDigits( ndExpProd );

    UINT32 cbModElement = SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( pkRsakey->nBitsOfModulus );

    PSYMCRYPT_INT piPrivExp = NULL;
    PSYMCRYPT_INT piPubExp = NULL;
    PSYMCRYPT_INT piExpProd = NULL;
    PSYMCRYPT_MODELEMENT peTmpY = NULL;
    PSYMCRYPT_MODELEMENT peTmpX = NULL;
    PSYMCRYPT_MODELEMENT peTmpPtr = NULL;
    PSYMCRYPT_MODELEMENT peOne = NULL;
    PSYMCRYPT_MODELEMENT peNegOne = NULL;

    PBYTE   pbFnScratch = pbScratch;
    UINT32  cbFnScratch = cbScratch;

    UINT64  low64ExpProd = 0;
    UINT32  trailingZeros = 0;

    BOOL bFoundNonTrivialRoot = FALSE;

    //
    // Recover primes from private exponent using probabilistic prime-factor recovery method
    // See SP800-56B rev2 Appendix C.1 and Boneh 1999
    //
    SYMCRYPT_ASSERT( pkRsakey->nPrimes == 2 );
    SYMCRYPT_ASSERT( cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_PRIME_RECOVERY(ndMod, ndPubExp, pkRsakey->nBitsOfModulus) );

    piPrivExp = SymCryptIntCreate( pbFnScratch, cbMod, ndMod );
    SYMCRYPT_ASSERT( piPrivExp != NULL );
    pbFnScratch += cbMod;
    cbFnScratch -= cbMod;
    piPubExp = SymCryptIntCreate( pbFnScratch, cbPubExp, ndPubExp );
    SYMCRYPT_ASSERT( piPubExp != NULL );
    pbFnScratch += cbPubExp;
    cbFnScratch -= cbPubExp;
    piExpProd = SymCryptIntCreate( pbFnScratch, cbExpProd, ndExpProd );
    SYMCRYPT_ASSERT( piExpProd != NULL );
    pbFnScratch += cbExpProd;
    cbFnScratch -= cbExpProd;

    peTmpY = SymCryptModElementCreate( pbFnScratch, cbModElement, pkRsakey->pmModulus );
    SYMCRYPT_ASSERT( peTmpY != NULL );
    pbFnScratch += cbModElement;
    cbFnScratch -= cbModElement;
    peTmpX = SymCryptModElementCreate( pbFnScratch, cbModElement, pkRsakey->pmModulus );
    SYMCRYPT_ASSERT( peTmpX != NULL );
    pbFnScratch += cbModElement;
    cbFnScratch -= cbModElement;
    peOne = SymCryptModElementCreate( pbFnScratch, cbModElement, pkRsakey->pmModulus );
    SYMCRYPT_ASSERT( peOne != NULL );
    pbFnScratch += cbModElement;
    cbFnScratch -= cbModElement;
    peNegOne = SymCryptModElementCreate( pbFnScratch, cbModElement, pkRsakey->pmModulus );
    SYMCRYPT_ASSERT( peNegOne != NULL );
    pbFnScratch += cbModElement;
    cbFnScratch -= cbModElement;

    // Ensure that modulus is odd - this is required for later SymCryptIntExtendedGcd
    if( (SymCryptIntGetValueLsbits32(SymCryptIntFromModulus( pkRsakey->pmModulus ))& 1)==0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Import private exponent
    scError = SymCryptIntSetValue(pbPrivateExponent, cbPrivateExponent, numFormat, piPrivExp);
    if( scError != SYMCRYPT_NO_ERROR )
    {
        // The integer cannot fit the private exponent (SYMCRYPT_BUFFER_TOO_SMALL),
        // only if the caller providing a private exponent larger than the public modulus
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Basic range check
    if( !SymCryptIntIsLessThan(piPrivExp, SymCryptIntFromModulus(pkRsakey->pmModulus)) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Given range check, we can guarantee to compute
    // Private exponent (d) * Public exponent (e)
    // In piExpProd without overflow
    SymCryptIntSetValueUint64( pkRsakey->au64PubExp[0], piPubExp );

    // compute upper bound on product bit count based on public data (nBitsOfModulus (public) >= nBitsOfPrivateExponent (private))
    nBitsExpProd = pkRsakey->nBitsOfModulus + SymCryptIntBitsizeOfValue( piPubExp );

    SymCryptIntMulMixedSize( piPrivExp, piPubExp, piExpProd, pbFnScratch, cbFnScratch );

    // Ensure d*e is odd
    low64ExpProd = SymCryptIntGetValueLsbits64( piExpProd );

    if( (low64ExpProd & 1) == 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Compute how many trailing zeros in m = d*e - 1
    //
    // We are variable time w.r.t. the number of trailing (up to 64) zeroes. An attacker using
    // sidechannels to determine the number of trailing zeroes of m can glean information about
    // the private exponent proportionate to the number of trailing zeroes. As we bound this to
    // 64, at most an attacker can theoretically determine 64-bits of an expected 2048-bits of
    // private exponent - and they can only do this for 1 in 2^64 keys.
    //
    // It would be possible to mask the number of trailing zeroes from sidechannels by always
    // squaring by 64 times in the inner loop below and using masked operations to select out
    // any found non-trivial root. We can consider doing this as a hardening measure if this API
    // does see a lot of usage, but the expectation is that this method will almost never be
    // used and it is just not enough of a leak for an attacker to even try to measuring.
    trailingZeros = SymCryptCountTrailingZeros64( low64ExpProd-1 );

    // If there are 64 trailing zeroes then we abort because the prime factor recovery method
    // could theoretically leak more than 64-bits of the private exponent. The likelihood of any
    // key which has this many trailing zeroes _ever_ having being generated by a legitimate key
    // generation process is extremely small given the cost of RSA key generation. This much more
    // likely indicates faulty inputs or a hardware fault rather than a legitimate keypair we
    // should try to import.
    if( trailingZeros == 64 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    SymCryptIntDivPow2( piExpProd, trailingZeros, piExpProd );  // r = m >> t
    SymCryptModElementSetValueUint32( 1, pkRsakey->pmModulus, peOne, pbFnScratch, cbFnScratch );
    SymCryptModElementSetValueNegUint32( 1, pkRsakey->pmModulus, peNegOne, pbFnScratch, cbFnScratch );

    for( UINT32 i=0; i<SYMCRYPT_MAX_PRIME_RECOVERY_ITERATIONS; i++ )
    {
        // y is random value g in [2,n-2]
        SymCryptModSetRandom( pkRsakey->pmModulus, peTmpY, 0, pbFnScratch, cbFnScratch );

        // ModExp y = g^r in place
        // could make this a bit faster and leakier using trailingZeros to reduce nBitsExp, but
        // not normally a big performance win
        SymCryptModExp(
            pkRsakey->pmModulus,
            peTmpY,
            piExpProd,
            nBitsExpProd-1,
            0,
            peTmpY,
            pbFnScratch, cbFnScratch );

        // if y == 1 or y == -1, start over (we found a trivial root of 1)
        if( SymCryptModElementIsEqual( pkRsakey->pmModulus, peTmpY, peOne ) ||
            SymCryptModElementIsEqual( pkRsakey->pmModulus, peTmpY, peNegOne ) )
        {
            continue;
        }

        for( UINT32 j=1; j<=trailingZeros; j++ )
        {
            // x = y^2
            SymCryptModSquare( pkRsakey->pmModulus, peTmpY, peTmpX, pbFnScratch, cbFnScratch );

            // if x == 1 then y is a non-trivial root of 1 (it is not -1 or 1)
            if( SymCryptModElementIsEqual( pkRsakey->pmModulus, peTmpX, peOne) )
            {
                bFoundNonTrivialRoot = TRUE;
                break;
            }

            // if x == -1, start over
            if( SymCryptModElementIsEqual( pkRsakey->pmModulus, peTmpX, peNegOne) )
            {
                break; // just break out of inner loop; continues outer loop
            }

            // swap x and y
            peTmpPtr = peTmpY;
            peTmpY = peTmpX;
            peTmpX = peTmpPtr;
        }
        if( bFoundNonTrivialRoot )
        {
            break;
        }
    }

    if( !bFoundNonTrivialRoot )
    {
        // we failed to find a non-trivial root of 1, so we cannot recover prime factors
        // it is almost certain that this means that the inputs were wrong
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // piPrivExp = y
    SymCryptModElementToInt( pkRsakey->pmModulus, peTmpY, piPrivExp, pbFnScratch, cbFnScratch );
    // piPrivExp = y-1 (we know this cannot borrow as y^2 is 1, so y != 0)
    SymCryptIntSubUint32( piPrivExp, 1, piPrivExp );

    // piPrivExp = p0 = GCD(y-1, n)
    SymCryptIntExtendedGcd(
        piPrivExp,
        SymCryptIntFromModulus( pkRsakey->pmModulus ),
        SYMCRYPT_FLAG_GCD_INPUTS_NOT_BOTH_EVEN,
        piPrivExp,
        NULL,
        NULL,
        NULL,
        pbFnScratch, cbFnScratch );

    // compute the sizes of the primes
    pkRsakey->nBitsOfPrimes[0] = SymCryptIntBitsizeOfValue(piPrivExp);
    pkRsakey->nBitsOfPrimes[1] = pkRsakey->nBitsOfModulus - pkRsakey->nBitsOfPrimes[0];
    for( UINT32 i=0; i<2; i++ )
    {
        pkRsakey->nDigitsOfPrimes[i] = SymCryptDigitsFromBits(pkRsakey->nBitsOfPrimes[i]);
        if( pkRsakey->nBitsOfPrimes[i] < SYMCRYPT_RSAKEY_MIN_BITSIZE_PRIME )
        {
            scError = SYMCRYPT_WRONG_KEY_SIZE;
            goto cleanup;
        }
    }
    pkRsakey->nMaxDigitsOfPrimes = SYMCRYPT_MAX(pkRsakey->nDigitsOfPrimes[0], pkRsakey->nDigitsOfPrimes[1]);

    // Create all the objects
    SymCryptRsakeyCreateAllObjects(pkRsakey);

    scError = SymCryptIntCopyMixedSize( piPrivExp, SymCryptIntFromModulus( pkRsakey->pmPrimes[0] ) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        // only fails if we computed the wrong bit-size for the primes above
        scError = SYMCRYPT_HARDWARE_FAILURE;
        goto cleanup;
    }

    SymCryptIntToModulus(   SymCryptIntFromModulus( pkRsakey->pmPrimes[0] ),
                            pkRsakey->pmPrimes[0],
                            pkRsakey->nBitsOfModulus,   // Average number of operations
                            SYMCRYPT_FLAG_MODULUS_PARITY_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
                            pbFnScratch, cbFnScratch );

    SymCryptIntDivMod( SymCryptIntFromModulus( pkRsakey->pmModulus ),
                       SymCryptDivisorFromModulus( pkRsakey->pmPrimes[0] ),
                       piExpProd, // n / p0 - use piExpProd as Quotient.nDigits must be >= Src.nDigits
                       piPrivExp, // n % p0 - use piPrivExp as Remainder.nDigits must be >= Divisor.nDigits
                       pbFnScratch, cbFnScratch );

    // Check remainder from dividing n by p0 is 0
    if( !SymCryptIntIsEqualUint32( piPrivExp, 0 ) )
    {
        // Should always be true as p0 is GCD(y-1, n) so is definitionally a divisor of n
        // Failure here indicates something wrong in our math, or hardware failure
        scError = SYMCRYPT_HARDWARE_FAILURE;
        goto cleanup;
    }

    scError = SymCryptIntCopyMixedSize( piExpProd, SymCryptIntFromModulus( pkRsakey->pmPrimes[1] ) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        // only fails if we computed the wrong bit-size for the primes above
        scError = SYMCRYPT_HARDWARE_FAILURE;
        goto cleanup;
    }

    SymCryptIntToModulus(   SymCryptIntFromModulus( pkRsakey->pmPrimes[1] ),
                            pkRsakey->pmPrimes[1],
                            pkRsakey->nBitsOfModulus,   // Average number of operations
                            SYMCRYPT_FLAG_MODULUS_PARITY_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
                            pbFnScratch, cbFnScratch );

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeySetValueInternal(
    _In_reads_bytes_( cbModulus )               PCBYTE                  pbModulus,
                                                SIZE_T                  cbModulus,
    _In_reads_( nPubExp )                       PCUINT64                pu64PubExp,
                                                UINT32                  nPubExp,
    _In_reads_bytes_opt_( cbPrivateExponent )   PCBYTE                  pbPrivateExponent,
                                                SIZE_T                  cbPrivateExponent,
    _In_reads_opt_( nPrimes )                   PCBYTE *                ppPrimes,
    _In_reads_opt_( nPrimes )                   SIZE_T *                pcbPrimes,
                                                UINT32                  nPrimes,
                                                SYMCRYPT_NUMBER_FORMAT  numFormat,
                                                UINT32                  flags,
    _Inout_                                     PSYMCRYPT_RSAKEY        pkRsakey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    // 3 digit sizes of temporary integers:
    //  - ndPrimes = max digitsize of prime buffers
    //  - ndMod = pkRsakey->nDigitsOfModulus
    //  - ndLarge = ndPrimes + ndMod

    UINT32 cbDivisor = 0;
    PSYMCRYPT_DIVISOR pdTmp = NULL;

    UINT32 ndMod = 0;
    UINT32 cbMod = 0;
    PSYMCRYPT_INT piPhi = NULL;

    UINT32 cbLarge = 0;
    PSYMCRYPT_INT piAcc = NULL;

    PBYTE           pbScratch = NULL;
    UINT32          cbScratch = 0;
    PBYTE           pbFnScratch = NULL;
    UINT32          cbFnScratch = 0;

    // Ensure caller has specified what algorithm(s) the key will be used with
    UINT32 algorithmFlags = SYMCRYPT_FLAG_RSAKEY_SIGN | SYMCRYPT_FLAG_RSAKEY_ENCRYPT;
    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION | algorithmFlags;

    if ( ( ( flags & ~allowedFlags ) != 0 ) ||
         ( ( flags & algorithmFlags ) == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Check that minimal validation flag only specified with no fips
    if ( ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 ) &&
         ( ( flags & SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION ) != 0 ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Internal requirement that private key is either specified by primes or by private exponent, not both
    // This is not exposed to external API surface - if we were to dynamically check, the SYMCRYPT_ERROR
    // should indicate internal logic error - for now just assert
    SYMCRYPT_ASSERT( (nPrimes==0) || (pbPrivateExponent==NULL) );

    // Check if the arguments are correct
    if ( (pbModulus==NULL) || (cbModulus==0) ||         // Modulus is needed
         (nPubExp != 1) || (pu64PubExp==NULL) ||        // Exactly 1 public exponent is needed
         ((nPrimes != 2) && (nPrimes != 0)) ||
         ((nPrimes == 2) && ((ppPrimes==NULL) || (pcbPrimes==NULL) ||
                             (ppPrimes[0]==NULL) || (ppPrimes[1]==NULL) ||
                             (pcbPrimes[0]==0) || (pcbPrimes[1]==0))) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    ndMod = pkRsakey->nDigitsOfModulus;

    // Calculate scratch spaces
	// No integer overflows as all numbers are limited by ndMod which is checked during Create
    if ( (pbPrivateExponent != NULL) || (nPrimes > 0) )
    {
        if( pkRsakey->nPrimes != 2 )
        {
            // The key was not allocated with space for private key material
            // so we cannot set it with private key material
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        cbMod = SymCryptSizeofIntFromDigits( ndMod );
        cbLarge = SymCryptSizeofIntFromDigits( 2 * ndMod ); // 2*ndMod is still < SymCryptDigitsFromBits(SYMCRYPT_INT_MAX_BITS)
        cbDivisor = SymCryptSizeofDivisorFromDigits( ndMod );

        cbScratch = cbMod + cbLarge + cbDivisor +
                    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS(ndMod),
                    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_CRT_GENERATION(ndMod),
                    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_EXTENDED_GCD(ndMod),
                    SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_DIVISOR(ndMod),
                         SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( ndMod, ndMod )
                        ))));

        if( pbPrivateExponent != NULL )
        {
            // We use at least as much scratch space when importing by private exponent, but probably more
            SYMCRYPT_ASSERT( SymCryptDigitsFromBits( 64 ) == 1 );

            cbScratch = SYMCRYPT_MAX( cbScratch,
                SYMCRYPT_SCRATCH_BYTES_FOR_PRIME_RECOVERY(ndMod, 1, pkRsakey->nSetBitsOfModulus) );
        }
    }
    else
    {
        cbScratch = SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS(ndMod);
    }

    pbScratch = (PBYTE)SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Modulus
    scError = SymCryptIntSetValue( pbModulus, cbModulus, numFormat, SymCryptIntFromModulus( pkRsakey->pmModulus ) );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Compute actual modulus size, and check that it isn't bigger than the created size
    pkRsakey->nBitsOfModulus = SymCryptIntBitsizeOfValue(SymCryptIntFromModulus(pkRsakey->pmModulus));
    if (pkRsakey->nBitsOfModulus > pkRsakey->nSetBitsOfModulus)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (pkRsakey->nBitsOfModulus < SYMCRYPT_RSAKEY_MIN_BITSIZE_MODULUS)
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    // IntToModulus requirement:
    //      nBitsOfModulus >= SYMCRYPT_RSAKEY_MIN_BITSIZE_MODULUS --> pmModulus > 0
    SymCryptIntToModulus(
            SymCryptIntFromModulus( pkRsakey->pmModulus ),
            pkRsakey->pmModulus,
            pkRsakey->nBitsOfModulus,
            SYMCRYPT_FLAG_DATA_PUBLIC,
            pbScratch,
            cbScratch );

    // Public exponents
    pkRsakey->nPubExp = nPubExp;
    for (UINT32 i = 0; i<pkRsakey->nPubExp; i++)
    {
        pkRsakey->au64PubExp[i] = pu64PubExp[i];
    }

    //  Private key import either by private exponent or primes
    if ( (pbPrivateExponent != NULL) || (nPrimes > 0) )
    {
        if (pbPrivateExponent != NULL)
        {
            // Private exponent
            scError = SymCryptRsakeyCalculatePrimesFromPrivateExponent(
                pkRsakey,
                pbPrivateExponent, cbPrivateExponent,
                numFormat,
                pbScratch, cbScratch );
            if (scError != SYMCRYPT_NO_ERROR)
            {
                goto cleanup;
            }

            pbFnScratch = pbScratch;
            cbFnScratch = cbScratch;

            // Create temporary piPhi
            piPhi = SymCryptIntCreate( pbFnScratch, cbMod, ndMod ); pbFnScratch += cbMod; cbFnScratch -= cbMod;
        }
        else //if (nPrimes > 0)
        {
            // Primes
            pbFnScratch = pbScratch;
            cbFnScratch = cbScratch;

            // Create temporary piPhi
            piPhi = SymCryptIntCreate( pbFnScratch, cbMod, ndMod ); pbFnScratch += cbMod; cbFnScratch -= cbMod;

            // First fix the tight number of digits of each prime
            pkRsakey->nMaxDigitsOfPrimes = 0;
            for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
            {
#pragma warning(suppress: 26007) // "Incorrect Annotation" - cannot phrase array of pointers to arrays in SAL
                scError = SymCryptIntSetValue( ppPrimes[i], pcbPrimes[i], numFormat, piPhi );
                if (scError != SYMCRYPT_NO_ERROR )
                {
                    goto cleanup;
                }

                pkRsakey->nBitsOfPrimes[i] = SymCryptIntBitsizeOfValue(piPhi);
                pkRsakey->nDigitsOfPrimes[i] = SymCryptDigitsFromBits(pkRsakey->nBitsOfPrimes[i]);

                pkRsakey->nMaxDigitsOfPrimes = SYMCRYPT_MAX(pkRsakey->nMaxDigitsOfPrimes, pkRsakey->nDigitsOfPrimes[i]);

                if (pkRsakey->nBitsOfPrimes[i] < SYMCRYPT_RSAKEY_MIN_BITSIZE_PRIME)
                {
                    scError = SYMCRYPT_WRONG_KEY_SIZE;
                    goto cleanup;
                }
            }

            // Create all the objects
            SymCryptRsakeyCreateAllObjects(pkRsakey);

            // Set the values
            for (UINT32 i=0; i<pkRsakey->nPrimes; i++)
            {
#pragma warning(suppress: 26007) // "Incorrect Annotation" - cannot phrase array of pointers to arrays in SAL
                scError = SymCryptIntSetValue( ppPrimes[i], pcbPrimes[i], numFormat, SymCryptIntFromModulus( pkRsakey->pmPrimes[i] ) );
                if (scError != SYMCRYPT_NO_ERROR )
                {
                    goto cleanup;
                }

                // Check that this prime is odd (should we check for primality?)
                if ((SymCryptIntGetValueLsbits32(SymCryptIntFromModulus( pkRsakey->pmPrimes[i] ))& 1)==0)
                {
                    scError = SYMCRYPT_INVALID_ARGUMENT;
                    goto cleanup;
                }

                // IntToModulus requirement:
                //      nBitsOfPrimes >= SYMCRYPT_RSAKEY_MIN_BITSIZE_PRIME --> pmPrimes[i] > 0
                SymCryptIntToModulus(
                        SymCryptIntFromModulus( pkRsakey->pmPrimes[i] ),
                        pkRsakey->pmPrimes[i],
                        pkRsakey->nBitsOfModulus,   // Average number of operations
                        SYMCRYPT_FLAG_MODULUS_PARITY_PUBLIC | SYMCRYPT_FLAG_MODULUS_PRIME,
                        pbFnScratch,
                        cbFnScratch );
            }
        }

        // Create remaining temporaries
        piAcc = SymCryptIntCreate( pbFnScratch, cbLarge, 2 * ndMod ); pbFnScratch += cbLarge; cbFnScratch -= cbLarge;
        pdTmp = SymCryptDivisorCreate( pbFnScratch, cbDivisor, ndMod ); pbFnScratch += cbDivisor; cbFnScratch -= cbDivisor;

        // Calculate the rest of the fields
        scError = SymCryptRsakeyCalculatePrivateFields( pkRsakey, pdTmp, piPhi, piAcc, pbFnScratch, cbFnScratch, flags & SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION );
        if (scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Everything is set here
        pkRsakey->hasPrivateKey = TRUE;
    }

    pkRsakey->fAlgorithmInfo = flags; // We want to track all of the flags in the Rsakey

    if ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 )
    {
        // Ensure RSA algorithm selftest is run before first use of RSA algorithm
        SYMCRYPT_RUN_SELFTEST_ONCE(
            SymCryptRsaSelftest,
            SYMCRYPT_SELFTEST_ALGORITHM_RSA);

        // PCT does not need to be run on import - mark it as done
        pkRsakey->fAlgorithmInfo |= SYMCRYPT_PCT_RSA_SIGN;
    }

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeySetValue(
    _In_reads_bytes_( cbModulus )   PCBYTE                  pbModulus,
                                    SIZE_T                  cbModulus,
    _In_reads_( nPubExp )           PCUINT64                pu64PubExp,
                                    UINT32                  nPubExp,
    _In_reads_opt_( nPrimes )       PCBYTE *                ppPrimes,
    _In_reads_opt_( nPrimes )       SIZE_T *                pcbPrimes,
                                    UINT32                  nPrimes,
                                    SYMCRYPT_NUMBER_FORMAT  numFormat,
                                    UINT32                  flags,
    _Inout_                         PSYMCRYPT_RSAKEY        pkRsakey )
{
    return SymCryptRsakeySetValueInternal(
        pbModulus, cbModulus,
        pu64PubExp, nPubExp,
        NULL, 0,
        ppPrimes, pcbPrimes, nPrimes,
        numFormat,
        flags,
        pkRsakey );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeySetValueFromPrivateExponent(
    _In_reads_bytes_( cbModulus )           PCBYTE                  pbModulus,
                                            SIZE_T                  cbModulus,
                                            UINT64                  u64PubExp,
    _In_reads_bytes_( cbPrivateExponent )   PCBYTE                  pbPrivateExponent,
                                            SIZE_T                  cbPrivateExponent,
                                            SYMCRYPT_NUMBER_FORMAT  numFormat,
                                            UINT32                  flags,
    _Inout_                                 PSYMCRYPT_RSAKEY        pkRsakey )
{
    return SymCryptRsakeySetValueInternal(
        pbModulus, cbModulus,
        &u64PubExp, 1,
        pbPrivateExponent, cbPrivateExponent,
        NULL, NULL, 0,
        numFormat,
        flags,
        pkRsakey );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyGetValue(
    _In_                            PCSYMCRYPT_RSAKEY       pkRsakey,
    _Out_writes_bytes_( cbModulus ) PBYTE                   pbModulus,
                                    SIZE_T                  cbModulus,
    _Out_writes_opt_( nPubExp )     PUINT64                 pu64PubExp,
                                    UINT32                  nPubExp,
    _Out_writes_opt_( nPrimes )     PBYTE *                 ppPrimes,
    _In_reads_opt_( nPrimes )       SIZE_T *                pcbPrimes,
                                    UINT32                  nPrimes,
                                    SYMCRYPT_NUMBER_FORMAT  numFormat,
                                    UINT32                  flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    UNREFERENCED_PARAMETER( flags );

    // Check if private key needed but not there
    if ((nPrimes!=0) && (pkRsakey->hasPrivateKey == FALSE))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Modulus
    if (pbModulus!=NULL)
    {
        // We'll get an error if cbModulus is 0 or too small
        scError = SymCryptIntGetValue( SymCryptIntFromModulus( pkRsakey->pmModulus ), pbModulus, cbModulus, numFormat );
        if (scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    // Public exponents
    if( pu64PubExp != NULL )
    {
        if( nPubExp != 1 )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
        pu64PubExp[0] = pkRsakey->au64PubExp[0];
    }

    // Primes i.e. private key
    if( nPrimes != 0 )
    {
        if( nPrimes != 2 || ppPrimes == NULL || pcbPrimes == NULL )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        for (UINT32 i=0; i<nPrimes; i++)
        {
            if (ppPrimes[i]!=NULL)
            {
                scError = SymCryptIntGetValue( SymCryptIntFromModulus( pkRsakey->pmPrimes[i] ), ppPrimes[i], pcbPrimes[i], numFormat );
                if (scError != SYMCRYPT_NO_ERROR )
                {
                    goto cleanup;
                }
            }
        }
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyGetCrtValue(
    _In_                                        PCSYMCRYPT_RSAKEY       pkRsakey,
    _Out_writes_opt_(nCrtExponents)             PBYTE *                 ppCrtExponents,
    _In_reads_(nCrtExponents)                   SIZE_T *                pcbCrtExponents,
                                                UINT32                  nCrtExponents,
    _Out_writes_bytes_opt_(cbCrtCoefficient)    PBYTE                   pbCrtCoefficient,
                                                SIZE_T                  cbCrtCoefficient,
    _Out_writes_bytes_opt_(cbPrivateExponent)   PBYTE                   pbPrivateExponent,
                                                SIZE_T                  cbPrivateExponent,
                                                SYMCRYPT_NUMBER_FORMAT  numFormat,
                                                UINT32                  flags)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;

    UNREFERENCED_PARAMETER( flags );

    // Check if the arguments are correct
    if ( (ppCrtExponents==NULL) && (nCrtExponents!=0) ||
         (nCrtExponents != 0 && nCrtExponents != 2 ))
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Crt value can only be available we have private key.
    if (pkRsakey->hasPrivateKey == FALSE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Crt exponents
    for (UINT32 i=0; i<nCrtExponents; i++)
    {
        if (ppCrtExponents[i]!=NULL)
        {
            scError = SymCryptIntGetValue( pkRsakey->piCrtPrivExps[i], ppCrtExponents[i], pcbCrtExponents[i], numFormat );
            if (scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }
        }
    }

    if (pbCrtCoefficient!=NULL)
    {
        cbScratch = SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( pkRsakey->nDigitsOfModulus );
        pbScratch = SymCryptCallbackAlloc( cbScratch );

        if (pbScratch==NULL)
        {
            scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }

        scError = SymCryptModElementGetValue(
                      pkRsakey->pmPrimes[0],
                      pkRsakey->peCrtInverses[0],
                      pbCrtCoefficient,
                      cbCrtCoefficient,
                      numFormat,
                      pbScratch,
                      cbScratch);
        if (scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    if (pbPrivateExponent!=NULL)
    {
        scError = SymCryptIntGetValue( pkRsakey->piPrivExps[0], pbPrivateExponent, cbPrivateExponent, numFormat );
        if (scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

cleanup:

    if (pbScratch!=NULL)
    {
        SymCryptWipe(pbScratch,cbScratch);
        SymCryptCallbackFree(pbScratch);
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyExtendKeyUsage(
    _Inout_ PSYMCRYPT_RSAKEY    pkRsakey,
            UINT32              flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    // Ensure caller has specified what algorithm(s) the key will be used with
    UINT32 algorithmFlags = SYMCRYPT_FLAG_RSAKEY_SIGN | SYMCRYPT_FLAG_RSAKEY_ENCRYPT;

    if ( ( ( flags & ~algorithmFlags ) != 0 ) ||
         ( ( flags & algorithmFlags ) == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pkRsakey->fAlgorithmInfo |= flags;

cleanup:
    return scError;
}
