//
// Sha3_224.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//


const SYMCRYPT_HASH SymCryptSha3_224Algorithm_default = {
    &SymCryptSha3_224Init,
    &SymCryptSha3_224Append,
    &SymCryptSha3_224Result,
    NULL,                           // AppendBlocks function is not implemented for SHA-3
    &SymCryptSha3_224StateCopy,
    sizeof(SYMCRYPT_SHA3_224_STATE),
    SYMCRYPT_SHA3_224_RESULT_SIZE,
    SYMCRYPT_SHA3_224_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET(SYMCRYPT_SHA3_224_STATE, ks.state),
    SYMCRYPT_FIELD_SIZE(SYMCRYPT_SHA3_224_STATE, ks.state),
};

const PCSYMCRYPT_HASH SymCryptSha3_224Algorithm = &SymCryptSha3_224Algorithm_default;


//
// SymCryptSha3_224
//
#define ALG     SHA3_224
#define Alg     Sha3_224
#include "hash_pattern.c"
#undef ALG
#undef Alg


//
// SymCryptSha3_224Init
//
VOID
SYMCRYPT_CALL
SymCryptSha3_224Init(_Out_ PSYMCRYPT_SHA3_224_STATE pState)
{
    SymCryptKeccakInit(&pState->ks,
                        SYMCRYPT_SHA3_224_INPUT_BLOCK_SIZE,
                        SYMCRYPT_SHA3_PADDING_VALUE);

    SYMCRYPT_SET_MAGIC(pState);
}


//
// SymCryptSha3_224Append
//
VOID
SYMCRYPT_CALL
SymCryptSha3_224Append(
    _Inout_             PSYMCRYPT_SHA3_224_STATE    pState,
    _In_reads_(cbData)  PCBYTE                      pbData,
                        SIZE_T                      cbData)
{
    SymCryptKeccakAppend(&pState->ks, pbData, cbData);
}


//
// SymCryptSha3_224Result
//
VOID
SYMCRYPT_CALL
SymCryptSha3_224Result(
    _Inout_                                     PSYMCRYPT_SHA3_224_STATE    pState,
    _Out_writes_(SYMCRYPT_SHA3_224_RESULT_SIZE) PBYTE                       pbResult)
{
    SymCryptKeccakExtract(&pState->ks, pbResult, SYMCRYPT_SHA3_224_RESULT_SIZE, TRUE);
}


//
// SymCryptSha3_224StateExport
//
VOID
SYMCRYPT_CALL
SymCryptSha3_224StateExport(
    _In_                                                    PCSYMCRYPT_SHA3_224_STATE   pState,
    _Out_writes_bytes_(SYMCRYPT_SHA3_224_STATE_EXPORT_SIZE) PBYTE                       pbBlob)
{
    SYMCRYPT_CHECK_MAGIC(pState);
    SymCryptKeccakStateExport(SymCryptBlobTypeSha3_224State, &pState->ks, pbBlob);
}


//
// SymCryptSha3_224StateImport
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha3_224StateImport(
    _Out_                                                   PSYMCRYPT_SHA3_224_STATE    pState,
    _In_reads_bytes_(SYMCRYPT_SHA3_224_STATE_EXPORT_SIZE)   PCBYTE                      pbBlob)
{
    SYMCRYPT_ERROR scError = SymCryptKeccakStateImport(SymCryptBlobTypeSha3_224State, &pState->ks, pbBlob);

    if (scError == SYMCRYPT_NO_ERROR)
    {
        SYMCRYPT_SET_MAGIC(pState);
    }

    return scError;
}


//
// Simple test vector for FIPS module testing
//

static const BYTE sha3_224KATAnswer[28] = {
    0xe6, 0x42, 0x82, 0x4c, 0x3f, 0x8c, 0xf2, 0x4a,
    0xd0, 0x92, 0x34, 0xee, 0x7d, 0x3c, 0x76, 0x6f,
    0xc9, 0xa3, 0xa5, 0x16, 0x8d, 0x0c, 0x94, 0xad,
    0x73, 0xb4, 0x6f, 0xdf,
};

VOID
SYMCRYPT_CALL
SymCryptSha3_224Selftest(void)
{
    BYTE result[SYMCRYPT_SHA3_224_RESULT_SIZE];

    SymCryptSha3_224(SymCryptTestMsg3, sizeof(SymCryptTestMsg3), result);

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, sha3_224KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('SHA3');
    }
}
