//
// aes-pattern.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This file contains "pattern" code for AES-related functions. It's not intended to be compiled
// directly; rather it is included by other aes-*.c files which define the macros used here.
//

#if 0
#pragma makedep header
#endif

#if SYMCRYPT_CPU_ARM64

VOID
SYMCRYPT_CALL
SYMCRYPT_AesCtrMsbXxNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __n128          chain = *(__n128 *)pbChainingValue;
    const __n128 *  pSrc = (const __n128 *) pbSrc;
    __n128 *        pDst = (__n128 *) pbDst;

    const __n128 chainIncrement1 = SYMCRYPT_SET_N128_U64( 0, 1 );
    const __n128 chainIncrement2 = SYMCRYPT_SET_N128_U64( 0, 2 );
    const __n128 chainIncrement8 = SYMCRYPT_SET_N128_U64( 0, 8 );

    __n128 ctr0, ctr1, ctr2, ctr3, ctr4, ctr5, ctr6, ctr7;
    __n128 c0, c1, c2, c3, c4, c5, c6, c7;

    cbData &= ~(SYMCRYPT_AES_BLOCK_SIZE - 1);

    // Our chain variable is in integer format, not the MSBfirst format loaded from memory.
    ctr0 = vrev64q_u8( chain );
    ctr1 = VADDQ_UXX( ctr0, chainIncrement1 );
    ctr2 = VADDQ_UXX( ctr0, chainIncrement2 );
    ctr3 = VADDQ_UXX( ctr1, chainIncrement2 );
    ctr4 = VADDQ_UXX( ctr2, chainIncrement2 );
    ctr5 = VADDQ_UXX( ctr3, chainIncrement2 );
    ctr6 = VADDQ_UXX( ctr4, chainIncrement2 );
    ctr7 = VADDQ_UXX( ctr5, chainIncrement2 );

/*
    while cbData >= 5 * block
        generate 8 blocks of key stream
        if cbData < 8 * block
            break;
        process 8 blocks
    if cbData >= 5 * block
        process 5-7 blocks
        done
    if cbData >= 2 * block
        generate 4 blocks of key stream
        process 2-4 blocks
        done
    if cbData == 1 block
        generate 1 block of key stream
        process block
*/
    while( cbData >= 5 * SYMCRYPT_AES_BLOCK_SIZE )
    {
        c0 = vrev64q_u8( ctr0 );
        c1 = vrev64q_u8( ctr1 );
        c2 = vrev64q_u8( ctr2 );
        c3 = vrev64q_u8( ctr3 );
        c4 = vrev64q_u8( ctr4 );
        c5 = vrev64q_u8( ctr5 );
        c6 = vrev64q_u8( ctr6 );
        c7 = vrev64q_u8( ctr7 );

        ctr0 = VADDQ_UXX( ctr0, chainIncrement8 );
        ctr1 = VADDQ_UXX( ctr1, chainIncrement8 );
        ctr2 = VADDQ_UXX( ctr2, chainIncrement8 );
        ctr3 = VADDQ_UXX( ctr3, chainIncrement8 );
        ctr4 = VADDQ_UXX( ctr4, chainIncrement8 );
        ctr5 = VADDQ_UXX( ctr5, chainIncrement8 );
        ctr6 = VADDQ_UXX( ctr6, chainIncrement8 );
        ctr7 = VADDQ_UXX( ctr7, chainIncrement8 );

        AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        if( cbData < 8 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            break;
        }

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
        cbData -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    //
    // At this point we have one of the two following cases:
    // - cbData >= 5 * 16 and we have 8 blocks of key stream in c0-c7. ctr0-ctr7 is set to (c0+8)-(c7+8)
    // - cbData < 5 * 16 and we have no blocks of key stream, and ctr0-ctr7 set to the next 8 counters to use
    //

    if( cbData >= SYMCRYPT_AES_BLOCK_SIZE ) // quick exit of function if the request was a multiple of 8 blocks
    {
        if( cbData >= 5 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            //
            // We already have the key stream
            //
            pDst[0] = veorq_u64( pSrc[0], c0 );
            pDst[1] = veorq_u64( pSrc[1], c1 );
            pDst[2] = veorq_u64( pSrc[2], c2 );
            pDst[3] = veorq_u64( pSrc[3], c3 );
            pDst[4] = veorq_u64( pSrc[4], c4 );
            chain = VSUBQ_UXX( ctr5, chainIncrement8 );

            if( cbData >= 96 )
            {
                chain = VSUBQ_UXX( ctr6, chainIncrement8 );
                pDst[5] = veorq_u64( pSrc[5], c5 );
                if( cbData >= 112 )
                {
                    chain = VSUBQ_UXX( ctr7, chainIncrement8 );
                    pDst[6] = veorq_u64( pSrc[6], c6 );
                }
            }
        }
        else if( cbData >= 2 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            // Produce 4 blocks of key stream

            chain = ctr2;           // chain is only incremented by 2 for now

            c0 = vrev64q_u8( ctr0 );
            c1 = vrev64q_u8( ctr1 );
            c2 = vrev64q_u8( ctr2 );
            c3 = vrev64q_u8( ctr3 );

            AES_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3 );

            pDst[0] = veorq_u64( pSrc[0], c0 );
            pDst[1] = veorq_u64( pSrc[1], c1 );
            if( cbData >= 48 )
            {
                chain = ctr3;
                pDst[2] = veorq_u64( pSrc[2], c2 );
                if( cbData >= 64 )
                {
                    chain = ctr4;
                    pDst[3] = veorq_u64( pSrc[3], c3 );
                }
            }
        }
        else
        {
            // Exactly 1 block to process
            chain = ctr1;

            c0 = vrev64q_u8( ctr0 );

            AES_ENCRYPT_1( pExpandedKey, c0 );
            pDst[0] = veorq_u64( pSrc[0], c0 );
        }
    }
    else
    {
        chain = ctr0;
    }

    chain = vrev64q_u8( chain );
    *(__n128 *)pbChainingValue = chain;
}

