//
// HmacSha512.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define ALG SHA512
#define Alg Sha512
#define SET_DATALENGTH( state, len )    {state.dataLengthL = len; state.dataLengthH = 0;}
#include "hmac_pattern.c"
#undef SET_DATALENGTH
#undef Alg
#undef ALG

const SYMCRYPT_MAC SymCryptHmacSha512Algorithm_default = {
    SymCryptHmacSha512ExpandKey,
    SymCryptHmacSha512Init,
    SymCryptHmacSha512Append,
    SymCryptHmacSha512Result,
    sizeof(SYMCRYPT_HMAC_SHA512_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA512_STATE),
    SYMCRYPT_HMAC_SHA512_RESULT_SIZE,
    &SymCryptSha512Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA512_EXPANDED_KEY, outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha512Algorithm = &SymCryptHmacSha512Algorithm_default;

static const BYTE hmacSha512Kat[64] = {
    0x07, 0x64, 0xa6, 0x58, 0xeb, 0x3e, 0x2f, 0xb0, 0x2c, 0x06, 0x72, 0x93, 0xcd, 0xaa, 0x3c, 0x05,
    0x28, 0x73, 0x15, 0xf2, 0xd3, 0xb4, 0x5a, 0x28, 0x10, 0x20, 0x1e, 0x26, 0xc3, 0x89, 0x35, 0x48,
    0xe9, 0xea, 0xca, 0x72, 0xf0, 0x2e, 0x04, 0x19, 0x20, 0x31, 0x71, 0x68, 0xb5, 0x7a, 0x86, 0x40,
    0x29, 0x1b, 0x3b, 0xb7, 0xaa, 0x4a, 0x5f, 0xaf, 0x80, 0x26, 0xb4, 0xad, 0x23, 0x5a, 0xc4, 0x25,
};


VOID
SYMCRYPT_CALL
SymCryptHmacSha512Selftest(void)
{
    SYMCRYPT_HMAC_SHA512_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA512_RESULT_SIZE];

    SymCryptHmacSha512ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha512( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );
    if( memcmp( res, hmacSha512Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh5' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
