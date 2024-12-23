/*
 * Shlwapi string functions
 *
 * Copyright 1998 Juergen Schmied
 * Copyright 2002 Jon Griffiths
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

#include "config.h"
#include "wine/port.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "mlang.h"
#include "ddeml.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern HINSTANCE shlwapi_hInstance;

static HRESULT _SHStrDupAA(LPCSTR,LPSTR*);
static HRESULT _SHStrDupAW(LPCWSTR,LPSTR*);


static void FillNumberFmt(NUMBERFMTW *fmt, LPWSTR decimal_buffer, int decimal_bufwlen,
                          LPWSTR thousand_buffer, int thousand_bufwlen)
{
  WCHAR grouping[64];
  WCHAR *c;

  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO|LOCALE_RETURN_NUMBER, (LPWSTR)&fmt->LeadingZero, sizeof(fmt->LeadingZero)/sizeof(WCHAR));
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER|LOCALE_RETURN_NUMBER, (LPWSTR)&fmt->LeadingZero, sizeof(fmt->NegativeOrder)/sizeof(WCHAR));
  fmt->NumDigits = 0;
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimal_buffer, decimal_bufwlen);
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousand_buffer, thousand_bufwlen);
  fmt->lpThousandSep = thousand_buffer;
  fmt->lpDecimalSep = decimal_buffer;

  /* 
   * Converting grouping string to number as described on 
   * http://blogs.msdn.com/oldnewthing/archive/2006/04/18/578251.aspx
   */
  fmt->Grouping = 0;
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping, sizeof(grouping)/sizeof(WCHAR));
  for (c = grouping; *c; c++)
    if (*c >= '0' && *c < '9')
    {
      fmt->Grouping *= 10;
      fmt->Grouping += *c - '0';
    }

  if (fmt->Grouping % 10 == 0)
    fmt->Grouping /= 10;
  else
    fmt->Grouping *= 10;
}

/*************************************************************************
 * FormatInt   [internal]
 *
 * Format an integer according to the current locale
 *
 * RETURNS
 *  The number of characters written on success or 0 on failure
 */
static int FormatInt(LONGLONG qdwValue, LPWSTR pszBuf, int cchBuf)
{
  NUMBERFMTW fmt;
  WCHAR decimal[8], thousand[8];
  WCHAR buf[24];
  WCHAR *c;
  BOOL neg = (qdwValue < 0);

  FillNumberFmt(&fmt, decimal, sizeof decimal / sizeof (WCHAR),
                thousand, sizeof thousand / sizeof (WCHAR));

  c = &buf[24];
  *(--c) = 0;
  do
  {
    *(--c) = '0' + (qdwValue%10);
    qdwValue /= 10;
  } while (qdwValue > 0);
  if (neg)
    *(--c) = '-';
  
  return GetNumberFormatW(LOCALE_USER_DEFAULT, 0, c, &fmt, pszBuf, cchBuf);
}

/*************************************************************************
 * FormatDouble   [internal]
 *
 * Format an integer according to the current locale. Prints the specified number of digits
 * after the decimal point
 *
 * RETURNS
 *  The number of characters written on success or 0 on failure
 */
static int FormatDouble(double value, int decimals, LPWSTR pszBuf, int cchBuf)
{
  static const WCHAR flfmt[] = {'%','f',0};
  WCHAR buf[64];
  NUMBERFMTW fmt;
  WCHAR decimal[8], thousand[8];
  
  snprintfW(buf, 64, flfmt, value);

  FillNumberFmt(&fmt, decimal, sizeof decimal / sizeof (WCHAR),
                 thousand, sizeof thousand / sizeof (WCHAR));
  fmt.NumDigits = decimals;
  return GetNumberFormatW(LOCALE_USER_DEFAULT, 0, buf, &fmt, pszBuf, cchBuf);
}

/*************************************************************************
 * SHLWAPI_ChrCmpHelperA
 *
 * Internal helper for SHLWAPI_ChrCmpA/ChrCMPIA.
 *
 * NOTES
 *  Both this function and its Unicode counterpart are very inefficient. To
 *  fix this, CompareString must be completely implemented and optimised
 *  first. Then the core character test can be taken out of that function and
 *  placed here, so that it need never be called at all. Until then, do not
 *  attempt to optimise this code unless you are willing to test that it
 *  still performs correctly.
 */
static BOOL SHLWAPI_ChrCmpHelperA(WORD ch1, WORD ch2, DWORD dwFlags)
{
  char str1[3], str2[3];

  str1[0] = LOBYTE(ch1);
  if (IsDBCSLeadByte(str1[0]))
  {
    str1[1] = HIBYTE(ch1);
    str1[2] = '\0';
  }
  else
    str1[1] = '\0';

  str2[0] = LOBYTE(ch2);
  if (IsDBCSLeadByte(str2[0]))
  {
    str2[1] = HIBYTE(ch2);
    str2[2] = '\0';
  }
  else
    str2[1] = '\0';

  return CompareStringA(GetThreadLocale(), dwFlags, str1, -1, str2, -1) - CSTR_EQUAL;
}

/*************************************************************************
 * SHLWAPI_ChrCmpA
 *
 * Internal helper function.
 */
static BOOL WINAPI SHLWAPI_ChrCmpA(WORD ch1, WORD ch2)
{
  return SHLWAPI_ChrCmpHelperA(ch1, ch2, 0);
}

/*************************************************************************
 * ChrCmpIA	(SHLWAPI.385)
 *
 * Compare two characters, ignoring case.
 *
 * PARAMS
 *  ch1 [I] First character to compare
 *  ch2 [I] Second character to compare
 *
 * RETURNS
 *  FALSE, if the characters are equal.
 *  Non-zero otherwise.
 */
BOOL WINAPI ChrCmpIA(WORD ch1, WORD ch2)
{
  TRACE("(%d,%d)\n", ch1, ch2);

  return SHLWAPI_ChrCmpHelperA(ch1, ch2, NORM_IGNORECASE);
}

/*************************************************************************
 * ChrCmpIW	[SHLWAPI.386]
 *
 * See ChrCmpIA.
 */
BOOL WINAPI ChrCmpIW(WCHAR ch1, WCHAR ch2)
{
  return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, &ch1, 1, &ch2, 1) - CSTR_EQUAL;
}

/*************************************************************************
 * StrChrA	[SHLWAPI.@]
 *
 * Find a given character in a string.
 *
 * PARAMS
 *  lpszStr [I] String to search in.
 *  ch      [I] Character to search for.
 *
 * RETURNS
 *  Success: A pointer to the first occurrence of ch in lpszStr, or NULL if
 *           not found.
 *  Failure: NULL, if any arguments are invalid.
 */
LPSTR WINAPI StrChrA(LPCSTR lpszStr, WORD ch)
{
  TRACE("(%s,%i)\n", debugstr_a(lpszStr), ch);

  if (lpszStr)
  {
    while (*lpszStr)
    {
      if (!SHLWAPI_ChrCmpA(*lpszStr, ch))
        return (LPSTR)lpszStr;
      lpszStr = CharNextA(lpszStr);
    }
  }
  return NULL;
}

/*************************************************************************
 * StrChrW	[SHLWAPI.@]
 *
 * See StrChrA.
 */
LPWSTR WINAPI StrChrW(LPCWSTR lpszStr, WCHAR ch)
{
  LPWSTR lpszRet = NULL;

  TRACE("(%s,%i)\n", debugstr_w(lpszStr), ch);

  if (lpszStr)
    lpszRet = strchrW(lpszStr, ch);
  return lpszRet;
}

/*************************************************************************
 * StrChrIA	[SHLWAPI.@]
 *
 * Find a given character in a string, ignoring case.
 *
 * PARAMS
 *  lpszStr [I] String to search in.
 *  ch      [I] Character to search for.
 *
 * RETURNS
 *  Success: A pointer to the first occurrence of ch in lpszStr, or NULL if
 *           not found.
 *  Failure: NULL, if any arguments are invalid.
 */
LPSTR WINAPI StrChrIA(LPCSTR lpszStr, WORD ch)
{
  TRACE("(%s,%i)\n", debugstr_a(lpszStr), ch);

  if (lpszStr)
  {
    while (*lpszStr)
    {
      if (!ChrCmpIA(*lpszStr, ch))
        return (LPSTR)lpszStr;
      lpszStr = CharNextA(lpszStr);
    }
  }
  return NULL;
}

/*************************************************************************
 * StrChrIW	[SHLWAPI.@]
 *
 * See StrChrA.
 */
LPWSTR WINAPI StrChrIW(LPCWSTR lpszStr, WCHAR ch)
{
  TRACE("(%s,%i)\n", debugstr_w(lpszStr), ch);

  if (lpszStr)
  {
    ch = toupperW(ch);
    while (*lpszStr)
    {
      if (toupperW(*lpszStr) == ch)
        return (LPWSTR)lpszStr;
      lpszStr++;
    }
    lpszStr = NULL;
  }
  return (LPWSTR)lpszStr;
}

/*************************************************************************
 * StrChrNW	[SHLWAPI.@]
 */
LPWSTR WINAPI StrChrNW(LPCWSTR lpszStr, WCHAR ch, UINT cchMax)
{
  TRACE("(%s(%i),%i)\n", debugstr_wn(lpszStr,cchMax), cchMax, ch);

  if (lpszStr)
  {
    while (*lpszStr && cchMax-- > 0)
    {
      if (*lpszStr == ch)
        return (LPWSTR)lpszStr;
      lpszStr++;
    }
  }
  return NULL;
}

/*************************************************************************
 * StrCmpIW	[SHLWAPI.@]
 *
 * Compare two strings, ignoring case.
 *
 * PARAMS
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszStr is less than, the same, or greater than lpszComp.
 */
int WINAPI StrCmpIW(LPCWSTR lpszStr, LPCWSTR lpszComp)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr),debugstr_w(lpszComp));
  return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, lpszStr, -1, lpszComp, -1) - CSTR_EQUAL;
}

/*************************************************************************
 * StrCmpNA	[SHLWAPI.@]
 *
 * Compare two strings, up to a maximum length.
 *
 * PARAMS
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *  iLen     [I] Number of chars to compare
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszStr is less than, the same, or greater than lpszComp.
 */
INT WINAPI StrCmpNA(LPCSTR lpszStr, LPCSTR lpszComp, INT iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);
  return CompareStringA(GetThreadLocale(), 0, lpszStr, iLen, lpszComp, iLen) - CSTR_EQUAL;
}

/*************************************************************************
 * StrCmpNW	[SHLWAPI.@]
 *
 * See StrCmpNA.
 */
INT WINAPI StrCmpNW(LPCWSTR lpszStr, LPCWSTR lpszComp, INT iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszComp), iLen);
  return CompareStringW(GetThreadLocale(), 0, lpszStr, iLen, lpszComp, iLen) - CSTR_EQUAL;
}

/*************************************************************************
 * StrCmpNIA	[SHLWAPI.@]
 *
 * Compare two strings, up to a maximum length, ignoring case.
 *
 * PARAMS
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *  iLen     [I] Number of chars to compare
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszStr is less than, the same, or greater than lpszComp.
 */
int WINAPI StrCmpNIA(LPCSTR lpszStr, LPCSTR lpszComp, int iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);
  return CompareStringA(GetThreadLocale(), NORM_IGNORECASE, lpszStr, iLen, lpszComp, iLen) - CSTR_EQUAL;
}

/*************************************************************************
 * StrCmpNIW	[SHLWAPI.@]
 *
 * See StrCmpNIA.
 */
INT WINAPI StrCmpNIW(LPCWSTR lpszStr, LPCWSTR lpszComp, int iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszComp), iLen);
  return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, lpszStr, iLen, lpszComp, iLen) - CSTR_EQUAL;
}

