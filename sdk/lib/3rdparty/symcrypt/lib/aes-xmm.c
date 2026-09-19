//
// aes-xmm.c   code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// All XMM code for AES operations
// Requires compiler support for ssse3, aesni and pclmulqdq
//

#include "precomp.h"

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#include "xtsaes_definitions.h"
#include "ghash_definitions.h"

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("ssse3,aes,pclmul"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("ssse3,aes,pclmul")
#endif

VOID
SYMCRYPT_CALL
SymCryptAes4SboxXmm( _In_reads_(4) PCBYTE pIn, _Out_writes_(4) PBYTE pOut )
{
    __m128i x;
    x = _mm_set1_epi32( *(int *) pIn );

    x = _mm_aeskeygenassist_si128( x, 0 );

    // Could use _mm_storeu_si32( pOut, x ) but it is missing from some headers and _mm_store_ss will be as fast
    _mm_store_ss( (float *) pOut, _mm_castsi128_ps(x) );
}

VOID
SYMCRYPT_CALL
SymCryptAesCreateDecryptionRoundKeyXmm(
    _In_reads_(16)      PCBYTE  pEncryptionRoundKey,
    _Out_writes_(16)    PBYTE   pDecryptionRoundKey )
{
    //
    // On x86 our key structure is only 4-aligned (the best we can do) so we use unaligned load/stores.
    // On Amd64 our round keys are aligned, but recent CPUs have fast unaligned load/store if the address is
    // actually aligned properly.
    //
    _mm_storeu_si128( (__m128i *) pDecryptionRoundKey, _mm_aesimc_si128( _mm_loadu_si128( (__m128i *)pEncryptionRoundKey ) ) );
}

//
// The latency of AES instruction has increased up to 8 cycles in Ivy Bridge,
// and back to 7 in Haswell.
// We use 8-parallel code to expose the maximum parallelism to the CPU.
// On x86 it will introduce some register spilling, but the load/stores
// should be able to hide behind the AES instruction latencies.
// Silvermont x86 CPUs has AES-NI with latency = 8 and throughput = 5, so there
// the CPU parallelism is low.
// For things like BitLocker that is fine, but other uses, such as GCM & AES_CTR_DRBG
// use odd sizes.
// We try to do 5-8 blocks in 8-parallel code, 2-4 blocks in 4-parallel code, and
// 1 block in 1-parallel code.
// This is a compromise; the big cores can do 8 parallel in about the time of a 4-parallel,
// but Silvermont cannot and would pay a big price on small requests if we only use 8-parallel.
// Doing only 8-parallel and then 1-parallel would penalize the big cores a lot.
//
// We used to have 7-parallel code, but common request sizes are not multiples of 7
// blocks so we end up doing a lot of extra work. This is especially expensive on
// Silvermont where the extra work isn't hidden in the latencies.
//

#define AES_ENCRYPT_1( pExpandedKey, c0 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
\
    keyPtr = &pExpandedKey->RoundKey[0]; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
\
    c0 = _mm_xor_si128( c0, roundkey ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
    c0 = _mm_aesenc_si128( c0, roundkey ); \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesenc_si128( c0, roundkey ); \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesenc_si128( c0, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
\
    c0 = _mm_aesenclast_si128( c0, roundkey ); \
};


// Perform AES encryption without the first round key and with a specified last round key
//
// For algorithms where performance is dominated by a chain of dependent AES rounds (i.e. CBC encryption, CCM, CMAC)
// we can gain a reasonable performance uplift by computing (last round key ^ next plaintext block ^ first round key)
// off the critical path and using this computed value in place of last round key in AESENCLAST instructions.
#define AES_ENCRYPT_1_CHAIN( pExpandedKey, cipherState, mergedLastRoundKey ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
\
    keyPtr = &pExpandedKey->RoundKey[1]; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
\
    cipherState = _mm_aesenc_si128( cipherState, roundkey ); \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        cipherState = _mm_aesenc_si128( cipherState, roundkey ); \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        cipherState = _mm_aesenc_si128( cipherState, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    cipherState = _mm_aesenclast_si128( cipherState, mergedLastRoundKey ); \
};

#define AES_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
\
    keyPtr = &pExpandedKey->RoundKey[0]; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
\
    c0 = _mm_xor_si128( c0, roundkey ); \
    c1 = _mm_xor_si128( c1, roundkey ); \
    c2 = _mm_xor_si128( c2, roundkey ); \
    c3 = _mm_xor_si128( c3, roundkey ); \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesenc_si128( c0, roundkey ); \
        c1 = _mm_aesenc_si128( c1, roundkey ); \
        c2 = _mm_aesenc_si128( c2, roundkey ); \
        c3 = _mm_aesenc_si128( c3, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
\
    c0 = _mm_aesenclast_si128( c0, roundkey ); \
    c1 = _mm_aesenclast_si128( c1, roundkey ); \
    c2 = _mm_aesenclast_si128( c2, roundkey ); \
    c3 = _mm_aesenclast_si128( c3, roundkey ); \
};

#define AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
\
    keyPtr = &pExpandedKey->RoundKey[0]; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
\
    c0 = _mm_xor_si128( c0, roundkey ); \
    c1 = _mm_xor_si128( c1, roundkey ); \
    c2 = _mm_xor_si128( c2, roundkey ); \
    c3 = _mm_xor_si128( c3, roundkey ); \
    c4 = _mm_xor_si128( c4, roundkey ); \
    c5 = _mm_xor_si128( c5, roundkey ); \
    c6 = _mm_xor_si128( c6, roundkey ); \
    c7 = _mm_xor_si128( c7, roundkey ); \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesenc_si128( c0, roundkey ); \
        c1 = _mm_aesenc_si128( c1, roundkey ); \
        c2 = _mm_aesenc_si128( c2, roundkey ); \
        c3 = _mm_aesenc_si128( c3, roundkey ); \
        c4 = _mm_aesenc_si128( c4, roundkey ); \
        c5 = _mm_aesenc_si128( c5, roundkey ); \
        c6 = _mm_aesenc_si128( c6, roundkey ); \
        c7 = _mm_aesenc_si128( c7, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
\
    c0 = _mm_aesenclast_si128( c0, roundkey ); \
    c1 = _mm_aesenclast_si128( c1, roundkey ); \
    c2 = _mm_aesenclast_si128( c2, roundkey ); \
    c3 = _mm_aesenclast_si128( c3, roundkey ); \
    c4 = _mm_aesenclast_si128( c4, roundkey ); \
    c5 = _mm_aesenclast_si128( c5, roundkey ); \
    c6 = _mm_aesenclast_si128( c6, roundkey ); \
    c7 = _mm_aesenclast_si128( c7, roundkey ); \
};

#define AES_DECRYPT_1( pExpandedKey, c0 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
\
    keyPtr = pExpandedKey->lastEncRoundKey; \
    keyLimit = pExpandedKey->lastDecRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
\
    c0 = _mm_xor_si128( c0, roundkey ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
    c0 = _mm_aesdec_si128( c0, roundkey ); \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesdec_si128( c0, roundkey ); \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesdec_si128( c0, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
\
    c0 = _mm_aesdeclast_si128( c0, roundkey ); \
};

#define AES_DECRYPT_4( pExpandedKey, c0, c1, c2, c3 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
\
    keyPtr = pExpandedKey->lastEncRoundKey; \
    keyLimit = pExpandedKey->lastDecRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
\
    c0 = _mm_xor_si128( c0, roundkey ); \
    c1 = _mm_xor_si128( c1, roundkey ); \
    c2 = _mm_xor_si128( c2, roundkey ); \
    c3 = _mm_xor_si128( c3, roundkey ); \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesdec_si128( c0, roundkey ); \
        c1 = _mm_aesdec_si128( c1, roundkey ); \
        c2 = _mm_aesdec_si128( c2, roundkey ); \
        c3 = _mm_aesdec_si128( c3, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
\
    c0 = _mm_aesdeclast_si128( c0, roundkey ); \
    c1 = _mm_aesdeclast_si128( c1, roundkey ); \
    c2 = _mm_aesdeclast_si128( c2, roundkey ); \
    c3 = _mm_aesdeclast_si128( c3, roundkey ); \
};

#define AES_DECRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
\
    keyPtr = pExpandedKey->lastEncRoundKey; \
    keyLimit = pExpandedKey->lastDecRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
\
    c0 = _mm_xor_si128( c0, roundkey ); \
    c1 = _mm_xor_si128( c1, roundkey ); \
    c2 = _mm_xor_si128( c2, roundkey ); \
    c3 = _mm_xor_si128( c3, roundkey ); \
    c4 = _mm_xor_si128( c4, roundkey ); \
    c5 = _mm_xor_si128( c5, roundkey ); \
    c6 = _mm_xor_si128( c6, roundkey ); \
    c7 = _mm_xor_si128( c7, roundkey ); \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesdec_si128( c0, roundkey ); \
        c1 = _mm_aesdec_si128( c1, roundkey ); \
        c2 = _mm_aesdec_si128( c2, roundkey ); \
        c3 = _mm_aesdec_si128( c3, roundkey ); \
        c4 = _mm_aesdec_si128( c4, roundkey ); \
        c5 = _mm_aesdec_si128( c5, roundkey ); \
        c6 = _mm_aesdec_si128( c6, roundkey ); \
        c7 = _mm_aesdec_si128( c7, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
\
    c0 = _mm_aesdeclast_si128( c0, roundkey ); \
    c1 = _mm_aesdeclast_si128( c1, roundkey ); \
    c2 = _mm_aesdeclast_si128( c2, roundkey ); \
    c3 = _mm_aesdeclast_si128( c3, roundkey ); \
    c4 = _mm_aesdeclast_si128( c4, roundkey ); \
    c5 = _mm_aesdeclast_si128( c5, roundkey ); \
    c6 = _mm_aesdeclast_si128( c6, roundkey ); \
    c7 = _mm_aesdeclast_si128( c7, roundkey ); \
};


//
// The EncryptXmm code is tested through the CFB mode encryption which has no further optimizations.
//
VOID
SYMCRYPT_CALL
SymCryptAesEncryptXmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst )
{
    __m128i c;

    c = _mm_loadu_si128( ( __m128i * ) pbSrc);

    AES_ENCRYPT_1( pExpandedKey, c );

    _mm_storeu_si128( (__m128i *) pbDst, c );
}

//
// The DecryptXmm code is tested through the EcbDecrypt calls which has no further optimizations.
//
VOID
SYMCRYPT_CALL
SymCryptAesDecryptXmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst )
{
    __m128i c;

    c = _mm_loadu_si128( ( __m128i * ) pbSrc);

    AES_DECRYPT_1( pExpandedKey, c );

    _mm_storeu_si128( (__m128i *) pbDst, c );
}

// Disable warnings and VC++ runtime checks for use of uninitialized values (by design)
#pragma warning(push)
#pragma warning( disable: 6001 4701 )
#pragma runtime_checks( "u", off )
VOID
SYMCRYPT_CALL
SymCryptAesEcbEncryptXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i c0, c1, c2, c3, c4, c5, c6, c7;

    while( cbData >= 8 * SYMCRYPT_AES_BLOCK_SIZE )
    {
        c0 = _mm_loadu_si128( ( __m128i * ) (pbSrc +  0 ));
        c1 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 16 ));
        c2 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 32 ));
        c3 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 48 ));
        c4 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 64 ));
        c5 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 80 ));
        c6 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 96 ));
        c7 = _mm_loadu_si128( ( __m128i * ) (pbSrc +112 ));

        AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        _mm_storeu_si128( (__m128i *) (pbDst +  0 ), c0 );
        _mm_storeu_si128( (__m128i *) (pbDst + 16 ), c1 );
        _mm_storeu_si128( (__m128i *) (pbDst + 32 ), c2 );
        _mm_storeu_si128( (__m128i *) (pbDst + 48 ), c3 );
        _mm_storeu_si128( (__m128i *) (pbDst + 64 ), c4 );
        _mm_storeu_si128( (__m128i *) (pbDst + 80 ), c5 );
        _mm_storeu_si128( (__m128i *) (pbDst + 96 ), c6 );
        _mm_storeu_si128( (__m128i *) (pbDst +112 ), c7 );

        pbSrc   += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        pbDst   += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        cbData  -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbData < 16 )
    {
        return;
    }

    c0 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 0 ));
    if( cbData >= 32 )
    {
    c1 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 16 ));
        if( cbData >= 48 )
        {
    c2 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 32 ));
            if( cbData >= 64 )
            {
    c3 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 48 ));
                if( cbData >= 80 )
                {
    c4 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 64 ));
                    if( cbData >= 96 )
                    {
    c5 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 80 ));
                        if( cbData >= 112 )
                        {
    c6 = _mm_loadu_si128( ( __m128i * ) (pbSrc + 96 ));
                        }
                    }
                }
            }
        }
    }

    if( cbData >= 5 * SYMCRYPT_AES_BLOCK_SIZE )
    {
        AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );
    }
    else if( cbData >= 2 * SYMCRYPT_AES_BLOCK_SIZE )
    {
        AES_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3 );
    }
    else
    {
        AES_ENCRYPT_1( pExpandedKey, c0 );
    }

    _mm_storeu_si128( (__m128i *) (pbDst + 0  ), c0 );
    if( cbData >= 32 )
    {
    _mm_storeu_si128( (__m128i *) (pbDst + 16 ), c1 );
        if( cbData >= 48 )
        {
    _mm_storeu_si128( (__m128i *) (pbDst + 32 ), c2 );
            if( cbData >= 64 )
            {
    _mm_storeu_si128( (__m128i *) (pbDst + 48 ), c3 );
                if( cbData >= 80 )
                {
    _mm_storeu_si128( (__m128i *) (pbDst + 64 ), c4 );
                    if( cbData >= 96 )
                    {
    _mm_storeu_si128( (__m128i *) (pbDst + 80 ), c5 );
                        if( cbData >= 112 )
                        {
    _mm_storeu_si128( (__m128i *) (pbDst + 96 ), c6 );
                        }
                    }
                }
            }
        }
    }
}
#pragma runtime_checks( "u", restore )
#pragma warning( pop )



