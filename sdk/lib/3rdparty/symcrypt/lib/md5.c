//
// Md5.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement MD5 from RFC 1321
//
//
// This is a new implementation, NOT based on the existing one in RSA32.lib,
// which is the one from RSA data security. RFC-1321 also contains code that
// at a glance looks very similar to the RSA32.lib code.
//
// The implementation had to be refreshed anyway to conform to our coding
// guidelines for cryptographic functions.
// Re-implementing the function along the lines of our SHA-family implementations
// was easy, and it removes one file with RSA copyright from our system.
//
// The only data copied for this implementation is the round constant values
// which were copied from the RFC.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//

const SYMCRYPT_HASH SymCryptMd5Algorithm_default = {
    &SymCryptMd5Init,
    &SymCryptMd5Append,
    &SymCryptMd5Result,
    &SymCryptMd5AppendBlocks,
    &SymCryptMd5StateCopy,
    sizeof( SYMCRYPT_MD5_STATE ),
    SYMCRYPT_MD5_RESULT_SIZE,
    SYMCRYPT_MD5_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MD5_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_MD5_STATE, chain ),
};

const PCSYMCRYPT_HASH SymCryptMd5Algorithm = &SymCryptMd5Algorithm_default;

//
// The round constants used by MD5
//
// These are called T[i] in RFC1321 although T[i] uses the range [1..64] and we use [0..63]
// This array should be optimized away by the compiler as all values are inlined.
//
static const UINT32 md5Const[64] = {
    0xd76aa478UL,
    0xe8c7b756UL,
    0x242070dbUL,
    0xc1bdceeeUL,
    0xf57c0fafUL,
    0x4787c62aUL,
    0xa8304613UL,
    0xfd469501UL,
    0x698098d8UL,
    0x8b44f7afUL,
    0xffff5bb1UL,
    0x895cd7beUL,
    0x6b901122UL,
    0xfd987193UL,
    0xa679438eUL,
    0x49b40821UL,
    0xf61e2562UL,
    0xc040b340UL,
    0x265e5a51UL,
    0xe9b6c7aaUL,
    0xd62f105dUL,
    0x02441453UL,
    0xd8a1e681UL,
    0xe7d3fbc8UL,
    0x21e1cde6UL,
    0xc33707d6UL,
    0xf4d50d87UL,
    0x455a14edUL,
    0xa9e3e905UL,
    0xfcefa3f8UL,
    0x676f02d9UL,
    0x8d2a4c8aUL,
    0xfffa3942UL,
    0x8771f681UL,
    0x6d9d6122UL,
    0xfde5380cUL,
    0xa4beea44UL,
    0x4bdecfa9UL,
    0xf6bb4b60UL,
    0xbebfbc70UL,
    0x289b7ec6UL,
    0xeaa127faUL,
    0xd4ef3085UL,
    0x04881d05UL,
    0xd9d4d039UL,
    0xe6db99e5UL,
    0x1fa27cf8UL,
    0xc4ac5665UL,
    0xf4292244UL,
    0x432aff97UL,
    0xab9423a7UL,
    0xfc93a039UL,
    0x655b59c3UL,
    0x8f0ccc92UL,
    0xffeff47dUL,
    0x85845dd1UL,
    0x6fa87e4fUL,
    0xfe2ce6e0UL,
    0xa3014314UL,
    0x4e0811a1UL,
    0xf7537e82UL,
    0xbd3af235UL,
    0x2ad7d2bbUL,
    0xeb86d391UL,
};

//
// Round rotation amounts. This array is optimized away by the compiler
// as we inline all our rotations.
//
static const int md5Rotate[64] = {
    7, 12, 17, 22,
    7, 12, 17, 22,
    7, 12, 17, 22,
    7, 12, 17, 22,

    5, 9, 14, 20,
    5, 9, 14, 20,
    5, 9, 14, 20,
    5, 9, 14, 20,

    4, 11, 16, 23,
    4, 11, 16, 23,
    4, 11, 16, 23,
    4, 11, 16, 23,

    6, 10, 15, 21,
    6, 10, 15, 21,
    6, 10, 15, 21,
    6, 10, 15, 21,
};

//
// Message word index table. This array is optimized away by the compiler
// as we inline all our accesses.
//
static const int md5MsgIndex[64] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
     1,  6, 11,  0,  5, 10, 15,  4,  9, 14,  3,  8, 13,  2,  7, 12,
     5,  8, 11, 14,  1,  4,  7, 10, 13,  0,  3,  6,  9, 12, 15,  2,
     0,  7, 14,  5, 12,  3, 10,  1,  8, 15,  6, 13,  4, 11,  2,  9,
};

//
// Initial state
//
static const UINT32 md5InitialState[4] = {
    0x67452301UL,
    0xefcdab89UL,
    0x98badcfeUL,
    0x10325476UL,
};

