//
// sp800_108_hmacsha512.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// The SP800-108 SHA-384 / SHA-512 self tests
// This is in a separate module to avoid pulling in SHA-512 whenever we use SP800_108
//

static const BYTE    sp800_108_sha384Answer[] =
{
    0xc7, 0x10, 0x27, 0x87, 0xd8, 0x96, 0xbc, 0x89,
};

VOID
SYMCRYPT_CALL
SymCryptSp800_108_HmacSha384SelfTest(void)
{
    BYTE    res[sizeof(sp800_108_sha384Answer)];

    SymCryptSp800_108(
        SymCryptHmacSha384Algorithm,
        &SymCryptTestKey32[0], 8,       // key
        (PCBYTE)"Label", 5,             // label
        &SymCryptTestKey32[16], 16,     // context
        res,
        sizeof(res));

    SymCryptInjectError( res, sizeof( res ) );

    if (memcmp(res, sp800_108_sha384Answer, sizeof(res)) !=0)
    {
        SymCryptFatal('8108');
    }
}

static const BYTE    sp800_108_sha512Answer[] =
{
    0xdb, 0x3a, 0x18, 0xd9, 0x6c, 0x4a, 0xd4, 0x1e,
};

VOID
SYMCRYPT_CALL
SymCryptSp800_108_HmacSha512SelfTest(void)
{
    BYTE    res[sizeof(sp800_108_sha512Answer)];

    SymCryptSp800_108(
        SymCryptHmacSha512Algorithm,
        &SymCryptTestKey32[0], 8,       // key
        (PCBYTE)"Label", 5,             // label
        &SymCryptTestKey32[16], 16,     // context
        res,
        sizeof(res));

    SymCryptInjectError( res, sizeof( res ) );

    if (memcmp(res, sp800_108_sha512Answer, sizeof(res)) !=0)
    {
        SymCryptFatal('8108');
    }
}
