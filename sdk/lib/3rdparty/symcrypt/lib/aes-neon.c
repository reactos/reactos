//
// aes-neon.c   code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// All NEON-based code for AES operations
//

#include "precomp.h"

#if SYMCRYPT_CPU_ARM64

#pragma clang attribute push (__attribute__((target("aes"))), apply_to=function)

#define vzeroq()    vdupq_n_u64(0)


VOID
SYMCRYPT_CALL
SymCryptAes4SboxNeon( _In_reads_(4) PCBYTE pIn, _Out_writes_(4) PBYTE pOut )
{
    /*
    __m128i x;

    x = _mm_set1_epi32( *(int *) pIn );

    x = _mm_aeskeygenassist_si128( x, 0 );

    *(unsigned *) pOut = x.m128i_u32[0];
    */
    __n128 x;

    //
    // There is no pure S-box lookup instruction, but the AESE instruction
    // does a ShiftRow followed by a SubBytes.
    // If we duplicate the input value to all 4 lanes, then the ShiftRow does nothing
    // and the SubBytes will do the S-box lookup.
    //
    x = vdupq_n_u32( *(unsigned int *) pIn );
    x = vaeseq_u8( x, vzeroq() );
    vst1q_lane_s32( pOut, x, 0 );
    //*(unsigned int *) pOut = x.n128_u32[0];
}


VOID
SYMCRYPT_CALL
SymCryptAesCreateDecryptionRoundKeyNeon(
    _In_reads_(16)      PCBYTE  pEncryptionRoundKey,
    _Out_writes_(16)    PBYTE   pDecryptionRoundKey )
{
    *(__n128 *) pDecryptionRoundKey = vaesimcq_u8( *(__n128 *)pEncryptionRoundKey );
}

//
// When doing a full round of AES encryption, make sure to give compiler opportunity to schedule dependent
// aese/aesmc pairs to enable instruction fusion in many arm64 CPUs
//
#define AESE_AESMC( c, rk ) \
{ \
    c = vaeseq_u8( c, rk ); \
    c = vaesmcq_u8( c ); \
};

//
// When doing a full round of AES decryption, make sure to give compiler opportunity to schedule dependent
// aesd/aesimc pairs to enable instruction fusion in many arm64 CPUs
//
#define AESD_AESIMC( c, rk ) \
{ \
    c = vaesdq_u8( c, rk ); \
    c = vaesimcq_u8( c ); \
};

//
// Using a loop with AESE_AESMC and AESD_AESIMC, the compiler can still prematurely rearrange the loop and
// lose opportunity for scheduling adjacent pairs.
// Instead, explicitly unroll the AES rounds with this macro.
// Takes the name of first_round, full_round, and final_round macros, and uses them to construct block to
// handle AES (128|192|256) for either encrypt or decrypt. For now assume only need at most 8 state
// variables in the macros.
// Assumes roundKey, keyPtr, and keyLimit are defined in calling context.
//
#define UNROLL_AES_ROUNDS_FIRST( first_round, full_round, final_round, c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    /* Do 9 full rounds (AES-128|AES-192|AES-256) */ \
    roundKey = *keyPtr++; \
    first_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
    full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
    full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
    full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
    full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
    full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
    full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
    full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
    full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
    roundKey = *keyPtr++; \
\
    if ( keyPtr < keyLimit ) \
    { \
        /* Do 2 more full rounds (AES-192|AES-256) */ \
        full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
        roundKey = *keyPtr++; \
        full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
        roundKey = *keyPtr++; \
\
        if ( keyPtr < keyLimit ) \
        { \
            /* Do 2 more full rounds (AES-256) */ \
            full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
            roundKey = *keyPtr++; \
            full_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
            roundKey = *keyPtr++; \
        } \
    } \
\
    /* Do final round (AES-128|AES-192|AES-256) */ \
    final_round( c0, c1, c2, c3, c4, c5, c6, c7 ) \
};

// Only AES_ENCRYPT_1_CHAIN needs to specify the first round differently from the full round
#define UNROLL_AES_ROUNDS( full_round, final_round, c0, c1, c2, c3, c4, c5, c6, c7 ) \
    UNROLL_AES_ROUNDS_FIRST( full_round, full_round, final_round, c0, c1, c2, c3, c4, c5, c6, c7 )

#define AES_ENCRYPT_ROUND_1( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    AESE_AESMC( c0, roundKey ) \
};
#define AES_ENCRYPT_FINAL_1( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    c0 = vaeseq_u8( c0, roundKey ); \
    roundKey = *keyPtr; \
    c0 = veorq_u8( c0, roundKey ); \
};

#define AES_ENCRYPT_1( pExpandedKey, c0 ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)&pExpandedKey->RoundKey[0]; \
    keyLimit = (const __n128 *)pExpandedKey->lastEncRoundKey; \
\
    UNROLL_AES_ROUNDS( \
        AES_ENCRYPT_ROUND_1, \
        AES_ENCRYPT_FINAL_1, \
        c0, c1, c2, c3, c4, c5, c6, c7 \
    ) \
};

// Perform AES encryption without the last round key and with a specified first round key
//
// For algorithms where performance is dominated by a chain of dependent AES rounds (i.e. CBC encryption, CCM, CMAC)
// we can gain a reasonable performance uplift by computing (last round key ^ this plaintext block ^ first round key)
// off the critical path and using this computed value in place of first round key in the first AESE instruction.
#define AES_ENCRYPT_CHAIN_FIRST_1( c0, mergedFirstRoundKey, c2, c3, c4, c5, c6, c7 ) \
{ \
    AESE_AESMC( c0, mergedFirstRoundKey ) \
};
#define AES_ENCRYPT_CHAIN_FINAL_1( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    c0 = vaeseq_u8( c0, roundKey ); \
};

#define AES_ENCRYPT_1_CHAIN( pExpandedKey, c0, mergedFirstRoundKey ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)&pExpandedKey->RoundKey[0]; \
    keyLimit = (const __n128 *)pExpandedKey->lastEncRoundKey; \
\
    UNROLL_AES_ROUNDS_FIRST( \
        AES_ENCRYPT_CHAIN_FIRST_1, \
        AES_ENCRYPT_ROUND_1, \
        AES_ENCRYPT_CHAIN_FINAL_1, \
        c0, mergedFirstRoundKey, c2, c3, c4, c5, c6, c7 \
    ) \
};

#define AES_ENCRYPT_ROUND_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    AESE_AESMC( c0, roundKey ) \
    AESE_AESMC( c1, roundKey ) \
    AESE_AESMC( c2, roundKey ) \
    AESE_AESMC( c3, roundKey ) \
};
#define AES_ENCRYPT_FINAL_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    c0 = vaeseq_u8( c0, roundKey ); \
    c1 = vaeseq_u8( c1, roundKey ); \
    c2 = vaeseq_u8( c2, roundKey ); \
    c3 = vaeseq_u8( c3, roundKey ); \
    roundKey = *keyPtr; \
    c0 = veorq_u8( c0, roundKey ); \
    c1 = veorq_u8( c1, roundKey ); \
    c2 = veorq_u8( c2, roundKey ); \
    c3 = veorq_u8( c3, roundKey ); \
};

#define AES_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3 ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)&pExpandedKey->RoundKey[0]; \
    keyLimit = (const __n128 *)pExpandedKey->lastEncRoundKey; \
\
    UNROLL_AES_ROUNDS( \
        AES_ENCRYPT_ROUND_4, \
        AES_ENCRYPT_FINAL_4, \
        c0, c1, c2, c3, c4, c5, c6, c7 \
    ) \
};

#define AES_ENCRYPT_ROUND_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    AESE_AESMC( c0, roundKey ) \
    AESE_AESMC( c1, roundKey ) \
    AESE_AESMC( c2, roundKey ) \
    AESE_AESMC( c3, roundKey ) \
    AESE_AESMC( c4, roundKey ) \
    AESE_AESMC( c5, roundKey ) \
    AESE_AESMC( c6, roundKey ) \
    AESE_AESMC( c7, roundKey ) \
};
#define AES_ENCRYPT_FINAL_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    c0 = vaeseq_u8( c0, roundKey ); \
    c1 = vaeseq_u8( c1, roundKey ); \
    c2 = vaeseq_u8( c2, roundKey ); \
    c3 = vaeseq_u8( c3, roundKey ); \
    c4 = vaeseq_u8( c4, roundKey ); \
    c5 = vaeseq_u8( c5, roundKey ); \
    c6 = vaeseq_u8( c6, roundKey ); \
    c7 = vaeseq_u8( c7, roundKey ); \
    roundKey = *keyPtr; \
    c0 = veorq_u8( c0, roundKey ); \
    c1 = veorq_u8( c1, roundKey ); \
    c2 = veorq_u8( c2, roundKey ); \
    c3 = veorq_u8( c3, roundKey ); \
    c4 = veorq_u8( c4, roundKey ); \
    c5 = veorq_u8( c5, roundKey ); \
    c6 = veorq_u8( c6, roundKey ); \
    c7 = veorq_u8( c7, roundKey ); \
};

