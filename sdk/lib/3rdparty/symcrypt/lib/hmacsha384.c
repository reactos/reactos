//
// HmacSha512.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define SymCryptSha384AppendBlocks  SymCryptSha512AppendBlocks

#define ALG SHA384
#define Alg Sha384
#define SET_DATALENGTH( state, len )    {state.dataLengthL = len; state.dataLengthH = 0;}
#include "hmac_pattern.c"
#undef SET_DATALENGTH
#undef Alg
#undef ALG

const SYMCRYPT_MAC SymCryptHmacSha384Algorithm_default = {
    SymCryptHmacSha384ExpandKey,
    SymCryptHmacSha384Init,
    SymCryptHmacSha384Append,
    SymCryptHmacSha384Result,
    sizeof(SYMCRYPT_HMAC_SHA384_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA384_STATE),
    SYMCRYPT_HMAC_SHA384_RESULT_SIZE,
    &SymCryptSha384Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA384_EXPANDED_KEY, outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha384Algorithm = &SymCryptHmacSha384Algorithm_default;

static const BYTE hmacSha384Kat[48] = {
    0x67, 0xdb, 0x9d, 0x4d, 0x66, 0xed, 0xf2, 0xe7, 0x2b, 0x88, 0xb8, 0x50, 0x55, 0x68, 0xa0, 0x00,
    0xa9, 0x83, 0x2b, 0xa3, 0x5e, 0x4f, 0xde, 0xcf, 0xe5, 0x38, 0x9a, 0x5d, 0x92, 0x79, 0x81, 0x53,
    0x6d, 0xdb, 0x94, 0xc0, 0xf6, 0xc0, 0xbd, 0x94, 0xc4, 0x18, 0x96, 0x4b, 0xbe, 0x4b, 0x6c, 0xf2,
};

VOID
SYMCRYPT_CALL
SymCryptHmacSha384Selftest(void)
{
    SYMCRYPT_HMAC_SHA384_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA384_RESULT_SIZE];

    SymCryptHmacSha384ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha384( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );
    if( memcmp( res, hmacSha384Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hsh3' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
