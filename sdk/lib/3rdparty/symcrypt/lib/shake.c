//
// Shake.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//



//
//  SHAKE128
//
#define Alg Shake128
#define ALG SHAKE128
#define SYMCRYPT_SHAKEXXX_INPUT_BLOCK_SIZE      SYMCRYPT_SHAKE128_INPUT_BLOCK_SIZE
#define SYMCRYPT_SHAKEXXX_RESULT_SIZE           SYMCRYPT_SHAKE128_RESULT_SIZE
#include "shake_pattern.c"
#undef SYMCRYPT_SHAKEXXX_RESULT_SIZE
#undef SYMCRYPT_SHAKEXXX_INPUT_BLOCK_SIZE
#undef ALG
#undef Alg

const SYMCRYPT_HASH SymCryptShake128HashAlgorithm_default = {
    &SymCryptShake128Init,
    &SymCryptShake128Append,
    &SymCryptShake128Result,
    NULL,                           // AppendBlocks function is not implemented for SHA-3
    &SymCryptShake128StateCopy,
    sizeof(SYMCRYPT_SHAKE128_STATE),
    SYMCRYPT_SHAKE128_RESULT_SIZE,
    SYMCRYPT_SHAKE128_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET(SYMCRYPT_SHAKE128_STATE, ks.state),
    SYMCRYPT_FIELD_SIZE(SYMCRYPT_SHAKE128_STATE, ks.state),
};

const PCSYMCRYPT_HASH SymCryptShake128HashAlgorithm = &SymCryptShake128HashAlgorithm_default;

static const BYTE shake128KATAnswer[SYMCRYPT_SHAKE128_RESULT_SIZE] = {
    0x58, 0x81, 0x09, 0x2d, 0xd8, 0x18, 0xbf, 0x5c,
    0xf8, 0xa3, 0xdd, 0xb7, 0x93, 0xfb, 0xcb, 0xa7,
    0x40, 0x97, 0xd5, 0xc5, 0x26, 0xa6, 0xd3, 0x5f,
    0x97, 0xb8, 0x33, 0x51, 0x94, 0x0f, 0x2c ,0xc8
};

VOID
SYMCRYPT_CALL
SymCryptShake128Selftest(void)
{
    BYTE result[SYMCRYPT_SHAKE128_RESULT_SIZE];

    SymCryptShake128(SymCryptTestMsg3, sizeof(SymCryptTestMsg3), result, sizeof(result));

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, shake128KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('shk1');
    }
}


//
//  SHAKE256
//
#define Alg Shake256
#define ALG SHAKE256
#define SYMCRYPT_SHAKEXXX_INPUT_BLOCK_SIZE      SYMCRYPT_SHAKE256_INPUT_BLOCK_SIZE
#define SYMCRYPT_SHAKEXXX_RESULT_SIZE           SYMCRYPT_SHAKE256_RESULT_SIZE
#include "shake_pattern.c"
#undef SYMCRYPT_SHAKEXXX_RESULT_SIZE
#undef SYMCRYPT_SHAKEXXX_INPUT_BLOCK_SIZE
#undef ALG
#undef Alg

const SYMCRYPT_HASH SymCryptShake256HashAlgorithm_default = {
    &SymCryptShake256Init,
    &SymCryptShake256Append,
    &SymCryptShake256Result,
    NULL,                           // AppendBlocks function is not implemented for SHA-3
    &SymCryptShake256StateCopy,
    sizeof(SYMCRYPT_SHAKE256_STATE),
    SYMCRYPT_SHAKE256_RESULT_SIZE,
    SYMCRYPT_SHAKE256_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET(SYMCRYPT_SHAKE256_STATE, ks.state),
    SYMCRYPT_FIELD_SIZE(SYMCRYPT_SHAKE256_STATE, ks.state),
};

const PCSYMCRYPT_HASH SymCryptShake256HashAlgorithm = &SymCryptShake256HashAlgorithm_default;

static const BYTE shake256KATAnswer[SYMCRYPT_SHAKE256_RESULT_SIZE] = {
    0x48, 0x33, 0x66, 0x60, 0x13, 0x60, 0xa8, 0x77, 0x1c, 0x68, 0x63, 0x08, 0x0c, 0xc4, 0x11, 0x4d,
    0x8d, 0xb4, 0x45, 0x30, 0xf8, 0xf1, 0xe1, 0xee, 0x4f, 0x94, 0xea, 0x37, 0xe7, 0x8b, 0x57, 0x39,
    0xd5, 0xa1, 0x5b, 0xef, 0x18, 0x6a, 0x53, 0x86, 0xc7, 0x57, 0x44, 0xc0, 0x52, 0x7e, 0x1f, 0xaa,
    0x9f, 0x87, 0x26, 0xe4, 0x62, 0xa1, 0x2a, 0x4f, 0xeb, 0x06, 0xbd, 0x88, 0x01, 0xe7, 0x51, 0xe4
};