/*************************************************************************
 * StrCmpW	[SHLWAPI.@]
 *
 * Compare two strings.
 *
 * PARAMS
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszStr is less than, the same, or greater than lpszComp.
 */
int WINAPI StrCmpW(LPCWSTR lpszStr, LPCWSTR lpszComp)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszComp));
  return CompareStringW(GetThreadLocale(), 0, lpszStr, -1, lpszComp, -1) - CSTR_EQUAL;
}

/*************************************************************************
 * StrCatW	[SHLWAPI.@]
 *
 * Concatenate two strings.
 *
 * PARAMS
 *  lpszStr [O] Initial string
 *  lpszSrc [I] String to concatenate
 *
 * RETURNS
 *  lpszStr.
 */
LPWSTR WINAPI StrCatW(LPWSTR lpszStr, LPCWSTR lpszSrc)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSrc));

  if (lpszStr && lpszSrc)
    strcatW(lpszStr, lpszSrc);
  return lpszStr;
}

/*************************************************************************
 * StrCatChainW	[SHLWAPI.@]
 *
 * Concatenates two unicode strings.
 *
 * PARAMS
 *  lpszStr [O] Initial string
 *  cchMax  [I] Length of destination buffer
 *  ichAt   [I] Offset from the destination buffer to begin concatenation
 *  lpszCat [I] String to concatenate
 *
 * RETURNS
 *  The offset from the beginning of pszDst to the terminating NULL.
 */
DWORD WINAPI StrCatChainW(LPWSTR lpszStr, DWORD cchMax, DWORD ichAt, LPCWSTR lpszCat)
{
  TRACE("(%s,%u,%d,%s)\n", debugstr_w(lpszStr), cchMax, ichAt, debugstr_w(lpszCat));

  if (ichAt == -1)
    ichAt = strlenW(lpszStr);

  if (!cchMax)
    return ichAt;

  if (ichAt == cchMax)
    ichAt--;

  if (lpszCat && ichAt < cchMax)
  {
    lpszStr += ichAt;
    while (ichAt < cchMax - 1 && *lpszCat)
    {
      *lpszStr++ = *lpszCat++;
      ichAt++;
    }
    *lpszStr = 0;
  }

  return ichAt;
}

/*************************************************************************
 * StrCpyW	[SHLWAPI.@]
 *
 * Copy a string to another string.
 *
 * PARAMS
 *  lpszStr [O] Destination string
 *  lpszSrc [I] Source string
 *
 * RETURNS
 *  lpszStr.
 */
LPWSTR WINAPI StrCpyW(LPWSTR lpszStr, LPCWSTR lpszSrc)
{
  TRACE("(%p,%s)\n", lpszStr, debugstr_w(lpszSrc));

  if (lpszStr && lpszSrc)
    strcpyW(lpszStr, lpszSrc);
  return lpszStr;
}

/*************************************************************************
 * StrCpyNW	[SHLWAPI.@]
 *
 * Copy a string to another string, up to a maximum number of characters.
 *
 * PARAMS
 *  dst    [O] Destination string
 *  src    [I] Source string
 *  count  [I] Maximum number of chars to copy
 *
 * RETURNS
 *  dst.
 */
LPWSTR WINAPI StrCpyNW(LPWSTR dst, LPCWSTR src, int count)
{
  LPWSTR d = dst;
  LPCWSTR s = src;

  TRACE("(%p,%s,%i)\n", dst, debugstr_w(src), count);

  if (s)
  {
    while ((count > 1) && *s)
    {
      count--;
      *d++ = *s++;
    }
  }
  if (count) *d = 0;

  return dst;
}

/*************************************************************************
 * SHLWAPI_StrStrHelperA
 *
 * Internal implementation of StrStrA/StrStrIA
 */
static LPSTR SHLWAPI_StrStrHelperA(LPCSTR lpszStr, LPCSTR lpszSearch,
                                   INT (WINAPI *pStrCmpFn)(LPCSTR,LPCSTR,INT))
{
  size_t iLen;
  LPCSTR end;

  if (!lpszStr || !lpszSearch || !*lpszSearch)
    return NULL;

  iLen = strlen(lpszSearch);
  end = lpszStr + strlen(lpszStr);

  while (lpszStr + iLen <= end)
  {
    if (!pStrCmpFn(lpszStr, lpszSearch, iLen))
      return (LPSTR)lpszStr;
    lpszStr = CharNextA(lpszStr);
  }
  return NULL;
}

/*************************************************************************
 * StrStrA	[SHLWAPI.@]
 *
 * Find a substring within a string.
 *
 * PARAMS
 *  lpszStr    [I] String to search in
 *  lpszSearch [I] String to look for
 *
 * RETURNS
 *  The start of lpszSearch within lpszStr, or NULL if not found.
 */
LPSTR WINAPI StrStrA(LPCSTR lpszStr, LPCSTR lpszSearch)
{
  TRACE("(%s,%s)\n", debugstr_a(lpszStr), debugstr_a(lpszSearch));

  return SHLWAPI_StrStrHelperA(lpszStr, lpszSearch, StrCmpNA);
}

/*************************************************************************
 * StrStrW	[SHLWAPI.@]
 *
 * See StrStrA.
 */
LPWSTR WINAPI StrStrW(LPCWSTR lpszStr, LPCWSTR lpszSearch)
{
    TRACE("(%s, %s)\n", debugstr_w(lpszStr), debugstr_w(lpszSearch));

    if (!lpszStr || !lpszSearch || !*lpszSearch) return NULL;
    return strstrW( lpszStr, lpszSearch );
}

/*************************************************************************
 * StrRStrIA	[SHLWAPI.@]
 *
 * Find the last occurrence of a substring within a string.
 *
 * PARAMS
 *  lpszStr    [I] String to search in
 *  lpszEnd    [I] End of lpszStr
 *  lpszSearch [I] String to look for
 *
 * RETURNS
 *  The last occurrence lpszSearch within lpszStr, or NULL if not found.
 */
LPSTR WINAPI StrRStrIA(LPCSTR lpszStr, LPCSTR lpszEnd, LPCSTR lpszSearch)
{
  LPSTR lpszRet = NULL;
  WORD ch1, ch2;
  INT iLen;

  TRACE("(%s,%s)\n", debugstr_a(lpszStr), debugstr_a(lpszSearch));

  if (!lpszStr || !lpszSearch || !*lpszSearch)
    return NULL;

  if (IsDBCSLeadByte(*lpszSearch))
    ch1 = *lpszSearch << 8 | (UCHAR)lpszSearch[1];
  else
    ch1 = *lpszSearch;
  iLen = lstrlenA(lpszSearch);

  if (!lpszEnd)
    lpszEnd = lpszStr + lstrlenA(lpszStr);
  else /* reproduce the broken behaviour on Windows */
    lpszEnd += min(iLen - 1, lstrlenA(lpszEnd));

  while (lpszStr + iLen <= lpszEnd && *lpszStr)
  {
    ch2 = IsDBCSLeadByte(*lpszStr)? *lpszStr << 8 | (UCHAR)lpszStr[1] : *lpszStr;
    if (!ChrCmpIA(ch1, ch2))
    {
      if (!StrCmpNIA(lpszStr, lpszSearch, iLen))
        lpszRet = (LPSTR)lpszStr;
    }
    lpszStr = CharNextA(lpszStr);
  }
  return lpszRet;
}

/*************************************************************************
 * StrRStrIW	[SHLWAPI.@]
 *
 * See StrRStrIA.
 */
LPWSTR WINAPI StrRStrIW(LPCWSTR lpszStr, LPCWSTR lpszEnd, LPCWSTR lpszSearch)
{
  LPWSTR lpszRet = NULL;
  INT iLen;

  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSearch));

  if (!lpszStr || !lpszSearch || !*lpszSearch)
    return NULL;

  iLen = strlenW(lpszSearch);

  if (!lpszEnd)
    lpszEnd = lpszStr + strlenW(lpszStr);
  else /* reproduce the broken behaviour on Windows */
    lpszEnd += min(iLen - 1, lstrlenW(lpszEnd));

  while (lpszStr + iLen <= lpszEnd && *lpszStr)
  {
    if (!ChrCmpIW(*lpszSearch, *lpszStr))
    {
      if (!StrCmpNIW(lpszStr, lpszSearch, iLen))
        lpszRet = (LPWSTR)lpszStr;
    }
    lpszStr++;
  }
  return lpszRet;
}

/*************************************************************************
 * StrStrIA	[SHLWAPI.@]
 *
 * Find a substring within a string, ignoring case.
 *
 * PARAMS
 *  lpszStr    [I] String to search in
 *  lpszSearch [I] String to look for
 *
 * RETURNS
 *  The start of lpszSearch within lpszStr, or NULL if not found.
 */
LPSTR WINAPI StrStrIA(LPCSTR lpszStr, LPCSTR lpszSearch)
{
  TRACE("(%s,%s)\n", debugstr_a(lpszStr), debugstr_a(lpszSearch));

  return SHLWAPI_StrStrHelperA(lpszStr, lpszSearch, StrCmpNIA);
}

/*************************************************************************
 * StrStrIW	[SHLWAPI.@]
 *
 * See StrStrIA.
 */
LPWSTR WINAPI StrStrIW(LPCWSTR lpszStr, LPCWSTR lpszSearch)
{
  int iLen;
  LPCWSTR end;

  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSearch));

  if (!lpszStr || !lpszSearch || !*lpszSearch)
    return NULL;

  iLen = strlenW(lpszSearch);
  end = lpszStr + strlenW(lpszStr);

  while (lpszStr + iLen <= end)
  {
    if (!StrCmpNIW(lpszStr, lpszSearch, iLen))
      return (LPWSTR)lpszStr;
    lpszStr++;
  }
  return NULL;
}

/*************************************************************************
 * StrStrNW	[SHLWAPI.@]
 *
 * Find a substring within a string up to a given number of initial characters.
 *
 * PARAMS
 *  lpFirst    [I] String to search in
 *  lpSrch     [I] String to look for
 *  cchMax     [I] Maximum number of initial search characters
 *
 * RETURNS
 *  The start of lpFirst within lpSrch, or NULL if not found.
 */
LPWSTR WINAPI StrStrNW(LPCWSTR lpFirst, LPCWSTR lpSrch, UINT cchMax)
{
    UINT i;
    int len;

    TRACE("(%s, %s, %u)\n", debugstr_w(lpFirst), debugstr_w(lpSrch), cchMax);

    if (!lpFirst || !lpSrch || !*lpSrch || !cchMax)
        return NULL;

    len = strlenW(lpSrch);

    for (i = cchMax; *lpFirst && (i > 0); i--, lpFirst++)
    {
        if (!strncmpW(lpFirst, lpSrch, len))
            return (LPWSTR)lpFirst;
    }

    return NULL;
}

/*************************************************************************
 * StrStrNIW	[SHLWAPI.@]
 *
 * Find a substring within a string up to a given number of initial characters,
 * ignoring case.
 *
 * PARAMS
 *  lpFirst    [I] String to search in
 *  lpSrch     [I] String to look for
 *  cchMax     [I] Maximum number of initial search characters
 *
 * RETURNS
 *  The start of lpFirst within lpSrch, or NULL if not found.
 */
