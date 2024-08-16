/*
 * xmmintrin.h
 *
 * This file is part of the ReactOS CRT package.
 *
 * Contributors:
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once
#ifndef _INCLUDED_MM2
#define _INCLUDED_MM2

#include <mmintrin.h>

#if defined(_MM2_FUNCTIONALITY) && !defined(_MM_FUNCTIONALITY)
#define _MM_FUNCTIONALITY
#endif

#if !defined _VCRT_BUILD && !defined _INC_MALLOC
#include <malloc.h> // For _mm_malloc() and _mm_free()
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(__clang__)

typedef union _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(16) __m128
{
    float m128_f32[4];
    unsigned __int64 m128_u64[2];
    __int8 m128_i8[16];
    __int16 m128_i16[8];
    __int32 m128_i32[4];
    __int64 m128_i64[2];
    unsigned __int8 m128_u8[16];
    unsigned __int16 m128_u16[8];
    unsigned __int32 m128_u32[4];
} __m128;

#define __ATTRIBUTE_SSE__

#else /* _MSC_VER */

    typedef        float __v4sf __attribute__((__vector_size__(16)));
    typedef   signed int __v4si __attribute__((__vector_size__(16)));
    typedef unsigned int __v4su __attribute__((__vector_size__(16)));
    typedef float __m128_u __attribute__((__vector_size__(16), __aligned__(1)));

    typedef        float __m128 __attribute__((__vector_size__(16), __aligned__(16)));

#ifdef __clang__
#define __ATTRIBUTE_SSE__ __attribute__((__target__("sse"),__min_vector_width__(128)))
#else
#define __ATTRIBUTE_SSE__ __attribute__((__target__("sse")))
#endif
#define __INTRIN_INLINE_SSE __INTRIN_INLINE __ATTRIBUTE_SSE__ 

#endif /* _MSC_VER */

#define _MM_ALIGN16 _VCRT_ALIGN(16)

/* Constants for use with _mm_prefetch.  */
#define _MM_HINT_NTA  0
#define _MM_HINT_T0   1
#define _MM_HINT_T1   2
#define _MM_HINT_T2   3
#define _MM_HINT_ENTA 4
#if 0 // Not supported yet
#define _MM_HINT_ET0  5
#define _MM_HINT_ET1  6
#define _MM_HINT_ET2  7
#endif

/* Create a selector for use with the SHUFPS instruction.  */
#define _MM_SHUFFLE(fp3, fp2, fp1, fp0) \
    (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | (fp0))

/* Bits in the MXCSR.  */
#define _MM_EXCEPT_MASK       0x003f
#define _MM_EXCEPT_INVALID    0x0001
#define _MM_EXCEPT_DENORM     0x0002
#define _MM_EXCEPT_DIV_ZERO   0x0004
#define _MM_EXCEPT_OVERFLOW   0x0008
#define _MM_EXCEPT_UNDERFLOW  0x0010
#define _MM_EXCEPT_INEXACT    0x0020

#define _MM_MASK_MASK         0x1f80
#define _MM_MASK_INVALID      0x0080
#define _MM_MASK_DENORM       0x0100
#define _MM_MASK_DIV_ZERO     0x0200
#define _MM_MASK_OVERFLOW     0x0400
#define _MM_MASK_UNDERFLOW    0x0800
#define _MM_MASK_INEXACT      0x1000

#define _MM_ROUND_MASK        0x6000
#define _MM_ROUND_NEAREST     0x0000
#define _MM_ROUND_DOWN        0x2000
#define _MM_ROUND_UP          0x4000
#define _MM_ROUND_TOWARD_ZERO 0x6000

#define _MM_FLUSH_ZERO_MASK   0x8000
#define _MM_FLUSH_ZERO_ON     0x8000
#define _MM_FLUSH_ZERO_OFF    0x0000

#ifdef __ICL
void* __cdecl _mm_malloc(size_t Size, size_t Al);
void __cdecl _mm_free(void* P);
#endif

