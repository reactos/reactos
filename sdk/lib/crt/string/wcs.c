/*
 * PROJECT:         ReactOS CRT library
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            lib/sdk/crt/string/wcs.c
 * PURPOSE:         wcs* CRT functions
 * PROGRAMMERS:     Wine team
 *                  Ported to ReactOS by Aleksey Bragin (aleksey@reactos.org)
 */

/*
 * msvcrt.dll wide-char functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffiths
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
#ifdef __REACTOS__
#include <precomp.h>
#include <assert.h>

#include <internal/wine/msvcrt.h>

#include "wine/unicode.h"
#undef sprintf
#undef wsprintf
#undef snprintf
#undef vsnprintf
#undef vprintf
#undef vwprintf

#define MSVCRT_malloc malloc
#define MSVCRT_wcstod wcstod
#define MSVCRT_vsnprintf vsnprintf
#define MSVCRT_vsprintf vsprintf
#define MSVCRT__snprintf _snprintf
#define MSVCRT_vsnwprintf vsnwprintf
#define MSVCRT__snwprintf _snwprintf
#define MSVCRT_sprintf sprintf
#define MSVCRT_swprintf swprintf
#define MSVCRT_vswprintf vswprintf
#define MSVCRT_wcscoll wcscoll
#define MSVCRT_wcspbrk wcspbrk
#define MSVCRT_wcscpy_s wcscpy_s
#define MSVCRT_wcstok wcstok
#define MSVCRT_wcscat_s wcscat_s
#define MSVCRT_wcsncat_s wcsncat_s
#define MSVCRT_wcsncpy_s wcsncpy_s
#define MSVCRT__wcsnset _wcsnset
#define MSVCRT__wcsupr_s _wcsupr_s
#define MSVCRT__vsnprintf _vsnprintf
#define MSVCRT__vsnwprintf _vsnwprintf
#define MSVCRT__errno _errno
#define MSVCRT__invalid_parameter _invalid_parameter
#define isinf(x) (!_finite(x))

struct _str_ctx_a { size_t len; char *buf; };
struct _str_ctx_w { size_t len; wchar_t *buf; };
#else
#include "config.h"
#include "wine/port.h"

#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "msvcrt.h"
#include "winnls.h"
#include "wtypes.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#include "printf.h"
#define PRINTF_WIDE
#include "printf.h"
#undef PRINTF_WIDE

#endif

#ifdef _MSC_VER
#pragma function(_wcsset)
#endif

#ifndef _LIBCNT_
/*********************************************************************
 *		_wcsdup (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL _wcsdup( const MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = NULL;
  if (str)
  {
    MSVCRT_size_t size = (strlenW(str) + 1) * sizeof(MSVCRT_wchar_t);
    ret = MSVCRT_malloc( size );
    if (ret) memcpy( ret, str, size );
  }
  return ret;
}
/*********************************************************************
 *		_wcsicoll (MSVCRT.@)
 */
INT CDECL _wcsicoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
  /* FIXME: handle collates */
  return strcmpiW( str1, str2 );
}
#endif

/*********************************************************************
 *		_wcsnicoll (MSVCRT.@)
 */
INT CDECL _wcsnicoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2, MSVCRT_size_t count )
{
  /* FIXME: handle collates */
  return strncmpiW( str1, str2, count );
}

/*********************************************************************
 *		_wcsnset (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wcsnset( MSVCRT_wchar_t* str, MSVCRT_wchar_t c, MSVCRT_size_t n )
{
  MSVCRT_wchar_t* ret = str;
  while ((n-- > 0) && *str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL _wcsrev( MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = str;
  MSVCRT_wchar_t* end = str + strlenW(str) - 1;
  while (end > str)
  {
    MSVCRT_wchar_t t = *end;
    *end--  = *str;
    *str++  = t;
  }
  return ret;
}

#ifndef _LIBCNT_
/*********************************************************************
 *		_wcsset (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL _wcsset( MSVCRT_wchar_t* str, MSVCRT_wchar_t c )
{
  MSVCRT_wchar_t* ret = str;
  while (*str) *str++ = c;
  return ret;
}

/******************************************************************
 *		_wcsupr_s_l (MSVCRT.@)
 */
