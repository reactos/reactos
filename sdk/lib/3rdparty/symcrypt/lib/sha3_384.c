//
// Sha3_384.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//


const SYMCRYPT_HASH SymCryptSha3_384Algorithm_default = {
    &SymCryptSha3_384Init,
    &SymCryptSha3_384Append,
    &SymCryptSha3_384Result,
    NULL,                           // AppendBlocks function is not implemented for SHA-3
    &SymCryptSha3_384StateCopy,
    sizeof(SYMCRYPT_SHA3_384_STATE),
    SYMCRYPT_SHA3_384_RESULT_SIZE,
    SYMCRYPT_SHA3_384_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET(SYMCRYPT_SHA3_384_STATE, ks.state),
    SYMCRYPT_FIELD_SIZE(SYMCRYPT_SHA3_384_STATE, ks.state),
};

const PCSYMCRYPT_HASH SymCryptSha3_384Algorithm = &SymCryptSha3_384Algorithm_default;


//
// SymCryptSha3_384
//
#define ALG     SHA3_384
#define Alg     Sha3_384
#include "hash_pattern.c"
#undef ALG
#undef Alg


//
// SymCryptSha3_384Init
//
VOID
SYMCRYPT_CALL
SymCryptSha3_384Init(_Out_ PSYMCRYPT_SHA3_384_STATE pState)
{
    SymCryptKeccakInit(&pState->ks,
                        SYMCRYPT_SHA3_384_INPUT_BLOCK_SIZE,
                        SYMCRYPT_SHA3_PADDING_VALUE);

    SYMCRYPT_SET_MAGIC(pState);
}


//
// SymCryptSha3_384Append
//
VOID
SYMCRYPT_CALL
SymCryptSha3_384Append(
    _Inout_             PSYMCRYPT_SHA3_384_STATE    pState,
    _In_reads_(cbData)  PCBYTE                      pbData,
                        SIZE_T                      cbData)
{
    SymCryptKeccakAppend(&pState->ks, pbData, cbData);
}


//
// SymCryptSha3_384Result
//
VOID
SYMCRYPT_CALL
SymCryptSha3_384Result(
    _Inout_                                     PSYMCRYPT_SHA3_384_STATE    pState,
    _Out_writes_(SYMCRYPT_SHA3_384_RESULT_SIZE) PBYTE                       pbResult)
{
    SymCryptKeccakExtract(&pState->ks, pbResult, SYMCRYPT_SHA3_384_RESULT_SIZE, TRUE);
}


//
// SymCryptSha3_384StateExport
//
VOID
SYMCRYPT_CALL
SymCryptSha3_384StateExport(
    _In_                                                    PCSYMCRYPT_SHA3_384_STATE   pState,
    _Out_writes_bytes_(SYMCRYPT_SHA3_384_STATE_EXPORT_SIZE) PBYTE                       pbBlob)
{
    SYMCRYPT_CHECK_MAGIC(pState);
    SymCryptKeccakStateExport(SymCryptBlobTypeSha3_384State, &pState->ks, pbBlob);
}


//
// SymCryptSha3_384StateImport
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha3_384StateImport(
    _Out_                                                   PSYMCRYPT_SHA3_384_STATE    pState,
    _In_reads_bytes_(SYMCRYPT_SHA3_384_STATE_EXPORT_SIZE)   PCBYTE                      pbBlob)
{
    SYMCRYPT_ERROR scError = SymCryptKeccakStateImport(SymCryptBlobTypeSha3_384State, &pState->ks, pbBlob);

    if (scError == SYMCRYPT_NO_ERROR)
    {
        SYMCRYPT_SET_MAGIC(pState);
    }

    return scError;
}


//
// Simple test vector for FIPS module testing
//

static const BYTE sha3_384KATAnswer[48] = {
    0xec, 0x01, 0x49, 0x82, 0x88, 0x51, 0x6f, 0xc9,
    0x26, 0x45, 0x9f, 0x58, 0xe2, 0xc6, 0xad, 0x8d,
    0xf9, 0xb4, 0x73, 0xcb, 0x0f, 0xc0, 0x8c, 0x25,
    0x96, 0xda, 0x7c, 0xf0, 0xe4, 0x9b, 0xe4, 0xb2,
    0x98, 0xd8, 0x8c, 0xea, 0x92, 0x7a, 0xc7, 0xf5,
    0x39, 0xf1, 0xed, 0xf2, 0x28, 0x37, 0x6d, 0x25
};

VOID
SYMCRYPT_CALL
SymCryptSha3_384Selftest(void)
{
    BYTE result[SYMCRYPT_SHA3_384_RESULT_SIZE];

    SymCryptSha3_384(SymCryptTestMsg3, sizeof(SymCryptTestMsg3), result);

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, sha3_384KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('SHA3');
    }
}