VOID
SYMCRYPT_CALL
SymCryptShake256Selftest(void)
{
    BYTE result[SYMCRYPT_SHAKE256_RESULT_SIZE];

    SymCryptShake256(SymCryptTestMsg3, sizeof(SymCryptTestMsg3), result, sizeof(result));

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, shake256KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('shk2');
    }
}


//
//  CSHAKE128
//
#define Alg CShake128
#define ALG CSHAKE128
#define SYMCRYPT_SHAKEXXX_INIT                  SymCryptShake128Init
#define SYMCRYPT_SHAKEXXX_STATE                 SYMCRYPT_SHAKE128_STATE
#define SYMCRYPT_CSHAKEXXX_INPUT_BLOCK_SIZE     SYMCRYPT_CSHAKE128_INPUT_BLOCK_SIZE
#define SYMCRYPT_CSHAKEXXX_RESULT_SIZE          SYMCRYPT_CSHAKE128_RESULT_SIZE
#include "cshake_pattern.c"
#undef SYMCRYPT_CSHAKEXXX_RESULT_SIZE
#undef SYMCRYPT_CSHAKEXXX_INPUT_BLOCK_SIZE
#undef SYMCRYPT_SHAKEXXX_STATE
#undef SYMCRYPT_SHAKEXXX_INIT
#undef ALG
#undef Alg


static const BYTE cshake128KATAnswer[SYMCRYPT_CSHAKE128_RESULT_SIZE] = {
    0x14, 0xe5, 0xdf, 0xf3, 0xae, 0xfd, 0xfe, 0x8e,
    0xa6, 0xae, 0xed, 0xfd, 0x99, 0xe6, 0x84, 0x74,
    0xbc, 0x61, 0xb9, 0xd6, 0x17, 0x4e, 0x9f, 0x4a,
    0xe3, 0xbd, 0x87, 0xdf, 0x0e, 0xf2, 0x16, 0xdb,
};

VOID
SYMCRYPT_CALL
SymCryptCShake128Selftest(void)
{
    BYTE result[SYMCRYPT_CSHAKE128_RESULT_SIZE];
    static const unsigned char Nstr[] = { 'N' };
    static const unsigned char Sstr[] = { 'S' };

    SymCryptCShake128(  Nstr, sizeof(Nstr),
                        Sstr, sizeof(Sstr),
                        SymCryptTestMsg3, sizeof(SymCryptTestMsg3),
                        result, sizeof(result));

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, cshake128KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('cshk');
    }
}


//
//  CSHAKE256
//
#define Alg CShake256
#define ALG CSHAKE256
#define SYMCRYPT_SHAKEXXX_INIT                  SymCryptShake256Init
#define SYMCRYPT_SHAKEXXX_STATE                 SYMCRYPT_SHAKE256_STATE
#define SYMCRYPT_CSHAKEXXX_INPUT_BLOCK_SIZE     SYMCRYPT_CSHAKE256_INPUT_BLOCK_SIZE
#define SYMCRYPT_CSHAKEXXX_RESULT_SIZE          SYMCRYPT_CSHAKE256_RESULT_SIZE
#include "cshake_pattern.c"
#undef SYMCRYPT_CSHAKEXXX_RESULT_SIZE
#undef SYMCRYPT_CSHAKEXXX_INPUT_BLOCK_SIZE
#undef SYMCRYPT_SHAKEXXX_STATE
#undef SYMCRYPT_SHAKEXXX_INIT
#undef ALG
#undef Alg


static const BYTE cshake256KATAnswer[SYMCRYPT_CSHAKE256_RESULT_SIZE] = {
    0x4d, 0xe8, 0x71, 0x6c, 0x4a, 0x16, 0x7e, 0x28, 0x2c, 0x18, 0xc5, 0x1e, 0xed, 0xa6, 0x00, 0xb8,
    0x91, 0x92, 0x4f, 0xea, 0x2e, 0x20, 0x7f, 0x71, 0x2c, 0xfd, 0xe2, 0x95, 0xfd, 0x1c, 0x67, 0x32,
    0x31, 0x49, 0x98, 0x23, 0xc0, 0x5e, 0x6a, 0xe3, 0x89, 0xad, 0x4d, 0xa2, 0x32, 0x9c, 0xc9, 0x2e,
    0x0f, 0xd6, 0x90, 0xb9, 0xee, 0x91, 0x0e, 0x86, 0xf7, 0x1d, 0x03, 0x88, 0xb5, 0x95, 0x61, 0x95
};

