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

typedef struct _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(16) __m128d
{
    double m128d_f64[2];
} __m128d;

extern __m128d _mm_load_sd(double const*);

extern int _mm_cvtsd_si32(__m128d);


#endif /* _INCLUDED_EMM */
