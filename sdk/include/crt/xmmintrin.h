/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#pragma once
#ifndef _INCLUDED_MM2
#define _INCLUDED_MM2

#include <crtdefs.h>
#include <mmintrin.h>

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


#ifdef __cplusplus
extern "C" {
#endif

extern __m128 _mm_load_ss(float const*);
extern int _mm_cvt_ss2si(__m128);
__m128 _mm_xor_ps(__m128 a, __m128 b);
__m128 _mm_div_ps(__m128 a, __m128 b);

#ifdef _MSC_VER
unsigned int _mm_getcsr(void);
#pragma intrinsic(_mm_getcsr)
void _mm_setcsr(unsigned int);
#pragma intrinsic(_mm_setcsr)
#pragma intrinsic(_mm_xor_ps)
#pragma intrinsic(_mm_div_ps)
#else

#ifndef __INTRIN_INLINE
#ifdef __clang__
#define __INTRIN_INLINE __forceinline
#else
#define __ATTRIBUTE_ARTIFICIAL __attribute__((artificial))
#define __INTRIN_INLINE extern __inline__ __attribute__((__always_inline__,__gnu_inline__)) __ATTRIBUTE_ARTIFICIAL
#endif
#endif

/* 
 * We can't use __builtin_ia32_* functions,
 * are they are only available with the -msse2 compiler switch
 */
__INTRIN_INLINE unsigned int _mm_getcsr(void)
{
    unsigned int retval;
    __asm__ __volatile__("stmxcsr %0" : "=m"(retval));
    return retval;
}

__INTRIN_INLINE void _mm_setcsr(unsigned int val)
{
    __asm__ __volatile__("ldmxcsr %0" : : "m"(val));
}
#endif

/* Alternate names */
#define _mm_cvtss_si32 _mm_cvt_ss2si

#ifdef __cplusplus
}
#endif

/* _mm_prefetch constants */
#define _MM_HINT_T0 1
#define _MM_HINT_T1 2
#define _MM_HINT_T2 3
#define _MM_HINT_NTA 0
#define _MM_HINT_ET1 6


#endif /* _INCLUDED_MM2 */
