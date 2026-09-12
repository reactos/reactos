//
// HmacSha3_256.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha3_512ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey)
{
    return SymCryptHmacExpandKey(SymCryptSha3_512Algorithm, &pExpandedKey->generic, pbKey, cbKey);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY  pDst)
{
    SymCryptHmacKeyCopy(&pSrc->generic, &pDst->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512(
    _In_                                                PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                                PCBYTE                                  pbData,
                                                        SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_512_RESULT_SIZE )  PBYTE                                   pbResult )
{
    SymCryptHmac(&pExpandedKey->generic, pbData, cbData, pbResult);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA3_512_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA3_512_STATE         pDst )
{
    SymCryptHmacStateCopy(&pSrc->generic, pExpandedKey == NULL ? NULL : &pExpandedKey->generic, &pDst->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512Init(
    _Out_   PSYMCRYPT_HMAC_SHA3_512_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY pExpandedKey)
{
    SymCryptHmacInit(&pState->generic, &pExpandedKey->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA3_512_STATE   pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData )
{
    SymCryptHmacAppend(&pState->generic, pbData, cbData);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512Result(
    _Inout_                                             PSYMCRYPT_HMAC_SHA3_512_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_512_RESULT_SIZE )  PBYTE                           pbResult )
{
    SymCryptHmacResult(&pState->generic, pbResult);
}


const SYMCRYPT_MAC SymCryptHmacSha3_512Algorithm_default = {
    SymCryptHmacSha3_512ExpandKey,
    SymCryptHmacSha3_512Init,
    SymCryptHmacSha3_512Append,
    SymCryptHmacSha3_512Result,
    sizeof(SYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA3_512_STATE),
    SYMCRYPT_HMAC_SHA3_512_RESULT_SIZE,
    &SymCryptSha3_512Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY, generic.outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha3_512Algorithm = &SymCryptHmacSha3_512Algorithm_default;


static const BYTE hmacSha3_512Kat[64] =
{
    0x83, 0x3b, 0x31, 0xe7, 0x77, 0xd6, 0xb3, 0x3d,
    0x75, 0x23, 0xa5, 0x79, 0xcc, 0x3b, 0xeb, 0x27,
    0x6f, 0xd6, 0x52, 0x57, 0x54, 0xc4, 0xc5, 0x4b,
    0x2d, 0x5a, 0x34, 0x7d, 0x36, 0x24, 0x07, 0x91,
    0x7a, 0x3c, 0x62, 0x6e, 0x7e, 0xdb, 0x8e, 0x49,
    0x3b, 0x42, 0xc8, 0xe5, 0xa6, 0x96, 0xd5, 0xe6,
    0x6b, 0xa7, 0xad, 0x20, 0x00, 0xeb, 0x6c, 0xff,
    0x76, 0xcb, 0x1e, 0xc0, 0x30, 0x13, 0x0e, 0x81
};

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512Selftest(void)
{
    SYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA3_512_RESULT_SIZE];

    SymCryptHmacSha3_512ExpandKey( &xKey, SymCryptTestKey32, 32 );
    SymCryptHmacSha3_512( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha3_512Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh3' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
