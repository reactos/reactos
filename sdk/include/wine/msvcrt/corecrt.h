/*
 * CRT definitions
 *
 * Copyright 2000 Francois Gouget.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_CORECRT_H
#define __WINE_CORECRT_H

#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifdef __WINE_CONFIG_H
# error You cannot use config.h with msvcrt
#endif

#ifdef WINE_UNIX_LIB
# error msvcrt headers cannot be used in Unix code
#endif

#ifndef _WIN32
# define _WIN32
#endif

#ifndef WIN32
# define WIN32
#endif

#if (defined(__x86_64__) || defined(__powerpc64__) || defined(__aarch64__)) && !defined(_WIN64)
#define _WIN64
#endif

#ifndef _MSVCR_VER
# define _MSVCR_VER 140
#endif

#if !defined(_UCRT) && _MSVCR_VER >= 140
# define _UCRT
#endif

#include <sal.h>

#ifndef _MSC_VER
#  ifndef __int8
#    define __int8  char
#  endif
#  ifndef __int16
#    define __int16 short
#  endif
#  ifndef __int32
#    define __int32 int
#  endif
#  ifndef __int64
#    if defined(_WIN64) && !defined(__MINGW64__)
#      define __int64 long
#    else
#      define __int64 long long
#    endif
#  endif
#endif

#ifndef NULL
#ifdef __cplusplus
#ifndef _WIN64
#define NULL 0
#else
#define NULL 0LL
#endif
#else
#define NULL  ((void *)0)
#endif
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef __has_declspec_attribute
# if defined(_MSC_VER)
#  define __has_declspec_attribute(x) 1
# else
#  define __has_declspec_attribute(x) 0
# endif
#endif

#if !defined(_MSC_VER) && !defined(__MINGW32__)
# undef __stdcall
# undef __cdecl
# if defined(__i386__) && defined(__GNUC__)
#  if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)) || defined(__APPLE__)
#   define __stdcall __attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))
#   define __cdecl __attribute__((__cdecl__)) __attribute__((__force_align_arg_pointer__))
#  else
#   define __stdcall __attribute__((__stdcall__))
#   define __cdecl __attribute__((__cdecl__))
#  endif
# elif defined(__x86_64__) && defined(__GNUC__)
#  if __has_attribute(__force_align_arg_pointer__)
#   define __stdcall __attribute__((ms_abi)) __attribute__((__force_align_arg_pointer__))
#  else
#   define __stdcall __attribute__((ms_abi))
#  endif
#  define __cdecl __stdcall
# else
#  define __stdcall
#  define __cdecl
# endif
#endif  /* _MSC_VER || __MINGW32__ */

#ifndef DECLSPEC_NORETURN
# ifdef __GNUC__
#  define DECLSPEC_NORETURN __attribute__((noreturn))
# elif __has_declspec_attribute(noreturn) && !defined(MIDL_PASS)
#  define DECLSPEC_NORETURN __declspec(noreturn)
# else
#  define DECLSPEC_NORETURN
# endif
#endif

#ifndef DECLSPEC_ALIGN
# ifdef __GNUC__
#  define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))
# elif __has_declspec_attribute(align) &&  !defined(MIDL_PASS)
#  define DECLSPEC_ALIGN(x) __declspec(align(x))
# else
#  define DECLSPEC_ALIGN(x)
# endif
#endif

#ifndef _ACRTIMP
# ifdef _CRTIMP
#  define _ACRTIMP _CRTIMP
# elif __has_declspec_attribute(dllimport)
#  define _ACRTIMP __declspec(dllimport)
# elif defined(__MINGW32__) || defined(__CYGWIN__)
#  define _ACRTIMP __attribute__((dllimport))
# else
#  define _ACRTIMP
# endif
#endif

#define _ARGMAX 100
#define _CRT_INT_MAX 0x7fffffff

#ifndef _MSVCRT_LONG_DEFINED
#define _MSVCRT_LONG_DEFINED
/* we need 32-bit longs even on 64-bit */
#ifdef __LP64__
typedef int __msvcrt_long;
typedef unsigned int __msvcrt_ulong;
#else
typedef long __msvcrt_long;
typedef unsigned long __msvcrt_ulong;
#endif
#endif

#ifndef _INTPTR_T_DEFINED
#ifdef  _WIN64
typedef __int64 intptr_t;
#else
typedef int intptr_t;
#endif
#define _INTPTR_T_DEFINED
#endif

