//
// mldsa.c   ML-DSA related functionality
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

_Use_decl_annotations_
PSYMCRYPT_MLDSAKEY
SYMCRYPT_CALL
SymCryptMlDsakeyAllocate(
    SYMCRYPT_MLDSA_PARAMS params )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PSYMCRYPT_MLDSAKEY pkMlDsakey = NULL;
    PSYMCRYPT_MLDSA_INTERNAL_PARAMS pInternalParams = NULL;
    PBYTE pbKey = NULL;
    UINT32 cbKey = 0;

    scError = SymCryptMlDsaGetInternalParamsFromParams( params, &pInternalParams );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    cbKey = SYMCRYPT_INTERNAL_MLDSA_SIZEOF_KEY( pInternalParams->nRows, pInternalParams->nCols );
    pbKey = SymCryptCallbackAlloc( cbKey );
    if( pbKey == NULL )
    {
        goto cleanup;
    }

    pkMlDsakey = SymCryptMlDsakeyInitialize( pInternalParams, pbKey, cbKey );
    if( pkMlDsakey == NULL )
    {
        goto cleanup;
    }

    // On success, memory is owned by pkMlDsakey
    pbKey = NULL;

cleanup:
    if( pbKey != NULL )
    {
        SymCryptCallbackFree( pbKey );
    }

    return pkMlDsakey;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptMlDsakeyFree(
    PSYMCRYPT_MLDSAKEY pkMlDsakey )
{
    SYMCRYPT_CHECK_MAGIC( pkMlDsakey );

    SymCryptWipe( pkMlDsakey, pkMlDsakey->cbTotalSize );
    SymCryptCallbackFree( pkMlDsakey );
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaKeyGenerateEx(
    PSYMCRYPT_MLDSAKEY  pkMlDsakey,
    PCBYTE              pbRootSeed,
    SIZE_T              cbRootSeed,
    UINT32              flags )
{
    UNREFERENCED_PARAMETER( flags );

    SYMCRYPT_ASSERT( cbRootSeed == SYMCRYPT_MLDSA_ROOT_SEED_SIZE );

    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pParams = pkMlDsakey->pParams;
    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemps = NULL;

    BYTE privateVectorSeed[SYMCRYPT_MLDSA_PRIVATE_VECTOR_SEED_SIZE];

    pTemps = SymCryptMlDsaTemporariesAllocateAndInitialize(
        pParams,
        1, // row vectors
        0, // column vectors
        1, // poly elements
        pParams->cbEncodedPublicKey ); // scratch space
    if( pTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    memcpy( pkMlDsakey->rootSeed, pbRootSeed, cbRootSeed );

    {
        PSYMCRYPT_SHAKE256_STATE pShakeState = &(pTemps->shake256State);
        SymCryptShake256Init( pShakeState );
        SymCryptShake256Append( pShakeState, pkMlDsakey->rootSeed, cbRootSeed );
        SymCryptShake256Append( pShakeState, (PCBYTE) &pParams->nRows, sizeof(BYTE) );
        SymCryptShake256Append( pShakeState, (PCBYTE) &pParams->nCols, sizeof(BYTE) );

        SymCryptShake256Extract( pShakeState, pkMlDsakey->publicSeed, sizeof(pkMlDsakey->publicSeed), FALSE);
        SymCryptShake256Extract( pShakeState, privateVectorSeed, sizeof(privateVectorSeed), FALSE );
        SymCryptShake256Extract( pShakeState, pkMlDsakey->privateSigningSeed, sizeof(pkMlDsakey->privateSigningSeed), FALSE); // Wiped when pTemps is freed
    }

    SymCryptMlDsaExpandA( pkMlDsakey->publicSeed, sizeof(pkMlDsakey->publicSeed), pkMlDsakey->pmA );

    SymCryptMlDsaExpandS(
        pkMlDsakey->pParams,
        privateVectorSeed,
        sizeof(privateVectorSeed),
        pkMlDsakey->pvs1,
        pkMlDsakey->pvs2 );

    // Convert s1 and s2 to NTT form
    SymCryptMlDsaVectorNTT( pkMlDsakey->pvs1 );
    SymCryptMlDsaVectorNTT( pkMlDsakey->pvs2 );

    SymCryptMlDsakeyComputeT(
        pkMlDsakey->pmA,
        pkMlDsakey->pvs1,
        pkMlDsakey->pvs2,
        pkMlDsakey->pvt0,
        pkMlDsakey->pvt1,
        pTemps->pvRowVectors[0],
        pTemps->pePolyElements[0] );

    // Convert t0 and t1 to NTT form
    SymCryptMlDsaVectorNTT( pkMlDsakey->pvt0 );
    SymCryptMlDsaVectorNTT( pkMlDsakey->pvt1 );

    scError = SymCryptMlDsaPkEncode( pkMlDsakey, pTemps->pbScratch, pParams->cbEncodedPublicKey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptShake256(
        pTemps->pbScratch,
        pParams->cbEncodedPublicKey,
        pkMlDsakey->publicKeyHash,
        sizeof(pkMlDsakey->publicKeyHash) );

    pkMlDsakey->hasRootSeed = TRUE;
    pkMlDsakey->hasPrivateKey = TRUE;

cleanup:
    if( pTemps != NULL )
    {
        SymCryptMlDsaTemporariesFree( pTemps );
    }

    SymCryptWipeKnownSize( privateVectorSeed, sizeof(privateVectorSeed) );

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsakeyGenerate(
    PSYMCRYPT_MLDSAKEY  pkMlDsakey,
    UINT32              flags)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    BYTE random[SYMCRYPT_MLDSA_ROOT_SEED_SIZE];
    PBYTE  pbPctSignature = NULL;
    SIZE_T cbPctSignature = 0;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS;

    if ( ( flags & ~allowedFlags ) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptCallbackRandom( random, sizeof(random) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptMlDsakeySetValue( random, sizeof(random), SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_SEED, flags, pkMlDsakey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // SymCryptMlDsakeySetValue ensures the self-test is run before
    // first operational use of MlDsa

    if( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 )
    {
        // PCT on key generation, sign/verify the empty message with the generated key

        cbPctSignature = pkMlDsakey->pParams->cbEncodedSignature;

        pbPctSignature = SymCryptCallbackAlloc( cbPctSignature );
        if( pbPctSignature == NULL )
        {
            scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }

        scError = SymCryptMlDsaSign( pkMlDsakey,
            NULL, 0,
            NULL, 0,
            0,
            pbPctSignature, cbPctSignature );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            scError = SYMCRYPT_FIPS_FAILURE;
            goto cleanup;
        }

        scError = SymCryptMlDsaVerify( pkMlDsakey,
            NULL, 0,
            NULL, 0,
            pbPctSignature, cbPctSignature,
            0 );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            scError = SYMCRYPT_FIPS_FAILURE;
            goto cleanup;
        }

        // could track having run the PCT with a flag in pkMlDsakey->fAlgorithmInfo,
        // but currently no need to do that given we don't ever defer the PCT
    }

cleanup:
    if( pbPctSignature != NULL )
    {
        // Wiping is not required for security, but has low relative cost
        // and better to be on the safe side for FIPS
        SymCryptWipe( pbPctSignature, cbPctSignature );
        SymCryptCallbackFree( pbPctSignature );
    }

    SymCryptWipeKnownSize( random, sizeof(random) );

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsakeySetValue(
    PCBYTE                      pbSrc,
    SIZE_T                      cbSrc,
    SYMCRYPT_MLDSAKEY_FORMAT    mlDsakeyFormat,
    UINT32                      flags,
    PSYMCRYPT_MLDSAKEY          pkMlDsakey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS;

    if ( ( flags & ~allowedFlags ) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 )
    {
        // Ensure ML-DSA algorithm selftest is run before first use of ML-DSA algorithms;
        // notably _before_ first full KeyGen
        SYMCRYPT_RUN_SELFTEST_ONCE(
            SymCryptMlDsaSelftest,
            SYMCRYPT_SELFTEST_ALGORITHM_MLDSA);
    }

    switch( mlDsakeyFormat )
    {
        case SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_SEED:
            if( cbSrc != SYMCRYPT_MLDSA_ROOT_SEED_SIZE )
            {
                scError = SYMCRYPT_WRONG_KEY_SIZE;
                goto cleanup;
            }

            scError = SymCryptMlDsaKeyGenerateEx( pkMlDsakey, pbSrc, cbSrc, flags );
            break;
        case SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_KEY:
            scError = SymCryptMlDsaSkDecode( pbSrc, cbSrc, flags, pkMlDsakey );
            break;
        case SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY:
            scError = SymCryptMlDsaPkDecode( pbSrc, cbSrc, flags, pkMlDsakey );
            break;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            break;
    }

cleanup:
    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsakeyGetValue(
    PCSYMCRYPT_MLDSAKEY         pkMlDsakey,
    PBYTE                       pbDst,
    SIZE_T                      cbDst,
    SYMCRYPT_MLDSAKEY_FORMAT    mlDsakeyFormat,
    UINT32                      flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if( flags != 0 ) // No flags currently supported
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    switch( mlDsakeyFormat )
    {
        case SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_KEY:
            scError = SymCryptMlDsaSkEncode(
                pkMlDsakey,
                pbDst,
                cbDst );
            break;
        case SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY:
            scError = SymCryptMlDsaPkEncode(
                pkMlDsakey,
                pbDst,
                cbDst );
            break;
        case SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_SEED:
            if( cbDst < SYMCRYPT_MLDSA_ROOT_SEED_SIZE )
            {
                scError = SYMCRYPT_BUFFER_TOO_SMALL;
                goto cleanup;
            }

            if( !pkMlDsakey->hasRootSeed )
            {
                scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
                goto cleanup;
            }

            memcpy( pbDst, pkMlDsakey->rootSeed, SYMCRYPT_MLDSA_ROOT_SEED_SIZE );

            break;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            break;
    }

cleanup:
    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSignEx(
    PCSYMCRYPT_MLDSAKEY     pkMlDsakey,
    PCBYTE                  pbInput,
    SIZE_T                  cbInput,
    PCBYTE                  pbContext,
    SIZE_T                  cbContext,
    PCBYTE                  pbHashOid,
    SIZE_T                  cbHashOid,
    PCBYTE                  pbRandom,
    SIZE_T                  cbRandom,
    UINT32                  flags,
    PBYTE                   pbSignature,
    SIZE_T                  cbSignature )
{
    SYMCRYPT_ASSERT( pkMlDsakey->hasPrivateKey == TRUE );
    SYMCRYPT_ASSERT( cbContext <= SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH );
    SYMCRYPT_ASSERT( cbRandom == SYMCRYPT_MLDSA_SIGNING_RANDOM_SIZE );
    SYMCRYPT_ASSERT( pbHashOid != NULL || cbHashOid == 0 );
    SYMCRYPT_ASSERT( pbContext != NULL || cbContext == 0 );
    SYMCRYPT_ASSERT( cbSignature == pkMlDsakey->pParams->cbEncodedSignature );
    SYMCRYPT_ASSERT( (flags & ~SYMCRYPT_FLAG_MLDSA_EXTERNALMU) == 0 );
    SYMCRYPT_ASSERT( ((flags & SYMCRYPT_FLAG_MLDSA_EXTERNALMU) == 0) || (pbContext == NULL && pbHashOid == NULL) );

    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pParams = pkMlDsakey->pParams;
    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemps = NULL;

    const UINT32 beta = (UINT32) pParams->nChallengeNonZeroCoeffs * pParams->privateKeyRange;

    BOOL bExternalMu = (flags & SYMCRYPT_FLAG_MLDSA_EXTERNALMU) != 0;
    UINT8 modeId = (pbHashOid == NULL) ? 0 : 1; // 0 for ML-DSA, 1 for HashML-DSA
    UINT8 cbContextByte = (UINT8) cbContext;
    BYTE messageRepresentative[SYMCRYPT_SHAKE256_RESULT_SIZE];
    BYTE privateRandom[SYMCRYPT_SHAKE256_RESULT_SIZE];
    BYTE commitmentHash[64]; // Largest possible size for commitment hash

    const UINT32 cbw1Encoded = pParams->nRows * pParams->w1EncodeCoefficientBitLength *
        ( SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8 );

    pTemps = SymCryptMlDsaTemporariesAllocateAndInitialize(
        pParams,
        2,              // row vectors - W, W1, cs2, ct0, r0, hint (not all needed simultaneously)
        3,              // column vectors - mask, cs1, response
        1,              // poly element - challenge
        cbw1Encoded );  // scratch space - w1 encoded
    if( pTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    PSYMCRYPT_SHAKE256_STATE pShakeState = &(pTemps->shake256State);
    SymCryptShake256Init( pShakeState );

    if ( bExternalMu )
    {
        // Caller passes the externally-computed message representative mu
        SYMCRYPT_ASSERT( cbInput == SYMCRYPT_SHAKE256_RESULT_SIZE );
        memcpy( messageRepresentative, pbInput, SYMCRYPT_SHAKE256_RESULT_SIZE );
    }
    else
    {
        // Line 6: calculate message representative mu
        // = SHAKE256( public key hash || modeId || cbContextByte || context || OID? || message/hash, 64 )
        // The OID is only included in the HashML-DSA mode
        SymCryptShake256Append( pShakeState, pkMlDsakey->publicKeyHash, sizeof(pkMlDsakey->publicKeyHash) );
        SymCryptShake256Append( pShakeState, &modeId, sizeof( modeId ) );
        SymCryptShake256Append( pShakeState, &cbContextByte, sizeof( cbContextByte ) );

        // These appends are no-ops if the length is zero
        SymCryptShake256Append( pShakeState, pbContext, cbContext );
        SymCryptShake256Append( pShakeState, pbHashOid, cbHashOid );

        SymCryptShake256Append( pShakeState, pbInput, cbInput );
        SymCryptShake256Result( pShakeState, messageRepresentative );
    }

    // Line 7: Calculate private random seed rho prime prime
    // = SHAKE256( private signing seed K || pbRandom || message representative mu, 64 )
    SymCryptShake256Append( pShakeState, pkMlDsakey->privateSigningSeed, sizeof(pkMlDsakey->privateSigningSeed) );
    SymCryptShake256Append( pShakeState, pbRandom, cbRandom );
    SymCryptShake256Append( pShakeState, messageRepresentative, sizeof(messageRepresentative) );
    SymCryptShake256Result( pShakeState, privateRandom );

    PSYMCRYPT_MLDSA_VECTOR pvW = pTemps->pvRowVectors[0];
    PSYMCRYPT_MLDSA_VECTOR pvHint = NULL;

    PSYMCRYPT_MLDSA_VECTOR pvMask = pTemps->pvColVectors[0];
    PSYMCRYPT_MLDSA_VECTOR pvResponse = pTemps->pvColVectors[1];
    PSYMCRYPT_MLDSA_VECTOR pvcs1 = pTemps->pvColVectors[2];

    PBYTE pbW1Encoded = pTemps->pbScratch;

    UINT16 k = 0;
    while( TRUE )
    {
        SymCryptMlDsaExpandMask(
            pParams,
            pShakeState,
            privateRandom,
            sizeof(privateRandom),
            k,
            pvMask );

        // Increment k early so we can continue to the next loop iteration when validity checks fail
        // It's okay to leak how many iterations this loop takes because the SHAKE inputs and
        // outputs are still unpredictable; this does not leak information about the private key
        k += (UINT16) pParams->nCols;

        SymCryptMlDsaMatrixVectorMontMul( pkMlDsakey->pmA, pvMask, pvW, pTemps->pePolyElements[0] );

        for(UINT8 i = 0; i < pvW->nElems; ++i)
        {
            SymCryptMlDsaPolyElementMulR( SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT(i, pvW) );
        }

        SymCryptMlDsaVectorINTT( pvW );

        {
            // Scope for pvW1
            PSYMCRYPT_MLDSA_VECTOR pvW1 = pTemps->pvRowVectors[1];
            SymCryptMlDsaVectorHighBits( pParams, pvW, pvW1 );
            SymCryptMlDsaVectorEncode( pvW1, pParams->w1EncodeCoefficientBitLength, 0, pbW1Encoded );
        }

        // Calculate commitment hash
        SymCryptShake256Append( pShakeState, messageRepresentative, sizeof(messageRepresentative) );
        SymCryptShake256Append( pShakeState, pbW1Encoded, cbw1Encoded );
        SymCryptShake256Extract( pShakeState, commitmentHash, pParams->cbCommitmentHash, TRUE );

        // Calculate challenge
        // Reusing poly element 0 for challenge (previously temp space for multiplication)
        PSYMCRYPT_MLDSA_POLYELEMENT peC = pTemps->pePolyElements[0];
        SymCryptMlDsaSampleInBall( pParams, commitmentHash, pParams->cbCommitmentHash, peC );

        SymCryptMlDsaPolyElementNTT( peC );
        SymCryptMlDsaPolyElementMulR( peC );

        {
            // Scope for cs2 - reusing row vector 1, previously W1
            PSYMCRYPT_MLDSA_VECTOR pvcs2 = pTemps->pvRowVectors[1];
            SymCryptMlDsaVectorPolyElementMontMul( pkMlDsakey->pvs1, peC, pvcs1 );
            SymCryptMlDsaVectorPolyElementMontMul( pkMlDsakey->pvs2, peC, pvcs2 );

            SymCryptMlDsaVectorINTT( pvcs1 );
            SymCryptMlDsaVectorINTT( pvcs2 );

            SymCryptMlDsaVectorINTT( pvMask );
            SymCryptMlDsaVectorAdd( pvMask, pvcs1, pvResponse );

            // (w - cs2) is an input to both LowBits (for r0) and MakeHint
            SymCryptMlDsaVectorSub( pvW, pvcs2, pvW );
        }

        {
            // Scope for r0 - reusing row vector 1, previously cs2
            PSYMCRYPT_MLDSA_VECTOR pvr0 = pTemps->pvRowVectors[1];
            SymCryptMlDsaVectorLowBits( pParams, pvW, pvr0 );

            UINT32 zInfinityNorm = SymCryptMlDsaVectorInfinityNorm( pvResponse );
            UINT32 r0InfinityNorm = SymCryptMlDsaVectorInfinityNorm( pvr0 );

            if( (zInfinityNorm >= (1 << pParams->maskCoefficientRangeLog2) - beta) ||
                (r0InfinityNorm >= pParams->commitmentRoundingRange - beta) )
            {
                continue;
            }
        }

        {
            // Scope for ct0 - reusing row vector 1, previously r0
            PSYMCRYPT_MLDSA_VECTOR pvct0 = pTemps->pvRowVectors[1];

            SymCryptMlDsaVectorPolyElementMontMul( pkMlDsakey->pvt0, peC, pvct0 );
            SymCryptMlDsaVectorINTT( pvct0 );

            UINT32 ct0InfinityNorm = SymCryptMlDsaVectorInfinityNorm( pvct0 );
            if( ct0InfinityNorm >= pParams->commitmentRoundingRange )
            {
                continue;
            }

            // MakeHint vectors
            // w - cs2 = pvW
            // w - cs2 + ct0 = pvct0 + pvW
            SymCryptMlDsaVectorAdd( pvct0, pvW, pvct0 );

            // Write hint in-place over ct0
            UINT32 nHintBitsSet = 0;
            SymCryptMlDsaMakeHint( pParams, pvW, pvct0, pvct0, &nHintBitsSet );

            if( nHintBitsSet > pParams->nHintNonZeroCoeffs )
            {
                continue;
            }
        }

        // Row vector 1, previously ct0, now contains the hint
        pvHint = pTemps->pvRowVectors[1];

        break;
    }

    SYMCRYPT_ASSERT( pvHint != NULL );

    SymCryptMlDsaSigEncode(
        pParams,
        commitmentHash,
        pParams->cbCommitmentHash,
        pvResponse,
        pvHint,
        pbSignature,
        cbSignature );

cleanup:
    if( pTemps != NULL )
    {
        SymCryptMlDsaTemporariesFree( pTemps );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSign(
    PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    PCBYTE              pbMessage,
    SIZE_T              cbMessage,
    PCBYTE              pbContext,
    SIZE_T              cbContext,
    UINT32              flags,
    PBYTE               pbSignature,
    SIZE_T              cbSignature )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if( (flags != 0) || // No flags currently supported
        (cbContext > SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH) ||
        (pkMlDsakey->hasPrivateKey == FALSE) ||
        (cbSignature != pkMlDsakey->pParams->cbEncodedSignature) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    BYTE random[SYMCRYPT_MLDSA_SIGNING_RANDOM_SIZE];
    scError = SymCryptCallbackRandom( random, sizeof(random) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptMlDsaSignEx(
        pkMlDsakey,
        pbMessage,
        cbMessage,
        pbContext,
        cbContext,
        NULL, // pbHashOid
        0, // cbHashOid
        random,
        sizeof(random),
        flags,
        pbSignature,
        cbSignature );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    SymCryptWipeKnownSize( random, sizeof(random) );

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptExternalMuMlDsaSign(
    PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    PCBYTE              pbMu,
    SIZE_T              cbMu,
    UINT32              flags,
    PBYTE               pbSignature,
    SIZE_T              cbSignature )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if( (flags != 0) || // No flags currently supported
        (pkMlDsakey->hasPrivateKey == FALSE) ||
        (cbMu != SYMCRYPT_SHAKE256_RESULT_SIZE) ||
        (cbSignature != pkMlDsakey->pParams->cbEncodedSignature) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    BYTE random[SYMCRYPT_MLDSA_SIGNING_RANDOM_SIZE];
    scError = SymCryptCallbackRandom( random, sizeof(random) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptMlDsaSignEx(
        pkMlDsakey,
        pbMu,
        cbMu,
        NULL, // pbContext
        0, // cbContext
        NULL, // pbHashOid
        0, // cbHashOid
        random,
        sizeof(random),
        SYMCRYPT_FLAG_MLDSA_EXTERNALMU,
        pbSignature,
        cbSignature );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    SymCryptWipeKnownSize( random, sizeof(random) );

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHashMlDsaSign(
    PCSYMCRYPT_MLDSAKEY     pkMlDsakey,
    SYMCRYPT_PQDSA_HASH_ID  hashAlg,
    PCBYTE                  pbHash,
    SIZE_T                  cbHash,
    PCBYTE                  pbContext,
    SIZE_T                  cbContext,
    UINT32                  flags,
    PBYTE                   pbSignature,
    SIZE_T                  cbSignature )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_OID pHashOid = NULL;

    if( (flags != 0) || // No flags currently supported
        (cbContext > SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH) ||
        (pkMlDsakey->hasPrivateKey == FALSE) ||
        (cbSignature != pkMlDsakey->pParams->cbEncodedSignature) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    BYTE random[SYMCRYPT_MLDSA_SIGNING_RANDOM_SIZE];
    scError = SymCryptCallbackRandom( random, sizeof(random) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptHashMlDsaValidateHashAlgAndGetOid(
        pkMlDsakey->pParams,
        hashAlg,
        cbHash,
        &pHashOid );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptMlDsaSignEx(
        pkMlDsakey,
        pbHash,
        cbHash,
        pbContext,
        cbContext,
        pHashOid->pbOID,
        pHashOid->cbOID,
        random,
        sizeof(random),
        flags,
        pbSignature,
        cbSignature );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }


cleanup:
    SymCryptWipeKnownSize( random, sizeof(random) );

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaVerifyEx(
    PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    PCBYTE              pbInput,
    SIZE_T              cbInput,
    PCBYTE              pbContext,
    SIZE_T              cbContext,
    PCBYTE              pbHashOid,
    SIZE_T              cbHashOid,
    PCBYTE              pbSignature,
    SIZE_T              cbSignature,
    UINT32              flags )
{
    UNREFERENCED_PARAMETER( flags );

    SYMCRYPT_ASSERT( cbContext <= SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH );
    SYMCRYPT_ASSERT( pbHashOid != NULL || cbHashOid == 0 );
    SYMCRYPT_ASSERT( pbContext != NULL || cbContext == 0 );
    SYMCRYPT_ASSERT( cbSignature == pkMlDsakey->pParams->cbEncodedSignature );
    SYMCRYPT_ASSERT( (flags & ~SYMCRYPT_FLAG_MLDSA_EXTERNALMU) == 0 );
    SYMCRYPT_ASSERT( ((flags & SYMCRYPT_FLAG_MLDSA_EXTERNALMU) == 0) || (pbContext == NULL && pbHashOid == NULL) );

    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pParams = pkMlDsakey->pParams;
    PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemps = NULL;

    const UINT32 beta = (UINT32) pParams->nChallengeNonZeroCoeffs * pParams->privateKeyRange;

    BOOL bExternalMu = (flags & SYMCRYPT_FLAG_MLDSA_EXTERNALMU) != 0;
    UINT8 modeId = (pbHashOid == NULL) ? 0 : 1; // 0 for ML-DSA, 1 for HashML-DSA
    UINT8 cbContextByte = (UINT8) cbContext;
    BYTE messageRepresentative[SYMCRYPT_SHAKE256_RESULT_SIZE];
    BYTE commitmentHash[64]; // Largest possible size for commitment hash
    BYTE recalculatedCommitmentHash[64];
    UINT32 responseInfinityNorm;

    const UINT32 cbw1Encoded = pParams->nRows * pParams->w1EncodeCoefficientBitLength *
        ( SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8 );

    pTemps = SymCryptMlDsaTemporariesAllocateAndInitialize(
        pParams,
        4, // row vectors - hint, A*NTT(z), T1*(2^d), commitment
        1, // column vectors - response
        2, // poly elements - challenge, temp space for multiplication
        cbw1Encoded ); // scratch space - w1 encoded
    if( pTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // Row vectors
    PSYMCRYPT_MLDSA_VECTOR pvHint = pTemps->pvRowVectors[0];
    PSYMCRYPT_MLDSA_VECTOR pvATimesNTTz = pTemps->pvRowVectors[1];
    PSYMCRYPT_MLDSA_VECTOR pvT1Times2D = pTemps->pvRowVectors[2];
    PSYMCRYPT_MLDSA_VECTOR pvCommitment = pTemps->pvRowVectors[3];

    // Column vectors
    PSYMCRYPT_MLDSA_VECTOR pvResponse = pTemps->pvColVectors[0];

    // Poly elements
    PSYMCRYPT_MLDSA_POLYELEMENT peC = pTemps->pePolyElements[0];
    PSYMCRYPT_MLDSA_POLYELEMENT peTmp = pTemps->pePolyElements[1];

    PBYTE pbw1Encoded = pTemps->pbScratch;

    scError = SymCryptMlDsaSigDecode(
        pParams,
        pbSignature,
        cbSignature,
        commitmentHash,
        pParams->cbCommitmentHash,
        pvResponse,
        pvHint );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    responseInfinityNorm = SymCryptMlDsaVectorInfinityNorm( pvResponse );

    // For the signature to be valid, the response infinity norm must be <= (gamma_1 - beta)
    // gamma_1 = (1 << maskCoefficientRangeLog2)
    if( responseInfinityNorm >= (1 << pParams->maskCoefficientRangeLog2) - beta )
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

    SymCryptMlDsaSampleInBall(
        pParams,
        commitmentHash,
        pParams->cbCommitmentHash,
        peC );

    SymCryptMlDsaVectorNTT( pvResponse );

    for(UINT8 i = 0; i < pvResponse->nElems; ++i)
    {
        SymCryptMlDsaPolyElementMulR( SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT(i, pvResponse) );
    }

    SymCryptMlDsaMatrixVectorMontMul(
        pkMlDsakey->pmA,
        pvResponse,
        pvATimesNTTz,
        peTmp );

    // TODO osgvsowi/55435592 - Consider precomputing t1 * 2^d
    const UINT32 pow2DTimesR = 4214781;
    for(UINT8 i = 0; i < pkMlDsakey->pvt1->nElems; ++i)
    {
        PSYMCRYPT_MLDSA_POLYELEMENT peSrc = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pkMlDsakey->pvt1 );
        PSYMCRYPT_MLDSA_POLYELEMENT peDst = SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( i, pvT1Times2D );
        for(UINT32 j = 0; j < SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; ++j)
        {
            peDst->coeffs[j] = SymCryptMlDsaMontMul( peSrc->coeffs[j], pow2DTimesR );
        }
    }

    SymCryptMlDsaPolyElementNTT( peC );
    SymCryptMlDsaPolyElementMulR( peC );

    SymCryptMlDsaVectorPolyElementMontMul(
        pvT1Times2D,
        peC,
        pvT1Times2D );

    SymCryptMlDsaVectorSub(
        pvATimesNTTz,
        pvT1Times2D,
        pvCommitment );

    SymCryptMlDsaVectorINTT( pvCommitment );

    SymCryptMlDsaUseHint(
        pParams,
        pvHint,
        pvCommitment );

    SymCryptMlDsaVectorEncode(
        pvCommitment,
        pParams->w1EncodeCoefficientBitLength,
        0,
        pbw1Encoded );

    PSYMCRYPT_SHAKE256_STATE pShakeState = &(pTemps->shake256State);
    SymCryptShake256Init( pShakeState );

    if ( bExternalMu )
    {
        // Caller passes the externally-computed message representative mu
        SYMCRYPT_ASSERT( cbInput == SYMCRYPT_SHAKE256_RESULT_SIZE );
        memcpy( messageRepresentative, pbInput, SYMCRYPT_SHAKE256_RESULT_SIZE );
    }
    else
    {
        // Line 7: calculate message representative mu
        // = SHAKE256( public key hash || modeId || cbContextByte || context || OID? || message/hash, 64 )
        // The OID is only included in the HashML-DSA mode
        SymCryptShake256Append( pShakeState, pkMlDsakey->publicKeyHash, sizeof(pkMlDsakey->publicKeyHash) );
        SymCryptShake256Append( pShakeState, &modeId, sizeof( modeId ) );
        SymCryptShake256Append( pShakeState, &cbContextByte, sizeof( cbContextByte ) );

        SymCryptShake256Append( pShakeState, pbContext, cbContext );
        SymCryptShake256Append( pShakeState, pbHashOid, cbHashOid );

        SymCryptShake256Append( pShakeState, pbInput, cbInput );
        SymCryptShake256Result( pShakeState, messageRepresentative );
    }

    SymCryptShake256Append( pShakeState, messageRepresentative, sizeof(messageRepresentative) );
    SymCryptShake256Append( pShakeState, pbw1Encoded, cbw1Encoded );
    SymCryptShake256Extract( pShakeState, recalculatedCommitmentHash, pParams->cbCommitmentHash, TRUE );

    if( !SymCryptEqual( recalculatedCommitmentHash, commitmentHash, pParams->cbCommitmentHash ) )
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

cleanup:
    if( pTemps != NULL )
    {
        SymCryptMlDsaTemporariesFree( pTemps );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaVerify(
    PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    PCBYTE              pbMessage,
    SIZE_T              cbMessage,
    PCBYTE              pbContext,
    SIZE_T              cbContext,
    PCBYTE              pbSignature,
    SIZE_T              cbSignature,
    UINT32              flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if( (flags != 0) || // No flags currently supported
        (cbContext > SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH) ||
        (cbSignature != pkMlDsakey->pParams->cbEncodedSignature) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptMlDsaVerifyEx(
        pkMlDsakey,
        pbMessage,
        cbMessage,
        pbContext,
        cbContext,
        NULL, // pbHashOid
        0, // cbHashOid
        pbSignature,
        cbSignature,
        flags );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptExternalMuMlDsaVerify(
    PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    PCBYTE              pbMu,
    SIZE_T              cbMu,
    PCBYTE              pbSignature,
    SIZE_T              cbSignature,
    UINT32              flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if( (flags != 0) || // No flags currently supported
        (cbMu != SYMCRYPT_SHAKE256_RESULT_SIZE) ||
        (cbSignature != pkMlDsakey->pParams->cbEncodedSignature) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptMlDsaVerifyEx(
        pkMlDsakey,
        pbMu,
        cbMu,
        NULL, // pbContext
        0, // cbContext
        NULL, // pbHashOid
        0, // cbHashOid
        pbSignature,
        cbSignature,
        SYMCRYPT_FLAG_MLDSA_EXTERNALMU );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHashMlDsaVerify(
    PCSYMCRYPT_MLDSAKEY     pkMlDsakey,
    SYMCRYPT_PQDSA_HASH_ID  hashAlg,
    PCBYTE                  pbHash,
    SIZE_T                  cbHash,
    PCBYTE                  pbContext,
    SIZE_T                  cbContext,
    PCBYTE                  pbSignature,
    SIZE_T                  cbSignature,
    UINT32                  flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_OID pHashOid = NULL;

    if( (flags != 0) || // No flags currently supported
        (cbContext > SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH) ||
        (cbSignature != pkMlDsakey->pParams->cbEncodedSignature) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptHashMlDsaValidateHashAlgAndGetOid(
        pkMlDsakey->pParams,
        hashAlg,
        cbHash,
        &pHashOid );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptMlDsaVerifyEx(
        pkMlDsakey,
        pbHash,
        cbHash,
        pbContext,
        cbContext,
        pHashOid->pbOID,
        pHashOid->cbOID,
        pbSignature,
        cbSignature,
        flags );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    return scError;
}
