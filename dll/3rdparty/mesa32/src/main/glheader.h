/**
 * \file glheader.h
 * Top-most include file.
 *
 * This is the top-most include file of the Mesa sources.
 * It includes gl.h and all system headers which are needed.
 * Other Mesa source files should \e not directly include any system
 * headers.  This allows system-dependent hacks/workarounds to be
 * collected in one place.
 *
 * \note Actually, a lot of system-dependent stuff is now in imports.[ch].
 *
 * If you touch this file, everything gets recompiled!
 *
 * This file should be included before any other header in the .c files.
 *
 * Put compiler/OS/assembly pragmas and macros here to avoid
 * cluttering other source files.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#ifndef GLHEADER_H
#define GLHEADER_H

#include <assert.h>
#include <ctype.h>
#if defined(__alpha__) && defined(CCPML)
#include <cpml.h> /* use Compaq's Fast Math Library on Alpha */
#else
#include <math.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined(__linux__) && defined(__i386__)
#include <fpu_control.h>
#endif
#include <float.h>
#include <stdarg.h>


/* Get typedefs for uintptr_t and friends */
#if defined(__MINGW32__) || defined(__NetBSD__)
#  include <stdint.h>
#elif defined(_WIN32)
#  include <BaseTsd.h>
#  if _MSC_VER == 1200
     typedef UINT_PTR uintptr_t;
#  endif 
#elif defined(__INTERIX)
/* Interix 3.x has a gcc that shadows this. */
#  ifndef _UINTPTR_T_DEFINED
     typedef unsigned long uintptr_t;
#  define _UINTPTR_T_DEFINED
#  endif
#else
#  include <inttypes.h>
#endif


/* Sun compilers define __i386 instead of the gcc-style __i386__ */
#ifdef __SUNPRO_C
# if !defined(__i386__) && defined(__i386)
#  define __i386__
# elif !defined(__amd64__) && defined(__amd64)
#  define __amd64__
# elif !defined(__sparc__) && defined(__sparc)
#  define __sparc__
# endif
# if !defined(__volatile)
#  define __volatile volatile
# endif
#endif

#if defined(_WIN32) && !defined(__WIN32__) && !defined(__CYGWIN__) && !defined(BUILD_FOR_SNAP)
#  define __WIN32__
#  define finite _finite
#endif

#if defined(__WATCOMC__)
#  define finite _finite
#  pragma disable_message(201) /* Disable unreachable code warnings */
#endif

#ifdef WGLAPI
#	undef WGLAPI
#endif

#if !defined(OPENSTEP) && (defined(__WIN32__) && !defined(__CYGWIN__)) && !defined(BUILD_FOR_SNAP)
#  if !defined(__GNUC__) /* mingw environment */
#    pragma warning( disable : 4068 ) /* unknown pragma */
#    pragma warning( disable : 4710 ) /* function 'foo' not inlined */
#    pragma warning( disable : 4711 ) /* function 'foo' selected for automatic inline expansion */
#    pragma warning( disable : 4127 ) /* conditional expression is constant */
#    if defined(MESA_MINWARN)
#      pragma warning( disable : 4244 ) /* '=' : conversion from 'const double ' to 'float ', possible loss of data */
#      pragma warning( disable : 4018 ) /* '<' : signed/unsigned mismatch */
#      pragma warning( disable : 4305 ) /* '=' : truncation from 'const double ' to 'float ' */
#      pragma warning( disable : 4550 ) /* 'function' undefined; assuming extern returning int */
#      pragma warning( disable : 4761 ) /* integral size mismatch in argument; conversion supplied */
#    endif
#  endif
#  if (defined(_MSC_VER) || defined(__MINGW32__)) && defined(BUILD_GL32) /* tag specify we're building mesa as a DLL */
#    define WGLAPI __declspec(dllexport)
#  elif (defined(_MSC_VER) || defined(__MINGW32__)) && defined(_DLL) /* tag specifying we're building for DLL runtime support */
#    define WGLAPI __declspec(dllimport)
#  else /* for use with static link lib build of Win32 edition only */
#    define WGLAPI __declspec(dllimport)
#  endif /* _STATIC_MESA support */
#endif /* WIN32 / CYGWIN bracket */


/*
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
#include <sys/endian.h>
#define CPU_TO_LE32( x )	bswap32( x )
#endif /*__linux__*/
#define MESA_BIG_ENDIAN 1
#else
#define CPU_TO_LE32( x )	( x )
#define MESA_LITTLE_ENDIAN 1
#endif
#define LE32_TO_CPU( x )	CPU_TO_LE32( x )


#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "GL/glext.h"


#ifndef GL_FIXED
#define GL_FIXED 0x140C
#endif


#ifndef GL_OES_point_size_array
#define GL_POINT_SIZE_ARRAY_OES                                 0x8B9C
#define GL_POINT_SIZE_ARRAY_TYPE_OES                            0x898A
#define GL_POINT_SIZE_ARRAY_STRIDE_OES                          0x898B
#define GL_POINT_SIZE_ARRAY_POINTER_OES                         0x898C
#define GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES                  0x8B9F
#endif


#ifndef GL_OES_draw_texture
#define GL_TEXTURE_CROP_RECT_OES  0x8B9D
#endif


#if !defined(CAPI) && defined(WIN32) && !defined(BUILD_FOR_SNAP)
#define CAPI _cdecl
#endif


/* This is a macro on IRIX */
#ifdef _P
#undef _P
#endif


/* Turn off macro checking systems used by other libraries */
#ifdef CHECK
#undef CHECK
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


/* Function inlining */
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
#elif defined(__SUNPRO_C) && defined(__C99FEATURES__)
#  define INLINE inline
#  define __inline inline
#  define __inline__ inline
#elif (__STDC_VERSION__ >= 199901L) /* C99 */
#  define INLINE inline
#else
#  define INLINE
#endif


/* If we build the library with gcc's -fvisibility=hidden flag, we'll
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


/* Some compilers don't like some of Mesa's const usage */
#ifdef NO_CONST
#  define CONST
#else
#  define CONST const
#endif


#if !defined(_WIN32_WCE)
#if defined(BUILD_FOR_SNAP) && defined(CHECKED)
#  define ASSERT(X)   _CHECK(X) 
#elif defined(DEBUG)
#  define ASSERT(X)   assert(X)
#else
#  define ASSERT(X)
#endif
#endif


#if (!defined(__GNUC__) || __GNUC__ < 3) && (!defined(__IBMC__) || __IBMC__ < 900)
#  define __builtin_expect(x, y) x
#endif

/* The __FUNCTION__ gcc variable is generally only used for debugging.
 * If we're not using gcc, define __FUNCTION__ as a cpp symbol here.
 * Don't define it if using a newer Windows compiler.
 */
#ifndef __FUNCTION__
# if defined(__VMS)
#  define __FUNCTION__ "VMS$NL:"
# elif ((!defined __GNUC__) || (__GNUC__ < 2)) && (!defined __xlC__) && \
      (!defined(_MSC_VER) || _MSC_VER < 1300)
#  if (__STDC_VERSION__ >= 199901L) /* C99 */ || \
    (defined(__SUNPRO_C) && defined(__C99FEATURES__))
#   define __FUNCTION__ __func__
#  else
#   define __FUNCTION__ "<unknown>"
#  endif
# endif
#endif


#include "config.h"

#endif /* GLHEADER_H */
