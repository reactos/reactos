//
// srtp_kdf.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module implements SRTP-KDF specified in RFC 3711 Section 4.3.1.
//

#include "precomp.h"


#define SYMCRYPT_SRTP_KDF_SALT_SIZE (112 / 8)



SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSrtpKdfExpandKey(
    _Out_               PSYMCRYPT_SRTPKDF_EXPANDED_KEY      pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                              pbKey,
                        SIZE_T                              cbKey)
{
    return SymCryptAesExpandKeyEncryptOnly(&pExpandedKey->aesExpandedKey, pbKey, cbKey);
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSrtpKdfDerive(
    _In_                    PCSYMCRYPT_SRTPKDF_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSalt)      PCBYTE  pbSalt,
                            SIZE_T  cbSalt,
                            UINT32  uKeyDerivationRate,
                            UINT64  uIndex,
                            UINT32  uIndexWidth,
                            BYTE    label,
    _Out_writes_(cbOutput)  PBYTE   pbOutput,
                            SIZE_T  cbOutput)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    BYTE X[16] = { 0 };

    //
    // uIndexWidth must be one of 0, 32 or 48. RFC 3711 defines SRTP indices to be
    // 48-bits. SRTCP indices were first specified as 32-bit values and then updated to
    // 48-bits by Errata ID 3712. uIndexWidth parameter allows specifying the width of
    // the uIndex parameter for both SRTP and SRTCP indices. The test vectors use
    // 32-bit SRTCP index values.
    //
    // The default value of 0 is equivalent to setting uIndexWidth to 48.
    if (uIndexWidth == 0)
    {
        uIndexWidth = 48;
    }
    else if (uIndexWidth != 32 && uIndexWidth != 48)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (cbSalt != SYMCRYPT_SRTP_KDF_SALT_SIZE)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // uKeyDerivationRate must be zero or 2^i for 0 <= i <= 24.
    // This is verified by checking both it is not greater than 2^24 and it is either zero or a power of two.
    if( (uKeyDerivationRate > (1 << 24))  || ((uKeyDerivationRate & (uKeyDerivationRate - 1)) != 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Initialize X to Salt || 0
    memcpy(X, pbSalt, cbSalt);

    // (uIndex DIV uKeyDerivationRate) operation can be performed with a right shift as
    // uKeyDerivationRate is either zero or a power of 2. When uKeyDerivationRate is zero,
    // DIV operation should evaluate to zero, which can be performed by shifting uIndex by 48 bits,
    // i.e., maximum value it may have.
    UINT32 kdrShift = 48;
    if (uKeyDerivationRate)
    {
        for (UINT32 i = 0; i <= 24; i++)
        {
            if (uKeyDerivationRate == (1UL << i))
            {
                kdrShift = i;
                break;
            }
        }
    }

    UINT64 r = uIndex >> kdrShift;

    UINT64 key_id = ((UINT64)label << uIndexWidth) | r;

    // XOR key_id into salt
    //
    // X = S0 ... |S6 ... S13| 0 0
    //            |  key_id  |
    //
    PBYTE pbXorPos = &X[SYMCRYPT_SRTP_KDF_SALT_SIZE - sizeof(key_id)];
    UINT64 uSaltLsb = SYMCRYPT_LOAD_MSBFIRST64(pbXorPos);
    SYMCRYPT_STORE_MSBFIRST64(pbXorPos, uSaltLsb ^ key_id);

    //
    // We break the read-once/write once rule here by writing to the pbOutput buffer twice.
    // The first write wipes the buffer so that we get the raw keystream bytes from AES-CTR encryption.
    // The second write to pbOutput occurs with the SymCryptAesCtrMsb64() call that produces the keystream bytes.
    //
    // Modification of pbOutput between the two calls does not leak any information, it just results in flipping of the
    // corresponding bits of the correct output.
    SymCryptWipe(pbOutput, cbOutput);
    SymCryptAesCtrMsb64(&pExpandedKey->aesExpandedKey, X, pbOutput, pbOutput, cbOutput & ~0xf);

    // SymCryptAesCtrMsb64 only processes full blocks. If cbOutput is not a multiple of 16 we generate the last block of
    // keystream to local buffer and copy the necessary number of bytes to output.
    if (cbOutput & 0xf)
    {
        BYTE lastBlockBytes[16] = { 0 };

        SymCryptAesCtrMsb64(&pExpandedKey->aesExpandedKey, X, lastBlockBytes, lastBlockBytes, 16);

        memcpy(pbOutput + 16 * (cbOutput / 16), lastBlockBytes, cbOutput & 0xf);

        SymCryptWipeKnownSize(lastBlockBytes, sizeof(lastBlockBytes));
    }

cleanup:

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSrtpKdf(
    _In_reads_(cbKey)           PCBYTE  pbKey,
                                SIZE_T  cbKey,
    _In_reads_(cbSalt)          PCBYTE  pbSalt,
                                SIZE_T  cbSalt,
                                UINT32  uKeyDerivationRate,
                                UINT64  uIndex,
                                UINT32  uIndexWidth,
                                BYTE    label,
    _Out_writes_(cbOutput)      PBYTE   pbOutput,
                                SIZE_T  cbOutput)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_SRTPKDF_EXPANDED_KEY expandedKey;

    scError = SymCryptSrtpKdfExpandKey(&expandedKey, pbKey, cbKey);

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptSrtpKdfDerive(&expandedKey,
                                    pbSalt, cbSalt,
                                    uKeyDerivationRate,
                                    uIndex, uIndexWidth,
                                    label,
                                    pbOutput, cbOutput);

cleanup:

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));

    return scError;
}
