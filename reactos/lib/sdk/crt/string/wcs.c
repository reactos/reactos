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
#define MINGW_HAS_SECURE_API 1
#include <precomp.h>
#include <assert.h>
#include <locale.h>

#ifndef _LIBCNT_
#include <internal/wine/msvcrt.h>
#endif

#include "wine/unicode.h"


#undef sprintf
#undef wsprintf
#undef snprintf
#undef vsnprintf
#undef vprintf
#undef vwprintf

#ifdef _MSC_VER
#pragma function(_wcsset)
#endif

#ifndef _LIBCNT_
/*********************************************************************
 *		_wcsdup (MSVCRT.@)
 */
wchar_t* CDECL _wcsdup( const wchar_t* str )
{
  wchar_t* ret = NULL;
  if (str)
  {
    int size = (strlenW(str) + 1) * sizeof(wchar_t);
    ret = malloc( size );
    if (ret) memcpy( ret, str, size );
  }
  return ret;
}
/*********************************************************************
 *		_wcsicoll (MSVCRT.@)
 */
INT CDECL _wcsicoll( const wchar_t* str1, const wchar_t* str2 )
{
  /* FIXME: handle collates */
  return strcmpiW( str1, str2 );
}
#endif

/*********************************************************************
 *		_wcsnset (MSVCRT.@)
 */