void _mm_prefetch(_In_ char const* p, _In_ int i);
__m128 _mm_setzero_ps(void);
__m128 _mm_add_ss(__m128 a, __m128 b);
__m128 _mm_sub_ss(__m128 a, __m128 b);
__m128 _mm_mul_ss(__m128 a, __m128 b);
__m128 _mm_div_ss(__m128 a, __m128 b);
__m128 _mm_sqrt_ss(__m128 a);
__m128 _mm_rcp_ss(__m128 a);
__m128 _mm_rsqrt_ss(__m128 a);
__m128 _mm_min_ss(__m128 a, __m128 b);
__m128 _mm_max_ss(__m128 a, __m128 b);
__m128 _mm_add_ps(__m128 a, __m128 b);
__m128 _mm_sub_ps(__m128 a, __m128 b);
__m128 _mm_mul_ps(__m128 a, __m128 b);
__m128 _mm_div_ps(__m128 a, __m128 b);
__m128 _mm_sqrt_ps(__m128 a);
__m128 _mm_rcp_ps(__m128 a);
__m128 _mm_rsqrt_ps(__m128 a);
__m128 _mm_min_ps(__m128 a, __m128 b);
__m128 _mm_max_ps(__m128 a, __m128 b);
__m128 _mm_and_ps(__m128 a, __m128 b);
__m128 _mm_andnot_ps(__m128 a, __m128 b);
__m128 _mm_or_ps(__m128 a, __m128 b);
__m128 _mm_xor_ps(__m128 a, __m128 b);
__m128 _mm_cmpeq_ss(__m128 a, __m128 b);
__m128 _mm_cmplt_ss(__m128 a, __m128 b);
__m128 _mm_cmple_ss(__m128 a, __m128 b);
__m128 _mm_cmpgt_ss(__m128 a, __m128 b);
__m128 _mm_cmpge_ss(__m128 a, __m128 b);
__m128 _mm_cmpneq_ss(__m128 a, __m128 b);
__m128 _mm_cmpnlt_ss(__m128 a, __m128 b);
__m128 _mm_cmpnle_ss(__m128 a, __m128 b);
__m128 _mm_cmpngt_ss(__m128 a, __m128 b);
__m128 _mm_cmpnge_ss(__m128 a, __m128 b);
__m128 _mm_cmpord_ss(__m128 a, __m128 b);
__m128 _mm_cmpunord_ss(__m128 a, __m128 b);
__m128 _mm_cmpeq_ps(__m128 a, __m128 b);
__m128 _mm_cmplt_ps(__m128 a, __m128 b);
__m128 _mm_cmple_ps(__m128 a, __m128 b);
__m128 _mm_cmpgt_ps(__m128 a, __m128 b);
__m128 _mm_cmpge_ps(__m128 a, __m128 b);
__m128 _mm_cmpneq_ps(__m128 a, __m128 b);
__m128 _mm_cmpnlt_ps(__m128 a, __m128 b);
__m128 _mm_cmpnle_ps(__m128 a, __m128 b);
__m128 _mm_cmpngt_ps(__m128 a, __m128 b);
__m128 _mm_cmpnge_ps(__m128 a, __m128 b);
__m128 _mm_cmpord_ps(__m128 a, __m128 b);
__m128 _mm_cmpunord_ps(__m128 a, __m128 b);
int _mm_comieq_ss(__m128 a, __m128 b);
int _mm_comilt_ss(__m128 a, __m128 b);
int _mm_comile_ss(__m128 a, __m128 b);
int _mm_comigt_ss(__m128 a, __m128 b);
int _mm_comige_ss(__m128 a, __m128 b);
int _mm_comineq_ss(__m128 a, __m128 b);
int _mm_ucomieq_ss(__m128 a, __m128 b);
int _mm_ucomilt_ss(__m128 a, __m128 b);
int _mm_ucomile_ss(__m128 a, __m128 b);
int _mm_ucomigt_ss(__m128 a, __m128 b);
int _mm_ucomige_ss(__m128 a, __m128 b);
int _mm_ucomineq_ss(__m128 a, __m128 b);
int _mm_cvt_ss2si(__m128 a);
int _mm_cvtt_ss2si(__m128 a);
__m128 _mm_cvt_si2ss(__m128 a, int b);
#ifdef _M_IX86
__m64 _mm_cvt_ps2pi(__m128 a);
__m64 _mm_cvtt_ps2pi(__m128 a);
__m128 _mm_cvt_pi2ps(__m128 a, __m64 b);
#endif
__m128 _mm_shuffle_ps(__m128 a, __m128 b, unsigned int imm8);
__m128 _mm_unpackhi_ps(__m128 a, __m128 b);
__m128 _mm_unpacklo_ps(__m128 a, __m128 b);
__m128 _mm_loadh_pi(__m128 a, __m64 const* p);
void _mm_storeh_pi(__m64* p, __m128 a);
__m128 _mm_movehl_ps(__m128 a, __m128 b);
__m128 _mm_movelh_ps(__m128 a, __m128 b);
__m128 _mm_loadl_pi(__m128 a, __m64 const* p);
void _mm_storel_pi(__m64* p, __m128 a);
int _mm_movemask_ps(__m128 a);
unsigned int _mm_getcsr(void);
void _mm_setcsr(unsigned int a);
__m128 _mm_set_ss(float a);
__m128 _mm_set_ps1(float a);
__m128 _mm_load_ss(float const* p);
__m128 _mm_load_ps1(float const* p);
__m128 _mm_load_ps(float const* p);
__m128 _mm_loadu_ps(float const* p);
__m128 _mm_loadr_ps(float const* p);
__m128 _mm_set_ps(float e3, float e2, float e1, float e0);
__m128 _mm_setr_ps(float e3, float e2, float e1, float e0);
void _mm_store_ss(float* p, __m128 a);
float _mm_cvtss_f32(__m128 a);
void _mm_store_ps(float* p, __m128 a);
void _mm_storeu_ps(float* p, __m128 a);
void _mm_store_ps1(float* p, __m128 a);
void _mm_storer_ps(float* p, __m128 a);
__m128 _mm_move_ss(__m128 a, __m128 b);
#ifdef _M_IX86
int _m_pextrw(__m64 a, int imm8);
__m64 _m_pinsrw(__m64 a, int i, int imm8);
__m64 _m_pmaxsw(__m64 a, __m64 b);
__m64 _m_pmaxub(__m64 a, __m64 b);
__m64 _m_pminsw(__m64 a, __m64 b);
__m64 _m_pminub(__m64 a, __m64 b);
int _m_pmovmskb(__m64 a);
__m64 _m_pmulhuw(__m64 a, __m64 b);
__m64 _m_pshufw(__m64 a, int imm8);
void _m_maskmovq(__m64 a, __m64 b, char*);
__m64 _m_pavgb(__m64 a, __m64 b);
__m64 _m_pavgw(__m64 a, __m64 b);
__m64 _m_psadbw(__m64 a, __m64 b);
void _mm_stream_pi(__m64* p, __m64 a);
#endif
void _mm_stream_ps(float* p, __m128 a);
void _mm_sfence(void);
#ifdef _M_AMD64
__int64 _mm_cvtss_si64(__m128 a);
__int64 _mm_cvttss_si64(__m128 a);
__m128  _mm_cvtsi64_ss(__m128 a, __int64 b);
#endif

