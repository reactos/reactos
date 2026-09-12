//
// hmac_pattern.c
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#if 0
#pragma makedep header
#endif

VOID
SYMCRYPT_CALL
SYMCRYPT_HmacXxxStateCopy(
    _In_        PCSYMCRYPT_HMAC_XXX_STATE           pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_XXX_EXPANDED_KEY    pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_XXX_STATE            pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );

    SYMCRYPT_XxxStateCopy( &pSrc->hash, &pDst->hash );

    if( pExpandedKey != NULL )
    {
        SYMCRYPT_CHECK_MAGIC( pExpandedKey );
        pDst->pKey = pExpandedKey;
    }
    else
    {
        SYMCRYPT_CHECK_MAGIC( pSrc->pKey );
        pDst->pKey = pSrc->pKey;
    }
    SYMCRYPT_SET_MAGIC( pDst );
}

VOID
SYMCRYPT_CALL
SYMCRYPT_HmacXxxKeyCopy( _In_ PCSYMCRYPT_HMAC_XXX_EXPANDED_KEY pSrc, _Out_ PSYMCRYPT_HMAC_XXX_EXPANDED_KEY pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );
    *pDst = *pSrc;
    SYMCRYPT_SET_MAGIC( pDst );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SYMCRYPT_HmacXxxExpandKey(
    _Out_                   PSYMCRYPT_HMAC_XXX_EXPANDED_KEY pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                          pbKey,
                            SIZE_T                          cbKey )
{
    SYMCRYPT_XXX_STATE  hashState;
    SYMCRYPT_ALIGN BYTE iblock[ SYMCRYPT_XXX_INPUT_BLOCK_SIZE ];  // One input block for the hash function
    SIZE_T tmp;

    SYMCRYPT_SET_MAGIC( pExpandedKey );

    //
    // Initialize our hash state and our input block
    // We wipe the whole block & then copy the key into it. This is often faster
    // as the compiler can optimize the wipe because it knows the size at compile time.
    //
    SYMCRYPT_XxxInit( &hashState );
    memset( iblock, 0, sizeof( iblock ) );

    if( cbKey <= sizeof( iblock ) )
    {
        if( cbKey > 0 )
        {
            memcpy( iblock, pbKey, cbKey );
        }
    } else {
        //
        // We can use the existing MD5 state to hash the long key.
        // The state is re-initialized by the SymCryptMd5Result() function.
        //
        SYMCRYPT_XxxAppend( &hashState, pbKey, cbKey );
        SYMCRYPT_XxxResult( &hashState, iblock );
    }

    XorByteIntoBuffer( iblock, sizeof( iblock )/8, HMAC_IPAD_BYTE );

    //
    // Copy the initial chaining state to both states in the expanded key
    //
    pExpandedKey->innerState = hashState.chain;
    pExpandedKey->outerState = hashState.chain;

    //
    // Update the state in the expanded key directly
    //
    SYMCRYPT_XxxAppendBlocks( &pExpandedKey->innerState, iblock, sizeof( iblock ), &tmp );

    XorByteIntoBuffer( iblock, sizeof( iblock )/8, HMAC_IPAD_BYTE ^ HMAC_OPAD_BYTE );

    SYMCRYPT_XxxAppendBlocks( &pExpandedKey->outerState, iblock, sizeof( iblock ), &tmp );

    SymCryptWipeKnownSize( iblock, sizeof( iblock ) );
    SymCryptWipeKnownSize( &hashState, sizeof( hashState ) );

    return SYMCRYPT_NO_ERROR;
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SYMCRYPT_HmacXxxInit(
    _Out_   PSYMCRYPT_HMAC_XXX_STATE            pState,
    _In_    PCSYMCRYPT_HMAC_XXX_EXPANDED_KEY    pExpandedKey)
{
    SYMCRYPT_CHECK_MAGIC( pExpandedKey );

    SYMCRYPT_SET_MAGIC( pState );

    //
    // We don't call SymCryptXxxInit on the hash sub-state;
    // instead we directly initialize its fields.
    //
    SYMCRYPT_SET_MAGIC( &pState->hash );
    pState->hash.chain = pExpandedKey->innerState;
    SET_DATALENGTH( pState->hash, SYMCRYPT_XXX_INPUT_BLOCK_SIZE );
    pState->hash.bytesInBuffer = 0;
    pState->pKey = pExpandedKey;
}

VOID
SYMCRYPT_CALL
SYMCRYPT_HmacXxxAppend(
    _Inout_                 PSYMCRYPT_HMAC_XXX_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData )
{
    SYMCRYPT_XxxAppend( &pState->hash, pbData, cbData );
}

C_ASSERT( SYMCRYPT_XXX_RESULT_SIZE == SYMCRYPT_HMAC_XXX_RESULT_SIZE );

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SYMCRYPT_HmacXxxResult(
    _Inout_                                         PSYMCRYPT_HMAC_XXX_STATE    pState,
    _Out_writes_( SYMCRYPT_HMAC_XXX_RESULT_SIZE )   PBYTE                       pbResult )
{
    BYTE    innerRes[SYMCRYPT_XXX_RESULT_SIZE];

    SYMCRYPT_CHECK_MAGIC( pState );

    //
    // We have to buffer the inner hash result. We can't put it directly in the
    // hash state data buffer as the Result() function wipes that buffer before returning.
    //

    SYMCRYPT_XxxResult( &pState->hash, innerRes );

    SYMCRYPT_CHECK_MAGIC( pState->pKey )

    pState->hash.chain = pState->pKey->outerState;

    //
    // We put the data directly in the buffer, rather than call the Append function.
    //
    memcpy( &pState->hash.buffer, innerRes, sizeof( innerRes ) );
    SET_DATALENGTH(  pState->hash, SYMCRYPT_XXX_INPUT_BLOCK_SIZE + SYMCRYPT_XXX_RESULT_SIZE );
    pState->hash.bytesInBuffer = SYMCRYPT_XXX_RESULT_SIZE;

    SYMCRYPT_XxxResult( &pState->hash, pbResult );

    //
    // The SymCryptXxxResult already wipes the hash state.
    // We only need to wipe our own buffer.
    //
    // We also set the key pointer to NULL. This is not for security;
    // it creates a clear error when callers forget to call the Init routine
    // when re-using a state. Rather than the wrong result, they will get
    // a NULL pointer exception, and they will fix their code.
    //

    SymCryptWipe( innerRes, sizeof( innerRes ) );
    pState->pKey = NULL;
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SYMCRYPT_HmacXxx(
    _In_                                            PCSYMCRYPT_HMAC_XXX_EXPANDED_KEY    pExpandedKey,
    _In_reads_( cbData )                           PCBYTE                              pbData,
                                                    SIZE_T                              cbData,
    _Out_writes_( SYMCRYPT_HMAC_XXX_RESULT_SIZE )   PBYTE                               pbResult )
{
    SYMCRYPT_HMAC_XXX_STATE state;

    SYMCRYPT_HmacXxxInit( &state, pExpandedKey );
    SYMCRYPT_HmacXxxAppend( &state, pbData, cbData );
    SYMCRYPT_HmacXxxResult( &state, pbResult );

}
