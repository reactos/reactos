//
// hash_buffer_pattern.c
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#if 0
#pragma makedep header
#endif

/*
SymCryptXxxAppend( _Inout_               SYMCRYPT_Xxx_STATE * state,
                   _In_reads_bytes_( cbData ) PCBYTE               pbData,
                                         SIZE_T               cbData )
{
    <Set up a SIZE_T variable 'bytesInBuffer' that contains the # bytes in the buffer>
*/

    //
    // Truncate bytesInBuffer so that we never have an integer overflow.
    //
    bytesInBuffer &= SYMCRYPT_XXX_INPUT_BLOCK_SIZE - 1;

    //
    // If previous data in buffer, buffer new input and transform if possible.
    //
    if (bytesInBuffer > 0)
    {
        SIZE_T freeInBuffer = SYMCRYPT_XXX_INPUT_BLOCK_SIZE - bytesInBuffer;
        if( cbData < freeInBuffer )
        {
            //
            // All the data will fit in the buffer.
            // We don't do anything here.
            // As cbData < INPUT_BLOCK_SIZE the bulk data processing is skipped,
            // and the data will be copied to the buffer at the end
            // of this code.
        } else {
            //
            // Enough data to fill the whole buffer & process it
            //
            memcpy(&state->buffer[bytesInBuffer], pbData, freeInBuffer);
            pbData += freeInBuffer;
            cbData -= freeInBuffer;
            SYMCRYPT_XxxAppendBlocks( &state->chain, state->buffer, SYMCRYPT_XXX_INPUT_BLOCK_SIZE );

            //
            // Set bytesInBuffer to zero to ensure that the trailing data in the
            // buffer will be copied to the right location of the buffer below.
            //
            bytesInBuffer = 0;
        }
    }

    //
    // Internal buffer is empty; process all remaining whole blocks in the input
    //
    if( cbData >= SYMCRYPT_XXX_INPUT_BLOCK_SIZE )
    {
        SIZE_T cbDataRoundedDown = cbData & ~(SIZE_T)(SYMCRYPT_XXX_INPUT_BLOCK_SIZE - 1);
        SYMCRYPT_XxxAppendBlocks( &state->chain, pbData, cbDataRoundedDown );
        pbData += cbDataRoundedDown;
        cbData -= cbDataRoundedDown;
    }

    //
    // buffer remaining input if necessary.
    //
    if( cbData > 0 )
    {
        memcpy( &state->buffer[bytesInBuffer], pbData, cbData );
    }

/*
}
*/
