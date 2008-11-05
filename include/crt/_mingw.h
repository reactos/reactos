/*
 * _mingw.h
 *
 * Mingw specific macros included by ALL include files.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Mumit Khan  <khan@xraylith.wisc.edu>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __MINGW_H
#define __MINGW_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

/* These are defined by the user (or the compiler)
   to specify how identifiers are imported from a DLL.

   __DECLSPEC_SUPPORTED    Defined if dllimport attribute is supported.
   __MINGW_IMPORT          The attribute definition to specify imported
                           variables/functions.
   _CRTIMP                 As above.  For MS compatibility.
   __MINGW32_VERSION       Runtime version.
   __MINGW32_MAJOR_VERSION Runtime major version.
   __MINGW32_MINOR_VERSION Runtime minor version.
   __MINGW32_BUILD_DATE    Runtime build date.

   Other macros:

   __int64                 define to be long long. Using a typedef doesn't
                           work for "unsigned __int64"

   All headers should include this first, and then use __DECLSPEC_SUPPORTED
   to choose between the old ``__imp__name'' style or __MINGW_IMPORT
   style declarations.  */

/* Try to avoid problems with outdated checks for GCC __attribute__ support.  */
#undef __attribute__

#if defined(_MSC_VER)
# ifdef _DLL
# ifndef __MINGW_IMPORT
#  define __MINGW_IMPORT  __declspec(dllimport)
# endif
# ifndef _CRTIMP
#  define _CRTIMP  __declspec(dllimport)
# endif
# else
#  ifndef __MINGW_IMPORT
#   define __MINGW_IMPORT
#  endif
#  ifndef _CRTIMP
#   define _CRTIMP
#  endif
# endif
# define __DECLSPEC_SUPPORTED
# define __attribute__(x) /* nothing */
# define __restrict__ /* nothing */
#elif defined(__GNUC__)
# ifdef __declspec
#  ifndef __MINGW_IMPORT
#   ifdef _DLL
   /* Note the extern. This is needed to work around GCC's
      limitations in handling dllimport attribute.  */
#   define __MINGW_IMPORT  extern __attribute__ ((__dllimport__))
#   else
#    define __MINGW_IMPORT  extern
#  endif
#  endif
#  ifndef _CRTIMP
#   ifdef __USE_CRTIMP
#    ifdef _DLL
#    define _CRTIMP  __attribute__ ((dllimport))
#   else
#    define _CRTIMP
#   endif
#   else
#    define _CRTIMP
#  endif
#  endif
#  define __DECLSPEC_SUPPORTED
# else /* __declspec */
#  undef __DECLSPEC_SUPPORTED
#  undef __MINGW_IMPORT
#  ifndef _CRTIMP
#   define _CRTIMP
#  endif
# endif /* __declspec */

/*
   The next two defines can cause problems if user code adds the __cdecl attribute
   like so:
   void __attribute__ ((__cdecl)) foo(void);
*/
# ifndef __cdecl
#  define __cdecl  __attribute__ ((__cdecl__))
# endif
# ifndef __stdcall
#  define __stdcall __attribute__ ((__stdcall__))
# endif
# ifndef __int64
#  define __int64 long long
# endif
# ifndef __int32
#  define __int32 long
# endif
# ifndef __int16
#  define __int16 short
# endif
# ifndef __int8
#  define __int8 char
# endif
# ifndef __small
#  define __small char
# endif
# ifndef __hyper
#  define __hyper long long
# endif
#else
# ifndef __MINGW_IMPORT
#  define __MINGW_IMPORT  __declspec(dllimport)
# endif
# ifndef _CRTIMP
#  define _CRTIMP  __declspec(dllimport)
# endif
# define __DECLSPEC_SUPPORTED
# define __attribute__(x) /* nothing */
#endif /* __GNUC__ */

