//
// xtsaes_definitions.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// Multiply by alpha
//
// <</>> indicate shifts on 128-bit values
// <<<</>>>> indicate shifts on 32-bit values (a word)
//

// Multiply by ALPHA
// Since there's no instruction to shift the 128 bit register left by one, the following shifts do the trick.
// All shifts are zero extended
// t1 = _in <<<< 1                          words shifted left by 1, this is almost a _in << 1 but there are
//                                          gaps at first bit of each word, the following two shifts fixes that.
// t2 = _in >>>> 31                         words shifted right by 31
// t1 = t1 ^ (t2 << 32)                     t1 = _in << 1, note ^ could be |
// Do the special case for first byte of _in where last carry means xor with 135 for first byte.
// t2 = t2 >> 96                            t2 = _in >> 127, i.e., last bit of _in is placed in first bit
// t2 = (t2 <<<< 7) + (t2 <<<<3) - (t2)     t2 = 135 if last bit of t2 is set
// res = t1 ^ t2
#define XTS_MUL_ALPHA_old( _in, _res ) \
{\
    __m128i _t1, _t2;\
\
    _t1 = _mm_slli_epi32( _in, 1 ); \
    _t2 = _mm_srli_epi32( _in, 31); \
    _t1 = _mm_xor_si128( _t1, _mm_slli_si128( _t2, 4 )); \
    _t2 = _mm_srli_si128( _t2, 12 ); \
    _t2 = _mm_sub_epi32( _mm_add_epi32( _mm_slli_epi32( _t2, 7 ), _mm_slli_epi32( _t2, 3 ) ), _t2 ); \
    _res = _mm_xor_si128( _t1, _t2 ); \
}

// An improved approach; use arithmetic shift-right to duplicate the carry-out, PSHUFD to re-arrange, and an AND to
// implement both the polynomial and mask the other words down to 1 bit again.

// __m128i XTS_ALPHA_MASK = _mm_set_epi32( 1, 1, 1, 0x87 );
#define XTS_MUL_ALPHA( _in, _res ) \
{\
    __m128i _t1, _t2;\
\
    _t1 = _mm_slli_epi32( _in, 1 ); \
    _t2 = _mm_srai_epi32( _in, 31); \
    _t2 = _mm_shuffle_epi32( _t2, _MM_SHUFFLE( 2, 1, 0, 3 ) ); \
    _t2 = _mm_and_si128( _t2, XTS_ALPHA_MASK ); \
    _res = _mm_xor_si128( _t1, _t2 ); \
}

// Like XTS_MUL_ALPHA_old but operate on __m512i for _in and _res.
// TODO: do this with VSHUFPS.
#define XTS_MUL_ALPHA_ZMM_old( _in, _res ) \
{\
    __m512i _t1, _t2;\
\
    _t1 = _mm512_slli_epi32( _in, 1 ); \
    _t2 = _mm512_srli_epi32( _in, 31); \
    _t1 = _mm512_xor_si512( _t1, _mm512_bslli_epi128( _t2, 4 )); \
    _t2 = _mm512_bsrli_epi128( _t2, 12 ); \
    _t2 = _mm512_sub_epi32( _mm512_add_epi32( _mm512_slli_epi32( _t2, 7 ), _mm512_slli_epi32( _t2, 3 ) ), _t2 ); \
    _res = _mm512_xor_si512( _t1, _t2 ); \
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
    __m128i _t1, _t2;\
\
    _t1 = _mm_slli_epi32( _in, 2 ); \
    _t2 = _mm_srli_epi32( _in, 30); \
    _t1 = _mm_xor_si128( _t1, _mm_slli_si128( _t2, 4 )); \
    _t2 = _mm_srli_si128( _t2, 12 ); \
    _t2 = _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_slli_epi32( _t2, 7 ), _mm_slli_epi32( _t2, 2 ) ), _mm_slli_epi32( _t2, 1 )), _t2 ); \
    _res = _mm_xor_si128( _t1, _t2 ); \
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
    __m128i _t1, _t2;\
\
    _t1 = _mm_slli_epi32( _in, 4 ); \
    _t2 = _mm_srli_epi32( _in, 28); \
    _t1 = _mm_xor_si128( _t1, _mm_slli_si128( _t2, 4 )); \
    _t2 = _mm_srli_si128( _t2, 12 ); \
    _t2 = _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_slli_epi32( _t2, 7 ), _mm_slli_epi32( _t2, 2 ) ), _mm_slli_epi32( _t2, 1 )), _t2 ); \
    _res = _mm_xor_si128( _t1, _t2 ); \
}

