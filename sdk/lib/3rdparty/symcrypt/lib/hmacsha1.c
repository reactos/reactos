//
// HmacSha1.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// This implementation of HMAC uses extensive knowledge of the internal workings of the
// SHA1 implementation and uses internal routines.
// This reduces the overhead per HMAC computation by up to 20%, which is significant
// enough to take on the added complexity.
//

#define ALG SHA1
#define Alg Sha1
#define SET_DATALENGTH( state, len )    {state.dataLengthL = len;}
#include "hmac_pattern.c"
#undef SET_DATALENGTH
#undef Alg
#undef ALG

const SYMCRYPT_MAC SymCryptHmacSha1Algorithm_default = {
    SymCryptHmacSha1ExpandKey,
    SymCryptHmacSha1Init,
    SymCryptHmacSha1Append,
    SymCryptHmacSha1Result,
    sizeof(SYMCRYPT_HMAC_SHA1_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_SHA1_STATE),
    SYMCRYPT_HMAC_SHA1_RESULT_SIZE,
    &SymCryptSha1Algorithm,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_HMAC_SHA1_EXPANDED_KEY, outerState ),
};

const PCSYMCRYPT_MAC SymCryptHmacSha1Algorithm = &SymCryptHmacSha1Algorithm_default;

static const BYTE hmacSha1Kat[20] = {
    0x2a, 0x29, 0x85, 0x40, 0x23, 0xba, 0x2e, 0xf1,
    0x49, 0x0f, 0x8c, 0xd8, 0x97, 0xa8, 0xcc, 0x6b,
    0x55, 0x7b, 0x2a, 0x12,
};

VOID
SYMCRYPT_CALL
SymCryptHmacSha1Selftest(void)
{
    SYMCRYPT_HMAC_SHA1_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_SHA1_RESULT_SIZE];

    SymCryptHmacSha1ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacSha1( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacSha1Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hSh1' );
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
