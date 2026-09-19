//
// hash.c generic code used in many hash implementations.
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

VOID
SYMCRYPT_CALL
SymCryptHashAppendInternal(
    _In_                        PCSYMCRYPT_HASH             pHash,
    _Inout_                     PSYMCRYPT_COMMON_HASH_STATE pState,
    _In_reads_bytes_( cbData )  PCBYTE                      pbData,
                                SIZE_T                      cbData )
{
    UINT32  bytesInBuffer;
    UINT32  freeInBuffer;
    SIZE_T  tmp;

    SYMCRYPT_CHECK_MAGIC( pState );

    pState->dataLengthL += cbData;
    if( pState->dataLengthL < cbData ) {
        pState->dataLengthH ++;                         // This is almost-unreachable code as it requires 2^64 bytes to be hashed.
    }

    bytesInBuffer = pState->bytesInBuffer;

    //
    // If previous data in buffer, buffer new input and transform if possible.
    //
    if( bytesInBuffer > 0 )
    {
        SYMCRYPT_ASSERT( pHash->inputBlockSize > bytesInBuffer );

        freeInBuffer = pHash->inputBlockSize - bytesInBuffer;
        if( cbData < freeInBuffer )
        {
            //
            // All the data will fit in the buffer.
            // We don't do anything here.
            // As cbData < inputBlockSize the bulk data processing is skipped,
            // and the data will be copied to the buffer at the end
            // of this code.
        } else {
            //
            // Enough data to fill the whole buffer & process it
            //
            memcpy(&pState->buffer[bytesInBuffer], pbData, freeInBuffer);
            pbData += freeInBuffer;
            cbData -= freeInBuffer;
            (*pHash->appendBlockFunc)( (PBYTE)pState + pHash->chainOffset, &pState->buffer[0], pHash->inputBlockSize, &tmp );

            bytesInBuffer = 0;
        }
    }

    //
    // Internal buffer is empty; process all remaining whole blocks in the input
    //
    if( cbData >= pHash->inputBlockSize )
    {
        (*pHash->appendBlockFunc)( (PBYTE)pState + pHash->chainOffset, pbData, cbData, &tmp );
        SYMCRYPT_ASSERT( tmp < pHash->inputBlockSize );
        pbData += cbData - tmp;
        cbData = tmp;
    }

    SYMCRYPT_ASSERT( cbData < pHash->inputBlockSize );

    //
    // buffer remaining input if necessary.
    //
    if( cbData > 0 )
    {
        memcpy( &pState->buffer[bytesInBuffer], pbData, cbData );
        bytesInBuffer += (UINT32) cbData;
    }

    pState->bytesInBuffer = bytesInBuffer;
}

VOID
SYMCRYPT_CALL
SymCryptHashCommonPaddingMd4Style(
    _In_                        PCSYMCRYPT_HASH             pHash,
    _Inout_                     PSYMCRYPT_COMMON_HASH_STATE pState )
{
    SIZE_T tmp;
    SIZE_T bytesInBuffer = pState->bytesInBuffer;

    SYMCRYPT_CHECK_MAGIC( pState );
    SYMCRYPT_ASSERT( pHash->inputBlockSize == 64 );
    SYMCRYPT_ASSERT( bytesInBuffer == (pState->dataLengthL & 0x3f) );

    //
    // The buffer is never completely full, so we can always put the first
    // padding byte in.
    //
    pState->buffer[bytesInBuffer++] = 0x80;

    if( bytesInBuffer > 64-8 ) {
        //
        // No room for the rest of the padding. Pad with zeroes & process block
        // bytesInBuffer is at most 64, so we do not have an integer underflow
        //
        SymCryptWipe( &pState->buffer[bytesInBuffer], 64-bytesInBuffer );
        (*pHash->appendBlockFunc)( (PBYTE)pState + pHash->chainOffset, pState->buffer, 64, &tmp );
        SYMCRYPT_ASSERT( tmp == 0 );
        bytesInBuffer = 0;
    }

    //
    // Set rest of padding
    // At this point bytesInBuffer <= 64-8, so we don't have an underflow
    // We wipe to the end of the buffer as it is 16-aligned,
    // and it is faster to wipe to an aligned point
    //
    SymCryptWipe( &pState->buffer[bytesInBuffer], 64-bytesInBuffer );
    SYMCRYPT_STORE_LSBFIRST64( &pState->buffer[64-8], pState->dataLengthL * 8 );

    //
    // Process the final block
    //
    (*pHash->appendBlockFunc)( (PBYTE)pState + pHash->chainOffset, pState->buffer, 64, &tmp );
}