VOID
SYMCRYPT_CALL
SymCryptAesCbcEncryptXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i c = _mm_loadu_si128( (__m128i *) pbChainingValue );
    __m128i rk0 = _mm_loadu_si128( (__m128i *) &pExpandedKey->RoundKey[0] );
    __m128i rkLast = _mm_loadu_si128( (__m128i *) pExpandedKey->lastEncRoundKey );
    __m128i d;

    if (cbData < SYMCRYPT_AES_BLOCK_SIZE)
        return;

    // This algorithm is dominated by chain of dependent AES rounds, so we want to avoid XOR
    // instructions on the critical path where possible
    // We can compute (last round key ^ next plaintext block ^ first round key) off the critical
    // path and use this with AES_ENCRYPT_1_CHAIN so that only AES instructions write to c in
    // the main loop
    d = _mm_xor_si128( _mm_loadu_si128( (__m128i *) pbSrc ), rk0 );
    c = _mm_xor_si128( c, d );
    pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
    cbData -= SYMCRYPT_AES_BLOCK_SIZE;

    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        d = _mm_xor_si128( _mm_loadu_si128( (__m128i *) pbSrc ), rk0 );
        AES_ENCRYPT_1_CHAIN( pExpandedKey, c, _mm_xor_si128(d, rkLast ) );
        _mm_storeu_si128( (__m128i *) pbDst, _mm_xor_si128(c, d) );

        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
    AES_ENCRYPT_1_CHAIN( pExpandedKey, c, rkLast );
    _mm_storeu_si128( (__m128i *) pbDst, c );
    _mm_storeu_si128( (__m128i *) pbChainingValue, c );
}

