//
// mlkem.c   ML-KEM related functionality
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define NROWS_MLKEM512   (2)
#define NROWS_MLKEM768   (3)
#define NROWS_MLKEM1024  (4)

const SYMCRYPT_MLKEM_INTERNAL_PARAMS SymCryptMlKemInternalParamsMlKem512 =
{
    .params         = SYMCRYPT_MLKEM_PARAMS_MLKEM512,
    .cbPolyElement  = SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT,
    .nRows          = NROWS_MLKEM512,
    .cbVector       = sizeof(SYMCRYPT_MLKEM_VECTOR) + (NROWS_MLKEM512 * SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT),
    .cbMatrix       = sizeof(SYMCRYPT_MLKEM_MATRIX) + (NROWS_MLKEM512 *
                                                       NROWS_MLKEM512 *
                                                       SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT),
    .nEta1          = 3,
    .nEta2          = 2,
    .nBitsOfU       = 10,
    .nBitsOfV       = 4,
};

const SYMCRYPT_MLKEM_INTERNAL_PARAMS SymCryptMlKemInternalParamsMlKem768 =
{
    .params         = SYMCRYPT_MLKEM_PARAMS_MLKEM768,
    .cbPolyElement  = SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT,
    .nRows          = NROWS_MLKEM768,
    .cbVector       = sizeof(SYMCRYPT_MLKEM_VECTOR) + (NROWS_MLKEM768 * SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT),
    .cbMatrix       = sizeof(SYMCRYPT_MLKEM_MATRIX) + (NROWS_MLKEM768 *
                                                       NROWS_MLKEM768 *
                                                       SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT),
    .nEta1          = 2,
    .nEta2          = 2,
    .nBitsOfU       = 10,
    .nBitsOfV       = 4,
};

const SYMCRYPT_MLKEM_INTERNAL_PARAMS SymCryptMlKemInternalParamsMlKem1024 =
{
    .params         = SYMCRYPT_MLKEM_PARAMS_MLKEM1024,
    .cbPolyElement  = SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT,
    .nRows          = NROWS_MLKEM1024,
    .cbVector       = sizeof(SYMCRYPT_MLKEM_VECTOR) + (NROWS_MLKEM1024 * SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT),
    .cbMatrix       = sizeof(SYMCRYPT_MLKEM_MATRIX) + (NROWS_MLKEM1024 *
                                                       NROWS_MLKEM1024 *
                                                       SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT),
    .nEta1          = 2,
    .nEta2          = 2,
    .nBitsOfU       = 11,
    .nBitsOfV       = 5,
};

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemkeyGetInternalParamsFromParams(
            SYMCRYPT_MLKEM_PARAMS           params,
    _Out_   PSYMCRYPT_MLKEM_INTERNAL_PARAMS pInternalParams )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    switch( params )
    {
        case SYMCRYPT_MLKEM_PARAMS_MLKEM512:
            *pInternalParams = SymCryptMlKemInternalParamsMlKem512;
            break;
        case SYMCRYPT_MLKEM_PARAMS_MLKEM768:
            *pInternalParams = SymCryptMlKemInternalParamsMlKem768;
            break;
        case SYMCRYPT_MLKEM_PARAMS_MLKEM1024:
            *pInternalParams = SymCryptMlKemInternalParamsMlKem1024;
            break;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

cleanup:
    return scError;
}

static
PSYMCRYPT_MLKEMKEY
SYMCRYPT_CALL
SymCryptMlKemkeyInitialize(
    _In_                        PCSYMCRYPT_MLKEM_INTERNAL_PARAMS    pInternalParams,
    _Out_writes_bytes_(cbKey)   PBYTE                               pbKey,
                                UINT32                              cbKey )
{
    PSYMCRYPT_MLKEMKEY pRes = NULL;
    PSYMCRYPT_MLKEMKEY pKey = (PSYMCRYPT_MLKEMKEY)pbKey;
    PBYTE pbCurr = pbKey + sizeof(SYMCRYPT_MLKEMKEY);

    SymCryptWipeKnownSize( pbKey, cbKey );

    pKey->fAlgorithmInfo = 0;
    pKey->params = *pInternalParams;
    pKey->cbTotalSize = cbKey;
    pKey->hasPrivateSeed = FALSE;
    pKey->hasPrivateKey = FALSE;

    pKey->pmAtranspose = SymCryptMlKemMatrixCreate( pbCurr, pInternalParams->cbMatrix, pInternalParams->nRows );
    if( pKey->pmAtranspose == NULL )
    {
        goto cleanup;
    }
    pbCurr += pInternalParams->cbMatrix;

    pKey->pvt = SymCryptMlKemVectorCreate( pbCurr, pInternalParams->cbVector, pInternalParams->nRows );
    if( pKey->pvt == NULL )
    {
        goto cleanup;
    }
    pbCurr += pInternalParams->cbVector;

    pKey->pvs = SymCryptMlKemVectorCreate( pbCurr, pInternalParams->cbVector, pInternalParams->nRows );
    if( pKey->pvs == NULL )
    {
        goto cleanup;
    }
    pbCurr += pInternalParams->cbVector;

    SYMCRYPT_ASSERT( pbCurr == (pbKey + cbKey) );

    SYMCRYPT_SET_MAGIC( pKey );

    pRes = pKey;

cleanup:
    return pRes;
}

PSYMCRYPT_MLKEMKEY
SYMCRYPT_CALL
SymCryptMlKemkeyAllocate(
    SYMCRYPT_MLKEM_PARAMS   params )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE  pbKey = NULL;
    UINT32 cbKey;
    SYMCRYPT_MLKEM_INTERNAL_PARAMS internalParams;

    PSYMCRYPT_MLKEMKEY pKey = NULL;

    scError = SymCryptMlKemkeyGetInternalParamsFromParams(params, &internalParams);
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    cbKey = sizeof(SYMCRYPT_MLKEMKEY) + internalParams.cbMatrix + (2*internalParams.cbVector);

    pbKey = SymCryptCallbackAlloc( cbKey );
    if ( pbKey == NULL )
    {
        goto cleanup;
    }

    pKey = SymCryptMlKemkeyInitialize( &internalParams, pbKey, cbKey );
    if ( pKey == NULL )
    {
        goto cleanup;
    }

    pbKey = NULL;

