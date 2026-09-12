//
// Sha256.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement SHA2-256 from FIPS 180-2
//
// This revised implementation is based on the older one in RSA32LIB by Scott Field from 2001
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//

const SYMCRYPT_HASH SymCryptSha224Algorithm_default = {
    &SymCryptSha224Init,
    &SymCryptSha224Append,
    &SymCryptSha224Result,
    &SymCryptSha256AppendBlocks,
    &SymCryptSha224StateCopy,
    sizeof( SYMCRYPT_SHA224_STATE ),
    SYMCRYPT_SHA224_RESULT_SIZE,
    SYMCRYPT_SHA224_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_SHA224_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_SHA224_STATE, chain ),
};

const SYMCRYPT_HASH SymCryptSha256Algorithm_default = {
    &SymCryptSha256Init,
    &SymCryptSha256Append,
    &SymCryptSha256Result,
    &SymCryptSha256AppendBlocks,
    &SymCryptSha256StateCopy,
    sizeof( SYMCRYPT_SHA256_STATE ),
    SYMCRYPT_SHA256_RESULT_SIZE,
    SYMCRYPT_SHA256_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_SHA256_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_SHA256_STATE, chain ),
};

const PCSYMCRYPT_HASH SymCryptSha224Algorithm = &SymCryptSha224Algorithm_default;
const PCSYMCRYPT_HASH SymCryptSha256Algorithm = &SymCryptSha256Algorithm_default;

//
// SHA-256 uses 64 magic constants of 32 bits each. These are
// referred to as K^{256}_i for i=0...63 by FIPS 180-2.
// This array is also used by the parallel SHA256 implementation
// For performance we align to 256 bytes, which gives optimal cache alignment.
//
SYMCRYPT_ALIGN_AT( 256 ) const  UINT32 SymCryptSha256K[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
    0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
    0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
    0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
    0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};


//
// Initial state
//
static const UINT32 sha224InitialState[8] = {
    0xc1059ed8UL,
    0x367cd507UL,
    0x3070dd17UL,
    0xf70e5939UL,
    0xffc00b31UL,
    0x68581511UL,
    0x64f98fa7UL,
    0xbefa4fa4UL,
};

static const UINT32 sha256InitialState[8] = {
    0x6a09e667UL,
    0xbb67ae85UL,
    0x3c6ef372UL,
    0xa54ff53aUL,
    0x510e527fUL,
    0x9b05688cUL,
    0x1f83d9abUL,
    0x5be0cd19UL,
};

//
// SymCryptSha224
//
#define ALG SHA224
#define Alg Sha224
#include "hash_pattern.c"
#undef ALG
#undef Alg

//
// SymCryptSha256
//
#define ALG SHA256
#define Alg Sha256
#include "hash_pattern.c"
#undef ALG
#undef Alg



//
// SymCryptSha256Init
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha256Init( _Out_ PSYMCRYPT_SHA256_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthL = 0;
    //pState->dataLengthH = 0;      // not used
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &sha256InitialState[0], sizeof( sha256InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


//
// SymCryptSha224Init
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha224Init( _Out_ PSYMCRYPT_SHA224_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthL = 0;
    //pState->dataLengthH = 0;      // not used
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &sha224InitialState[0], sizeof( sha224InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


//
// SymCryptSha256Append
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha256Append(
    _Inout_                 PSYMCRYPT_SHA256_STATE  pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData )
{
    UINT32  bytesInBuffer;
    UINT32  freeInBuffer;
    SIZE_T  tmp;

    SYMCRYPT_CHECK_MAGIC( pState );

    pState->dataLengthL += cbData;      // dataLengthH is not used...

    bytesInBuffer = pState->bytesInBuffer;

    //
    // If previous data in buffer, buffer new input and transform if possible.
    //
    if( bytesInBuffer > 0 )
    {
        SYMCRYPT_ASSERT( SYMCRYPT_SHA256_INPUT_BLOCK_SIZE > bytesInBuffer );

        freeInBuffer = SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - bytesInBuffer;
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
            SymCryptSha256AppendBlocks( &pState->chain, &pState->buffer[0], SYMCRYPT_SHA256_INPUT_BLOCK_SIZE, &tmp );

            bytesInBuffer = 0;
        }
    }

    //
    // Internal buffer is empty; process all remaining whole blocks in the input
    //
    if( cbData >= SYMCRYPT_SHA256_INPUT_BLOCK_SIZE )
    {
        SymCryptSha256AppendBlocks( &pState->chain, pbData, cbData, &tmp );
        SYMCRYPT_ASSERT( tmp < SYMCRYPT_SHA256_INPUT_BLOCK_SIZE );
        pbData += cbData - tmp;
        cbData = tmp;
    }

    SYMCRYPT_ASSERT( cbData < SYMCRYPT_SHA256_INPUT_BLOCK_SIZE );

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


//
// SymCryptSha224Append
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha224Append(
    _Inout_                 PSYMCRYPT_SHA224_STATE  pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData )
{
    SymCryptSha256Append( (PSYMCRYPT_SHA256_STATE)pState, pbData, cbData );
}


//
// SymCryptSha256Result
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha256Result(
    _Inout_                                     PSYMCRYPT_SHA256_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA256_RESULT_SIZE ) PBYTE                   pbResult )
{
    //
    // We don't use the common padding code as that is slower, and SHA-256 is very frequently used in
    // performance-sensitive areas.
    //
    UINT32 bytesInBuffer;
    SIZE_T tmp;

    SYMCRYPT_CHECK_MAGIC( pState );

    bytesInBuffer = pState->bytesInBuffer;

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
        SymCryptSha256AppendBlocks( &pState->chain, pState->buffer, 64, &tmp );
        bytesInBuffer = 0;
    }

    //
    // Set rest of padding
    // At this point bytesInBuffer <= 64-8, so we don't have an underflow
    // We wipe to the end of the buffer as it is 16-aligned,
    // and it is faster to wipe to an aligned point
    //
    SymCryptWipe( &pState->buffer[bytesInBuffer], 64-bytesInBuffer );
    SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[64-8], pState->dataLengthL * 8 );

    //
    // Process the final block
    //
    SymCryptSha256AppendBlocks( &pState->chain, pState->buffer, 64, &tmp );

    //
    // Write the output in the correct byte order
    //
    SymCryptUint32ToMsbFirst( &pState->chain.H[0], pbResult, 8 );

    //
    // Wipe & re-initialize
    // We have to wipe the whole state because the Init call
    // might be optimized away by a smart compiler.
    //
    SymCryptWipeKnownSize( pState, sizeof( *pState ) );

    memcpy( &pState->chain.H[0], &sha256InitialState[0], sizeof( sha256InitialState ) );
    SYMCRYPT_SET_MAGIC( pState );
}


//
// SymCryptSha224Result
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha224Result(
    _Inout_                                     PSYMCRYPT_SHA224_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA224_RESULT_SIZE ) PBYTE                   pbResult )
{
    SYMCRYPT_ALIGN BYTE sha256Result[SYMCRYPT_SHA256_RESULT_SIZE];      // Buffer for SHA-256 output

    //
    // The SHA-3224 result is the first 28 bytes of the SHA-256 result of our state
    //
    SymCryptSha256Result( (PSYMCRYPT_SHA256_STATE)pState, sha256Result );
    memcpy( pbResult, sha256Result, SYMCRYPT_SHA224_RESULT_SIZE );

    //
    // The buffer was already wiped by the SymCryptSha256Result function, we
    // just have to re-initialize for SHA-224
    //
    SymCryptSha224Init( pState );

    SymCryptWipeKnownSize( sha256Result, sizeof( sha256Result ) );
}