LPWSTR WINAPI StrStrNIW(LPCWSTR lpFirst, LPCWSTR lpSrch, UINT cchMax)
{
    UINT i;
    int len;

    TRACE("(%s, %s, %u)\n", debugstr_w(lpFirst), debugstr_w(lpSrch), cchMax);

    if (!lpFirst || !lpSrch || !*lpSrch || !cchMax)
        return NULL;

    len = strlenW(lpSrch);

    for (i = cchMax; *lpFirst && (i > 0); i--, lpFirst++)
    {
        if (!strncmpiW(lpFirst, lpSrch, len))
            return (LPWSTR)lpFirst;
    }

    return NULL;
}

/*************************************************************************
 * StrToIntA	[SHLWAPI.@]
 *
 * Read a signed integer from a string.
 *
 * PARAMS
 *  lpszStr [I] String to read integer from
 *
 * RETURNS
 *   The signed integer value represented by the string, or 0 if no integer is
 *   present.
 *
 * NOTES
 *  No leading space is allowed before the number, although a leading '-' is.
 */
int WINAPI StrToIntA(LPCSTR lpszStr)
{
  int iRet = 0;

  TRACE("(%s)\n", debugstr_a(lpszStr));

  if (!lpszStr)
  {
    WARN("Invalid lpszStr would crash under Win32!\n");
    return 0;
  }

  if (*lpszStr == '-' || isdigit(*lpszStr))
    StrToIntExA(lpszStr, 0, &iRet);
  return iRet;
}

/*************************************************************************
 * StrToIntW	[SHLWAPI.@]
 *
 * See StrToIntA.
 */
int WINAPI StrToIntW(LPCWSTR lpszStr)
{
  int iRet = 0;

  TRACE("(%s)\n", debugstr_w(lpszStr));

  if (!lpszStr)
  {
    WARN("Invalid lpszStr would crash under Win32!\n");
    return 0;
  }

  if (*lpszStr == '-' || isdigitW(*lpszStr))
    StrToIntExW(lpszStr, 0, &iRet);
  return iRet;
}

/*************************************************************************
 * StrToIntExA	[SHLWAPI.@]
 *
 * Read an integer from a string.
 *
 * PARAMS
 *  lpszStr [I] String to read integer from
 *  dwFlags [I] Flags controlling the conversion
 *  lpiRet  [O] Destination for read integer.
 *
 * RETURNS
 *  Success: TRUE. lpiRet contains the integer value represented by the string.
 *  Failure: FALSE, if the string is invalid, or no number is present.
 *
 * NOTES
 *  Leading whitespace, '-' and '+' are allowed before the number. If
 *  dwFlags includes STIF_SUPPORT_HEX, hexadecimal numbers are allowed, if
 *  preceded by '0x'. If this flag is not set, or there is no '0x' prefix,
 *  the string is treated as a decimal string. A leading '-' is ignored for
 *  hexadecimal numbers.
 */
BOOL WINAPI StrToIntExA(LPCSTR lpszStr, DWORD dwFlags, int *lpiRet)
{
  LONGLONG li;
  BOOL bRes;

  TRACE("(%s,%08X,%p)\n", debugstr_a(lpszStr), dwFlags, lpiRet);

  bRes = StrToInt64ExA(lpszStr, dwFlags, &li);
  if (bRes) *lpiRet = li;
  return bRes;
}

/*************************************************************************
 * StrToInt64ExA	[SHLWAPI.@]
 *
 * See StrToIntExA.
 */
BOOL WINAPI StrToInt64ExA(LPCSTR lpszStr, DWORD dwFlags, LONGLONG *lpiRet)
{
  BOOL bNegative = FALSE;
  LONGLONG iRet = 0;

  TRACE("(%s,%08X,%p)\n", debugstr_a(lpszStr), dwFlags, lpiRet);

  if (!lpszStr || !lpiRet)
  {
    WARN("Invalid parameter would crash under Win32!\n");
    return FALSE;
  }
  if (dwFlags > STIF_SUPPORT_HEX) WARN("Unknown flags %08x\n", dwFlags);

  /* Skip leading space, '+', '-' */
  while (isspace(*lpszStr))
    lpszStr = CharNextA(lpszStr);

  if (*lpszStr == '-')
  {
    bNegative = TRUE;
    lpszStr++;
  }
  else if (*lpszStr == '+')
    lpszStr++;

  if (dwFlags & STIF_SUPPORT_HEX &&
      *lpszStr == '0' && tolower(lpszStr[1]) == 'x')
  {
    /* Read hex number */
    lpszStr += 2;

    if (!isxdigit(*lpszStr))
      return FALSE;

    while (isxdigit(*lpszStr))
    {
      iRet = iRet * 16;
      if (isdigit(*lpszStr))
        iRet += (*lpszStr - '0');
      else
        iRet += 10 + (tolower(*lpszStr) - 'a');
      lpszStr++;
    }
    *lpiRet = iRet;
    return TRUE;
  }

  /* Read decimal number */
  if (!isdigit(*lpszStr))
    return FALSE;

  while (isdigit(*lpszStr))
  {
    iRet = iRet * 10;
    iRet += (*lpszStr - '0');
    lpszStr++;
  }
  *lpiRet = bNegative ? -iRet : iRet;
  return TRUE;
}

/*************************************************************************
 * StrToIntExW	[SHLWAPI.@]
 *
 * See StrToIntExA.
 */
BOOL WINAPI StrToIntExW(LPCWSTR lpszStr, DWORD dwFlags, int *lpiRet)
{
  LONGLONG li;
  BOOL bRes;

  TRACE("(%s,%08X,%p)\n", debugstr_w(lpszStr), dwFlags, lpiRet);

  bRes = StrToInt64ExW(lpszStr, dwFlags, &li);
  if (bRes) *lpiRet = li;
  return bRes;
}

/*************************************************************************
 * StrToInt64ExW	[SHLWAPI.@]
 *
 * See StrToIntExA.
 */
BOOL WINAPI StrToInt64ExW(LPCWSTR lpszStr, DWORD dwFlags, LONGLONG *lpiRet)
{
  BOOL bNegative = FALSE;
  LONGLONG iRet = 0;

  TRACE("(%s,%08X,%p)\n", debugstr_w(lpszStr), dwFlags, lpiRet);

  if (!lpszStr || !lpiRet)
  {
    WARN("Invalid parameter would crash under Win32!\n");
    return FALSE;
  }
  if (dwFlags > STIF_SUPPORT_HEX) WARN("Unknown flags %08x\n", dwFlags);

  /* Skip leading space, '+', '-' */
  while (isspaceW(*lpszStr)) lpszStr++;

  if (*lpszStr == '-')
  {
    bNegative = TRUE;
    lpszStr++;
  }
  else if (*lpszStr == '+')
    lpszStr++;

  if (dwFlags & STIF_SUPPORT_HEX &&
      *lpszStr == '0' && tolowerW(lpszStr[1]) == 'x')
  {
    /* Read hex number */
    lpszStr += 2;

    if (!isxdigitW(*lpszStr))
      return FALSE;

    while (isxdigitW(*lpszStr))
    {
      iRet = iRet * 16;
      if (isdigitW(*lpszStr))
        iRet += (*lpszStr - '0');
      else
        iRet += 10 + (tolowerW(*lpszStr) - 'a');
      lpszStr++;
    }
    *lpiRet = iRet;
    return TRUE;
  }

  /* Read decimal number */
  if (!isdigitW(*lpszStr))
    return FALSE;

  while (isdigitW(*lpszStr))
  {
    iRet = iRet * 10;
    iRet += (*lpszStr - '0');
    lpszStr++;
  }
  *lpiRet = bNegative ? -iRet : iRet;
  return TRUE;
}

/*************************************************************************
 * StrDupA	[SHLWAPI.@]
 *
 * Duplicate a string.
 *
 * PARAMS
 *  lpszStr [I] String to duplicate.
 *
 * RETURNS
 *  Success: A pointer to a new string containing the contents of lpszStr
 *  Failure: NULL, if memory cannot be allocated
 *
 * NOTES
 *  The string memory is allocated with LocalAlloc(), and so should be released
 *  by calling LocalFree().
 */
LPSTR WINAPI StrDupA(LPCSTR lpszStr)
{
  int iLen;
  LPSTR lpszRet;

  TRACE("(%s)\n",debugstr_a(lpszStr));

#ifdef __REACTOS__
  if (!lpszStr)
    return NULL;
#endif
  iLen = lpszStr ? strlen(lpszStr) + 1 : 1;
  lpszRet = LocalAlloc(LMEM_FIXED, iLen);

  if (lpszRet)
  {
    if (lpszStr)
      memcpy(lpszRet, lpszStr, iLen);
    else
      *lpszRet = '\0';
  }
  return lpszRet;
}

/*************************************************************************
 * StrDupW	[SHLWAPI.@]
 *
 * See StrDupA.
 */
LPWSTR WINAPI StrDupW(LPCWSTR lpszStr)
{
  int iLen;
  LPWSTR lpszRet;

  TRACE("(%s)\n",debugstr_w(lpszStr));

#ifdef __REACTOS__
  if (!lpszStr)
    return NULL;
#endif
  iLen = (lpszStr ? strlenW(lpszStr) + 1 : 1) * sizeof(WCHAR);
  lpszRet = LocalAlloc(LMEM_FIXED, iLen);

  if (lpszRet)
  {
    if (lpszStr)
      memcpy(lpszRet, lpszStr, iLen);
    else
      *lpszRet = '\0';
  }
  return lpszRet;
}

/*************************************************************************
 * SHLWAPI_StrSpnHelperA
 *
 * Internal implementation of StrSpnA/StrCSpnA/StrCSpnIA
 */
static int SHLWAPI_StrSpnHelperA(LPCSTR lpszStr, LPCSTR lpszMatch,
                                 LPSTR (WINAPI *pStrChrFn)(LPCSTR,WORD),
                                 BOOL bInvert)
{
  LPCSTR lpszRead = lpszStr;
  if (lpszStr && *lpszStr && lpszMatch)
  {
    while (*lpszRead)
    {
      LPCSTR lpszTest = pStrChrFn(lpszMatch, *lpszRead);

      if (!bInvert && !lpszTest)
        break;
      if (bInvert && lpszTest)
        break;
      lpszRead = CharNextA(lpszRead);
    };
  }
  return lpszRead - lpszStr;
}

/*************************************************************************
 * StrSpnA	[SHLWAPI.@]
 *
 * Find the length of the start of a string that contains only certain
 * characters.
 *
 * PARAMS
 *  lpszStr   [I] String to search
 *  lpszMatch [I] Characters that can be in the substring
 *
 * RETURNS
 *  The length of the part of lpszStr containing only chars from lpszMatch,
 *  or 0 if any parameter is invalid.
 */
int WINAPI StrSpnA(LPCSTR lpszStr, LPCSTR lpszMatch)
{
  TRACE("(%s,%s)\n",debugstr_a(lpszStr), debugstr_a(lpszMatch));

  return SHLWAPI_StrSpnHelperA(lpszStr, lpszMatch, StrChrA, FALSE);
}

/*************************************************************************
 * StrSpnW	[SHLWAPI.@]
 *
 * See StrSpnA.
 */
int WINAPI StrSpnW(LPCWSTR lpszStr, LPCWSTR lpszMatch)
{
    if (!lpszStr || !lpszMatch) return 0;
    return strspnW( lpszStr, lpszMatch );
}

/*************************************************************************
 * StrCSpnA	[SHLWAPI.@]
 *
 * Find the length of the start of a string that does not contain certain
 * characters.
 *
 * PARAMS
 *  lpszStr   [I] String to search
 *  lpszMatch [I] Characters that cannot be in the substring
 *
 * RETURNS
 *  The length of the part of lpszStr containing only chars not in lpszMatch,
 *  or 0 if any parameter is invalid.
 */
