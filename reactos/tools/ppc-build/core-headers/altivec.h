/* PowerPC AltiVec include file.
   Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
   Contributed by Aldy Hernandez (aldyh@redhat.com).

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the
   Free Software Foundation, 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.  */

/* As a special exception, if you include this header file into source
   files compiled by GCC, this header file does not by itself cause
   the resulting executable to be covered by the GNU General Public
   License.  This exception does not however invalidate any other
   reasons why the executable file might be covered by the GNU General
   Public License.  */

/* Implemented to conform to the specification included in the AltiVec
   Technology Programming Interface Manual (ALTIVECPIM/D 6/1999 Rev 0).  */

#ifndef _ALTIVEC_H
#define _ALTIVEC_H 1

#if !defined(__VEC__) || !defined(__ALTIVEC__)
#error Use the "-maltivec" flag to enable PowerPC AltiVec support
#endif

/* If __APPLE_ALTIVEC__ is defined, the compiler supports 'vector',
   'pixel' and 'bool' as context-sensitive AltiVec keywords (in 
   non-AltiVec contexts, they revert to their original meanings,
   if any), so we do not need to define them as macros.  */

#if !defined(__APPLE_ALTIVEC__)
/* You are allowed to undef these for C++ compatibility.  */
#define vector __vector
#define pixel __pixel
#define bool __bool
#endif

/* Condition register codes for AltiVec predicates. */

#define __CR6_EQ		0
#define __CR6_EQ_REV		1
#define __CR6_LT		2
#define __CR6_LT_REV		3

/* These are easy... Same exact arguments.  */

#define vec_vaddcuw vec_addc
#define vec_vand vec_and
#define vec_vandc vec_andc
#define vec_vrfip vec_ceil
#define vec_vcmpbfp vec_cmpb
#define vec_vcmpgefp vec_cmpge
#define vec_vctsxs vec_cts
#define vec_vctuxs vec_ctu
#define vec_vexptefp vec_expte
#define vec_vrfim vec_floor
#define vec_lvx vec_ld
#define vec_lvxl vec_ldl
#define vec_vlogefp vec_loge
#define vec_vmaddfp vec_madd
#define vec_vmhaddshs vec_madds
#define vec_vmladduhm vec_mladd
#define vec_vmhraddshs vec_mradds
#define vec_vnmsubfp vec_nmsub
#define vec_vnor vec_nor
#define vec_vor vec_or
#define vec_vpkpx vec_packpx
#define vec_vperm vec_perm
#define vec_vrefp vec_re
#define vec_vrfin vec_round
#define vec_vrsqrtefp vec_rsqrte
#define vec_vsel vec_sel
#define vec_vsldoi vec_sld
#define vec_vsl vec_sll
#define vec_vslo vec_slo
#define vec_vspltisb vec_splat_s8
#define vec_vspltish vec_splat_s16
#define vec_vspltisw vec_splat_s32
#define vec_vsr vec_srl
#define vec_vsro vec_sro
#define vec_stvx vec_st
#define vec_stvxl vec_stl
#define vec_vsubcuw vec_subc
#define vec_vsum2sws vec_sum2s
#define vec_vsumsws vec_sums
#define vec_vrfiz vec_trunc
#define vec_vxor vec_xor

#ifdef __cplusplus

