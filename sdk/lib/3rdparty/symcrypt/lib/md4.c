//
// Md4.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement MD4 from RFC 1320
//
//
// This is a new implementation, NOT based on the existing ones in RSA32.lib.
// There are 2 versions in RSA32.lib, one from RSA data security and one from
// Scott Fields.
//
// MD4 and MD5 are extremely similar. Having already done a new MD5 implementation it
// was very little work to copy the code & turn it into an MD4 implementation.
// In fact, it was easier than reviewing & modifying the old code to bring it up to
// the current implementation guidelines.
//
// This also ensures that this file is not a derived work from RSA data security
// code which simplifies the copyright situation.
//
// We dropped the assembler implementation. MD4 is so weak that it should be removed
// from use, not sped up.
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//

const SYMCRYPT_HASH SymCryptMd4Algorithm_default = {
    &SymCryptMd4Init,
    &SymCryptMd4Append,
    &SymCryptMd4Result,
    &SymCryptMd4AppendBlocks,
    &SymCryptMd4StateCopy,
    sizeof( SYMCRYPT_MD4_STATE ),
    SYMCRYPT_MD4_RESULT_SIZE,
    SYMCRYPT_MD4_INPUT_BLOCK_SIZE,
    SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MD4_STATE, chain ),
    SYMCRYPT_FIELD_SIZE( SYMCRYPT_MD4_STATE, chain ),
};

const PCSYMCRYPT_HASH SymCryptMd4Algorithm = &SymCryptMd4Algorithm_default;

//
// The round constants used by MD4
//
//
static const UINT32 md4Const[3] = {
    0x00000000UL,
    0x5A827999UL,
    0x6ED9EBA1UL,
    };

//
// Round rotation amounts. This array is optimized away by the compiler
// as we inline all our rotations.
//
static const int md4Rotate[48] = {
    3, 7, 11, 19,
    3, 7, 11, 19,
    3, 7, 11, 19,
    3, 7, 11, 19,

    3, 5, 9, 13,
    3, 5, 9, 13,
    3, 5, 9, 13,
    3, 5, 9, 13,

    3, 9, 11, 15,
    3, 9, 11, 15,
    3, 9, 11, 15,
    3, 9, 11, 15,

};

//
// Message word index table. This array is optimized away by the compiler
// as we inline all our accesses.
//
static const int md4MsgIndex[48] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
     0,  4,  8, 12,  1,  5,  9, 13,  2,  6, 10, 14,  3,  7, 11, 15,
     0,  8,  4, 12,  2, 10,  6, 14,  1,  9,  5, 13,  3, 11,  7, 15,
};

//
// Initial state
//
static const UINT32 md4InitialState[4] = {
    0x67452301UL,
    0xefcdab89UL,
    0x98badcfeUL,
    0x10325476UL,
};


//
// SymCryptMd4
//
#define ALG MD4
#define Alg Md4
#include "hash_pattern.c"
#undef ALG
#undef Alg



//
// SymCryptmd4Init
//
VOID
SYMCRYPT_CALL
SymCryptMd4Init( _Out_ PSYMCRYPT_MD4_STATE pState )
{
    SYMCRYPT_SET_MAGIC( pState );

    pState->dataLengthL = 0;
    pState->dataLengthH = 0;
    pState->bytesInBuffer = 0;

    memcpy( &pState->chain.H[0], &md4InitialState[0], sizeof( md4InitialState ) );

    //
    // There is no need to initialize the buffer part of the state as that will be
    // filled before it is used.
    //
}


//
// SymCryptMd4Append
//
VOID
SYMCRYPT_CALL
SymCryptMd4Append( _Inout_              PSYMCRYPT_MD4_STATE    pState,
                   _In_reads_( cbData ) PCBYTE                 pbData,
                                        SIZE_T                 cbData )
{
    SymCryptHashAppendInternal( SymCryptMd4Algorithm, (PSYMCRYPT_COMMON_HASH_STATE)pState, pbData, cbData );
}


//
// SymCryptmd4Result
//
VOID
SYMCRYPT_CALL
SymCryptMd4Result(
    _Inout_                                 PSYMCRYPT_MD4_STATE pState,
   _Out_writes_( SYMCRYPT_MD4_RESULT_SIZE ) PBYTE               pbResult )
{
    SymCryptHashCommonPaddingMd4Style( SymCryptMd4Algorithm, (PSYMCRYPT_COMMON_HASH_STATE)pState );

    //
    // Write the output in the correct byte order
    //
    SymCryptUint32ToLsbFirst( &pState->chain.H[0], pbResult, 4 );

    //
    // Wipe & re-initialize
    // We have to wipe the whole state as the initialization might be optimized away.
    //
    SymCryptWipeKnownSize( pState, sizeof( *pState ));
    SymCryptMd4Init( pState );
    }