VOID
SYMCRYPT_CALL
SymCryptSha256StateExportCore(
    _In_                                                    PCSYMCRYPT_SHA256_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA256_STATE_EXPORT_SIZE ) PBYTE                   pbBlob,
    _In_                                                    UINT32                  type )
{
    SYMCRYPT_ALIGN SYMCRYPT_SHA256_STATE_EXPORT_BLOB blob;           // local copy to have proper alignment.
    C_ASSERT( sizeof( blob ) == SYMCRYPT_SHA256_STATE_EXPORT_SIZE );

    SYMCRYPT_CHECK_MAGIC( pState );

    SymCryptWipeKnownSize( &blob, sizeof( blob ) ); // wipe to avoid any data leakage

    blob.header.magic = SYMCRYPT_BLOB_MAGIC;
    blob.header.size = SYMCRYPT_SHA256_STATE_EXPORT_SIZE;
    blob.header.type = type;

    //
    // Copy the relevant data. Buffer will be 0-padded.
    //

    SymCryptUint32ToMsbFirst( &pState->chain.H[0], &blob.chain[0], 8 );
    blob.dataLength = pState->dataLengthL;
    memcpy( &blob.buffer[0], &pState->buffer[0], blob.dataLength & 0x3f );

    SYMCRYPT_ASSERT( (PCBYTE) &blob + sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ) == (PCBYTE) &blob.trailer );
    SymCryptMarvin32( SymCryptMarvin32DefaultSeed, (PCBYTE) &blob, sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ), &blob.trailer.checksum[0] );

    memcpy( pbBlob, &blob, sizeof( blob ) );

//cleanup:
    SymCryptWipeKnownSize( &blob, sizeof( blob ) );
    return;
}


VOID
SYMCRYPT_CALL
SymCryptSha256StateExport(
    _In_                                                    PCSYMCRYPT_SHA256_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA256_STATE_EXPORT_SIZE ) PBYTE                   pbBlob)
{
    SymCryptSha256StateExportCore( pState, pbBlob, SymCryptBlobTypeSha256State );
}


VOID
SYMCRYPT_CALL
SymCryptSha224StateExport(
    _In_                                                    PCSYMCRYPT_SHA224_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA256_STATE_EXPORT_SIZE ) PBYTE                   pbBlob)
{
    SymCryptSha256StateExportCore( (PSYMCRYPT_SHA256_STATE)pState, pbBlob, SymCryptBlobTypeSha224State );
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha256StateImportCore(
    _Out_                                                   PSYMCRYPT_SHA256_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA256_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob,
    _In_                                                    UINT32                  type )
{
    SYMCRYPT_ERROR                      scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_ALIGN SYMCRYPT_SHA256_STATE_EXPORT_BLOB   blob;                       // local copy to have proper alignment.
    BYTE                                checksum[8];

    C_ASSERT( sizeof( blob ) == SYMCRYPT_SHA256_STATE_EXPORT_SIZE );
    memcpy( &blob, pbBlob, sizeof( blob ) );

    if( blob.header.magic != SYMCRYPT_BLOB_MAGIC ||
        blob.header.size != SYMCRYPT_SHA256_STATE_EXPORT_SIZE ||
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

    SymCryptMsbFirstToUint32( &blob.chain[0], &pState->chain.H[0], 8 );
    pState->dataLengthL = blob.dataLength;
    pState->bytesInBuffer = blob.dataLength & 0x3f;
    memcpy( &pState->buffer[0], &blob.buffer[0], pState->bytesInBuffer );

    SYMCRYPT_SET_MAGIC( pState );

cleanup:
    SymCryptWipeKnownSize( &blob, sizeof(blob) );
    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha256StateImport(
    _Out_                                                   PSYMCRYPT_SHA256_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA256_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob )
{
    return SymCryptSha256StateImportCore( pState, pbBlob, SymCryptBlobTypeSha256State );
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha224StateImport(
    _Out_                                                   PSYMCRYPT_SHA224_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA224_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob )
{
    return SymCryptSha256StateImportCore( (PSYMCRYPT_SHA256_STATE)pState, pbBlob, SymCryptBlobTypeSha224State );
}



//
// Simple test vector for FIPS module testing
//

const BYTE SymCryptSha256KATAnswer[ 32 ] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
    } ;

VOID
SYMCRYPT_CALL
SymCryptSha256Selftest(void)
{
    BYTE result[SYMCRYPT_SHA256_RESULT_SIZE];

    SymCryptSha256( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, SymCryptSha256KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'SH25' );
    }
}

//
// Simple test vector for FIPS module testing
//

const BYTE SymCryptSha224KATAnswer[ 28 ] = {
    0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22,
    0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3,
    0x2a, 0xad, 0xbc, 0xe4, 0xbd, 0xa0, 0xb3, 0xf7,
    0xe3, 0x6c, 0x9d, 0xa7,
    } ;

VOID
SYMCRYPT_CALL
SymCryptSha224Selftest(void)
{
    BYTE result[SYMCRYPT_SHA224_RESULT_SIZE];

    SymCryptSha224( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, SymCryptSha224KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'SH22' );
    }
}



//
// Below are multiple implementations of the SymCryptSha256AppendBlocks function,
// with a compile-time switch about which one to use.
// We keep the multiple implementations here for future reference;
// as CPU architectures evolve we might want to switch to one of the
// other implementations.
// All implementations here have been tested, but some lack production hardening.
//

//
// Enable frame pointer omission to free up an extra register on X86.
//
#if SYMCRYPT_CPU_X86 && SYMCRYPT_MS_VC && !defined(__clang__)
#pragma optimize( "y", on )
#endif

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

//
// We have two versions of the rotate-and-xor functions.
// one is just a macro that does the rotations and xors.
// This works well on ARM
// For Intel/AMD we have one where we use the rotated value
// from one intermediate result to derive the next rotated
// value from. This removes one register copy from the
// code stream.
//
// In practice, our compiler doesn't take advantage of the
// reduction in the # operations required, and inserts a
// bunch of extra register copies anyway.
// It actually hurts on AMD64.
//
// This should be re-tuned for every release to get the best overall
// SHA-256 performance.
// At the moment we get an improvement from 19.76 c/B to 19.40 c/B on a Core 2 core.
// We should probably tune this to the Atom CPU.
//
#if SYMCRYPT_CPU_X86
#define USE_CSIGMA0_MULTIROT 1
#define USE_CSIGMA1_MULTIROT 0
#define USE_LSIGMA0_MULTIROT 0
#define USE_LSIGMA1_MULTIROT 0

#else
//
// On ARM we have no reason to believe this helps at all.
// on AMD64 it slows our code down.
//
#define USE_CSIGMA0_MULTIROT 0
#define USE_CSIGMA1_MULTIROT 0
#define USE_LSIGMA0_MULTIROT 0
#define USE_LSIGMA1_MULTIROT 0
#endif

#if USE_CSIGMA0_MULTIROT
FORCEINLINE
UINT32
CSIGMA0( UINT32 x )
{
    UINT32 res;
    x = ROR32( x, 2 );
    res = x;
    x = ROR32( x, 11 );
    res ^= x;
    x = ROR32( x, 9 );
    res ^= x;
    return res;
}
#else
#define CSIGMA0( x )    (ROR32((x),  2) ^ ROR32((x), 13) ^ ROR32((x), 22))
#endif

#if USE_CSIGMA1_MULTIROT
FORCEINLINE
UINT32
CSIGMA1( UINT32 x )
{
    UINT32 res;
    x = ROR32( x, 6 );
    res = x;
    x = ROR32( x, 5 );
    res ^= x;
    x = ROR32( x, 14 );
    res ^= x;
    return res;
}
#else
#define CSIGMA1( x )    (ROR32((x),  6) ^ ROR32((x), 11) ^ ROR32((x), 25))
#endif

#if USE_LSIGMA0_MULTIROT
FORCEINLINE
UINT32
LSIGMA0( UINT32 x )
{
    UINT32 res;
    res = x >> 3;
    x = ROR32( x, 7 );
    res ^= x;
    x = ROR32( x, 11 );
    res ^= x;
    return res;
}
#else
#define LSIGMA0( x )    (ROR32((x),  7) ^ ROR32((x), 18) ^ ((x)>> 3))
#endif

#if USE_LSIGMA1_MULTIROT
FORCEINLINE
UINT32
LSIGMA1( UINT32 x )
{
    UINT32 res;
    res = x >> 10;
    x = ROR32( x, 17 );
    res ^= x;
    x = ROR32( x, 2 );
    res ^= x;
    return res;
}
#else
#define LSIGMA1( x )    (ROR32((x), 17) ^ ROR32((x), 19) ^ ((x)>>10))
#endif


//
// The values a-h are stored in an array called ah.
// We have unrolled the loop 16 times. This makes both the indices into
// the ah array constant, and it makes the message addressing constant.
// This provides a significant speed improvement, at the cost of making
// the main loop about 4 kB in code.
//
// The earlier implementation had the loop unrolled 8 times, and is
// around 10 cycles/byte slower. If loading the code from disk takes
// 100 cycles/byte, then we break even once you have hashed 20 kB.
// This is a worthwhile tradeoff as all code is codesigned with SHA-256.
//

//
// Core round macro
//
// r16 is the round number mod 16, r is the round number.
// r16 is a separate macro argument because it is always a compile-time constant
// which allows much better optimizations of the memory accesses.
//
// ah[ r16   &7] = h
// ah[(r16+1)&7] = g;
// ah[(r16+2)&7] = f;
// ah[(r16+3)&7] = e;
// ah[(r16+4)&7] = d;
// ah[(r16+5)&7] = c;
// ah[(r16+6)&7] = b;
// ah[(r16+7)&7] = a;
//
// After that incrementing the round number will automatically map a->b, b->c, etc.
//
// The core round, after the message word has been computed for this round and put in Wt.
// r16 is the round number modulo 16. (Static after loop unrolling)
// r is the round number (dynamic, which is why we don't use (r&0xf) for r16)
// In more readable form this macro does the following:
//      h += CSIGMA( e ) + CH( e, f, g ) + K[round] + W[round];
//      d += h;
//      h += CSIGMA( a ) + MAJ( a, b, c );
//
#define CROUND( r16, r ) {;\
    ah[ r16   &7] += CSIGMA1(ah[(r16+3)&7]) + CH(ah[(r16+3)&7], ah[(r16+2)&7], ah[(r16+1)&7]) + SymCryptSha256K[r] + Wt;\
    ah[(r16+4)&7] += ah[r16 &7];\
    ah[ r16   &7] += CSIGMA0(ah[(r16+7)&7]) + MAJ(ah[(r16+7)&7], ah[(r16+6)&7], ah[(r16+5)&7]);\
}

