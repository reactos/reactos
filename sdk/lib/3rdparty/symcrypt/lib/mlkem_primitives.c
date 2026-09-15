//
// mlkem_primitives.c   ML-KEM related functionality
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

//
// Current approach is to represent polynomial ring elements as a 512-byte buffer (256 UINT16s).
//

// Coefficients are added and subtracted when polynomials are in the NTT domain and in the lattice domain.
//
// Coefficients are only multiplied in the NTT/INTT operations, and in MulAdd which only operates on
// polynomials in NTT form.
// We choose to perform modular multiplication exclusively using Montgomery multiplication, that is, we choose
// a Montgomery divisor R, and modular multiplication always divides by R, as this make reduction logic easy
// and quick.
// i.e. MontMul(a,b) -> ((a*b) / R) mod Q
//
// For powers of Zeta used in as multiplication twiddle factors in NTT/INTT and base polynomial multiplication,
// we pre-multiply the constants by R s.t.
//  MontMul(x, twiddleForZetaToTheK) -> x*(Zeta^K) mod Q.
//
// Most other modular multiplication can be done with a fixup deferred until the INTT. The one exception is in key
// generation, where A o s + e = t, we need to pre-multiply s'

// R = 2^16
const UINT32 SYMCRYPT_MLKEM_Rlog2 = 16;
const UINT32 SYMCRYPT_MLKEM_Rmask = 0xffff;

// NegQInvModR = -Q^(-1) mod R
const UINT32 SYMCRYPT_MLKEM_NegQInvModR = 3327;

// Rsqr = R^2 = (1<<32) mod Q
const UINT32 SYMCRYPT_MLKEM_Rsqr = 1353;
// RsqrTimesNegQInvModR = R^2 = ((1<<32) mod Q) * -Q^(-1) mod R
const UINT32 SYMCRYPT_MLKEM_RsqrTimesNegQInvModR = 44983;

//
// Zeta tables.
// Zeta = 17, which is a primitive 256-th root of unity modulo Q
//
// In ML-KEM we use powers of zeta to convert to and from NTT form
// and to perform multiplication between polynomials in NTT form
//

// This table is a lookup for (Zeta^(BitRev(index)) * R) mod Q
// Used in NTT and INTT
// i.e. element 1 is Zeta^(BitRev(1)) * (2^16) mod Q == (17^64)*(2^16) mod 3329 == 2571
//
// MlKemZetaBitRevTimesR = [ (pow(17, bitRev(i), 3329) << 16) % 3329 for i in range(128) ]
const UINT16 MlKemZetaBitRevTimesR[128] =
{
    2285, 2571, 2970, 1812, 1493, 1422,  287,  202,
    3158,  622, 1577,  182,  962, 2127, 1855, 1468,
     573, 2004,  264,  383, 2500, 1458, 1727, 3199,
    2648, 1017,  732,  608, 1787,  411, 3124, 1758,
    1223,  652, 2777, 1015, 2036, 1491, 3047, 1785,
     516, 3321, 3009, 2663, 1711, 2167,  126, 1469,
    2476, 3239, 3058,  830,  107, 1908, 3082, 2378,
    2931,  961, 1821, 2604,  448, 2264,  677, 2054,
    2226,  430,  555,  843, 2078,  871, 1550,  105,
     422,  587,  177, 3094, 3038, 2869, 1574, 1653,
    3083,  778, 1159, 3182, 2552, 1483, 2727, 1119,
    1739,  644, 2457,  349,  418,  329, 3173, 3254,
     817, 1097,  603,  610, 1322, 2044, 1864,  384,
    2114, 3193, 1218, 1994, 2455,  220, 2142, 1670,
    2144, 1799, 2051,  794, 1819, 2475, 2459,  478,
    3221, 3021,  996,  991,  958, 1869, 1522, 1628,
};

// This table is a lookup for ((Zeta^(BitRev(index)) * R) mod Q) * -Q^(-1) mod R
// Used in NTT and INTT
//
// MlKemZetaBitRevTimesRTimesNegQInvModR = [ (((pow(17, bitRev(i), Q) << 16) % Q) * 3327) & 0xffff for i in range(128) ]
const UINT16 MlKemZetaBitRevTimesRTimesNegQInvModR[128] =
{
       19, 34037, 50790, 64748, 52011, 12402, 37345, 16694,
    20906, 37778,  3799, 15690, 54846, 64177, 11201, 34372,
     5827, 48172, 26360, 29057, 59964,  1102, 44097, 26241,
    28072, 41223, 10532, 56736, 47109, 56677, 38860, 16162,
     5689,  6516, 64039, 34569, 23564, 45357, 44825, 40455,
    12796, 38919, 49471, 12441, 56401,   649, 25986, 37699,
    45652, 28249, 15886,  8898, 28309, 56460, 30198, 47286,
    52109, 51519, 29155, 12756, 48704, 61224, 24155, 17914,
      334, 54354, 11477, 52149, 32226, 14233, 45042, 21655,
    27738, 52405, 64591,  4586, 14882, 42443, 59354, 60043,
    33525, 32502, 54905, 35218, 36360, 18741, 28761, 52897,
    18485, 45436, 47975, 47011, 14430, 46007,  5275, 12618,
    31183, 45239, 40101, 63390,  7382, 50180, 41144, 32384,
    20926,  6279, 54590, 14902, 41321, 11044, 48546, 51066,
    55200, 21497,  7933, 20198, 22501, 42325, 54629, 17442,
    33899, 23859, 36892, 20257, 41538, 57779, 17422, 42404,
};

// This table is a lookup for ((Zeta^(2*BitRev(index) + 1) * R) mod Q)
// Used in multiplication of 2 NTT-form polynomials
//
// zetaTwoTimesBitRevPlus1TimesR =  [ (pow(17, 2*bitRev(i)+1, 3329) << 16) % 3329 for i in range(128) ]
const UINT16 zetaTwoTimesBitRevPlus1TimesR[128] =
{
    2226, 1103,  430, 2899,  555, 2774,  843, 2486,
    2078, 1251,  871, 2458, 1550, 1779,  105, 3224,
     422, 2907,  587, 2742,  177, 3152, 3094,  235,
    3038,  291, 2869,  460, 1574, 1755, 1653, 1676,
    3083,  246,  778, 2551, 1159, 2170, 3182,  147,
    2552,  777, 1483, 1846, 2727,  602, 1119, 2210,
    1739, 1590,  644, 2685, 2457,  872,  349, 2980,
     418, 2911,  329, 3000, 3173,  156, 3254,   75,
     817, 2512, 1097, 2232,  603, 2726,  610, 2719,
    1322, 2007, 2044, 1285, 1864, 1465,  384, 2945,
    2114, 1215, 3193,  136, 1218, 2111, 1994, 1335,
    2455,  874,  220, 3109, 2142, 1187, 1670, 1659,
    2144, 1185, 1799, 1530, 2051, 1278,  794, 2535,
    1819, 1510, 2475,  854, 2459,  870,  478, 2851,
    3221,  108, 3021,  308,  996, 2333,  991, 2338,
     958, 2371, 1869, 1460, 1522, 1807, 1628, 1701,
};

PSYMCRYPT_MLKEM_POLYELEMENT
SYMCRYPT_CALL
SymCryptMlKemPolyElementCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    UINT32  cbBuffer )
{
    PSYMCRYPT_MLKEM_POLYELEMENT pDst = (PSYMCRYPT_MLKEM_POLYELEMENT) pbBuffer;

    UNREFERENCED_PARAMETER( cbBuffer );

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );
    SYMCRYPT_ASSERT( cbBuffer == SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT );

    return pDst;
}

PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR
SYMCRYPT_CALL
SymCryptMlKemPolyElementAccumulatorCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    UINT32  cbBuffer )
{
    PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR pDst = (PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR) pbBuffer;

    UNREFERENCED_PARAMETER( cbBuffer );

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );
    SYMCRYPT_ASSERT( cbBuffer == SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT_ACCUMULATOR );

    return pDst;
}

PSYMCRYPT_MLKEM_VECTOR
SYMCRYPT_CALL
SymCryptMlKemVectorCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    UINT32  cbBuffer,
                                    UINT32  nRows )
{
    PSYMCRYPT_MLKEM_VECTOR pDst = NULL;
    PSYMCRYPT_MLKEM_VECTOR pVector = (PSYMCRYPT_MLKEM_VECTOR)pbBuffer;
    PSYMCRYPT_MLKEM_POLYELEMENT peTmp = NULL;
    UINT32 i;
    PBYTE pbTmp = pbBuffer + sizeof(SYMCRYPT_MLKEM_VECTOR);

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );

    SYMCRYPT_ASSERT( nRows > 0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );

    pVector->nRows = nRows;
    pVector->cbTotalSize = cbBuffer;

    for( i=0; i<nRows; i++ )
    {
        peTmp = SymCryptMlKemPolyElementCreate( pbTmp, SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT );
        if( peTmp == NULL )
        {
            goto cleanup;
        }

        pbTmp += SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT;
    }

    SYMCRYPT_ASSERT( pbTmp == (pbBuffer + cbBuffer) );

    pDst = pVector;

cleanup:
    return pDst;
}

PSYMCRYPT_MLKEM_MATRIX
SYMCRYPT_CALL
SymCryptMlKemMatrixCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    UINT32  cbBuffer,
                                    UINT32  nRows )
{
    PSYMCRYPT_MLKEM_MATRIX pDst = NULL;
    PSYMCRYPT_MLKEM_MATRIX pMatrix = (PSYMCRYPT_MLKEM_MATRIX)pbBuffer;
    UINT32 i;
    PBYTE pbTmp = pbBuffer + sizeof(SYMCRYPT_MLKEM_MATRIX);

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pbBuffer );

    SYMCRYPT_ASSERT( nRows > 0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );

    pMatrix->nRows = nRows;
    pMatrix->cbTotalSize = cbBuffer;

    for( i=0; i<(nRows*nRows); i++ )
    {
        pMatrix->apPolyElements[i] = SymCryptMlKemPolyElementCreate( pbTmp, SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT );
        if( pMatrix->apPolyElements[i] == NULL )
        {
            goto cleanup;
        }

        pbTmp += SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT;
    }

    SYMCRYPT_ASSERT( pbTmp == (pbBuffer + cbBuffer) );

    pDst = pMatrix;