#define XTS_MUL_ALPHA5( _in, _res ) \
{\
    __m128i _t1, _t2;\
\
    _t1 = _mm_slli_epi32( _in, 5 ); \
    _t2 = _mm_srli_epi32( _in, 27); \
    _t1 = _mm_xor_si128( _t1, _mm_slli_si128( _t2, 4 )); \
    _t2 = _mm_srli_si128( _t2, 12 ); \
    _t2 = _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_slli_epi32( _t2, 7 ), _mm_slli_epi32( _t2, 2 ) ), _mm_slli_epi32( _t2, 1 )), _t2 ); \
    _res = _mm_xor_si128( _t1, _t2 ); \
}


// Multiply by ALPHA^8
// t2 = Input >> 120
// t2 = (t2 <<<< 7) ^ (t2 <<<< 2) ^ (t2 <<<< 1) ^ t2
// res = (Input << 8) ^ t2
//
// Only currently used with VPCLMULQDQ (in Ymm / Zmm versions) as support for non-vectorized PCLMULQDQ is not always supported with AESNI,
// and is sometimes slower than shift+xor

// __m256i XTS_ALPHA_MULTIPLIER_Ymm = _mm256_set_epi64x( 0, 0x87, 0, 0x87);
#define XTS_MUL_ALPHA8_YMM( _in, _res ) \
{\
    __m256i _t2;\
\
    _t2 = _mm256_srli_si256( _in, 15 ); /* AVX2 */ \
    _res = _mm256_slli_si256( _in, 1 ); \
    _t2 = _mm256_clmulepi64_epi128( _t2, XTS_ALPHA_MULTIPLIER_Ymm, 0x00 ); \
    _res = _mm256_xor_si256( _res, _t2 ); \
}

#define XTS_MUL_ALPHA16_YMM( _in, _res ) \
{\
    __m256i _t2;\
\
    _t2 = _mm256_srli_si256( _in, 14 ); /* AVX2 */ \
    _res = _mm256_slli_si256( _in, 2 ); \
    _t2 = _mm256_clmulepi64_epi128( _t2, XTS_ALPHA_MULTIPLIER_Ymm, 0x00 ); \
    _res = _mm256_xor_si256( _res, _t2 ); \
}

// __m512i XTS_ALPHA_MULTIPLIER_Zmm = _mm512_set_epi64( 0, 0x87, 0, 0x87, 0, 0x87, 0, 0x87 );
#define XTS_MUL_ALPHA8_ZMM( _in, _res ) \
{\
    __m512i _t2; \
\
    _t2 = _mm512_bsrli_epi128( _in, 15 ); \
    _res = _mm512_bslli_epi128( _in, 1 ); \
    _t2 = _mm512_clmulepi64_epi128( _t2, XTS_ALPHA_MULTIPLIER_Zmm, 0x00 ); \
    _res = _mm512_xor_si512( _res, _t2 ); \
}

#define XTS_MUL_ALPHA16_ZMM( _in, _res ) \
{\
    __m512i _t2; \
\
    _t2 = _mm512_bsrli_epi128( _in, 14 ); \
    _res = _mm512_bslli_epi128( _in, 2 ); \
    _t2 = _mm512_clmulepi64_epi128( _t2, XTS_ALPHA_MULTIPLIER_Zmm, 0x00 ); \
    _res = _mm512_xor_si512( _res, _t2 ); \
}

// Currently only use UINT64 for x86 and amd64 - this does regress perf on x86
// but we don't expect a lot of XTS in x86. If the regression causes any real problems
// we can consider introducing another variant. Not doing this now to avoid code bloat
#define XTS_MUL_ALPHA_Scalar( _inout_low_u64, _inout_high_u64 ) \
{ \
    UINT64 tmp = (UINT64) ((INT64)_inout_high_u64 >> 63); \
    \
    _inout_high_u64 = (_inout_high_u64 << 1) ^ (_inout_low_u64 >> 63); \
    _inout_low_u64 = (_inout_low_u64 << 1) ^ (tmp & 0x87); \
}