int CDECL MSVCRT__wcsupr_s_l( MSVCRT_wchar_t* str, MSVCRT_size_t n,
   MSVCRT__locale_t locale )
{
  MSVCRT_wchar_t* ptr = str;

  if (!str || !n)
  {
    if (str) *str = '\0';
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
  }

  while (n--)
  {
    if (!*ptr) return 0;
    /* FIXME: add locale support */
    *ptr = toupperW(*ptr);
    ptr++;
  }

  /* MSDN claims that the function should return and set errno to
   * ERANGE, which doesn't seem to be true based on the tests. */
  *str = '\0';
  *MSVCRT__errno() = MSVCRT_EINVAL;
  return MSVCRT_EINVAL;
}

/******************************************************************
 *		_wcsupr_s (MSVCRT.@)
 *
 */
INT CDECL MSVCRT__wcsupr_s( MSVCRT_wchar_t* str, MSVCRT_size_t n )
{
  return MSVCRT__wcsupr_s_l( str, n, NULL );
}

/******************************************************************
 *		_wcslwr_s (MSVCRT.@)
 */
int CDECL MSVCRT__wcslwr_s( MSVCRT_wchar_t* str, MSVCRT_size_t n )
{
  MSVCRT_wchar_t* ptr = str;

  if (!str || !n)
  {
    if (str) *str = '\0';
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
  }

  while (n--)
  {
    if (!*ptr) return 0;
    *ptr = tolowerW(*ptr);
    ptr++;
  }

  /* MSDN claims that the function should return and set errno to
   * ERANGE, which doesn't seem to be true based on the tests. */
  *str = '\0';
  *MSVCRT__errno() = MSVCRT_EINVAL;
  return MSVCRT_EINVAL;
}

/*********************************************************************
 * _wcstod_l - not exported in native msvcrt
 */
double CDECL MSVCRT__wcstod_l(const MSVCRT_wchar_t* str, MSVCRT_wchar_t** end,
        MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    unsigned __int64 d=0, hlp;
    unsigned fpcontrol;
    int exp=0, sign=1;
    const MSVCRT_wchar_t *p;
    double ret;
    BOOL found_digit = FALSE;

    if (!MSVCRT_CHECK_PMT(str != NULL)) {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return 0;
    }

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    p = str;
    while(isspaceW(*p))
        p++;

    if(*p == '-') {
        sign = -1;
        p++;
    } else  if(*p == '+')
        p++;

    while(isdigitW(*p)) {
        found_digit = TRUE;
        hlp = d*10+*(p++)-'0';
        if(d>MSVCRT_UI64_MAX/10 || hlp<d) {
            exp++;
            break;
        } else
            d = hlp;
    }
    while(isdigitW(*p)) {
        exp++;
        p++;
    }
    if(*p == *locinfo->lconv->decimal_point)
        p++;

    while(isdigitW(*p)) {
        found_digit = TRUE;
        hlp = d*10+*(p++)-'0';
        if(d>MSVCRT_UI64_MAX/10 || hlp<d)
            break;

        d = hlp;
        exp--;
    }
    while(isdigitW(*p))
        p++;

    if(!found_digit) {
        if(end)
            *end = (MSVCRT_wchar_t*)str;
        return 0.0;
    }

    if(*p=='e' || *p=='E' || *p=='d' || *p=='D') {
        int e=0, s=1;

        p++;
        if(*p == '-') {
            s = -1;
            p++;
        } else if(*p == '+')
            p++;

        if(isdigitW(*p)) {
            while(isdigitW(*p)) {
                if(e>INT_MAX/10 || (e=e*10+*p-'0')<0)
                    e = INT_MAX;
                p++;
            }
            e *= s;

            if(exp<0 && e<0 && exp+e>=0) exp = INT_MIN;
            else if(exp>0 && e>0 && exp+e<0) exp = INT_MAX;
            else exp += e;
        } else {
            if(*p=='-' || *p=='+')
                p--;
            p--;
        }
    }

    fpcontrol = _control87(0, 0);
    _control87(MSVCRT__EM_DENORMAL|MSVCRT__EM_INVALID|MSVCRT__EM_ZERODIVIDE
            |MSVCRT__EM_OVERFLOW|MSVCRT__EM_UNDERFLOW|MSVCRT__EM_INEXACT, 0xffffffff);

    if(exp>0)
        ret = (double)sign*d*pow(10, exp);
    else
        ret = (double)sign*d/pow(10, -exp);

    _control87(fpcontrol, 0xffffffff);

    if((d && ret==0.0) || isinf(ret))
        *MSVCRT__errno() = MSVCRT_ERANGE;

    if(end)
        *end = (MSVCRT_wchar_t*)p;

    return ret;
}

