//
// ChaCha20.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

VOID
SYMCRYPT_CALL
SymCryptChaCha20CryptBlocks(
    _Inout_                 PSYMCRYPT_CHACHA20_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbSrc,
    _Out_writes_( cbData )  PBYTE                       pbDst,
                            SIZE_T                      cbData );
// Encrypt Src to Dst using whole blocks, starting at block floor(pState->offset/64).
// # blocks processed is floor( cbData / 64 )
// pState->offset point is updated by 64 for each block encrypted



#define OFFSET_MASK     (((UINT64)1 << 38) - 1)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptChaCha20Init(
    _Out_                   PSYMCRYPT_CHACHA20_STATE    pState,
    _In_reads_( cbKey )     PCBYTE                      pbKey,
    _In_                    SIZE_T                      cbKey,
    _In_reads_( cbNonce )   PCBYTE                      pbNonce,
                            SIZE_T                      cbNonce,
                            UINT64                      offset )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if (cbKey != 32)
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    if (cbNonce != 12)
    {
        scError = SYMCRYPT_WRONG_NONCE_SIZE;
        goto cleanup;
    }

    SymCryptLsbFirstToUint32( pbKey, &pState->key[0], 8 );
    SymCryptLsbFirstToUint32( pbNonce, &pState->nonce[0], 3 );

    SymCryptChaCha20SetOffset( pState, offset );

cleanup:
    return scError;
}

VOID
SYMCRYPT_CALL
SymCryptChaCha20SetOffset(
    _Inout_                 PSYMCRYPT_CHACHA20_STATE    pState,
                            UINT64                      offset )
{
    pState->offset = offset;
    pState->keystreamBufferValid = FALSE;
}

VOID
SYMCRYPT_CALL
SymCryptChaCha20Crypt(
    _Inout_                 PSYMCRYPT_CHACHA20_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbSrc,
    _Out_writes_( cbData )  PBYTE                       pbDst,
                            SIZE_T                      cbData )
{
    UINT32  blockOffset;
    SIZE_T  nBytes;

    blockOffset = pState->offset & 0x3f;

    // If the offset is in the middle of the block, we first crypt until the end
    // of the block
    if( blockOffset != 0 )
    {
        if( !pState->keystreamBufferValid )
        {
            // Generate a block of key stream
            SymCryptWipe( &pState->keystream[0], 64 );
            SymCryptChaCha20CryptBlocks(    pState,
                                            &pState->keystream[0],
                                            &pState->keystream[0],
                                            64 );
            pState->offset -= 64;   // Don't update the offset yet
        }

        nBytes = 64 - blockOffset;  // # bytes in buffer starting at offset
        if( cbData < nBytes )
        {
            // We don't use the generated block to the end. The buffer will be valid
            // at the end as the offset won't advance beyond the block.
            nBytes = cbData;
            pState->keystreamBufferValid = TRUE;
        } else {
            // We'll use the rest of the generated block. After that the key stream
            // buffer won't be valid as the offset will advance beyond it.
            pState->keystreamBufferValid = FALSE;
        }

        SymCryptXorBytes( pbSrc, &pState->keystream[ blockOffset ], pbDst, nBytes );
        pbSrc += nBytes;
        pbDst += nBytes;
        cbData -= nBytes;
        pState->offset += nBytes;
    }

    // Here: pbSrc, pbDst, cbData, and pState->offset all in sync
    // and either cbData == 0 or offset is at a block boundary

    if( cbData >= 64 )
    {
        nBytes = cbData & ~0x3f;
        SymCryptChaCha20CryptBlocks( pState, pbSrc, pbDst, nBytes );
        pbSrc += nBytes;
        pbDst += nBytes;
        cbData -= nBytes;
    }

    if( cbData > 0 )
    {
        // Generate a block of key stream
        SymCryptWipe( &pState->keystream[0], 64 );
        SymCryptChaCha20CryptBlocks(    pState,
                                        &pState->keystream[0],
                                        &pState->keystream[0],
                                        64 );
        pState->offset -= 64;   // Don't update the offset yet
        pState->keystreamBufferValid = TRUE;

        SymCryptXorBytes( pbSrc, &pState->keystream[0], pbDst, cbData );
        pState->offset += cbData;
        // The following updates are correct but not needed
        // pbSrc += cbData;
        // pbDst += cbData;
        // cbData -= cbData;
    }
}

