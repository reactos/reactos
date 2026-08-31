//
// Sha512.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement SHA2-512 from FIPS 180-2
//


#include "precomp.h"

//
// SHA-512 uses 80 magic constants of 64 bits each. These are
// referred to as K^{512}_i for i=0...79 by FIPS 180-2.
// We use a static array as that does not pollute the linker name space
// For performance we align to the cache line size of 64 bytes
// We have one extra value at the end to allow an XMM read from each element
// of the array.
//
SYMCRYPT_ALIGN_AT( 64 ) const  UINT64 SymCryptSha512K[81] = {
    0x428a2f98d728ae22UL, 0x7137449123ef65cdUL,
    0xb5c0fbcfec4d3b2fUL, 0xe9b5dba58189dbbcUL,
    0x3956c25bf348b538UL, 0x59f111f1b605d019UL,
    0x923f82a4af194f9bUL, 0xab1c5ed5da6d8118UL,
    0xd807aa98a3030242UL, 0x12835b0145706fbeUL,
    0x243185be4ee4b28cUL, 0x550c7dc3d5ffb4e2UL,
    0x72be5d74f27b896fUL, 0x80deb1fe3b1696b1UL,
    0x9bdc06a725c71235UL, 0xc19bf174cf692694UL,
    0xe49b69c19ef14ad2UL, 0xefbe4786384f25e3UL,
    0x0fc19dc68b8cd5b5UL, 0x240ca1cc77ac9c65UL,
    0x2de92c6f592b0275UL, 0x4a7484aa6ea6e483UL,
    0x5cb0a9dcbd41fbd4UL, 0x76f988da831153b5UL,
    0x983e5152ee66dfabUL, 0xa831c66d2db43210UL,
    0xb00327c898fb213fUL, 0xbf597fc7beef0ee4UL,
    0xc6e00bf33da88fc2UL, 0xd5a79147930aa725UL,
    0x06ca6351e003826fUL, 0x142929670a0e6e70UL,
    0x27b70a8546d22ffcUL, 0x2e1b21385c26c926UL,
    0x4d2c6dfc5ac42aedUL, 0x53380d139d95b3dfUL,
    0x650a73548baf63deUL, 0x766a0abb3c77b2a8UL,
    0x81c2c92e47edaee6UL, 0x92722c851482353bUL,
    0xa2bfe8a14cf10364UL, 0xa81a664bbc423001UL,
    0xc24b8b70d0f89791UL, 0xc76c51a30654be30UL,
    0xd192e819d6ef5218UL, 0xd69906245565a910UL,
    0xf40e35855771202aUL, 0x106aa07032bbd1b8UL,
    0x19a4c116b8d2d0c8UL, 0x1e376c085141ab53UL,
    0x2748774cdf8eeb99UL, 0x34b0bcb5e19b48a8UL,
    0x391c0cb3c5c95a63UL, 0x4ed8aa4ae3418acbUL,
    0x5b9cca4f7763e373UL, 0x682e6ff3d6b2b8a3UL,
    0x748f82ee5defb2fcUL, 0x78a5636f43172f60UL,
    0x84c87814a1f0ab72UL, 0x8cc702081a6439ecUL,
    0x90befffa23631e28UL, 0xa4506cebde82bde9UL,
    0xbef9a3f7b2c67915UL, 0xc67178f2e372532bUL,
    0xca273eceea26619cUL, 0xd186b8c721c0c207UL,
    0xeada7dd6cde0eb1eUL, 0xf57d4f7fee6ed178UL,
    0x06f067aa72176fbaUL, 0x0a637dc5a2c898a6UL,
    0x113f9804bef90daeUL, 0x1b710b35131c471bUL,
    0x28db77f523047d84UL, 0x32caab7b40c72493UL,
    0x3c9ebe0a15c9bebcUL, 0x431d67c49c100d4cUL,
    0x4cc5d4becb3e42b6UL, 0x597f299cfc657e2aUL,
    0x5fcb6fab3ad6faecUL, 0x6c44198c4a475817UL,
};

//
// Initial states
//
const UINT64 SymCryptSha512InitialState[8] = {
    0x6a09e667f3bcc908UL,
    0xbb67ae8584caa73bUL,
    0x3c6ef372fe94f82bUL,
    0xa54ff53a5f1d36f1UL,
    0x510e527fade682d1UL,
    0x9b05688c2b3e6c1fUL,
    0x1f83d9abfb41bd6bUL,
    0x5be0cd19137e2179UL,
};

const UINT64 SymCryptSha384InitialState[8] = {
    0xcbbb9d5dc1059ed8UL,
    0x629a292a367cd507UL,
    0x9159015a3070dd17UL,
    0x152fecd8f70e5939UL,
    0x67332667ffc00b31UL,
    0x8eb44a8768581511UL,
    0xdb0c2e0d64f98fa7UL,
    0x47b5481dbefa4fa4UL,
};

const UINT64 SymCryptSha512_224InitialState[8] = {
    0x8c3d37c819544da2UL,
    0x73e1996689dcd4d6UL,
    0x1dfab7ae32ff9c82UL,
    0x679dd514582f9fcfUL,
    0x0f6d2b697bd44da8UL,
    0x77e36f7304c48942UL,
    0x3f9d85a86a1d36c8UL,
    0x1112e6ad91d692a1UL,
};

const UINT64 SymCryptSha512_256InitialState[8] = {
    0x22312194fc2bf72cUL,
    0x9f555fa3c84c64c2UL,
    0x2393b86b6f53b151UL,
    0x963877195940eabdUL,
    0x96283ee2a88effe3UL,
    0xbe5e1e2553863992UL,
    0x2b0199fc2c85b8aaUL,
    0x0eb72ddc81c52ca2UL,
};


//
// Todo: this structure pulls in the SHA284 code anytime someone uses
// SHA-512; should be split into a separate file.
//
const SYMCRYPT_HASH SymCryptSha384Algorithm_default = {
    &SymCryptSha384Init,
    &SymCryptSha384Append,
    &SymCryptSha384Result,
    &SymCryptSha512AppendBlocks,
    &SymCryptSha384StateCopy,
    sizeof( SYMCRYPT_SHA384_STATE ),
    SYMCRYPT_SHA384_RESULT_SIZE,
    SYMCRYPT_SHA384_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_SHA384_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_SHA384_STATE, chain ),
};

const SYMCRYPT_HASH SymCryptSha512Algorithm_default = {
    &SymCryptSha512Init,
    &SymCryptSha512Append,
    &SymCryptSha512Result,
    &SymCryptSha512AppendBlocks,
    &SymCryptSha512StateCopy,
    sizeof( SYMCRYPT_SHA512_STATE ),
    SYMCRYPT_SHA512_RESULT_SIZE,
    SYMCRYPT_SHA512_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_SHA512_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_SHA512_STATE, chain ),
};

const SYMCRYPT_HASH SymCryptSha512_224Algorithm_default = {
    &SymCryptSha512_224Init,
    &SymCryptSha512_224Append,
    &SymCryptSha512_224Result,
    &SymCryptSha512AppendBlocks,
    &SymCryptSha512_224StateCopy,
    sizeof( SYMCRYPT_SHA512_224_STATE ),
    SYMCRYPT_SHA512_224_RESULT_SIZE,
    SYMCRYPT_SHA512_224_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_SHA512_224_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_SHA512_224_STATE, chain ),
};

const SYMCRYPT_HASH SymCryptSha512_256Algorithm_default = {
    &SymCryptSha512_256Init,
    &SymCryptSha512_256Append,
    &SymCryptSha512_256Result,
    &SymCryptSha512AppendBlocks,
    &SymCryptSha512_256StateCopy,
    sizeof( SYMCRYPT_SHA512_256_STATE ),
    SYMCRYPT_SHA512_256_RESULT_SIZE,
    SYMCRYPT_SHA512_256_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_SHA512_256_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_SHA512_256_STATE, chain ),
};

