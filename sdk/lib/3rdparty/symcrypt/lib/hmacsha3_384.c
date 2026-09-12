//
// HmacSha3_256.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha3_384ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey)
{
    return SymCryptHmacExpandKey(SymCryptSha3_384Algorithm, &pExpandedKey->generic, pbKey, cbKey);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY  pDst)
{
    SymCryptHmacKeyCopy(&pSrc->generic, &pDst->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384(
    _In_                                                PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                                PCBYTE                                  pbData,
                                                        SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_384_RESULT_SIZE )  PBYTE                                   pbResult )
{
    SymCryptHmac(&pExpandedKey->generic, pbData, cbData, pbResult);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA3_384_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA3_384_STATE         pDst )
{
    SymCryptHmacStateCopy(&pSrc->generic, pExpandedKey == NULL ? NULL : &pExpandedKey->generic, &pDst->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384Init(
    _Out_   PSYMCRYPT_HMAC_SHA3_384_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY pExpandedKey)
{
    SymCryptHmacInit(&pState->generic, &pExpandedKey->generic);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA3_384_STATE   pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData )
{
    SymCryptHmacAppend(&pState->generic, pbData, cbData);
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384Result(
    _Inout_                                             PSYMCRYPT_HMAC_SHA3_384_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_384_RESULT_SIZE )  PBYTE                           pbResult )
{
    SymCryptHmacResult(&pState->generic, pbResult);
}


const SYMCRYPT_MAC SymCryptHmacSha3_384Algorithm_default = {
    SymCryptHmacSha3_384ExpandKey,
    SymCryptHmacSha3_384Init,
    SymCryptHmacSha3_384Append,
    SymCryptHmacSha3_384Result,
    sizeof(SYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA3_384_STATE),
    SYMCRYPT_HMAC_SHA3_384_RESULT_SIZE,
    &SymCryptSha3_384Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY, generic.outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha3_384Algorithm = &SymCryptHmacSha3_384Algorithm_default;


static const BYTE hmacSha3_384Kat[48] =
{
    0x8f, 0xc4, 0x8a, 0x84, 0xb5, 0xa7, 0xa1, 0x36,
    0x3c, 0x3b, 0x4b, 0x21, 0x3c, 0xfb, 0x65, 0x36,
    0xa6, 0x2b, 0xa3, 0x4c, 0x12, 0x33, 0xa1, 0x27,
    0xbc, 0xfc, 0xb2, 0xd7, 0xae, 0xaf, 0x30, 0x6b,
    0xc9, 0xe6, 0x90, 0xfd, 0xf1, 0xfa, 0x12, 0x61,
    0xa4, 0x7e, 0xb2, 0x27, 0x1a, 0xeb, 0xf1, 0x34
};

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384Selftest(void)
{
    SYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA3_384_RESULT_SIZE];

    SymCryptHmacSha3_384ExpandKey( &xKey, SymCryptTestKey32, 24 );
    SymCryptHmacSha3_384( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha3_384Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh3' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