VOID
SYMCRYPT_CALL
SymCryptCShake256Selftest(void)
{
    BYTE result[SYMCRYPT_CSHAKE256_RESULT_SIZE];
    static const unsigned char Nstr[] = { 'N' };
    static const unsigned char Sstr[] = { 'S' };

    SymCryptCShake256(Nstr, sizeof(Nstr),
        Sstr, sizeof(Sstr),
        SymCryptTestMsg3, sizeof(SymCryptTestMsg3),
        result, sizeof(result));

    SymCryptInjectError(result, sizeof(result));

    if (memcmp(result, cshake256KATAnswer, sizeof(result)) != 0)
    {
        SymCryptFatal('cshk');
    }
}

//
// CShake helper functions
//

//
// SymCryptCShakeEncodeInputStrings
//
VOID
SYMCRYPT_CALL
SymCryptCShakeEncodeInputStrings(
    _Inout_                             PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_( cbFunctionNameString )  PCBYTE                  pbFunctionNameString,
                                        SIZE_T                  cbFunctionNameString,
    _In_reads_( cbCustomizationString ) PCBYTE                  pbCustomizationString,
                                        SIZE_T                  cbCustomizationString)
{
    SYMCRYPT_ASSERT((cbFunctionNameString > 0) || (cbCustomizationString > 0));

    // left_encode( inputBlockSize ) for byte_pad function
    //
    // SymCryptKeccakEncodeTimes8 function encodes 8 times the value passed to
    // it. Here, we want the actual value of pState->inputBlockSize to be encoded,
    // hence the division by 8.
    SymCryptKeccakAppendEncodeTimes8(pState, pState->inputBlockSize / 8, TRUE);

    SymCryptKeccakAppendEncodedString(pState, pbFunctionNameString, cbFunctionNameString);
    SymCryptKeccakAppendEncodedString(pState, pbCustomizationString, cbCustomizationString);

    // Appending of Customization String may have already called the permutation
    // if the appended data is aligned to input block size, in which case the zero
    // padding has been done.
    if (pState->stateIndex != 0)
    {
        SymCryptKeccakZeroAppendBlock(pState);
    }
}

//
// SymCryptKeccakEncodeTimes8
//
SIZE_T
SYMCRYPT_CALL
SymCryptKeccakEncodeTimes8(
                            UINT64  uInput,
    _Out_writes_(cbOutput)  PBYTE   pbOutput,
                            SIZE_T  cbOutput,
                            BOOLEAN bLeftEncode)
{
    BYTE encoding[1 + sizeof(UINT64)];
    SIZE_T ret = 0;

    // longest encoding is 1 byte for length + 9 bytes for uInput * 8
    SYMCRYPT_ASSERT(cbOutput >= (1 + sizeof(encoding)));
    UNREFERENCED_PARAMETER(cbOutput);

    //
    // encoding[0] .. encoding[8] will contain (uInput * 8) in big endian form
    encoding[0] = (BYTE)(uInput >> 61);
    SYMCRYPT_STORE_MSBFIRST64(&encoding[1], uInput * 8);

    SIZE_T length = 1;                              // number of bytes required to encode uInput
    PCBYTE pbMsb = &encoding[sizeof(encoding) - 1]; // pointer to the most significant byte

    // Locate the most significant non-zero byte
    for (int i = 0; i < sizeof(encoding); i++)
    {
        // Do not early terminate on the most significant byte
        if (encoding[i] != 0 && length == 1)
        {
            length = sizeof(encoding) - i;
            pbMsb = &encoding[i];
        }
    }

    ret = 1 + length;

    if (bLeftEncode)
    {
        // length for left_encode
        *pbOutput++ = (BYTE)length;
    }

    memcpy(pbOutput, pbMsb, length);

    if(!bLeftEncode)
    {
        // length for right_encode
        pbOutput[length] = (BYTE)length;
    }

    return ret; // total number of bytes written to pbOutput
}

//
// SymCryptKeccakAppendEncodeTimes8
//
VOID
SYMCRYPT_CALL
SymCryptKeccakAppendEncodeTimes8(
    _Inout_ SYMCRYPT_KECCAK_STATE *pState,
            UINT64  uValue,
            BOOLEAN bLeftEncode)

{
    BYTE encoding[1 + (1 + sizeof(UINT64))];
    SIZE_T ret;

    ret = SymCryptKeccakEncodeTimes8(uValue, encoding, sizeof(encoding), bLeftEncode);

    SymCryptKeccakAppend(pState, encoding, ret);
}


//
// SymCryptKeccakAppendEncodedString
//
VOID
SYMCRYPT_CALL
SymCryptKeccakAppendEncodedString(
    _Inout_                 PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_(cbString)    PCBYTE                  pbString,
                            SIZE_T                  cbString)
{
    SymCryptKeccakAppendEncodeTimes8(pState, cbString, TRUE);
    SymCryptKeccakAppend(pState, pbString, cbString);
}