const PCSYMCRYPT_HASH SymCryptSha384Algorithm = &SymCryptSha384Algorithm_default;
const PCSYMCRYPT_HASH SymCryptSha512Algorithm = &SymCryptSha512Algorithm_default;
const PCSYMCRYPT_HASH SymCryptSha512_224Algorithm = &SymCryptSha512_224Algorithm_default;
const PCSYMCRYPT_HASH SymCryptSha512_256Algorithm = &SymCryptSha512_256Algorithm_default;

//
// SymCryptSha384
//
#define ALG SHA384
#define Alg Sha384
#include "hash_pattern.c"
#undef ALG
#undef Alg

//
// SymCryptSha512
//
#define ALG SHA512
#define Alg Sha512
#include "hash_pattern.c"
#undef ALG
#undef Alg

//
// SymCryptSha512/224
//
#define ALG SHA512_224
#define Alg Sha512_224
#include "hash_pattern.c"
#undef ALG
#undef Alg

//
// SymCryptSha512/256
//
#define ALG SHA512_256
#define Alg Sha512_256
#include "hash_pattern.c"
#undef ALG
#undef Alg


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512Init( _Out_ PSYMCRYPT_SHA512_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthH = 0;
    pState->dataLengthL = 0;
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &SymCryptSha512InitialState[0], sizeof( SymCryptSha512InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha384Init( _Out_ PSYMCRYPT_SHA384_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthH = 0;
    pState->dataLengthL = 0;
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &SymCryptSha384InitialState[0], sizeof( SymCryptSha384InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512_224Init( _Out_ PSYMCRYPT_SHA512_224_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthH = 0;
    pState->dataLengthL = 0;
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &SymCryptSha512_224InitialState[0], sizeof( SymCryptSha512_224InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512_256Init( _Out_ PSYMCRYPT_SHA512_256_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthH = 0;
    pState->dataLengthL = 0;
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &SymCryptSha512_256InitialState[0], sizeof( SymCryptSha512_256InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512Append(
    _Inout_                 PSYMCRYPT_SHA512_STATE  pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData )
{
    UINT32 bytesInBuffer;
    UINT32 freeInBuffer;
    SIZE_T tmp;

    SYMCRYPT_CHECK_MAGIC( pState );

    pState->dataLengthL += cbData;
    if( pState->dataLengthL < cbData ) {
        pState->dataLengthH++;
    }

    bytesInBuffer = pState->bytesInBuffer;

    //
    // If previous data in buffer, buffer new input and transform if possible.
    //
    if( bytesInBuffer > 0 )
    {
        SYMCRYPT_ASSERT( SYMCRYPT_SHA512_INPUT_BLOCK_SIZE > bytesInBuffer );

        freeInBuffer = SYMCRYPT_SHA512_INPUT_BLOCK_SIZE - bytesInBuffer;
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
            SymCryptSha512AppendBlocks( &pState->chain, &pState->buffer[0], SYMCRYPT_SHA512_INPUT_BLOCK_SIZE, &tmp );

            bytesInBuffer = 0;
        }
    }

    //
    // Internal buffer is empty; process all remaining whole blocks in the input
    //
    if( cbData >= SYMCRYPT_SHA512_INPUT_BLOCK_SIZE )
    {
        SymCryptSha512AppendBlocks( &pState->chain, pbData, cbData, &tmp );
        SYMCRYPT_ASSERT( tmp < SYMCRYPT_SHA512_INPUT_BLOCK_SIZE );
        pbData += cbData - tmp;
        cbData = tmp;
    }

    SYMCRYPT_ASSERT( cbData < SYMCRYPT_SHA512_INPUT_BLOCK_SIZE );

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

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha384Append(
    _Inout_                 PSYMCRYPT_SHA384_STATE  pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData )
{

    SymCryptSha512Append( (PSYMCRYPT_SHA512_STATE)pState, pbData, cbData );

}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512_224Append(
    _Inout_                 PSYMCRYPT_SHA512_224_STATE  pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData )
{
    SymCryptSha512Append( (PSYMCRYPT_SHA512_STATE)pState, pbData, cbData );
}

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512_256Append(
    _Inout_                 PSYMCRYPT_SHA512_256_STATE  pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData )
{
    SymCryptSha512Append( (PSYMCRYPT_SHA512_STATE)pState, pbData, cbData );
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512Result(
    _Inout_                                     PSYMCRYPT_SHA512_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA512_RESULT_SIZE ) PBYTE                   pbResult )
{
    UINT32 bytesInBuffer;
    SIZE_T tmp;

    SYMCRYPT_CHECK_MAGIC( pState );

    bytesInBuffer = pState->bytesInBuffer;

    //
    // The buffer is never completely full, so we can always put the first
    // padding byte in.
    //
    pState->buffer[bytesInBuffer++] = 0x80;

    if( bytesInBuffer > 128-16 ) {
        //
        // No room for the rest of the padding. Pad with zeroes & process block
        // bytesInBuffer is at most 128, so we do not have an integer underflow
        //
        SymCryptWipe( &pState->buffer[bytesInBuffer], 128-bytesInBuffer );
        SymCryptSha512AppendBlocks( &pState->chain, pState->buffer, 128, &tmp );
        bytesInBuffer = 0;
    }

    //
    // Set rest of padding
    // We wipe to the end of the buffer as it is 16-aligned,
    // and it is faster to wipe to an aligned point
    //
    SymCryptWipe( &pState->buffer[bytesInBuffer], 128-bytesInBuffer );
    SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[128-16], (pState->dataLengthH << 3) + (pState->dataLengthL >> 61)  );
    SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[128- 8], (pState->dataLengthL << 3) );

    SymCryptSha512AppendBlocks( &pState->chain, pState->buffer, 128, &tmp );

    SymCryptUint64ToMsbFirst( &pState->chain.H[0], pbResult, 8 );

    //
    // We have to wipe the whole state because the Init call
    // might be optimized away by a smart compiler.
    //
    SymCryptWipeKnownSize( pState, sizeof( *pState ) );

    SYMCRYPT_SET_MAGIC( pState );

    memcpy( &pState->chain.H[0], &SymCryptSha512InitialState[0], sizeof( SymCryptSha512InitialState ) );
    }

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha384Result(
    _Inout_                                     PSYMCRYPT_SHA384_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA384_RESULT_SIZE ) PBYTE                   pbResult )
{
    //
    // For simplicity we re-use SymCryptSha512Result. This is slightly slower,
    // but SHA-384 isn't used that much.
    //
    SYMCRYPT_ALIGN BYTE sha512Result[SYMCRYPT_SHA512_RESULT_SIZE];      // Buffer for SHA-512 output

    //
    // The SHA-384 result is the first 48 bytes of the SHA-512 result of our state
    //
    SymCryptSha512Result( (PSYMCRYPT_SHA512_STATE)pState, sha512Result );
    memcpy( pbResult, sha512Result, SYMCRYPT_SHA384_RESULT_SIZE );

    //
    // The buffer was already wiped by the SymCryptSha512Result function, we
    // just have to re-initialize for SHA-384
    //
    SymCryptSha384Init( pState );

    SymCryptWipeKnownSize( sha512Result, sizeof( sha512Result ) );
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512_224Result(
    _Inout_                                         PSYMCRYPT_SHA512_224_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA512_224_RESULT_SIZE ) PBYTE                       pbResult )
{
    SYMCRYPT_ALIGN BYTE sha512Result[SYMCRYPT_SHA512_RESULT_SIZE];      // Buffer for SHA-512 output

    //
    // The SHA-512/224 result is the first 28 bytes of the SHA-512 result of our state
    //
    SymCryptSha512Result( (PSYMCRYPT_SHA512_STATE)pState, sha512Result );
    memcpy( pbResult, sha512Result, SYMCRYPT_SHA512_224_RESULT_SIZE );

    //
    // The buffer was already wiped by the SymCryptSha512Result function, we
    // just have to re-initialize for SHA-512/224
    //
    SymCryptSha512_224Init( pState );

    SymCryptWipeKnownSize( sha512Result, sizeof( sha512Result ) );
}


SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512_256Result(
    _Inout_                                         PSYMCRYPT_SHA512_256_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA512_256_RESULT_SIZE ) PBYTE                       pbResult )
{
    SYMCRYPT_ALIGN BYTE sha512Result[SYMCRYPT_SHA512_RESULT_SIZE];      // Buffer for SHA-512 output

    //
    // The SHA-512/256 result is the first 32 bytes of the SHA-512 result of our state
    //
    SymCryptSha512Result( (PSYMCRYPT_SHA512_STATE)pState, sha512Result );
    memcpy( pbResult, sha512Result, SYMCRYPT_SHA512_256_RESULT_SIZE );

    //
    // The buffer was already wiped by the SymCryptSha512Result function, we
    // just have to re-initialize for SHA-512/256
    //
    SymCryptSha512_256Init( pState );

    SymCryptWipeKnownSize( sha512Result, sizeof( sha512Result ) );
}


VOID
SYMCRYPT_CALL
SymCryptSha512StateExportCore(
    _In_                                                    PCSYMCRYPT_SHA512_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA512_STATE_EXPORT_SIZE ) PBYTE                   pbBlob,
    _In_                                                    UINT32                  type )
{
    SYMCRYPT_ALIGN SYMCRYPT_SHA512_STATE_EXPORT_BLOB blob;           // local copy to have proper alignment.
    C_ASSERT( sizeof( blob ) == SYMCRYPT_SHA512_STATE_EXPORT_SIZE );

    SYMCRYPT_CHECK_MAGIC( pState );

    SymCryptWipeKnownSize( &blob, sizeof( blob ) ); // wipe to avoid any data leakage

    blob.header.magic = SYMCRYPT_BLOB_MAGIC;
    blob.header.size = SYMCRYPT_SHA512_STATE_EXPORT_SIZE;
    blob.header.type = type;

    //
    // Copy the relevant data. Buffer will be 0-padded.
    //

    SymCryptUint64ToMsbFirst( &pState->chain.H[0], &blob.chain[0], 8 );
    blob.dataLengthL = pState->dataLengthL;
    blob.dataLengthH = pState->dataLengthH;
    memcpy( &blob.buffer[0], &pState->buffer[0], blob.dataLengthL & 0x7f );

    SYMCRYPT_ASSERT( (PCBYTE) &blob + sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ) == (PCBYTE) &blob.trailer );
    SymCryptMarvin32( SymCryptMarvin32DefaultSeed, (PCBYTE) &blob, sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ), &blob.trailer.checksum[0] );

    memcpy( pbBlob, &blob, sizeof( blob ) );

//cleanup:
    SymCryptWipeKnownSize( &blob, sizeof( blob ) );
    return;
}

VOID
SYMCRYPT_CALL
SymCryptSha512StateExport(
    _In_                                                    PCSYMCRYPT_SHA512_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA512_STATE_EXPORT_SIZE ) PBYTE                   pbBlob )
{
    SymCryptSha512StateExportCore( pState, pbBlob, SymCryptBlobTypeSha512State );
}

