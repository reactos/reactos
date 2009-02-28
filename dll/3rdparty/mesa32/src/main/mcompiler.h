/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */




/**
 * \file mcopmiler.h
 *
 * Define helper macros/etc that are platform or compiler-specific.
 * There shouldn't be anything specific to GL in here.
 */

#ifndef MCOMPILER_H
#define MCOMPILER_H



/* Get typedefs for uintptr_t and friends */
#if defined(__MINGW32__) || defined(__NetBSD__)
#  include <stdint.h>
#elif defined(_WIN32)
#  include <BaseTsd.h>
#  if _MSC_VER == 1200
     typedef UINT_PTR uintptr_t;
#  endif 
#else
#  include <inttypes.h>
#endif




#ifdef __cplusplus
extern "C" {
#endif



/**
 * NULL
 */
#ifndef NULL
#define NULL 0
#endif



/**
 * Function inlining
 */
#if defined(__GNUC__)
#  define INLINE __inline__
#elif defined(__MSC__)
#  define INLINE __inline
#elif defined(_MSC_VER)
#  define INLINE __inline
#elif defined(__ICL)
#  define INLINE __inline
#elif defined(__INTEL_COMPILER)
#  define INLINE inline
#elif defined(__WATCOMC__) && (__WATCOMC__ >= 1100)
#  define INLINE __inline
#else
#  define INLINE
#endif


/**
 * PUBLIC/USED macros for symbol export.
 *
 * If we build the library with gcc's -fvisibility=hidden flag, we'll
 * use the PUBLIC macro to mark functions that are to be exported.
 *
 * We also need to define a USED attribute, so the optimizer doesn't 
 * inline a static function that we later use in an alias. - ajax
 */
#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303
#  define PUBLIC __attribute__((visibility("default")))
#  define USED __attribute__((used))
#else
#  define PUBLIC
#  define USED
#endif


/**
 * Some compilers don't like some of Mesa's const usage.
 */
#ifdef NO_CONST
#  define CONST
#else
#  define CONST const
#endif


/**
 * ASSERT macro
 */
#if defined(BUILD_FOR_SNAP) && defined(CHECKED)
#  define ASSERT(X)   _CHECK(X) 
#elif defined(DEBUG)
#  define ASSERT(X)   assert(X)
#else
#  define ASSERT(X)
#endif


/**
 * __builtin_expect() dummy for non-gcc
 */
#if (!defined(__GNUC__) || __GNUC__ < 3) && (!defined(__IBMC__) || __IBMC__ < 900)
#  define __builtin_expect(x, y) x
#endif



/**
 * The __FUNCTION__ gcc variable is generally only used for debugging.
 * If we're not using gcc, define __FUNCTION__ as a cpp symbol here.
 * Don't define it if using a newer Windows compiler.
 */
#if defined(__VMS)
# define __FUNCTION__ "VMS$NL:"
#elif __STDC_VERSION__ < 199901L
# if ((!defined __GNUC__) || (__GNUC__ < 2)) && (!defined __xlC__) && \
      (!defined(_MSC_VER) || _MSC_VER < 1300)
#  define __FUNCTION__ "<unknown>"
# endif
#endif




/** gcc -pedantic warns about long string literals, LONGSTRING silences that */
#if !defined(__GNUC__) || (__GNUC__ < 2) || \
    ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 7))
# define LONGSTRING
#else
# define LONGSTRING __extension__
#endif


/* Create a macro so that asm functions can be linked into compilers other
 * than GNU C
 */
#ifndef _ASMAPI
#if defined(WIN32) && !defined(BUILD_FOR_SNAP)/* was: !defined( __GNUC__ ) && !defined( VMS ) && !defined( __INTEL_COMPILER )*/
#define _ASMAPI __cdecl
#else
#define _ASMAPI
#endif
#ifdef	PTR_DECL_IN_FRONT
#define	_ASMAPIP * _ASMAPI
#else
#define	_ASMAPIP _ASMAPI *
#endif
#endif

#ifdef USE_X86_ASM
#define _NORMAPI _ASMAPI
#define _NORMAPIP _ASMAPIP
#else
#define _NORMAPI
#define _NORMAPIP *
#endif



/**
 * XXX is this used anymore?
 */
#if !defined(CAPI) && defined(WIN32) && !defined(BUILD_FOR_SNAP)
#define CAPI _cdecl
#endif



/**
 * USE_IEEE: Determine if we're using IEEE floating point
 */
#if defined(__i386__) || defined(__386__) || defined(__sparc__) || \
    defined(__s390x__) || defined(__powerpc__) || \
    defined(__x86_64__) || \
    defined(ia64) || defined(__ia64__) || \
    defined(__hppa__) || defined(hpux) || \
    defined(__mips) || defined(_MIPS_ARCH) || \
    defined(__arm__) || \
    defined(__sh__) || defined(__m32r__) || \
    (defined(__sun) && defined(_IEEE_754)) || \
    (defined(__alpha__) && (defined(__IEEE_FLOAT) || !defined(VMS)))
#define USE_IEEE
#define IEEE_ONE 0x3f800000
#endif



/**
 * finite macro
 */
#if defined(_WIN32) && !defined(__WIN32__) && !defined(__CYGWIN__) && !defined(BUILD_FOR_SNAP)
#  define __WIN32__
#  define finite _finite
#endif

#if defined(__WATCOMC__)
#  define finite _finite
#  pragma disable_message(201) /* Disable unreachable code warnings */
#endif


/**
 * IS_INF_OR_NAN: test if float is infinite or NaN
 */
