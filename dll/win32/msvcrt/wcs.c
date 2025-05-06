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

#define _NO_CRT_STDIO_INLINE
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <assert.h>
#include <wchar.h>
#include <wctype.h>
#include "msvcrt.h"
#include "winnls.h"
#include "wtypes.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef struct
{
    enum { LEN_DEFAULT, LEN_SHORT, LEN_LONG } IntegerLength;
    BOOLEAN IntegerDouble, IntegerNative, LeftAlign, Alternate, PadZero;
    BOOLEAN WideString, NaturalString;
    int FieldLength, Precision;
    char Sign, Format;
} pf_flags;

static BOOL n_format_enabled = TRUE;

#include "printf.h"
#define PRINTF_WIDE
#include "printf.h"
#undef PRINTF_WIDE

#if _MSVCR_VER>=80

/*********************************************************************
 *		_get_printf_count_output (MSVCR80.@)
 */
int CDECL _get_printf_count_output( void )
{
    return n_format_enabled ? 1 : 0;
}

/*********************************************************************
 *		_set_printf_count_output (MSVCR80.@)
 */
int CDECL _set_printf_count_output( int enable )
{
    BOOL old = n_format_enabled;
    n_format_enabled = enable != 0;
    return old ? 1 : 0;
}

#endif /* _MSVCR_VER>=80 */

/*********************************************************************
 *		_wcsdup (MSVCRT.@)
 */
wchar_t* CDECL _wcsdup( const wchar_t* str )
{
  wchar_t* ret = NULL;
  if (str)
  {
    size_t size = (wcslen(str) + 1) * sizeof(wchar_t);
    ret = malloc( size );
    if (ret) memcpy( ret, str, size );
  }
  return ret;
}

/*********************************************************************
 *              _towlower_l (MSVCRT.@)
 */
wint_t CDECL _towlower_l(wint_t c, _locale_t locale)
{
    pthreadlocinfo locinfo;
    wchar_t ret;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[LC_CTYPE]) {
        if(c >= 'A' && c <= 'Z')
            return c + 'a' - 'A';
        return c;
    }

    if(!LCMapStringW(locinfo->lc_handle[LC_CTYPE], LCMAP_LOWERCASE, &c, 1, &ret, 1))
        return c;
    return ret;
}

/*********************************************************************
 *              towlower (MSVCRT.@)
 */
wint_t CDECL towlower(wint_t c)
{
    return _towlower_l(c, NULL);
}

INT CDECL _wcsicmp_l(const wchar_t *str1, const wchar_t *str2, _locale_t locale)
{
    _locale_tstruct tmp = {0};
    wchar_t c1, c2;

    if(!MSVCRT_CHECK_PMT(str1 != NULL) || !MSVCRT_CHECK_PMT(str2 != NULL))
        return _NLSCMPERROR;

    if(!locale)
        locale = get_current_locale_noalloc(&tmp);

    do
    {
        c1 = _towlower_l(*str1++, locale);
        c2 = _towlower_l(*str2++, locale);
    } while(c1 && (c1 == c2));

    free_locale_noalloc(&tmp);
    return c1 - c2;
}

/*********************************************************************
 *      towctrans (MSVCR120.@)
 */
wint_t CDECL towctrans(wint_t c, wctrans_t category)
{
    if(category == 1)
        return _towupper_l(c, NULL);
    return _towlower_l(c, NULL);
}

/*********************************************************************
 *		_wcsicmp (MSVCRT.@)
 */
INT CDECL _wcsicmp( const wchar_t* str1, const wchar_t* str2 )
{
    return _wcsicmp_l(str1, str2, NULL);
}

/*********************************************************************
 *              _wcsnicmp_l (MSVCRT.@)
 */
INT CDECL _wcsnicmp_l(const wchar_t *str1, const wchar_t *str2,
        size_t n, _locale_t locale)
{
    _locale_tstruct tmp = {0};
    wchar_t c1, c2;

    if (!n)
        return 0;

    if(!MSVCRT_CHECK_PMT(str1 != NULL) || !MSVCRT_CHECK_PMT(str2 != NULL))
        return _NLSCMPERROR;

    if(!locale)
        locale = get_current_locale_noalloc(&tmp);

    do
    {
        c1 = _towlower_l(*str1++, locale);
        c2 = _towlower_l(*str2++, locale);
    } while(--n && c1 && (c1 == c2));

    free_locale_noalloc(&tmp);
    return c1 - c2;
}

/*********************************************************************
 *              _wcsnicmp (MSVCRT.@)
 */
INT CDECL _wcsnicmp(const wchar_t *str1, const wchar_t *str2, size_t n)
{
    return _wcsnicmp_l(str1, str2, n, NULL);
}

/*********************************************************************
 *              _wcsicoll_l (MSVCRT.@)
 */
int CDECL _wcsicoll_l(const wchar_t* str1, const wchar_t* str2, _locale_t locale)
{
    pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[LC_COLLATE])
    {
        wchar_t c1, c2;

        do
        {
            c1 = *str1++;
            if (c1 >= 'A' && c1 <= 'Z')
                c1 += 'a' - 'A';

            c2 = *str2++;
            if (c2 >= 'A' && c2 <= 'Z')
                c2 += 'a' - 'A';
        } while(c1 && (c1 == c2));
        return c1 - c2;
    }

    return CompareStringW(locinfo->lc_handle[LC_COLLATE], NORM_IGNORECASE,
			  str1, -1, str2, -1)-CSTR_EQUAL;
}

/*********************************************************************
 *		_wcsicoll (MSVCRT.@)
 */
INT CDECL _wcsicoll( const wchar_t* str1, const wchar_t* str2 )
{
    return _wcsicoll_l(str1, str2, NULL);
}

/*********************************************************************
 *              _wcsnicoll_l (MSVCRT.@)
 */
int CDECL _wcsnicoll_l(const wchar_t* str1, const wchar_t* str2,
			      size_t count, _locale_t locale)
{
    pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[LC_COLLATE])
    {
        wchar_t c1, c2;

        if (!count)
            return 0;

        do
        {
            c1 = *str1++;
            if (c1 >= 'A' && c1 <= 'Z')
                c1 += 'a' - 'A';

            c2 = *str2++;
            if (c2 >= 'A' && c2 <= 'Z')
                c2 += 'a' - 'A';
        } while(--count && c1 && (c1 == c2));
        return c1 - c2;
    }

    return CompareStringW(locinfo->lc_handle[LC_COLLATE], NORM_IGNORECASE,
			  str1, wcsnlen(str1, count),
                          str2, wcsnlen(str2, count))-CSTR_EQUAL;
}

/*********************************************************************
 *		_wcsnicoll (MSVCRT.@)
 */
INT CDECL _wcsnicoll( const wchar_t* str1, const wchar_t* str2, size_t count )
{
    return _wcsnicoll_l(str1, str2, count, NULL);
}

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
 *              _wcsnset_s (MSVCRT.@)
 */