VOID
SYMCRYPT_CALL
SymCryptSha384StateExport(
    _In_                                                    PCSYMCRYPT_SHA384_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA384_STATE_EXPORT_SIZE ) PBYTE                   pbBlob )
{
    SymCryptSha512StateExportCore( (PCSYMCRYPT_SHA512_STATE)pState, pbBlob, SymCryptBlobTypeSha384State );
}

VOID
SYMCRYPT_CALL
SymCryptSha512_224StateExport(
    _In_                                                        PCSYMCRYPT_SHA512_224_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA512_224_STATE_EXPORT_SIZE ) PBYTE                       pbBlob )
{
    SymCryptSha512StateExportCore( (PCSYMCRYPT_SHA512_STATE)pState, pbBlob, SymCryptBlobTypeSha512_224State );
}

VOID
SYMCRYPT_CALL
SymCryptSha512_256StateExport(
    _In_                                                        PCSYMCRYPT_SHA512_256_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA512_256_STATE_EXPORT_SIZE ) PBYTE                       pbBlob )
{
    SymCryptSha512StateExportCore( (PCSYMCRYPT_SHA512_STATE)pState, pbBlob, SymCryptBlobTypeSha512_256State );
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha512StateImportCore(
    _Out_                                                   PSYMCRYPT_SHA512_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA512_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob,
    _In_                                                    UINT32                  type )
{
    SYMCRYPT_ERROR                                      scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_ALIGN SYMCRYPT_SHA512_STATE_EXPORT_BLOB    blob;                       // local copy to have proper alignment.
    BYTE                                                checksum[8];

    C_ASSERT( sizeof( blob ) == SYMCRYPT_SHA512_STATE_EXPORT_SIZE );
    memcpy( &blob, pbBlob, sizeof( blob ) );

    if( blob.header.magic != SYMCRYPT_BLOB_MAGIC ||
        blob.header.size != SYMCRYPT_SHA512_STATE_EXPORT_SIZE ||
        blob.header.type != type )
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

    SymCryptMsbFirstToUint64( &blob.chain[0], &pState->chain.H[0], 8 );
    pState->dataLengthL = blob.dataLengthL;
    pState->dataLengthH = blob.dataLengthH;
    pState->bytesInBuffer = blob.dataLengthL & 0x7f;
    memcpy( &pState->buffer[0], &blob.buffer[0], pState->bytesInBuffer );

    SYMCRYPT_SET_MAGIC( pState );

cleanup:
    SymCryptWipeKnownSize( &blob, sizeof(blob) );
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha512StateImport(
    _Out_                                                   PSYMCRYPT_SHA512_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA512_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob )
{
    return SymCryptSha512StateImportCore( pState, pbBlob, SymCryptBlobTypeSha512State );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha384StateImport(
    _Out_                                                   PSYMCRYPT_SHA384_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA384_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob )
{
    return SymCryptSha512StateImportCore( (PSYMCRYPT_SHA512_STATE)pState, pbBlob, SymCryptBlobTypeSha384State );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha512_224StateImport(
    _Out_                                                       PSYMCRYPT_SHA512_224_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA512_224_STATE_EXPORT_SIZE)    PCBYTE                      pbBlob )
{
    return SymCryptSha512StateImportCore( (PSYMCRYPT_SHA512_STATE)pState, pbBlob, SymCryptBlobTypeSha512_224State );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha512_256StateImport(
    _Out_                                                       PSYMCRYPT_SHA512_256_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA512_256_STATE_EXPORT_SIZE)    PCBYTE                      pbBlob )
{
    return SymCryptSha512StateImportCore( (PSYMCRYPT_SHA512_STATE)pState, pbBlob, SymCryptBlobTypeSha512_256State );
}


//
// A simple test case intended for module testing for
// FIPS compliance.
// This is the one-block example message from FIPS 180-2 appendix C
//

const BYTE SymCryptSha512KATAnswer[64] =
{
    0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
    0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
    0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
    0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
    0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
    0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
    0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
    0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f,
};

VOID
SYMCRYPT_CALL
SymCryptSha512Selftest(void)
{
    BYTE result[SYMCRYPT_SHA512_RESULT_SIZE];

    SymCryptSha512( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, SymCryptSha512KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'SH51' );
    }
}

//
// A simple test case intended for module testing for
// FIPS compliance.
// This is the one-block example message from FIPS 180-2 appendix D
//

const BYTE SymCryptSha384KATAnswer[ 48 ] =
{
    0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
    0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
    0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
    0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
    0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
    0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7,
};

VOID
SYMCRYPT_CALL
SymCryptSha384Selftest(void)
{
    BYTE result[SYMCRYPT_SHA384_RESULT_SIZE];

    SymCryptSha384( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, SymCryptSha384KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'SH38' );
    }
}

//
// Simple test vector for FIPS module testing
//

const BYTE SymCryptSha512_224KATAnswer[ 28 ] =
{
    0x46, 0x34, 0x27, 0x0f, 0x70, 0x7b, 0x6a, 0x54,
    0xda, 0xae, 0x75, 0x30, 0x46, 0x08, 0x42, 0xe2,
    0x0e, 0x37, 0xed, 0x26, 0x5c, 0xee, 0xe9, 0xa4,
    0x3e, 0x89, 0x24, 0xaa,
};

VOID
SYMCRYPT_CALL
SymCryptSha512_224Selftest(void)
{
    BYTE result[SYMCRYPT_SHA512_224_RESULT_SIZE];

    SymCryptSha512_224( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, SymCryptSha512_224KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'SH51' );
    }
}

