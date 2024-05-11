
#pragma once

#include <_mingw.h>
#include <sal.h>
#include <vadefs.h>

#define _CRT_STRINGIZE_(_Value) #_Value
#define _CRT_STRINGIZE(_Value) _CRT_STRINGIZE_(_Value)

#define _CRT_WIDE_(_String) L ## _String
#define _CRT_WIDE(_String) _CRT_WIDE_(_String)

#ifndef _CRTIMP
 #ifdef CRTDLL /* Defined for ntdll, crtdll, msvcrt, etc */
  #define _CRTIMP
 #elif defined(_DLL)
  #define _CRTIMP __declspec(dllimport)
 #else /* !CRTDLL && !_DLL */
  #define _CRTIMP
 #endif /* CRTDLL || _DLL */
#endif /* !_CRTIMP */

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