int CDECL _wcsnset_s( wchar_t *str, size_t size, wchar_t c, size_t count )
{
    size_t i;

    if(!str && !size && !count) return 0;
    if(!MSVCRT_CHECK_PMT(str != NULL)) return EINVAL;
    if(!MSVCRT_CHECK_PMT(size > 0)) return EINVAL;

    for(i=0; i<size-1 && i<count; i++) {
        if(!str[i]) return 0;
        str[i] = c;
    }
    for(; i<size; i++)
        if(!str[i]) return 0;

    str[0] = 0;
    _invalid_parameter(NULL, NULL, NULL, 0, 0);
    *_errno() = EINVAL;
    return EINVAL;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
 */
wchar_t* CDECL _wcsrev( wchar_t* str )
{
  wchar_t* ret = str;
  wchar_t* end = str + wcslen(str) - 1;
  while (end > str)
  {
    wchar_t t = *end;
    *end--  = *str;
    *str++  = t;
  }
  return ret;
}

/*********************************************************************
 *              _wcsset_s (MSVCRT.@)
 */
int CDECL _wcsset_s( wchar_t *str, size_t n, wchar_t c )
{
    wchar_t *p = str;

    if(!MSVCRT_CHECK_PMT(str != NULL)) return EINVAL;
    if(!MSVCRT_CHECK_PMT(n)) return EINVAL;

    while(*p && --n) *p++ = c;
    if(!n) {
        str[0] = 0;
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *_errno() = EINVAL;
        return EINVAL;
    }
    return 0;
}

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
 *		_wcsupr_s_l (MSVCRT.@)
 */
int CDECL _wcsupr_s_l( wchar_t* str, size_t n, _locale_t locale )
{
  _locale_tstruct tmp = {0};
  wchar_t* ptr = str;

  if (!str || !n)
  {
    if (str) *str = '\0';
    *_errno() = EINVAL;
    return EINVAL;
  }

  if(!locale)
    locale = get_current_locale_noalloc(&tmp);

  while (n--)
  {
    if (!*ptr)
    {
      free_locale_noalloc(&tmp);
      return 0;
    }
    *ptr = _towupper_l(*ptr, locale);
    ptr++;
  }

  free_locale_noalloc(&tmp);

  /* MSDN claims that the function should return and set errno to
   * ERANGE, which doesn't seem to be true based on the tests. */
  *str = '\0';
  *_errno() = EINVAL;
  return EINVAL;
}

/******************************************************************
 *		_wcsupr_s (MSVCRT.@)
 *
 */
INT CDECL _wcsupr_s( wchar_t* str, size_t n )
{
  return _wcsupr_s_l( str, n, NULL );
}

/******************************************************************
 *              _wcsupr_l (MSVCRT.@)
 */
wchar_t* CDECL _wcsupr_l( wchar_t *str, _locale_t locale )
{
    _wcsupr_s_l( str, -1, locale);
    return str;
}

/******************************************************************
 *              _wcsupr (MSVCRT.@)
 */
wchar_t* CDECL _wcsupr( wchar_t *str )
{
    return _wcsupr_l(str, NULL);
}

/******************************************************************
 *		_wcslwr_s_l (MSVCRT.@)
 */
int CDECL _wcslwr_s_l( wchar_t* str, size_t n, _locale_t locale )
{
  _locale_tstruct tmp = {0};
  wchar_t* ptr = str;

  if (!str || !n)
  {
    if (str) *str = '\0';
    *_errno() = EINVAL;
    return EINVAL;
  }

  if(!locale)
    locale = get_current_locale_noalloc(&tmp);

  while (n--)
  {
    if (!*ptr)
    {
      free_locale_noalloc(&tmp);
      return 0;
    }
    *ptr = _towlower_l(*ptr, locale);
    ptr++;
  }

  free_locale_noalloc(&tmp);

  /* MSDN claims that the function should return and set errno to
   * ERANGE, which doesn't seem to be true based on the tests. */
  *str = '\0';
  *_errno() = EINVAL;
  return EINVAL;
}

/******************************************************************
 *		_wcslwr_s (MSVCRT.@)
 */
int CDECL _wcslwr_s( wchar_t* str, size_t n )
{
    return _wcslwr_s_l(str, n, NULL);
}

/******************************************************************
 *		_wcslwr_l (MSVCRT.@)
 */
wchar_t* CDECL _wcslwr_l( wchar_t* str, _locale_t locale )
{
    _wcslwr_s_l(str, -1, locale);
    return str;
}

/******************************************************************
 *		_wcslwr (MSVCRT.@)
 */
wchar_t* CDECL _wcslwr( wchar_t* str )
{
    _wcslwr_s_l(str, -1, NULL);
    return str;
}

/*********************************************************************
 *           wcscspn    (MSVCRT.@)
 */
size_t __cdecl wcscspn(const wchar_t *str, const wchar_t *reject)
{
    const wchar_t *ptr;
    for (ptr = str; *ptr; ptr++) if (wcschr( reject, *ptr )) break;
    return ptr - str;
}

/*********************************************************************
 *           wcsspn    (MSVCRT.@)
 */
size_t __cdecl wcsspn(const wchar_t *str, const wchar_t *accept)
{
    const wchar_t *ptr;
    for (ptr = str; *ptr; ptr++) if (!wcschr( accept, *ptr )) break;
    return ptr - str;
}

/*********************************************************************
 *           wcsncmp    (MSVCRT.@)
 */
int CDECL wcsncmp(const wchar_t *str1, const wchar_t *str2, size_t n)
{
    if (!n)
        return 0;
    while(--n && *str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

/*********************************************************************
 *              _wcsncoll_l (MSVCRT.@)
 */
int CDECL _wcsncoll_l(const wchar_t* str1, const wchar_t* str2,
			      size_t count, _locale_t locale)
{
    pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[LC_COLLATE])
        return wcsncmp(str1, str2, count);
    return CompareStringW(locinfo->lc_handle[LC_COLLATE], SORT_STRINGSORT,
              str1, wcsnlen(str1, count),
              str2, wcsnlen(str2, count))-CSTR_EQUAL;
}

/*********************************************************************
 *              _wcsncoll (MSVCRT.@)
 */
int CDECL _wcsncoll(const wchar_t* str1, const wchar_t* str2, size_t count)
{
    return _wcsncoll_l(str1, str2, count, NULL);
}

static wchar_t strtod_wstr_get(void *ctx)
{
    const wchar_t **p = ctx;
    if (!**p) return WEOF;
    return *(*p)++;
}

static void strtod_wstr_unget(void *ctx)
{
    const wchar_t **p = ctx;
    (*p)--;
}

/*********************************************************************
 *		_wcstod_l (MSVCRT.@)
 */
double CDECL _wcstod_l(const wchar_t* str, wchar_t** end,
        _locale_t locale)
{
    pthreadlocinfo locinfo;
    const wchar_t *beg, *p;
    struct fpnum fp;
    double ret;
    int err;

    if (!MSVCRT_CHECK_PMT(str != NULL)) {
        if (end) *end = NULL;
        return 0;
    }

    if (!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    p = str;
    while(_iswspace_l(*p, locale))
        p++;
    beg = p;

    fp = fpnum_parse(strtod_wstr_get, strtod_wstr_unget, &p, locinfo, FALSE);
    if (end) *end = (p == beg ? (wchar_t*)str : (wchar_t*)p);

    err = fpnum_double(&fp, &ret);
    if(err) *_errno() = err;
    return ret;
}

/*********************************************************************
 * wcsrtombs_l (INTERNAL)
 */
static size_t wcsrtombs_l(char *mbstr, const wchar_t **wcstr,
        size_t count, _locale_t locale)
{
    pthreadlocinfo locinfo;
    size_t tmp = 0;
    BOOL used_default = FALSE;
    BOOL *pused_default;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_codepage) {
        size_t i;

        if(!mbstr)
            return wcslen(*wcstr);

        for(i=0; i<count; i++) {
            if((*wcstr)[i] > 255) {
                *_errno() = EILSEQ;
                return -1;
            }

            mbstr[i] = (*wcstr)[i];
            if(!(*wcstr)[i]) break;
        }

        if(i < count) *wcstr = NULL;
        else *wcstr += i;
        return i;
    }

    pused_default = (locinfo->lc_codepage != CP_UTF8 ? &used_default : NULL);

    if(!mbstr) {
        tmp = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                *wcstr, -1, NULL, 0, NULL, pused_default);
        if(!tmp || used_default) {
            *_errno() = EILSEQ;
            return -1;
        }
        return tmp-1;
    }

    while(**wcstr) {
        char buf[3];
        size_t i, size;

        size = WideCharToMultiByte(locinfo->lc_codepage, WC_NO_BEST_FIT_CHARS,
                *wcstr, 1, buf, 3, NULL, pused_default);
        if(!size || used_default) {
            *_errno() = EILSEQ;
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
size_t CDECL _wcstombs_l(char *mbstr, const wchar_t *wcstr,
        size_t count, _locale_t locale)
{
    return wcsrtombs_l(mbstr, &wcstr, count, locale);
}

/*********************************************************************
 *		wcstombs (MSVCRT.@)
 */
size_t CDECL wcstombs(char *mbstr, const wchar_t *wcstr,
        size_t count)
{
    return wcsrtombs_l(mbstr, &wcstr, count, NULL);
}

/*********************************************************************
 *		wcsrtombs (MSVCRT.@)
 */
size_t CDECL wcsrtombs(char *mbstr, const wchar_t **wcstr,
        size_t count, mbstate_t *mbstate)
{
    if(mbstate)
        *mbstate = 0;

    return wcsrtombs_l(mbstr, wcstr, count, NULL);
}

/*********************************************************************
 * wcsrtombs_s_l (INTERNAL)
 */
static int wcsrtombs_s_l(size_t *ret, char *mbstr, size_t size,
        const wchar_t **wcstr, size_t count, _locale_t locale)
{
    size_t conv;
    int err;

    if(!mbstr && !size && wcstr) {
        conv = wcsrtombs_l(NULL, wcstr, 0, locale);
        if(ret)
            *ret = conv+1;
        if(conv == -1)
            return *_errno();
        return 0;
    }

    if (!MSVCRT_CHECK_PMT(mbstr != NULL)) return EINVAL;
    if (size) mbstr[0] = '\0';
    if (!MSVCRT_CHECK_PMT(wcstr != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(*wcstr != NULL)) return EINVAL;

    if(count==_TRUNCATE || size<count)
        conv = size;
    else
        conv = count;

    err = 0;
    conv = wcsrtombs_l(mbstr, wcstr, conv, locale);
    if(conv == -1) {
        conv = 0;
        if(size)
            mbstr[0] = '\0';
        err = *_errno();
    }else if(conv < size)
        mbstr[conv++] = '\0';
    else if(conv==size && (count==_TRUNCATE || mbstr[conv-1]=='\0')) {
        mbstr[conv-1] = '\0';
        if(count==_TRUNCATE)
            err = STRUNCATE;
    }else {
        MSVCRT_INVALID_PMT("mbstr[size] is too small", ERANGE);
        conv = 0;
        if(size)
            mbstr[0] = '\0';
        err = ERANGE;
    }

    if(ret)
        *ret = conv;
    return err;
}

/*********************************************************************
 *		_wcstombs_s_l (MSVCRT.@)
 */
int CDECL _wcstombs_s_l(size_t *ret, char *mbstr, size_t size,
        const wchar_t *wcstr, size_t count, _locale_t locale)
{
    return wcsrtombs_s_l(ret, mbstr, size, &wcstr,count, locale);
}

/*********************************************************************
 *		wcstombs_s (MSVCRT.@)
 */
int CDECL wcstombs_s(size_t *ret, char *mbstr, size_t size,
        const wchar_t *wcstr, size_t count)
{
    return wcsrtombs_s_l(ret, mbstr, size, &wcstr, count, NULL);
}

/*********************************************************************
 *		wcsrtombs_s (MSVCRT.@)
 */
int CDECL wcsrtombs_s(size_t *ret, char *mbstr, size_t size,
        const wchar_t **wcstr, size_t count, mbstate_t *mbstate)
{
    if(mbstate)
        *mbstate = 0;

    return wcsrtombs_s_l(ret, mbstr, size, wcstr, count, NULL);
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

#if _MSVCR_VER>=120

/*********************************************************************
 *              _wcstof_l  (MSVCR120.@)
 */
float CDECL _wcstof_l( const wchar_t *str, wchar_t **end, _locale_t locale )
{
    double ret = _wcstod_l(str, end, locale);
    if (ret && isfinite(ret)) {
        float f = ret;
        if (!f || !isfinite(f))
            *_errno() = ERANGE;
    }
    return ret;
}

/*********************************************************************
 *              wcstof  (MSVCR120.@)
 */
float CDECL wcstof( const wchar_t *str, wchar_t **end )
{
    return _wcstof_l(str, end, NULL);
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 * arg_clbk_valist (INTERNAL)
 */
printf_arg arg_clbk_valist(void *ctx, int arg_pos, int type, va_list *valist)
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
 * arg_clbk_positional (INTERNAL)
 */
printf_arg arg_clbk_positional(void *ctx, int pos, int type, va_list *valist)
{
    printf_arg *args = ctx;
    return args[pos];
}

#if _MSVCR_VER < 140

/*********************************************************************
 *              _vsnprintf (MSVCRT.@)
 */
int CDECL _vsnprintf( char *str, size_t len, const char *format, va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, NULL, 0,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);
    return ret;
}

#else

static int puts_clbk_str_c99_a(void *ctx, int len, const char *str)
{
    struct _str_ctx_a *out = ctx;

    if(!out->buf)
        return len;

    if(out->len < len) {
        memmove(out->buf, str, out->len);
        out->buf += out->len;
        out->len = 0;
        return len;
    }

    memmove(out->buf, str, len);
    out->buf += len;
    out->len -= len;
    return len;
}

/*********************************************************************
 *              __stdio_common_vsprintf (UCRTBASE.@)
 */
int CDECL __stdio_common_vsprintf( unsigned __int64 options, char *str, size_t len, const char *format,
                                   _locale_t locale, va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);
    ret = pf_printf_a(puts_clbk_str_c99_a,
            &ctx, format, (_locale_t)locale, options & UCRTBASE_PRINTF_MASK, arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);

    if(!str)
        return ret;
    if(options & _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION)
        return ret>len ? -1 : ret;
    if(ret>=len) {
        if(len) str[len-1] = 0;
        if(options & _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR)
            return ret;
        return len > 0 ? -2 : -1;
    }
    return ret;
}

#endif /* _MSVCR_VER>=140 */

/*********************************************************************
 *		_vsnprintf_l (MSVCRT.@)
 */
int CDECL _vsnprintf_l( char *str, size_t len, const char *format,
                            _locale_t locale, va_list valist )
{
    static const char nullbyte = '\0';
    struct _str_ctx_a ctx = {len, str};
    int ret;

    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, locale, 0,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *		_vsprintf_l (MSVCRT.@)
 */
int CDECL _vsprintf_l( char *str, const char *format,
                            _locale_t locale, va_list valist )
{
    return _vsnprintf_l(str, INT_MAX, format, locale, valist);
}

/*********************************************************************
 *		_sprintf_l (MSVCRT.@)
 */
int WINAPIV _sprintf_l(char *str, const char *format,
                           _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vsnprintf_l(str, INT_MAX, format, locale, valist);
    va_end(valist);
    return retval;
}

static int CDECL vsnprintf_s_l_opt( char *str, size_t sizeOfBuffer,
        size_t count, const char *format, DWORD options,
        _locale_t locale, va_list valist )
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
    ret = pf_printf_a(puts_clbk_str_a, &ctx, format, locale, MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_a(&ctx, 1, &nullbyte);

    if(ret<0 || ret==len) {
        if(count!=_TRUNCATE && count>sizeOfBuffer) {
            MSVCRT_INVALID_PMT("str[sizeOfBuffer] is too small", ERANGE);
            memset(str, 0, sizeOfBuffer);
        } else
            str[len-1] = '\0';

        return -1;
    }

    return ret;
}

static int vsnwprintf_s_l_opt( wchar_t *str, size_t sizeOfBuffer,
        size_t count, const wchar_t *format, DWORD options,
        _locale_t locale, va_list valist)
{
    struct _str_ctx_w ctx;
    int len, ret;

    len = sizeOfBuffer;
    if(count!=-1 && len>count+1)
        len = count+1;

    ctx.len = len;
    ctx.buf = str;
    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, locale, MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, L"");

    if(ret<0 || ret==len) {
        if(count!=_TRUNCATE && count>sizeOfBuffer) {
            MSVCRT_INVALID_PMT("str[sizeOfBuffer] is too small", ERANGE);
            memset(str, 0, sizeOfBuffer*sizeof(wchar_t));
        } else
            str[len-1] = '\0';

        return -1;
    }

    return ret;
}

/*********************************************************************
 *		_vsnprintf_s_l (MSVCRT.@)
 */
int CDECL _vsnprintf_s_l( char *str, size_t sizeOfBuffer,
        size_t count, const char *format,
        _locale_t locale, va_list valist )
{
    return vsnprintf_s_l_opt(str, sizeOfBuffer, count, format, 0, locale, valist);
}

/*********************************************************************
 *		_vsprintf_s_l (MSVCRT.@)
 */
int CDECL _vsprintf_s_l( char *str, size_t count, const char *format,
                               _locale_t locale, va_list valist )
{
    return _vsnprintf_s_l(str, INT_MAX, count, format, locale, valist);
}

/*********************************************************************
 *		_sprintf_s_l (MSVCRT.@)
 */
int WINAPIV _sprintf_s_l( char *str, size_t count, const char *format,
                                _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vsnprintf_s_l(str, INT_MAX, count, format, locale, valist);
    va_end(valist);
    return retval;
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
 *              _vsnprintf_c_l (MSVCRT.@)
 */
int CDECL _vsnprintf_c_l(char *str, size_t len, const char *format,
        _locale_t locale, va_list valist)
{
    return vsnprintf_s_l_opt(str, len, len, format, 0, locale, valist);
}

/*********************************************************************
 *              _vsnprintf_c (MSVCRT.@)
 */
int CDECL _vsnprintf_c(char *str, size_t len,
        const char *format, va_list valist)
{
    return _vsnprintf_c_l(str, len, format, NULL, valist);
}

#if _MSVCR_VER>=140

/*********************************************************************
 *              __stdio_common_vsnprintf_s (UCRTBASE.@)
 */
int CDECL __stdio_common_vsnprintf_s( unsigned __int64 options,
        char *str, size_t sizeOfBuffer, size_t count,
        const char *format, _locale_t locale, va_list valist )
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);
    return vsnprintf_s_l_opt(str, sizeOfBuffer, count, format, options & UCRTBASE_PRINTF_MASK, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vsnwprintf_s (UCRTBASE.@)
 */
int CDECL __stdio_common_vsnwprintf_s( unsigned __int64 options,
        wchar_t *str, size_t sizeOfBuffer, size_t count,
        const wchar_t *format, _locale_t locale, va_list valist )
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);
    return vsnwprintf_s_l_opt(str, sizeOfBuffer, count, format, options & UCRTBASE_PRINTF_MASK, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vswprintf_s (UCRTBASE.@)
 */
int CDECL __stdio_common_vswprintf_s( unsigned __int64 options,
        wchar_t *str, size_t count, const wchar_t *format,
        _locale_t locale, va_list valist )
{
    return __stdio_common_vsnwprintf_s(options, str, INT_MAX, count, format, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vsprintf_s (UCRTBASE.@)
 */
int CDECL __stdio_common_vsprintf_s( unsigned __int64 options,
        char *str, size_t count, const char *format,
        _locale_t locale, va_list valist )
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);
    return vsnprintf_s_l_opt(str, INT_MAX, count, format, options & UCRTBASE_PRINTF_MASK, locale, valist);
}

#endif /* _MSVCR_VER>=140 */

/*********************************************************************
 *		vsprintf (MSVCRT.@)
 */
int CDECL vsprintf( char *str, const char *format, va_list valist)
{
    return vsnprintf(str, INT_MAX, format, valist);
}

/*********************************************************************
 *		vsprintf_s (MSVCRT.@)
 */
int CDECL vsprintf_s( char *str, size_t num, const char *format, va_list valist)
{
    return vsnprintf(str, num, format, valist);
}

/*********************************************************************
 *		_vscprintf (MSVCRT.@)
 */
int CDECL _vscprintf( const char *format, va_list valist )
{
    return _vsnprintf_l( NULL, INT_MAX, format, NULL, valist );
}

/*********************************************************************
 *              _vscprintf_l (MSVCRT.@)
 */
int CDECL _vscprintf_l(const char *format,
        _locale_t locale, va_list valist)
{
    return _vsnprintf_l(NULL, INT_MAX, format, locale, valist);
}

/*********************************************************************
 *		_vscprintf_p_l (MSVCRT.@)
 */
int CDECL _vscprintf_p_l(const char *format,
        _locale_t locale, va_list args)
{
    printf_arg args_ctx[_ARGMAX+1];
    struct _str_ctx_a puts_ctx = {INT_MAX, NULL};
    int ret;

    memset(args_ctx, 0, sizeof(args_ctx));

    ret = create_positional_ctx_a(args_ctx, format, args);
    if(ret < 0)  {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *_errno() = EINVAL;
        return ret;
    } else if(ret == 0) {
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale,
                MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER,
                arg_clbk_valist, NULL, &args);
    } else {
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale,
                MSVCRT_PRINTF_POSITIONAL_PARAMS | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER,
                arg_clbk_positional, args_ctx, NULL);
    }

    return ret;
}

/*********************************************************************
 *		_vscprintf_p (MSVCR80.@)
 */
int CDECL _vscprintf_p(const char *format, va_list argptr)
{
    return _vscprintf_p_l(format, NULL, argptr);
}

/*********************************************************************
 *		_snprintf (MSVCRT.@)
 */
int WINAPIV _snprintf(char *str, size_t len, const char *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = vsnprintf(str, len, format, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snprintf_l (MSVCRT.@)
 */
int WINAPIV _snprintf_l(char *str, size_t count, const char *format,
        _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vsnprintf_l(str, count, format, locale, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _snprintf_c_l (MSVCRT.@)
 */
int WINAPIV _snprintf_c_l(char *str, size_t count, const char *format,
        _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vsnprintf_c_l(str, count, format, locale, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _snprintf_c (MSVCRT.@)
 */
int WINAPIV _snprintf_c(char *str, size_t count, const char *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = _vsnprintf_c(str, count, format, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _snprintf_s_l (MSVCRT.@)
 */
int WINAPIV _snprintf_s_l(char *str, size_t len, size_t count,
        const char *format, _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vsnprintf_s_l(str, len, count, format, locale, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snprintf_s (MSVCRT.@)
 */
int WINAPIV _snprintf_s(char *str, size_t len, size_t count,
    const char *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = _vsnprintf_s_l(str, len, count, format, NULL, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _scprintf (MSVCRT.@)
 */
int WINAPIV _scprintf(const char *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = _vscprintf(format, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _scprintf_l (MSVCRT.@)
 */
int WINAPIV _scprintf_l(const char *format, _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vscprintf_l(format, locale, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _scprintf_p (MSVCRT.@)
 */
int WINAPIV _scprintf_p(const char *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = _vscprintf_p_l(format, NULL, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _scprintf_p_l (MSVCRT.@)
 */
int WINAPIV _scprintf_p_l(const char *format, _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vscprintf_p_l(format, locale, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _vsnwprintf (MSVCRT.@)
 */
int CDECL _vsnwprintf(wchar_t *str, size_t len,
        const wchar_t *format, va_list valist)
{
    struct _str_ctx_w ctx = {len, str};
    int ret;

    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, NULL, 0,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, L"");
    return ret;
}

/*********************************************************************
 *              _vsnwprintf_l (MSVCRT.@)
 */
int CDECL _vsnwprintf_l(wchar_t *str, size_t len, const wchar_t *format,
        _locale_t locale, va_list valist)
{
    struct _str_ctx_w ctx = {len, str};
    int ret;

    ret = pf_printf_w(puts_clbk_str_w, &ctx, format, locale, 0,
            arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, L"");
    return ret;
}

/*********************************************************************
 *              _vswprintf_c_l (MSVCRT.@)
 */
int CDECL _vswprintf_c_l(wchar_t *str, size_t len, const wchar_t *format,
        _locale_t locale, va_list valist)
{
    return vsnwprintf_s_l_opt(str, len, len, format, 0, locale, valist);
}

/*********************************************************************
 *              _vswprintf_c (MSVCRT.@)
 */
int CDECL _vswprintf_c(wchar_t *str, size_t len,
        const wchar_t *format, va_list valist)
{
    return _vswprintf_c_l(str, len, format, NULL, valist);
}

static int vswprintf_p_l_opt(wchar_t *buffer, size_t length,
        const wchar_t *format, DWORD options, _locale_t locale, va_list args)
{
    printf_arg args_ctx[_ARGMAX+1];
    struct _str_ctx_w puts_ctx = {length, buffer};
    int ret;

    memset(args_ctx, 0, sizeof(args_ctx));

    ret = create_positional_ctx_w(args_ctx, format, args);
    if(ret < 0)  {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *_errno() = EINVAL;
        return ret;
    } else if(ret == 0)
        ret = pf_printf_w(puts_clbk_str_w, &puts_ctx, format, locale, MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
                arg_clbk_valist, NULL, &args);
    else
        ret = pf_printf_w(puts_clbk_str_w, &puts_ctx, format, locale,
                MSVCRT_PRINTF_POSITIONAL_PARAMS | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
                arg_clbk_positional, args_ctx, NULL);

    puts_clbk_str_w(&puts_ctx, 1, L"");
    return ret;
}

/*********************************************************************
 *		_vswprintf_p_l (MSVCRT.@)
 */
int CDECL _vswprintf_p_l(wchar_t *buffer, size_t length,
        const wchar_t *format, _locale_t locale, va_list args)
{
    return vswprintf_p_l_opt(buffer, length, format, 0, locale, args);
}

#if _MSVCR_VER>=80
/*********************************************************************
 * _vswprintf_p (MSVCR80.@)
 */
int CDECL _vswprintf_p(wchar_t *buffer, size_t length,
        const wchar_t *format, va_list args)
{
    return vswprintf_p_l_opt(buffer, length, format, 0, NULL, args);
}
#endif

#if _MSVCR_VER>=140
/*********************************************************************
 *              __stdio_common_vswprintf_p (UCRTBASE.@)
 */
int CDECL __stdio_common_vswprintf_p( unsigned __int64 options,
        wchar_t *str, size_t count, const wchar_t *format,
        _locale_t locale, va_list valist )
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);
    return vswprintf_p_l_opt(str, count, format, options & UCRTBASE_PRINTF_MASK, locale, valist);
}
#endif

/*********************************************************************
 *              _vsnwprintf_s_l (MSVCRT.@)
 */
int CDECL _vsnwprintf_s_l( wchar_t *str, size_t sizeOfBuffer,
        size_t count, const wchar_t *format,
        _locale_t locale, va_list valist)
{
    return vsnwprintf_s_l_opt(str, sizeOfBuffer, count, format, 0, locale, valist);
}

/*********************************************************************
 *              _vsnwprintf_s (MSVCRT.@)
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
int WINAPIV _snwprintf( wchar_t *str, size_t len, const wchar_t *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = _vsnwprintf(str, len, format, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snwprintf_l (MSVCRT.@)
 */
int WINAPIV _snwprintf_l( wchar_t *str, size_t len, const wchar_t *format,
        _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vsnwprintf_l(str, len, format, locale, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *		_snwprintf_s (MSVCRT.@)
 */
int WINAPIV _snwprintf_s( wchar_t *str, size_t len, size_t count,
    const wchar_t *format, ...)
{
    int retval;
    va_list valist;
    va_start(valist, format);
    retval = _vsnwprintf_s_l(str, len, count, format, NULL, valist);
    va_end(valist);
    return retval;
}

/*********************************************************************
 *              _snwprintf_s_l (MSVCRT.@)
 */
int WINAPIV _snwprintf_s_l( wchar_t *str, size_t len, size_t count,
        const wchar_t *format, _locale_t locale, ... )
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vsnwprintf_s_l(str, len, count, format, locale, valist);
    va_end(valist);
    return retval;
}

#if _MSVCR_VER>=140

static int puts_clbk_str_c99_w(void *ctx, int len, const wchar_t *str)
{
    struct _str_ctx_w *out = ctx;

    if(!out->buf)
        return len;

    if(out->len < len) {
        memcpy(out->buf, str, out->len*sizeof(wchar_t));
        out->buf += out->len;
        out->len = 0;
        return len;
    }

    memcpy(out->buf, str, len*sizeof(wchar_t));
    out->buf += len;
    out->len -= len;
    return len;
}

/*********************************************************************
 *              __stdio_common_vswprintf (UCRTBASE.@)
 */
int CDECL __stdio_common_vswprintf( unsigned __int64 options,
        wchar_t *str, size_t len, const wchar_t *format,
        _locale_t locale, va_list valist )
{
    struct _str_ctx_w ctx = {len, str};
    int ret;

    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);
    ret = pf_printf_w(puts_clbk_str_c99_w,
            &ctx, format, locale, options & UCRTBASE_PRINTF_MASK, arg_clbk_valist, NULL, &valist);
    puts_clbk_str_w(&ctx, 1, L"");

    if(!str)
        return ret;
    if(options & _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION)
        return ret>len ? -1 : ret;
    if(ret>=len) {
        if(len) str[len-1] = 0;
        if(options & _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR)
            return ret;
        return len > 0 ? -2 : -1;
    }
    return ret;
}

#endif /* _MSVCR_VER>=140 */

/*********************************************************************
 *		sprintf (MSVCRT.@)
 */
int WINAPIV sprintf( char *str, const char *format, ... )
{
    va_list ap;
    int r;

    va_start( ap, format );
    r = vsnprintf( str, INT_MAX, format, ap );
    va_end( ap );
    return r;
}

/*********************************************************************
 *		sprintf_s (MSVCRT.@)
 */
int WINAPIV sprintf_s( char *str, size_t num, const char *format, ... )
{
    va_list ap;
    int r;

    va_start( ap, format );
    r = vsnprintf( str, num, format, ap );
    va_end( ap );
    return r;
}

/*********************************************************************
 *		_scwprintf_l (MSVCRT.@)
 */
int WINAPIV _scwprintf_l( const wchar_t *format, _locale_t locale, ... )
{
    va_list ap;
    int r;

    va_start( ap, locale );
    r = _vsnwprintf_l( NULL, INT_MAX, format, locale, ap );
    va_end( ap );
    return r;
}

/*********************************************************************
 *		_scwprintf_p_l (MSVCRT.@)
 */
int WINAPIV _scwprintf_p_l( const wchar_t *format, _locale_t locale, ... )
{
    va_list ap;
    int r;

    va_start( ap, locale );
    r = vswprintf_p_l_opt( NULL, INT_MAX, format, 0, locale, ap );
    va_end( ap );
    return r;
}

#if _MSVCR_VER>=80
/*********************************************************************
 *		_scwprintf_p (MSVCRT.@)
 */
int WINAPIV _scwprintf_p( const wchar_t *format, ... )
{
    va_list ap;
    int r;

    va_start( ap, format );
    r = vswprintf_p_l_opt( NULL, INT_MAX, format, 0, NULL, ap );
    va_end( ap );
    return r;
}
#endif

/*********************************************************************
 *		_scwprintf (MSVCRT.@)
 */
int WINAPIV _scwprintf( const wchar_t *format, ... )
{
    va_list ap;
    int r;

    va_start( ap, format );
    r = _vsnwprintf( NULL, INT_MAX, format, ap );
    va_end( ap );
    return r;
}

/*********************************************************************
 *		swprintf (MSVCRT.@)
 */
int WINAPIV _swprintf( wchar_t *str, const wchar_t *format, ... )
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
int WINAPIV swprintf_s(wchar_t *str, size_t numberOfElements,
        const wchar_t *format, ... )
{
    va_list ap;
    int r;

    va_start(ap, format);
    r = _vsnwprintf_s(str, numberOfElements, INT_MAX, format, ap);
    va_end(ap);

    return r;
}

/*********************************************************************
 *              _swprintf_s_l (MSVCRT.@)
 */
int WINAPIV _swprintf_s_l(wchar_t *str, size_t numberOfElements,
        const wchar_t *format, _locale_t locale, ... )
{
    va_list ap;
    int r;

    va_start(ap, locale);
    r = _vsnwprintf_s_l(str, numberOfElements, INT_MAX, format, locale, ap);
    va_end(ap);

    return r;
}

/*********************************************************************
 *              _swprintf_c_l (MSVCRT.@)
 */
int WINAPIV _swprintf_c_l(wchar_t *str, size_t len,
        const wchar_t *format, _locale_t locale, ... )
{
    va_list ap;
    int r;

    va_start(ap, locale);
    r = _vswprintf_c_l(str, len, format, locale, ap);
    va_end(ap);

    return r;
}

/*********************************************************************
 *              _swprintf_c (MSVCRT.@)
 */
int WINAPIV _swprintf_c(wchar_t *str, size_t len,
        const wchar_t *format, ... )
{
    va_list ap;
    int r;

    va_start(ap, format);
    r = _vswprintf_c(str, len, format, ap);
    va_end(ap);

    return r;
}

/*********************************************************************
 *		_vswprintf (MSVCRT.@)
 */
int CDECL _vswprintf( wchar_t* str, const wchar_t* format, va_list args )
{
    return _vsnwprintf( str, INT_MAX, format, args );
}

/*********************************************************************
 *		_vswprintf (MSVCRT.@)
 */
int CDECL _vswprintf_l( wchar_t* str, const wchar_t* format,
        _locale_t locale, va_list args )
{
    return _vsnwprintf_l( str, INT_MAX, format, locale, args );
}

/*********************************************************************
 *		_vscwprintf (MSVCRT.@)
 */
int CDECL _vscwprintf( const wchar_t *format, va_list args )
{
    return _vsnwprintf( NULL, INT_MAX, format, args );
}

/*********************************************************************
 *		_vscwprintf_l (MSVCRT.@)
 */
int CDECL _vscwprintf_l( const wchar_t *format, _locale_t locale, va_list args )
{
    return _vsnwprintf_l( NULL, INT_MAX, format, locale, args );
}

/*********************************************************************
 *		_vscwprintf_p_l (MSVCRT.@)
 */
int CDECL _vscwprintf_p_l( const wchar_t *format, _locale_t locale, va_list args )
{
    return vswprintf_p_l_opt( NULL, INT_MAX, format, 0, locale, args );
}

#if _MSVCR_VER>=80
/*********************************************************************
 * _vscwprintf_p (MSVCR80.@)
 */
int CDECL _vscwprintf_p(const wchar_t *format, va_list args)
{
    return vswprintf_p_l_opt(NULL, INT_MAX, format, 0, NULL, args);
}
#endif

/*********************************************************************
 *		vswprintf_s (MSVCRT.@)
 */
int CDECL vswprintf_s(wchar_t* str, size_t numberOfElements,
        const wchar_t* format, va_list args)
{
    return _vsnwprintf_s(str, numberOfElements, INT_MAX, format, args );
}

/*********************************************************************
 *              _vswprintf_s_l (MSVCRT.@)
 */
int CDECL _vswprintf_s_l(wchar_t* str, size_t numberOfElements,
        const wchar_t* format, _locale_t locale, va_list args)
{
    return _vsnwprintf_s_l(str, numberOfElements, INT_MAX,
            format, locale, args );
}

static int vsprintf_p_l_opt(char *buffer, size_t length, const char *format,
        DWORD options, _locale_t locale, va_list args)
{
    static const char nullbyte = '\0';
    printf_arg args_ctx[_ARGMAX+1];
    struct _str_ctx_a puts_ctx = {length, buffer};
    int ret;

    memset(args_ctx, 0, sizeof(args_ctx));

    ret = create_positional_ctx_a(args_ctx, format, args);
    if(ret < 0) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *_errno() = EINVAL;
        return ret;
    } else if(ret == 0)
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale,
                MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options, arg_clbk_valist, NULL, &args);
    else
        ret = pf_printf_a(puts_clbk_str_a, &puts_ctx, format, locale,
                MSVCRT_PRINTF_POSITIONAL_PARAMS | MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER | options,
                arg_clbk_positional, args_ctx, NULL);

    puts_clbk_str_a(&puts_ctx, 1, &nullbyte);
    return ret;
}

/*********************************************************************
 *              _vsprintf_p_l (MSVCRT.@)
 */
int CDECL _vsprintf_p_l(char *buffer, size_t length, const char *format,
        _locale_t locale, va_list args)
{
    return vsprintf_p_l_opt(buffer, length, format, 0, locale, args);
}

/*********************************************************************
 *		_vsprintf_p (MSVCRT.@)
 */
int CDECL _vsprintf_p(char *buffer, size_t length,
        const char *format, va_list args)
{
    return _vsprintf_p_l(buffer, length, format, NULL, args);
}

#if _MSVCR_VER>=140
/*********************************************************************
 *              __stdio_common_vsprintf_p (UCRTBASE.@)
 */
int CDECL __stdio_common_vsprintf_p(unsigned __int64 options, char *buffer, size_t length,
        const char *format, _locale_t locale, va_list args)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %#I64x not handled\n", options);
    return vsprintf_p_l_opt(buffer, length, format, options & UCRTBASE_PRINTF_MASK, locale, args);
}
#endif

/*********************************************************************
 *		_sprintf_p_l (MSVCRT.@)
 */
int WINAPIV _sprintf_p_l(char *buffer, size_t length,
        const char *format, _locale_t locale, ...)
{
    va_list valist;
    int r;

    va_start(valist, locale);
    r = _vsprintf_p_l(buffer, length, format, locale, valist);
    va_end(valist);

    return r;
}

/*********************************************************************
 *		__swprintf_l (MSVCRT.@)
 */
int WINAPIV __swprintf_l( wchar_t *str, const wchar_t *format,
        _locale_t locale, ...)
{
    int retval;
    va_list valist;
    va_start(valist, locale);
    retval = _vswprintf_l(str, format, locale, valist);
    va_end(valist);
    return retval;
}

#if _MSVCR_VER>=80
/*********************************************************************
 * _sprintf_p (MSVCR80.@)
 */
int WINAPIV _sprintf_p(char *buffer, size_t length, const char *format, ...)
{
    va_list valist;
    int r;

    va_start(valist, format);
    r = _vsprintf_p_l(buffer, length, format, NULL, valist);
    va_end(valist);

    return r;
}
#endif

/*********************************************************************
 *		_swprintf_p (MSVCRT.@)
 */
int WINAPIV _swprintf_p(wchar_t *buffer, size_t length,
        const wchar_t *format, ...)
{
    va_list valist;
    int r;

    va_start(valist, format);
    r = vswprintf_p_l_opt(buffer, length, format, 0, NULL, valist);
    va_end(valist);

    return r;
}

/*********************************************************************
 *		_swprintf_p_l (MSVCRT.@)
 */
int WINAPIV _swprintf_p_l(wchar_t *buffer, size_t length,
        const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int r;

    va_start(valist, locale);
    r = vswprintf_p_l_opt(buffer, length, format, 0, locale, valist);
    va_end(valist);

    return r;
}

/*********************************************************************
 *              wcscmp (MSVCRT.@)
 */
int CDECL wcscmp(const wchar_t *str1, const wchar_t *str2)
{
    while (*str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }

    if (*str1 < *str2)
        return -1;
    if (*str1 > *str2)
        return 1;
    return 0;
}

/*********************************************************************
 *              _wcscoll_l (MSVCRT.@)
 */
int CDECL _wcscoll_l(const wchar_t* str1, const wchar_t* str2, _locale_t locale)
{
    pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[LC_COLLATE])
        return wcscmp(str1, str2);
    return CompareStringW(locinfo->lc_handle[LC_COLLATE], SORT_STRINGSORT,
              str1, -1, str2, -1)-CSTR_EQUAL;
}

/*********************************************************************
 *		wcscoll (MSVCRT.@)
 */
int CDECL wcscoll( const wchar_t* str1, const wchar_t* str2 )
{
    return _wcscoll_l(str1, str2, NULL);
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

/*********************************************************************
 *		wcstok_s  (MSVCRT.@)
 */
wchar_t * CDECL wcstok_s( wchar_t *str, const wchar_t *delim,
                                 wchar_t **next_token )
{
    wchar_t *ret;

    if (!MSVCRT_CHECK_PMT(delim != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(next_token != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(str != NULL || *next_token != NULL)) return NULL;

    if (!str) str = *next_token;

    while (*str && wcschr( delim, *str )) str++;
    if (!*str) ret = NULL;
    else
    {
        ret = str++;
        while (*str && !wcschr( delim, *str )) str++;
        if (*str) *str++ = 0;
    }
    *next_token = str;
    return ret;
}

/*********************************************************************
 *		wcstok  (MSVCRT.@)
 */
#if _MSVCR_VER>=140
wchar_t * CDECL wcstok( wchar_t *str, const wchar_t *delim, wchar_t **ctx )
{
    if (!ctx)
        ctx = &msvcrt_get_thread_data()->wcstok_next;
    return wcstok_s(str, delim, ctx);
}
#else
wchar_t * CDECL wcstok( wchar_t *str, const wchar_t *delim )
{
    return wcstok_s(str, delim, &msvcrt_get_thread_data()->wcstok_next);
}
#endif

/*********************************************************************
 *		_wctomb_s_l (MSVCRT.@)
 */
int CDECL _wctomb_s_l(int *len, char *mbchar, size_t size,
        wchar_t wch, _locale_t locale)
{
    pthreadlocinfo locinfo;
    BOOL error = FALSE;
    BOOL *perror;
    int mblen;

    if(!mbchar && size>0) {
        if(len)
            *len = 0;
        return 0;
    }

    if(len)
        *len = -1;

    if(!MSVCRT_CHECK_PMT(size <= INT_MAX))
        return EINVAL;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_codepage) {
        if(wch > 0xff) {
            if(mbchar && size>0)
                memset(mbchar, 0, size);
            *_errno() = EILSEQ;
            return EILSEQ;
        }

        if(!MSVCRT_CHECK_PMT_ERR(size >= 1, ERANGE))
            return ERANGE;

        *mbchar = wch;
        if(len)
            *len = 1;
        return 0;
    }

    perror = (locinfo->lc_codepage != CP_UTF8 ? &error : NULL);
    mblen = WideCharToMultiByte(locinfo->lc_codepage, 0, &wch, 1, mbchar, size, NULL, perror);
    if(!mblen || error) {
        if(!mblen && GetLastError()==ERROR_INSUFFICIENT_BUFFER) {
            if(mbchar && size>0)
                memset(mbchar, 0, size);

            MSVCRT_INVALID_PMT("insufficient buffer size", ERANGE);
            return ERANGE;
        }

        *_errno() = EILSEQ;
        return EILSEQ;
    }

    if(len)
        *len = mblen;
    return 0;
}

/*********************************************************************
 *              wctomb_s (MSVCRT.@)
 */
int CDECL wctomb_s(int *len, char *mbchar, size_t size, wchar_t wch)
{
    return _wctomb_s_l(len, mbchar, size, wch, NULL);
}

/*********************************************************************
 *              _wctomb_l (MSVCRT.@)
 */
int CDECL _wctomb_l(char *dst, wchar_t ch, _locale_t locale)
{
    int len;

    _wctomb_s_l(&len, dst, dst ? MB_LEN_MAX : 0, ch, locale);
    return len;
}

/*********************************************************************
 *		wctomb (MSVCRT.@)
 */
INT CDECL wctomb( char *dst, wchar_t ch )
{
    return _wctomb_l(dst, ch, NULL);
}

/*********************************************************************
 *		wctob (MSVCRT.@)
 */
INT CDECL wctob( wint_t wchar )
{
    char out;
    BOOL error = FALSE;
    BOOL *perror;
    UINT codepage = get_locinfo()->lc_codepage;

    perror = (codepage != CP_UTF8 ? &error : NULL);

    if(!codepage) {
        if (wchar < 0xff)
            return (signed char)wchar;
        else
            return EOF;
    } else if(WideCharToMultiByte( codepage, 0, &wchar, 1, &out, 1, NULL, perror ) && !error)
        return (INT)out;
    return EOF;
}

/*********************************************************************
 *              wcrtomb_s (MSVCRT.@)
 */
INT CDECL wcrtomb_s(size_t *len, char *mbchar,
        size_t size, wchar_t wch, mbstate_t *s)
{
    int ilen, ret;

    if (s) *s = 0;
    ret = wctomb_s(&ilen, mbchar, size, wch);
    if (len) *len = ilen;
    return ret;
}

/*********************************************************************
 *              wcrtomb (MSVCRT.@)
 */
size_t CDECL wcrtomb( char *dst, wchar_t ch, mbstate_t *s)
{
    if(s)
        *s = 0;
    return wctomb(dst, ch);
}

/*********************************************************************
 *		_iswctype_l (MSVCRT.@)
 */
INT CDECL _iswctype_l( wchar_t wc, wctype_t type, _locale_t locale )
{
    WORD ct;

    if (wc == WEOF) return 0;
    if (wc < 256) return MSVCRT__pwctype[wc] & type;

    if (!GetStringTypeW(CT_CTYPE1, &wc, 1, &ct))
    {
        ERR("GetStringTypeW failed for %x\n", wc);
        return 0;
    }
    return ct & type;
}

/*********************************************************************
 *		iswctype    (MSVCRT.@)
 */
INT CDECL iswctype( wchar_t wc, wctype_t type )
{
    return _iswctype_l( wc, type, NULL );
}

/*********************************************************************
 *		_iswalnum_l (MSVCRT.@)
 */
int CDECL _iswalnum_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _ALPHA | _DIGIT, locale );
}

/*********************************************************************
 *		iswalnum (MSVCRT.@)
 */
INT CDECL iswalnum( wchar_t wc )
{
    return _iswalnum_l( wc, NULL );
}

/*********************************************************************
 *              iswalpha_l (MSVCRT.@)
 */
INT CDECL _iswalpha_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _ALPHA, locale );
}

/*********************************************************************
 *		iswalpha (MSVCRT.@)
 */
INT CDECL iswalpha( wchar_t wc )
{
    return _iswalpha_l( wc, NULL );
}

/*********************************************************************
 *		_iswcntrl_l (MSVCRT.@)
 */
int CDECL _iswcntrl_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _CONTROL, locale );
}

/*********************************************************************
 *		iswcntrl (MSVCRT.@)
 */
INT CDECL iswcntrl( wchar_t wc )
{
    return _iswcntrl_l( wc, NULL );
}

/*********************************************************************
 *		_iswdigit_l (MSVCRT.@)
 */
INT CDECL _iswdigit_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _DIGIT, locale );
}

/*********************************************************************
 *		iswdigit (MSVCRT.@)
 */
INT CDECL iswdigit( wchar_t wc )
{
    return _iswdigit_l( wc, NULL );
}

/*********************************************************************
 *		_iswgraph_l (MSVCRT.@)
 */
int CDECL _iswgraph_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _ALPHA | _DIGIT | _PUNCT, locale );
}

/*********************************************************************
 *		iswgraph (MSVCRT.@)
 */
INT CDECL iswgraph( wchar_t wc )
{
    return _iswgraph_l( wc, NULL );
}

/*********************************************************************
 *		_iswlower_l (MSVCRT.@)
 */
int CDECL _iswlower_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _LOWER, locale );
}

/*********************************************************************
 *		iswlower (MSVCRT.@)
 */
INT CDECL iswlower( wchar_t wc )
{
    return _iswlower_l( wc, NULL );
}

/*********************************************************************
 *		_iswprint_l (MSVCRT.@)
 */
int CDECL _iswprint_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _ALPHA | _BLANK | _DIGIT | _PUNCT, locale );
}

/*********************************************************************
 *		iswprint (MSVCRT.@)
 */
INT CDECL iswprint( wchar_t wc )
{
    return _iswprint_l( wc, NULL );
}

/*********************************************************************
 *		_iswpunct_l (MSVCRT.@)
 */
INT CDECL _iswpunct_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _PUNCT, locale );
}

/*********************************************************************
 *		iswpunct (MSVCRT.@)
 */
INT CDECL iswpunct( wchar_t wc )
{
    return _iswpunct_l( wc, NULL );
}

/*********************************************************************
 *		_iswspace_l (MSVCRT.@)
 */
INT CDECL _iswspace_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _SPACE, locale );
}

/*********************************************************************
 *		iswspace (MSVCRT.@)
 */
INT CDECL iswspace( wchar_t wc )
{
    return _iswspace_l( wc, NULL );
}

/*********************************************************************
 *		_iswupper_l (MSVCRT.@)
 */
int CDECL _iswupper_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _UPPER, locale );
}