#ifdef USE_IEEE
static INLINE int IS_INF_OR_NAN( float x )
{
   fi_type tmp;
   tmp.f = x;
   return !(int)((unsigned int)((tmp.i & 0x7fffffff)-0x7f800000) >> 31);
}
#elif defined(isfinite)
#define IS_INF_OR_NAN(x)        (!isfinite(x))
#elif defined(finite)
#define IS_INF_OR_NAN(x)        (!finite(x))
#elif defined(__VMS)
#define IS_INF_OR_NAN(x)        (!finite(x))
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define IS_INF_OR_NAN(x)        (!isfinite(x))
#else
#define IS_INF_OR_NAN(x)        (!finite(x))
#endif



/***
 *** START_FAST_MATH: Set x86 FPU to faster, 32-bit precision mode (and save
 ***                  original mode to a temporary).
 *** END_FAST_MATH: Restore x86 FPU to original mode.
 ***/
#if defined(__GNUC__) && defined(__i386__)
/*
 * Set the x86 FPU control word to guarentee only 32 bits of precision
 * are stored in registers.  Allowing the FPU to store more introduces
 * differences between situations where numbers are pulled out of memory
 * vs. situations where the compiler is able to optimize register usage.
 *
 * In the worst case, we force the compiler to use a memory access to
 * truncate the float, by specifying the 'volatile' keyword.
 */
/* Hardware default: All exceptions masked, extended double precision,
 * round to nearest (IEEE compliant):
 */
#define DEFAULT_X86_FPU		0x037f
/* All exceptions masked, single precision, round to nearest:
 */
#define FAST_X86_FPU		0x003f
/* The fldcw instruction will cause any pending FP exceptions to be
 * raised prior to entering the block, and we clear any pending
 * exceptions before exiting the block.  Hence, asm code has free
 * reign over the FPU while in the fast math block.
 */
#if defined(NO_FAST_MATH)
#define START_FAST_MATH(x)						\
do {									\
   static unsigned mask = DEFAULT_X86_FPU;				\
   __asm__ ( "fnstcw %0" : "=m" (*&(x)) );				\
   __asm__ ( "fldcw %0" : : "m" (mask) );				\
} while (0)
#else
#define START_FAST_MATH(x)						\
do {									\
   static unsigned mask = FAST_X86_FPU;					\
   __asm__ ( "fnstcw %0" : "=m" (*&(x)) );				\
   __asm__ ( "fldcw %0" : : "m" (mask) );				\
} while (0)
#endif
/* Restore original FPU mode, and clear any exceptions that may have
 * occurred in the FAST_MATH block.
 */
#define END_FAST_MATH(x)						\
do {									\
   __asm__ ( "fnclex ; fldcw %0" : : "m" (*&(x)) );			\
} while (0)

#elif defined(__WATCOMC__) && defined(__386__)
#define DEFAULT_X86_FPU		0x037f /* See GCC comments above */
#define FAST_X86_FPU		0x003f /* See GCC comments above */
void _watcom_start_fast_math(unsigned short *x,unsigned short *mask);
#pragma aux _watcom_start_fast_math =                                   \
   "fnstcw  word ptr [eax]"                                             \
   "fldcw   word ptr [ecx]"                                             \
   parm [eax] [ecx]                                                     \
   modify exact [];
void _watcom_end_fast_math(unsigned short *x);
#pragma aux _watcom_end_fast_math =                                     \
   "fnclex"                                                             \
   "fldcw   word ptr [eax]"                                             \
   parm [eax]                                                           \
   modify exact [];
#if defined(NO_FAST_MATH)
#define START_FAST_MATH(x)                                              \
do {                                                                    \
   static unsigned short mask = DEFAULT_X86_FPU;	                \
   _watcom_start_fast_math(&x,&mask);                                   \
} while (0)
#else
#define START_FAST_MATH(x)                                              \
do {                                                                    \
   static unsigned short mask = FAST_X86_FPU;                           \
   _watcom_start_fast_math(&x,&mask);                                   \
} while (0)
#endif
#define END_FAST_MATH(x)  _watcom_end_fast_math(&x)

#elif defined(_MSC_VER) && defined(_M_IX86)
#define DEFAULT_X86_FPU		0x037f /* See GCC comments above */
#define FAST_X86_FPU		0x003f /* See GCC comments above */
#if defined(NO_FAST_MATH)
#define START_FAST_MATH(x) do {\
	static unsigned mask = DEFAULT_X86_FPU;\
	__asm fnstcw word ptr [x]\
	__asm fldcw word ptr [mask]\
} while(0)
#else
#define START_FAST_MATH(x) do {\
	static unsigned mask = FAST_X86_FPU;\
	__asm fnstcw word ptr [x]\
	__asm fldcw word ptr [mask]\
} while(0)
#endif
#define END_FAST_MATH(x) do {\
	__asm fnclex\
	__asm fldcw word ptr [x]\
} while(0)

#else
#define START_FAST_MATH(x)  x = 0
#define END_FAST_MATH(x)  (void)(x)
#endif



/**
 * Either define MESA_BIG_ENDIAN or MESA_LITTLE_ENDIAN.
 * Do not use them unless absolutely necessary!
 * Try to use a runtime test instead.
 * For now, only used by some DRI hardware drivers for color/texel packing.
 */
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN
#if defined(__linux__)
#include <byteswap.h>
#define CPU_TO_LE32( x )	bswap_32( x )
#else /*__linux__*/
#define CPU_TO_LE32( x )	( x )  /* fix me for non-Linux big-endian! */
#endif /*__linux__*/
#define MESA_BIG_ENDIAN 1
#else
#define CPU_TO_LE32( x )	( x )
#define MESA_LITTLE_ENDIAN 1
#endif
#define LE32_TO_CPU( x )	CPU_TO_LE32( x )



#ifdef __cplusplus
}
#endif


#endif /* MCOMPILER_H */
