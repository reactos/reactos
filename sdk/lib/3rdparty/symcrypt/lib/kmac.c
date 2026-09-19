//
// kmac.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"


//
//  KMAC128
//
#define Alg Kmac128
#define ALG KMAC128
#define SYMCRYPT_CSHAKEXXX_INIT             SymCryptCShake128Init
#define SYMCRYPT_CSHAKEXXX_STATE            SYMCRYPT_CSHAKE128_STATE
#define SYMCRYPT_KMACXXX_RESULT_SIZE        SYMCRYPT_KMAC128_RESULT_SIZE
#include "kmac_pattern.c"
#undef SYMCRYPT_KMACXXX_RESULT_SIZE
#undef SYMCRYPT_CSHAKEXXX_STATE
#undef SYMCRYPT_CSHAKEXXX_INIT
#undef ALG
#undef Alg

// MAC interface
const SYMCRYPT_MAC SymCryptKmac128Algorithm_Default = {
    SymCryptKmac128ExpandKey,
    SymCryptKmac128Init,
    SymCryptKmac128Append,
    SymCryptKmac128Result,
    sizeof(SYMCRYPT_KMAC128_EXPANDED_KEY),
    sizeof(SYMCRYPT_KMAC128_STATE),
    SYMCRYPT_KMAC128_RESULT_SIZE,
    NULL,   // ppHashAlgorithm
    0,      // outerChainingStateOffset
};

const PCSYMCRYPT_MAC SymCryptKmac128Algorithm = &SymCryptKmac128Algorithm_Default;

static const BYTE kmac128KATAnswer[SYMCRYPT_KMAC128_RESULT_SIZE] = {
    0xea, 0xe9, 0xde, 0xd3, 0xee, 0x2f, 0x34, 0x8a,
    0xd6, 0xd2, 0xcb, 0x70, 0x4b, 0xba, 0xd4, 0x47,
    0x15, 0x32, 0x46, 0x82, 0x8e, 0x41, 0x3a, 0xf5,
    0xf5, 0x62, 0x96, 0x1a, 0xf7, 0x67, 0x48, 0xc1
};

VOID
SYMCRYPT_CALL
SymCryptKmac128Selftest(void)
{
    BYTE result[SYMCRYPT_KMAC128_RESULT_SIZE];
    static const unsigned char Sstr[] = { 'S' };
    SYMCRYPT_KMAC128_EXPANDED_KEY expandedKey;

    SymCryptKmac128ExpandKeyEx(&expandedKey, SymCryptTestKey32, 16, Sstr, sizeof(Sstr));

    SymCryptKmac128(&expandedKey, SymCryptTestMsg16, sizeof(SymCryptTestMsg16), result);

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, kmac128KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('kmac');
    }
}


//
//  KMAC256
//
#define Alg Kmac256
#define ALG KMAC256
#define SYMCRYPT_CSHAKEXXX_INIT             SymCryptCShake256Init
#define SYMCRYPT_CSHAKEXXX_STATE            SYMCRYPT_CSHAKE256_STATE
#define SYMCRYPT_KMACXXX_RESULT_SIZE        SYMCRYPT_KMAC256_RESULT_SIZE
#include "kmac_pattern.c"
#undef SYMCRYPT_KMACXXX_RESULT_SIZE
#undef SYMCRYPT_CSHAKEXXX_STATE
#undef SYMCRYPT_CSHAKEXXX_INIT
#undef ALG
#undef Alg

// MAC interface
const SYMCRYPT_MAC SymCryptKmac256Algorithm_Default = {
    SymCryptKmac256ExpandKey,
    SymCryptKmac256Init,
    SymCryptKmac256Append,
    SymCryptKmac256Result,
    sizeof(SYMCRYPT_KMAC256_EXPANDED_KEY),
    sizeof(SYMCRYPT_KMAC256_STATE),
    SYMCRYPT_KMAC256_RESULT_SIZE,
    NULL,   // ppHashAlgorithm
    0,      // outerChainingStateOffset
};

const PCSYMCRYPT_MAC SymCryptKmac256Algorithm = &SymCryptKmac256Algorithm_Default;

static const BYTE kmac256KATAnswer[SYMCRYPT_KMAC256_RESULT_SIZE] = {
    0xa9, 0x1d, 0x09, 0x00, 0x71, 0x0c, 0x63, 0xc5, 0x0f, 0xb6, 0x4d, 0xfa, 0xd8, 0x75, 0x4d, 0x78,
    0x2d, 0xc0, 0x82, 0x4b, 0x87, 0x97, 0xda, 0xf2, 0x36, 0xde, 0xe9, 0x35, 0x69, 0x2e, 0x50, 0x81,
    0x0a, 0xea, 0x3b, 0x05, 0xaf, 0x1b, 0x82, 0x3b, 0xc8, 0xa1, 0x9e, 0xe9, 0x9c, 0x5f, 0xd5, 0x5a,
    0x20, 0x92, 0x89, 0x46, 0xa4, 0xe4, 0x1a, 0xdd, 0x3d, 0xb6, 0x47, 0x4d, 0xf2, 0xa5, 0xfc, 0x73
};

VOID
SYMCRYPT_CALL
SymCryptKmac256Selftest(void)
{
    BYTE result[SYMCRYPT_KMAC256_RESULT_SIZE];
    static const unsigned char Sstr[] = { 'S' };
    SYMCRYPT_KMAC256_EXPANDED_KEY expandedKey;

    SymCryptKmac256ExpandKeyEx(&expandedKey, SymCryptTestKey32, 32, Sstr, sizeof(Sstr));

    SymCryptKmac256(&expandedKey, SymCryptTestMsg16, sizeof(SymCryptTestMsg16), result);

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, kmac256KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('kmac');
    }
}
