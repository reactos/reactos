//
// gcm.c    Implementation of the GCM block cipher mode
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define GCM_MIN_NONCE_SIZE          (1)
#define GCM_MIN_TAG_SIZE            (12)
#define GCM_MAX_TAG_SIZE            (16)


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptGcmValidateParameters(
    _In_    PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_    SIZE_T                  cbNonce,
    _In_    UINT64                  cbAssociatedData,
    _In_    UINT64                  cbData,
    _In_    SIZE_T                  cbTag )
{
    if( pBlockCipher->blockSize != SYMCRYPT_GCM_BLOCK_SIZE )
    {
        return SYMCRYPT_WRONG_BLOCK_SIZE;
    }

    //
    // SP800-38D specifies that the nonce must be at least one bit, but we operate on bytes,
    // so the minimum is one byte.
    //
    if( cbNonce < GCM_MIN_NONCE_SIZE )
    {
        return SYMCRYPT_WRONG_NONCE_SIZE;
    }

    //
    // cbAssociatedData is limited to <2^61 bytes
    //
    if( (cbAssociatedData >> 61) > 0 )
    {
        return SYMCRYPT_WRONG_DATA_SIZE;
    }

    //
    // per SP800-38D cbData is limited to 2^36 - 32 bytes
    //
    if( cbData > SYMCRYPT_GCM_MAX_DATA_SIZE )
    {
        return SYMCRYPT_WRONG_DATA_SIZE;
    }

    if( cbTag < GCM_MIN_TAG_SIZE || cbTag > GCM_MAX_TAG_SIZE )
    {
        return SYMCRYPT_WRONG_TAG_SIZE;
    }

    return SYMCRYPT_NO_ERROR;
}



VOID
SYMCRYPT_CALL
SymCryptGcmAddMacData(
    _Inout_                     PSYMCRYPT_GCM_STATE  pState,
    _In_reads_opt_( cbData )    PCBYTE               pbData,
                                SIZE_T               cbData )
{
    SIZE_T bytesToProcess;
    if( pState->bytesInMacBlock > 0 )
    {
        bytesToProcess = SYMCRYPT_MIN( cbData, SYMCRYPT_GCM_BLOCK_SIZE - pState->bytesInMacBlock );
        memcpy( &pState->macBlock[pState->bytesInMacBlock], pbData, bytesToProcess );
        pbData += bytesToProcess;
        cbData -= bytesToProcess;
        pState->bytesInMacBlock += bytesToProcess;

        if( pState->bytesInMacBlock == SYMCRYPT_GCM_BLOCK_SIZE )
        {
            SymCryptGHashAppendData(    &pState->pKey->ghashKey,
                                        &pState->ghashState,
                                        &pState->macBlock[0],
                                        SYMCRYPT_GCM_BLOCK_SIZE );
            pState->bytesInMacBlock = 0;
        }
    }

    if( cbData >= SYMCRYPT_GCM_BLOCK_SIZE )
    {
        bytesToProcess = cbData & SYMCRYPT_GCM_BLOCK_ROUND_MASK;

        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, pbData, bytesToProcess );

        pbData += bytesToProcess;
        cbData -= bytesToProcess;
    }

    if( cbData > 0 )
    {
        memcpy( &pState->macBlock[0], pbData, cbData );
        pState->bytesInMacBlock = cbData;
    }
}



VOID
SYMCRYPT_CALL
SymCryptGcmPadMacData( _Inout_ PSYMCRYPT_GCM_STATE  pState )
{
    SIZE_T nBytes;
    //
    // Pad the MAC data with zeroes until we hit the block size.
    //
    nBytes = pState->bytesInMacBlock;
    if( nBytes > 0 )
    {
        SymCryptWipe( &pState->macBlock[nBytes], SYMCRYPT_GCM_BLOCK_SIZE - nBytes );
        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &pState->macBlock[0], SYMCRYPT_GCM_BLOCK_SIZE );
        pState->bytesInMacBlock = 0;
    }
}