wchar_t* CDECL _wcsnset( wchar_t* str, wchar_t c, size_t n )
{
  wchar_t* ret = str;
  while ((n-- > 0) && *str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
 */
wchar_t* CDECL _wcsrev( wchar_t* str )
{
  wchar_t* ret = str;
  wchar_t* end = str + strlenW(str) - 1;
  while (end > str)
  {
    wchar_t t = *end;
    *end--  = *str;
    *str++  = t;
  }
  return ret;
}

#ifndef _LIBCNT_
/*********************************************************************
 *		_wcsset (MSVCRT.@)
 */
wchar_t* CDECL _wcsset( wchar_t* str, wchar_t c )
{
  wchar_t* ret = str;
  while (*str) *str++ = c;
  return ret;
}

/******************************************************************
 *		_wcsupr_s (MSVCRT.@)
 *
 */
INT CDECL _wcsupr_s( wchar_t* str, size_t n )
{
  wchar_t* ptr = str;

  if (!str || !n)
  {
    if (str) *str = '\0';
    __set_errno(EINVAL);
    return EINVAL;
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
  __set_errno(EINVAL);
  return EINVAL;
}

/*********************************************************************
 * _wcstod_l - not exported in native msvcrt
 */
double CDECL _wcstod_l(const wchar_t* str, wchar_t** end,
        _locale_t locale)
{
    unsigned __int64 d=0, hlp;
    int exp=0, sign=1;
    const wchar_t *p;
    double ret;

    if(!str) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *_errno() = EINVAL;
        return 0;
    }

    if(!locale)
        locale = get_locale();

    p = str;
    while(isspaceW(*p))
        p++;

    if(*p == '-') {
        sign = -1;
        p++;
    } else  if(*p == '+')
        p++;

    while(isdigitW(*p)) {
        hlp = d*10+*(p++)-'0';
        if(d>UINT64_MAX/10 || hlp<d) {
            exp++;
            break;
        } else
            d = hlp;
    }
    while(isdigitW(*p)) {
        exp++;
        p++;
    }
    if(*p == *locale->locinfo->lconv->decimal_point)
        p++;

    while(isdigitW(*p)) {
        hlp = d*10+*(p++)-'0';
        if(d>UINT64_MAX/10 || hlp<d)
            break;

        d = hlp;
        exp--;
    }
    while(isdigitW(*p))
        p++;

    if(p == str) {
        if(end)
            *end = (wchar_t*)str;
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

    if(exp>0)
        ret = (double)sign*d*pow(10, exp);
    else
        ret = (double)sign*d/pow(10, -exp);

    if((d && ret==0.0) || isinf(ret))
        *_errno() = ERANGE;

    if(end)
        *end = (wchar_t*)p;

    return ret;
}

/*********************************************************************
 *		_wcstombs_s_l (MSVCRT.@)
 */
int CDECL _wcstombs_s_l(size_t *ret, char *mbstr,
        size_t size, const wchar_t *wcstr,
        size_t count, _locale_t locale)
{
    char default_char = '\0', *p;
    int hlp, len;

    if(!size)
        return 0;

    if(!mbstr || !wcstr) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        if(mbstr)
            *mbstr = '\0';
        *_errno() = EINVAL;
        return EINVAL;
    }

    if(!locale)
        locale = get_locale();

    if(size<=count)
        len = size;
    else if(count==_TRUNCATE)
        len = size-1;
    else
        len = count;

    p = mbstr;
    *ret = 0;
    while(1) {
        if(!len)
            break;

        if(*wcstr == '\0') {
            *p = '\0';
            break;
        }

        hlp = WideCharToMultiByte(locale->locinfo->lc_codepage,
                WC_NO_BEST_FIT_CHARS, wcstr, 1, p, len, &default_char, NULL);
        if(!hlp || *p=='\0')
            break;

        p += hlp;
        len -= hlp;

        wcstr++;
        *ret += 1;
    }

    if(!len && size<=count) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *mbstr = '\0';
        *_errno() = ERANGE;
        return ERANGE;
    }

    if(*wcstr == '\0')
        *ret += 1;
    *p = '\0';
    return 0;
}

/*********************************************************************
 *		wcstombs_s (MSVCRT.@)
 */
int CDECL wcstombs_s(size_t *ret, char *mbstr,
        size_t size, const MSVCRT_wchar_t *wcstr, size_t count)
{
    return _wcstombs_s_l(ret, mbstr, size, wcstr, count, NULL);
}

/*********************************************************************
 *		wcstod (MSVCRT.@)
 */
double CDECL wcstod(const wchar_t* lpszStr, wchar_t** end)
{
    return _wcstod_l(lpszStr, end, NULL);
}

/*********************************************************************
 *		_wtof (MSVCRT.@)
 */
double CDECL _wtof(const wchar_t *str)
{
    return _wcstod_l(str, NULL, NULL);
}

/*********************************************************************
 *		_wtof_l (MSVCRT.@)
 */
double CDECL _wtof_l(const wchar_t *str, _locale_t locale)
{
    return _wcstod_l(str, NULL, locale);
}

typedef struct pf_output_t
{
    int used;
    int len;
    BOOL unicode;
    union {
        LPWSTR W;
        LPSTR  A;
    } buf;
} pf_output;

typedef struct pf_flags_t
{
    char Sign, LeftAlign, Alternate, PadZero;
    int FieldLength, Precision;
    char IntegerLength, IntegerDouble;
    char WideString;
    char Format;
} pf_flags;

/*
 * writes a string of characters to the output
 * returns -1 if the string doesn't fit in the output buffer
 * return the length of the string if all characters were written
 */
static inline int pf_output_stringW( pf_output *out, LPCWSTR str, int len )
{
    int space = out->len - out->used;

    if( len < 0 )
        len = strlenW( str );
    if( out->unicode )
    {
        LPWSTR p = out->buf.W + out->used;

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
        LPSTR p = out->buf.A + out->used;

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
    int space = out->len - out->used;

    if( len < 0 )
        len = strlen( str );
    if( !out->unicode )
    {
        LPSTR p = out->buf.A + out->used;

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
        LPWSTR p = out->buf.W + out->used;

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
        lnx_sprintf(p, "%d", flags->FieldLength);
        p += strlen(p);
    }
    if( flags->Precision >= 0 )
    {
        lnx_sprintf(p, ".%d", flags->Precision);
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
static int pf_vsnprintf( pf_output *out, const WCHAR *format,
        _locale_t locale, BOOL valid, va_list valist )
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
            if( *p == 'h' || *p == 'l' || *p == 'L' )
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
                lnx_sprintf(pointer, "0X%0*lX", 2 * (int)sizeof(ptr), (ULONG_PTR)ptr);
            else
                lnx_sprintf(pointer, "%0*lX", 2 * (int)sizeof(ptr), (ULONG_PTR)ptr);
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
                lnx_sprintf( x, fmt, va_arg(valist, double) );
                if (toupper(flags.Format) == 'E' || toupper(flags.Format) == 'G')
                    pf_fixup_exponent( x );
            }
            else
                lnx_sprintf( x, fmt, va_arg(valist, int) );

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
                _invalid_parameter( NULL, NULL, NULL, 0, 0 );
                *_errno() = EINVAL;
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
static inline int vsnprintf_internal( char *str, size_t len, const char *format,
        _locale_t locale, BOOL valid, __ms_va_list valist )
{
    DWORD sz;
    LPWSTR formatW = NULL;
    pf_output out;
    int r;

    out.unicode = FALSE;
    out.buf.A = str;
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
int CDECL _vsnprintf( char *str, size_t len,
                            const char *format, va_list valist )
{
    return vsnprintf_internal(str, len, format, NULL, FALSE, valist);
}

/*********************************************************************
*		_vsnprintf_l (MSVCRT.@)
 */
int CDECL _vsnprintf_l( char *str, size_t len, const char *format,
                            _locale_t locale, va_list valist )
{
    return vsnprintf_internal(str, len, format, locale, FALSE, valist);
}

/*********************************************************************
 *		_vsnprintf_s_l (MSVCRT.@)
 */
int CDECL _vsnprintf_s_l( char *str, size_t sizeOfBuffer,
        size_t count, const char *format,
        _locale_t locale, va_list valist )
{
    int len, ret;

    if(sizeOfBuffer<count+1 || count==-1)
        len = sizeOfBuffer;
    else
        len = count+1;

    ret = vsnprintf_internal(str, len, format, locale, TRUE, valist);

    if(ret<0 || ret==len) {
        if(count!=_TRUNCATE && count>sizeOfBuffer) {
            _invalid_parameter( NULL, NULL, NULL, 0, 0 );
            *_errno() = ERANGE;
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
int CDECL _vsnprintf_s( char *str, size_t sizeOfBuffer,
        size_t count, const char *format, va_list valist )
{
    return _vsnprintf_s_l(str,sizeOfBuffer, count, format, NULL, valist);
}

/*********************************************************************
 *		vsprintf (MSVCRT.@)
 */
int CDECL vsprintf( char *str, const char *format, va_list valist)
{
    return _vsnprintf(str, INT_MAX, format, valist);
}

/*********************************************************************
 *		vsprintf_s (MSVCRT.@)
 */
int CDECL vsprintf_s( char *str, size_t num, const char *format, va_list valist)
{
    return _vsnprintf(str, num, format, valist);
}

/*********************************************************************
 *		_vscprintf (MSVCRT.@)
 */
int CDECL _vscprintf( const char *format, va_list valist )
{
    return _vsnprintf( NULL, INT_MAX, format, valist );
}

/*********************************************************************
 *		_snprintf (MSVCRT.@)
 */
int CDECL _snprintf(char *str, size_t len, const char *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = _vsnprintf(str, len, format, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 * vsnwsprintf_internal (INTERNAL)
 */
static inline int vsnwprintf_internal(wchar_t *str, size_t len,
        const wchar_t *format, _locale_t locale, BOOL valid,
        va_list valist)
{
    pf_output out;

    out.unicode = TRUE;
    out.buf.W = str;
    out.used = 0;
    out.len = len;

    return pf_vsnprintf( &out, format, locale, valid, valist );
}

/*********************************************************************
 *              _vsnwsprintf (MSVCRT.@)
 */
int CDECL _vsnwprintf(wchar_t *str, size_t len,
        const wchar_t *format, va_list valist)
{
    return vsnwprintf_internal(str, len, format, NULL, FALSE, valist);
}

/*********************************************************************
 *              _vsnwsprintf_l (MSVCRT.@)
 */
int CDECL _vsnwprintf_l(wchar_t *str, size_t len,
        const wchar_t *format, _locale_t locale,
        va_list valist)
{
        return vsnwprintf_internal(str, len, format, locale, FALSE, valist);
}

/*********************************************************************
 *              _vsnwsprintf_s_l (MSVCRT.@)
 */
int CDECL _vsnwprintf_s_l( wchar_t *str, size_t sizeOfBuffer,
        size_t count, const wchar_t *format,
        _locale_t locale, va_list valist)
{
    int len, ret;

    len = sizeOfBuffer/sizeof(wchar_t);
    if(count!=-1 && len>count+1)
        len = count+1;

    ret = vsnwprintf_internal(str, len, format, locale, TRUE, valist);

    if(ret<0 || ret==len) {
        if(count!=_TRUNCATE && count>sizeOfBuffer/sizeof(wchar_t)) {
            _invalid_parameter( NULL, NULL, NULL, 0, 0 );
            *_errno() = ERANGE;
            memset(str, 0, sizeOfBuffer);
        } else
            str[len-1] = '\0';

        return -1;
    }

    return ret;
}

/*********************************************************************
 *              _vsnwsprintf_s (MSVCRT.@)
 */
int CDECL _vsnwprintf_s(wchar_t *str, size_t sizeOfBuffer,
        size_t count, const wchar_t *format, va_list valist)
{
    return _vsnwprintf_s_l(str, sizeOfBuffer, count,
            format, NULL, valist);
}

/*********************************************************************
 *		_snwprintf (MSVCRT.@)
 */
int CDECL _snwprintf( wchar_t *str, size_t len, const wchar_t *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = _vsnwprintf(str, len, format, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *		sprintf (MSVCRT.@)
 */
int CDECL sprintf( char *str, const char *format, ... )
{
    va_list ap;
    int r;

    va_start( ap, format );
    r = _vsnprintf( str, INT_MAX, format, ap );
    va_end( ap );
    return r;
}

/*********************************************************************
 *		sprintf_s (MSVCRT.@)
 */
int CDECL sprintf_s( char *str, size_t num, const char *format, ... )
{
    va_list ap;
    int r;

    va_start( ap, format );
    r = _vsnprintf( str, num, format, ap );
    va_end( ap );
    return r;
}

/*********************************************************************
 *		swprintf (MSVCRT.@)
 */
int CDECL swprintf( wchar_t *str, const wchar_t *format, ... )
{
    va_list ap;
    int r;

    va_start( ap, format );
    r = _vsnwprintf( str, INT_MAX, format, ap );
    va_end( ap );
    return r;
}

/*********************************************************************
 *		swprintf_s (MSVCRT.@)
 */
int CDECL swprintf_s(wchar_t *str, size_t numberOfElements,
        const wchar_t *format, ... )
{
    va_list ap;
    int r;

    va_start(ap, format);
    r = _vsnwprintf_s(str, numberOfElements*sizeof(wchar_t),
            INT_MAX, format, ap);
    va_end(ap);

    return r;
}

/*********************************************************************
 *		vswprintf (MSVCRT.@)
 */
int CDECL vswprintf( wchar_t* str, const wchar_t* format, va_list args )
{
    return _vsnwprintf( str, INT_MAX, format, args );
}

/*********************************************************************
 *		_vscwprintf (MSVCRT.@)
 */
int CDECL _vscwprintf( const wchar_t *format, va_list args )
{
    return _vsnwprintf( NULL, INT_MAX, format, args );
}

/*********************************************************************
 *		vswprintf_s (MSVCRT.@)
 */
int CDECL vswprintf_s(wchar_t* str, size_t numberOfElements,
        const wchar_t* format,va_list args)
{
    return _vsnwprintf_s(str, numberOfElements*sizeof(wchar_t),
            INT_MAX, format, args);
}

/*********************************************************************
 *              _vswprintf_s_l (MSVCRT.@)
 */
int CDECL _vswprintf_s_l(wchar_t* str, size_t numberOfElements,
        const wchar_t* format, _locale_t locale, va_list args)
{
    return _vsnwprintf_s_l(str, numberOfElements*sizeof(wchar_t),
            INT_MAX, format, locale, args );
}
#endif
/*********************************************************************
 *		wcscoll (MSVCRT.@)
 */
int CDECL wcscoll( const wchar_t* str1, const wchar_t* str2 )
{
  /* FIXME: handle collates */
  return strcmpW( str1, str2 );
}

/*********************************************************************
 *		wcspbrk (MSVCRT.@)
 */
wchar_t* CDECL wcspbrk( const wchar_t* str, const wchar_t* accept )
{
  const wchar_t* p;
  while (*str)
  {
    for (p = accept; *p; p++) if (*p == *str) return (wchar_t*)str;
      str++;
  }
  return NULL;
}

#ifndef _LIBCNT_
/*********************************************************************
 *		wcstok  (MSVCRT.@)
 */
wchar_t * CDECL wcstok( wchar_t *str, const wchar_t *delim )
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    wchar_t *ret;

    if (!str)
        if (!(str = data->wcstok_next)) return NULL;

    while (*str && strchrW( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !strchrW( delim, *str )) str++;
    if (*str) *str++ = 0;
    data->wcstok_next = str;
    return ret;
}

/*********************************************************************
 *		wctomb (MSVCRT.@)
 */
INT CDECL wctomb( char *dst, wchar_t ch )
{
  return WideCharToMultiByte( CP_ACP, 0, &ch, 1, dst, 6, NULL, NULL );
}

#endif

/*********************************************************************
 *		wcscpy_s (MSVCRT.@)
 */
INT CDECL wcscpy_s( wchar_t* wcDest, size_t numElement, const  wchar_t *wcSrc)
{
    size_t size = 0;

    if(!wcDest || !numElement)
        return EINVAL;

    wcDest[0] = 0;

    if(!wcSrc)
    {
        return EINVAL;
    }

    size = strlenW(wcSrc) + 1;

    if(size > numElement)
    {
        return ERANGE;
    }

    memcpy( wcDest, wcSrc, size*sizeof(WCHAR) );

    return 0;
}

/******************************************************************
 *		wcsncpy_s (MSVCRT.@)
 */
INT CDECL wcsncpy_s( wchar_t* wcDest, size_t numElement, const wchar_t *wcSrc,
                            size_t count )
{
    size_t size = 0;

    if (!wcDest || !numElement)
        return EINVAL;

    wcDest[0] = 0;

    if (!wcSrc)
    {
        return EINVAL;
    }

    size = min(strlenW(wcSrc), count);

    if (size >= numElement)
    {
        return ERANGE;
    }

    memcpy( wcDest, wcSrc, size*sizeof(WCHAR) );
    wcDest[size] = '\0';

    return 0;
}

/******************************************************************
 *		wcscat_s (MSVCRT.@)
 *
 */
INT CDECL wcscat_s(wchar_t* dst, size_t elem, const wchar_t* src)
{
    wchar_t* ptr = dst;

    if (!dst || elem == 0) return EINVAL;
    if (!src)
    {
        dst[0] = '\0';
        return EINVAL;
    }

    /* seek to end of dst string (or elem if no end of string is found */
    while (ptr < dst + elem && *ptr != '\0') ptr++;
    while (ptr < dst + elem)
    {
        if ((*ptr++ = *src++) == '\0') return 0;
    }
    /* not enough space */
    dst[0] = '\0';
    return ERANGE;
}

#ifndef _LIBCNT_
/*********************************************************************
 *  _wctoi64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL _wcstoi64_l(const wchar_t *nptr,
        wchar_t **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if(!nptr || base<0 || base>36 || base==1) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
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

        if(!negative && (ret>INT64_MAX/base || ret*base>INT64_MAX-v)) {
            ret = INT64_MAX;
            *_errno() = ERANGE;
        } else if(negative && (ret<INT64_MIN/base || ret*base<INT64_MIN-v)) {
            ret = INT64_MIN;
            *_errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (wchar_t*)nptr;

    return ret;
}

/*********************************************************************
 *  _wcstoui64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL _wcstoui64_l(const wchar_t *nptr,
        wchar_t **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if(!nptr || base<0 || base>36 || base==1) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
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

        if(ret>UINT64_MAX/base || ret*base>UINT64_MAX-v) {
            ret = UINT64_MAX;
            *_errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (wchar_t*)nptr;

    return negative ? -ret : ret;
}
#endif
