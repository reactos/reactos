//
// Sha1.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This revised implementation is based on the older one in RSA32LIB by
// Scott Field and Dan Shumow. It is not based on any 3rd party code.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//

const SYMCRYPT_HASH SymCryptSha1Algorithm_default = {
    &SymCryptSha1Init,
    &SymCryptSha1Append,
    &SymCryptSha1Result,
    &SymCryptSha1AppendBlocks,
    &SymCryptSha1StateCopy,
    sizeof( SYMCRYPT_SHA1_STATE ),
    SYMCRYPT_SHA1_RESULT_SIZE,
    SYMCRYPT_SHA1_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_SHA1_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_SHA1_STATE, chain ),
};

const PCSYMCRYPT_HASH SymCryptSha1Algorithm = &SymCryptSha1Algorithm_default;


//
// The round constants used by SHA-1
//
static const UINT32 Sha1K[4] = {
    0x5a827999UL, 0x6ed9eba1UL, 0x8f1bbcdcUL, 0xca62c1d6UL,
};

//
// Initial state
//
static const UINT32 sha1InitialState[5] = {
    0x67452301UL,
    0xefcdab89UL,
    0x98badcfeUL,
    0x10325476UL,
    0xc3d2e1f0UL,
};

//
// SymCryptSha1
//
#define ALG     SHA1
#define Alg     Sha1
#include "hash_pattern.c"
#undef ALG
#undef Alg




//
// SymCryptSha1Init
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha1Init( _Out_ PSYMCRYPT_SHA1_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthL = 0;
    pState->dataLengthH = 0;
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &sha1InitialState[0], sizeof( sha1InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


//
// SymCryptSha1Append
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha1Append(
    _Inout_                 PSYMCRYPT_SHA1_STATE    pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData )
{
    SymCryptHashAppendInternal( SymCryptSha1Algorithm, (PSYMCRYPT_COMMON_HASH_STATE)pState, pbData, cbData );
}


//
// SymCryptSha1Result
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptSha1Result(
    _Inout_                                     PSYMCRYPT_SHA1_STATE    pState,
    _Out_writes_( SYMCRYPT_SHA1_RESULT_SIZE )   PBYTE                   pbResult )
{
    UINT32 bytesInBuffer;
    SIZE_T tmp;

    //
    // SHA-1 uses almost the MD4 padding, except that the length in the padding is stored
    // MSBFirst, rather than LSBFirst.
    // As SHA-256 has a dedicated (fast) padding anyway, there is no gain to create a
    // common padding routine for SHA-1 as it wouldn't be shared by anyone right now.
    //
    SYMCRYPT_CHECK_MAGIC( pState );

    bytesInBuffer = (UINT32)(pState->bytesInBuffer);

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
        memset( &pState->buffer[bytesInBuffer], 0, 64-bytesInBuffer );
        SymCryptSha1AppendBlocks( &pState->chain, pState->buffer, 64, &tmp );
        bytesInBuffer = 0;
    }

    //
    // Set rest of padding
    // At this point bytesInBuffer <= 64-8, so we don't have an underflow
    // We wipe to the end of the buffer as it is 16-aligned,
    // and it is faster to wipe to an aligned point
    //
    memset( &pState->buffer[bytesInBuffer], 0, 64-bytesInBuffer );
    SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[64-8], pState->dataLengthL * 8 );

    //
    // Process the final block
    //
    SymCryptSha1AppendBlocks( &pState->chain, pState->buffer, 64, &tmp );

    //
    // Write the output in the correct byte order
    //
    SymCryptUint32ToMsbFirst( &pState->chain.H[0], pbResult, 5 );

    //
    // Wipe & re-initialize
    // We have to wipe the whole state because the Init call
    // might be optimized away by a smart compiler.
    //
    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
    SymCryptSha1Init( pState );
}


//
// For documentation on these function see FIPS 180-2
//
// CH, MAJ and PARITY are the functions Ch, Maj, and Parity from the standard.
//
//#define CH( x, y, z )     (((x) & (y)) ^ ((~(x)) & (z)))
//#define MAJ( x, y, z )    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define MAJ( x, y, z )  ((((x) | (y)) & (z) ) | ((x) & (y)))
#define CH( x, y, z )  ((((z) ^ (y)) & (x)) ^ (z))

