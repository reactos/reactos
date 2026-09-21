//
// Sha3_512.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//


const SYMCRYPT_HASH SymCryptSha3_512Algorithm_default = {
    &SymCryptSha3_512Init,
    &SymCryptSha3_512Append,
    &SymCryptSha3_512Result,
    NULL,                           // AppendBlocks function is not implemented for SHA-3
    &SymCryptSha3_512StateCopy,
    sizeof(SYMCRYPT_SHA3_512_STATE),
    SYMCRYPT_SHA3_512_RESULT_SIZE,
    SYMCRYPT_SHA3_512_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET(SYMCRYPT_SHA3_512_STATE, ks.state),
    SYMCRYPT_FIELD_SIZE(SYMCRYPT_SHA3_512_STATE, ks.state),
};

const PCSYMCRYPT_HASH SymCryptSha3_512Algorithm = &SymCryptSha3_512Algorithm_default;


//
// SymCryptSha3_512
//
#define ALG     SHA3_512
#define Alg     Sha3_512
#include "hash_pattern.c"
#undef ALG
#undef Alg


//
// SymCryptSha3_512Init
//
VOID
SYMCRYPT_CALL
SymCryptSha3_512Init(_Out_ PSYMCRYPT_SHA3_512_STATE pState)
{
    SymCryptKeccakInit(&pState->ks,
                        SYMCRYPT_SHA3_512_INPUT_BLOCK_SIZE,
                        SYMCRYPT_SHA3_PADDING_VALUE);

    SYMCRYPT_SET_MAGIC(pState);
}


//
// SymCryptSha3_512Append
//
VOID
SYMCRYPT_CALL
SymCryptSha3_512Append(
    _Inout_             PSYMCRYPT_SHA3_512_STATE    pState,
    _In_reads_(cbData)  PCBYTE                      pbData,
                        SIZE_T                      cbData)
{
    SymCryptKeccakAppend(&pState->ks, pbData, cbData);
}


//
// SymCryptSha3_512Result
//
VOID
SYMCRYPT_CALL
SymCryptSha3_512Result(
    _Inout_                                     PSYMCRYPT_SHA3_512_STATE    pState,
    _Out_writes_(SYMCRYPT_SHA3_512_RESULT_SIZE) PBYTE                       pbResult)
{
    SymCryptKeccakExtract(&pState->ks, pbResult, SYMCRYPT_SHA3_512_RESULT_SIZE, TRUE);
}


//
// SymCryptSha3_512StateExport
//
VOID
SYMCRYPT_CALL
SymCryptSha3_512StateExport(
    _In_                                                    PCSYMCRYPT_SHA3_512_STATE   pState,
    _Out_writes_bytes_(SYMCRYPT_SHA3_512_STATE_EXPORT_SIZE) PBYTE                       pbBlob)
{
    SYMCRYPT_CHECK_MAGIC(pState);
    SymCryptKeccakStateExport(SymCryptBlobTypeSha3_512State, &pState->ks, pbBlob);
}

//
// SymCryptSha3_512StateExport
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha3_512StateImport(
    _Out_                                                   PSYMCRYPT_SHA3_512_STATE    pState,
    _In_reads_bytes_(SYMCRYPT_SHA3_512_STATE_EXPORT_SIZE)   PCBYTE                      pbBlob)
{
    SYMCRYPT_ERROR scError = SymCryptKeccakStateImport(SymCryptBlobTypeSha3_512State, &pState->ks, pbBlob);

    if (scError == SYMCRYPT_NO_ERROR)
    {
        SYMCRYPT_SET_MAGIC(pState);
    }

    return scError;
}


//
// Simple test vector for FIPS module testing
//

static const BYTE sha3_512KATAnswer[64] = {
    0xb7, 0x51, 0x85, 0x0b, 0x1a, 0x57, 0x16, 0x8a,
    0x56, 0x93, 0xcd, 0x92, 0x4b, 0x6b, 0x09, 0x6e,
    0x08, 0xf6, 0x21, 0x82, 0x74, 0x44, 0xf7, 0x0d,
    0x88, 0x4f, 0x5d, 0x02, 0x40, 0xd2, 0x71, 0x2e,
    0x10, 0xe1, 0x16, 0xe9, 0x19, 0x2a, 0xf3, 0xc9,
    0x1a, 0x7e, 0xc5, 0x76, 0x47, 0xe3, 0x93, 0x40,
    0x57, 0x34, 0x0b, 0x4c, 0xf4, 0x08, 0xd5, 0xa5,
    0x65, 0x92, 0xf8, 0x27, 0x4e, 0xec, 0x53, 0xf0
};

VOID
SYMCRYPT_CALL
SymCryptSha3_512Selftest(void)
{
    BYTE result[SYMCRYPT_SHA3_512_RESULT_SIZE];

    SymCryptSha3_512(SymCryptTestMsg3, sizeof(SymCryptTestMsg3), result);

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, sha3_512KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('SHA3');
    }
}