#endif // SYMCRYPT_CPU_ARM64

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

VOID
SYMCRYPT_CALL
SYMCRYPT_AesCtrMsbXxXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i chain = _mm_loadu_si128( (__m128i *) pbChainingValue );

    __m128i BYTE_REVERSE_ORDER = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );

    __m128i chainIncrement1 = _mm_set_epi32( 0, 0, 0, 1 );
    __m128i chainIncrement2 = _mm_set_epi32( 0, 0, 0, 2 );
    __m128i chainIncrement3 = _mm_set_epi32( 0, 0, 0, 3 );
    //__m128i chainIncrement8 = _mm_set_epi32( 0, 0, 0, 8 );

    __m128i c0, c1, c2, c3, c4, c5, c6, c7;

    cbData &= ~(SYMCRYPT_AES_BLOCK_SIZE - 1);

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER );

/*
    while cbData >= 5 * block
        generate 8 blocks of key stream
        if cbData < 8 * block
            break;
        process 8 blocks
    if cbData >= 5 * block
        process 5-7 blocks
        done
    if cbData > 1 block
        generate 4 blocks of key stream
        process 2-4 blocks
        done
    if cbData == 1 block
        generate 1 block of key stream
        process block
*/
    while( cbData >= 5 * SYMCRYPT_AES_BLOCK_SIZE )
    {
        c0 = chain;
        c1 = MM_ADD_EPIXX( chain, chainIncrement1 );
        c2 = MM_ADD_EPIXX( chain, chainIncrement2 );
        c3 = MM_ADD_EPIXX( c1, chainIncrement2 );
        c4 = MM_ADD_EPIXX( c2, chainIncrement2 );
        c5 = MM_ADD_EPIXX( c3, chainIncrement2 );
        c6 = MM_ADD_EPIXX( c4, chainIncrement2 );
        c7 = MM_ADD_EPIXX( c5, chainIncrement2 );
        chain = MM_ADD_EPIXX( c6, chainIncrement2 );

        c0 = _mm_shuffle_epi8( c0, BYTE_REVERSE_ORDER );
        c1 = _mm_shuffle_epi8( c1, BYTE_REVERSE_ORDER );
        c2 = _mm_shuffle_epi8( c2, BYTE_REVERSE_ORDER );
        c3 = _mm_shuffle_epi8( c3, BYTE_REVERSE_ORDER );
        c4 = _mm_shuffle_epi8( c4, BYTE_REVERSE_ORDER );
        c5 = _mm_shuffle_epi8( c5, BYTE_REVERSE_ORDER );
        c6 = _mm_shuffle_epi8( c6, BYTE_REVERSE_ORDER );
        c7 = _mm_shuffle_epi8( c7, BYTE_REVERSE_ORDER );

        AES_ENCRYPT_8( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        if( cbData < 8 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            break;
        }

        _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0 ) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 16), _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc + 16 ) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 32), _mm_xor_si128( c2, _mm_loadu_si128( ( __m128i * ) (pbSrc + 32 ) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 48), _mm_xor_si128( c3, _mm_loadu_si128( ( __m128i * ) (pbSrc + 48 ) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 64), _mm_xor_si128( c4, _mm_loadu_si128( ( __m128i * ) (pbSrc + 64 ) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 80), _mm_xor_si128( c5, _mm_loadu_si128( ( __m128i * ) (pbSrc + 80 ) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst + 96), _mm_xor_si128( c6, _mm_loadu_si128( ( __m128i * ) (pbSrc + 96 ) ) ) );
        _mm_storeu_si128( (__m128i *) (pbDst +112), _mm_xor_si128( c7, _mm_loadu_si128( ( __m128i * ) (pbSrc +112 ) ) ) );
        pbDst  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        pbSrc  += 8 * SYMCRYPT_AES_BLOCK_SIZE;
        cbData -= 8 * SYMCRYPT_AES_BLOCK_SIZE;
    }

    //
    // At this point we have one of the two following cases:
    // - cbData >= 5 * 16 and we have 8 blocks of key stream in c0-c7. chain is set to c7 + 1
    // - cbData < 5 * 16 and we have no blocks of key stream, with chain the next value to use
    //

    if( cbData >= SYMCRYPT_AES_BLOCK_SIZE ) // quick exit of function if the request was a multiple of 8 blocks
    {
        if( cbData >= 5 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            //
            // We already have the key stream
            //
            _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0 ) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 16), _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc + 16 ) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 32), _mm_xor_si128( c2, _mm_loadu_si128( ( __m128i * ) (pbSrc + 32 ) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 48), _mm_xor_si128( c3, _mm_loadu_si128( ( __m128i * ) (pbSrc + 48 ) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 64), _mm_xor_si128( c4, _mm_loadu_si128( ( __m128i * ) (pbSrc + 64 ) ) ) );
            chain = MM_SUB_EPIXX( chain, chainIncrement3 );

            if( cbData >= 96 )
            {
                chain = MM_ADD_EPIXX( chain, chainIncrement1 );
                _mm_storeu_si128( (__m128i *) (pbDst + 80), _mm_xor_si128( c5, _mm_loadu_si128( ( __m128i * ) (pbSrc + 80 ) ) ) );
                if( cbData >= 112 )
                {
                    chain = MM_ADD_EPIXX( chain, chainIncrement1 );
                    _mm_storeu_si128( (__m128i *) (pbDst + 96), _mm_xor_si128( c6, _mm_loadu_si128( ( __m128i * ) (pbSrc + 96 ) ) ) );
                }
            }
        }
        else if( cbData >= 2 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            // Produce 4 blocks of key stream

            c0 = chain;
            c1 = MM_ADD_EPIXX( chain, chainIncrement1 );
            c2 = MM_ADD_EPIXX( chain, chainIncrement2 );
            c3 = MM_ADD_EPIXX( c1, chainIncrement2 );
            chain = c2;             // chain is only incremented by 2 for now

            c0 = _mm_shuffle_epi8( c0, BYTE_REVERSE_ORDER );
            c1 = _mm_shuffle_epi8( c1, BYTE_REVERSE_ORDER );
            c2 = _mm_shuffle_epi8( c2, BYTE_REVERSE_ORDER );
            c3 = _mm_shuffle_epi8( c3, BYTE_REVERSE_ORDER );

            AES_ENCRYPT_4( pExpandedKey, c0, c1, c2, c3 );

            _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0 ) ) ) );
            _mm_storeu_si128( (__m128i *) (pbDst + 16), _mm_xor_si128( c1, _mm_loadu_si128( ( __m128i * ) (pbSrc + 16 ) ) ) );
            if( cbData >= 48 )
            {
                chain = MM_ADD_EPIXX( chain, chainIncrement1 );
                _mm_storeu_si128( (__m128i *) (pbDst + 32), _mm_xor_si128( c2, _mm_loadu_si128( ( __m128i * ) (pbSrc + 32 ) ) ) );
                if( cbData >= 64 )
                {
                    chain = MM_ADD_EPIXX( chain, chainIncrement1 );
                    _mm_storeu_si128( (__m128i *) (pbDst + 48), _mm_xor_si128( c3, _mm_loadu_si128( ( __m128i * ) (pbSrc + 48 ) ) ) );
                }
            }
        }
        else
        {
            // Exactly 1 block to process
            c0 = chain;
            chain = MM_ADD_EPIXX( chain, chainIncrement1 );

            c0 = _mm_shuffle_epi8( c0, BYTE_REVERSE_ORDER );

            AES_ENCRYPT_1( pExpandedKey, c0 );
            _mm_storeu_si128( (__m128i *) (pbDst +  0), _mm_xor_si128( c0, _mm_loadu_si128( ( __m128i * ) (pbSrc +  0 ) ) ) );
        }
    }

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER );
    _mm_storeu_si128( (__m128i *) pbChainingValue, chain );
}

#endif // SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
