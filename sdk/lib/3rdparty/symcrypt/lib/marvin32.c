//
// Marvin32.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement the Marvin32 checksum function
//
//

#include "precomp.h"

//
// See the symcrypt.h file for documentation on what the various functions do.
//


//
// Default initial seed, first 8 bytes of SHA256( "Marvin32" );
//
static const SYMCRYPT_MARVIN32_EXPANDED_SEED SymCryptMarvin32DefaultSeedStruct = {
    {0xcd0893b7, 0xd53cd9ce},
#if defined( SYMCRYPT_MAGIC_ENABLED )
    SYMCRYPT_MAGIC_VALUE( &SymCryptMarvin32DefaultSeedStruct ),
#endif
    };

PCSYMCRYPT_MARVIN32_EXPANDED_SEED const SymCryptMarvin32DefaultSeed = &SymCryptMarvin32DefaultSeedStruct;

//
// Round rotation amounts. This array is optimized away by the compiler
// as we inline all our rotations.
//
static const int rotate[4] = {
    20, 9, 27, 19,
};


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMarvin32ExpandSeed(
    _Out_               PSYMCRYPT_MARVIN32_EXPANDED_SEED    pExpandedSeed,
    _In_reads_(cbSeed)  PCBYTE                              pbSeed,
                        SIZE_T                              cbSeed )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if( cbSeed != SYMCRYPT_MARVIN32_SEED_SIZE )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }
    pExpandedSeed->s[0] = SYMCRYPT_LOAD_LSBFIRST32( pbSeed );
    pExpandedSeed->s[1] = SYMCRYPT_LOAD_LSBFIRST32( pbSeed + 4 );

    SYMCRYPT_SET_MAGIC( pExpandedSeed );

cleanup:
    return scError;
}

VOID
SYMCRYPT_CALL
SymCryptMarvin32SeedCopy(   _In_    PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pSrc,
                            _Out_   PSYMCRYPT_MARVIN32_EXPANDED_SEED    pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );
    *pDst = *pSrc;
    SYMCRYPT_SET_MAGIC( pDst );
}


VOID
SYMCRYPT_CALL
SymCryptMarvin32StateCopy(
    _In_        PCSYMCRYPT_MARVIN32_STATE           pSrc,
    _In_opt_    PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pExpandedSeed,
    _Out_       PSYMCRYPT_MARVIN32_STATE            pDst )
{
    SYMCRYPT_CHECK_MAGIC( pSrc );
    *pDst = *pSrc;

    if( pExpandedSeed == NULL )
    {
        SYMCRYPT_CHECK_MAGIC( pSrc->pSeed );
        pDst->pSeed = pSrc->pSeed;
    }
    else
    {
        SYMCRYPT_CHECK_MAGIC( pExpandedSeed );
        pDst->pSeed = pExpandedSeed;
    }

    SYMCRYPT_SET_MAGIC( pDst );
}


VOID
SYMCRYPT_CALL
SymCryptMarvin32Init(   _Out_   PSYMCRYPT_MARVIN32_STATE            pState,
                        _In_    PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pExpandedSeed)
{
    pState->chain = *pExpandedSeed;
    pState->dataLength = 0;
    pState->pSeed = pExpandedSeed;

    *(UINT32 *) &pState->buffer[4] = 0; // wipe the last 4 bytes of the buffer.

    SYMCRYPT_SET_MAGIC( pState );
}


//
// SymCryptMarvin32Append
//

VOID
SYMCRYPT_CALL
SymCryptMarvin32Append(     _Inout_                 PSYMCRYPT_MARVIN32_STATE    state,
                            _In_reads_( cbData )    PCBYTE                      pbData,
                                                    SIZE_T                      cbData )
{
    UINT32 bytesInBuffer = state->dataLength;

    SYMCRYPT_CHECK_MAGIC( state );

    state->dataLength += (UINT32) cbData;    // We only keep track of the last 2 bits...

#define ALG MARVIN32
#define Alg Marvin32
#include "hash_buffer_pattern.c"
#undef ALG
#undef Alg

}


//
// SymCryptMarvin32Result
//
VOID
SYMCRYPT_CALL
SymCryptMarvin32Result(
     _Inout_                                        PSYMCRYPT_MARVIN32_STATE    pState,
     _Out_writes_( SYMCRYPT_MARVIN32_RESULT_SIZE )  PBYTE                       pbResult )
{
    SIZE_T bytesInBuffer = ( pState->dataLength) & 0x3;

    SYMCRYPT_CHECK_MAGIC( pState );

    //
    // Wipe four bytes in the buffer.
    // Doing this first ensures that this write is aligned when the input was of
    // length 0 mod 4.
    // The buffer is 8 bytes long, so we never overwrite anything else.
    //
    *(UINT32 *) &pState->buffer[bytesInBuffer] = 0;

    //
    // The buffer is never completely full, so we can always put the first
    // padding byte in.
    //
    pState->buffer[bytesInBuffer++] = 0x80;

    //
    // Process the final block
    //
    SymCryptMarvin32AppendBlocks( &pState->chain, pState->buffer, 8 );

    SYMCRYPT_STORE_LSBFIRST32( pbResult    , pState->chain.s[0] );
    SYMCRYPT_STORE_LSBFIRST32( pbResult + 4, pState->chain.s[1] );

    //
    // Wipe only those things that we need to wipe.
    //

    *(UINT32 *) &pState->buffer[0] = 0;
    pState->dataLength = 0;

    pState->chain = *pState->pSeed;
}

