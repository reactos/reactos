//
// HmacSha3_256.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha3_256ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey)
{
    return SymCryptHmacExpandKey(SymCryptSha3_256Algorithm, &pExpandedKey->generic, pbKey, cbKey);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY  pDst)
{
    SymCryptHmacKeyCopy(&pSrc->generic, &pDst->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256(
    _In_                                                PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                                PCBYTE                                  pbData,
                                                        SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_256_RESULT_SIZE )  PBYTE                                   pbResult )
{
    SymCryptHmac(&pExpandedKey->generic, pbData, cbData, pbResult);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA3_256_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA3_256_STATE         pDst )
{
    SymCryptHmacStateCopy(&pSrc->generic, pExpandedKey == NULL ? NULL : &pExpandedKey->generic, &pDst->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256Init(
    _Out_   PSYMCRYPT_HMAC_SHA3_256_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY pExpandedKey)
{
    SymCryptHmacInit(&pState->generic, &pExpandedKey->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA3_256_STATE   pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData )
{
    SymCryptHmacAppend(&pState->generic, pbData, cbData);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256Result(
    _Inout_                                             PSYMCRYPT_HMAC_SHA3_256_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_256_RESULT_SIZE )  PBYTE                           pbResult )
{
    SymCryptHmacResult(&pState->generic, pbResult);
}


const SYMCRYPT_MAC SymCryptHmacSha3_256Algorithm_default = {
    SymCryptHmacSha3_256ExpandKey,
    SymCryptHmacSha3_256Init,
    SymCryptHmacSha3_256Append,
    SymCryptHmacSha3_256Result,
    sizeof(SYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA3_256_STATE),
    SYMCRYPT_HMAC_SHA3_256_RESULT_SIZE,
    &SymCryptSha3_256Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY, generic.outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha3_256Algorithm = &SymCryptHmacSha3_256Algorithm_default;


static const BYTE hmacSha3_256Kat[32] =
{
    0x18, 0xe8, 0x2e, 0xa4, 0x5a, 0x94, 0x07, 0xcc,
    0xb7, 0x87, 0x29, 0x16, 0x80, 0x99, 0xd6, 0xc6,
    0x73, 0x1b, 0x56, 0x2e, 0x0d, 0x16, 0x67, 0x5a,
    0x1f, 0xe2, 0xe3, 0xd6, 0x81, 0x56, 0x52, 0x77
};

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256Selftest(void)
{
    SYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA3_256_RESULT_SIZE];

    SymCryptHmacSha3_256ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha3_256( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha3_256Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh3' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