#define PARITY( x, y, z ) ((x) ^ (y) ^ (z) )


//
// The values a-e are stored in an array called ae.
// We have unrolled the code completely. This makes both the indices into
// the ae array constant, and it makes the message addressing constant.
//

//
// Initial round macro
//
// r is the round number
// ae[(r+0)%5] = e;
// ae[(r+1)%5] = d;
// ae[(r+2)%5] = c;
// ae[(r+3)%5] = b;
// ae[(r+4)%5] = a;
// After that incrementing the round number will automatically map a->b, b->c, etc.
//

//
// The core round routine (excluding the message schedule)
//
// In more readable form this macro does the following:
//      e = ROL(a,5) + F(b,c,d) + e + K[r/20] + W[round]
//      b = ROL( b, 30 )
//

#define CROUND( a, b, c, d, e, r, F ) {\
    W[r%16] = Wt; \
    e += ROL32( a, 5 ) + F(b, c, d) + Sha1K[r/20] + Wt;\
    b = ROR32( b, 2 );\
}

#define IROUND( a, b, c, d, e, r, F ) { \
    Wt = SYMCRYPT_LOAD_MSBFIRST32( &pbData[ 4*r ] ); \
    CROUND( a, b, c, d, e, r, F ); \
}

//
// Subsequent rounds.
// This is the same as the IROUND except that it adds the message schedule,
// and takes the message word from the intermediate
//
#define FROUND( a, b, c, d, e, r, F ) { \
    Wt = ROL32( W[(r+13)%16] ^ W[(r+8)%16] ^ W[(r+2)%16] ^ W[r%16], 1 );\
    CROUND( a, b, c, d, e, r, F ); \
}