#define BLOCK( a, b ) \
{\
    b ^= a; a = ROL32( a, rotate[0] );\
    a += b; b = ROL32( b, rotate[1] );\
    b ^= a; a = ROL32( a, rotate[2] );\
    a += b; b = ROL32( b, rotate[3] );\
}

VOID
SYMCRYPT_CALL
SymCryptMarvin32AppendBlocks(
    _Inout_                 PSYMCRYPT_MARVIN32_CHAINING_STATE   pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData )
{
    UINT32 s0 = pChain->s[0];
    UINT32 s1 = pChain->s[1];

    SIZE_T bytesInFirstBlock = cbData & 0xc;        // 0, 4, 8, or 12

    SYMCRYPT_ASSERT( (cbData & 3) == 0 );


    pbData += bytesInFirstBlock;
    cbData -= bytesInFirstBlock;

    switch( bytesInFirstBlock )
    {
    case 0: // This handles the cbData == 0 case too
        while( cbData > 0 )
        {
            pbData += 16;
            cbData -= 16;

            s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData - 16 );
            BLOCK( s0, s1 );
    case 12:
            s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData - 12 );
            BLOCK( s0, s1 );
    case 8:
            s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData -  8 );
            BLOCK( s0, s1 );
    case 4:
            s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData -  4 );
            BLOCK( s0, s1 );
        }
    }

    pChain->s[0] = s0;
    pChain->s[1] = s1;
}


VOID
SYMCRYPT_CALL
SymCryptMarvin32(
    _In_                                            PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pExpandedSeed,
    _In_reads_( cbData )                            PCBYTE                              pbData,
                                                    SIZE_T                              cbData,
    _Out_writes_( SYMCRYPT_MARVIN32_RESULT_SIZE )   PBYTE                               pbResult )
//
// To reduce the per-computation overhead, we have a dedicated code here instead of the whole Init/Append/Result stuff.
//
{
    UINT32 tmp;

    UINT32 s0 = pExpandedSeed->s[0];
    UINT32 s1 = pExpandedSeed->s[1];

    while( cbData > 7 )
    {
        s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData );
        BLOCK( s0, s1 );
        s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData + 4 );
        BLOCK( s0, s1 );
        pbData += 8;
        cbData -= 8;
    }

    /*
    switch( cbData )
    {
    case 3:
        buf[2] = pbData[2];
    case 2:
        *(UINT16 *) &buf[0] = *(UINT16 *) pbData;
        break;
    case 1:
        buf[0] = pbData[0];
    case 0:
        ;
    }

    buf[ cbData ] = 0x80;

    s0 += LOAD_LSBFIRST32( buf );
    */


    switch( cbData )
    {
    default:
    case 4: s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData ); BLOCK( s0, s1 ); pbData += 4;
    case 0: tmp = 0x80; break;

    case 5: s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData ); BLOCK( s0, s1 ); pbData += 4;
    case 1: tmp = 0x8000 | pbData[0]; break;

    case 6: s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData ); BLOCK( s0, s1 ); pbData += 4;
    case 2: tmp = 0x800000 | SYMCRYPT_LOAD_LSBFIRST16( pbData ); break;

    case 7: s0 += SYMCRYPT_LOAD_LSBFIRST32( pbData ); BLOCK( s0, s1 ); pbData += 4;
    case 3: tmp = SYMCRYPT_LOAD_LSBFIRST16( pbData ) | (pbData[2] << 16) | 0x80000000; break;
    }
    s0 += tmp;


    BLOCK( s0, s1 );
    BLOCK( s0, s1 );

    SYMCRYPT_STORE_LSBFIRST32( pbResult    , s0 );
    SYMCRYPT_STORE_LSBFIRST32( pbResult + 4, s1 );
}



//
// Simple test vector
//

static const BYTE   marvin32KATAnswer[ 8 ] = {
    0xbf, 0x69, 0x27, 0x49, 0x39, 0x43, 0xc7, 0x22,
} ;

VOID
SYMCRYPT_CALL
SymCryptMarvin32Selftest(void)
{
    BYTE res[SYMCRYPT_MARVIN32_RESULT_SIZE];

    SymCryptMarvin32( SymCryptMarvin32DefaultSeed, SymCryptTestMsg3, sizeof( SymCryptTestMsg3 ), res );

    SymCryptInjectError( res, sizeof( res ) );
    if( memcmp( res, marvin32KATAnswer, sizeof( res ) ) != 0 )
    {
        SymCryptFatal( 'marv' );
    }
}