//
// SymCryptMd5
//
#define ALG MD5
#define Alg Md5
#include "hash_pattern.c"
#undef ALG
#undef Alg




//
// SymCryptMd5Init
//
VOID
SYMCRYPT_CALL
SymCryptMd5Init( _Out_ PSYMCRYPT_MD5_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthL = 0;
    pState->dataLengthH = 0;
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &md5InitialState[0], sizeof( md5InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


//
// SymCryptMd5Append
//
VOID
SYMCRYPT_CALL
SymCryptMd5Append(
    _Inout_             PSYMCRYPT_MD5_STATE pState,
   _In_reads_( cbData ) PCBYTE              pbData,
                        SIZE_T              cbData )
{
    SymCryptHashAppendInternal( SymCryptMd5Algorithm, (PSYMCRYPT_COMMON_HASH_STATE)pState, pbData, cbData );
}

//
// SymCryptMd5Result
//
VOID
SYMCRYPT_CALL
SymCryptMd5Result(
    _Inout_                                 PSYMCRYPT_MD5_STATE pState,
   _Out_writes_( SYMCRYPT_MD5_RESULT_SIZE ) PBYTE               pbResult )
{
    SymCryptHashCommonPaddingMd4Style( SymCryptMd5Algorithm, (PSYMCRYPT_COMMON_HASH_STATE) pState );

    //
    // Write the output in the correct byte order
    //
    SymCryptUint32ToLsbFirst( &pState->chain.H[0], pbResult, 4 );

    //
    // Wipe & re-initialize
    // We have to wipe the whole state because the Init call
    // might be optimized away by a smart compiler.
    // And we need to wipe old data.
    //
    SymCryptWipeKnownSize( pState, sizeof( *pState ) );
    SymCryptMd5Init( pState );
}


//
// For documentation on these function see rfc-1321
//
//#define F( x, y, z )     (((x) & (y)) ^ ((~(x)) & (z)))
#define F( x, y, z )  ((((z) ^ (y)) & (x)) ^ (z))
#define G( x, y, z )  F( (z), (x), (y) )
#define H( x, y, z ) ((x) ^ (y) ^ (z) )
#define I( x, y, z ) ((y) ^ ((x) | ~(z)))

//
// The values a-d are stored in an array called ad.
// We have unrolled the code completely. This makes both the indices into
// the ad array constant, and it makes the message addressing constant.
//
// We copy the message into our own buffer to obey the read-once rule.
// Memory is sometimes aliased so that multiple threads or processes can access
// the same memory at the same time. With MD5 there is a danger that some other
// process could modify the memory while the computation is ongoing and introduce
// changes in the computation not envisioned by the designers or cryptanalysts.
// At this level in the library we cannot guarantee that this is not the case,
// and we can't trust the higher layers to respect a don't-change-it-while-computing-md5
// restriction. (In practice, such restrictions are lost through the many
// layers in the stack.)
//
//
// Initial round macro
//
// r is the round number
// ad[(r+0)%4] = a;
// ad[(r+1)%4] = d;
// ad[(r+2)%4] = c;
// ad[(r+3)%4] = b;
//
// When r increments the register re-naming is automatically correct.
//
#define CROUND( r, Func ) { \
    ad[r%4] = ad[(r+3)%4] + ROL32( ad[r%4] + Func(ad[(r+3)%4], ad[(r+2)%4], ad[(r+1)%4]) + Wt + md5Const[r], md5Rotate[r] ); \
}

#define IROUND( r, Func ) { \
    Wt = SYMCRYPT_LOAD_LSBFIRST32( &pbData[ 4*md5MsgIndex[r] ] ); \
    W[r] = Wt; \
    CROUND( r, Func ); \
}

//
// Subsequent rounds.
// This is the same as the IROUND except that it uses the copied message.
//
#define FROUND( r, Func ) { \
    Wt = W[md5MsgIndex[r]];\
    CROUND( r, Func ); \
}

