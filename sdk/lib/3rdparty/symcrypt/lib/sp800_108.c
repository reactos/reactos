//
// sp800_108.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement the SP800-108 CTR KDF function
//
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSp800_108Derive(
    _In_                        PCSYMCRYPT_SP800_108_EXPANDED_KEY   pExpandedKey,
    _In_reads_opt_(cbLabel)     PCBYTE                              pbLabel,
                                SIZE_T                              cbLabel,
    _In_reads_opt_(cbContext)   PCBYTE                              pbContext,
                                SIZE_T                              cbContext,
    _Out_writes_(cbResult)      PBYTE                               pbResult,
                                SIZE_T                              cbResult)
{
    SYMCRYPT_MAC_STATE  macState;
    UINT32 iBlock;
    SIZE_T bytes;
    SIZE_T blockSize = pExpandedKey->macAlg->resultSize;
    SIZE_T bytesRemaining = cbResult;
    BYTE  buf[4];
    SYMCRYPT_ALIGN BYTE  rbBlockResult[SYMCRYPT_MAC_MAX_RESULT_SIZE];

    SYMCRYPT_ASSERT(
        blockSize <= SYMCRYPT_MAC_MAX_RESULT_SIZE &&
        bytesRemaining > 0 );

    if( cbResult > UINT32_MAX/8 )
    {
        // SP800-108 requires the output size in bits to be encoded in a 32-bit value.
        // cbResults that are too large are impossible.
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    iBlock = 0;
    while( bytesRemaining > 0 )
    {
        iBlock += 1;
        pExpandedKey->macAlg->initFunc  ( &macState, &pExpandedKey->macKey);

        //
        // We append the pieces into the MAC function. This is inefficient but works always.
        // If we need more speed for large outputs, we could use a fixed-size stack buffer to build the
        // concatenation & do a single append. This reduces the # calls in the loop, but adds one memcpy to
        // the parameters. For small output sizes this is probably a wash.
        //

        SYMCRYPT_STORE_MSBFIRST32( &buf[0], iBlock );
        pExpandedKey->macAlg->appendFunc( &macState, &buf[0], 4 );          // block count encoded in 4 bytes

        if( cbLabel != (SIZE_T) -1 )
        {
            //
            // cbLabel == -1 signals a generic input in the Context field.
            //
            pExpandedKey->macAlg->appendFunc( &macState, pbLabel, cbLabel );    // label

            buf[0] = 0;
            pExpandedKey->macAlg->appendFunc( &macState, &buf[0], 1 );          // zero byte
        }

        pExpandedKey->macAlg->appendFunc( &macState, pbContext, cbContext); // Context

        SYMCRYPT_STORE_MSBFIRST32( &buf[0], 8 * (UINT32)cbResult );
        pExpandedKey->macAlg->appendFunc( &macState, &buf[0], 4 );         // output length, in bits

        pExpandedKey->macAlg->resultFunc( &macState, rbBlockResult );

        bytes = SYMCRYPT_MIN( bytesRemaining, blockSize );
        memcpy( pbResult, rbBlockResult, bytes );
        pbResult += bytes;
        bytesRemaining -= bytes;
    }

    SymCryptWipeKnownSize( rbBlockResult, sizeof( rbBlockResult ) );
    return SYMCRYPT_NO_ERROR;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSp800_108ExpandKey(
    _Out_               PSYMCRYPT_SP800_108_EXPANDED_KEY    pExpandedKey,
    _In_                PCSYMCRYPT_MAC                      macAlgorithm,
    _In_reads_(cbKey)   PCBYTE                              pbKey,
                        SIZE_T                              cbKey )
{
    SYMCRYPT_ASSERT( macAlgorithm->expandedKeySize <= sizeof( pExpandedKey->macKey ) );

    pExpandedKey->macAlg = macAlgorithm;
    return macAlgorithm->expandKeyFunc(&pExpandedKey->macKey, pbKey, cbKey );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSp800_108(
                                PCSYMCRYPT_MAC  macAlgorithm,
    _In_reads_(cbKey)           PCBYTE          pbKey,
                                SIZE_T          cbKey,
    _In_reads_opt_(cbLabel)     PCBYTE          pbLabel,
                                SIZE_T          cbLabel,
    _In_reads_opt_(cbContext)   PCBYTE          pbContext,
                                SIZE_T          cbContext,
    _Out_writes_(cbResult)      PBYTE           pbResult,
                                SIZE_T          cbResult)
{
    SYMCRYPT_SP800_108_EXPANDED_KEY key;
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    scError = SymCryptSp800_108ExpandKey( &key, macAlgorithm, pbKey, cbKey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptSp800_108Derive( &key, pbLabel, cbLabel, pbContext, cbContext, pbResult, cbResult );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:

    SymCryptWipeKnownSize( &key, sizeof( key ) );

    return scError;

}


//
// Self tests are in sp800_108_*.c files
// to avoid pulling in SHA-1 when only SP800-108-SHA256 is used and
// similar scenarios.
//
