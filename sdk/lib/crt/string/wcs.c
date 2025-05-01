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

#else
#include "config.h"
#include "wine/port.h"

#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "msvcrt.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);
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
 *		_wcsupr_s (MSVCRT.@)
 *
 */
INT CDECL MSVCRT__wcsupr_s( MSVCRT_wchar_t* str, MSVCRT_size_t n )
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
            *ret = conv;
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

typedef struct pf_flags_t
{
    char Sign, LeftAlign, Alternate, PadZero;
    int FieldLength, Precision;
    char IntegerLength, IntegerDouble;
    char WideString;
    char Format;
} pf_flags;

static inline BOOL pf_is_auto_grow(pf_output *out)
{
    return (out->unicode) ? !!out->grow.W : !!out->grow.A;
}

static inline int pf_check_auto_grow(pf_output *out, unsigned delta)
{
    if (pf_is_auto_grow(out) && out->used + delta > out->len)
    {
        out->len = max(out->len * 2, out->used + delta);
        if (out->unicode)
        {
            if (out->buf.W != out->grow.W)
                out->buf.W = MSVCRT_realloc(out->buf.W, out->len * sizeof(WCHAR));
            else
                out->buf.W = MSVCRT_malloc(out->len * sizeof(WCHAR));
            if (!out->buf.W) return -1;
        }
        else
        {
            if (out->buf.A != out->grow.A)
                out->buf.A = MSVCRT_realloc(out->buf.A, out->len * sizeof(char));
            else
                out->buf.A = MSVCRT_malloc(out->len * sizeof(char));
            if (!out->buf.A) return -1;
        }
    }
    return 0;
}

/*
 * writes a string of characters to the output
 * returns -1 if the string doesn't fit in the output buffer
 * return the length of the string if all characters were written
 */
static inline int pf_output_stringW( pf_output *out, LPCWSTR str, int len )
{
    int space;

    if( len < 0 )
        len = strlenW( str );
    if( out->unicode )
    {
        LPWSTR p;

        if(pf_check_auto_grow(out, len) == -1) return -1;
        space = out->len - out->used;
        p = out->buf.W + out->used;
        if( space >= len )
        {
            if (out->buf.W) memcpy( p, str, len*sizeof(WCHAR) );
            out->used += len;
            return len;
        }
        if( space > 0 && out->buf.W )
            memcpy( p, str, space*sizeof(WCHAR) );
        out->used += len;
    }
    else
    {
        int n = WideCharToMultiByte( CP_ACP, 0, str, len, NULL, 0, NULL, NULL );
        LPSTR p;

        if(pf_check_auto_grow(out, n) == -1) return -1;
        space = out->len - out->used;
        p = out->buf.A + out->used;
        if( space >= n )
        {
            if (out->buf.A) WideCharToMultiByte( CP_ACP, 0, str, len, p, n, NULL, NULL );
            out->used += n;
            return len;
        }
        if( space > 0 && out->buf.A )
            WideCharToMultiByte( CP_ACP, 0, str, len, p, space, NULL, NULL );
        out->used += n;
    }
    return -1;
}

static inline int pf_output_stringA( pf_output *out, LPCSTR str, int len )
{
    int space;

    if( len < 0 )
        len = strlen( str );
    if( !out->unicode )
    {
        LPSTR p;

        if (pf_check_auto_grow(out, len) == -1) return -1;
        p = out->buf.A + out->used;
        space = out->len - out->used;
        if( space >= len )
        {
            if (out->buf.A) memcpy( p, str, len );
            out->used += len;
            return len;
        }
        if( space > 0 && out->buf.A )
            memcpy( p, str, space );
        out->used += len;
    }
    else
    {
        int n = MultiByteToWideChar( CP_ACP, 0, str, len, NULL, 0 );
        LPWSTR p;

        if (pf_check_auto_grow(out, n) == -1) return -1;
        p = out->buf.W + out->used;
        space = out->len - out->used;
        if( space >= n )
        {
            if (out->buf.W) MultiByteToWideChar( CP_ACP, 0, str, len, p, n );
            out->used += n;
            return len;
        }
        if( space > 0 && out->buf.W )
            MultiByteToWideChar( CP_ACP, 0, str, len, p, space );
        out->used += n;
    }
    return -1;
}