#define CHACHA_QUARTERROUND( a, b, c, d ) { \
    a += b; d ^= a; d = ROL32( d, 16 ); \
    c += d; b ^= c; b = ROL32( b, 12 ); \
    a += b; d ^= a; d = ROL32( d, 8 ); \
    c += d; b ^= c; b = ROL32( b, 7 ); \
}

VOID
SYMCRYPT_CALL
SymCryptChaCha20CryptBlocks(
    _Inout_                 PSYMCRYPT_CHACHA20_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbSrc,
    _Out_writes_( cbData )  PBYTE                       pbDst,
                            SIZE_T                      cbData )
{
    UINT32 counter;
    UINT32 s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15;
    int i;

    counter = (UINT32)(pState->offset >> 6);

    while( cbData >= 64 )
    {
        // Initialize the state
        s0  = 0x61707865;
        s1  = 0x3320646e;
        s2  = 0x79622d32;
        s3  = 0x6b206574;
        s4  = pState->key[0];
        s5  = pState->key[1];
        s6  = pState->key[2];
        s7  = pState->key[3];
        s8  = pState->key[4];
        s9  = pState->key[5];
        s10 = pState->key[6];
        s11 = pState->key[7];
        s12 = counter;
        s13 = pState->nonce[0];
        s14 = pState->nonce[1];
        s15 = pState->nonce[2];

        for( i=0; i<10; i++ )
        {
            CHACHA_QUARTERROUND( s0 , s4 , s8 , s12 );
            CHACHA_QUARTERROUND( s1 , s5 , s9 , s13 );
            CHACHA_QUARTERROUND( s2 , s6 , s10, s14 );
            CHACHA_QUARTERROUND( s3 , s7 , s11, s15 );

            CHACHA_QUARTERROUND( s0 , s5 , s10, s15 );
            CHACHA_QUARTERROUND( s1 , s6 , s11, s12 );
            CHACHA_QUARTERROUND( s2 , s7 , s8 , s13 );
            CHACHA_QUARTERROUND( s3 , s4 , s9 , s14 );
        }

        s0  += 0x61707865;
        s1  += 0x3320646e;
        s2  += 0x79622d32;
        s3  += 0x6b206574;
        s4  += pState->key[0];
        s5  += pState->key[1];
        s6  += pState->key[2];
        s7  += pState->key[3];
        s8  += pState->key[4];
        s9  += pState->key[5];
        s10 += pState->key[6];
        s11 += pState->key[7];
        s12 += counter;
        s13 += pState->nonce[0];
        s14 += pState->nonce[1];
        s15 += pState->nonce[2];

        SYMCRYPT_STORE_LSBFIRST32( pbDst +  0, s0  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc +  0 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst +  4, s1  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc +  4 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst +  8, s2  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc +  8 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 12, s3  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 12 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 16, s4  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 16 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 20, s5  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 20 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 24, s6  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 24 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 28, s7  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 28 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 32, s8  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 32 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 36, s9  ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 36 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 40, s10 ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 40 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 44, s11 ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 44 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 48, s12 ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 48 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 52, s13 ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 52 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 56, s14 ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 56 ) );
        SYMCRYPT_STORE_LSBFIRST32( pbDst + 60, s15 ^ SYMCRYPT_LOAD_LSBFIRST32( pbSrc + 60 ) );

        counter ++;
        // If counter overflows then the caller has encrypted more than 256GB of data with a single stream, which is
        // called out as being insecure. It is the caller's responsibility to avoid this!
        pbSrc += 64;
        pbDst += 64;
        cbData -= 64;
        pState->offset += 64;
    }
}

static const BYTE   chacha20KatAnswer[ 3 ] = { 0xb5, 0xe0, 0x54 };

VOID
SYMCRYPT_CALL
SymCryptChaCha20Selftest(void)
{
    BYTE buf[3];
    SYMCRYPT_CHACHA20_STATE  state;

    SymCryptChaCha20Init(   &state,
                            SymCryptTestKey32, sizeof( SymCryptTestKey32 ),
                            SymCryptTestMsg16, 12,
                            0 );

    SymCryptChaCha20Crypt( &state, SymCryptTestMsg3, buf, sizeof( buf ) );

    SymCryptInjectError( buf, sizeof( buf ) );

    if( memcmp( buf, chacha20KatAnswer, sizeof( buf )) != 0 )
    {
        SymCryptFatal( 'Cha2' );
    }
}
