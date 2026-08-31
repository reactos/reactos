//
// HmacSha3_224.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha3_224ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey)
{
    return SymCryptHmacExpandKey(SymCryptSha3_224Algorithm, &pExpandedKey->generic, pbKey, cbKey);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY  pDst)
{
    SymCryptHmacKeyCopy(&pSrc->generic, &pDst->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224(
    _In_                                                PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                                PCBYTE                                  pbData,
                                                        SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_224_RESULT_SIZE )  PBYTE                                   pbResult )
{
    SymCryptHmac(&pExpandedKey->generic, pbData, cbData, pbResult);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA3_224_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA3_224_STATE         pDst )
{
    SymCryptHmacStateCopy(&pSrc->generic, pExpandedKey == NULL ? NULL : &pExpandedKey->generic, &pDst->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224Init(
    _Out_   PSYMCRYPT_HMAC_SHA3_224_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY pExpandedKey)
{
    SymCryptHmacInit(&pState->generic, &pExpandedKey->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA3_224_STATE   pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData )
{
    SymCryptHmacAppend(&pState->generic, pbData, cbData);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224Result(
    _Inout_                                             PSYMCRYPT_HMAC_SHA3_224_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_224_RESULT_SIZE )  PBYTE                           pbResult )
{
    SymCryptHmacResult(&pState->generic, pbResult);
}


const SYMCRYPT_MAC SymCryptHmacSha3_224Algorithm_default = {
    SymCryptHmacSha3_224ExpandKey,
    SymCryptHmacSha3_224Init,
    SymCryptHmacSha3_224Append,
    SymCryptHmacSha3_224Result,
    sizeof(SYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA3_224_STATE),
    SYMCRYPT_HMAC_SHA3_224_RESULT_SIZE,
    &SymCryptSha3_224Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY, generic.outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha3_224Algorithm = &SymCryptHmacSha3_224Algorithm_default;


static const BYTE hmacSha3_224Kat[28] =
{
    0x10, 0x90, 0xac, 0xa1, 0xd5, 0xad, 0xc4, 0x12,
    0xf5, 0xe7, 0xb4, 0xdf, 0xd2, 0x87, 0x09, 0xdd,
    0x24, 0x82, 0xc0, 0x4a, 0x5e, 0x9a, 0x3b, 0xf0,
    0xc3, 0x35, 0x7e, 0x12
};

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224Selftest(void)
{
    SYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA3_224_RESULT_SIZE];

    SymCryptHmacSha3_224ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha3_224( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha3_224Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh3' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