//
// Initial round that reads the message.
// r is the round number 0..15
//
#define IROUND( r ) {\
    Wt = SYMCRYPT_LOAD_MSBFIRST32( &pbData[ 4*r ] );\
    W[r] = Wt; \
    CROUND(r,r);\
    }

//
// Subsequent rounds.
// r16 is the round number mod 16. rb is the round number minus r16.
//
#define FROUND(r16, rb) {                                      \
    Wt = LSIGMA1( W[(r16-2) & 15] ) +   W[(r16-7) & 15] +    \
         LSIGMA0( W[(r16-15) & 15]) +   W[r16 & 15];       \
    W[r16] = Wt; \
    CROUND( r16, r16+rb ); \
}

//
// UINT32 implementation 1
//
VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_ul1(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE *    pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    SYMCRYPT_ALIGN UINT32 W[16];
    SYMCRYPT_ALIGN UINT32 ah[8];
    int round;
    UINT32 Wt;

    while( cbData >= 64 )
    {
        ah[7] = pChain->H[0];
        ah[6] = pChain->H[1];
        ah[5] = pChain->H[2];
        ah[4] = pChain->H[3];
        ah[3] = pChain->H[4];
        ah[2] = pChain->H[5];
        ah[1] = pChain->H[6];
        ah[0] = pChain->H[7];

        //
        // initial rounds 1 to 16
        //

        IROUND(  0 );
        IROUND(  1 );
        IROUND(  2 );
        IROUND(  3 );
        IROUND(  4 );
        IROUND(  5 );
        IROUND(  6 );
        IROUND(  7 );
        IROUND(  8 );
        IROUND(  9 );
        IROUND( 10 );
        IROUND( 11 );
        IROUND( 12 );
        IROUND( 13 );
        IROUND( 14 );
        IROUND( 15 );


        //
        // rounds 16 to 64.
        //
        for( round=16; round<64; round += 16 )
        {
            FROUND(  0, round );
            FROUND(  1, round );
            FROUND(  2, round );
            FROUND(  3, round );
            FROUND(  4, round );
            FROUND(  5, round );
            FROUND(  6, round );
            FROUND(  7, round );
            FROUND(  8, round );
            FROUND(  9, round );
            FROUND( 10, round );
            FROUND( 11, round );
            FROUND( 12, round );
            FROUND( 13, round );
            FROUND( 14, round );
            FROUND( 15, round );
        }

        pChain->H[0] = ah[7] + pChain->H[0];
        pChain->H[1] = ah[6] + pChain->H[1];
        pChain->H[2] = ah[5] + pChain->H[2];
        pChain->H[3] = ah[4] + pChain->H[3];
        pChain->H[4] = ah[3] + pChain->H[4];
        pChain->H[5] = ah[2] + pChain->H[5];
        pChain->H[6] = ah[1] + pChain->H[6];
        pChain->H[7] = ah[0] + pChain->H[7];

        pbData += 64;
        cbData -= 64;

    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( ah, sizeof( ah ) );
    SymCryptWipeKnownSize( W, sizeof( W ) );
    SYMCRYPT_FORCE_WRITE32( &Wt, 0 );
}

VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_ul2(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE *    pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    //
    // Different arrangement of the code, currently 25 c/B vs 20 c/b for the version above.
    // On Atom: 50 c/B vs 41 c/B for the one above.
    //
    SYMCRYPT_ALIGN UINT32 buf[4 + 8 + 64];    // chaining state concatenated with the expanded input block
    UINT32 * W = &buf[4 + 8];
    UINT32 * ha = &buf[4]; // initial state words, in order h, g, ..., b, a
    UINT32 A, B, C, D, T;
    int r;

    ha[7] = pChain->H[0]; buf[3] = ha[7];
    ha[6] = pChain->H[1]; buf[2] = ha[6];
    ha[5] = pChain->H[2]; buf[1] = ha[5];
    ha[4] = pChain->H[3]; buf[0] = ha[4];
    ha[3] = pChain->H[4];
    ha[2] = pChain->H[5];
    ha[1] = pChain->H[6];
    ha[0] = pChain->H[7];

    while( cbData >= 64 )
    {
        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r++ )
        {
            W[r] = SYMCRYPT_LOAD_MSBFIRST32( &pbData[ 4*r ] );
        }

        //
        // Expand the message
        //
        A = W[15];
        B = W[14];
        D = W[0];
        for( r=16; r<64; r+= 2 )
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
                        b =  d + LSIGMA1( b ) + W[r-7] + LSIGMA0( c ); \
                        W[r] = b; \

            EXPAND( A, B, C, D, r );
            EXPAND( B, A, D, C, (r+1));

            #undef EXPAND
        }

        A = ha[7];
        B = ha[6];
        C = ha[5];
        D = ha[4];

        for( r=0; r<64; r += 4 )
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
                t = W[r] + CSIGMA1( W[r-5] ) + W[r-8] + CH( W[r-5], W[r-6], W[r-7] ) + SymCryptSha256K[r]; \
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

        pbData += 64;
        cbData -= 64;
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

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
    SYMCRYPT_FORCE_WRITE32( &A, 0 );
    SYMCRYPT_FORCE_WRITE32( &B, 0 );
    SYMCRYPT_FORCE_WRITE32( &D, 0 );
    SYMCRYPT_FORCE_WRITE32( &T, 0 );
}

#undef CROUND
#undef IROUND
#undef FROUND

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

