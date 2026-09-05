//
// kmac_pattern.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#if 0
#pragma makedep header
#endif

//
// This source file implements KMAC128 and KMAC256
//
// See the symcrypt.h file for documentation on what the various functions do.
//


//
// SymCryptKmac
//
VOID
SYMCRYPT_CALL
SYMCRYPT_Xxx(
        _In_                                                PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbInput )                         PCBYTE  pbInput,
                                                            SIZE_T  cbInput,
        _Out_writes_bytes_( SYMCRYPT_KMACXXX_RESULT_SIZE )  PBYTE   pbResult)
{
    SYMCRYPT_XXX_STATE state;

    SYMCRYPT_XxxInit(&state, pExpandedKey);
    SYMCRYPT_XxxAppend(&state, pbInput, cbInput);
    SYMCRYPT_XxxResult(&state, pbResult);
}

//
// SymCryptKmacEx
//
VOID
SYMCRYPT_CALL
SYMCRYPT_XxxEx(
        _In_                            PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbInput )     PCBYTE  pbInput,
                                        SIZE_T  cbInput,
        _Out_writes_bytes_( cbResult )  PBYTE   pbResult,
                                        SIZE_T  cbResult)
{
    SYMCRYPT_XXX_STATE state;

    SYMCRYPT_XxxInit(&state, pExpandedKey);
    SYMCRYPT_XxxAppend(&state, pbInput, cbInput);
    SYMCRYPT_XxxResultEx(&state, pbResult, cbResult);
}


//
// SymCryptKmacExpandKey
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SYMCRYPT_XxxExpandKey(
    _Out_                       PSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
    _In_reads_bytes_(cbKey)     PCBYTE  pbKey,
                                SIZE_T  cbKey)
{
    return SYMCRYPT_XxxExpandKeyEx(pExpandedKey, pbKey, cbKey, NULL, 0);
}

//
// SymCryptKmacExpandKeyEx
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SYMCRYPT_XxxExpandKeyEx(
        _Out_                                       PSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbKey )                   PCBYTE  pbKey,
                                                    SIZE_T  cbKey,
        _In_reads_bytes_( cbCustomizationString )   PCBYTE  pbCustomizationString,
                                                    SIZE_T  cbCustomizationString)
{
    static const BYTE nameString[] = { 0x4b, 0x4d, 0x41, 0x43 };  // "KMAC"

    C_ASSERT( sizeof(SYMCRYPT_XXX_EXPANDED_KEY) == sizeof(SYMCRYPT_CSHAKEXXX_STATE) );

    SYMCRYPT_CSHAKEXXX_INIT( (SYMCRYPT_CSHAKEXXX_STATE*)pExpandedKey, nameString, sizeof(nameString), pbCustomizationString, cbCustomizationString);

    SYMCRYPT_KECCAK_STATE* pks = &pExpandedKey->ks;

    // byte_pad( encode_string( K ) )
    SymCryptKeccakAppendEncodeTimes8(pks, pks->inputBlockSize / 8, TRUE);
    SymCryptKeccakAppendEncodedString(pks, pbKey, cbKey);

    if (pks->stateIndex != 0)
    {
        SymCryptKeccakZeroAppendBlock(pks);
    }

    return SYMCRYPT_NO_ERROR;
}

//
// SymCryptKmacInit
//
VOID
SYMCRYPT_CALL
SYMCRYPT_XxxInit(
    _Out_   PSYMCRYPT_XXX_STATE         pState,
    _In_    PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey)
{
    C_ASSERT(sizeof(*pState) == sizeof(*pExpandedKey));

    SYMCRYPT_CHECK_MAGIC(pExpandedKey);
    memcpy(pState, pExpandedKey, sizeof(*pState));
    SYMCRYPT_SET_MAGIC(pState);
}

//
// SymCryptKmacAppend
//
VOID
SYMCRYPT_CALL
SYMCRYPT_XxxAppend(
    _Inout_                 PSYMCRYPT_XXX_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbData,
                            SIZE_T              cbData )
{
    SYMCRYPT_ASSERT(!pState->ks.squeezeMode);
    SymCryptKeccakAppend(&pState->ks, pbData, cbData);
}

//
// SymCryptKmacExtract
//
VOID
SYMCRYPT_CALL
SYMCRYPT_XxxExtract(
    _Inout_                     PSYMCRYPT_XXX_STATE pState,
    _Out_writes_( cbOutput )    PBYTE               pbOutput,
                                SIZE_T              cbOutput,
                                BOOLEAN             bWipe)
{
    // This function uses KMAC in XOF mode.
    //
    // If this is the first time Extract is being called, append right_encode(0)
    // to indicate that we're in XOF mode. This padding will be applied only once
    // as SymCryptKeccakExtract will transition the state to squeeze mode.
    if (!pState->ks.squeezeMode)
    {
        SymCryptKeccakAppendEncodeTimes8(&pState->ks, 0, FALSE);
    }

    SymCryptKeccakExtract(&pState->ks, pbOutput, cbOutput, bWipe);
}

//
// SymCryptKmacResult
//
VOID
SYMCRYPT_CALL
SYMCRYPT_XxxResult(
    _Inout_                                         PSYMCRYPT_XXX_STATE pState,
    _Out_writes_( SYMCRYPT_KMACXXX_RESULT_SIZE )    PBYTE               pbOutput)
{
    SYMCRYPT_XxxResultEx(pState, pbOutput, SYMCRYPT_KMACXXX_RESULT_SIZE);
}


//
// SymCryptKmacResultEx
//
VOID
SYMCRYPT_CALL
SYMCRYPT_XxxResultEx(
    _Inout_                     PSYMCRYPT_XXX_STATE pState,
    _Out_writes_( cbOutput )    PBYTE               pbOutput,
                                SIZE_T              cbOutput)
{
    // Result and ResultEx functions are used to extract data only once.
    // KMAC requires the output length to be encoded and appended to the
    // end of the input before the state switches to squeeze mode.
    //
    // If Result or ResultEx is called after an Extract call with bWipe=FALSE,
    // this means KMAC was used in XOF mode and length padding has already been
    // applied. In this case, Result and ResultEx functions extract data one last
    // time in XOF mode and wipe the state afterwards.

    if (!pState->ks.squeezeMode)
    {
        // Append right_encode(L)
        SymCryptKeccakAppendEncodeTimes8(&pState->ks, cbOutput, FALSE);
    }

    SymCryptKeccakExtract(&pState->ks, pbOutput, cbOutput, TRUE);
}

//
// SymCryptKmacKeyCopy
//
VOID
SYMCRYPT_CALL
SYMCRYPT_XxxKeyCopy(_In_ PCSYMCRYPT_XXX_EXPANDED_KEY pSrc, _Out_ PSYMCRYPT_XXX_EXPANDED_KEY pDst)
{
    SYMCRYPT_CHECK_MAGIC(pSrc);
    *pDst = *pSrc;
    SYMCRYPT_SET_MAGIC(pDst);
}

//
// SymCryptKmacStateCopy
//
VOID
SYMCRYPT_CALL
SYMCRYPT_XxxStateCopy(_In_ const SYMCRYPT_XXX_STATE* pSrc, _Out_ SYMCRYPT_XXX_STATE* pDst)
{
    SYMCRYPT_CHECK_MAGIC(pSrc);
    *pDst = *pSrc;
    SYMCRYPT_SET_MAGIC(pDst);
}