int WINAPI StrCSpnA(LPCSTR lpszStr, LPCSTR lpszMatch)
{
  TRACE("(%s,%s)\n",debugstr_a(lpszStr), debugstr_a(lpszMatch));

  return SHLWAPI_StrSpnHelperA(lpszStr, lpszMatch, StrChrA, TRUE);
}

/*************************************************************************
 * StrCSpnW	[SHLWAPI.@]
 *
 * See StrCSpnA.
 */
int WINAPI StrCSpnW(LPCWSTR lpszStr, LPCWSTR lpszMatch)
{
    if (!lpszStr || !lpszMatch) return 0;
    return strcspnW( lpszStr, lpszMatch );
}

/*************************************************************************
 * StrCSpnIA	[SHLWAPI.@]
 *
 * Find the length of the start of a string that does not contain certain
 * characters, ignoring case.
 *
 * PARAMS
 *  lpszStr   [I] String to search
 *  lpszMatch [I] Characters that cannot be in the substring
 *
 * RETURNS
 *  The length of the part of lpszStr containing only chars not in lpszMatch,
 *  or 0 if any parameter is invalid.
 */
int WINAPI StrCSpnIA(LPCSTR lpszStr, LPCSTR lpszMatch)
{
  TRACE("(%s,%s)\n",debugstr_a(lpszStr), debugstr_a(lpszMatch));

  return SHLWAPI_StrSpnHelperA(lpszStr, lpszMatch, StrChrIA, TRUE);
}

/*************************************************************************
 * StrCSpnIW	[SHLWAPI.@]
 *
 * See StrCSpnIA.
 */
int WINAPI StrCSpnIW(LPCWSTR lpszStr, LPCWSTR lpszMatch)
{
  LPCWSTR lpszRead = lpszStr;

  TRACE("(%s,%s)\n",debugstr_w(lpszStr), debugstr_w(lpszMatch));

  if (lpszStr && *lpszStr && lpszMatch)
  {
    while (*lpszRead)
    {
      if (StrChrIW(lpszMatch, *lpszRead)) break;
      lpszRead++;
    }
  }
  return lpszRead - lpszStr;
}

/*************************************************************************
 * StrPBrkA	[SHLWAPI.@]
 *
 * Search a string for any of a group of characters.
 *
 * PARAMS
 *  lpszStr   [I] String to search
 *  lpszMatch [I] Characters to match
 *
 * RETURNS
 *  A pointer to the first matching character in lpszStr, or NULL if no
 *  match was found.
 */
LPSTR WINAPI StrPBrkA(LPCSTR lpszStr, LPCSTR lpszMatch)
{
  TRACE("(%s,%s)\n",debugstr_a(lpszStr), debugstr_a(lpszMatch));

  if (lpszStr && lpszMatch && *lpszMatch)
  {
    while (*lpszStr)
    {
      if (StrChrA(lpszMatch, *lpszStr))
        return (LPSTR)lpszStr;
      lpszStr = CharNextA(lpszStr);
    }
  }
  return NULL;
}

/*************************************************************************
 * StrPBrkW	[SHLWAPI.@]
 *
 * See StrPBrkA.
 */
LPWSTR WINAPI StrPBrkW(LPCWSTR lpszStr, LPCWSTR lpszMatch)
{
    if (!lpszStr || !lpszMatch) return NULL;
    return strpbrkW( lpszStr, lpszMatch );
}

/*************************************************************************
 * SHLWAPI_StrRChrHelperA
 *
 * Internal implementation of StrRChrA/StrRChrIA.
 */
static LPSTR SHLWAPI_StrRChrHelperA(LPCSTR lpszStr,
                                    LPCSTR lpszEnd, WORD ch,
                                    BOOL (WINAPI *pChrCmpFn)(WORD,WORD))
{
  LPCSTR lpszRet = NULL;

  if (lpszStr)
  {
    WORD ch2;

    if (!lpszEnd)
      lpszEnd = lpszStr + lstrlenA(lpszStr);

    while (*lpszStr && lpszStr <= lpszEnd)
    {
      ch2 = IsDBCSLeadByte(*lpszStr)? *lpszStr << 8 | lpszStr[1] : *lpszStr;

      if (!pChrCmpFn(ch, ch2))
        lpszRet = lpszStr;
      lpszStr = CharNextA(lpszStr);
    }
  }
  return (LPSTR)lpszRet;
}

/**************************************************************************
 * StrRChrA	[SHLWAPI.@]
 *
 * Find the last occurrence of a character in string.
 *
 * PARAMS
 *  lpszStr [I] String to search in
 *  lpszEnd [I] Place to end search, or NULL to search until the end of lpszStr
 *  ch      [I] Character to search for.
 *
 * RETURNS
 *  Success: A pointer to the last occurrence of ch in lpszStr before lpszEnd,
 *           or NULL if not found.
 *  Failure: NULL, if any arguments are invalid.
 */
LPSTR WINAPI StrRChrA(LPCSTR lpszStr, LPCSTR lpszEnd, WORD ch)
{
  TRACE("(%s,%s,%x)\n", debugstr_a(lpszStr), debugstr_a(lpszEnd), ch);

  return SHLWAPI_StrRChrHelperA(lpszStr, lpszEnd, ch, SHLWAPI_ChrCmpA);
}

/**************************************************************************
 * StrRChrW	[SHLWAPI.@]
 *
 * See StrRChrA.
 */
LPWSTR WINAPI StrRChrW(LPCWSTR str, LPCWSTR end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + strlenW(str);
    while (str < end)
    {
        if (*str == ch) ret = (WCHAR *)str;
        str++;
    }
    return ret;
}

/**************************************************************************
 * StrRChrIA	[SHLWAPI.@]
 *
 * Find the last occurrence of a character in string, ignoring case.
 *
 * PARAMS
 *  lpszStr [I] String to search in
 *  lpszEnd [I] Place to end search, or NULL to search until the end of lpszStr
 *  ch      [I] Character to search for.
 *
 * RETURNS
 *  Success: A pointer to the last occurrence of ch in lpszStr before lpszEnd,
 *           or NULL if not found.
 *  Failure: NULL, if any arguments are invalid.
 */
LPSTR WINAPI StrRChrIA(LPCSTR lpszStr, LPCSTR lpszEnd, WORD ch)
{
  TRACE("(%s,%s,%x)\n", debugstr_a(lpszStr), debugstr_a(lpszEnd), ch);

  return SHLWAPI_StrRChrHelperA(lpszStr, lpszEnd, ch, ChrCmpIA);
}

/**************************************************************************
 * StrRChrIW	[SHLWAPI.@]
 *
 * See StrRChrIA.
 */
LPWSTR WINAPI StrRChrIW(LPCWSTR str, LPCWSTR end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + strlenW(str);
    while (str < end)
    {
        if (!ChrCmpIW(*str, ch)) ret = (WCHAR *)str;
        str++;
    }
    return ret;
}

/*************************************************************************
 * StrCatBuffA	[SHLWAPI.@]
 *
 * Concatenate two strings together.
 *
 * PARAMS
 *  lpszStr [O] String to concatenate to
 *  lpszCat [I] String to add to lpszCat
 *  cchMax  [I] Maximum number of characters for the whole string
 *
 * RETURNS
 *  lpszStr.
 *
 * NOTES
 *  cchMax determines the number of characters in the final length of the
 *  string, not the number appended to lpszStr from lpszCat.
 */
LPSTR WINAPI StrCatBuffA(LPSTR lpszStr, LPCSTR lpszCat, INT cchMax)
{
  INT iLen;

  TRACE("(%p,%s,%d)\n", lpszStr, debugstr_a(lpszCat), cchMax);

  if (!lpszStr)
  {
    WARN("Invalid lpszStr would crash under Win32!\n");
    return NULL;
  }

  iLen = strlen(lpszStr);
  cchMax -= iLen;

  if (cchMax > 0)
    StrCpyNA(lpszStr + iLen, lpszCat, cchMax);
  return lpszStr;
}

/*************************************************************************
 * StrCatBuffW	[SHLWAPI.@]
 *
 * See StrCatBuffA.
 */
LPWSTR WINAPI StrCatBuffW(LPWSTR lpszStr, LPCWSTR lpszCat, INT cchMax)
{
  INT iLen;

  TRACE("(%p,%s,%d)\n", lpszStr, debugstr_w(lpszCat), cchMax);

  if (!lpszStr)
  {
    WARN("Invalid lpszStr would crash under Win32!\n");
    return NULL;
  }

  iLen = strlenW(lpszStr);
  cchMax -= iLen;

  if (cchMax > 0)
    StrCpyNW(lpszStr + iLen, lpszCat, cchMax);
  return lpszStr;
}

/*************************************************************************
 * StrRetToBufA					[SHLWAPI.@]
 *
 * Convert a STRRET to a normal string.
 *
 * PARAMS
 *  lpStrRet [O] STRRET to convert
 *  pIdl     [I] ITEMIDLIST for lpStrRet->uType == STRRET_OFFSET
 *  lpszDest [O] Destination for normal string
 *  dwLen    [I] Length of lpszDest
 *
 * RETURNS
 *  Success: S_OK. lpszDest contains up to dwLen characters of the string.
 *           If lpStrRet is of type STRRET_WSTR, its memory is freed with
 *           CoTaskMemFree() and its type set to STRRET_CSTRA.
 *  Failure: E_FAIL, if any parameters are invalid.
 */