//
// Don't omit frame pointer for XMM code; it isn't register-starved as much
//
#if SYMCRYPT_CPU_X86 && SYMCRYPT_MS_VC && !defined(__clang__)
#pragma optimize( "y", off )
#endif

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("ssse3,sha"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("ssse3,sha")
#endif

//
// Code that uses the XMM registers.
// This code is currently unused. It was written in case it would provide better performance, but
// it did not. We are retaining it in case it might be useful in a future CPU generation.
//
#if 0

#define MAJXMM( x, y, z ) _mm_or_si128( _mm_and_si128( _mm_or_si128( z, y ), x ), _mm_and_si128( z, y ))
#define CHXMM( x, y, z )  _mm_xor_si128( _mm_and_si128( _mm_xor_si128( z, y ), x ), z )

#define CSIGMA0XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,30)  , _mm_srli_epi32(x,  2) ),\
        _mm_slli_epi32(x,19) ), _mm_srli_epi32(x, 13) ),\
        _mm_slli_epi32(x,10) ), _mm_srli_epi32(x, 22) )
#define CSIGMA1XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,26)  , _mm_srli_epi32(x,  6) ),\
        _mm_slli_epi32(x,21) ), _mm_srli_epi32(x, 11) ),\
        _mm_slli_epi32(x,7) ), _mm_srli_epi32(x, 25) )
#define LSIGMA0XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,25)  , _mm_srli_epi32(x,  7) ),\
        _mm_slli_epi32(x,14) ), _mm_srli_epi32(x, 18) ),\
        _mm_srli_epi32(x, 3) )
#define LSIGMA1XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,15)  , _mm_srli_epi32(x, 17) ),\
        _mm_slli_epi32(x,13) ), _mm_srli_epi32(x, 19) ),\
        _mm_srli_epi32(x,10) )

VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_xmm1(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE *    pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    //
    // Implementation that has one value in each XMM register.
    // This is significantly slower than the _ul1 implementation
    // but can be extended to compute 4 hash blocks in parallel.
    //
    SYMCRYPT_ALIGN __m128i buf[4 + 8 + 64];    // chaining state concatenated with the expanded input block
    __m128i * W = &buf[4 + 8];
    __m128i * ha = &buf[4]; // initial state words, in order h, g, ..., b, a
    __m128i A, B, C, D, T;
    int r;

    //
    // For 1-input only; set the input buffer to zero so that we have known values in every byte
    //
    //SymCryptWipeKnownSize( buf, sizeof( buf ) );

    //
    // Copy the chaining state into the start of the buffer, order = h,g,f,e,d,c,b,a
    //
    ha[7] = _mm_insert_epi32(ha[7], pChain->H[0], 0);
    ha[6] = _mm_insert_epi32(ha[6], pChain->H[1], 0);
    ha[5] = _mm_insert_epi32(ha[5], pChain->H[2], 0);
    ha[4] = _mm_insert_epi32(ha[4], pChain->H[3], 0);
    ha[3] = _mm_insert_epi32(ha[3], pChain->H[4], 0);
    ha[2] = _mm_insert_epi32(ha[2], pChain->H[5], 0);
    ha[1] = _mm_insert_epi32(ha[1], pChain->H[6], 0);
    ha[0] = _mm_insert_epi32(ha[0], pChain->H[7], 0);

    buf[0] = ha[4];
    buf[1] = ha[5];
    buf[2] = ha[6];
    buf[3] = ha[7];

    while( cbData >= 64 )
    {

        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r++ )
        {
            W[r] = _mm_insert_epi32(W[r], SYMCRYPT_LOAD_MSBFIRST32( &pbData[ 4*r ] ), 0);
        }

        //
        // Expand the message
        //
        A = W[15];
        B = W[14];
        D = W[0];
        for( r=16; r<64; r+= 2 )
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
                        b = _mm_add_epi32( _mm_add_epi32( _mm_add_epi32( d, LSIGMA1XMM( b ) ), W[r-7] ), LSIGMA0XMM( c ) ); \
                        W[r] = b; \

            EXPAND( A, B, C, D, r );
            EXPAND( B, A, D, C, (r+1));

            #undef EXPAND
        }

        A = ha[7];
        B = ha[6];
        C = ha[5];
        D = ha[4];

        for( r=0; r<64; r += 4 )
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
                t = W[r]; \
                t = _mm_add_epi32( t, CSIGMA1XMM( W[r-5] ) ); \
                t = _mm_add_epi32( t, W[r-8] ); \
                t = _mm_add_epi32( t, CHXMM( W[r-5], W[r-6], W[r-7] ) ); \
                t = _mm_add_epi32( t, _mm_cvtsi32_si128( SymCryptSha256K[r] ) ); \
                W[r-4] = _mm_add_epi32( t, d ); \
                d = _mm_add_epi32( t, CSIGMA0XMM( a ) ); \
                d = _mm_add_epi32( d, MAJXMM( c, b, a ) );

            DO_ROUND( A, B, C, D, T, r );
            DO_ROUND( D, A, B, C, T, (r+1) );
            DO_ROUND( C, D, A, B, T, (r+2) );
            DO_ROUND( B, C, D, A, T, (r+3) );
            #undef DO_ROUND
        }

        buf[3] = ha[7] = _mm_add_epi32( buf[3], A );
        buf[2] = ha[6] = _mm_add_epi32( buf[2], B );
        buf[1] = ha[5] = _mm_add_epi32( buf[1], C );
        buf[0] = ha[4] = _mm_add_epi32( buf[0], D );
        ha[3] = _mm_add_epi32( ha[3], W[r-5] );
        ha[2] = _mm_add_epi32( ha[2], W[r-6] );
        ha[1] = _mm_add_epi32( ha[1], W[r-7] );
        ha[0] = _mm_add_epi32( ha[0], W[r-8] );

        pbData += 64;
        cbData -= 64;
    }

    //
    // Copy the chaining state back into the hash structure
    //
    pChain->H[0] = _mm_extract_epi32(ha[7], 0);
    pChain->H[1] = _mm_extract_epi32(ha[6], 0);
    pChain->H[2] = _mm_extract_epi32(ha[5], 0);
    pChain->H[3] = _mm_extract_epi32(ha[4], 0);
    pChain->H[4] = _mm_extract_epi32(ha[3], 0);
    pChain->H[5] = _mm_extract_epi32(ha[2], 0);
    pChain->H[6] = _mm_extract_epi32(ha[1], 0);
    pChain->H[7] = _mm_extract_epi32(ha[0], 0);

    *pcbRemaining = cbData;

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
    SymCryptWipeKnownSize( &A, sizeof( A ) );
    SymCryptWipeKnownSize( &B, sizeof( B ) );
    SymCryptWipeKnownSize( &C, sizeof( C ) );
    SymCryptWipeKnownSize( &D, sizeof( D ) );
    SymCryptWipeKnownSize( &T, sizeof( T ) );
}


//
// XMM implementation 2
// We use the XMM registers to compute part of the message schedule.
// The load, BSWAP, and part of the message schedule recursion are done in XMM registers.
// The rest of the work is done using integers.
//
// Core2: 0.1 c/B slower than the _ul1
// Atom: 1.0 c/B slower than _ul1   (42.34 vs 41.39 c/B)
//
VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_xmm2(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE *    pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    SYMCRYPT_ALIGN union { UINT32 ul[16]; __m128i xmm[4]; } W;
    SYMCRYPT_ALIGN UINT32 ah[8];
    int round;
    UINT32 Wt;
    const __m128i BYTE_REVERSE_32 = _mm_set_epi8( 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 );

    ah[7] = pChain->H[0];
    ah[6] = pChain->H[1];
    ah[5] = pChain->H[2];
    ah[4] = pChain->H[3];
    ah[3] = pChain->H[4];
    ah[2] = pChain->H[5];
    ah[1] = pChain->H[6];
    ah[0] = pChain->H[7];

#define CROUND( r16, r ) {;\
    ah[ r16   &7] += CSIGMA1(ah[(r16+3)&7]) + CH(ah[(r16+3)&7], ah[(r16+2)&7], ah[(r16+1)&7]) + SymCryptSha256K[r] + Wt;\
    ah[(r16+4)&7] += ah[r16 &7];\
    ah[ r16   &7] += CSIGMA0(ah[(r16+7)&7]) + MAJ(ah[(r16+7)&7], ah[(r16+6)&7], ah[(r16+5)&7]);\
}