VOID
SYMCRYPT_CALL
SymCryptGcmEncryptDecryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
    SIZE_T      bytesToProcess;
    SIZE_T      bytesUsedInKeyStreamBuffer;

    bytesUsedInKeyStreamBuffer = (SIZE_T) (pState->cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK);

    //
    // We update pState->cbData once before we modify cbData.
    // pState->cbData is not used in the rest of this function
    //
    SYMCRYPT_ASSERT( pState->cbData + cbData <= SYMCRYPT_GCM_MAX_DATA_SIZE );
    pState->cbData += cbData;

    if( bytesUsedInKeyStreamBuffer != 0 )
    {
        bytesToProcess = SYMCRYPT_MIN( cbData, SYMCRYPT_GCM_BLOCK_SIZE - bytesUsedInKeyStreamBuffer );
        SymCryptXorBytes( pbSrc, &pState->keystreamBlock[bytesUsedInKeyStreamBuffer], pbDst, bytesToProcess );
        pbSrc += bytesToProcess;
        pbDst += bytesToProcess;
        cbData -= bytesToProcess;

        //
        // If there are bytes left in the key stream buffer, then cbData == 0 and we're done.
        // If we used up all the bytes, then we are fine, no need to compute the next key stream block
        //
    }

    if( cbData >= SYMCRYPT_GCM_BLOCK_SIZE )
    {
        bytesToProcess = cbData & SYMCRYPT_GCM_BLOCK_ROUND_MASK;

        SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );
        SymCryptCtrMsb32(   pState->pKey->pBlockCipher,
                            &pState->pKey->blockcipherKey,
                            &pState->counterBlock[0],
                            pbSrc,
                            pbDst,
                            bytesToProcess );

        pbSrc += bytesToProcess;
        pbDst += bytesToProcess;
        cbData -= bytesToProcess;
    }

    if( cbData > 0 )
    {
        SymCryptWipeKnownSize( &pState->keystreamBlock[0], SYMCRYPT_GCM_BLOCK_SIZE );

        SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );
        SymCryptCtrMsb32(   pState->pKey->pBlockCipher,
                            &pState->pKey->blockcipherKey,
                            &pState->counterBlock[0],
                            &pState->keystreamBlock[0],
                            &pState->keystreamBlock[0],
                            SYMCRYPT_GCM_BLOCK_SIZE );

        SymCryptXorBytes( &pState->keystreamBlock[0], pbSrc, pbDst, cbData );

        //
        // pState->cbData contains the data length after this call already, so it knows how many
        // bytes are left in the keystream block
        //
    }

}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmResetCounterBlock(
    _Inout_ PSYMCRYPT_GCM_STATE pState )
{
    // Computing the tag for GCM requires invoking the GCTR function with the pre-counter
    // block which was computed when the nonce was set. Historically, we only supported 12-byte
    // nonces, so we could trivially reset the counter block by just setting the last 4 bytes to
    // (DWORD) 1. With support for larger IVs, the pre-counter block is computed from a GHash of
    // the nonce, and we don't store the value. Adding a field in the GCM struct to store the value
    // would be ABI-breaking, so instead we can recompute the value by decrementing the last 32 bits
    // of the counter block by the number of blocks that have been processed (since the counter is
    // incremented once per block), plus one for the initial increment.
    UINT32 preCounter32 = SYMCRYPT_LOAD_MSBFIRST32(&pState->counterBlock[12]) -
        (UINT32) ((pState->cbData + SYMCRYPT_GCM_BLOCK_SIZE - 1) / SYMCRYPT_GCM_BLOCK_SIZE) - 1;

    SYMCRYPT_STORE_MSBFIRST32(&pState->counterBlock[12], preCounter32);
}