cleanup:
    if ( pbKey != NULL )
    {
        SymCryptCallbackFree( pbKey );
    }

    return pKey;
}

VOID
SYMCRYPT_CALL
SymCryptMlKemkeyFree(
    _Inout_ PSYMCRYPT_MLKEMKEY  pkMlKemkey )
{
    SYMCRYPT_CHECK_MAGIC( pkMlKemkey );

    SymCryptWipe( (PBYTE) pkMlKemkey, pkMlKemkey->cbTotalSize );

    SymCryptCallbackFree( pkMlKemkey );
}

#define SYMCRYPT_MLKEM_SIZEOF_ENCODED_UNCOMPRESSED_VECTOR(_nRows)   (384UL * _nRows)

// s and t are encoded uncompressed vectors
// public seed, H(encapsulation key) and z are each 32 bytes
#define SYMCRYPT_MLKEM_SIZEOF_FORMAT_DECAPSULATION_KEY(_nRows)  ((2*SYMCRYPT_MLKEM_SIZEOF_ENCODED_UNCOMPRESSED_VECTOR(_nRows)) + (3*32))
// t is encoded uncompressed vector
// public seed is 32 bytes
#define SYMCRYPT_MLKEM_SIZEOF_FORMAT_ENCAPSULATION_KEY(_nRows)  (SYMCRYPT_MLKEM_SIZEOF_ENCODED_UNCOMPRESSED_VECTOR(_nRows) + 32)

C_ASSERT( SYMCRYPT_MLKEM_SIZEOF_FORMAT_DECAPSULATION_KEY(NROWS_MLKEM512) == SYMCRYPT_MLKEM_DECAPSULATION_KEY_SIZE_MLKEM512 );
C_ASSERT( SYMCRYPT_MLKEM_SIZEOF_FORMAT_DECAPSULATION_KEY(NROWS_MLKEM768) == SYMCRYPT_MLKEM_DECAPSULATION_KEY_SIZE_MLKEM768 );
C_ASSERT( SYMCRYPT_MLKEM_SIZEOF_FORMAT_DECAPSULATION_KEY(NROWS_MLKEM1024) == SYMCRYPT_MLKEM_DECAPSULATION_KEY_SIZE_MLKEM1024 );

C_ASSERT( SYMCRYPT_MLKEM_SIZEOF_FORMAT_ENCAPSULATION_KEY(NROWS_MLKEM512) == SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM512 );
C_ASSERT( SYMCRYPT_MLKEM_SIZEOF_FORMAT_ENCAPSULATION_KEY(NROWS_MLKEM768) == SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM768 );
C_ASSERT( SYMCRYPT_MLKEM_SIZEOF_FORMAT_ENCAPSULATION_KEY(NROWS_MLKEM1024) == SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM1024 );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemSizeofKeyFormatFromParams(
            SYMCRYPT_MLKEM_PARAMS       params,
            SYMCRYPT_MLKEMKEY_FORMAT    mlKemkeyFormat,
    _Out_   SIZE_T*                     pcbKeyFormat )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_MLKEM_INTERNAL_PARAMS internalParams;

    if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_NULL )
    {
        scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
        goto cleanup;
    }

    scError = SymCryptMlKemkeyGetInternalParamsFromParams(params, &internalParams);
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    switch( mlKemkeyFormat )
    {
        case SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED:
            *pcbKeyFormat = SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE;
            break;

        case SYMCRYPT_MLKEMKEY_FORMAT_DECAPSULATION_KEY:
            *pcbKeyFormat = SYMCRYPT_MLKEM_SIZEOF_FORMAT_DECAPSULATION_KEY(internalParams.nRows);
            break;

        case SYMCRYPT_MLKEMKEY_FORMAT_ENCAPSULATION_KEY:
            *pcbKeyFormat = SYMCRYPT_MLKEM_SIZEOF_FORMAT_ENCAPSULATION_KEY(internalParams.nRows);
            break;

        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemSizeofCiphertextFromParams(
            SYMCRYPT_MLKEM_PARAMS       params,
    _Out_   SIZE_T*                     pcbCiphertext )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_MLKEM_INTERNAL_PARAMS internalParams;
    SIZE_T cbU, cbV;

    scError = SymCryptMlKemkeyGetInternalParamsFromParams(params, &internalParams);
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // u vector encoded with nBitsOfU * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS bits per polynomial
    cbU = ((SIZE_T)internalParams.nRows) * internalParams.nBitsOfU * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
    // v polynomial encoded with nBitsOfV * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS bits
    cbV = ((SIZE_T)internalParams.nBitsOfV) * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
    *pcbCiphertext = cbU + cbV;

    SYMCRYPT_ASSERT( (internalParams.params != SYMCRYPT_MLKEM_PARAMS_MLKEM512)  || ((cbU + cbV) == SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_MLKEM512)  );
    SYMCRYPT_ASSERT( (internalParams.params != SYMCRYPT_MLKEM_PARAMS_MLKEM768)  || ((cbU + cbV) == SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_MLKEM768)  );
    SYMCRYPT_ASSERT( (internalParams.params != SYMCRYPT_MLKEM_PARAMS_MLKEM1024) || ((cbU + cbV) == SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_MLKEM1024) );

cleanup:
    return scError;
}

