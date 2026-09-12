//
// aes-ymm.c    code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// All YMM code for AES operations
// Requires compiler support for aesni, pclmulqdq, avx2, vaes and vpclmulqdq
//

#include "precomp.h"

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("avx2,pclmul,vaes,vpclmulqdq"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("avx2,pclmul,vaes,vpclmulqdq")
#endif

#include "xtsaes_definitions.h"
#include "ghash_definitions.h"

#define AES_ENCRYPT_YMM_2048( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m256i roundkeys; \
\
    keyPtr = pExpandedKey->RoundKey; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    /* _mm256_broadcastsi128_si256 requires AVX2 */ \
    roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
    keyPtr ++; \
\
    /* _mm256_xor_si256 requires AVX2 */ \
    c0 = _mm256_xor_si256( c0, roundkeys ); \
    c1 = _mm256_xor_si256( c1, roundkeys ); \
    c2 = _mm256_xor_si256( c2, roundkeys ); \
    c3 = _mm256_xor_si256( c3, roundkeys ); \
    c4 = _mm256_xor_si256( c4, roundkeys ); \
    c5 = _mm256_xor_si256( c5, roundkeys ); \
    c6 = _mm256_xor_si256( c6, roundkeys ); \
    c7 = _mm256_xor_si256( c7, roundkeys ); \
\
    do \
    { \
        roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
        keyPtr ++; \
        c0 = _mm256_aesenc_epi128( c0, roundkeys ); \
        c1 = _mm256_aesenc_epi128( c1, roundkeys ); \
        c2 = _mm256_aesenc_epi128( c2, roundkeys ); \
        c3 = _mm256_aesenc_epi128( c3, roundkeys ); \
        c4 = _mm256_aesenc_epi128( c4, roundkeys ); \
        c5 = _mm256_aesenc_epi128( c5, roundkeys ); \
        c6 = _mm256_aesenc_epi128( c6, roundkeys ); \
        c7 = _mm256_aesenc_epi128( c7, roundkeys ); \
    } while( keyPtr < keyLimit ); \
\
    roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
\
    c0 = _mm256_aesenclast_epi128( c0, roundkeys ); \
    c1 = _mm256_aesenclast_epi128( c1, roundkeys ); \
    c2 = _mm256_aesenclast_epi128( c2, roundkeys ); \
    c3 = _mm256_aesenclast_epi128( c3, roundkeys ); \
    c4 = _mm256_aesenclast_epi128( c4, roundkeys ); \
    c5 = _mm256_aesenclast_epi128( c5, roundkeys ); \
    c6 = _mm256_aesenclast_epi128( c6, roundkeys ); \
    c7 = _mm256_aesenclast_epi128( c7, roundkeys ); \
};

#define AES_DECRYPT_YMM_2048( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m256i roundkeys; \
\
    keyPtr = pExpandedKey->lastEncRoundKey; \
    keyLimit = pExpandedKey->lastDecRoundKey; \
\
    /* _mm256_broadcastsi128_si256 requires AVX2 */ \
    roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
    keyPtr ++; \
\
    /* _mm256_xor_si256 requires AVX2 */ \
    c0 = _mm256_xor_si256( c0, roundkeys ); \
    c1 = _mm256_xor_si256( c1, roundkeys ); \
    c2 = _mm256_xor_si256( c2, roundkeys ); \
    c3 = _mm256_xor_si256( c3, roundkeys ); \
    c4 = _mm256_xor_si256( c4, roundkeys ); \
    c5 = _mm256_xor_si256( c5, roundkeys ); \
    c6 = _mm256_xor_si256( c6, roundkeys ); \
    c7 = _mm256_xor_si256( c7, roundkeys ); \
\
    do \
    { \
        roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
        keyPtr ++; \
        c0 = _mm256_aesdec_epi128( c0, roundkeys ); \
        c1 = _mm256_aesdec_epi128( c1, roundkeys ); \
        c2 = _mm256_aesdec_epi128( c2, roundkeys ); \
        c3 = _mm256_aesdec_epi128( c3, roundkeys ); \
        c4 = _mm256_aesdec_epi128( c4, roundkeys ); \
        c5 = _mm256_aesdec_epi128( c5, roundkeys ); \
        c6 = _mm256_aesdec_epi128( c6, roundkeys ); \
        c7 = _mm256_aesdec_epi128( c7, roundkeys ); \
    } while( keyPtr < keyLimit ); \
\
    roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