HRESULT WINAPI StrRetToBufA (LPSTRRET src, const ITEMIDLIST *pidl, LPSTR dest, UINT len)
{
	/* NOTE:
	 *  This routine is identical to that in dlls/shell32/shellstring.c.
	 *  It was duplicated because not every version of Shlwapi.dll exports
	 *  StrRetToBufA. If you change one routine, change them both.
	 */
        TRACE("dest=%p len=0x%x strret=%p pidl=%p\n", dest, len, src, pidl);

	if (!src)
	{
	  WARN("Invalid lpStrRet would crash under Win32!\n");
	  if (dest)
	    *dest = '\0';
	  return E_FAIL;
	}

	if (!dest || !len)
	  return E_FAIL;

	*dest = '\0';

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    WideCharToMultiByte(CP_ACP, 0, src->u.pOleStr, -1, dest, len, NULL, NULL);
	    CoTaskMemFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTR:
            lstrcpynA(dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSET:
	    lstrcpynA((LPSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	    break;

	  default:
	    FIXME("unknown type!\n");
	    return E_NOTIMPL;
	}
	return S_OK;
}

/*************************************************************************
 * StrRetToBufW	[SHLWAPI.@]
 *
 * See StrRetToBufA.
 */
HRESULT WINAPI StrRetToBufW (LPSTRRET src, const ITEMIDLIST *pidl, LPWSTR dest, UINT len)
{
    TRACE("dest=%p len=0x%x strret=%p pidl=%p\n", dest, len, src, pidl);

    if (!dest || !len)
        return E_FAIL;

    if (!src)
    {
        WARN("Invalid lpStrRet would crash under Win32!\n");
        if (dest)
            *dest = '\0';
        return E_FAIL;
    }

    *dest = '\0';

    switch (src->uType) {
    case STRRET_WSTR: {
        size_t dst_len;
        if (!src->u.pOleStr)
            return E_FAIL;
        dst_len = strlenW(src->u.pOleStr);
        memcpy(dest, src->u.pOleStr, min(dst_len, len-1) * sizeof(WCHAR));
        dest[min(dst_len, len-1)] = 0;
        CoTaskMemFree(src->u.pOleStr);
        if (len <= dst_len)
        {
            dest[0] = 0;
            return E_NOT_SUFFICIENT_BUFFER;
        }
        break;
    }

    case STRRET_CSTR:
        if (!MultiByteToWideChar( CP_ACP, 0, src->u.cStr, -1, dest, len ))
            dest[len-1] = 0;
        break;

    case STRRET_OFFSET:
        if (pidl)
	{
            if (!MultiByteToWideChar( CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->u.uOffset, -1,
                                      dest, len ))
                dest[len-1] = 0;
        }
        break;

    default:
        FIXME("unknown type!\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

/*************************************************************************
 * StrRetToStrA					[SHLWAPI.@]
 *
 * Converts a STRRET to a normal string.
 *
 * PARAMS
 *  lpStrRet [O] STRRET to convert
 *  pidl     [I] ITEMIDLIST for lpStrRet->uType == STRRET_OFFSET
 *  ppszName [O] Destination for converted string
 *
 * RETURNS
 *  Success: S_OK. ppszName contains the new string, allocated with CoTaskMemAlloc().
 *  Failure: E_FAIL, if any parameters are invalid.
 */
HRESULT WINAPI StrRetToStrA(LPSTRRET lpStrRet, const ITEMIDLIST *pidl, LPSTR *ppszName)
{
  HRESULT hRet = E_FAIL;

  switch (lpStrRet->uType)
  {
  case STRRET_WSTR:
    hRet = _SHStrDupAW(lpStrRet->u.pOleStr, ppszName);
    CoTaskMemFree(lpStrRet->u.pOleStr);
    break;

  case STRRET_CSTR:
    hRet = _SHStrDupAA(lpStrRet->u.cStr, ppszName);
    break;

  case STRRET_OFFSET:
    hRet = _SHStrDupAA(((LPCSTR)&pidl->mkid) + lpStrRet->u.uOffset, ppszName);
    break;

  default:
    *ppszName = NULL;
  }

  return hRet;
}

/*************************************************************************
 * StrRetToStrW					[SHLWAPI.@]
 *
 * See StrRetToStrA.
 */
HRESULT WINAPI StrRetToStrW(LPSTRRET lpStrRet, const ITEMIDLIST *pidl, LPWSTR *ppszName)
{
  HRESULT hRet = E_FAIL;

  switch (lpStrRet->uType)
  {
  case STRRET_WSTR:
#ifdef __REACTOS__
    hRet = lpStrRet->u.pOleStr ? S_OK : E_FAIL;
    *ppszName = lpStrRet->u.pOleStr;
    lpStrRet->u.pOleStr = NULL; /* Windows does this, presumably in case someone calls SHFree */
#else
    hRet = SHStrDupW(lpStrRet->u.pOleStr, ppszName);
    CoTaskMemFree(lpStrRet->u.pOleStr);
#endif
    break;

  case STRRET_CSTR:
    hRet = SHStrDupA(lpStrRet->u.cStr, ppszName);
    break;

  case STRRET_OFFSET:
    hRet = SHStrDupA(((LPCSTR)&pidl->mkid) + lpStrRet->u.uOffset, ppszName);
    break;

  default:
    *ppszName = NULL;
  }

  return hRet;
}

/* Create an ASCII string copy using SysAllocString() */
static HRESULT _SHStrDupAToBSTR(LPCSTR src, BSTR *pBstrOut)
{
    *pBstrOut = NULL;

    if (src)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
        WCHAR* szTemp = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

        if (szTemp)
        {
            MultiByteToWideChar(CP_ACP, 0, src, -1, szTemp, len);
            *pBstrOut = SysAllocString(szTemp);
            HeapFree(GetProcessHeap(), 0, szTemp);

            if (*pBstrOut)
                return S_OK;
        }
    }
    return E_OUTOFMEMORY;
}

/*************************************************************************
 * StrRetToBSTR	[SHLWAPI.@]
 *
 * Converts a STRRET to a BSTR.
 *
 * PARAMS
 *  lpStrRet [O] STRRET to convert
 *  pidl     [I] ITEMIDLIST for lpStrRet->uType = STRRET_OFFSET
 *  pBstrOut [O] Destination for converted BSTR
 *
 * RETURNS
 *  Success: S_OK. pBstrOut contains the new string.
 *  Failure: E_FAIL, if any parameters are invalid.
 */
HRESULT WINAPI StrRetToBSTR(STRRET *lpStrRet, LPCITEMIDLIST pidl, BSTR* pBstrOut)
{
  HRESULT hRet = E_FAIL;

  switch (lpStrRet->uType)
  {
  case STRRET_WSTR:
    *pBstrOut = SysAllocString(lpStrRet->u.pOleStr);
    if (*pBstrOut)
      hRet = S_OK;
    CoTaskMemFree(lpStrRet->u.pOleStr);
    break;

  case STRRET_CSTR:
    hRet = _SHStrDupAToBSTR(lpStrRet->u.cStr, pBstrOut);
    break;

  case STRRET_OFFSET:
    hRet = _SHStrDupAToBSTR(((LPCSTR)&pidl->mkid) + lpStrRet->u.uOffset, pBstrOut);
    break;

  default:
    *pBstrOut = NULL;
  }

  return hRet;
}

/*************************************************************************
 * StrFormatKBSizeA	[SHLWAPI.@]
 *
 * Create a formatted string containing a byte count in Kilobytes.
 *
 * PARAMS
 *  llBytes  [I] Byte size to format
 *  lpszDest [I] Destination for formatted string
 *  cchMax   [I] Size of lpszDest
 *
 * RETURNS
 *  lpszDest.
 */
LPSTR WINAPI StrFormatKBSizeA(LONGLONG llBytes, LPSTR lpszDest, UINT cchMax)
{
  WCHAR wszBuf[256];
  
  if (!StrFormatKBSizeW(llBytes, wszBuf, 256))
    return NULL;
  if (!WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, lpszDest, cchMax, NULL, NULL))
    return NULL;
  return lpszDest;
}

/*************************************************************************
 * StrFormatKBSizeW	[SHLWAPI.@]
 *
 * See StrFormatKBSizeA.
 */
LPWSTR WINAPI StrFormatKBSizeW(LONGLONG llBytes, LPWSTR lpszDest, UINT cchMax)
{
  static const WCHAR kb[] = {' ','K','B',0};
  LONGLONG llKB = (llBytes + 1023) >> 10;
  int len;

  TRACE("(0x%s,%p,%d)\n", wine_dbgstr_longlong(llBytes), lpszDest, cchMax);

  if (!FormatInt(llKB, lpszDest, cchMax))
    return NULL;

  len = lstrlenW(lpszDest);
  if (cchMax - len < 4)
      return NULL;
  lstrcatW(lpszDest, kb);
  return lpszDest;
}

/*************************************************************************
 * StrNCatA	[SHLWAPI.@]
 *
 * Concatenate two strings together.
 *
 * PARAMS
 *  lpszStr [O] String to concatenate to
 *  lpszCat [I] String to add to lpszCat
 *  cchMax  [I] Maximum number of characters to concatenate
 *
 * RETURNS
 *  lpszStr.
 *
 * NOTES
 *  cchMax determines the number of characters that are appended to lpszStr,
 *  not the total length of the string.
 */
LPSTR WINAPI StrNCatA(LPSTR lpszStr, LPCSTR lpszCat, INT cchMax)
{
  LPSTR lpszRet = lpszStr;

  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszCat), cchMax);

  if (!lpszStr)
  {
    WARN("Invalid lpszStr would crash under Win32!\n");
    return NULL;
  }

  StrCpyNA(lpszStr + strlen(lpszStr), lpszCat, cchMax);
  return lpszRet;
}

/*************************************************************************
 * StrNCatW	[SHLWAPI.@]
 *
 * See StrNCatA.
 */
LPWSTR WINAPI StrNCatW(LPWSTR lpszStr, LPCWSTR lpszCat, INT cchMax)
{
  LPWSTR lpszRet = lpszStr;

  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszCat), cchMax);

  if (!lpszStr)
  {
    WARN("Invalid lpszStr would crash under Win32\n");
    return NULL;
  }

  StrCpyNW(lpszStr + strlenW(lpszStr), lpszCat, cchMax);
  return lpszRet;
}

/*************************************************************************
 * StrTrimA	[SHLWAPI.@]
 *
 * Remove characters from the start and end of a string.
 *
 * PARAMS
 *  lpszStr  [O] String to remove characters from
 *  lpszTrim [I] Characters to remove from lpszStr
 *
 * RETURNS
 *  TRUE  If lpszStr was valid and modified
 *  FALSE Otherwise
 */
BOOL WINAPI StrTrimA(LPSTR lpszStr, LPCSTR lpszTrim)
{
  DWORD dwLen;
  LPSTR lpszRead = lpszStr;
  BOOL bRet = FALSE;

  TRACE("(%s,%s)\n", debugstr_a(lpszStr), debugstr_a(lpszTrim));

  if (lpszRead && *lpszRead)
  {
    while (*lpszRead && StrChrA(lpszTrim, *lpszRead))
      lpszRead = CharNextA(lpszRead); /* Skip leading matches */

    dwLen = strlen(lpszRead);

    if (lpszRead != lpszStr)
    {
      memmove(lpszStr, lpszRead, dwLen + 1);
      bRet = TRUE;
    }
    if (dwLen > 0)
    {
      lpszRead = lpszStr + dwLen;
      while (StrChrA(lpszTrim, lpszRead[-1]))
        lpszRead = CharPrevA(lpszStr, lpszRead); /* Skip trailing matches */

      if (lpszRead != lpszStr + dwLen)
      {
        *lpszRead = '\0';
        bRet = TRUE;
      }
    }
  }
  return bRet;
}

/*************************************************************************
 * StrTrimW	[SHLWAPI.@]
 *
 * See StrTrimA.
 */
BOOL WINAPI StrTrimW(LPWSTR lpszStr, LPCWSTR lpszTrim)
{
  DWORD dwLen;
  LPWSTR lpszRead = lpszStr;
  BOOL bRet = FALSE;

  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszTrim));

  if (lpszRead && *lpszRead)
  {
    while (*lpszRead && StrChrW(lpszTrim, *lpszRead)) lpszRead++;

    dwLen = strlenW(lpszRead);

    if (lpszRead != lpszStr)
    {
      memmove(lpszStr, lpszRead, (dwLen + 1) * sizeof(WCHAR));
      bRet = TRUE;
    }
    if (dwLen > 0)
    {
      lpszRead = lpszStr + dwLen;
      while (StrChrW(lpszTrim, lpszRead[-1]))
        lpszRead--; /* Skip trailing matches */

      if (lpszRead != lpszStr + dwLen)
      {
        *lpszRead = '\0';
        bRet = TRUE;
      }
    }
  }
  return bRet;
}

/*************************************************************************
 *      _SHStrDupAA	[INTERNAL]
 *
 * Duplicates a ASCII string to ASCII. The destination buffer is allocated.
 */
static HRESULT _SHStrDupAA(LPCSTR src, LPSTR * dest)
{
	HRESULT hr;
	int len = 0;

	if (src) {
	    len = lstrlenA(src) + 1;
	    *dest = CoTaskMemAlloc(len);
	} else {
	    *dest = NULL;
	}

	if (*dest) {
	    lstrcpynA(*dest,src, len);
	    hr = S_OK;
	} else {
	    hr = E_OUTOFMEMORY;
	}

	TRACE("%s->(%p)\n", debugstr_a(src), *dest);
	return hr;
}

/*************************************************************************
 * SHStrDupA	[SHLWAPI.@]
 *
 * Return a Unicode copy of a string, in memory allocated by CoTaskMemAlloc().
 *
 * PARAMS
 *  lpszStr   [I] String to copy
 *  lppszDest [O] Destination for the new string copy
 *
 * RETURNS
 *  Success: S_OK. lppszDest contains the new string in Unicode format.
 *  Failure: E_OUTOFMEMORY, If any arguments are invalid or memory allocation
 *           fails.
 */