VOID
SYMCRYPT_CALL
SymCryptGcmComputeTag(
    _Inout_                                 PSYMCRYPT_GCM_STATE pState,
    _Out_writes_( SYMCRYPT_GCM_BLOCK_SIZE ) PBYTE               pbTag )
{
    SYMCRYPT_ALIGN BYTE  buf[2 * SYMCRYPT_GCM_BLOCK_SIZE];

    SYMCRYPT_STORE_MSBFIRST64( &buf[16], pState->cbAuthData * 8 );
    SYMCRYPT_STORE_MSBFIRST64( &buf[24], pState->cbData * 8 );

    if( pState->bytesInMacBlock > 0 )
    {
        //
        // Pad the MAC data with zeroes until we hit the block size
        //
        SymCryptWipeKnownSize( &buf[0], SYMCRYPT_GCM_BLOCK_SIZE );
        memcpy( buf, &pState->macBlock[0], pState->bytesInMacBlock );

        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &buf[0], 2 * SYMCRYPT_GCM_BLOCK_SIZE );
    }
    else
    {
        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &buf[16], SYMCRYPT_GCM_BLOCK_SIZE );
    }

    SymCryptGcmResetCounterBlock(pState);

    //
    // Convert the GHash state to an array of bytes
    //
    SYMCRYPT_STORE_MSBFIRST64( &buf[0], pState->ghashState.ull[1] );
    SYMCRYPT_STORE_MSBFIRST64( &buf[8], pState->ghashState.ull[0] );

    SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );
    SymCryptCtrMsb32(   pState->pKey->pBlockCipher,
                        &pState->pKey->blockcipherKey,
                        &pState->counterBlock[0],
                        buf,
                        pbTag,
                        SYMCRYPT_GCM_BLOCK_SIZE );

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
}

SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptGcmExpandKey(
    _Out_               PSYMCRYPT_GCM_EXPANDED_KEY  pExpandedKey,
    _In_                PCSYMCRYPT_BLOCKCIPHER      pBlockCipher,
    _In_reads_( cbKey ) PCBYTE                      pbKey,
                        SIZE_T                      cbKey )
{
    SYMCRYPT_ALIGN BYTE    H[SYMCRYPT_GCM_BLOCK_SIZE];
    SYMCRYPT_ERROR status = SYMCRYPT_NO_ERROR;

    if( cbKey > SYMCRYPT_GCM_MAX_KEY_SIZE )
    {
        status = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    //
    // Perform the Block cipher key expansion first
    //
    pExpandedKey->pBlockCipher = pBlockCipher;
    status = pBlockCipher->expandKeyFunc( &pExpandedKey->blockcipherKey, pbKey, cbKey );

    if( status != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    //
    // We keep a copy of the key to make it easy to
    // implement the SymCryptGcmKeyCopy function
    //
    pExpandedKey->cbKey = cbKey;
    memcpy( &pExpandedKey->abKey[0], pbKey, cbKey );

    //
    // Compute H and the GHASH expanded key
    //
    SymCryptWipeKnownSize( H, sizeof( H ) );
    pBlockCipher->encryptFunc( &pExpandedKey->blockcipherKey, H, H );


    SymCryptGHashExpandKey( &pExpandedKey->ghashKey, H );


    SYMCRYPT_SET_MAGIC( pExpandedKey );

    SymCryptWipeKnownSize( H, sizeof( H ) );

cleanup:

    return status;
}

VOID
SYMCRYPT_CALL
SymCryptGcmKeyCopy( _In_ PCSYMCRYPT_GCM_EXPANDED_KEY pSrc, _Out_ PSYMCRYPT_GCM_EXPANDED_KEY pDst )
{
    SYMCRYPT_ERROR  status;

    SYMCRYPT_CHECK_MAGIC( pSrc );

    status = SymCryptGcmExpandKey( pDst, pSrc->pBlockCipher, &pSrc->abKey[0], pSrc->cbKey );
    SYMCRYPT_ASSERT( status == SYMCRYPT_NO_ERROR );
}

VOID
SYMCRYPT_CALL
SymCryptGcmSetNonce(
    _Out_                   PSYMCRYPT_GCM_STATE         pState,
    _In_reads_( cbNonce )   PCBYTE                      pbNonce,
                            SIZE_T                      cbNonce )
{
    SYMCRYPT_ASSERT( cbNonce >= GCM_MIN_NONCE_SIZE );

    // Handle the nonce depending on its size, as specified in NIST SP800-38D
    if( cbNonce == 12 )
    {
        // If len(nonce) = 96 bits (12 bytes), pre-counter block = nonce || (DWORD) 1
        memcpy( &pState->counterBlock[0], pbNonce, cbNonce );
        SymCryptWipeKnownSize( &pState->counterBlock[12], 4 );
        pState->counterBlock[15] = 1;
    }
    else
    {
        // If len(nonce) != 96 bits (12 bytes),
        // pre-counter block = GHASH(nonce padded to a multiple of 128 bits || (QWORD) len(nonce))
        BYTE buf[SYMCRYPT_GF128_BLOCK_SIZE];
        SIZE_T cbNonceRemainder = cbNonce & (SYMCRYPT_GF128_BLOCK_SIZE - 1);

        // Process all full blocks of the nonce, i.e. all nonce bytes up to a multiple of
        // SYMCRYPT_GF128_BLOCK_SIZE. SymCryptGHashAppendData ignores additional data that are
        // not a multiple of the block size. We will handle any such remaining data below.
        // (This also works if the nonce is less than the block size.)
        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, pbNonce, cbNonce );

        // If the nonce length is not a multiple of SYMCRYPT_GF128_BLOCK_SIZE, we need to pad any
        // remaining data to a multiple of the block size.
        if(cbNonceRemainder > 0)
        {
            SymCryptWipeKnownSize( buf, sizeof(buf) );
            memcpy(buf, pbNonce + cbNonce - cbNonceRemainder, cbNonceRemainder);
            SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, buf, sizeof(buf) );
        }

        // Now we append the length of the nonce in bits. We take the length as a 64-bit integer,
        // but it too must be padded to 128 bits for use in GHASH.
        SymCryptWipeKnownSize( buf, 8 );
        SYMCRYPT_STORE_MSBFIRST64( &buf[8], cbNonce * 8 );
        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, buf, sizeof(buf) );

        SymCryptGHashResult( &pState->ghashState, pState->counterBlock );
        SymCryptWipeKnownSize( &pState->ghashState, sizeof( pState->ghashState ) );
    }

    // Increment the last 32 bits of the counter. We'll recalculate the pre-counter block later
    // when computing the tag.
    SYMCRYPT_STORE_MSBFIRST32(
        &pState->counterBlock[12],
        1 + SYMCRYPT_LOAD_MSBFIRST32( &pState->counterBlock[12] ) );
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmInit(
    _Out_                   PSYMCRYPT_GCM_STATE         pState,
    _In_                    PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbNonce )   PCBYTE                      pbNonce,
                            SIZE_T                      cbNonce )
{
    UNREFERENCED_PARAMETER( cbNonce );          // It is used in an ASSERT, but only in CHKed builds.

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    pState->pKey = pExpandedKey;
    pState->cbData = 0;
    pState->cbAuthData = 0;
    pState->bytesInMacBlock = 0;
    SymCryptWipeKnownSize( &pState->ghashState, sizeof( pState->ghashState ) );

    SymCryptGcmSetNonce(pState, pbNonce, cbNonce);

    SYMCRYPT_SET_MAGIC( pState );
}