/* pf_fill: takes care of signs, alignment, zero and field padding */
static inline int pf_fill( pf_output *out, int len, pf_flags *flags, char left )
{
    int i, r = 0;

    if( flags->Sign && !( flags->Format == 'd' || flags->Format == 'i' ) )
        flags->Sign = 0;

    if( left && flags->Sign )
    {
        flags->FieldLength--;
        if( flags->PadZero )
            r = pf_output_stringA( out, &flags->Sign, 1 );
    }

    if( ( !left &&  flags->LeftAlign ) || 
        (  left && !flags->LeftAlign ))
    {
        for( i=0; (i<(flags->FieldLength-len)) && (r>=0); i++ )
        {
            if( left && flags->PadZero )
                r = pf_output_stringA( out, "0", 1 );
            else
                r = pf_output_stringA( out, " ", 1 );
        }
    }

    if( left && flags->Sign && !flags->PadZero )
        r = pf_output_stringA( out, &flags->Sign, 1 );

    return r;
}

static inline int pf_output_format_W( pf_output *out, LPCWSTR str,
                                      int len, pf_flags *flags )
{
    int r = 0;

    if( len < 0 )
        len = strlenW( str );

    if (flags->Precision >= 0 && flags->Precision < len)
        len = flags->Precision;

    r = pf_fill( out, len, flags, 1 );

    if( r>=0 )
        r = pf_output_stringW( out, str, len );

    if( r>=0 )
        r = pf_fill( out, len, flags, 0 );

    return r;
}

static inline int pf_output_format_A( pf_output *out, LPCSTR str,
                                      int len, pf_flags *flags )
{
    int r = 0;

    if( len < 0 )
        len = strlen( str );

    if (flags->Precision >= 0 && flags->Precision < len)
        len = flags->Precision;

    r = pf_fill( out, len, flags, 1 );

    if( r>=0 )
        r = pf_output_stringA( out, str, len );

    if( r>=0 )
        r = pf_fill( out, len, flags, 0 );

    return r;
}

static int pf_handle_string_format( pf_output *out, const void* str, int len,
                             pf_flags *flags, BOOL capital_letter)
{
     if(str == NULL)  /* catch NULL pointer */
        return pf_output_format_A( out, "(null)", -1, flags);

     /* prefixes take priority over %c,%s vs. %C,%S, so we handle them first */
    if(flags->WideString || flags->IntegerLength == 'l')
        return pf_output_format_W( out, str, len, flags);
    if(flags->IntegerLength == 'h')
        return pf_output_format_A( out, str, len, flags);

    /* %s,%c ->  chars in ansi functions & wchars in unicode
     * %S,%C -> wchars in ansi functions &  chars in unicode */
    if( capital_letter == out->unicode) /* either both TRUE or both FALSE */
        return pf_output_format_A( out, str, len, flags);
    else
        return pf_output_format_W( out, str, len, flags);
}

static inline BOOL pf_is_integer_format( char fmt )
{
    static const char float_fmts[] = "diouxX";
    if (!fmt)
        return FALSE;
    return strchr( float_fmts, fmt ) ? TRUE : FALSE;
}

static inline BOOL pf_is_double_format( char fmt )
{
    static const char float_fmts[] = "aeEfgG";
    if (!fmt)
        return FALSE;
    return strchr( float_fmts, fmt ) ? TRUE : FALSE;
}

static inline BOOL pf_is_valid_format( char fmt )
{
    static const char float_fmts[] = "acCdeEfgGinouxX";
    if (!fmt)
        return FALSE;
    return strchr( float_fmts, fmt ) ? TRUE : FALSE;
}

static void pf_rebuild_format_string( char *p, pf_flags *flags )
{
    *p++ = '%';
    if( flags->Sign )
        *p++ = flags->Sign;
    if( flags->LeftAlign )
        *p++ = flags->LeftAlign;
    if( flags->Alternate )
        *p++ = flags->Alternate;
    if( flags->PadZero )
        *p++ = flags->PadZero;
    if( flags->FieldLength )
    {
        sprintf(p, "%d", flags->FieldLength);
        p += strlen(p);
    }
    if( flags->Precision >= 0 )
    {
        sprintf(p, ".%d", flags->Precision);
        p += strlen(p);
    }
    *p++ = flags->Format;
    *p++ = 0;
}

/* pf_integer_conv:  prints x to buf, including alternate formats and
   additional precision digits, but not field characters or the sign */