/*********************************************************************
 *		_wcstombs_l (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT__wcstombs_l(char *mbstr, const MSVCRT_wchar_t *wcstr,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    char default_char = '\0';
    MSVCRT_size_t tmp = 0;
    BOOL used_default;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!mbstr)
        return WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                wcstr, -1, NULL, 0, &default_char, &used_default)-1;

    while(*wcstr) {
        char buf[3];
        MSVCRT_size_t i, size;

        size = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                wcstr, 1, buf, 3, &default_char, &used_default);
        if(used_default)
            return -1;
        if(tmp+size > count)
            return tmp;

        for(i=0; i<size; i++)
            mbstr[tmp++] = buf[i];
        wcstr++;
    }

    if(tmp < count)
        mbstr[tmp] = '\0';
    return tmp;
}

/*********************************************************************
 *		wcstombs (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcstombs(char *mbstr, const MSVCRT_wchar_t *wcstr,
        MSVCRT_size_t count)
{
    return MSVCRT__wcstombs_l(mbstr, wcstr, count, NULL);
}

/*********************************************************************
 *		_wcstombs_s_l (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT__wcstombs_s_l(MSVCRT_size_t *ret, char *mbstr,
        MSVCRT_size_t size, const MSVCRT_wchar_t *wcstr,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_size_t conv;

    if(!mbstr && !size) {
        conv = MSVCRT__wcstombs_l(NULL, wcstr, 0, locale);
        if(ret)
            *ret = conv+1;
        return 0;
    }

    if (!MSVCRT_CHECK_PMT(wcstr != NULL) || !MSVCRT_CHECK_PMT(mbstr != NULL)) {
        if(mbstr && size)
            mbstr[0] = '\0';
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    if(count==MSVCRT__TRUNCATE || size<count)
        conv = size;
    else
        conv = count;

    conv = MSVCRT__wcstombs_l(mbstr, wcstr, conv, locale);
    if(conv<size)
        mbstr[conv++] = '\0';
    else if(conv==size && (count==MSVCRT__TRUNCATE || mbstr[conv-1]=='\0'))
        mbstr[conv-1] = '\0';
    else {
        MSVCRT_INVALID_PMT("mbstr[size] is too small", MSVCRT_ERANGE);
        if(size)
            mbstr[0] = '\0';
        *MSVCRT__errno() = MSVCRT_ERANGE;
        return MSVCRT_ERANGE;
    }

    if(ret)
        *ret = conv;
    return 0;
}

/*********************************************************************
 *		wcstombs_s (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcstombs_s(MSVCRT_size_t *ret, char *mbstr,
        MSVCRT_size_t size, const MSVCRT_wchar_t *wcstr, MSVCRT_size_t count)
{
    return MSVCRT__wcstombs_s_l(ret, mbstr, size, wcstr, count, NULL);
}

/*********************************************************************
 *		wcstod (MSVCRT.@)
 */
double CDECL MSVCRT_wcstod(const MSVCRT_wchar_t* lpszStr, MSVCRT_wchar_t** end)
{
    return MSVCRT__wcstod_l(lpszStr, end, NULL);
}

/*********************************************************************
 *		_wtof (MSVCRT.@)
 */
double CDECL MSVCRT__wtof(const MSVCRT_wchar_t *str)
{
    return MSVCRT__wcstod_l(str, NULL, NULL);
}

/*********************************************************************
 *		_wtof_l (MSVCRT.@)
 */
double CDECL MSVCRT__wtof_l(const MSVCRT_wchar_t *str, MSVCRT__locale_t locale)
{
    return MSVCRT__wcstod_l(str, NULL, locale);
}
#endif

#ifndef __REACTOS__

/*********************************************************************
 * arg_clbk_valist (INTERNAL)
 */
printf_arg arg_clbk_valist(void *ctx, int arg_pos, int type, __ms_va_list *valist)
{
    printf_arg ret;

    if(type == VT_I8)
        ret.get_longlong = va_arg(*valist, LONGLONG);
    else if(type == VT_INT)
        ret.get_int = va_arg(*valist, int);
    else if(type == VT_R8)
        ret.get_double = va_arg(*valist, double);
    else if(type == VT_PTR)
        ret.get_ptr = va_arg(*valist, void*);
    else {
        ERR("Incorrect type\n");
        ret.get_int = 0;
    }

    return ret;
}

