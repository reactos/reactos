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
#include <precomp.h>
#include <assert.h>

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
    size_t size = (strlenW(str) + 1) * sizeof(wchar_t);
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
    _set_errno(EINVAL);
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
  _set_errno(EINVAL);
  return EINVAL;
}

/*********************************************************************
 *		wcstod (MSVCRT.@)
 */
double CDECL wcstod(const wchar_t* lpszStr, wchar_t** end)
{
  const wchar_t* str = lpszStr;
  int negative = 0;
  double ret = 0, divisor = 10.0;

  TRACE("(%s,%p) semi-stub\n", debugstr_w(lpszStr), end);

  /* FIXME:
   * - Should set errno on failure
   * - Should fail on overflow
   * - Need to check which input formats are allowed
   */
  while (isspaceW(*str))
    str++;

  if (*str == '-')
  {
    negative = 1;
    str++;
  }

  while (isdigitW(*str))
  {
    ret = ret * 10.0 + (*str - '0');
    str++;
  }
  if (*str == '.')
    str++;
  while (isdigitW(*str))
  {
    ret = ret + (*str - '0') / divisor;
    divisor *= 10;
    str++;
  }

  if (*str == 'E' || *str == 'e' || *str == 'D' || *str == 'd')
  {
    int negativeExponent = 0;
    int exponent = 0;
    if (*(++str) == '-')
    {
      negativeExponent = 1;
      str++;
    }
    while (isdigitW(*str))
    {
      exponent = exponent * 10 + (*str - '0');
      str++;
    }
    if (exponent != 0)
    {
      if (negativeExponent)
        ret = ret / pow(10.0, exponent);
      else
        ret = ret * pow(10.0, exponent);
    }
  }

  if (negative)
    ret = -ret;

  if (end)
    *end = (wchar_t*)str;

  TRACE("returning %g\n", ret);
  return ret;
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

/*********************************************************************
 *  wcsncat_s (MSVCRT.@)
 *
 */
INT CDECL wcsncat_s(wchar_t *dst, size_t elem,
        const wchar_t *src, size_t count)
{
    size_t srclen;
    wchar_t dststart;
    INT ret = 0;

    if (!MSVCRT_CHECK_PMT(dst != NULL) || !MSVCRT_CHECK_PMT(elem > 0))
    {
#ifndef _LIBCNT_
        _set_errno(EINVAL);
#endif
        return EINVAL;
    }
    if (!MSVCRT_CHECK_PMT(src != NULL || count == 0))
        return EINVAL;
    if (count == 0)
        return 0;

    for (dststart = 0; dststart < elem; dststart++)
    {
        if (dst[dststart] == '\0')
            break;
    }
    if (dststart == elem)
    {
        MSVCRT_INVALID_PMT("dst[elem] is not NULL terminated\n", EINVAL);
        return EINVAL;
    }

    if (count == _TRUNCATE)
    {
        srclen = strlenW(src);
        if (srclen >= (elem - dststart))
        {
            srclen = elem - dststart - 1;
            ret = STRUNCATE;
        }
    }
    else
        srclen = min(strlenW(src), count);
    if (srclen < (elem - dststart))
    {
        memcpy(&dst[dststart], src, srclen*sizeof(wchar_t));
        dst[dststart+srclen] = '\0';
        return ret;
    }
    MSVCRT_INVALID_PMT("dst[elem] is too small", ERANGE);
    dst[0] = '\0';
    return ERANGE;
}