static void pf_integer_conv( char *buf, int buf_len, pf_flags *flags,
                             LONGLONG x )
{
    unsigned int base;
    const char *digits;

    int i, j, k;
    char number[40], *tmp = number;

    if( buf_len > sizeof number )
        tmp = HeapAlloc( GetProcessHeap(), 0, buf_len );

    base = 10;
    if( flags->Format == 'o' )
        base = 8;
    else if( flags->Format == 'x' || flags->Format == 'X' )
        base = 16;

    if( flags->Format == 'X' )
        digits = "0123456789ABCDEFX";
    else
        digits = "0123456789abcdefx";

    if( x < 0 && ( flags->Format == 'd' || flags->Format == 'i' ) )
    {
        x = -x;
        flags->Sign = '-';
    }

    /* Do conversion (backwards) */
    i = 0;
    if( x == 0 && flags->Precision )
        tmp[i++] = '0';
    else
        while( x != 0 )
        {
            j = (ULONGLONG) x % base;
            x = (ULONGLONG) x / base;
            tmp[i++] = digits[j];
        }
    k = flags->Precision - i;
    while( k-- > 0 )
        tmp[i++] = '0';
    if( flags->Alternate )
    {
        if( base == 16 )
        {
            tmp[i++] = digits[16];
            tmp[i++] = '0';
        }
        else if( base == 8 && tmp[i-1] != '0' )
            tmp[i++] = '0';
    }

    /* Reverse for buf */
    j = 0;
    while( i-- > 0 )
        buf[j++] = tmp[i];
    buf[j] = '\0';

    /* Adjust precision so pf_fill won't truncate the number later */
    flags->Precision = strlen( buf );

    if( tmp != number )
        HeapFree( GetProcessHeap(), 0, tmp );

    return;
}

/* pf_fixup_exponent: convert a string containing a 2 digit exponent
   to 3 digits, accounting for padding, in place. Needed to match
   the native printf's which always use 3 digits. */
static void pf_fixup_exponent( char *buf )
{
    char* tmp = buf;

    while (tmp[0] && toupper(tmp[0]) != 'E')
        tmp++;

    if (tmp[0] && (tmp[1] == '+' || tmp[1] == '-') &&
        isdigit(tmp[2]) && isdigit(tmp[3]))
    {
        char final;

        if (isdigit(tmp[4]))
            return; /* Exponent already 3 digits */

        /* We have a 2 digit exponent. Prepend '0' to make it 3 */
        tmp += 2;
        final = tmp[2];
        tmp[2] = tmp[1];
        tmp[1] = tmp[0];
        tmp[0] = '0';
        if (final == '\0')
        {
            /* We didn't expand into trailing space, so this string isn't left
             * justified. Terminate the string and strip a ' ' at the start of
             * the string if there is one (as there may be if the string is
             * right justified).
             */
            tmp[3] = '\0';
            if (buf[0] == ' ')
                memmove(buf, buf + 1, (tmp - buf) + 3);
        }
        /* Otherwise, we expanded into trailing space -> nothing to do */
    }
}

/*********************************************************************
 *  pf_vsnprintf  (INTERNAL)
 *
 *  implements both A and W vsnprintf functions
 */