VOID
SYMCRYPT_CALL
SymCryptMd5AppendBlocks(
    _Inout_                 SYMCRYPT_MD5_CHAINING_STATE   * pChain,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T                        * pcbRemaining )
{

    UINT32 W[16];
    UINT32 ad[4];
    UINT32 Wt;

    ad[0] = pChain->H[0];
    ad[1] = pChain->H[3];
    ad[2] = pChain->H[2];
    ad[3] = pChain->H[1];

    while( cbData >= 64 )
    {
        //
        // initial rounds 1 to 16
        //

        IROUND(  0, F );
        IROUND(  1, F );
        IROUND(  2, F );
        IROUND(  3, F );
        IROUND(  4, F );
        IROUND(  5, F );
        IROUND(  6, F );
        IROUND(  7, F );
        IROUND(  8, F );
        IROUND(  9, F );
        IROUND( 10, F );
        IROUND( 11, F );
        IROUND( 12, F );
        IROUND( 13, F );
        IROUND( 14, F );
        IROUND( 15, F );

        FROUND( 16, G );
        FROUND( 17, G );
        FROUND( 18, G );
        FROUND( 19, G );
        FROUND( 20, G );
        FROUND( 21, G );
        FROUND( 22, G );
        FROUND( 23, G );
        FROUND( 24, G );
        FROUND( 25, G );
        FROUND( 26, G );
        FROUND( 27, G );
        FROUND( 28, G );
        FROUND( 29, G );
        FROUND( 30, G );
        FROUND( 31, G );

        FROUND( 32, H );
        FROUND( 33, H );
        FROUND( 34, H );
        FROUND( 35, H );
        FROUND( 36, H );
        FROUND( 37, H );
        FROUND( 38, H );
        FROUND( 39, H );
        FROUND( 40, H );
        FROUND( 41, H );
        FROUND( 42, H );
        FROUND( 43, H );
        FROUND( 44, H );
        FROUND( 45, H );
        FROUND( 46, H );
        FROUND( 47, H );

        FROUND( 48, I );
        FROUND( 49, I );
        FROUND( 50, I );
        FROUND( 51, I );
        FROUND( 52, I );
        FROUND( 53, I );
        FROUND( 54, I );
        FROUND( 55, I );
        FROUND( 56, I );
        FROUND( 57, I );
        FROUND( 58, I );
        FROUND( 59, I );
        FROUND( 60, I );
        FROUND( 61, I );
        FROUND( 62, I );
        FROUND( 63, I );

        pChain->H[0] = ad[0] = ad[0] + pChain->H[0];
        pChain->H[3] = ad[1] = ad[1] + pChain->H[3];
        pChain->H[2] = ad[2] = ad[2] + pChain->H[2];
        pChain->H[1] = ad[3] = ad[3] + pChain->H[1];

        pbData += 64;
        cbData -= 64;
    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize( ad, sizeof( ad ) );
    SymCryptWipeKnownSize( W, sizeof( W ) );
    SymCryptWipeKnownSize( &Wt, sizeof( Wt ) );
}

VOID
SYMCRYPT_CALL
SymCryptMd5StateExport(
    _In_                                                    PCSYMCRYPT_MD5_STATE    pState,
    _Out_writes_bytes_( SYMCRYPT_MD5_STATE_EXPORT_SIZE )    PBYTE                   pbBlob )
{
    SYMCRYPT_MD5_STATE_EXPORT_BLOB    blob;           // local copy to have proper alignment.
    C_ASSERT( sizeof( blob ) == SYMCRYPT_MD5_STATE_EXPORT_SIZE );

    SYMCRYPT_CHECK_MAGIC( pState );

    SymCryptWipeKnownSize( &blob, sizeof( blob ) ); // wipe to avoid any data leakage

    blob.header.magic = SYMCRYPT_BLOB_MAGIC;
    blob.header.size = SYMCRYPT_MD5_STATE_EXPORT_SIZE;
    blob.header.type = SymCryptBlobTypeMd5State;

    //
    // Copy the relevant data. Buffer will be 0-padded.
    //

    SymCryptUint32ToLsbFirst( &pState->chain.H[0], &blob.chain[0], 4 );
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
SymCryptMd5StateImport(
    _Out_                                               PSYMCRYPT_MD5_STATE     pState,
    _In_reads_bytes_( SYMCRYPT_MD5_STATE_EXPORT_SIZE)   PCBYTE                  pbBlob )
{
    SYMCRYPT_ERROR                  scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_MD5_STATE_EXPORT_BLOB  blob;                       // local copy to have proper alignment.
    BYTE                            checksum[8];

    C_ASSERT( sizeof( blob ) == SYMCRYPT_MD5_STATE_EXPORT_SIZE );
    memcpy( &blob, pbBlob, sizeof( blob ) );

    if( blob.header.magic != SYMCRYPT_BLOB_MAGIC ||
        blob.header.size != SYMCRYPT_MD5_STATE_EXPORT_SIZE ||
        blob.header.type != SymCryptBlobTypeMd5State )
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

    SymCryptLsbFirstToUint32( &blob.chain[0], &pState->chain.H[0], 4 );
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

static const BYTE   md5KATAnswer[ 16 ] = {
    0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
    0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72,
} ;

VOID
SYMCRYPT_CALL
SymCryptMd5Selftest(void)
{
    BYTE result[SYMCRYPT_MD5_RESULT_SIZE];

    SymCryptMd5( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, md5KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'MD5t' );
    }
}
