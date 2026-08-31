//
// dlkey.c   Dlkey functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//

#include "precomp.h"

PSYMCRYPT_DLKEY
SYMCRYPT_CALL
SymCryptDlkeyAllocate( _In_ PCSYMCRYPT_DLGROUP pDlgroup )
{
    PVOID               p;
    SIZE_T              cb;
    PSYMCRYPT_DLKEY     res = NULL;

    cb = SymCryptSizeofDlkeyFromDlgroup( pDlgroup );

    p = SymCryptCallbackAlloc( cb );

    if ( p==NULL )
    {
        goto cleanup;
    }

    res = SymCryptDlkeyCreate( p, cb, pDlgroup );

cleanup:
    return res;
}

VOID
SYMCRYPT_CALL
SymCryptDlkeyFree( _Out_ PSYMCRYPT_DLKEY pkObj )
{
    SYMCRYPT_CHECK_MAGIC( pkObj );
    SymCryptDlkeyWipe( pkObj );
    SymCryptCallbackFree( pkObj );
}

UINT32
SYMCRYPT_CALL
SymCryptSizeofDlkeyFromDlgroup( _In_ PCSYMCRYPT_DLGROUP pDlgroup )
{
    // Always allocate memory for large private keys
    return sizeof(SYMCRYPT_DLKEY) + SymCryptSizeofModElementFromModulus( pDlgroup->pmP ) + SymCryptSizeofIntFromDigits( pDlgroup->nDigitsOfP );
}

PSYMCRYPT_DLKEY
SYMCRYPT_CALL
SymCryptDlkeyCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
    _In_                            PCSYMCRYPT_DLGROUP  pDlgroup )
{
    PSYMCRYPT_DLKEY pkRes = NULL;
    UINT32 cbModElement = SymCryptSizeofModElementFromModulus( pDlgroup->pmP );

    SYMCRYPT_ASSERT( cbBuffer >= SymCryptSizeofDlkeyFromDlgroup( pDlgroup ) );
    SYMCRYPT_ASSERT( cbBuffer >= sizeof(SYMCRYPT_DLKEY) + cbModElement );
    UNREFERENCED_PARAMETER( cbBuffer );     // only referenced in above ASSERTs...
    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );

    pkRes = (PSYMCRYPT_DLKEY) pbBuffer;

    // DLKEY parameters
    pkRes->fAlgorithmInfo = 0;
    pkRes->pDlgroup = pDlgroup;
    pkRes->fHasPrivateKey = FALSE;
    pkRes->fPrivateModQ = FALSE;            // This will be properly set during generate or setvalue
    pkRes->nBitsPriv = pDlgroup->nDefaultBitsPriv;

    // Create SymCrypt objects
    pbBuffer += sizeof(SYMCRYPT_DLKEY);

    pkRes->pePublicKey = SymCryptModElementCreate( pbBuffer, cbModElement, pDlgroup->pmP );
    if (pkRes->pePublicKey == NULL)
    {
        goto cleanup;
    }
    pbBuffer += cbModElement;

    //
    // **** Always defer the creation of the private key until the key generation or
    // set value.
    //
    // In place of the pbPrivate pointer store the pointer to the allocated buffer.
    //
    pkRes->pbPrivate = pbBuffer;
    pkRes->piPrivateKey = NULL;

    // Setting the magic
    SYMCRYPT_SET_MAGIC( pkRes );

cleanup:
    return pkRes;
}

VOID
SYMCRYPT_CALL
SymCryptDlkeyWipe( _Out_ PSYMCRYPT_DLKEY pkDst )
{
    SymCryptWipe( (PBYTE) pkDst, SymCryptSizeofDlkeyFromDlgroup(pkDst->pDlgroup) );
}