HRESULT WINAPI SHStrDupA(LPCSTR lpszStr, LPWSTR * lppszDest)
{
  HRESULT hRet;
  int len = 0;

  if (lpszStr)
  {
    len = MultiByteToWideChar(CP_ACP, 0, lpszStr, -1, NULL, 0) * sizeof(WCHAR);
    *lppszDest = CoTaskMemAlloc(len);
  }
  else
    *lppszDest = NULL;

  if (*lppszDest)
  {
    MultiByteToWideChar(CP_ACP, 0, lpszStr, -1, *lppszDest, len/sizeof(WCHAR));
    hRet = S_OK;
  }
  else
    hRet = E_OUTOFMEMORY;

  TRACE("%s->(%p)\n", debugstr_a(lpszStr), *lppszDest);
  return hRet;
}

/*************************************************************************
 *      _SHStrDupAW	[INTERNAL]
 *
 * Duplicates a UNICODE to a ASCII string. The destination buffer is allocated.
 */
static HRESULT _SHStrDupAW(LPCWSTR src, LPSTR * dest)
{
	HRESULT hr;
	int len = 0;

	if (src) {
	    len = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, NULL);
	    *dest = CoTaskMemAlloc(len);
	} else {
	    *dest = NULL;
	}

	if (*dest) {
	    WideCharToMultiByte(CP_ACP, 0, src, -1, *dest, len, NULL, NULL);
	    hr = S_OK;
	} else {
	    hr = E_OUTOFMEMORY;
	}

	TRACE("%s->(%p)\n", debugstr_w(src), *dest);
	return hr;
}

/*************************************************************************
 * SHStrDupW	[SHLWAPI.@]
 *
 * See SHStrDupA.
 */
HRESULT WINAPI SHStrDupW(LPCWSTR src, LPWSTR * dest)
{
	HRESULT hr;
	int len = 0;

	if (src) {
	    len = (lstrlenW(src) + 1) * sizeof(WCHAR);
	    *dest = CoTaskMemAlloc(len);
	} else {
	    *dest = NULL;
	}

	if (*dest) {
	    memcpy(*dest, src, len);
	    hr = S_OK;
	} else {
	    hr = E_OUTOFMEMORY;
	}

	TRACE("%s->(%p)\n", debugstr_w(src), *dest);
	return hr;
}

/*************************************************************************
 * SHLWAPI_WriteReverseNum
 *
 * Internal helper for SHLWAPI_WriteTimeClass.
 */
static inline LPWSTR SHLWAPI_WriteReverseNum(LPWSTR lpszOut, DWORD dwNum)
{
  *lpszOut-- = '\0';

  /* Write a decimal number to a string, backwards */
  do
  {
    DWORD dwNextDigit = dwNum % 10;
    *lpszOut-- = '0' + dwNextDigit;
    dwNum = (dwNum - dwNextDigit) / 10;
  } while (dwNum > 0);

  return lpszOut;
}

/*************************************************************************
 * SHLWAPI_FormatSignificant
 *
 * Internal helper for SHLWAPI_WriteTimeClass.
 */
static inline int SHLWAPI_FormatSignificant(LPWSTR lpszNum, int dwDigits)
{
  /* Zero non significant digits, return remaining significant digits */
  while (*lpszNum)
  {
    lpszNum++;
    if (--dwDigits == 0)
    {
      while (*lpszNum)
        *lpszNum++ = '0';
      return 0;
    }
  }
  return dwDigits;
}

/*************************************************************************
 * SHLWAPI_WriteTimeClass
 *
 * Internal helper for StrFromTimeIntervalW.
 */
static int SHLWAPI_WriteTimeClass(LPWSTR lpszOut, DWORD dwValue,
                                  UINT uClassStringId, int iDigits)
{
  WCHAR szBuff[64], *szOut = szBuff + 32;

  szOut = SHLWAPI_WriteReverseNum(szOut, dwValue);
  iDigits = SHLWAPI_FormatSignificant(szOut + 1, iDigits);
  *szOut = ' ';
  LoadStringW(shlwapi_hInstance, uClassStringId, szBuff + 32, 32);
  strcatW(lpszOut, szOut);
  return iDigits;
}

/*************************************************************************
 * StrFromTimeIntervalA	[SHLWAPI.@]
 *
 * Format a millisecond time interval into a string
 *
 * PARAMS
 *  lpszStr  [O] Output buffer for formatted time interval
 *  cchMax   [I] Size of lpszStr
 *  dwMS     [I] Number of milliseconds
 *  iDigits  [I] Number of digits to print
 *
 * RETURNS
 *  The length of the formatted string, or 0 if any parameter is invalid.
 *
 * NOTES
 *  This implementation mimics the Win32 behaviour of always writing a leading
 *  space before the time interval begins.
 *
 *  iDigits is used to provide approximate times if accuracy is not important.
 *  This number of digits will be written of the first non-zero time class
 *  (hours/minutes/seconds). If this does not complete the time classification,
 *  the remaining digits are changed to zeros (i.e. The time is _not_ rounded).
 *  If there are digits remaining following the writing of a time class, the
 *  next time class will be written.
 *
 *  For example, given dwMS represents 138 hours,43 minutes and 15 seconds, the
 *  following will result from the given values of iDigits:
 *
 *|  iDigits    1        2        3        4               5               ...
 *|  lpszStr   "100 hr" "130 hr" "138 hr" "138 hr 40 min" "138 hr 43 min"  ...
 */
INT WINAPI StrFromTimeIntervalA(LPSTR lpszStr, UINT cchMax, DWORD dwMS,
                                int iDigits)
{
  INT iRet = 0;

  TRACE("(%p,%d,%d,%d)\n", lpszStr, cchMax, dwMS, iDigits);

  if (lpszStr && cchMax)
  {
    WCHAR szBuff[128];
    StrFromTimeIntervalW(szBuff, sizeof(szBuff)/sizeof(WCHAR), dwMS, iDigits);
    WideCharToMultiByte(CP_ACP,0,szBuff,-1,lpszStr,cchMax,0,0);
  }
  return iRet;
}


/*************************************************************************
 * StrFromTimeIntervalW	[SHLWAPI.@]
 *
 * See StrFromTimeIntervalA.
 */
INT WINAPI StrFromTimeIntervalW(LPWSTR lpszStr, UINT cchMax, DWORD dwMS,
                                int iDigits)
{
  INT iRet = 0;

  TRACE("(%p,%d,%d,%d)\n", lpszStr, cchMax, dwMS, iDigits);

  if (lpszStr && cchMax)
  {
    WCHAR szCopy[128];
    DWORD dwHours, dwMinutes;

    if (!iDigits || cchMax == 1)
    {
      *lpszStr = '\0';
      return 0;
    }

    /* Calculate the time classes */
    dwMS = (dwMS + 500) / 1000;
    dwHours = dwMS / 3600;
    dwMS -= dwHours * 3600;
    dwMinutes = dwMS / 60;
    dwMS -= dwMinutes * 60;

    szCopy[0] = '\0';

    if (dwHours)
      iDigits = SHLWAPI_WriteTimeClass(szCopy, dwHours, IDS_TIME_INTERVAL_HOURS, iDigits);

    if (dwMinutes && iDigits)
      iDigits = SHLWAPI_WriteTimeClass(szCopy, dwMinutes, IDS_TIME_INTERVAL_MINUTES, iDigits);

    if (iDigits) /* Always write seconds if we have significant digits */
      SHLWAPI_WriteTimeClass(szCopy, dwMS, IDS_TIME_INTERVAL_SECONDS, iDigits);

    lstrcpynW(lpszStr, szCopy, cchMax);
    iRet = strlenW(lpszStr);
  }
  return iRet;
}

/*************************************************************************
 * StrIsIntlEqualA	[SHLWAPI.@]
 *
 * Compare two strings.
 *
 * PARAMS
 *  bCase    [I] Whether to compare case sensitively
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *  iLen     [I] Length to compare
 *
 * RETURNS
 *  TRUE  If the strings are equal.
 *  FALSE Otherwise.
 */
BOOL WINAPI StrIsIntlEqualA(BOOL bCase, LPCSTR lpszStr, LPCSTR lpszComp,
                            int iLen)
{
  DWORD dwFlags;

  TRACE("(%d,%s,%s,%d)\n", bCase,
        debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);

  /* FIXME: This flag is undocumented and unknown by our CompareString.
   *        We need a define for it.
   */
  dwFlags = 0x10000000;
  if (!bCase) dwFlags |= NORM_IGNORECASE;

  return (CompareStringA(GetThreadLocale(), dwFlags, lpszStr, iLen, lpszComp, iLen) == CSTR_EQUAL);
}

/*************************************************************************
 * StrIsIntlEqualW	[SHLWAPI.@]
 *
 * See StrIsIntlEqualA.
 */
BOOL WINAPI StrIsIntlEqualW(BOOL bCase, LPCWSTR lpszStr, LPCWSTR lpszComp,
                            int iLen)
{
  DWORD dwFlags;

  TRACE("(%d,%s,%s,%d)\n", bCase,
        debugstr_w(lpszStr),debugstr_w(lpszComp), iLen);

  /* FIXME: This flag is undocumented and unknown by our CompareString.
   *        We need a define for it.
   */
  dwFlags = 0x10000000;
  if (!bCase) dwFlags |= NORM_IGNORECASE;

  return (CompareStringW(GetThreadLocale(), dwFlags, lpszStr, iLen, lpszComp, iLen) == CSTR_EQUAL);
}

/*************************************************************************
 * @    [SHLWAPI.399]
 *
 * Copy a string to another string, up to a maximum number of characters.
 *
 * PARAMS
 *  lpszDest [O] Destination string
 *  lpszSrc  [I] Source string
 *  iLen     [I] Maximum number of chars to copy
 *
 * RETURNS
 *  Success: A pointer to the last character written to lpszDest.
 *  Failure: lpszDest, if any arguments are invalid.
 */
LPSTR WINAPI StrCpyNXA(LPSTR lpszDest, LPCSTR lpszSrc, int iLen)
{
  TRACE("(%p,%s,%i)\n", lpszDest, debugstr_a(lpszSrc), iLen);

  if (lpszDest && lpszSrc && iLen > 0)
  {
    while ((iLen-- > 1) && *lpszSrc)
      *lpszDest++ = *lpszSrc++;
    if (iLen >= 0)
     *lpszDest = '\0';
  }
  return lpszDest;
}

/*************************************************************************
 * @    [SHLWAPI.400]
 *
 * Unicode version of StrCpyNXA.
 */
LPWSTR WINAPI StrCpyNXW(LPWSTR lpszDest, LPCWSTR lpszSrc, int iLen)
{
  TRACE("(%p,%s,%i)\n", lpszDest, debugstr_w(lpszSrc), iLen);

  if (lpszDest && lpszSrc && iLen > 0)
  {
    while ((iLen-- > 1) && *lpszSrc)
      *lpszDest++ = *lpszSrc++;
    if (iLen >= 0)
     *lpszDest = '\0';
  }
  return lpszDest;
}

/*************************************************************************
 * StrCmpLogicalW	[SHLWAPI.@]
 *
 * Compare two strings, ignoring case and comparing digits as numbers.
 *
 * PARAMS
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *  iLen     [I] Length to compare
 *
 * RETURNS
 *  TRUE  If the strings are equal.
 *  FALSE Otherwise.
 */
