//
// aescmac.c    Implementation of the AES-CMAC block cipher mode
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

const SYMCRYPT_MAC SymCryptAesCmacAlgorithm_fast = {
    SymCryptAesCmacExpandKey,
    SymCryptAesCmacInit,
    SymCryptAesCmacAppend,
    SymCryptAesCmacResult,
    sizeof(SYMCRYPT_AES_CMAC_EXPANDED_KEY),
    sizeof(SYMCRYPT_AES_CMAC_STATE),
    SYMCRYPT_AES_CMAC_RESULT_SIZE,
    NULL,
    0,
};

const PCSYMCRYPT_MAC SymCryptAesCmacAlgorithm = &SymCryptAesCmacAlgorithm_fast;

VOID
SYMCRYPT_CALL
SymCryptCmacMunge(
    _Inout_updates_bytes_(SYMCRYPT_AES_BLOCK_SIZE)  BYTE    buf[SYMCRYPT_AES_BLOCK_SIZE] )
{
    SIZE_T carry = 0;
    SIZE_T tmp;
    int i;

    for( i=15; i>=0; i-- )
    {
        tmp = buf[i];
        buf[i] = ((tmp << 1) | carry) & 0xff;
        carry = tmp >> 7;
    }

    buf[15] ^= (0 - carry) & 0x87;                // This is the R_128 value from SP 800-38B 5.3
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesCmacExpandKey(
    _Out_               PSYMCRYPT_AES_CMAC_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                          pbKey,
                        SIZE_T                          cbKey )
{
    SYMCRYPT_ERROR    scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_ALIGN  BYTE            buf[SYMCRYPT_AES_BLOCK_SIZE];

    scError = SymCryptAesExpandKey( &pExpandedKey->aesKey, pbKey, cbKey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptWipeKnownSize( buf, sizeof( buf ) );

    SymCryptAesEncrypt( &pExpandedKey->aesKey, buf, buf );

    SymCryptCmacMunge( buf );
    memcpy( &pExpandedKey->K1, buf, sizeof( buf ) );
    SymCryptCmacMunge( buf );
    memcpy( &pExpandedKey->K2, buf, sizeof( buf ) );

    SymCryptWipeKnownSize( buf, sizeof( buf ) );

    SYMCRYPT_SET_MAGIC( pExpandedKey );

cleanup:

    return scError;
}


VOID
SYMCRYPT_CALL
SymCryptAesCmacKeyCopy(
    _In_    PCSYMCRYPT_AES_CMAC_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_AES_CMAC_EXPANDED_KEY  pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );
    SymCryptAesKeyCopy( &pSrc->aesKey, &pDst->aesKey );
    memcpy( pDst->K1, pSrc->K1, sizeof( pDst->K1 ) );
    memcpy( pDst->K2, pSrc->K2, sizeof( pDst->K2 ) );
    SYMCRYPT_SET_MAGIC( pDst );
}


VOID
SYMCRYPT_CALL
SymCryptAesCmac(
    _In_                                            PSYMCRYPT_AES_CMAC_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                          pbData,
                                                    SIZE_T                          cbData,
    _Out_writes_( SYMCRYPT_AES_CMAC_RESULT_SIZE )   PBYTE                           pbResult )
{
    SYMCRYPT_AES_CMAC_STATE state;

    SymCryptAesCmacInit( &state, pExpandedKey );
    SymCryptAesCmacAppend( &state, pbData, cbData );
    SymCryptAesCmacResult( &state, pbResult );

    SymCryptWipeKnownSize( &state, sizeof( state ) );
}


VOID
SYMCRYPT_CALL
SymCryptAesCmacStateCopy(
    _In_        PCSYMCRYPT_AES_CMAC_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_AES_CMAC_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_AES_CMAC_STATE         pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );
    *pDst = *pSrc;

    if( pExpandedKey == NULL )
    {
        SYMCRYPT_CHECK_MAGIC( pSrc->pKey );
        pDst->pKey = pSrc->pKey;
    }
    else
    {
        SYMCRYPT_CHECK_MAGIC( pExpandedKey );
        pDst->pKey = pExpandedKey;
    }

    SYMCRYPT_SET_MAGIC( pDst );
}