VOID
SYMCRYPT_CALL
SymCryptDlkeyCopy(
    _In_    PCSYMCRYPT_DLKEY   pkSrc,
    _Out_   PSYMCRYPT_DLKEY    pkDst )
{
    PCSYMCRYPT_DLGROUP pDlgroup = pkSrc->pDlgroup;

    //
    // in-place copy is somewhat common...
    //
    if( pkSrc != pkDst )
    {
        pkDst->fAlgorithmInfo = pkSrc->fAlgorithmInfo;
        pkDst->fHasPrivateKey = pkSrc->fHasPrivateKey;
        pkDst->fPrivateModQ = pkSrc->fPrivateModQ;
        pkDst->nBitsPriv = pkSrc->nBitsPriv;

        // Copy the public key
        SymCryptModElementCopy( pDlgroup->pmP, pkSrc->pePublicKey, pkDst->pePublicKey );

        // Copy the private key
        SymCryptIntCopy( pkSrc->piPrivateKey, pkDst->piPrivateKey );
    }
}


// DLKEY specific functions

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeySetPrivateKeyLength( _Inout_ PSYMCRYPT_DLKEY pkDlkey, UINT32 nBitsPriv, UINT32 flags )
{
    if( nBitsPriv > pkDlkey->pDlgroup->nBitsOfQ ||
        nBitsPriv < pkDlkey->pDlgroup->nMinBitsPriv ||
        flags != 0 )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    pkDlkey->nBitsPriv = nBitsPriv;
    return SYMCRYPT_NO_ERROR;
}

PCSYMCRYPT_DLGROUP
SYMCRYPT_CALL
SymCryptDlkeyGetGroup( _In_ PCSYMCRYPT_DLKEY pkDlkey )
{
    return pkDlkey->pDlgroup;
}

UINT32
SYMCRYPT_CALL
SymCryptDlkeySizeofPublicKey( _In_ PCSYMCRYPT_DLKEY pkDlkey )
{
    return pkDlkey->pDlgroup->cbPrimeP;
}

UINT32
SYMCRYPT_CALL
SymCryptDlkeySizeofPrivateKey( _In_ PCSYMCRYPT_DLKEY pkDlkey )
{
    PCSYMCRYPT_DLGROUP pDlgroup = pkDlkey->pDlgroup;

    if (pkDlkey->fPrivateModQ)
    {
        if (pDlgroup->fHasPrimeQ)
        {
            if (pkDlkey->nBitsPriv != pDlgroup->nBitsOfQ)
            {
                return (pkDlkey->nBitsPriv + 7) / 8;
            }
            else
            {
                return pDlgroup->cbPrimeQ;
            }
        }
        else
        {
            return pDlgroup->cbPrimeP;  // Somehow the group has no prime Q but the key was set with prime Q, return the safe option
        }
    }
    else
    {
        return pDlgroup->cbPrimeP;
    }
}

BOOLEAN
SYMCRYPT_CALL
SymCryptDlkeyHasPrivateKey( _In_ PCSYMCRYPT_DLKEY pkDlkey )
{
    return pkDlkey->fHasPrivateKey;
}

