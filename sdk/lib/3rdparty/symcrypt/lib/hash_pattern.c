//
// hash_pattern.c
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#if 0
#pragma makedep header
#endif

//
// This is a file that is #included to define the
// all-in-one hash function.
//



SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SYMCRYPT_Xxx(
    _In_reads_( cbData )                                    PCBYTE  pbData,
                                                            SIZE_T  cbData,
    _Out_writes_( CONCAT3( SYMCRYPT_, ALG, _RESULT_SIZE ) ) PBYTE   pbResult )
{
    SYMCRYPT_XXX_STATE state;

    SYMCRYPT_XxxInit( &state );
    SYMCRYPT_XxxAppend( &state, pbData, cbData );
    SYMCRYPT_XxxResult( & state, pbResult );
}

VOID
SYMCRYPT_CALL
SYMCRYPT_XxxStateCopy( _In_ const SYMCRYPT_XXX_STATE * pSrc, _Out_ SYMCRYPT_XXX_STATE * pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );
    *pDst = *pSrc;
    SYMCRYPT_SET_MAGIC( pDst );
}
