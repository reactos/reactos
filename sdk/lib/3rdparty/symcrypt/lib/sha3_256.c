//
// Sha3_256.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//


const SYMCRYPT_HASH SymCryptSha3_256Algorithm_default = {
    &SymCryptSha3_256Init,
    &SymCryptSha3_256Append,
    &SymCryptSha3_256Result,
    NULL,                           // AppendBlocks function is not implemented for SHA-3
    &SymCryptSha3_256StateCopy,
    sizeof(SYMCRYPT_SHA3_256_STATE),
    SYMCRYPT_SHA3_256_RESULT_SIZE,
    SYMCRYPT_SHA3_256_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET(SYMCRYPT_SHA3_256_STATE, ks.state),
    SYMCRYPT_FIELD_SIZE(SYMCRYPT_SHA3_256_STATE, ks.state),
};

const PCSYMCRYPT_HASH SymCryptSha3_256Algorithm = &SymCryptSha3_256Algorithm_default;


//
// SymCryptSha3_256
//
#define ALG     SHA3_256
#define Alg     Sha3_256
#include "hash_pattern.c"
#undef ALG
#undef Alg


//
// SymCryptSha3_256Init
//
VOID
SYMCRYPT_CALL
SymCryptSha3_256Init(_Out_ PSYMCRYPT_SHA3_256_STATE pState)
{
    SymCryptKeccakInit(&pState->ks,
                        SYMCRYPT_SHA3_256_INPUT_BLOCK_SIZE,
                        SYMCRYPT_SHA3_PADDING_VALUE);

    SYMCRYPT_SET_MAGIC(pState);
}


//
// SymCryptSha3_256Append
//
VOID
SYMCRYPT_CALL
SymCryptSha3_256Append(
    _Inout_             PSYMCRYPT_SHA3_256_STATE    pState,
    _In_reads_(cbData)  PCBYTE                      pbData,
                        SIZE_T                      cbData)
{
    SymCryptKeccakAppend(&pState->ks, pbData, cbData);
}


//
// SymCryptSha3_256Result
//
VOID
SYMCRYPT_CALL
SymCryptSha3_256Result(
    _Inout_                                     PSYMCRYPT_SHA3_256_STATE    pState,
    _Out_writes_(SYMCRYPT_SHA3_256_RESULT_SIZE) PBYTE                       pbResult)
{
    SymCryptKeccakExtract(&pState->ks, pbResult, SYMCRYPT_SHA3_256_RESULT_SIZE, TRUE);
}


//
// SymCryptSha3_256StateExport
//
VOID
SYMCRYPT_CALL
SymCryptSha3_256StateExport(
    _In_                                                    PCSYMCRYPT_SHA3_256_STATE   pState,
    _Out_writes_bytes_(SYMCRYPT_SHA3_256_STATE_EXPORT_SIZE) PBYTE                       pbBlob)
{
    SYMCRYPT_CHECK_MAGIC(pState);
    SymCryptKeccakStateExport(SymCryptBlobTypeSha3_256State, &pState->ks, pbBlob);
}


//
// SymCryptSha3_256StateImport
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha3_256StateImport(
    _Out_                                                   PSYMCRYPT_SHA3_256_STATE    pState,
    _In_reads_bytes_(SYMCRYPT_SHA3_256_STATE_EXPORT_SIZE)   PCBYTE                      pbBlob)
{
    SYMCRYPT_ERROR scError = SymCryptKeccakStateImport(SymCryptBlobTypeSha3_256State, &pState->ks, pbBlob);

    if (scError == SYMCRYPT_NO_ERROR)
    {
        SYMCRYPT_SET_MAGIC(pState);
    }

    return scError;
}


//
// Simple test vector for FIPS module testing
//

static const BYTE sha3_256KATAnswer[32] = {
    0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2,
    0x04, 0x5c, 0x17, 0x2d, 0x6b, 0xd3, 0x90, 0xbd,
    0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d, 0x52, 0x5b,
    0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32
};

VOID
SYMCRYPT_CALL
SymCryptSha3_256Selftest(void)
{
    BYTE result[SYMCRYPT_SHA3_256_RESULT_SIZE];

    SymCryptSha3_256(SymCryptTestMsg3, sizeof(SymCryptTestMsg3), result);

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, sha3_256KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('SHA3');
    }
}