static
VOID
SYMCRYPT_CALL
SymCryptMlKemkeyExpandPublicMatrixFromPublicSeed(
    _Inout_ PSYMCRYPT_MLKEMKEY                                  pkMlKemkey,
    _Inout_ PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES    pCompTemps )
{
    UINT32 i, j;
    BYTE coordinates[2];

    PSYMCRYPT_SHAKE128_STATE pShakeStateBase = &pCompTemps->hashState0.shake128State;
    PSYMCRYPT_SHAKE128_STATE pShakeStateWork = &pCompTemps->hashState1.shake128State;
    const UINT32 nRows = pkMlKemkey->params.nRows;

    SymCryptShake128Init( pShakeStateBase );
    SymCryptShake128Append( pShakeStateBase, pkMlKemkey->publicSeed, sizeof(pkMlKemkey->publicSeed) );

    for( i=0; i<nRows; i++ )
    {
        coordinates[1] = (BYTE)i;
        for( j=0; j<nRows; j++ )
        {
            coordinates[0] = (BYTE)j;
            SymCryptShake128StateCopy( pShakeStateBase, pShakeStateWork );
            SymCryptShake128Append( pShakeStateWork, coordinates, sizeof(coordinates) );

            SymCryptMlKemPolyElementSampleNTTFromShake128( pShakeStateWork, pkMlKemkey->pmAtranspose->apPolyElements[(i*nRows)+j] );
        }
    }

    // no need to wipe; everything computed here is always public
}

static
VOID
SYMCRYPT_CALL
SymCryptMlKemkeyComputeEncapsulationKeyHash(
    _Inout_ PSYMCRYPT_MLKEMKEY                                  pkMlKemkey,
    _Inout_ PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES    pCompTemps,
            SIZE_T                                              cbEncodedVector )
{
    PSYMCRYPT_SHA3_256_STATE pState = &pCompTemps->hashState0.sha3_256State;

    SymCryptSha3_256Init( pState );
    SymCryptSha3_256Append( pState, pkMlKemkey->encodedT, cbEncodedVector );
    SymCryptSha3_256Append( pState, pkMlKemkey->publicSeed, sizeof(pkMlKemkey->publicSeed) );
    SymCryptSha3_256Result( pState, pkMlKemkey->encapsKeyHash );
}

