/*
 * mmintrin.h
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
#ifndef _MMINTRIN_H_INCLUDED
#define _MMINTRIN_H_INCLUDED

#include <vcruntime.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define DECLSPEC_INTRINTYPE __declspec(intrin_type)
#else
#define DECLSPEC_INTRINTYPE
#endif

#if defined(_MSC_VER) && !defined(__clang__)

    typedef union DECLSPEC_INTRINTYPE _CRT_ALIGN(8) __m64
    {
        unsigned __int64 m64_u64;
        float m64_f32[2];
        __int8 m64_i8[8];
        __int16 m64_i16[4];
        __int32 m64_i32[2];
        __int64 m64_i64;
        unsigned __int8 m64_u8[8];
        unsigned __int16 m64_u16[4];
        unsigned __int32 m64_u32[2];
    } __m64;

#else /* _MSC_VER */

    typedef long long __v1di __attribute__((__vector_size__(8)));
    typedef int __v2si __attribute__((__vector_size__(8)));
    typedef short __v4hi __attribute__((__vector_size__(8)));
    typedef char __v8qi __attribute__((__vector_size__(8)));

    typedef float __m64 __attribute__((__vector_size__(8), __aligned__(16)));

#ifdef __clang__
#define __INTRIN_INLINE_MMX __INTRIN_INLINE __attribute__((__target__("mmx"),__min_vector_width__(64)))
#else
#define __INTRIN_INLINE_MMX __INTRIN_INLINE __attribute__((__target__("mmx")))
#endif

#endif /* _MSC_VER */

#ifdef _M_IX86

void  _m_empty(void);
__m64 _m_from_int(int i);
int   _m_to_int(__m64 m);
__m64 _m_packsswb(__m64 a, __m64 b);
__m64 _m_packssdw(__m64 a, __m64 b);
__m64 _m_packuswb(__m64 a, __m64 b);
__m64 _m_punpckhbw(__m64 a, __m64 b);
__m64 _m_punpckhwd(__m64 a, __m64 b);
__m64 _m_punpckhdq(__m64 a, __m64 b);
__m64 _m_punpcklbw(__m64 a, __m64 b);
__m64 _m_punpcklwd(__m64 a, __m64 b);
__m64 _m_punpckldq(__m64 a, __m64 b);
__m64 _m_paddb(__m64 a, __m64 b);
__m64 _m_paddw(__m64 a, __m64 b);
__m64 _m_paddd(__m64 a, __m64 b);
__m64 _m_paddsb(__m64 a, __m64 b);
__m64 _m_paddsw(__m64 a, __m64 b);
__m64 _m_paddusb(__m64 a, __m64 b);
__m64 _m_paddusw(__m64 a, __m64 b);
__m64 _m_psubb(__m64 a, __m64 b);
__m64 _m_psubw(__m64 a, __m64 b);
__m64 _m_psubd(__m64 a, __m64 b);
__m64 _m_psubsb(__m64 a, __m64 b);
__m64 _m_psubsw(__m64 a, __m64 b);
__m64 _m_psubusb(__m64 a, __m64 b);
__m64 _m_psubusw(__m64 a, __m64 b);
__m64 _m_pmaddwd(__m64 a, __m64 b);
__m64 _m_pmulhw(__m64 a, __m64 b);
__m64 _m_pmullw(__m64 a, __m64 b);
__m64 _m_psllw(__m64 a, __m64 count);
__m64 _m_psllwi(__m64 a, int imm8);
__m64 _m_pslld(__m64 a, __m64 count);
__m64 _m_pslldi(__m64 a, int imm8);
__m64 _m_psllq(__m64 a, __m64 count);
__m64 _m_psllqi(__m64 a, int imm8);
__m64 _m_psraw(__m64 a, __m64 count);
__m64 _m_psrawi(__m64 a, int imm8);
__m64 _m_psrad(__m64 a, __m64 count);
__m64 _m_psradi(__m64 a, int imm8);
__m64 _m_psrlw(__m64 a, __m64 count);
__m64 _m_psrlwi(__m64 a, int imm8);
__m64 _m_psrld(__m64 a, __m64 count);
__m64 _m_psrldi(__m64 a, int imm8);
__m64 _m_psrlq(__m64 a, __m64 count);
__m64 _m_psrlqi(__m64 a, int imm8);
__m64 _m_pand(__m64 a, __m64 b);
__m64 _m_pandn(__m64 a, __m64 b);
__m64 _m_por(__m64 a, __m64 b);
__m64 _m_pxor(__m64 a, __m64 b);
__m64 _m_pcmpeqb(__m64 a, __m64 b);
__m64 _m_pcmpgtb(__m64 a, __m64 b);
__m64 _m_pcmpeqw(__m64 a, __m64 b);
__m64 _m_pcmpgtw(__m64 a, __m64 b);
__m64 _m_pcmpeqd(__m64 a, __m64 b);
__m64 _m_pcmpgtd(__m64 a, __m64 b);
__m64 _mm_setzero_si64(void);
__m64 _mm_set_pi32(int i1, int i0);
__m64 _mm_set_pi16(short s3, short s2, short s1, short s0);
__m64 _mm_set_pi8(char b7, char b6, char b5, char b4,
                  char b3, char b2, char b1, char b0);