/*********************************************************************
 *              _vsnprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf( char *str, MSVCRT_size_t len,
                            const char *format, __ms_va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, NULL, FALSE, FALSE,
            arg_clbk_valist, NULL, valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
*		_vsnprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_l( char *str, MSVCRT_size_t len, const char *format,
                            MSVCRT__locale_t locale, __ms_va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, locale, FALSE, FALSE,
            arg_clbk_valist, NULL, valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *		_vsnprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_s_l( char *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const char *format,
        MSVCRT__locale_t locale, __ms_va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx;
    int len, ret;

    if(sizeOfBuffer<count+1 || count==-1)
        len = sizeOfBuffer;
    else
        len = count+1;

    ctx.len = len;
    ctx.buf = str;
    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, locale, FALSE, TRUE,
            arg_clbk_valist, NULL, valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);

    if(ret<0 || ret==len) {
        if(count!=MSVCRT__TRUNCATE && count>sizeOfBuffer) {
            MSVCRT_INVALID_PMT("str[sizeOfBuffer] is too small");
            *MSVCRT__errno() = MSVCRT_ERANGE;
            memset(str, 0, sizeOfBuffer);
        } else
            str[len-1] = '\0';

        return -1;
    }

    return ret;
}

/*********************************************************************
 *              _vsnprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_s( char *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const char *format, __ms_va_list valist )
{
    return MSVCRT_vsnprintf_s_l(str,sizeOfBuffer, count, format, NULL, valist);
}

/*********************************************************************
 *		vsprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf( char *str, const char *format, __ms_va_list valist)
{
    return MSVCRT_vsnprintf(str, INT_MAX, format, valist);
}

/*********************************************************************
 *		vsprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vsprintf_s( char *str, MSVCRT_size_t num, const char *format, __ms_va_list valist)
{
    return MSVCRT_vsnprintf(str, num, format, valist);
}

/*********************************************************************
 *		_vscprintf (MSVCRT.@)
 */
int CDECL _vscprintf( const char *format, __ms_va_list valist )
{
    return MSVCRT_vsnprintf( NULL, INT_MAX, format, valist );
}

/*********************************************************************
 *		_snprintf (MSVCRT.@)
 */