#define AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)&pExpandedKey->RoundKey[0]; \
    keyLimit = (const __n128 *)pExpandedKey->lastEncRoundKey; \
\
    UNROLL_AES_ROUNDS( \
        AES_ENCRYPT_ROUND_8, \
        AES_ENCRYPT_FINAL_8, \
        c0, c1, c2, c3, c4, c5, c6, c7 \
    ) \
};

#define AES_DECRYPT_ROUND_1( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    AESD_AESIMC( c0, roundKey ) \
};
#define AES_DECRYPT_FINAL_1( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    c0 = vaesdq_u8( c0, roundKey ); \
    roundKey = *keyPtr; \
    c0 = veorq_u8( c0, roundKey ); \
};

#define AES_DECRYPT_1( pExpandedKey, c0 ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)pExpandedKey->lastEncRoundKey; \
    keyLimit = (const __n128 *)pExpandedKey->lastDecRoundKey; \
\
    UNROLL_AES_ROUNDS( \
        AES_DECRYPT_ROUND_1, \
        AES_DECRYPT_FINAL_1, \
        c0, c1, c2, c3, c4, c5, c6, c7 \
    ) \
};

#define AES_DECRYPT_ROUND_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    AESD_AESIMC( c0, roundKey ) \
    AESD_AESIMC( c1, roundKey ) \
    AESD_AESIMC( c2, roundKey ) \
    AESD_AESIMC( c3, roundKey ) \
};
#define AES_DECRYPT_FINAL_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    c0 = vaesdq_u8( c0, roundKey ); \
    c1 = vaesdq_u8( c1, roundKey ); \
    c2 = vaesdq_u8( c2, roundKey ); \
    c3 = vaesdq_u8( c3, roundKey ); \
    roundKey = *keyPtr; \
    c0 = veorq_u8( c0, roundKey ); \
    c1 = veorq_u8( c1, roundKey ); \
    c2 = veorq_u8( c2, roundKey ); \
    c3 = veorq_u8( c3, roundKey ); \
};

#define AES_DECRYPT_4( pExpandedKey, c0, c1, c2, c3 ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)pExpandedKey->lastEncRoundKey; \
    keyLimit = (const __n128 *)pExpandedKey->lastDecRoundKey; \
\
    UNROLL_AES_ROUNDS( \
        AES_DECRYPT_ROUND_4, \
        AES_DECRYPT_FINAL_4, \
        c0, c1, c2, c3, c4, c5, c6, c7 \
    ) \
};

#define AES_DECRYPT_ROUND_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    AESD_AESIMC( c0, roundKey ) \
    AESD_AESIMC( c1, roundKey ) \
    AESD_AESIMC( c2, roundKey ) \
    AESD_AESIMC( c3, roundKey ) \
    AESD_AESIMC( c4, roundKey ) \
    AESD_AESIMC( c5, roundKey ) \
    AESD_AESIMC( c6, roundKey ) \
    AESD_AESIMC( c7, roundKey ) \
};
#define AES_DECRYPT_FINAL_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    c0 = vaesdq_u8( c0, roundKey ); \
    c1 = vaesdq_u8( c1, roundKey ); \
    c2 = vaesdq_u8( c2, roundKey ); \
    c3 = vaesdq_u8( c3, roundKey ); \
    c4 = vaesdq_u8( c4, roundKey ); \
    c5 = vaesdq_u8( c5, roundKey ); \
    c6 = vaesdq_u8( c6, roundKey ); \
    c7 = vaesdq_u8( c7, roundKey ); \
    roundKey = *keyPtr; \
    c0 = veorq_u8( c0, roundKey ); \
    c1 = veorq_u8( c1, roundKey ); \
    c2 = veorq_u8( c2, roundKey ); \
    c3 = veorq_u8( c3, roundKey ); \
    c4 = veorq_u8( c4, roundKey ); \
    c5 = veorq_u8( c5, roundKey ); \
    c6 = veorq_u8( c6, roundKey ); \
    c7 = veorq_u8( c7, roundKey ); \
};

#define AES_DECRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)pExpandedKey->lastEncRoundKey; \
    keyLimit = (const __n128 *)pExpandedKey->lastDecRoundKey; \
\
    UNROLL_AES_ROUNDS( \
        AES_DECRYPT_ROUND_8, \
        AES_DECRYPT_FINAL_8, \
        c0, c1, c2, c3, c4, c5, c6, c7 \
    ) \
};



VOID
SYMCRYPT_CALL
SymCryptAesEncryptNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst )
{
    __n128 c;

    c = *( __n128 * ) pbSrc;

    AES_ENCRYPT_1( pExpandedKey, c );

    *(__n128 *) pbDst = c;
}

VOID
SYMCRYPT_CALL
SymCryptAesDecryptNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst )
{
    __n128 c;

    c = *( __n128 * ) pbSrc;

    AES_DECRYPT_1( pExpandedKey, c );

    *(__n128 *) pbDst = c;
}


VOID
SYMCRYPT_CALL
SymCryptAesCbcEncryptNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __n128 c = *(__n128 *)pbChainingValue;
    __n128 rk0 = *(__n128 *) &pExpandedKey->RoundKey[0];
    __n128 rkLast = *(__n128 *) pExpandedKey->lastEncRoundKey;
    __n128 d, rk0AndLast;

    // This algorithm is dominated by chain of dependent AES rounds, so we want to avoid EOR
    // instructions on the critical path where possible
    // We can compute (last round key ^ this plaintext block ^ first round key) off the critical
    // path and use this with AES_ENCRYPT_1_CHAIN so that only AES instructions write to c in
    // the main loop
    rk0AndLast = veorq_u8( rk0, rkLast );

    c = veorq_u8( c, rkLast );

    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        d = veorq_u8( *(__n128 *)pbSrc, rk0AndLast);
        AES_ENCRYPT_1_CHAIN( pExpandedKey, c, d );
        *(__n128 *)pbDst = veorq_u8( c, rkLast );

        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
    *(__n128 *)pbChainingValue = veorq_u8( c, rkLast );
}