__m64 _mm_setr_pi32(int i1, int i0);
__m64 _mm_setr_pi16(short s3, short s2, short s1, short s0);
__m64 _mm_setr_pi8(char b7, char b6, char b5, char b4,
                   char b3, char b2, char b1, char b0);
__m64 _mm_set1_pi32(int i);
__m64 _mm_set1_pi16(short s);
__m64 _mm_set1_pi8(char b);

/* Alternate names */
#define _mm_empty _m_empty
#define _mm_cvtsi32_si64 _m_from_int
#define _mm_cvtsi64_si32 _m_to_int
#define _mm_packs_pi16 _m_packsswb
#define _mm_packs_pi32 _m_packssdw
#define _mm_packs_pu16 _m_packuswb
#define _mm_unpackhi_pi8 _m_punpckhbw
#define _mm_unpackhi_pi16 _m_punpckhwd
#define _mm_unpackhi_pi32 _m_punpckhdq
#define _mm_unpacklo_pi8 _m_punpcklbw
#define _mm_unpacklo_pi16 _m_punpcklwd
#define _mm_unpacklo_pi32 _m_punpckldq
#define _mm_add_pi8 _m_paddb
#define _mm_add_pi16 _m_paddw
#define _mm_add_pi32 _m_paddd
#define _mm_adds_pi8 _m_paddsb
#define _mm_adds_pi16 _m_paddsw
#define _mm_adds_pu8 _m_paddusb
#define _mm_adds_pu16 _m_paddusw
#define _mm_sub_pi8 _m_psubb
#define _mm_sub_pi16 _m_psubw
#define _mm_sub_pi32 _m_psubd
#define _mm_subs_pi8 _m_psubsb
#define _mm_subs_pi16 _m_psubsw
#define _mm_subs_pu8 _m_psubusb
#define _mm_subs_pu16 _m_psubusw
#define _mm_madd_pi16 _m_pmaddwd
#define _mm_mulhi_pi16 _m_pmulhw
#define _mm_mullo_pi16 _m_pmullw
#define _mm_sll_pi16 _m_psllw
#define _mm_slli_pi16 _m_psllwi
#define _mm_sll_pi32 _m_pslld
#define _mm_slli_pi32 _m_pslldi
#define _mm_sll_si64 _m_psllq
#define _mm_slli_si64 _m_psllqi
#define _mm_sra_pi16 _m_psraw
#define _mm_srai_pi16 _m_psrawi
#define _mm_sra_pi32 _m_psrad
#define _mm_srai_pi32 _m_psradi
#define _mm_srl_pi16 _m_psrlw
#define _mm_srli_pi16 _m_psrlwi
#define _mm_srl_pi32 _m_psrld
#define _mm_srli_pi32 _m_psrldi
#define _mm_srl_si64 _m_psrlq
#define _mm_srli_si64 _m_psrlqi
#define _mm_and_si64 _m_pand
#define _mm_andnot_si64 _m_pandn
#define _mm_or_si64 _m_por
#define _mm_xor_si64 _m_pxor
#define _mm_cmpeq_pi8 _m_pcmpeqb
#define _mm_cmpgt_pi8 _m_pcmpgtb
#define _mm_cmpeq_pi16 _m_pcmpeqw
#define _mm_cmpgt_pi16 _m_pcmpgtw
#define _mm_cmpeq_pi32 _m_pcmpeqd
#define _mm_cmpgt_pi32 _m_pcmpgtd

