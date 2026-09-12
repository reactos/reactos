//
// ssh_kdf.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module implements SSH-KDF specified in RFC 4253 Section 7.2.
//

#include "precomp.h"



SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSshKdfExpandKey(
    _Out_               PSYMCRYPT_SSHKDF_EXPANDED_KEY   pExpandedKey,
    _In_                PCSYMCRYPT_HASH                 pHashFunc,
    _In_reads_(cbKey)   PCBYTE                          pbKey,
                        SIZE_T                          cbKey)
{
    pExpandedKey->pHashFunc = pHashFunc;

    SymCryptHashInit(pHashFunc, &pExpandedKey->hashState);
    SymCryptHashAppend(pHashFunc, &pExpandedKey->hashState, pbKey, cbKey);

    return SYMCRYPT_NO_ERROR;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSshKdfDerive(
    _In_                        PCSYMCRYPT_SSHKDF_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbHashValue)     PCBYTE                          pbHashValue,
                                SIZE_T                          cbHashValue,
                                BYTE                            label,
    _In_reads_(cbSessionId)     PCBYTE                          pbSessionId,
                                SIZE_T                          cbSessionId,
    _Inout_updates_(cbOutput)   PBYTE                           pbOutput,
                                SIZE_T                          cbOutput)
{
    SYMCRYPT_ERROR      scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_HASH_STATE hashState;

    PCBYTE          pcbOutputSave = pbOutput;
    PCSYMCRYPT_HASH pHashFunc = pExpandedKey->pHashFunc;
    SIZE_T          cbHashResultSize = SymCryptHashResultSize(pHashFunc);


    while (cbOutput > 0)
    {
        SIZE_T cbGeneratedOutput = pbOutput - pcbOutputSave;

        SymCryptHashStateCopy(pHashFunc, &pExpandedKey->hashState, &hashState);
        SymCryptHashAppend(pHashFunc, &hashState, pbHashValue, cbHashValue);                // hashState has (K || H)

        // label and session ID are appended only in the first iteration
        if (cbGeneratedOutput == 0)
        {
            SymCryptHashAppend(pHashFunc, &hashState, &label, 1);
            SymCryptHashAppend(pHashFunc, &hashState, pbSessionId, cbSessionId);
        }
        else
        {
            // We break the read-once write-once rule here by appending data to a
            // hash computation from pbOutput that was written by SymCryptHashResult()
            // below.
            // Modification of data in pbOutput buffer after it's written and before
            // used again will have uncontrolled disturbances in the hash output and cannot
            // be used to gain knowledge about the secret key.
            SymCryptHashAppend(pHashFunc, &hashState, pcbOutputSave, cbGeneratedOutput);    // hashState has (K || H || K1 .. Ki)
        }

        SymCryptHashResult(pHashFunc, &hashState, pbOutput, cbOutput);

        SIZE_T bytesCopied = SYMCRYPT_MIN(cbOutput, cbHashResultSize);

        pbOutput += bytesCopied;
        cbOutput -= bytesCopied;
    }

    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSshKdf(
    _In_                    PCSYMCRYPT_HASH     pHashFunc,
    _In_reads_(cbKey)       PCBYTE              pbKey,
                            SIZE_T              cbKey,
    _In_reads_(cbHashValue) PCBYTE              pbHashValue,
                            SIZE_T              cbHashValue,
                            BYTE                label,
    _In_reads_(cbSessionId) PCBYTE              pbSessionId,
                            SIZE_T              cbSessionId,
    _Out_writes_(cbOutput)  PBYTE               pbOutput,
                            SIZE_T              cbOutput)
{
    SYMCRYPT_SSHKDF_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR  scError = SYMCRYPT_NO_ERROR;

    scError = SymCryptSshKdfExpandKey(&expandedKey, pHashFunc, pbKey, cbKey);

    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptSshKdfDerive(&expandedKey,
                                    pbHashValue, cbHashValue,
                                    label,
                                    pbSessionId, cbSessionId,
                                    pbOutput, cbOutput);

 cleanup:

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));

    return scError;
}
