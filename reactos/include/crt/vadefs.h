/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_VADEFS
#define _INC_VADEFS

#ifndef _WIN32
#error Only Win32 target is supported!
#endif

#include <crtdefs.h>

#undef _CRT_PACKING
#define _CRT_PACKING 8
#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#ifndef __uintptr_t_defined
#define __uintptr_t_defined
#undef uintptr_t
#ifdef _WIN64
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
  typedef unsigned int uintptr_t __attribute__ ((mode (DI)));
#else
  typedef unsigned __int64 uintptr_t;
#endif
#else
  typedef unsigned long uintptr_t;
#endif
#endif
#endif

#ifdef __GNUC__
#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
  typedef __builtin_va_list __gnuc_va_list;
#endif
#endif

#ifndef _VA_LIST_DEFINED
#define _VA_LIST_DEFINED
#if defined(__GNUC__)
  typedef __gnuc_va_list va_list;
#elif defined(_MSC_VER)
  typedef char *  va_list;
#endif
#endif

#ifdef __cplusplus
#define _ADDRESSOF(v) (&reinterpret_cast<const char &>(v))
#else
#define _ADDRESSOF(v) (&(v))
#endif

#if defined(__ia64__)
#define _VA_ALIGN 8
#define _SLOTSIZEOF(t) ((sizeof(t) + _VA_ALIGN - 1) & ~(_VA_ALIGN - 1))

#define _VA_STRUCT_ALIGN 16

#define _ALIGNOF(ap) ((((ap)+_VA_STRUCT_ALIGN - 1) & ~(_VA_STRUCT_ALIGN -1)) - (ap))
#define _APALIGN(t,ap) (__alignof(t) > 8 ? _ALIGNOF((uintptr_t) ap) : 0)
#else
#define _SLOTSIZEOF(t) (sizeof(t))
#define _APALIGN(t,ap) (__alignof(t))
#endif

#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))

#if defined(__GNUC__)
#define _crt_va_start(v,l)	__builtin_va_start(v,l)
#define _crt_va_arg(v,l)	__builtin_va_arg(v,l)
#define _crt_va_end(v)	__builtin_va_end(v)
#define __va_copy(d,s)	__builtin_va_copy(d,s)
#elif defined(_MSC_VER)

#if defined(_M_IA64) || defined(_M_AMD64) || defined(_M_CEE)
#error Please implement me
#endif

#define _crt_va_start(v,l)	((void)((v) = (va_list)_ADDRESSOF(l) + _INTSIZEOF(l)))
#define _crt_va_arg(v,l)	(*(l *)(((v) += _INTSIZEOF(l)) - _INTSIZEOF(l)))
#define _crt_va_end(v)	((void)((v) = (va_list)0))
#define __va_copy(d,s)	((void)((d) = (s)))
#endif

#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L
#define va_copy(d,s)	__va_copy((d),(s))
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)
#endif