// Disable warnings and VC++ runtime checks for use of uninitialized values (by design)
#pragma warning(push)
#pragma warning( disable: 6001 4701 )
#pragma runtime_checks( "u", off )
VOID
SYMCRYPT_CALL
SymCryptAesCbcDecryptNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __n128 chain;
    __n128 c0, c1, c2, c3, c4, c5, c6, c7;
    __n128 d0, d1, d2, d3, d4, d5, d6, d7;
    const __n128 * pSrc = (const __n128 *) pbSrc;
    __n128 * pDst = (__n128 *) pbDst;
    SIZE_T  cData = cbData / SYMCRYPT_AES_BLOCK_SIZE;

    if( cData < 1 )
    {
        return;
    }

    chain = *(__n128 *) pbChainingValue;

    //
    // First we do all multiples of 8 blocks
    //

    while( cData >= 8 )
    {
        d0 = c0 = pSrc[0];
        d1 = c1 = pSrc[1];
        d2 = c2 = pSrc[2];
        d3 = c3 = pSrc[3];
        d4 = c4 = pSrc[4];
        d5 = c5 = pSrc[5];
        d6 = c6 = pSrc[6];
        d7 = c7 = pSrc[7];

        AES_DECRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        c0 = veorq_u8( c0, chain );
        c1 = veorq_u8( c1, d0 );
        c2 = veorq_u8( c2, d1 );
        c3 = veorq_u8( c3, d2 );
        c4 = veorq_u8( c4, d3 );
        c5 = veorq_u8( c5, d4 );
        c6 = veorq_u8( c6, d5 );
        c7 = veorq_u8( c7, d6 );
        chain = d7;

        pDst[0] = c0;
        pDst[1] = c1;
        pDst[2] = c2;
        pDst[3] = c3;
        pDst[4] = c4;
        pDst[5] = c5;
        pDst[6] = c6;
        pDst[7] = c7;

        pSrc   += 8;
        pDst   += 8;
        cData  -= 8;
    }

    if( cData >= 1 )
    {
        //
        // There is remaining work to be done
        //
        d0 = c0 = pSrc[0];
        if( cData >= 2 )
        {
        d1 = c1 = pSrc[1];
            if( cData >= 3 )
            {
        d2 = c2 = pSrc[2];
                if( cData >= 4 )
                {
        d3 = c3 = pSrc[3];
                    if( cData >= 5 )
                    {
        d4 = c4 = pSrc[4];
                        if( cData >= 6 )
                        {
        d5 = c5 = pSrc[5];
                            if( cData >= 7 )
                            {
        d6 = c6 = pSrc[6];
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
        if( cData > 4 )
        {
            AES_DECRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );
            c0 = veorq_u8( c0, chain );
            c1 = veorq_u8( c1, d0 );
            c2 = veorq_u8( c2, d1 );
            c3 = veorq_u8( c3, d2 );
            c4 = veorq_u8( c4, d3 );
            c5 = veorq_u8( c5, d4 );
            c6 = veorq_u8( c6, d5 );
        }
        else if( cData > 1 )
        {
            AES_DECRYPT_4( pExpandedKey, c0, c1, c2, c3 );
            c0 = veorq_u8( c0, chain );
            c1 = veorq_u8( c1, d0 );
            c2 = veorq_u8( c2, d1 );
            c3 = veorq_u8( c3, d2 );
        } else
        {
            AES_DECRYPT_1( pExpandedKey, c0 );
            c0 = veorq_u8( c0, chain );
        }

        chain = pSrc[ cData - 1];
        pDst[0] = c0;
        if( cData >= 2 )
        {
        pDst[1] = c1;
            if( cData >= 3 )
            {
        pDst[2] = c2;
                if( cData >= 4 )
                {
        pDst[3] = c3;
                    if( cData >= 5 )
                    {
        pDst[4] = c4;
                        if( cData >= 6 )
                        {
        pDst[5] = c5;
                            if( cData >= 7 )
                            {
        pDst[6] = c6;
                            }
                        }
                    }
                }
            }
        }
    }

    *(__n128 *)pbChainingValue = chain;

    return;
}
#pragma runtime_checks( "u", restore )
#pragma warning( pop )



VOID
SYMCRYPT_CALL
SymCryptAesCbcMacNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData )
{
    __n128 c = *(__n128 *)pbChainingValue;
    __n128 rk0 = *(__n128 *) &pExpandedKey->RoundKey[0];
    __n128 rkLast = *(__n128 *) pExpandedKey->lastEncRoundKey;
    __n128 d, rk0AndLast;

    // This algorithm is dominated by chain of dependent AES rounds, so we want to avoid EOR
    // instructions on the critical path where possible
    // We can compute (last round key ^ this plaintext block ^ first round key) off the critical
    // path and use this with AES_ENCRYPT_1_CHAIN so that only AES instructions write to c in
    // the main loop
    rk0AndLast = veorq_u8( rk0, rkLast );

    c = veorq_u8( c, rkLast );

    while( cbData >= SYMCRYPT_AES_BLOCK_SIZE )
    {
        d = veorq_u8( *(__n128 *)pbData, rk0AndLast);
        AES_ENCRYPT_1_CHAIN( pExpandedKey, c, d );

        pbData += SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= SYMCRYPT_AES_BLOCK_SIZE;
    }
    *(__n128 *)pbChainingValue = veorq_u8( c, rkLast );
}

// Disable warnings and VC++ runtime checks for use of uninitialized values (by design)
#pragma warning(push)
#pragma warning( disable: 6001 4701 )
#pragma runtime_checks( "u", off )
VOID
SYMCRYPT_CALL
SymCryptAesEcbEncryptNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __n128 c0, c1, c2, c3, c4, c5, c6, c7;
    const __n128 * pSrc = (const __n128 *) pbSrc;
    __n128 * pDst = (__n128 *) pbDst;

    while( cbData >= 8 * SYMCRYPT_AES_BLOCK_SIZE )
    {
        c0 = pSrc[0];
        c1 = pSrc[1];
        c2 = pSrc[2];
        c3 = pSrc[3];
        c4 = pSrc[4];
        c5 = pSrc[5];
        c6 = pSrc[6];
        c7 = pSrc[7];

        AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        pDst[0] = c0;
        pDst[1] = c1;
        pDst[2] = c2;
        pDst[3] = c3;
        pDst[4] = c4;
        pDst[5] = c5;
        pDst[6] = c6;
        pDst[7] = c7;

        pSrc  += 8;
        pDst  += 8;
        cbData -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbData < 16 )
    {
        return;
    }

    c0 = pSrc[0];
    if( cbData >= 32 )
    {
    c1 = pSrc[1];
        if( cbData >= 48 )
        {
    c2 = pSrc[2];
            if( cbData >= 64 )
            {
    c3 = pSrc[3];
                if( cbData >= 80 )
                {
    c4 = pSrc[4];
                    if( cbData >= 96 )
                    {
    c5 = pSrc[5];
                        if( cbData >= 112 )
                        {
    c6 = pSrc[6];
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

    pDst[0] = c0;
    if( cbData >= 32 )
    {
    pDst[1] = c1;
        if( cbData >= 48 )
        {
    pDst[2] = c2;
            if( cbData >= 64 )
            {
    pDst[3] = c3;
                if( cbData >= 80 )
                {
    pDst[4] = c4;
                    if( cbData >= 96 )
                    {
    pDst[5] = c5;
                        if( cbData >= 112 )
                        {
    pDst[6] = c6;
                        }
                    }
                }
            }
        }
    }
}
#pragma runtime_checks( "u", restore)
#pragma warning( pop )

#pragma warning(push)
#pragma warning( disable:4701 ) // "Use of uninitialized variable"
#pragma runtime_checks( "u", off )

#define SYMCRYPT_AesCtrMsbXxNeon    SymCryptAesCtrMsb64Neon
#define VADDQ_UXX                   vaddq_u64
#define VSUBQ_UXX                   vsubq_u64

#include "aes-pattern.c"

#undef VSUBQ_UXX
#undef VADDQ_UXX
#undef SYMCRYPT_AesCtrMsbXxNeon

#define SYMCRYPT_AesCtrMsbXxNeon    SymCryptAesCtrMsb32Neon
#define VADDQ_UXX                   vaddq_u32
#define VSUBQ_UXX                   vsubq_u32

#include "aes-pattern.c"

#undef VSUBQ_UXX
#undef VADDQ_UXX
#undef SYMCRYPT_AesCtrMsbXxNeon

#pragma runtime_checks( "u", restore )
#pragma warning(pop)


//
// Multiply by alpha
//
// <</>> indicate shifts on 128-bit values
// <<<</>>>> indicate shifts on 32-bit values
//

// Multiply by ALPHA
// t1 = Input <<<< 1                        words shifted left by 1
// t2 = Input >>>> 31                       words shifted right by 31
// t1 = t1 ^ (t2 << 32)                     t1 = S << 1
// t2 = t2 >> 96                            t2 = highest bit of S
// t2 = (t2 <<<< 7) + (t2 <<<<3) - (t2)     multiply polynomially by 0x87 , we can use - because we only have one bit input
// res = t1 ^ t2
//
#define XTS_MUL_ALPHA_old( _in, _res ) \
{\
    __n128 _t1, _t2;\
\
    _t1 = vshlq_n_u32( _in, 1 ); \
    _t2 = vshrq_n_u32( _in, 31); \
    _t1 = veorq_u32( _t1, vextq_u32( vZero, _t2, 3 )); \
    _t2 = vextq_u32( _t2, vZero, 3); \
    _t2 = vsubq_u32( vaddq_u32( vshlq_n_u32( _t2, 7 ), vshlq_n_u32( _t2, 3 ) ), _t2 ); \
    _res = veorq_u32( _t1, _t2 ); \
}

//
// Another approach, use signed shift right to duplicate the bits of the leftmost byte
// and an AND to mask the modulo reduction and the extraneous bits in the other bytes at the same time.
// vAlphaMask = (1, 1, ..., 1, 0x87 )
//
#define XTS_MUL_ALPHA( _in, _res ) \
{\
    __n128 _t1, _t2;\
\
    _t1 = vshlq_n_u8( _in, 1 ); \
    _t2 = vshrq_n_s8( _in, 7 ); \
    _t2 = vextq_u8( _t2, _t2, 15 ); \
    _t2 = vandq_u8( _t2, vAlphaMask ); \
    _res = veorq_u8( _t2, _t1 ); \
}


// Multiply by ALPHA^2
// t1 = Input <<<< 2
// t2 = Input >>>> 30
// t1 = t1 ^ (t2 << 32)
// t2 = t2 >> 96
// t2 = (t2 <<<< 7) ^ (t2 <<<< 2) ^ (t2 <<<< 1) ^ t2
// res = t1 ^ t2
#define XTS_MUL_ALPHA2( _in, _res ) \
{\
    __n128 _t1, _t2;\
\
    _t1 = vshlq_n_u32( _in, 2 ); \
    _t2 = vshrq_n_u32( _in, 30); \
    _t1 = veorq_u32( _t1, vextq_u32( vZero, _t2, 3 )); \
    _t2 = vextq_u32( _t2, vZero, 3 ); \
    _t2 = veorq_u32( veorq_u32( veorq_u32( _t2, vshlq_n_u32( _t2, 7 )), vshlq_n_u32( _t2, 2 ) ), vshlq_n_u32( _t2, 1 ) ); \
    _res = veorq_u32( _t1, _t2 ); \
}

// Multiply by ALPHA^4
// t1 = Input <<<< 4
// t2 = Input >>>> 28
// t1 = t1 ^ (t2 << 32)
// t2 = t2 >> 96
// t2 = (t2 <<<< 7) ^ (t2 <<<< 2) ^ (t2 <<<< 1) ^ t2
// res = t1 ^ t2
#define XTS_MUL_ALPHA4( _in, _res ) \
{\
    __n128 _t1, _t2;\
\
    _t1 = vshlq_n_u32( _in, 4 ); \
    _t2 = vshrq_n_u32( _in, 28); \
    _t1 = veorq_u32( _t1, vextq_u32( vZero, _t2, 3 )); \
    _t2 = vextq_u32( _t2, vZero, 3 ); \
    _t2 = veorq_u32( veorq_u32( veorq_u32( _t2, vshlq_n_u32( _t2, 7 )), vshlq_n_u32( _t2, 2 ) ), vshlq_n_u32( _t2, 1 ) ); \
    _res = veorq_u32( _t1, _t2 ); \
}

#define XTS_MUL_ALPHA5( _in, _res ) \
{\
    __n128 _t1, _t2;\
\
    _t1 = vshlq_n_u32( _in, 5 ); \
    _t2 = vshrq_n_u32( _in, 27); \
    _t1 = veorq_u32( _t1, vextq_u32( vZero, _t2, 3 )); \
    _t2 = vextq_u32( _t2, vZero, 3 ); \
    _t2 = veorq_u32( veorq_u32( veorq_u32( _t2, vshlq_n_u32( _t2, 7 )), vshlq_n_u32( _t2, 2 ) ), vshlq_n_u32( _t2, 1 ) ); \
    _res = veorq_u32( _t1, _t2 ); \
}

// Multiply by ALPHA^8
// res = (Input << 8) | (Input >> 120)
// t2 = (Input >> 120) * 0x86
//      i.e. ((Input >> 120) <<<< 7) ^ ((Input >> 120) <<<< 2) ^ ((Input >> 120) <<<< 1)
//           the 0x01 component is already in res where we want it
// res = res ^ t2
//
// vAlphaMultiplier = (0, 0, ..., 0, 0x86 )

#define XTS_MUL_ALPHA8( _in, _res ) \
{\
    __n128 _t2;\
\
    _res = vextq_u8( _in, _in, 15 ); \
    _t2 = vmull_p8( vget_low_p8(_res), vAlphaMultiplier ); \
    _res = veorq_u32( _res, _t2 ); \
}


VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_(SYMCRYPT_AES_BLOCK_SIZE)PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __n128 t0, t1, t2, t3, t4, t5, t6, t7;
    __n128 c0, c1, c2, c3, c4, c5, c6, c7;
    const __n128 vZero = vmovq_n_u8(0);
    const __n128 vAlphaMask = SYMCRYPT_SET_N128_U8(0x87, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
    const __n64 vAlphaMultiplier = SYMCRYPT_SET_N64_U64(0x0000000000000086);

    SIZE_T cbDataMain;  // number of bytes to handle in the main loop
    SIZE_T cbDataTail;  // number of bytes to handle in the tail loop
    BYTE tailBuf[2*SYMCRYPT_AES_BLOCK_SIZE];

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

    t0 = *(__n128 *)pbTweakBlock;

    if( cbDataMain > 0 )
    {
        // Set up for main loop entry
        // NOTE: We load the first 8 blocks and store the last 8 blocks out of the loop to allow
        // greater instruction interleaving in the main loop.
        // This appears to give about 5-8% performance uplift on little (in-order) cores and has
        // no effect on big cores.
        XTS_MUL_ALPHA4( t0, t4 );
        XTS_MUL_ALPHA ( t0, t1 );
        XTS_MUL_ALPHA ( t4, t5 );
        XTS_MUL_ALPHA ( t1, t2 );
        XTS_MUL_ALPHA ( t5, t6 );
        XTS_MUL_ALPHA ( t2, t3 );
        XTS_MUL_ALPHA ( t6, t7 );

        c0 = veorq_u32( vld1q_u8( pbSrc + (0*16) ), t0 );
        c1 = veorq_u32( vld1q_u8( pbSrc + (1*16) ), t1 );
        c2 = veorq_u32( vld1q_u8( pbSrc + (2*16) ), t2 );
        c3 = veorq_u32( vld1q_u8( pbSrc + (3*16) ), t3 );
        c4 = veorq_u32( vld1q_u8( pbSrc + (4*16) ), t4 );
        c5 = veorq_u32( vld1q_u8( pbSrc + (5*16) ), t5 );
        c6 = veorq_u32( vld1q_u8( pbSrc + (6*16) ), t6 );
        c7 = veorq_u32( vld1q_u8( pbSrc + (7*16) ), t7 );

        for(;;)
        {
            pbSrc += 8 * SYMCRYPT_AES_BLOCK_SIZE;

            AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

            cbDataMain -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
            if( cbDataMain < 8 * SYMCRYPT_AES_BLOCK_SIZE )
            {
                break;
            }

            // Interleave the final xor, write, and compute next tweak block, and load, and first xor.
            // This reduces register pressure and is more efficient.
            vst1q_u8( pbDst + (0*16), veorq_u32( c0, t0 ) );
            vst1q_u8( pbDst + (1*16), veorq_u32( c1, t1 ) );
            vst1q_u8( pbDst + (2*16), veorq_u32( c2, t2 ) );
            vst1q_u8( pbDst + (3*16), veorq_u32( c3, t3 ) );
            vst1q_u8( pbDst + (4*16), veorq_u32( c4, t4 ) );
            vst1q_u8( pbDst + (5*16), veorq_u32( c5, t5 ) );
            vst1q_u8( pbDst + (6*16), veorq_u32( c6, t6 ) );
            vst1q_u8( pbDst + (7*16), veorq_u32( c7, t7 ) );

            XTS_MUL_ALPHA8( t0, t0 );
            XTS_MUL_ALPHA8( t1, t1 );
            XTS_MUL_ALPHA8( t2, t2 );
            XTS_MUL_ALPHA8( t3, t3 );
            XTS_MUL_ALPHA8( t4, t4 );
            XTS_MUL_ALPHA8( t5, t5 );
            XTS_MUL_ALPHA8( t6, t6 );
            XTS_MUL_ALPHA8( t7, t7 );

            c0 = veorq_u32( vld1q_u8( pbSrc + (0*16) ), t0 );
            c1 = veorq_u32( vld1q_u8( pbSrc + (1*16) ), t1 );
            c2 = veorq_u32( vld1q_u8( pbSrc + (2*16) ), t2 );
            c3 = veorq_u32( vld1q_u8( pbSrc + (3*16) ), t3 );
            c4 = veorq_u32( vld1q_u8( pbSrc + (4*16) ), t4 );
            c5 = veorq_u32( vld1q_u8( pbSrc + (5*16) ), t5 );
            c6 = veorq_u32( vld1q_u8( pbSrc + (6*16) ), t6 );
            c7 = veorq_u32( vld1q_u8( pbSrc + (7*16) ), t7 );

            pbDst += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        }

        vst1q_u8( pbDst + (0*16), veorq_u32( c0, t0 ) );
        vst1q_u8( pbDst + (1*16), veorq_u32( c1, t1 ) );
        vst1q_u8( pbDst + (2*16), veorq_u32( c2, t2 ) );
        vst1q_u8( pbDst + (3*16), veorq_u32( c3, t3 ) );
        vst1q_u8( pbDst + (4*16), veorq_u32( c4, t4 ) );
        vst1q_u8( pbDst + (5*16), veorq_u32( c5, t5 ) );
        vst1q_u8( pbDst + (6*16), veorq_u32( c6, t6 ) );
        vst1q_u8( pbDst + (7*16), veorq_u32( c7, t7 ) );

        // We won't do another 8-block set
        // Update only the first tweak block in case it is needed for tail
        XTS_MUL_ALPHA8( t0, t0 );

        pbDst += 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbDataTail == 0 )
    {
        return; // <-- expected case; early return here
    }

    // Rare case, with data unit length not being multiple of 128 bytes, handle the tail one block at a time
    while( cbDataTail >= 2*SYMCRYPT_AES_BLOCK_SIZE )
    {
        c0 = veorq_u32( vld1q_u8(pbSrc), t0 );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        AES_ENCRYPT_1( pExpandedKey, c0 );
        vst1q_u8( pbDst, veorq_u32( c0, t0 ) );
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

        // Encrypt penultimate plaintext block into tailBuf
        c0 = veorq_u32( vld1q_u8(pbSrc), t0 );
        AES_ENCRYPT_1( pExpandedKey, c0 );
        c0 = veorq_u32( c0, t0 );
        vst1q_u8( &tailBuf[0], c0 );
        vst1q_u8( &tailBuf[SYMCRYPT_AES_BLOCK_SIZE], c0 );

        cbDataTail -= SYMCRYPT_AES_BLOCK_SIZE;

        // Copy final plaintext bytes to prefix of tailBuf - we must read before writing to support in-place encryption
        memcpy( &tailBuf[0], pbSrc + SYMCRYPT_AES_BLOCK_SIZE, cbDataTail );
        // Copy prefix of tailBuf[SYMCRYPT_AES_BLOCK_SIZE] to the right place in the destination buffer
        memcpy( pbDst + SYMCRYPT_AES_BLOCK_SIZE, &tailBuf[SYMCRYPT_AES_BLOCK_SIZE], cbDataTail );

        // Do final tweak update
        XTS_MUL_ALPHA( t0, t0 );

        // Load updated tailBuf into c0
        c0 = vld1q_u8( &tailBuf[0] );
    } else {
        // Just load final plaintext block into c0
        c0 = vld1q_u8( pbSrc );
    }

    // Final full block encryption
    c0 = veorq_u32( c0, t0 );
    AES_ENCRYPT_1( pExpandedKey, c0 );
    vst1q_u8( pbDst, veorq_u32( c0, t0 ) );
}


VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_(SYMCRYPT_AES_BLOCK_SIZE)PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __n128 t0, t1, t2, t3, t4, t5, t6, t7;
    __n128 c0, c1, c2, c3, c4, c5, c6, c7;
    const __n128 vZero = vmovq_n_u8(0);
    const __n128 vAlphaMask = SYMCRYPT_SET_N128_U8(0x87, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
    const __n64 vAlphaMultiplier = SYMCRYPT_SET_N64_U64(0x0000000000000086);

    SIZE_T cbDataMain;  // number of bytes to handle in the main loop
    SIZE_T cbDataTail;  // number of bytes to handle in the tail loop
    BYTE tailBuf[2*SYMCRYPT_AES_BLOCK_SIZE];

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

    t0 = *(__n128 *)pbTweakBlock;
    t7 = t0;

    if( cbDataMain > 0 )
    {
        // Set up for main loop entry
        // NOTE: We load the first 8 blocks and store the last 8 blocks out of the loop to allow
        // greater instruction interleaving in the main loop.
        // This appears to give about 5-8% performance uplift on little (in-order) cores and has
        // no effect on big cores.
        XTS_MUL_ALPHA4( t0, t4 );
        XTS_MUL_ALPHA ( t0, t1 );
        XTS_MUL_ALPHA ( t4, t5 );
        XTS_MUL_ALPHA ( t1, t2 );
        XTS_MUL_ALPHA ( t5, t6 );
        XTS_MUL_ALPHA ( t2, t3 );
        XTS_MUL_ALPHA ( t6, t7 );

        c0 = veorq_u32( vld1q_u8( pbSrc + (0*16) ), t0 );
        c1 = veorq_u32( vld1q_u8( pbSrc + (1*16) ), t1 );
        c2 = veorq_u32( vld1q_u8( pbSrc + (2*16) ), t2 );
        c3 = veorq_u32( vld1q_u8( pbSrc + (3*16) ), t3 );
        c4 = veorq_u32( vld1q_u8( pbSrc + (4*16) ), t4 );
        c5 = veorq_u32( vld1q_u8( pbSrc + (5*16) ), t5 );
        c6 = veorq_u32( vld1q_u8( pbSrc + (6*16) ), t6 );
        c7 = veorq_u32( vld1q_u8( pbSrc + (7*16) ), t7 );

        for(;;)
        {
            pbSrc += 8 * SYMCRYPT_AES_BLOCK_SIZE;

            AES_DECRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

            cbDataMain -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
            if( cbDataMain < 8 * SYMCRYPT_AES_BLOCK_SIZE )
            {
                break;
            }

            // Interleave the final xor, write, and compute next tweak block, and load, and first xor.
            // This reduces register pressure and is more efficient.
            vst1q_u8( pbDst + (0*16), veorq_u32( c0, t0 ) );
            vst1q_u8( pbDst + (1*16), veorq_u32( c1, t1 ) );
            vst1q_u8( pbDst + (2*16), veorq_u32( c2, t2 ) );
            vst1q_u8( pbDst + (3*16), veorq_u32( c3, t3 ) );
            vst1q_u8( pbDst + (4*16), veorq_u32( c4, t4 ) );
            vst1q_u8( pbDst + (5*16), veorq_u32( c5, t5 ) );
            vst1q_u8( pbDst + (6*16), veorq_u32( c6, t6 ) );
            vst1q_u8( pbDst + (7*16), veorq_u32( c7, t7 ) );

            XTS_MUL_ALPHA8( t0, t0 );
            XTS_MUL_ALPHA8( t1, t1 );
            XTS_MUL_ALPHA8( t2, t2 );
            XTS_MUL_ALPHA8( t3, t3 );
            XTS_MUL_ALPHA8( t4, t4 );
            XTS_MUL_ALPHA8( t5, t5 );
            XTS_MUL_ALPHA8( t6, t6 );
            XTS_MUL_ALPHA8( t7, t7 );

            c0 = veorq_u32( vld1q_u8( pbSrc + (0*16) ), t0 );
            c1 = veorq_u32( vld1q_u8( pbSrc + (1*16) ), t1 );
            c2 = veorq_u32( vld1q_u8( pbSrc + (2*16) ), t2 );
            c3 = veorq_u32( vld1q_u8( pbSrc + (3*16) ), t3 );
            c4 = veorq_u32( vld1q_u8( pbSrc + (4*16) ), t4 );
            c5 = veorq_u32( vld1q_u8( pbSrc + (5*16) ), t5 );
            c6 = veorq_u32( vld1q_u8( pbSrc + (6*16) ), t6 );
            c7 = veorq_u32( vld1q_u8( pbSrc + (7*16) ), t7 );

            pbDst += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        }

        vst1q_u8( pbDst + (0*16), veorq_u32( c0, t0 ) );
        vst1q_u8( pbDst + (1*16), veorq_u32( c1, t1 ) );
        vst1q_u8( pbDst + (2*16), veorq_u32( c2, t2 ) );
        vst1q_u8( pbDst + (3*16), veorq_u32( c3, t3 ) );
        vst1q_u8( pbDst + (4*16), veorq_u32( c4, t4 ) );
        vst1q_u8( pbDst + (5*16), veorq_u32( c5, t5 ) );
        vst1q_u8( pbDst + (6*16), veorq_u32( c6, t6 ) );
        vst1q_u8( pbDst + (7*16), veorq_u32( c7, t7 ) );

        // We won't do another 8-block set
        // Update only the first tweak block in case it is needed for tail
        XTS_MUL_ALPHA8( t0, t0 );

        pbDst += 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    if( cbDataTail == 0 )
    {
        return; // <-- expected case; early return here
    }

    // Rare case, with data unit length not being multiple of 128 bytes, handle the tail one block at a time
    while( cbDataTail >= 2*SYMCRYPT_AES_BLOCK_SIZE )
    {
        c0 = veorq_u32( vld1q_u8( pbSrc ), t0 );
        pbSrc += SYMCRYPT_AES_BLOCK_SIZE;
        AES_DECRYPT_1( pExpandedKey, c0 );
        vst1q_u8( pbDst, veorq_u32( c0, t0 ) );
        pbDst += SYMCRYPT_AES_BLOCK_SIZE;
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

        // Do final tweak update into t1
        // Penultimate tweak is in t0, ready for final decryption
        XTS_MUL_ALPHA( t0, t1 );

        // Decrypt penultimate ciphertext block into tailBuf
        c0 = veorq_u32( vld1q_u8( pbSrc ), t1 );
        AES_DECRYPT_1( pExpandedKey, c0 );
        c0 = veorq_u32( c0, t1 );
        vst1q_u8( &tailBuf[0], c0 );
        vst1q_u8( &tailBuf[SYMCRYPT_AES_BLOCK_SIZE], c0 );

        cbDataTail -= SYMCRYPT_AES_BLOCK_SIZE;

        // Copy final ciphertext bytes to prefix of tailBuf - we must read before writing to support in-place decryption
        memcpy( &tailBuf[0], pbSrc + SYMCRYPT_AES_BLOCK_SIZE, cbDataTail );
        // Copy prefix of tailBuf[SYMCRYPT_AES_BLOCK_SIZE] to the right place in the destination buffer
        memcpy( pbDst + SYMCRYPT_AES_BLOCK_SIZE, &tailBuf[SYMCRYPT_AES_BLOCK_SIZE], cbDataTail );

        // Load updated tailBuf into c0
        c0 = vld1q_u8( &tailBuf[0] );
    } else {
        // Just load final ciphertext block into c0
        c0 = vld1q_u8( pbSrc );
    }

    // Final full block decryption
    c0 = veorq_u32( c0, t0 );
    AES_DECRYPT_1( pExpandedKey, c0 );
    vst1q_u8( pbDst, veorq_u32( c0, t0 ) );
}

#include "ghash_definitions.h"

#define AES_ENCRYPT_ROUND_4_GHASH_1( c0, c1, c2, c3, r0, r0x, t0, t1, gHashPointer, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    AESE_AESMC( c0, roundKey ) \
    AESE_AESMC( c1, roundKey ) \
    AESE_AESMC( c2, roundKey ) \
    AESE_AESMC( c3, roundKey ) \
\
    r0x = *gHashPointer; \
    r0x = vrev64q_u8( r0x ); \
    r0 = vextq_u8( r0x, r0x, 8 ); \
    r0x = veorq_u8( r0, r0x ); \
    gHashPointer++; \
\
    t1 = GHASH_H_POWER(gHashExpandedKeyTable, todo); \
    t0 = vmullq_p64( r0, t1 ); \
    t1 = vmull_high_p64( r0, t1 ); \
\
    resl = veorq_u8( resl, t0 ); \
    resh = veorq_u8( resh, t1 ); \
\
    t1 = GHASH_Hx_POWER(gHashExpandedKeyTable, todo); \
    t1 = vmullq_p64( r0x, t1 ); \
\
    resm = veorq_u8( resm, t1 ); \
    todo--; \
};

//
// Using a loop with AESE_AESMC and AESD_AESIMC, the compiler can still prematurely rearrange the loop and
// lose opportunity for scheduling adjacent pairs.
// Instead, explicitly unroll the AES rounds with this macro.
//
#define AES_GCM_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3, gHashPointer, gHashRounds, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)&pExpandedKey->RoundKey[0]; \
    keyLimit = (const __n128 *)pExpandedKey->lastEncRoundKey; \
    __n128 t0, t1, r0, r0x; \
    SIZE_T aesEncryptGhashLoop; \
\
    /* Do gHashRounds full rounds (AES-128|AES-192|AES-256) with stitched GHASH */ \
    roundKey = *keyPtr++; \
    for( aesEncryptGhashLoop = 0; aesEncryptGhashLoop < gHashRounds; aesEncryptGhashLoop++) \
    { \
        AES_ENCRYPT_ROUND_4_GHASH_1( c0, c1, c2, c3, r0, r0x, t0, t1, gHashPointer, gHashExpandedKeyTable, todo, resl, resm, resh ) \
        roundKey = *keyPtr++; \
    } \
\
    /* Do 9-gHashRounds full rounds (AES-128|AES-192|AES-256) */ \
    for( aesEncryptGhashLoop = 0; aesEncryptGhashLoop < (9-gHashRounds); aesEncryptGhashLoop++) \
    { \
        AES_ENCRYPT_ROUND_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
        roundKey = *keyPtr++; \
    } \
\
    if ( keyPtr < keyLimit ) \
    { \
        /* Do 2 more full rounds (AES-192|AES-256) */ \
        AES_ENCRYPT_ROUND_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
        roundKey = *keyPtr++; \
        AES_ENCRYPT_ROUND_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
        roundKey = *keyPtr++; \
\
        if ( keyPtr < keyLimit ) \
        { \
            /* Do 2 more full rounds (AES-256) */ \
            AES_ENCRYPT_ROUND_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
            roundKey = *keyPtr++; \
            AES_ENCRYPT_ROUND_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
            roundKey = *keyPtr++; \
        } \
    } \
\
    /* Do final round (AES-128|AES-192|AES-256) */ \
    AES_ENCRYPT_FINAL_4( c0, c1, c2, c3, c4, c5, c6, c7 ) \
};

#define AES_ENCRYPT_ROUND_8_GHASH_1( c0, c1, c2, c3, c4, c5, c6, c7, r0, r0x, t0, t1, gHashPointer, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    AESE_AESMC( c0, roundKey ) \
    AESE_AESMC( c1, roundKey ) \
    AESE_AESMC( c2, roundKey ) \
    AESE_AESMC( c3, roundKey ) \
    AESE_AESMC( c4, roundKey ) \
    AESE_AESMC( c5, roundKey ) \
    AESE_AESMC( c6, roundKey ) \
    AESE_AESMC( c7, roundKey ) \
\
    r0x = *gHashPointer; \
    r0x = vrev64q_u8( r0x ); \
    r0 = vextq_u8( r0x, r0x, 8 ); \
    r0x = veorq_u8( r0, r0x ); \
    gHashPointer++; \
\
    t1 = GHASH_H_POWER(gHashExpandedKeyTable, todo); \
    t0 = vmullq_p64( r0, t1 ); \
    t1 = vmull_high_p64( r0, t1 ); \
\
    resl = veorq_u8( resl, t0 ); \
    resh = veorq_u8( resh, t1 ); \
\
    t1 = GHASH_Hx_POWER(gHashExpandedKeyTable, todo); \
    t1 = vmullq_p64( r0x, t1 ); \
\
    resm = veorq_u8( resm, t1 ); \
    todo--; \
};

//
// Using a loop with AESE_AESMC and AESD_AESIMC, the compiler can still prematurely rearrange the loop and
// lose opportunity for scheduling adjacent pairs.
// Instead, explicitly unroll the AES rounds with this macro.
//
#define AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, gHashPointer, gHashRounds, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    const __n128 *keyPtr; \
    const __n128 *keyLimit; \
    __n128 roundKey; \
\
    keyPtr = (const __n128 *)&pExpandedKey->RoundKey[0]; \
    keyLimit = (const __n128 *)pExpandedKey->lastEncRoundKey; \
    __n128 t0, t1, r0, r0x; \
    SIZE_T aesEncryptGhashLoop; \
\
    /* Do gHashRounds full rounds (AES-128|AES-192|AES-256) with stitched GHASH */ \
    roundKey = *keyPtr++; \
    for( aesEncryptGhashLoop = 0; aesEncryptGhashLoop < gHashRounds; aesEncryptGhashLoop++) \
    { \
        AES_ENCRYPT_ROUND_8_GHASH_1( c0, c1, c2, c3, c4, c5, c6, c7, r0, r0x, t0, t1, gHashPointer, gHashExpandedKeyTable, todo, resl, resm, resh ) \
        roundKey = *keyPtr++; \
    } \
\
    /* Do 9-gHashRounds full rounds (AES-128|AES-192|AES-256) */ \
    for( aesEncryptGhashLoop = 0; aesEncryptGhashLoop < (9-gHashRounds); aesEncryptGhashLoop++) \
    { \
        AES_ENCRYPT_ROUND_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
        roundKey = *keyPtr++; \
    } \
\
    if ( keyPtr < keyLimit ) \
    { \
        /* Do 2 more full rounds (AES-192|AES-256) */ \
        AES_ENCRYPT_ROUND_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
        roundKey = *keyPtr++; \
        AES_ENCRYPT_ROUND_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
        roundKey = *keyPtr++; \
\
        if ( keyPtr < keyLimit ) \
        { \
            /* Do 2 more full rounds (AES-256) */ \
            AES_ENCRYPT_ROUND_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
            roundKey = *keyPtr++; \
            AES_ENCRYPT_ROUND_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
            roundKey = *keyPtr++; \
        } \
    } \
\
    /* Do final round (AES-128|AES-192|AES-256) */ \
    AES_ENCRYPT_FINAL_8( c0, c1, c2, c3, c4, c5, c6, c7 ) \
};

// This call is functionally identical to:
// SymCryptAesCtrMsb64Neon( pExpandedKey,
//                          pbChainingValue,
//                          pbSrc,
//                          pbDst,
//                          cbData );
// SymCryptGHashAppendDataPmull(    expandedKeyTable,
//                                  pState,
//                                  pbDstOrig,
//                                  cbDataOrig );
VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptStitchedNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __n128          chain = *(__n128 *)pbChainingValue;
    const __n128 *  pSrc = (const __n128 *) pbSrc;
    const __n128 *  pGhashSrc = (const __n128 *) pbDst;
    __n128 *        pDst = (__n128 *) pbDst;

    const __n128 chainIncrement1 = SYMCRYPT_SET_N128_U64( 0, 1 );
    const __n128 chainIncrement2 = SYMCRYPT_SET_N128_U64( 0, 2 );
    const __n128 chainIncrement8 = SYMCRYPT_SET_N128_U64( 0, 8 );

    __n128 ctr0, ctr1, ctr2, ctr3, ctr4, ctr5, ctr6, ctr7;
    __n128 c0, c1, c2, c3, c4, c5, c6, c7;
    __n128 r0, r1;
    __n128 r0x, r1x;

    __n128 state;
    __n128 a0, a1, a2;
    const __n64 vMultiplicationConstant = SYMCRYPT_SET_N64_U64(0xc200000000000000);
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    SIZE_T todo;

    SYMCRYPT_ASSERT( (cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == 0 ); // cbData is multiple of block size

    // Our chain variable is in integer format, not the MSBfirst format loaded from memory.
    ctr0 = vrev64q_u8( chain );
    ctr1 = vaddq_u32( ctr0, chainIncrement1 );
    ctr2 = vaddq_u32( ctr0, chainIncrement2 );
    ctr3 = vaddq_u32( ctr1, chainIncrement2 );
    ctr4 = vaddq_u32( ctr2, chainIncrement2 );
    ctr5 = vaddq_u32( ctr3, chainIncrement2 );
    ctr6 = vaddq_u32( ctr4, chainIncrement2 );
    ctr7 = vaddq_u32( ctr5, chainIncrement2 );

    state = *(__n128 *) pState;

    todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PMULL_HPOWERS );
    CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );

    // Do 8 blocks of CTR either for tail (if total blocks <8) or for encryption of first 8 blocks
    c0 = vrev64q_u8( ctr0 );
    c1 = vrev64q_u8( ctr1 );
    c2 = vrev64q_u8( ctr2 );
    c3 = vrev64q_u8( ctr3 );
    c4 = vrev64q_u8( ctr4 );
    c5 = vrev64q_u8( ctr5 );
    c6 = vrev64q_u8( ctr6 );
    c7 = vrev64q_u8( ctr7 );

    AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

    if ( cbData >= 8 * SYMCRYPT_AES_BLOCK_SIZE )
    {
        ctr0 = vaddq_u32( ctr0, chainIncrement8 );
        ctr1 = vaddq_u32( ctr1, chainIncrement8 );
        ctr2 = vaddq_u32( ctr2, chainIncrement8 );
        ctr3 = vaddq_u32( ctr3, chainIncrement8 );
        ctr4 = vaddq_u32( ctr4, chainIncrement8 );
        ctr5 = vaddq_u32( ctr5, chainIncrement8 );
        ctr6 = vaddq_u32( ctr6, chainIncrement8 );
        ctr7 = vaddq_u32( ctr7, chainIncrement8 );

        // Encrypt first 8 blocks
        pDst[0] = veorq_u64( pSrc[0], c0 );
        pDst[1] = veorq_u64( pSrc[1], c1 );
        pDst[2] = veorq_u64( pSrc[2], c2 );
        pDst[3] = veorq_u64( pSrc[3], c3 );
        pDst[4] = veorq_u64( pSrc[4], c4 );
        pDst[5] = veorq_u64( pSrc[5], c5 );
        pDst[6] = veorq_u64( pSrc[6], c6 );
        pDst[7] = veorq_u64( pSrc[7], c7 );

        pDst  += 8;
        pSrc  += 8;

        while( nBlocks >= 16 )
        {
            // In this loop we always have 8 blocks to encrypt and we have already encrypted the previous 8 blocks ready for GHASH
            c0 = vrev64q_u8( ctr0 );
            c1 = vrev64q_u8( ctr1 );
            c2 = vrev64q_u8( ctr2 );
            c3 = vrev64q_u8( ctr3 );
            c4 = vrev64q_u8( ctr4 );
            c5 = vrev64q_u8( ctr5 );
            c6 = vrev64q_u8( ctr6 );
            c7 = vrev64q_u8( ctr7 );

            ctr0 = vaddq_u32( ctr0, chainIncrement8 );
            ctr1 = vaddq_u32( ctr1, chainIncrement8 );
            ctr2 = vaddq_u32( ctr2, chainIncrement8 );
            ctr3 = vaddq_u32( ctr3, chainIncrement8 );
            ctr4 = vaddq_u32( ctr4, chainIncrement8 );
            ctr5 = vaddq_u32( ctr5, chainIncrement8 );
            ctr6 = vaddq_u32( ctr6, chainIncrement8 );
            ctr7 = vaddq_u32( ctr7, chainIncrement8 );

            AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pGhashSrc, 8, expandedKeyTable, todo, a0, a1, a2 );

            pDst[0] = veorq_u64( pSrc[0], c0 );
            pDst[1] = veorq_u64( pSrc[1], c1 );
            pDst[2] = veorq_u64( pSrc[2], c2 );
            pDst[3] = veorq_u64( pSrc[3], c3 );
            pDst[4] = veorq_u64( pSrc[4], c4 );
            pDst[5] = veorq_u64( pSrc[5], c5 );
            pDst[6] = veorq_u64( pSrc[6], c6 );
            pDst[7] = veorq_u64( pSrc[7], c7 );

            pDst  += 8;
            pSrc  += 8;
            nBlocks -= 8;

            if (todo == 0)
            {
                CLMUL_3_POST( a0, a1, a2 );
                MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );

                todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PMULL_HPOWERS );
                CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );
            }
        }

        // We now have at least 8 blocks of encrypted data to GHASH and at most 7 blocks left to encrypt
        // Do 8 blocks of GHASH in parallel with generating 0, 4, or 8 AES-CTR blocks for tail encryption
        nBlocks -= 8;
        if (nBlocks > 0)
        {
            c0 = vrev64q_u8( ctr0 );
            c1 = vrev64q_u8( ctr1 );
            c2 = vrev64q_u8( ctr2 );
            c3 = vrev64q_u8( ctr3 );

            if (nBlocks > 4)
            {
                // Do 8 rounds of AES-CTR for tail in parallel with 8 rounds of GHASH
                c4 = vrev64q_u8( ctr4 );
                c5 = vrev64q_u8( ctr5 );
                c6 = vrev64q_u8( ctr6 );

                AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pGhashSrc, 8, expandedKeyTable, todo, a0, a1, a2 );
            }
            else
            {
                // Do 4 rounds of AES-CTR for tail in parallel with 8 rounds of GHASH
                AES_GCM_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3, pGhashSrc, 8, expandedKeyTable, todo, a0, a1, a2 );
            }

            if( todo == 0)
            {
                CLMUL_3_POST( a0, a1, a2 );
                MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );

                todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PMULL_HPOWERS );
                CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );
            }
        }
        else
        {
            // Just do the final 8 rounds of GHASH
            for( todo=8; todo>0; todo-- )
            {
                r0x = vrev64q_u8( pGhashSrc[0] );
                r0 = vextq_u8( r0x, r0x, 8 );
                r0x = veorq_u8( r0, r0x );
                pGhashSrc++;

                CLMUL_ACCX_3( r0, r0x, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );
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
            ctr0 = vaddq_u32( ctr0, chainIncrement2 );

            r0 = veorq_u64( pSrc[0], c0 );
            r1 = veorq_u64( pSrc[1], c1 );

            pDst[0] = r0;
            pDst[1] = r1;

            r0x = vrev64q_u8( r0 );
            r1x = vrev64q_u8( r1 );
            r0 = vextq_u8( r0x, r0x, 8 );
            r1 = vextq_u8( r1x, r1x, 8 );
            r0x = veorq_u8( r0, r0x );
            r1x = veorq_u8( r1, r1x );

            CLMUL_ACCX_3( r0, r0x, GHASH_H_POWER(expandedKeyTable, todo - 0), GHASH_Hx_POWER(expandedKeyTable, todo - 0), a0, a1, a2 );
            CLMUL_ACCX_3( r1, r1x, GHASH_H_POWER(expandedKeyTable, todo - 1), GHASH_Hx_POWER(expandedKeyTable, todo - 1), a0, a1, a2 );

            pDst    += 2;
            pSrc    += 2;
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
            ctr0 = vaddq_u32( ctr0, chainIncrement1 );

            r0 = veorq_u64( pSrc[0], c0 );
            pDst[0] = r0;
            r0x = vrev64q_u8( r0 );
            r0 = vextq_u8( r0x, r0x, 8 );
            r0x = veorq_u8( r0, r0x );

            CLMUL_ACCX_3( r0, r0x, GHASH_H_POWER(expandedKeyTable, 1), GHASH_Hx_POWER(expandedKeyTable, 1), a0, a1, a2 );
        }

        CLMUL_3_POST( a0, a1, a2 );
        MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );
    }

    chain = vrev64q_u8( ctr0 );
    *(__n128 *)pbChainingValue = chain;
    *(__n128 *)pState = state;
}