int CDECL MSVCRT__snprintf(char *str, unsigned int len, const char *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnprintf(str, len, format, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT__snprintf_s(char *str, unsigned int len, unsigned int count,
    const char *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnprintf_s_l(str, len, count, format, NULL, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _scprintf (MSVCRT.@)
 */
int CDECL MSVCRT__scprintf(const char *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = _vscprintf(format, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *              _vsnwprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    static const MSVCRT_wchar_t nullbyte = '\0';
    struct _str_ctx_w ctx = {len, str};
    int ret;

    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, NULL, FALSE, FALSE,
            arg_clbk_valist, NULL, valist);
    puts_clbk_str_w(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *              _vsnwprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_l(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale,
        __ms_va_list valist)
{
    static const MSVCRT_wchar_t nullbyte = '\0';
    struct _str_ctx_w ctx = {len, str};
    int ret;

    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, locale, FALSE, FALSE,
            arg_clbk_valist, NULL, valist);
    puts_clbk_str_w(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *              _vsnwprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_s_l( MSVCRT_wchar_t *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, __ms_va_list valist)
{
    static const MSVCRT_wchar_t nullbyte = '\0';
    struct _str_ctx_w ctx;
    int len, ret;

    len = sizeOfBuffer;
    if(count!=-1 && len>count+1)
        len = count+1;

    ctx.len = len;
    ctx.buf = str;
    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, locale, FALSE, TRUE,
            arg_clbk_valist, NULL, valist);
    puts_clbk_str_w(&ctx, 1, &nullbyte);

    if(ret<0 || ret==len) {
        if(count!=MSVCRT__TRUNCATE && count>sizeOfBuffer) {
            MSVCRT_INVALID_PMT("str[sizeOfBuffer] is too small");
            *MSVCRT__errno() = MSVCRT_ERANGE;
            memset(str, 0, sizeOfBuffer*sizeof(MSVCRT_wchar_t));
        } else
            str[len-1] = '\0';

        return -1;
    }

    return ret;
}

/*********************************************************************
 *              _vsnwprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_s(MSVCRT_wchar_t *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    return MSVCRT_vsnwprintf_s_l(str, sizeOfBuffer, count,
            format, NULL, valist);
}

/*********************************************************************
 *		_snwprintf (MSVCRT.@)
 */
int CDECL MSVCRT__snwprintf( MSVCRT_wchar_t *str, unsigned int len, const MSVCRT_wchar_t *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnwprintf(str, len, format, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snwprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT__snwprintf_s( MSVCRT_wchar_t *str, unsigned int len, unsigned int count,
    const MSVCRT_wchar_t *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = MSVCRT_vsnwprintf_s_l(str, len, count, format, NULL, valist);
    __ms_va_end(valist);
    return retval;
}

/*********************************************************************
 *		sprintf (MSVCRT.@)
 */
int CDECL MSVCRT_sprintf( char *str, const char *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start( ap, format );
    r = MSVCRT_vsnprintf( str, INT_MAX, format, ap );
    __ms_va_end( ap );
    return r;
}

/*********************************************************************
 *		sprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_sprintf_s( char *str, MSVCRT_size_t num, const char *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start( ap, format );
    r = MSVCRT_vsnprintf( str, num, format, ap );
    __ms_va_end( ap );
    return r;
}

/*********************************************************************
 *		_scwprintf (MSVCRT.@)
 */
int CDECL MSVCRT__scwprintf( const MSVCRT_wchar_t *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start( ap, format );
    r = MSVCRT_vsnwprintf( NULL, INT_MAX, format, ap );
    __ms_va_end( ap );
    return r;
}

/*********************************************************************
 *		swprintf (MSVCRT.@)
 */
int CDECL MSVCRT_swprintf( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start( ap, format );
    r = MSVCRT_vsnwprintf( str, INT_MAX, format, ap );
    __ms_va_end( ap );
    return r;
}

/*********************************************************************
 *		swprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_swprintf_s(MSVCRT_wchar_t *str, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t *format, ... )
{
    __ms_va_list ap;
    int r;

    __ms_va_start(ap, format);
    r = MSVCRT_vsnwprintf_s(str, numberOfElements, INT_MAX, format, ap);
    __ms_va_end(ap);

    return r;
}

/*********************************************************************
 *		vswprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf( MSVCRT_wchar_t* str, const MSVCRT_wchar_t* format, __ms_va_list args )
{
    return MSVCRT_vsnwprintf( str, INT_MAX, format, args );
}

/*********************************************************************
 *		_vscwprintf (MSVCRT.@)
 */
int CDECL _vscwprintf( const MSVCRT_wchar_t *format, __ms_va_list args )
{
    return MSVCRT_vsnwprintf( NULL, INT_MAX, format, args );
}

/*********************************************************************
 *		vswprintf_s (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_s(MSVCRT_wchar_t* str, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t* format, __ms_va_list args)
{
    return MSVCRT_vsnwprintf_s(str, numberOfElements, INT_MAX, format, args );
}

/*********************************************************************
 *              _vswprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vswprintf_s_l(MSVCRT_wchar_t* str, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t* format, MSVCRT__locale_t locale, __ms_va_list args)
{
    return MSVCRT_vsnwprintf_s_l(str, numberOfElements, INT_MAX,
            format, locale, args );
}
#endif // !__REACTOS__

/*********************************************************************
 *		wcscoll (MSVCRT.@)
 */
int CDECL MSVCRT_wcscoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
  /* FIXME: handle collates */
  return strcmpW( str1, str2 );
}

/*********************************************************************
 *		wcspbrk (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT_wcspbrk( const MSVCRT_wchar_t* str, const MSVCRT_wchar_t* accept )
{
  const MSVCRT_wchar_t* p;
  while (*str)
  {
    for (p = accept; *p; p++) if (*p == *str) return (MSVCRT_wchar_t*)str;
      str++;
  }
  return NULL;
}

#ifndef __REACTOS__
/*********************************************************************
 *		wcstok_s  (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL wcstok_s( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *delim,
                                 MSVCRT_wchar_t **next_token )
{
    MSVCRT_wchar_t *ret;

    if (!MSVCRT_CHECK_PMT(delim != NULL) || !MSVCRT_CHECK_PMT(next_token != NULL) ||
        !MSVCRT_CHECK_PMT(str != NULL || *next_token != NULL))
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }
    if (!str) str = *next_token;

    while (*str && strchrW( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !strchrW( delim, *str )) str++;
    if (*str) *str++ = 0;
    *next_token = str;
    return ret;
}
#endif // !__REACTOS__

#ifndef _LIBCNT_

/*********************************************************************
 *		wctomb (MSVCRT.@)
 */
/*********************************************************************
 *         wctomb (MSVCRT.@)
 */
INT CDECL wctomb( char *dst, wchar_t ch )
{
    BOOL error;
    INT size;

    size = WideCharToMultiByte(get_locinfo()->lc_codepage, 0, &ch, 1, dst, dst ? 6 : 0, NULL, &error);
    if(!size || error) {
        *_errno() = EINVAL;
        return EOF;
    }
    return size;
}

/*********************************************************************
 * wcsrtombs_l (INTERNAL)
 */
static size_t CDECL wcsrtombs_l(char *mbstr, const wchar_t **wcstr,
            size_t count, _locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    size_t tmp = 0;
    BOOL used_default;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = ((MSVCRT__locale_t)locale)->locinfo;

    if(!locinfo->lc_codepage) {
        size_t i;

        if(!mbstr)
            return strlenW(*wcstr);

        for(i=0; i<count; i++) {
            if((*wcstr)[i] > 255) {
                _set_errno(EILSEQ);
                return -1;
            }

            mbstr[i] = (*wcstr)[i];
            if(!(*wcstr)[i]) break;
        }
        return i;
    }

    if(!mbstr) {
        tmp = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                *wcstr, -1, NULL, 0, NULL, &used_default);
        if(!tmp || used_default) {
            _set_errno(EILSEQ);
            return -1;
        }
        return tmp-1;
    }

    while(**wcstr) {
        char buf[3];
        size_t i, size;

        size = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                *wcstr, 1, buf, 3, NULL, &used_default);
        if(!size || used_default) {
            _set_errno(EILSEQ);
            return -1;
        }
        if(tmp+size > count)
            return tmp;

        for(i=0; i<size; i++)
            mbstr[tmp++] = buf[i];
        (*wcstr)++;
    }

    if(tmp < count) {
        mbstr[tmp] = '\0';
        *wcstr = NULL;
    }
    return tmp;
}

/*********************************************************************
 *		_wcstombs_l (MSVCRT.@)
 */
size_t CDECL _wcstombs_l(char *mbstr, const wchar_t *wcstr, size_t count, _locale_t locale)
{
    return wcsrtombs_l(mbstr, &wcstr, count, locale);
}

/*********************************************************************
 *		wcstombs (MSVCRT.@)
 */
size_t CDECL wcstombs(char *mbstr, const wchar_t *wcstr, size_t count)
{
    return wcsrtombs_l(mbstr, &wcstr, count, NULL);
}
#endif

#ifndef __REACTOS__

/*********************************************************************
 *		wcstok  (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL MSVCRT_wcstok( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *delim )
{
    return wcstok_s(str, delim, &msvcrt_get_thread_data()->wcstok_next);
}

/*********************************************************************
 *		wctob (MSVCRT.@)
 */
INT CDECL MSVCRT_wctob( MSVCRT_wint_t wchar )
{
    char out;
    BOOL error;

    if(WideCharToMultiByte( get_locinfo()->lc_codepage, 0, &wchar, 1, &out, 1, NULL, &error ) && !error)
        return (INT)out;
    return MSVCRT_EOF;
}

/*********************************************************************
 *		wctomb (MSVCRT.@)
 */
INT CDECL MSVCRT_wctomb( char *dst, MSVCRT_wchar_t ch )
{
    return WideCharToMultiByte( get_locinfo()->lc_codepage, 0, &ch, 1, dst, 6, NULL, NULL );
}

/*********************************************************************
 *		iswalnum (MSVCRT.@)
 */
INT CDECL MSVCRT_iswalnum( MSVCRT_wchar_t wc )
{
    return isalnumW( wc );
}

/*********************************************************************
 *		iswalpha (MSVCRT.@)
 */
INT CDECL MSVCRT_iswalpha( MSVCRT_wchar_t wc )
{
    return isalphaW( wc );
}

/*********************************************************************
 *              iswalpha_l (MSVCRT.@)
 */
INT CDECL MSVCRT__iswalpha_l( MSVCRT_wchar_t wc, MSVCRT__locale_t locale )
{
    return isalphaW( wc );
}

/*********************************************************************
 *		iswcntrl (MSVCRT.@)
 */
INT CDECL MSVCRT_iswcntrl( MSVCRT_wchar_t wc )
{
    return iscntrlW( wc );
}

/*********************************************************************
 *		iswdigit (MSVCRT.@)
 */
INT CDECL MSVCRT_iswdigit( MSVCRT_wchar_t wc )
{
    return isdigitW( wc );
}

/*********************************************************************
 *		iswgraph (MSVCRT.@)
 */
INT CDECL MSVCRT_iswgraph( MSVCRT_wchar_t wc )
{
    return isgraphW( wc );
}

/*********************************************************************
 *		iswlower (MSVCRT.@)
 */
INT CDECL MSVCRT_iswlower( MSVCRT_wchar_t wc )
{
    return islowerW( wc );
}

/*********************************************************************
 *		iswprint (MSVCRT.@)
 */
INT CDECL MSVCRT_iswprint( MSVCRT_wchar_t wc )
{
    return isprintW( wc );
}

/*********************************************************************
 *		iswpunct (MSVCRT.@)
 */
INT CDECL MSVCRT_iswpunct( MSVCRT_wchar_t wc )
{
    return ispunctW( wc );
}

/*********************************************************************
 *		iswspace (MSVCRT.@)
 */
INT CDECL MSVCRT_iswspace( MSVCRT_wchar_t wc )
{
    return isspaceW( wc );
}

/*********************************************************************
 *		iswupper (MSVCRT.@)
 */
INT CDECL MSVCRT_iswupper( MSVCRT_wchar_t wc )
{
    return isupperW( wc );
}

/*********************************************************************
 *		iswxdigit (MSVCRT.@)
 */
INT CDECL MSVCRT_iswxdigit( MSVCRT_wchar_t wc )
{
    return isxdigitW( wc );
}

#endif // !__REACTOS__

/*********************************************************************
 *		wcscpy_s (MSVCRT.@)
 */
INT CDECL MSVCRT_wcscpy_s( MSVCRT_wchar_t* wcDest, MSVCRT_size_t numElement, const  MSVCRT_wchar_t *wcSrc)
{
    MSVCRT_size_t size = 0;

    if(!wcDest || !numElement)
        return MSVCRT_EINVAL;

    wcDest[0] = 0;

    if(!wcSrc)
    {
        return MSVCRT_EINVAL;
    }

    size = strlenW(wcSrc) + 1;

    if(size > numElement)
    {
        return MSVCRT_ERANGE;
    }

    memcpy( wcDest, wcSrc, size*sizeof(WCHAR) );

    return 0;
}

/******************************************************************
 *		wcsncpy_s (MSVCRT.@)
 */
INT CDECL MSVCRT_wcsncpy_s( MSVCRT_wchar_t* wcDest, MSVCRT_size_t numElement, const MSVCRT_wchar_t *wcSrc,
                            MSVCRT_size_t count )
{
    MSVCRT_size_t size = 0;

    if (!wcDest || !numElement)
        return MSVCRT_EINVAL;

    wcDest[0] = 0;

    if (!wcSrc)
    {
        return MSVCRT_EINVAL;
    }

    size = min(strlenW(wcSrc), count);

    if (size >= numElement)
    {
        return MSVCRT_ERANGE;
    }

    memcpy( wcDest, wcSrc, size*sizeof(WCHAR) );
    wcDest[size] = '\0';

    return 0;
}

/******************************************************************
 *		wcscat_s (MSVCRT.@)
 *
 */
INT CDECL MSVCRT_wcscat_s(MSVCRT_wchar_t* dst, MSVCRT_size_t elem, const MSVCRT_wchar_t* src)
{
    MSVCRT_wchar_t* ptr = dst;

    if (!dst || elem == 0) return MSVCRT_EINVAL;
    if (!src)
    {
        dst[0] = '\0';
        return MSVCRT_EINVAL;
    }

    /* seek to end of dst string (or elem if no end of string is found */
    while (ptr < dst + elem && *ptr != '\0') ptr++;
    while (ptr < dst + elem)
    {
        if ((*ptr++ = *src++) == '\0') return 0;
    }
    /* not enough space */
    dst[0] = '\0';
    return MSVCRT_ERANGE;
}

/*********************************************************************
 *  wcsncat_s (MSVCRT.@)
 *
 */
INT CDECL MSVCRT_wcsncat_s(MSVCRT_wchar_t *dst, MSVCRT_size_t elem,
        const MSVCRT_wchar_t *src, MSVCRT_size_t count)
{
    MSVCRT_size_t srclen;
    MSVCRT_wchar_t dststart;
    INT ret = 0;

    if (!MSVCRT_CHECK_PMT(dst != NULL) || !MSVCRT_CHECK_PMT(elem > 0))
    {
#ifndef _LIBCNT_
        *MSVCRT__errno() = MSVCRT_EINVAL;
#endif
        return MSVCRT_EINVAL;
    }
    if (!MSVCRT_CHECK_PMT(src != NULL || count == 0))
        return MSVCRT_EINVAL;
    if (count == 0)
        return 0;

    for (dststart = 0; dststart < elem; dststart++)
    {
        if (dst[dststart] == '\0')
            break;
    }
    if (dststart == elem)
    {
        MSVCRT_INVALID_PMT("dst[elem] is not NULL terminated\n", MSVCRT_EINVAL);
        return MSVCRT_EINVAL;
    }

    if (count == MSVCRT__TRUNCATE)
    {
        srclen = strlenW(src);
        if (srclen >= (elem - dststart))
        {
            srclen = elem - dststart - 1;
            ret = MSVCRT_STRUNCATE;
        }
    }
    else
        srclen = min(strlenW(src), count);
    if (srclen < (elem - dststart))
    {
        memcpy(&dst[dststart], src, srclen*sizeof(MSVCRT_wchar_t));
        dst[dststart+srclen] = '\0';
        return ret;
    }
    MSVCRT_INVALID_PMT("dst[elem] is too small", MSVCRT_ERANGE);
    dst[0] = '\0';
    return MSVCRT_ERANGE;
}

#ifndef _LIBCNT_
/*********************************************************************
 *  _wcstoi64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL MSVCRT__wcstoi64_l(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL) || !MSVCRT_CHECK_PMT(base == 0 || base >= 2) ||
        !MSVCRT_CHECK_PMT(base <= 36)) {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return 0;
    }

    while(isspaceW(*nptr)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && tolowerW(*(nptr+1))=='x') {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(*nptr=='0')
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        char cur = tolowerW(*nptr);
        int v;

        if(isdigitW(cur)) {
            if(cur >= '0'+base)
                break;
            v = cur-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }

        if(negative)
            v = -v;

        nptr++;

        if(!negative && (ret>MSVCRT_I64_MAX/base || ret*base>MSVCRT_I64_MAX-v)) {
            ret = MSVCRT_I64_MAX;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else if(negative && (ret<MSVCRT_I64_MIN/base || ret*base<MSVCRT_I64_MIN-v)) {
            ret = MSVCRT_I64_MIN;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (MSVCRT_wchar_t*)nptr;

    return ret;
}

/*********************************************************************
 *  _wcstoi64 (MSVCRT.@)
 */
__int64 CDECL MSVCRT__wcstoi64(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base)
{
    return MSVCRT__wcstoi64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _wcstoui64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL MSVCRT__wcstoui64_l(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base, MSVCRT__locale_t locale)
{
    BOOL negative = FALSE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL) || !MSVCRT_CHECK_PMT(base == 0 || base >= 2) ||
        !MSVCRT_CHECK_PMT(base <= 36)) {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return 0;
    }

    while(isspaceW(*nptr)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && tolowerW(*(nptr+1))=='x') {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(*nptr=='0')
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        char cur = tolowerW(*nptr);
        int v;

        if(isdigit(cur)) {
            if(cur >= '0'+base)
                break;
            v = *nptr-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }

        nptr++;

        if(ret>MSVCRT_UI64_MAX/base || ret*base>MSVCRT_UI64_MAX-v) {
            ret = MSVCRT_UI64_MAX;
            *MSVCRT__errno() = MSVCRT_ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (MSVCRT_wchar_t*)nptr;

    return negative ? -ret : ret;
}

/*********************************************************************
 *  _wcstoui64 (MSVCRT.@)
 */
unsigned __int64 CDECL MSVCRT__wcstoui64(const MSVCRT_wchar_t *nptr,
        MSVCRT_wchar_t **endptr, int base)
{
    return MSVCRT__wcstoui64_l(nptr, endptr, base, NULL);
}
#endif // !_LIBCNT_

/******************************************************************
 *  wcsnlen (MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_wcsnlen(const MSVCRT_wchar_t *s, MSVCRT_size_t maxlen)
{
    MSVCRT_size_t i;

    for (i = 0; i < maxlen; i++)
        if (!s[i]) break;
    return i;
}