//
// Simple test vector for FIPS module testing
//

const BYTE SymCryptSha512_256KATAnswer[ 32 ] =
{
    0x53, 0x04, 0x8e, 0x26, 0x81, 0x94, 0x1e, 0xf9,
    0x9b, 0x2e, 0x29, 0xb7, 0x6b, 0x4c, 0x7d, 0xab,
    0xe4, 0xc2, 0xd0, 0xc6, 0x34, 0xfc, 0x6d, 0x46,
    0xe0, 0xe2, 0xf1, 0x31, 0x07, 0xe7, 0xaf, 0x23,
};

VOID
SYMCRYPT_CALL
SymCryptSha512_256Selftest(void)
{
    BYTE result[SYMCRYPT_SHA512_256_RESULT_SIZE];

    SymCryptSha512_256( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, SymCryptSha512_256KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'SH51' );
    }
}

//
// We keep multiple implementations in this file.
// This allows us to switch different platforms to different implementations, whichever
// is faster. Even if we don't use a particular implementation in one release,
// we keep it around in case it becomes the preferred one for a new CPU release.
// (Performance can change a lot with changes in micro-architecture.)
//

//===================================================================================
// Implementation of compression function using UINT64s
//

//
// For documentation on these function see FIPS 180-2
//
// MAJ and CH are the functions Maj and Ch from the standard.
// CSIGMA0 and CSIGMA1 are the capital sigma functions.
// LSIGMA0 and LSIGMA1 are the lowercase sigma functions.
//
// The canonical definitions of the MAJ and CH functions are:
//#define MAJ( x, y, z )    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
//#define CH( x, y, z )  (((x) & (y)) ^ ((~(x)) & (z)))
// We use optimized versions defined below
//
#define MAJ( x, y, z )  ((((z) | (y)) & (x) ) | ((z) & (y)))
#define CH( x, y, z )  ((((z) ^ (y)) & (x)) ^ (z))

//
// The four Sigma functions
//

//#define CSIGMA0( x )    (ROR64((x), 28) ^ ROR64((x), 34) ^ ROR64((x), 39))
//#define CSIGMA1( x )    (ROR64((x), 14) ^ ROR64((x), 18) ^ ROR64((x), 41))
//#define LSIGMA0( x )    (ROR64((x),  1) ^ ROR64((x),  8) ^ ((x)>> 7))
//#define LSIGMA1( x )    (ROR64((x), 19) ^ ROR64((x), 61) ^ ((x)>> 6))

#define CSIGMA0( x )	(ROR64((ROR64((x), 6) ^ ROR64((x), 11) ^ (x)), 28))
#define CSIGMA1( x )	(ROR64((ROR64((x), 4) ^ ROR64((x), 27) ^ (x)), 14))
#define LSIGMA0( x )    (ROR64((x) ^ ROR64((x),  7),  1) ^ ((x)>> 7))
#define LSIGMA1( x )    (ROR64((x) ^ ROR64((x), 42), 19) ^ ((x)>> 6))



//
// The values a-h were stored in an array called ah.
// We have unrolled the loop 16 times. This makes both the indices into
// the ah array constant, and it makes the message addressing constant.
// This provides a significant speed improvement, at the cost of making
// the main loop about 4 kB in code.
//
// Initial round; r16 is the round number mod 16
// ah[ r16   &7] = h
// ah[(r16+1)&7] = g;
// ah[(r16+2)&7] = f;
// ah[(r16+3)&7] = e;
// ah[(r16+4)&7] = d;
// ah[(r16+5)&7] = c;
// ah[(r16+6)&7] = b;
// ah[(r16+7)&7] = a;
//
// Unfortunately, the compiler seems to choke on this, allocating an extra variable for
// each of the array indices, with duplicate stores to both locations.
//

//
// The core round, after the message word has been computed for this round and put in Wt.
// r16 is the round number modulo 16. (Static after loop unrolling)
// r is the round number
#define CROUND( a, b, c, d, e, f, g, h, r, r16 ) {;\
    W[r16] = Wt; \
    h += CSIGMA1(e) + CH(e, f, g) + SymCryptSha512K[r] + Wt;\
    d += h;\
    h += CSIGMA0(a) + MAJ(a, b, c);\
}

//
// Initial round that reads the message.
// r is the round number 0..15
//
#define IROUND( a, b, c, d, e, f, g, h, r ) {\
    Wt = SYMCRYPT_LOAD_MSBFIRST64( &pbData[ 8*r ] );\
    CROUND( a, b, c, d, e, f, g, h, r, r);\
    }
//
// Subsequent rounds.
// r is the round number, r16 is the round number mod 16.
// These are separate as typically r is run-time and r16 is compile time constant.
//
#define FROUND( a, b, c, d, e, f, g, h, r, r16 ) {                                      \
    Wt = LSIGMA1( W[(r16-2) & 15] ) +   W[(r16-7) & 15] +    \
         LSIGMA0( W[(r16-15) & 15]) +   W[r16 & 15];       \
    CROUND( a, b, c, d, e, f, g, h, r, r16 ); \
    }

