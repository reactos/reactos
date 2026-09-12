//
// pbkdf2_hmacsha256.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// The PBKDF SHA-256 test
// This is in a separate module to avoid pulling in SHA-256 whenever we use PBKDF
//

static const UINT64 pbkdf2_IterationCnt = 5;

static const BYTE    pbkdf2_sha256Answer[] =
{
    0x05, 0x98, 0x1e, 0x89, 0x48, 0xd2, 0x84, 0x61,
};

VOID
SYMCRYPT_CALL
SymCryptPbkdf2_HmacSha256SelfTest(void)
{
    BYTE    res[sizeof(pbkdf2_sha256Answer)];

    SymCryptPbkdf2(
        SymCryptHmacSha256Algorithm,
        &SymCryptTestKey32[0], 8,
        &SymCryptTestKey32[16], 16,
        pbkdf2_IterationCnt,
        res,
        sizeof(res));

    SymCryptInjectError( res, sizeof( res ) );

    if (memcmp(res, pbkdf2_sha256Answer, sizeof(res)) !=0)
    {
        SymCryptFatal('Pbk2');
    }
}
