//
// hmac.c
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

VOID
SYMCRYPT_CALL
SymCryptHmacStateCopy(
    _In_        PCSYMCRYPT_HMAC_STATE           pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_EXPANDED_KEY    pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_STATE            pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );

    PCSYMCRYPT_HASH pHash = pSrc->pKey->pHash;

    SymCryptHashStateCopy( pHash, &pSrc->hash, &pDst->hash );

    if( pExpandedKey != NULL )
    {
        SYMCRYPT_CHECK_MAGIC( pExpandedKey );
        pDst->pKey = pExpandedKey;
    }
    else
    {
        SYMCRYPT_CHECK_MAGIC( pSrc->pKey );
        pDst->pKey = pSrc->pKey;
    }
    SYMCRYPT_SET_MAGIC( pDst );
}

VOID
SYMCRYPT_CALL
SymCryptHmacKeyCopy(
    _In_    PCSYMCRYPT_HMAC_EXPANDED_KEY    pSrc,
    _Out_   PSYMCRYPT_HMAC_EXPANDED_KEY     pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );

    // Copy innerState and outerState
    SymCryptHashStateCopy(pSrc->pHash, &pSrc->innerState, &pDst->innerState );
    SymCryptHashStateCopy(pSrc->pHash, &pSrc->outerState, &pDst->outerState );

    pDst->pHash = pSrc->pHash;

    SYMCRYPT_SET_MAGIC( pDst );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacExpandKey(
    _In_                    PCSYMCRYPT_HASH             pHash,
    _Out_                   PSYMCRYPT_HMAC_EXPANDED_KEY pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                      pbKey,
                            SIZE_T                      cbKey )
{
    // This buffer has to be large enough to hold one input block
    // and the result of the hash function.
    // Using SHA3-224 input block size to satisfy those requirements.
    SYMCRYPT_ALIGN BYTE iblock[ SYMCRYPT_SHA3_224_INPUT_BLOCK_SIZE ];

    SYMCRYPT_ASSERT( sizeof(iblock) >= pHash->inputBlockSize );
    SYMCRYPT_ASSERT( sizeof(iblock) >= pHash->resultSize );

    // XorByteIntoBuffer function updates the buffer in multiples of 8-bytes
    SYMCRYPT_ASSERT( pHash->inputBlockSize % 8 == 0);

    memset( iblock, 0, sizeof( iblock ) );

    if( cbKey <= pHash->inputBlockSize )
    {
        if( cbKey > 0 )
        {
            memcpy( iblock, pbKey, cbKey );
        }
    }
    else
    {
        SymCryptHash( pHash, pbKey, cbKey, iblock, pHash->resultSize );
    }

    XorByteIntoBuffer( iblock, pHash->inputBlockSize / 8, HMAC_IPAD_BYTE );

    //
    // Initialize the inner and outer states in the expanded key
    //
    SymCryptHashInit( pHash, &pExpandedKey->innerState );
    SymCryptHashInit( pHash, &pExpandedKey->outerState );

    // Update the inner state in the expanded key
    SymCryptHashAppend( pHash, &pExpandedKey->innerState, iblock, pHash->inputBlockSize );

    XorByteIntoBuffer( iblock, pHash->inputBlockSize / 8, HMAC_IPAD_BYTE ^ HMAC_OPAD_BYTE );

    // Update the outer state in the expanded key
    SymCryptHashAppend( pHash, &pExpandedKey->outerState, iblock, pHash->inputBlockSize );

    SymCryptWipeKnownSize( iblock, sizeof( iblock ) );

    // Save the hash function in the expanded key, it will be used in other
    // generic HMAC function calls.
    pExpandedKey->pHash = pHash;

    SYMCRYPT_SET_MAGIC(pExpandedKey);

    return SYMCRYPT_NO_ERROR;
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptHmacInit(
    _Out_   PSYMCRYPT_HMAC_STATE            pState,
    _In_    PCSYMCRYPT_HMAC_EXPANDED_KEY    pExpandedKey )
{
    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    SymCryptHashStateCopy( pExpandedKey->pHash, &pExpandedKey->innerState, &pState->hash );

    pState->pKey = pExpandedKey;

    SYMCRYPT_SET_MAGIC(pState);
}

VOID
SYMCRYPT_CALL
SymCryptHmacAppend(
    _Inout_                 PSYMCRYPT_HMAC_STATE    pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData )
{
    SymCryptHashAppend( pState->pKey->pHash, &pState->hash, pbData, cbData );
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptHmacResult(
    _Inout_                                         PSYMCRYPT_HMAC_STATE    pState,
    _Out_writes_( pState->pKey->pHash->resultSize ) PBYTE                   pbResult )
{
    BYTE    innerRes[64];

    PCSYMCRYPT_HASH pHash = pState->pKey->pHash;

    SYMCRYPT_ASSERT(sizeof(innerRes) >= pHash->resultSize);

    SYMCRYPT_CHECK_MAGIC( pState );

    //
    // We have to buffer the inner hash result. We can't put it directly in the
    // hash state data buffer as the Result() function wipes that buffer before returning.
    //
    SymCryptHashResult( pHash, &pState->hash, innerRes, pHash->resultSize );

    SYMCRYPT_CHECK_MAGIC( pState->pKey )

    SymCryptHashStateCopy( pHash, &pState->pKey->outerState, &pState->hash );

    SymCryptHashAppend( pHash, &pState->hash, innerRes, pHash->resultSize );

    SymCryptHashResult( pHash, &pState->hash, pbResult, pHash->resultSize );

    //
    // The SymCryptHashResult already wipes the hash state.
    // We only need to wipe our own buffer.
    //
    // We also set the key pointer to NULL. This is not for security;
    // it creates a clear error when callers forget to call the Init routine
    // when re-using a state. Rather than the wrong result, they will get
    // a NULL pointer exception, and they will fix their code.
    //

    SymCryptWipeKnownSize( innerRes, sizeof( innerRes ) );
    pState->pKey = NULL;
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptHmac(
    _In_                                            PCSYMCRYPT_HMAC_EXPANDED_KEY    pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                          pbData,
                                                    SIZE_T                          cbData,
    _Out_writes_( pExpandedKey->pHash->resultSize ) PBYTE                           pbResult )
{
    SYMCRYPT_HMAC_STATE state;

    SymCryptHmacInit( &state, pExpandedKey );
    SymCryptHmacAppend( &state, pbData, cbData );
    SymCryptHmacResult( &state, pbResult );
}