//
// This is the core routine that does the actual hard work
// This is based on the older one in RSA32LIB by Scott Field from 2001
//
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ull(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    SYMCRYPT_ALIGN UINT64 W[16];
    UINT64 A, B, C, D, E, F, G, H;
    int round;
    UINT64 Wt;


    while( cbData >= 128 )
    {
        A = pChain->H[0];
        B = pChain->H[1];
        C = pChain->H[2];
        D = pChain->H[3];
        E = pChain->H[4];
        F = pChain->H[5];
        G = pChain->H[6];
        H = pChain->H[7];

        //
        // initial rounds 1 to 16
        //

        IROUND( A, B, C, D, E, F, G, H,  0 );
        IROUND( H, A, B, C, D, E, F, G,  1 );
        IROUND( G, H, A, B, C, D, E, F,  2 );
        IROUND( F, G, H, A, B, C, D, E,  3 );
        IROUND( E, F, G, H, A, B, C, D,  4 );
        IROUND( D, E, F, G, H, A, B, C,  5 );
        IROUND( C, D, E, F, G, H, A, B,  6 );
        IROUND( B, C, D, E, F, G, H, A,  7 );
        IROUND( A, B, C, D, E, F, G, H,  8 );
        IROUND( H, A, B, C, D, E, F, G,  9 );
        IROUND( G, H, A, B, C, D, E, F, 10 );
        IROUND( F, G, H, A, B, C, D, E, 11 );
        IROUND( E, F, G, H, A, B, C, D, 12 );
        IROUND( D, E, F, G, H, A, B, C, 13 );
        IROUND( C, D, E, F, G, H, A, B, 14 );
        IROUND( B, C, D, E, F, G, H, A, 15 );

        for( round=16; round<80; round += 16 )
        {
            FROUND( A, B, C, D, E, F, G, H, round +  0,  0 );
            FROUND( H, A, B, C, D, E, F, G, round +  1,  1 );
            FROUND( G, H, A, B, C, D, E, F, round +  2,  2 );
            FROUND( F, G, H, A, B, C, D, E, round +  3,  3 );
            FROUND( E, F, G, H, A, B, C, D, round +  4,  4 );
            FROUND( D, E, F, G, H, A, B, C, round +  5,  5 );
            FROUND( C, D, E, F, G, H, A, B, round +  6,  6 );
            FROUND( B, C, D, E, F, G, H, A, round +  7,  7 );
            FROUND( A, B, C, D, E, F, G, H, round +  8,  8 );
            FROUND( H, A, B, C, D, E, F, G, round +  9,  9 );
            FROUND( G, H, A, B, C, D, E, F, round + 10, 10 );
            FROUND( F, G, H, A, B, C, D, E, round + 11, 11 );
            FROUND( E, F, G, H, A, B, C, D, round + 12, 12 );
            FROUND( D, E, F, G, H, A, B, C, round + 13, 13 );
            FROUND( C, D, E, F, G, H, A, B, round + 14, 14 );
            FROUND( B, C, D, E, F, G, H, A, round + 15, 15 );
        }

        pChain->H[0] = A + pChain->H[0];
        pChain->H[1] = B + pChain->H[1];
        pChain->H[2] = C + pChain->H[2];
        pChain->H[3] = D + pChain->H[3];
        pChain->H[4] = E + pChain->H[4];
        pChain->H[5] = F + pChain->H[5];
        pChain->H[6] = G + pChain->H[6];
        pChain->H[7] = H + pChain->H[7];

        pbData += 128;
        cbData -= 128;
    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( W, sizeof( W ) );
    SYMCRYPT_FORCE_WRITE64( &A, 0 );
    SYMCRYPT_FORCE_WRITE64( &B, 0 );
    SYMCRYPT_FORCE_WRITE64( &C, 0 );
    SYMCRYPT_FORCE_WRITE64( &D, 0 );
    SYMCRYPT_FORCE_WRITE64( &E, 0 );
    SYMCRYPT_FORCE_WRITE64( &F, 0 );
    SYMCRYPT_FORCE_WRITE64( &G, 0 );
    SYMCRYPT_FORCE_WRITE64( &H, 0 );
    SYMCRYPT_FORCE_WRITE64( &Wt, 0 );
}

//
// UINT64 based implementation that
// first computes the expanded message, and then the
// actual hash computation.
// It tries to use fewer registers; this is probably a good approach for CPUs with only 8
// 64-bit registers; which is what you would use on x86 XMM, but we have XMM code below.
// This uses more memory, but might allow better register re-use and thereby
// reduce the number of load/stores.
//

VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ull2(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    SYMCRYPT_ALIGN UINT64 buf[4 + 8 + 80];    // 4 words original chaining state, chaining state, and expanded input block
    UINT64 * W = &buf[4 + 8];
    UINT64 * ha = &buf[4]; // initial state words, in order h, g, ..., b, a
    UINT64 A, B, C, D, T;
    int r;

    ha[7] = pChain->H[0]; buf[3] = ha[7];
    ha[6] = pChain->H[1]; buf[2] = ha[6];
    ha[5] = pChain->H[2]; buf[1] = ha[5];
    ha[4] = pChain->H[3]; buf[0] = ha[4];
    ha[3] = pChain->H[4];
    ha[2] = pChain->H[5];
    ha[1] = pChain->H[6];
    ha[0] = pChain->H[7];

    while( cbData >= 128 )
    {

        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r+= 2 )
        {
            W[r  ] = SYMCRYPT_LOAD_MSBFIRST64( &pbData[ 8* r    ] );
            W[r+1] = SYMCRYPT_LOAD_MSBFIRST64( &pbData[ 8*(r+1) ] );
        }

        //
        // Expand the message
        //
        A = W[15];
        B = W[14];
        D = W[0];
        for( r=16; r<80; r+= 2 )
        {
            // Loop invariant: A=W[r-1], B = W[r-2], D = W[r-16]

            //
            // Macro for one word of message expansion.
            // Invariant:
            // on entry: a = W[r-1], b = W[r-2], d = W[r-16]
            // on exit:  W[r] computed, a = W[r-1], b = W[r], c = W[r-15]
            //
            #define EXPAND( a, b, c, d, r ) \
                        c = W[r-15]; \
                        b = d + LSIGMA1( b ) + W[r-7] + LSIGMA0( c ); \
                        W[r] = b; \

            EXPAND( A, B, C, D, r );
            EXPAND( B, A, D, C, (r+1));

            #undef EXPAND
        }

        A = ha[7];
        B = ha[6];
        C = ha[5];
        D = ha[4];

        for( r=0; r<80; r += 4 )
        {
            //
            // Loop invariant:
            // A, B, C, and D are the a,b,c,d values of the current state.
            // W[r] is the next expanded message word to be processed.
            // W[r-8 .. r-5] contain the current state words h, g, f, e.
            //

            //
            // Macro to compute one round
            //
            #define DO_ROUND( a, b, c, d, t, r ) \
                t = W[r] + CSIGMA1( W[r-5] ) + W[r-8] + CH( W[r-5], W[r-6], W[r-7] ) + SymCryptSha512K[r]; \
                W[r-4] = t + d; \
                d = t + CSIGMA0( a ) + MAJ( c, b, a );

            DO_ROUND( A, B, C, D, T, r );
            DO_ROUND( D, A, B, C, T, (r+1) );
            DO_ROUND( C, D, A, B, T, (r+2) );
            DO_ROUND( B, C, D, A, T, (r+3) );
            #undef DO_ROUND
        }

        buf[3] = ha[7] = buf[3] + A;
        buf[2] = ha[6] = buf[2] + B;
        buf[1] = ha[5] = buf[1] + C;
        buf[0] = ha[4] = buf[0] + D;
        ha[3] += W[r-5];
        ha[2] += W[r-6];
        ha[1] += W[r-7];
        ha[0] += W[r-8];

        pbData += 128;
        cbData -= 128;
    }

    pChain->H[0] = ha[7];
    pChain->H[1] = ha[6];
    pChain->H[2] = ha[5];
    pChain->H[3] = ha[4];
    pChain->H[4] = ha[3];
    pChain->H[5] = ha[2];
    pChain->H[6] = ha[1];
    pChain->H[7] = ha[0];

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( buf, sizeof( buf ) );
    SYMCRYPT_FORCE_WRITE64( &A, 0 );
    SYMCRYPT_FORCE_WRITE64( &B, 0 );
    SYMCRYPT_FORCE_WRITE64( &C, 0 );
    SYMCRYPT_FORCE_WRITE64( &D, 0 );
    SYMCRYPT_FORCE_WRITE64( &T, 0 );

}

//
// UINT64 based implementation that
// first computes the expanded message, and then the
// actual hash computation.
// This one uses more registers than the previous one.
//

VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ull3(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    SYMCRYPT_ALIGN UINT64 W[80];
    SYMCRYPT_ALIGN UINT64 ha[8];
    UINT64 A, B, C, D, E, F, G, H;
    int r;

    ha[7] = pChain->H[0];
    ha[6] = pChain->H[1];
    ha[5] = pChain->H[2];
    ha[4] = pChain->H[3];
    ha[3] = pChain->H[4];
    ha[2] = pChain->H[5];
    ha[1] = pChain->H[6];
    ha[0] = pChain->H[7];

    while( cbData >= 128 )
    {

        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r+= 2 )
        {
            W[r  ] = SYMCRYPT_LOAD_MSBFIRST64( &pbData[ 8* r    ] );
            W[r+1] = SYMCRYPT_LOAD_MSBFIRST64( &pbData[ 8*(r+1) ] );
        }

        //
        // Expand the message
        //
        A = W[15];
        B = W[14];
        D = W[0];
        for( r=16; r<80; r+= 2 )
        {
            // Loop invariant: A=W[r-1], B = W[r-2], D = W[r-16]

            //
            // Macro for one word of message expansion.
            // Invariant:
            // on entry: a = W[r-1], b = W[r-2], d = W[r-16]
            // on exit:  W[r] computed, a = W[r-1], b = W[r], c = W[r-15]
            //
            #define EXPAND( a, b, c, d, r ) \
                        c = W[r-15]; \
                        b = d + LSIGMA1( b ) + W[r-7] + LSIGMA0( c ); \
                        W[r] = b; \

            EXPAND( A, B, C, D, r );
            EXPAND( B, A, D, C, (r+1));

            #undef EXPAND
        }

        A = ha[7];
        B = ha[6];
        C = ha[5];
        D = ha[4];
        E = ha[3];
        F = ha[2];
        G = ha[1];
        H = ha[0];

        for( r=0; r<80; r += 8 )
        {
            //
            // Loop invariant:
            // A, B, C, and D, E, F, G, H, are the values of the current state.
            // W[r] is the next expanded message word to be processed.
            //

            //
            // Macro to compute one round
            //
            #define DO_ROUND( a, b, c, d, e, f, g, h, r ) \
                h += W[r] + CSIGMA1( e ) + CH( e, f, g ) + SymCryptSha512K[r]; \
                d += h; \
                h += CSIGMA0( a ) + MAJ( c, b, a );

            DO_ROUND( A, B, C, D, E, F, G, H, (r  ) );
            DO_ROUND( H, A, B, C, D, E, F, G, (r+1) );
            DO_ROUND( G, H, A, B, C, D, E, F, (r+2) );
            DO_ROUND( F, G, H, A, B, C, D, E, (r+3) );
            DO_ROUND( E, F, G, H, A, B, C, D, (r+4) );
            DO_ROUND( D, E, F, G, H, A, B, C, (r+5) );
            DO_ROUND( C, D, E, F, G, H, A, B, (r+6) );
            DO_ROUND( B, C, D, E, F, G, H, A, (r+7) );
            #undef DO_ROUND
        }

        ha[7] += A;
        ha[6] += B;
        ha[5] += C;
        ha[4] += D;
        ha[3] += E;
        ha[2] += F;
        ha[1] += G;
        ha[0] += H;

        pbData += 128;
        cbData -= 128;
    }

    pChain->H[0] = ha[7];
    pChain->H[1] = ha[6];
    pChain->H[2] = ha[5];
    pChain->H[3] = ha[4];
    pChain->H[4] = ha[3];
    pChain->H[5] = ha[2];
    pChain->H[6] = ha[1];
    pChain->H[7] = ha[0];

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( W, sizeof( W ) );
    SymCryptWipeKnownSize( ha, sizeof( ha ) );
    SYMCRYPT_FORCE_WRITE64( &A, 0 );
    SYMCRYPT_FORCE_WRITE64( &B, 0 );
    SYMCRYPT_FORCE_WRITE64( &C, 0 );
    SYMCRYPT_FORCE_WRITE64( &D, 0 );
    SYMCRYPT_FORCE_WRITE64( &E, 0 );
    SYMCRYPT_FORCE_WRITE64( &F, 0 );
    SYMCRYPT_FORCE_WRITE64( &G, 0 );
    SYMCRYPT_FORCE_WRITE64( &H, 0 );
}

#undef MAJ
#undef CH
#undef CSIGMA0
#undef CSIGMA1
#undef LSIGMA0
#undef LSIGMA1
#undef CROUND
#undef IROUND
#undef FROUND

//======================================================================================
// Implementation using Xmm registers
//
#if SYMCRYPT_CPU_X86 // only on X86; AMD64 is faster when using UINT64s

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("ssse3"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("ssse3")
#endif


#define XMMADD( _a, _b ) _mm_add_epi64((_a), (_b))
#define XMMAND( _a, _b ) _mm_and_si128((_a), (_b))
#define XMMOR(  _a, _b ) _mm_or_si128((_a), (_b))
#define XMMROR( _a, _n ) _mm_xor_si128( _mm_slli_epi64( (_a), 64-(_n)), _mm_srli_epi64( (_a), (_n)) )
#define XMMSHR( _a, _n ) _mm_srli_epi64((_a), (_n))
#define XMMXOR( _a, _b ) _mm_xor_si128((_a), (_b))
#define XMMSTORE_UINT64( _a, _addr ) _mm_storel_epi64((__m128i*)(_addr), (_a))

#define XMMMAJ( x, y, z )  XMMOR( XMMAND( XMMOR( (z), (y)), (x)), XMMAND( (z), (y) ) )
#define XMMCH(  x, y, z )  XMMXOR( XMMAND( XMMXOR( (z), (y) ), (x)), (z))
#define XMMCSIGMA0( x )    XMMXOR( XMMXOR( XMMROR((x), 28), XMMROR((x), 34)), XMMROR((x), 39))
#define XMMCSIGMA1( x )    XMMXOR( XMMXOR( XMMROR((x), 14), XMMROR((x), 18)), XMMROR((x), 41))
#define XMMLSIGMA0( x )    XMMXOR( XMMXOR( XMMROR((x),  1), XMMROR((x),  8)), XMMSHR((x), 7))
#define XMMLSIGMA1( x )    XMMXOR( XMMXOR( XMMROR((x), 19), XMMROR((x), 61)), XMMSHR((x), 6))

//
// Core round takes two arguments: r16 = round number modulo 16, r = round number - r16.
// On entry, Wt must be equal to the sum of the round constant and the expanded message word for this round.
// Only the lower word of each Xmm register is used.
//
#define XMMCROUND( r16, r ) {;\
    ah[r16 & 7] = XMMADD( XMMADD( XMMADD( ah[r16 & 7], XMMCSIGMA1(ah[(r16+3)&7]) ), XMMCH(ah[(r16+3)&7], ah[(r16+2)&7], ah[(r16+1)&7]) ), Wt );\
    ah[(r16+4)&7] = XMMADD( ah[(r16+4)&7], ah[r16 &7] );\
    ah[r16 & 7] = XMMADD( XMMADD( ah[r16 & 7], XMMCSIGMA0(ah[(r16+7)&7])), XMMMAJ(ah[(r16+7)&7], ah[(r16+6)&7], ah[(r16+5)&7]) );\
}

#pragma warning( disable: 4127 )   // conditional expression is constant

//
// Initial round; reads data and performs a round.
// Data is read in 128-bit chunks every other round.
//
#define XMMIROUND( r ) {\
    if( (r&1) == 0 ) \
    { \
        Wt = _mm_loadu_si128( (__m128i *)&pbData[ 8*r ] ); \
        Wt = _mm_shuffle_epi8( Wt, BYTE_REVERSE_64 ); \
        W[r/2] = Wt; \
        Wt = XMMADD( Wt, _mm_load_si128( (__m128i *)&SymCryptSha512K[r] ) ); \
        Ws = _mm_srli_si128( Wt, 8 ); \
    } else {\
        Wt = Ws;\
    }\
    XMMCROUND( r, r );\
}

//
// Working version of XMMIROUND:
//    Wt = XMMFROM_MSBF( &pbData[ 8*r ] );\
//    W[r] = Wt;\
//    Wt = XMMADD( XMMFROM_UINT64(SymCryptSha512K[r]), Wt );\
//    XMMCROUND(r,r);\