SIZE_T
SYMCRYPT_CALL
SymCryptHashResultSize( _In_ PCSYMCRYPT_HASH pHash )
{
    return pHash->resultSize;
}


SIZE_T
SYMCRYPT_CALL
SymCryptHashInputBlockSize( _In_ PCSYMCRYPT_HASH pHash )
{
    return pHash->inputBlockSize;
}

SIZE_T
SYMCRYPT_CALL
SymCryptHashStateSize( _In_ PCSYMCRYPT_HASH pHash )
{
    return pHash->stateSize;
}


VOID
SYMCRYPT_CALL
SymCryptHash(
    _In_                                                         PCSYMCRYPT_HASH pHash,
    _In_reads_( cbData )                                         PCBYTE          pbData,
                                                                 SIZE_T          cbData,
    _Out_writes_( SYMCRYPT_MIN( cbResult, pHash->resultSize ) )  PBYTE           pbResult,
                                                                 SIZE_T          cbResult )
{
    SYMCRYPT_HASH_STATE hash;

    _Analysis_assume_( pHash->stateSize <= sizeof( hash ) );
    SymCryptHashInit( pHash, &hash );
    SymCryptHashAppend( pHash, &hash, pbData, cbData );
    SymCryptHashResult( pHash, &hash, pbResult, cbResult );
    SymCryptWipe( &hash, pHash->stateSize );
}

VOID
SYMCRYPT_CALL
SymCryptHashInit(
    _In_                                        PCSYMCRYPT_HASH pHash,
    _Out_writes_bytes_( pHash->stateSize )      PVOID           pState )
{
    (*pHash->initFunc)( pState );
}

VOID
SYMCRYPT_CALL
SymCryptHashAppend(
    _In_                                        PCSYMCRYPT_HASH pHash,
    _Inout_updates_bytes_( pHash->stateSize )   PVOID           pState,
    _In_reads_( cbData )                        PCBYTE          pbData,
                                                SIZE_T          cbData )
{
    (*pHash->appendFunc)( pState, pbData, cbData );
}

VOID
SYMCRYPT_CALL
SymCryptHashResult(
    _In_                                                         PCSYMCRYPT_HASH pHash,
    _Inout_updates_bytes_( pHash->stateSize )                    PVOID           pState,
    _Out_writes_( SYMCRYPT_MIN( cbResult, pHash->resultSize ) )  PBYTE           pbResult,
                                                                 SIZE_T          cbResult )
{
    BYTE    buf[SYMCRYPT_HASH_MAX_RESULT_SIZE];

    _Analysis_assume_( pHash->resultSize <= SYMCRYPT_HASH_MAX_RESULT_SIZE );

    (*pHash->resultFunc)( pState, buf );
    memcpy( pbResult, buf, SYMCRYPT_MIN( cbResult, pHash->resultSize ));
    SymCryptWipe( buf, pHash->resultSize );
}

VOID
SYMCRYPT_CALL
SymCryptHashStateCopy(
    _In_                                PCSYMCRYPT_HASH pHash,
    _In_reads_( pHash->stateSize )      PCVOID          pSrc,
    _Out_writes_( pHash->stateSize )    PVOID           pDst)
{
    (*pHash->stateCopyFunc)( pSrc, pDst );
}