// Disable warnings and VC++ runtime checks for use of uninitialized values (by design)
#pragma warning(push)
#pragma warning( disable: 6001 4701 )
#pragma runtime_checks( "u", off )
VOID
SYMCRYPT_CALL
SymCryptAesCbcDecryptXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i chain;
    __m128i c0, c1, c2, c3, c4, c5, c6, c7;
    __m128i d0, d1, d2, d3, d4, d5, d6, d7;

    if( cbData < SYMCRYPT_AES_BLOCK_SIZE )
    {
        return;
    }

    chain = _mm_loadu_si128( (__m128i *) pbChainingValue );

    //
    // First we do all multiples of 8 blocks
    //

    while( cbData >= 8 * SYMCRYPT_AES_BLOCK_SIZE )
    {
        d0 = c0 = _mm_loadu_si128( (__m128i *) (pbSrc + 0 * SYMCRYPT_AES_BLOCK_SIZE ) );
        d1 = c1 = _mm_loadu_si128( (__m128i *) (pbSrc + 1 * SYMCRYPT_AES_BLOCK_SIZE ) );
        d2 = c2 = _mm_loadu_si128( (__m128i *) (pbSrc + 2 * SYMCRYPT_AES_BLOCK_SIZE ) );
        d3 = c3 = _mm_loadu_si128( (__m128i *) (pbSrc + 3 * SYMCRYPT_AES_BLOCK_SIZE ) );
        d4 = c4 = _mm_loadu_si128( (__m128i *) (pbSrc + 4 * SYMCRYPT_AES_BLOCK_SIZE ) );
        d5 = c5 = _mm_loadu_si128( (__m128i *) (pbSrc + 5 * SYMCRYPT_AES_BLOCK_SIZE ) );
        d6 = c6 = _mm_loadu_si128( (__m128i *) (pbSrc + 6 * SYMCRYPT_AES_BLOCK_SIZE ) );
        d7 = c7 = _mm_loadu_si128( (__m128i *) (pbSrc + 7 * SYMCRYPT_AES_BLOCK_SIZE ) );

        AES_DECRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        c0 = _mm_xor_si128( c0, chain );
        c1 = _mm_xor_si128( c1, d0 );
        c2 = _mm_xor_si128( c2, d1 );
        c3 = _mm_xor_si128( c3, d2 );
        c4 = _mm_xor_si128( c4, d3 );
        c5 = _mm_xor_si128( c5, d4 );
        c6 = _mm_xor_si128( c6, d5 );
        c7 = _mm_xor_si128( c7, d6 );
        chain = d7;

        _mm_storeu_si128( (__m128i *) (pbDst + 0 * SYMCRYPT_AES_BLOCK_SIZE ), c0 );
        _mm_storeu_si128( (__m128i *) (pbDst + 1 * SYMCRYPT_AES_BLOCK_SIZE ), c1 );
        _mm_storeu_si128( (__m128i *) (pbDst + 2 * SYMCRYPT_AES_BLOCK_SIZE ), c2 );
        _mm_storeu_si128( (__m128i *) (pbDst + 3 * SYMCRYPT_AES_BLOCK_SIZE ), c3 );
        _mm_storeu_si128( (__m128i *) (pbDst + 4 * SYMCRYPT_AES_BLOCK_SIZE ), c4 );
        _mm_storeu_si128( (__m128i *) (pbDst + 5 * SYMCRYPT_AES_BLOCK_SIZE ), c5 );
        _mm_storeu_si128( (__m128i *) (pbDst + 6 * SYMCRYPT_AES_BLOCK_SIZE ), c6 );
        _mm_storeu_si128( (__m128i *) (pbDst + 7 * SYMCRYPT_AES_BLOCK_SIZE ), c7 );

        pbSrc  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        pbDst  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbData >= 16 )
    {
        //
        // There is remaining work to be done
        //
        d0 = c0 = _mm_loadu_si128( (__m128i *) (pbSrc + 0 * SYMCRYPT_AES_BLOCK_SIZE ) );
        if( cbData >= 32 )
        {
        d1 = c1 = _mm_loadu_si128( (__m128i *) (pbSrc + 1 * SYMCRYPT_AES_BLOCK_SIZE ) );
            if( cbData >= 48 )
            {
        d2 = c2 = _mm_loadu_si128( (__m128i *) (pbSrc + 2 * SYMCRYPT_AES_BLOCK_SIZE ) );
                if( cbData >= 64 )
                {
        d3 = c3 = _mm_loadu_si128( (__m128i *) (pbSrc + 3 * SYMCRYPT_AES_BLOCK_SIZE ) );
                    if( cbData >= 80 )
                    {
        d4 = c4 = _mm_loadu_si128( (__m128i *) (pbSrc + 4 * SYMCRYPT_AES_BLOCK_SIZE ) );
                        if( cbData >= 96 )
                        {
        d5 = c5 = _mm_loadu_si128( (__m128i *) (pbSrc + 5 * SYMCRYPT_AES_BLOCK_SIZE ) );
                            if( cbData >= 112 )
                            {
        d6 = c6 = _mm_loadu_si128( (__m128i *) (pbSrc + 6 * SYMCRYPT_AES_BLOCK_SIZE ) );
                            }
                        }
                    }
                }
            }
        }

        //
        // Decrypt 1, 4, or 8 blocks in AES-CBC mode. This might decrypt uninitialized registers,
        // but those will not be used when we store the results.
        //
        if( cbData > 4 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            AES_DECRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );
            c0 = _mm_xor_si128( c0, chain );
            c1 = _mm_xor_si128( c1, d0 );
            c2 = _mm_xor_si128( c2, d1 );
            c3 = _mm_xor_si128( c3, d2 );
            c4 = _mm_xor_si128( c4, d3 );
            c5 = _mm_xor_si128( c5, d4 );
            c6 = _mm_xor_si128( c6, d5 );
        }
        else if( cbData > SYMCRYPT_AES_BLOCK_SIZE )
        {
            AES_DECRYPT_4( pExpandedKey, c0, c1, c2, c3 );
            c0 = _mm_xor_si128( c0, chain );
            c1 = _mm_xor_si128( c1, d0 );
            c2 = _mm_xor_si128( c2, d1 );
            c3 = _mm_xor_si128( c3, d2 );
        } else
        {
            AES_DECRYPT_1( pExpandedKey, c0 );
            c0 = _mm_xor_si128( c0, chain );
        }

        chain = _mm_loadu_si128( (__m128i *) (pbSrc + cbData - SYMCRYPT_AES_BLOCK_SIZE ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 0 * SYMCRYPT_AES_BLOCK_SIZE ), c0 );
        if( cbData >= 32 )
        {
        _mm_storeu_si128( (__m128i *) (pbDst + 1 * SYMCRYPT_AES_BLOCK_SIZE ), c1 );
            if( cbData >= 48 )
            {
        _mm_storeu_si128( (__m128i *) (pbDst + 2 * SYMCRYPT_AES_BLOCK_SIZE ), c2 );
                if( cbData >= 64 )
                {
        _mm_storeu_si128( (__m128i *) (pbDst + 3 * SYMCRYPT_AES_BLOCK_SIZE ), c3 );
                    if( cbData >= 80 )
                    {
        _mm_storeu_si128( (__m128i *) (pbDst + 4 * SYMCRYPT_AES_BLOCK_SIZE ), c4 );
                        if( cbData >= 96 )
                        {
        _mm_storeu_si128( (__m128i *) (pbDst + 5 * SYMCRYPT_AES_BLOCK_SIZE ), c5 );
                            if( cbData >= 112 )
                            {
        _mm_storeu_si128( (__m128i *) (pbDst + 6 * SYMCRYPT_AES_BLOCK_SIZE ), c6 );
                            }
                        }
                    }
                }
            }
        }
    }

    _mm_storeu_si128( (__m128i *) pbChainingValue, chain );

    return;
}
#pragma runtime_checks( "u", restore )
#pragma warning( pop )

VOID
SYMCRYPT_CALL
SymCryptAesCbcMacXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData )
{
    __m128i c = _mm_loadu_si128( (__m128i *) pbChainingValue );
    __m128i rk0 = _mm_loadu_si128( (__m128i *) &pExpandedKey->RoundKey[0] );
    __m128i rkLast = _mm_loadu_si128( (__m128i *) pExpandedKey->lastEncRoundKey );
    __m128i d, rk0AndLast;

    if (cbData < SYMCRYPT_AES_BLOCK_SIZE)
        return;

    // This algorithm is dominated by chain of dependent AES rounds, so we want to avoid XOR
    // instructions on the critical path where possible
    // We can compute (last round key ^ next plaintext block ^ first round key) off the critical
    // path and use this with AES_ENCRYPT_1_CHAIN so that only AES instructions write to c in
    // the main loop
    d = _mm_xor_si128( _mm_loadu_si128( (__m128i *) pbData ), rk0 );
    c = _mm_xor_si128( c, d );
    pbData += SYMCRYPT_AES_BLOCK_SIZE;
    cbData -= SYMCRYPT_AES_BLOCK_SIZE;

    // As we don't compute ciphertext here, we only need to XOR rk0 and rkLast once
    rk0AndLast = _mm_xor_si128( rk0, rkLast );

    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        d = _mm_xor_si128( _mm_loadu_si128( (__m128i *) pbData ), rk0AndLast );
        AES_ENCRYPT_1_CHAIN( pExpandedKey, c, d );

        pbData += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
    AES_ENCRYPT_1_CHAIN( pExpandedKey, c, rkLast );
    _mm_storeu_si128( (__m128i *) pbChainingValue, c );
}


#pragma warning(push)
#pragma warning( disable:4701 ) // "Use of uninitialized variable"
#pragma runtime_checks( "u", off )

#define SYMCRYPT_AesCtrMsbXxXmm     SymCryptAesCtrMsb64Xmm
#define MM_ADD_EPIXX               _mm_add_epi64
#define MM_SUB_EPIXX               _mm_sub_epi64

#include "aes-pattern.c"

#undef MM_SUB_EPIXX
#undef MM_ADD_EPIXX
#undef SYMCRYPT_AesCtrMsbXxXmm

#define SYMCRYPT_AesCtrMsbXxXmm     SymCryptAesCtrMsb32Xmm
#define MM_ADD_EPIXX               _mm_add_epi32
#define MM_SUB_EPIXX               _mm_sub_epi32

#include "aes-pattern.c"

#undef MM_SUB_EPIXX
#undef MM_ADD_EPIXX
#undef SYMCRYPT_AesCtrMsbXxXmm

#pragma runtime_checks( "u", restore )
#pragma warning(pop)

