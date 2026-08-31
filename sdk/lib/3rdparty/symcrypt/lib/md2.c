//
// Md2.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//
// This module contains the routines to implement MD2 from RFC 1319
//
// This is a new implementation, NOT based on the existing one in RSA32.lib,
// which is the one from RSA data security.
//
// The implementation had to be refreshed anyway to conform to our coding
// guidelines for cryptographic functions.
// Re-implementing the function along the lines of our SHA-family implementations
// was easy, and it removes a file with RSA copyright from our system.
//
// The only data copied for this implementation is the S table from the
// RFC.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//

const SYMCRYPT_HASH SymCryptMd2Algorithm_default = {
    &SymCryptMd2Init,
    &SymCryptMd2Append,
    &SymCryptMd2Result,
    &SymCryptMd2AppendBlocks,
    &SymCryptMd2StateCopy,
    sizeof( SYMCRYPT_MD2_STATE ),
    SYMCRYPT_MD2_RESULT_SIZE,
    SYMCRYPT_MD2_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MD2_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_MD2_STATE, chain ),
};

const PCSYMCRYPT_HASH SymCryptMd2Algorithm = &SymCryptMd2Algorithm_default;

//
// These entries are called S[i] in RFC1319
//
const BYTE SymCryptMd2STable[256] = {
    41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6,
    19, 98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188,
    76, 130, 202, 30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24,
    138, 23, 229, 18, 190, 78, 196, 214, 218, 158, 222, 73, 160, 251,
    245, 142, 187, 47, 238, 122, 169, 104, 121, 145, 21, 178, 7, 63,
    148, 194, 16, 137, 11, 34, 95, 33, 128, 127, 93, 154, 90, 144, 50,
    39, 53, 62, 204, 231, 191, 247, 151, 3, 255, 25, 48, 179, 72, 165,
    181, 209, 215, 94, 146, 42, 172, 86, 170, 198, 79, 184, 56, 210,
    150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241, 69, 157,
    112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2, 27,
    96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
    85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197,
    234, 38, 44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65,
    129, 77, 82, 106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123,
    8, 12, 189, 177, 74, 120, 136, 149, 139, 227, 99, 232, 109, 233,
    203, 213, 254, 59, 0, 29, 57, 242, 239, 183, 14, 102, 88, 208, 228,
    166, 119, 114, 248, 235, 117, 75, 10, 49, 68, 80, 180, 143, 237,
    31, 26, 219, 153, 141, 51, 159, 17, 131, 20
};


//
// SymCryptMd2
//
#define ALG MD2
#define Alg Md2
#include "hash_pattern.c"
#undef ALG
#undef Alg


//
// SymCryptMd2Init
//
VOID
SYMCRYPT_CALL
SymCryptMd2Init( _Out_ PSYMCRYPT_MD2_STATE  pState )
{
    //
    // We use the secure wipe as the init routine is also used to re-initialize
    // (and wipe) the state after a hash computation.
    // In that case the compiler might conclude that this wipe can be optimized
    // away, and that would leak data.
    //
    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
    SYMCRYPT_SET_MAGIC( pState );
}


//
// SymCryptMd2Append
//
VOID
SYMCRYPT_CALL
SymCryptMd2Append( _Inout_              PSYMCRYPT_MD2_STATE    pState,
                   _In_reads_( cbData ) PCBYTE                 pbData,
                                        SIZE_T                 cbData )
{
    SymCryptHashAppendInternal( SymCryptMd2Algorithm, (PSYMCRYPT_COMMON_HASH_STATE)pState, pbData, cbData );
}


//
// SymCryptMd2Result
//
VOID
SYMCRYPT_CALL
SymCryptMd2Result(  _Inout_                                 PSYMCRYPT_MD2_STATE state,
                   _Out_writes_( SYMCRYPT_MD2_RESULT_SIZE ) PBYTE               pbResult )
{
    //
    // The buffer is never completely full, so it is easy to compute the actual padding.
    //
    SIZE_T tmp;
    SIZE_T paddingBytes = 16 - state->bytesInBuffer;


    SYMCRYPT_CHECK_MAGIC( state );

    memset( &state->buffer[state->bytesInBuffer], (BYTE)paddingBytes, paddingBytes );

    SymCryptMd2AppendBlocks( &state->chain, state->buffer, SYMCRYPT_MD2_INPUT_BLOCK_SIZE, &tmp );

    //
    // Append the checksum
    //
    SymCryptMd2AppendBlocks( &state->chain, state->chain.C, SYMCRYPT_MD2_INPUT_BLOCK_SIZE, &tmp );

    memcpy( pbResult, &state->chain.X[0], SYMCRYPT_MD2_RESULT_SIZE );

    //
    // Wipe & re-initialize
    //
    // (Our init code wipes the buffer too, so we don't have to.)
    //
    SymCryptMd2Init( state );
}