/*********************************************************************
 *		iswupper (MSVCRT.@)
 */
INT CDECL iswupper( wchar_t wc )
{
    return _iswupper_l( wc, NULL );
}

/*********************************************************************
 *		_iswxdigit_l (MSVCRT.@)
 */
int CDECL _iswxdigit_l( wchar_t wc, _locale_t locale )
{
    return _iswctype_l( wc, _HEX, locale );
}

/*********************************************************************
 *		iswxdigit (MSVCRT.@)
 */
INT CDECL iswxdigit( wchar_t wc )
{
    return _iswxdigit_l( wc, NULL );
}

/*********************************************************************
 *		_iswblank_l (MSVCRT.@)
 */
INT CDECL _iswblank_l( wchar_t wc, _locale_t locale )
{
    return wc == '\t' || _iswctype_l( wc, _BLANK, locale );
}

/*********************************************************************
 *		iswblank (MSVCRT.@)
 */
INT CDECL iswblank( wchar_t wc )
{
    return wc == '\t' || _iswctype_l( wc, _BLANK, NULL );
}

/*********************************************************************
 *		wcscpy_s (MSVCRT.@)
 */
INT CDECL wcscpy_s( wchar_t* wcDest, size_t numElement, const  wchar_t *wcSrc)
{
    size_t size = 0;

    if(!MSVCRT_CHECK_PMT(wcDest)) return EINVAL;
    if(!MSVCRT_CHECK_PMT(numElement)) return EINVAL;

    if(!MSVCRT_CHECK_PMT(wcSrc))
    {
        wcDest[0] = 0;
        return EINVAL;
    }

    size = wcslen(wcSrc) + 1;

    if(!MSVCRT_CHECK_PMT_ERR(size <= numElement, ERANGE))
    {
        wcDest[0] = 0;
        return ERANGE;
    }

    memmove( wcDest, wcSrc, size*sizeof(WCHAR) );

    return 0;
}