/*
    if( cbData >= 16 )
    {
        if( cbData >= 32 )
        {
            if( cbData >= 48 )
            {
                if( cbData >= 64 )
                {
                    if( cbData >= 80 )
                    {
                        if( cbData >= 96 )
                        {
                            if( cbData >= 112 )
                            {
                            }
                        }
                    }
                }
            }
        }
    }
*/

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )       PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i t0;
    __m128i c0, c1, c2, c3, c4, c5, c6, c7;
    __m128i roundkey, firstRoundKey, lastRoundKey;
    __m128i XTS_ALPHA_MASK = _mm_set_epi32( 1, 1, 1, 0x87 );
    SYMCRYPT_GF128_ELEMENT* tweakBuffer = (SYMCRYPT_GF128_ELEMENT*) pbScratch;

    const BYTE (*keyPtr)[4][4];
    const BYTE (*keyLimit)[4][4] = pExpandedKey->lastEncRoundKey;
    UINT64 lastTweakLow, lastTweakHigh;
    int aesEncryptXtsLoop;

    SIZE_T cbDataMain;  // number of bytes to handle in the main loop
    SIZE_T cbDataTail;  // number of bytes to handle in the tail loop

    SYMCRYPT_ASSERT(cbData >= SYMCRYPT_AES_BLOCK_SIZE);

    // To simplify logic and unusual size processing, we handle all
    // data not a multiple of 8 blocks in the tail loop
    cbDataTail = cbData & ((8*SYMCRYPT_AES_BLOCK_SIZE)-1);
    // Additionally, so that ciphertext stealing logic does not rely on
    // reading back from the destination buffer, when we have a non-zero
    // tail, we ensure that we handle at least 1 whole block in the tail
    //
    // Note that our caller has ensured we have at least 1 whole block
    // to process, this is checked in debug build
    // This means that cbDataTail is in [1,15] at this point iff there are
    // at least 8 whole blocks to process; so the below does not cause
    // cbDataTail or cbDataMain to exceed cbData
    cbDataTail += ((cbDataTail > 0) && (cbDataTail < SYMCRYPT_AES_BLOCK_SIZE)) ? (8*SYMCRYPT_AES_BLOCK_SIZE) : 0;
    cbDataMain = cbData - cbDataTail;

    SYMCRYPT_ASSERT(cbDataMain <= cbData);
    SYMCRYPT_ASSERT(cbDataTail <= cbData);
    SYMCRYPT_ASSERT((cbDataMain & ((8*SYMCRYPT_AES_BLOCK_SIZE)-1)) == 0);

    c0 = _mm_loadu_si128( (__m128i *) pbTweakBlock );
    XTS_MUL_ALPHA( c0, c1 );
    XTS_MUL_ALPHA( c1, c2 );
    XTS_MUL_ALPHA( c2, c3 );

    XTS_MUL_ALPHA4( c0, c4 );
    XTS_MUL_ALPHA ( c4, c5 );
    XTS_MUL_ALPHA ( c5, c6 );
    XTS_MUL_ALPHA ( c6, c7 );

    tweakBuffer[0].m128i = c0;
    tweakBuffer[1].m128i = c1;
    tweakBuffer[2].m128i = c2;
    tweakBuffer[3].m128i = c3;
    tweakBuffer[4].m128i = c4;
    tweakBuffer[5].m128i = c5;
    tweakBuffer[6].m128i = c6;
    tweakBuffer[7].m128i = c7;
    lastTweakLow  = tweakBuffer[7].ull[0];
    lastTweakHigh = tweakBuffer[7].ull[1];

    firstRoundKey = _mm_loadu_si128( (__m128i *) &pExpandedKey->RoundKey[0] );
    lastRoundKey = _mm_loadu_si128( (__m128i *) pExpandedKey->lastEncRoundKey );

    while( cbDataMain > 0 )
    {
        // At loop entry, tweakBuffer[0-7] are tweakValues for the next 8 blocks
        c0 = _mm_xor_si128( tweakBuffer[0].m128i, firstRoundKey );
        c1 = _mm_xor_si128( tweakBuffer[1].m128i, firstRoundKey );
        c2 = _mm_xor_si128( tweakBuffer[2].m128i, firstRoundKey );
        c3 = _mm_xor_si128( tweakBuffer[3].m128i, firstRoundKey );
        c4 = _mm_xor_si128( tweakBuffer[4].m128i, firstRoundKey );
        c5 = _mm_xor_si128( tweakBuffer[5].m128i, firstRoundKey );
        c6 = _mm_xor_si128( tweakBuffer[6].m128i, firstRoundKey );
        c7 = _mm_xor_si128( tweakBuffer[7].m128i, firstRoundKey );

        c0 = _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +   0) ) );
        c1 = _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc +  16) ) );
        c2 = _mm_xor_si128( c2, _mm_loadu_si128( ( __m128i * ) (pbSrc +  32) ) );
        c3 = _mm_xor_si128( c3, _mm_loadu_si128( ( __m128i * ) (pbSrc +  48) ) );
        c4 = _mm_xor_si128( c4, _mm_loadu_si128( ( __m128i * ) (pbSrc +  64) ) );
        c5 = _mm_xor_si128( c5, _mm_loadu_si128( ( __m128i * ) (pbSrc +  80) ) );
        c6 = _mm_xor_si128( c6, _mm_loadu_si128( ( __m128i * ) (pbSrc +  96) ) );
        c7 = _mm_xor_si128( c7, _mm_loadu_si128( ( __m128i * ) (pbSrc + 112) ) );

        keyPtr = &pExpandedKey->RoundKey[1];

        // Do 8 full rounds (AES-128|AES-192|AES-256) with stitched XTS (performed in scalar registers)
        for( aesEncryptXtsLoop = 0; aesEncryptXtsLoop < 8; aesEncryptXtsLoop++ )
        {
            roundkey = _mm_loadu_si128( (__m128i *) keyPtr );
            keyPtr ++;
            c0 = _mm_aesenc_si128( c0, roundkey );
            c1 = _mm_aesenc_si128( c1, roundkey );
            c2 = _mm_aesenc_si128( c2, roundkey );
            c3 = _mm_aesenc_si128( c3, roundkey );
            c4 = _mm_aesenc_si128( c4, roundkey );
            c5 = _mm_aesenc_si128( c5, roundkey );
            c6 = _mm_aesenc_si128( c6, roundkey );
            c7 = _mm_aesenc_si128( c7, roundkey );

            // Prepare tweakBuffer[8-15] with tweak^lastRoundKey
            tweakBuffer[ 8+aesEncryptXtsLoop ].m128i = _mm_xor_si128( tweakBuffer[ aesEncryptXtsLoop ].m128i, lastRoundKey );
            // Prepare tweakBuffer[0-7] with tweaks for next 8 blocks
            XTS_MUL_ALPHA_Scalar( lastTweakLow, lastTweakHigh );
            tweakBuffer[ aesEncryptXtsLoop ].ull[0] = lastTweakLow;
            tweakBuffer[ aesEncryptXtsLoop ].ull[1] = lastTweakHigh;
        }

        do
        {
            roundkey = _mm_loadu_si128( (__m128i *) keyPtr );
            keyPtr ++;
            c0 = _mm_aesenc_si128( c0, roundkey );
            c1 = _mm_aesenc_si128( c1, roundkey );
            c2 = _mm_aesenc_si128( c2, roundkey );
            c3 = _mm_aesenc_si128( c3, roundkey );
            c4 = _mm_aesenc_si128( c4, roundkey );
            c5 = _mm_aesenc_si128( c5, roundkey );
            c6 = _mm_aesenc_si128( c6, roundkey );
            c7 = _mm_aesenc_si128( c7, roundkey );
        } while( keyPtr < keyLimit );

        _mm_storeu_si128( (__m128i *) (pbDst +   0), _mm_aesenclast_si128( c0, tweakBuffer[ 8].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  16), _mm_aesenclast_si128( c1, tweakBuffer[ 9].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  32), _mm_aesenclast_si128( c2, tweakBuffer[10].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  48), _mm_aesenclast_si128( c3, tweakBuffer[11].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  64), _mm_aesenclast_si128( c4, tweakBuffer[12].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  80), _mm_aesenclast_si128( c5, tweakBuffer[13].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  96), _mm_aesenclast_si128( c6, tweakBuffer[14].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 112), _mm_aesenclast_si128( c7, tweakBuffer[15].m128i ) );

        pbSrc += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        cbDataMain -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbDataTail == 0 )
    {
        return; // <-- expected case; early return here
    }

    // Rare case, with data unit length not being multiple of 128 bytes, handle the tail one block at a time
    t0 = tweakBuffer[0].m128i;

    while( cbDataTail >= 2*SYMCRYPT_AES_BLOCK_SIZE )
    {
        c0 = _mm_xor_si128( _mm_loadu_si128( ( __m128i * ) pbSrc ), t0 );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        AES_ENCRYPT_1( pExpandedKey, c0 );
        _mm_storeu_si128( (__m128i *) pbDst, _mm_xor_si128( c0, t0 ) );
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        XTS_MUL_ALPHA( t0, t0 );
        cbDataTail -= SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbDataTail > SYMCRYPT_AES_BLOCK_SIZE )
    {
        // Ciphertext stealing encryption
        //
        //                      +--------------+
        //                      |              |
        //                      |              V
        // +-----------------+  |  +-----+-----------+
        // |      P_m-1      |  |  | P_m |++++CP+++++|
        // +-----------------+  |  +-----+-----------+
        //          |           |           |
        //       enc_m-1        |         enc_m
        //          |           |           |
        //          V           |           V
        // +-----+-----------+  |  +-----------------+
        // | C_m |++++CP+++++|--+  |      C_m-1      |
        // +-----+-----------+     +-----------------+
        //    |                   /
        //    +----------------  /  --+
        //                      /     |
        //                      |     V
        // +-----------------+  |  +-----+
        // |      C_m-1      |<-+  | C_m |
        // +-----------------+     +-----+

        // Encrypt penultimate plaintext block into tweakBuffer[0]
        c0 = _mm_xor_si128( _mm_loadu_si128( (__m128i *) pbSrc ), t0 );
        AES_ENCRYPT_1( pExpandedKey, c0 );
        tweakBuffer[0].m128i = _mm_xor_si128( c0, t0 );

        cbDataTail -= SYMCRYPT_AES_BLOCK_SIZE;

        // Copy tweakBuffer[0] to tweakBuffer[1]
        tweakBuffer[1].m128i = tweakBuffer[0].m128i;
        // Copy final plaintext bytes to prefix of tweakBuffer[0] - we must read before writing to support in-place encryption
        memcpy( &tweakBuffer[0].ul[0], pbSrc + SYMCRYPT_AES_BLOCK_SIZE, cbDataTail );
        // Copy prefix of tweakBuffer[1] to the right place in the destination buffer
        memcpy( pbDst + SYMCRYPT_AES_BLOCK_SIZE, &tweakBuffer[1].ul[0], cbDataTail );

        // Do final tweak update
        XTS_MUL_ALPHA( t0, t0 );

        // Load updated tweakBuffer[0] into c0
        c0 = tweakBuffer[0].m128i;
    } else {
        // Just load final plaintext block into c0
        c0 = _mm_loadu_si128( (__m128i*) pbSrc );
    }

    // Final full block encryption
    c0 = _mm_xor_si128( c0, t0 );
    AES_ENCRYPT_1( pExpandedKey, c0 );
    _mm_storeu_si128( (__m128i *) pbDst, _mm_xor_si128( c0, t0 ) );
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )       PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i t0;
    __m128i c0, c1, c2, c3, c4, c5, c6, c7;
    __m128i roundkey, firstRoundKey, lastRoundKey;
    __m128i XTS_ALPHA_MASK = _mm_set_epi32( 1, 1, 1, 0x87 );
    SYMCRYPT_GF128_ELEMENT* tweakBuffer = (SYMCRYPT_GF128_ELEMENT*) pbScratch;

    const BYTE (*keyPtr)[4][4];
    const BYTE (*keyLimit)[4][4] = pExpandedKey->lastDecRoundKey;
    UINT64 lastTweakLow, lastTweakHigh;
    int aesDecryptXtsLoop;

    SIZE_T cbDataMain;  // number of bytes to handle in the main loop
    SIZE_T cbDataTail;  // number of bytes to handle in the tail loop

    SYMCRYPT_ASSERT(cbData >= SYMCRYPT_AES_BLOCK_SIZE);

    // To simplify logic and unusual size processing, we handle all
    // data not a multiple of 8 blocks in the tail loop
    cbDataTail = cbData & ((8*SYMCRYPT_AES_BLOCK_SIZE)-1);
    // Additionally, so that ciphertext stealing logic does not rely on
    // reading back from the destination buffer, when we have a non-zero
    // tail, we ensure that we handle at least 1 whole block in the tail
    //
    // Note that our caller has ensured we have at least 1 whole block
    // to process, this is checked in debug build
    // This means that cbDataTail is in [1,15] at this point iff there are
    // at least 8 whole blocks to process; so the below does not cause
    // cbDataTail or cbDataMain to exceed cbData
    cbDataTail += ((cbDataTail > 0) && (cbDataTail < SYMCRYPT_AES_BLOCK_SIZE)) ? (8*SYMCRYPT_AES_BLOCK_SIZE) : 0;
    cbDataMain = cbData - cbDataTail;

    SYMCRYPT_ASSERT(cbDataMain <= cbData);
    SYMCRYPT_ASSERT(cbDataTail <= cbData);
    SYMCRYPT_ASSERT((cbDataMain & ((8*SYMCRYPT_AES_BLOCK_SIZE)-1)) == 0);

    c0 = _mm_loadu_si128( (__m128i *) pbTweakBlock );
    XTS_MUL_ALPHA( c0, c1 );
    XTS_MUL_ALPHA( c1, c2 );
    XTS_MUL_ALPHA( c2, c3 );

    XTS_MUL_ALPHA4( c0, c4 );
    XTS_MUL_ALPHA ( c4, c5 );
    XTS_MUL_ALPHA ( c5, c6 );
    XTS_MUL_ALPHA ( c6, c7 );

    tweakBuffer[0].m128i = c0;
    tweakBuffer[1].m128i = c1;
    tweakBuffer[2].m128i = c2;
    tweakBuffer[3].m128i = c3;
    tweakBuffer[4].m128i = c4;
    tweakBuffer[5].m128i = c5;
    tweakBuffer[6].m128i = c6;
    tweakBuffer[7].m128i = c7;
    lastTweakLow  = tweakBuffer[7].ull[0];
    lastTweakHigh = tweakBuffer[7].ull[1];

    firstRoundKey = _mm_loadu_si128( (__m128i *) pExpandedKey->lastEncRoundKey );
    lastRoundKey = _mm_loadu_si128( (__m128i *) pExpandedKey->lastDecRoundKey );

    while( cbDataMain > 0 )
    {
        // At loop entry, tweakBuffer[0-7] are tweakValues for the next 8 blocks
        c0 = _mm_xor_si128( tweakBuffer[0].m128i, firstRoundKey );
        c1 = _mm_xor_si128( tweakBuffer[1].m128i, firstRoundKey );
        c2 = _mm_xor_si128( tweakBuffer[2].m128i, firstRoundKey );
        c3 = _mm_xor_si128( tweakBuffer[3].m128i, firstRoundKey );
        c4 = _mm_xor_si128( tweakBuffer[4].m128i, firstRoundKey );
        c5 = _mm_xor_si128( tweakBuffer[5].m128i, firstRoundKey );
        c6 = _mm_xor_si128( tweakBuffer[6].m128i, firstRoundKey );
        c7 = _mm_xor_si128( tweakBuffer[7].m128i, firstRoundKey );

        c0 = _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +   0) ) );
        c1 = _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc +  16) ) );
        c2 = _mm_xor_si128( c2, _mm_loadu_si128( ( __m128i * ) (pbSrc +  32) ) );
        c3 = _mm_xor_si128( c3, _mm_loadu_si128( ( __m128i * ) (pbSrc +  48) ) );
        c4 = _mm_xor_si128( c4, _mm_loadu_si128( ( __m128i * ) (pbSrc +  64) ) );
        c5 = _mm_xor_si128( c5, _mm_loadu_si128( ( __m128i * ) (pbSrc +  80) ) );
        c6 = _mm_xor_si128( c6, _mm_loadu_si128( ( __m128i * ) (pbSrc +  96) ) );
        c7 = _mm_xor_si128( c7, _mm_loadu_si128( ( __m128i * ) (pbSrc + 112) ) );

        keyPtr = pExpandedKey->lastEncRoundKey + 1;

        // Do 8 full rounds (AES-128|AES-192|AES-256) with stitched XTS (performed in scalar registers)
        for( aesDecryptXtsLoop = 0; aesDecryptXtsLoop < 8; aesDecryptXtsLoop++ )
        {
            roundkey = _mm_loadu_si128( (__m128i *) keyPtr );
            keyPtr ++;
            c0 = _mm_aesdec_si128( c0, roundkey );
            c1 = _mm_aesdec_si128( c1, roundkey );
            c2 = _mm_aesdec_si128( c2, roundkey );
            c3 = _mm_aesdec_si128( c3, roundkey );
            c4 = _mm_aesdec_si128( c4, roundkey );
            c5 = _mm_aesdec_si128( c5, roundkey );
            c6 = _mm_aesdec_si128( c6, roundkey );
            c7 = _mm_aesdec_si128( c7, roundkey );

            // Prepare tweakBuffer[8-15] with tweak^lastRoundKey
            tweakBuffer[ 8+aesDecryptXtsLoop ].m128i = _mm_xor_si128( tweakBuffer[ aesDecryptXtsLoop ].m128i, lastRoundKey );
            // Prepare tweakBuffer[0-7] with tweaks for next 8 blocks
            XTS_MUL_ALPHA_Scalar( lastTweakLow, lastTweakHigh );
            tweakBuffer[ aesDecryptXtsLoop ].ull[0] = lastTweakLow;
            tweakBuffer[ aesDecryptXtsLoop ].ull[1] = lastTweakHigh;
        }

        do
        {
            roundkey = _mm_loadu_si128( (__m128i *) keyPtr );
            keyPtr ++;
            c0 = _mm_aesdec_si128( c0, roundkey );
            c1 = _mm_aesdec_si128( c1, roundkey );
            c2 = _mm_aesdec_si128( c2, roundkey );
            c3 = _mm_aesdec_si128( c3, roundkey );
            c4 = _mm_aesdec_si128( c4, roundkey );
            c5 = _mm_aesdec_si128( c5, roundkey );
            c6 = _mm_aesdec_si128( c6, roundkey );
            c7 = _mm_aesdec_si128( c7, roundkey );
        } while( keyPtr < keyLimit );

        _mm_storeu_si128( (__m128i *) (pbDst +   0), _mm_aesdeclast_si128( c0, tweakBuffer[ 8].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  16), _mm_aesdeclast_si128( c1, tweakBuffer[ 9].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  32), _mm_aesdeclast_si128( c2, tweakBuffer[10].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  48), _mm_aesdeclast_si128( c3, tweakBuffer[11].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  64), _mm_aesdeclast_si128( c4, tweakBuffer[12].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  80), _mm_aesdeclast_si128( c5, tweakBuffer[13].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst +  96), _mm_aesdeclast_si128( c6, tweakBuffer[14].m128i ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 112), _mm_aesdeclast_si128( c7, tweakBuffer[15].m128i ) );

        pbSrc += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        cbDataMain -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbDataTail == 0 )
    {
        return; // <-- expected case; early return here
    }

    // Rare case, with data unit length not being multiple of 128 bytes, handle the tail one block at a time
    t0 = tweakBuffer[0].m128i;

    while( cbDataTail >= 2*SYMCRYPT_AES_BLOCK_SIZE )
    {
        c0 = _mm_xor_si128( _mm_loadu_si128( ( __m128i * ) pbSrc ), t0 );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        AES_DECRYPT_1( pExpandedKey, c0 );
        _mm_storeu_si128( (__m128i *) pbDst, _mm_xor_si128( c0, t0 ) );
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        c7 = t0;
        XTS_MUL_ALPHA( t0, t0 );
        cbDataTail -= SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbDataTail > SYMCRYPT_AES_BLOCK_SIZE )
    {
        // Ciphertext stealing decryption
        //
        //                      +--------------+
        //                      |              |
        //                      |              V
        // +-----------------+  |  +-----+-----------+
        // |      C_m-1      |  |  | C_m |++++CP+++++|
        // +-----------------+  |  +-----+-----------+
        //          |           |           |
        //        dec_m         |        dec_m-1
        //          |           |           |
        //          V           |           V
        // +-----+-----------+  |  +-----------------+
        // | P_m |++++CP+++++|--+  |      P_m-1      |
        // +-----+-----------+     +-----------------+
        //    |                   /
        //    +----------------  /  --+
        //                      /     |
        //                      |     V
        // +-----------------+  |  +-----+
        // |      P_m-1      |<-+  | P_m |
        // +-----------------+     +-----+

        // Do final tweak update into c1
        // Penultimate tweak is in t0, ready for final decryption
        XTS_MUL_ALPHA( t0, c1 );

        // Decrypt penultimate ciphertext block into tweakBuffer[0]
        c0 = _mm_xor_si128( _mm_loadu_si128( (__m128i *) pbSrc ), c1 );
        AES_DECRYPT_1( pExpandedKey, c0 );
        tweakBuffer[0].m128i = _mm_xor_si128( c0, c1 );

        cbDataTail -= SYMCRYPT_AES_BLOCK_SIZE;

        // Copy tweakBuffer[0] to tweakBuffer[1]
        tweakBuffer[1].m128i = tweakBuffer[0].m128i;
        // Copy final ciphertext bytes to prefix of tweakBuffer[0] - we must read before writing to support in-place decryption
        memcpy( &tweakBuffer[0].ul[0], pbSrc + SYMCRYPT_AES_BLOCK_SIZE, cbDataTail );
        // Copy prefix of tweakBuffer[1] to the right place in the destination buffer
        memcpy( pbDst + SYMCRYPT_AES_BLOCK_SIZE, &tweakBuffer[1].ul[0], cbDataTail );

        // Load updated tweakBuffer[0] into c0
        c0 = tweakBuffer[0].m128i;
    } else {
        // Just load final ciphertext block into c0
        c0 = _mm_loadu_si128( (__m128i*) pbSrc );
    }

    // Final full block decryption
    c0 = _mm_xor_si128( c0, t0 );
    AES_DECRYPT_1( pExpandedKey, c0 );
    _mm_storeu_si128( (__m128i *) pbDst, _mm_xor_si128( c0, t0 ) );
}

