/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Common definitions
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#define _VCRUNTIME_H

#include <_mingw.h>
#include <sal.h>
#include <vadefs.h>

#define _CRT_STRINGIZE_(_Value) #_Value
#define _CRT_STRINGIZE(_Value) _CRT_STRINGIZE_(_Value)

#define _CRT_WIDE_(_String) L ## _String
#define _CRT_WIDE(_String) _CRT_WIDE_(_String)

#define _CRT_CONCATENATE_(a, b) a ## b
#define _CRT_CONCATENATE(a, b)  _CRT_CONCATENATE_(a, b)

#define _CRT_UNPARENTHESIZE_(...) __VA_ARGS__
#define _CRT_UNPARENTHESIZE(...)  _CRT_UNPARENTHESIZE_ __VA_ARGS__

#if !defined(_MSC_VER) && !defined(__pragma)
#define __pragma(x) _Pragma(_CRT_STRINGIZE(x))
#endif

#ifdef __cplusplus
    #define _CRT_BEGIN_C_HEADER \
        __pragma(pack(push, _CRT_PACKING_IDENTIFIER, _CRT_PACKING)) \
        extern "C" {
    #define _CRT_END_C_HEADER \
        } \
        __pragma(pack(pop, _CRT_PACKING_IDENTIFIER))
#else
    #define _CRT_BEGIN_C_HEADER \
        __pragma(pack(push, _CRT_PACKING_IDENTIFIER, _CRT_PACKING))
    #define _CRT_END_C_HEADER \
        __pragma(pack(pop, _CRT_PACKING_IDENTIFIER))
#endif

_CRT_BEGIN_C_HEADER

#ifndef _CRTIMP
 #ifdef CRTDLL /* Defined for ntdll, crtdll, msvcrt, etc */
  #define _CRTIMP
 #elif defined(_DLL)
  #define _CRTIMP __declspec(dllimport)
 #else /* !CRTDLL && !_DLL */
  #define _CRTIMP
 #endif /* CRTDLL || _DLL */
#endif /* !_CRTIMP */

#ifndef _VCRTIMP
 #ifndef _VCRT_DEFINED_CRTIMP
  #define _VCRTIMP _CRTIMP
 #elif defined(_VCRT_BUILD) && defined(CRTDLL) && !defined(_VCRT_SAT_1)
  #define _VCRTIMP __declspec(dllexport)
 #else
  #define _VCRTIMP
 #endif
#endif

#ifndef __CRTDECL
#define __CRTDECL __cdecl
#endif

#ifndef _CONST_RETURN
#ifdef __cplusplus
#define _CONST_RETURN const
#define _CRT_CONST_CORRECT_OVERLOADS
#else
#define _CONST_RETURN
#endif
#endif

#ifdef _BUILD_STD_MODULE
 #define _VCRT_EXPORT_STD export
#else
 #define _VCRT_EXPORT_STD
#endif

#ifdef __GNUC__
#define _CRT_DEPRECATE_TEXT(_Text) __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define _CRT_DEPRECATE_TEXT(_Text) __declspec(deprecated(_Text))
#else
#define _CRT_DEPRECATE_TEXT(_Text)
#endif

#ifndef _CRT_INSECURE_DEPRECATE
# ifdef _CRT_SECURE_NO_DEPRECATE
#  define _CRT_INSECURE_DEPRECATE(_Replacement)
# else
#  define _CRT_INSECURE_DEPRECATE(_Replacement) \
    _CRT_DEPRECATE_TEXT("This may be unsafe, Try " #_Replacement " instead!")
# endif
#endif

#ifndef _CRT_INSECURE_DEPRECATE_MEMORY
#define _CRT_INSECURE_DEPRECATE_MEMORY(_Replacement)
#endif

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
#undef size_t
#ifdef _WIN64
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
  typedef unsigned int size_t __attribute__ ((mode (DI)));
#else
  typedef unsigned __int64 size_t;
#endif
#else
  typedef unsigned int size_t;
#endif
#endif

#ifndef _INTPTR_T_DEFINED
#define _INTPTR_T_DEFINED
#ifndef __intptr_t_defined
#define __intptr_t_defined
#undef intptr_t
#ifdef _WIN64
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
  typedef int intptr_t __attribute__ ((mode (DI)));
#else
  typedef __int64 intptr_t;
#endif
#else
  typedef int intptr_t;
#endif
#endif
#endif

#ifndef _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_DEFINED
#ifndef _PTRDIFF_T_
#undef ptrdiff_t
#ifdef _WIN64
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
  typedef int ptrdiff_t __attribute__ ((mode (DI)));
#else
  __MINGW_EXTENSION typedef __int64 ptrdiff_t;
#endif
#else
  typedef int ptrdiff_t;
#endif
#endif
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#if defined(_MSC_VER) || !defined(__cplusplus)
  typedef unsigned short wchar_t;
#endif
#endif

#ifndef _CRT_ALIGN
 #if defined (__midl) || defined(__WIDL__)
  #define _CRT_ALIGN(x)
 #elif defined(_MSC_VER)
  #define _CRT_ALIGN(x) __declspec(align(x))
 #else
  #define _CRT_ALIGN(x) __attribute__ ((aligned(x)))
 #endif
#endif

#if defined (__midl) || defined(__WIDL__)
  #define _VCRT_ALIGN(x)
#elif defined(_MSC_VER)
  #define _CRT_ALIGN(x) __declspec(align(x))
#else
  #define _VCRT_ALIGN(x) __attribute__ ((aligned(x)))
#endif

#if defined __cplusplus
    typedef bool  __vcrt_bool;
#elif defined __midl
    typedef char __vcrt_bool;
#else
    typedef _Bool __vcrt_bool;
#endif

#ifndef _HAS_NODISCARD
 #ifndef __has_cpp_attribute
  #define _HAS_NODISCARD 0
 #elif __has_cpp_attribute(nodiscard) >= 201603L
  #define _HAS_NODISCARD 1
 #else
  #define _HAS_NODISCARD 0
 #endif
#endif // _HAS_NODISCARD

#if _HAS_NODISCARD
 #define _NODISCARD [[nodiscard]]
#else
 #define _NODISCARD
#endif // _HAS_NODISCARD

#if defined _M_X64 || defined _M_ARM || defined _M_ARM64
    #define _UNALIGNED __unaligned
#else
    #define _UNALIGNED
#endif

#ifdef __cplusplus
    // Safer than the C definition, as it ensures that the argument is not a pointer
    extern "C++" template<typename _T, size_t _Size>
    char (*__crt_countof_helper(_UNALIGNED _T(&_Array)[_Size]))[_Size];
    #define __crt_countof(_Array) sizeof(*__crt_countof_helper(_Array))
#else
    #define __crt_countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

void __cdecl __security_init_cookie(void);
extern uintptr_t __security_cookie;

_CRT_END_C_HEADER