#if defined (__GNUC__) && defined (__GNUC_MINOR__)
#define __MINGW_GNUC_PREREQ(major, minor) \
  (__GNUC__ > (major) \
   || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define __MINGW_GNUC_PREREQ(major, minor)  0
#endif

#if defined (_MSC_VER)
#define __MINGW_MSC_PREREQ(major, minor) \
  ((_MSC_VER / 100) > (major) \
   || ((_MSC_VER / 100) == (major) && (_MSC_VER % 100) >= (minor)))
#else
#define __MINGW_MSC_PREREQ(major, minor)  0
#endif

#ifdef __cplusplus
# define __CRT_INLINE inline
#else
# if defined(_MSC_VER)
#  define __CRT_INLINE __inline
# elif __GNUC_STDC_INLINE__
#  define __CRT_INLINE extern inline __attribute__((__gnu_inline__))
# else
#  define __CRT_INLINE extern __inline__
# endif
#endif

#ifdef __cplusplus
# define __UNUSED_PARAM(x)
#else
# ifdef __GNUC__
#  define __UNUSED_PARAM(x) x __attribute__ ((__unused__))
# else
#  define __UNUSED_PARAM(x) x
# endif
#endif

#ifdef __GNUC__
#define __MINGW_ATTRIB_NORETURN __attribute__ ((__noreturn__))
#define __MINGW_ATTRIB_CONST __attribute__ ((__const__))
#elif __MINGW_MSC_PREREQ(12, 0)
#define __MINGW_ATTRIB_NORETURN __declspec(noreturn)
#define __MINGW_ATTRIB_CONST
#else
#define __MINGW_ATTRIB_NORETURN
#define __MINGW_ATTRIB_CONST
#endif

#if defined(__GNUC__)
#define __mingw_va_list __gnuc_va_list
#define __mingw_va_start(v,l) __gnuc_va_start(v,l)
#define __mingw_va_end(v) __gnuc_va_end(v)
#define __mingw_va_arg(v,l)	__gnuc_va_arg(v,l)
#define __mingw_va_copy(d,s) __gnuc_va_copy(d,s)
#elif defined(_MSC_VER)
#define __mingw_va_list __msc_va_list
#define __mingw_va_start(v,l) __msc_va_start(v,l)
#define __mingw_va_end(v) __msc_va_end(v)
#define __mingw_va_arg(v,l)	__msc_va_arg(v,l)
#define __mingw_va_copy(d,s) __msc_va_copy(d,s)
#endif

#if __MINGW_GNUC_PREREQ (3, 0)
#define __MINGW_ATTRIB_MALLOC __attribute__ ((__malloc__))
#define __MINGW_ATTRIB_PURE __attribute__ ((__pure__))
#elif __MINGW_MSC_PREREQ (14, 0)
#define __MINGW_ATTRIB_MALLOC __declspec(noalias) __declspec(restrict)
#define __MINGW_ATTRIB_PURE
#else
#define __MINGW_ATTRIB_MALLOC
#define __MINGW_ATTRIB_PURE
#endif

/* Attribute `nonnull' was valid as of gcc 3.3.  We don't use GCC's
   variadiac macro facility, because variadic macros cause syntax
   errors with  --traditional-cpp.  */
#if  __MINGW_GNUC_PREREQ (3, 3)
#define __MINGW_ATTRIB_NONNULL(arg) __attribute__ ((__nonnull__ (arg)))
#else
#define __MINGW_ATTRIB_NONNULL(arg)
#endif /* GNUC >= 3.3 */

#if  __MINGW_GNUC_PREREQ (3, 1)
#define __MINGW_ATTRIB_DEPRECATED __attribute__ ((__deprecated__))
#elif __MINGW_MSC_PREREQ (12, 0)
#define __MINGW_ATTRIB_DEPRECATED __declspec (deprecated)
#else
#define __MINGW_ATTRIB_DEPRECATED
#endif

#if  __MINGW_GNUC_PREREQ (3, 3)
#define __MINGW_NOTHROW __attribute__ ((__nothrow__))
#elif __MINGW_MSC_PREREQ (12, 0) && defined (__cplusplus)
#define __MINGW_NOTHROW __declspec (nothrow)
#else
#define __MINGW_NOTHROW
#endif

/* TODO: Mark (almost) all CRT functions as __MINGW_NOTHROW.  This will
allow GCC to optimize away some EH unwind code, at least in DW2 case.  */

#ifndef __MSVCRT_VERSION__
/*  High byte is the major version, low byte is the minor. */
# define __MSVCRT_VERSION__ 0x0600
#endif

#define __MINGW32_VERSION 3.13
#define __MINGW32_MAJOR_VERSION 3
#define __MINGW32_MINOR_VERSION 13

#endif /* __MINGW_H */