#define AES_FULLROUND_4_GHASH_1( roundkey, keyPtr, c0, c1, c2, c3, r0, t0, t1, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
    c0 = _mm_aesenc_si128( c0, roundkey ); \
    c1 = _mm_aesenc_si128( c1, roundkey ); \
    c2 = _mm_aesenc_si128( c2, roundkey ); \
    c3 = _mm_aesenc_si128( c3, roundkey ); \
\
    r0 = _mm_loadu_si128( (__m128i *) gHashPointer ); \
    r0 = _mm_shuffle_epi8( r0, byteReverseOrder ); \
    gHashPointer += 16; \
\
    t1 = _mm_loadu_si128( (__m128i *) &GHASH_H_POWER(gHashExpandedKeyTable, todo) ); \
    t0 = _mm_clmulepi64_si128( r0, t1, 0x00 ); \
    t1 = _mm_clmulepi64_si128( r0, t1, 0x11 ); \
\
    resl = _mm_xor_si128( resl, t0 ); \
    resh = _mm_xor_si128( resh, t1 ); \
\
    t0 = _mm_srli_si128( r0, 8 ); \
    r0 = _mm_xor_si128( r0, t0 ); \
    t1 = _mm_loadu_si128( (__m128i *) &GHASH_Hx_POWER(gHashExpandedKeyTable, todo) ); \
    t1 = _mm_clmulepi64_si128( r0, t1, 0x00 ); \