#define XMMFROUND(r16, rb) { \
    if( (r16 & 1) == 0 ) \
    {\
        Wt = XMMADD( XMMADD( XMMADD(    XMMLSIGMA1( W[((r16 -  2)&15)/2] ), \
                                        _mm_alignr_epi8( W[((r16 - 6)&15)/2], W[((r16 - 7)&15)/2], 8 ) ), \
                                        XMMLSIGMA0( _mm_alignr_epi8( W[((r16 - 14)&15)/2], W[((r16 - 15)&15)/2], 8 ) ) ), \
                                        W[((r16 - 16)&15)/2] ); \
        W[r16/2] = Wt;\
        Ws = _mm_load_si128( (__m128i *)&SymCryptSha512K[r16 + rb] );\
        Wt = XMMADD( Ws , Wt );\
        Ws = _mm_srli_si128( Wt, 8 );\
    } else {\
        Wt = Ws;\
    }\
    XMMCROUND( r16, r16+rb ); \
}

VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_xmm(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    SYMCRYPT_ALIGN __m128i W[8];   // message expansion buffer, 8 elements each storing 2 consecutive UINT64s
    SYMCRYPT_ALIGN __m128i ah[8];
    SYMCRYPT_ALIGN __m128i feedf[8];
    int round;
    __m128i Wt, Ws;
    const __m128i BYTE_REVERSE_64 = _mm_set_epi8( 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 );

    Wt = _mm_loadu_si128( (__m128i *) &pChain->H[0] );
    feedf[7] = ah[7] = Wt;
    feedf[6] = ah[6] = _mm_srli_si128( Wt, 8 );
    Wt = _mm_loadu_si128( (__m128i *) &pChain->H[2] );
    feedf[5] = ah[5] = Wt;
    feedf[4] = ah[4] = _mm_srli_si128( Wt, 8 );
    Wt = _mm_loadu_si128( (__m128i *) &pChain->H[4] );
    feedf[3] = ah[3] = Wt;
    feedf[2] = ah[2] = _mm_srli_si128( Wt, 8 );
    Wt = _mm_loadu_si128( (__m128i *) &pChain->H[6] );
    feedf[1] = ah[1] = Wt;
    feedf[0] = ah[0] = _mm_srli_si128( Wt, 8 );

    while( cbData >= 128 )
    {
        //
        // initial rounds 1 to 16
        //

        XMMIROUND(  0 );
        XMMIROUND(  1 );
        XMMIROUND(  2 );
        XMMIROUND(  3 );
        XMMIROUND(  4 );
        XMMIROUND(  5 );
        XMMIROUND(  6 );
        XMMIROUND(  7 );
        XMMIROUND(  8 );
        XMMIROUND(  9 );
        XMMIROUND( 10 );
        XMMIROUND( 11 );
        XMMIROUND( 12 );
        XMMIROUND( 13 );
        XMMIROUND( 14 );
        XMMIROUND( 15 );

        for( round=16; round<80; round += 16 )
        {
            XMMFROUND(  0, round );
            XMMFROUND(  1, round );
            XMMFROUND(  2, round );
            XMMFROUND(  3, round );
            XMMFROUND(  4, round );
            XMMFROUND(  5, round );
            XMMFROUND(  6, round );
            XMMFROUND(  7, round );
            XMMFROUND(  8, round );
            XMMFROUND(  9, round );
            XMMFROUND( 10, round );
            XMMFROUND( 11, round );
            XMMFROUND( 12, round );
            XMMFROUND( 13, round );
            XMMFROUND( 14, round );
            XMMFROUND( 15, round );
        }

        feedf[0] = ah[0] = XMMADD( ah[0], feedf[0] );
        feedf[1] = ah[1] = XMMADD( ah[1], feedf[1] );
        feedf[2] = ah[2] = XMMADD( ah[2], feedf[2] );
        feedf[3] = ah[3] = XMMADD( ah[3], feedf[3] );
        feedf[4] = ah[4] = XMMADD( ah[4], feedf[4] );
        feedf[5] = ah[5] = XMMADD( ah[5], feedf[5] );
        feedf[6] = ah[6] = XMMADD( ah[6], feedf[6] );
        feedf[7] = ah[7] = XMMADD( ah[7], feedf[7] );

        pbData += 128;
        cbData -= 128;

    }

    XMMSTORE_UINT64( ah[7], &(pChain->H[0]) );
    XMMSTORE_UINT64( ah[6], &(pChain->H[1]) );
    XMMSTORE_UINT64( ah[5], &(pChain->H[2]) );
    XMMSTORE_UINT64( ah[4], &(pChain->H[3]) );
    XMMSTORE_UINT64( ah[3], &(pChain->H[4]) );
    XMMSTORE_UINT64( ah[2], &(pChain->H[5]) );
    XMMSTORE_UINT64( ah[1], &(pChain->H[6]) );
    XMMSTORE_UINT64( ah[0], &(pChain->H[7]) );

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( ah, sizeof( ah ) );
    SymCryptWipeKnownSize( feedf, sizeof( feedf ) );
    SymCryptWipeKnownSize( W, sizeof( W ) );
    SymCryptWipeKnownSize( &Wt, sizeof( Wt ));
    SymCryptWipeKnownSize( &Ws, sizeof( Ws ));
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif



//======================================================================================
// Implementation using NEON registers
//
#if SYMCRYPT_CPU_ARM


#define ROR( _a, _n ) vorr_u64( vshl_n_u64( _a, 64 - _n ), vshr_n_u64( _a, _n ) )
#define ADD( x, y ) vadd_u64( (x), (y) )

#define MAJ( x, y, z )  vorr_u64( vand_u64( vorr_u64( (z), (y)), (x)), vand_u64( (z), (y) ) )
#define CH(  x, y, z )  veor_u64( vand_u64( veor_u64( (z), (y) ), (x)), (z))
#define CSIGMA0( x )    veor_u64( veor_u64( ROR((x), 28), ROR((x), 34)), ROR((x), 39))
#define CSIGMA1( x )    veor_u64( veor_u64( ROR((x), 14), ROR((x), 18)), ROR((x), 41))
#define LSIGMA0( x )    veor_u64( veor_u64( ROR((x),  1), ROR((x),  8)), vshr_n_u64((x), 7))
#define LSIGMA1( x )    veor_u64( veor_u64( ROR((x), 19), ROR((x), 61)), vshr_n_u64((x), 6))

//
// r = round number, r16 = r mod 16 (often a compile-time constant when r is not)
//
#define CROUND( a, b, c, d, e, f, g, h, r, r16 ) {\
    W[r16] = Wt; \
    h = ADD( h, ADD( ADD( ADD( CSIGMA1(e), CH(e, f, g)), *(__n64 *)&SymCryptSha512K[r]),  Wt ));\
    d = ADD( d, h );\
    h = ADD( h, ADD( CSIGMA0(a), MAJ(a, b, c)));\
}

//
// Initial round that reads the message.
// r is the round number 0..15
//
#define IROUND( a, b, c, d, e, f, g, h, r ) {\
    Wt = vmov_n_u64( SYMCRYPT_LOAD_MSBFIRST64( &pbData[ 8*r ] ) );\
    CROUND( a, b, c, d, e, f, g, h, r, r);\
    }
//
// Subsequent rounds.
// r is the round number, r16 is the round number mod 16.
// These are separate as typically r is run-time and r16 is compile time constant.
//
#define FROUND( a, b, c, d, e, f, g, h, r, r16 ) {                                      \
    Wt = ADD( ADD( LSIGMA1( W[(r16-2) & 15] ), LSIGMA0( W[(r16-15) & 15])) , ADD( W[(r16-7) & 15], W[r16 & 15])); \
    CROUND( a, b, c, d, e, f, g, h, r, r16 ); \
    }