VOID
SYMCRYPT_CALL
SymCryptMd2AppendBlocks(
    _Inout_                 PSYMCRYPT_MD2_CHAINING_STATE    pChain,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T                        * pcbRemaining )
{
    //
    // For variable names see RFC 1319.
    //
    unsigned int t;
    int j,k;

    while( cbData >= SYMCRYPT_MD2_INPUT_BLOCK_SIZE )
    {
        BYTE L;
        //
        // read the data once into our structure
        //
        memcpy( &pChain->X[16], pbData, SYMCRYPT_MD2_INPUT_BLOCK_SIZE );

        //
        // Update the checksum block.
        // The L value at the end of the previous block is in the last byte of the checksum
        //
        L = pChain->C[15];

        for( j=0; j<16; j++ )
        {
            pChain->C[j] = L = pChain->C[j] ^  SymCryptMd2STable[ L ^ pChain->X[16+j] ];
        }

        //
        // Now we compute the actual hash
        //
        SymCryptXorBytes( &pChain->X[0], &pChain->X[16], &pChain->X[32], 16 );

        t = 0;
        for( j=0; j<18; j++ )
        {
            for( k=0; k<48; k++ )
            {
                t = pChain->X[k] ^ SymCryptMd2STable[t];
                pChain->X[k] = (BYTE) t;
            }
            t = (t + j)& 0xff;
        }

    pbData += SYMCRYPT_MD2_INPUT_BLOCK_SIZE;
    cbData -= SYMCRYPT_MD2_INPUT_BLOCK_SIZE;
    }

    *pcbRemaining = cbData;
}


VOID
SYMCRYPT_CALL
SymCryptMd2StateExport(
    _In_                                                    PCSYMCRYPT_MD2_STATE    pState,
    _Out_writes_bytes_( SYMCRYPT_MD2_STATE_EXPORT_SIZE )    PBYTE                   pbBlob )
{
    SYMCRYPT_ALIGN SYMCRYPT_MD2_STATE_EXPORT_BLOB    blob;           // local copy to have proper alignment.
    C_ASSERT( sizeof( blob ) == SYMCRYPT_MD2_STATE_EXPORT_SIZE );

    SYMCRYPT_CHECK_MAGIC( pState );

    SymCryptWipeKnownSize( &blob, sizeof( blob ) ); // wipe to avoid any data leakage

    blob.header.magic = SYMCRYPT_BLOB_MAGIC;
    blob.header.size = SYMCRYPT_MD2_STATE_EXPORT_SIZE;
    blob.header.type = SymCryptBlobTypeMd2State;

    //
    // Copy the relevant data. Buffer will be 0-padded.
    //
    memcpy( &blob.C[0], &pState->chain.C[0], 16 );
    memcpy( &blob.X[0], &pState->chain.X[0], 16 );
    blob.bytesInBuffer = (UINT32) pState->bytesInBuffer;
    memcpy( &blob.buffer[0], &pState->buffer[0], blob.bytesInBuffer );

    SYMCRYPT_ASSERT( (PCBYTE) &blob + sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ) == (PCBYTE) &blob.trailer );
    SymCryptMarvin32( SymCryptMarvin32DefaultSeed, (PCBYTE) &blob, sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ), &blob.trailer.checksum[0] );

    memcpy( pbBlob, &blob, sizeof( blob ) );

//cleanup:
    SymCryptWipeKnownSize( &blob, sizeof( blob ) );
    return;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMd2StateImport(
    _Out_                                               PSYMCRYPT_MD2_STATE     pState,
    _In_reads_bytes_( SYMCRYPT_MD2_STATE_EXPORT_SIZE)   PCBYTE                  pbBlob )
{
    SYMCRYPT_ERROR                  scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_ALIGN SYMCRYPT_MD2_STATE_EXPORT_BLOB  blob;                       // local copy to have proper alignment.
    BYTE                            checksum[8];

    C_ASSERT( sizeof( blob ) == SYMCRYPT_MD2_STATE_EXPORT_SIZE );
    memcpy( &blob, pbBlob, sizeof( blob ) );

    if( blob.header.magic != SYMCRYPT_BLOB_MAGIC ||
        blob.header.size != SYMCRYPT_MD2_STATE_EXPORT_SIZE ||
        blob.header.type != SymCryptBlobTypeMd2State )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    SymCryptMarvin32( SymCryptMarvin32DefaultSeed, (PCBYTE) &blob, sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ), checksum );
    if( memcmp( checksum, &blob.trailer.checksum[0], 8 ) != 0 )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    memcpy( &pState->chain.C[0], &blob.C[0], 16 );
    memcpy( &pState->chain.X[0], &blob.X[0], 16 );
    memcpy( &pState->buffer[0], &blob.buffer[0], 16 );
    pState->bytesInBuffer = blob.bytesInBuffer;

    pState->dataLengthL = blob.bytesInBuffer;
    pState->dataLengthH = 1;

    SYMCRYPT_SET_MAGIC( pState );

cleanup:
    SymCryptWipeKnownSize( &blob, sizeof(blob) );
    return scError;
}






//
// Simple test vector for FIPS module testing
//

static const BYTE   md2KATAnswer[ 16 ] = {
    0xda, 0x85, 0x3b, 0x0d, 0x3f, 0x88, 0xd9, 0x9b,
    0x30, 0x28, 0x3a, 0x69, 0xe6, 0xde, 0xd6, 0xbb,
} ;

VOID
SYMCRYPT_CALL
SymCryptMd2Selftest(void)
{
    BYTE result[SYMCRYPT_MD2_RESULT_SIZE];

    SymCryptMd2( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, md2KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'MD2t' );
    }
}
