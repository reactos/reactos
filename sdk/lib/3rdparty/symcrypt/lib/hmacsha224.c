//
// HmacSha224.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define SymCryptSha224AppendBlocks  SymCryptSha256AppendBlocks

#define ALG SHA224
#define Alg Sha224
#define SET_DATALENGTH( state, len )    {state.dataLengthL = len;}
#include "hmac_pattern.c"
#undef SET_DATALENGTH
#undef Alg
#undef ALG

const SYMCRYPT_MAC SymCryptHmacSha224Algorithm_default = {
    SymCryptHmacSha224ExpandKey,
    SymCryptHmacSha224Init,
    SymCryptHmacSha224Append,
    SymCryptHmacSha224Result,
    sizeof(SYMCRYPT_HMAC_SHA224_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA224_STATE),
    SYMCRYPT_HMAC_SHA224_RESULT_SIZE,
    &SymCryptSha224Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA224_EXPANDED_KEY, outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha224Algorithm = &SymCryptHmacSha224Algorithm_default;

static const BYTE hmacSha224Kat[28] = {
    0x3e, 0x1c, 0x48, 0x2f, 0x66, 0x49, 0x67, 0xa9,
    0xad, 0x4f, 0x76, 0x52, 0x36, 0xf8, 0x5a, 0x1f,
    0x63, 0x5b, 0x34, 0xe9, 0x35, 0x71, 0x62, 0x35,
    0xa2, 0x9e, 0x61, 0xb1
};


VOID
SYMCRYPT_CALL
SymCryptHmacSha224Selftest(void)
{
    SYMCRYPT_HMAC_SHA224_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA224_RESULT_SIZE];

    SymCryptHmacSha224ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha224( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha224Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh4' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