static
VOID
SYMCRYPT_CALL
SymCryptMlKemkeyExpandFromPrivateSeed(
    _Inout_ PSYMCRYPT_MLKEMKEY                                  pkMlKemkey,
    _Inout_ PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES    pCompTemps )
{
    BYTE privateSeedHash[SYMCRYPT_SHA3_512_RESULT_SIZE];
    BYTE CBDSampleBuffer[3*64 + 1];
    PSYMCRYPT_MLKEM_VECTOR pvTmp;
    PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paTmp;
    PSYMCRYPT_SHAKE256_STATE pShakeStateBase = &pCompTemps->hashState0.shake256State;
    PSYMCRYPT_SHAKE256_STATE pShakeStateWork = &pCompTemps->hashState1.shake256State;
    UINT32 i;
    const UINT32 nRows = pkMlKemkey->params.nRows;
    const UINT32 nEta1 = pkMlKemkey->params.nEta1;
    const SIZE_T cbEncodedVector = SYMCRYPT_MLKEM_SIZEOF_ENCODED_UNCOMPRESSED_VECTOR(nRows);
    const UINT32 cbPolyElement = pkMlKemkey->params.cbPolyElement;
    const UINT32 cbVector = pkMlKemkey->params.cbVector;

    SYMCRYPT_ASSERT( pkMlKemkey->hasPrivateSeed );
    SYMCRYPT_ASSERT( (nEta1 == 2) || (nEta1 == 3) );
    SYMCRYPT_ASSERT( cbEncodedVector <= sizeof(pkMlKemkey->encodedT) );

    pvTmp = SymCryptMlKemVectorCreate( pCompTemps->abVectorBuffer0, cbVector, nRows );
    SYMCRYPT_ASSERT( pvTmp != NULL );
    paTmp = SymCryptMlKemPolyElementAccumulatorCreate( pCompTemps->abPolyElementAccumulatorBuffer, 2*cbPolyElement );
    SYMCRYPT_ASSERT( paTmp != NULL );

    // (rho || sigma) = G(d || k)
    // use CBDSampleBuffer to concatenate the private seed and encoding of nRows
    memcpy( CBDSampleBuffer, pkMlKemkey->privateSeed, sizeof(pkMlKemkey->privateSeed) );
    CBDSampleBuffer[sizeof(pkMlKemkey->privateSeed)] = (BYTE) nRows;
    SymCryptSha3_512( CBDSampleBuffer, sizeof(pkMlKemkey->privateSeed)+1, privateSeedHash );

    // copy public seed
    memcpy( pkMlKemkey->publicSeed, privateSeedHash, sizeof(pkMlKemkey->publicSeed) );

    // generate A from public seed
    SymCryptMlKemkeyExpandPublicMatrixFromPublicSeed( pkMlKemkey, pCompTemps );

    // Initialize pShakeStateBase with sigma
    SymCryptShake256Init( pShakeStateBase );
    SymCryptShake256Append( pShakeStateBase, privateSeedHash+sizeof(pkMlKemkey->publicSeed), 32 );

    // Expand s in place
    for( i=0; i<nRows; i++ )
    {
        CBDSampleBuffer[0] = (BYTE) i;
        SymCryptShake256StateCopy( pShakeStateBase, pShakeStateWork );
        SymCryptShake256Append( pShakeStateWork, CBDSampleBuffer, 1 );

        SymCryptShake256Extract( pShakeStateWork, CBDSampleBuffer, 64ul*nEta1, FALSE );

        SymCryptMlKemPolyElementSampleCBDFromBytes( CBDSampleBuffer, nEta1, SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT(i, pkMlKemkey->pvs) );
    }
    // Expand e in t, ready for multiply-add
    for( i=0; i<nRows; i++ )
    {
        CBDSampleBuffer[0] = (BYTE) (nRows+i);
        SymCryptShake256StateCopy( pShakeStateBase, pShakeStateWork );
        SymCryptShake256Append( pShakeStateWork, CBDSampleBuffer, 1 );

        SymCryptShake256Extract( pShakeStateWork, CBDSampleBuffer, 64ul*nEta1, FALSE );

        SymCryptMlKemPolyElementSampleCBDFromBytes( CBDSampleBuffer, nEta1, SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT(i, pkMlKemkey->pvt) );
    }

    // Perform NTT on s and e
    SymCryptMlKemVectorNTT( pkMlKemkey->pvs );
    SymCryptMlKemVectorNTT( pkMlKemkey->pvt );

    // pvTmp = s .* R
    SymCryptMlKemVectorMulR( pkMlKemkey->pvs, pvTmp );

    // t = ((A o (s .* R)) ./ R) + e = A o s + e
    SymCryptMlKemMatrixVectorMontMulAndAdd( pkMlKemkey->pmAtranspose, pvTmp, pkMlKemkey->pvt, paTmp );

    // transpose A
    SymCryptMlKemMatrixTranspose( pkMlKemkey->pmAtranspose );

    // precompute byte-encoding of public vector t
    SymCryptMlKemVectorCompressAndEncode( pkMlKemkey->pvt, 12, pkMlKemkey->encodedT, cbEncodedVector );

    // precompute hash of encapsulation key blob
    SymCryptMlKemkeyComputeEncapsulationKeyHash( pkMlKemkey, pCompTemps, cbEncodedVector );

    // Cleanup!
    SymCryptWipeKnownSize( privateSeedHash, sizeof(privateSeedHash) );
    SymCryptWipeKnownSize( CBDSampleBuffer, sizeof(CBDSampleBuffer) );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemkeySetValue(
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_MLKEMKEY_FORMAT    mlKemkeyFormat,
                                UINT32                      flags,
    _Inout_                     PSYMCRYPT_MLKEMKEY          pkMlKemkey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCBYTE pbCurr = pbSrc;
    PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES pCompTemps = NULL;
    const UINT32 nRows = pkMlKemkey->params.nRows;
    const SIZE_T cbEncodedVector = SYMCRYPT_MLKEM_SIZEOF_ENCODED_UNCOMPRESSED_VECTOR( nRows );

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION;

    if ( ( flags & ~allowedFlags ) != 0 )
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

    if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_NULL )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 )
    {
        // Ensure ML-KEM algorithm selftest is run before first use of ML-KEM algorithms;
        // notably _before_ first full KeyGen
        SYMCRYPT_RUN_SELFTEST_ONCE(
            SymCryptMlKemSelftest,
            SYMCRYPT_SELFTEST_ALGORITHM_MLKEM);
    }

    pCompTemps = SymCryptCallbackAlloc( sizeof(SYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES) );
    if( pCompTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED )
    {
        if( cbSrc != SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE )
        {
            scError = SYMCRYPT_WRONG_KEY_SIZE;
            goto cleanup;
        }

        pkMlKemkey->hasPrivateSeed = TRUE;
        memcpy( pkMlKemkey->privateSeed, pbCurr, sizeof(pkMlKemkey->privateSeed) );
        pbCurr += sizeof(pkMlKemkey->privateSeed);

        pkMlKemkey->hasPrivateKey = TRUE;
        memcpy( pkMlKemkey->privateRandom, pbCurr, sizeof(pkMlKemkey->privateRandom) );
        pbCurr += sizeof(pkMlKemkey->privateRandom);

        SymCryptMlKemkeyExpandFromPrivateSeed( pkMlKemkey, pCompTemps );
    }
    else if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_DECAPSULATION_KEY )
    {
        if( cbSrc != SYMCRYPT_MLKEM_SIZEOF_FORMAT_DECAPSULATION_KEY( nRows ) )
        {
            scError = SYMCRYPT_WRONG_KEY_SIZE;
            goto cleanup;
        }

        // decode s
        scError = SymCryptMlKemVectorDecodeAndDecompress( pbCurr, cbEncodedVector, 12, pkMlKemkey->pvs );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
        pbCurr += cbEncodedVector;

        // copy t and decode t
        memcpy( pkMlKemkey->encodedT, pbCurr, cbEncodedVector );
        pbCurr += cbEncodedVector;
        scError = SymCryptMlKemVectorDecodeAndDecompress( pkMlKemkey->encodedT, cbEncodedVector, 12, pkMlKemkey->pvt );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // copy public seed and expand public matrix
        memcpy( pkMlKemkey->publicSeed, pbCurr, sizeof(pkMlKemkey->publicSeed) );
        pbCurr += sizeof(pkMlKemkey->publicSeed);
        SymCryptMlKemkeyExpandPublicMatrixFromPublicSeed( pkMlKemkey, pCompTemps );

        // transpose A
        SymCryptMlKemMatrixTranspose( pkMlKemkey->pmAtranspose );

        // compute hash of encapsulation key blob
        SymCryptMlKemkeyComputeEncapsulationKeyHash( pkMlKemkey, pCompTemps, cbEncodedVector );

        // check hash of encapsulation key matches hash in the provided blob
        if( !SymCryptEqual( pbCurr, pkMlKemkey->encapsKeyHash, sizeof(pkMlKemkey->encapsKeyHash) ) )
        {
            scError = SYMCRYPT_INVALID_BLOB;
            goto cleanup;
        }
        pbCurr += sizeof(pkMlKemkey->encapsKeyHash);

        // copy private random
        memcpy( pkMlKemkey->privateRandom, pbCurr, sizeof(pkMlKemkey->privateRandom) );
        pbCurr += sizeof(pkMlKemkey->privateRandom);

        pkMlKemkey->hasPrivateSeed = FALSE;
        pkMlKemkey->hasPrivateKey  = TRUE;
    }
    else if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_ENCAPSULATION_KEY )
    {
        if( cbSrc != SYMCRYPT_MLKEM_SIZEOF_FORMAT_ENCAPSULATION_KEY( nRows ) )
        {
            scError = SYMCRYPT_WRONG_KEY_SIZE;
            goto cleanup;
        }

        // copy t and decode t
        memcpy( pkMlKemkey->encodedT, pbCurr, cbEncodedVector );
        pbCurr += cbEncodedVector;
        scError = SymCryptMlKemVectorDecodeAndDecompress( pkMlKemkey->encodedT, cbEncodedVector, 12, pkMlKemkey->pvt );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // copy public seed and expand public matrix
        memcpy( pkMlKemkey->publicSeed, pbCurr, sizeof(pkMlKemkey->publicSeed) );
        pbCurr += sizeof(pkMlKemkey->publicSeed);
        SymCryptMlKemkeyExpandPublicMatrixFromPublicSeed( pkMlKemkey, pCompTemps );

        // transpose A
        SymCryptMlKemMatrixTranspose( pkMlKemkey->pmAtranspose );

        // precompute hash of encapsulation key blob
        SymCryptMlKemkeyComputeEncapsulationKeyHash( pkMlKemkey, pCompTemps, cbEncodedVector );

        pkMlKemkey->hasPrivateSeed = FALSE;
        pkMlKemkey->hasPrivateKey  = FALSE;
    }
    else
    {
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

    SYMCRYPT_ASSERT( pbCurr == pbSrc + cbSrc );

cleanup:
    if( pCompTemps != NULL )
    {
        SymCryptWipe( pCompTemps, sizeof(*pCompTemps) );
        SymCryptCallbackFree( pCompTemps );
    }

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemkeyGetValue(
    _In_                        PCSYMCRYPT_MLKEMKEY         pkMlKemkey,
    _Out_writes_bytes_( cbDst ) PBYTE                       pbDst,
                                SIZE_T                      cbDst,
                                SYMCRYPT_MLKEMKEY_FORMAT    mlKemkeyFormat,
                                UINT32                      flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbCurr = pbDst;
    const UINT32 nRows = pkMlKemkey->params.nRows;
    const SIZE_T cbEncodedVector = SYMCRYPT_MLKEM_SIZEOF_ENCODED_UNCOMPRESSED_VECTOR( nRows );

    UNREFERENCED_PARAMETER( flags );

    if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_NULL )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED )
    {
        if( cbDst != SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE )
        {
            scError = SYMCRYPT_WRONG_KEY_SIZE;
            goto cleanup;
        }

        if( !pkMlKemkey->hasPrivateSeed )
        {
            scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
            goto cleanup;
        }

        memcpy( pbCurr, pkMlKemkey->privateSeed, sizeof(pkMlKemkey->privateSeed) );
        pbCurr += sizeof(pkMlKemkey->privateSeed);

        memcpy( pbCurr, pkMlKemkey->privateRandom, sizeof(pkMlKemkey->privateRandom) );
        pbCurr += sizeof(pkMlKemkey->privateRandom);
    }
    else if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_DECAPSULATION_KEY )
    {
        if( cbDst != SYMCRYPT_MLKEM_SIZEOF_FORMAT_DECAPSULATION_KEY( nRows ) )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        if( !pkMlKemkey->hasPrivateKey )
        {
            scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
            goto cleanup;
        }

        // We don't precompute byte-encoding of private key as exporting decapsulation key is not a critical path operation
        // All other fields are kept in memory
        SymCryptMlKemVectorCompressAndEncode( pkMlKemkey->pvs, 12, pbCurr, cbEncodedVector );
        pbCurr += cbEncodedVector;

        memcpy( pbCurr, pkMlKemkey->encodedT, cbEncodedVector );
        pbCurr += cbEncodedVector;

        memcpy( pbCurr, pkMlKemkey->publicSeed, sizeof(pkMlKemkey->publicSeed) );
        pbCurr += sizeof(pkMlKemkey->publicSeed);

        memcpy( pbCurr, pkMlKemkey->encapsKeyHash, sizeof(pkMlKemkey->encapsKeyHash) );
        pbCurr += sizeof(pkMlKemkey->encapsKeyHash);

        memcpy( pbCurr, pkMlKemkey->privateRandom, sizeof(pkMlKemkey->privateRandom) );
        pbCurr += sizeof(pkMlKemkey->privateRandom);
    }
    else if( mlKemkeyFormat == SYMCRYPT_MLKEMKEY_FORMAT_ENCAPSULATION_KEY )
    {
        if( cbDst != SYMCRYPT_MLKEM_SIZEOF_FORMAT_ENCAPSULATION_KEY( nRows ) )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        memcpy( pbCurr, pkMlKemkey->encodedT, cbEncodedVector );
        pbCurr += cbEncodedVector;

        memcpy( pbCurr, pkMlKemkey->publicSeed, sizeof(pkMlKemkey->publicSeed) );
        pbCurr += sizeof(pkMlKemkey->publicSeed);
    }
    else
    {
        scError = SYMCRYPT_NOT_IMPLEMENTED;
        goto cleanup;
    }

    SYMCRYPT_ASSERT( pbCurr == pbDst + cbDst );

