//
// pbkdf2_hmacsha1.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// The PBKDF SHA-1 test
// This is in a separate module to avoid pulling in SHA-1 whenever we use PBKDF
//

static const UINT64 pbkdf2_IterationCnt = 5;

static const BYTE    pbkdf2_sha1Answer[] =
{
    0xef, 0xa9, 0xbf, 0xea, 0xa3, 0x4d, 0x70, 0x64,
};

VOID
SYMCRYPT_CALL
SymCryptPbkdf2_HmacSha1SelfTest(void)
{
    BYTE    res[sizeof(pbkdf2_sha1Answer)];

    SymCryptPbkdf2(
        SymCryptHmacSha1Algorithm,
        &SymCryptTestKey32[0], 8,
        &SymCryptTestKey32[16], 16,
        pbkdf2_IterationCnt,
        res,
        sizeof(res));

    SymCryptInjectError( res, sizeof( res ) );

    if (memcmp(res, pbkdf2_sha1Answer, sizeof(res)) !=0)
    {
        SymCryptFatal('Pbk2');
    }
}