//
// For documentation on these function see rfc-1320
//
//#define F( x, y, z )     (((x) & (y)) | ((~(x)) & (z)))
//#define G( x, y, z )    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define F( x, y, z )    ((((z) ^ (y)) & (x)) ^ (z))
#define G( x, y, z )    ((((x) | (y)) & (z) ) | ((x) & (y)))
#define H( x, y, z )    ((x) ^ (y) ^ (z) )

//
// The values a-d are stored in an array called ad.
// We have unrolled the code completely. This makes both the indices into
// the ad array constant, and it makes the message addressing constant.
//
// We copy the message into our own buffer to obey the read-once rule.
// Memory is sometimes aliased so that multiple threads or processes can access
// the same memory at the same time. With MD4 there is a danger that some other
// process could modify the memory while the computation is ongoing and introduce
// changes in the computation not envisioned by the designers or cryptanalists.
// At this level in the library we cannot guarantee that this is not the case,
// and we can't trust the higher layers to respect a don't-change-it-while-computing-md4
// restriction. (In practice, such restrictions are lost through the many
// layers in the stack.)
//

//
// r is the round number
// ad[(r+0)%4] = a;
// ad[(r+1)%4] = d;
// ad[(r+2)%4] = c;
// ad[(r+3)%4] = b;
//
// When r increments the register re-naming is automatically correct.
//

//
// CROUND is the core round function
//
#define CROUND( r, Func ) { \
    ad[r%4] = ROL32( ad[r%4] + Func(ad[(r+3)%4], ad[(r+2)%4], ad[(r+1)%4]) + Wt + md4Const[r/16], md4Rotate[r] ); \
}

//
// IROUND is the initial round that loads the message and copies it into our buffer.
//
#define IROUND( r, Func ) { \
    Wt = SYMCRYPT_LOAD_LSBFIRST32( &pbData[ 4*md4MsgIndex[r] ] ); \
    W[r] = Wt; \
    CROUND( r, Func ); \
}

//
// FROUND are the subsequent rounds.
//
#define FROUND( r, Func ) { \
    Wt = W[md4MsgIndex[r]];\
    CROUND( r, Func ); \
}

VOID
SYMCRYPT_CALL
SymCryptMd4AppendBlocks(
    _Inout_                 PSYMCRYPT_MD4_CHAINING_STATE    pChain,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T                        * pcbRemaining )
{

    SYMCRYPT_ALIGN UINT32 W[16];
    SYMCRYPT_ALIGN UINT32 ad[4];
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
    SYMCRYPT_FORCE_WRITE32( &Wt, 0 );
}

VOID
SYMCRYPT_CALL
SymCryptMd4StateExport(
    _In_                                                    PCSYMCRYPT_MD4_STATE    pState,
    _Out_writes_bytes_( SYMCRYPT_MD4_STATE_EXPORT_SIZE )    PBYTE                   pbBlob )
{
    SYMCRYPT_ALIGN SYMCRYPT_MD4_STATE_EXPORT_BLOB    blob;           // local copy to have proper alignment.
    C_ASSERT( sizeof( blob ) == SYMCRYPT_MD4_STATE_EXPORT_SIZE );

    SYMCRYPT_CHECK_MAGIC( pState );

    SymCryptWipeKnownSize( &blob, sizeof( blob ) ); // wipe to avoid any data leakage

    blob.header.magic = SYMCRYPT_BLOB_MAGIC;
    blob.header.size = SYMCRYPT_MD4_STATE_EXPORT_SIZE;
    blob.header.type = SymCryptBlobTypeMd4State;

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
SymCryptMd4StateImport(
    _Out_                                               PSYMCRYPT_MD4_STATE     pState,
    _In_reads_bytes_( SYMCRYPT_MD4_STATE_EXPORT_SIZE)   PCBYTE                  pbBlob )
{
    SYMCRYPT_ERROR                  scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_ALIGN SYMCRYPT_MD4_STATE_EXPORT_BLOB  blob;                       // local copy to have proper alignment.
    BYTE                            checksum[8];

    C_ASSERT( sizeof( blob ) == SYMCRYPT_MD4_STATE_EXPORT_SIZE );
    memcpy( &blob, pbBlob, sizeof( blob ) );

    if( blob.header.magic != SYMCRYPT_BLOB_MAGIC ||
        blob.header.size != SYMCRYPT_MD4_STATE_EXPORT_SIZE ||
        blob.header.type != SymCryptBlobTypeMd4State )
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

static const BYTE   md4KATAnswer[ 16 ] = {
    0xa4, 0x48, 0x01, 0x7a, 0xaf, 0x21, 0xd8, 0x52,
    0x5f, 0xc1, 0x0a, 0xe8, 0x7a, 0xa6, 0x72, 0x9d,
} ;

VOID
SYMCRYPT_CALL
SymCryptMd4Selftest(void)
{
    BYTE result[SYMCRYPT_MD4_RESULT_SIZE];

    SymCryptMd4( SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), result );

    SymCryptInjectError( result, sizeof( result ) );

    if( memcmp( result, md4KATAnswer, sizeof( result ) ) != 0 ) {
        SymCryptFatal( 'MD4t' );
    }
}
