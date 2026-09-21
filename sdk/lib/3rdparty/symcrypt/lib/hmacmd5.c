//
// HmacMd5.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define ALG MD5
#define Alg Md5
#define SET_DATALENGTH( state, len )    {state.dataLengthL = len;}
#include "hmac_pattern.c"
#undef SET_DATALENGTH
#undef Alg
#undef ALG

const SYMCRYPT_MAC SymCryptHmacMd5Algorithm_default = {
    SymCryptHmacMd5ExpandKey,
    SymCryptHmacMd5Init,
    SymCryptHmacMd5Append,
    SymCryptHmacMd5Result,
    sizeof(SYMCRYPT_HMAC_MD5_EXPANDED_KEY),
    sizeof(SYMCRYPT_HMAC_MD5_STATE),
    SYMCRYPT_HMAC_MD5_RESULT_SIZE,
    &SymCryptMd5Algorithm,
    0,
};

const PCSYMCRYPT_MAC SymCryptHmacMd5Algorithm = &SymCryptHmacMd5Algorithm_default;

static const BYTE hmacMd5Kat[16] = {
    0x77, 0x33, 0x69, 0x79, 0x9e, 0x54, 0xeb, 0x49, 0xff, 0x21, 0xe6, 0xf9, 0x63, 0xe5, 0xbb, 0x49,
};

VOID
SYMCRYPT_CALL
SymCryptHmacMd5Selftest(void)
{
    SYMCRYPT_HMAC_MD5_EXPANDED_KEY xKey;
    BYTE res[SYMCRYPT_HMAC_MD5_RESULT_SIZE];

    SymCryptHmacMd5ExpandKey( &xKey, SymCryptTestKey32, 16 );
    SymCryptHmacMd5( &xKey, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );

    if( memcmp( res, hmacMd5Kat, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'hmd5');
    }

    //
    // Normally we would wipe the expanded key structure here,
    // but as this is a selftest with known data this is not needed.
    //
}
