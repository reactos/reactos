//
// HmacSha256.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define ALG SHA256
#define Alg Sha256
#define SET_DATALENGTH( state, len )    {state.dataLengthL = len;}
#include "hmac_pattern.c"
#undef SET_DATALENGTH
#undef Alg
#undef ALG

const SYMCRYPT_MAC SymCryptHmacSha256Algorithm_default = {
    SymCryptHmacSha256ExpandKey,
    SymCryptHmacSha256Init,
    SymCryptHmacSha256Append,
    SymCryptHmacSha256Result,
    sizeof(SYMCRYPT_HMAC_SHA256_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA256_STATE),
    SYMCRYPT_HMAC_SHA256_RESULT_SIZE,
    &SymCryptSha256Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA256_EXPANDED_KEY, outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha256Algorithm = &SymCryptHmacSha256Algorithm_default;

static const BYTE hmacSha256Kat[32] = {
    0xd6, 0x01, 0xcc, 0x17, 0x75, 0x59, 0xb0, 0x24,
    0x84, 0x59, 0x78, 0x7f, 0x7e, 0x80, 0x4e, 0xd7,
    0xf2, 0x76, 0x89, 0xb5, 0x99, 0x5c, 0x59, 0xb6,
    0x61, 0x80, 0x2d, 0x96, 0x82, 0xfd, 0xf8, 0xd2,
};


VOID
SYMCRYPT_CALL
SymCryptHmacSha256Selftest(void)
{
    SYMCRYPT_HMAC_SHA256_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA256_RESULT_SIZE];

    SymCryptHmacSha256ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha256( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha256Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh2' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