#define SYMCRYPT_FLAG_DLKEY_PUBLIC_KEY_ORDER_VALIDATION (0x1)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeyPerformPublicKeyValidation(
    _In_                            PCSYMCRYPT_DLKEY        pkDlkey,
    _In_                            UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    PCSYMCRYPT_DLGROUP pDlgroup = pkDlkey->pDlgroup;

    PSYMCRYPT_MODELEMENT peTmp = NULL;
    PSYMCRYPT_MODELEMENT peTmpPublicKeyExpQ = NULL;
    UINT32 cbModElement = SymCryptSizeofModElementFromModulus( pDlgroup->pmP );

    SYMCRYPT_ASSERT( cbScratch >= (2 * cbModElement) +
                                    SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP(pDlgroup->nDigitsOfP) );

    // Check if Public key is 0
    if ( SymCryptModElementIsZero( pDlgroup->pmP, pkDlkey->pePublicKey ) )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    peTmp = SymCryptModElementCreate( pbScratch, cbModElement, pDlgroup->pmP);
    pbScratch += cbModElement;
    cbScratch -= cbModElement;

    // Check if Public key is P-1
    SymCryptModElementSetValueNegUint32( 1, pDlgroup->pmP, peTmp, pbScratch, cbScratch );
    if ( SymCryptModElementIsEqual( pDlgroup->pmP, pkDlkey->pePublicKey, peTmp ) )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    // Check if Public key is 1 (do this check second as we may reuse 1 element in next check)
    SymCryptModElementSetValueUint32( 1, pDlgroup->pmP, peTmp, pbScratch, cbScratch );
    if ( SymCryptModElementIsEqual( pDlgroup->pmP, pkDlkey->pePublicKey, peTmp ) )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    // Perform validation that Public key is in a subgroup of order Q.
    if ( (flags & SYMCRYPT_FLAG_DLKEY_PUBLIC_KEY_ORDER_VALIDATION) != 0 )
    {
        peTmpPublicKeyExpQ = SymCryptModElementCreate( pbScratch, cbModElement, pDlgroup->pmP);
        pbScratch += cbModElement;
        cbScratch -= cbModElement;

        // Ensure that Q is specified in the Dlgroup
        if ( !pDlgroup->fHasPrimeQ )
        {
            return SYMCRYPT_INVALID_ARGUMENT;
        }

        // Calculate peTmpPublicKeyExpQ = (Public key)^Q
        SymCryptModExp(
            pDlgroup->pmP,
            pkDlkey->pePublicKey,
            SymCryptIntFromModulus( pDlgroup->pmQ ),
            pDlgroup->nBitsOfQ,
            SYMCRYPT_FLAG_DATA_PUBLIC, // No need for side-channel safety for public key validation
            peTmpPublicKeyExpQ,
            pbScratch,
            cbScratch );

        // Ensure (Public key)^Q == 1 mod P
        if ( !SymCryptModElementIsEqual( pDlgroup->pmP, peTmpPublicKeyExpQ, peTmp ) )
        {
            return SYMCRYPT_INVALID_ARGUMENT;
        }
    }

    return SYMCRYPT_NO_ERROR;
}