VOID
SYMCRYPT_CALL
SymCryptGcmStateCopy(
    _In_        PCSYMCRYPT_GCM_STATE            pSrc,
    _In_opt_    PCSYMCRYPT_GCM_EXPANDED_KEY     pExpandedKeyCopy,
    _Out_       PSYMCRYPT_GCM_STATE             pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );

    *pDst = *pSrc;
    if( pExpandedKeyCopy != NULL )
    {
        pDst->pKey = pExpandedKeyCopy;
    }

    SYMCRYPT_SET_MAGIC( pDst );
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmAuthPart(
    _Inout_                     PSYMCRYPT_GCM_STATE pState,
    _In_reads_opt_( cbData )    PCBYTE              pbAuthData,
                                SIZE_T              cbData )
{
    SYMCRYPT_CHECK_MAGIC( pState );
    SYMCRYPT_ASSERT( pState->cbData == 0 );

    SymCryptGcmAddMacData( pState, pbAuthData, cbData );
    pState->cbAuthData += cbData;
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmEncryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
    if( pState->cbData == 0 )
    {
        //
        // This is the first actual encryption data, pad the Auth data with zeroes if needed.
        //
        SymCryptGcmPadMacData( pState );
    }

    if ( pState->pKey->pBlockCipher->gcmEncryptPartFunc != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pState->pKey->pBlockCipher->gcmEncryptPartFunc) ( pState, pbSrc, pbDst, cbData );
        SYMCRYPT_ASSERT( pState->bytesInMacBlock <= 15 );
    }
    else
    {
        SymCryptGcmEncryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmEncryptPartTwoPass(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
    //
    // Do the actual encryption
    //
    SymCryptGcmEncryptDecryptPart( pState, pbSrc, pbDst, cbData );

    //
    // We break the read-once/write once rule here by reading the pbDst data back.
    // In this particular situation this is safe, and avoiding it is expensive as it
    // requires an extra copy and an extra memory buffer.
    // The first write exposes the GCM key stream, independent of the underlying data that
    // we are processing. From an attacking point of view we can think of this as literally
    // handing over the key stream. So encryption consists of two steps:
    // - hand over the key stream
    // - MAC some ciphertext
    // In this view (which has equivalent security properties to GCM) is obviously doesn't
    // matter that we read pbDst back.
    //

    SymCryptGcmAddMacData( pState, pbDst, cbData );
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmDecryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
    if( pState->cbData == 0 )
    {
        //
        // This is the first actual encryption data, pad the Auth data with zeroes if needed.
        //
        SymCryptGcmPadMacData( pState );
    }

    if ( pState->pKey->pBlockCipher->gcmDecryptPartFunc != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pState->pKey->pBlockCipher->gcmDecryptPartFunc) ( pState, pbSrc, pbDst, cbData );
        SYMCRYPT_ASSERT( pState->bytesInMacBlock <= 15 );
    }
    else
    {
        SymCryptGcmDecryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmDecryptPartTwoPass(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData )
{
    SymCryptGcmAddMacData( pState, pbSrc, cbData );

    //
    // Do the actual decryption
    // This violates the read-once rule, but it is safe for the same reasons as above
    // in the encryption case.
    //

    SymCryptGcmEncryptDecryptPart( pState, pbSrc, pbDst, cbData );
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmEncryptFinal(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _Out_writes_( cbTag )   PBYTE               pbTag,
                            SIZE_T              cbTag )
{
    SYMCRYPT_ALIGN BYTE    buf[SYMCRYPT_GCM_BLOCK_SIZE];

    SYMCRYPT_ASSERT( cbTag >= GCM_MIN_TAG_SIZE && cbTag <= GCM_MAX_TAG_SIZE );

    SymCryptGcmComputeTag( pState, &buf[0] );
    memcpy( pbTag, buf, cbTag );

    SymCryptWipeKnownSize( buf, sizeof( buf ) );

    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
    SYMCRYPT_ASSERT( pState->bytesInMacBlock == 0 );
}

SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptGcmDecryptFinal(
    _Inout_             PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbTag ) PCBYTE              pbTag,
                        SIZE_T              cbTag )
{
    SYMCRYPT_ALIGN BYTE    buf[SYMCRYPT_GCM_BLOCK_SIZE];
    SYMCRYPT_ERROR status;

    SYMCRYPT_ASSERT( cbTag >= GCM_MIN_TAG_SIZE && cbTag <= GCM_MAX_TAG_SIZE );

    SymCryptGcmComputeTag( pState, &buf[0] );

    if( !SymCryptEqual( pbTag, buf, cbTag ) )
    {
        status = SYMCRYPT_AUTHENTICATION_FAILURE;
    }
    else
    {
        status = SYMCRYPT_NO_ERROR;
    }

    SymCryptWipeKnownSize( buf, sizeof( buf ) );

    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
    SYMCRYPT_ASSERT( pState->bytesInMacBlock == 0 );

    return status;
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptGcmEncrypt(
    _In_                            PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbNonce )           PCBYTE                      pbNonce,
                                    SIZE_T                      cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE                      pbAuthData,
                                    SIZE_T                      cbAuthData,
    _In_reads_( cbData )            PCBYTE                      pbSrc,
    _Out_writes_( cbData )          PBYTE                       pbDst,
                                    SIZE_T                      cbData,
    _Out_writes_( cbTag )           PBYTE                       pbTag,
                                    SIZE_T                      cbTag )
{
    SYMCRYPT_ALIGN BYTE buf[2 * SYMCRYPT_GCM_BLOCK_SIZE];
    SYMCRYPT_GCM_STATE  state;
    PSYMCRYPT_GCM_STATE pState = &state;

    // SymCryptGcmInit( &state, pExpandedKey, pbNonce, cbNonce );
    UNREFERENCED_PARAMETER( cbNonce );          // It is used in an ASSERT, but only in CHKed builds.

    SYMCRYPT_ASSERT( cbNonce >= GCM_MIN_NONCE_SIZE );
    SYMCRYPT_ASSERT( cbTag >= GCM_MIN_TAG_SIZE && cbTag <= GCM_MAX_TAG_SIZE );

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    pState->pKey = pExpandedKey;
    pState->cbData = 0;
    pState->cbAuthData = 0;
    pState->bytesInMacBlock = 0;
    SymCryptWipeKnownSize( &pState->ghashState, sizeof( pState->ghashState ) );

    SymCryptGcmSetNonce( pState, pbNonce, cbNonce );

    // SymCryptGcmAuthPart( &state, pbAuthData, cbAuthData );
    pState->cbAuthData += cbAuthData;
    if( cbAuthData >= SYMCRYPT_GCM_BLOCK_SIZE )
    {
        SIZE_T bytesToDo = cbAuthData & SYMCRYPT_GCM_BLOCK_ROUND_MASK;

        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, pbAuthData, bytesToDo );

        pbAuthData += bytesToDo;
        cbAuthData -= bytesToDo;
    }

    if( cbAuthData > 0 )
    {
        //
        // Pad the MAC data with zeroes until we hit the block size.
        //
        SymCryptWipeKnownSize( &pState->macBlock[0], SYMCRYPT_GCM_BLOCK_SIZE );
        memcpy( &pState->macBlock[0], pbAuthData, cbAuthData );
        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &pState->macBlock[0], SYMCRYPT_GCM_BLOCK_SIZE );
    }

    // SymCryptGcmEncryptPart( &state, pbSrc, pbDst, cbData );
    if ( pState->pKey->pBlockCipher->gcmEncryptPartFunc != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pState->pKey->pBlockCipher->gcmEncryptPartFunc) ( pState, pbSrc, pbDst, cbData );
    }
    else
    {
        SymCryptGcmEncryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }

    // SymCryptGcmEncryptFinal( &state, pbTag, cbTag );
    SYMCRYPT_STORE_MSBFIRST64( &buf[16], pState->cbAuthData * 8 );
    SYMCRYPT_STORE_MSBFIRST64( &buf[24], pState->cbData * 8 );

    if( pState->bytesInMacBlock > 0 )
    {
        //
        // Pad the MAC data with zeroes until we hit the block size
        //
        SymCryptWipeKnownSize( &buf[0], SYMCRYPT_GCM_BLOCK_SIZE );
        memcpy( buf, &pState->macBlock[0], pState->bytesInMacBlock );

        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &buf[0], 2 * SYMCRYPT_GCM_BLOCK_SIZE );
    }
    else
    {
        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &buf[16], SYMCRYPT_GCM_BLOCK_SIZE );
    }

    // Reset the counter block prior to computing the tag
    SymCryptGcmResetCounterBlock( pState );

    //
    // Convert the GHash state to an array of bytes
    //
    SYMCRYPT_STORE_MSBFIRST64( &buf[0], pState->ghashState.ull[1] );
    SYMCRYPT_STORE_MSBFIRST64( &buf[8], pState->ghashState.ull[0] );

    SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );
    SymCryptCtrMsb32(   pState->pKey->pBlockCipher,
                        &pState->pKey->blockcipherKey,
                        &pState->counterBlock[0],
                        buf,
                        buf,
                        SYMCRYPT_GCM_BLOCK_SIZE );

    memcpy( pbTag, buf, cbTag );

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
}


