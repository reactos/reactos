/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * SSE intrinsics portability header.
 * 
 * Although the SSE intrinsics are support by all modern x86 and x86-64 
 * compilers, there are some intrisincs missing in some implementations 
 * (especially older MSVC versions). This header abstracts that away.
 */

#ifndef U_SSE_H_
#define U_SSE_H_

#include "pipe/p_config.h"

#if defined(PIPE_ARCH_SSE)

#include <emmintrin.h>


/* MSVC before VC8 does not support the _mm_castxxx_yyy */
#if defined(_MSC_VER) && _MSC_VER < 1500

union __declspec(align(16)) m128_types {
   __m128 m128;
   __m128i m128i;
   __m128d m128d;
};

static __inline __m128
_mm_castsi128_ps(__m128i a)
{
   union m128_types u;
   u.m128i = a;
   return u.m128;
}

static __inline __m128i
_mm_castps_si128(__m128 a)
{
   union m128_types u;
   u.m128 = a;
   return u.m128i;
}

#endif /* defined(_MSC_VER) && _MSC_VER < 1500 */

union m128i {
   __m128i m;
   ubyte ub[16];
   ushort us[8];
   uint ui[4];
};

static INLINE void u_print_epi8(const char *name, __m128i r)
{
   union { __m128i m; ubyte ub[16]; } u;
   u.m = r;

   debug_printf("%s: "
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x/"
                "%02x\n",
                name,
                u.ub[0],  u.ub[1],  u.ub[2],  u.ub[3],
                u.ub[4],  u.ub[5],  u.ub[6],  u.ub[7],
                u.ub[8],  u.ub[9],  u.ub[10], u.ub[11],
                u.ub[12], u.ub[13], u.ub[14], u.ub[15]);
}

static INLINE void u_print_epi16(const char *name, __m128i r)
{
   union { __m128i m; ushort us[8]; } u;
   u.m = r;

   debug_printf("%s: "
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x/"
                "%04x\n",
                name,
                u.us[0],  u.us[1],  u.us[2],  u.us[3],
                u.us[4],  u.us[5],  u.us[6],  u.us[7]);
}

static INLINE void u_print_epi32(const char *name, __m128i r)
{
   union { __m128i m; uint ui[4]; } u;
   u.m = r;

   debug_printf("%s: "
                "%08x/"
                "%08x/"
                "%08x/"
                "%08x\n",
                name,
                u.ui[0],  u.ui[1],  u.ui[2],  u.ui[3]);
}

static INLINE void u_print_ps(const char *name, __m128 r)
{
   union { __m128 m; float f[4]; } u;
   u.m = r;

   debug_printf("%s: "
                "%f/"
                "%f/"
                "%f/"
                "%f\n",
                name,
                u.f[0],  u.f[1],  u.f[2],  u.f[3]);
}


#define U_DUMP_EPI32(a) u_print_epi32(#a, a)
#define U_DUMP_EPI16(a) u_print_epi16(#a, a)
#define U_DUMP_EPI8(a)  u_print_epi8(#a, a)
#define U_DUMP_PS(a)    u_print_ps(#a, a)



#if defined(PIPE_ARCH_SSSE3)

#include <tmmintrin.h>

#else /* !PIPE_ARCH_SSSE3 */

/**
 * Describe _mm_shuffle_epi8() with gcc extended inline assembly, for cases
 * where -mssse3 is not supported/enabled.
 *
 * MSVC will never get in here as its intrinsics support do not rely on
 * compiler command line options.
 */
static __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_shuffle_epi8(__m128i a, __m128i mask)
{
    __m128i result;
    __asm__("pshufb %1, %0"
            : "=x" (result)
            : "xm" (mask), "0" (a));
    return result;
}

#endif /* !PIPE_ARCH_SSSE3 */




/* Provide an SSE2 implementation of _mm_mullo_epi32() in terms of
 * _mm_mul_epu32().
 *
 * I suspect this works fine for us because one of our operands is
 * always positive, but not sure that this can be used for general
 * signed integer multiplication.
 *
 * This seems close enough to the speed of SSE4 and the real
 * _mm_mullo_epi32() intrinsic as to not justify adding an sse4
 * dependency at this point.
 */
static INLINE __m128i mm_mullo_epi32(const __m128i a, const __m128i b)
{
   __m128i a4   = _mm_srli_epi64(a, 32);  /* shift by one dword */
   __m128i b4   = _mm_srli_epi64(b, 32);  /* shift by one dword */
   __m128i ba   = _mm_mul_epu32(b, a);   /* multply dwords 0, 2 */
   __m128i b4a4 = _mm_mul_epu32(b4, a4); /* multiply dwords 1, 3 */

   /* Interleave the results, either with shuffles or (slightly
    * faster) direct bit operations:
    */
#if 0
   __m128i ba8             = _mm_shuffle_epi32(ba, 8);
   __m128i b4a48           = _mm_shuffle_epi32(b4a4, 8);
   __m128i result          = _mm_unpacklo_epi32(ba8, b4a48);
#else
   __m128i mask            = _mm_setr_epi32(~0,0,~0,0);
   __m128i ba_mask         = _mm_and_si128(ba, mask);
   __m128i b4a4_mask_shift = _mm_slli_epi64(b4a4, 32);
   __m128i result          = _mm_or_si128(ba_mask, b4a4_mask_shift);
#endif

   return result;
}


static INLINE void
transpose4_epi32(const __m128i * restrict a,
                 const __m128i * restrict b,
                 const __m128i * restrict c,
                 const __m128i * restrict d,
                 __m128i * restrict o,
                 __m128i * restrict p,
                 __m128i * restrict q,
                 __m128i * restrict r)
{
  __m128i t0 = _mm_unpacklo_epi32(*a, *b);
  __m128i t1 = _mm_unpacklo_epi32(*c, *d);
  __m128i t2 = _mm_unpackhi_epi32(*a, *b);
  __m128i t3 = _mm_unpackhi_epi32(*c, *d);

  *o = _mm_unpacklo_epi64(t0, t1);
  *p = _mm_unpackhi_epi64(t0, t1);
  *q = _mm_unpacklo_epi64(t2, t3);
  *r = _mm_unpackhi_epi64(t2, t3);
}

#define SCALAR_EPI32(m, i) _mm_shuffle_epi32((m), _MM_SHUFFLE(i,i,i,i))


#endif /* PIPE_ARCH_SSE */

#endif /* U_SSE_H_ */