int pf_vsnprintf( pf_output *out, const WCHAR *format,
                  MSVCRT__locale_t locale, BOOL valid, __ms_va_list valist )
{
    int r;
    LPCWSTR q, p = format;
    pf_flags flags;

    if(!locale)
        locale = get_locale();

    TRACE("format is %s\n",debugstr_w(format));
    while (*p)
    {
        q = strchrW( p, '%' );

        /* there's no % characters left, output the rest of the string */
        if( !q )
        {
            r = pf_output_stringW(out, p, -1);
            if( r<0 )
                return r;
            p += r;
            continue;
        }

        /* there's characters before the %, output them */
        if( q != p )
        {
            r = pf_output_stringW(out, p, q - p);
            if( r<0 )
                return r;
            p = q;
        }

        /* we must be at a % now, skip over it */
        assert( *p == '%' );
        p++;

        /* output a single % character */
        if( *p == '%' )
        {
            r = pf_output_stringW(out, p++, 1);
            if( r<0 )
                return r;
            continue;
        }

        /* parse the flags */
        memset( &flags, 0, sizeof flags );
        while (*p)
        {
            if( *p == '+' || *p == ' ' )
            {
                if ( flags.Sign != '+' )
                    flags.Sign = *p;
            }
            else if( *p == '-' )
                flags.LeftAlign = *p;
            else if( *p == '0' )
                flags.PadZero = *p;
            else if( *p == '#' )
                flags.Alternate = *p;
            else
                break;
            p++;
        }

        /* deal with the field width specifier */
        flags.FieldLength = 0;
        if( *p == '*' )
        {
            flags.FieldLength = va_arg( valist, int );
            if (flags.FieldLength < 0)
            {
                flags.LeftAlign = '-';
                flags.FieldLength = -flags.FieldLength;
            }
            p++;
        }
        else while( isdigit(*p) )
        {
            flags.FieldLength *= 10;
            flags.FieldLength += *p++ - '0';
        }

        /* deal with precision */
        flags.Precision = -1;
        if( *p == '.' )
        {
            flags.Precision = 0;
            p++;
            if( *p == '*' )
            {
                flags.Precision = va_arg( valist, int );
                p++;
            }
            else while( isdigit(*p) )
            {
                flags.Precision *= 10;
                flags.Precision += *p++ - '0';
            }
        }

        /* deal with integer width modifier */
        while( *p )
        {
            if( *p == 'l' && *(p+1) == 'l' )
            {
                flags.IntegerDouble++;
                p += 2;
            }
            else if( *p == 'h' || *p == 'l' || *p == 'L' )
            {
                flags.IntegerLength = *p;
                p++;
            }
            else if( *p == 'I' )
            {
                if( *(p+1) == '6' && *(p+2) == '4' )
                {
                    flags.IntegerDouble++;
                    p += 3;
                }
                else if( *(p+1) == '3' && *(p+2) == '2' )
                    p += 3;
                else if( isdigit(*(p+1)) || *(p+1) == 0 )
                    break;
                else
                    p++;
            }
            else if( *p == 'w' )
                flags.WideString = *p++;
            else if( *p == 'F' )
                p++; /* ignore */
            else
                break;
        }

        flags.Format = *p;
        r = 0;

        if (flags.Format == '$')
        {
            FIXME("Positional parameters are not supported (%s)\n", wine_dbgstr_w(format));
            return -1;
        }
        /* output a string */
        if(  flags.Format == 's' || flags.Format == 'S' )
            r = pf_handle_string_format( out, va_arg(valist, const void*), -1,
                                         &flags, (flags.Format == 'S') );

        /* output a single character */
        else if( flags.Format == 'c' || flags.Format == 'C' )
        {
            INT ch = va_arg( valist, int );

            r = pf_handle_string_format( out, &ch, 1, &flags, (flags.Format == 'C') );
        }

        /* output a pointer */
        else if( flags.Format == 'p' )
        {
            char pointer[32];
            void *ptr = va_arg( valist, void * );

            flags.PadZero = 0;
            if( flags.Alternate )
                sprintf(pointer, "0X%0*lX", 2 * (int)sizeof(ptr), (ULONG_PTR)ptr);
            else
                sprintf(pointer, "%0*lX", 2 * (int)sizeof(ptr), (ULONG_PTR)ptr);
            r = pf_output_format_A( out, pointer, -1, &flags );
        }

        /* deal with %n */
        else if( flags.Format == 'n' )
        {
            int *x = va_arg(valist, int *);
            *x = out->used;
        }

        /* deal with 64-bit integers */
        else if( pf_is_integer_format( flags.Format ) && flags.IntegerDouble )
        {
            char number[40], *x = number;

            /* Estimate largest possible required buffer size:
               * Chooses the larger of the field or precision
               * Includes extra bytes: 1 byte for null, 1 byte for sign,
                 4 bytes for exponent, 2 bytes for alternate formats, 1 byte 
                 for a decimal, and 1 byte for an additional float digit. */
            int x_len = ((flags.FieldLength > flags.Precision) ? 
                        flags.FieldLength : flags.Precision) + 10;

            if( x_len >= sizeof number)
                x = HeapAlloc( GetProcessHeap(), 0, x_len );

            pf_integer_conv( x, x_len, &flags, va_arg(valist, LONGLONG) );

            r = pf_output_format_A( out, x, -1, &flags );
            if( x != number )
                HeapFree( GetProcessHeap(), 0, x );
        }

        /* deal with integers and floats using libc's printf */
        else if( pf_is_valid_format( flags.Format ) )
        {
            char fmt[20], number[40], *x = number, *decimal_point;

            /* Estimate largest possible required buffer size:
               * Chooses the larger of the field or precision
               * Includes extra bytes: 1 byte for null, 1 byte for sign,
                 4 bytes for exponent, 2 bytes for alternate formats, 1 byte 
                 for a decimal, and 1 byte for an additional float digit. */
            int x_len = ((flags.FieldLength > flags.Precision) ? 
                        flags.FieldLength : flags.Precision) + 10;

            if( x_len >= sizeof number)
                x = HeapAlloc( GetProcessHeap(), 0, x_len );

            pf_rebuild_format_string( fmt, &flags );

            if( pf_is_double_format( flags.Format ) )
            {
                sprintf( x, fmt, va_arg(valist, double) );
                if (toupper(flags.Format) == 'E' || toupper(flags.Format) == 'G')
                    pf_fixup_exponent( x );
            }
            else
                sprintf( x, fmt, va_arg(valist, int) );

            decimal_point = strchr(x, '.');
            if(decimal_point)
                *decimal_point = *locale->locinfo->lconv->decimal_point;

            r = pf_output_stringA( out, x, -1 );
            if( x != number )
                HeapFree( GetProcessHeap(), 0, x );
            if(r < 0)
                return r;
        }
        else
        {
            if( valid )
            {
                MSVCRT__invalid_parameter( NULL, NULL, NULL, 0, 0 );
                *MSVCRT__errno() = MSVCRT_EINVAL;
                return -1;
            }

            continue;
        }

        if( r<0 )
            return r;
        p++;
    }

    /* check we reached the end, and null terminate the string */
    assert( *p == 0 );
    pf_output_stringW( out, p, 1 );

    return out->used - 1;
}