//
// Initial round that reads the message.
// r is the round number 0..15
//
//    Wt = LOAD_MSBFIRST32( &pbData[ 4*r ] );\
//    W.ul[r] = Wt; \

#define IROUND( r ) {\
    Wt = W.ul[r];\
    CROUND(r,r);\
    }

//
// Subsequent rounds.
// r16 is the round number mod 16. rb is the round number minus r16.
//
#define FROUND(r16, rb) { \
    Wt = W.ul[r16];\
    CROUND( r16, r16+rb ); \
}


    while( cbData >= 64 )
    {
        //
        // The code is faster if we directly access the W.ul array, rather than the W.xmm alias.
        // I think the compiler gets more confused if you use the W.xmm values.
        // We retain them in the union to ensure alignment
        //
        _mm_store_si128( (__m128i *)&W.ul[ 0], _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *)&pbData[   0 ] ), BYTE_REVERSE_32 ));
        _mm_store_si128( (__m128i *)&W.ul[ 4], _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *)&pbData[  16 ] ), BYTE_REVERSE_32 ));
        _mm_store_si128( (__m128i *)&W.ul[ 8], _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *)&pbData[  32 ] ), BYTE_REVERSE_32 ));
        _mm_store_si128( (__m128i *)&W.ul[12], _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *)&pbData[  48 ] ), BYTE_REVERSE_32 ));

        //
        // initial rounds 1 to 16
        //

        IROUND(  0 );
        IROUND(  1 );
        IROUND(  2 );
        IROUND(  3 );
        IROUND(  4 );
        IROUND(  5 );
        IROUND(  6 );
        IROUND(  7 );
        IROUND(  8 );
        IROUND(  9 );
        IROUND( 10 );
        IROUND( 11 );
        IROUND( 12 );
        IROUND( 13 );
        IROUND( 14 );
        IROUND( 15 );


        //
        // rounds 16 to 64.
        //
        for( round=16; round<64; round += 16 )
        {
            __m128i Tmp;

            Tmp = _mm_add_epi32( _mm_add_epi32(
                    LSIGMA0XMM(_mm_loadu_si128( (__m128i *)&W.ul[1] )),
                    _mm_load_si128( (__m128i *)&W.ul[0] ) ),
                    _mm_loadu_si128( (__m128i *)&W.ul[9] ) );

            //
            // The final part of the message schedule can be done in XMM registers, but it isn't worth it.
            // The rotates in XMM take two shifts and an OR/XOR, vs one instruction in integer registers.
            // As the sigma1( W_{t-2} ) recursion component can only be computed 2 at a time
            // (because the result of the first two are the inputs to the second two)
            // you lose more than you gain by using XMM registers.
            //
            //Tmp = _mm_add_epi32( Tmp, LSIGMA1XMM( _mm_srli_si128( _mm_load_si128( (__m128i *)&W.ul[12] ), 8 ) ) );
            //Tmp = _mm_add_epi32( Tmp, LSIGMA1XMM( _mm_slli_si128( Tmp, 8 ) ) );
            //_mm_store_si128( (__m128i *)&W.ul[0], Tmp );
            //

            _mm_store_si128( (__m128i *)&W.ul[0], Tmp );
            W.ul[0] += LSIGMA1( W.ul[14] );
            W.ul[1] += LSIGMA1( W.ul[15] );
            W.ul[2] += LSIGMA1( W.ul[0] );
            W.ul[3] += LSIGMA1( W.ul[1] );

            FROUND(  0, round );
            FROUND(  1, round );
            FROUND(  2, round );
            FROUND(  3, round );

            Tmp = _mm_add_epi32( _mm_add_epi32(
                    LSIGMA0XMM(_mm_loadu_si128( (__m128i *)&W.ul[5] )),
                    _mm_load_si128( (__m128i *)&W.ul[4] ) ),
                    _mm_alignr_epi8( _mm_load_si128( (__m128i *)&W.ul[0] ), _mm_load_si128( (__m128i *)&W.ul[12] ), 4) );

            _mm_store_si128( (__m128i *)&W.ul[4], Tmp );

            W.ul[4] += LSIGMA1( W.ul[2] );
            W.ul[5] += LSIGMA1( W.ul[3] );
            W.ul[6] += LSIGMA1( W.ul[4] );
            W.ul[7] += LSIGMA1( W.ul[5] );

            FROUND(  4, round );
            FROUND(  5, round );
            FROUND(  6, round );
            FROUND(  7, round );

            Tmp = _mm_add_epi32( _mm_add_epi32(
                    LSIGMA0XMM(_mm_loadu_si128( (__m128i *)&W.ul[9] )),
                    _mm_load_si128( (__m128i *)&W.ul[8] ) ),
                    _mm_loadu_si128( (__m128i *)&W.ul[1] ) );

            _mm_store_si128( (__m128i *)&W.ul[8], Tmp );
            W.ul[ 8] += LSIGMA1( W.ul[6] );
            W.ul[ 9] += LSIGMA1( W.ul[7] );
            W.ul[10] += LSIGMA1( W.ul[8] );
            W.ul[11] += LSIGMA1( W.ul[9] );

            FROUND(  8, round );
            FROUND(  9, round );
            FROUND( 10, round );
            FROUND( 11, round );


            Tmp = _mm_add_epi32( _mm_add_epi32(
                    LSIGMA0XMM( _mm_alignr_epi8( _mm_load_si128( (__m128i *)&W.ul[0] ), _mm_load_si128( (__m128i *)&W.ul[12] ), 4) ),
                    _mm_load_si128( (__m128i *)&W.ul[12] ) ),
                    _mm_loadu_si128( (__m128i *)&W.ul[5] ) );

            _mm_store_si128( (__m128i *)&W.ul[12], Tmp );
            W.ul[12] += LSIGMA1( W.ul[10] );
            W.ul[13] += LSIGMA1( W.ul[11] );
            W.ul[14] += LSIGMA1( W.ul[12] );
            W.ul[15] += LSIGMA1( W.ul[13] );

            FROUND( 12, round );
            FROUND( 13, round );
            FROUND( 14, round );
            FROUND( 15, round );
        }

        pChain->H[0] = ah[7] = ah[7] + pChain->H[0];
        pChain->H[1] = ah[6] = ah[6] + pChain->H[1];
        pChain->H[2] = ah[5] = ah[5] + pChain->H[2];
        pChain->H[3] = ah[4] = ah[4] + pChain->H[3];
        pChain->H[4] = ah[3] = ah[3] + pChain->H[4];
        pChain->H[5] = ah[2] = ah[2] + pChain->H[5];
        pChain->H[6] = ah[1] = ah[1] + pChain->H[6];
        pChain->H[7] = ah[0] = ah[0] + pChain->H[7];

        pbData += 64;
        cbData -= 64;

    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( ah, sizeof( ah ) );
    SymCryptWipeKnownSize( &W, sizeof( W ) );
    SYMCRYPT_FORCE_WRITE32( &Wt, 0 );

#undef IROUND
#undef FROUND
#undef CROUND
}

#endif

//
// SHA-NI Implementation
//

#if SYMCRYPT_MS_VC && !defined(__clang__)
// Intrinsic definitions included here
// until the header is updated.
// *******************************
// *******************************
// *******************************
extern __m128i _mm_sha256rnds2_epu32(__m128i, __m128i, __m128i);
extern __m128i _mm_sha256msg1_epu32(__m128i, __m128i);
extern __m128i _mm_sha256msg2_epu32(__m128i, __m128i);
// *******************************
// *******************************
// *******************************
#endif

// For the SHA-NI implementation we will utilize 128-bit XMM registers. Each
// XMM state will be denoted as (R_3, R_2, R_1, R_0), where each R_i
// is a 32-bit word and R_i refers to bits [32*i : (32*i + 31)] of the
// 128-bit XMM state.
//
// The following macro updates the state variables A,B,C,...,H of the SHA algorithms
// for 4 rounds using:
//  - The current round number t with 0<=t<= 63 and t a multiple of 4.
//  - A current message XMM state _MSG which consists of 4 32-bit words
//      ( W_(t+3), W_(t+2), W_(t+1), W_(t+0) ).
//  - Two XMM states _ABEF and _CDGH which contain the variables
//      ( A, B, E, F ) and ( C, D, G, H ) respectively.