//
// This is the core routine that does the actual hard work
// This is based on the older one in RSA32LIB by Scott Field from 2001
//
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_neon(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    SYMCRYPT_ALIGN __n64 W[16];
    __n64 A, B, C, D, E, F, G, H;
    int round;
    __n64 Wt;
    __n64 * pH = (__n64 *) &pChain->H[0];

    A = pH[0];
    B = pH[1];
    C = pH[2];
    D = pH[3];
    E = pH[4];
    F = pH[5];
    G = pH[6];
    H = pH[7];

    while( cbData >= 128 )
    {
        //
        // initial rounds 1 to 16
        //

        IROUND( A, B, C, D, E, F, G, H,  0 );
        IROUND( H, A, B, C, D, E, F, G,  1 );
        IROUND( G, H, A, B, C, D, E, F,  2 );
        IROUND( F, G, H, A, B, C, D, E,  3 );
        IROUND( E, F, G, H, A, B, C, D,  4 );
        IROUND( D, E, F, G, H, A, B, C,  5 );
        IROUND( C, D, E, F, G, H, A, B,  6 );
        IROUND( B, C, D, E, F, G, H, A,  7 );
        IROUND( A, B, C, D, E, F, G, H,  8 );
        IROUND( H, A, B, C, D, E, F, G,  9 );
        IROUND( G, H, A, B, C, D, E, F, 10 );
        IROUND( F, G, H, A, B, C, D, E, 11 );
        IROUND( E, F, G, H, A, B, C, D, 12 );
        IROUND( D, E, F, G, H, A, B, C, 13 );
        IROUND( C, D, E, F, G, H, A, B, 14 );
        IROUND( B, C, D, E, F, G, H, A, 15 );

        for( round=16; round<80; round += 16 )
        {
            FROUND( A, B, C, D, E, F, G, H, round +  0,  0 );
            FROUND( H, A, B, C, D, E, F, G, round +  1,  1 );
            FROUND( G, H, A, B, C, D, E, F, round +  2,  2 );
            FROUND( F, G, H, A, B, C, D, E, round +  3,  3 );
            FROUND( E, F, G, H, A, B, C, D, round +  4,  4 );
            FROUND( D, E, F, G, H, A, B, C, round +  5,  5 );
            FROUND( C, D, E, F, G, H, A, B, round +  6,  6 );
            FROUND( B, C, D, E, F, G, H, A, round +  7,  7 );
            FROUND( A, B, C, D, E, F, G, H, round +  8,  8 );
            FROUND( H, A, B, C, D, E, F, G, round +  9,  9 );
            FROUND( G, H, A, B, C, D, E, F, round + 10, 10 );
            FROUND( F, G, H, A, B, C, D, E, round + 11, 11 );
            FROUND( E, F, G, H, A, B, C, D, round + 12, 12 );
            FROUND( D, E, F, G, H, A, B, C, round + 13, 13 );
            FROUND( C, D, E, F, G, H, A, B, round + 14, 14 );
            FROUND( B, C, D, E, F, G, H, A, round + 15, 15 );
        }

        pH[0] = A = ADD( A, pH[0] );
        pH[1] = B = ADD( B, pH[1] );
        pH[2] = C = ADD( C, pH[2] );
        pH[3] = D = ADD( D, pH[3] );
        pH[4] = E = ADD( E, pH[4] );
        pH[5] = F = ADD( F, pH[5] );
        pH[6] = G = ADD( G, pH[6] );
        pH[7] = H = ADD( H, pH[7] );

        pbData += 128;
        cbData -= 128;
    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( W, sizeof( W ) );
    SymCryptWipeKnownSize( &A, sizeof( A ) );
    SymCryptWipeKnownSize( &B, sizeof( B ) );
    SymCryptWipeKnownSize( &C, sizeof( C ) );
    SymCryptWipeKnownSize( &D, sizeof( D ) );
    SymCryptWipeKnownSize( &E, sizeof( E ) );
    SymCryptWipeKnownSize( &F, sizeof( F ) );
    SymCryptWipeKnownSize( &G, sizeof( G ) );
    SymCryptWipeKnownSize( &H, sizeof( H ) );
    SymCryptWipeKnownSize( &Wt, sizeof( Wt ) );
}

#endif

//======================================================================================
//
// Switch between different implementations of compression function
//
//FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE *    pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
#if SYMCRYPT_CPU_AMD64

    // Temporarily disabling use of Ymm in SHA2
    // SYMCRYPT_EXTENDED_SAVE_DATA SaveData;

    // if (SYMCRYPT_CPU_FEATURES_PRESENT(SYMCRYPT_CPU_FEATURE_AVX512 | SYMCRYPT_CPU_FEATURE_BMI2) &&
    //     SymCryptSaveYmm(&SaveData) == SYMCRYPT_NO_ERROR)
    // {
    //     SymCryptSha512AppendBlocks_ymm_avx512vl_asm(pChain, pbData, cbData, pcbRemaining);

    //     SymCryptRestoreYmm(&SaveData);
    // }
    // else if (SYMCRYPT_CPU_FEATURES_PRESENT(SYMCRYPT_CPU_FEATURE_AVX2 | SYMCRYPT_CPU_FEATURE_BMI2) &&
    //     SymCryptSaveYmm(&SaveData) == SYMCRYPT_NO_ERROR)
    // {
    //     //SymCryptSha512AppendBlocks_ymm_1block(pChain, pbData, cbData, pcbRemaining);
    //     //SymCryptSha512AppendBlocks_ymm_2blocks(pChain, pbData, cbData, pcbRemaining);
    //     //SymCryptSha512AppendBlocks_ymm_4blocks(pChain, pbData, cbData, pcbRemaining);
    //     SymCryptSha512AppendBlocks_ymm_avx2_asm(pChain, pbData, cbData, pcbRemaining);

    //     SymCryptRestoreYmm(&SaveData);
    // }
    // else
    {
        SymCryptSha512AppendBlocks_ull( pChain, pbData, cbData, pcbRemaining );
        //SymCryptSha512AppendBlocks_ull2( pChain, pbData, cbData, pcbRemaining );
        //SymCryptSha512AppendBlocks_ull3( pChain, pbData, cbData, pcbRemaining );
    }


#elif SYMCRYPT_CPU_ARM

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON ) )
    {
        SymCryptSha512AppendBlocks_neon( pChain, pbData, cbData, pcbRemaining );      // Tegra T3: 48 c/B
    } else {
        SymCryptSha512AppendBlocks_ull( pChain, pbData, cbData, pcbRemaining );       // Tegra T3: 65.34 c/B
        //SymCryptSha512AppendBlocks_ull2( pChain, pbData, cbData, pcbRemaining );      // Tegra T3: 77.4 c/B
        //SymCryptSha512AppendBlocks_ull3( pChain, pbData, cbData, pcbRemaining );      // Tegra T3: 71.6 c/B
    }

#elif SYMCRYPT_CPU_X86

    SYMCRYPT_EXTENDED_SAVE_DATA SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSSE3 ) && SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptSha512AppendBlocks_xmm( pChain, pbData, cbData, pcbRemaining );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptSha512AppendBlocks_ull( pChain, pbData, cbData, pcbRemaining );       // core2: 36.40 c/B
        //SymCryptSha512AppendBlocks_ull2( pChain, pbData, cbData, pcbRemaining );      // core2: 49.09 c/B
        //SymCryptSha512AppendBlocks_ull3( pChain, pbData, cbData, pcbRemaining );      // core2: 38.29 c/B
    }

#else

    SymCryptSha512AppendBlocks_ull( pChain, pbData, cbData, pcbRemaining );       // need tuning...

#endif
}