/* Alternate names */
#define _mm_cvtss_si32 _mm_cvt_ss2si
#define _mm_cvttss_si32 _mm_cvtt_ss2si
#define _mm_cvtsi32_ss _mm_cvt_si2ss
#define _mm_set1_ps _mm_set_ps1
#define _mm_load1_ps _mm_load_ps1f
#define _mm_store1_ps _mm_store_ps1
#define _mm_cvtps_pi32    _mm_cvt_ps2pi
#define _mm_cvttps_pi32   _mm_cvtt_ps2pi
#define _mm_cvtpi32_ps    _mm_cvt_pi2ps
#define _mm_extract_pi16  _m_pextrw
#define _mm_insert_pi16   _m_pinsrw
#define _mm_max_pi16      _m_pmaxsw
#define _mm_max_pu8       _m_pmaxub
#define _mm_min_pi16      _m_pminsw
#define _mm_min_pu8       _m_pminub
#define _mm_movemask_pi8  _m_pmovmskb
#define _mm_mulhi_pu16    _m_pmulhuw
#define _mm_shuffle_pi16  _m_pshufw
#define _mm_maskmove_si64 _m_maskmovq
#define _mm_avg_pu8       _m_pavgb
#define _mm_avg_pu16      _m_pavgw
#define _mm_sad_pu8       _m_psadbw

#ifdef _M_IX86
/* Inline functions from Clang: https://github.com/llvm/llvm-project/blob/main/clang/lib/Headers/xmmintrin.h */

__ATTRIBUTE_SSE__
static __inline __m128 _mm_cvtpi16_ps(__m64 __a)
{
    __m64 __b, __c;
    __m128 __r;

    __b = _mm_setzero_si64();
    __b = _mm_cmpgt_pi16(__b, __a);
    __c = _mm_unpackhi_pi16(__a, __b);
    __r = _mm_setzero_ps();
    __r = _mm_cvtpi32_ps(__r, __c);
    __r = _mm_movelh_ps(__r, __r);
    __c = _mm_unpacklo_pi16(__a, __b);
    __r = _mm_cvtpi32_ps(__r, __c);

    return __r;
}

__ATTRIBUTE_SSE__
static __inline __m128 _mm_cvtpu16_ps(__m64 __a)
{
    __m64 __b, __c;
    __m128 __r;

    __b = _mm_setzero_si64();
    __c = _mm_unpackhi_pi16(__a, __b);
    __r = _mm_setzero_ps();
    __r = _mm_cvtpi32_ps(__r, __c);
    __r = _mm_movelh_ps(__r, __r);
    __c = _mm_unpacklo_pi16(__a, __b);
    __r = _mm_cvtpi32_ps(__r, __c);

    return __r;
}

__ATTRIBUTE_SSE__
static __inline __m128 _mm_cvtpi8_ps(__m64 __a)
{
    __m64 __b;

    __b = _mm_setzero_si64();
    __b = _mm_cmpgt_pi8(__b, __a);
    __b = _mm_unpacklo_pi8(__a, __b);

    return _mm_cvtpi16_ps(__b);
}

__ATTRIBUTE_SSE__
static __inline __m128 _mm_cvtpu8_ps(__m64 __a)
{
    __m64 __b;

    __b = _mm_setzero_si64();
    __b = _mm_unpacklo_pi8(__a, __b);

    return _mm_cvtpi16_ps(__b);
}

__ATTRIBUTE_SSE__
static __inline __m128 _mm_cvtpi32x2_ps(__m64 __a, __m64 __b)
{
    __m128 __c;

    __c = _mm_setzero_ps();
    __c = _mm_cvtpi32_ps(__c, __b);
    __c = _mm_movelh_ps(__c, __c);

    return _mm_cvtpi32_ps(__c, __a);
}

__ATTRIBUTE_SSE__
static __inline __m64 _mm_cvtps_pi16(__m128 __a)
{
    __m64 __b, __c;

    __b = _mm_cvtps_pi32(__a);
    __a = _mm_movehl_ps(__a, __a);
    __c = _mm_cvtps_pi32(__a);

    return _mm_packs_pi32(__b, __c);
}

__ATTRIBUTE_SSE__
static __inline __m64 _mm_cvtps_pi8(__m128 __a)
{
    __m64 __b, __c;

    __b = _mm_cvtps_pi16(__a);
    __c = _mm_setzero_si64();

    return _mm_packs_pi16(__b, __c);
}

#endif /* _M_IX86 */

/* Transpose the 4x4 matrix composed of row[0-3].  */
#define _MM_TRANSPOSE4_PS(row0, row1, row2, row3) \
do {                                              \
    __m128 t0 = _mm_unpacklo_ps(row0, row1);      \
    __m128 t1 = _mm_unpacklo_ps(row2, row3);      \
    __m128 t2 = _mm_unpackhi_ps(row0, row1);      \
    __m128 t3 = _mm_unpackhi_ps(row2, row3);      \
    (row0) = _mm_movelh_ps(t0, t1);               \
    (row1) = _mm_movehl_ps(t1, t0);               \
    (row2) = _mm_movelh_ps(t2, t3);               \
    (row3) = _mm_movehl_ps(t3, t2);               \
} while (0)

#define _MM_GET_EXCEPTION_STATE() \
    (_mm_getcsr() & _MM_EXCEPT_MASK)

#define _MM_GET_EXCEPTION_MASK() \
    (_mm_getcsr() & _MM_MASK_MASK)

#define _MM_GET_ROUNDING_MODE() \
    (_mm_getcsr() & _MM_ROUND_MASK)

#define _MM_GET_FLUSH_ZERO_MODE() \
    (_mm_getcsr() & _MM_FLUSH_ZERO_MASK)