/* Use intrinsics on MSVC */
#if defined(_MSC_VER) && !defined(__clang__)
#pragma intrinsic(_m_empty)
#pragma intrinsic(_m_from_int)
#pragma intrinsic(_m_to_int)
#pragma intrinsic(_m_packsswb)
#pragma intrinsic(_m_packssdw)
#pragma intrinsic(_m_packuswb)
#pragma intrinsic(_m_punpckhbw)
#pragma intrinsic(_m_punpckhwd)
#pragma intrinsic(_m_punpckhdq)
#pragma intrinsic(_m_punpcklbw)
#pragma intrinsic(_m_punpcklwd)
#pragma intrinsic(_m_punpckldq)
#pragma intrinsic(_m_paddb)
#pragma intrinsic(_m_paddw)
#pragma intrinsic(_m_paddd)
#pragma intrinsic(_m_paddsb)
#pragma intrinsic(_m_paddsw)
#pragma intrinsic(_m_paddusb)
#pragma intrinsic(_m_paddusw)
#pragma intrinsic(_m_psubb)
#pragma intrinsic(_m_psubw)
#pragma intrinsic(_m_psubd)
#pragma intrinsic(_m_psubsb)
#pragma intrinsic(_m_psubsw)
#pragma intrinsic(_m_psubusb)
#pragma intrinsic(_m_psubusw)
#pragma intrinsic(_m_pmaddwd)
#pragma intrinsic(_m_pmulhw)
#pragma intrinsic(_m_pmullw)
#pragma intrinsic(_m_psllw)
#pragma intrinsic(_m_psllwi)
#pragma intrinsic(_m_pslld)
#pragma intrinsic(_m_pslldi)
#pragma intrinsic(_m_psllq)
#pragma intrinsic(_m_psllqi)
#pragma intrinsic(_m_psraw)
#pragma intrinsic(_m_psrawi)
#pragma intrinsic(_m_psrad)
#pragma intrinsic(_m_psradi)
#pragma intrinsic(_m_psrlw)
#pragma intrinsic(_m_psrlwi)
#pragma intrinsic(_m_psrld)
#pragma intrinsic(_m_psrldi)
#pragma intrinsic(_m_psrlq)
#pragma intrinsic(_m_psrlqi)
#pragma intrinsic(_m_pand)
#pragma intrinsic(_m_pandn)
#pragma intrinsic(_m_por)
#pragma intrinsic(_m_pxor)
#pragma intrinsic(_m_pcmpeqb)
#pragma intrinsic(_m_pcmpgtb)
#pragma intrinsic(_m_pcmpeqw)
#pragma intrinsic(_m_pcmpgtw)
#pragma intrinsic(_m_pcmpeqd)
#pragma intrinsic(_m_pcmpgtd)
#pragma intrinsic(_mm_setzero_si64)
#pragma intrinsic(_mm_set_pi32)
#pragma intrinsic(_mm_set_pi16)
#pragma intrinsic(_mm_set_pi8)
#pragma intrinsic(_mm_setr_pi32)
#pragma intrinsic(_mm_setr_pi16)
#pragma intrinsic(_mm_setr_pi8)
#pragma intrinsic(_mm_set1_pi32)
#pragma intrinsic(_mm_set1_pi16)
#pragma intrinsic(_mm_set1_pi8)

/* Use inline functions on GCC/Clang */
#else // GCC / Clang  Clang-CL

/*
- GCC: https://github.com/gcc-mirror/gcc/blob/master/gcc/config/i386/mmintrin.h
- Clang: https://github.com/llvm/llvm-project/blob/main/clang/lib/Headers/mmintrin.h
*/

