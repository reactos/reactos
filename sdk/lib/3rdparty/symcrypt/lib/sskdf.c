//
// sskdf.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module implements Single-Step KDF as specified in SP800-56C section 4.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//


#define SYMCRYPT_SSKDF_KMAC_128_DEFAULT_SALT_SIZE (164)
#define SYMCRYPT_SSKDF_KMAC_256_DEFAULT_SALT_SIZE (132)
#define SYMCRYPT_SSKDF_DEFAULT_SALT_MAX SYMCRYPT_SSKDF_KMAC_128_DEFAULT_SALT_SIZE


static const BYTE   pbKmacCustomizationString[3] = { 'K', 'D', 'F' };

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSskdfMacExpandSalt(
    _Out_                   PSYMCRYPT_SSKDF_MAC_EXPANDED_SALT   pExpandedSalt,
    _In_                    PCSYMCRYPT_MAC                      macAlgorithm,
    _In_reads_opt_(cbSalt)  PCBYTE                              pbSalt,
                            SIZE_T                              cbSalt)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    const BYTE pbSaltDefault[SYMCRYPT_SSKDF_DEFAULT_SALT_MAX] = { 0 };

    SYMCRYPT_ASSERT( macAlgorithm->expandedKeySize <= sizeof( pExpandedSalt->macKey ) );

    pExpandedSalt->macAlg = macAlgorithm;

    if ( pbSalt == NULL )
    {
        if ( macAlgorithm == SymCryptKmac128Algorithm )
        {
            cbSalt = SYMCRYPT_SSKDF_KMAC_128_DEFAULT_SALT_SIZE;
        }
        else if ( macAlgorithm == SymCryptKmac256Algorithm )
        {
            cbSalt = SYMCRYPT_SSKDF_KMAC_256_DEFAULT_SALT_SIZE;
        }
        else
        {
            cbSalt = SymCryptHashInputBlockSize( *(macAlgorithm->ppHashAlgorithm) );
        }

        pbSalt = pbSaltDefault;
    }

    if ( macAlgorithm == SymCryptKmac128Algorithm )
    {
        scError = SymCryptKmac128ExpandKeyEx(
            &pExpandedSalt->macKey.kmac128Key,
            pbSalt,
            cbSalt,
            pbKmacCustomizationString,
            sizeof( pbKmacCustomizationString ) );
    }
    else if ( macAlgorithm == SymCryptKmac256Algorithm )
    {
        scError = SymCryptKmac256ExpandKeyEx(
            &pExpandedSalt->macKey.kmac256Key,
            pbSalt,
            cbSalt,
            pbKmacCustomizationString,
            sizeof( pbKmacCustomizationString ) );
    }
    else
    {
        scError = macAlgorithm->expandKeyFunc( &pExpandedSalt->macKey, pbSalt, cbSalt );
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSskdfMacDerive(
    _In_                    PCSYMCRYPT_SSKDF_MAC_EXPANDED_SALT  pExpandedSalt,
                            SIZE_T                              cbMacOutputSize,
    _In_reads_(cbSecret)    PCBYTE                              pbSecret,
                            SIZE_T                              cbSecret,
    _In_reads_opt_(cbInfo)  PCBYTE                              pbInfo,
                            SIZE_T                              cbInfo,
    _Out_writes_(cbResult)  PBYTE                               pbResult,
                            SIZE_T                              cbResult)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_MAC_STATE state;
    PCSYMCRYPT_MAC pMacAlgorithm = pExpandedSalt->macAlg;
    PSYMCRYPT_MAC_RESULT_EX resultFuncEx = NULL;

    SYMCRYPT_ALIGN BYTE rbPartialResult[SYMCRYPT_MAC_MAX_RESULT_SIZE];
    PBYTE pbCurr = pbResult;

    SIZE_T cbMacResultSize = pMacAlgorithm->resultSize;
    SIZE_T cbBlock;
    UINT32 cntr = 1;
    BYTE ctrBuf[4];

    if ( cbMacOutputSize > 64 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if ( pMacAlgorithm == SymCryptKmac128Algorithm )
    {
        cbMacResultSize = cbMacOutputSize;
        resultFuncEx = SymCryptKmac128ResultEx;
    }
    else if ( pMacAlgorithm == SymCryptKmac256Algorithm )
    {
        cbMacResultSize = cbMacOutputSize;
        resultFuncEx = SymCryptKmac256ResultEx;
    }
    else if ( cbMacOutputSize > 0 && cbMacOutputSize != pMacAlgorithm->resultSize )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    while ( cbResult > 0 )
    {
        SYMCRYPT_STORE_MSBFIRST32( ctrBuf, cntr );

        // Calculate K(i) = H(counter || Z || FixedInfo)
        pMacAlgorithm->initFunc( &state, &pExpandedSalt->macKey );
        pMacAlgorithm->appendFunc( &state, ctrBuf, sizeof( ctrBuf ) );
        pMacAlgorithm->appendFunc( &state, pbSecret, cbSecret );
        pMacAlgorithm->appendFunc( &state, pbInfo, cbInfo );

        cbBlock = SYMCRYPT_MIN( cbResult, cbMacResultSize );

        if ( resultFuncEx != NULL )
        {
            if ( cbMacOutputSize > 0 )
            {
                resultFuncEx( &state, rbPartialResult, cbMacOutputSize );
            }
            else
            {
                // If the output size is not specified, calculate the full result
                resultFuncEx( &state, pbResult, cbResult );
                break;
            }
        }
        else
        {
            pMacAlgorithm->resultFunc( &state, rbPartialResult );
        }

        // Store the result in the output buffer
        memcpy( pbCurr, rbPartialResult, cbBlock );

        // Update counters
        cntr++;
        pbCurr += cbBlock;
        cbResult -= cbBlock;
    }

cleanup:

    SymCryptWipeKnownSize( &rbPartialResult[0], sizeof( rbPartialResult ) );

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSskdfMac(
    _In_                    PCSYMCRYPT_MAC  macAlgorithm,
                            SIZE_T          cbMacOutputSize,
    _In_reads_(cbSecret)    PCBYTE          pbSecret,
                            SIZE_T          cbSecret,
    _In_reads_opt_(cbSalt)  PCBYTE          pbSalt,
                            SIZE_T          cbSalt,
    _In_reads_opt_(cbInfo)  PCBYTE          pbInfo,
                            SIZE_T          cbInfo,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult)
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_SSKDF_MAC_EXPANDED_SALT expandedSalt;

    scError = SymCryptSskdfMacExpandSalt( &expandedSalt, macAlgorithm, pbSalt, cbSalt );

    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptSskdfMacDerive(
        &expandedSalt,
        cbMacOutputSize,
        pbSecret,
        cbSecret,
        pbInfo,
        cbInfo,
        pbResult,
        cbResult );

cleanup:

    SymCryptWipeKnownSize( &expandedSalt, sizeof( expandedSalt ) );

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSskdfHash(
    _In_                    PCSYMCRYPT_HASH hashAlgorithm,
                            SIZE_T          cbHashOutputSize,
    _In_reads_(cbSecret)    PCBYTE          pbSecret,
                            SIZE_T          cbSecret,
    _In_reads_opt_(cbInfo)  PCBYTE          pbInfo,
                            SIZE_T          cbInfo,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult)
{
    SYMCRYPT_HASH_STATE state;

    PBYTE pbCurr = pbResult;

    SIZE_T cbHashResultSize = hashAlgorithm->resultSize;
    SIZE_T cbPartialResult;
    UINT32 cntr = 1;
    BYTE ctrBuf[4];

    if ( cbHashOutputSize > 64 ||
         cbHashOutputSize > 0 && cbHashOutputSize != hashAlgorithm->resultSize )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    while ( cbResult > 0 )
    {
        SYMCRYPT_STORE_MSBFIRST32( ctrBuf, cntr );

        cbPartialResult = SYMCRYPT_MIN( cbResult, cbHashResultSize );

        // Calculate K(i) = H(counter || Z || FixedInfo)
        SymCryptHashInit( hashAlgorithm, &state );
        SymCryptHashAppend( hashAlgorithm, &state, ctrBuf, sizeof( ctrBuf ) );
        SymCryptHashAppend( hashAlgorithm, &state, pbSecret, cbSecret );
        SymCryptHashAppend( hashAlgorithm, &state, pbInfo, cbInfo );
        SymCryptHashResult( hashAlgorithm, &state, pbCurr, cbPartialResult );

        // Update counters
        cntr++;
        pbCurr += cbPartialResult;
        cbResult -= cbPartialResult;
    }

    return SYMCRYPT_NO_ERROR;
}
