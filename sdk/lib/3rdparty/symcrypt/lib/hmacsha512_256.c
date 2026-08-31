//
// HmacSha512_256.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define SymCryptSha512_256AppendBlocks  SymCryptSha512AppendBlocks

#define ALG SHA512_256
#define Alg Sha512_256
#define SET_DATALENGTH( state, len )    {state.dataLengthL = len; state.dataLengthH = 0;}
#include "hmac_pattern.c"
#undef SET_DATALENGTH
#undef Alg
#undef ALG

const SYMCRYPT_MAC SymCryptHmacSha512_256Algorithm_default = {
    SymCryptHmacSha512_256ExpandKey,
    SymCryptHmacSha512_256Init,
    SymCryptHmacSha512_256Append,
    SymCryptHmacSha512_256Result,
    sizeof(SYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA512_256_STATE),
    SYMCRYPT_HMAC_SHA512_256_RESULT_SIZE,
    &SymCryptSha512_256Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY, outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha512_256Algorithm = &SymCryptHmacSha512_256Algorithm_default;

static const BYTE hmacSha512_256Kat[32] = {
    0x79, 0x44, 0xb9, 0x97, 0xc0, 0xaa, 0xf7, 0x11,
    0xdd, 0xb3, 0x78, 0x60, 0x68, 0xdb, 0x2b, 0xa1,
    0x40, 0x80, 0x4f, 0xdc, 0xb7, 0x02, 0x7b, 0x6a,
    0xe9, 0x9f, 0x5a, 0x38, 0xc8, 0x28, 0x67, 0x4c
};


VOID
SYMCRYPT_CALL
SymCryptHmacSha512_256Selftest(void)
{
    SYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA512_256_RESULT_SIZE];

    SymCryptHmacSha512_256ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha512_256( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha512_256Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh4' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