// _m_empty
__INTRIN_INLINE_MMX void _mm_empty(void)
{
    __builtin_ia32_emms();
}

// _m_from_int
__INTRIN_INLINE_MMX __m64 _mm_cvtsi32_si64(int i)
{
    return (__m64)__builtin_ia32_vec_init_v2si(i, 0);
}

// _m_to_int
__INTRIN_INLINE_MMX int _mm_cvtsi64_si32(__m64 m)
{
    return __builtin_ia32_vec_ext_v2si((__v2si)m, 0);
}

// _m_packsswb
__INTRIN_INLINE_MMX __m64 _mm_packs_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_packsswb((__v4hi)a, (__v4hi)b);
}

// _m_packssdw
__INTRIN_INLINE_MMX __m64 _mm_packs_pi32(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_packssdw((__v2si)a, (__v2si)b);
}

// _m_packuswb
__INTRIN_INLINE_MMX __m64 _mm_packs_pu16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_packuswb((__v4hi)a, (__v4hi)b);
}

// _m_punpckhbw
__INTRIN_INLINE_MMX __m64 _mm_unpackhi_pi8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_punpckhbw((__v8qi)a, (__v8qi)b);
}

// _m_punpckhwd
__INTRIN_INLINE_MMX __m64 _mm_unpackhi_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_punpckhwd((__v4hi)a, (__v4hi)b);
}

// _m_punpckhdq
__INTRIN_INLINE_MMX __m64 _mm_unpackhi_pi32(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_punpckhdq((__v2si)a, (__v2si)b);
}

// _m_punpcklbw
__INTRIN_INLINE_MMX __m64 _mm_unpacklo_pi8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_punpcklbw((__v8qi)a, (__v8qi)b);
}

// _m_punpcklwd
__INTRIN_INLINE_MMX __m64 _mm_unpacklo_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_punpcklwd((__v4hi)a, (__v4hi)b);
}

// _m_punpckldq
__INTRIN_INLINE_MMX __m64 _mm_unpacklo_pi32(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_punpckldq((__v2si)a, (__v2si)b);
}

// _m_paddb
__INTRIN_INLINE_MMX __m64 _mm_add_pi8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_paddb((__v8qi)a, (__v8qi)b);
}

// _m_paddw
__INTRIN_INLINE_MMX __m64 _mm_add_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_paddw((__v4hi)a, (__v4hi)b);
}

// _m_paddd
__INTRIN_INLINE_MMX __m64 _mm_add_pi32(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_paddd((__v2si)a, (__v2si)b);
}

// _m_paddsb
__INTRIN_INLINE_MMX __m64 _mm_adds_pi8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_paddsb((__v8qi)a, (__v8qi)b);
}

// _m_paddsw
__INTRIN_INLINE_MMX __m64 _mm_adds_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_paddsw((__v4hi)a, (__v4hi)b);
}

// _m_paddusb
__INTRIN_INLINE_MMX __m64 _mm_adds_pu8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_paddusb((__v8qi)a, (__v8qi)b);
}

// _m_paddusw
__INTRIN_INLINE_MMX __m64 _mm_adds_pu16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_paddusw((__v4hi)a, (__v4hi)b);
}

// _m_psubb
__INTRIN_INLINE_MMX __m64 _mm_sub_pi8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_psubb((__v8qi)a, (__v8qi)b);
}

// _m_psubw
__INTRIN_INLINE_MMX __m64 _mm_sub_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_psubw((__v4hi)a, (__v4hi)b);
}

// _m_psubd
__INTRIN_INLINE_MMX __m64 _mm_sub_pi32(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_psubd((__v2si)a, (__v2si)b);
}

// _m_psubsb
__INTRIN_INLINE_MMX __m64 _mm_subs_pi8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_psubsb((__v8qi)a, (__v8qi)b);
}

// _m_psubsw
__INTRIN_INLINE_MMX __m64 _mm_subs_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_psubsw((__v4hi)a, (__v4hi)b);
}