#define SHANI_UPDATE_STATE( _round, _MSG, _ABEF, _CDGH ) \
    _MSG = _mm_add_epi32( _MSG, *(__m128i *)&SymCryptSha256K[_round] );     /* Add the K_t constants to the W_t's */    \
    _CDGH = _mm_sha256rnds2_epu32( _CDGH, _ABEF, _MSG );                    /* 2 rounds using SHA-NI */                 \
    _MSG = _mm_shuffle_epi32( _MSG, 0x0e );                                 /* Move words 2 & 3 to positions 0 & 1 */   \
    _ABEF = _mm_sha256rnds2_epu32( _ABEF, _CDGH, _MSG );                    /* 2 rounds using SHA-NI */

// For the SHA message schedule (i.e. to create words W_16 to W_63) we use 4 XMM states / accumulators.
// Each accumulator holds 4 words.
//
// The final result for each word will be of the form W_t = X_t + Y_t, where
//          X_t = W_(t-16) + \sigma_0(W_(t-15)) and
//          Y_t = W_(t- 7) + \sigma_1(W_(t- 2))
//
//          The X_t's are calculated by the _mm_sha256msg1_epu32 intrinsic.
//          The \sigma_1(W_(t-2)) part of the Y_t's by the _mm_sha256msg2_epu32 intrinsic.
//
// Remarks:
//      - Calculation of the first four X_t's (i.e. 16<=t<=19) can start from round 4 (since 19-15 = 4).
//      - Calculation of the first four Y_t's can start from round 12 (since 19-7=12 and W_(19-7) is calculated
//        in the intrinsic call).
//      - Due to the W_(t-7) term, producing the Y_t's need special shifting via the _mm_alignr_epi8 intrinsic and
//        adding the correct accumulator into another variable MTEMP.
//
// For rounds 16 - 51 we execute the following macro in a loop. For all the other rounds we
// use specific code.
//
// The loop invariant to be satisfied at the beginning of iteration i (corresponding to rounds
// (16+4*i) to (19+4*i) ) is the following:
//      _MSG_0 = ( W_(19 + 4*i), W_(18 + 4*i), W_(17 + 4*i), W_(16 + 4*i) )
//      _MSG_1 = ( X_(23 + 4*i), X_(22 + 4*i), X_(21 + 4*i), X_(20 + 4*i) )
//      _MSG_2 = ( X_(27 + 4*i), X_(26 + 4*i), X_(25 + 4*i), X_(24 + 4*i) )
//      _MSG_3 = ( W_(15 + 4*i), W_(14 + 4*i), W_(13 + 4*i), W_(12 + 4*i) )
//
#define SHANI_MESSAGE_SCHEDULE( _MSG_0, _MSG_1, _MSG_2, _MSG_3, _MTEMP ) \
    _MTEMP = _mm_alignr_epi8( _MSG_0, _MSG_3, 4);       /* _MTEMP := ( W_(16 + 4*i), W_(15 + 4*i), W_(14 + 4*i), W_(13 + 4*i) ) */          \
    _MSG_1 = _mm_add_epi32( _MSG_1, _MTEMP);            /* _MSG_1 := _MSG_1 + ( W_(16 + 4*i), W_(15 + 4*i), W_(14 + 4*i), W_(13 + 4*i) ) */ \
    _MSG_1 = _mm_sha256msg2_epu32( _MSG_1, _MSG_0 );    /* _MSG_1 := ( W_(23 + 4*i), W_(22 + 4*i), W_(21 + 4*i), W_(20 + 4*i) ) */          \
    _MSG_3 = _mm_sha256msg1_epu32( _MSG_3, _MSG_0 );    /* _MSG_3 := ( X_(31+4*i), X_(30+4*i), X_(29+4*i), X_(28+4*i) ) */
//
// After each iteration the subsequent call rotates the accumulators so that the loop
// invariant is preserved (please verify!):
//          -- MSG_0 <---- MSG_1 <--- MSG_2 <--- MSG_3 <--
//          |                                            |
//          ----------------------------------------------

VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_shani(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE *    pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    const __m128i BYTE_REVERSE_32 = _mm_set_epi8( 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 );

    // Our chain state is in order A, B, ..., H.
    // First load our chaining state
    __m128i DCBA = _mm_loadu_si128( (__m128i *)&(pChain->H[0]) );   // (D, C, B, A)
    __m128i HGFE = _mm_loadu_si128( (__m128i *)&(pChain->H[4]) );   // (H, G, F, E)
    __m128i FEBA = _mm_unpacklo_epi64( DCBA, HGFE );                // (F, E, B, A)
    __m128i HGDC = _mm_unpackhi_epi64( DCBA, HGFE );                // (H, G, D, C)
    __m128i ABEF = _mm_shuffle_epi32( FEBA, 0x1b );                 // (A, B, E, F)
    __m128i CDGH = _mm_shuffle_epi32( HGDC, 0x1b );                 // (C, D, G, H)

    while( cbData >= 64 )
    {
        // Save the current state for the feed-forward later
        __m128i ABEF_start = ABEF;
        __m128i CDGH_start = CDGH;

        // Current message and temporary state
        __m128i MSG;

        // Accumulators
        __m128i MSG_0;
        __m128i MSG_1;
        __m128i MSG_2;
        __m128i MSG_3;

        // Rounds 0-3
        MSG = _mm_loadu_si128( (__m128i *)pbData );         // Reversed word - ( M_3, M_2, M_1, M_0 )
        pbData += 16;
        MSG = _mm_shuffle_epi8( MSG, BYTE_REVERSE_32 );     // Reverse each word
        MSG_0 = MSG;                                        // MSG_0 := ( W_3 = M3, W_2 = M_2, W_1 = M_1, W_0 = M_0 )

        SHANI_UPDATE_STATE( 0, MSG, ABEF, CDGH );

        // Rounds 4-7
        MSG = _mm_loadu_si128( (__m128i *)pbData );         // Reversed word - ( M_7, M_6, M_5, M_4 )
        pbData += 16;
        MSG = _mm_shuffle_epi8( MSG, BYTE_REVERSE_32 );     // Reverse each word
        MSG_1 = MSG;                                        // MSG_1 := ( W_7 = M_7, W_6 = M_6, W_5 = M_5, W_4 = M_4 )

        SHANI_UPDATE_STATE( 4, MSG, ABEF, CDGH );

        MSG_0 = _mm_sha256msg1_epu32( MSG_0, MSG_1 );       // MSG_0 := ( X_19, X_18, X_17, X_16 ) =
                                                            // ( W_3 + \sigma_0(W_4), ..., W_0 + \sigma_0(W_1) )

        // Rounds 8-11
        MSG = _mm_loadu_si128( (__m128i *)pbData );         // Reversed word - ( M_11, M_10, M_9, M_8 )
        pbData += 16;
        MSG = _mm_shuffle_epi8( MSG, BYTE_REVERSE_32 );     // Reverse each word
        MSG_2 = MSG;                                        // MSG_2 := ( W_11 = M_11, W_10 = M_10, W_9 = M_9, W_8 = M_8 )

        SHANI_UPDATE_STATE( 8, MSG, ABEF, CDGH );

        MSG_1 = _mm_sha256msg1_epu32( MSG_1, MSG_2 );       // MSG_1 := ( X_23, X_22, X_21, X_20 )

        // Rounds 12-15
        MSG = _mm_loadu_si128( (__m128i *)pbData );         // Reversed word - ( M_15, M_14, M_13, M_12 )
        pbData += 16;
        MSG = _mm_shuffle_epi8( MSG, BYTE_REVERSE_32 );     // Reverse each word
        MSG_3 = MSG;                                        // MSG_3 := ( W_15 = M_15, W_14 = M_14, W_13 = M_13, W_12 = M_12 )

        SHANI_UPDATE_STATE( 12, MSG, ABEF, CDGH );

        MSG = _mm_alignr_epi8( MSG_3, MSG_2, 4);            // MSG := ( W_12, W_11, W_10, W_9 )
        MSG_0 = _mm_add_epi32( MSG_0, MSG);                 // MSG_0 := MSG_0 + ( W_12, W_11, W_10, W_9 )
        MSG_0 = _mm_sha256msg2_epu32( MSG_0, MSG_3 );       // MSG_0 := ( W_19, W_18, W_17, W_16 ) =
                                                            // ( X_19 + W_12 + \sigma_1(W_17)], ..., X_16 + W_9 + \sigma_1(W_14)] )

        MSG_2 = _mm_sha256msg1_epu32( MSG_2, MSG_3 );       // MSG_2 := ( X_27, X_26, X_25, X_24 )


        // Rounds 16 - 19
        MSG = MSG_0;
        SHANI_UPDATE_STATE( 16, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_0, MSG_1, MSG_2, MSG_3, MSG );

        // Rounds 20 - 23
        MSG = MSG_1;
        SHANI_UPDATE_STATE( 20, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_1, MSG_2, MSG_3, MSG_0, MSG );

        // Rounds 24 - 27
        MSG = MSG_2;
        SHANI_UPDATE_STATE( 24, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_2, MSG_3, MSG_0, MSG_1, MSG );

        // Rounds 28 - 31
        MSG = MSG_3;
        SHANI_UPDATE_STATE( 28, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_3, MSG_0, MSG_1, MSG_2, MSG );

        // Rounds 32 - 35
        MSG = MSG_0;
        SHANI_UPDATE_STATE( 32, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_0, MSG_1, MSG_2, MSG_3, MSG );

        // Rounds 36 - 39
        MSG = MSG_1;
        SHANI_UPDATE_STATE( 36, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_1, MSG_2, MSG_3, MSG_0, MSG );

        // Rounds 40 - 43
        MSG = MSG_2;
        SHANI_UPDATE_STATE( 40, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_2, MSG_3, MSG_0, MSG_1, MSG );

        // Rounds 44 - 47
        MSG = MSG_3;
        SHANI_UPDATE_STATE( 44, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_3, MSG_0, MSG_1, MSG_2, MSG );

        // Rounds 48 - 51
        MSG = MSG_0;
        SHANI_UPDATE_STATE( 48, MSG, ABEF, CDGH );
        SHANI_MESSAGE_SCHEDULE( MSG_0, MSG_1, MSG_2, MSG_3, MSG );

        // Rounds 52 - 55
        MSG = MSG_1;                                                    // ( W_55, W_54, W_53, W_52 )
        SHANI_UPDATE_STATE( 52, MSG, ABEF, CDGH );

        MSG = _mm_alignr_epi8( MSG_1, MSG_0, 4);                        // MSG := ( W_52, W_51, W_50, W_49 )
        MSG_2 = _mm_add_epi32( MSG_2, MSG);                             // MSG_2 := MSG_2 + ( W_52, W_51, W_50, W_49 )
        MSG_2 = _mm_sha256msg2_epu32( MSG_2, MSG_1 );                   // Calculate ( W_59, W_58, W_57, W_56 )

        // Rounds 56 - 59
        MSG = MSG_2;                                                    // ( W_59, W_58, W_57, W_56 )
        SHANI_UPDATE_STATE( 56, MSG, ABEF, CDGH );

        MSG = _mm_alignr_epi8( MSG_2, MSG_1, 4);                        // MSG := ( W_56, W_55, W_54, W_53 )
        MSG_3 = _mm_add_epi32( MSG_3, MSG);                             // MSG_3 := MSG_3 + ( W_56, W_55, W_54, W_53 )
        MSG_3 = _mm_sha256msg2_epu32( MSG_3, MSG_2 );                   // Calculate ( W_63, W_62, W_61, W_60 )

        // Rounds 60 - 63
        SHANI_UPDATE_STATE( 60, MSG_3, ABEF, CDGH );

        // Add the feed-forward
        ABEF = _mm_add_epi32( ABEF, ABEF_start );
        CDGH = _mm_add_epi32( CDGH, CDGH_start );

        cbData -= 64;
    }

    // Unpack the state registers and store them in the state
    FEBA = _mm_shuffle_epi32( ABEF, 0x1b );
    HGDC = _mm_shuffle_epi32( CDGH, 0x1b );
    DCBA = _mm_unpacklo_epi64( FEBA, HGDC );                        // (D, C, B, A)
    HGFE = _mm_unpackhi_epi64( FEBA, HGDC );                        // (H, G, F, E)
    _mm_storeu_si128 ( (__m128i *)&(pChain->H[0]), DCBA);            // (D, C, B, A)
    _mm_storeu_si128 ( (__m128i *)&(pChain->H[4]), HGFE);            // (H, G, F, E)

    *pcbRemaining = cbData;
}