VOID
SYMCRYPT_CALL
SymCryptAesCmacInit(
    _Out_   PSYMCRYPT_AES_CMAC_STATE        pState,
    _In_    PCSYMCRYPT_AES_CMAC_EXPANDED_KEY pExpandedKey)
{
    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    pState->bytesInBuf = 0;
    SymCryptWipeKnownSize( pState->chain, sizeof( pState->chain ) );
    pState->pKey = pExpandedKey;

    SYMCRYPT_SET_MAGIC( pState );
}

VOID
SYMCRYPT_CALL
SymCryptAesCmacAppend(
    _Inout_                 PSYMCRYPT_AES_CMAC_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData )
{

    SYMCRYPT_CHECK_MAGIC( pState );

    if( pState->bytesInBuf != 0 )
    {
        SIZE_T freeInBuf = SYMCRYPT_AES_BLOCK_SIZE - pState->bytesInBuf;
        SYMCRYPT_ASSERT( freeInBuf < SYMCRYPT_AES_BLOCK_SIZE );

        if( cbData <= freeInBuf )
        {
            // Do nothing.
            // the data will be copied into the buf at the end of this function
            //
        }
        else
        {
            memcpy( &pState->buf[pState->bytesInBuf], pbData, freeInBuf );
            pbData += freeInBuf;
            cbData -= freeInBuf;
            SymCryptAesCbcMac( &pState->pKey->aesKey, &pState->chain[0], &pState->buf[0], SYMCRYPT_AES_BLOCK_SIZE );
            pState->bytesInBuf = 0;
        }
    }

    //
    // At this point, either pState->bytesInBuf == 0, or it is !=0 but cbData is small enough that all the
    // data will still fit in the buffer without further processing.
    //

    if( cbData > SYMCRYPT_AES_BLOCK_SIZE )
    {
        SIZE_T bytesToDo = (cbData-1) & ~(SIZE_T)(SYMCRYPT_AES_BLOCK_SIZE - 1);
        SymCryptAesCbcMac( &pState->pKey->aesKey, &pState->chain[0], pbData, bytesToDo );
        pbData += bytesToDo;
        cbData -= bytesToDo;
    }

    if( cbData > 0 )
    {
        memcpy( &pState->buf[pState->bytesInBuf], pbData, cbData );
        pState->bytesInBuf += cbData;
    }
}


VOID
SYMCRYPT_CALL
SymCryptAesCmacResult(
    _Inout_                                         PSYMCRYPT_AES_CMAC_STATE    pState,
    _Out_writes_( SYMCRYPT_AES_CMAC_RESULT_SIZE )   PBYTE                       pbResult )
{
    SYMCRYPT_CHECK_MAGIC( pState );

    if( pState->bytesInBuf < SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptWipe( &pState->buf[pState->bytesInBuf + 1], SYMCRYPT_AES_BLOCK_SIZE - pState->bytesInBuf - 1 );
        pState->buf[pState->bytesInBuf] = 0x80;
        SymCryptXorBytes( &pState->buf[0], &pState->pKey->K2[0], &pState->buf[0], SYMCRYPT_AES_BLOCK_SIZE );
    }
    else
    {
        SymCryptXorBytes( &pState->buf[0], &pState->pKey->K1[0], &pState->buf[0], SYMCRYPT_AES_BLOCK_SIZE );
    }

    SymCryptAesCbcMac( &pState->pKey->aesKey, &pState->chain[0], &pState->buf[0], SYMCRYPT_AES_BLOCK_SIZE );
    memcpy( pbResult, &pState->chain[0], SYMCRYPT_AES_BLOCK_SIZE );

    //
    // Put the state back in the original starting state,
    // and wipe any traces of the data.
    //
    pState->bytesInBuf = 0;
    SymCryptWipeKnownSize( pState->chain, sizeof( pState->chain ) );
    SymCryptWipeKnownSize( pState->buf, sizeof( pState->buf ) );
}



static const BYTE aesCmacKat[SYMCRYPT_AES_CMAC_RESULT_SIZE] = {
    0x0a, 0x54, 0xa6, 0xa4, 0x25, 0xd4, 0x84, 0x38, 0xc3, 0xf8, 0xbb, 0xe0, 0x9b, 0xf9, 0x44, 0xcc,
};


VOID
SYMCRYPT_CALL
SymCryptAesCmacSelftest(void)
{
    SYMCRYPT_AES_CMAC_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_AES_CMAC_RESULT_SIZE];

    SymCryptAesCmacExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptAesCmac( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );
    if( memcmp( res, aesCmacKat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh5' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
