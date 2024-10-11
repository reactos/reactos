/*===---- emmintrin.h - SSE2 intrinsics ------------------------------------===
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

#pragma once
#ifndef _INCLUDED_EMM
#define _INCLUDED_EMM

#include <vcruntime.h>
#include <xmmintrin.h>

#if defined(_MSC_VER) && !defined(__clang__)

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
_STATIC_ASSERT(sizeof(__m128i) == 16);

typedef struct _DECLSPEC_INTRIN_TYPE _CRT_ALIGN(16) __m128d
{
    double m128d_f64[2];
} __m128d;

typedef __declspec(align(1)) __m128i __m128i_u;

#define __ATTRIBUTE_SSE2__

#else /* _MSC_VER */

typedef double __m128d __attribute__((__vector_size__(16), __aligned__(16)));
typedef long long __m128i __attribute__((__vector_size__(16), __aligned__(16)));

typedef double __m128d_u __attribute__((__vector_size__(16), __aligned__(1)));
typedef long long __m128i_u __attribute__((__vector_size__(16), __aligned__(1)));

/* Type defines.  */
typedef double __v2df __attribute__((__vector_size__(16)));
typedef long long __v2di __attribute__((__vector_size__(16)));
typedef short __v8hi __attribute__((__vector_size__(16)));
typedef char __v16qi __attribute__((__vector_size__(16)));

/* Unsigned types */
typedef unsigned long long __v2du __attribute__((__vector_size__(16)));
typedef unsigned short __v8hu __attribute__((__vector_size__(16)));
typedef unsigned char __v16qu __attribute__((__vector_size__(16)));

/* We need an explicitly signed variant for char. Note that this shouldn't
 * appear in the interface though. */
typedef signed char __v16qs __attribute__((__vector_size__(16)));

#ifdef __clang__
#define __ATTRIBUTE_SSE2__ __attribute__((__target__("sse2"),__min_vector_width__(128)))
#define __ATTRIBUTE_MMXSSE2__ __attribute__((__target__("mmx,sse2"),__min_vector_width__(128)))
#else
#define __ATTRIBUTE_SSE2__ __attribute__((__target__("sse2")))
#define __ATTRIBUTE_MMXSSE2__ __attribute__((__target__("mmx,sse2")))
#endif
#define __INTRIN_INLINE_SSE2 __INTRIN_INLINE __ATTRIBUTE_SSE2__
#define __INTRIN_INLINE_MMXSSE2 __INTRIN_INLINE __ATTRIBUTE_MMXSSE2__

#endif /* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif

extern __m128d _mm_add_sd(__m128d a, __m128d b);
extern __m128d _mm_add_pd(__m128d a, __m128d b);
extern __m128d _mm_sub_sd(__m128d a, __m128d b);
extern __m128d _mm_sub_pd(__m128d a, __m128d b);
extern __m128d _mm_mul_sd(__m128d a, __m128d b);
extern __m128d _mm_mul_pd(__m128d a, __m128d b);
extern __m128d _mm_div_sd(__m128d a, __m128d b);
extern __m128d _mm_div_pd(__m128d a, __m128d b);
extern __m128d _mm_sqrt_sd(__m128d a, __m128d b);
extern __m128d _mm_sqrt_pd(__m128d a);
extern __m128d _mm_min_sd(__m128d a, __m128d b);
extern __m128d _mm_min_pd(__m128d a, __m128d b);
extern __m128d _mm_max_sd(__m128d a, __m128d b);
extern __m128d _mm_max_pd(__m128d a, __m128d b);
extern __m128d _mm_and_pd(__m128d a, __m128d b);
extern __m128d _mm_andnot_pd(__m128d a, __m128d b);
extern __m128d _mm_or_pd(__m128d a, __m128d b);
extern __m128d _mm_xor_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpeq_pd(__m128d a, __m128d b);
extern __m128d _mm_cmplt_pd(__m128d a, __m128d b);
extern __m128d _mm_cmple_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpgt_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpge_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpord_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpunord_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpneq_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpnlt_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpnle_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpngt_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpnge_pd(__m128d a, __m128d b);
extern __m128d _mm_cmpeq_sd(__m128d a, __m128d b);
extern __m128d _mm_cmplt_sd(__m128d a, __m128d b);
extern __m128d _mm_cmple_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpgt_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpge_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpord_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpunord_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpneq_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpnlt_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpnle_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpngt_sd(__m128d a, __m128d b);
extern __m128d _mm_cmpnge_sd(__m128d a, __m128d b);
extern int _mm_comieq_sd(__m128d a, __m128d b);
extern int _mm_comilt_sd(__m128d a, __m128d b);
extern int _mm_comile_sd(__m128d a, __m128d b);
extern int _mm_comigt_sd(__m128d a, __m128d b);
extern int _mm_comige_sd(__m128d a, __m128d b);
extern int _mm_comineq_sd(__m128d a, __m128d b);
extern int _mm_ucomieq_sd(__m128d a, __m128d b);
extern int _mm_ucomilt_sd(__m128d a, __m128d b);
extern int _mm_ucomile_sd(__m128d a, __m128d b);
extern int _mm_ucomigt_sd(__m128d a, __m128d b);
extern int _mm_ucomige_sd(__m128d a, __m128d b);
extern int _mm_ucomineq_sd(__m128d a, __m128d b);
extern __m128 _mm_cvtpd_ps(__m128d a);
extern __m128d _mm_cvtps_pd(__m128 a);
extern __m128d _mm_cvtepi32_pd(__m128i a);
extern __m128i _mm_cvtpd_epi32(__m128d a);
extern int _mm_cvtsd_si32(__m128d a);
extern __m128 _mm_cvtsd_ss(__m128 a, __m128d b);
extern __m128d _mm_cvtsi32_sd(__m128d a, int b);
extern __m128d _mm_cvtss_sd(__m128d a, __m128 b);
extern __m128i _mm_cvttpd_epi32(__m128d a);
extern int _mm_cvttsd_si32(__m128d a);
extern __m64 _mm_cvtpd_pi32(__m128d a);
extern __m64 _mm_cvttpd_pi32(__m128d a);
extern __m128d _mm_cvtpi32_pd(__m64 a);
extern double _mm_cvtsd_f64(__m128d a);
extern __m128d _mm_load_pd(double const *dp);
extern __m128d _mm_load1_pd(double const *dp);
extern __m128d _mm_loadr_pd(double const *dp);
extern __m128d _mm_loadu_pd(double const *dp);
//extern __m128i _mm_loadu_si64(void const *a);
//extern __m128i _mm_loadu_si32(void const *a);
//extern __m128i _mm_loadu_si16(void const *a);
extern __m128d _mm_load_sd(double const *dp);
extern __m128d _mm_loadh_pd(__m128d a, double const *dp);
extern __m128d _mm_loadl_pd(__m128d a, double const *dp);
//extern __m128d _mm_undefined_pd(void);
extern __m128d _mm_set_sd(double w);
extern __m128d _mm_set1_pd(double w);
extern __m128d _mm_set_pd(double w, double x);
extern __m128d _mm_setr_pd(double w, double x);
extern __m128d _mm_setzero_pd(void);
extern __m128d _mm_move_sd(__m128d a, __m128d b);
extern void _mm_store_sd(double *dp, __m128d a);
extern void _mm_store_pd(double *dp, __m128d a);
extern void _mm_store1_pd(double *dp, __m128d a);
extern void _mm_storeu_pd(double *dp, __m128d a);
extern void _mm_storer_pd(double *dp, __m128d a);
extern void _mm_storeh_pd(double *dp, __m128d a);
extern void _mm_storel_pd(double *dp, __m128d a);
extern __m128i _mm_add_epi8(__m128i a, __m128i b);
extern __m128i _mm_add_epi16(__m128i a, __m128i b);
extern __m128i _mm_add_epi32(__m128i a, __m128i b);
extern __m64 _mm_add_si64(__m64 a, __m64 b);
extern __m128i _mm_add_epi64(__m128i a, __m128i b);
extern __m128i _mm_adds_epi8(__m128i a, __m128i b);
extern __m128i _mm_adds_epi16(__m128i a, __m128i b);
extern __m128i _mm_adds_epu8(__m128i a, __m128i b);
extern __m128i _mm_adds_epu16(__m128i a, __m128i b);
extern __m128i _mm_avg_epu8(__m128i a, __m128i b);
extern __m128i _mm_avg_epu16(__m128i a, __m128i b);
extern __m128i _mm_madd_epi16(__m128i a, __m128i b);
extern __m128i _mm_max_epi16(__m128i a, __m128i b);
extern __m128i _mm_max_epu8(__m128i a, __m128i b);
extern __m128i _mm_min_epi16(__m128i a, __m128i b);
extern __m128i _mm_min_epu8(__m128i a, __m128i b);
extern __m128i _mm_mulhi_epi16(__m128i a, __m128i b);
extern __m128i _mm_mulhi_epu16(__m128i a, __m128i b);
extern __m128i _mm_mullo_epi16(__m128i a, __m128i b);
extern __m64 _mm_mul_su32(__m64 a, __m64 b);
extern __m128i _mm_mul_epu32(__m128i a, __m128i b);
extern __m128i _mm_sad_epu8(__m128i a, __m128i b);
extern __m128i _mm_sub_epi8(__m128i a, __m128i b);
extern __m128i _mm_sub_epi16(__m128i a, __m128i b);
extern __m128i _mm_sub_epi32(__m128i a, __m128i b);
extern __m64 _mm_sub_si64(__m64 a, __m64 b);
extern __m128i _mm_sub_epi64(__m128i a, __m128i b);
extern __m128i _mm_subs_epi8(__m128i a, __m128i b);
extern __m128i _mm_subs_epi16(__m128i a, __m128i b);
extern __m128i _mm_subs_epu8(__m128i a, __m128i b);
extern __m128i _mm_subs_epu16(__m128i a, __m128i b);
extern __m128i _mm_and_si128(__m128i a, __m128i b);
extern __m128i _mm_andnot_si128(__m128i a, __m128i b);
extern __m128i _mm_or_si128(__m128i a, __m128i b);
extern __m128i _mm_xor_si128(__m128i a, __m128i b);
extern __m128i _mm_slli_si128(__m128i a, int i);
extern __m128i _mm_slli_epi16(__m128i a, int count);
extern __m128i _mm_sll_epi16(__m128i a, __m128i count);
extern __m128i _mm_slli_epi32(__m128i a, int count);
extern __m128i _mm_sll_epi32(__m128i a, __m128i count);
extern __m128i _mm_slli_epi64(__m128i a, int count);
extern __m128i _mm_sll_epi64(__m128i a, __m128i count);
extern __m128i _mm_srai_epi16(__m128i a, int count);
extern __m128i _mm_sra_epi16(__m128i a, __m128i count);
extern __m128i _mm_srai_epi32(__m128i a, int count);
extern __m128i _mm_sra_epi32(__m128i a, __m128i count);
extern __m128i _mm_srli_si128(__m128i a, int imm);
extern __m128i _mm_srli_epi16(__m128i a, int count);
extern __m128i _mm_srl_epi16(__m128i a, __m128i count);
extern __m128i _mm_srli_epi32(__m128i a, int count);
extern __m128i _mm_srl_epi32(__m128i a, __m128i count);
extern __m128i _mm_srli_epi64(__m128i a, int count);
extern __m128i _mm_srl_epi64(__m128i a, __m128i count);
extern __m128i _mm_cmpeq_epi8(__m128i a, __m128i b);
extern __m128i _mm_cmpeq_epi16(__m128i a, __m128i b);
extern __m128i _mm_cmpeq_epi32(__m128i a, __m128i b);
extern __m128i _mm_cmpgt_epi8(__m128i a, __m128i b);
extern __m128i _mm_cmpgt_epi16(__m128i a, __m128i b);
extern __m128i _mm_cmpgt_epi32(__m128i a, __m128i b);
extern __m128i _mm_cmplt_epi8(__m128i a, __m128i b);
extern __m128i _mm_cmplt_epi16(__m128i a, __m128i b);
extern __m128i _mm_cmplt_epi32(__m128i a, __m128i b);
#ifdef _M_AMD64
extern __m128d _mm_cvtsi64_sd(__m128d a, long long b);
extern long long _mm_cvtsd_si64(__m128d a);
extern long long _mm_cvttsd_si64(__m128d a);
#endif
extern __m128 _mm_cvtepi32_ps(__m128i a);
extern __m128i _mm_cvtps_epi32(__m128 a);
extern __m128i _mm_cvttps_epi32(__m128 a);
extern __m128i _mm_cvtsi32_si128(int a);
#ifdef _M_AMD64
extern __m128i _mm_cvtsi64_si128(long long a);
#endif
extern int _mm_cvtsi128_si32(__m128i a);
#ifdef _M_AMD64
extern long long _mm_cvtsi128_si64(__m128i a);
#endif
extern __m128i _mm_load_si128(__m128i const *p);
extern __m128i _mm_loadu_si128(__m128i_u const *p);
extern __m128i _mm_loadl_epi64(__m128i_u const *p);
//extern __m128i _mm_undefined_si128(void);
//extern __m128i _mm_set_epi64x(long long q1, long long q0); // FIXME
extern __m128i _mm_set_epi64(__m64 q1, __m64 q0);
//extern __m128i _mm_set_epi32(int i3, int i1, int i0);
extern __m128i _mm_set_epi32(int i3, int i2, int i1, int i0);
//extern __m128i _mm_set_epi16(short w7, short w2, short w1, short w0);
extern __m128i _mm_set_epi16(short w7, short w6, short w5, short w4, short w3, short w2, short w1, short w0);
//extern __m128i _mm_set_epi8(char b15, char b10, char b4, char b3, char b2, char b1, char b0);
extern __m128i _mm_set_epi8(char b15, char b14, char b13, char b12, char b11, char b10, char b9, char b8, char b7, char b6, char b5, char b4, char b3, char b2, char b1, char b0);
//extern __m128i _mm_set1_epi64x(long long q); // FIXME
extern __m128i _mm_set1_epi64(__m64 q);
extern __m128i _mm_set1_epi32(int i);
extern __m128i _mm_set1_epi16(short w);
extern __m128i _mm_set1_epi8(char b);
extern __m128i _mm_setl_epi64(__m128i q); // FIXME: clang?
extern __m128i _mm_setr_epi64(__m64 q0, __m64 q1);
//extern __m128i _mm_setr_epi32(int i0, int i2, int i3);
extern __m128i _mm_setr_epi32(int i0, int i1, int i2, int i3);
//extern __m128i _mm_setr_epi16(short w0, short w5, short w6, short w7);
extern __m128i _mm_setr_epi16(short w0, short w1, short w2, short w3, short w4, short w5, short w6, short w7);
//extern __m128i _mm_setr_epi8(char b0, char b6, char b11, char b12, char b13, char b14, char b15);
extern __m128i _mm_setr_epi8(char b15, char b14, char b13, char b12, char b11, char b10, char b9, char b8, char b7, char b6, char b5, char b4, char b3, char b2, char b1, char b0);
extern __m128i _mm_setzero_si128(void);
extern void _mm_store_si128(__m128i *p, __m128i b);
extern void _mm_storeu_si128(__m128i_u *p, __m128i b);
//extern void _mm_storeu_si64(void *p, __m128i b);
//extern void _mm_storeu_si32(void *p, __m128i b);
//extern void _mm_storeu_si16(void *p, __m128i b);
extern void _mm_maskmoveu_si128(__m128i d, __m128i n, _Out_writes_bytes_(16) char *p);
extern void _mm_storel_epi64(__m128i_u *p, __m128i a);
extern void _mm_stream_pd(double *p, __m128d a);
extern void _mm_stream_si128(__m128i *p, __m128i a);
extern void _mm_stream_si32(int *p, int a);
extern void _mm_clflush(void const *p);
extern void _mm_lfence(void);
extern void _mm_mfence(void);
extern __m128i _mm_packs_epi16(__m128i a, __m128i b);
extern __m128i _mm_packs_epi32(__m128i a, __m128i b);
extern __m128i _mm_packus_epi16(__m128i a, __m128i b);
extern int _mm_extract_epi16(__m128i a, int imm);
extern __m128i _mm_insert_epi16(__m128i a, int b, int imm);
extern int _mm_movemask_epi8(__m128i a);
extern __m128i _mm_shuffle_epi32(__m128i a, int imm);
extern __m128i _mm_shufflelo_epi16(__m128i a, int imm);
extern __m128i _mm_shufflehi_epi16(__m128i a, int imm);
extern __m128i _mm_unpackhi_epi8(__m128i a, __m128i b);
extern __m128i _mm_unpackhi_epi16(__m128i a, __m128i b);
extern __m128i _mm_unpackhi_epi32(__m128i a, __m128i b);
extern __m128i _mm_unpackhi_epi64(__m128i a, __m128i b);
extern __m128i _mm_unpacklo_epi8(__m128i a, __m128i b);
extern __m128i _mm_unpacklo_epi16(__m128i a, __m128i b);
extern __m128i _mm_unpacklo_epi32(__m128i a, __m128i b);
extern __m128i _mm_unpacklo_epi64(__m128i a, __m128i b);
extern __m64 _mm_movepi64_pi64(__m128i a);
extern __m128i _mm_movpi64_epi64(__m64 a);
extern __m128i _mm_move_epi64(__m128i a);
extern __m128d _mm_unpackhi_pd(__m128d a, __m128d b);
extern __m128d _mm_unpacklo_pd(__m128d a, __m128d b);
extern int _mm_movemask_pd(__m128d a);
extern __m128d _mm_shuffle_pd(__m128d a, __m128d b, int imm);
extern __m128 _mm_castpd_ps(__m128d a);
extern __m128i _mm_castpd_si128(__m128d a);
extern __m128d _mm_castps_pd(__m128 a);
extern __m128i _mm_castps_si128(__m128 a);
extern __m128 _mm_castsi128_ps(__m128i a);
extern __m128d _mm_castsi128_pd(__m128i a);
void _mm_pause(void);