cleanup:
    return pDst;
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM64

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("sse2"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("sse2")
#endif

#define VEC128_TYPE_UINT16 __m128i

#define VEC128_LOAD_UINT16( addr )       _mm_loadu_si128( (__m128i*) (addr) )
#define VEC64_LOAD_UINT16( addr )        _mm_loadl_epi64( (__m128i const*) (addr) )
#define VEC32_LOAD_UINT16( addr )        _mm_cvtsi32_si128( SYMCRYPT_LOAD_LSBFIRST32( addr ) )

#define VEC128_STORE_UINT16( addr, vec ) _mm_storeu_si128( (__m128i*) (addr), (vec) )
#define VEC64_STORE_UINT16( addr, vec )  _mm_storel_epi64( (__m128i*) (addr), (vec) )
#define VEC32_STORE_UINT16( addr, vec )  SYMCRYPT_STORE_LSBFIRST32( (addr), _mm_cvtsi128_si32( vec ) )

#define VEC128_SET_UINT16( value )       _mm_set1_epi16( (value) )

#define VEC128_MOD_SUB_UINT16( res, a, b, Q, zero, tmp1 ) \
    /* res = a - b */ \
    res = _mm_sub_epi16( a, b ); \
    /* tmp1 = (a - b) < 0 ? -1 : 0 */ \
    tmp1 = _mm_cmpgt_epi16( zero, res ); \
    /* tmp1 = (a - b) < 0 ? Q : 0 */ \
    tmp1 = _mm_and_si128( tmp1, Q ); \
    /* res = (a - b) mod Q */ \
    res = _mm_add_epi16( res, tmp1 );

#define VEC128_MOD_ADD_UINT16( res, a, b, Q, tmp1 ) \
    /* res = a + b */ \
    res = _mm_add_epi16( a, b ); \
    /* tmp1 = (a + b) < Q ? -1 : 0 */ \
    tmp1 = _mm_cmpgt_epi16( Q, res ); \
    /* tmp1 = (a + b) < Q ? 0 : Q */ \
    tmp1 = _mm_andnot_si128( tmp1, Q ); \
    /* res = (a + b) mod Q */ \
    res = _mm_sub_epi16( res, tmp1 );

#define VEC128_MONTGOMERY_MUL_UINT16( res, a, b, bTimesNegQInvModR, Q, zero, one, tmp1, tmp2 ) \
    /* tmp1 = a *low  bTimesNegQInvModR */ \
    tmp1 = _mm_mullo_epi16( a, bTimesNegQInvModR ); \
    /* res  = a *high b */ \
    res = _mm_mulhi_epu16( a, b ); \
    /* tmp2 = (tmp1 == 0) ? -1 : 0 */ \
    tmp2 = _mm_cmpeq_epi16( tmp1, zero ); \
    /* tmp1 = (a *low bTimesNegQInvModR) *high Q */ \
    tmp1 = _mm_mulhi_epu16( tmp1, Q ); \
    /* res = a *high b + 1 */ \
    res = _mm_add_epi16( res, one ); \
    /* res  = a *high b (+ 1 if a != 0) */ \
    res = _mm_add_epi16( res, tmp2 ); \
    /* res  = a *high b + inv*Q (+ 1 if a != 0) */ \
    res = _mm_add_epi16( res, tmp1 ); \
    /* res  = (a*b + inv*Q >> 16) mod Q */ \
    VEC128_MOD_SUB_UINT16( res, res, Q, Q, zero, tmp1 );

#elif SYMCRYPT_CPU_ARM64

#define VEC128_TYPE_UINT16 uint16x8_t

#define VEC128_LOAD_UINT16( addr )       vld1q_u16( addr )
#define VEC64_LOAD_UINT16( addr )        vld1q_dup_u64( addr )
#define VEC32_LOAD_UINT16( addr )        vld1q_dup_u32( addr )

#define VEC128_STORE_UINT16( addr, vec ) vst1q_u16( (addr), (vec) )
#define VEC64_STORE_UINT16( addr, vec )  vst1_u16( (uint16_t*) (addr), vget_low_u16(vec) )
#define VEC32_STORE_UINT16( addr, vec )  vst1_lane_u32( (PBYTE) (addr), vget_low_u32(vec), 0 )

#define VEC128_SET_UINT16( value )       vdupq_n_u16( (value) )

#define VEC128_MOD_SUB_UINT16( res, a, b, Q, zero, tmp1 ) \
    /* res = a - b */ \
    res = vsubq_u16( a, b ); \
    /* tmp1 = (a - b) < 0 ? -1 : 0 */ \
    tmp1 = vcltzq_s16( res ); \
    /* tmp1 = (a - b) < 0 ? Q : 0 */ \
    tmp1 = vandq_u16( tmp1, Q ); \
    /* res = (a - b) mod Q */ \
    res = vaddq_u16( res, tmp1 );

#define VEC128_MOD_ADD_UINT16( res, a, b, Q, tmp1 ) \
    /* res = a + b */ \
    res = vaddq_u16( a, b ); \
    /* tmp1 = (a + b) >= Q ? -1 : 0 */ \
    tmp1 = vcgeq_u16( res, Q ); \
    /* tmp1 = (a + b) >= Q ? Q : 0 */ \
    tmp1 = vandq_u16( tmp1, Q ); \
    /* res = (a + b) mod Q */ \
    res = vsubq_u16( res, tmp1 );

#define VEC128_MONTGOMERY_MUL_UINT16( res, a, b, bTimesNegQInvModR, Q, zero, one, tmp1, tmp2 ) \
    /* tmp1 = a *low  bTimesNegQInvModR */ \
    tmp1 = vmulq_u16( a, bTimesNegQInvModR ); \
    /* tmp2 = a*b [0-3]*/ \
    tmp2  = vmull_u16( vget_low_u16(a), vget_low_u16(b) ); \
    /* res  = a*b [4-7]*/ \
    res = vmull_high_u16( a, b ); \
    /* tmp2 = a*b + inv*Q [0-3]*/ \
    tmp2  = vmlal_u16( tmp2, vget_low_u16(tmp1), vget_low_u16(Q) ); \
    /* res  = a*b + inv*Q [4-7]*/ \
    res = vmlal_high_u16( res, tmp1, Q ); \
    /* res  = a*b + inv*Q >> 16 */ \
    res  = vuzp2q_u16( tmp2, res ); \
    /* res  = (a*b + inv*Q >> 16) mod Q */ \
    VEC128_MOD_SUB_UINT16( res, res, Q, Q, zero, tmp1 );

#endif

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementNTTLayerVec128(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT  peSrc,
            UINT32                       k,
            UINT32                       len )
{
    UINT32 start, j;
    VEC128_TYPE_UINT16 vc0, vc1, vTmp0, vTmp1, vc1Twiddle, vTwiddleFactor, vTwiddleFactorMont, vQ, vZero, vOne;

    SYMCRYPT_ASSERT( len >= 2 );

    vQ = VEC128_SET_UINT16( SYMCRYPT_MLKEM_Q );
    vZero = VEC128_SET_UINT16( 0 );
    vOne = VEC128_SET_UINT16( 1 );

    for( start=0; start<256; start+=(2*len) )
    {
        vTwiddleFactor     = VEC128_SET_UINT16( MlKemZetaBitRevTimesR[k] );
        vTwiddleFactorMont = VEC128_SET_UINT16( MlKemZetaBitRevTimesRTimesNegQInvModR[k] );
        k++;
        for( j=0; j<len; j+=8 )
        {
            if( len >= 8 )
            {
                vc0 = VEC128_LOAD_UINT16( &(peSrc->coeffs[start+j]    ) );
                vc1 = VEC128_LOAD_UINT16( &(peSrc->coeffs[start+j+len]) );
            }
            else if ( len == 4 )
            {
                vc0 = VEC64_LOAD_UINT16( &(peSrc->coeffs[start+j]    ) );
                vc1 = VEC64_LOAD_UINT16( &(peSrc->coeffs[start+j+len]) );
            }
            else /*if ( len == 2 )*/
            {
                vc0 = VEC32_LOAD_UINT16( &(peSrc->coeffs[start+j]    ) );
                vc1 = VEC32_LOAD_UINT16( &(peSrc->coeffs[start+j+len]) );
            }

            // c1TimesTwiddle = twiddleFactor * c1 mod Q;
            VEC128_MONTGOMERY_MUL_UINT16( vc1Twiddle, vc1, vTwiddleFactor, vTwiddleFactorMont, vQ, vZero, vOne, vTmp0, vTmp1 );
            // c1 = c0 - c1TimesTwiddle mod Q
            VEC128_MOD_SUB_UINT16( vc1, vc0, vc1Twiddle, vQ, vZero, vTmp0 );
            // c0 = c0 + c1TimesTwiddle mod Q
            VEC128_MOD_ADD_UINT16( vc0, vc0, vc1Twiddle, vQ, vTmp1 );

            if( len >= 8 )
            {
                VEC128_STORE_UINT16( &(peSrc->coeffs[start+j]    ), vc0 );
                VEC128_STORE_UINT16( &(peSrc->coeffs[start+j+len]), vc1 );
            }
            else if ( len == 4 )
            {
                VEC64_STORE_UINT16( &(peSrc->coeffs[start+j]    ), vc0 );
                VEC64_STORE_UINT16( &(peSrc->coeffs[start+j+len]), vc1 );
            }
            else /*if ( len == 2 )*/
            {
                VEC32_STORE_UINT16( &(peSrc->coeffs[start+j]    ), vc0 );
                VEC32_STORE_UINT16( &(peSrc->coeffs[start+j+len]), vc1 );
            }
        }
    }
}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementINTTLayerVec128(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT  peSrc,
            UINT32                       k,
            UINT32                       len )
{
    UINT32 start, j;
    VEC128_TYPE_UINT16 vc0, vc1, vTmp0, vTmp1, vTmp2, vTwiddleFactor, vTwiddleFactorMont, vQ, vZero, vOne;

    SYMCRYPT_ASSERT( len >= 2 );

    vQ = VEC128_SET_UINT16( SYMCRYPT_MLKEM_Q );
    vZero = VEC128_SET_UINT16( 0 );
    vOne = VEC128_SET_UINT16( 1 );

    for( start=0; start<256; start+=(2*len) )
    {
        vTwiddleFactor     = VEC128_SET_UINT16( MlKemZetaBitRevTimesR[k] );
        vTwiddleFactorMont = VEC128_SET_UINT16( MlKemZetaBitRevTimesRTimesNegQInvModR[k] );
        k--;
        for( j=0; j<len; j+=8 )
        {
            if( len >= 8 )
            {
                vc0 = VEC128_LOAD_UINT16( &(peSrc->coeffs[start+j]    ) );
                vc1 = VEC128_LOAD_UINT16( &(peSrc->coeffs[start+j+len]) );
            }
            else if ( len == 4 )
            {
                vc0 = VEC64_LOAD_UINT16( &(peSrc->coeffs[start+j]    ) );
                vc1 = VEC64_LOAD_UINT16( &(peSrc->coeffs[start+j+len]) );
            }
            else /*if ( len == 2 )*/
            {
                vc0 = VEC32_LOAD_UINT16( &(peSrc->coeffs[start+j]    ) );
                vc1 = VEC32_LOAD_UINT16( &(peSrc->coeffs[start+j+len]) );
            }

            // tmp = c0 + c1 mod Q
            VEC128_MOD_ADD_UINT16( vTmp2, vc0, vc1, vQ, vTmp0 );
            // c1 = c1 - c0 mod Q
            VEC128_MOD_SUB_UINT16( vc1, vc1, vc0, vQ, vZero, vTmp1 );
            // c1 = twiddleFactor * c1;
            VEC128_MONTGOMERY_MUL_UINT16( vc1, vc1, vTwiddleFactor, vTwiddleFactorMont, vQ, vZero, vOne, vTmp0, vTmp1 );

            if( len >= 8 )
            {
                VEC128_STORE_UINT16( &(peSrc->coeffs[start+j]    ), vTmp2 );
                VEC128_STORE_UINT16( &(peSrc->coeffs[start+j+len]), vc1 );
            }
            else if ( len == 4 )
            {
                VEC64_STORE_UINT16( &(peSrc->coeffs[start+j]    ), vTmp2 );
                VEC64_STORE_UINT16( &(peSrc->coeffs[start+j+len]), vc1 );
            }
            else /*if ( len == 2 )*/
            {
                VEC32_STORE_UINT16( &(peSrc->coeffs[start+j]    ), vTmp2 );
                VEC32_STORE_UINT16( &(peSrc->coeffs[start+j+len]), vc1 );
            }
        }
    }
}

#endif

FORCEINLINE
UINT32
SYMCRYPT_CALL
SymCryptMlKemModAdd(
    UINT32 a,
    UINT32 b )
{
    UINT32 res;

    SYMCRYPT_ASSERT( a < SYMCRYPT_MLKEM_Q );
    SYMCRYPT_ASSERT( b < SYMCRYPT_MLKEM_Q );

    res = a + b - SYMCRYPT_MLKEM_Q;
    SYMCRYPT_ASSERT( ((res >> 16) == 0) || ((res >> 16) == 0xffff) );
    res = res + (SYMCRYPT_MLKEM_Q & (res >> 16));
    SYMCRYPT_ASSERT( res < SYMCRYPT_MLKEM_Q );

    return res;
}

FORCEINLINE
UINT32
SYMCRYPT_CALL
SymCryptMlKemModSub(
    UINT32 a,
    UINT32 b )
{
    UINT32 res;

    SYMCRYPT_ASSERT( a < 2*SYMCRYPT_MLKEM_Q );
    SYMCRYPT_ASSERT( b <= SYMCRYPT_MLKEM_Q );

    res = a - b;
    SYMCRYPT_ASSERT( ((res >> 16) == 0) || ((res >> 16) == 0xffff) );
    res = res + (SYMCRYPT_MLKEM_Q & (res >> 16));
    SYMCRYPT_ASSERT( res < SYMCRYPT_MLKEM_Q );

    return res;
}

FORCEINLINE
UINT32
SYMCRYPT_CALL
SymCryptMlKemMontMul(
    UINT32 a,
    UINT32 b,
    UINT32 bMont )
{
    UINT32 res, inv;

    SYMCRYPT_ASSERT( a < SYMCRYPT_MLKEM_Q );
    SYMCRYPT_ASSERT( b < SYMCRYPT_MLKEM_Q );
    SYMCRYPT_ASSERT( bMont <= SYMCRYPT_MLKEM_Rmask );
    SYMCRYPT_ASSERT( bMont == ((b * SYMCRYPT_MLKEM_NegQInvModR) & SYMCRYPT_MLKEM_Rmask) );

    res = a * b;
    inv = (a * bMont) & SYMCRYPT_MLKEM_Rmask;
    res += inv * SYMCRYPT_MLKEM_Q;
    SYMCRYPT_ASSERT( (res & SYMCRYPT_MLKEM_Rmask) == 0 );
    res = res >> SYMCRYPT_MLKEM_Rlog2;

    return SymCryptMlKemModSub( res, SYMCRYPT_MLKEM_Q );
}

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementNTTLayerC(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT  peSrc,
            UINT32                       k,
            UINT32                       len )
{
    UINT32 start, j;
    UINT32 twiddleFactor, twiddleFactorMont, c0, c1, c1TimesTwiddle;

    for( start=0; start<256; start+=(2*len) )
    {
        twiddleFactor = MlKemZetaBitRevTimesR[k];
        twiddleFactorMont = MlKemZetaBitRevTimesRTimesNegQInvModR[k];
        k++;
        for( j=0; j<len; j++ )
        {
            c0 = peSrc->coeffs[start+j];
            SYMCRYPT_ASSERT( c0 < SYMCRYPT_MLKEM_Q );
            c1 = peSrc->coeffs[start+j+len];
            SYMCRYPT_ASSERT( c1 < SYMCRYPT_MLKEM_Q );

            c1TimesTwiddle = SymCryptMlKemMontMul( c1, twiddleFactor, twiddleFactorMont );
            c1 = SymCryptMlKemModSub( c0, c1TimesTwiddle );
            c0 = SymCryptMlKemModAdd( c0, c1TimesTwiddle );

            peSrc->coeffs[start+j]      = (UINT16) c0;
            peSrc->coeffs[start+j+len]  = (UINT16) c1;
        }
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementINTTLayerC(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT  peSrc,
            UINT32                       k,
            UINT32                       len )
{
    UINT32 start, j;
    UINT32 twiddleFactor, twiddleFactorMont, c0, c1, tmp;

    for( start=0; start<256; start+=(2*len) )
    {
        twiddleFactor = MlKemZetaBitRevTimesR[k];
        twiddleFactorMont = MlKemZetaBitRevTimesRTimesNegQInvModR[k];
        k--;
        for( j=0; j<len; j++ )
        {
            c0 = peSrc->coeffs[start+j];
            SYMCRYPT_ASSERT( c0 < SYMCRYPT_MLKEM_Q );
            c1 = peSrc->coeffs[start+j+len];
            SYMCRYPT_ASSERT( c1 < SYMCRYPT_MLKEM_Q );

            tmp = SymCryptMlKemModAdd( c0, c1 );
            c1 = SymCryptMlKemModSub( c1, c0 );
            c1 = SymCryptMlKemMontMul( c1, twiddleFactor, twiddleFactorMont );

            peSrc->coeffs[start+j]      = (UINT16) tmp;
            peSrc->coeffs[start+j+len]  = (UINT16) c1;
        }
    }
}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementNTTLayer(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT  peSrc,
            UINT32                       k,
            UINT32                       len )
{
#if SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSE2 ) && SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptMlKemPolyElementNTTLayerVec128( peSrc, k, len );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptMlKemPolyElementNTTLayerC( peSrc, k, len );
    }
#elif SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSE2 ) )
    {
        SymCryptMlKemPolyElementNTTLayerVec128( peSrc, k, len );
    } else {
        SymCryptMlKemPolyElementNTTLayerC( peSrc, k, len );
    }
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON ) )
    {
        SymCryptMlKemPolyElementNTTLayerVec128( peSrc, k, len );
    } else {
        SymCryptMlKemPolyElementNTTLayerC( peSrc, k, len );
    }
#else
    SymCryptMlKemPolyElementNTTLayerC( peSrc, k, len );
#endif
}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementINTTLayer(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT  peSrc,
            UINT32                       k,
            UINT32                       len )
{
#if SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA  SaveData;
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSE2 ) && SymCryptSaveXmm( &SaveData ) == SYMCRYPT_NO_ERROR )
    {
        SymCryptMlKemPolyElementINTTLayerVec128( peSrc, k, len );
        SymCryptRestoreXmm( &SaveData );
    } else {
        SymCryptMlKemPolyElementINTTLayerC( peSrc, k, len );
    }
