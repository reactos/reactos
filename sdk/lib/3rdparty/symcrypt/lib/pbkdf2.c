//
// pbkdf2.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement the pbkdf2 function
//
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptPbkdf2Derive(
    _In_                    PCSYMCRYPT_PBKDF2_EXPANDED_KEY  pExpandedKey,
    _In_reads_opt_(cbSalt)  PCBYTE                          pbSalt,
                            SIZE_T                          cbSalt,
                            UINT64                          iterationCnt,
    _Out_writes_(cbResult)  PBYTE                           pbResult,
                            SIZE_T                          cbResult)
{
    SYMCRYPT_MAC_STATE  macState;
    UINT32 iBlock;
    SIZE_T bytes;
    SIZE_T blockSize = pExpandedKey->macAlg->resultSize;
    UINT64 iterations;
    SYMCRYPT_ALIGN BYTE  rbBlockResult[SYMCRYPT_MAC_MAX_RESULT_SIZE];
    SYMCRYPT_ALIGN BYTE  rbWorkBuffer[SYMCRYPT_MAC_MAX_RESULT_SIZE];

    SYMCRYPT_ASSERT(
        blockSize <= SYMCRYPT_MAC_MAX_RESULT_SIZE &&
        cbResult > 0 );

    if (iterationCnt == 0)
    {
        return SYMCRYPT_WRONG_ITERATION_COUNT;
    }

    iBlock = 0;
    while( cbResult > 0 )
    {
        iBlock += 1;
        SYMCRYPT_STORE_MSBFIRST32( &rbBlockResult[0], iBlock );      // use result buf as temp

        pExpandedKey->macAlg->initFunc  ( &macState, &pExpandedKey->macKey);
        pExpandedKey->macAlg->appendFunc( &macState, pbSalt, cbSalt);
        pExpandedKey->macAlg->appendFunc( &macState, &rbBlockResult[0], 4 );         // block count encoded in 4 bytes
        pExpandedKey->macAlg->resultFunc( &macState, rbWorkBuffer);

#pragma warning(suppress: 22105)
        memcpy( rbBlockResult, rbWorkBuffer, blockSize );
        for( iterations = 1; iterations < iterationCnt; iterations++ )
        {
            pExpandedKey->macAlg->initFunc  ( &macState, &pExpandedKey->macKey );
            pExpandedKey->macAlg->appendFunc( &macState, rbWorkBuffer, blockSize );
            pExpandedKey->macAlg->resultFunc( &macState, rbWorkBuffer );
            SymCryptXorBytes( &rbWorkBuffer[0], &rbBlockResult[0], &rbBlockResult[0], blockSize );
        }

        bytes = SYMCRYPT_MIN( cbResult, blockSize );
        memcpy( pbResult, rbBlockResult, bytes );
        pbResult += bytes;
        cbResult -= bytes;
    }

    SymCryptWipeKnownSize( &rbWorkBuffer[0], sizeof( rbWorkBuffer ) );
    SymCryptWipeKnownSize( &rbBlockResult[0], sizeof( rbBlockResult ) );
    return SYMCRYPT_NO_ERROR;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptPbkdf2ExpandKey(
    _Out_               PSYMCRYPT_PBKDF2_EXPANDED_KEY   pExpandedKey,
    _In_                PCSYMCRYPT_MAC                  macAlgorithm,
    _In_reads_(cbKey)   PCBYTE                          pbKey,
                        SIZE_T                          cbKey )
{
    SYMCRYPT_ASSERT( macAlgorithm->expandedKeySize <= sizeof( pExpandedKey->macKey ) );

    pExpandedKey->macAlg = macAlgorithm;
    return macAlgorithm->expandKeyFunc(&pExpandedKey->macKey, pbKey, cbKey );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptPbkdf2(
                            PCSYMCRYPT_MAC  macAlgorithm,
    _In_reads_(cbKey)       PCBYTE          pbKey,
                            SIZE_T          cbKey,
    _In_reads_opt_(cbSalt)  PCBYTE          pbSalt,
                            SIZE_T          cbSalt,
                            UINT64          iterationCnt,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult)
{
    SYMCRYPT_PBKDF2_EXPANDED_KEY key;
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    scError = SymCryptPbkdf2ExpandKey( &key, macAlgorithm, pbKey, cbKey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptPbkdf2Derive( &key, pbSalt, cbSalt, iterationCnt, pbResult, cbResult );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:

    SymCryptWipeKnownSize( &key, sizeof( key ) );

    return scError;

}

//
// Self tests are in pbkdf_*.c files
// to avoid pulling in SHA-1 when only PBKDF-SHA256 is used and
// similar scenarios.
//