/* Alternate names */
#define _mm_set_pd1(a) _mm_set1_pd(a)
#define _mm_load_pd1(p) _mm_load1_pd(p)
#define _mm_store_pd1(p, a) _mm_store1_pd((p), (a))
#define _mm_bslli_si128 _mm_slli_si128
#define _mm_bsrli_si128 _mm_srli_si128
#define _mm_stream_si64 _mm_stream_si64x

#if defined(_MSC_VER) && !defined(__clang__)

#pragma intrinsic(_mm_add_sd)
#pragma intrinsic(_mm_add_pd)
#pragma intrinsic(_mm_sub_sd)
#pragma intrinsic(_mm_sub_pd)
#pragma intrinsic(_mm_mul_sd)
#pragma intrinsic(_mm_mul_pd)
#pragma intrinsic(_mm_div_sd)
#pragma intrinsic(_mm_div_pd)
#pragma intrinsic(_mm_sqrt_sd)
#pragma intrinsic(_mm_sqrt_pd)
#pragma intrinsic(_mm_min_sd)
#pragma intrinsic(_mm_min_pd)
#pragma intrinsic(_mm_max_sd)
#pragma intrinsic(_mm_max_pd)
#pragma intrinsic(_mm_and_pd)
#pragma intrinsic(_mm_andnot_pd)
#pragma intrinsic(_mm_or_pd)
#pragma intrinsic(_mm_xor_pd)
#pragma intrinsic(_mm_cmpeq_pd)
#pragma intrinsic(_mm_cmplt_pd)
#pragma intrinsic(_mm_cmple_pd)
#pragma intrinsic(_mm_cmpgt_pd)
#pragma intrinsic(_mm_cmpge_pd)
#pragma intrinsic(_mm_cmpord_pd)
#pragma intrinsic(_mm_cmpunord_pd)
#pragma intrinsic(_mm_cmpneq_pd)
#pragma intrinsic(_mm_cmpnlt_pd)
#pragma intrinsic(_mm_cmpnle_pd)
#pragma intrinsic(_mm_cmpngt_pd)
#pragma intrinsic(_mm_cmpnge_pd)
#pragma intrinsic(_mm_cmpeq_sd)
#pragma intrinsic(_mm_cmplt_sd)
#pragma intrinsic(_mm_cmple_sd)
#pragma intrinsic(_mm_cmpgt_sd)
#pragma intrinsic(_mm_cmpge_sd)
#pragma intrinsic(_mm_cmpord_sd)
#pragma intrinsic(_mm_cmpunord_sd)
#pragma intrinsic(_mm_cmpneq_sd)
#pragma intrinsic(_mm_cmpnlt_sd)
#pragma intrinsic(_mm_cmpnle_sd)
#pragma intrinsic(_mm_cmpngt_sd)
#pragma intrinsic(_mm_cmpnge_sd)
#pragma intrinsic(_mm_comieq_sd)
#pragma intrinsic(_mm_comilt_sd)
#pragma intrinsic(_mm_comile_sd)
#pragma intrinsic(_mm_comigt_sd)
#pragma intrinsic(_mm_comige_sd)
#pragma intrinsic(_mm_comineq_sd)
#pragma intrinsic(_mm_ucomieq_sd)
#pragma intrinsic(_mm_ucomilt_sd)
#pragma intrinsic(_mm_ucomile_sd)
#pragma intrinsic(_mm_ucomigt_sd)
#pragma intrinsic(_mm_ucomige_sd)
#pragma intrinsic(_mm_ucomineq_sd)
#pragma intrinsic(_mm_cvtpd_ps)
#pragma intrinsic(_mm_cvtps_pd)
#pragma intrinsic(_mm_cvtepi32_pd)
#pragma intrinsic(_mm_cvtpd_epi32)
#pragma intrinsic(_mm_cvtsd_si32)
#pragma intrinsic(_mm_cvtsd_ss)
#pragma intrinsic(_mm_cvtsi32_sd)
#pragma intrinsic(_mm_cvtss_sd)
#pragma intrinsic(_mm_cvttpd_epi32)
#pragma intrinsic(_mm_cvttsd_si32)
//#pragma intrinsic(_mm_cvtpd_pi32)
//#pragma intrinsic(_mm_cvttpd_pi32)
//#pragma intrinsic(_mm_cvtpi32_pd)
#pragma intrinsic(_mm_cvtsd_f64)
#pragma intrinsic(_mm_load_pd)
#pragma intrinsic(_mm_load1_pd)
#pragma intrinsic(_mm_loadr_pd)
#pragma intrinsic(_mm_loadu_pd)
//#pragma intrinsic(_mm_loadu_si64)
//#pragma intrinsic(_mm_loadu_si32)
//#pragma intrinsic(_mm_loadu_si16)
#pragma intrinsic(_mm_load_sd)
#pragma intrinsic(_mm_loadh_pd)
#pragma intrinsic(_mm_loadl_pd)
//#pragma intrinsic(_mm_undefined_pd)
#pragma intrinsic(_mm_set_sd)
#pragma intrinsic(_mm_set1_pd)
#pragma intrinsic(_mm_set_pd)
#pragma intrinsic(_mm_setr_pd)
#pragma intrinsic(_mm_setzero_pd)
#pragma intrinsic(_mm_move_sd)
#pragma intrinsic(_mm_store_sd)
#pragma intrinsic(_mm_store_pd)
#pragma intrinsic(_mm_store1_pd)
#pragma intrinsic(_mm_storeu_pd)
#pragma intrinsic(_mm_storer_pd)
#pragma intrinsic(_mm_storeh_pd)
#pragma intrinsic(_mm_storel_pd)
#pragma intrinsic(_mm_add_epi8)
#pragma intrinsic(_mm_add_epi16)
#pragma intrinsic(_mm_add_epi32)
//#pragma intrinsic(_mm_add_si64)
#pragma intrinsic(_mm_add_epi64)
#pragma intrinsic(_mm_adds_epi8)
#pragma intrinsic(_mm_adds_epi16)
#pragma intrinsic(_mm_adds_epu8)
#pragma intrinsic(_mm_adds_epu16)
#pragma intrinsic(_mm_avg_epu8)
#pragma intrinsic(_mm_avg_epu16)
#pragma intrinsic(_mm_madd_epi16)
#pragma intrinsic(_mm_max_epi16)
#pragma intrinsic(_mm_max_epu8)
#pragma intrinsic(_mm_min_epi16)
#pragma intrinsic(_mm_min_epu8)
#pragma intrinsic(_mm_mulhi_epi16)
#pragma intrinsic(_mm_mulhi_epu16)
#pragma intrinsic(_mm_mullo_epi16)
//#pragma intrinsic(_mm_mul_su32)
#pragma intrinsic(_mm_mul_epu32)
#pragma intrinsic(_mm_sad_epu8)
#pragma intrinsic(_mm_sub_epi8)
#pragma intrinsic(_mm_sub_epi16)
#pragma intrinsic(_mm_sub_epi32)
//#pragma intrinsic(_mm_sub_si64)
#pragma intrinsic(_mm_sub_epi64)
#pragma intrinsic(_mm_subs_epi8)
#pragma intrinsic(_mm_subs_epi16)
#pragma intrinsic(_mm_subs_epu8)
#pragma intrinsic(_mm_subs_epu16)
#pragma intrinsic(_mm_and_si128)
#pragma intrinsic(_mm_andnot_si128)
#pragma intrinsic(_mm_or_si128)
#pragma intrinsic(_mm_xor_si128)
#pragma intrinsic(_mm_slli_si128)
#pragma intrinsic(_mm_slli_epi16)
#pragma intrinsic(_mm_sll_epi16)
#pragma intrinsic(_mm_slli_epi32)
#pragma intrinsic(_mm_sll_epi32)
#pragma intrinsic(_mm_slli_epi64)
#pragma intrinsic(_mm_sll_epi64)
#pragma intrinsic(_mm_srai_epi16)
#pragma intrinsic(_mm_sra_epi16)
#pragma intrinsic(_mm_srai_epi32)
#pragma intrinsic(_mm_sra_epi32)
#pragma intrinsic(_mm_srli_si128)
#pragma intrinsic(_mm_srli_epi16)
#pragma intrinsic(_mm_srl_epi16)
#pragma intrinsic(_mm_srli_epi32)
#pragma intrinsic(_mm_srl_epi32)
#pragma intrinsic(_mm_srli_epi64)
#pragma intrinsic(_mm_srl_epi64)
#pragma intrinsic(_mm_cmpeq_epi8)
#pragma intrinsic(_mm_cmpeq_epi16)
#pragma intrinsic(_mm_cmpeq_epi32)
#pragma intrinsic(_mm_cmpgt_epi8)
#pragma intrinsic(_mm_cmpgt_epi16)
#pragma intrinsic(_mm_cmpgt_epi32)
#pragma intrinsic(_mm_cmplt_epi8)
#pragma intrinsic(_mm_cmplt_epi16)
#pragma intrinsic(_mm_cmplt_epi32)
#ifdef _M_AMD64
#pragma intrinsic(_mm_cvtsi64_sd)
#pragma intrinsic(_mm_cvtsd_si64)
#pragma intrinsic(_mm_cvttsd_si64)
#endif
#pragma intrinsic(_mm_cvtepi32_ps)
#pragma intrinsic(_mm_cvtps_epi32)
#pragma intrinsic(_mm_cvttps_epi32)
#pragma intrinsic(_mm_cvtsi32_si128)
#ifdef _M_AMD64
#pragma intrinsic(_mm_cvtsi64_si128)
#endif
#pragma intrinsic(_mm_cvtsi128_si32)
#ifdef _M_AMD64
#pragma intrinsic(_mm_cvtsi128_si64)
#endif
#pragma intrinsic(_mm_load_si128)
#pragma intrinsic(_mm_loadu_si128)
#pragma intrinsic(_mm_loadl_epi64)
//#pragma intrinsic(_mm_undefined_si128)
//#pragma intrinsic(_mm_set_epi64x)
//#pragma intrinsic(_mm_set_epi64)
#pragma intrinsic(_mm_set_epi32)
#pragma intrinsic(_mm_set_epi16)
#pragma intrinsic(_mm_set_epi8)
//#pragma intrinsic(_mm_set1_epi64x)
//#pragma intrinsic(_mm_set1_epi64)
#pragma intrinsic(_mm_set1_epi32)
#pragma intrinsic(_mm_set1_epi16)
#pragma intrinsic(_mm_set1_epi8)
#pragma intrinsic(_mm_setl_epi64)
//#pragma intrinsic(_mm_setr_epi64)
#pragma intrinsic(_mm_setr_epi32)
#pragma intrinsic(_mm_setr_epi16)
#pragma intrinsic(_mm_setr_epi8)
#pragma intrinsic(_mm_setzero_si128)
#pragma intrinsic(_mm_store_si128)
#pragma intrinsic(_mm_storeu_si128)
//#pragma intrinsic(_mm_storeu_si64)
//#pragma intrinsic(_mm_storeu_si32)
//#pragma intrinsic(_mm_storeu_si16)
#pragma intrinsic(_mm_maskmoveu_si128)
#pragma intrinsic(_mm_storel_epi64)
#pragma intrinsic(_mm_stream_pd)
#pragma intrinsic(_mm_stream_si128)
#pragma intrinsic(_mm_stream_si32)
#pragma intrinsic(_mm_clflush)
#pragma intrinsic(_mm_lfence)
#pragma intrinsic(_mm_mfence)
#pragma intrinsic(_mm_packs_epi16)
#pragma intrinsic(_mm_packs_epi32)
#pragma intrinsic(_mm_packus_epi16)
#pragma intrinsic(_mm_extract_epi16)
#pragma intrinsic(_mm_insert_epi16)
#pragma intrinsic(_mm_movemask_epi8)
#pragma intrinsic(_mm_shuffle_epi32)
#pragma intrinsic(_mm_shufflelo_epi16)
#pragma intrinsic(_mm_shufflehi_epi16)
#pragma intrinsic(_mm_unpackhi_epi8)
#pragma intrinsic(_mm_unpackhi_epi16)
#pragma intrinsic(_mm_unpackhi_epi32)
#pragma intrinsic(_mm_unpackhi_epi64)
#pragma intrinsic(_mm_unpacklo_epi8)
#pragma intrinsic(_mm_unpacklo_epi16)
#pragma intrinsic(_mm_unpacklo_epi32)
#pragma intrinsic(_mm_unpacklo_epi64)
//#pragma intrinsic(_mm_movepi64_pi64)
//#pragma intrinsic(_mm_movpi64_epi64)
#pragma intrinsic(_mm_move_epi64)
#pragma intrinsic(_mm_unpackhi_pd)
#pragma intrinsic(_mm_unpacklo_pd)
#pragma intrinsic(_mm_movemask_pd)
#pragma intrinsic(_mm_shuffle_pd)
#pragma intrinsic(_mm_castpd_ps)
#pragma intrinsic(_mm_castpd_si128)
#pragma intrinsic(_mm_castps_pd)
#pragma intrinsic(_mm_castps_si128)
#pragma intrinsic(_mm_castsi128_ps)
#pragma intrinsic(_mm_castsi128_pd)
#pragma intrinsic(_mm_pause)

