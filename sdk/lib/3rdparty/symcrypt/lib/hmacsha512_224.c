//
// HmacSha512_224.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define SymCryptSha512_224AppendBlocks  SymCryptSha512AppendBlocks

#define ALG SHA512_224
#define Alg Sha512_224
#define SET_DATALENGTH( state, len )    {state.dataLengthL = len; state.dataLengthH = 0;}
#include "hmac_pattern.c"
#undef SET_DATALENGTH
#undef Alg
#undef ALG

const SYMCRYPT_MAC SymCryptHmacSha512_224Algorithm_default = {
    SymCryptHmacSha512_224ExpandKey,
    SymCryptHmacSha512_224Init,
    SymCryptHmacSha512_224Append,
    SymCryptHmacSha512_224Result,
    sizeof(SYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA512_224_STATE),
    SYMCRYPT_HMAC_SHA512_224_RESULT_SIZE,
    &SymCryptSha512_224Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY, outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha512_224Algorithm = &SymCryptHmacSha512_224Algorithm_default;

static const BYTE hmacSha512_224Kat[28] = {
    0x62, 0xc9, 0x59, 0xc7, 0x5b, 0x3c, 0xb2, 0xaf,
    0x95, 0xf5, 0x59, 0x73, 0x2c, 0x46, 0x1d, 0x72,
    0x06, 0x9e, 0xf9, 0x52, 0x9a, 0x8d, 0x84, 0x1a,
    0x73, 0x97, 0xa6, 0x9c
};


VOID
SYMCRYPT_CALL
SymCryptHmacSha512_224Selftest(void)
{
    SYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA512_224_RESULT_SIZE];

    SymCryptHmacSha512_224ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha512_224( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha512_224Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh4' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