SYMCRYPT_NOINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptGcmDecrypt(
    _In_                            PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbNonce )           PCBYTE                      pbNonce,
                                    SIZE_T                      cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE                      pbAuthData,
                                    SIZE_T                      cbAuthData,
    _In_reads_( cbData )            PCBYTE                      pbSrc,
    _Out_writes_( cbData )          PBYTE                       pbDst,
                                    SIZE_T                      cbData,
    _In_reads_( cbTag )             PCBYTE                      pbTag,
                                    SIZE_T                      cbTag )
{
    SYMCRYPT_ERROR status;
    SYMCRYPT_ALIGN BYTE buf[2 * SYMCRYPT_GCM_BLOCK_SIZE];
    SYMCRYPT_GCM_STATE  state;
    PSYMCRYPT_GCM_STATE pState = &state;

    // SymCryptGcmInit( &state, pExpandedKey, pbNonce, cbNonce );
    UNREFERENCED_PARAMETER( cbNonce );          // It is used in an ASSERT, but only in CHKed builds.

    SYMCRYPT_ASSERT( cbNonce >= GCM_MIN_NONCE_SIZE );
    SYMCRYPT_ASSERT( cbTag >= GCM_MIN_TAG_SIZE && cbTag <= GCM_MAX_TAG_SIZE );

    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    pState->pKey = pExpandedKey;
    pState->cbData = 0;
    pState->cbAuthData = 0;
    pState->bytesInMacBlock = 0;
    SymCryptWipeKnownSize( &pState->ghashState, sizeof( pState->ghashState ) );

    SymCryptGcmSetNonce( pState, pbNonce, cbNonce );

    // SymCryptGcmAuthPart( &state, pbAuthData, cbAuthData );
    pState->cbAuthData += cbAuthData;
    if( cbAuthData >= SYMCRYPT_GCM_BLOCK_SIZE )
    {
        SIZE_T bytesToDo = cbAuthData & SYMCRYPT_GCM_BLOCK_ROUND_MASK;

        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, pbAuthData, bytesToDo );

        pbAuthData += bytesToDo;
        cbAuthData -= bytesToDo;
    }

    if( cbAuthData > 0 )
    {
        //
        // Pad the MAC data with zeroes until we hit the block size.
        //
        SymCryptWipeKnownSize( &pState->macBlock[0], SYMCRYPT_GCM_BLOCK_SIZE );
        memcpy( &pState->macBlock[0], pbAuthData, cbAuthData );
        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &pState->macBlock[0], SYMCRYPT_GCM_BLOCK_SIZE );
    }

    // SymCryptGcmDecryptPart( &state, pbSrc, pbDst, cbData );
    if ( pState->pKey->pBlockCipher->gcmDecryptPartFunc != NULL )
    {
        //
        // Use optimized implementation if available
        //
        (*pState->pKey->pBlockCipher->gcmDecryptPartFunc) ( pState, pbSrc, pbDst, cbData );
    }
    else
    {
        SymCryptGcmDecryptPartTwoPass( pState, pbSrc, pbDst, cbData );
    }

    //status = SymCryptGcmDecryptFinal( &state, pbTag, cbTag );
    SYMCRYPT_STORE_MSBFIRST64( &buf[16], pState->cbAuthData * 8 );
    SYMCRYPT_STORE_MSBFIRST64( &buf[24], pState->cbData * 8 );

    if( pState->bytesInMacBlock > 0 )
    {
        //
        // Pad the MAC data with zeroes until we hit the block size
        //
        SymCryptWipeKnownSize( &buf[0], SYMCRYPT_GCM_BLOCK_SIZE );
        memcpy( buf, &pState->macBlock[0], pState->bytesInMacBlock );

        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &buf[0], 2 * SYMCRYPT_GCM_BLOCK_SIZE );
    }
    else
    {
        SymCryptGHashAppendData( &pState->pKey->ghashKey, &pState->ghashState, &buf[16], SYMCRYPT_GCM_BLOCK_SIZE );
    }

    SymCryptGcmResetCounterBlock( pState );

    //
    // Convert the GHash state to an array of bytes
    //
    SYMCRYPT_STORE_MSBFIRST64( &buf[0], pState->ghashState.ull[1] );
    SYMCRYPT_STORE_MSBFIRST64( &buf[8], pState->ghashState.ull[0] );

    SYMCRYPT_ASSERT( pState->pKey->pBlockCipher->blockSize == SYMCRYPT_GCM_BLOCK_SIZE );
    SymCryptCtrMsb32(   pState->pKey->pBlockCipher,
                        &pState->pKey->blockcipherKey,
                        &pState->counterBlock[0],
                        buf,
                        buf,
                        SYMCRYPT_GCM_BLOCK_SIZE );

    if( !SymCryptEqual( pbTag, buf, cbTag ) )
    {
        status = SYMCRYPT_AUTHENTICATION_FAILURE;
    }
    else
    {
        status = SYMCRYPT_NO_ERROR;
    }

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
    SymCryptWipeKnownSize( pState, sizeof( *pState ) );

    if( status != SYMCRYPT_NO_ERROR )
    {
        SymCryptWipe( pbDst, cbData );
    }

    return status;
}


