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
INT CDECL wctomb(char *mbchar, wchar_t wchar)
{
    BOOL bUsedDefaultChar;
    char chMultiByte[MB_LEN_MAX];
    int nBytes;

    /* At least one parameter needs to be given, the length of a null character cannot be queried (verified by tests under WinXP SP2) */
    if(!mbchar && !wchar)
        return 0;

    /* Use WideCharToMultiByte for doing the conversion using the codepage currently set with setlocale() */
    nBytes = WideCharToMultiByte(MSVCRT___lc_codepage, 0, &wchar, 1, chMultiByte, MB_LEN_MAX, NULL, &bUsedDefaultChar);

    /* Only copy the character if an 'mbchar' pointer was given.

       The "C" locale is emulated with codepage 1252 here. This codepage has a default character "?", but the "C" locale doesn't have one.
       Therefore don't copy the character in this case. */
    if(mbchar && !(MSVCRT_current_lc_all[0] == 'C' && !MSVCRT_current_lc_all[1] && bUsedDefaultChar))
        memcpy(mbchar, chMultiByte, nBytes);

    /* If the default character was used, set errno to EILSEQ and return -1. */
    if(bUsedDefaultChar)
    {
        _set_errno(EILSEQ);
        return -1;
    }

    /* Otherwise return the number of bytes this character occupies. */
    return nBytes;
}

size_t CDECL wcstombs(char *mbstr, const wchar_t *wcstr, size_t count)
{
    BOOL bUsedDefaultChar;
    char* p = mbstr;
    int nResult;

    /* Does the caller query for output buffer size? */
    if(!mbstr)
    {
        int nLength;

        /* If we currently use the "C" locale, the length of the input string is returned (verified by tests under WinXP SP2) */
        if(MSVCRT_current_lc_all[0] == 'C' && !MSVCRT_current_lc_all[1])
            return wcslen(wcstr);

        /* Otherwise check the length each character needs and build a final return value out of this */
        count = wcslen(wcstr);
        nLength = 0;

        while((int)(--count) >= 0 && *wcstr)
        {
            /* Get the length of this character */
            nResult = wctomb(NULL, *wcstr++);

            /* If this character is not convertible in the current locale, the end result will be -1 */
            if(nResult == -1)
                return -1;

            nLength += nResult;
        }

        /* Return the final length */
        return nLength;
    }

    /* Convert the string then */
    bUsedDefaultChar = FALSE;

    for(;;)
    {
        char chMultiByte[MB_LEN_MAX];
        UINT uLength;

        /* Are we at the terminating null character? */
        if(!*wcstr)
        {
            /* Set the null character, but don't increment the pointer as the returned length never includes the terminating null character */
            *p = 0;
            break;
        }

        /* Convert this character into the temporary chMultiByte variable */
        ZeroMemory(chMultiByte, MB_LEN_MAX);
        nResult = wctomb(chMultiByte, *wcstr++);

        /* Check if this was an invalid character */
        if(nResult == -1)
            bUsedDefaultChar = TRUE;

        /* If we got no character, stop the conversion process here */
        if(!chMultiByte[0])
            break;

        /* Determine whether this is a double-byte or a single-byte character */
        if(chMultiByte[1])
            uLength = 2;
        else
            uLength = 1;

        /* Decrease 'count' by the character length and check if the buffer can still hold the full character */
        count -= uLength;

        if((int)count < 0)
            break;

        /* It can, so copy it and move the pointer forward */
        memcpy(p, chMultiByte, uLength);
        p += uLength;
    }

    if(bUsedDefaultChar)
        return -1;

    /* Return the length in bytes of the copied characters (without the terminating null character) */
    return p - mbstr;
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
        MSVCRT_INVALID_PMT("dst[elem] is not NULL terminated\n");
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
    MSVCRT_INVALID_PMT("dst[elem] is too small");
    dst[0] = '\0';
    return ERANGE;
}