#undef SHANI_UPDATE_STATE
#undef SHANI_MESSAGE_SCHEDULE

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif  // SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#if SYMCRYPT_CPU_ARM64
/*
ARM64 has special SHA-256 instructions

SHA256H and SHA256H2 implement 4 rounds of SHA-256. The inputs are two registers containing the 256-bit state,
and one register containing 128 bits of expanded message plus the round constants.
These instructions perform the same computation, but SHA256H returns the first half of the 256-bit result,
and SHA256H2 returns the second half of the 256-bit result.

SHA256H( ABCDE, FGHIJ, W )
Where the least significant word of the ABCDE vector is A. The W vector contains W_i + K_i for the four rounds being computed.

SHA256SU0 is the message schedule update function.
It takes 2 inputs and produces 1 output.
We describe the vectors for i=0,1,2,3
Inputs:  [W_{t-16+i}], [W_{t-12+i}]
Output: [Sigma0(W_{t-15+i}) + W_{t-16+i}]

SHA256SU1 is the second message schedule update function
Takes 3 inputs and produces 1 output
Input 1: Output of SHA256SU0: [Sigma0(W_{t-15+i}) + W_{t-16+i}]
Input 2:
Input 3: [W_{t-4+i}]

*/

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("sha2"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("sha2")
#endif

#define vldq(_p)     (*(__n128 *)(_p))
#define vstq(_p, _v) (*(__n128 *)(_p) = (_v) )

VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_instr(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE *    pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining )
{
    //
    // Armv8 has 32 Neon registers. We can use a lot of variables.
    // 16 for the constants, 4 for the message, 2 for the current state, 2 for the starting state,
    // total = 24 which leaves enough for some temp values
    //
    __n128      ABCD, ABCDstart;
    __n128      EFGH, EFGHstart;
    __n128      W0, W1, W2, W3;
    __n128      K0, K1, K2, K3, K4, K5, K6, K7, K8, K9, K10, K11, K12, K13, K14, K15;

    __n128      Wr;
    __n128      t;

    ABCD = ABCDstart = vldq( &pChain->H[0] );
    EFGH = EFGHstart = vldq( &pChain->H[4] );

    K0  = vldq( &SymCryptSha256K[ 4 *  0 ] );
    K1  = vldq( &SymCryptSha256K[ 4 *  1 ] );
    K2  = vldq( &SymCryptSha256K[ 4 *  2 ] );
    K3  = vldq( &SymCryptSha256K[ 4 *  3 ] );
    K4  = vldq( &SymCryptSha256K[ 4 *  4 ] );
    K5  = vldq( &SymCryptSha256K[ 4 *  5 ] );
    K6  = vldq( &SymCryptSha256K[ 4 *  6 ] );
    K7  = vldq( &SymCryptSha256K[ 4 *  7 ] );
    K8  = vldq( &SymCryptSha256K[ 4 *  8 ] );
    K9  = vldq( &SymCryptSha256K[ 4 *  9 ] );
    K10 = vldq( &SymCryptSha256K[ 4 * 10 ] );
    K11 = vldq( &SymCryptSha256K[ 4 * 11 ] );
    K12 = vldq( &SymCryptSha256K[ 4 * 12 ] );
    K13 = vldq( &SymCryptSha256K[ 4 * 13 ] );
    K14 = vldq( &SymCryptSha256K[ 4 * 14 ] );
    K15 = vldq( &SymCryptSha256K[ 4 * 15 ] );

    while( cbData >= 64 )
    {
        W0 = vrev32q_u8( vldq( &pbData[ 0] ) );
        W1 = vrev32q_u8( vldq( &pbData[16] ) );
        W2 = vrev32q_u8( vldq( &pbData[32] ) );
        W3 = vrev32q_u8( vldq( &pbData[48] ) );

        //
        // The sha256h/sha256h2 instructions overwrite one of the two state input registers.
        // This implies we have to have a copy made of one of the input states.
        //
#define ROUNDOP {\
    t = ABCD;\
    ABCD = vsha256hq_u32 ( ABCD, EFGH, Wr );\
    EFGH = vsha256h2q_u32( EFGH, t, Wr );\
    }

        Wr = vaddq_u32( W0, K0 );
        ROUNDOP;
        Wr = vaddq_u32( W1, K1 );
        ROUNDOP;
        Wr = vaddq_u32( W2, K2 );
        ROUNDOP;
        Wr = vaddq_u32( W3, K3 );
        ROUNDOP;

        t = vsha256su0q_u32( W0, W1 );
        W0 = vsha256su1q_u32( t, W2, W3 );
        Wr = vaddq_u32( W0, K4 );
        ROUNDOP;

        t = vsha256su0q_u32( W1, W2 );
        W1 = vsha256su1q_u32( t, W3, W0 );
        Wr = vaddq_u32( W1, K5 );
        ROUNDOP;

        t = vsha256su0q_u32( W2, W3 );
        W2 = vsha256su1q_u32( t, W0, W1 );
        Wr = vaddq_u32( W2, K6 );
        ROUNDOP;

        t = vsha256su0q_u32( W3, W0 );
        W3 = vsha256su1q_u32( t, W1, W2 );
        Wr = vaddq_u32( W3, K7 );
        ROUNDOP;


        t = vsha256su0q_u32( W0, W1 );
        W0 = vsha256su1q_u32( t, W2, W3 );
        Wr = vaddq_u32( W0, K8 );
        ROUNDOP;

        t = vsha256su0q_u32( W1, W2 );
        W1 = vsha256su1q_u32( t, W3, W0 );
        Wr = vaddq_u32( W1, K9 );
        ROUNDOP;

        t = vsha256su0q_u32( W2, W3 );
        W2 = vsha256su1q_u32( t, W0, W1 );
        Wr = vaddq_u32( W2, K10 );
        ROUNDOP;

        t = vsha256su0q_u32( W3, W0 );
        W3 = vsha256su1q_u32( t, W1, W2 );
        Wr = vaddq_u32( W3, K11 );
        ROUNDOP;


        t = vsha256su0q_u32( W0, W1 );
        W0 = vsha256su1q_u32( t, W2, W3 );
        Wr = vaddq_u32( W0, K12 );
        ROUNDOP;

        t = vsha256su0q_u32( W1, W2 );
        W1 = vsha256su1q_u32( t, W3, W0 );
        Wr = vaddq_u32( W1, K13 );
        ROUNDOP;

        t = vsha256su0q_u32( W2, W3 );
        W2 = vsha256su1q_u32( t, W0, W1 );
        Wr = vaddq_u32( W2, K14 );
        ROUNDOP;

        t = vsha256su0q_u32( W3, W0 );
        W3 = vsha256su1q_u32( t, W1, W2 );
        Wr = vaddq_u32( W3, K15 );
        ROUNDOP;

        ABCDstart = ABCD = vaddq_u32( ABCDstart, ABCD );
        EFGHstart = EFGH = vaddq_u32( EFGHstart, EFGH );

        pbData += 64;
        cbData -= 64;
#undef ROUNDOP

    }

    *pcbRemaining = cbData;
    vstq( &pChain->H[0], ABCD );
    vstq( &pChain->H[4], EFGH );

    //
    // All our local variables should be in registers, so no way to wipe them.
    //
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif



//
// Easy switch between different implementations
//
//FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE* pChain,
    _In_reads_(cbData)      PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T*                         pcbRemaining)
{
#if SYMCRYPT_CPU_AMD64

    SYMCRYPT_EXTENDED_SAVE_DATA SaveData;

    if (SYMCRYPT_CPU_FEATURES_PRESENT(SYMCRYPT_CPU_FEATURES_FOR_SHANI_CODE) &&
        SymCryptSaveXmm(&SaveData) == SYMCRYPT_NO_ERROR)
    {
        SymCryptSha256AppendBlocks_shani(pChain, pbData, cbData, pcbRemaining);

        SymCryptRestoreXmm(&SaveData);
    }
    // Temporarily disabling use of Ymm in SHA2
    // else if (SYMCRYPT_CPU_FEATURES_PRESENT(SYMCRYPT_CPU_FEATURE_AVX2 | SYMCRYPT_CPU_FEATURE_BMI2) &&
    //         SymCryptSaveYmm(&SaveData) == SYMCRYPT_NO_ERROR)
    // {
    //     //SymCryptSha256AppendBlocks_ul1(pChain, pbData, cbData, pcbRemaining);
    //     //SymCryptSha256AppendBlocks_ymm_8blocks(pChain, pbData, cbData, pcbRemaining);
    //     SymCryptSha256AppendBlocks_ymm_avx2_asm(pChain, pbData, cbData, pcbRemaining);

    //     SymCryptRestoreYmm(&SaveData);
    // }
    else if (SYMCRYPT_CPU_FEATURES_PRESENT(SYMCRYPT_CPU_FEATURE_SSSE3 | SYMCRYPT_CPU_FEATURE_BMI2) &&
            SymCryptSaveXmm(&SaveData) == SYMCRYPT_NO_ERROR)
    {
        //SymCryptSha256AppendBlocks_xmm_4blocks(pChain, pbData, cbData, pcbRemaining);
        SymCryptSha256AppendBlocks_xmm_ssse3_asm(pChain, pbData, cbData, pcbRemaining);

        SymCryptRestoreXmm(&SaveData);
    }
    else
    {
       SymCryptSha256AppendBlocks_ul1( pChain, pbData, cbData, pcbRemaining );
       //SymCryptSha256AppendBlocks_ul2(pChain, pbData, cbData, pcbRemaining);
    }
#elif SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURES_FOR_SHANI_CODE | SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL ) &&
        SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptSha256AppendBlocks_shani( pChain, pbData, cbData, pcbRemaining );
        SymCryptRestoreXmm( &SaveData );
    }
    else if (SYMCRYPT_CPU_FEATURES_PRESENT(SYMCRYPT_CPU_FEATURE_SSSE3 | SYMCRYPT_CPU_FEATURE_BMI2)
            && SymCryptSaveXmm(&SaveData) == SYMCRYPT_NO_ERROR)
    {
        SymCryptSha256AppendBlocks_xmm_4blocks(pChain, pbData, cbData, pcbRemaining);
        SymCryptRestoreXmm(&SaveData);
    }
    else {
        SymCryptSha256AppendBlocks_ul1( pChain, pbData, cbData, pcbRemaining );
    }
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON_SHA256 ) )
    {
        SymCryptSha256AppendBlocks_instr( pChain, pbData, cbData, pcbRemaining );
    } else {
        SymCryptSha256AppendBlocks_ul1( pChain, pbData, cbData, pcbRemaining );
    }
#else
    SymCryptSha256AppendBlocks_ul1( pChain, pbData, cbData, pcbRemaining );
#endif

    //SymCryptSha256AppendBlocks_ul2( pChain, pbData, cbData, pcbRemaining );
    //SymCryptSha256AppendBlocks_xmm1( pChain, pbData, cbData, pcbRemaining );  !!! Needs Save/restore logic
    //SymCryptSha256AppendBlocks_xmm2( pChain, pbData, cbData, pcbRemaining );
}