VOID
SYMCRYPT_CALL
SymCryptSha1AppendBlocks(
    _Inout_                 SYMCRYPT_SHA1_CHAINING_STATE *  pChain,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T                        * pcbRemaining )
{

    SYMCRYPT_ALIGN UINT32 W[16];
    UINT32 A, B, C, D, E;
    UINT32 Wt;

    A = pChain->H[0];
    B = pChain->H[1];
    C = pChain->H[2];
    D = pChain->H[3];
    E = pChain->H[4];

    while( cbData >= 64 )
    {
        //
        // initial rounds 1 to 16
        //

        IROUND(  A, B, C, D, E,  0, CH );
        IROUND(  E, A, B, C, D,  1, CH );
        IROUND(  D, E, A, B, C,  2, CH );
        IROUND(  C, D, E, A, B,  3, CH );
        IROUND(  B, C, D, E, A,  4, CH );
        IROUND(  A, B, C, D, E,  5, CH );
        IROUND(  E, A, B, C, D,  6, CH );
        IROUND(  D, E, A, B, C,  7, CH );
        IROUND(  C, D, E, A, B,  8, CH );
        IROUND(  B, C, D, E, A,  9, CH );
        IROUND(  A, B, C, D, E, 10, CH );
        IROUND(  E, A, B, C, D, 11, CH );
        IROUND(  D, E, A, B, C, 12, CH );
        IROUND(  C, D, E, A, B, 13, CH );
        IROUND(  B, C, D, E, A, 14, CH );
        IROUND(  A, B, C, D, E, 15, CH );

        //
        // Full rounds (including msg expansion) from here on
        //
        FROUND(  E, A, B, C, D, 16, CH );
        FROUND(  D, E, A, B, C, 17, CH );
        FROUND(  C, D, E, A, B, 18, CH );
        FROUND(  B, C, D, E, A, 19, CH );


        FROUND(  A, B, C, D, E, 20, PARITY );
        FROUND(  E, A, B, C, D, 21, PARITY );
        FROUND(  D, E, A, B, C, 22, PARITY );
        FROUND(  C, D, E, A, B, 23, PARITY );
        FROUND(  B, C, D, E, A, 24, PARITY );
        FROUND(  A, B, C, D, E, 25, PARITY );
        FROUND(  E, A, B, C, D, 26, PARITY );
        FROUND(  D, E, A, B, C, 27, PARITY );
        FROUND(  C, D, E, A, B, 28, PARITY );
        FROUND(  B, C, D, E, A, 29, PARITY );
        FROUND(  A, B, C, D, E, 30, PARITY );
        FROUND(  E, A, B, C, D, 31, PARITY );
        FROUND(  D, E, A, B, C, 32, PARITY );
        FROUND(  C, D, E, A, B, 33, PARITY );
        FROUND(  B, C, D, E, A, 34, PARITY );
        FROUND(  A, B, C, D, E, 35, PARITY );
        FROUND(  E, A, B, C, D, 36, PARITY );
        FROUND(  D, E, A, B, C, 37, PARITY );
        FROUND(  C, D, E, A, B, 38, PARITY );
        FROUND(  B, C, D, E, A, 39, PARITY );


        FROUND(  A, B, C, D, E, 40, MAJ );
        FROUND(  E, A, B, C, D, 41, MAJ );
        FROUND(  D, E, A, B, C, 42, MAJ );
        FROUND(  C, D, E, A, B, 43, MAJ );
        FROUND(  B, C, D, E, A, 44, MAJ );
        FROUND(  A, B, C, D, E, 45, MAJ );
        FROUND(  E, A, B, C, D, 46, MAJ );
        FROUND(  D, E, A, B, C, 47, MAJ );
        FROUND(  C, D, E, A, B, 48, MAJ );
        FROUND(  B, C, D, E, A, 49, MAJ );
        FROUND(  A, B, C, D, E, 50, MAJ );
        FROUND(  E, A, B, C, D, 51, MAJ );
        FROUND(  D, E, A, B, C, 52, MAJ );
        FROUND(  C, D, E, A, B, 53, MAJ );
        FROUND(  B, C, D, E, A, 54, MAJ );
        FROUND(  A, B, C, D, E, 55, MAJ );
        FROUND(  E, A, B, C, D, 56, MAJ );
        FROUND(  D, E, A, B, C, 57, MAJ );
        FROUND(  C, D, E, A, B, 58, MAJ );
        FROUND(  B, C, D, E, A, 59, MAJ );

        FROUND(  A, B, C, D, E, 60, PARITY );
        FROUND(  E, A, B, C, D, 61, PARITY );
        FROUND(  D, E, A, B, C, 62, PARITY );
        FROUND(  C, D, E, A, B, 63, PARITY );
        FROUND(  B, C, D, E, A, 64, PARITY );
        FROUND(  A, B, C, D, E, 65, PARITY );
        FROUND(  E, A, B, C, D, 66, PARITY );
        FROUND(  D, E, A, B, C, 67, PARITY );
        FROUND(  C, D, E, A, B, 68, PARITY );
        FROUND(  B, C, D, E, A, 69, PARITY );
        FROUND(  A, B, C, D, E, 70, PARITY );
        FROUND(  E, A, B, C, D, 71, PARITY );
        FROUND(  D, E, A, B, C, 72, PARITY );
        FROUND(  C, D, E, A, B, 73, PARITY );
        FROUND(  B, C, D, E, A, 74, PARITY );
        FROUND(  A, B, C, D, E, 75, PARITY );
        FROUND(  E, A, B, C, D, 76, PARITY );
        FROUND(  D, E, A, B, C, 77, PARITY );
        FROUND(  C, D, E, A, B, 78, PARITY );
        FROUND(  B, C, D, E, A, 79, PARITY );


        pChain->H[0] = A = A + pChain->H[0];
        pChain->H[1] = B = B + pChain->H[1];
        pChain->H[2] = C = C + pChain->H[2];
        pChain->H[3] = D = D + pChain->H[3];
        pChain->H[4] = E = E + pChain->H[4];

        pbData += 64;
        cbData -= 64;
    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( W, sizeof( W ) );
    SYMCRYPT_FORCE_WRITE32( &A, 0 );
    SYMCRYPT_FORCE_WRITE32( &B, 0 );
    SYMCRYPT_FORCE_WRITE32( &C, 0 );
    SYMCRYPT_FORCE_WRITE32( &D, 0 );
    SYMCRYPT_FORCE_WRITE32( &E, 0 );
    SYMCRYPT_FORCE_WRITE32( &Wt, 0 );
}


VOID
SYMCRYPT_CALL
SymCryptSha1StateExport(
    _In_                                                    PCSYMCRYPT_SHA1_STATE   pState,
    _Out_writes_bytes_( SYMCRYPT_SHA1_STATE_EXPORT_SIZE )   PBYTE                   pbBlob )
{
    SYMCRYPT_ALIGN SYMCRYPT_SHA1_STATE_EXPORT_BLOB    blob;           // local copy to have proper alignment.
    C_ASSERT( sizeof( blob ) == SYMCRYPT_SHA1_STATE_EXPORT_SIZE );

    SYMCRYPT_CHECK_MAGIC( pState );

    SymCryptWipeKnownSize( &blob, sizeof( blob ) ); // wipe to avoid any data leakage

    blob.header.magic = SYMCRYPT_BLOB_MAGIC;
    blob.header.size = SYMCRYPT_SHA1_STATE_EXPORT_SIZE;
    blob.header.type = SymCryptBlobTypeSha1State;

    //
    // Copy the relevant data. Buffer will be 0-padded.
    //

    SymCryptUint32ToMsbFirst( &pState->chain.H[0], &blob.chain[0], 5 );
    blob.dataLength = pState->dataLengthL;
    memcpy( &blob.buffer[0], &pState->buffer[0], blob.dataLength & 0x3f );

    SYMCRYPT_ASSERT( (PCBYTE) &blob + sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ) == (PCBYTE) &blob.trailer );
    SymCryptMarvin32( SymCryptMarvin32DefaultSeed, (PCBYTE) &blob, sizeof( blob ) - sizeof( SYMCRYPT_BLOB_TRAILER ), &blob.trailer.checksum[0] );

    memcpy( pbBlob, &blob, sizeof( blob ) );

//cleanup:
    SymCryptWipeKnownSize( &blob, sizeof( blob ) );
    return;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha1StateImport(
    _Out_                                               PSYMCRYPT_SHA1_STATE    pState,
    _In_reads_bytes_( SYMCRYPT_SHA1_STATE_EXPORT_SIZE)  PCBYTE                  pbBlob )
{
    SYMCRYPT_ERROR                  scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_ALIGN SYMCRYPT_SHA1_STATE_EXPORT_BLOB blob;                       // local copy to have proper alignment.
    BYTE                            checksum[8];

    C_ASSERT( sizeof( blob ) == SYMCRYPT_SHA1_STATE_EXPORT_SIZE );
    memcpy( &blob, pbBlob, sizeof( blob ) );

    if( blob.header.magic != SYMCRYPT_BLOB_MAGIC ||
        blob.header.size != SYMCRYPT_SHA1_STATE_EXPORT_SIZE ||
        blob.header.type != SymCryptBlobTypeSha1State )
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

    SymCryptMsbFirstToUint32( &blob.chain[0], &pState->chain.H[0], 5 );
    pState->dataLengthL = blob.dataLength;
    pState->dataLengthH = 0;
    pState->bytesInBuffer = blob.dataLength & 0x3f;
    memcpy( &pState->buffer[0], &blob.buffer[0], pState->bytesInBuffer );

    SYMCRYPT_SET_MAGIC( pState );

cleanup:
    SymCryptWipeKnownSize( &blob, sizeof(blob) );
    return scError;
}



//
// Simple test vector for FIPS module testing
//

static const BYTE sha1KATAnswer[ 20 ] = {
    0xa9, 0x99, 0x3e, 0x36,
    0x47, 0x06, 0x81, 0x6a,
    0xba, 0x3e, 0x25, 0x71,
    0x78, 0x50, 0xc2, 0x6c,
    0x9c, 0xd0, 0xd8, 0x9d
    } ;

VOID
SYMCRYPT_CALL
SymCryptSha1Selftest(void)
{
    BYTE result[SYMCRYPT_SHA1_RESULT_SIZE];

    SymCryptSha1( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, sha1KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'SHA1' );
    }
}
