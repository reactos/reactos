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

#if defined(__GNUC__) || defined(__clang__)
#define _crt_va_start(v,l)	__builtin_va_start(v,l)
#define _crt_va_arg(v,l)	__builtin_va_arg(v,l)
#define _crt_va_end(v)	__builtin_va_end(v)
#define __va_copy(d,s)	__builtin_va_copy(d,s)
#elif defined(_MSC_VER)

#if defined(_M_IX86)
#define _crt_va_start(v,l)	((void)((v) = (va_list)_ADDRESSOF(l) + _INTSIZEOF(l)))
#define _crt_va_arg(v,l)	(*(l *)(((v) += _INTSIZEOF(l)) - _INTSIZEOF(l)))
#define _crt_va_end(v)	((void)((v) = (va_list)0))
#define __va_copy(d,s)	((void)((d) = (s)))
#elif defined(_M_AMD64)
#define _PTRSIZEOF(n) ((sizeof(n) + sizeof(void*) - 1) & ~(sizeof(void*) - 1))
#define _ISSTRUCT(t) ((sizeof(t) > sizeof(void*)) || (sizeof(t) & (sizeof(t)-1)) != 0)
#define _crt_va_start(v,l)	((void)((v) = (va_list)_ADDRESSOF(l) + _PTRSIZEOF(l)))
#define _crt_va_arg(v,t)	(_ISSTRUCT(t) ? \
                            (**(t**)(((v) += sizeof(void*)) - sizeof(void*))) : \
                            (*(t*)(((v) += sizeof(void*)) - sizeof(void*))))
#define _crt_va_end(v)	((void)((v) = (va_list)0))
#define __va_copy(d,s)	((void)((d) = (s)))
#elif defined(_M_ARM)
#ifdef  __cplusplus
  extern void __cdecl __va_start(va_list*, ...);
  #define _crt_va_start(ap,v) __va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), _ADDRESSOF(v))
#else
  #define _crt_va_start(ap,v) (ap = (va_list)_ADDRESSOF(v) + _SLOTSIZEOF(v))
#endif
#define _crt_va_arg(ap,t) (*(t*)((ap += _SLOTSIZEOF(t) + _APALIGN(t,ap))  - _SLOTSIZEOF(t)))
#define _crt_va_end(ap)      ( ap = (va_list)0 )
#define __va_copy(d,s)	((void)((d) = (s)))
#elif defined(_M_ARM64)
extern void __cdecl __va_start(va_list*, ...);
#define __crt_va_start(ap,v) ((void)(__va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), __alignof(v), _ADDRESSOF(v))))
#define __crt_va_arg(ap, t)                                                \
    ((sizeof(t) > (2 * sizeof(__int64)))                                   \
        ? **(t**)((ap += sizeof(__int64)) - sizeof(__int64))               \
        : *(t*)((ap += _SLOTSIZEOF(t) + _APALIGN(t,ap)) - _SLOTSIZEOF(t)))
#define __crt_va_end(ap)       ((void)(ap = (va_list)0))
#define __va_copy(d,s)	((void)((d) = (s)))
#else //if defined(_M_IA64) || defined(_M_CEE)
#error Please implement me
#endif

#endif

#if !defined(va_copy) && (!defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L)
#define va_copy(d,s)	__va_copy((d),(s))
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)
#endif