extern "C++" {

/* Prototypes for builtins that take literals and must always be
   inlined.  */
inline __vector float vec_ctf (__vector unsigned int, const int) __attribute__ ((always_inline));
inline __vector float vec_ctf (__vector signed int, const int) __attribute__ ((always_inline));
inline __vector float vec_vcfsx (__vector signed int a1, const int a2) __attribute__ ((always_inline));
inline __vector float vec_vcfux (__vector unsigned int a1, const int a2) __attribute__ ((always_inline));
inline __vector signed int vec_cts (__vector float, const int) __attribute__ ((always_inline));
inline __vector unsigned int vec_ctu (__vector float, const int) __attribute__ ((always_inline));
inline void vec_dss (const int) __attribute__ ((always_inline));

inline void vec_dst (const __vector unsigned char *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector signed char *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector __bool char *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector unsigned short *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector signed short *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector __bool short *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector __pixel *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector unsigned int *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector signed int *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector __bool int *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const __vector float *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const unsigned char *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const signed char *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const unsigned short *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const short *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const unsigned int *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const int *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const unsigned long *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const long *, int, const int) __attribute__ ((always_inline));
inline void vec_dst (const float *, int, const int) __attribute__ ((always_inline));

inline void vec_dstst (const __vector unsigned char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector signed char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector __bool char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector unsigned short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector signed short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector __bool short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector __pixel *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector unsigned int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector signed int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector __bool int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const __vector float *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const unsigned char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const signed char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const unsigned short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const unsigned int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const unsigned long *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const long *, int, const int) __attribute__ ((always_inline));
inline void vec_dstst (const float *, int, const int) __attribute__ ((always_inline));

inline void vec_dststt (const __vector unsigned char *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector signed char *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector __bool char *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector unsigned short *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector signed short *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector __bool short *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector __pixel *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector unsigned int *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector signed int *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector __bool int *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const __vector float *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const unsigned char *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const signed char *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const unsigned short *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const short *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const unsigned int *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const int *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const unsigned long *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const long *, int, const int) __attribute__ ((always_inline));
inline void vec_dststt (const float *, int, const int) __attribute__ ((always_inline));

inline void vec_dstt (const __vector unsigned char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector signed char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector __bool char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector unsigned short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector signed short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector __bool short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector __pixel *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector unsigned int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector signed int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector __bool int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const __vector float *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const unsigned char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const signed char *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const unsigned short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const short *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const unsigned int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const int *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const unsigned long *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const long *, int, const int) __attribute__ ((always_inline));
inline void vec_dstt (const float *, int, const int) __attribute__ ((always_inline));

inline __vector float vec_sld (__vector float, __vector float, const int) __attribute__ ((always_inline));
inline __vector signed int vec_sld (__vector signed int, __vector signed int, const int) __attribute__ ((always_inline));
inline __vector unsigned int vec_sld (__vector unsigned int, __vector unsigned int, const int) __attribute__ ((always_inline));
inline __vector __bool int vec_sld (__vector __bool int, __vector __bool int, const int) __attribute__ ((always_inline));
inline __vector signed short vec_sld (__vector signed short, __vector signed short, const int) __attribute__ ((always_inline));
inline __vector unsigned short vec_sld (__vector unsigned short, __vector unsigned short, const int) __attribute__ ((always_inline));
inline __vector __bool short vec_sld (__vector __bool short, __vector __bool short, const int) __attribute__ ((always_inline));
inline __vector __pixel vec_sld (__vector __pixel, __vector __pixel, const int) __attribute__ ((always_inline));
inline __vector signed char vec_sld (__vector signed char, __vector signed char, const int) __attribute__ ((always_inline));
inline __vector unsigned char vec_sld (__vector unsigned char, __vector unsigned char, const int) __attribute__ ((always_inline));
inline __vector __bool char vec_sld (__vector __bool char, __vector __bool char, const int) __attribute__ ((always_inline));
inline __vector signed char vec_splat (__vector signed char, const int) __attribute__ ((always_inline));
inline __vector unsigned char vec_splat (__vector unsigned char, const int) __attribute__ ((always_inline));
inline __vector __bool char vec_splat (__vector __bool char, const int) __attribute__ ((always_inline));
inline __vector signed short vec_splat (__vector signed short, const int) __attribute__ ((always_inline));
inline __vector unsigned short vec_splat (__vector unsigned short, const int) __attribute__ ((always_inline));
inline __vector __bool short vec_splat (__vector __bool short, const int) __attribute__ ((always_inline));
inline __vector __pixel vec_splat (__vector __pixel, const int) __attribute__ ((always_inline));
inline __vector float vec_splat (__vector float, const int) __attribute__ ((always_inline));
inline __vector signed int vec_splat (__vector signed int, const int) __attribute__ ((always_inline));
inline __vector unsigned int vec_splat (__vector unsigned int, const int) __attribute__ ((always_inline));
inline __vector __bool int vec_splat (__vector __bool int, const int) __attribute__ ((always_inline));
inline __vector signed char vec_splat_s8 (const int) __attribute__ ((always_inline));
inline __vector signed short vec_splat_s16 (const int) __attribute__ ((always_inline));
inline __vector signed int vec_splat_s32 (const int) __attribute__ ((always_inline));
inline __vector unsigned char vec_splat_u8 (const int) __attribute__ ((always_inline));
inline __vector unsigned short vec_splat_u16 (const int) __attribute__ ((always_inline));
inline __vector unsigned int vec_splat_u32 (const int) __attribute__ ((always_inline));
inline __vector float vec_vspltw (__vector float a1, const int a2) __attribute__ ((always_inline));
inline __vector __bool int vec_vspltw (__vector __bool int a1, const int a2) __attribute__ ((always_inline));
inline __vector signed int vec_vspltw (__vector signed int a1, const int a2) __attribute__ ((always_inline));
inline __vector unsigned int vec_vspltw (__vector unsigned int a1, const int a2) __attribute__ ((always_inline));
inline __vector __bool short vec_vsplth (__vector __bool short a1, const int a2) __attribute__ ((always_inline));
inline __vector signed short vec_vsplth (__vector signed short a1, const int a2) __attribute__ ((always_inline));
inline __vector unsigned short vec_vsplth (__vector unsigned short a1, const int a2) __attribute__ ((always_inline));
inline __vector __pixel vec_vsplth (__vector __pixel a1, const int a2) __attribute__ ((always_inline));
inline __vector __bool char vec_vspltb (__vector __bool char a1, const int a2) __attribute__ ((always_inline));
inline __vector signed char vec_vspltb (__vector signed char a1, const int a2) __attribute__ ((always_inline));
inline __vector unsigned char vec_vspltb (__vector unsigned char a1, const int a2) __attribute__ ((always_inline));

/* vec_step */

template<typename _Tp>
struct __vec_step_help
{
  // All proper __vector types will specialize _S_elem.
};

template<>
struct __vec_step_help<__vector signed short>
{
  static const int _S_elem = 8;
};

template<>
struct __vec_step_help<__vector unsigned short>
{
  static const int _S_elem = 8;
};

template<>
struct __vec_step_help<__vector __bool short>
{
  static const int _S_elem = 8;
};

template<>
struct __vec_step_help<__vector __pixel>
{
  static const int _S_elem = 8;
};

template<>
struct __vec_step_help<__vector signed int>
{
  static const int _S_elem = 4;
};

template<>
struct __vec_step_help<__vector unsigned int>
{
  static const int _S_elem = 4;
};

template<>
struct __vec_step_help<__vector __bool int>
{
  static const int _S_elem = 4;
};

template<>
struct __vec_step_help<__vector unsigned char>
{
  static const int _S_elem = 16;
};

template<>
struct __vec_step_help<__vector signed char>
{
  static const int _S_elem = 16;
};

template<>
struct __vec_step_help<__vector __bool char>
{
  static const int _S_elem = 16;
};

template<>
struct __vec_step_help<__vector float>
{
  static const int _S_elem = 4;
};

#define vec_step(t)  __vec_step_help<typeof(t)>::_S_elem

/* vec_abs */

inline __vector signed char
vec_abs (__vector signed char a1)
{
  return __builtin_altivec_abs_v16qi (a1);
}

inline __vector signed short
vec_abs (__vector signed short a1)
{
  return __builtin_altivec_abs_v8hi (a1);
}

inline __vector signed int
vec_abs (__vector signed int a1)
{
  return __builtin_altivec_abs_v4si (a1);
}

inline __vector float
vec_abs (__vector float a1)
{
  return __builtin_altivec_abs_v4sf (a1);
}

/* vec_abss */

inline __vector signed char
vec_abss (__vector signed char a1)
{
  return __builtin_altivec_abss_v16qi (a1);
}

inline __vector signed short
vec_abss (__vector signed short a1)
{
  return __builtin_altivec_abss_v8hi (a1);
}

inline __vector signed int
vec_abss (__vector signed int a1)
{
  return __builtin_altivec_abss_v4si (a1);
}

/* vec_add */

inline __vector signed char
vec_add (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_add (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_add (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_add (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_add (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_add (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed short
vec_add (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_add (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_add (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_add (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_add (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_add (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed int
vec_add (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_add (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_add (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_add (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_add (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_add (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_add (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vaddfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vaddfp */

inline __vector float
vec_vaddfp (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vaddfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vadduwm */

inline __vector signed int
vec_vadduwm (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vadduwm (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vadduwm (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vadduwm (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vadduwm (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vadduwm (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vadduhm */

inline __vector signed short
vec_vadduhm (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vadduhm (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vadduhm (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vadduhm (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vadduhm (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vadduhm (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vaddubm */

inline __vector signed char
vec_vaddubm (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vaddubm (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vaddubm (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vaddubm (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vaddubm (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vaddubm (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_addc */

inline __vector unsigned int
vec_addc (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vaddcuw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_adds */

inline __vector unsigned char
vec_adds (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_adds (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_adds (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_adds (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_adds (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_adds (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned short
vec_adds (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_adds (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_adds (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_adds (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_adds (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_adds (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned int
vec_adds (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_adds (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_adds (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_adds (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_adds (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_adds (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vaddsws */

inline __vector signed int
vec_vaddsws (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vaddsws (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vaddsws (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vadduws */

inline __vector unsigned int
vec_vadduws (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vadduws (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vadduws (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vaddshs */

inline __vector signed short
vec_vaddshs (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vaddshs (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vaddshs (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vadduhs */

inline __vector unsigned short
vec_vadduhs (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vadduhs (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vadduhs (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vaddsbs */

inline __vector signed char
vec_vaddsbs (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vaddsbs (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vaddsbs (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_vaddubs */

inline __vector unsigned char
vec_vaddubs (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vaddubs (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vaddubs (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_and */

inline __vector float
vec_and (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_and (__vector float a1, __vector __bool int a2)
{
  return (__vector float) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_and (__vector __bool int a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_and (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_and (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_and (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_and (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_and (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_and (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_and (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_and (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_and (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_and (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_and (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_and (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_and (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_and (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_and (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_and (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_and (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_and (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_and (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_and (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_and (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vand ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_andc */

inline __vector float
vec_andc (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_andc (__vector float a1, __vector __bool int a2)
{
  return (__vector float) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_andc (__vector __bool int a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_andc (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_andc (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_andc (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_andc (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_andc (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_andc (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_andc (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_andc (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_andc (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_andc (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_andc (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_andc (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_andc (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_andc (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_andc (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_andc (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_andc (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_andc (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_andc (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_andc (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_andc (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vandc ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_avg */

inline __vector unsigned char
vec_avg (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vavgub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_avg (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vavgsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned short
vec_avg (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vavguh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_avg (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vavgsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned int
vec_avg (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vavguw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_avg (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vavgsw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vavgsw */

inline __vector signed int
vec_vavgsw (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vavgsw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vavguw */

inline __vector unsigned int
vec_vavguw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vavguw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vavgsh */

inline __vector signed short
vec_vavgsh (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vavgsh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vavguh */

inline __vector unsigned short
vec_vavguh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vavguh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vavgsb */

inline __vector signed char
vec_vavgsb (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vavgsb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_vavgub */

inline __vector unsigned char
vec_vavgub (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vavgub ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_ceil */

inline __vector float
vec_ceil (__vector float a1)
{
  return (__vector float) __builtin_altivec_vrfip ((__vector float) a1);
}

/* vec_cmpb */

inline __vector signed int
vec_cmpb (__vector float a1, __vector float a2)
{
  return (__vector signed int) __builtin_altivec_vcmpbfp ((__vector float) a1, (__vector float) a2);
}

/* vec_cmpeq */

inline __vector __bool char
vec_cmpeq (__vector signed char a1, __vector signed char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpequb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector __bool char
vec_cmpeq (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpequb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector __bool short
vec_cmpeq (__vector signed short a1, __vector signed short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpequh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __bool short
vec_cmpeq (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpequh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __bool int
vec_cmpeq (__vector signed int a1, __vector signed int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpequw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_cmpeq (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpequw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_cmpeq (__vector float a1, __vector float a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpeqfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vcmpeqfp */

inline __vector __bool int
vec_vcmpeqfp (__vector float a1, __vector float a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpeqfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vcmpequw */

inline __vector __bool int
vec_vcmpequw (__vector signed int a1, __vector signed int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpequw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_vcmpequw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpequw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vcmpequh */

inline __vector __bool short
vec_vcmpequh (__vector signed short a1, __vector signed short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpequh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __bool short
vec_vcmpequh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpequh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vcmpequb */

inline __vector __bool char
vec_vcmpequb (__vector signed char a1, __vector signed char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpequb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector __bool char
vec_vcmpequb (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpequb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_cmpge */

inline __vector __bool int
vec_cmpge (__vector float a1, __vector float a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgefp ((__vector float) a1, (__vector float) a2);
}

/* vec_cmpgt */

inline __vector __bool char
vec_cmpgt (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpgtub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector __bool char
vec_cmpgt (__vector signed char a1, __vector signed char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpgtsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector __bool short
vec_cmpgt (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpgtuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __bool short
vec_cmpgt (__vector signed short a1, __vector signed short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpgtsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __bool int
vec_cmpgt (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_cmpgt (__vector signed int a1, __vector signed int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_cmpgt (__vector float a1, __vector float a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vcmpgtfp */

inline __vector __bool int
vec_vcmpgtfp (__vector float a1, __vector float a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vcmpgtsw */

inline __vector __bool int
vec_vcmpgtsw (__vector signed int a1, __vector signed int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtsw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vcmpgtuw */

inline __vector __bool int
vec_vcmpgtuw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtuw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vcmpgtsh */

inline __vector __bool short
vec_vcmpgtsh (__vector signed short a1, __vector signed short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpgtsh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vcmpgtuh */

inline __vector __bool short
vec_vcmpgtuh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpgtuh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vcmpgtsb */

inline __vector __bool char
vec_vcmpgtsb (__vector signed char a1, __vector signed char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpgtsb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_vcmpgtub */

inline __vector __bool char
vec_vcmpgtub (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpgtub ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_cmple */

inline __vector __bool int
vec_cmple (__vector float a1, __vector float a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgefp ((__vector float) a2, (__vector float) a1);
}

/* vec_cmplt */

inline __vector __bool char
vec_cmplt (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpgtub ((__vector signed char) a2, (__vector signed char) a1);
}

inline __vector __bool char
vec_cmplt (__vector signed char a1, __vector signed char a2)
{
  return (__vector __bool char) __builtin_altivec_vcmpgtsb ((__vector signed char) a2, (__vector signed char) a1);
}

inline __vector __bool short
vec_cmplt (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpgtuh ((__vector signed short) a2, (__vector signed short) a1);
}

inline __vector __bool short
vec_cmplt (__vector signed short a1, __vector signed short a2)
{
  return (__vector __bool short) __builtin_altivec_vcmpgtsh ((__vector signed short) a2, (__vector signed short) a1);
}

inline __vector __bool int
vec_cmplt (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtuw ((__vector signed int) a2, (__vector signed int) a1);
}

inline __vector __bool int
vec_cmplt (__vector signed int a1, __vector signed int a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtsw ((__vector signed int) a2, (__vector signed int) a1);
}

inline __vector __bool int
vec_cmplt (__vector float a1, __vector float a2)
{
  return (__vector __bool int) __builtin_altivec_vcmpgtfp ((__vector float) a2, (__vector float) a1);
}

/* vec_ctf */

inline __vector float
vec_ctf (__vector unsigned int a1, const int a2)
{
  return (__vector float) __builtin_altivec_vcfux ((__vector signed int) a1, a2);
}

inline __vector float
vec_ctf (__vector signed int a1, const int a2)
{
  return (__vector float) __builtin_altivec_vcfsx ((__vector signed int) a1, a2);
}

/* vec_vcfsx */

inline __vector float
vec_vcfsx (__vector signed int a1, const int a2)
{
  return (__vector float) __builtin_altivec_vcfsx ((__vector signed int) a1, a2);
}

/* vec_vcfux */

inline __vector float
vec_vcfux (__vector unsigned int a1, const int a2)
{
  return (__vector float) __builtin_altivec_vcfux ((__vector signed int) a1, a2);
}

/* vec_cts */

inline __vector signed int
vec_cts (__vector float a1, const int a2)
{
  return (__vector signed int) __builtin_altivec_vctsxs ((__vector float) a1, a2);
}

/* vec_ctu */

inline __vector unsigned int
vec_ctu (__vector float a1, const int a2)
{
  return (__vector unsigned int) __builtin_altivec_vctuxs ((__vector float) a1, a2);
}

/* vec_dss */

inline void
vec_dss (const int a1)
{
  __builtin_altivec_dss (a1);
}

/* vec_dssall */

inline void
vec_dssall (void)
{
  __builtin_altivec_dssall ();
}

/* vec_dst */

inline void
vec_dst (const __vector unsigned char *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector signed char *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector __bool char *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector unsigned short *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector signed short *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector __bool short *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector __pixel *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector unsigned int *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector signed int *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector __bool int *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const __vector float *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const unsigned char *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const signed char *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const unsigned short *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const short *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const unsigned int *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const int *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const unsigned long *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const long *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

inline void
vec_dst (const float *a1, int a2, const int a3)
{
  __builtin_altivec_dst ((void *) a1, a2, a3);
}

/* vec_dstst */

inline void
vec_dstst (const __vector unsigned char *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector signed char *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector __bool char *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector unsigned short *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector signed short *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector __bool short *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector __pixel *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector unsigned int *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector signed int *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector __bool int *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const __vector float *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const unsigned char *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const signed char *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const unsigned short *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const short *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const unsigned int *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const int *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const unsigned long *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const long *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

inline void
vec_dstst (const float *a1, int a2, const int a3)
{
  __builtin_altivec_dstst ((void *) a1, a2, a3);
}

/* vec_dststt */

inline void
vec_dststt (const __vector unsigned char *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector signed char *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector __bool char *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector unsigned short *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector signed short *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector __bool short *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector __pixel *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector unsigned int *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector signed int *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector __bool int *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const __vector float *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const unsigned char *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const signed char *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const unsigned short *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const short *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const unsigned int *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const int *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const unsigned long *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const long *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

inline void
vec_dststt (const float *a1, int a2, const int a3)
{
  __builtin_altivec_dststt ((void *) a1, a2, a3);
}

/* vec_dstt */

inline void
vec_dstt (const __vector unsigned char *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector signed char *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector __bool char *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector unsigned short *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector signed short *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector __bool short *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector __pixel *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector unsigned int *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector signed int *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector __bool int *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const __vector float *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const unsigned char *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const signed char *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const unsigned short *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const short *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const unsigned int *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const int *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const unsigned long *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const long *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

inline void
vec_dstt (const float *a1, int a2, const int a3)
{
  __builtin_altivec_dstt ((void *) a1, a2, a3);
}

/* vec_expte */

inline __vector float
vec_expte (__vector float a1)
{
  return (__vector float) __builtin_altivec_vexptefp ((__vector float) a1);
}

/* vec_floor */

inline __vector float
vec_floor (__vector float a1)
{
  return (__vector float) __builtin_altivec_vrfim ((__vector float) a1);
}

/* vec_ld */

inline __vector float
vec_ld (int a1, const __vector float *a2)
{
  return (__vector float) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector float
vec_ld (int a1, const float *a2)
{
  return (__vector float) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector __bool int
vec_ld (int a1, const __vector __bool int *a2)
{
  return (__vector __bool int) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector signed int
vec_ld (int a1, const __vector signed int *a2)
{
  return (__vector signed int) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector signed int
vec_ld (int a1, const int *a2)
{
  return (__vector signed int) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector signed int
vec_ld (int a1, const long *a2)
{
  return (__vector signed int) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector unsigned int
vec_ld (int a1, const __vector unsigned int *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector unsigned int
vec_ld (int a1, const unsigned int *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector unsigned int
vec_ld (int a1, const unsigned long *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector __bool short
vec_ld (int a1, const __vector __bool short *a2)
{
  return (__vector __bool short) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector __pixel
vec_ld (int a1, const __vector __pixel *a2)
{
  return (__vector __pixel) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector signed short
vec_ld (int a1, const __vector signed short *a2)
{
  return (__vector signed short) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector signed short
vec_ld (int a1, const short *a2)
{
  return (__vector signed short) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector unsigned short
vec_ld (int a1, const __vector unsigned short *a2)
{
  return (__vector unsigned short) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector unsigned short
vec_ld (int a1, const unsigned short *a2)
{
  return (__vector unsigned short) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector __bool char
vec_ld (int a1, const __vector __bool char *a2)
{
  return (__vector __bool char) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector signed char
vec_ld (int a1, const __vector signed char *a2)
{
  return (__vector signed char) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector signed char
vec_ld (int a1, const signed char *a2)
{
  return (__vector signed char) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector unsigned char
vec_ld (int a1, const __vector unsigned char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvx (a1, (void *) a2);
}

inline __vector unsigned char
vec_ld (int a1, const unsigned char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvx (a1, (void *) a2);
}

/* vec_lde */

inline __vector signed char
vec_lde (int a1, const signed char *a2)
{
  return (__vector signed char) __builtin_altivec_lvebx (a1, (void *) a2);
}

inline __vector unsigned char
vec_lde (int a1, const unsigned char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvebx (a1, (void *) a2);
}

inline __vector signed short
vec_lde (int a1, const short *a2)
{
  return (__vector signed short) __builtin_altivec_lvehx (a1, (void *) a2);
}

inline __vector unsigned short
vec_lde (int a1, const unsigned short *a2)
{
  return (__vector unsigned short) __builtin_altivec_lvehx (a1, (void *) a2);
}

inline __vector float
vec_lde (int a1, const float *a2)
{
  return (__vector float) __builtin_altivec_lvewx (a1, (void *) a2);
}

inline __vector signed int
vec_lde (int a1, const int *a2)
{
  return (__vector signed int) __builtin_altivec_lvewx (a1, (void *) a2);
}

inline __vector unsigned int
vec_lde (int a1, const unsigned int *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvewx (a1, (void *) a2);
}

inline __vector signed int
vec_lde (int a1, const long *a2)
{
  return (__vector signed int) __builtin_altivec_lvewx (a1, (void *) a2);
}

inline __vector unsigned int
vec_lde (int a1, const unsigned long *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvewx (a1, (void *) a2);
}

/* vec_lvewx */

inline __vector float
vec_lvewx (int a1, float *a2)
{
  return (__vector float) __builtin_altivec_lvewx (a1, (void *) a2);
}

inline __vector signed int
vec_lvewx (int a1, int *a2)
{
  return (__vector signed int) __builtin_altivec_lvewx (a1, (void *) a2);
}

inline __vector unsigned int
vec_lvewx (int a1, unsigned int *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvewx (a1, (void *) a2);
}

inline __vector signed int
vec_lvewx (int a1, long *a2)
{
  return (__vector signed int) __builtin_altivec_lvewx (a1, (void *) a2);
}

inline __vector unsigned int
vec_lvewx (int a1, unsigned long *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvewx (a1, (void *) a2);
}

/* vec_lvehx */

inline __vector signed short
vec_lvehx (int a1, short *a2)
{
  return (__vector signed short) __builtin_altivec_lvehx (a1, (void *) a2);
}

inline __vector unsigned short
vec_lvehx (int a1, unsigned short *a2)
{
  return (__vector unsigned short) __builtin_altivec_lvehx (a1, (void *) a2);
}

/* vec_lvebx */

inline __vector signed char
vec_lvebx (int a1, signed char *a2)
{
  return (__vector signed char) __builtin_altivec_lvebx (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvebx (int a1, unsigned char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvebx (a1, (void *) a2);
}

/* vec_ldl */

inline __vector float
vec_ldl (int a1, const __vector float *a2)
{
  return (__vector float) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector float
vec_ldl (int a1, const float *a2)
{
  return (__vector float) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector __bool int
vec_ldl (int a1, const __vector __bool int *a2)
{
  return (__vector __bool int) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector signed int
vec_ldl (int a1, const __vector signed int *a2)
{
  return (__vector signed int) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector signed int
vec_ldl (int a1, const int *a2)
{
  return (__vector signed int) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector signed int
vec_ldl (int a1, const long *a2)
{
  return (__vector signed int) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector unsigned int
vec_ldl (int a1, const __vector unsigned int *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector unsigned int
vec_ldl (int a1, const unsigned int *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector unsigned int
vec_ldl (int a1, const unsigned long *a2)
{
  return (__vector unsigned int) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector __bool short
vec_ldl (int a1, const __vector __bool short *a2)
{
  return (__vector __bool short) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector __pixel
vec_ldl (int a1, const __vector __pixel *a2)
{
  return (__vector __pixel) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector signed short
vec_ldl (int a1, const __vector signed short *a2)
{
  return (__vector signed short) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector signed short
vec_ldl (int a1, const short *a2)
{
  return (__vector signed short) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector unsigned short
vec_ldl (int a1, const __vector unsigned short *a2)
{
  return (__vector unsigned short) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector unsigned short
vec_ldl (int a1, const unsigned short *a2)
{
  return (__vector unsigned short) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector __bool char
vec_ldl (int a1, const __vector __bool char *a2)
{
  return (__vector __bool char) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector signed char
vec_ldl (int a1, const __vector signed char *a2)
{
  return (__vector signed char) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector signed char
vec_ldl (int a1, const signed char *a2)
{
  return (__vector signed char) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector unsigned char
vec_ldl (int a1, const __vector unsigned char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvxl (a1, (void *) a2);
}

inline __vector unsigned char
vec_ldl (int a1, const unsigned char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvxl (a1, (void *) a2);
}

/* vec_loge */

inline __vector float
vec_loge (__vector float a1)
{
  return (__vector float) __builtin_altivec_vlogefp ((__vector float) a1);
}

/* vec_lvsl */

inline __vector unsigned char
vec_lvsl (int a1, const volatile unsigned char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsl (int a1, const volatile signed char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsl (int a1, const volatile unsigned short *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsl (int a1, const volatile short *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsl (int a1, const volatile unsigned int *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsl (int a1, const volatile int *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsl (int a1, const volatile unsigned long *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsl (int a1, const volatile long *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsl (int a1, const volatile float *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsl (a1, (void *) a2);
}

/* vec_lvsr */

inline __vector unsigned char
vec_lvsr (int a1, const volatile unsigned char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsr (int a1, const volatile signed char *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsr (int a1, const volatile unsigned short *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsr (int a1, const volatile short *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsr (int a1, const volatile unsigned int *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsr (int a1, const volatile int *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsr (int a1, const volatile unsigned long *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsr (int a1, const volatile long *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

inline __vector unsigned char
vec_lvsr (int a1, const volatile float *a2)
{
  return (__vector unsigned char) __builtin_altivec_lvsr (a1, (void *) a2);
}

/* vec_madd */

inline __vector float
vec_madd (__vector float a1, __vector float a2, __vector float a3)
{
  return (__vector float) __builtin_altivec_vmaddfp ((__vector float) a1, (__vector float) a2, (__vector float) a3);
}

/* vec_madds */

inline __vector signed short
vec_madds (__vector signed short a1, __vector signed short a2, __vector signed short a3)
{
  return (__vector signed short) __builtin_altivec_vmhaddshs ((__vector signed short) a1, (__vector signed short) a2, (__vector signed short) a3);
}

/* vec_max */

inline __vector unsigned char
vec_max (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_max (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_max (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_max (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_max (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_max (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned short
vec_max (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_max (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_max (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_max (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_max (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_max (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned int
vec_max (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_max (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_max (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_max (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_max (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_max (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_max (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vmaxfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vmaxfp */

inline __vector float
vec_vmaxfp (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vmaxfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vmaxsw */

inline __vector signed int
vec_vmaxsw (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vmaxsw (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vmaxsw (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vmaxuw */

inline __vector unsigned int
vec_vmaxuw (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vmaxuw (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vmaxuw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vmaxsh */

inline __vector signed short
vec_vmaxsh (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vmaxsh (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vmaxsh (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmaxuh */

inline __vector unsigned short
vec_vmaxuh (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vmaxuh (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vmaxuh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmaxsb */

inline __vector signed char
vec_vmaxsb (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vmaxsb (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vmaxsb (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_vmaxub */

inline __vector unsigned char
vec_vmaxub (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vmaxub (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vmaxub (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_mergeh */

inline __vector __bool char
vec_mergeh (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vmrghb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_mergeh (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vmrghb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_mergeh (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmrghb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector __bool short
vec_mergeh (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vmrghh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __pixel
vec_mergeh (__vector __pixel a1, __vector __pixel a2)
{
  return (__vector __pixel) __builtin_altivec_vmrghh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_mergeh (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vmrghh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_mergeh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmrghh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector float
vec_mergeh (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vmrghw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_mergeh (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vmrghw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_mergeh (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vmrghw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_mergeh (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmrghw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vmrghw */

inline __vector float
vec_vmrghw (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vmrghw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_vmrghw (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vmrghw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vmrghw (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vmrghw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vmrghw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmrghw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vmrghh */

inline __vector __bool short
vec_vmrghh (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vmrghh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vmrghh (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vmrghh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vmrghh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmrghh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __pixel
vec_vmrghh (__vector __pixel a1, __vector __pixel a2)
{
  return (__vector __pixel) __builtin_altivec_vmrghh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmrghb */

inline __vector __bool char
vec_vmrghb (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vmrghb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vmrghb (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vmrghb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vmrghb (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmrghb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_mergel */

inline __vector __bool char
vec_mergel (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vmrglb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_mergel (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vmrglb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_mergel (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmrglb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector __bool short
vec_mergel (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vmrglh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __pixel
vec_mergel (__vector __pixel a1, __vector __pixel a2)
{
  return (__vector __pixel) __builtin_altivec_vmrglh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_mergel (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vmrglh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_mergel (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmrglh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector float
vec_mergel (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vmrglw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_mergel (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vmrglw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_mergel (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vmrglw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_mergel (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmrglw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vmrglw */

inline __vector float
vec_vmrglw (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vmrglw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vmrglw (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vmrglw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vmrglw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vmrglw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_vmrglw (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vmrglw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vmrglh */

inline __vector __bool short
vec_vmrglh (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vmrglh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vmrglh (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vmrglh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vmrglh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vmrglh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __pixel
vec_vmrglh (__vector __pixel a1, __vector __pixel a2)
{
  return (__vector __pixel) __builtin_altivec_vmrglh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmrglb */

inline __vector __bool char
vec_vmrglb (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vmrglb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vmrglb (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vmrglb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vmrglb (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vmrglb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_mfvscr */

inline __vector unsigned short
vec_mfvscr (void)
{
  return (__vector unsigned short) __builtin_altivec_mfvscr ();
}

/* vec_min */

inline __vector unsigned char
vec_min (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_min (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_min (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_min (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vminsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_min (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vminsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_min (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vminsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned short
vec_min (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_min (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_min (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_min (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vminsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_min (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vminsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_min (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vminsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned int
vec_min (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_min (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_min (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_min (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vminsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_min (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vminsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_min (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vminsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_min (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vminfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vminfp */

inline __vector float
vec_vminfp (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vminfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vminsw */

inline __vector signed int
vec_vminsw (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vminsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vminsw (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vminsw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vminsw (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vminsw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vminuw */

inline __vector unsigned int
vec_vminuw (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vminuw (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vminuw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vminsh */

inline __vector signed short
vec_vminsh (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vminsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vminsh (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vminsh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vminsh (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vminsh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vminuh */

inline __vector unsigned short
vec_vminuh (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vminuh (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vminuh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vminsb */

inline __vector signed char
vec_vminsb (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vminsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vminsb (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vminsb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vminsb (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vminsb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_vminub */

inline __vector unsigned char
vec_vminub (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vminub (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vminub (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_mladd */

inline __vector signed short
vec_mladd (__vector signed short a1, __vector signed short a2, __vector signed short a3)
{
  return (__vector signed short) __builtin_altivec_vmladduhm ((__vector signed short) a1, (__vector signed short) a2, (__vector signed short) a3);
}

inline __vector signed short
vec_mladd (__vector signed short a1, __vector unsigned short a2, __vector unsigned short a3)
{
  return (__vector signed short) __builtin_altivec_vmladduhm ((__vector signed short) a1, (__vector signed short) a2, (__vector signed short) a3);
}

inline __vector signed short
vec_mladd (__vector unsigned short a1, __vector signed short a2, __vector signed short a3)
{
  return (__vector signed short) __builtin_altivec_vmladduhm ((__vector signed short) a1, (__vector signed short) a2, (__vector signed short) a3);
}

inline __vector unsigned short
vec_mladd (__vector unsigned short a1, __vector unsigned short a2, __vector unsigned short a3)
{
  return (__vector unsigned short) __builtin_altivec_vmladduhm ((__vector signed short) a1, (__vector signed short) a2, (__vector signed short) a3);
}

/* vec_mradds */

inline __vector signed short
vec_mradds (__vector signed short a1, __vector signed short a2, __vector signed short a3)
{
  return (__vector signed short) __builtin_altivec_vmhraddshs ((__vector signed short) a1, (__vector signed short) a2, (__vector signed short) a3);
}

/* vec_msum */

inline __vector unsigned int
vec_msum (__vector unsigned char a1, __vector unsigned char a2, __vector unsigned int a3)
{
  return (__vector unsigned int) __builtin_altivec_vmsumubm ((__vector signed char) a1, (__vector signed char) a2, (__vector signed int) a3);
}

inline __vector signed int
vec_msum (__vector signed char a1, __vector unsigned char a2, __vector signed int a3)
{
  return (__vector signed int) __builtin_altivec_vmsummbm ((__vector signed char) a1, (__vector signed char) a2, (__vector signed int) a3);
}

inline __vector unsigned int
vec_msum (__vector unsigned short a1, __vector unsigned short a2, __vector unsigned int a3)
{
  return (__vector unsigned int) __builtin_altivec_vmsumuhm ((__vector signed short) a1, (__vector signed short) a2, (__vector signed int) a3);
}

inline __vector signed int
vec_msum (__vector signed short a1, __vector signed short a2, __vector signed int a3)
{
  return (__vector signed int) __builtin_altivec_vmsumshm ((__vector signed short) a1, (__vector signed short) a2, (__vector signed int) a3);
}

/* vec_vmsumshm */

inline __vector signed int
vec_vmsumshm (__vector signed short a1, __vector signed short a2, __vector signed int a3)
{
  return (__vector signed int) __builtin_altivec_vmsumshm ((__vector signed short) a1, (__vector signed short) a2, (__vector signed int) a3);
}

/* vec_vmsumuhm */

inline __vector unsigned int
vec_vmsumuhm (__vector unsigned short a1, __vector unsigned short a2, __vector unsigned int a3)
{
  return (__vector unsigned int) __builtin_altivec_vmsumuhm ((__vector signed short) a1, (__vector signed short) a2, (__vector signed int) a3);
}

/* vec_vmsummbm */

inline __vector signed int
vec_vmsummbm (__vector signed char a1, __vector unsigned char a2, __vector signed int a3)
{
  return (__vector signed int) __builtin_altivec_vmsummbm ((__vector signed char) a1, (__vector signed char) a2, (__vector signed int) a3);
}

/* vec_vmsumubm */

inline __vector unsigned int
vec_vmsumubm (__vector unsigned char a1, __vector unsigned char a2, __vector unsigned int a3)
{
  return (__vector unsigned int) __builtin_altivec_vmsumubm ((__vector signed char) a1, (__vector signed char) a2, (__vector signed int) a3);
}

/* vec_msums */

inline __vector unsigned int
vec_msums (__vector unsigned short a1, __vector unsigned short a2, __vector unsigned int a3)
{
  return (__vector unsigned int) __builtin_altivec_vmsumuhs ((__vector signed short) a1, (__vector signed short) a2, (__vector signed int) a3);
}

inline __vector signed int
vec_msums (__vector signed short a1, __vector signed short a2, __vector signed int a3)
{
  return (__vector signed int) __builtin_altivec_vmsumshs ((__vector signed short) a1, (__vector signed short) a2, (__vector signed int) a3);
}

/* vec_vmsumshs */

inline __vector signed int
vec_vmsumshs (__vector signed short a1, __vector signed short a2, __vector signed int a3)
{
  return (__vector signed int) __builtin_altivec_vmsumshs ((__vector signed short) a1, (__vector signed short) a2, (__vector signed int) a3);
}

/* vec_vmsumuhs */

inline __vector unsigned int
vec_vmsumuhs (__vector unsigned short a1, __vector unsigned short a2, __vector unsigned int a3)
{
  return (__vector unsigned int) __builtin_altivec_vmsumuhs ((__vector signed short) a1, (__vector signed short) a2, (__vector signed int) a3);
}

/* vec_mtvscr */

inline void
vec_mtvscr (__vector signed int a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector unsigned int a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector __bool int a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector signed short a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector unsigned short a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector __bool short a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector __pixel a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector signed char a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector unsigned char a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

inline void
vec_mtvscr (__vector __bool char a1)
{
  __builtin_altivec_mtvscr ((__vector signed int) a1);
}

/* vec_mule */

inline __vector unsigned short
vec_mule (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned short) __builtin_altivec_vmuleub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed short
vec_mule (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed short) __builtin_altivec_vmulesb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned int
vec_mule (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned int) __builtin_altivec_vmuleuh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed int
vec_mule (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed int) __builtin_altivec_vmulesh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmulesh */

inline __vector signed int
vec_vmulesh (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed int) __builtin_altivec_vmulesh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmuleuh */

inline __vector unsigned int
vec_vmuleuh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned int) __builtin_altivec_vmuleuh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmulesb */

inline __vector signed short
vec_vmulesb (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed short) __builtin_altivec_vmuleub ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_vmuleub */

inline __vector unsigned short
vec_vmuleub (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned short) __builtin_altivec_vmuleub ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_mulo */

inline __vector unsigned short
vec_mulo (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned short) __builtin_altivec_vmuloub ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed short
vec_mulo (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed short) __builtin_altivec_vmulosb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned int
vec_mulo (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned int) __builtin_altivec_vmulouh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed int
vec_mulo (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed int) __builtin_altivec_vmulosh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmulosh */

inline __vector signed int
vec_vmulosh (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed int) __builtin_altivec_vmulosh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmulouh */

inline __vector unsigned int
vec_vmulouh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned int) __builtin_altivec_vmulouh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vmulosb */

inline __vector signed short
vec_vmulosb (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed short) __builtin_altivec_vmulosb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_vmuloub */

inline __vector unsigned short
vec_vmuloub (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned short) __builtin_altivec_vmuloub ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_nmsub */

inline __vector float
vec_nmsub (__vector float a1, __vector float a2, __vector float a3)
{
  return (__vector float) __builtin_altivec_vnmsubfp ((__vector float) a1, (__vector float) a2, (__vector float) a3);
}

/* vec_nor */

inline __vector float
vec_nor (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_nor (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_nor (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_nor (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_nor (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_nor (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_nor (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_nor (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_nor (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_nor (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vnor ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_or */

inline __vector float
vec_or (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_or (__vector float a1, __vector __bool int a2)
{
  return (__vector float) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_or (__vector __bool int a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_or (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_or (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_or (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_or (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_or (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_or (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_or (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_or (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_or (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_or (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_or (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_or (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_or (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_or (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_or (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_or (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_or (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_or (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_or (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_or (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_or (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vor ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_pack */

inline __vector signed char
vec_pack (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed char) __builtin_altivec_vpkuhum ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned char
vec_pack (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned char) __builtin_altivec_vpkuhum ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector __bool char
vec_pack (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool char) __builtin_altivec_vpkuhum ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_pack (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed short) __builtin_altivec_vpkuwum ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_pack (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned short) __builtin_altivec_vpkuwum ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_pack (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool short) __builtin_altivec_vpkuwum ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vpkuwum */

inline __vector __bool short
vec_vpkuwum (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool short) __builtin_altivec_vpkuwum ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_vpkuwum (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed short) __builtin_altivec_vpkuwum ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_vpkuwum (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned short) __builtin_altivec_vpkuwum ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vpkuhum */

inline __vector __bool char
vec_vpkuhum (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool char) __builtin_altivec_vpkuhum ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed char
vec_vpkuhum (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed char) __builtin_altivec_vpkuhum ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned char
vec_vpkuhum (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned char) __builtin_altivec_vpkuhum ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_packpx */

inline __vector __pixel
vec_packpx (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector __pixel) __builtin_altivec_vpkpx ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_packs */

inline __vector unsigned char
vec_packs (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned char) __builtin_altivec_vpkuhus ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed char
vec_packs (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed char) __builtin_altivec_vpkshss ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_packs (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned short) __builtin_altivec_vpkuwus ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_packs (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed short) __builtin_altivec_vpkswss ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vpkswss */

inline __vector signed short
vec_vpkswss (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed short) __builtin_altivec_vpkswss ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vpkuwus */

inline __vector unsigned short
vec_vpkuwus (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned short) __builtin_altivec_vpkuwus ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vpkshss */

inline __vector signed char
vec_vpkshss (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed char) __builtin_altivec_vpkshss ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vpkuhus */

inline __vector unsigned char
vec_vpkuhus (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned char) __builtin_altivec_vpkuhus ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_packsu */

inline __vector unsigned char
vec_packsu (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned char) __builtin_altivec_vpkuhus ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned char
vec_packsu (__vector signed short a1, __vector signed short a2)
{
  return (__vector unsigned char) __builtin_altivec_vpkshus ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_packsu (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned short) __builtin_altivec_vpkuwus ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_packsu (__vector signed int a1, __vector signed int a2)
{
  return (__vector unsigned short) __builtin_altivec_vpkswus ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vpkswus */

inline __vector unsigned short
vec_vpkswus (__vector signed int a1, __vector signed int a2)
{
  return (__vector unsigned short) __builtin_altivec_vpkswus ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vpkshus */

inline __vector unsigned char
vec_vpkshus (__vector signed short a1, __vector signed short a2)
{
  return (__vector unsigned char) __builtin_altivec_vpkshus ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_perm */

inline __vector float
vec_perm (__vector float a1, __vector float a2, __vector unsigned char a3)
{
  return (__vector float) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector signed int
vec_perm (__vector signed int a1, __vector signed int a2, __vector unsigned char a3)
{
  return (__vector signed int) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector unsigned int
vec_perm (__vector unsigned int a1, __vector unsigned int a2, __vector unsigned char a3)
{
  return (__vector unsigned int) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector __bool int
vec_perm (__vector __bool int a1, __vector __bool int a2, __vector unsigned char a3)
{
  return (__vector __bool int) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector signed short
vec_perm (__vector signed short a1, __vector signed short a2, __vector unsigned char a3)
{
  return (__vector signed short) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector unsigned short
vec_perm (__vector unsigned short a1, __vector unsigned short a2, __vector unsigned char a3)
{
  return (__vector unsigned short) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector __bool short
vec_perm (__vector __bool short a1, __vector __bool short a2, __vector unsigned char a3)
{
  return (__vector __bool short) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector __pixel
vec_perm (__vector __pixel a1, __vector __pixel a2, __vector unsigned char a3)
{
  return (__vector __pixel) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector signed char
vec_perm (__vector signed char a1, __vector signed char a2, __vector unsigned char a3)
{
  return (__vector signed char) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector unsigned char
vec_perm (__vector unsigned char a1, __vector unsigned char a2, __vector unsigned char a3)
{
  return (__vector unsigned char) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

inline __vector __bool char
vec_perm (__vector __bool char a1, __vector __bool char a2, __vector unsigned char a3)
{
  return (__vector __bool char) __builtin_altivec_vperm_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed char) a3);
}

/* vec_re */

inline __vector float
vec_re (__vector float a1)
{
  return (__vector float) __builtin_altivec_vrefp ((__vector float) a1);
}

/* vec_rl */

inline __vector signed char
vec_rl (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vrlb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_rl (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vrlb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed short
vec_rl (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vrlh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_rl (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vrlh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed int
vec_rl (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vrlw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_rl (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vrlw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vrlw */

inline __vector signed int
vec_vrlw (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vrlw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vrlw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vrlw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vrlh */

inline __vector signed short
vec_vrlh (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vrlh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vrlh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vrlh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vrlb */

inline __vector signed char
vec_vrlb (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vrlb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vrlb (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vrlb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_round */

inline __vector float
vec_round (__vector float a1)
{
  return (__vector float) __builtin_altivec_vrfin ((__vector float) a1);
}

/* vec_rsqrte */

inline __vector float
vec_rsqrte (__vector float a1)
{
  return (__vector float) __builtin_altivec_vrsqrtefp ((__vector float) a1);
}

/* vec_sel */

inline __vector float
vec_sel (__vector float a1, __vector float a2, __vector __bool int a3)
{
  return (__vector float) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector float
vec_sel (__vector float a1, __vector float a2, __vector unsigned int a3)
{
  return (__vector float) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector signed int
vec_sel (__vector signed int a1, __vector signed int a2, __vector __bool int a3)
{
  return (__vector signed int) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector signed int
vec_sel (__vector signed int a1, __vector signed int a2, __vector unsigned int a3)
{
  return (__vector signed int) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector unsigned int
vec_sel (__vector unsigned int a1, __vector unsigned int a2, __vector __bool int a3)
{
  return (__vector unsigned int) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector unsigned int
vec_sel (__vector unsigned int a1, __vector unsigned int a2, __vector unsigned int a3)
{
  return (__vector unsigned int) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector __bool int
vec_sel (__vector __bool int a1, __vector __bool int a2, __vector __bool int a3)
{
  return (__vector __bool int) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector __bool int
vec_sel (__vector __bool int a1, __vector __bool int a2, __vector unsigned int a3)
{
  return (__vector __bool int) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector signed short
vec_sel (__vector signed short a1, __vector signed short a2, __vector __bool short a3)
{
  return (__vector signed short) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector signed short
vec_sel (__vector signed short a1, __vector signed short a2, __vector unsigned short a3)
{
  return (__vector signed short) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector unsigned short
vec_sel (__vector unsigned short a1, __vector unsigned short a2, __vector __bool short a3)
{
  return (__vector unsigned short) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector unsigned short
vec_sel (__vector unsigned short a1, __vector unsigned short a2, __vector unsigned short a3)
{
  return (__vector unsigned short) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector __bool short
vec_sel (__vector __bool short a1, __vector __bool short a2, __vector __bool short a3)
{
  return (__vector __bool short) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector __bool short
vec_sel (__vector __bool short a1, __vector __bool short a2, __vector unsigned short a3)
{
  return (__vector __bool short) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector signed char
vec_sel (__vector signed char a1, __vector signed char a2, __vector __bool char a3)
{
  return (__vector signed char) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector signed char
vec_sel (__vector signed char a1, __vector signed char a2, __vector unsigned char a3)
{
  return (__vector signed char) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector unsigned char
vec_sel (__vector unsigned char a1, __vector unsigned char a2, __vector __bool char a3)
{
  return (__vector unsigned char) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector unsigned char
vec_sel (__vector unsigned char a1, __vector unsigned char a2, __vector unsigned char a3)
{
  return (__vector unsigned char) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector __bool char
vec_sel (__vector __bool char a1, __vector __bool char a2, __vector __bool char a3)
{
  return (__vector __bool char) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

inline __vector __bool char
vec_sel (__vector __bool char a1, __vector __bool char a2, __vector unsigned char a3)
{
  return (__vector __bool char) __builtin_altivec_vsel_4si ((__vector signed int) a1, (__vector signed int) a2, (__vector signed int) a3);
}

/* vec_sl */

inline __vector signed char
vec_sl (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vslb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_sl (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vslb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed short
vec_sl (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vslh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_sl (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vslh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed int
vec_sl (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vslw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sl (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vslw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vslw */

inline __vector signed int
vec_vslw (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vslw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vslw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vslw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vslh */

inline __vector signed short
vec_vslh (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vslh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vslh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vslh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vslb */

inline __vector signed char
vec_vslb (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vslb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vslb (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vslb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_sld */

inline __vector float
vec_sld (__vector float a1, __vector float a2, const int a3)
{
  return (__vector float) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector signed int
vec_sld (__vector signed int a1, __vector signed int a2, const int a3)
{
  return (__vector signed int) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector unsigned int
vec_sld (__vector unsigned int a1, __vector unsigned int a2, const int a3)
{
  return (__vector unsigned int) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector __bool int
vec_sld (__vector __bool int a1, __vector __bool int a2, const int a3)
{
  return (__vector __bool int) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector signed short
vec_sld (__vector signed short a1, __vector signed short a2, const int a3)
{
  return (__vector signed short) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector unsigned short
vec_sld (__vector unsigned short a1, __vector unsigned short a2, const int a3)
{
  return (__vector unsigned short) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector __bool short
vec_sld (__vector __bool short a1, __vector __bool short a2, const int a3)
{
  return (__vector __bool short) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector __pixel
vec_sld (__vector __pixel a1, __vector __pixel a2, const int a3)
{
  return (__vector __pixel) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector signed char
vec_sld (__vector signed char a1, __vector signed char a2, const int a3)
{
  return (__vector signed char) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector unsigned char
vec_sld (__vector unsigned char a1, __vector unsigned char a2, const int a3)
{
  return (__vector unsigned char) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

inline __vector __bool char
vec_sld (__vector __bool char a1, __vector __bool char a2, const int a3)
{
  return (__vector __bool char) __builtin_altivec_vsldoi_4si ((__vector signed int) a1, (__vector signed int) a2, a3);
}

/* vec_sll */

inline __vector signed int
vec_sll (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_sll (__vector signed int a1, __vector unsigned short a2)
{
  return (__vector signed int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_sll (__vector signed int a1, __vector unsigned char a2)
{
  return (__vector signed int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sll (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sll (__vector unsigned int a1, __vector unsigned short a2)
{
  return (__vector unsigned int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sll (__vector unsigned int a1, __vector unsigned char a2)
{
  return (__vector unsigned int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_sll (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector __bool int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_sll (__vector __bool int a1, __vector unsigned short a2)
{
  return (__vector __bool int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_sll (__vector __bool int a1, __vector unsigned char a2)
{
  return (__vector __bool int) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_sll (__vector signed short a1, __vector unsigned int a2)
{
  return (__vector signed short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_sll (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_sll (__vector signed short a1, __vector unsigned char a2)
{
  return (__vector signed short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_sll (__vector unsigned short a1, __vector unsigned int a2)
{
  return (__vector unsigned short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_sll (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_sll (__vector unsigned short a1, __vector unsigned char a2)
{
  return (__vector unsigned short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_sll (__vector __bool short a1, __vector unsigned int a2)
{
  return (__vector __bool short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_sll (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector __bool short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_sll (__vector __bool short a1, __vector unsigned char a2)
{
  return (__vector __bool short) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_sll (__vector __pixel a1, __vector unsigned int a2)
{
  return (__vector __pixel) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_sll (__vector __pixel a1, __vector unsigned short a2)
{
  return (__vector __pixel) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_sll (__vector __pixel a1, __vector unsigned char a2)
{
  return (__vector __pixel) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_sll (__vector signed char a1, __vector unsigned int a2)
{
  return (__vector signed char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_sll (__vector signed char a1, __vector unsigned short a2)
{
  return (__vector signed char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_sll (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_sll (__vector unsigned char a1, __vector unsigned int a2)
{
  return (__vector unsigned char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_sll (__vector unsigned char a1, __vector unsigned short a2)
{
  return (__vector unsigned char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_sll (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_sll (__vector __bool char a1, __vector unsigned int a2)
{
  return (__vector __bool char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_sll (__vector __bool char a1, __vector unsigned short a2)
{
  return (__vector __bool char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_sll (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector __bool char) __builtin_altivec_vsl ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_slo */

inline __vector float
vec_slo (__vector float a1, __vector signed char a2)
{
  return (__vector float) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_slo (__vector float a1, __vector unsigned char a2)
{
  return (__vector float) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_slo (__vector signed int a1, __vector signed char a2)
{
  return (__vector signed int) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_slo (__vector signed int a1, __vector unsigned char a2)
{
  return (__vector signed int) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_slo (__vector unsigned int a1, __vector signed char a2)
{
  return (__vector unsigned int) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_slo (__vector unsigned int a1, __vector unsigned char a2)
{
  return (__vector unsigned int) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_slo (__vector signed short a1, __vector signed char a2)
{
  return (__vector signed short) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_slo (__vector signed short a1, __vector unsigned char a2)
{
  return (__vector signed short) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_slo (__vector unsigned short a1, __vector signed char a2)
{
  return (__vector unsigned short) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_slo (__vector unsigned short a1, __vector unsigned char a2)
{
  return (__vector unsigned short) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_slo (__vector __pixel a1, __vector signed char a2)
{
  return (__vector __pixel) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_slo (__vector __pixel a1, __vector unsigned char a2)
{
  return (__vector __pixel) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_slo (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_slo (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_slo (__vector unsigned char a1, __vector signed char a2)
{
  return (__vector unsigned char) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_slo (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vslo ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_splat */

inline __vector signed char
vec_splat (__vector signed char a1, const int a2)
{
  return (__vector signed char) __builtin_altivec_vspltb ((__vector signed char) a1,  a2);
}

inline __vector unsigned char
vec_splat (__vector unsigned char a1, const int a2)
{
  return (__vector unsigned char) __builtin_altivec_vspltb ((__vector signed char) a1,  a2);
}

inline __vector __bool char
vec_splat (__vector __bool char a1, const int a2)
{
  return (__vector __bool char) __builtin_altivec_vspltb ((__vector signed char) a1,  a2);
}

inline __vector signed short
vec_splat (__vector signed short a1, const int a2)
{
  return (__vector signed short) __builtin_altivec_vsplth ((__vector signed short) a1,  a2);
}

inline __vector unsigned short
vec_splat (__vector unsigned short a1, const int a2)
{
  return (__vector unsigned short) __builtin_altivec_vsplth ((__vector signed short) a1,  a2);
}

inline __vector __bool short
vec_splat (__vector __bool short a1, const int a2)
{
  return (__vector __bool short) __builtin_altivec_vsplth ((__vector signed short) a1,  a2);
}

inline __vector __pixel
vec_splat (__vector __pixel a1, const int a2)
{
  return (__vector __pixel) __builtin_altivec_vsplth ((__vector signed short) a1,  a2);
}

inline __vector float
vec_splat (__vector float a1, const int a2)
{
  return (__vector float) __builtin_altivec_vspltw ((__vector signed int) a1,  a2);
}

inline __vector signed int
vec_splat (__vector signed int a1, const int a2)
{
  return (__vector signed int) __builtin_altivec_vspltw ((__vector signed int) a1,  a2);
}

inline __vector unsigned int
vec_splat (__vector unsigned int a1, const int a2)
{
  return (__vector unsigned int) __builtin_altivec_vspltw ((__vector signed int) a1,  a2);
}

inline __vector __bool int
vec_splat (__vector __bool int a1, const int a2)
{
  return (__vector __bool int) __builtin_altivec_vspltw ((__vector signed int) a1,  a2);
}

/* vec_vspltw */

inline __vector float
vec_vspltw (__vector float a1, const int a2)
{
  return (__vector float) __builtin_altivec_vspltw ((__vector signed int) a1,  a2);
}

inline __vector signed int
vec_vspltw (__vector signed int a1, const int a2)
{
  return (__vector signed int) __builtin_altivec_vspltw ((__vector signed int) a1,  a2);
}

inline __vector unsigned int
vec_vspltw (__vector unsigned int a1, const int a2)
{
  return (__vector unsigned int) __builtin_altivec_vspltw ((__vector signed int) a1,  a2);
}

inline __vector __bool int
vec_vspltw (__vector __bool int a1, const int a2)
{
  return (__vector __bool int) __builtin_altivec_vspltw ((__vector signed int) a1,  a2);
}

/* vec_vsplth */

inline __vector __bool short
vec_vsplth (__vector __bool short a1, const int a2)
{
  return (__vector __bool short) __builtin_altivec_vsplth ((__vector signed short) a1,  a2);
}

inline __vector signed short
vec_vsplth (__vector signed short a1, const int a2)
{
  return (__vector signed short) __builtin_altivec_vsplth ((__vector signed short) a1,  a2);
}

inline __vector unsigned short
vec_vsplth (__vector unsigned short a1, const int a2)
{
  return (__vector unsigned short) __builtin_altivec_vsplth ((__vector signed short) a1,  a2);
}

inline __vector __pixel
vec_vsplth (__vector __pixel a1, const int a2)
{
  return (__vector __pixel) __builtin_altivec_vsplth ((__vector signed short) a1,  a2);
}

/* vec_vspltb */

inline __vector signed char
vec_vspltb (__vector signed char a1, const int a2)
{
  return (__vector signed char) __builtin_altivec_vspltb ((__vector signed char) a1,  a2);
}

inline __vector unsigned char
vec_vspltb (__vector unsigned char a1, const int a2)
{
  return (__vector unsigned char) __builtin_altivec_vspltb ((__vector signed char) a1,  a2);
}

inline __vector __bool char
vec_vspltb (__vector __bool char a1, const int a2)
{
  return (__vector __bool char) __builtin_altivec_vspltb ((__vector signed char) a1,  a2);
}

/* vec_splat_s8 */

inline __vector signed char
vec_splat_s8 (const int a1)
{
  return (__vector signed char) __builtin_altivec_vspltisb (a1);
}

/* vec_splat_s16 */

inline __vector signed short
vec_splat_s16 (const int a1)
{
  return (__vector signed short) __builtin_altivec_vspltish (a1);
}

/* vec_splat_s32 */

inline __vector signed int
vec_splat_s32 (const int a1)
{
  return (__vector signed int) __builtin_altivec_vspltisw (a1);
}

/* vec_splat_u8 */

inline __vector unsigned char
vec_splat_u8 (const int a1)
{
  return (__vector unsigned char) __builtin_altivec_vspltisb (a1);
}

/* vec_splat_u16 */

inline __vector unsigned short
vec_splat_u16 (const int a1)
{
  return (__vector unsigned short) __builtin_altivec_vspltish (a1);
}

/* vec_splat_u32 */

inline __vector unsigned int
vec_splat_u32 (const int a1)
{
  return (__vector unsigned int) __builtin_altivec_vspltisw (a1);
}

/* vec_sr */

inline __vector signed char
vec_sr (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vsrb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_sr (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsrb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed short
vec_sr (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vsrh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_sr (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsrh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed int
vec_sr (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vsrw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sr (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsrw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vsrw */

inline __vector signed int
vec_vsrw (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vsrw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vsrw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsrw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vsrh */

inline __vector signed short
vec_vsrh (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vsrh ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vsrh (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsrh ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vsrb */

inline __vector signed char
vec_vsrb (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vsrb ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vsrb (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsrb ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_sra */

inline __vector signed char
vec_sra (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vsrab ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_sra (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsrab ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed short
vec_sra (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vsrah ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_sra (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsrah ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed int
vec_sra (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vsraw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sra (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsraw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vsraw */

inline __vector signed int
vec_vsraw (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vsraw ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vsraw (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsraw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vsrah */

inline __vector signed short
vec_vsrah (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vsrah ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vsrah (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsrah ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vsrab */

inline __vector signed char
vec_vsrab (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vsrab ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vsrab (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsrab ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_srl */

inline __vector signed int
vec_srl (__vector signed int a1, __vector unsigned int a2)
{
  return (__vector signed int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_srl (__vector signed int a1, __vector unsigned short a2)
{
  return (__vector signed int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_srl (__vector signed int a1, __vector unsigned char a2)
{
  return (__vector signed int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_srl (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_srl (__vector unsigned int a1, __vector unsigned short a2)
{
  return (__vector unsigned int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_srl (__vector unsigned int a1, __vector unsigned char a2)
{
  return (__vector unsigned int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_srl (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector __bool int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_srl (__vector __bool int a1, __vector unsigned short a2)
{
  return (__vector __bool int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_srl (__vector __bool int a1, __vector unsigned char a2)
{
  return (__vector __bool int) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_srl (__vector signed short a1, __vector unsigned int a2)
{
  return (__vector signed short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_srl (__vector signed short a1, __vector unsigned short a2)
{
  return (__vector signed short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_srl (__vector signed short a1, __vector unsigned char a2)
{
  return (__vector signed short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_srl (__vector unsigned short a1, __vector unsigned int a2)
{
  return (__vector unsigned short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_srl (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_srl (__vector unsigned short a1, __vector unsigned char a2)
{
  return (__vector unsigned short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_srl (__vector __bool short a1, __vector unsigned int a2)
{
  return (__vector __bool short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_srl (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector __bool short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_srl (__vector __bool short a1, __vector unsigned char a2)
{
  return (__vector __bool short) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_srl (__vector __pixel a1, __vector unsigned int a2)
{
  return (__vector __pixel) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_srl (__vector __pixel a1, __vector unsigned short a2)
{
  return (__vector __pixel) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_srl (__vector __pixel a1, __vector unsigned char a2)
{
  return (__vector __pixel) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_srl (__vector signed char a1, __vector unsigned int a2)
{
  return (__vector signed char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_srl (__vector signed char a1, __vector unsigned short a2)
{
  return (__vector signed char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_srl (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_srl (__vector unsigned char a1, __vector unsigned int a2)
{
  return (__vector unsigned char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_srl (__vector unsigned char a1, __vector unsigned short a2)
{
  return (__vector unsigned char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_srl (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_srl (__vector __bool char a1, __vector unsigned int a2)
{
  return (__vector __bool char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_srl (__vector __bool char a1, __vector unsigned short a2)
{
  return (__vector __bool char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_srl (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector __bool char) __builtin_altivec_vsr ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_sro */

inline __vector float
vec_sro (__vector float a1, __vector signed char a2)
{
  return (__vector float) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_sro (__vector float a1, __vector unsigned char a2)
{
  return (__vector float) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_sro (__vector signed int a1, __vector signed char a2)
{
  return (__vector signed int) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_sro (__vector signed int a1, __vector unsigned char a2)
{
  return (__vector signed int) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sro (__vector unsigned int a1, __vector signed char a2)
{
  return (__vector unsigned int) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sro (__vector unsigned int a1, __vector unsigned char a2)
{
  return (__vector unsigned int) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_sro (__vector signed short a1, __vector signed char a2)
{
  return (__vector signed short) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_sro (__vector signed short a1, __vector unsigned char a2)
{
  return (__vector signed short) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_sro (__vector unsigned short a1, __vector signed char a2)
{
  return (__vector unsigned short) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_sro (__vector unsigned short a1, __vector unsigned char a2)
{
  return (__vector unsigned short) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_sro (__vector __pixel a1, __vector signed char a2)
{
  return (__vector __pixel) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __pixel
vec_sro (__vector __pixel a1, __vector unsigned char a2)
{
  return (__vector __pixel) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_sro (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_sro (__vector signed char a1, __vector unsigned char a2)
{
  return (__vector signed char) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_sro (__vector unsigned char a1, __vector signed char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_sro (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsro ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_st */

inline void
vec_st (__vector float a1, int a2, __vector float *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector float a1, int a2, float *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector signed int a1, int a2, __vector signed int *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector signed int a1, int a2, int *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector unsigned int a1, int a2, __vector unsigned int *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector unsigned int a1, int a2, unsigned int *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool int a1, int a2, __vector __bool int *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool int a1, int a2, unsigned int *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool int a1, int a2, int *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector signed short a1, int a2, __vector signed short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector signed short a1, int a2, short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector unsigned short a1, int a2, __vector unsigned short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector unsigned short a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool short a1, int a2, __vector __bool short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool short a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __pixel a1, int a2, __vector __pixel *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __pixel a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __pixel a1, int a2, short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool short a1, int a2, short *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector signed char a1, int a2, __vector signed char *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector signed char a1, int a2, signed char *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector unsigned char a1, int a2, __vector unsigned char *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector unsigned char a1, int a2, unsigned char *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool char a1, int a2, __vector __bool char *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool char a1, int a2, unsigned char *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_st (__vector __bool char a1, int a2, signed char *a3)
{
  __builtin_altivec_stvx ((__vector signed int) a1, a2, (void *) a3);
}

/* vec_ste */

inline void
vec_ste (__vector signed char a1, int a2, signed char *a3)
{
  __builtin_altivec_stvebx ((__vector signed char) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector unsigned char a1, int a2, unsigned char *a3)
{
  __builtin_altivec_stvebx ((__vector signed char) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector __bool char a1, int a2, signed char *a3)
{
  __builtin_altivec_stvebx ((__vector signed char) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector __bool char a1, int a2, unsigned char *a3)
{
  __builtin_altivec_stvebx ((__vector signed char) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector signed short a1, int a2, short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector unsigned short a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector __bool short a1, int a2, short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector __bool short a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector __pixel a1, int a2, short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector __pixel a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector float a1, int a2, float *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector signed int a1, int a2, int *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector unsigned int a1, int a2, unsigned int *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector __bool int a1, int a2, int *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_ste (__vector __bool int a1, int a2, unsigned int *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

/* vec_stvewx */

inline void
vec_stvewx (__vector float a1, int a2, float *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stvewx (__vector signed int a1, int a2, int *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stvewx (__vector unsigned int a1, int a2, unsigned int *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stvewx (__vector __bool int a1, int a2, int *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stvewx (__vector __bool int a1, int a2, unsigned int *a3)
{
  __builtin_altivec_stvewx ((__vector signed int) a1, a2, (void *) a3);
}

/* vec_stvehx */

inline void
vec_stvehx (__vector signed short a1, int a2, short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_stvehx (__vector unsigned short a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_stvehx (__vector __bool short a1, int a2, short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_stvehx (__vector __bool short a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_stvehx (__vector __pixel a1, int a2, short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

inline void
vec_stvehx (__vector __pixel a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvehx ((__vector signed short) a1, a2, (void *) a3);
}

/* vec_stvebx */

inline void
vec_stvebx (__vector signed char a1, int a2, signed char *a3)
{
  __builtin_altivec_stvebx ((__vector signed char) a1, a2, (void *) a3);
}

inline void
vec_stvebx (__vector unsigned char a1, int a2, unsigned char *a3)
{
  __builtin_altivec_stvebx ((__vector signed char) a1, a2, (void *) a3);
}

inline void
vec_stvebx (__vector __bool char a1, int a2, signed char *a3)
{
  __builtin_altivec_stvebx ((__vector signed char) a1, a2, (void *) a3);
}

inline void
vec_stvebx (__vector __bool char a1, int a2, unsigned char *a3)
{
  __builtin_altivec_stvebx ((__vector signed char) a1, a2, (void *) a3);
}

/* vec_stl */

inline void
vec_stl (__vector float a1, int a2, __vector float *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector float a1, int a2, float *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector signed int a1, int a2, __vector signed int *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector signed int a1, int a2, int *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector unsigned int a1, int a2, __vector unsigned int *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector unsigned int a1, int a2, unsigned int *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool int a1, int a2, __vector __bool int *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool int a1, int a2, unsigned int *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool int a1, int a2, int *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector signed short a1, int a2, __vector signed short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector signed short a1, int a2, short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector unsigned short a1, int a2, __vector unsigned short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector unsigned short a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool short a1, int a2, __vector __bool short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool short a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool short a1, int a2, short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __pixel a1, int a2, __vector __pixel *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __pixel a1, int a2, unsigned short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __pixel a1, int a2, short *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector signed char a1, int a2, __vector signed char *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector signed char a1, int a2, signed char *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector unsigned char a1, int a2, __vector unsigned char *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector unsigned char a1, int a2, unsigned char *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool char a1, int a2, __vector __bool char *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool char a1, int a2, unsigned char *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

inline void
vec_stl (__vector __bool char a1, int a2, signed char *a3)
{
  __builtin_altivec_stvxl ((__vector signed int) a1, a2, (void *) a3);
}

/* vec_sub */

inline __vector signed char
vec_sub (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_sub (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_sub (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_sub (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_sub (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_sub (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed short
vec_sub (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_sub (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_sub (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_sub (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_sub (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_sub (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed int
vec_sub (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_sub (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_sub (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sub (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sub (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_sub (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_sub (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vsubfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vsubfp */

inline __vector float
vec_vsubfp (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vsubfp ((__vector float) a1, (__vector float) a2);
}

/* vec_vsubuwm */

inline __vector signed int
vec_vsubuwm (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vsubuwm (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vsubuwm (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vsubuwm (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vsubuwm (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vsubuwm (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vsubuhm */

inline __vector signed short
vec_vsubuhm (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vsubuhm (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vsubuhm (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vsubuhm (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vsubuhm (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vsubuhm (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vsububm */

inline __vector signed char
vec_vsububm (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vsububm (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vsububm (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vsububm (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vsububm (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vsububm (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_subc */

inline __vector unsigned int
vec_subc (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubcuw ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_subs */

inline __vector unsigned char
vec_subs (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_subs (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_subs (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_subs (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_subs (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_subs (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned short
vec_subs (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_subs (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_subs (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_subs (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_subs (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_subs (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned int
vec_subs (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_subs (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_subs (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_subs (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_subs (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_subs (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vsubsws */

inline __vector signed int
vec_vsubsws (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vsubsws (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_vsubsws (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vsubuws */

inline __vector unsigned int
vec_vsubuws (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vsubuws (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_vsubuws (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_vsubshs */

inline __vector signed short
vec_vsubshs (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vsubshs (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector signed short
vec_vsubshs (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vsubuhs */

inline __vector unsigned short
vec_vsubuhs (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vsubuhs (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) a1, (__vector signed short) a2);
}

inline __vector unsigned short
vec_vsubuhs (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) a1, (__vector signed short) a2);
}

/* vec_vsubsbs */

inline __vector signed char
vec_vsubsbs (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vsubsbs (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector signed char
vec_vsubsbs (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_vsububs */

inline __vector unsigned char
vec_vsububs (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vsububs (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) a1, (__vector signed char) a2);
}

inline __vector unsigned char
vec_vsububs (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) a1, (__vector signed char) a2);
}

/* vec_sum4s */

inline __vector unsigned int
vec_sum4s (__vector unsigned char a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsum4ubs ((__vector signed char) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_sum4s (__vector signed char a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsum4sbs ((__vector signed char) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_sum4s (__vector signed short a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsum4shs ((__vector signed short) a1, (__vector signed int) a2);
}

/* vec_vsum4shs */

inline __vector signed int
vec_vsum4shs (__vector signed short a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsum4shs ((__vector signed short) a1, (__vector signed int) a2);
}

/* vec_vsum4sbs */

inline __vector signed int
vec_vsum4sbs (__vector signed char a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsum4sbs ((__vector signed char) a1, (__vector signed int) a2);
}

/* vec_vsum4ubs */

inline __vector unsigned int
vec_vsum4ubs (__vector unsigned char a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vsum4ubs ((__vector signed char) a1, (__vector signed int) a2);
}

/* vec_sum2s */

inline __vector signed int
vec_sum2s (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsum2sws ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_sums */

inline __vector signed int
vec_sums (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vsumsws ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_trunc */

inline __vector float
vec_trunc (__vector float a1)
{
  return (__vector float) __builtin_altivec_vrfiz ((__vector float) a1);
}

/* vec_unpackh */

inline __vector signed short
vec_unpackh (__vector signed char a1)
{
  return (__vector signed short) __builtin_altivec_vupkhsb ((__vector signed char) a1);
}

inline __vector __bool short
vec_unpackh (__vector __bool char a1)
{
  return (__vector __bool short) __builtin_altivec_vupkhsb ((__vector signed char) a1);
}

inline __vector signed int
vec_unpackh (__vector signed short a1)
{
  return (__vector signed int) __builtin_altivec_vupkhsh ((__vector signed short) a1);
}

inline __vector __bool int
vec_unpackh (__vector __bool short a1)
{
  return (__vector __bool int) __builtin_altivec_vupkhsh ((__vector signed short) a1);
}

inline __vector unsigned int
vec_unpackh (__vector __pixel a1)
{
  return (__vector unsigned int) __builtin_altivec_vupkhpx ((__vector signed short) a1);
}

/* vec_vupkhsh */

inline __vector __bool int
vec_vupkhsh (__vector __bool short a1)
{
  return (__vector __bool int) __builtin_altivec_vupkhsh ((__vector signed short) a1);
}

inline __vector signed int
vec_vupkhsh (__vector signed short a1)
{
  return (__vector signed int) __builtin_altivec_vupkhsh ((__vector signed short) a1);
}

/* vec_vupkhpx */

inline __vector unsigned int
vec_vupkhpx (__vector __pixel a1)
{
  return (__vector unsigned int) __builtin_altivec_vupkhpx ((__vector signed short) a1);
}

/* vec_vupkhsb */

inline __vector __bool short
vec_vupkhsb (__vector __bool char a1)
{
  return (__vector __bool short) __builtin_altivec_vupkhsb ((__vector signed char) a1);
}

inline __vector signed short
vec_vupkhsb (__vector signed char a1)
{
  return (__vector signed short) __builtin_altivec_vupkhsb ((__vector signed char) a1);
}

/* vec_unpackl */

inline __vector signed short
vec_unpackl (__vector signed char a1)
{
  return (__vector signed short) __builtin_altivec_vupklsb ((__vector signed char) a1);
}

inline __vector __bool short
vec_unpackl (__vector __bool char a1)
{
  return (__vector __bool short) __builtin_altivec_vupklsb ((__vector signed char) a1);
}

inline __vector unsigned int
vec_unpackl (__vector __pixel a1)
{
  return (__vector unsigned int) __builtin_altivec_vupklpx ((__vector signed short) a1);
}

inline __vector signed int
vec_unpackl (__vector signed short a1)
{
  return (__vector signed int) __builtin_altivec_vupklsh ((__vector signed short) a1);
}

inline __vector __bool int
vec_unpackl (__vector __bool short a1)
{
  return (__vector __bool int) __builtin_altivec_vupklsh ((__vector signed short) a1);
}

/* vec_vupklpx */

inline __vector unsigned int
vec_vupklpx (__vector __pixel a1)
{
  return (__vector unsigned int) __builtin_altivec_vupklpx ((__vector signed short) a1);
}

/* vec_upklsh */

inline __vector __bool int
vec_vupklsh (__vector __bool short a1)
{
  return (__vector __bool int) __builtin_altivec_vupklsh ((__vector signed short) a1);
}

inline __vector signed int
vec_vupklsh (__vector signed short a1)
{
  return (__vector signed int) __builtin_altivec_vupklsh ((__vector signed short) a1);
}

/* vec_vupklsb */

inline __vector __bool short
vec_vupklsb (__vector __bool char a1)
{
  return (__vector __bool short) __builtin_altivec_vupklsb ((__vector signed char) a1);
}

inline __vector signed short
vec_vupklsb (__vector signed char a1)
{
  return (__vector signed short) __builtin_altivec_vupklsb ((__vector signed char) a1);
}

/* vec_xor */

inline __vector float
vec_xor (__vector float a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_xor (__vector float a1, __vector __bool int a2)
{
  return (__vector float) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector float
vec_xor (__vector __bool int a1, __vector float a2)
{
  return (__vector float) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool int
vec_xor (__vector __bool int a1, __vector __bool int a2)
{
  return (__vector __bool int) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_xor (__vector __bool int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_xor (__vector signed int a1, __vector __bool int a2)
{
  return (__vector signed int) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed int
vec_xor (__vector signed int a1, __vector signed int a2)
{
  return (__vector signed int) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_xor (__vector __bool int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_xor (__vector unsigned int a1, __vector __bool int a2)
{
  return (__vector unsigned int) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned int
vec_xor (__vector unsigned int a1, __vector unsigned int a2)
{
  return (__vector unsigned int) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool short
vec_xor (__vector __bool short a1, __vector __bool short a2)
{
  return (__vector __bool short) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_xor (__vector __bool short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_xor (__vector signed short a1, __vector __bool short a2)
{
  return (__vector signed short) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed short
vec_xor (__vector signed short a1, __vector signed short a2)
{
  return (__vector signed short) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_xor (__vector __bool short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_xor (__vector unsigned short a1, __vector __bool short a2)
{
  return (__vector unsigned short) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned short
vec_xor (__vector unsigned short a1, __vector unsigned short a2)
{
  return (__vector unsigned short) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_xor (__vector __bool char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector __bool char
vec_xor (__vector __bool char a1, __vector __bool char a2)
{
  return (__vector __bool char) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_xor (__vector signed char a1, __vector __bool char a2)
{
  return (__vector signed char) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector signed char
vec_xor (__vector signed char a1, __vector signed char a2)
{
  return (__vector signed char) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_xor (__vector __bool char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_xor (__vector unsigned char a1, __vector __bool char a2)
{
  return (__vector unsigned char) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

inline __vector unsigned char
vec_xor (__vector unsigned char a1, __vector unsigned char a2)
{
  return (__vector unsigned char) __builtin_altivec_vxor ((__vector signed int) a1, (__vector signed int) a2);
}

/* vec_all_eq */

inline int
vec_all_eq (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT, a1, (__vector signed char) a2);
}

inline int
vec_all_eq (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT, a1, a2);
}

inline int
vec_all_eq (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_eq (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_eq (__vector __bool char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_eq (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_eq (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_eq (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_eq (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_eq (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_eq (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_eq (__vector __bool short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_eq (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_eq (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_eq (__vector __pixel a1, __vector __pixel a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_eq (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_eq (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_eq (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_eq (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_eq (__vector __bool int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_eq (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_eq (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_eq (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpeqfp_p (__CR6_LT, a1, a2);
}

/* vec_all_ge */

inline int
vec_all_ge (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_ge (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_ge (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_ge (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_ge (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_ge (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_ge (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_ge (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_ge (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_ge (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_ge (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_ge (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_ge (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_ge (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_ge (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_ge (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_ge (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_ge (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_ge (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgefp_p (__CR6_LT, a1, a2);
}

/* vec_all_gt */

inline int
vec_all_gt (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_gt (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_gt (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_gt (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_gt (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_gt (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_gt (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_gt (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_gt (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_gt (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_gt (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_gt (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_gt (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_gt (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_gt (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_gt (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_gt (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_gt (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_gt (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgtfp_p (__CR6_LT, a1, a2);
}

/* vec_all_in */

inline int
vec_all_in (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpbfp_p (__CR6_EQ, a1, a2);
}

/* vec_all_le */

inline int
vec_all_le (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_le (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_le (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_le (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_le (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_le (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_le (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_le (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_le (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_le (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_le (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_le (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_le (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_le (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_le (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_le (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_le (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_le (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_le (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgefp_p (__CR6_LT, a2, a1);
}

/* vec_all_lt */

inline int
vec_all_lt (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_lt (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_lt (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_lt (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_lt (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_lt (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_all_lt (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_lt (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_lt (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_lt (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_lt (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_lt (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_all_lt (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_lt (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_lt (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_lt (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_lt (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_lt (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_all_lt (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgtfp_p (__CR6_LT, a2, a1);
}

/* vec_all_nan */

inline int
vec_all_nan (__vector float a1)
{
  return __builtin_altivec_vcmpeqfp_p (__CR6_EQ, a1, a1);
}

/* vec_all_ne */

inline int
vec_all_ne (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_ne (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_ne (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_ne (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_ne (__vector __bool char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_ne (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_ne (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_all_ne (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_ne (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_ne (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_ne (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_ne (__vector __bool short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_ne (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_ne (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_ne (__vector __pixel a1, __vector __pixel a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_all_ne (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_ne (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_ne (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_ne (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_ne (__vector __bool int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_ne (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_ne (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_all_ne (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpeqfp_p (__CR6_EQ, a1, a2);
}

/* vec_all_nge */

inline int
vec_all_nge (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgefp_p (__CR6_EQ, a1, a2);
}

/* vec_all_ngt */

inline int
vec_all_ngt (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgtfp_p (__CR6_EQ, a1, a2);
}

/* vec_all_nle */

inline int
vec_all_nle (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgefp_p (__CR6_EQ, a2, a1);
}

/* vec_all_nlt */

inline int
vec_all_nlt (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgtfp_p (__CR6_EQ, a2, a1);
}

/* vec_all_numeric */

inline int
vec_all_numeric (__vector float a1)
{
  return __builtin_altivec_vcmpeqfp_p (__CR6_LT, a1, a1);
}

/* vec_any_eq */

inline int
vec_any_eq (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_eq (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_eq (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_eq (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_eq (__vector __bool char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_eq (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_eq (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_eq (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_eq (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_eq (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_eq (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_eq (__vector __bool short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_eq (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_eq (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_eq (__vector __pixel a1, __vector __pixel a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_eq (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_eq (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_eq (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_eq (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_eq (__vector __bool int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_eq (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_eq (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_eq (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpeqfp_p (__CR6_EQ_REV, a1, a2);
}

/* vec_any_ge */

inline int
vec_any_ge (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_ge (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_ge (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_ge (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_ge (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_ge (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_ge (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_ge (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_ge (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_ge (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_ge (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_ge (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_ge (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_ge (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_ge (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_ge (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_ge (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_ge (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_ge (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgefp_p (__CR6_EQ_REV, a1, a2);
}

/* vec_any_gt */

inline int
vec_any_gt (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_gt (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_gt (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_gt (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_gt (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_gt (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_gt (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_gt (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_gt (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_gt (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_gt (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_gt (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_gt (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_gt (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_gt (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_gt (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_gt (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_gt (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_gt (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgtfp_p (__CR6_EQ_REV, a1, a2);
}

/* vec_any_le */

inline int
vec_any_le (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_le (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_le (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_le (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_le (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_le (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_le (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_le (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_le (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_le (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_le (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_le (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_le (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_le (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_le (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_le (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_le (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_le (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_le (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgefp_p (__CR6_EQ_REV, a2, a1);
}

/* vec_any_lt */

inline int
vec_any_lt (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_lt (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_lt (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_lt (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_lt (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_lt (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) a2, (__vector signed char) a1);
}

inline int
vec_any_lt (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_lt (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_lt (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_lt (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_lt (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_lt (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) a2, (__vector signed short) a1);
}

inline int
vec_any_lt (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_lt (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_lt (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_lt (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_lt (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_lt (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) a2, (__vector signed int) a1);
}

inline int
vec_any_lt (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgtfp_p (__CR6_EQ_REV, a2, a1);
}

/* vec_any_nan */

inline int
vec_any_nan (__vector float a1)
{
  return __builtin_altivec_vcmpeqfp_p (__CR6_LT_REV, a1, a1);
}

/* vec_any_ne */

inline int
vec_any_ne (__vector signed char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_ne (__vector signed char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_ne (__vector unsigned char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_ne (__vector unsigned char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_ne (__vector __bool char a1, __vector __bool char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_ne (__vector __bool char a1, __vector unsigned char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_ne (__vector __bool char a1, __vector signed char a2)
{
  return __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) a1, (__vector signed char) a2);
}

inline int
vec_any_ne (__vector signed short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_ne (__vector signed short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_ne (__vector unsigned short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_ne (__vector unsigned short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_ne (__vector __bool short a1, __vector __bool short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_ne (__vector __bool short a1, __vector unsigned short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_ne (__vector __bool short a1, __vector signed short a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_ne (__vector __pixel a1, __vector __pixel a2)
{
  return __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) a1, (__vector signed short) a2);
}

inline int
vec_any_ne (__vector signed int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_ne (__vector signed int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_ne (__vector unsigned int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_ne (__vector unsigned int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_ne (__vector __bool int a1, __vector __bool int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_ne (__vector __bool int a1, __vector unsigned int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_ne (__vector __bool int a1, __vector signed int a2)
{
  return __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) a1, (__vector signed int) a2);
}

inline int
vec_any_ne (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpeqfp_p (__CR6_LT_REV, a1, a2);
}

/* vec_any_nge */

inline int
vec_any_nge (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgefp_p (__CR6_LT_REV, a1, a2);
}

/* vec_any_ngt */

inline int
vec_any_ngt (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgtfp_p (__CR6_LT_REV, a1, a2);
}

/* vec_any_nle */

inline int
vec_any_nle (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgefp_p (__CR6_LT_REV, a2, a1);
}

/* vec_any_nlt */

inline int
vec_any_nlt (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpgtfp_p (__CR6_LT_REV, a2, a1);
}

/* vec_any_numeric */

inline int
vec_any_numeric (__vector float a1)
{
  return __builtin_altivec_vcmpeqfp_p (__CR6_EQ_REV, a1, a1);
}

/* vec_any_out */

inline int
vec_any_out (__vector float a1, __vector float a2)
{
  return __builtin_altivec_vcmpbfp_p (__CR6_EQ_REV, a1, a2);
}

} /* extern "C++" */

#else /* not C++ */

/* "... and so I think no man in a century will suffer as greatly as
   you will."  */

/* Helper macros.  */

#define __un_args_eq(xtype, x)						\
	__builtin_types_compatible_p (xtype, typeof (x))

#define __bin_args_eq(xtype, x, ytype, y)				\
	(__builtin_types_compatible_p (xtype, typeof (x))		\
	 && __builtin_types_compatible_p (ytype, typeof (y)))

#define __tern_args_eq(xtype, x, ytype, y, ztype, z)                    \
        (__builtin_types_compatible_p (xtype, typeof (x))               \
         && __builtin_types_compatible_p (ytype, typeof (y))		\
	 && __builtin_types_compatible_p (ztype, typeof (z)))

#define __ch(x, y, z)	__builtin_choose_expr (x, y, z)

#define vec_step(t) \
  __ch (__builtin_types_compatible_p (typeof (t), __vector signed int), 4,      \
  __ch (__builtin_types_compatible_p (typeof (t), __vector unsigned int), 4,    \
  __ch (__builtin_types_compatible_p (typeof (t), __vector __bool int), 4,        \
  __ch (__builtin_types_compatible_p (typeof (t), __vector signed short), 8,    \
  __ch (__builtin_types_compatible_p (typeof (t), __vector unsigned short), 8,  \
  __ch (__builtin_types_compatible_p (typeof (t), __vector __bool short), 8,      \
  __ch (__builtin_types_compatible_p (typeof (t), __vector __pixel), 8,           \
  __ch (__builtin_types_compatible_p (typeof (t), __vector signed char), 16,    \
  __ch (__builtin_types_compatible_p (typeof (t), __vector unsigned char), 16,  \
  __ch (__builtin_types_compatible_p (typeof (t), __vector __bool char), 16,      \
  __ch (__builtin_types_compatible_p (typeof (t), __vector float), 4,           \
  __builtin_altivec_compiletime_error ("vec_step"))))))))))))

#define vec_abs(a) \
  __ch (__un_args_eq (__vector signed char, (a)), \
        ((__vector signed char) __builtin_altivec_abs_v16qi ((__vector signed char) (a))), \
  __ch (__un_args_eq (__vector signed short, (a)), \
        ((__vector signed short) __builtin_altivec_abs_v8hi ((__vector signed short) (a))), \
  __ch (__un_args_eq (__vector signed int, (a)), \
        ((__vector signed int) __builtin_altivec_abs_v4si ((__vector signed int) (a))), \
  __ch (__un_args_eq (__vector float, (a)), \
        ((__vector float) __builtin_altivec_abs_v4sf ((__vector float) (a))), \
  __builtin_altivec_compiletime_error ("vec_abs")))))

#define vec_abss(a) \
  __ch (__un_args_eq (__vector signed char, (a)), \
        ((__vector signed char) __builtin_altivec_abss_v16qi ((__vector signed char) (a))), \
  __ch (__un_args_eq (__vector signed short, (a)), \
        ((__vector signed short) __builtin_altivec_abss_v8hi ((__vector signed short) (a))), \
  __ch (__un_args_eq (__vector signed int, (a)), \
        ((__vector signed int) __builtin_altivec_abss_v4si ((__vector signed int) (a))), \
  __builtin_altivec_compiletime_error ("vec_abss"))))

#define vec_vaddubm(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vaddubm")))))))

#define vec_vadduhm(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vadduhm")))))))

#define vec_vadduwm(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vadduwm")))))))

#define vec_vaddfp(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vaddfp ((__vector float) (a1), (__vector float) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vaddfp"))

#define vec_add(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vaddfp ((__vector float) (a1), (__vector float) (a2))), \
    __builtin_altivec_compiletime_error ("vec_add"))))))))))))))))))))

#define vec_addc(a1, a2) \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vaddcuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_addc"))

#define vec_adds(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_adds")))))))))))))))))))

#define vec_vaddsws(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vaddsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vaddsws"))))

#define vec_vadduws(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vadduws ((__vector signed int) (a1), (__vector signed int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vadduws"))))

#define vec_vaddshs(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vaddshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vaddshs"))))

#define vec_vadduhs(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vadduhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vadduhs"))))

#define vec_vaddsbs(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vaddsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vaddsbs"))))

#define vec_vaddubs(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vaddubs ((__vector signed char) (a1), (__vector signed char) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vaddubs"))))

#define vec_and(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector __bool int, (a2)), \
      ((__vector float) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vand ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_and")))))))))))))))))))))))))

#define vec_andc(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector __bool int, (a2)), \
      ((__vector float) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vandc ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_andc")))))))))))))))))))))))))

#define vec_avg(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vavgub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vavgsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vavguh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vavgsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vavguw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vavgsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_avg")))))))

#define vec_vavgsw(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vavgsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vavgsw"))

#define vec_vavguw(a1, a2) \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vavguw ((__vector signed int) (a1), (__vector signed int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vavguw"))

#define vec_vavgsh(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vavgsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vavgsh"))

#define vec_vavguh(a1, a2) \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vavguh ((__vector signed short) (a1), (__vector signed short) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vavguh"))

#define vec_vavgsb(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vavgsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vavgsb"))

#define vec_vavgub(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vavgub ((__vector signed char) (a1), (__vector signed char) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vavgub"))

#define vec_ceil(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vrfip ((__vector float) (a1))), \
  __builtin_altivec_compiletime_error ("vec_ceil"))

#define vec_cmpb(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector signed int) __builtin_altivec_vcmpbfp ((__vector float) (a1), (__vector float) (a2))), \
  __builtin_altivec_compiletime_error ("vec_cmpb"))

#define vec_cmpeq(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpequb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpequb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpequh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpequh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpequw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpequw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpeqfp ((__vector float) (a1), (__vector float) (a2))), \
    __builtin_altivec_compiletime_error ("vec_cmpeq"))))))))

#define vec_vcmpeqfp(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpeqfp ((__vector float) (a1), (__vector float) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpeqfp"))

#define vec_vcmpequw(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpequw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpequw ((__vector signed int) (a1), (__vector signed int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpequw")))

#define vec_vcmpequh(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpequh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpequh ((__vector signed short) (a1), (__vector signed short) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpequh")))

#define vec_vcmpequb(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpequb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpequb ((__vector signed char) (a1), (__vector signed char) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpequb")))

#define vec_cmpge(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgefp ((__vector float) (a1), (__vector float) (a2))), \
  __builtin_altivec_compiletime_error ("vec_cmpge"))

#define vec_cmpgt(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpgtub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpgtsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpgtuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpgtsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtfp ((__vector float) (a1), (__vector float) (a2))), \
    __builtin_altivec_compiletime_error ("vec_cmpgt"))))))))

#define vec_vcmpgtfp(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtfp ((__vector float) (a1), (__vector float) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpgtfp"))

#define vec_vcmpgtsw(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpgtsw"))

#define vec_vcmpgtuw(a1, a2) \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpgtuw"))

#define vec_vcmpgtsh(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpgtsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpgtsh"))

#define vec_vcmpgtuh(a1, a2) \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpgtuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpgtuh"))

#define vec_vcmpgtsb(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpgtsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpgtsb"))

#define vec_vcmpgtub(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpgtub ((__vector signed char) (a1), (__vector signed char) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcmpgtub"))

#define vec_cmple(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgefp ((__vector float) (a2), (__vector float) (a1))), \
  __builtin_altivec_compiletime_error ("vec_cmple"))

#define vec_cmplt(a2, a1) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpgtub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vcmpgtsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpgtuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vcmpgtsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector __bool int) __builtin_altivec_vcmpgtfp ((__vector float) (a1), (__vector float) (a2))), \
    __builtin_altivec_compiletime_error ("vec_cmplt"))))))))

#define vec_ctf(a1, a2) \
__ch (__un_args_eq (__vector unsigned int, (a1)), \
      ((__vector float) __builtin_altivec_vcfux ((__vector signed int) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector signed int, (a1)), \
      ((__vector float) __builtin_altivec_vcfsx ((__vector signed int) (a1), (const int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_ctf")))

#define vec_vcfsx(a1, a2) \
__ch (__un_args_eq (__vector signed int, (a1)), \
      ((__vector float) __builtin_altivec_vcfsx ((__vector signed int) (a1), (const int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcfsx"))

#define vec_vcfux(a1, a2) \
__ch (__un_args_eq (__vector unsigned int, (a1)), \
      ((__vector float) __builtin_altivec_vcfux ((__vector signed int) (a1), (const int) (a2))), \
  __builtin_altivec_compiletime_error ("vec_vcfux"))

#define vec_cts(a1, a2) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector signed int) __builtin_altivec_vctsxs ((__vector float) (a1), (const int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_cts"))

#define vec_ctu(a1, a2) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector unsigned int) __builtin_altivec_vctuxs ((__vector float) (a1), (const int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_ctu"))

#define vec_dss(a1) __builtin_altivec_dss ((const int) (a1));

#define vec_dssall() __builtin_altivec_dssall ()

#define vec_dst(a1, a2, a3) \
__ch (__un_args_eq (const __vector unsigned char, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed char, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool char, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector unsigned short, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed short, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool short, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __pixel, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector unsigned int, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed int, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool int, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector float, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned char, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const signed char, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned short, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const short, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned int, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const int, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned long, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const long, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const float, *(a1)), \
      __builtin_altivec_dst ((void *) (a1), (a2), (a3)), \
  __builtin_altivec_compiletime_error ("vec_dst")))))))))))))))))))))

#define vec_dstst(a1, a2, a3) \
__ch (__un_args_eq (const __vector unsigned char, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed char, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool char, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector unsigned short, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed short, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool short, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __pixel, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector unsigned int, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed int, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool int, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector float, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned char, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const signed char, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned short, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const short, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned int, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const int, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned long, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const long, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const float, *(a1)), \
      __builtin_altivec_dstst ((void *) (a1), (a2), (a3)), \
  __builtin_altivec_compiletime_error ("vec_dstst")))))))))))))))))))))

#define vec_dststt(a1, a2, a3) \
__ch (__un_args_eq (const __vector unsigned char, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed char, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool char, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector unsigned short, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed short, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool short, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __pixel, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector unsigned int, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed int, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool int, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector float, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned char, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const signed char, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned short, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const short, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned int, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const int, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned long, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const long, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const float, *(a1)), \
      __builtin_altivec_dststt ((void *) (a1), (a2), (a3)), \
  __builtin_altivec_compiletime_error ("vec_dststt")))))))))))))))))))))

#define vec_dstt(a1, a2, a3) \
__ch (__un_args_eq (const __vector unsigned char, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed char, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool char, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector unsigned short, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed short, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool short, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __pixel, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector unsigned int, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector signed int, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector __bool int, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const __vector float, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned char, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const signed char, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned short, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const short, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned int, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const int, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const unsigned long, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const long, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
__ch (__un_args_eq (const float, *(a1)), \
      __builtin_altivec_dstt ((void *) (a1), (a2), (a3)), \
  __builtin_altivec_compiletime_error ("vec_dstt")))))))))))))))))))))

#define vec_expte(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vexptefp ((__vector float) (a1))), \
  __builtin_altivec_compiletime_error ("vec_expte"))

#define vec_floor(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vrfim ((__vector float) (a1))), \
  __builtin_altivec_compiletime_error ("vec_floor"))

#define vec_ld(a, b) \
__ch (__un_args_eq (const __vector unsigned char, *(b)), \
      ((__vector unsigned char) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const unsigned char, *(b)), \
      ((__vector unsigned char) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector signed char, *(b)), \
      ((__vector signed char) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const signed char, *(b)), \
      ((__vector signed char) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector __bool char, *(b)), \
      ((__vector __bool char) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector unsigned short, *(b)), \
      ((__vector unsigned short) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const unsigned short, *(b)), \
      ((__vector unsigned short) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector signed short, *(b)), \
      ((__vector signed short) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const short, *(b)), \
      ((__vector signed short) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector __bool short, *(b)), \
      ((__vector __bool short) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector __pixel, *(b)), \
      ((__vector __pixel) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector unsigned int, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const unsigned int, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const unsigned long, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector signed int, *(b)), \
      ((__vector signed int) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const int, *(b)), \
      ((__vector signed int) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const long, *(b)), \
      ((__vector signed int) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector __bool int, *(b)), \
      ((__vector __bool int) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const __vector float, *(b)), \
      ((__vector float) __builtin_altivec_lvx ((a), (b))), \
__ch (__un_args_eq (const float, *(b)), \
      ((__vector float) __builtin_altivec_lvx ((a), (b))), \
__builtin_altivec_compiletime_error ("vec_ld")))))))))))))))))))))

#define vec_lde(a, b) \
__ch (__un_args_eq (const unsigned char, *(b)), \
      ((__vector unsigned char) __builtin_altivec_lvebx ((a), (b))), \
__ch (__un_args_eq (const signed char, *(b)), \
      ((__vector signed char) __builtin_altivec_lvebx ((a), (b))), \
__ch (__un_args_eq (const unsigned short, *(b)), \
      ((__vector unsigned short) __builtin_altivec_lvehx ((a), (b))), \
__ch (__un_args_eq (const short, *(b)), \
      ((__vector signed short) __builtin_altivec_lvehx ((a), (b))), \
__ch (__un_args_eq (const unsigned long, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvewx ((a), (b))), \
__ch (__un_args_eq (const long, *(b)), \
      ((__vector signed int) __builtin_altivec_lvewx ((a), (b))), \
__ch (__un_args_eq (const unsigned int, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvewx ((a), (b))), \
__ch (__un_args_eq (const int, *(b)), \
      ((__vector signed int) __builtin_altivec_lvewx ((a), (b))), \
__ch (__un_args_eq (const float, *(b)), \
      ((__vector float) __builtin_altivec_lvewx ((a), (b))), \
__builtin_altivec_compiletime_error ("vec_lde"))))))))))

#define vec_lvewx(a, b) \
__ch (__un_args_eq (unsigned int, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvewx ((a), (b))), \
__ch (__un_args_eq (signed int, *(b)), \
      ((__vector signed int) __builtin_altivec_lvewx ((a), (b))), \
__ch (__un_args_eq (unsigned long, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvewx ((a), (b))), \
__ch (__un_args_eq (signed long, *(b)), \
      ((__vector signed int) __builtin_altivec_lvewx ((a), (b))), \
__ch (__un_args_eq (float, *(b)), \
      ((__vector float) __builtin_altivec_lvewx ((a), (b))), \
__builtin_altivec_compiletime_error ("vec_lvewx"))))))

#define vec_lvehx(a, b) \
__ch (__un_args_eq (unsigned short, *(b)), \
      ((__vector unsigned short) __builtin_altivec_lvehx ((a), (b))), \
__ch (__un_args_eq (signed short, *(b)), \
      ((__vector signed short) __builtin_altivec_lvehx ((a), (b))), \
__builtin_altivec_compiletime_error ("vec_lvehx")))

#define vec_lvebx(a, b) \
__ch (__un_args_eq (unsigned char, *(b)), \
      ((__vector unsigned char) __builtin_altivec_lvebx ((a), (b))), \
__ch (__un_args_eq (signed char, *(b)), \
      ((__vector signed char) __builtin_altivec_lvebx ((a), (b))), \
__builtin_altivec_compiletime_error ("vec_lvebx")))

#define vec_ldl(a, b) \
__ch (__un_args_eq (const __vector unsigned char, *(b)), \
      ((__vector unsigned char) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const unsigned char, *(b)), \
      ((__vector unsigned char) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector signed char, *(b)), \
      ((__vector signed char) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const signed char, *(b)), \
      ((__vector signed char) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector __bool char, *(b)), \
      ((__vector __bool char) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector unsigned short, *(b)), \
      ((__vector unsigned short) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const unsigned short, *(b)), \
      ((__vector unsigned short) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector signed short, *(b)), \
      ((__vector signed short) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const short, *(b)), \
      ((__vector signed short) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector __bool short, *(b)), \
      ((__vector __bool short) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector __pixel, *(b)), \
      ((__vector __pixel) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector unsigned int, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const unsigned int, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const unsigned long, *(b)), \
      ((__vector unsigned int) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector signed int, *(b)), \
      ((__vector signed int) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const int, *(b)), \
      ((__vector signed int) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const long, *(b)), \
      ((__vector signed int) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector __bool int, *(b)), \
      ((__vector __bool int) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const __vector float, *(b)), \
      ((__vector float) __builtin_altivec_lvxl ((a), (b))), \
__ch (__un_args_eq (const float, *(b)), \
      ((__vector float) __builtin_altivec_lvxl ((a), (b))), \
__builtin_altivec_compiletime_error ("vec_ldl")))))))))))))))))))))

#define vec_loge(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vlogefp ((__vector float) (a1))), \
  __builtin_altivec_compiletime_error ("vec_loge"))

#define vec_lvsl(a1, a2) \
__ch (__un_args_eq (const volatile unsigned char, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile signed char, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile unsigned short, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile signed short, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile unsigned int, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile signed int, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile unsigned long, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile signed long, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile float, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsl ((a1), (void *) (a2))), \
__builtin_altivec_compiletime_error ("vec_lvsl"))))))))))

#define vec_lvsr(a1, a2) \
__ch (__un_args_eq (const volatile unsigned char, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile signed char, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile unsigned short, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile signed short, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile unsigned int, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile signed int, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile unsigned long, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile signed long, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__ch (__un_args_eq (const volatile float, *(a2)), \
      ((__vector unsigned char) __builtin_altivec_lvsr ((a1), (void *) (a2))), \
__builtin_altivec_compiletime_error ("vec_lvsr"))))))))))

#define vec_madd(a1, a2, a3) \
__ch (__tern_args_eq (__vector float, (a1), __vector float, (a2), __vector float, (a3)), \
      ((__vector float) __builtin_altivec_vmaddfp ((a1), (a2), (a3))), \
__builtin_altivec_compiletime_error ("vec_madd"))

#define vec_madds(a1, a2, a3) \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector signed short, (a3)), \
      ((__vector signed short) __builtin_altivec_vmhaddshs ((a1), (a2), (a3))), \
__builtin_altivec_compiletime_error ("vec_madds"))

#define vec_max(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vmaxfp ((__vector float) (a1), (__vector float) (a2))), \
    __builtin_altivec_compiletime_error ("vec_max"))))))))))))))))))))

#define vec_vmaxfp(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vmaxfp ((__vector float) (a1), (__vector float) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmaxfp"))

#define vec_vmaxsw(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmaxsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmaxsw"))))

#define vec_vmaxuw(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmaxuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmaxuw"))))

#define vec_vmaxsh(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmaxsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmaxsh"))))

#define vec_vmaxuh(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmaxuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmaxuh"))))

#define vec_vmaxsb(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmaxsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmaxsb"))))

#define vec_vmaxub(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmaxub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmaxub"))))

#define vec_mergeh(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmrghb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmrghb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vmrghb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmrghh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmrghh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vmrghh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      ((__vector __pixel) __builtin_altivec_vmrghh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vmrghw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmrghw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmrghw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vmrghw ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_mergeh"))))))))))))

#define vec_vmrghw(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vmrghw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vmrghw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmrghw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmrghw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmrghw")))))

#define vec_vmrghh(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vmrghh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmrghh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmrghh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      ((__vector __pixel) __builtin_altivec_vmrghh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmrghh")))))

#define vec_vmrghb(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vmrghb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmrghb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmrghb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmrghb"))))

#define vec_mergel(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmrglb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmrglb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vmrglb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmrglh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmrglh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vmrglh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      ((__vector __pixel) __builtin_altivec_vmrglh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vmrglw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmrglw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmrglw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vmrglw ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_mergel"))))))))))))

#define vec_vmrglw(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vmrglw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vmrglw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmrglw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vmrglw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmrglw")))))

#define vec_vmrglh(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vmrglh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vmrglh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmrglh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      ((__vector __pixel) __builtin_altivec_vmrglh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmrglh")))))

#define vec_vmrglb(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vmrglb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vmrglb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vmrglb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmrglb"))))

#define vec_mfvscr()  (((__vector unsigned short) __builtin_altivec_mfvscr ()))

#define vec_min(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vminsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vminsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vminsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vminsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vminsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vminsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vminsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vminsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vminsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vminfp ((__vector float) (a1), (__vector float) (a2))), \
    __builtin_altivec_compiletime_error ("vec_min"))))))))))))))))))))

#define vec_vminfp(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vminfp ((__vector float) (a1), (__vector float) (a2))), \
__builtin_altivec_compiletime_error ("vec_vminfp"))

#define vec_vminsw(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vminsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vminsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vminsw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vminsw"))))

#define vec_vminuw(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vminuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vminuw"))))

#define vec_vminsh(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vminsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vminsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vminsh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vminsh"))))

#define vec_vminuh(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vminuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vminuh"))))

#define vec_vminsb(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vminsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vminsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vminsb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_minsb"))))

#define vec_vminub(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vminub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vminub"))))

#define vec_mladd(a1, a2, a3) \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector signed short, (a3)), \
      ((__vector signed short) __builtin_altivec_vmladduhm ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed short) (a3))), \
__ch (__tern_args_eq (__vector signed short, (a1), __vector unsigned short, (a2), __vector unsigned short, (a3)), \
      ((__vector signed short) __builtin_altivec_vmladduhm ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed short) (a3))), \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector signed short, (a2), __vector signed short, (a3)), \
      ((__vector signed short) __builtin_altivec_vmladduhm ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed short) (a3))), \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2), __vector unsigned short, (a3)), \
      ((__vector unsigned short) __builtin_altivec_vmladduhm ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed short) (a3))), \
    __builtin_altivec_compiletime_error ("vec_mladd")))))

#define vec_mradds(a1, a2, a3) \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector signed short, (a3)), \
      ((__vector signed short) __builtin_altivec_vmhraddshs ((a1), (a2), (a3))), \
__builtin_altivec_compiletime_error ("vec_mradds"))

#define vec_msum(a1, a2, a3) \
__ch (__tern_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2), __vector unsigned int, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vmsumubm ((__vector signed char) (a1), (__vector signed char) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed char, (a1), __vector unsigned char, (a2), __vector signed int, (a3)), \
      ((__vector signed int) __builtin_altivec_vmsummbm ((__vector signed char) (a1), (__vector signed char) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2), __vector unsigned int, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vmsumuhm ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector signed int, (a3)), \
      ((__vector signed int) __builtin_altivec_vmsumshm ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed int) (a3))), \
    __builtin_altivec_compiletime_error ("vec_msum")))))

#define vec_vmsumshm(a1, a2, a3) \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector signed int, (a3)), \
      ((__vector signed int) __builtin_altivec_vmsumshm ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed int) (a3))), \
__builtin_altivec_compiletime_error ("vec_vmsumshm"))

#define vec_vmsumuhm(a1, a2, a3) \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2), __vector unsigned int, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vmsumuhm ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed int) (a3))), \
__builtin_altivec_compiletime_error ("vec_vmsumuhm"))

#define vec_vmsummbm(a1, a2, a3) \
__ch (__tern_args_eq (__vector signed char, (a1), __vector unsigned char, (a2), __vector signed int, (a3)), \
      ((__vector signed int) __builtin_altivec_vmsummbm ((__vector signed char) (a1), (__vector signed char) (a2), (__vector signed int) (a3))), \
__builtin_altivec_compiletime_error ("vec_vmsummbm"))

#define vec_vmsumubm(a1, a2, a3) \
__ch (__tern_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2), __vector unsigned int, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vmsumubm ((__vector signed char) (a1), (__vector signed char) (a2), (__vector signed int) (a3))), \
__builtin_altivec_compiletime_error ("vec_vmsummbm"))

#define vec_msums(a1, a2, a3) \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2), __vector unsigned int, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vmsumuhs ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector signed int, (a3)), \
      ((__vector signed int) __builtin_altivec_vmsumshs ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed int) (a3))), \
    __builtin_altivec_compiletime_error ("vec_msums")))

#define vec_vmsumshs(a1, a2, a3) \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector signed int, (a3)), \
      ((__vector signed int) __builtin_altivec_vmsumshs ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed int) (a3))), \
__builtin_altivec_compiletime_error ("vec_vmsumshs"))

#define vec_vmsumuhs(a1, a2, a3) \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2), __vector unsigned int, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vmsumuhs ((__vector signed short) (a1), (__vector signed short) (a2), (__vector signed int) (a3))), \
__builtin_altivec_compiletime_error ("vec_vmsumuhs"))

#define vec_mtvscr(a1) \
__ch (__un_args_eq (__vector signed int, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector unsigned int, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector __bool int, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector signed short, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector unsigned short, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector __bool short, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector __pixel, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector signed char, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector unsigned char, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
__ch (__un_args_eq (__vector __bool char, (a1)), \
      __builtin_altivec_mtvscr ((__vector signed int) (a1)), \
    __builtin_altivec_compiletime_error ("vec_mtvscr")))))))))))

#define vec_mule(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmuleub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed short) __builtin_altivec_vmulesb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmuleuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed int) __builtin_altivec_vmulesh ((__vector signed short) (a1), (__vector signed short) (a2))), \
    __builtin_altivec_compiletime_error ("vec_mule")))))

#define vec_vmulesh(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed int) __builtin_altivec_vmulesh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmulesh"))

#define vec_vmuleuh(a1, a2) \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmuleuh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmuleuh"))

#define vec_vmulesb(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed short) __builtin_altivec_vmulesb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmulesb"))

#define vec_vmuleub(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmuleub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmuleub"))

#define vec_mulo(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmuloub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed short) __builtin_altivec_vmulosb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmulouh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed int) __builtin_altivec_vmulosh ((__vector signed short) (a1), (__vector signed short) (a2))), \
    __builtin_altivec_compiletime_error ("vec_mulo")))))

#define vec_vmulosh(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed int) __builtin_altivec_vmulosh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmulosh"))

#define vec_vmulouh(a1, a2) \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vmulouh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmulouh"))

#define vec_vmulosb(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed short) __builtin_altivec_vmulosb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmulosb"))

#define vec_vmuloub(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vmuloub ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vmuloub"))

#define vec_nmsub(a1, a2, a3) \
__ch (__tern_args_eq (__vector float, (a1), __vector float, (a2), __vector float, (a3)), \
      ((__vector float) __builtin_altivec_vnmsubfp ((__vector float) (a1), (__vector float) (a2), (__vector float) (a3))), \
    __builtin_altivec_compiletime_error ("vec_nmsub"))

#define vec_nor(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vnor ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_nor")))))))))))

#define vec_or(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector __bool int, (a2)), \
      ((__vector float) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vor ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_or")))))))))))))))))))))))))

#define vec_pack(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed char) __builtin_altivec_vpkuhum ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vpkuhum ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool char) __builtin_altivec_vpkuhum ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed short) __builtin_altivec_vpkuwum ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vpkuwum ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool short) __builtin_altivec_vpkuwum ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_pack")))))))

#define vec_vpkuwum(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool short) __builtin_altivec_vpkuwum ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed short) __builtin_altivec_vpkuwum ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vpkuwum ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vpkuwum"))))

#define vec_vpkuhum(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool char) __builtin_altivec_vpkuhum ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed char) __builtin_altivec_vpkuhum ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vpkuhum ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vpkuhum"))))

#define vec_packpx(a1, a2) \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
  ((__vector __pixel) __builtin_altivec_vpkpx ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_packpx"))

#define vec_packs(a1, a2) \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vpkuhus ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed char) __builtin_altivec_vpkshss ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vpkuwus ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed short) __builtin_altivec_vpkswss ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_packs")))))

#define vec_vpkswss(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed short) __builtin_altivec_vpkswss ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vpkswss"))

#define vec_vpkuwus(a1, a2) \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vpkuwus ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vpkuwus"))

#define vec_vpkshss(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed char) __builtin_altivec_vpkshss ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vpkshss"))

#define vec_vpkuhus(a1, a2) \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vpkuhus ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vpkuhus"))

#define vec_packsu(a1, a2) \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vpkuhus ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vpkshus ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vpkuwus ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vpkswus ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_packsu")))))

#define vec_vpkswus(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vpkswus ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vpkswus"))

#define vec_vpkshus(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vpkshus ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vpkshus"))

#define vec_perm(a1, a2, a3) \
__ch (__tern_args_eq (__vector float, (a1), __vector float, (a2), __vector unsigned char, (a3)), \
      ((__vector float) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector signed int, (a1), __vector signed int, (a2), __vector unsigned char, (a3)), \
      ((__vector signed int) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2), __vector unsigned char, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector __bool int, (a1), __vector __bool int, (a2), __vector unsigned char, (a3)), \
      ((__vector __bool int) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector unsigned char, (a3)), \
      ((__vector signed short) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2), __vector unsigned char, (a3)), \
      ((__vector unsigned short) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector __bool short, (a1), __vector __bool short, (a2), __vector unsigned char, (a3)), \
      ((__vector __bool short) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector __pixel, (a1), __vector __pixel, (a2), __vector unsigned char, (a3)), \
      ((__vector __pixel) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector signed char, (a1), __vector signed char, (a2), __vector unsigned char, (a3)), \
      ((__vector signed char) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2), __vector unsigned char, (a3)), \
      ((__vector unsigned char) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
__ch (__tern_args_eq (__vector __bool char, (a1), __vector __bool char, (a2), __vector unsigned char, (a3)), \
      ((__vector __bool char) __builtin_altivec_vperm_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed char) (a3))), \
    __builtin_altivec_compiletime_error ("vec_perm"))))))))))))

#define vec_re(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vrefp ((__vector float) (a1))), \
__builtin_altivec_compiletime_error ("vec_re"))

#define vec_rl(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vrlb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vrlb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vrlh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vrlh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vrlw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vrlw ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_rl")))))))

#define vec_vrlw(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vrlw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vrlw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vrlw")))

#define vec_vrlh(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vrlh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vrlh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vrlh")))

#define vec_vrlb(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vrlb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vrlb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vrlb")))

#define vec_round(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vrfin ((__vector float) (a1))), \
__builtin_altivec_compiletime_error ("vec_round"))

#define vec_rsqrte(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vrsqrtefp ((__vector float) (a1))), \
__builtin_altivec_compiletime_error ("vec_rsqrte"))

#define vec_sel(a1, a2, a3) \
__ch (__tern_args_eq (__vector float, (a1), __vector float, (a2), __vector __bool int, (a3)), \
      ((__vector float) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector float, (a1), __vector float, (a2), __vector unsigned int, (a3)), \
      ((__vector float) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector __bool int, (a1), __vector __bool int, (a2), __vector __bool int, (a3)), \
      ((__vector __bool int) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector __bool int, (a1), __vector __bool int, (a2), __vector unsigned int, (a3)), \
      ((__vector __bool int) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed int, (a1), __vector signed int, (a2), __vector __bool int, (a3)), \
      ((__vector signed int) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed int, (a1), __vector signed int, (a2), __vector unsigned int, (a3)), \
      ((__vector signed int) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2), __vector __bool int, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2), __vector unsigned int, (a3)), \
      ((__vector unsigned int) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector __bool short, (a1), __vector __bool short, (a2), __vector __bool short, (a3)), \
      ((__vector __bool short) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector __bool short, (a1), __vector __bool short, (a2), __vector unsigned short, (a3)), \
      ((__vector __bool short) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector __bool short, (a3)), \
      ((__vector signed short) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed short, (a1), __vector signed short, (a2), __vector unsigned short, (a3)), \
      ((__vector signed short) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2), __vector __bool short, (a3)), \
      ((__vector unsigned short) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2), __vector unsigned short, (a3)), \
      ((__vector unsigned short) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector __bool char, (a1), __vector __bool char, (a2), __vector __bool char, (a3)), \
      ((__vector __bool char) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector __bool char, (a1), __vector __bool char, (a2), __vector unsigned char, (a3)), \
      ((__vector __bool char) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed char, (a1), __vector signed char, (a2), __vector __bool char, (a3)), \
      ((__vector signed char) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector signed char, (a1), __vector signed char, (a2), __vector unsigned char, (a3)), \
      ((__vector signed char) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2), __vector __bool char, (a3)), \
      ((__vector unsigned char) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
__ch (__tern_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2), __vector unsigned char, (a3)), \
      ((__vector unsigned char) __builtin_altivec_vsel_4si ((__vector signed int) (a1), (__vector signed int) (a2), (__vector signed int) (a3))), \
    __builtin_altivec_compiletime_error ("vec_sel")))))))))))))))))))))

#define vec_sl(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vslb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vslb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vslh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vslh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vslw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vslw ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_sl")))))))

#define vec_vslw(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vslw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vslw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vslw")))

#define vec_vslh(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vslh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vslh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vslh")))

#define vec_vslb(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vslb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vslb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vslb")))

#define vec_sld(a1, a2, a3) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vsldoi_4si ((__vector signed int) (a1), (__vector signed int) (a2), (const int) (a3))), \
    __builtin_altivec_compiletime_error ("vec_sld"))))))))))))

#define vec_sll(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned short, (a2)), \
      ((__vector signed int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned char, (a2)), \
      ((__vector signed int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool int) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned int, (a2)), \
      ((__vector signed short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned char, (a2)), \
      ((__vector signed short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool short) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector unsigned int, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector unsigned short, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector unsigned char, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned int, (a2)), \
      ((__vector signed char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned short, (a2)), \
      ((__vector signed char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vsl ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_sll")))))))))))))))))))))))))))))))

#define vec_slo(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector signed char, (a2)), \
      ((__vector float) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector unsigned char, (a2)), \
      ((__vector float) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed char, (a2)), \
      ((__vector signed int) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned char, (a2)), \
      ((__vector signed int) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector signed char, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed char, (a2)), \
      ((__vector signed short) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned char, (a2)), \
      ((__vector signed short) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector signed char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector signed char, (a2)), \
      ((__vector __pixel) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector unsigned char, (a2)), \
      ((__vector __pixel) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector signed char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vslo ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_slo")))))))))))))))))

#define vec_splat(a1, a2) \
__ch (__un_args_eq (__vector signed char, (a1)), \
      ((__vector signed char) __builtin_altivec_vspltb ((__vector signed char) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector unsigned char, (a1)), \
      ((__vector unsigned char) __builtin_altivec_vspltb ((__vector signed char) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector __bool char, (a1)), \
      ((__vector __bool char) __builtin_altivec_vspltb ((__vector signed char) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector signed short, (a1)), \
      ((__vector signed short) __builtin_altivec_vsplth ((__vector signed short) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector unsigned short, (a1)), \
      ((__vector unsigned short) __builtin_altivec_vsplth ((__vector signed short) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector __bool short, (a1)), \
      ((__vector __bool short) __builtin_altivec_vsplth ((__vector signed short) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector __pixel, (a1)), \
      ((__vector __pixel) __builtin_altivec_vsplth ((__vector signed short) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vspltw ((__vector signed int) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector signed int, (a1)), \
      ((__vector signed int) __builtin_altivec_vspltw ((__vector signed int) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector unsigned int, (a1)), \
      ((__vector unsigned int) __builtin_altivec_vspltw ((__vector signed int) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector __bool int, (a1)), \
      ((__vector __bool int) __builtin_altivec_vspltw ((__vector signed int) (a1), (const int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_splat"))))))))))))

#define vec_vspltw(a1, a2) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vspltw ((__vector signed int) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector __bool int, (a1)), \
      ((__vector __bool int) __builtin_altivec_vspltw ((__vector signed int) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector signed int, (a1)), \
      ((__vector signed int) __builtin_altivec_vspltw ((__vector signed int) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector unsigned int, (a1)), \
      ((__vector unsigned int) __builtin_altivec_vspltw ((__vector signed int) (a1), (const int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vspltw")))))

#define vec_vsplth(a1, a2) \
__ch (__un_args_eq (__vector __bool short, (a1)), \
      ((__vector __bool short) __builtin_altivec_vsplth ((__vector signed short) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector signed short, (a1)), \
      ((__vector signed short) __builtin_altivec_vsplth ((__vector signed short) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector unsigned short, (a1)), \
      ((__vector unsigned short) __builtin_altivec_vsplth ((__vector signed short) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector __pixel, (a1)), \
      ((__vector __pixel) __builtin_altivec_vsplth ((__vector signed short) (a1), (const int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsplth")))))

#define vec_vspltb(a1, a2) \
__ch (__un_args_eq (__vector __bool char, (a1)), \
      ((__vector __bool char) __builtin_altivec_vspltb ((__vector signed char) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector signed char, (a1)), \
      ((__vector signed char) __builtin_altivec_vspltb ((__vector signed char) (a1), (const int) (a2))), \
__ch (__un_args_eq (__vector unsigned char, (a1)), \
      ((__vector unsigned char) __builtin_altivec_vspltb ((__vector signed char) (a1), (const int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vspltb"))))

#define vec_splat_s8(a1) ((__vector signed char) __builtin_altivec_vspltisb (a1))

#define vec_splat_s16(a1) ((__vector signed short) __builtin_altivec_vspltish (a1))

#define vec_splat_s32(a1) ((__vector signed int) __builtin_altivec_vspltisw (a1))

#define vec_splat_u8(a1) ((__vector unsigned char) __builtin_altivec_vspltisb (a1))

#define vec_splat_u16(a1) ((__vector unsigned short) __builtin_altivec_vspltish (a1))

#define vec_splat_u32(a1) ((__vector unsigned int) __builtin_altivec_vspltisw (a1))

#define vec_sr(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsrb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsrb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsrh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsrh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsrw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsrw ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_sr")))))))

#define vec_vsrw(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsrw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsrw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsrw")))

#define vec_vsrh(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsrh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsrh ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsrh")))

#define vec_vsrb(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsrb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsrb ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsrb")))

#define vec_sra(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsrab ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsrab ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsrah ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsrah ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsraw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsraw ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_sra")))))))

#define vec_vsraw(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsraw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsraw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsraw")))

#define vec_vsrah(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsrah ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsrah ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsrah")))

#define vec_vsrab(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsrab ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsrab ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsrab")))

#define vec_srl(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned short, (a2)), \
      ((__vector signed int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned char, (a2)), \
      ((__vector signed int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool int) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned int, (a2)), \
      ((__vector signed short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned char, (a2)), \
      ((__vector signed short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool short) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector unsigned int, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector unsigned short, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector unsigned char, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned int, (a2)), \
      ((__vector signed char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned short, (a2)), \
      ((__vector signed char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned int, (a2)), \
      ((__vector __bool char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned short, (a2)), \
      ((__vector __bool char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vsr ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_srl")))))))))))))))))))))))))))))))

#define vec_sro(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector signed char, (a2)), \
      ((__vector float) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector unsigned char, (a2)), \
      ((__vector float) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed char, (a2)), \
      ((__vector signed int) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector unsigned char, (a2)), \
      ((__vector signed int) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector signed char, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed char, (a2)), \
      ((__vector signed short) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector unsigned char, (a2)), \
      ((__vector signed short) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector signed char, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector unsigned char, (a2)), \
      ((__vector __pixel) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector signed char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector unsigned char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector signed char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsro ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_sro")))))))))))))))))

#define vec_st(a1, a2, a3) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), unsigned char, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed char, (a1), signed char, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool char, (a1), unsigned char, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool char, (a1), signed char, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), unsigned short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed short, (a1), short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool short, (a1), unsigned short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool short, (a1), short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __pixel, (a1), unsigned short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __pixel, (a1), short, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), unsigned int, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed int, (a1), int, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool int, (a1), unsigned int, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool int, (a1), int, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector float, (a1), float, *(a3)), \
  __builtin_altivec_stvx ((__vector signed int) (a1), (a2), (void *) (a3)), \
__builtin_altivec_compiletime_error ("vec_st")))))))))))))))))))))))))))

#define vec_stl(a1, a2, a3) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), unsigned char, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed char, (a1), signed char, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool char, (a1), unsigned char, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool char, (a1), signed char, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), unsigned short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed short, (a1), short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool short, (a1), unsigned short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool short, (a1), short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __pixel, (a1), unsigned short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __pixel, (a1), short, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), unsigned int, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector signed int, (a1), int, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool int, (a1), unsigned int, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector __bool int, (a1), int, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__ch (__bin_args_eq (__vector float, (a1), float, *(a3)), \
  __builtin_altivec_stvxl ((__vector signed int) (a1), (a2), (void *) (a3)), \
__builtin_altivec_compiletime_error ("vec_stl")))))))))))))))))))))))))))

#define vec_ste(a, b, c) \
__ch (__bin_args_eq (__vector unsigned char, (a), unsigned char, *(c)), \
      __builtin_altivec_stvebx ((__vector signed char) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector signed char, (a), signed char, *(c)), \
      __builtin_altivec_stvebx ((__vector signed char) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector __bool char, (a), unsigned char, *(c)), \
      __builtin_altivec_stvebx ((__vector signed char) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector __bool char, (a), signed char, *(c)), \
      __builtin_altivec_stvebx ((__vector signed char) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector unsigned short, (a), unsigned short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector signed short, (a), short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector __bool short, (a), unsigned short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector __bool short, (a), short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector __pixel, (a), unsigned short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector __pixel, (a), short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector unsigned int, (a), unsigned int, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector signed int, (a), int, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector __bool int, (a), unsigned int, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector __bool int, (a), int, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (void *) (c)), \
__ch (__bin_args_eq (__vector float, (a), float, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (void *) (c)), \
     __builtin_altivec_compiletime_error ("vec_ste"))))))))))))))))

#define vec_stvewx(a, b, c) \
__ch (__bin_args_eq (__vector unsigned int, (a), unsigned int, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (c)), \
__ch (__bin_args_eq (__vector signed int, (a), int, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (c)), \
__ch (__bin_args_eq (__vector __bool int, (a), unsigned int, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (c)), \
__ch (__bin_args_eq (__vector __bool int, (a), int, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (c)), \
__ch (__bin_args_eq (__vector float, (a), float, *(c)), \
     __builtin_altivec_stvewx ((__vector signed int) (a), (b), (c)), \
__builtin_altivec_compiletime_error ("vec_stvewx"))))))

#define vec_stvehx(a, b, c) \
__ch (__bin_args_eq (__vector unsigned short, (a), unsigned short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (c)), \
__ch (__bin_args_eq (__vector signed short, (a), short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (c)), \
__ch (__bin_args_eq (__vector __bool short, (a), unsigned short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (c)), \
__ch (__bin_args_eq (__vector __bool short, (a), short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (c)), \
__ch (__bin_args_eq (__vector __pixel, (a), unsigned short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (c)), \
__ch (__bin_args_eq (__vector __pixel, (a), short, *(c)), \
     __builtin_altivec_stvehx ((__vector signed short) (a), (b), (c)), \
__builtin_altivec_compiletime_error ("vec_stvehx")))))))

#define vec_stvebx(a, b, c) \
__ch (__bin_args_eq (__vector unsigned char, (a), unsigned char, *(c)), \
      __builtin_altivec_stvebx ((__vector signed char) (a), (b), (c)), \
__ch (__bin_args_eq (__vector signed char, (a), signed char, *(c)), \
      __builtin_altivec_stvebx ((__vector signed char) (a), (b), (c)), \
__ch (__bin_args_eq (__vector __bool char, (a), unsigned char, *(c)), \
      __builtin_altivec_stvebx ((__vector signed char) (a), (b), (c)), \
__ch (__bin_args_eq (__vector __bool char, (a), signed char, *(c)), \
      __builtin_altivec_stvebx ((__vector signed char) (a), (b), (c)), \
__builtin_altivec_compiletime_error ("vec_stvebx")))))

#define vec_sub(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vsubfp ((__vector float) (a1), (__vector float) (a2))), \
    __builtin_altivec_compiletime_error ("vec_sub"))))))))))))))))))))

#define vec_vsubfp(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vsubfp ((__vector float) (a1), (__vector float) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsubfp"))

#define vec_vsubuwm(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuwm ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsubuwm")))))))

#define vec_vsubuhm(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhm ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsubuhm")))))))

#define vec_vsububm(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububm ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsububm")))))))

#define vec_subc(a1, a2) \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
  ((__vector unsigned int) __builtin_altivec_vsubcuw ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_subc"))

#define vec_subs(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_subs")))))))))))))))))))

#define vec_vsubsws(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsubsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsubsws"))))

#define vec_vsubuws(a1, a2) \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsubuws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsubuws"))))

#define vec_vsubshs(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vsubshs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsubshs"))))

#define vec_vsubuhs(a1, a2) \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vsubuhs ((__vector signed short) (a1), (__vector signed short) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsubuhs"))))

#define vec_vsubsbs(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vsubsbs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsubsbs"))))

#define vec_vsububs(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vsububs ((__vector signed char) (a1), (__vector signed char) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsububs"))))

#define vec_sum4s(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsum4ubs ((__vector signed char) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsum4sbs ((__vector signed char) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsum4shs ((__vector signed short) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_sum4s"))))

#define vec_vsum4shs(a1, a2) \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsum4shs ((__vector signed short) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsum4shs"))

#define vec_vsum4sbs(a1, a2) \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsum4sbs ((__vector signed char) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsum4sbs"))

#define vec_vsum4ubs(a1, a2) \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vsum4ubs ((__vector signed char) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_vsum4ubs"))

#define vec_sum2s(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsum2sws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_sum2s"))

#define vec_sums(a1, a2) \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vsumsws ((__vector signed int) (a1), (__vector signed int) (a2))), \
__builtin_altivec_compiletime_error ("vec_sums"))

#define vec_trunc(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      ((__vector float) __builtin_altivec_vrfiz ((__vector float) (a1))), \
__builtin_altivec_compiletime_error ("vec_trunc"))

#define vec_unpackh(a1) \
__ch (__un_args_eq (__vector signed char, (a1)), \
      ((__vector signed short) __builtin_altivec_vupkhsb ((__vector signed char) (a1))), \
__ch (__un_args_eq (__vector __bool char, (a1)), \
      ((__vector __bool short) __builtin_altivec_vupkhsb ((__vector signed char) (a1))), \
__ch (__un_args_eq (__vector __pixel, (a1)), \
      ((__vector unsigned int) __builtin_altivec_vupkhpx ((__vector signed short) (a1))), \
__ch (__un_args_eq (__vector signed short, (a1)), \
      ((__vector signed int) __builtin_altivec_vupkhsh ((__vector signed short) (a1))), \
__ch (__un_args_eq (__vector __bool short, (a1)), \
      ((__vector __bool int) __builtin_altivec_vupkhsh ((__vector signed short) (a1))), \
    __builtin_altivec_compiletime_error ("vec_unpackh"))))))

#define vec_vupkhsh(a1) \
__ch (__un_args_eq (__vector __bool short, (a1)), \
      ((__vector __bool int) __builtin_altivec_vupkhsh ((__vector signed short) (a1))), \
__ch (__un_args_eq (__vector signed short, (a1)), \
      ((__vector signed int) __builtin_altivec_vupkhsh ((__vector signed short) (a1))), \
__builtin_altivec_compiletime_error ("vec_vupkhsh")))

#define vec_vupkhpx(a1) \
__ch (__un_args_eq (__vector __pixel, (a1)), \
      ((__vector unsigned int) __builtin_altivec_vupkhpx ((__vector signed short) (a1))), \
__builtin_altivec_compiletime_error ("vec_vupkhpx"))

#define vec_vupkhsb(a1) \
__ch (__un_args_eq (__vector __bool char, (a1)), \
      ((__vector __bool short) __builtin_altivec_vupkhsb ((__vector signed char) (a1))), \
__ch (__un_args_eq (__vector signed char, (a1)), \
      ((__vector signed short) __builtin_altivec_vupkhsb ((__vector signed char) (a1))), \
__builtin_altivec_compiletime_error ("vec_vupkhsb")))

#define vec_unpackl(a1) \
__ch (__un_args_eq (__vector signed char, (a1)), \
      ((__vector signed short) __builtin_altivec_vupklsb ((__vector signed char) (a1))), \
__ch (__un_args_eq (__vector __bool char, (a1)), \
      ((__vector __bool short) __builtin_altivec_vupklsb ((__vector signed char) (a1))), \
__ch (__un_args_eq (__vector __pixel, (a1)), \
      ((__vector unsigned int) __builtin_altivec_vupklpx ((__vector signed short) (a1))), \
__ch (__un_args_eq (__vector signed short, (a1)), \
      ((__vector signed int) __builtin_altivec_vupklsh ((__vector signed short) (a1))), \
__ch (__un_args_eq (__vector __bool short, (a1)), \
      ((__vector __bool int) __builtin_altivec_vupklsh ((__vector signed short) (a1))), \
    __builtin_altivec_compiletime_error ("vec_unpackl"))))))

#define vec_vupklsh(a1) \
__ch (__un_args_eq (__vector __bool short, (a1)), \
      ((__vector __bool int) __builtin_altivec_vupklsh ((__vector signed short) (a1))), \
__ch (__un_args_eq (__vector signed short, (a1)), \
      ((__vector signed int) __builtin_altivec_vupklsh ((__vector signed short) (a1))), \
__builtin_altivec_compiletime_error ("vec_vupklsh")))

#define vec_vupklpx(a1) \
__ch (__un_args_eq (__vector __pixel, (a1)), \
      ((__vector unsigned int) __builtin_altivec_vupklpx ((__vector signed short) (a1))), \
__builtin_altivec_compiletime_error ("vec_vupklpx"))

#define vec_vupklsb(a1) \
__ch (__un_args_eq (__vector __bool char, (a1)), \
      ((__vector __bool short) __builtin_altivec_vupklsb ((__vector signed char) (a1))), \
__ch (__un_args_eq (__vector signed char, (a1)), \
      ((__vector signed short) __builtin_altivec_vupklsb ((__vector signed char) (a1))), \
__builtin_altivec_compiletime_error ("vec_vupklsb")))

#define vec_xor(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector float, (a1), __vector __bool int, (a2)), \
      ((__vector float) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector float, (a2)), \
      ((__vector float) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      ((__vector __bool int) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      ((__vector signed int) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      ((__vector signed int) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      ((__vector unsigned int) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      ((__vector __bool short) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      ((__vector signed short) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      ((__vector signed short) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      ((__vector unsigned short) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      ((__vector __bool char) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      ((__vector signed char) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      ((__vector signed char) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      ((__vector unsigned char) __builtin_altivec_vxor ((__vector signed int) (a1), (__vector signed int) (a2))), \
    __builtin_altivec_compiletime_error ("vec_xor")))))))))))))))))))))))))

/* Predicates.  */

#define vec_all_eq(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpeqfp_p (__CR6_LT, (__vector float) (a1), (__vector float) (a2)), \
    __builtin_altivec_compiletime_error ("vec_all_eq"))))))))))))))))))))))))

#define vec_all_ge(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgefp_p (__CR6_LT, (__vector float) (a1), (__vector float) (a2)), \
    __builtin_altivec_compiletime_error ("vec_all_ge"))))))))))))))))))))

#define vec_all_gt(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgtfp_p (__CR6_LT, (__vector float) (a1), (__vector float) (a2)), \
    __builtin_altivec_compiletime_error ("vec_all_gt"))))))))))))))))))))

#define vec_all_in(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpbfp_p (__CR6_EQ, (a1), (a2)), \
    __builtin_altivec_compiletime_error ("vec_all_in"))

#define vec_all_le(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgefp_p (__CR6_LT, (__vector float) (a2), (__vector float) (a1)), \
    __builtin_altivec_compiletime_error ("vec_all_le"))))))))))))))))))))

#define vec_all_lt(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgtfp_p (__CR6_LT, (__vector float) (a2), (__vector float) (a1)), \
    __builtin_altivec_compiletime_error ("vec_all_lt"))))))))))))))))))))

#define vec_all_nan(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      __builtin_altivec_vcmpeqfp_p (__CR6_EQ, (a1), (a1)), \
    __builtin_altivec_compiletime_error ("vec_all_nan"))

#define vec_all_ne(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpeqfp_p (__CR6_EQ, (__vector float) (a1), (__vector float) (a2)), \
    __builtin_altivec_compiletime_error ("vec_all_ne"))))))))))))))))))))))))

#define vec_all_nge(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgefp_p (__CR6_EQ, (a1), (a2)), \
    __builtin_altivec_compiletime_error ("vec_all_nge"))

#define vec_all_ngt(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgtfp_p (__CR6_EQ, (a1), (a2)), \
    __builtin_altivec_compiletime_error ("vec_all_ngt"))

#define vec_all_nle(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgefp_p (__CR6_EQ, (a2), (a1)), \
    __builtin_altivec_compiletime_error ("vec_all_nle"))

#define vec_all_nlt(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgtfp_p (__CR6_EQ, (a2), (a1)), \
    __builtin_altivec_compiletime_error ("vec_all_nlt"))

#define vec_all_numeric(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      __builtin_altivec_vcmpeqfp_p (__CR6_LT, (a1), (a1)), \
    __builtin_altivec_compiletime_error ("vec_all_numeric"))

#define vec_any_eq(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpeqfp_p (__CR6_EQ_REV, (__vector float) (a1), (__vector float) (a2)), \
    __builtin_altivec_compiletime_error ("vec_any_eq"))))))))))))))))))))))))

#define vec_any_ge(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgefp_p (__CR6_EQ_REV, (__vector float) (a1), (__vector float) (a2)), \
    __builtin_altivec_compiletime_error ("vec_any_ge"))))))))))))))))))))

#define vec_any_gt(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgtfp_p (__CR6_EQ_REV, (__vector float) (a1), (__vector float) (a2)), \
    __builtin_altivec_compiletime_error ("vec_any_gt"))))))))))))))))))))

#define vec_any_le(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgefp_p (__CR6_EQ_REV, (__vector float) (a2), (__vector float) (a1)), \
    __builtin_altivec_compiletime_error ("vec_any_le"))))))))))))))))))))

#define vec_any_lt(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpgtub_p (__CR6_EQ_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpgtsb_p (__CR6_EQ_REV, (__vector signed char) (a2), (__vector signed char) (a1)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpgtuh_p (__CR6_EQ_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpgtsh_p (__CR6_EQ_REV, (__vector signed short) (a2), (__vector signed short) (a1)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpgtuw_p (__CR6_EQ_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpgtsw_p (__CR6_EQ_REV, (__vector signed int) (a2), (__vector signed int) (a1)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgtfp_p (__CR6_EQ_REV, (__vector float) (a2), (__vector float) (a1)), \
    __builtin_altivec_compiletime_error ("vec_any_lt"))))))))))))))))))))

#define vec_any_nan(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      __builtin_altivec_vcmpeqfp_p (__CR6_LT_REV, (a1), (a1)), \
    __builtin_altivec_compiletime_error ("vec_any_nan"))

#define vec_any_ne(a1, a2) \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector signed char, (a1), __vector signed char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector unsigned char, (a1), __vector unsigned char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool char, (a1), __vector __bool char, (a2)), \
      __builtin_altivec_vcmpequb_p (__CR6_LT_REV, (__vector signed char) (a1), (__vector signed char) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector signed short, (a1), __vector signed short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector unsigned short, (a1), __vector unsigned short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool short, (a1), __vector __bool short, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __pixel, (a1), __vector __pixel, (a2)), \
      __builtin_altivec_vcmpequh_p (__CR6_LT_REV, (__vector signed short) (a1), (__vector signed short) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector signed int, (a1), __vector signed int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector unsigned int, (a1), __vector unsigned int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector __bool int, (a1), __vector __bool int, (a2)), \
      __builtin_altivec_vcmpequw_p (__CR6_LT_REV, (__vector signed int) (a1), (__vector signed int) (a2)), \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpeqfp_p (__CR6_LT_REV, (__vector float) (a1), (__vector float) (a2)), \
    __builtin_altivec_compiletime_error ("vec_any_ne"))))))))))))))))))))))))

#define vec_any_nge(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgefp_p (__CR6_LT_REV, (a1), (a2)), \
    __builtin_altivec_compiletime_error ("vec_any_nge"))

#define vec_any_ngt(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgtfp_p (__CR6_LT_REV, (a1), (a2)), \
    __builtin_altivec_compiletime_error ("vec_any_ngt"))

#define vec_any_nle(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgefp_p (__CR6_LT_REV, (a2), (a1)), \
    __builtin_altivec_compiletime_error ("vec_any_nle"))

#define vec_any_nlt(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpgtfp_p (__CR6_LT_REV, (a2), (a1)), \
    __builtin_altivec_compiletime_error ("vec_any_nlt"))

#define vec_any_numeric(a1) \
__ch (__un_args_eq (__vector float, (a1)), \
      __builtin_altivec_vcmpeqfp_p (__CR6_EQ_REV, (a1), (a1)), \
    __builtin_altivec_compiletime_error ("vec_any_numeric"))

#define vec_any_out(a1, a2) \
__ch (__bin_args_eq (__vector float, (a1), __vector float, (a2)), \
      __builtin_altivec_vcmpbfp_p (__CR6_EQ_REV, (a1), (a2)), \
    __builtin_altivec_compiletime_error ("vec_any_out"))


#endif /* __cplusplus */

#endif /* _ALTIVEC_H */
