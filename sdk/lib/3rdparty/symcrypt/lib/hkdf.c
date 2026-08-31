//
// hkdf.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement the HKDF
// function for the TLS protocol 1.3. It is used in
// the protocol's key derivation function.
//
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdfExpandKey(
    _Out_                   PSYMCRYPT_HKDF_EXPANDED_KEY     pExpandedKey,
    _In_                    PCSYMCRYPT_MAC                  macAlgorithm,
    _In_reads_(cbIkm)       PCBYTE                          pbIkm,
                            SIZE_T                          cbIkm,
    _In_reads_opt_(cbSalt)  PCBYTE                          pbSalt,
                            SIZE_T                          cbSalt )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_ALIGN BYTE rbPrk[SYMCRYPT_MAC_MAX_RESULT_SIZE] = { 0 };

    SYMCRYPT_ASSERT( macAlgorithm->expandedKeySize <= sizeof( pExpandedKey->macKey ) );

    scError = SymCryptHkdfExtractPrk( macAlgorithm, pbIkm, cbIkm, pbSalt, cbSalt, rbPrk, macAlgorithm->resultSize );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    scError = SymCryptHkdfPrkExpandKey( pExpandedKey, macAlgorithm, rbPrk, macAlgorithm->resultSize );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:
    SymCryptWipeKnownSize(&rbPrk[0], sizeof(rbPrk));

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdfExtractPrk(
    _In_                    PCSYMCRYPT_MAC                  macAlgorithm,
    _In_reads_(cbIkm)       PCBYTE                          pbIkm,
                            SIZE_T                          cbIkm,
    _In_reads_opt_(cbSalt)  PCBYTE                          pbSalt,
                            SIZE_T                          cbSalt,
    _Out_writes_(cbPrk)     PBYTE                           pbPrk,
                            SIZE_T                          cbPrk )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_MAC_STATE state;
    SYMCRYPT_MAC_EXPANDED_KEY key;

    // Ensure that pbPrk is the correct size
    if (cbPrk != macAlgorithm->resultSize)
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Calculation of PRK = HMAC-Hash(salt, IKM)
    scError = macAlgorithm->expandKeyFunc( &key, pbSalt, cbSalt );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    macAlgorithm->initFunc( &state, &key );
    macAlgorithm->appendFunc( &state, pbIkm, cbIkm );
    macAlgorithm->resultFunc( &state, pbPrk );

cleanup:
    SymCryptWipeKnownSize(&key, sizeof(key));

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdfPrkExpandKey(
    _Out_                   PSYMCRYPT_HKDF_EXPANDED_KEY     pExpandedKey,
    _In_                    PCSYMCRYPT_MAC                  macAlgorithm,
    _In_reads_(cbPrk)       PCBYTE                          pbPrk,
                            SIZE_T                          cbPrk )
{
    SYMCRYPT_ASSERT( macAlgorithm->expandedKeySize <= sizeof( pExpandedKey->macKey ) );

    pExpandedKey->macAlg = macAlgorithm;
    return macAlgorithm->expandKeyFunc( &pExpandedKey->macKey, pbPrk, cbPrk );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdfDerive(
    _In_                    PCSYMCRYPT_HKDF_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbInfo)  PCBYTE                          pbInfo,
                            SIZE_T                          cbInfo,
    _Out_writes_(cbResult)  PBYTE                           pbResult,
                            SIZE_T                          cbResult)
{

    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    SYMCRYPT_MAC_STATE state;

    PCSYMCRYPT_MAC pMacAlgorithm = pExpandedKey->macAlg;

    SYMCRYPT_ALIGN BYTE    rbPartialResult[SYMCRYPT_MAC_MAX_RESULT_SIZE];
                   BYTE *  pbCurr = pbResult;

    SIZE_T  cbMacResultSize = pMacAlgorithm->resultSize;

    BYTE cntr = 0x01;

    // Check that cbResult <= 255*HashLen
    if (cbResult > 0xff * cbMacResultSize)
    {
        scError = SYMCRYPT_WRONG_DATA_SIZE;
        goto cleanup;
    }

    // In the first iteration T(0) is the empty string
    // Calculate T(1) = HMAC-Hash(PRK, T(0) | info | 0x01)
    pMacAlgorithm->initFunc( &state, pExpandedKey );
    pMacAlgorithm->appendFunc( &state, pbInfo, cbInfo );
    pMacAlgorithm->appendFunc( &state, &cntr, sizeof(cntr) );
    pMacAlgorithm->resultFunc( &state, rbPartialResult );

    // Store the result in the output buffer
    memcpy(pbCurr, rbPartialResult, SYMCRYPT_MIN(cbResult, cbMacResultSize));
    if (cbResult <= cbMacResultSize)
    {
        goto cleanup;
    }

    // Update counters
    cntr++;
    pbCurr += cbMacResultSize;
    cbResult -= cbMacResultSize;

    while( cbResult > 0 )
    {
        // Calculate T(i) = HMAC-Hash(PRK, T(i-1) | info | 0xi)
        pMacAlgorithm->initFunc( &state, pExpandedKey );
        pMacAlgorithm->appendFunc( &state, rbPartialResult, cbMacResultSize );
        pMacAlgorithm->appendFunc( &state, pbInfo, cbInfo );
        pMacAlgorithm->appendFunc( &state, &cntr, sizeof(cntr) );
        pMacAlgorithm->resultFunc( &state, rbPartialResult );

        // Store the result in the output buffer
        memcpy(pbCurr, rbPartialResult, SYMCRYPT_MIN(cbResult, cbMacResultSize));
        if (cbResult <= cbMacResultSize)
        {
            goto cleanup;
        }

        // Update counters
        cntr++;
        pbCurr += cbMacResultSize;
        cbResult -= cbMacResultSize;
    }

cleanup:
    SymCryptWipeKnownSize(&rbPartialResult[0], sizeof(rbPartialResult));

    return scError;
}

//
// The full HKDF
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdf(
                            PCSYMCRYPT_MAC  macAlgorithm,
    _In_reads_(cbIkm)       PCBYTE          pbIkm,
                            SIZE_T          cbIkm,
    _In_reads_opt_(cbSalt)  PCBYTE          pbSalt,
                            SIZE_T          cbSalt,
    _In_reads_opt_(cbInfo)  PCBYTE          pbInfo,
                            SIZE_T          cbInfo,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_HKDF_EXPANDED_KEY key;

    // Create the expanded key
    scError = SymCryptHkdfExpandKey(
        &key,
        macAlgorithm,
        pbIkm,
        cbIkm,
        pbSalt,
        cbSalt );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

    // Derive the key
    scError = SymCryptHkdfDerive(
        &key,
        pbInfo,
        cbInfo,
        pbResult,
        cbResult );
    if (scError != SYMCRYPT_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:
    SymCryptWipeKnownSize(&key, sizeof(key));

    return scError;
}