static const BYTE SymCryptGcmSelftestResult[3 + SYMCRYPT_AES_BLOCK_SIZE ] =
{
    0xa5, 0x4c, 0x60,
    0x80, 0xb0, 0x48, 0x6d, 0x03, 0x9f, 0xea, 0xc3, 0x3c, 0x28, 0x96, 0x3f, 0x99, 0x8a, 0x77, 0x43,
};

VOID
SYMCRYPT_CALL
SymCryptGcmSelftest(void)
{
    BYTE    buf[ 3 + SYMCRYPT_AES_BLOCK_SIZE ];
    SYMCRYPT_GCM_EXPANDED_KEY key;
    SYMCRYPT_ERROR err;

    if( SymCryptGcmExpandKey( &key, SymCryptAesBlockCipher, SymCryptTestKey32, 16 ) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'gcm0' );
    }

    SymCryptGcmEncrypt( &key,
                        &SymCryptTestKey32[16], 12,
                        NULL, 0,
                        &SymCryptTestMsg3[0], buf, 3,
                        &buf[3], SYMCRYPT_AES_BLOCK_SIZE );

    SymCryptInjectError( buf, sizeof( buf ) );
    if( memcmp( buf, SymCryptGcmSelftestResult, sizeof( buf ) ) != 0 )
    {
        SymCryptFatal( 'gcm1' );
    }

    // inject error into the ciphertext or tag
    SymCryptInjectError( buf, sizeof( buf ) );

    err = SymCryptGcmDecrypt(   &key,
                                &SymCryptTestKey32[16], 12,
                                NULL, 0,
                                buf, buf, 3,
                                &buf[3], SYMCRYPT_AES_BLOCK_SIZE );

    SymCryptInjectError( buf, 3 );

    if( err != SYMCRYPT_NO_ERROR || memcmp( buf, SymCryptTestMsg3, 3 ) != 0 )
    {
        SymCryptFatal( 'gcm2' );
    }

}