cleanup:
    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemkeyGenerate(
    _Inout_                     PSYMCRYPT_MLKEMKEY  pkMlKemkey,
                                UINT32              flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    BYTE privateSeed[SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE];
    PBYTE  pbPctCipherText = NULL;
    SIZE_T cbPctCipherText = 0;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS;

    if ( ( flags & ~allowedFlags ) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptCallbackRandom( privateSeed, sizeof(privateSeed) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptMlKemkeySetValue( privateSeed, sizeof(privateSeed), SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED, flags, pkMlKemkey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    // SymCryptMlKemkeySetValue ensures the self-test is run before
    // first operational use of MlKem

    if( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 )
    {
        // PCT on key generation, encaps/decaps and check that both parties get the same shared secret with the generated key
        SIZE_T cbU, cbV;
        const UINT32 nRows = pkMlKemkey->params.nRows;
        const UINT32 nBitsOfU = pkMlKemkey->params.nBitsOfU;
        const UINT32 nBitsOfV = pkMlKemkey->params.nBitsOfV;

        // u vector encoded with nBitsOfU * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS bits per polynomial
        cbU = nRows * nBitsOfU * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
        // v polynomial encoded with nBitsOfV * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS bits
        cbV = nBitsOfV * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
        cbPctCipherText = cbU + cbV;

        pbPctCipherText = SymCryptCallbackAlloc( cbPctCipherText );
        if( pbPctCipherText == NULL )
        {
            scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }

        C_ASSERT( SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE >= 2*SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET );

        // reuse bytes 0..31 of privateSeed buffer for encapsulation shared secret
        scError = SymCryptMlKemEncapsulate(
            pkMlKemkey,
            &privateSeed[0], SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET,
            pbPctCipherText, cbPctCipherText );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            scError = SYMCRYPT_FIPS_FAILURE;
            goto cleanup;
        }

        // reuse second 32..63 bytes of privateSeed buffer for encapsulation shared secret
        scError = SymCryptMlKemDecapsulate(
            pkMlKemkey,
            pbPctCipherText, cbPctCipherText,
            &privateSeed[SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET], SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            scError = SYMCRYPT_FIPS_FAILURE;
            goto cleanup;
        }

        if( !SymCryptEqual( &privateSeed[0], &privateSeed[SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET], SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET ) )
        {
            // Do not fatal on PCT failure here, as it is expected with very low probability that
            // with correct keygen and encaps/decaps, the agreed secrets do not match
            scError = SYMCRYPT_FIPS_FAILURE;
            goto cleanup;
        }

        // could track having run the PCT with a flag in pkMlKemkey->fAlgorithmInfo,
        // but currently no need to do that given we don't ever defer the PCT
    }

cleanup:
    if( pbPctCipherText != NULL )
    {
        // Wiping is not required for security, but has low relative cost
        // and better to be on the safe side for FIPS
        SymCryptWipe( pbPctCipherText, cbPctCipherText );
        SymCryptCallbackFree( pbPctCipherText );
    }

    SymCryptWipeKnownSize( privateSeed, sizeof(privateSeed) );

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemEncapsulateInternal(
    _In_    PCSYMCRYPT_MLKEMKEY                                 pkMlKemkey,
    _Out_writes_bytes_( cbAgreedSecret )
            PBYTE                                               pbAgreedSecret,
            SIZE_T                                              cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )
            PBYTE                                               pbCiphertext,
            SIZE_T                                              cbCiphertext,
    _In_reads_bytes_( SYMCRYPT_MLKEM_SIZEOF_ENCAPS_RANDOM )
            PCBYTE                                              pbRandom,
    _Inout_ PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES    pCompTemps )
{
    BYTE CBDSampleBuffer[3*64 + 1];
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PSYMCRYPT_MLKEM_VECTOR pvrInner;
    PSYMCRYPT_MLKEM_VECTOR pvTmp;
    PSYMCRYPT_MLKEM_POLYELEMENT peTmp0, peTmp1;
    PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paTmp;
    PSYMCRYPT_SHA3_512_STATE pHashState = &pCompTemps->hashState0.sha3_512State;
    PSYMCRYPT_SHAKE256_STATE pShakeBaseState = &pCompTemps->hashState0.shake256State;
    PSYMCRYPT_SHAKE256_STATE pShakeWorkState = &pCompTemps->hashState1.shake256State;
    SIZE_T cbU, cbV;
    UINT32 i;
    const UINT32 nRows = pkMlKemkey->params.nRows;
    const UINT32 nBitsOfU = pkMlKemkey->params.nBitsOfU;
    const UINT32 nBitsOfV = pkMlKemkey->params.nBitsOfV;
    const UINT32 nEta1 = pkMlKemkey->params.nEta1;
    const UINT32 nEta2 = pkMlKemkey->params.nEta2;
    const UINT32 cbPolyElement = pkMlKemkey->params.cbPolyElement;
    const UINT32 cbVector = pkMlKemkey->params.cbVector;

    // u vector encoded with nBitsOfU * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS bits per polynomial
    cbU = nRows * nBitsOfU * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
    // v polynomial encoded with nBitsOfV * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS bits
    cbV = nBitsOfV * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);

    if( (cbAgreedSecret != SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET) ||
        (cbCiphertext != cbU + cbV) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pvrInner = SymCryptMlKemVectorCreate( pCompTemps->abVectorBuffer0, cbVector, nRows );
    SYMCRYPT_ASSERT( pvrInner != NULL );
    pvTmp = SymCryptMlKemVectorCreate( pCompTemps->abVectorBuffer1, cbVector, nRows );
    SYMCRYPT_ASSERT( pvTmp != NULL );
    peTmp0 = SymCryptMlKemPolyElementCreate( pCompTemps->abPolyElementBuffer0, cbPolyElement );
    SYMCRYPT_ASSERT( peTmp0 != NULL );
    peTmp1 = SymCryptMlKemPolyElementCreate( pCompTemps->abPolyElementBuffer1, cbPolyElement );
    SYMCRYPT_ASSERT( peTmp1 != NULL );
    paTmp = SymCryptMlKemPolyElementAccumulatorCreate( pCompTemps->abPolyElementAccumulatorBuffer, 2*cbPolyElement );
    SYMCRYPT_ASSERT( paTmp != NULL );

    // CBDSampleBuffer = (K || rOuter) = SHA3-512(pbRandom || encapsKeyHash)
    SymCryptSha3_512Init( pHashState );
    SymCryptSha3_512Append( pHashState, pbRandom, SYMCRYPT_MLKEM_SIZEOF_ENCAPS_RANDOM );
    SymCryptSha3_512Append( pHashState, pkMlKemkey->encapsKeyHash, sizeof(pkMlKemkey->encapsKeyHash) );
    SymCryptSha3_512Result( pHashState, CBDSampleBuffer );

    // Write K to pbAgreedSecret
    memcpy( pbAgreedSecret, CBDSampleBuffer, SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET );

    // Initialize pShakeStateBase with rOuter
    SymCryptShake256Init( pShakeBaseState );
    SymCryptShake256Append( pShakeBaseState, CBDSampleBuffer+cbAgreedSecret, 32 );

    // Expand rInner vector
    for( i=0; i<nRows; i++ )
    {
        CBDSampleBuffer[0] = (BYTE) i;
        SymCryptShake256StateCopy( pShakeBaseState, pShakeWorkState );
        SymCryptShake256Append( pShakeWorkState, CBDSampleBuffer, 1 );

        SymCryptShake256Extract( pShakeWorkState, CBDSampleBuffer, 64ul*nEta1, FALSE );

        SymCryptMlKemPolyElementSampleCBDFromBytes( CBDSampleBuffer, nEta1, SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT(i, pvrInner) );
    }

    // Perform NTT on rInner
    SymCryptMlKemVectorNTT( pvrInner );

    // Set pvTmp to 0
    SymCryptMlKemVectorSetZero( pvTmp );

    // pvTmp = (Atranspose o rInner) ./ R
    SymCryptMlKemMatrixVectorMontMulAndAdd( pkMlKemkey->pmAtranspose, pvrInner, pvTmp, paTmp );

    // pvTmp = INTT(Atranspose o rInner)
    SymCryptMlKemVectorINTTAndMulR( pvTmp );

    // Expand e1 and add it to pvTmp - do addition PolyElement-wise to reduce memory usage
    for( i=0; i<nRows; i++ )
    {
        CBDSampleBuffer[0] = (BYTE) (nRows+i);
        SymCryptShake256StateCopy( pShakeBaseState, pShakeWorkState );
        SymCryptShake256Append( pShakeWorkState, CBDSampleBuffer, 1 );

        SymCryptShake256Extract( pShakeWorkState, CBDSampleBuffer, 64ul*nEta2, FALSE );

        SymCryptMlKemPolyElementSampleCBDFromBytes( CBDSampleBuffer, nEta2, peTmp0 );

        SymCryptMlKemPolyElementAdd( SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT(i, pvTmp), peTmp0, SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT(i, pvTmp) );
    }

    // pvTmp = u = INTT(Atranspose o rInner) + e1
    // Compress and encode u into prefix of ciphertext
    SymCryptMlKemVectorCompressAndEncode( pvTmp, nBitsOfU, pbCiphertext, cbU );

    // peTmp0 = (t o r) ./ R
    SymCryptMlKemVectorMontDotProduct( pkMlKemkey->pvt, pvrInner, peTmp0, paTmp );

    // peTmp0 = INTT(t o r)
    SymCryptMlKemPolyElementINTTAndMulR( peTmp0 );

    // Expand e2 polynomial in peTmp1
    CBDSampleBuffer[0] = (BYTE) (2*nRows);
    SymCryptShake256StateCopy( pShakeBaseState, pShakeWorkState );
    SymCryptShake256Append( pShakeWorkState, CBDSampleBuffer, 1 );

    SymCryptShake256Extract( pShakeWorkState, CBDSampleBuffer, 64ul*nEta2, FALSE );

    SymCryptMlKemPolyElementSampleCBDFromBytes( CBDSampleBuffer, nEta2, peTmp1 );

    // peTmp = INTT(t o r) + e2
    SymCryptMlKemPolyElementAdd( peTmp0, peTmp1, peTmp0 );

    // peTmp1 = mu
    SymCryptMlKemPolyElementDecodeAndDecompress( pbRandom, 1, peTmp1 );

    // peTmp0 = v = INTT(t o r) + e2 + mu
    SymCryptMlKemPolyElementAdd( peTmp0, peTmp1, peTmp0 );

    // Compress and encode v into remainder of ciphertext
    SymCryptMlKemPolyElementCompressAndEncode( peTmp0, nBitsOfV, pbCiphertext+cbU );

cleanup:
    SymCryptWipeKnownSize( CBDSampleBuffer, sizeof(CBDSampleBuffer) );

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemEncapsulateEx(
    _In_                                    PCSYMCRYPT_MLKEMKEY pkMlKemkey,
    _In_reads_bytes_( cbRandom )            PCBYTE              pbRandom,
                                            SIZE_T              cbRandom,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE               pbAgreedSecret,
                                            SIZE_T              cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )      PBYTE               pbCiphertext,
                                            SIZE_T              cbCiphertext )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES pCompTemps = NULL;

    if( cbRandom != SYMCRYPT_MLKEM_SIZEOF_ENCAPS_RANDOM )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pCompTemps = SymCryptCallbackAlloc( sizeof(SYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES) );
    if( pCompTemps == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    scError = SymCryptMlKemEncapsulateInternal(
        pkMlKemkey,
        pbAgreedSecret, cbAgreedSecret,
        pbCiphertext, cbCiphertext,
        pbRandom,
        pCompTemps );

cleanup:
    if( pCompTemps != NULL )
    {
        SymCryptWipe( pCompTemps, sizeof(*pCompTemps) );
        SymCryptCallbackFree( pCompTemps );
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemEncapsulate(
    _In_                                    PCSYMCRYPT_MLKEMKEY pkMlKemkey,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE               pbAgreedSecret,
                                            SIZE_T              cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )      PBYTE               pbCiphertext,
                                            SIZE_T              cbCiphertext )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    BYTE pbm[SYMCRYPT_MLKEM_SIZEOF_ENCAPS_RANDOM];

    scError = SymCryptCallbackRandom( pbm, sizeof(pbm) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptMlKemEncapsulateEx(
        pkMlKemkey,
        pbm, sizeof(pbm),
        pbAgreedSecret, cbAgreedSecret,
        pbCiphertext, cbCiphertext );

cleanup:
    SymCryptWipeKnownSize( pbm, sizeof(pbm) );

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemDecapsulate(
    _In_                                    PCSYMCRYPT_MLKEMKEY pkMlKemkey,
    _In_reads_bytes_( cbCiphertext )        PCBYTE              pbCiphertext,
                                            SIZE_T              cbCiphertext,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE               pbAgreedSecret,
                                            SIZE_T              cbAgreedSecret )
{
    PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES pCompTemps = NULL;
    BYTE pbDecryptedRandom[SYMCRYPT_MLKEM_SIZEOF_ENCAPS_RANDOM];
    BYTE pbDecapsulatedSecret[SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET];
    BYTE pbImplicitRejectionSecret[SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET];
    PBYTE pbReadCiphertext, pbReencapsulatedCiphertext;
    BOOLEAN successfulReencrypt;

    PBYTE pbCurr;
    PBYTE pbAlloc = NULL;
    const SIZE_T cbAlloc = sizeof(SYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES) + (2*cbCiphertext);

    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SIZE_T cbU, cbV, cbCopy;
    PSYMCRYPT_MLKEM_VECTOR pvu;
    PSYMCRYPT_MLKEM_POLYELEMENT peTmp0, peTmp1;
    PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paTmp;
    PSYMCRYPT_SHAKE256_STATE pShakeState;
    const UINT32 nRows = pkMlKemkey->params.nRows;
    const UINT32 nBitsOfU = pkMlKemkey->params.nBitsOfU;
    const UINT32 nBitsOfV = pkMlKemkey->params.nBitsOfV;
    const UINT32 cbPolyElement = pkMlKemkey->params.cbPolyElement;
    const UINT32 cbVector = pkMlKemkey->params.cbVector;

    // u vector encoded with nBitsOfU * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS bits per polynomial
    cbU = nRows * nBitsOfU * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
    // v polynomial encoded with nBitsOfV * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS bits
    cbV = nBitsOfV * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);

    if( (cbAgreedSecret != SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET) ||
        (cbCiphertext != cbU + cbV) ||
        !pkMlKemkey->hasPrivateKey )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    pbAlloc = SymCryptCallbackAlloc( cbAlloc );
    if( pbAlloc == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }
    pbCurr = pbAlloc;

    pCompTemps = (PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES) pbCurr;
    pbCurr += sizeof(SYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES);

    pbReadCiphertext = pbCurr;
    pbCurr += cbCiphertext;

    pbReencapsulatedCiphertext = pbCurr;
    pbCurr += cbCiphertext;

    SYMCRYPT_ASSERT( pbCurr == (pbAlloc + cbAlloc) );

    // Read the input ciphertext once to local pbReadCiphertext to ensure our view of ciphertext consistent
    memcpy( pbReadCiphertext, pbCiphertext, cbCiphertext );

    pvu = SymCryptMlKemVectorCreate( pCompTemps->abVectorBuffer0, cbVector, nRows );
    SYMCRYPT_ASSERT( pvu != NULL );
    peTmp0 = SymCryptMlKemPolyElementCreate( pCompTemps->abPolyElementBuffer0, cbPolyElement );
    SYMCRYPT_ASSERT( peTmp0 != NULL );
    peTmp1 = SymCryptMlKemPolyElementCreate( pCompTemps->abPolyElementBuffer1, cbPolyElement );
    SYMCRYPT_ASSERT( peTmp1 != NULL );
    paTmp = SymCryptMlKemPolyElementAccumulatorCreate( pCompTemps->abPolyElementAccumulatorBuffer, 2*cbPolyElement );
    SYMCRYPT_ASSERT( paTmp != NULL );

    // Decode and decompress u
    scError = SymCryptMlKemVectorDecodeAndDecompress( pbReadCiphertext, cbU, nBitsOfU, pvu );
    SYMCRYPT_ASSERT( scError == SYMCRYPT_NO_ERROR );

    // Perform NTT on u
    SymCryptMlKemVectorNTT( pvu );

    // peTmp0 = (s o NTT(u)) ./ R
    SymCryptMlKemVectorMontDotProduct( pkMlKemkey->pvs, pvu, peTmp0, paTmp );

    // peTmp0 = INTT(s o NTT(u))
    SymCryptMlKemPolyElementINTTAndMulR( peTmp0 );

    // Decode and decompress v
    scError = SymCryptMlKemPolyElementDecodeAndDecompress( pbReadCiphertext+cbU, nBitsOfV, peTmp1 );
    SYMCRYPT_ASSERT( scError == SYMCRYPT_NO_ERROR );

    // peTmp0 = w = v - INTT(s o NTT(u))
    SymCryptMlKemPolyElementSub( peTmp1, peTmp0, peTmp0 );

    // pbDecryptedRandom = m' = Encoding of w
    SymCryptMlKemPolyElementCompressAndEncode( peTmp0, 1, pbDecryptedRandom );

    // Compute:
    //  pbDecapsulatedSecret = K' = Decapsulated secret (without implicit rejection)
    //  pbReencapsulatedCiphertext = c' = Ciphertext from re-encapsulating decrypted random value
    scError = SymCryptMlKemEncapsulateInternal(
        pkMlKemkey,
        pbDecapsulatedSecret, sizeof(pbDecapsulatedSecret),
        pbReencapsulatedCiphertext, cbCiphertext,
        pbDecryptedRandom,
        pCompTemps );
    SYMCRYPT_ASSERT( scError == SYMCRYPT_NO_ERROR );

    // Compute the secret we will return if using implicit rejection
    // pbImplicitRejectionSecret = K_bar = SHAKE256( z || c )
    pShakeState = &pCompTemps->hashState0.shake256State;
    SymCryptShake256Init( pShakeState );
    SymCryptShake256Append( pShakeState, pkMlKemkey->privateRandom, sizeof(pkMlKemkey->privateRandom) );
    SymCryptShake256Append( pShakeState, pbReadCiphertext, cbCiphertext );
    SymCryptShake256Extract( pShakeState, pbImplicitRejectionSecret, sizeof(pbImplicitRejectionSecret), FALSE );

    // Constant time test if re-encryption successful
    successfulReencrypt = SymCryptEqual( pbReencapsulatedCiphertext, pbReadCiphertext, cbCiphertext );

    // If not successful, perform side-channel-safe copy of Implicit Rejection secret over Decapsulated secret
    cbCopy = (((SIZE_T)successfulReencrypt)-1) & SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET;
    SymCryptScsCopy( pbImplicitRejectionSecret, cbCopy, pbDecapsulatedSecret, SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET );

    // Write agreed secret (with implicit rejection) to pbAgreedSecret
    memcpy( pbAgreedSecret, pbDecapsulatedSecret, SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET );

cleanup:
    if( pbAlloc != NULL )
    {
        SymCryptWipe( pbAlloc, cbAlloc );
        SymCryptCallbackFree( pbAlloc );
    }

    SymCryptWipeKnownSize( pbDecryptedRandom, sizeof(pbDecryptedRandom) );
    SymCryptWipeKnownSize( pbDecapsulatedSecret, sizeof(pbDecapsulatedSecret) );
    SymCryptWipeKnownSize( pbImplicitRejectionSecret, sizeof(pbImplicitRejectionSecret) );

    return scError;
}
