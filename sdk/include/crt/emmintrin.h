/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#pragma once
#ifndef _INCLUDED_EMM
#define _INCLUDED_EMM

#include <crtdefs.h>
#include <xmmintrin.h>

typedef union _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(16) __m128i
{
    __int8  m128i_i8[16];
    __int16 m128i_i16[8];
    __int32 m128i_i32[4];
    __int64 m128i_i64[2];
    unsigned __int8  m128i_u8[16];
    unsigned __int16 m128i_u16[8];
    unsigned __int32 m128i_u32[4];
    unsigned __int64 m128i_u64[2];
} __m128i;
C_ASSERT(sizeof(__m128i) == 16);

typedef struct _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(16) __m128d
{
    double m128d_f64[2];
} __m128d;

extern __m128d _mm_load_sd(double const*);

extern int _mm_cvtsd_si32(__m128d);

extern __m128i _mm_setzero_si128(void);

extern void _mm_stream_si128(__m128i *, __m128i);


#endif /* _INCLUDED_EMM */