\
    c0 = _mm256_aesdeclast_epi128( c0, roundkeys ); \
    c1 = _mm256_aesdeclast_epi128( c1, roundkeys ); \
    c2 = _mm256_aesdeclast_epi128( c2, roundkeys ); \
    c3 = _mm256_aesdeclast_epi128( c3, roundkeys ); \
    c4 = _mm256_aesdeclast_epi128( c4, roundkeys ); \
    c5 = _mm256_aesdeclast_epi128( c5, roundkeys ); \
    c6 = _mm256_aesdeclast_epi128( c6, roundkeys ); \
    c7 = _mm256_aesdeclast_epi128( c7, roundkeys ); \
};

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitYmm_2048(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i t0, t1, t2, t3, t4, t5, t6, t7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m128i XTS_ALPHA_MASK;
    __m256i XTS_ALPHA_MULTIPLIER_Ymm;

    // Load tweaks into big T
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;

    SIZE_T cbDataMain;  // number of bytes to handle in the main loop
    SIZE_T cbDataTail;  // number of bytes to handle in the tail loop

    // To simplify logic and unusual size processing, we handle all
    // data not a multiple of 16 blocks in the tail loop
    cbDataTail = cbData & ((16*SYMCRYPT_AES_BLOCK_SIZE)-1);
    // Additionally, so that ciphertext stealing logic does not rely on
    // reading back from the destination buffer, when we have a non-zero
    // tail, we ensure that we handle at least 1 whole block in the tail
    cbDataTail += ((cbDataTail > 0) && (cbDataTail < SYMCRYPT_AES_BLOCK_SIZE)) ? (16*SYMCRYPT_AES_BLOCK_SIZE) : 0;
    cbDataMain = cbData - cbDataTail;

    SYMCRYPT_ASSERT(cbDataMain <= cbData);
    SYMCRYPT_ASSERT(cbDataTail <= cbData);
    SYMCRYPT_ASSERT((cbDataMain & ((16*SYMCRYPT_AES_BLOCK_SIZE)-1)) == 0);

    if( cbDataMain == 0 )
    {
        SymCryptXtsAesEncryptDataUnitXmm( pExpandedKey, pbTweakBlock, pbScratch, pbSrc, pbDst, cbDataTail );
        return;
    }

    t0 = _mm_loadu_si128( (__m128i *) pbTweakBlock );
    XTS_ALPHA_MASK = _mm_set_epi32( 1, 1, 1, 0x87 );
    XTS_ALPHA_MULTIPLIER_Ymm = _mm256_set_epi64x( 0, 0x87, 0, 0x87 );

    // Do not stall.
    XTS_MUL_ALPHA4( t0, t4 );
    XTS_MUL_ALPHA ( t0, t1 );
    XTS_MUL_ALPHA ( t4, t5 );
    XTS_MUL_ALPHA ( t1, t2 );
    XTS_MUL_ALPHA ( t5, t6 );
    XTS_MUL_ALPHA ( t2, t3 );
    XTS_MUL_ALPHA ( t6, t7 );

    T0 = _mm256_insertf128_si256( _mm256_castsi128_si256( t0 ), t1, 1 ); // AVX
    T1 = _mm256_insertf128_si256( _mm256_castsi128_si256( t2 ), t3, 1 );
    T2 = _mm256_insertf128_si256( _mm256_castsi128_si256( t4 ), t5, 1 );
    T3 = _mm256_insertf128_si256( _mm256_castsi128_si256( t6 ), t7, 1 );
    XTS_MUL_ALPHA8_YMM(T0, T4);
    XTS_MUL_ALPHA8_YMM(T1, T5);
    XTS_MUL_ALPHA8_YMM(T2, T6);
    XTS_MUL_ALPHA8_YMM(T3, T7);

    for(;;)
    {
        c0 = _mm256_xor_si256( T0, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +                           0 ) ) );
        c1 = _mm256_xor_si256( T1, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +   2*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c2 = _mm256_xor_si256( T2, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +   4*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c3 = _mm256_xor_si256( T3, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +   6*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c4 = _mm256_xor_si256( T4, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +   8*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c5 = _mm256_xor_si256( T5, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +  10*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c6 = _mm256_xor_si256( T6, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +  12*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c7 = _mm256_xor_si256( T7, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +  14*SYMCRYPT_AES_BLOCK_SIZE ) ) );

        pbSrc += 16 * SYMCRYPT_AES_BLOCK_SIZE;

        AES_ENCRYPT_YMM_2048( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        _mm256_storeu_si256( ( __m256i * ) ( pbDst +                          0 ), _mm256_xor_si256( c0, T0 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst +  2*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c1, T1 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst +  4*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c2, T2 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst +  6*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c3, T3 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst +  8*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c4, T4 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst + 10*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c5, T5 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst + 12*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c6, T6 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst + 14*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c7, T7 ) );

        pbDst += 16 * SYMCRYPT_AES_BLOCK_SIZE;

        cbDataMain -= 16 * SYMCRYPT_AES_BLOCK_SIZE;
        if( cbDataMain < 16 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            break;
        }

        XTS_MUL_ALPHA16_YMM(T0, T0);
        XTS_MUL_ALPHA16_YMM(T1, T1);
        XTS_MUL_ALPHA16_YMM(T2, T2);
        XTS_MUL_ALPHA16_YMM(T3, T3);
        XTS_MUL_ALPHA16_YMM(T4, T4);
        XTS_MUL_ALPHA16_YMM(T5, T5);
        XTS_MUL_ALPHA16_YMM(T6, T6);
        XTS_MUL_ALPHA16_YMM(T7, T7);
    }

    // We won't do another 16-block set so we don't update the tweak blocks

    if( cbDataTail > 0 )
    {
        //
        // This is a rare case: the data unit length is not a multiple of 256 bytes.
        // We do this in the Xmm implementation.
        // Fix up the tweak block first
        //
        t7 = _mm256_extracti128_si256 ( T7, 1 /* Highest 128 bits */ ); // AVX2
        _mm256_zeroupper();
        XTS_MUL_ALPHA( t7, t0 );
        _mm_storeu_si128( (__m128i *) pbTweakBlock, t0 );

        SymCryptXtsAesEncryptDataUnitXmm( pExpandedKey, pbTweakBlock, pbScratch, pbSrc, pbDst, cbDataTail );
    }
    else {
        _mm256_zeroupper();
    }
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitYmm_2048(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i t0, t1, t2, t3, t4, t5, t6, t7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m128i XTS_ALPHA_MASK;
    __m256i XTS_ALPHA_MULTIPLIER_Ymm;

    // Load tweaks into big T
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;

    SIZE_T cbDataMain;  // number of bytes to handle in the main loop
    SIZE_T cbDataTail;  // number of bytes to handle in the tail loop

    // To simplify logic and unusual size processing, we handle all
    // data not a multiple of 16 blocks in the tail loop
    cbDataTail = cbData & ((16*SYMCRYPT_AES_BLOCK_SIZE)-1);
    // Additionally, so that ciphertext stealing logic does not rely on
    // reading back from the destination buffer, when we have a non-zero
    // tail, we ensure that we handle at least 1 whole block in the tail
    cbDataTail += ((cbDataTail > 0) && (cbDataTail < SYMCRYPT_AES_BLOCK_SIZE)) ? (16*SYMCRYPT_AES_BLOCK_SIZE) : 0;
    cbDataMain = cbData - cbDataTail;

    SYMCRYPT_ASSERT(cbDataMain <= cbData);
    SYMCRYPT_ASSERT(cbDataTail <= cbData);
    SYMCRYPT_ASSERT((cbDataMain & ((16*SYMCRYPT_AES_BLOCK_SIZE)-1)) == 0);

    if( cbDataMain == 0 )
    {
        SymCryptXtsAesDecryptDataUnitXmm( pExpandedKey, pbTweakBlock, pbScratch, pbSrc, pbDst, cbDataTail );
        return;
    }

    t0 = _mm_loadu_si128( (__m128i *) pbTweakBlock );
    XTS_ALPHA_MASK = _mm_set_epi32( 1, 1, 1, 0x87 );
    XTS_ALPHA_MULTIPLIER_Ymm = _mm256_set_epi64x( 0, 0x87, 0, 0x87 );

    // Do not stall.
    XTS_MUL_ALPHA4( t0, t4 );
    XTS_MUL_ALPHA ( t0, t1 );
    XTS_MUL_ALPHA ( t4, t5 );
    XTS_MUL_ALPHA ( t1, t2 );
    XTS_MUL_ALPHA ( t5, t6 );
    XTS_MUL_ALPHA ( t2, t3 );
    XTS_MUL_ALPHA ( t6, t7 );

    T0 = _mm256_insertf128_si256( _mm256_castsi128_si256( t0 ), t1, 1); // AVX
    T1 = _mm256_insertf128_si256( _mm256_castsi128_si256( t2 ), t3, 1);
    T2 = _mm256_insertf128_si256( _mm256_castsi128_si256( t4 ), t5, 1);
    T3 = _mm256_insertf128_si256( _mm256_castsi128_si256( t6 ), t7, 1);
    XTS_MUL_ALPHA8_YMM(T0, T4);
    XTS_MUL_ALPHA8_YMM(T1, T5);
    XTS_MUL_ALPHA8_YMM(T2, T6);
    XTS_MUL_ALPHA8_YMM(T3, T7);

    for(;;)
    {
        c0 = _mm256_xor_si256( T0, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +                           0 ) ) );
        c1 = _mm256_xor_si256( T1, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +   2*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c2 = _mm256_xor_si256( T2, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +   4*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c3 = _mm256_xor_si256( T3, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +   6*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c4 = _mm256_xor_si256( T4, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +   8*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c5 = _mm256_xor_si256( T5, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +  10*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c6 = _mm256_xor_si256( T6, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +  12*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c7 = _mm256_xor_si256( T7, _mm256_loadu_si256( ( __m256i * ) ( pbSrc +  14*SYMCRYPT_AES_BLOCK_SIZE ) ) );

        pbSrc += 16 * SYMCRYPT_AES_BLOCK_SIZE;

        AES_DECRYPT_YMM_2048( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        _mm256_storeu_si256( ( __m256i * ) ( pbDst +                          0 ), _mm256_xor_si256( c0, T0 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst +  2*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c1, T1 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst +  4*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c2, T2 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst +  6*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c3, T3 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst +  8*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c4, T4 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst + 10*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c5, T5 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst + 12*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c6, T6 ) );
        _mm256_storeu_si256( ( __m256i * ) ( pbDst + 14*SYMCRYPT_AES_BLOCK_SIZE ), _mm256_xor_si256( c7, T7 ) );

        pbDst += 16 * SYMCRYPT_AES_BLOCK_SIZE;

        cbDataMain -= 16 * SYMCRYPT_AES_BLOCK_SIZE;
        if( cbDataMain < 16 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            break;
        }

        XTS_MUL_ALPHA16_YMM(T0, T0);
        XTS_MUL_ALPHA16_YMM(T1, T1);
        XTS_MUL_ALPHA16_YMM(T2, T2);
        XTS_MUL_ALPHA16_YMM(T3, T3);
        XTS_MUL_ALPHA16_YMM(T4, T4);
        XTS_MUL_ALPHA16_YMM(T5, T5);
        XTS_MUL_ALPHA16_YMM(T6, T6);
        XTS_MUL_ALPHA16_YMM(T7, T7);
    }

    // We won't do another 16-block set so we don't update the tweak blocks

    if( cbDataTail > 0 )
    {
        //
        // This is a rare case: the data unit length is not a multiple of 256 bytes.
        // We do this in the Xmm implementation.
        // Fix up the tweak block first
        //
        t7 = _mm256_extracti128_si256 ( T7, 1 /* Highest 128 bits */ ); // AVX2
        _mm256_zeroupper();
        XTS_MUL_ALPHA( t7, t0 );
        _mm_storeu_si128( (__m128i *) pbTweakBlock, t0 );

        SymCryptXtsAesDecryptDataUnitXmm( pExpandedKey, pbTweakBlock, pbScratch, pbSrc, pbDst, cbDataTail );
    }
    else {
        _mm256_zeroupper();
    }
}

#define AES_FULLROUND_16_GHASH_2_Ymm( roundkeys, keyPtr, c0, c1, c2, c3, c4, c5, c6, c7, r0, t0, t1, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
    keyPtr ++; \
    c0 = _mm256_aesenc_epi128( c0, roundkeys ); \
    c1 = _mm256_aesenc_epi128( c1, roundkeys ); \
    c2 = _mm256_aesenc_epi128( c2, roundkeys ); \
    c3 = _mm256_aesenc_epi128( c3, roundkeys ); \
    c4 = _mm256_aesenc_epi128( c4, roundkeys ); \
    c5 = _mm256_aesenc_epi128( c5, roundkeys ); \
    c6 = _mm256_aesenc_epi128( c6, roundkeys ); \
    c7 = _mm256_aesenc_epi128( c7, roundkeys ); \
\
    r0 = _mm256_loadu_si256( (__m256i *) gHashPointer ); \
    r0 = _mm256_shuffle_epi8( r0, byteReverseOrder ); \
    gHashPointer += 32; \
\
    t1 = _mm256_loadu_si256( (__m256i *) &GHASH_H_POWER(gHashExpandedKeyTable, todo) ); \
    t0 = _mm256_clmulepi64_epi128( r0, t1, 0x00 ); \
    t1 = _mm256_clmulepi64_epi128( r0, t1, 0x11 ); \
\
    resl = _mm256_xor_si256( resl, t0 ); \
    resh = _mm256_xor_si256( resh, t1 ); \
\
    t0 = _mm256_srli_si256( r0, 8 ); \
    r0 = _mm256_xor_si256( r0, t0 ); \
    t1 = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(gHashExpandedKeyTable, todo) ); \
    t1 = _mm256_clmulepi64_epi128( r0, t1, 0x00 ); \
\
    resm = _mm256_xor_si256( resm, t1 ); \
    todo -= 2; \
};

#define AES_GCM_ENCRYPT_16_Ymm( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m256i roundkeys; \
    __m256i t0, t1; \
    __m256i r0; \
    int aesEncryptGhashLoop; \
\
    keyPtr = pExpandedKey->RoundKey; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    /* _mm256_broadcastsi128_si256 requires AVX2 */ \
    roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
    keyPtr ++; \
\
    /* _mm256_xor_si256 requires AVX2 */ \
    c0 = _mm256_xor_si256( c0, roundkeys ); \
    c1 = _mm256_xor_si256( c1, roundkeys ); \
    c2 = _mm256_xor_si256( c2, roundkeys ); \
    c3 = _mm256_xor_si256( c3, roundkeys ); \
    c4 = _mm256_xor_si256( c4, roundkeys ); \
    c5 = _mm256_xor_si256( c5, roundkeys ); \
    c6 = _mm256_xor_si256( c6, roundkeys ); \
    c7 = _mm256_xor_si256( c7, roundkeys ); \
\
    /* Do 8(x2) full rounds (AES-128|AES-192|AES-256) with stitched GHASH */ \
    for( aesEncryptGhashLoop = 0; aesEncryptGhashLoop < 4; aesEncryptGhashLoop++ ) \
    { \
        AES_FULLROUND_16_GHASH_2_Ymm( roundkeys, keyPtr, c0, c1, c2, c3, c4, c5, c6, c7, r0, t0, t1, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ); \
        AES_FULLROUND_16_GHASH_2_Ymm( roundkeys, keyPtr, c0, c1, c2, c3, c4, c5, c6, c7, r0, t0, t1, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ); \
    } \
\
    do \
    { \
        roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
        keyPtr ++; \
        c0 = _mm256_aesenc_epi128( c0, roundkeys ); \
        c1 = _mm256_aesenc_epi128( c1, roundkeys ); \
        c2 = _mm256_aesenc_epi128( c2, roundkeys ); \
        c3 = _mm256_aesenc_epi128( c3, roundkeys ); \
        c4 = _mm256_aesenc_epi128( c4, roundkeys ); \
        c5 = _mm256_aesenc_epi128( c5, roundkeys ); \
        c6 = _mm256_aesenc_epi128( c6, roundkeys ); \
        c7 = _mm256_aesenc_epi128( c7, roundkeys ); \
    } while( keyPtr < keyLimit ); \
\
    roundkeys =  _mm256_broadcastsi128_si256( *( (const __m128i *) keyPtr ) ); \
\
    c0 = _mm256_aesenclast_epi128( c0, roundkeys ); \
    c1 = _mm256_aesenclast_epi128( c1, roundkeys ); \
    c2 = _mm256_aesenclast_epi128( c2, roundkeys ); \
    c3 = _mm256_aesenclast_epi128( c3, roundkeys ); \
    c4 = _mm256_aesenclast_epi128( c4, roundkeys ); \
    c5 = _mm256_aesenclast_epi128( c5, roundkeys ); \
    c6 = _mm256_aesenclast_epi128( c6, roundkeys ); \
    c7 = _mm256_aesenclast_epi128( c7, roundkeys ); \
};

VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptStitchedYmm_2048(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __m128i chain = _mm_loadu_si128( (__m128i *) pbChainingValue );

    __m128i BYTE_REVERSE_ORDER_xmm = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
    __m256i BYTE_REVERSE_ORDER = _mm256_set_epi64x( 0x0001020304050607, 0x08090a0b0c0d0e0f, 0x0001020304050607, 0x08090a0b0c0d0e0f );
    __m128i vMultiplicationConstant = _mm_set_epi32( 0, 0, 0xc2000000, 0 );

    __m256i chainIncrementUpper1  = _mm256_set_epi64x( 0,  1, 0,  0 );
    __m256i chainIncrement2  = _mm256_set_epi64x( 0,  2, 0,  2 );
    __m256i chainIncrement4  = _mm256_set_epi64x( 0,  4, 0,  4 );
    __m256i chainIncrement16 = _mm256_set_epi64x( 0, 16, 0, 16 );

    __m256i ctr0, ctr1, ctr2, ctr3, ctr4, ctr5, ctr6, ctr7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m256i r0, r1, r2, r3, r4, r5, r6, r7;
    __m256i Hi, Hix;

    __m128i state;
    __m128i a0_xmm, a1_xmm, a2_xmm;
    __m256i a0, a1, a2;
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    SIZE_T todo;
    PCBYTE pbGhashSrc = pbDst;

    SYMCRYPT_ASSERT( (cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == 0 ); // cbData is multiple of block size
    SYMCRYPT_ASSERT( nBlocks >= GCM_YMM_MINBLOCKS );

    todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS ) & ~(GCM_YMM_MINBLOCKS-1);
    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER_xmm );

    state = _mm_loadu_si128( (__m128i *) pState );
    ctr0 = _mm256_insertf128_si256( _mm256_castsi128_si256( chain ), chain, 1); // AVX
    ctr0 = _mm256_add_epi32( ctr0, chainIncrementUpper1 );
    ctr1 = _mm256_add_epi32( ctr0, chainIncrement2 );
    ctr2 = _mm256_add_epi32( ctr0, chainIncrement4 );
    ctr3 = _mm256_add_epi32( ctr1, chainIncrement4 );
    ctr4 = _mm256_add_epi32( ctr2, chainIncrement4 );
    ctr5 = _mm256_add_epi32( ctr3, chainIncrement4 );
    ctr6 = _mm256_add_epi32( ctr4, chainIncrement4 );
    ctr7 = _mm256_add_epi32( ctr5, chainIncrement4 );

    CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0_xmm, a1_xmm, a2_xmm );
    a0 = a1 = a2 = _mm256_setzero_si256();

    c0 = _mm256_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );
    c1 = _mm256_shuffle_epi8( ctr1, BYTE_REVERSE_ORDER );
    c2 = _mm256_shuffle_epi8( ctr2, BYTE_REVERSE_ORDER );
    c3 = _mm256_shuffle_epi8( ctr3, BYTE_REVERSE_ORDER );
    c4 = _mm256_shuffle_epi8( ctr4, BYTE_REVERSE_ORDER );
    c5 = _mm256_shuffle_epi8( ctr5, BYTE_REVERSE_ORDER );
    c6 = _mm256_shuffle_epi8( ctr6, BYTE_REVERSE_ORDER );
    c7 = _mm256_shuffle_epi8( ctr7, BYTE_REVERSE_ORDER );

    ctr0 = _mm256_add_epi32( ctr0, chainIncrement16 );
    ctr1 = _mm256_add_epi32( ctr1, chainIncrement16 );
    ctr2 = _mm256_add_epi32( ctr2, chainIncrement16 );
    ctr3 = _mm256_add_epi32( ctr3, chainIncrement16 );
    ctr4 = _mm256_add_epi32( ctr4, chainIncrement16 );
    ctr5 = _mm256_add_epi32( ctr5, chainIncrement16 );
    ctr6 = _mm256_add_epi32( ctr6, chainIncrement16 );
    ctr7 = _mm256_add_epi32( ctr7, chainIncrement16 );

    AES_ENCRYPT_YMM_2048( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

    _mm256_storeu_si256( (__m256i *) (pbDst +  0), _mm256_xor_si256( c0, _mm256_loadu_si256( ( __m256i * ) (pbSrc +  0) ) ) );
    _mm256_storeu_si256( (__m256i *) (pbDst + 32), _mm256_xor_si256( c1, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 32) ) ) );
    _mm256_storeu_si256( (__m256i *) (pbDst + 64), _mm256_xor_si256( c2, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 64) ) ) );
    _mm256_storeu_si256( (__m256i *) (pbDst + 96), _mm256_xor_si256( c3, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 96) ) ) );
    _mm256_storeu_si256( (__m256i *) (pbDst +128), _mm256_xor_si256( c4, _mm256_loadu_si256( ( __m256i * ) (pbSrc +128) ) ) );
    _mm256_storeu_si256( (__m256i *) (pbDst +160), _mm256_xor_si256( c5, _mm256_loadu_si256( ( __m256i * ) (pbSrc +160) ) ) );
    _mm256_storeu_si256( (__m256i *) (pbDst +192), _mm256_xor_si256( c6, _mm256_loadu_si256( ( __m256i * ) (pbSrc +192) ) ) );
    _mm256_storeu_si256( (__m256i *) (pbDst +224), _mm256_xor_si256( c7, _mm256_loadu_si256( ( __m256i * ) (pbSrc +224) ) ) );

    pbDst  += 16 * SYMCRYPT_AES_BLOCK_SIZE;
    pbSrc  += 16 * SYMCRYPT_AES_BLOCK_SIZE;

    while( nBlocks >= 2*GCM_YMM_MINBLOCKS )
    {
        c0 = _mm256_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );
        c1 = _mm256_shuffle_epi8( ctr1, BYTE_REVERSE_ORDER );
        c2 = _mm256_shuffle_epi8( ctr2, BYTE_REVERSE_ORDER );
        c3 = _mm256_shuffle_epi8( ctr3, BYTE_REVERSE_ORDER );
        c4 = _mm256_shuffle_epi8( ctr4, BYTE_REVERSE_ORDER );
        c5 = _mm256_shuffle_epi8( ctr5, BYTE_REVERSE_ORDER );
        c6 = _mm256_shuffle_epi8( ctr6, BYTE_REVERSE_ORDER );
        c7 = _mm256_shuffle_epi8( ctr7, BYTE_REVERSE_ORDER );

        ctr0 = _mm256_add_epi32( ctr0, chainIncrement16 );
        ctr1 = _mm256_add_epi32( ctr1, chainIncrement16 );
        ctr2 = _mm256_add_epi32( ctr2, chainIncrement16 );
        ctr3 = _mm256_add_epi32( ctr3, chainIncrement16 );
        ctr4 = _mm256_add_epi32( ctr4, chainIncrement16 );
        ctr5 = _mm256_add_epi32( ctr5, chainIncrement16 );
        ctr6 = _mm256_add_epi32( ctr6, chainIncrement16 );
        ctr7 = _mm256_add_epi32( ctr7, chainIncrement16 );

        AES_GCM_ENCRYPT_16_Ymm( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pbGhashSrc, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );

        _mm256_storeu_si256( (__m256i *) (pbDst +  0), _mm256_xor_si256( c0, _mm256_loadu_si256( ( __m256i * ) (pbSrc +  0) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst + 32), _mm256_xor_si256( c1, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 32) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst + 64), _mm256_xor_si256( c2, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 64) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst + 96), _mm256_xor_si256( c3, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 96) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst +128), _mm256_xor_si256( c4, _mm256_loadu_si256( ( __m256i * ) (pbSrc +128) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst +160), _mm256_xor_si256( c5, _mm256_loadu_si256( ( __m256i * ) (pbSrc +160) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst +192), _mm256_xor_si256( c6, _mm256_loadu_si256( ( __m256i * ) (pbSrc +192) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst +224), _mm256_xor_si256( c7, _mm256_loadu_si256( ( __m256i * ) (pbSrc +224) ) ) );

        pbDst  += 16 * SYMCRYPT_AES_BLOCK_SIZE;
        pbSrc  += 16 * SYMCRYPT_AES_BLOCK_SIZE;
        nBlocks -= 16;

        if ( todo == 0 )
        {
            a0_xmm = _mm_xor_si128( a0_xmm, _mm256_extracti128_si256 ( a0, 0 /* Lowest 128 bits */ ));
            a1_xmm = _mm_xor_si128( a1_xmm, _mm256_extracti128_si256 ( a1, 0 /* Lowest 128 bits */ ));
            a2_xmm = _mm_xor_si128( a2_xmm, _mm256_extracti128_si256 ( a2, 0 /* Lowest 128 bits */ ));

            a0_xmm = _mm_xor_si128( a0_xmm, _mm256_extracti128_si256 ( a0, 1 /* Highest 128 bits */ ));
            a1_xmm = _mm_xor_si128( a1_xmm, _mm256_extracti128_si256 ( a1, 1 /* Highest 128 bits */ ));
            a2_xmm = _mm_xor_si128( a2_xmm, _mm256_extracti128_si256 ( a2, 1 /* Highest 128 bits */ ));
            CLMUL_3_POST( a0_xmm, a1_xmm, a2_xmm );
            MODREDUCE( vMultiplicationConstant, a0_xmm, a1_xmm, a2_xmm, state );

            todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS ) & ~(GCM_YMM_MINBLOCKS-1);
            CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0_xmm, a1_xmm, a2_xmm );
            a0 = a1 = a2 = _mm256_setzero_si256();
        }
    }

    r0 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) (pbGhashSrc +  0) ), BYTE_REVERSE_ORDER );
    r1 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) (pbGhashSrc + 32) ), BYTE_REVERSE_ORDER );
    r2 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) (pbGhashSrc + 64) ), BYTE_REVERSE_ORDER );
    r3 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) (pbGhashSrc + 96) ), BYTE_REVERSE_ORDER );
    r4 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) (pbGhashSrc +128) ), BYTE_REVERSE_ORDER );
    r5 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) (pbGhashSrc +160) ), BYTE_REVERSE_ORDER );
    r6 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) (pbGhashSrc +192) ), BYTE_REVERSE_ORDER );
    r7 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) (pbGhashSrc +224) ), BYTE_REVERSE_ORDER );

    Hi  = _mm256_loadu_si256( (__m256i *)  &GHASH_H_POWER(expandedKeyTable, todo - 0) );
    Hix = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 0) );
    CLMUL_ACC_3_Ymm( r0, Hi, Hix, a0, a1, a2 );
    Hi  = _mm256_loadu_si256( (__m256i *)  &GHASH_H_POWER(expandedKeyTable, todo - 2) );
    Hix = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 2) );
    CLMUL_ACC_3_Ymm( r1, Hi, Hix, a0, a1, a2 );
    Hi  = _mm256_loadu_si256( (__m256i *)  &GHASH_H_POWER(expandedKeyTable, todo - 4) );
    Hix = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 4) );
    CLMUL_ACC_3_Ymm( r2, Hi, Hix, a0, a1, a2 );
    Hi  = _mm256_loadu_si256( (__m256i *)  &GHASH_H_POWER(expandedKeyTable, todo - 6) );
    Hix = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 6) );
    CLMUL_ACC_3_Ymm( r3, Hi, Hix, a0, a1, a2 );
    Hi  = _mm256_loadu_si256( (__m256i *)  &GHASH_H_POWER(expandedKeyTable, todo - 8) );
    Hix = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 8) );
    CLMUL_ACC_3_Ymm( r4, Hi, Hix, a0, a1, a2 );
    Hi  = _mm256_loadu_si256( (__m256i *)  &GHASH_H_POWER(expandedKeyTable, todo -10) );
    Hix = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(expandedKeyTable, todo -10) );
    CLMUL_ACC_3_Ymm( r5, Hi, Hix, a0, a1, a2 );
    Hi  = _mm256_loadu_si256( (__m256i *)  &GHASH_H_POWER(expandedKeyTable, todo -12) );
    Hix = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(expandedKeyTable, todo -12) );
    CLMUL_ACC_3_Ymm( r6, Hi, Hix, a0, a1, a2 );
    Hi  = _mm256_loadu_si256( (__m256i *)  &GHASH_H_POWER(expandedKeyTable, todo -14) );
    Hix = _mm256_loadu_si256( (__m256i *) &GHASH_Hx_POWER(expandedKeyTable, todo -14) );
    CLMUL_ACC_3_Ymm( r7, Hi, Hix, a0, a1, a2 );

    a0_xmm = _mm_xor_si128( a0_xmm, _mm256_extracti128_si256 ( a0, 0 /* Lowest 128 bits */ ));
    a1_xmm = _mm_xor_si128( a1_xmm, _mm256_extracti128_si256 ( a1, 0 /* Lowest 128 bits */ ));
    a2_xmm = _mm_xor_si128( a2_xmm, _mm256_extracti128_si256 ( a2, 0 /* Lowest 128 bits */ ));

    a0_xmm = _mm_xor_si128( a0_xmm, _mm256_extracti128_si256 ( a0, 1 /* Highest 128 bits */ ));
    a1_xmm = _mm_xor_si128( a1_xmm, _mm256_extracti128_si256 ( a1, 1 /* Highest 128 bits */ ));
    a2_xmm = _mm_xor_si128( a2_xmm, _mm256_extracti128_si256 ( a2, 1 /* Highest 128 bits */ ));
    CLMUL_3_POST( a0_xmm, a1_xmm, a2_xmm );
    MODREDUCE( vMultiplicationConstant, a0_xmm, a1_xmm, a2_xmm, state );

    chain = _mm256_extracti128_si256 ( ctr0, 0 /* Lowest 128 bits */ );
    _mm256_zeroupper();

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER_xmm );
    _mm_storeu_si128((__m128i *) pbChainingValue, chain );
    _mm_storeu_si128((__m128i *) pState, state );

    cbData &= ( GCM_YMM_MINBLOCKS*SYMCRYPT_AES_BLOCK_SIZE ) - 1;
    SYMCRYPT_ASSERT( cbData == (nBlocks-16)*SYMCRYPT_AES_BLOCK_SIZE );
    if ( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptAesGcmEncryptStitchedXmm( pExpandedKey, pbChainingValue, expandedKeyTable, pState, pbSrc, pbDst, cbData);
    }
}

VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptStitchedYmm_2048(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __m128i chain = _mm_loadu_si128( (__m128i *) pbChainingValue );

    __m128i BYTE_REVERSE_ORDER_xmm = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
    __m256i BYTE_REVERSE_ORDER = _mm256_set_epi64x( 0x0001020304050607, 0x08090a0b0c0d0e0f, 0x0001020304050607, 0x08090a0b0c0d0e0f );
    __m128i vMultiplicationConstant = _mm_set_epi32( 0, 0, 0xc2000000, 0 );

    __m256i chainIncrementUpper1  = _mm256_set_epi64x( 0,  1, 0,  0 );
    __m256i chainIncrement2  = _mm256_set_epi64x( 0,  2, 0,  2 );
    __m256i chainIncrement4  = _mm256_set_epi64x( 0,  4, 0,  4 );
    __m256i chainIncrement16 = _mm256_set_epi64x( 0, 16, 0, 16 );

    __m256i ctr0, ctr1, ctr2, ctr3, ctr4, ctr5, ctr6, ctr7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;

    __m128i state;
    __m128i a0_xmm, a1_xmm, a2_xmm;
    __m256i a0, a1, a2;
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    SIZE_T todo;
    PCBYTE pbGhashSrc = pbSrc;

    SYMCRYPT_ASSERT( (cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == 0 ); // cbData is multiple of block size
    SYMCRYPT_ASSERT( nBlocks >= GCM_YMM_MINBLOCKS );

    todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS ) & ~(GCM_YMM_MINBLOCKS-1);
    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER_xmm );

    state = _mm_loadu_si128( (__m128i *) pState );
    ctr0 = _mm256_insertf128_si256( _mm256_castsi128_si256( chain ), chain, 1); // AVX
    ctr0 = _mm256_add_epi32( ctr0, chainIncrementUpper1 );
    ctr1 = _mm256_add_epi32( ctr0, chainIncrement2 );
    ctr2 = _mm256_add_epi32( ctr0, chainIncrement4 );
    ctr3 = _mm256_add_epi32( ctr1, chainIncrement4 );
    ctr4 = _mm256_add_epi32( ctr2, chainIncrement4 );
    ctr5 = _mm256_add_epi32( ctr3, chainIncrement4 );
    ctr6 = _mm256_add_epi32( ctr4, chainIncrement4 );
    ctr7 = _mm256_add_epi32( ctr5, chainIncrement4 );

    CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0_xmm, a1_xmm, a2_xmm );
    a0 = a1 = a2 = _mm256_setzero_si256();

    while( nBlocks >= GCM_YMM_MINBLOCKS )
    {
        c0 = _mm256_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );
        c1 = _mm256_shuffle_epi8( ctr1, BYTE_REVERSE_ORDER );
        c2 = _mm256_shuffle_epi8( ctr2, BYTE_REVERSE_ORDER );
        c3 = _mm256_shuffle_epi8( ctr3, BYTE_REVERSE_ORDER );
        c4 = _mm256_shuffle_epi8( ctr4, BYTE_REVERSE_ORDER );
        c5 = _mm256_shuffle_epi8( ctr5, BYTE_REVERSE_ORDER );
        c6 = _mm256_shuffle_epi8( ctr6, BYTE_REVERSE_ORDER );
        c7 = _mm256_shuffle_epi8( ctr7, BYTE_REVERSE_ORDER );

        ctr0 = _mm256_add_epi32( ctr0, chainIncrement16 );
        ctr1 = _mm256_add_epi32( ctr1, chainIncrement16 );
        ctr2 = _mm256_add_epi32( ctr2, chainIncrement16 );
        ctr3 = _mm256_add_epi32( ctr3, chainIncrement16 );
        ctr4 = _mm256_add_epi32( ctr4, chainIncrement16 );
        ctr5 = _mm256_add_epi32( ctr5, chainIncrement16 );
        ctr6 = _mm256_add_epi32( ctr6, chainIncrement16 );
        ctr7 = _mm256_add_epi32( ctr7, chainIncrement16 );

        AES_GCM_ENCRYPT_16_Ymm( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pbGhashSrc, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );

        _mm256_storeu_si256( (__m256i *) (pbDst +  0), _mm256_xor_si256( c0, _mm256_loadu_si256( ( __m256i * ) (pbSrc +  0) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst + 32), _mm256_xor_si256( c1, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 32) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst + 64), _mm256_xor_si256( c2, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 64) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst + 96), _mm256_xor_si256( c3, _mm256_loadu_si256( ( __m256i * ) (pbSrc + 96) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst +128), _mm256_xor_si256( c4, _mm256_loadu_si256( ( __m256i * ) (pbSrc +128) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst +160), _mm256_xor_si256( c5, _mm256_loadu_si256( ( __m256i * ) (pbSrc +160) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst +192), _mm256_xor_si256( c6, _mm256_loadu_si256( ( __m256i * ) (pbSrc +192) ) ) );
        _mm256_storeu_si256( (__m256i *) (pbDst +224), _mm256_xor_si256( c7, _mm256_loadu_si256( ( __m256i * ) (pbSrc +224) ) ) );

        pbDst  += 16 * SYMCRYPT_AES_BLOCK_SIZE;
        pbSrc  += 16 * SYMCRYPT_AES_BLOCK_SIZE;
        nBlocks -= 16;

        if ( todo == 0 )
        {
            a0_xmm = _mm_xor_si128( a0_xmm, _mm256_extracti128_si256 ( a0, 0 /* Lowest 128 bits */ ));
            a1_xmm = _mm_xor_si128( a1_xmm, _mm256_extracti128_si256 ( a1, 0 /* Lowest 128 bits */ ));
            a2_xmm = _mm_xor_si128( a2_xmm, _mm256_extracti128_si256 ( a2, 0 /* Lowest 128 bits */ ));

            a0_xmm = _mm_xor_si128( a0_xmm, _mm256_extracti128_si256 ( a0, 1 /* Highest 128 bits */ ));
            a1_xmm = _mm_xor_si128( a1_xmm, _mm256_extracti128_si256 ( a1, 1 /* Highest 128 bits */ ));
            a2_xmm = _mm_xor_si128( a2_xmm, _mm256_extracti128_si256 ( a2, 1 /* Highest 128 bits */ ));
            CLMUL_3_POST( a0_xmm, a1_xmm, a2_xmm );
            MODREDUCE( vMultiplicationConstant, a0_xmm, a1_xmm, a2_xmm, state );

            if ( nBlocks > 0 )
            {
                todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS ) & ~(GCM_YMM_MINBLOCKS-1);
                CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0_xmm, a1_xmm, a2_xmm );
                a0 = a1 = a2 = _mm256_setzero_si256();
            }
        }
    }

    chain = _mm256_extracti128_si256 ( ctr0, 0 /* Lowest 128 bits */ );
    _mm256_zeroupper();

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER_xmm );
    _mm_storeu_si128((__m128i *) pbChainingValue, chain );
    _mm_storeu_si128((__m128i *) pState, state );

    cbData &= ( GCM_YMM_MINBLOCKS*SYMCRYPT_AES_BLOCK_SIZE ) - 1;
    SYMCRYPT_ASSERT( cbData == nBlocks*SYMCRYPT_AES_BLOCK_SIZE );
    if ( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        SymCryptAesGcmDecryptStitchedXmm( pExpandedKey, pbChainingValue, expandedKeyTable, pState, pbSrc, pbDst, cbData);
    }
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif // CPU_X86 | CPU_AMD64