// _m_psubusb
__INTRIN_INLINE_MMX __m64 _mm_subs_pu8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_psubusb((__v8qi)a, (__v8qi)b);
}

// _m_psubusw
__INTRIN_INLINE_MMX __m64 _mm_subs_pu16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_psubusw((__v4hi)a, (__v4hi)b);
}

// _m_pmaddwd
__INTRIN_INLINE_MMX __m64 _mm_madd_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pmaddwd((__v4hi)a, (__v4hi)b);
}

// _m_pmulhw
__INTRIN_INLINE_MMX __m64 _mm_mulhi_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pmulhw((__v4hi)a, (__v4hi)b);
}

// _m_pmullw
__INTRIN_INLINE_MMX __m64 _mm_mullo_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pmullw((__v4hi)a, (__v4hi)b);
}

// _m_psllw
__INTRIN_INLINE_MMX __m64 _mm_sll_pi16(__m64 a, __m64 count)
{
    return (__m64)__builtin_ia32_psllw((__v4hi)a, (__v4hi)count);
}

// _m_psllwi
__INTRIN_INLINE_MMX __m64 _mm_slli_pi16(__m64 a, int imm8)
{
    return (__m64)__builtin_ia32_psllwi((__v4hi)a, imm8);
}

// _m_pslld
__INTRIN_INLINE_MMX __m64 _mm_sll_pi32(__m64 a, __m64 count)
{
    return (__m64)__builtin_ia32_pslld((__v2si)a, (__v2si)count);
}

// _m_pslldi
__INTRIN_INLINE_MMX __m64 _mm_slli_pi32(__m64 a, int imm8)
{
    return (__m64)__builtin_ia32_pslldi((__v2si)a, imm8);
}

// _m_psllq
__INTRIN_INLINE_MMX __m64 _mm_sll_si64(__m64 a, __m64 count)
{
    return (__m64)__builtin_ia32_psllq((__v1di)a, (__v1di)count);
}

// _m_psllqi
__INTRIN_INLINE_MMX __m64 _mm_slli_si64(__m64 a, int imm8)
{
    return (__m64)__builtin_ia32_psllqi((__v1di)a, imm8);
}

// _m_psraw
__INTRIN_INLINE_MMX __m64 _mm_sra_pi16(__m64 a, __m64 count)
{
    return (__m64)__builtin_ia32_psraw((__v4hi)a, (__v4hi)count);
}

// _m_psrawi
__INTRIN_INLINE_MMX __m64 _mm_srai_pi16(__m64 a, int imm8)
{
    return (__m64)__builtin_ia32_psrawi((__v4hi)a, imm8);
}

// _m_psrad
__INTRIN_INLINE_MMX __m64 _mm_sra_pi32(__m64 a, __m64 count)
{
    return (__m64)__builtin_ia32_psrad((__v2si)a, (__v2si)count);
}

// _m_psradi
__INTRIN_INLINE_MMX __m64 _mm_srai_pi32(__m64 a, int imm8)
{
    return (__m64)__builtin_ia32_psradi((__v2si)a, imm8);
}

// _m_psrlw
__INTRIN_INLINE_MMX __m64 _mm_srl_pi16(__m64 a, __m64 count)
{
    return (__m64)__builtin_ia32_psrlw((__v4hi)a, (__v4hi)count);
}

// _m_psrlwi
__INTRIN_INLINE_MMX __m64 _mm_srli_pi16(__m64 a, int imm8)
{
    return (__m64)__builtin_ia32_psrlwi((__v4hi)a, imm8);
}

// _m_psrld
__INTRIN_INLINE_MMX __m64 _mm_srl_pi32(__m64 a, __m64 count)
{
    return (__m64)__builtin_ia32_psrld((__v2si)a, (__v2si)count);
}

// _m_psrldi
__INTRIN_INLINE_MMX __m64 _mm_srli_pi32(__m64 a, int imm8)
{
    return (__m64)__builtin_ia32_psrldi((__v2si)a, imm8);
}

// _m_psrlq
__INTRIN_INLINE_MMX __m64 _mm_srl_si64(__m64 a, __m64 count)
{
    return (__m64)__builtin_ia32_psrlq((__v1di)a, (__v1di)count);
}