/*********************************************************************
 * vsnprintf_internal (INTERNAL)
 */
static inline int vsnprintf_internal( char *str, MSVCRT_size_t len, const char *format,
        MSVCRT__locale_t locale, BOOL valid, __ms_va_list valist )
{
    DWORD sz;
    LPWSTR formatW = NULL;
    pf_output out;
    int r;

    out.unicode = FALSE;
    out.buf.A = str;
    out.grow.A = NULL;
    out.used = 0;
    out.len = len;

    sz = MultiByteToWideChar( CP_ACP, 0, format, -1, NULL, 0 );
    formatW = HeapAlloc( GetProcessHeap(), 0, sz*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, format, -1, formatW, sz );

    r = pf_vsnprintf( &out, formatW, locale, valid, valist );

    HeapFree( GetProcessHeap(), 0, formatW );

    return r;
}

/*********************************************************************
 *              _vsnprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf( char *str, MSVCRT_size_t len,
                            const char *format, __ms_va_list valist )
{
    return vsnprintf_internal(str, len, format, NULL, FALSE, valist);
}

/*********************************************************************
*		_vsnprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_l( char *str, MSVCRT_size_t len, const char *format,
                            MSVCRT__locale_t locale, __ms_va_list valist )
{
    return vsnprintf_internal(str, len, format, locale, FALSE, valist);
}

/*********************************************************************
 *		_vsnprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnprintf_s_l( char *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const char *format,
        MSVCRT__locale_t locale, __ms_va_list valist )
{
    int len, ret;

    if(sizeOfBuffer<count+1 || count==-1)
        len = sizeOfBuffer;
    else
        len = count+1;

    ret = vsnprintf_internal(str, len, format, locale, TRUE, valist);

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
 * vsnwprintf_internal (INTERNAL)
 */
static inline int vsnwprintf_internal(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale, BOOL valid,
        __ms_va_list valist)
{
    pf_output out;

    out.unicode = TRUE;
    out.buf.W = str;
    out.grow.W = NULL;
    out.used = 0;
    out.len = len;

    return pf_vsnprintf( &out, format, locale, valid, valist );
}

/*********************************************************************
 *              _vsnwprintf (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, __ms_va_list valist)
{
    return vsnwprintf_internal(str, len, format, NULL, FALSE, valist);
}

/*********************************************************************
 *              _vsnwprintf_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_l(MSVCRT_wchar_t *str, MSVCRT_size_t len,
        const MSVCRT_wchar_t *format, MSVCRT__locale_t locale,
        __ms_va_list valist)
{
        return vsnwprintf_internal(str, len, format, locale, FALSE, valist);
}

/*********************************************************************
 *              _vsnwprintf_s_l (MSVCRT.@)
 */
int CDECL MSVCRT_vsnwprintf_s_l( MSVCRT_wchar_t *str, MSVCRT_size_t sizeOfBuffer,
        MSVCRT_size_t count, const MSVCRT_wchar_t *format,
        MSVCRT__locale_t locale, __ms_va_list valist)
{
    int len, ret;

    len = sizeOfBuffer;
    if(count!=-1 && len>count+1)
        len = count+1;

    ret = vsnwprintf_internal(str, len, format, locale, TRUE, valist);

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
 *		wctomb (MSVCRT.@)
 */
INT CDECL MSVCRT_wctomb( char *dst, MSVCRT_wchar_t ch )
{
  return WideCharToMultiByte( CP_ACP, 0, &ch, 1, dst, 6, NULL, NULL );
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