#ifndef _UINTPTR_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 uintptr_t;
#else
typedef unsigned int uintptr_t;
#endif
#define _UINTPTR_T_DEFINED
#endif

#ifndef _PTRDIFF_T_DEFINED
#ifdef _WIN64
typedef __int64 ptrdiff_t;
#else
typedef int ptrdiff_t;
#endif
#define _PTRDIFF_T_DEFINED
#endif

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _TIME32_T_DEFINED
typedef __msvcrt_long __time32_t;
#define _TIME32_T_DEFINED
#endif

#ifndef _TIME64_T_DEFINED
typedef __int64 DECLSPEC_ALIGN(8) __time64_t;
#define _TIME64_T_DEFINED
#endif

#ifdef _USE_32BIT_TIME_T
# ifdef _WIN64
#  error You cannot use 32-bit time_t in Win64
# endif
#elif !defined(_WIN64)
# define _USE_32BIT_TIME_T
#endif

#ifndef _TIME_T_DEFINED
#ifdef _USE_32BIT_TIME_T
typedef __time32_t time_t;
#else
typedef __time64_t time_t;
#endif
#define _TIME_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#define _WCHAR_T_DEFINED
#endif

#ifndef _WCTYPE_T_DEFINED
typedef unsigned short  wint_t;
typedef unsigned short  wctype_t;
#define _WCTYPE_T_DEFINED
#endif

#ifndef _ERRNO_T_DEFINED
typedef int errno_t;
#define _ERRNO_T_DEFINED
#endif

struct threadlocaleinfostruct;
struct threadmbcinfostruct;
typedef struct threadlocaleinfostruct *pthreadlocinfo;
typedef struct threadmbcinfostruct *pthreadmbcinfo;

typedef struct localeinfo_struct
{
    pthreadlocinfo locinfo;
    pthreadmbcinfo mbcinfo;
} _locale_tstruct, *_locale_t;

#ifndef _TAGLC_ID_DEFINED
typedef struct tagLC_ID {
    unsigned short wLanguage;
    unsigned short wCountry;
    unsigned short wCodePage;
} LC_ID, *LPLC_ID;
#define _TAGLC_ID_DEFINED
#endif

#ifndef _THREADLOCALEINFO
typedef struct threadlocaleinfostruct {
#if _MSVCR_VER >= 140
    unsigned short *pctype;
    int mb_cur_max;
    unsigned int lc_codepage;
#endif

    int refcount;
#if _MSVCR_VER < 140
    unsigned int lc_codepage;
#endif
    unsigned int lc_collate_cp;
    __msvcrt_ulong lc_handle[6];
    LC_ID lc_id[6];
    struct {
        char *locale;
        wchar_t *wlocale;
        int *refcount;
        int *wrefcount;
    } lc_category[6];
    int lc_clike;
#if _MSVCR_VER < 140
    int mb_cur_max;
#endif
    int *lconv_intl_refcount;
    int *lconv_num_refcount;
    int *lconv_mon_refcount;
    struct lconv *lconv;
    int *ctype1_refcount;
    unsigned short *ctype1;
#if _MSVCR_VER < 140
    unsigned short *pctype;
#endif
    const unsigned char *pclmap;
    const unsigned char *pcumap;
    struct __lc_time_data *lc_time_curr;
#if _MSVCR_VER >= 110
    wchar_t *lc_name[6];
#endif
} threadlocinfo;
#define _THREADLOCALEINFO
#endif

#if defined(__MINGW32__) || (defined(_MSC_VER) && defined(__clang__))
#define __WINE_CRT_PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#define __WINE_CRT_SCANF_ATTR(fmt,args)  __attribute__((format (scanf,fmt,args)))
#else
#define __WINE_CRT_PRINTF_ATTR(fmt,args)
#define __WINE_CRT_SCANF_ATTR(fmt,args)
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
#define __WINE_ALLOC_SIZE(...) __attribute__((__alloc_size__(__VA_ARGS__)))
#else
#define __WINE_ALLOC_SIZE(...)
#endif

#if defined(__GNUC__) && (__GNUC__ > 10)
#define __WINE_DEALLOC(...) __attribute__((malloc (__VA_ARGS__)))
#else
#define __WINE_DEALLOC(...)
#endif

#if defined(__GNUC__) && (__GNUC__ > 2)
#define __WINE_MALLOC __attribute__((malloc))
#else
#define __WINE_MALLOC
#endif

#endif /* __WINE_CORECRT_H */