#define _MM_SET_EXCEPTION_STATE(__mask) \
    _mm_setcsr((_mm_getcsr() & ~_MM_EXCEPT_MASK) | (__mask))

#define _MM_SET_EXCEPTION_MASK(__mask) \
    _mm_setcsr((_mm_getcsr() & ~_MM_MASK_MASK) | (__mask))

#define _MM_SET_ROUNDING_MODE(__mode) \
    _mm_setcsr((_mm_getcsr() & ~_MM_ROUND_MASK) | (__mode))

#define _MM_SET_FLUSH_ZERO_MODE(__mode) \
    _mm_setcsr((_mm_getcsr() & ~_MM_FLUSH_ZERO_MASK) | (__mode))

/* Use intrinsics on MSVC */
#if defined(_MSC_VER) && !defined(__clang__)
#pragma intrinsic(_mm_prefetch)
#pragma intrinsic(_mm_setzero_ps)
#pragma intrinsic(_mm_add_ss)
#pragma intrinsic(_mm_sub_ss)
#pragma intrinsic(_mm_mul_ss)
#pragma intrinsic(_mm_div_ss)
#pragma intrinsic(_mm_sqrt_ss)
#pragma intrinsic(_mm_rcp_ss)
#pragma intrinsic(_mm_rsqrt_ss)
#pragma intrinsic(_mm_min_ss)
#pragma intrinsic(_mm_max_ss)
#pragma intrinsic(_mm_add_ps)
#pragma intrinsic(_mm_sub_ps)
#pragma intrinsic(_mm_mul_ps)
#pragma intrinsic(_mm_div_ps)
#pragma intrinsic(_mm_sqrt_ps)
#pragma intrinsic(_mm_rcp_ps)
#pragma intrinsic(_mm_rsqrt_ps)
#pragma intrinsic(_mm_min_ps)
#pragma intrinsic(_mm_max_ps)
#pragma intrinsic(_mm_and_ps)
#pragma intrinsic(_mm_andnot_ps)
#pragma intrinsic(_mm_or_ps)
#pragma intrinsic(_mm_xor_ps)
#pragma intrinsic(_mm_cmpeq_ss)
#pragma intrinsic(_mm_cmplt_ss)
#pragma intrinsic(_mm_cmple_ss)
#pragma intrinsic(_mm_cmpgt_ss)
#pragma intrinsic(_mm_cmpge_ss)
#pragma intrinsic(_mm_cmpneq_ss)
#pragma intrinsic(_mm_cmpnlt_ss)
#pragma intrinsic(_mm_cmpnle_ss)
#pragma intrinsic(_mm_cmpngt_ss)
#pragma intrinsic(_mm_cmpnge_ss)
#pragma intrinsic(_mm_cmpord_ss)
#pragma intrinsic(_mm_cmpunord_ss)
#pragma intrinsic(_mm_cmpeq_ps)
#pragma intrinsic(_mm_cmplt_ps)
#pragma intrinsic(_mm_cmple_ps)
#pragma intrinsic(_mm_cmpgt_ps)
#pragma intrinsic(_mm_cmpge_ps)
#pragma intrinsic(_mm_cmpneq_ps)
#pragma intrinsic(_mm_cmpnlt_ps)
#pragma intrinsic(_mm_cmpnle_ps)
#pragma intrinsic(_mm_cmpngt_ps)
#pragma intrinsic(_mm_cmpnge_ps)
#pragma intrinsic(_mm_cmpord_ps)
#pragma intrinsic(_mm_cmpunord_ps)
#pragma intrinsic(_mm_comieq_ss)
#pragma intrinsic(_mm_comilt_ss)
#pragma intrinsic(_mm_comile_ss)
#pragma intrinsic(_mm_comigt_ss)
#pragma intrinsic(_mm_comige_ss)
#pragma intrinsic(_mm_comineq_ss)
#pragma intrinsic(_mm_ucomieq_ss)
#pragma intrinsic(_mm_ucomilt_ss)
#pragma intrinsic(_mm_ucomile_ss)
#pragma intrinsic(_mm_ucomigt_ss)
#pragma intrinsic(_mm_ucomige_ss)
#pragma intrinsic(_mm_ucomineq_ss)
#pragma intrinsic(_mm_cvt_ss2si)
#pragma intrinsic(_mm_cvtt_ss2si)
#pragma intrinsic(_mm_cvt_si2ss)
#ifdef _M_IX86
#pragma intrinsic(_mm_cvt_ps2pi)
#pragma intrinsic(_mm_cvtt_ps2pi)
#pragma intrinsic(_mm_cvt_pi2ps)
#endif // _M_IX86
#pragma intrinsic(_mm_shuffle_ps)
#pragma intrinsic(_mm_unpackhi_ps)
#pragma intrinsic(_mm_unpacklo_ps)
#pragma intrinsic(_mm_loadh_pi)
#pragma intrinsic(_mm_storeh_pi)
#pragma intrinsic(_mm_movehl_ps)
#pragma intrinsic(_mm_movelh_ps)
#pragma intrinsic(_mm_loadl_pi)
#pragma intrinsic(_mm_storel_pi)
#pragma intrinsic(_mm_movemask_ps)
#pragma intrinsic(_mm_getcsr)
#pragma intrinsic(_mm_setcsr)
#pragma intrinsic(_mm_set_ss)
#pragma intrinsic(_mm_set_ps1)
#pragma intrinsic(_mm_load_ss)
#pragma intrinsic(_mm_load_ps1)
#pragma intrinsic(_mm_load_ps)
#pragma intrinsic(_mm_loadu_ps)
#pragma intrinsic(_mm_loadr_ps)
#pragma intrinsic(_mm_set_ps)
#pragma intrinsic(_mm_setr_ps)
#pragma intrinsic(_mm_store_ss)
#pragma intrinsic(_mm_cvtss_f32)
#pragma intrinsic(_mm_store_ps)
#pragma intrinsic(_mm_storeu_ps)
#pragma intrinsic(_mm_store_ps1)
#pragma intrinsic(_mm_storer_ps)
#pragma intrinsic(_mm_move_ss)
#ifdef _M_IX86
#pragma intrinsic(_m_pextrw)
#pragma intrinsic(_m_pinsrw)
#pragma intrinsic(_m_pmaxsw)
#pragma intrinsic(_m_pmaxub)
#pragma intrinsic(_m_pminsw)
#pragma intrinsic(_m_pminub)
#pragma intrinsic(_m_pmovmskb)
#pragma intrinsic(_m_pmulhuw)
#pragma intrinsic(_m_pshufw)
#pragma intrinsic(_m_maskmovq)
#pragma intrinsic(_m_pavgb)
#pragma intrinsic(_m_pavgw)
#pragma intrinsic(_m_psadbw)
#pragma intrinsic(_mm_stream_pi)
#endif // _M_IX86
#pragma intrinsic(_mm_stream_ps)
#pragma intrinsic(_mm_sfence)
#ifdef _M_AMD64
#pragma intrinsic(_mm_cvtss_si64)
#pragma intrinsic(_mm_cvttss_si64)
#pragma intrinsic(_mm_cvtsi64_ss)
#endif // _M_AMD64