\
    resm = _mm_xor_si128( resm, t1 ); \
    todo --; \
};

#define AES_GCM_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3, gHashPointer, ghashRounds, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
    __m128i t0, t1; \
    __m128i r0; \
    SIZE_T aesEncryptGhashLoop; \
\
    keyPtr = &pExpandedKey->RoundKey[0]; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
    c0 = _mm_xor_si128( c0, roundkey ); \
    c1 = _mm_xor_si128( c1, roundkey ); \
    c2 = _mm_xor_si128( c2, roundkey ); \
    c3 = _mm_xor_si128( c3, roundkey ); \
\
    /* Do ghashRounds full rounds (AES-128|AES-192|AES-256) with stitched GHASH */ \
    for( aesEncryptGhashLoop = 0; aesEncryptGhashLoop < ghashRounds; aesEncryptGhashLoop++ ) \
    { \
        AES_FULLROUND_4_GHASH_1( roundkey, keyPtr, c0, c1, c2, c3, r0, t0, t1, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ); \
    } \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesenc_si128( c0, roundkey ); \
        c1 = _mm_aesenc_si128( c1, roundkey ); \
        c2 = _mm_aesenc_si128( c2, roundkey ); \
        c3 = _mm_aesenc_si128( c3, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
\
    c0 = _mm_aesenclast_si128( c0, roundkey ); \
    c1 = _mm_aesenclast_si128( c1, roundkey ); \
    c2 = _mm_aesenclast_si128( c2, roundkey ); \
    c3 = _mm_aesenclast_si128( c3, roundkey ); \
};

#define AES_FULLROUND_8_GHASH_1( roundkey, keyPtr, c0, c1, c2, c3, c4, c5, c6, c7, r0, t0, t1, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
    c0 = _mm_aesenc_si128( c0, roundkey ); \
    c1 = _mm_aesenc_si128( c1, roundkey ); \
    c2 = _mm_aesenc_si128( c2, roundkey ); \
    c3 = _mm_aesenc_si128( c3, roundkey ); \
    c4 = _mm_aesenc_si128( c4, roundkey ); \
    c5 = _mm_aesenc_si128( c5, roundkey ); \
    c6 = _mm_aesenc_si128( c6, roundkey ); \
    c7 = _mm_aesenc_si128( c7, roundkey ); \
\
    r0 = _mm_loadu_si128( (__m128i *) gHashPointer ); \
    r0 = _mm_shuffle_epi8( r0, byteReverseOrder ); \
    gHashPointer += 16; \
\
    t1 = _mm_loadu_si128( (__m128i *) &GHASH_H_POWER(gHashExpandedKeyTable, todo) ); \
    t0 = _mm_clmulepi64_si128( r0, t1, 0x00 ); \
    t1 = _mm_clmulepi64_si128( r0, t1, 0x11 ); \
\
    resl = _mm_xor_si128( resl, t0 ); \
    resh = _mm_xor_si128( resh, t1 ); \
\
    t0 = _mm_srli_si128( r0, 8 ); \
    r0 = _mm_xor_si128( r0, t0 ); \
    t1 = _mm_loadu_si128( (__m128i *) &GHASH_Hx_POWER(gHashExpandedKeyTable, todo) ); \
    t1 = _mm_clmulepi64_si128( r0, t1, 0x00 ); \
\
    resm = _mm_xor_si128( resm, t1 ); \
    todo --; \
};

#define AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, gHashPointer, ghashRounds, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m128i roundkey; \
    __m128i t0, t1; \
    __m128i r0; \
    SIZE_T aesEncryptGhashLoop; \
\
    keyPtr = &pExpandedKey->RoundKey[0]; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
    keyPtr ++; \
    c0 = _mm_xor_si128( c0, roundkey ); \
    c1 = _mm_xor_si128( c1, roundkey ); \
    c2 = _mm_xor_si128( c2, roundkey ); \
    c3 = _mm_xor_si128( c3, roundkey ); \
    c4 = _mm_xor_si128( c4, roundkey ); \
    c5 = _mm_xor_si128( c5, roundkey ); \
    c6 = _mm_xor_si128( c6, roundkey ); \
    c7 = _mm_xor_si128( c7, roundkey ); \
\
    /* Do ghashRounds full rounds (AES-128|AES-192|AES-256) with stitched GHASH */ \
    for( aesEncryptGhashLoop = 0; aesEncryptGhashLoop < ghashRounds; aesEncryptGhashLoop++ ) \
    { \
        AES_FULLROUND_8_GHASH_1( roundkey, keyPtr, c0, c1, c2, c3, c4, c5, c6, c7, r0, t0, t1, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ); \
    } \