INT WINAPI StrCmpLogicalW(LPCWSTR lpszStr, LPCWSTR lpszComp)
{
  INT iDiff;

  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszComp));

  if (lpszStr && lpszComp)
  {
    while (*lpszStr)
    {
      if (!*lpszComp)
        return 1;
      else if (isdigitW(*lpszStr))
      {
        int iStr, iComp;

        if (!isdigitW(*lpszComp))
          return -1;

        /* Compare the numbers */
        StrToIntExW(lpszStr, 0, &iStr);
        StrToIntExW(lpszComp, 0, &iComp);

        if (iStr < iComp)
          return -1;
        else if (iStr > iComp)
          return 1;

        /* Skip */
        while (isdigitW(*lpszStr))
          lpszStr++;
        while (isdigitW(*lpszComp))
          lpszComp++;
      }
      else if (isdigitW(*lpszComp))
        return 1;
      else
      {
        iDiff = ChrCmpIW(*lpszStr,*lpszComp);
        if (iDiff > 0)
          return 1;
        else if (iDiff < 0)
          return -1;

        lpszStr++;
        lpszComp++;
      }
    }
    if (*lpszComp)
      return -1;
  }
  return 0;
}

/* Structure for formatting byte strings */
typedef struct tagSHLWAPI_BYTEFORMATS
{
  LONGLONG dLimit;
  double   dDivisor;
  double   dNormaliser;
  int      nDecimals;
#ifdef __REACTOS__
  UINT     nFormatID;
#else
  WCHAR     wPrefix;
#endif
} SHLWAPI_BYTEFORMATS;

/*************************************************************************
 * StrFormatByteSizeW	[SHLWAPI.@]
 *
 * Create a string containing an abbreviated byte count of up to 2^63-1.
 *
 * PARAMS
 *  llBytes  [I] Byte size to format
 *  lpszDest [I] Destination for formatted string
 *  cchMax   [I] Size of lpszDest
 *
 * RETURNS
 *  lpszDest.
 *
 * NOTES
 *  There is no StrFormatByteSize64W function, it is called StrFormatByteSizeW().
 */
LPWSTR WINAPI StrFormatByteSizeW(LONGLONG llBytes, LPWSTR lpszDest, UINT cchMax)
{
#define KB ((ULONGLONG)1024)
#define MB (KB*KB)
#define GB (KB*KB*KB)
#define TB (KB*KB*KB*KB)
#define PB (KB*KB*KB*KB*KB)

  static const SHLWAPI_BYTEFORMATS bfFormats[] =
  {
#ifdef __REACTOS__
    { 10*KB, 10.24, 100.0, 2, IDS_KB_FORMAT }, /* 10 KB */
    { 100*KB, 102.4, 10.0, 1, IDS_KB_FORMAT }, /* 100 KB */
    { 1000*KB, 1024.0, 1.0, 0, IDS_KB_FORMAT }, /* 1000 KB */
    { 10*MB, 10485.76, 100.0, 2, IDS_MB_FORMAT }, /* 10 MB */
    { 100*MB, 104857.6, 10.0, 1, IDS_MB_FORMAT }, /* 100 MB */
    { 1000*MB, 1048576.0, 1.0, 0, IDS_MB_FORMAT }, /* 1000 MB */
    { 10*GB, 10737418.24, 100.0, 2, IDS_GB_FORMAT }, /* 10 GB */
    { 100*GB, 107374182.4, 10.0, 1, IDS_GB_FORMAT }, /* 100 GB */
    { 1000*GB, 1073741824.0, 1.0, 0, IDS_GB_FORMAT }, /* 1000 GB */
    { 10*TB, 10485.76, 100.0, 2, IDS_TB_FORMAT }, /* 10 TB */
    { 100*TB, 104857.6, 10.0, 1, IDS_TB_FORMAT }, /* 100 TB */
    { 1000*TB, 1048576.0, 1.0, 0, IDS_TB_FORMAT }, /* 1000 TB */
    { 10*PB, 10737418.24, 100.00, 2, IDS_PB_FORMAT }, /* 10 PB */
    { 100*PB, 107374182.4, 10.00, 1, IDS_PB_FORMAT }, /* 100 PB */
    { 1000*PB, 1073741824.0, 1.00, 0, IDS_PB_FORMAT }, /* 1000 PB */
    { 0, 10995116277.76, 100.00, 2, IDS_EB_FORMAT } /* EB's, catch all */
#else
    { 10*KB, 10.24, 100.0, 2, 'K' }, /* 10 KB */
    { 100*KB, 102.4, 10.0, 1, 'K' }, /* 100 KB */
    { 1000*KB, 1024.0, 1.0, 0, 'K' }, /* 1000 KB */
    { 10*MB, 10485.76, 100.0, 2, 'M' }, /* 10 MB */
    { 100*MB, 104857.6, 10.0, 1, 'M' }, /* 100 MB */
    { 1000*MB, 1048576.0, 1.0, 0, 'M' }, /* 1000 MB */
    { 10*GB, 10737418.24, 100.0, 2, 'G' }, /* 10 GB */
    { 100*GB, 107374182.4, 10.0, 1, 'G' }, /* 100 GB */
    { 1000*GB, 1073741824.0, 1.0, 0, 'G' }, /* 1000 GB */
    { 10*TB, 10485.76, 100.0, 2, 'T' }, /* 10 TB */
    { 100*TB, 104857.6, 10.0, 1, 'T' }, /* 100 TB */
    { 1000*TB, 1048576.0, 1.0, 0, 'T' }, /* 1000 TB */
    { 10*PB, 10737418.24, 100.00, 2, 'P' }, /* 10 PB */
    { 100*PB, 107374182.4, 10.00, 1, 'P' }, /* 100 PB */
    { 1000*PB, 1073741824.0, 1.00, 0, 'P' }, /* 1000 PB */
    { 0, 10995116277.76, 100.00, 2, 'E' } /* EB's, catch all */
#endif
  };
#ifdef __REACTOS__
  WCHAR szBuff[40], wszFormat[40];
#else
  WCHAR wszAdd[] = {' ','?','B',0};
#endif
  double dBytes;
  UINT i = 0;

  TRACE("(0x%s,%p,%d)\n", wine_dbgstr_longlong(llBytes), lpszDest, cchMax);

  if (!lpszDest || !cchMax)
    return lpszDest;

  if (llBytes < 1024)  /* 1K */
  {
    WCHAR wszBytesFormat[64];
    LoadStringW(shlwapi_hInstance, IDS_BYTES_FORMAT, wszBytesFormat, 64);
    snprintfW(lpszDest, cchMax, wszBytesFormat, (int)llBytes);
    return lpszDest;
  }

  /* Note that if this loop completes without finding a match, i will be
   * pointing at the last entry, which is a catch all for > 1000 PB
   */
  while (i < sizeof(bfFormats) / sizeof(SHLWAPI_BYTEFORMATS) - 1)
  {
    if (llBytes < bfFormats[i].dLimit)
      break;
    i++;
  }
  /* Above 1 TB we encounter problems with FP accuracy. So for amounts above
   * this number we integer shift down by 1 MB first. The table above has
   * the divisors scaled down from the '< 10 TB' entry onwards, to account
   * for this. We also add a small fudge factor to get the correct result for
   * counts that lie exactly on a 1024 byte boundary.
   */
  if (i > 8)
    dBytes = (double)(llBytes >> 20) + 0.001; /* Scale down by 1 MB */
  else
    dBytes = (double)llBytes + 0.00001;

  dBytes = floor(dBytes / bfFormats[i].dDivisor) / bfFormats[i].dNormaliser;

#ifdef __REACTOS__
  if (!FormatDouble(dBytes, bfFormats[i].nDecimals, szBuff, ARRAYSIZE(szBuff)))
    return NULL;
  LoadStringW(shlwapi_hInstance, bfFormats[i].nFormatID, wszFormat, ARRAYSIZE(wszFormat));
  snprintfW(lpszDest, cchMax, wszFormat, szBuff);
#else
  if (!FormatDouble(dBytes, bfFormats[i].nDecimals, lpszDest, cchMax))
    return NULL;
  wszAdd[1] = bfFormats[i].wPrefix;
  StrCatBuffW(lpszDest, wszAdd, cchMax);
#endif
  return lpszDest;
}

/*************************************************************************
 * StrFormatByteSize64A	[SHLWAPI.@]
 *
 * See StrFormatByteSizeW.
 */
LPSTR WINAPI StrFormatByteSize64A(LONGLONG llBytes, LPSTR lpszDest, UINT cchMax)
{
  WCHAR wszBuff[32];

  StrFormatByteSizeW(llBytes, wszBuff, sizeof(wszBuff)/sizeof(WCHAR));

  if (lpszDest)
    WideCharToMultiByte(CP_ACP, 0, wszBuff, -1, lpszDest, cchMax, 0, 0);
  return lpszDest;
}

/*************************************************************************
 * StrFormatByteSizeA	[SHLWAPI.@]
 *
 * Create a string containing an abbreviated byte count of up to 2^31-1.
 *
 * PARAMS
 *  dwBytes  [I] Byte size to format
 *  lpszDest [I] Destination for formatted string
 *  cchMax   [I] Size of lpszDest
 *
 * RETURNS
 *  lpszDest.
 *
 * NOTES
 *  The Ascii and Unicode versions of this function accept a different
 *  integer type for dwBytes. See StrFormatByteSize64A().
 */
LPSTR WINAPI StrFormatByteSizeA(DWORD dwBytes, LPSTR lpszDest, UINT cchMax)
{
  TRACE("(%d,%p,%d)\n", dwBytes, lpszDest, cchMax);

  return StrFormatByteSize64A(dwBytes, lpszDest, cchMax);
}

/*************************************************************************
 *      @	[SHLWAPI.162]
 *
 * Remove a hanging lead byte from the end of a string, if present.
 *
 * PARAMS
 *  lpStr [I] String to check for a hanging lead byte
 *  size  [I] Length of lpStr
 *
 * RETURNS
 *  Success: The new length of the string. Any hanging lead bytes are removed.
 *  Failure: 0, if any parameters are invalid.
 */
DWORD WINAPI SHTruncateString(LPSTR lpStr, DWORD size)
{
  if (lpStr && size)
  {
    LPSTR lastByte = lpStr + size - 1;

    while(lpStr < lastByte)
      lpStr += IsDBCSLeadByte(*lpStr) ? 2 : 1;

    if(lpStr == lastByte && IsDBCSLeadByte(*lpStr))
    {
      *lpStr = '\0';
      size--;
    }
    return size;
  }
  return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.203]
 *
 * Remove a single non-trailing ampersand ('&') from a string.
 *
 * PARAMS
 *  lpszStr [I/O] String to remove ampersand from.
 *
 * RETURNS
 *  The character after the first ampersand in lpszStr, or the first character
 *  in lpszStr if there is no ampersand in the string.
 */
char WINAPI SHStripMneumonicA(LPCSTR lpszStr)
{
  LPSTR lpszIter, lpszTmp;
  char ch;

  TRACE("(%s)\n", debugstr_a(lpszStr));

  ch = *lpszStr;

  if ((lpszIter = StrChrA(lpszStr, '&')))
  {
    lpszTmp = CharNextA(lpszIter);
    if (*lpszTmp)
    {
      if (*lpszTmp != '&')
        ch =  *lpszTmp;

      memmove( lpszIter, lpszTmp, strlen(lpszTmp) + 1 );
    }
  }

  return ch;
}

/*************************************************************************
 *      @	[SHLWAPI.225]
 *
 * Unicode version of SHStripMneumonicA.
 */