#elif SYMCRYPT_CPU_AMD64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSE2 ) )
    {
        SymCryptMlKemPolyElementINTTLayerVec128( peSrc, k, len );
    } else {
        SymCryptMlKemPolyElementINTTLayerC( peSrc, k, len );
    }
#elif SYMCRYPT_CPU_ARM64
    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON ) )
    {
        SymCryptMlKemPolyElementINTTLayerVec128( peSrc, k, len );
    } else {
        SymCryptMlKemPolyElementINTTLayerC( peSrc, k, len );
    }
#else
    SymCryptMlKemPolyElementINTTLayerC( peSrc, k, len );
#endif
}

#define SYMCRYPT_MLKEM_MaxCoeff                   (SYMCRYPT_MLKEM_Q - 1)
#define SYMCRYPT_MLKEM_MaxCoeffProduct            (SYMCRYPT_MLKEM_MaxCoeff*SYMCRYPT_MLKEM_MaxCoeff)

// max([ ((i*j) + ((((i*j)*NegQInvModR) & Rmask)*Q)) >> Rlog2 for i in range(Q) for j in range(Q) ])
#define SYMCRYPT_MLKEM_MaxFirstStepReduction      (3494)
// max([ ( pow(17, (2*i)+1, Q) << Rlog2 ) % Q for i in range(128) ])
#define SYMCRYPT_MLKEM_MaxZetaTwoTimesPlus1TimesR (3254)
#define SYMCRYPT_MLKEM_MaxA1B1ZetaPow             (SYMCRYPT_MLKEM_MaxFirstStepReduction*SYMCRYPT_MLKEM_MaxZetaTwoTimesPlus1TimesR)

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementMulAndAccumulate(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT            peSrc1,
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT            peSrc2,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paDst )
{
    UINT32 i;
    UINT32 a0, a1, b0, b1, c0, c1;
    UINT32 a0b0, a1b1, a0b1, a1b0, a1b1zetapow, inv;

    for( i=0; i<(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 2); i++ )
    {
        a0 = peSrc1->coeffs[(2*i)  ];
        SYMCRYPT_ASSERT( a0 < SYMCRYPT_MLKEM_Q );
        a1 = peSrc1->coeffs[(2*i)+1];
        SYMCRYPT_ASSERT( a1 < SYMCRYPT_MLKEM_Q );

        b0 = peSrc2->coeffs[(2*i)  ];
        SYMCRYPT_ASSERT( b0 < SYMCRYPT_MLKEM_Q );
        b1 = peSrc2->coeffs[(2*i)+1];
        SYMCRYPT_ASSERT( b1 < SYMCRYPT_MLKEM_Q );

        c0 = paDst->coeffs[(2*i)  ];
        SYMCRYPT_ASSERT( c0 <= 3*(SYMCRYPT_MLKEM_MaxCoeffProduct + SYMCRYPT_MLKEM_MaxA1B1ZetaPow) );
        c1 = paDst->coeffs[(2*i)+1];
        SYMCRYPT_ASSERT( c1 <= 3*(SYMCRYPT_MLKEM_MaxCoeffProduct + SYMCRYPT_MLKEM_MaxA1B1ZetaPow) );

        // multiplication results in range [0, MaxCoeffProduct = 3328*3328]
        a0b0 = a0 * b0;
        a1b1 = a1 * b1;
        a0b1 = a0 * b1;
        a1b0 = a1 * b0;

        // we need a1*b1*zetaTwoTimesBitRevPlus1TimesR[i]
        // eagerly reduce a1*b1 with montgomery reduction
        // a1b1 = red(a1*b1) -> range [0, MaxFirstStepReduction = 3494]
        //   (3494 is maximum result of first step of montgomery reduction of x*y for x,y in [0, 3328])
        // we do not need to do final reduction yet
        inv = (a1b1 * SYMCRYPT_MLKEM_NegQInvModR) & SYMCRYPT_MLKEM_Rmask;
        a1b1 = (a1b1 + (inv * SYMCRYPT_MLKEM_Q)) >> SYMCRYPT_MLKEM_Rlog2; // in range [0, MaxFirstStepReduction]
        SYMCRYPT_ASSERT( a1b1 <= SYMCRYPT_MLKEM_MaxFirstStepReduction );

        // now multiply a1b1 by power of zeta
        a1b1zetapow = a1b1 * zetaTwoTimesBitRevPlus1TimesR[i];
        // MaxZetaTwoTimesPlus1TimesR = 3254
        // MaxA1B1ZetaPow = MaxFirstStepReduction*MaxZetaTwoTimesPlus1TimesR = 3494*3254
        SYMCRYPT_ASSERT( a1b1zetapow <= SYMCRYPT_MLKEM_MaxA1B1ZetaPow );

        // sum pairs of products
        a0b0 += a1b1zetapow;    // a0*b0 + red(a1*b1)*zetapower in range [0, MaxCoeffProduct + MaxA1B1ZetaPow]
        SYMCRYPT_ASSERT( a0b0 <= SYMCRYPT_MLKEM_MaxCoeffProduct + SYMCRYPT_MLKEM_MaxA1B1ZetaPow );
        a0b1 += a1b0;           // a0*b1 + a1*b0                in range [0, 2*MaxCoeffProduct]
        SYMCRYPT_ASSERT( a0b1 <= 2*SYMCRYPT_MLKEM_MaxCoeffProduct );

        // We sum at most 4 pairs of products into an accumulator in ML-KEM
        C_ASSERT( SYMCRYPT_MLKEM_MATRIX_MAX_NROWS <= 4 );
        c0 += a0b0; // in range [0,4*MaxCoeffProduct + 4*MaxA1B1ZetaPow]
        SYMCRYPT_ASSERT( c0 < (4*SYMCRYPT_MLKEM_MaxCoeffProduct) + (4*SYMCRYPT_MLKEM_MaxA1B1ZetaPow) );
        c1 += a0b1; // in range [0,5*MaxCoeffProduct + 3*MaxA1B1ZetaPow]
        SYMCRYPT_ASSERT( c1 < (5*SYMCRYPT_MLKEM_MaxCoeffProduct) + (3*SYMCRYPT_MLKEM_MaxA1B1ZetaPow) );

        paDst->coeffs[(2*i)  ] = c0;
        paDst->coeffs[(2*i)+1] = c1;
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemMontgomeryReduceAndAddPolyElementAccumulatorToPolyElement(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paSrc,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT             peDst )
{
    UINT32 i;
    UINT32 a, c, inv;

    for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        a = paSrc->coeffs[i];
        SYMCRYPT_ASSERT( a <= 4*(SYMCRYPT_MLKEM_MaxCoeffProduct + SYMCRYPT_MLKEM_MaxA1B1ZetaPow) );
        paSrc->coeffs[i] = 0;

        c = peDst->coeffs[i];
        SYMCRYPT_ASSERT( c < SYMCRYPT_MLKEM_Q );

        // montgomery reduce sum of products
        inv = (a * SYMCRYPT_MLKEM_NegQInvModR) & SYMCRYPT_MLKEM_Rmask;
        a = (a + (inv * SYMCRYPT_MLKEM_Q)) >> SYMCRYPT_MLKEM_Rlog2; // in range [0, 4711]
        SYMCRYPT_ASSERT( a <= 4711 );

        // add destination
        c += a;
        SYMCRYPT_ASSERT( c <= 8039 );

        // subtraction and conditional additions for constant time range reduction
        c -= 2*SYMCRYPT_MLKEM_Q;           // in range [-2Q, 1381]
        SYMCRYPT_ASSERT( (c >= ((UINT32)(-2*SYMCRYPT_MLKEM_Q))) || (c < 1381) );
        c += SYMCRYPT_MLKEM_Q & (c >> 16); // in range [-Q, Q-1]
        SYMCRYPT_ASSERT( (c >= ((UINT32)-SYMCRYPT_MLKEM_Q)) || (c < SYMCRYPT_MLKEM_Q) );
        c += SYMCRYPT_MLKEM_Q & (c >> 16); // in range [0, Q-1]
        SYMCRYPT_ASSERT( c < SYMCRYPT_MLKEM_Q );

        peDst->coeffs[i] = (UINT16) c;
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementMulR(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT    peSrc,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT     peDst )
{
    UINT32 i;
    for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        peDst->coeffs[i] = (UINT16) SymCryptMlKemMontMul(
            peSrc->coeffs[i], SYMCRYPT_MLKEM_Rsqr, SYMCRYPT_MLKEM_RsqrTimesNegQInvModR );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementAdd(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc1,
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc2,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT  peDst )
{
    UINT32 i;
    for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        peDst->coeffs[i] = (UINT16) SymCryptMlKemModAdd( peSrc1->coeffs[i], peSrc2->coeffs[i] );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementSub(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc1,
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc2,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT  peDst )
{
    UINT32 i;
    for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        peDst->coeffs[i] = (UINT16) SymCryptMlKemModSub( peSrc1->coeffs[i], peSrc2->coeffs[i] );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementNTT(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT  peSrc )
{
    SymCryptMlKemPolyElementNTTLayer( peSrc,  1, 128 );
    SymCryptMlKemPolyElementNTTLayer( peSrc,  2,  64 );
    SymCryptMlKemPolyElementNTTLayer( peSrc,  4,  32 );
    SymCryptMlKemPolyElementNTTLayer( peSrc,  8,  16 );
    SymCryptMlKemPolyElementNTTLayer( peSrc, 16,   8 );
    SymCryptMlKemPolyElementNTTLayer( peSrc, 32,   4 );
    SymCryptMlKemPolyElementNTTLayer( peSrc, 64,   2 );
}

// INTTFixupTimesRsqr = R^2 * 3303 = (3303<<32) mod Q
// 3303 constant is fixup from FIPS 203
// Multiplied by R^2 to additionally multiply coefficients by R after montgomery reduction
const UINT32 SYMCRYPT_MLKEM_INTTFixupTimesRsqr = 1441;
const UINT32 SYMCRYPT_MLKEM_INTTFixupTimesRsqrTimesNegQInvModR = 10079;

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementINTTAndMulR(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT  peSrc )
{
    UINT32 i;

    SymCryptMlKemPolyElementINTTLayer( peSrc, 127,   2 );
    SymCryptMlKemPolyElementINTTLayer( peSrc,  63,   4 );
    SymCryptMlKemPolyElementINTTLayer( peSrc,  31,   8 );
    SymCryptMlKemPolyElementINTTLayer( peSrc,  15,  16 );
    SymCryptMlKemPolyElementINTTLayer( peSrc,   7,  32 );
    SymCryptMlKemPolyElementINTTLayer( peSrc,   3,  64 );
    SymCryptMlKemPolyElementINTTLayer( peSrc,   1, 128 );

    for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++)
    {
        peSrc->coeffs[i] = (UINT16) SymCryptMlKemMontMul(
            peSrc->coeffs[i], SYMCRYPT_MLKEM_INTTFixupTimesRsqr, SYMCRYPT_MLKEM_INTTFixupTimesRsqrTimesNegQInvModR );
    }
}

// ((1<<33) / SYMCRYPT_MLKEM_Q) rounded to nearest integer
//
// 1<<33 is the smallest power of 2 s.t. the constant has sufficient precision to round
// all inputs correctly in compression for all nBitsPerCoefficient < 12. A smaller
// constant could be used for smaller nBitsPerCoefficient for a small performance gain
//
const UINT32 SYMCRYPT_MLKEM_COMPRESS_MULCONSTANT = 0x275f6f;
const UINT32 SYMCRYPT_MLKEM_COMPRESS_SHIFTCONSTANT = 33;

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementCompressAndEncode(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT    peSrc,
            UINT32                          nBitsPerCoefficient,
    _Out_writes_bytes_(nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8))
            PBYTE                           pbDst )
{
    UINT32 i;
    UINT64 multiplication;
    UINT32 coefficient;
    UINT32 nBitsInCoefficient;
    UINT32 bitsToEncode;
    UINT32 nBitsToEncode;
    UINT32 cbDstWritten = 0;
    UINT32 accumulator = 0;
    UINT32 nBitsInAccumulator = 0;

    SYMCRYPT_ASSERT( nBitsPerCoefficient >  0  );
    SYMCRYPT_ASSERT( nBitsPerCoefficient <= 12 );

    for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        nBitsInCoefficient = nBitsPerCoefficient;
        coefficient = peSrc->coeffs[i]; // in range [0, Q-1]
        SYMCRYPT_ASSERT( coefficient < SYMCRYPT_MLKEM_Q );

        // first compress the coefficient
        // when nBitsPerCoefficient < 12 we compress per Compress_d in FIPS 203;
        if(nBitsPerCoefficient < 12)
        {
            // Multiply by 2^(nBitsPerCoefficient+1) / Q by multiplying by constant and shifting right
            multiplication = SYMCRYPT_MUL32x32TO64(coefficient, SYMCRYPT_MLKEM_COMPRESS_MULCONSTANT);
            coefficient = (UINT32) (multiplication >> (SYMCRYPT_MLKEM_COMPRESS_SHIFTCONSTANT-(nBitsPerCoefficient+1)));

            // add "half" to round to nearest integer
            coefficient++;

            // final divide by two to get multiplication by 2^nBitsPerCoefficient / Q
            coefficient >>= 1;                              // in range [0, 2^nBitsPerCoefficient]
            SYMCRYPT_ASSERT(coefficient <= (1UL<<nBitsPerCoefficient));

            // modular reduction by masking
            coefficient &= (1UL<<nBitsPerCoefficient)-1;    // in range [0, 2^nBitsPerCoefficient - 1]
            SYMCRYPT_ASSERT(coefficient <  (1UL<<nBitsPerCoefficient));
        }

        // encode the coefficient
        // simple loop to add bits to accumulator and write accumulator to output
        do
        {
            nBitsToEncode = SYMCRYPT_MIN(nBitsInCoefficient, 32-nBitsInAccumulator);

            bitsToEncode = coefficient & ((1UL<<nBitsToEncode)-1);
            coefficient >>= nBitsToEncode;
            nBitsInCoefficient -= nBitsToEncode;

            accumulator |= (bitsToEncode << nBitsInAccumulator);
            nBitsInAccumulator += nBitsToEncode;
            if(nBitsInAccumulator == 32)
            {
                SYMCRYPT_STORE_LSBFIRST32( pbDst+cbDstWritten, accumulator );
                cbDstWritten += 4;
                accumulator = 0;
                nBitsInAccumulator = 0;
            }
        } while( nBitsInCoefficient > 0 );
    }

    SYMCRYPT_ASSERT(nBitsInAccumulator == 0);
    SYMCRYPT_ASSERT(cbDstWritten == (nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8)));
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemPolyElementDecodeAndDecompress(
    _In_reads_bytes_(nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8))
            PCBYTE                      pbSrc,
            UINT32                      nBitsPerCoefficient,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT peDst )
{
    UINT32 i;
    UINT32 coefficient;
    UINT32 nBitsInCoefficient;
    UINT32 bitsToDecode;
    UINT32 nBitsToDecode;
    UINT32 cbSrcRead = 0;
    UINT32 accumulator = 0;
    UINT32 nBitsInAccumulator = 0;

    SYMCRYPT_ASSERT( nBitsPerCoefficient >  0  );
    SYMCRYPT_ASSERT( nBitsPerCoefficient <= 12 );

    for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i++ )
    {
        coefficient = 0;
        nBitsInCoefficient = 0;

        // first gather and decode bits from pbSrc
        do
        {
            if(nBitsInAccumulator == 0)
            {
                accumulator = SYMCRYPT_LOAD_LSBFIRST32( pbSrc+cbSrcRead );
                cbSrcRead += 4;
                nBitsInAccumulator = 32;
            }

            nBitsToDecode = SYMCRYPT_MIN(nBitsPerCoefficient-nBitsInCoefficient, nBitsInAccumulator);
            SYMCRYPT_ASSERT(nBitsToDecode <= nBitsInAccumulator);

            bitsToDecode = accumulator & ((1UL<<nBitsToDecode)-1);
            accumulator >>= nBitsToDecode;
            nBitsInAccumulator -= nBitsToDecode;

            coefficient |= (bitsToDecode << nBitsInCoefficient);
            nBitsInCoefficient += nBitsToDecode;
        } while( nBitsPerCoefficient > nBitsInCoefficient );
        SYMCRYPT_ASSERT(nBitsInCoefficient == nBitsPerCoefficient);

        // decompress the coefficient
        // when nBitsPerCoefficient < 12 we decompress per Decompress_d in FIPS 203
        // otherwise we perform input validation per 203 6.2 Input validation 2 (Modulus check)
        if(nBitsPerCoefficient < 12)
        {
            // Multiply by Q / 2^(nBitsPerCoefficient-1) by multiplying by constant and shifting right
            coefficient *= SYMCRYPT_MLKEM_Q;
            coefficient >>= (nBitsPerCoefficient-1);

            // add "half" to round to nearest integer
            coefficient++;

            // final divide by two to get multiplication by Q / 2^nBitsPerCoefficient
            coefficient >>= 1;  // in range [0, Q]

            // modular reduction by conditional subtraction
            coefficient = SymCryptMlKemModSub( coefficient, SYMCRYPT_MLKEM_Q );
            SYMCRYPT_ASSERT( coefficient < SYMCRYPT_MLKEM_Q );
        }
        else if( coefficient >= SYMCRYPT_MLKEM_Q )
        {
            // input validation failure - this can happen with a malformed or corrupt encapsulation
            // or decapsulation key; we do not need to be constant time because we treat the
            // validity of an imported key as public information.
            return SYMCRYPT_INVALID_BLOB;
        }

        peDst->coeffs[i] = (UINT16) coefficient;
    }

    SYMCRYPT_ASSERT(nBitsInAccumulator == 0);
    SYMCRYPT_ASSERT(cbSrcRead == (nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8)));

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementSampleNTTFromShake128(
    _Inout_ PSYMCRYPT_SHAKE128_STATE    pState,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT  peDst )
{
    UINT32 i=0;
    BYTE shakeOutputBuf[3*8]; // Keccak likes extracting multiples of 8-bytes
    UINT32 currBufIndex = sizeof(shakeOutputBuf);
    UINT16 sample0, sample1;

    while( i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS )
    {
        SYMCRYPT_ASSERT(currBufIndex <= sizeof(shakeOutputBuf));
        if( currBufIndex == sizeof(shakeOutputBuf) )
        {
            SymCryptShake128Extract(pState, shakeOutputBuf, sizeof(shakeOutputBuf), FALSE);
            currBufIndex = 0;
        }

        sample0 = SYMCRYPT_LOAD_LSBFIRST16( shakeOutputBuf+currBufIndex ) & 0xfff;
        sample1 = SYMCRYPT_LOAD_LSBFIRST16( shakeOutputBuf+currBufIndex+1 ) >> 4;
        currBufIndex += 3;

        peDst->coeffs[i] = sample0;
        i += sample0 < SYMCRYPT_MLKEM_Q;

        if( i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS )
        {
            peDst->coeffs[i] = sample1;
            i += sample1 < SYMCRYPT_MLKEM_Q;
        }
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementSampleCBDFromBytes(
    _In_reads_bytes_(eta*2*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) + 1)
                    PCBYTE                      pbSrc,
    _In_range_(2,3) UINT32                      eta,
    _Out_           PSYMCRYPT_MLKEM_POLYELEMENT peDst )
{
    UINT32 i, j;
    UINT32 sampleBits;
    UINT32 coefficient;

    SYMCRYPT_ASSERT((eta == 2) || (eta == 3));
    if( eta == 3 )
    {
        for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i+=4 )
        {
            // unconditionally load 4 bytes into sampleBits, but only treat the load
            // as being 3 bytes (24-bits -> 4 coefficients) for eta==3 to align to
            // byte boundaries. Source buffer must be 1 byte larger than shake output
            sampleBits = SYMCRYPT_LOAD_LSBFIRST32( pbSrc );
            pbSrc += 3;

            // sum bit samples - each consecutive slice of eta bits is summed together
            sampleBits = (sampleBits&0x249249) + ((sampleBits>>1)&0x249249) + ((sampleBits>>2)&0x249249);

            for( j=0; j<4; j++ )
            {
                // each coefficient is formed by taking the difference of two consecutive slices of eta bits
                // the first eta bits are positive, the second eta bits are negative
                coefficient = sampleBits & 0x3f;
                sampleBits >>= 6;
                coefficient = (coefficient&3) - (coefficient>>3);
                SYMCRYPT_ASSERT((coefficient >= ((UINT32)-3)) || (coefficient <= 3));

                coefficient = coefficient + (SYMCRYPT_MLKEM_Q & (coefficient >> 16));     // in range [0, Q-1]
                SYMCRYPT_ASSERT( coefficient < SYMCRYPT_MLKEM_Q );

                peDst->coeffs[i+j] = (UINT16) coefficient;
            }
        }
    }
    else
    {
        for( i=0; i<SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS; i+=8 )
        {
            // unconditionally load 4 bytes (32-bits -> 8 coefficients) into sampleBits
            sampleBits = SYMCRYPT_LOAD_LSBFIRST32( pbSrc );
            pbSrc += 4;

            // sum bit samples - each consecutive slice of eta bits is summed together
            sampleBits = (sampleBits&0x55555555) + ((sampleBits>>1)&0x55555555);

            for( j=0; j<8; j++ )
            {
                // each coefficient is formed by taking the difference of two consecutive slices of eta bits
                // the first eta bits are positive, the second eta bits are negative
                coefficient = sampleBits & 0xf;
                sampleBits >>= 4;
                coefficient = (coefficient&3) - (coefficient>>2);
                SYMCRYPT_ASSERT((coefficient >= ((UINT32)-2)) || (coefficient <= 2));

                coefficient = coefficient + (SYMCRYPT_MLKEM_Q & (coefficient >> 16));     // in range [0, Q-1]
                SYMCRYPT_ASSERT( coefficient < SYMCRYPT_MLKEM_Q );

                peDst->coeffs[i+j] = (UINT16) coefficient;
            }
        }
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemMatrixTranspose(
    _Inout_ PSYMCRYPT_MLKEM_MATRIX  pmSrc )
{
    UINT32 i, j;
    PSYMCRYPT_MLKEM_POLYELEMENT swap;
    const UINT32 nRows = pmSrc->nRows;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );

    for( i=0; i<nRows; i++ )
    {
        for( j=i+1; j<nRows; j++ )
        {
            swap = pmSrc->apPolyElements[(i*nRows) + j];
            pmSrc->apPolyElements[(i*nRows) + j] = pmSrc->apPolyElements[(j*nRows) + i];
            pmSrc->apPolyElements[(j*nRows) + i] = swap;
        }
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemMatrixVectorMontMulAndAdd(
    _In_    PCSYMCRYPT_MLKEM_MATRIX                 pmSrc1,
    _In_    PCSYMCRYPT_MLKEM_VECTOR                 pvSrc2,
    _Inout_ PSYMCRYPT_MLKEM_VECTOR                  pvDst,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paTmp )
{
    UINT32 i, j;
    const UINT32 nRows = pmSrc1->nRows;
    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc1, peSrc2;
    PSYMCRYPT_MLKEM_POLYELEMENT  peDst;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );
    SYMCRYPT_ASSERT( pvSrc2->nRows == nRows );
    SYMCRYPT_ASSERT( pvDst->nRows == nRows );

    // Zero paTmp
    SymCryptWipeKnownSize( paTmp, SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT_ACCUMULATOR );

    for( i=0; i<nRows; i++ )
    {
        for( j=0; j<nRows; j++ )
        {
            peSrc1 = pmSrc1->apPolyElements[(i*nRows) + j];
            peSrc2 = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( j, pvSrc2 );
            SymCryptMlKemPolyElementMulAndAccumulate( peSrc1, peSrc2, paTmp );
        }

        // write accumulator to dest and zero accumulator
        peDst  = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvDst );
        SymCryptMlKemMontgomeryReduceAndAddPolyElementAccumulatorToPolyElement( paTmp, peDst );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorMontDotProduct(
    _In_    PCSYMCRYPT_MLKEM_VECTOR                 pvSrc1,
    _In_    PCSYMCRYPT_MLKEM_VECTOR                 pvSrc2,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT             peDst,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paTmp )
{
    UINT32 i;
    const UINT32 nRows = pvSrc1->nRows;
    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc1, peSrc2;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );
    SYMCRYPT_ASSERT( pvSrc2->nRows == nRows );

    // Zero paTmp and peDst
    SymCryptWipeKnownSize( paTmp, SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT_ACCUMULATOR );
    SymCryptWipeKnownSize( peDst, SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT );

    for( i=0; i<nRows; i++ )
    {
        peSrc1 = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc1 );
        peSrc2 = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc2 );
        SymCryptMlKemPolyElementMulAndAccumulate( peSrc1, peSrc2, paTmp );
    }

    // write accumulator to dest and zero accumulator
    SymCryptMlKemMontgomeryReduceAndAddPolyElementAccumulatorToPolyElement( paTmp, peDst );
}

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorSetZero(
    _Inout_ PSYMCRYPT_MLKEM_VECTOR  pvSrc )
{
    const UINT32 nRows = pvSrc->nRows;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );

    SymCryptWipe( (PBYTE) SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( 0, pvSrc ), nRows*SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT );
}

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorMulR(
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc,
    _Out_   PSYMCRYPT_MLKEM_VECTOR  pvDst )
{
    UINT32 i;
    const UINT32 nRows = pvSrc->nRows;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );
    SYMCRYPT_ASSERT( pvDst->nRows == nRows );

    for( i=0; i<nRows; i++ )
    {
        SymCryptMlKemPolyElementMulR(
            SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc ),
            SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvDst ) );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorAdd(
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc1,
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc2,
    _Out_   PSYMCRYPT_MLKEM_VECTOR  pvDst )
{
    UINT32 i;
    const UINT32 nRows = pvSrc1->nRows;
    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc1, peSrc2;
    PSYMCRYPT_MLKEM_POLYELEMENT  peDst;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );
    SYMCRYPT_ASSERT( pvSrc2->nRows == nRows );
    SYMCRYPT_ASSERT( pvDst->nRows == nRows );

    for( i=0; i<nRows; i++ )
    {
        peSrc1 = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc1 );
        peSrc2 = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc2 );
        peDst  = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvDst );
        SymCryptMlKemPolyElementAdd( peSrc1, peSrc2, peDst );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorSub(
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc1,
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc2,
    _Out_   PSYMCRYPT_MLKEM_VECTOR  pvDst )
{
    UINT32 i;
    const UINT32 nRows = pvSrc1->nRows;
    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc1, peSrc2;
    PSYMCRYPT_MLKEM_POLYELEMENT  peDst;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );
    SYMCRYPT_ASSERT( pvSrc2->nRows == nRows );
    SYMCRYPT_ASSERT( pvDst->nRows == nRows );

    for( i=0; i<nRows; i++ )
    {
        peSrc1 = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc1 );
        peSrc2 = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc2 );
        peDst  = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvDst );
        SymCryptMlKemPolyElementSub( peSrc1, peSrc2, peDst );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorNTT(
    _Inout_ PSYMCRYPT_MLKEM_VECTOR  pvSrc )
{
    UINT32 i;
    const UINT32 nRows = pvSrc->nRows;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );

    for( i=0; i<nRows; i++ )
    {
        SymCryptMlKemPolyElementNTT( SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc ) );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorINTTAndMulR(
    _Inout_ PSYMCRYPT_MLKEM_VECTOR  pvSrc )
{
    UINT32 i;
    const UINT32 nRows = pvSrc->nRows;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );

    for( i=0; i<nRows; i++ )
    {
        SymCryptMlKemPolyElementINTTAndMulR( SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc ) );
    }
}

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorCompressAndEncode(
    _In_                        PCSYMCRYPT_MLKEM_VECTOR pvSrc,
                                UINT32                  nBitsPerCoefficient,
    _Out_writes_bytes_(cbDst)   PBYTE                   pbDst,
                                SIZE_T                  cbDst )
{
    UINT32 i;
    const UINT32 nRows = pvSrc->nRows;
    PCSYMCRYPT_MLKEM_POLYELEMENT peSrc;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );
    SYMCRYPT_ASSERT( nBitsPerCoefficient >  0  );
    SYMCRYPT_ASSERT( nBitsPerCoefficient <= 12 );
    SYMCRYPT_ASSERT( cbDst == nRows*nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) );

    UNREFERENCED_PARAMETER( cbDst );

    for( i=0; i<nRows; i++ )
    {
        peSrc  = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvSrc );
        SymCryptMlKemPolyElementCompressAndEncode( peSrc, nBitsPerCoefficient, pbDst );
        pbDst += nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
    }
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemVectorDecodeAndDecompress(
    _In_reads_bytes_(cbSrc) PCBYTE                  pbSrc,
                            SIZE_T                  cbSrc,
                            UINT32                  nBitsPerCoefficient,
    _Out_                   PSYMCRYPT_MLKEM_VECTOR  pvDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 i;
    const UINT32 nRows = pvDst->nRows;
    PSYMCRYPT_MLKEM_POLYELEMENT peDst;

    SYMCRYPT_ASSERT( nRows >  0 );
    SYMCRYPT_ASSERT( nRows <= SYMCRYPT_MLKEM_MATRIX_MAX_NROWS );
    SYMCRYPT_ASSERT( nBitsPerCoefficient >  0  );
    SYMCRYPT_ASSERT( nBitsPerCoefficient <= 12 );
    SYMCRYPT_ASSERT( cbSrc == nRows*nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) );

    UNREFERENCED_PARAMETER( cbSrc );

    for( i=0; i<nRows; i++ )
    {
        peDst  = SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( i, pvDst );
        scError = SymCryptMlKemPolyElementDecodeAndDecompress( pbSrc, nBitsPerCoefficient, peDst );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
        pbSrc += nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8);
    }

cleanup:
    return scError;
}

VOID
SYMCRYPT_CALL
SymCryptMlKemkeyWipePrivateState(
    _Inout_ PSYMCRYPT_MLKEMKEY  pkMlKemkey )
{
    SymCryptMlKemVectorSetZero( pkMlKemkey->pvs );
    SymCryptWipeKnownSize( pkMlKemkey->privateRandom, sizeof(pkMlKemkey->privateRandom) );
    SymCryptWipeKnownSize( pkMlKemkey->privateSeed, sizeof(pkMlKemkey->privateSeed) );
    pkMlKemkey->hasPrivateKey = FALSE;
    pkMlKemkey->hasPrivateSeed = FALSE;
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86
#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif
#endif
