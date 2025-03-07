/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_MINGW
#define _INC_MINGW

#ifndef _INTEGRAL_MAX_BITS
#define _INTEGRAL_MAX_BITS 64
#endif

#ifndef MINGW64
#define MINGW64
#define MINGW64_VERSION	1.0
#define MINGW64_VERSION_MAJOR	1
#define MINGW64_VERSION_MINOR	0
#define MINGW64_VERSION_STATE	"alpha"
#endif

#ifdef _WIN64
#ifdef __stdcall
#undef __stdcall
#endif
#define __stdcall
#endif

#ifdef __GNUC__
 /* These compilers do support __declspec */
# if !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(__CYGWIN32__)
#  define __declspec(x) __attribute__((x))
# endif
#endif

#ifdef _MSC_VER
#define __restrict__ /* nothing */
#endif

#if defined (__GNUC__) && defined (__GNUC_MINOR__)
#define __MINGW_GNUC_PREREQ(major, minor) \
  (__GNUC__ > (major) \
   || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define __MINGW_GNUC_PREREQ(major, minor)  0
#endif

#if defined (_MSC_VER)
#define __MINGW_MSC_PREREQ(major, minor) (_MSC_VER >= (major * 100 + minor * 10))
#else
#define __MINGW_MSC_PREREQ(major, minor) 0
#endif

#define USE___UUIDOF	0

#ifdef __cplusplus
# define __CRT_INLINE inline
#elif defined(_MSC_VER)
# define __CRT_INLINE __inline
#elif defined(__GNUC__)
# if defined(__clang__) || ( __MINGW_GNUC_PREREQ(4, 3)  &&  __STDC_VERSION__ >= 199901L)
#  define __CRT_INLINE extern inline __attribute__((__always_inline__,__gnu_inline__))
# else
#  define __CRT_INLINE extern __inline__ __attribute__((__always_inline__))
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

#ifdef __cplusplus
# define __unaligned
#else
# ifdef __GNUC__
#  define __unaligned
# elif defined(_MSC_VER) && !defined(_M_IA64) && !defined(_M_AMD64)
#  define __unaligned
# else
#  define __unaligned
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

#if __MINGW_GNUC_PREREQ (3, 0)
#define __MINGW_ATTRIB_MALLOC __attribute__ ((__malloc__))
#define __MINGW_ATTRIB_PURE __attribute__ ((__pure__))
#elif __MINGW_MSC_PREREQ(14, 0)
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

#ifdef __GNUC__
#define __MINGW_ATTRIB_UNUSED __attribute__ ((__unused__))
#else
#define __MINGW_ATTRIB_UNUSED
#endif /* ATTRIBUTE_UNUSED */

#if  __MINGW_GNUC_PREREQ (3, 1)
#define __MINGW_ATTRIB_DEPRECATED __attribute__ ((__deprecated__))
#elif __MINGW_MSC_PREREQ(12, 0)
#define __MINGW_ATTRIB_DEPRECATED __declspec(deprecated)
#else
#define __MINGW_ATTRIB_DEPRECATED
#endif

#if  __MINGW_GNUC_PREREQ (3, 1)
#define __MINGW_ATTRIB_DEPRECATED_SEC_WARN //__attribute__ ((__deprecated__))
#elif __MINGW_MSC_PREREQ(12, 0)
#define __MINGW_ATTRIB_DEPRECATED_SEC_WARN //__declspec(deprecated)
#else
#define __MINGW_ATTRIB_DEPRECATED_SEC_WARN
#endif

#if  __MINGW_GNUC_PREREQ (3, 1)
#define __MINGW_ATTRIB_DEPRECATED_MSVC2005 //__attribute__ ((__deprecated__))
#elif __MINGW_MSC_PREREQ(12, 0)
#define __MINGW_ATTRIB_DEPRECATED_MSVC2005 //__declspec(deprecated)
#else
#define __MINGW_ATTRIB_DEPRECATED_MSVC2005
#endif

#if  __MINGW_GNUC_PREREQ (3, 3)
#define __MINGW_NOTHROW __attribute__ ((__nothrow__))
#elif __MINGW_MSC_PREREQ(12, 0) && defined (__cplusplus)
#define __MINGW_NOTHROW __declspec(nothrow)
#else
#define __MINGW_NOTHROW
#endif

/* TODO: Mark (almost) all CRT functions as __MINGW_NOTHROW.  This will
allow GCC to optimize away some EH unwind code, at least in DW2 case.  */

#ifndef __MINGW_EXTENSION
#if defined(__GNUC__) || defined(__GNUG__)
#define __MINGW_EXTENSION       __extension__
#else
#define __MINGW_EXTENSION
#endif
#endif

#ifndef __MSVCRT_VERSION__
/*  High byte is the major version, low byte is the minor. */
# define __MSVCRT_VERSION__ 0x0700
#endif

//#ifndef WINVER
//#define WINVER 0x0502
//#endif

//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x502
//#endif

#ifdef __GNUC__
#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long
# define __ptr32
# define __ptr64
# ifdef __cplusplus
#  define __forceinline inline __attribute__((__always_inline__))
# else
#  if (( __MINGW_GNUC_PREREQ(4, 3)  &&  __STDC_VERSION__ >= 199901L) || defined(__clang__))
#   define __forceinline extern inline __attribute__((__always_inline__,__gnu_inline__))
#  else
#   define __forceinline extern __inline__ __attribute__((__always_inline__))
#  endif
# endif
#endif

#ifdef __cplusplus
#ifndef __nothrow
#define __nothrow __declspec(nothrow)
#endif
#else
#ifndef __nothrow
#define __nothrow
#endif
#endif

#if defined(_WIN64) && !defined(_MSC_VER)
#undef USE_MINGW_SETJMP_TWO_ARGS
#define USE_MINGW_SETJMP_TWO_ARGS
#endif

/* Disable deprecation for now! */
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE_CORE

#define __crt_typefix(ctype)

#ifndef _CRT_UNUSED
#define _CRT_UNUSED(x) (void)x
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define ATTRIB_NORETURN
#define _DECLSPEC_INTRIN_TYPE __declspec(intrin_type)
#else
#define ATTRIB_NORETURN DECLSPEC_NORETURN
#define _DECLSPEC_INTRIN_TYPE
#endif

/* Define to a function attribute for Microsoft hotpatch assembly prefix. */
#ifndef DECLSPEC_HOTPATCH
#if defined(_MSC_VER) || defined(__clang__)
/* FIXME: http://llvm.org/bugs/show_bug.cgi?id=20888 */
#define DECLSPEC_HOTPATCH
#else
#define DECLSPEC_HOTPATCH __attribute__((__ms_hook_prologue__))
#endif
#endif /* DECLSPEC_HOTPATCH */

#ifndef __INTRIN_INLINE
#  define __INTRIN_INLINE extern __inline__ __attribute__((__always_inline__,__gnu_inline__,artificial))
#endif

#ifndef HAS_BUILTIN
#  ifdef __clang__
#    define HAS_BUILTIN(x) __has_builtin(x)
#  else
#    define HAS_BUILTIN(x) 0
#  endif
#endif

#ifdef __cplusplus
#  define __mingw_ovr  inline __cdecl
#elif defined (__GNUC__)
#  define __mingw_ovr static \
      __attribute__ ((__unused__)) __inline__ __cdecl
#else
#  define __mingw_ovr static __cdecl
#endif /* __cplusplus */

#if defined(__cplusplus) && !defined(_MSC_VER)
#define __nullptr nullptr
#endif

#include "_mingw_mac.h"

#endif /* !_INC_MINGW */