WCHAR WINAPI SHStripMneumonicW(LPCWSTR lpszStr)
{
  LPWSTR lpszIter, lpszTmp;
  WCHAR ch;

  TRACE("(%s)\n", debugstr_w(lpszStr));

  ch = *lpszStr;

  if ((lpszIter = StrChrW(lpszStr, '&')))
  {
    lpszTmp = lpszIter + 1;
    if (*lpszTmp)
    {
      if (*lpszTmp != '&')
        ch =  *lpszTmp;

      memmove( lpszIter, lpszTmp, (strlenW(lpszTmp) + 1) * sizeof(WCHAR) );
    }
  }

  return ch;
}

/*************************************************************************
 *      @	[SHLWAPI.216]
 *
 * Convert an Ascii string to Unicode.
 *
 * PARAMS
 * dwCp     [I] Code page for the conversion
 * lpSrcStr [I] Source Ascii string to convert
 * lpDstStr [O] Destination for converted Unicode string
 * iLen     [I] Length of lpDstStr
 *
 * RETURNS
 *  The return value of the MultiByteToWideChar() function called on lpSrcStr.
 */
DWORD WINAPI SHAnsiToUnicodeCP(DWORD dwCp, LPCSTR lpSrcStr, LPWSTR lpDstStr, int iLen)
{
  DWORD dwRet;

  dwRet = MultiByteToWideChar(dwCp, 0, lpSrcStr, -1, lpDstStr, iLen);
  TRACE("%s->%s,ret=%d\n", debugstr_a(lpSrcStr), debugstr_w(lpDstStr), dwRet);
  return dwRet;
}

/*************************************************************************
 *      @	[SHLWAPI.215]
 *
 * Convert an Ascii string to Unicode.
 *
 * PARAMS
 * lpSrcStr [I] Source Ascii string to convert
 * lpDstStr [O] Destination for converted Unicode string
 * iLen     [I] Length of lpDstStr
 *
 * RETURNS
 *  The return value of the MultiByteToWideChar() function called on lpSrcStr.
 *
 * NOTES
 *  This function simply calls SHAnsiToUnicodeCP with code page CP_ACP.
 */
DWORD WINAPI SHAnsiToUnicode(LPCSTR lpSrcStr, LPWSTR lpDstStr, int iLen)
{
  return SHAnsiToUnicodeCP(CP_ACP, lpSrcStr, lpDstStr, iLen);
}

/*************************************************************************
 *      @	[SHLWAPI.218]
 *
 * Convert a Unicode string to Ascii.
 *
 * PARAMS
 *  CodePage [I] Code page to use for the conversion
 *  lpSrcStr [I] Source Unicode string to convert
 *  lpDstStr [O] Destination for converted Ascii string
 *  dstlen   [I] Length of buffer at lpDstStr
 *
 * RETURNS
 *  Success: The length in bytes of the result at lpDstStr (including the terminator)
 *  Failure: When using CP_UTF8, CP_UTF7 or 0xc350 as codePage, 0 is returned and
 *           the result is not nul-terminated.
 *           When using a different codepage, the length in bytes of the truncated
 *           result at lpDstStr (including the terminator) is returned and
 *           lpDstStr is always nul-terminated.
 *
 */
DWORD WINAPI SHUnicodeToAnsiCP(UINT CodePage, LPCWSTR lpSrcStr, LPSTR lpDstStr, int dstlen)
{
  static const WCHAR emptyW[] = { '\0' };
  int len , reqLen;
  LPSTR mem;

  if (!lpDstStr || !dstlen)
    return 0;

  if (!lpSrcStr)
    lpSrcStr = emptyW;

  *lpDstStr = '\0';

  len = strlenW(lpSrcStr) + 1;

  switch (CodePage)
  {
  case CP_WINUNICODE:
    CodePage = CP_UTF8; /* Fall through... */
  case 0x0000C350: /* FIXME: CP_ #define */
  case CP_UTF7:
  case CP_UTF8:
    {
      DWORD dwMode = 0;
      INT lenW = len - 1;
      INT needed = dstlen - 1;
      HRESULT hr;

      /* try the user supplied buffer first */
      hr = ConvertINetUnicodeToMultiByte(&dwMode, CodePage, lpSrcStr, &lenW, lpDstStr, &needed);
      if (hr == S_OK)
      {
        lpDstStr[needed] = '\0';
        return needed + 1;
      }

      /* user buffer too small. exclude termination and copy as much as possible */
      lenW = len;
      hr = ConvertINetUnicodeToMultiByte(&dwMode, CodePage, lpSrcStr, &lenW, NULL, &needed);
      needed++;
      mem = HeapAlloc(GetProcessHeap(), 0, needed);
      if (!mem)
        return 0;

      hr = ConvertINetUnicodeToMultiByte(&dwMode, CodePage, lpSrcStr, &len, mem, &needed);
      if (hr == S_OK)
      {
          reqLen = SHTruncateString(mem, dstlen);
          if (reqLen > 0) memcpy(lpDstStr, mem, reqLen-1);
      }
      HeapFree(GetProcessHeap(), 0, mem);
      return 0;
    }
  default:
    break;
  }

  /* try the user supplied buffer first */
  reqLen = WideCharToMultiByte(CodePage, 0, lpSrcStr, len, lpDstStr, dstlen, NULL, NULL);

  if (!reqLen && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    reqLen = WideCharToMultiByte(CodePage, 0, lpSrcStr, len, NULL, 0, NULL, NULL);
    if (reqLen)
    {
      mem = HeapAlloc(GetProcessHeap(), 0, reqLen);
      if (mem)
      {
        WideCharToMultiByte(CodePage, 0, lpSrcStr, len, mem, reqLen, NULL, NULL);

        reqLen = SHTruncateString(mem, dstlen -1);
        reqLen++;

        lstrcpynA(lpDstStr, mem, reqLen);
        HeapFree(GetProcessHeap(), 0, mem);
        lpDstStr[reqLen-1] = '\0';
      }
    }
  }
  return reqLen;
}

/*************************************************************************
 *      @	[SHLWAPI.217]
 *
 * Convert a Unicode string to Ascii.
 *
 * PARAMS
 *  lpSrcStr [I] Source Unicode string to convert
 *  lpDstStr [O] Destination for converted Ascii string
 *  iLen     [O] Length of lpDstStr in characters
 *
 * RETURNS
 *  See SHUnicodeToAnsiCP

 * NOTES
 *  This function simply calls SHUnicodeToAnsiCP() with CodePage = CP_ACP.
 */
INT WINAPI SHUnicodeToAnsi(LPCWSTR lpSrcStr, LPSTR lpDstStr, INT iLen)
{
    return SHUnicodeToAnsiCP(CP_ACP, lpSrcStr, lpDstStr, iLen);
}

/*************************************************************************
 *      @	[SHLWAPI.345]
 *
 * Copy one string to another.
 *
 * PARAMS
 *  lpszSrc [I] Source string to copy
 *  lpszDst [O] Destination for copy
 *  iLen    [I] Length of lpszDst in characters
 *
 * RETURNS
 *  The length of the copied string, including the terminating NUL. lpszDst
 *  contains iLen characters of lpszSrc.
 */
DWORD WINAPI SHAnsiToAnsi(LPCSTR lpszSrc, LPSTR lpszDst, int iLen)
{
    LPSTR lpszRet;

    TRACE("(%s,%p,0x%08x)\n", debugstr_a(lpszSrc), lpszDst, iLen);

    lpszRet = StrCpyNXA(lpszDst, lpszSrc, iLen);
    return lpszRet - lpszDst + 1;
}

/*************************************************************************
 *      @	[SHLWAPI.346]
 *
 * Unicode version of SSHAnsiToAnsi.
 */
DWORD WINAPI SHUnicodeToUnicode(LPCWSTR lpszSrc, LPWSTR lpszDst, int iLen)
{
    LPWSTR lpszRet;

    TRACE("(%s,%p,0x%08x)\n", debugstr_w(lpszSrc), lpszDst, iLen);

    lpszRet = StrCpyNXW(lpszDst, lpszSrc, iLen);
    return lpszRet - lpszDst + 1;
}

/*************************************************************************
 *      @	[SHLWAPI.364]
 *
 * Determine if an Ascii string converts to Unicode and back identically.
 *
 * PARAMS
 *  lpSrcStr [I] Source Unicode string to convert
 *  lpDst    [O] Destination for resulting Ascii string
 *  iLen     [I] Length of lpDst in characters
 *
 * RETURNS
 *  TRUE, since Ascii strings always convert identically.
 */
BOOL WINAPI DoesStringRoundTripA(LPCSTR lpSrcStr, LPSTR lpDst, INT iLen)
{
  lstrcpynA(lpDst, lpSrcStr, iLen);
  return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.365]
 *
 * Determine if a Unicode string converts to Ascii and back identically.
 *
 * PARAMS
 *  lpSrcStr [I] Source Unicode string to convert
 *  lpDst    [O] Destination for resulting Ascii string
 *  iLen     [I] Length of lpDst in characters
 *
 * RETURNS
 *  TRUE, if lpSrcStr converts to Ascii and back identically,
 *  FALSE otherwise.
 */
BOOL WINAPI DoesStringRoundTripW(LPCWSTR lpSrcStr, LPSTR lpDst, INT iLen)
{
    WCHAR szBuff[MAX_PATH];

    SHUnicodeToAnsi(lpSrcStr, lpDst, iLen);
    SHAnsiToUnicode(lpDst, szBuff, MAX_PATH);
    return !strcmpW(lpSrcStr, szBuff);
}

/*************************************************************************
 *      SHLoadIndirectString    [SHLWAPI.@]
 *
 * If passed a string that begins with '@', extract the string from the
 * appropriate resource, otherwise do a straight copy.
 *
 */
HRESULT WINAPI SHLoadIndirectString(LPCWSTR src, LPWSTR dst, UINT dst_len, void **reserved)
{
    WCHAR *dllname = NULL;
    HMODULE hmod = NULL;
    HRESULT hr = E_FAIL;
#ifdef __REACTOS__
    WCHAR szExpanded[512];
#endif

    TRACE("(%s %p %08x %p)\n", debugstr_w(src), dst, dst_len, reserved);

    if(src[0] == '@')
    {
        WCHAR *index_str;
        int index;

#ifdef __REACTOS__
        if (wcschr(src, '%') != NULL)
        {
            ExpandEnvironmentStringsW(src, szExpanded, ARRAY_SIZE(szExpanded));
            src = szExpanded;
        }
#endif
        dst[0] = 0;
        dllname = StrDupW(src + 1);
        index_str = strchrW(dllname, ',');

        if(!index_str) goto end;

        *index_str = 0;
        index_str++;
        index = atoiW(index_str);
  
#ifdef __REACTOS__
        hmod = LoadLibraryExW(dllname, NULL, LOAD_LIBRARY_AS_DATAFILE);
#else
        hmod = LoadLibraryW(dllname);
#endif
        if(!hmod) goto end;

        if(index < 0)
        {
            if(LoadStringW(hmod, -index, dst, dst_len))
                hr = S_OK;
        }
        else
            FIXME("can't handle non-negative indices (%d)\n", index);
    }
    else
    {
        if(dst != src)
            lstrcpynW(dst, src, dst_len);
        hr = S_OK;
    }

    TRACE("returning %s\n", debugstr_w(dst));
end:
    if(hmod) FreeLibrary(hmod);
    LocalFree(dllname);
    return hr;
}

BOOL WINAPI IsCharSpaceA(CHAR c)
{
    WORD CharType;
    return GetStringTypeA(GetSystemDefaultLCID(), CT_CTYPE1, &c, 1, &CharType) && (CharType & C1_SPACE);
}

/*************************************************************************
 *      @	[SHLWAPI.29]
 *
 * Determine if a Unicode character is a space.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a space,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharSpaceW(WCHAR wc)
{
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_SPACE);
}