// _m_psrlqi
__INTRIN_INLINE_MMX __m64 _mm_srli_si64(__m64 a, int imm8)
{
    return (__m64)__builtin_ia32_psrlqi((__v1di)a, imm8);
}

// _m_pand
__INTRIN_INLINE_MMX __m64 _mm_and_si64(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pand((__v2si)a, (__v2si)b);
}

// _m_pandn
__INTRIN_INLINE_MMX __m64 _mm_andnot_si64(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pandn((__v2si)a, (__v2si)b);
}

// _m_por
__INTRIN_INLINE_MMX __m64 _mm_or_si64(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_por((__v2si)a, (__v2si)b);
}

// _m_pxor
__INTRIN_INLINE_MMX __m64 _mm_xor_si64(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pxor((__v2si)a, (__v2si)b);
}

// _m_pcmpeqb
__INTRIN_INLINE_MMX __m64 _mm_cmpeq_pi8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pcmpeqb((__v8qi)a, (__v8qi)b);
}

// _m_pcmpgtb
__INTRIN_INLINE_MMX __m64 _mm_cmpgt_pi8(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pcmpgtb((__v8qi)a, (__v8qi)b);
}

// _m_pcmpeqw
__INTRIN_INLINE_MMX __m64 _mm_cmpeq_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pcmpeqw((__v4hi)a, (__v4hi)b);
}

// _m_pcmpgtw
__INTRIN_INLINE_MMX __m64 _mm_cmpgt_pi16(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pcmpgtw((__v4hi)a, (__v4hi)b);
}

// _m_pcmpeqd
__INTRIN_INLINE_MMX __m64 _mm_cmpeq_pi32(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pcmpeqd((__v2si)a, (__v2si)b);
}

// _m_pcmpgtd
__INTRIN_INLINE_MMX __m64 _mm_cmpgt_pi32(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pcmpgtd((__v2si)a, (__v2si)b);
}

__INTRIN_INLINE_MMX __m64 _mm_setzero_si64(void)
{
    return (__m64) { 0 };
}

__INTRIN_INLINE_MMX __m64 _mm_set_pi32(int i1, int i0)
{
    return (__m64)__builtin_ia32_vec_init_v2si(i0, i1);
}

__INTRIN_INLINE_MMX __m64 _mm_set_pi16(short s3, short s2, short s1, short s0)
{
    return (__m64)__builtin_ia32_vec_init_v4hi(s0, s1, s2, s3);
}

__INTRIN_INLINE_MMX __m64 _mm_set_pi8(char b7, char b6, char b5, char b4,
                                  char b3, char b2, char b1, char b0)
{
    return (__m64)__builtin_ia32_vec_init_v8qi(b0, b1, b2, b3, b4, b5, b6, b7);
}

__INTRIN_INLINE_MMX __m64 _mm_setr_pi32(int i1, int i0)
{
    return _mm_set_pi32(i0, i1);
}

__INTRIN_INLINE_MMX __m64 _mm_setr_pi16(short s3, short s2, short s1, short s0)
{
    return _mm_set_pi16(s0, s1, s2, s3);
}

__INTRIN_INLINE_MMX __m64 _mm_setr_pi8(char b7, char b6, char b5, char b4,
                                   char b3, char b2, char b1, char b0)
{
    return _mm_set_pi8(b7, b6, b5, b4, b3, b2, b1, b0);
}

__INTRIN_INLINE_MMX __m64 _mm_set1_pi32(int i)
{
    return _mm_set_pi32(i, i);
}

__INTRIN_INLINE_MMX __m64 _mm_set1_pi16(short s)
{
    return _mm_set_pi16(s, s, s, s);
}

__INTRIN_INLINE_MMX __m64 _mm_set1_pi8(char b)
{
    return _mm_set_pi8(b, b, b, b, b, b, b, b);
}

#endif /* __GNUC__ */

#endif /* _M_IX86 */

#ifdef __cplusplus
}
#endif

#endif /* _MMINTRIN_H_INCLUDED */
