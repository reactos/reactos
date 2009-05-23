/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_MINGW
#define _INC_MINGW

#define _INTEGRAL_MAX_BITS 64

// ROS HACK!
#ifndef _WIN64
 #ifndef _USE_32BIT_TIME_T
  #define _USE_32BIT_TIME_T
 #endif
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
#else
# define __attribute__(x) /* nothing */
#endif

#if defined (__GNUC__) && defined (__GNUC_MINOR__)
#define __MINGW_GNUC_PREREQ(major, minor) \
  (__GNUC__ > (major) \
   || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define __MINGW_GNUC_PREREQ(major, minor)  0
#endif

#if !defined (_MSC_VER)
#define __MINGW_MSC_PREREQ(major, minor)  0
#endif

#define USE___UUIDOF	0

#ifdef __cplusplus
# define __CRT_INLINE inline
#else
# if ( __MINGW_GNUC_PREREQ(4, 3)  &&  __STDC_VERSION__ >= 199901L)
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
#  define __unaligned __attribute((packed))
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

#if __MINGW_GNUC_PREREQ (3, 0)
#define __MINGW_ATTRIB_MALLOC __attribute__ ((__malloc__))
#define __MINGW_ATTRIB_PURE __attribute__ ((__pure__))
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
#elif __MINGW_MSC_PREREQ(12, 0)
#define __MINGW_ATTRIB_DEPRECATED __declspec(deprecated)
#else
#define __MINGW_ATTRIB_DEPRECATED
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
#ifdef _WIN64
   typedef int __int128 __attribute__ ((mode (TI)));
# endif
# define __ptr32
# define __ptr64
# define __forceinline extern __inline __attribute((always_inline))
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

#ifdef _WIN64
#undef USE_MINGW_SETJMP_TWO_ARGS
#define USE_MINGW_SETJMP_TWO_ARGS
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
  typedef __builtin_va_list __gnuc_va_list;
#endif

#ifndef _VA_LIST_DEFINED
#define _VA_LIST_DEFINED
  typedef __gnuc_va_list va_list;
#endif

/* Diable deprecation for now! */
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE_CORE
#ifdef __WINESRC__
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if (defined(_MSC_VER) && __STDC__)// || !defined(__WINESRC__)
#define NO_OLDNAMES
#endif

#ifdef __cplusplus
}
#endif

#define __crt_typefix(ctype)

#ifndef _CRT_UNUSED
#define _CRT_UNUSED(x) (void)x
#endif

/* These are here for intrin.h */
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
#ifdef _WIN64
  typedef unsigned __int64 size_t;
#else
  typedef unsigned int size_t;
#endif
#endif

#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#ifdef _WIN64
  typedef unsigned __int64 uintptr_t;
#else
  typedef unsigned int uintptr_t;
#endif
#endif

#include <mingw32/intrin.h>

#endif /* !_INC_MINGW */