#define DLKEY_GEN_RANDOM_GENERIC_LIMIT   (1000)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeyGenerate(
    _In_    UINT32                  flags,
    _Inout_ PSYMCRYPT_DLKEY         pkDlkey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbScratch = NULL;
    SIZE_T cbScratch = 0;
    PBYTE pbScratchInternal = NULL;
    SIZE_T cbScratchInternal = 0;

    PCSYMCRYPT_DLGROUP pDlgroup = pkDlkey->pDlgroup;

    PSYMCRYPT_MODELEMENT pePrivateKey = NULL;
    UINT32 cbPrivateKey = 0;

    PSYMCRYPT_MODULUS pmPriv = NULL;
    UINT32 nDigitsPriv = 0;
    UINT32 nBitsPriv = 0;
    UINT32 fFlagsForModSetRandom = 0;

    BOOLEAN useModSetRandom = TRUE;
    UINT32 nBytesPriv = 0;
    UINT32 dwShiftBits;
    BYTE   privMask;
    UINT32 cntr;

    PSYMCRYPT_MODELEMENT peTmp = NULL;
    UINT32 cbModElement = SymCryptSizeofModElementFromModulus( pDlgroup->pmP );

    // Ensure caller has specified what algorithm(s) the key will be used with
    UINT32 algorithmFlags = SYMCRYPT_FLAG_DLKEY_DSA | SYMCRYPT_FLAG_DLKEY_DH;
    // Make sure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_DLKEY_GEN_MODP | SYMCRYPT_FLAG_KEY_NO_FIPS | algorithmFlags;

    if ( ( ( flags & ~allowedFlags ) != 0 ) ||
         ( ( flags & algorithmFlags ) == 0 ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Extra sanity checks when running with FIPS
    // Either Dlgroup is named SafePrime group and key is for DH,
    // or Dlgroup is not named SafePrime group and key is for DSA
    if ( ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 ) &&
         ( (pDlgroup->isSafePrimeGroup && (flags & SYMCRYPT_FLAG_DLKEY_DSA)) ||
           (!(pDlgroup->isSafePrimeGroup) && (flags & SYMCRYPT_FLAG_DLKEY_DH)) ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pkDlkey->fPrivateModQ = (((flags & SYMCRYPT_FLAG_DLKEY_GEN_MODP)==0) && (pDlgroup->fHasPrimeQ));

    if (pkDlkey->fPrivateModQ)
    {
        pmPriv = pDlgroup->pmQ;
        nDigitsPriv = pDlgroup->nDigitsOfQ;
        nBitsPriv = pDlgroup->nBitsOfQ;
        fFlagsForModSetRandom = SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE | SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE; // 1 to Q-1

        if ( pDlgroup->isSafePrimeGroup && (pkDlkey->nBitsPriv != pDlgroup->nBitsOfQ) )
        {
            useModSetRandom = FALSE;
            SYMCRYPT_ASSERT( pkDlkey->nBitsPriv < pDlgroup->nBitsOfQ );     // 2^nBitsPriv < Q

            nBitsPriv = pkDlkey->nBitsPriv;                                 // 1 to (2^nBitsPriv)-1
            nBytesPriv = (pkDlkey->nBitsPriv + 7) / 8;
        }
    }
    else
    {
        // We perform Private key range validation by construction
        // The Private key is constructed in the range [1,min(2^nBitsPriv,Q)-1] precisely when pkDlkey->fPrivateModQ
        if ( (flags & SYMCRYPT_FLAG_KEY_NO_FIPS) == 0 )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        pmPriv = pDlgroup->pmP;
        nDigitsPriv = pDlgroup->nDigitsOfP;
        nBitsPriv = pDlgroup->nBitsOfP;
        fFlagsForModSetRandom = SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE;                                          // 1 to P-2
    }

    cbPrivateKey = SymCryptSizeofModElementFromModulus( pmPriv );

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = SYMCRYPT_MAX( cbPrivateKey + SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS(nDigitsPriv),
                    (2 * cbModElement) + SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP(pDlgroup->nDigitsOfP));
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Create the private key integer
    pkDlkey->piPrivateKey = SymCryptIntCreate( pkDlkey->pbPrivate, SymCryptSizeofIntFromDigits(nDigitsPriv), nDigitsPriv );

    if (useModSetRandom)
    {
        // Create the private key modelement
        pePrivateKey = SymCryptModElementCreate( pbScratch, cbPrivateKey, pmPriv );
        pbScratchInternal = pbScratch + cbPrivateKey;
        cbScratchInternal = cbScratch - cbPrivateKey;

        // Set a modelement from 1 to q-1 (or 1 to p-2)
        SymCryptModSetRandom(
            pmPriv,
            pePrivateKey,
            fFlagsForModSetRandom,
            pbScratchInternal,
            cbScratchInternal );

        // Set the private key
        SymCryptModElementToInt(
            pmPriv,
            pePrivateKey,
            pkDlkey->piPrivateKey,
            pbScratchInternal,
            cbScratchInternal );
    }
    else
    {
        // Set private key from 1 to (2^nBitsPriv)-1
        // Wipe any bytes we won't fill with random
        SymCryptWipe( pbScratch + nBytesPriv, (nDigitsPriv * SYMCRYPT_FDEF_DIGIT_SIZE) - nBytesPriv );

        dwShiftBits = (0u-nBitsPriv) & 7;
        privMask = (BYTE)(0xff >> dwShiftBits);

        for(cntr=0; cntr<DLKEY_GEN_RANDOM_GENERIC_LIMIT; cntr++)
        {
            // Try random values until we get one we like
            SymCryptCallbackRandom( pbScratch, nBytesPriv );

            pbScratch[nBytesPriv-1] &= privMask;

            // If non-zero we have a value in range [1, (2^nBitsPriv)-1]
            if( !SymCryptFdefRawIsEqualUint32( (PCUINT32)pbScratch, nDigitsPriv, 0 ) )
            {
                break;
            }
        }

        if (cntr >= DLKEY_GEN_RANDOM_GENERIC_LIMIT)
        {
            SymCryptFatal( 'rndl' );
        }

        scError = SymCryptIntSetValue( pbScratch, nBytesPriv, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, pkDlkey->piPrivateKey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    // Calculate the public key
    SymCryptModExp(
        pDlgroup->pmP,
        pDlgroup->peG,
        pkDlkey->piPrivateKey,
        nBitsPriv,
        0,      // Side-channel safe
        pkDlkey->pePublicKey,
        pbScratch, // We can overwrite pePrivateKey now
        cbScratch );

    // Perform range validation on generated Public key.
    if ( (flags & SYMCRYPT_FLAG_KEY_NO_FIPS) == 0 )
    {
        // Perform Public key validation.
        // Always perform range validation, and validation that Public key is in subgroup of order Q
        scError = SymCryptDlkeyPerformPublicKeyValidation(
            pkDlkey,
            SYMCRYPT_FLAG_DLKEY_PUBLIC_KEY_ORDER_VALIDATION,
            pbScratch,
            cbScratch );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    // Set the fHasPrivateKey flag
    pkDlkey->fHasPrivateKey = TRUE;

    pkDlkey->fAlgorithmInfo = flags; // We want to track all of the flags in the Dlkey

    if ( (flags & SYMCRYPT_FLAG_KEY_NO_FIPS) == 0 )
    {
        if( ( flags & SYMCRYPT_FLAG_DLKEY_DSA ) != 0 )
        {
            // Ensure DSA algorithm selftest is run before first use of DSA algorithm
            SYMCRYPT_RUN_SELFTEST_ONCE(
                SymCryptDsaSelftest,
                SYMCRYPT_SELFTEST_ALGORITHM_DSA );

            // Run PCT eagerly as the key can only be used for DSA - there is no value in deferring
            SYMCRYPT_RUN_KEY_GEN_PCT(
                SymCryptDsaPct,
                pkDlkey,
                SYMCRYPT_PCT_DSA );
        }

        if( ( flags & SYMCRYPT_FLAG_DLKEY_DH ) != 0 )
        {
            // Ensure we have run the algorithm selftest at least once.
            SYMCRYPT_RUN_SELFTEST_ONCE(
                SymCryptDhSecretAgreementSelftest,
                SYMCRYPT_SELFTEST_ALGORITHM_DH );

            // Run PCT eagerly as the key can only be used for DH

            // DH PCT per SP80056a-rev3 5.6.2.1.4 b)
            // Recompute the public key from the private key
            // Option a) appears to be explicitly overruled by 140-3 IG

            // Calculate the public key from the private key in scratch
            pbScratchInternal = pbScratch;
            cbScratchInternal = cbScratch;

            peTmp = SymCryptModElementCreate( pbScratchInternal, cbModElement, pDlgroup->pmP );
            pbScratchInternal += cbModElement;
            cbScratchInternal -= cbModElement;

            SymCryptModExp(
                    pDlgroup->pmP,
                    pDlgroup->peG,
                    pkDlkey->piPrivateKey,
                    nBitsPriv,  // This is either bits of P, Q, or some caller-defined value i.e. public values
                    0,          // Side-channel safe
                    peTmp,
                    pbScratchInternal,
                    cbScratchInternal );

            SYMCRYPT_FIPS_ASSERT( SymCryptModElementIsEqual(pDlgroup->pmP, peTmp, pkDlkey->pePublicKey) );
        }
    }

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeySetValue(
    _In_reads_bytes_( cbPrivateKey )    PCBYTE                  pbPrivateKey,
                                        SIZE_T                  cbPrivateKey,
    _In_reads_bytes_( cbPublicKey )     PCBYTE                  pbPublicKey,
                                        SIZE_T                  cbPublicKey,
                                        SYMCRYPT_NUMBER_FORMAT  numFormat,
                                        UINT32                  flags,
    _Inout_                             PSYMCRYPT_DLKEY         pkDlkey )
{
    SYMCRYPT_ERROR      scError = SYMCRYPT_NO_ERROR;
    PBYTE               pbScratch = NULL;
    UINT32              cbScratch = 0;
    PBYTE               pbScratchInternal = NULL;
    UINT32              cbScratchInternal = 0;

    PCSYMCRYPT_DLGROUP pDlgroup = pkDlkey->pDlgroup;

    UINT32 nDigitsPriv = 0;
    UINT32 nBitsPriv = 0;

    PSYMCRYPT_MODELEMENT peTmp = NULL;
    UINT32 cbModElement = SymCryptSizeofModElementFromModulus( pDlgroup->pmP );
    UINT32 fValidatePublicKeyOrder = SYMCRYPT_FLAG_DLKEY_PUBLIC_KEY_ORDER_VALIDATION;

    if ( ((pbPrivateKey==NULL) && (cbPrivateKey!=0)) ||
         ((pbPublicKey==NULL) && (cbPublicKey!=0)) ||
         ((pbPrivateKey==NULL) && (pbPublicKey==NULL)) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Ensure caller has specified what algorithm(s) the key will be used with
    UINT32 algorithmFlags = SYMCRYPT_FLAG_DLKEY_DSA | SYMCRYPT_FLAG_DLKEY_DH;
    // Make sure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION | algorithmFlags;

    if ( ( ( flags & ~allowedFlags ) != 0 ) ||
         ( ( flags & algorithmFlags ) == 0 ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Extra sanity checks when running with FIPS
    // Either Dlgroup is named SafePrime group and key is for DH,
    // or Dlgroup is not named SafePrime group and key is for DSA
    if ( ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 ) &&
         ( (pDlgroup->isSafePrimeGroup && (flags & SYMCRYPT_FLAG_DLKEY_DSA)) ||
           (!(pDlgroup->isSafePrimeGroup) && (flags & SYMCRYPT_FLAG_DLKEY_DH)) ) )
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

    if ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) != 0 )
    {
        fValidatePublicKeyOrder = 0;
    }

    //
    // From symcrypt_internal.h we have:
    //      - sizeof results are upper bounded by 2^19
    //      - SYMCRYPT_SCRATCH_BYTES results are upper bounded by 2^27 (including RSA and ECURVE)
    // Thus the following calculation does not overflow cbScratch.
    //
    cbScratch = SYMCRYPT_MAX( cbModElement + SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS(pDlgroup->nDigitsOfP),
                     (2 * cbModElement) + SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP(pDlgroup->nDigitsOfP) );
    pbScratch = SymCryptCallbackAlloc( cbScratch );
    if (pbScratch == NULL)
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    if ( pbPrivateKey != NULL )
    {
        //
        // Check the size of the imported private key to detect if it is mod P or mod Q
        // If the group does not have a Q assume that the imported key is modulo P as
        // it wouldn't help us assume otherwise (the bitsize of the private key should be kept
        // secret from SC attacks).
        // If the private key has had some non-default value set for nBitsPriv then the caller
        // has explicitly opted in to more stringent range checking.
        //
        pkDlkey->fPrivateModQ = ( (pDlgroup->fHasPrimeQ) &&
            ((cbPrivateKey < pDlgroup->cbPrimeQ) ||
            ((cbPrivateKey == pDlgroup->cbPrimeQ) && (pDlgroup->cbPrimeQ < pDlgroup->cbPrimeP)) ||
            (pkDlkey->nBitsPriv != pDlgroup->nDefaultBitsPriv)) );

        if ( pkDlkey->fPrivateModQ )
        {
            nDigitsPriv = pDlgroup->nDigitsOfQ;
            nBitsPriv = pDlgroup->nBitsOfQ;

            if ( pDlgroup->isSafePrimeGroup )
            {
                nBitsPriv = pkDlkey->nBitsPriv;
            }
        }
        else
        {
            nDigitsPriv = pDlgroup->nDigitsOfP;
            nBitsPriv = pDlgroup->nBitsOfP;
        }

        pkDlkey->piPrivateKey = SymCryptIntCreate( pkDlkey->pbPrivate, SymCryptSizeofIntFromDigits(nDigitsPriv), nDigitsPriv );

        scError = SymCryptIntSetValue(
                        pbPrivateKey,
                        cbPrivateKey,
                        numFormat,
                        pkDlkey->piPrivateKey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Perform range validation on imported Private key.
        // Check if Private key is 0 - perform unconditionally as it is cheap
        // and it never makes sense for private key to be 0 intentionally
        if ( SymCryptIntIsEqualUint32( pkDlkey->piPrivateKey, 0 ) )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        // Continue range validation on imported Private key.
        if ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 )
        {
            // Ensure that Q is specified in the Dlgroup
            if ( !pDlgroup->fHasPrimeQ )
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }

            // If nBitsPriv is specified, check if Private key is greater than or equal to 2^nBitsPriv
            // Otherwise, check if Private key is greater than or equal to Q
            if ( ( ( (nBitsPriv <  pDlgroup->nBitsOfQ) &&
                    SymCryptIntBitsizeOfValue( pkDlkey->piPrivateKey ) > nBitsPriv ) ) ||
                   ( (nBitsPriv >= pDlgroup->nBitsOfQ) &&
                    !SymCryptIntIsLessThan( pkDlkey->piPrivateKey, SymCryptIntFromModulus( pDlgroup->pmQ ) ) ) )
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }
        }

        pkDlkey->fHasPrivateKey = TRUE;
    }

    if ( pbPublicKey != NULL )
    {
        scError = SymCryptModElementSetValue(
                        pbPublicKey,
                        cbPublicKey,
                        numFormat,
                        pDlgroup->pmP,
                        pkDlkey->pePublicKey,
                        pbScratch,
                        cbScratch );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Perform range validation on imported Public key.
        if ( (flags & SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION) == 0 )
        {
            // Perform Public key validation.
            // Always perform range validation
            // May also perform validation that Public key is in subgroup of order Q, depending on flags
            scError = SymCryptDlkeyPerformPublicKeyValidation(
                pkDlkey,
                fValidatePublicKeyOrder,
                pbScratch,
                cbScratch );
            if ( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }
        }
    }

    // Calculating the public key if no key was provided
    // or if needed for keypair regeneration validation
    if ( (pbPublicKey==NULL) ||
         ( ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 ) &&
          (pbPrivateKey!=NULL) && (pbPublicKey!=NULL) ) )
    {
        // Calculate the public key from the private key
        pbScratchInternal = pbScratch;
        cbScratchInternal = cbScratch;

        // By default calculate the public key directly where it will be persisted
        peTmp = pkDlkey->pePublicKey;

        if ( pbPublicKey != NULL )
        {
            // If doing regeneration validation calculate the public key in scratch
            peTmp = SymCryptModElementCreate( pbScratchInternal, cbModElement, pDlgroup->pmP);
            pbScratchInternal += cbModElement;
            cbScratchInternal -= cbModElement;
        }

        SymCryptModExp(
                pDlgroup->pmP,
                pDlgroup->peG,
                pkDlkey->piPrivateKey,
                nBitsPriv,  // This is either bits of P, Q, or some caller-defined value i.e. public values
                0,          // Side-channel safe
                peTmp,
                pbScratchInternal,
                cbScratchInternal );

        if ( pbPublicKey != NULL )
        {
            if ( !SymCryptModElementIsEqual(pDlgroup->pmP, peTmp, pkDlkey->pePublicKey) )
            {
                scError = SYMCRYPT_AUTHENTICATION_FAILURE;
                goto cleanup;
            }
        }
        else if ( ( flags & SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION ) == 0 )
        {
            // Perform Public key validation on generated public key.
            // Always perform range validation
            // May also perform validation that Public key is in subgroup of order Q, depending on flags
            scError = SymCryptDlkeyPerformPublicKeyValidation(
                pkDlkey,
                fValidatePublicKeyOrder,
                pbScratch,
                cbScratch );
            if ( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }
        }
    }

    pkDlkey->fAlgorithmInfo = flags; // We want to track all of the flags in the Dlkey

    if ( (flags & SYMCRYPT_FLAG_KEY_NO_FIPS) == 0 )
    {
        if( ( flags & SYMCRYPT_FLAG_DLKEY_DSA ) != 0 )
        {
            // Ensure DSA algorithm selftest is run before first use of DSA algorithm
            SYMCRYPT_RUN_SELFTEST_ONCE(
                SymCryptDsaSelftest,
                SYMCRYPT_SELFTEST_ALGORITHM_DSA );

            // PCT does not need to be run on import - mark it as done
            pkDlkey->fAlgorithmInfo |= SYMCRYPT_PCT_DSA;
        }

        if( ( flags & SYMCRYPT_FLAG_DLKEY_DH ) != 0 )
        {
            SYMCRYPT_RUN_SELFTEST_ONCE(
                SymCryptDhSecretAgreementSelftest,
                SYMCRYPT_SELFTEST_ALGORITHM_DH );
        }
    }

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeyGetValue(
    _In_    PCSYMCRYPT_DLKEY        pkDlkey,
    _Out_writes_bytes_( cbPrivateKey )
            PBYTE                   pbPrivateKey,
            SIZE_T                  cbPrivateKey,
    _Out_writes_bytes_( cbPublicKey )
            PBYTE                   pbPublicKey,
            SIZE_T                  cbPublicKey,
            SYMCRYPT_NUMBER_FORMAT  numFormat,
            UINT32                  flags )
{
    SYMCRYPT_ERROR      scError = SYMCRYPT_NO_ERROR;
    PBYTE               pbScratch = NULL;
    UINT32              cbScratch = 0;

    PCSYMCRYPT_DLGROUP pDlgroup = pkDlkey->pDlgroup;

    UNREFERENCED_PARAMETER( flags );

    if ( ((pbPrivateKey==NULL) && (cbPrivateKey!=0)) ||
         ((pbPublicKey==NULL) && (cbPublicKey!=0)) ||
         ((pbPrivateKey==NULL) && (pbPublicKey==NULL)) ||
         ((pbPrivateKey!=NULL) && !pkDlkey->fHasPrivateKey) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (pbPrivateKey != NULL)
    {
        scError = SymCryptIntGetValue(
                        pkDlkey->piPrivateKey,
                        pbPrivateKey,
                        cbPrivateKey,
                        numFormat );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }

    if (pbPublicKey != NULL)
    {
        cbScratch = SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS(pDlgroup->nDigitsOfP);
        pbScratch = SymCryptCallbackAlloc( cbScratch );
        if (pbScratch == NULL)
        {
            scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }

        scError = SymCryptModElementGetValue(
                        pDlgroup->pmP,
                        pkDlkey->pePublicKey,
                        pbPublicKey,
                        cbPublicKey,
                        numFormat,
                        pbScratch,
                        cbScratch );
        if (scError!=SYMCRYPT_NO_ERROR)
        {
            goto cleanup;
        }
    }

cleanup:
    if (pbScratch!=NULL)
    {
        SymCryptWipe( pbScratch, cbScratch );
        SymCryptCallbackFree( pbScratch );
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeyExtendKeyUsage(
    _Inout_ PSYMCRYPT_DLKEY pkDlkey,
            UINT32          flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    // Ensure caller has specified what algorithm(s) the key will be used with
    UINT32 algorithmFlags = SYMCRYPT_FLAG_DLKEY_DSA | SYMCRYPT_FLAG_DLKEY_DH;

    if ( ( ( flags & ~algorithmFlags ) != 0 ) ||
         ( ( flags & algorithmFlags ) == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pkDlkey->fAlgorithmInfo |= flags;

cleanup:
    return scError;
}