#else /* _MSC_VER */

/*
  GCC: https://github.com/gcc-mirror/gcc/blob/master/gcc/config/i386/xmmintrin.h
  Clang: https://github.com/llvm/llvm-project/blob/main/clang/lib/Headers/xmmintrin.h
*/

/* Use inline functions on GCC/Clang */

#if !HAS_BUILTIN(_mm_getcsr)
__INTRIN_INLINE_SSE unsigned int _mm_getcsr(void)
{
    return __builtin_ia32_stmxcsr();
}
#endif

#if !HAS_BUILTIN(_mm_setcsr)
__INTRIN_INLINE_SSE void _mm_setcsr(unsigned int a)
{
    __builtin_ia32_ldmxcsr(a);
}
#endif

__INTRIN_INLINE_SSE __m128 _mm_add_ss(__m128 __a, __m128 __b)
{
    __a[0] += __b[0];
    return __a;
}

__INTRIN_INLINE_SSE __m128 _mm_add_ps(__m128 __a, __m128 __b)
{
    return (__m128)((__v4sf)__a + (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_sub_ss(__m128 __a, __m128 __b)
{
    __a[0] -= __b[0];
    return __a;
}

__INTRIN_INLINE_SSE __m128 _mm_sub_ps(__m128 __a, __m128 __b)
{
    return (__m128)((__v4sf)__a - (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_mul_ss(__m128 __a, __m128 __b)
{
    __a[0] *= __b[0];
    return __a;
}

__INTRIN_INLINE_SSE __m128 _mm_mul_ps(__m128 __a, __m128 __b)
{
    return (__m128)((__v4sf)__a * (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_div_ss(__m128 __a, __m128 __b)
{
    __a[0] /= __b[0];
    return __a;
}

__INTRIN_INLINE_SSE __m128 _mm_div_ps(__m128 __a, __m128 __b)
{
    return (__m128)((__v4sf)__a / (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_sqrt_ss(__m128 __a)
{
    return (__m128)__builtin_ia32_sqrtss((__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_sqrt_ps(__m128 __a)
{
    return __builtin_ia32_sqrtps((__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_rcp_ss(__m128 __a)
{
    return (__m128)__builtin_ia32_rcpss((__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_rcp_ps(__m128 __a)
{
    return (__m128)__builtin_ia32_rcpps((__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_rsqrt_ss(__m128 __a)
{
    return __builtin_ia32_rsqrtss((__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_rsqrt_ps(__m128 __a)
{
    return __builtin_ia32_rsqrtps((__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_min_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_minss((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_min_ps(__m128 __a, __m128 __b)
{
    return __builtin_ia32_minps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_max_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_maxss((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_max_ps(__m128 __a, __m128 __b)
{
    return __builtin_ia32_maxps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_and_ps(__m128 __a, __m128 __b)
{
    return (__m128)((__v4su)__a & (__v4su)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_andnot_ps(__m128 __a, __m128 __b)
{
    return (__m128)(~(__v4su)__a & (__v4su)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_or_ps(__m128 __a, __m128 __b)
{
    return (__m128)((__v4su)__a | (__v4su)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_xor_ps(__m128 __a, __m128 __b)
{
    return (__m128)((__v4su)__a ^ (__v4su)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpeq_ss(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpeqss((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpeq_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpeqps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmplt_ss(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpltss((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmplt_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpltps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmple_ss(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpless((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmple_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpleps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpgt_ss(__m128 __a, __m128 __b)
{
    __v4sf temp = __builtin_ia32_cmpltss((__v4sf)__b, (__v4sf)__a);
#ifdef __clang__
    return (__m128)__builtin_shufflevector((__v4sf)__a, temp, 4, 1, 2, 3);
#else
    return (__m128)__builtin_ia32_movss((__v4sf)__a, temp);
#endif
}

__INTRIN_INLINE_SSE __m128 _mm_cmpgt_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpltps((__v4sf)__b, (__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpge_ss(__m128 __a, __m128 __b)
{
    __v4sf temp = __builtin_ia32_cmpless((__v4sf)__b, (__v4sf)__a);
#ifdef __clang__
    return (__m128)__builtin_shufflevector((__v4sf)__a, temp, 4, 1, 2, 3);
#else
    return (__m128)__builtin_ia32_movss((__v4sf)__a, temp);
#endif
}

__INTRIN_INLINE_SSE __m128 _mm_cmpge_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpleps((__v4sf)__b, (__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpneq_ss(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpneqss((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpneq_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpneqps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpnlt_ss(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpnltss((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpnlt_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpnltps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpnle_ss(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpnless((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpnle_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpnleps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpngt_ss(__m128 __a, __m128 __b)
{
    __v4sf temp = __builtin_ia32_cmpnltss((__v4sf)__b, (__v4sf)__a);
#ifdef  __clang__
    return (__m128)__builtin_shufflevector((__v4sf)__a, temp, 4, 1, 2, 3);
#else
    return (__m128)__builtin_ia32_movss((__v4sf)__a, temp);
#endif
}

__INTRIN_INLINE_SSE __m128 _mm_cmpngt_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpnltps((__v4sf)__b, (__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpnge_ss(__m128 __a, __m128 __b)
{
    __v4sf temp = (__v4sf)__builtin_ia32_cmpnless((__v4sf)__b, (__v4sf)__a);
#ifdef  __clang__
    return (__m128)__builtin_shufflevector((__v4sf)__a, temp, 4, 1, 2, 3);
#else
    return (__m128)__builtin_ia32_movss((__v4sf)__a, temp);
#endif
}

__INTRIN_INLINE_SSE __m128 _mm_cmpnge_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpnleps((__v4sf)__b, (__v4sf)__a);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpord_ss(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpordss((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpord_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpordps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpunord_ss(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpunordss((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE __m128 _mm_cmpunord_ps(__m128 __a, __m128 __b)
{
    return (__m128)__builtin_ia32_cmpunordps((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_comieq_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_comieq((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_comilt_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_comilt((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_comile_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_comile((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_comigt_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_comigt((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_comige_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_comige((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_comineq_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_comineq((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_ucomieq_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_ucomieq((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_ucomilt_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_ucomilt((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_ucomile_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_ucomile((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_ucomigt_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_ucomigt((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_ucomige_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_ucomige((__v4sf)__a, (__v4sf)__b);
}

__INTRIN_INLINE_SSE int _mm_ucomineq_ss(__m128 __a, __m128 __b)
{
    return __builtin_ia32_ucomineq((__v4sf)__a, (__v4sf)__b);
}

// _mm_cvt_ss2si
__INTRIN_INLINE_SSE int _mm_cvtss_si32(__m128 __a)
{
    return __builtin_ia32_cvtss2si((__v4sf)__a);
}

#ifdef _M_AMD64
__INTRIN_INLINE_SSE long long _mm_cvtss_si64(__m128 __a)
{
    return __builtin_ia32_cvtss2si64((__v4sf)__a);
}
#endif

// _mm_cvt_ps2pi
__INTRIN_INLINE_SSE __m64 _mm_cvtps_pi32(__m128 __a)
{
    return (__m64)__builtin_ia32_cvtps2pi((__v4sf)__a);
}

// _mm_cvtt_ss2si
__INTRIN_INLINE_SSE int _mm_cvttss_si32(__m128 __a)
{
    return __builtin_ia32_cvttss2si((__v4sf)__a);
}

#ifdef _M_AMD64
__INTRIN_INLINE_SSE long long _mm_cvttss_si64(__m128 __a)
{
    return __builtin_ia32_cvttss2si64((__v4sf)__a);
}
#endif

// _mm_cvtt_ps2pi
__INTRIN_INLINE_SSE __m64 _mm_cvttps_pi32(__m128 __a)
{
    return (__m64)__builtin_ia32_cvttps2pi((__v4sf)__a);
}

// _mm_cvt_si2ss
__INTRIN_INLINE_SSE __m128 _mm_cvtsi32_ss(__m128 __a, int __b)
{
    __a[0] = __b;
    return __a;
}

#ifdef _M_AMD64
__INTRIN_INLINE_SSE __m128 _mm_cvtsi64_ss(__m128 __a, long long __b)
{
    __a[0] = __b;
    return __a;
}
#endif

// _mm_cvt_pi2ps
__INTRIN_INLINE_SSE __m128 _mm_cvtpi32_ps(__m128 __a, __m64 __b)
{
    return __builtin_ia32_cvtpi2ps((__v4sf)__a, (__v2si)__b);
}

__INTRIN_INLINE_SSE float _mm_cvtss_f32(__m128 __a)
{
    return __a[0];
}

__INTRIN_INLINE_SSE __m128 _mm_loadh_pi(__m128 __a, const __m64 *__p)
{
#ifdef  __clang__
    typedef float __mm_loadh_pi_v2f32 __attribute__((__vector_size__(8)));
    struct __mm_loadh_pi_struct {
        __mm_loadh_pi_v2f32 __u;
    } __attribute__((__packed__, __may_alias__));
    __mm_loadh_pi_v2f32 __b = ((const struct __mm_loadh_pi_struct*)__p)->__u;
    __m128 __bb = __builtin_shufflevector(__b, __b, 0, 1, 0, 1);
    return __builtin_shufflevector(__a, __bb, 0, 1, 4, 5);
#else
    return (__m128)__builtin_ia32_loadhps(__a, __p);
#endif
}

__INTRIN_INLINE_SSE __m128 _mm_loadl_pi(__m128 __a, const __m64 *__p)
{
#ifdef  __clang__
    typedef float __mm_loadl_pi_v2f32 __attribute__((__vector_size__(8)));
    struct __mm_loadl_pi_struct {
        __mm_loadl_pi_v2f32 __u;
    } __attribute__((__packed__, __may_alias__));
    __mm_loadl_pi_v2f32 __b = ((const struct __mm_loadl_pi_struct*)__p)->__u;
    __m128 __bb = __builtin_shufflevector(__b, __b, 0, 1, 0, 1);
    return __builtin_shufflevector(__a, __bb, 4, 5, 2, 3);
#else
    return (__m128)__builtin_ia32_loadlps(__a, __p);
#endif
}

__INTRIN_INLINE_SSE __m128 _mm_load_ss(const float *__p)
{
    return _mm_set_ss(*__p);
}

// _mm_load_ps1
__INTRIN_INLINE_SSE __m128 _mm_load1_ps(const float *__p)
{
    return _mm_set1_ps(*__p);
}

__INTRIN_INLINE_SSE __m128 _mm_load_ps(const float *__p)
{
    return *(const __m128*)__p;
}

__INTRIN_INLINE_SSE __m128 _mm_loadu_ps(const float *__p)
{
    struct __loadu_ps {
        __m128_u __v;
    } __attribute__((__packed__, __may_alias__));
    return ((const struct __loadu_ps*)__p)->__v;
}

__INTRIN_INLINE_SSE __m128 _mm_loadr_ps(const float *__p)
{
    __m128 __a = _mm_load_ps(__p);
#ifdef  __clang__
    return __builtin_shufflevector((__v4sf)__a, (__v4sf)__a, 3, 2, 1, 0);
#else
    return (__m128)__builtin_ia32_shufps(__a, __a, _MM_SHUFFLE(0,1,2,3));
#endif
}

__INTRIN_INLINE_SSE __m128 _mm_undefined_ps(void)
{
#ifdef __clang__
    return (__m128)__builtin_ia32_undef128();
#else
    __m128 undef = undef;
    return undef;
#endif
}

__INTRIN_INLINE_SSE __m128 _mm_set_ss(float __w)
{
    return __extension__ (__m128){ __w, 0, 0, 0 };
}

// _mm_set_ps1
__INTRIN_INLINE_SSE __m128 _mm_set1_ps(float __w)
{
    return __extension__ (__m128){ __w, __w, __w, __w };
}

__INTRIN_INLINE_SSE __m128 _mm_set_ps(float __z, float __y, float __x, float __w)
{
    return __extension__ (__m128){ __w, __x, __y, __z };
}

__INTRIN_INLINE_SSE __m128 _mm_setr_ps(float __z, float __y, float __x, float __w)
{
    return __extension__ (__m128){ __z, __y, __x, __w };
}

__INTRIN_INLINE_SSE __m128 _mm_setzero_ps(void)
{
    return __extension__ (__m128){ 0, 0, 0, 0 };
}

__INTRIN_INLINE_SSE void _mm_storeh_pi(__m64 *__p, __m128 __a)
{
#ifdef __clang__
    typedef float __mm_storeh_pi_v2f32 __attribute__((__vector_size__(8)));
    struct __mm_storeh_pi_struct {
        __mm_storeh_pi_v2f32 __u;
    } __attribute__((__packed__, __may_alias__));
    ((struct __mm_storeh_pi_struct*)__p)->__u = __builtin_shufflevector(__a, __a, 2, 3);
#else
    __builtin_ia32_storehps(__p, __a);
#endif
}

__INTRIN_INLINE_SSE void _mm_storel_pi(__m64 *__p, __m128 __a)
{
#ifdef __clang__
    typedef float __mm_storeh_pi_v2f32 __attribute__((__vector_size__(8)));
    struct __mm_storeh_pi_struct {
        __mm_storeh_pi_v2f32 __u;
    } __attribute__((__packed__, __may_alias__));
    ((struct __mm_storeh_pi_struct*)__p)->__u = __builtin_shufflevector(__a, __a, 0, 1);
#else
    __builtin_ia32_storelps(__p, __a);
#endif
}

__INTRIN_INLINE_SSE void _mm_store_ss(float *__p, __m128 __a)
{
    *__p = ((__v4sf)__a)[0];
}

__INTRIN_INLINE_SSE void _mm_storeu_ps(float *__p, __m128 __a)
{
    *(__m128_u *)__p = __a;
}

__INTRIN_INLINE_SSE void _mm_store_ps(float *__p, __m128 __a)
{
    *(__m128*)__p = __a;
}

// _mm_store_ps1
__INTRIN_INLINE_SSE void _mm_store1_ps(float *__p, __m128 __a)
{
    // FIXME: Should we use a temp instead?
#ifdef __clang__
     __a = __builtin_shufflevector((__v4sf)__a, (__v4sf)__a, 0, 0, 0, 0);
#else
    __a = __builtin_ia32_shufps(__a, __a, _MM_SHUFFLE(0,0,0,0));
#endif
    _mm_store_ps(__p, __a);
}

__INTRIN_INLINE_SSE void _mm_storer_ps(float *__p, __m128 __a)
{
#ifdef  __clang__
    __m128 __tmp = __builtin_shufflevector((__v4sf)__a, (__v4sf)__a, 3, 2, 1, 0);
#else
    __m128 __tmp = __builtin_ia32_shufps(__a, __a, _MM_SHUFFLE(0,1,2,3));
#endif
    _mm_store_ps(__p, __tmp);
}

/* GCC / Clang specific consants */
#define _MM_HINT_NTA_ALT 0
#define _MM_HINT_T0_ALT  3
#define _MM_HINT_T1_ALT  2
#define _MM_HINT_T2_ALT  1
#define _MM_HINT_ENTA_ALT 4

// These are not supported yet
//#define _MM_HINT_ET0_ALT 7
//#define _MM_HINT_ET1_ALT 6
//#define _MM_HINT_ET2_ALT 5

#define _MM_HINT_MS_TO_ALT(sel) \
   (((sel) == _MM_HINT_NTA) ? _MM_HINT_NTA_ALT : \
    ((sel) == _MM_HINT_T0) ? _MM_HINT_T0_ALT : \
    ((sel) == _MM_HINT_T1) ? _MM_HINT_T1_ALT : \
    ((sel) == _MM_HINT_T2) ? _MM_HINT_T2_ALT : \
    ((sel) == _MM_HINT_ENTA) ? _MM_HINT_ENTA_ALT : 0)

#ifdef _MSC_VER1

/* On clang-cl we have an intrinsic, but the constants are different */
#pragma intrinsic(_mm_prefetch)
#define _mm_prefetch(p, sel) _mm_prefetch(p, _MM_HINT_MS_TO_ALT(sel))

#else /* _MSC_VER */

#define _mm_prefetch(p, sel) \
    __builtin_prefetch((const void *)(p), (_MM_HINT_MS_TO_ALT(sel) >> 2) & 1, _MM_HINT_MS_TO_ALT(sel) & 0x3)

#endif /* _MSC_VER */

__INTRIN_INLINE_SSE void _mm_stream_pi(__m64 *__p, __m64 __a)
{
#ifdef __clang__
    __builtin_ia32_movntq((__v1di*)__p, __a);
#else
    __builtin_ia32_movntq((long long unsigned int *)__p, (long long unsigned int)__a);
#endif
}

__INTRIN_INLINE_SSE void _mm_stream_ps(float *__p, __m128 __a)
{
#ifdef __clang__
    __builtin_nontemporal_store((__v4sf)__a, (__v4sf*)__p);
#else
    __builtin_ia32_movntps(__p, (__v4sf)__a);
#endif
}

#if !HAS_BUILTIN(_mm_sfence)
__INTRIN_INLINE_SSE void _mm_sfence(void)
{
    __builtin_ia32_sfence();
}
#endif

#ifdef __clang__
#define _m_pextrw(a, n) \
    ((int)__builtin_ia32_vec_ext_v4hi((__v4hi)a, (int)n))

#define _m_pinsrw(a, d, n) \
    ((__m64)__builtin_ia32_vec_set_v4hi((__v4hi)a, (int)d, (int)n))
#else
// _m_pextrw
__INTRIN_INLINE_SSE int _mm_extract_pi16(__m64 const __a, int const __n)
{
    return (unsigned short)__builtin_ia32_vec_ext_v4hi((__v4hi)__a, __n);
}

// _m_pinsrw
__INTRIN_INLINE_SSE __m64 _mm_insert_pi16 (__m64 const __a, int const __d, int const __n)
{
    return (__m64)__builtin_ia32_vec_set_v4hi ((__v4hi)__a, __d, __n);
}

#endif

// _m_pmaxsw
__INTRIN_INLINE_SSE __m64 _mm_max_pi16(__m64 __a, __m64 __b)
{
    return (__m64)__builtin_ia32_pmaxsw((__v4hi)__a, (__v4hi)__b);
}

// _m_pmaxub
__INTRIN_INLINE_SSE __m64 _mm_max_pu8(__m64 __a, __m64 __b)
{
    return (__m64)__builtin_ia32_pmaxub((__v8qi)__a, (__v8qi)__b);
}

// _m_pminsw
__INTRIN_INLINE_SSE __m64 _mm_min_pi16(__m64 __a, __m64 __b)
{
    return (__m64)__builtin_ia32_pminsw((__v4hi)__a, (__v4hi)__b);
}

// _m_pminub
__INTRIN_INLINE_SSE __m64 _mm_min_pu8(__m64 __a, __m64 __b)
{
    return (__m64)__builtin_ia32_pminub((__v8qi)__a, (__v8qi)__b);
}

// _m_pmovmskb
__INTRIN_INLINE_SSE int _mm_movemask_pi8(__m64 __a)
{
    return __builtin_ia32_pmovmskb((__v8qi)__a);
}

// _m_pmulhuw
__INTRIN_INLINE_SSE __m64 _mm_mulhi_pu16(__m64 __a, __m64 __b)
{
    return (__m64)__builtin_ia32_pmulhuw((__v4hi)__a, (__v4hi)__b);
}

#ifdef __clang__
#define _m_pshufw(a, n) \
    ((__m64)__builtin_ia32_pshufw((__v4hi)(__m64)(a), (n)))
#else
// _m_pshufw
__INTRIN_INLINE_MMX __m64 _mm_shuffle_pi16 (__m64 __a, int const __n)
{
    return (__m64) __builtin_ia32_pshufw ((__v4hi)__a, __n);
}
#endif

// _m_maskmovq
__INTRIN_INLINE_SSE void _mm_maskmove_si64(__m64 __d, __m64 __n, char *__p)
{
    __builtin_ia32_maskmovq((__v8qi)__d, (__v8qi)__n, __p);
}

// _m_pavgb
__INTRIN_INLINE_SSE __m64 _mm_avg_pu8(__m64 __a, __m64 __b)
{
    return (__m64)__builtin_ia32_pavgb((__v8qi)__a, (__v8qi)__b);
}

// _m_pavgw
__INTRIN_INLINE_SSE __m64 _mm_avg_pu16(__m64 __a, __m64 __b)
{
    return (__m64)__builtin_ia32_pavgw((__v4hi)__a, (__v4hi)__b);
}

// _m_psadbw
__INTRIN_INLINE_SSE __m64 _mm_sad_pu8(__m64 __a, __m64 __b)
{
    return (__m64)__builtin_ia32_psadbw((__v8qi)__a, (__v8qi)__b);
}

#endif // __GNUC__

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _INCLUDED_MM2 */