#pragma warning(push)
#pragma warning( disable:4701 ) // "Use of uninitialized variable" -
#pragma runtime_checks( "u", off )
// This call is functionally identical to:
// SymCryptGHashAppendDataPmull(expandedKeyTable,
//                              pState,
//                              pbSrc,
//                              cbData );
// SymCryptAesCtrMsb64Neon( pExpandedKey,
//                          pbChainingValue,
//                          pbSrc,
//                          pbDst,
//                          cbData );
VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptStitchedNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __n128          chain = *(__n128 *)pbChainingValue;
    const __n128 *  pSrc = (const __n128 *) pbSrc;
    const __n128 *  pGhashSrc = (const __n128 *) pbSrc;
    __n128 *        pDst = (__n128 *) pbDst;

    const __n128 chainIncrement1 = SYMCRYPT_SET_N128_U64( 0, 1 );
    const __n128 chainIncrement2 = SYMCRYPT_SET_N128_U64( 0, 2 );
    const __n128 chainIncrement8 = SYMCRYPT_SET_N128_U64( 0, 8 );

    __n128 ctr0, ctr1, ctr2, ctr3, ctr4, ctr5, ctr6, ctr7;
    __n128 c0, c1, c2, c3, c4, c5, c6, c7;

    __n128 state;
    __n128 a0, a1, a2;
    const __n64 vMultiplicationConstant = SYMCRYPT_SET_N64_U64(0xc200000000000000);
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    SIZE_T todo;

    SYMCRYPT_ASSERT( (cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == 0 ); // cbData is multiple of block size

    // Our chain variable is in integer format, not the MSBfirst format loaded from memory.
    ctr0 = vrev64q_u8( chain );
    ctr1 = vaddq_u32( ctr0, chainIncrement1 );
    ctr2 = vaddq_u32( ctr0, chainIncrement2 );
    ctr3 = vaddq_u32( ctr1, chainIncrement2 );
    ctr4 = vaddq_u32( ctr2, chainIncrement2 );
    ctr5 = vaddq_u32( ctr3, chainIncrement2 );
    ctr6 = vaddq_u32( ctr4, chainIncrement2 );
    ctr7 = vaddq_u32( ctr5, chainIncrement2 );

    state = *(__n128 *) pState;

    todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PMULL_HPOWERS );

    CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );

    while( nBlocks >= 8 )
    {
        // In this loop we always have 8 blocks to decrypt and GHASH
        c0 = vrev64q_u8( ctr0 );
        c1 = vrev64q_u8( ctr1 );
        c2 = vrev64q_u8( ctr2 );
        c3 = vrev64q_u8( ctr3 );
        c4 = vrev64q_u8( ctr4 );
        c5 = vrev64q_u8( ctr5 );
        c6 = vrev64q_u8( ctr6 );
        c7 = vrev64q_u8( ctr7 );

        ctr0 = vaddq_u32( ctr0, chainIncrement8 );
        ctr1 = vaddq_u32( ctr1, chainIncrement8 );
        ctr2 = vaddq_u32( ctr2, chainIncrement8 );
        ctr3 = vaddq_u32( ctr3, chainIncrement8 );
        ctr4 = vaddq_u32( ctr4, chainIncrement8 );
        ctr5 = vaddq_u32( ctr5, chainIncrement8 );
        ctr6 = vaddq_u32( ctr6, chainIncrement8 );
        ctr7 = vaddq_u32( ctr7, chainIncrement8 );

        AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pGhashSrc, 8, expandedKeyTable, todo, a0, a1, a2 );

        pDst[0] = veorq_u64( pSrc[0], c0 );
        pDst[1] = veorq_u64( pSrc[1], c1 );
        pDst[2] = veorq_u64( pSrc[2], c2 );
        pDst[3] = veorq_u64( pSrc[3], c3 );
        pDst[4] = veorq_u64( pSrc[4], c4 );
        pDst[5] = veorq_u64( pSrc[5], c5 );
        pDst[6] = veorq_u64( pSrc[6], c6 );
        pDst[7] = veorq_u64( pSrc[7], c7 );

        pDst  += 8;
        pSrc  += 8;
        nBlocks -= 8;

        if (todo == 0)
        {
            CLMUL_3_POST( a0, a1, a2 );
            MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );

            if ( nBlocks > 0 )
            {
                todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PMULL_HPOWERS );
                CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0, a1, a2 );
            }
        }
    }

    if( nBlocks > 0 )
    {
        // We have 1-7 blocks to GHASH and decrypt
        // Do the exact number of GHASH blocks we need in parallel with generating either 4 or 8 blocks of AES-CTR
        c0 = vrev64q_u8( ctr0 );
        c1 = vrev64q_u8( ctr1 );
        c2 = vrev64q_u8( ctr2 );
        c3 = vrev64q_u8( ctr3 );

        if( nBlocks > 4 )
        {
            c4 = vrev64q_u8( ctr4 );
            c5 = vrev64q_u8( ctr5 );
            c6 = vrev64q_u8( ctr6 );

            AES_GCM_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pGhashSrc, nBlocks, expandedKeyTable, todo, a0, a1, a2 );
        } else {
            AES_GCM_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3, pGhashSrc, nBlocks, expandedKeyTable, todo, a0, a1, a2 );
        }
        CLMUL_3_POST( a0, a1, a2 );
        MODREDUCE( vMultiplicationConstant, a0, a1, a2, state );

        // Decrypt 1-7 blocks with pre-generated AES-CTR blocks
        while( nBlocks >= 2 )
        {
            ctr0 = vaddq_u32( ctr0, chainIncrement2 );

            pDst[0] = veorq_u64( pSrc[0], c0 );
            pDst[1] = veorq_u64( pSrc[1], c1 );

            pDst    += 2;
            pSrc    += 2;
            nBlocks -= 2;
            c0 = c2;
            c1 = c3;
            c2 = c4;
            c3 = c5;
            c4 = c6;
        }

        if( nBlocks > 0 )
        {
            ctr0 = vaddq_u32( ctr0, chainIncrement1 );

            pDst[0] = veorq_u64( pSrc[0], c0 );
        }
    }

    chain = vrev64q_u8( ctr0 );
    *(__n128 *)pbChainingValue = chain;
    *(__n128 *)pState = state;
}
#pragma runtime_checks( "u", restore )
#pragma warning(pop)
#pragma clang attribute pop

#endif