\
    do \
    { \
        roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
        keyPtr ++; \
        c0 = _mm_aesenc_si128( c0, roundkey ); \
        c1 = _mm_aesenc_si128( c1, roundkey ); \
        c2 = _mm_aesenc_si128( c2, roundkey ); \
        c3 = _mm_aesenc_si128( c3, roundkey ); \
        c4 = _mm_aesenc_si128( c4, roundkey ); \
        c5 = _mm_aesenc_si128( c5, roundkey ); \
        c6 = _mm_aesenc_si128( c6, roundkey ); \
        c7 = _mm_aesenc_si128( c7, roundkey ); \
    } while( keyPtr < keyLimit ); \
\
    roundkey = _mm_loadu_si128( (__m128i *) keyPtr ); \
\
    c0 = _mm_aesenclast_si128( c0, roundkey ); \
    c1 = _mm_aesenclast_si128( c1, roundkey ); \
    c2 = _mm_aesenclast_si128( c2, roundkey ); \
    c3 = _mm_aesenclast_si128( c3, roundkey ); \
    c4 = _mm_aesenclast_si128( c4, roundkey ); \
    c5 = _mm_aesenclast_si128( c5, roundkey ); \
    c6 = _mm_aesenclast_si128( c6, roundkey ); \
    c7 = _mm_aesenclast_si128( c7, roundkey ); \
};

// This call is functionally identical to:
// SymCryptAesCtrMsb64Xmm( pExpandedKey,
//                         pbChainingValue,
//                         pbSrc,
//                         pbDst,
//                         cbData );
// SymCryptGHashAppendDataPclmulqdq(   expandedKeyTable,
//                                     pState,
//                                     pbDst,
//                                     cbData );
VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptStitchedXmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __m128i chain = _mm_loadu_si128( (__m128i *) pbChainingValue );

    __m128i BYTE_REVERSE_ORDER = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
    __m128i vMultiplicationConstant = _mm_set_epi32( 0, 0, 0xc2000000, 0 );

    __m128i chainIncrement1 = _mm_set_epi32( 0, 0, 0, 1 );
    __m128i chainIncrement2 = _mm_set_epi32( 0, 0, 0, 2 );
    __m128i chainIncrement8 = _mm_set_epi32( 0, 0, 0, 8 );

    __m128i c0, c1, c2, c3, c4, c5, c6, c7;
    __m128i r0, r1;

    __m128i state;
    __m128i a0, a1, a2;
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    SIZE_T todo;
    PCBYTE pbGhashSrc = pbDst;

    SYMCRYPT_ASSERT( (cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == 0 ); // cbData is multiple of block size

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER );
    state = _mm_loadu_si128( (__m128i *) pState );

    todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS );
    CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );

    // Do 8 blocks of CTR either for tail (if total blocks <8) or for encryption of first 8 blocks
    c0 = chain;
    c1 = _mm_add_epi32( chain, chainIncrement1 );
    c2 = _mm_add_epi32( chain, chainIncrement2 );
    c3 = _mm_add_epi32( c1, chainIncrement2 );
    c4 = _mm_add_epi32( c2, chainIncrement2 );
    c5 = _mm_add_epi32( c3, chainIncrement2 );
    c6 = _mm_add_epi32( c4, chainIncrement2 );
    c7 = _mm_add_epi32( c5, chainIncrement2 );

    c0 = _mm_shuffle_epi8( c0, BYTE_REVERSE_ORDER );
    c1 = _mm_shuffle_epi8( c1, BYTE_REVERSE_ORDER );
    c2 = _mm_shuffle_epi8( c2, BYTE_REVERSE_ORDER );
    c3 = _mm_shuffle_epi8( c3, BYTE_REVERSE_ORDER );
    c4 = _mm_shuffle_epi8( c4, BYTE_REVERSE_ORDER );
    c5 = _mm_shuffle_epi8( c5, BYTE_REVERSE_ORDER );
    c6 = _mm_shuffle_epi8( c6, BYTE_REVERSE_ORDER );
    c7 = _mm_shuffle_epi8( c7, BYTE_REVERSE_ORDER );

    AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

    if( nBlocks >= 8 )
    {
        // Encrypt first 8 blocks - update chain
        chain = _mm_add_epi32( chain, chainIncrement8 );

        _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 16), _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc + 16) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 32), _mm_xor_si128( c2, _mm_loadu_si128( ( __m128i * ) (pbSrc + 32) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 48), _mm_xor_si128( c3, _mm_loadu_si128( ( __m128i * ) (pbSrc + 48) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 64), _mm_xor_si128( c4, _mm_loadu_si128( ( __m128i * ) (pbSrc + 64) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 80), _mm_xor_si128( c5, _mm_loadu_si128( ( __m128i * ) (pbSrc + 80) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 96), _mm_xor_si128( c6, _mm_loadu_si128( ( __m128i * ) (pbSrc + 96) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst +112), _mm_xor_si128( c7, _mm_loadu_si128( ( __m128i * ) (pbSrc +112) ) ) );

        pbDst  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        pbSrc  += 8 * SYMCRYPT_AES_BLOCK_SIZE;

        while( nBlocks >= 16 )
        {
            // In this loop we always have 8 blocks to encrypt and we have already encrypted the previous 8 blocks ready for GHASH
            c0 = chain;
            c1 = _mm_add_epi32( chain, chainIncrement1 );
            c2 = _mm_add_epi32( chain, chainIncrement2 );
            c3 = _mm_add_epi32( c1, chainIncrement2 );
            c4 = _mm_add_epi32( c2, chainIncrement2 );
            c5 = _mm_add_epi32( c3, chainIncrement2 );
            c6 = _mm_add_epi32( c4, chainIncrement2 );
            c7 = _mm_add_epi32( c5, chainIncrement2 );
            chain = _mm_add_epi32( c6, chainIncrement2 );

            c0 = _mm_shuffle_epi8( c0, BYTE_REVERSE_ORDER );
            c1 = _mm_shuffle_epi8( c1, BYTE_REVERSE_ORDER );
            c2 = _mm_shuffle_epi8( c2, BYTE_REVERSE_ORDER );
            c3 = _mm_shuffle_epi8( c3, BYTE_REVERSE_ORDER );
            c4 = _mm_shuffle_epi8( c4, BYTE_REVERSE_ORDER );
            c5 = _mm_shuffle_epi8( c5, BYTE_REVERSE_ORDER );
            c6 = _mm_shuffle_epi8( c6, BYTE_REVERSE_ORDER );
            c7 = _mm_shuffle_epi8( c7, BYTE_REVERSE_ORDER );

            AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pbGhashSrc, 8, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );

            _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 16), _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc + 16) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 32), _mm_xor_si128( c2, _mm_loadu_si128( ( __m128i * ) (pbSrc + 32) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 48), _mm_xor_si128( c3, _mm_loadu_si128( ( __m128i * ) (pbSrc + 48) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 64), _mm_xor_si128( c4, _mm_loadu_si128( ( __m128i * ) (pbSrc + 64) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 80), _mm_xor_si128( c5, _mm_loadu_si128( ( __m128i * ) (pbSrc + 80) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 96), _mm_xor_si128( c6, _mm_loadu_si128( ( __m128i * ) (pbSrc + 96) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst +112), _mm_xor_si128( c7, _mm_loadu_si128( ( __m128i * ) (pbSrc +112) ) ) );

            pbDst  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
            pbSrc  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
            nBlocks -= 8;

            if( todo == 0 )
            {
                CLMUL_3_POST( a0, a1, a2 );
                MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );

                todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS );
                CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );
            }
        }

        // We now have at least 8 blocks of encrypted data to GHASH and at most 7 blocks left to encrypt
        // Do 8 blocks of GHASH in parallel with generating 0, 4, or 8 AES-CTR blocks for tail encryption
        nBlocks -= 8;
        if (nBlocks > 0)
        {
            c0 = chain;
            c1 = _mm_add_epi32( chain, chainIncrement1 );
            c2 = _mm_add_epi32( chain, chainIncrement2 );
            c3 = _mm_add_epi32( c1, chainIncrement2 );
            c4 = _mm_add_epi32( c2, chainIncrement2 );

            c0 = _mm_shuffle_epi8( c0, BYTE_REVERSE_ORDER );
            c1 = _mm_shuffle_epi8( c1, BYTE_REVERSE_ORDER );
            c2 = _mm_shuffle_epi8( c2, BYTE_REVERSE_ORDER );
            c3 = _mm_shuffle_epi8( c3, BYTE_REVERSE_ORDER );

            if (nBlocks > 4)
            {
                // Do 8 rounds of AES-CTR for tail in parallel with 8 rounds of GHASH
                c5 = _mm_add_epi32( c4, chainIncrement1 );
                c6 = _mm_add_epi32( c4, chainIncrement2 );

                c4 = _mm_shuffle_epi8( c4, BYTE_REVERSE_ORDER );
                c5 = _mm_shuffle_epi8( c5, BYTE_REVERSE_ORDER );
                c6 = _mm_shuffle_epi8( c6, BYTE_REVERSE_ORDER );

                AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pbGhashSrc, 8, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );
            }
            else
            {
                // Do 4 rounds of AES-CTR for tail in parallel with 8 rounds of GHASH
                AES_GCM_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3, pbGhashSrc, 8, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );
            }

            if( todo == 0)
            {
                CLMUL_3_POST( a0, a1, a2 );
                MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );

                todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS );
                CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );
            }
        }
        else
        {
            // Just do the final 8 rounds of GHASH
            for( todo=8; todo>0; todo-- )
            {
                r0 = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *) (pbGhashSrc +  0) ), BYTE_REVERSE_ORDER );
                pbGhashSrc += SYMCRYPT_AES_BLOCK_SIZE;

                CLMUL_ACC_3( r0, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );
            }

            CLMUL_3_POST( a0, a1, a2 );
            MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );
        }
    }

    if( nBlocks > 0 )
    {
        // Encrypt 1-7 blocks with pre-generated AES-CTR blocks and GHASH the results
        while( nBlocks >= 2 )
        {
            chain = _mm_add_epi32( chain, chainIncrement2 );

            r0 = _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0) ) );
            r1 = _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc + 16) ) );

            _mm_storeu_si128( (__m128i *) (pbDst +  0), r0 );
            _mm_storeu_si128( (__m128i *) (pbDst + 16), r1 );

            r0 = _mm_shuffle_epi8( r0, BYTE_REVERSE_ORDER );
            r1 = _mm_shuffle_epi8( r1, BYTE_REVERSE_ORDER );

            CLMUL_ACC_3( r0, GHASH_H_POWER(expandedKeyTable, todo - 0), GHASH_Hx_POWER(expandedKeyTable, todo - 0), a0, a1, a2 );
            CLMUL_ACC_3( r1, GHASH_H_POWER(expandedKeyTable, todo - 1), GHASH_Hx_POWER(expandedKeyTable, todo - 1), a0, a1, a2 );

            pbDst   += 2*SYMCRYPT_AES_BLOCK_SIZE;
            pbSrc   += 2*SYMCRYPT_AES_BLOCK_SIZE;
            todo    -= 2;
            nBlocks -= 2;
            c0 = c2;
            c1 = c3;
            c2 = c4;
            c3 = c5;
            c4 = c6;
        }

        if( nBlocks > 0 )
        {
            chain = _mm_add_epi32( chain, chainIncrement1 );

            r0 = _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0) ) );

            _mm_storeu_si128( (__m128i *) (pbDst +  0), r0 );

            r0 = _mm_shuffle_epi8( r0, BYTE_REVERSE_ORDER );

            CLMUL_ACC_3( r0, GHASH_H_POWER(expandedKeyTable, 1), GHASH_Hx_POWER(expandedKeyTable, 1), a0, a1, a2 );
        }

        CLMUL_3_POST( a0, a1, a2 );
        MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );
    }

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER );
    _mm_storeu_si128( (__m128i *) pbChainingValue, chain );
    _mm_storeu_si128( (__m128i *) pState, state );
}