/***********************************************************************
 *             wcscpy (MSVCRT.@)
 */
wchar_t* __cdecl wcscpy( wchar_t *dst, const wchar_t *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}

/******************************************************************
 *		wcsncpy (MSVCRT.@)
 */
wchar_t* __cdecl wcsncpy( wchar_t* s1, const wchar_t *s2, size_t n )
{
    size_t i;

    for(i=0; i<n; i++)
        if(!(s1[i] = s2[i])) break;
    for(; i<n; i++)
        s1[i] = 0;
    return s1;
}

/******************************************************************
 *		wcsncpy_s (MSVCRT.@)
 */
INT CDECL wcsncpy_s( wchar_t *dst, size_t elem, const wchar_t *src, size_t count )
{
    WCHAR *p = dst;
    BOOL truncate = (count == _TRUNCATE);

    if (!count)
    {
        if (dst && elem) *dst = 0;
        return 0;
    }

    if (!MSVCRT_CHECK_PMT(dst != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(elem != 0)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(src != NULL))
    {
        *dst = 0;
        return EINVAL;
    }

    while (elem && count && *src)
    {
        *p++ = *src++;
        elem--;
        count--;
    }
    if (!elem && truncate)
    {
        *(p-1) = 0;
        return STRUNCATE;
    }
    else if (!elem)
    {
        *dst = 0;
        return ERANGE;
    }
    *p = 0;
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

/***********************************************************************
 *           wcscat (MSVCRT.@)
 */
wchar_t* __cdecl wcscat( wchar_t *dst, const wchar_t *src )
{
    wcscpy( dst + wcslen(dst), src );
    return dst;
}

/*********************************************************************
 *           wcsncat_s (MSVCRT.@)
 */
errno_t CDECL wcsncat_s(wchar_t *dst, size_t elem, const wchar_t *src, size_t count)
{
    size_t i, j;

    if (!MSVCRT_CHECK_PMT(dst != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(elem > 0)) return EINVAL;
    if (count == 0) return 0;
    if (!MSVCRT_CHECK_PMT(src != NULL))
    {
        *dst = 0;
        return EINVAL;
    }

    for (i = 0; i < elem; i++) if (!dst[i]) break;

    if (i == elem)
    {
        MSVCRT_INVALID_PMT("dst[elem] is not NULL terminated\n", EINVAL);
        *dst = 0;
        return EINVAL;
    }

    for (j = 0; (j + i) < elem; j++)
    {
        if(count == _TRUNCATE && j + i == elem - 1)
        {
            dst[j + i] = '\0';
            return STRUNCATE;
        }
        if(j == count || (dst[j + i] = src[j]) == '\0')
        {
            dst[j + i] = '\0';
            return 0;
        }
    }

    MSVCRT_INVALID_PMT("dst[elem] is too small", ERANGE);
    dst[0] = '\0';
    return ERANGE;
}

/*********************************************************************
 *           wcsncat    (NTDLL.@)
 */
wchar_t * __cdecl wcsncat(wchar_t *s1, const wchar_t *s2, size_t n)
{
    wchar_t *ret = s1;
    while (*s1) s1++;
    while (n-- > 0) if (!(*s1++ = *s2++)) return ret;
    *s1 = 0;
    return ret;
}

/*********************************************************************
 * wctoint (INTERNAL)
 */
static int wctoint(WCHAR c, int base)
{
    int v = -1;
    if ('0' <= c && c <= '9')
        v = c - '0';
    else if ('A' <= c && c <= 'Z')
        v = c - 'A' + 10;
    else if ('a' <= c && c <= 'z')
        v = c - 'a' + 10;
    else {
        /* NOTE: MAP_FOLDDIGITS supports too many things. */
        /* Unicode points that contain digits 0-9; keep this sorted! */
        static const WCHAR zeros[] = {
            0x660, 0x6f0, 0x966, 0x9e6, 0xa66, 0xae6, 0xb66, 0xc66, 0xce6,
            0xd66, 0xe50, 0xed0, 0xf20, 0x1040, 0x17e0, 0x1810, 0xff10
        };
        int i;
        for (i = 0; i < ARRAY_SIZE(zeros) && c >= zeros[i]; ++i) {
            if (zeros[i] <= c && c <= zeros[i] + 9) {
                v = c - zeros[i];
                break;
            }
        }
    }
    return v < base ? v : -1;
}

/*********************************************************************
 *  _wcstoi64_l (MSVCRT.@)
 */
__int64 CDECL _wcstoi64_l(const wchar_t *nptr,
        wchar_t **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE, empty = TRUE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    if(endptr)
        *endptr = (wchar_t*)nptr;

    while(_iswspace_l(*nptr, locale)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && wctoint(*nptr, 1)==0 && (nptr[1]=='x' || nptr[1]=='X')) {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(wctoint(*nptr, 1)==0)
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        int v = wctoint(*nptr, base);
        if(v<0)
            break;

        if(negative)
            v = -v;

        nptr++;
        empty = FALSE;

        if(!negative && (ret>I64_MAX/base || ret*base>I64_MAX-v)) {
            ret = I64_MAX;
            *_errno() = ERANGE;
        } else if(negative && (ret<I64_MIN/base || ret*base<I64_MIN-v)) {
            ret = I64_MIN;
            *_errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr && !empty)
        *endptr = (wchar_t*)nptr;

    return ret;
}

/*********************************************************************
 *  _wcstoi64 (MSVCRT.@)
 */
__int64 CDECL _wcstoi64(const wchar_t *nptr,
        wchar_t **endptr, int base)
{
    return _wcstoi64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _wcstol_l (MSVCRT.@)
 */
__msvcrt_long CDECL _wcstol_l(const wchar_t *s,
        wchar_t **end, int base, _locale_t locale)
{
    __int64 ret = _wcstoi64_l(s, end, base, locale);

    if(ret > LONG_MAX) {
        ret = LONG_MAX;
        *_errno() = ERANGE;
    }else if(ret < LONG_MIN) {
        ret = LONG_MIN;
        *_errno() = ERANGE;
    }
    return ret;
}

/*********************************************************************
 *  wcstol (MSVCRT.@)
 */
__msvcrt_long CDECL wcstol(const wchar_t *s,
        wchar_t **end, int base)
{
    return _wcstol_l(s, end, base, NULL);
}

/*********************************************************************
 *  _wtoi_l (MSVCRT.@)
 */
int __cdecl _wtoi_l(const wchar_t *str, _locale_t locale)
{
    __int64 ret = _wcstoi64_l(str, NULL, 10, locale);

    if(ret > INT_MAX) {
        ret = INT_MAX;
        *_errno() = ERANGE;
    } else if(ret < INT_MIN) {
        ret = INT_MIN;
        *_errno() = ERANGE;
    }
    return ret;
}

/*********************************************************************
 *  _wtoi (MSVCRT.@)
 */
int __cdecl _wtoi(const wchar_t *str)
{
    return _wtoi_l(str, NULL);
}

/*********************************************************************
 *  _wtol_l (MSVCRT.@)
 */
__msvcrt_long __cdecl _wtol_l(const wchar_t *str, _locale_t locale)
{
    __int64 ret = _wcstoi64_l(str, NULL, 10, locale);

    if(ret > LONG_MAX) {
        ret = LONG_MAX;
        *_errno() = ERANGE;
    } else if(ret < LONG_MIN) {
        ret = LONG_MIN;
        *_errno() = ERANGE;
    }
    return ret;
}

/*********************************************************************
 *  _wtol (MSVCRT.@)
 */
__msvcrt_long __cdecl _wtol(const wchar_t *str)
{
    return _wtol_l(str, NULL);
}

#if _MSVCR_VER>=120

/*********************************************************************
 *  _wtoll_l (MSVCR120.@)
 */
__int64 __cdecl _wtoll_l(const wchar_t *str, _locale_t locale)
{
    return _wcstoi64_l(str, NULL, 10, locale);
}

/*********************************************************************
 *  _wtoll (MSVCR120.@)
 */
__int64 __cdecl _wtoll(const wchar_t *str)
{
    return _wtoll_l(str, NULL);
}

#endif /* _MSVCR_VER>=120 */

/*********************************************************************
 *  _wcstoui64_l (MSVCRT.@)
 */
unsigned __int64 CDECL _wcstoui64_l(const wchar_t *nptr,
        wchar_t **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE, empty = TRUE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    if(endptr)
        *endptr = (wchar_t*)nptr;

    while(_iswspace_l(*nptr, locale)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && wctoint(*nptr, 1)==0 && (nptr[1]=='x' || nptr[1]=='X')) {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(wctoint(*nptr, 1)==0)
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        int v = wctoint(*nptr, base);
        if(v<0)
            break;

        nptr++;
        empty = FALSE;

        if(ret>UI64_MAX/base || ret*base>UI64_MAX-v) {
            ret = UI64_MAX;
            *_errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr && !empty)
        *endptr = (wchar_t*)nptr;

    return negative ? -ret : ret;
}

/*********************************************************************
 *  _wcstoui64 (MSVCRT.@)
 */
unsigned __int64 CDECL _wcstoui64(const wchar_t *nptr,
        wchar_t **endptr, int base)
{
    return _wcstoui64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _wcstoul_l (MSVCRT.@)
 */
__msvcrt_ulong __cdecl _wcstoul_l(const wchar_t *s,
        wchar_t **end, int base, _locale_t locale)
{
    __int64 ret = _wcstoi64_l(s, end, base, locale);

    if(ret > ULONG_MAX) {
        ret = ULONG_MAX;
        *_errno() = ERANGE;
    }else if(ret < -(__int64)ULONG_MAX) {
        ret = 1;
        *_errno() = ERANGE;
    }
    return ret;
}

/*********************************************************************
   *  wcstoul (MSVCRT.@)
    */
__msvcrt_ulong __cdecl wcstoul(const wchar_t *s, wchar_t **end, int base)
{
    return _wcstoul_l(s, end, base, NULL);
}

/******************************************************************
 *  wcsnlen (MSVCRT.@)
 */
size_t CDECL wcsnlen(const wchar_t *s, size_t maxlen)
{
    size_t i;

    for (i = 0; i < maxlen; i++)
        if (!s[i]) break;
    return i;
}

/*********************************************************************
 *              _towupper_l (MSVCRT.@)
 */
wint_t CDECL _towupper_l(wint_t c, _locale_t locale)
{
    pthreadlocinfo locinfo;
    wchar_t ret;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[LC_CTYPE]) {
        if(c >= 'a' && c <= 'z')
            return c + 'A' - 'a';
        return c;
    }

    if(!LCMapStringW(locinfo->lc_handle[LC_CTYPE], LCMAP_UPPERCASE, &c, 1, &ret, 1))
        return c;
    return ret;
}

/*********************************************************************
 *              towupper (MSVCRT.@)
 */
wint_t CDECL towupper(wint_t c)
{
    return _towupper_l(c, NULL);
}

/*********************************************************************
 *              wcschr (MSVCRT.@)
 */
wchar_t* CDECL wcschr(const wchar_t *str, wchar_t ch)
{
    do { if (*str == ch) return (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return NULL;
}

/*********************************************************************
 *              wcsrchr (MSVCRT.@)
 */
wchar_t* CDECL wcsrchr(const wchar_t *str, wchar_t ch)
{
    WCHAR *ret = NULL;
    do { if (*str == ch) ret = (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return ret;
}

/***********************************************************************
 *              wcslen (MSVCRT.@)
 */
size_t CDECL wcslen(const wchar_t *str)
{
    const wchar_t *s = str;
    while (*s) s++;
    return s - str;
}

/*********************************************************************
 *              wcsstr (MSVCRT.@)
 */
wchar_t* CDECL wcsstr(const wchar_t *str, const wchar_t *sub)
{
    while(*str)
    {
        const wchar_t *p1 = str, *p2 = sub;
        while(*p1 && *p2 && *p1 == *p2)
        {
            p1++;
            p2++;
        }
        if(!*p2)
            return (wchar_t*)str;
        str++;
    }
    return NULL;
}

/*********************************************************************
 *              _wtoi64_l (MSVCRT.@)
 */
__int64 CDECL _wtoi64_l(const wchar_t *str, _locale_t locale)
{
    ULONGLONG RunningTotal = 0;
    BOOL bMinus = FALSE;

    while (_iswspace_l(*str, locale)) {
        str++;
    } /* while */

    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        bMinus = TRUE;
        str++;
    } /* if */

    while (*str >= '0' && *str <= '9') {
        RunningTotal = RunningTotal * 10 + *str - '0';
        str++;
    } /* while */

    return bMinus ? -RunningTotal : RunningTotal;
}

/*********************************************************************
 *              _wtoi64 (MSVCRT.@)
 */
__int64 CDECL _wtoi64(const wchar_t *str)
{
    return _wtoi64_l(str, NULL);
}

/*********************************************************************
 *              _wcsxfrm_l (MSVCRT.@)
 */
size_t CDECL _wcsxfrm_l(wchar_t *dest, const wchar_t *src,
        size_t len, _locale_t locale)
{
    pthreadlocinfo locinfo;
    int i, ret;

    if(!MSVCRT_CHECK_PMT(src)) return INT_MAX;
    if(!MSVCRT_CHECK_PMT(dest || !len)) return INT_MAX;

    if(len > INT_MAX) {
        FIXME("len > INT_MAX not supported\n");
        len = INT_MAX;
    }

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(!locinfo->lc_handle[LC_COLLATE]) {
        wcsncpy(dest, src, len);
        return wcslen(src);
    }

    ret = LCMapStringW(locinfo->lc_handle[LC_COLLATE],
            LCMAP_SORTKEY, src, -1, NULL, 0);
    if(!ret) {
        if(len) dest[0] = 0;
        *_errno() = EILSEQ;
        return INT_MAX;
    }
    if(!len) return ret-1;

    if(ret > len) {
        dest[0] = 0;
        *_errno() = ERANGE;
        return ret-1;
    }

    ret = LCMapStringW(locinfo->lc_handle[LC_COLLATE],
            LCMAP_SORTKEY, src, -1, dest, len) - 1;
    for(i=ret; i>=0; i--)
        dest[i] = ((unsigned char*)dest)[i];
    return ret;
}

/*********************************************************************
 *              wcsxfrm (MSVCRT.@)
 */
size_t CDECL wcsxfrm(wchar_t *dest, const wchar_t *src, size_t len)
{
    return _wcsxfrm_l(dest, src, len, NULL);
}