#else /* _MSC_VER */

/*
  Clang: https://github.com/llvm/llvm-project/blob/main/clang/lib/Headers/emmintrin.h
  Clang older version: https://github.com/llvm/llvm-project/blob/3ef88b31843e040c95f23ff2c3c206f1fa399c05/clang/lib/Headers/emmintrin.h
  unikraft: https://github.com/unikraft/lib-intel-intrinsics/blob/staging/include/emmintrin.h
*/

__INTRIN_INLINE_SSE2 __m128d _mm_add_sd(__m128d a, __m128d b)
{
    a[0] += b[0];
    return a;
}

__INTRIN_INLINE_SSE2 __m128d _mm_add_pd(__m128d a, __m128d b)
{
    return (__m128d)((__v2df)a + (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_sub_sd(__m128d a, __m128d b)
{
    a[0] -= b[0];
    return a;
}

__INTRIN_INLINE_SSE2 __m128d _mm_sub_pd(__m128d a, __m128d b)
{
    return (__m128d)((__v2df)a - (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_mul_sd(__m128d a, __m128d b)
{
    a[0] *= b[0];
    return a;
}

__INTRIN_INLINE_SSE2 __m128d _mm_mul_pd(__m128d a, __m128d b)
{
    return (__m128d)((__v2df)a * (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_div_sd(__m128d a, __m128d b)
{
    a[0] /= b[0];
    return a;
}

__INTRIN_INLINE_SSE2 __m128d _mm_div_pd(__m128d a, __m128d b)
{
    return (__m128d)((__v2df)a / (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_sqrt_sd(__m128d a, __m128d b)
{
    __m128d __c = __builtin_ia32_sqrtsd((__v2df)b);
    return __extension__(__m128d){__c[0], a[1]};
}

__INTRIN_INLINE_SSE2 __m128d _mm_sqrt_pd(__m128d a)
{
    return __builtin_ia32_sqrtpd((__v2df)a);
}

__INTRIN_INLINE_SSE2 __m128d _mm_min_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_minsd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_min_pd(__m128d a, __m128d b)
{
    return __builtin_ia32_minpd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_max_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_maxsd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_max_pd(__m128d a, __m128d b)
{
    return __builtin_ia32_maxpd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_and_pd(__m128d a, __m128d b)
{
    return (__m128d)((__v2du)a & (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_andnot_pd(__m128d a, __m128d b)
{
    return (__m128d)(~(__v2du)a & (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_or_pd(__m128d a, __m128d b)
{
    return (__m128d)((__v2du)a | (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_xor_pd(__m128d a, __m128d b)
{
    return (__m128d)((__v2du)a ^ (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpeq_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpeqpd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmplt_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpltpd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmple_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmplepd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpgt_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpltpd((__v2df)b, (__v2df)a);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpge_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmplepd((__v2df)b, (__v2df)a);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpord_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpordpd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpunord_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpunordpd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpneq_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpneqpd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpnlt_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpnltpd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpnle_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpnlepd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpngt_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpnltpd((__v2df)b, (__v2df)a);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpnge_pd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpnlepd((__v2df)b, (__v2df)a);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpeq_sd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpeqsd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmplt_sd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpltsd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmple_sd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmplesd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpgt_sd(__m128d a, __m128d b)
{
    __m128d __c = __builtin_ia32_cmpltsd((__v2df)b, (__v2df)a);
    return __extension__(__m128d){__c[0], a[1]};
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpge_sd(__m128d a, __m128d b)
{
    __m128d __c = __builtin_ia32_cmplesd((__v2df)b, (__v2df)a);
    return __extension__(__m128d){__c[0], a[1]};
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpord_sd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpordsd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpunord_sd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpunordsd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpneq_sd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpneqsd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpnlt_sd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpnltsd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpnle_sd(__m128d a, __m128d b)
{
    return (__m128d)__builtin_ia32_cmpnlesd((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpngt_sd(__m128d a, __m128d b)
{
    __m128d __c = __builtin_ia32_cmpnltsd((__v2df)b, (__v2df)a);
    return __extension__(__m128d){__c[0], a[1]};
}

__INTRIN_INLINE_SSE2 __m128d _mm_cmpnge_sd(__m128d a, __m128d b)
{
    __m128d __c = __builtin_ia32_cmpnlesd((__v2df)b, (__v2df)a);
    return __extension__(__m128d){__c[0], a[1]};
}

__INTRIN_INLINE_SSE2 int _mm_comieq_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_comisdeq((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_comilt_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_comisdlt((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_comile_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_comisdle((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_comigt_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_comisdgt((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_comige_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_comisdge((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_comineq_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_comisdneq((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_ucomieq_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_ucomisdeq((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_ucomilt_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_ucomisdlt((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_ucomile_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_ucomisdle((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_ucomigt_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_ucomisdgt((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_ucomige_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_ucomisdge((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 int _mm_ucomineq_sd(__m128d a, __m128d b)
{
    return __builtin_ia32_ucomisdneq((__v2df)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128 _mm_cvtpd_ps(__m128d a)
{
    return __builtin_ia32_cvtpd2ps((__v2df)a);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cvtps_pd(__m128 a)
{
#if HAS_BUILTIN(__builtin_convertvector)
    return (__m128d)__builtin_convertvector(__builtin_shufflevector((__v4sf)a, (__v4sf)a, 0, 1), __v2df);
#else
    return __builtin_ia32_cvtps2pd(a);
#endif
}

__INTRIN_INLINE_SSE2 __m128d _mm_cvtepi32_pd(__m128i a)
{
#if HAS_BUILTIN(__builtin_convertvector)
    return (__m128d)__builtin_convertvector(__builtin_shufflevector((__v4si)a, (__v4si)a, 0, 1), __v2df);
#else
    return __builtin_ia32_cvtdq2pd((__v4si)a);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_cvtpd_epi32(__m128d a)
{
    return (__m128i)__builtin_ia32_cvtpd2dq((__v2df)a);
}

__INTRIN_INLINE_SSE2 int _mm_cvtsd_si32(__m128d a)
{
    return __builtin_ia32_cvtsd2si((__v2df)a);
}

__INTRIN_INLINE_SSE2 __m128 _mm_cvtsd_ss(__m128 a, __m128d b)
{
    return (__m128)__builtin_ia32_cvtsd2ss((__v4sf)a, (__v2df)b);
}

__INTRIN_INLINE_SSE2 __m128d _mm_cvtsi32_sd(__m128d a,
                                                              int b)
{
    a[0] = b;
    return a;
}

__INTRIN_INLINE_SSE2 __m128d _mm_cvtss_sd(__m128d a, __m128 b)
{
    a[0] = b[0];
    return a;
}

__INTRIN_INLINE_SSE2 __m128i _mm_cvttpd_epi32(__m128d a)
{
    return (__m128i)__builtin_ia32_cvttpd2dq((__v2df)a);
}

__INTRIN_INLINE_SSE2 int _mm_cvttsd_si32(__m128d a)
{
    return __builtin_ia32_cvttsd2si((__v2df)a);
}

__INTRIN_INLINE_MMXSSE2 __m64 _mm_cvtpd_pi32(__m128d a)
{
    return (__m64)__builtin_ia32_cvtpd2pi((__v2df)a);
}

__INTRIN_INLINE_MMXSSE2 __m64 _mm_cvttpd_pi32(__m128d a)
{
    return (__m64)__builtin_ia32_cvttpd2pi((__v2df)a);
}

__INTRIN_INLINE_MMXSSE2 __m128d _mm_cvtpi32_pd(__m64 a)
{
    return __builtin_ia32_cvtpi2pd((__v2si)a);
}

__INTRIN_INLINE_SSE2 double _mm_cvtsd_f64(__m128d a)
{
    return a[0];
}

__INTRIN_INLINE_SSE2 __m128d _mm_load_pd(double const *dp)
{
    return *(const __m128d *)dp;
}

__INTRIN_INLINE_SSE2 __m128d _mm_load1_pd(double const *dp)
{
    struct __mm_load1_pd_struct {
      double __u;
    } __attribute__((__packed__, __may_alias__));
    double __u = ((const struct __mm_load1_pd_struct *)dp)->__u;
    return __extension__(__m128d){__u, __u};
}

// GCC:
/* Create a selector for use with the SHUFPD instruction.  */
#define _MM_SHUFFLE2(fp1,fp0) \
 (((fp1) << 1) | (fp0))

__INTRIN_INLINE_SSE2 __m128d _mm_loadr_pd(double const *dp)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    __m128d u = *(const __m128d *)dp;
    return __builtin_shufflevector((__v2df)u, (__v2df)u, 1, 0);
#else
    return (__m128d){ dp[1], dp[0] };
#endif
}

__INTRIN_INLINE_SSE2 __m128d _mm_loadu_pd(double const *dp)
{
    struct __loadu_pd {
      __m128d_u __v;
    } __attribute__((__packed__, __may_alias__));
    return ((const struct __loadu_pd *)dp)->__v;
}

__INTRIN_INLINE_SSE2 __m128i _mm_loadu_si64(void const *a)
{
    struct __loadu_si64 {
      long long __v;
    } __attribute__((__packed__, __may_alias__));
    long long __u = ((const struct __loadu_si64 *)a)->__v;
    return __extension__(__m128i)(__v2di){__u, 0LL};
}

__INTRIN_INLINE_SSE2 __m128i _mm_loadu_si32(void const *a)
{
    struct __loadu_si32 {
      int __v;
    } __attribute__((__packed__, __may_alias__));
    int __u = ((const struct __loadu_si32 *)a)->__v;
    return __extension__(__m128i)(__v4si){__u, 0, 0, 0};
}

__INTRIN_INLINE_SSE2 __m128i _mm_loadu_si16(void const *a)
{
    struct __loadu_si16 {
      short __v;
    } __attribute__((__packed__, __may_alias__));
    short __u = ((const struct __loadu_si16 *)a)->__v;
    return __extension__(__m128i)(__v8hi){__u, 0, 0, 0, 0, 0, 0, 0};
}

__INTRIN_INLINE_SSE2 __m128d _mm_load_sd(double const *dp)
{
    struct __mm_load_sd_struct {
      double __u;
    } __attribute__((__packed__, __may_alias__));
    double __u = ((const struct __mm_load_sd_struct *)dp)->__u;
    return __extension__(__m128d){__u, 0};
}

__INTRIN_INLINE_SSE2 __m128d _mm_loadh_pd(__m128d a, double const *dp)
{
    struct __mm_loadh_pd_struct {
      double __u;
    } __attribute__((__packed__, __may_alias__));
    double __u = ((const struct __mm_loadh_pd_struct *)dp)->__u;
    return __extension__(__m128d){a[0], __u};
}

__INTRIN_INLINE_SSE2 __m128d _mm_loadl_pd(__m128d a, double const *dp)
{
    struct __mm_loadl_pd_struct {
      double __u;
    } __attribute__((__packed__, __may_alias__));
    double __u = ((const struct __mm_loadl_pd_struct *)dp)->__u;
    return __extension__(__m128d){__u, a[1]};
}

__INTRIN_INLINE_SSE2 __m128d _mm_undefined_pd(void)
{
#if HAS_BUILTIN(__builtin_ia32_undef128)
    return (__m128d)__builtin_ia32_undef128();
#else
    __m128d undef = undef;
    return undef;
#endif
}

__INTRIN_INLINE_SSE2 __m128d _mm_set_sd(double w)
{
    return __extension__(__m128d){w, 0};
}

__INTRIN_INLINE_SSE2 __m128d _mm_set1_pd(double w)
{
    return __extension__(__m128d){w, w};
}

__INTRIN_INLINE_SSE2 __m128d _mm_set_pd(double w, double x)
{
    return __extension__(__m128d){x, w};
}

__INTRIN_INLINE_SSE2 __m128d _mm_setr_pd(double w, double x)
{
    return __extension__(__m128d){w, x};
}

__INTRIN_INLINE_SSE2 __m128d _mm_setzero_pd(void)
{
    return __extension__(__m128d){0, 0};
}

__INTRIN_INLINE_SSE2 __m128d _mm_move_sd(__m128d a, __m128d b)
{
    a[0] = b[0];
    return a;
}

__INTRIN_INLINE_SSE2 void _mm_store_sd(double *dp, __m128d a)
{
    struct __mm_store_sd_struct {
      double __u;
    } __attribute__((__packed__, __may_alias__));
    ((struct __mm_store_sd_struct *)dp)->__u = a[0];
}

__INTRIN_INLINE_SSE2 void _mm_store_pd(double *dp, __m128d a)
{
    *(__m128d *)dp = a;
}

__INTRIN_INLINE_SSE2 void _mm_store1_pd(double *dp, __m128d a)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    a = __builtin_shufflevector((__v2df)a, (__v2df)a, 0, 0);
    _mm_store_pd(dp, a);
#else
    dp[0] = a[0];
    dp[1] = a[0];
#endif
}

__INTRIN_INLINE_SSE2 void _mm_storeu_pd(double *dp, __m128d a)
{
    struct __storeu_pd {
      __m128d_u __v;
    } __attribute__((__packed__, __may_alias__));
    ((struct __storeu_pd *)dp)->__v = a;
}

__INTRIN_INLINE_SSE2 void _mm_storer_pd(double *dp, __m128d a)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    a = __builtin_shufflevector((__v2df)a, (__v2df)a, 1, 0);
    *(__m128d *)dp = a;
#else
    dp[0] = a[1];
    dp[1] = a[0];
#endif
}

__INTRIN_INLINE_SSE2 void _mm_storeh_pd(double *dp, __m128d a)
{
    struct __mm_storeh_pd_struct {
      double __u;
    } __attribute__((__packed__, __may_alias__));
    ((struct __mm_storeh_pd_struct *)dp)->__u = a[1];
}

__INTRIN_INLINE_SSE2 void _mm_storel_pd(double *dp, __m128d a)
{
    struct __mm_storeh_pd_struct {
      double __u;
    } __attribute__((__packed__, __may_alias__));
    ((struct __mm_storeh_pd_struct *)dp)->__u = a[0];
}

__INTRIN_INLINE_SSE2 __m128i _mm_add_epi8(__m128i a, __m128i b)
{
    return (__m128i)((__v16qu)a + (__v16qu)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_add_epi16(__m128i a, __m128i b)
{
    return (__m128i)((__v8hu)a + (__v8hu)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_add_epi32(__m128i a, __m128i b)
{
    return (__m128i)((__v4su)a + (__v4su)b);
}

__INTRIN_INLINE_MMXSSE2 __m64 _mm_add_si64(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_paddq((__v1di)a, (__v1di)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_add_epi64(__m128i a, __m128i b)
{
    return (__m128i)((__v2du)a + (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_adds_epi8(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_add_sat)
    return (__m128i)__builtin_elementwise_add_sat((__v16qs)a, (__v16qs)b);
#else
    return (__m128i)__builtin_ia32_paddsb128((__v16qi)a, (__v16qi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_adds_epi16(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_add_sat)
    return (__m128i)__builtin_elementwise_add_sat((__v8hi)a, (__v8hi)b);
#else
    return (__m128i)__builtin_ia32_paddsw128((__v8hi)a, (__v8hi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_adds_epu8(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_add_sat)
    return (__m128i)__builtin_elementwise_add_sat((__v16qu)a, (__v16qu)b);
#else
    return (__m128i)__builtin_ia32_paddusb128((__v16qi)a, (__v16qi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_adds_epu16(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_add_sat)
    return (__m128i)__builtin_elementwise_add_sat((__v8hu)a, (__v8hu)b);
#else
    return (__m128i)__builtin_ia32_paddusw128((__v8hi)a, (__v8hi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_avg_epu8(__m128i a, __m128i b)
{
    return (__m128i)__builtin_ia32_pavgb128((__v16qi)a, (__v16qi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_avg_epu16(__m128i a, __m128i b)
{
    return (__m128i)__builtin_ia32_pavgw128((__v8hi)a, (__v8hi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_madd_epi16(__m128i a, __m128i b)
{
    return (__m128i)__builtin_ia32_pmaddwd128((__v8hi)a, (__v8hi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_max_epi16(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_max)
    return (__m128i)__builtin_elementwise_max((__v8hi)a, (__v8hi)b);
#else
    return (__m128i)__builtin_ia32_pmaxsw128((__v8hi)a, (__v8hi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_max_epu8(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_max)
    return (__m128i)__builtin_elementwise_max((__v16qu)a, (__v16qu)b);
#else
    return (__m128i)__builtin_ia32_pmaxub128((__v16qi)a, (__v16qi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_min_epi16(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_min)
    return (__m128i)__builtin_elementwise_min((__v8hi)a, (__v8hi)b);
#else
    return (__m128i)__builtin_ia32_pminsw128((__v8hi)a, (__v8hi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_min_epu8(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_min)
    return (__m128i)__builtin_elementwise_min((__v16qu)a, (__v16qu)b);
#else
    return (__m128i)__builtin_ia32_pminub128((__v16qi)a, (__v16qi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_mulhi_epi16(__m128i a, __m128i b)
{
    return (__m128i)__builtin_ia32_pmulhw128((__v8hi)a, (__v8hi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_mulhi_epu16(__m128i a, __m128i b)
{
    return (__m128i)__builtin_ia32_pmulhuw128((__v8hi)a, (__v8hi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_mullo_epi16(__m128i a, __m128i b)
{
    return (__m128i)((__v8hu)a * (__v8hu)b);
}

__INTRIN_INLINE_MMXSSE2 __m64 _mm_mul_su32(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_pmuludq((__v2si)a, (__v2si)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_mul_epu32(__m128i a, __m128i b)
{
    return __builtin_ia32_pmuludq128((__v4si)a, (__v4si)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sad_epu8(__m128i a, __m128i b)
{
    return __builtin_ia32_psadbw128((__v16qi)a, (__v16qi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sub_epi8(__m128i a, __m128i b)
{
    return (__m128i)((__v16qu)a - (__v16qu)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sub_epi16(__m128i a, __m128i b)
{
    return (__m128i)((__v8hu)a - (__v8hu)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sub_epi32(__m128i a, __m128i b)
{
    return (__m128i)((__v4su)a - (__v4su)b);
}

__INTRIN_INLINE_MMXSSE2 __m64 _mm_sub_si64(__m64 a, __m64 b)
{
    return (__m64)__builtin_ia32_psubq((__v1di)a, (__v1di)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sub_epi64(__m128i a, __m128i b)
{
    return (__m128i)((__v2du)a - (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_subs_epi8(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_sub_sat)
    return (__m128i)__builtin_elementwise_sub_sat((__v16qs)a, (__v16qs)b);
#else
    return (__m128i)__builtin_ia32_psubsb128((__v16qi)a, (__v16qi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_subs_epi16(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_sub_sat)
    return (__m128i)__builtin_elementwise_sub_sat((__v8hi)a, (__v8hi)b);
#else
    return (__m128i)__builtin_ia32_psubsw128((__v8hi)a, (__v8hi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_subs_epu8(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_sub_sat)
    return (__m128i)__builtin_elementwise_sub_sat((__v16qu)a, (__v16qu)b);
#else
    return (__m128i)__builtin_ia32_psubusb128((__v16qi)a, (__v16qi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_subs_epu16(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_elementwise_sub_sat)
    return (__m128i)__builtin_elementwise_sub_sat((__v8hu)a, (__v8hu)b);
#else
    return (__m128i)__builtin_ia32_psubusw128((__v8hi)a, (__v8hi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_and_si128(__m128i a, __m128i b)
{
    return (__m128i)((__v2du)a & (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_andnot_si128(__m128i a, __m128i b)
{
    return (__m128i)(~(__v2du)a & (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_or_si128(__m128i a, __m128i b)
{
    return (__m128i)((__v2du)a | (__v2du)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_xor_si128(__m128i a, __m128i b)
{
    return (__m128i)((__v2du)a ^ (__v2du)b);
}

#define _mm_slli_si128(a, imm) \
    ((__m128i)__builtin_ia32_pslldqi128_byteshift((__v2di)(__m128i)(a), (int)(imm)))

__INTRIN_INLINE_SSE2 __m128i _mm_slli_epi16(__m128i a, int count)
{
    return (__m128i)__builtin_ia32_psllwi128((__v8hi)a, count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sll_epi16(__m128i a, __m128i count)
{
    return (__m128i)__builtin_ia32_psllw128((__v8hi)a, (__v8hi)count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_slli_epi32(__m128i a, int count)
{
    return (__m128i)__builtin_ia32_pslldi128((__v4si)a, count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sll_epi32(__m128i a, __m128i count)
{
    return (__m128i)__builtin_ia32_pslld128((__v4si)a, (__v4si)count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_slli_epi64(__m128i a, int count)
{
    return __builtin_ia32_psllqi128((__v2di)a, count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sll_epi64(__m128i a, __m128i count)
{
    return __builtin_ia32_psllq128((__v2di)a, (__v2di)count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_srai_epi16(__m128i a, int count)
{
    return (__m128i)__builtin_ia32_psrawi128((__v8hi)a, count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sra_epi16(__m128i a, __m128i count)
{
    return (__m128i)__builtin_ia32_psraw128((__v8hi)a, (__v8hi)count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_srai_epi32(__m128i a, int count)
{
    return (__m128i)__builtin_ia32_psradi128((__v4si)a, count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_sra_epi32(__m128i a, __m128i count)
{
    return (__m128i)__builtin_ia32_psrad128((__v4si)a, (__v4si)count);
}

#define _mm_srli_si128(a, imm) \
    ((__m128i)__builtin_ia32_psrldqi128_byteshift((__v2di)(__m128i)(a), (int)(imm)))

__INTRIN_INLINE_SSE2 __m128i _mm_srli_epi16(__m128i a, int count)
{
    return (__m128i)__builtin_ia32_psrlwi128((__v8hi)a, count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_srl_epi16(__m128i a, __m128i count)
{
    return (__m128i)__builtin_ia32_psrlw128((__v8hi)a, (__v8hi)count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_srli_epi32(__m128i a, int count)
{
    return (__m128i)__builtin_ia32_psrldi128((__v4si)a, count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_srl_epi32(__m128i a, __m128i count)
{
    return (__m128i)__builtin_ia32_psrld128((__v4si)a, (__v4si)count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_srli_epi64(__m128i a, int count)
{
    return __builtin_ia32_psrlqi128((__v2di)a, count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_srl_epi64(__m128i a, __m128i count)
{
    return __builtin_ia32_psrlq128((__v2di)a, (__v2di)count);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmpeq_epi8(__m128i a, __m128i b)
{
    return (__m128i)((__v16qi)a == (__v16qi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmpeq_epi16(__m128i a, __m128i b)
{
    return (__m128i)((__v8hi)a == (__v8hi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmpeq_epi32(__m128i a, __m128i b)
{
    return (__m128i)((__v4si)a == (__v4si)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmpgt_epi8(__m128i a, __m128i b)
{
    /* This function always performs a signed comparison, but __v16qi is a char
       which may be signed or unsigned, so use __v16qs. */
    return (__m128i)((__v16qs)a > (__v16qs)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmpgt_epi16(__m128i a, __m128i b)
{
    return (__m128i)((__v8hi)a > (__v8hi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmpgt_epi32(__m128i a, __m128i b)
{
    return (__m128i)((__v4si)a > (__v4si)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmplt_epi8(__m128i a, __m128i b)
{
    return _mm_cmpgt_epi8(b, a);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmplt_epi16(__m128i a, __m128i b)
{
    return _mm_cmpgt_epi16(b, a);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cmplt_epi32(__m128i a, __m128i b)
{
    return _mm_cmpgt_epi32(b, a);
}

#ifdef _M_AMD64

__INTRIN_INLINE_SSE2 __m128d _mm_cvtsi64_sd(__m128d a, long long b)
{
    a[0] = b;
    return a;
}

__INTRIN_INLINE_SSE2 long long _mm_cvtsd_si64(__m128d a)
{
    return __builtin_ia32_cvtsd2si64((__v2df)a);
}

__INTRIN_INLINE_SSE2 long long _mm_cvttsd_si64(__m128d a)
{
    return __builtin_ia32_cvttsd2si64((__v2df)a);
}
#endif

__INTRIN_INLINE_SSE2 __m128 _mm_cvtepi32_ps(__m128i a)
{
#if HAS_BUILTIN(__builtin_convertvector)
    return (__m128)__builtin_convertvector((__v4si)a, __v4sf);
#else
    return __builtin_ia32_cvtdq2ps((__v4si)a);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_cvtps_epi32(__m128 a)
{
    return (__m128i)__builtin_ia32_cvtps2dq((__v4sf)a);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cvttps_epi32(__m128 a)
{
    return (__m128i)__builtin_ia32_cvttps2dq((__v4sf)a);
}

__INTRIN_INLINE_SSE2 __m128i _mm_cvtsi32_si128(int a)
{
    return __extension__(__m128i)(__v4si){a, 0, 0, 0};
}

__INTRIN_INLINE_SSE2 __m128i _mm_cvtsi64_si128(long long a)
{
    return __extension__(__m128i)(__v2di){a, 0};
}

__INTRIN_INLINE_SSE2 int _mm_cvtsi128_si32(__m128i a)
{
    __v4si b = (__v4si)a;
    return b[0];
}

__INTRIN_INLINE_SSE2 long long _mm_cvtsi128_si64(__m128i a)
{
    return a[0];
}

__INTRIN_INLINE_SSE2 __m128i _mm_load_si128(__m128i const *p)
{
    return *p;
}

__INTRIN_INLINE_SSE2 __m128i _mm_loadu_si128(__m128i_u const *p)
{
    struct __loadu_si128 {
      __m128i_u __v;
    } __attribute__((__packed__, __may_alias__));
    return ((const struct __loadu_si128 *)p)->__v;
}

__INTRIN_INLINE_SSE2 __m128i _mm_loadl_epi64(__m128i_u const *p)
{
    struct __mm_loadl_epi64_struct {
      long long __u;
    } __attribute__((__packed__, __may_alias__));
    return __extension__(__m128i){
        ((const struct __mm_loadl_epi64_struct *)p)->__u, 0};
}

__INTRIN_INLINE_SSE2 __m128i _mm_undefined_si128(void)
{
#if HAS_BUILTIN(__builtin_ia32_undef128)
    return (__m128i)__builtin_ia32_undef128();
#else
    __m128i undef = undef;
    return undef;
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_set_epi64x(long long q1, long long q0)
{
    return __extension__(__m128i)(__v2di){q0, q1};
}

__INTRIN_INLINE_SSE2 __m128i _mm_set_epi64(__m64 q1, __m64 q0)
{
    return _mm_set_epi64x((long long)q1, (long long)q0);
}

__INTRIN_INLINE_SSE2 __m128i _mm_set_epi32(int i3, int i2, int i1, int i0)
{
    return __extension__(__m128i)(__v4si){i0, i1, i2, i3};
}

__INTRIN_INLINE_SSE2 __m128i _mm_set_epi16(
    short w7, short w6, short w5, short w4,
    short w3, short w2, short w1, short w0)
{
    return __extension__(__m128i)(__v8hi){w0, w1, w2, w3, w4, w5, w6, w7};
}

__INTRIN_INLINE_SSE2 __m128i _mm_set_epi8(
    char b15, char b14, char b13, char b12,
    char b11, char b10, char b9, char b8,
    char b7, char b6, char b5, char b4,
    char b3, char b2, char b1, char b0)
{
    return __extension__(__m128i)(__v16qi){
        b0, b1, b2,  b3,  b4,  b5,  b6,  b7,
        b8, b9, b10, b11, b12, b13, b14, b15};
}

__INTRIN_INLINE_SSE2 __m128i _mm_set1_epi64x(long long q)
{
    return _mm_set_epi64x(q, q);
}

__INTRIN_INLINE_SSE2 __m128i _mm_set1_epi64(__m64 q)
{
    return _mm_set_epi64(q, q);
}

__INTRIN_INLINE_SSE2 __m128i _mm_set1_epi32(int i)
{
    return _mm_set_epi32(i, i, i, i);
}

__INTRIN_INLINE_SSE2 __m128i _mm_set1_epi16(short w)
{
    return _mm_set_epi16(w, w, w, w, w, w, w, w);
}

__INTRIN_INLINE_SSE2 __m128i _mm_set1_epi8(char b)
{
    return _mm_set_epi8(b, b, b, b, b, b, b, b, b, b, b,
                        b, b, b, b, b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_setr_epi64(__m64 q0, __m64 q1)
{
    return _mm_set_epi64(q1, q0);
}

__INTRIN_INLINE_SSE2 __m128i _mm_setr_epi32(int i0, int i1, int i2, int i3)
{
    return _mm_set_epi32(i3, i2, i1, i0);
}

__INTRIN_INLINE_SSE2 __m128i _mm_setr_epi16(
    short w0, short w1, short w2, short w3,
    short w4, short w5, short w6, short w7)
{
    return _mm_set_epi16(w7, w6, w5, w4, w3, w2, w1, w0);
}

__INTRIN_INLINE_SSE2 __m128i _mm_setr_epi8(
    char b0, char b1, char b2, char b3,
    char b4, char b5, char b6, char b7,
    char b8, char b9, char b10,  char b11,
    char b12, char b13, char b14, char b15)
{
    return _mm_set_epi8(b15, b14, b13, b12, b11, b10, b9, b8,
                        b7, b6, b5, b4, b3, b2, b1, b0);
}

__INTRIN_INLINE_SSE2 __m128i _mm_setzero_si128(void)
{
    return __extension__(__m128i)(__v2di){0LL, 0LL};
}

__INTRIN_INLINE_SSE2 void _mm_store_si128(__m128i *p, __m128i b)
{
    *p = b;
}

__INTRIN_INLINE_SSE2 void _mm_storeu_si128(__m128i_u *p, __m128i b)
{
    struct __storeu_si128 {
      __m128i_u __v;
    } __attribute__((__packed__, __may_alias__));
    ((struct __storeu_si128 *)p)->__v = b;
}

__INTRIN_INLINE_SSE2 void _mm_storeu_si64(void *p, __m128i b)
{
    struct __storeu_si64 {
      long long __v;
    } __attribute__((__packed__, __may_alias__));
    ((struct __storeu_si64 *)p)->__v = ((__v2di)b)[0];
}

__INTRIN_INLINE_SSE2 void _mm_storeu_si32(void *p, __m128i b)
{
    struct __storeu_si32 {
      int __v;
    } __attribute__((__packed__, __may_alias__));
    ((struct __storeu_si32 *)p)->__v = ((__v4si)b)[0];
}

__INTRIN_INLINE_SSE2 void _mm_storeu_si16(void *p, __m128i b)
{
    struct __storeu_si16 {
      short __v;
    } __attribute__((__packed__, __may_alias__));
    ((struct __storeu_si16 *)p)->__v = ((__v8hi)b)[0];
}

__INTRIN_INLINE_SSE2 void _mm_maskmoveu_si128(__m128i d, __m128i n, char *p)
{
    __builtin_ia32_maskmovdqu((__v16qi)d, (__v16qi)n, p);
}

__INTRIN_INLINE_SSE2 void _mm_storel_epi64(__m128i_u *p, __m128i a)
{
    struct __mm_storel_epi64_struct {
      long long __u;
    } __attribute__((__packed__, __may_alias__));
    ((struct __mm_storel_epi64_struct *)p)->__u = a[0];
}

__INTRIN_INLINE_SSE2 void _mm_stream_pd(double *p, __m128d a)
{
#if HAS_BUILTIN(__builtin_nontemporal_store)
    __builtin_nontemporal_store((__v2df)a, (__v2df *)p);
#else
    __builtin_ia32_movntpd(p, a);
#endif
}

__INTRIN_INLINE_SSE2 void _mm_stream_si128(__m128i *p, __m128i a)
{
#if HAS_BUILTIN(__builtin_nontemporal_store)
    __builtin_nontemporal_store((__v2di)a, (__v2di*)p);
#else
    __builtin_ia32_movntdq(p, a);
#endif
}

__INTRIN_INLINE_SSE2 void _mm_stream_si32(int *p, int a)
{
    __builtin_ia32_movnti(p, a);
}

#ifdef _M_AMD64
__INTRIN_INLINE_SSE2 void _mm_stream_si64(long long *p, long long a)
{
    __builtin_ia32_movnti64(p, a);
}
#endif

void _mm_clflush(void const *p);

void _mm_lfence(void);

void _mm_mfence(void);

__INTRIN_INLINE_SSE2 __m128i _mm_packs_epi16(__m128i a, __m128i b)
{
    return (__m128i)__builtin_ia32_packsswb128((__v8hi)a, (__v8hi)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_packs_epi32(__m128i a, __m128i b)
{
    return (__m128i)__builtin_ia32_packssdw128((__v4si)a, (__v4si)b);
}

__INTRIN_INLINE_SSE2 __m128i _mm_packus_epi16(__m128i a, __m128i b)
{
    return (__m128i)__builtin_ia32_packuswb128((__v8hi)a, (__v8hi)b);
}

#define _mm_extract_epi16(a, imm)                                              \
    ((int)(unsigned short)__builtin_ia32_vec_ext_v8hi((__v8hi)(__m128i)(a),      \
                                                      (int)(imm)))

#define _mm_insert_epi16(a, b, imm)                                            \
    ((__m128i)__builtin_ia32_vec_set_v8hi((__v8hi)(__m128i)(a), (int)(b),        \
                                          (int)(imm)))

__INTRIN_INLINE_SSE2 int _mm_movemask_epi8(__m128i a)
{
    return __builtin_ia32_pmovmskb128((__v16qi)a);
}

#define _mm_shuffle_epi32(a, imm)                                              \
    ((__m128i)__builtin_ia32_pshufd((__v4si)(__m128i)(a), (int)(imm)))

#define _mm_shufflelo_epi16(a, imm)                                            \
    ((__m128i)__builtin_ia32_pshuflw((__v8hi)(__m128i)(a), (int)(imm)))

#define _mm_shufflehi_epi16(a, imm)                                            \
    ((__m128i)__builtin_ia32_pshufhw((__v8hi)(__m128i)(a), (int)(imm)))

__INTRIN_INLINE_SSE2 __m128i _mm_unpackhi_epi8(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return (__m128i)__builtin_shufflevector(
        (__v16qi)a, (__v16qi)b, 8, 16 + 8, 9, 16 + 9, 10, 16 + 10, 11,
        16 + 11, 12, 16 + 12, 13, 16 + 13, 14, 16 + 14, 15, 16 + 15);
#else
    return (__m128i)__builtin_ia32_punpckhbw128((__v16qi)a, (__v16qi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_unpackhi_epi16(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return (__m128i)__builtin_shufflevector((__v8hi)a, (__v8hi)b, 4, 8 + 4, 5,
                                            8 + 5, 6, 8 + 6, 7, 8 + 7);
#else
    return (__m128i)__builtin_ia32_punpckhwd128((__v8hi)a, (__v8hi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_unpackhi_epi32(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return (__m128i)__builtin_shufflevector((__v4si)a, (__v4si)b, 2, 4 + 2, 3,
                                            4 + 3);
#else
    return (__m128i)__builtin_ia32_punpckhdq128((__v4si)a, (__v4si)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_unpackhi_epi64(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return (__m128i)__builtin_shufflevector((__v2di)a, (__v2di)b, 1, 2 + 1);
#else
    return (__m128i)__builtin_ia32_punpckhqdq128((__v2di)a, (__v2di)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_unpacklo_epi8(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return (__m128i)__builtin_shufflevector(
        (__v16qi)a, (__v16qi)b, 0, 16 + 0, 1, 16 + 1, 2, 16 + 2, 3, 16 + 3, 4,
        16 + 4, 5, 16 + 5, 6, 16 + 6, 7, 16 + 7);
#else
    return (__m128i)__builtin_ia32_punpcklbw128((__v16qi)a, (__v16qi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_unpacklo_epi16(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return (__m128i)__builtin_shufflevector((__v8hi)a, (__v8hi)b, 0, 8 + 0, 1,
                                            8 + 1, 2, 8 + 2, 3, 8 + 3);
#else
    return (__m128i)__builtin_ia32_punpcklwd128((__v8hi)a, (__v8hi)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_unpacklo_epi32(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return (__m128i)__builtin_shufflevector((__v4si)a, (__v4si)b, 0, 4 + 0, 1,
                                            4 + 1);
#else
    return (__m128i)__builtin_ia32_punpckldq128((__v4si)a, (__v4si)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128i _mm_unpacklo_epi64(__m128i a, __m128i b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return (__m128i)__builtin_shufflevector((__v2di)a, (__v2di)b, 0, 2 + 0);
#else
    return (__m128i)__builtin_ia32_punpcklqdq128((__v2di)a, (__v2di)b);
#endif
}

__INTRIN_INLINE_SSE2 __m64 _mm_movepi64_pi64(__m128i a)
{
    return (__m64)a[0];
}

__INTRIN_INLINE_SSE2 __m128i _mm_movpi64_epi64(__m64 a)
{
    return __extension__(__m128i)(__v2di){(long long)a, 0};
}

__INTRIN_INLINE_SSE2 __m128i _mm_move_epi64(__m128i a)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return __builtin_shufflevector((__v2di)a, _mm_setzero_si128(), 0, 2);
#else
    return (__m128i)__builtin_ia32_movq128((__v2di)a);
#endif
}

__INTRIN_INLINE_SSE2 __m128d _mm_unpackhi_pd(__m128d a, __m128d b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return __builtin_shufflevector((__v2df)a, (__v2df)b, 1, 2 + 1);
#else
    return (__m128d)__builtin_ia32_unpckhpd((__v2df)a, (__v2df)b);
#endif
}

__INTRIN_INLINE_SSE2 __m128d _mm_unpacklo_pd(__m128d a, __m128d b)
{
#if HAS_BUILTIN(__builtin_shufflevector)
    return __builtin_shufflevector((__v2df)a, (__v2df)b, 0, 2 + 0);
#else
    return (__m128d)__builtin_ia32_unpcklpd((__v2df)a, (__v2df)b);
#endif
}

__INTRIN_INLINE_SSE2 int _mm_movemask_pd(__m128d a)
{
    return __builtin_ia32_movmskpd((__v2df)a);
}

#define _mm_shuffle_pd(a, b, i)                                                \
    ((__m128d)__builtin_ia32_shufpd((__v2df)(__m128d)(a), (__v2df)(__m128d)(b),  \
                                    (int)(i)))

__INTRIN_INLINE_SSE2 __m128 _mm_castpd_ps(__m128d a)
{
    return (__m128)a;
}

__INTRIN_INLINE_SSE2 __m128i _mm_castpd_si128(__m128d a)
{
    return (__m128i)a;
}

__INTRIN_INLINE_SSE2 __m128d _mm_castps_pd(__m128 a)
{
    return (__m128d)a;
}

__INTRIN_INLINE_SSE2 __m128i _mm_castps_si128(__m128 a)
{
    return (__m128i)a;
}

__INTRIN_INLINE_SSE2 __m128 _mm_castsi128_ps(__m128i a)
{
    return (__m128)a;
}

__INTRIN_INLINE_SSE2 __m128d _mm_castsi128_pd(__m128i a)
{
    return (__m128d)a;
}

void _mm_pause(void);

#endif /* _MSC_VER */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _INCLUDED_EMM */