#pragma warning(push)
#pragma warning( disable:4701 )
#pragma runtime_checks( "u", off )
// This call is functionally identical to:
// SymCryptGHashAppendDataPclmulqdq(   expandedKeyTable,
//                                     pState,
//                                     pbSrc,
//                                     cbData );
// SymCryptAesCtrMsb64Xmm( pExpandedKey,
//                         pbChainingValue,
//                         pbSrc,
//                         pbDst,
//                         cbData );
VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptStitchedXmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __m128i chain = _mm_loadu_si128( (__m128i *) pbChainingValue );

    __m128i BYTE_REVERSE_ORDER = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
    __m128i vMultiplicationConstant = _mm_set_epi32( 0, 0, 0xc2000000, 0 );

    __m128i chainIncrement1 = _mm_set_epi32( 0, 0, 0, 1 );
    __m128i chainIncrement2 = _mm_set_epi32( 0, 0, 0, 2 );

    __m128i c0, c1, c2, c3, c4, c5, c6, c7;

    __m128i state;
    __m128i a0, a1, a2;
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    SIZE_T todo = 0;
    PCBYTE pbGhashSrc = pbSrc;

    SYMCRYPT_ASSERT( (cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == 0 ); // cbData is multiple of block size

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER );
    state = _mm_loadu_si128( (__m128i *) pState );

    todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS );
    CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );

    while( nBlocks >= 8 )
    {
        // In this loop we always have 8 blocks to decrypt and GHASH
        c0 = chain;
        c1 = _mm_add_epi32( chain, chainIncrement1 );
        c2 = _mm_add_epi32( chain, chainIncrement2 );
        c3 = _mm_add_epi32( c1, chainIncrement2 );
        c4 = _mm_add_epi32( c2, chainIncrement2 );
        c5 = _mm_add_epi32( c3, chainIncrement2 );
        c6 = _mm_add_epi32( c4, chainIncrement2 );
        c7 = _mm_add_epi32( c5, chainIncrement2 );
        chain = _mm_add_epi32( c6, chainIncrement2 );

        c0 = _mm_shuffle_epi8( c0, BYTE_REVERSE_ORDER );
        c1 = _mm_shuffle_epi8( c1, BYTE_REVERSE_ORDER );
        c2 = _mm_shuffle_epi8( c2, BYTE_REVERSE_ORDER );
        c3 = _mm_shuffle_epi8( c3, BYTE_REVERSE_ORDER );
        c4 = _mm_shuffle_epi8( c4, BYTE_REVERSE_ORDER );
        c5 = _mm_shuffle_epi8( c5, BYTE_REVERSE_ORDER );
        c6 = _mm_shuffle_epi8( c6, BYTE_REVERSE_ORDER );
        c7 = _mm_shuffle_epi8( c7, BYTE_REVERSE_ORDER );

        AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pbGhashSrc, 8, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );

        _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 16), _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc + 16) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 32), _mm_xor_si128( c2, _mm_loadu_si128( ( __m128i * ) (pbSrc + 32) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 48), _mm_xor_si128( c3, _mm_loadu_si128( ( __m128i * ) (pbSrc + 48) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 64), _mm_xor_si128( c4, _mm_loadu_si128( ( __m128i * ) (pbSrc + 64) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 80), _mm_xor_si128( c5, _mm_loadu_si128( ( __m128i * ) (pbSrc + 80) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 96), _mm_xor_si128( c6, _mm_loadu_si128( ( __m128i * ) (pbSrc + 96) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst +112), _mm_xor_si128( c7, _mm_loadu_si128( ( __m128i * ) (pbSrc +112) ) ) );

        pbDst  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        pbSrc  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        nBlocks -= 8;

        if ( todo == 0 )
        {
            CLMUL_3_POST( a0, a1, a2 );
            MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );

            if ( nBlocks > 0 )
            {
                todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS );
                CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );
            }
        }
    }

    if( nBlocks > 0 )
    {
        // We have 1-7 blocks to GHASH and decrypt
        // Do the exact number of GHASH blocks we need in parallel with generating either 4 or 8 blocks of AES-CTR
        c0 = chain;
        c1 = _mm_add_epi32( chain, chainIncrement1 );
        c2 = _mm_add_epi32( chain, chainIncrement2 );
        c3 = _mm_add_epi32( c1, chainIncrement2 );
        c4 = _mm_add_epi32( c2, chainIncrement2 );

        c0 = _mm_shuffle_epi8( c0, BYTE_REVERSE_ORDER );
        c1 = _mm_shuffle_epi8( c1, BYTE_REVERSE_ORDER );
        c2 = _mm_shuffle_epi8( c2, BYTE_REVERSE_ORDER );
        c3 = _mm_shuffle_epi8( c3, BYTE_REVERSE_ORDER );

        if( nBlocks > 4 )
        {
            c5 = _mm_add_epi32( c4, chainIncrement1 );
            c6 = _mm_add_epi32( c4, chainIncrement2 );

            c4 = _mm_shuffle_epi8( c4, BYTE_REVERSE_ORDER );
            c5 = _mm_shuffle_epi8( c5, BYTE_REVERSE_ORDER );
            c6 = _mm_shuffle_epi8( c6, BYTE_REVERSE_ORDER );

            AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pbGhashSrc, nBlocks, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );
        } else {
            AES_GCM_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3, pbGhashSrc, nBlocks, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );
        }

        CLMUL_3_POST( a0, a1, a2 );
        MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );

        // Decrypt 1-7 blocks with pre-generated AES-CTR blocks
        while( nBlocks >= 2 )
        {
            chain = _mm_add_epi32( chain, chainIncrement2 );

            _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 16), _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc + 16) ) ) );

            pbDst   += 2*SYMCRYPT_AES_BLOCK_SIZE;
            pbSrc   += 2*SYMCRYPT_AES_BLOCK_SIZE;
            nBlocks -= 2;
            c0 = c2;
            c1 = c3;
            c2 = c4;
            c3 = c5;
            c4 = c6;
        }

        if( nBlocks > 0 )
        {
            chain = _mm_add_epi32( chain, chainIncrement1 );

            _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0) ) ) );
        }
    }

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER );
    _mm_storeu_si128( (__m128i *) pbChainingValue, chain );
    _mm_storeu_si128((__m128i *)pState, state );
}
#pragma runtime_checks( "u", restore )
#pragma warning(pop)

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif // CPU_X86 | CPU_AMD64
