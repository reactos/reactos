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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COM_NO_WINDOWS_H
#include "config.h"
#include "wine/port.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static HRESULT WINAPI _SHStrDupAA(LPCSTR,LPSTR*);
static HRESULT WINAPI _SHStrDupAW(LPCWSTR,LPSTR*);

/*************************************************************************
 * SHLWAPI_ChrCmpHelperA
 *
 * Internal helper for SHLWAPI_ChrCmpA/ChrCMPIA.
 *
 * NOTES
 *  Both this function and its Unicode counterpart are very inneficient. To
 *  fix this, CompareString must be completely implemented and optimised
 *  first. Then the core character test can be taken out of that function and
 *  placed here, so that it need never be called at all. Until then, do not
 *  attempt to optimise this code unless you are willing to test that it
 *  still performs correctly.
 */
static BOOL WINAPI SHLWAPI_ChrCmpHelperA(WORD ch1, WORD ch2, DWORD dwFlags)
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

  return CompareStringA(GetThreadLocale(), dwFlags, str1, -1, str2, -1) - 2;
}

/*************************************************************************
 * SHLWAPI_ChrCmpHelperW
 *
 * Internal helper for SHLWAPI_ChrCmpW/ChrCmpIW.
 */
static BOOL WINAPI SHLWAPI_ChrCmpHelperW(WCHAR ch1, WCHAR ch2, DWORD dwFlags)
{
  WCHAR str1[2], str2[2];

  str1[0] = ch1;
  str1[1] = '\0';
  str2[0] = ch2;
  str2[1] = '\0';
  return CompareStringW(GetThreadLocale(), dwFlags, str1, 2, str2, 2) - 2;
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
 * SHLWAPI_ChrCmpW
 *
 * Internal helper function.
 */
static BOOL WINAPI SHLWAPI_ChrCmpW(WCHAR ch1, WCHAR ch2)
{
  return SHLWAPI_ChrCmpHelperW(ch1, ch2, 0);
}

/*************************************************************************
 * ChrCmpIW	[SHLWAPI.386]
 *
 * See ChrCmpIA.
 */
BOOL WINAPI ChrCmpIW(WCHAR ch1, WCHAR ch2)
{
  return SHLWAPI_ChrCmpHelperW(ch1, ch2, NORM_IGNORECASE);
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
      lpszStr = CharNextW(lpszStr);
    }
    lpszStr = NULL;
  }
  return (LPWSTR)lpszStr;
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
  INT iRet;

  TRACE("(%s,%s)\n", debugstr_w(lpszStr),debugstr_w(lpszComp));

  iRet = strcmpiW(lpszStr, lpszComp);
  return iRet < 0 ? -1 : iRet ? 1 : 0;
}

/*************************************************************************
 * SHLWAPI_StrCmpNHelperA
 *
 * Internal helper for StrCmpNA/StrCmpNIA.
 */
static INT WINAPI SHLWAPI_StrCmpNHelperA(LPCSTR lpszStr, LPCSTR lpszComp,
                                         INT iLen,
                                         BOOL (WINAPI *pChrCmpFn)(WORD,WORD))
{
  if (!lpszStr)
  {
    if (!lpszComp)
      return 0;
    return 1;
  }
  else if (!lpszComp)
    return -1;

  while (iLen-- > 0)
  {
    int iDiff;
    WORD ch1, ch2;

    ch1 = IsDBCSLeadByte(*lpszStr)? *lpszStr << 8 | lpszStr[1] : *lpszStr;
    ch2 = IsDBCSLeadByte(*lpszComp)? *lpszComp << 8 | lpszComp[1] : *lpszComp;

    if ((iDiff = pChrCmpFn(ch1, ch2)) < 0)
      return -1;
    else if (iDiff > 0)
      return 1;
    else if (!*lpszStr && !*lpszComp)
      return 0;

    lpszStr = CharNextA(lpszStr);
    lpszComp = CharNextA(lpszComp);
  }
  return 0;
}

/*************************************************************************
 * StrCmpNA	[SHLWAPI.@]
 *
 * Compare two strings, up to a maximum length.
 *
 * PARAMS
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *  iLen     [I] Maximum number of chars to compare.
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszStr is less than, the same, or greater than lpszComp.
 */
INT WINAPI StrCmpNA(LPCSTR lpszStr, LPCSTR lpszComp, INT iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);

  return SHLWAPI_StrCmpNHelperA(lpszStr, lpszComp, iLen, SHLWAPI_ChrCmpA);
}

/*************************************************************************
 * StrCmpNW	[SHLWAPI.@]
 *
 * See StrCmpNA.
 */
INT WINAPI StrCmpNW(LPCWSTR lpszStr, LPCWSTR lpszComp, INT iLen)
{
  INT iRet;

  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszComp), iLen);

  iRet = strncmpW(lpszStr, lpszComp, iLen);
  return iRet < 0 ? -1 : iRet ? 1 : 0;
}

/*************************************************************************
 * StrCmpNIA	[SHLWAPI.@]
 *
 * Compare two strings, up to a maximum length, ignoring case.
 *
 * PARAMS
 *  lpszStr  [I] First string to compare
 *  lpszComp [I] Second string to compare
 *  iLen     [I] Maximum number of chars to compare.
 *
 * RETURNS
 *  An integer less than, equal to or greater than 0, indicating that
 *  lpszStr is less than, the same, or greater than lpszComp.
 *
 * NOTES
 *  The Win32 version of this function is _completely_ broken for cases
 *  where iLen is greater than the length of lpszComp. Examples:
 *
 *|  StrCmpNIA("foo.gif", "foo", 5) is -1 under Win32; Should return 1.
 *|  StrCmpNIA("\", "\\", 3) is 0 under Win32; Should return -1.
 *|  StrCmpNIA("\", "\..\foo\", 3) is 1 under Win32; Should return -1.
 *
 *  This implementation behaves correctly, since it is unlikely any
 *  applications actually rely on this function being broken.
 */
int WINAPI StrCmpNIA(LPCSTR lpszStr, LPCSTR lpszComp, int iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);

  return SHLWAPI_StrCmpNHelperA(lpszStr, lpszComp, iLen, ChrCmpIA);
}

/*************************************************************************
 * StrCmpNIW	[SHLWAPI.@]
 *
 * See StrCmpNIA.
 */
INT WINAPI StrCmpNIW(LPCWSTR lpszStr, LPCWSTR lpszComp, int iLen)
{
  INT iRet;

  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszComp), iLen);

  iRet = strncmpiW(lpszStr, lpszComp, iLen);
  return iRet < 0 ? -1 : iRet ? 1 : 0;
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
  INT iRet;

  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszComp));

  iRet = strcmpW(lpszStr, lpszComp);
  return iRet < 0 ? -1 : iRet ? 1 : 0;
}

/*************************************************************************
 * StrCatW	[SHLWAPI.@]
 *
 * Concatanate two strings.
 *
 * PARAMS
 *  lpszStr [O] Initial string
 *  lpszSrc [I] String to concatanate
 *
 * RETURNS
 *  lpszStr.
 */
LPWSTR WINAPI StrCatW(LPWSTR lpszStr, LPCWSTR lpszSrc)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSrc));

  strcatW(lpszStr, lpszSrc);
  return lpszStr;
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

  strcpyW(lpszStr, lpszSrc);
  return lpszStr;
}

/*************************************************************************
 * StrCpyNW	[SHLWAPI.@]
 *
 * Copy a string to another string, up to a maximum number of characters.
 *
 * PARAMS
 *  lpszStr  [O] Destination string
 *  lpszSrc  [I] Source string
 *  iLen     [I] Maximum number of chars to copy
 *
 * RETURNS
 *  lpszStr.
 */
LPWSTR WINAPI StrCpyNW(LPWSTR lpszStr, LPCWSTR lpszSrc, int iLen)
{
  TRACE("(%p,%s,%i)\n", lpszStr, debugstr_w(lpszSrc), iLen);

  lstrcpynW(lpszStr, lpszSrc, iLen);
  return lpszStr;
}



/*************************************************************************
 * SHLWAPI_StrStrHelperA
 *
 * Internal implementation of StrStrA/StrStrIA
 */
static LPSTR WINAPI SHLWAPI_StrStrHelperA(LPCSTR lpszStr, LPCSTR lpszSearch,
                                         int (*pStrCmpFn)(LPCSTR,LPCSTR,size_t))
{
  size_t iLen;

  if (!lpszStr || !lpszSearch || !*lpszSearch)
    return NULL;

  iLen = strlen(lpszSearch);

  while (*lpszStr)
  {
    if (!pStrCmpFn(lpszStr, lpszSearch, iLen))
      return (LPSTR)lpszStr;
    lpszStr = CharNextA(lpszStr);
  }
  return NULL;
}

/*************************************************************************
 * SHLWAPI_StrStrHelperW
 *
 * Internal implementation of StrStrW/StrStrIW
 */
static LPWSTR WINAPI SHLWAPI_StrStrHelperW(LPCWSTR lpszStr, LPCWSTR lpszSearch,
                                          int (*pStrCmpFn)(LPCWSTR,LPCWSTR,int))
{
  int iLen;

  if (!lpszStr || !lpszSearch || !*lpszSearch)
    return NULL;

  iLen = strlenW(lpszSearch);

  while (*lpszStr)
  {
    if (!pStrCmpFn(lpszStr, lpszSearch, iLen))
      return (LPWSTR)lpszStr;
    lpszStr = CharNextW(lpszStr);
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

  return SHLWAPI_StrStrHelperA(lpszStr, lpszSearch, strncmp);
}

/*************************************************************************
 * StrStrW	[SHLWAPI.@]
 *
 * See StrStrA.
 */
LPWSTR WINAPI StrStrW(LPCWSTR lpszStr, LPCWSTR lpszSearch)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSearch));

  return SHLWAPI_StrStrHelperW(lpszStr, lpszSearch, (int (*)(LPCWSTR,LPCWSTR,int))wcsncmp);
}

/*************************************************************************
 * StrRStrIA	[SHLWAPI.@]
 *
 * Find the last occurence of a substring within a string.
 *
 * PARAMS
 *  lpszStr    [I] String to search in
 *  lpszEnd    [I] End of lpszStr
 *  lpszSearch [I] String to look for
 *
 * RETURNS
 *  The last occurence lpszSearch within lpszStr, or NULL if not found.
 */
LPSTR WINAPI StrRStrIA(LPCSTR lpszStr, LPCSTR lpszEnd, LPCSTR lpszSearch)
{
  LPSTR lpszRet = NULL;
  WORD ch1, ch2;
  INT iLen;

  TRACE("(%s,%s)\n", debugstr_a(lpszStr), debugstr_a(lpszSearch));

  if (!lpszStr || !lpszSearch || !*lpszSearch)
    return NULL;

  if (!lpszEnd)
    lpszEnd = lpszStr + lstrlenA(lpszStr);

  if (IsDBCSLeadByte(*lpszSearch))
    ch1 = *lpszSearch << 8 | lpszSearch[1];
  else
    ch1 = *lpszSearch;
  iLen = lstrlenA(lpszSearch);

  while (lpszStr <= lpszEnd  && *lpszStr)
  {
    ch2 = IsDBCSLeadByte(*lpszStr)? *lpszStr << 8 | lpszStr[1] : *lpszStr;
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

  if (!lpszEnd)
    lpszEnd = lpszStr + strlenW(lpszStr);

  iLen = strlenW(lpszSearch);

  while (lpszStr <= lpszEnd  && *lpszStr)
  {
    if (!ChrCmpIA(*lpszSearch, *lpszStr))
    {
      if (!StrCmpNIW(lpszStr, lpszSearch, iLen))
        lpszRet = (LPWSTR)lpszStr;
    }
    lpszStr = CharNextW(lpszStr);
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

  return SHLWAPI_StrStrHelperA(lpszStr, lpszSearch, strncasecmp);
}

/*************************************************************************
 * StrStrIW	[SHLWAPI.@]
 *
 * See StrStrIA.
 */
LPWSTR WINAPI StrStrIW(LPCWSTR lpszStr, LPCWSTR lpszSearch)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSearch));

  return SHLWAPI_StrStrHelperW(lpszStr, lpszSearch, (int (*)(LPCWSTR,LPCWSTR,int))_wcsnicmp);
}

/*************************************************************************
 * StrToIntA	[SHLWAPI.@]
 *
 * Read an integer from a string.
 *
 * PARAMS
 *  lpszStr [I] String to read integer from
 *
 * RETURNS
 *   The integer value represented by the string, or 0 if no integer is
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
 *  dwFlags includes STIF_SUPPORT_HEX, hexidecimal numbers are allowed, if
 *  preceeded by '0x'. If this flag is not set, or there is no '0x' prefix,
 *  the string is treated as a decimal string. A leading '-' is ignored for
 *  hexidecimal numbers.
 */
BOOL WINAPI StrToIntExA(LPCSTR lpszStr, DWORD dwFlags, LPINT lpiRet)
{
  BOOL bNegative = FALSE;
  int iRet = 0;

  TRACE("(%s,%08lX,%p)\n", debugstr_a(lpszStr), dwFlags, lpiRet);

  if (!lpszStr || !lpiRet)
  {
    WARN("Invalid parameter would crash under Win32!\n");
    return FALSE;
  }
  if (dwFlags > STIF_SUPPORT_HEX)
  {
    WARN("Unknown flags (%08lX)!\n", dwFlags & ~STIF_SUPPORT_HEX);
  }

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
BOOL WINAPI StrToIntExW(LPCWSTR lpszStr, DWORD dwFlags, LPINT lpiRet)
{
  BOOL bNegative = FALSE;
  int iRet = 0;

  TRACE("(%s,%08lX,%p)\n", debugstr_w(lpszStr), dwFlags, lpiRet);

  if (!lpszStr || !lpiRet)
  {
    WARN("Invalid parameter would crash under Win32!\n");
    return FALSE;
  }
  if (dwFlags > STIF_SUPPORT_HEX)
  {
    WARN("Unknown flags (%08lX)!\n", dwFlags & ~STIF_SUPPORT_HEX);
  }

  /* Skip leading space, '+', '-' */
  while (isspaceW(*lpszStr))
    lpszStr = CharNextW(lpszStr);

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

  iLen = lpszStr ? strlen(lpszStr) + 1 : 1;
  lpszRet = (LPSTR)LocalAlloc(LMEM_FIXED, iLen);

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

  iLen = (lpszStr ? strlenW(lpszStr) + 1 : 1) * sizeof(WCHAR);
  lpszRet = (LPWSTR)LocalAlloc(LMEM_FIXED, iLen);

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
static int WINAPI SHLWAPI_StrSpnHelperA(LPCSTR lpszStr, LPCSTR lpszMatch,
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
 * SHLWAPI_StrSpnHelperW
 *
 * Internal implementation of StrSpnW/StrCSpnW/StrCSpnIW
 */
static int WINAPI SHLWAPI_StrSpnHelperW(LPCWSTR lpszStr, LPCWSTR lpszMatch,
                                      LPWSTR (WINAPI *pStrChrFn)(LPCWSTR,WCHAR),
                                      BOOL bInvert)
{
  LPCWSTR lpszRead = lpszStr;
  if (lpszStr && *lpszStr && lpszMatch)
  {
    while (*lpszRead)
    {
      LPCWSTR lpszTest = pStrChrFn(lpszMatch, *lpszRead);

      if (!bInvert && !lpszTest)
        break;
      if (bInvert && lpszTest)
        break;
      lpszRead = CharNextW(lpszRead);
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
  TRACE("(%s,%s)\n",debugstr_w(lpszStr), debugstr_w(lpszMatch));

  return SHLWAPI_StrSpnHelperW(lpszStr, lpszMatch, StrChrW, FALSE);
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
  TRACE("(%s,%s)\n",debugstr_w(lpszStr), debugstr_w(lpszMatch));

  return SHLWAPI_StrSpnHelperW(lpszStr, lpszMatch, StrChrW, TRUE);
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
  TRACE("(%s,%s)\n",debugstr_w(lpszStr), debugstr_w(lpszMatch));

  return SHLWAPI_StrSpnHelperW(lpszStr, lpszMatch, StrChrIW, TRUE);
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
  TRACE("(%s,%s)\n",debugstr_w(lpszStr), debugstr_w(lpszMatch));

  if (lpszStr && lpszMatch && *lpszMatch)
  {
    while (*lpszStr)
    {
      if (StrChrW(lpszMatch, *lpszStr))
        return (LPWSTR)lpszStr;
      lpszStr = CharNextW(lpszStr);
    }
  }
  return NULL;
}

/*************************************************************************
 * SHLWAPI_StrRChrHelperA
 *
 * Internal implementation of StrRChrA/StrRChrIA.
 */
static LPSTR WINAPI SHLWAPI_StrRChrHelperA(LPCSTR lpszStr,
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

/*************************************************************************
 * SHLWAPI_StrRChrHelperW
 *
 * Internal implementation of StrRChrW/StrRChrIW.
 */
static LPWSTR WINAPI SHLWAPI_StrRChrHelperW(LPCWSTR lpszStr,
                                         LPCWSTR lpszEnd, WCHAR ch,
                                         BOOL (WINAPI *pChrCmpFn)(WCHAR,WCHAR))
{
  LPCWSTR lpszRet = NULL;

  if (lpszStr)
  {
    if (!lpszEnd)
      lpszEnd = lpszStr + strlenW(lpszStr);

    while (*lpszStr && lpszStr <= lpszEnd)
    {
      if (!pChrCmpFn(ch, *lpszStr))
        lpszRet = lpszStr;
      lpszStr = CharNextW(lpszStr);
    }
  }
  return (LPWSTR)lpszRet;
}

/**************************************************************************
 * StrRChrA	[SHLWAPI.@]
 *
 * Find the last occurence of a character in string.
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
LPWSTR WINAPI StrRChrW(LPCWSTR lpszStr, LPCWSTR lpszEnd, WORD ch)
{
  TRACE("(%s,%s,%x)\n", debugstr_w(lpszStr), debugstr_w(lpszEnd), ch);

  return SHLWAPI_StrRChrHelperW(lpszStr, lpszEnd, ch, SHLWAPI_ChrCmpW);
}

/**************************************************************************
 * StrRChrIA	[SHLWAPI.@]
 *
 * Find the last occurence of a character in string, ignoring case.
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
LPWSTR WINAPI StrRChrIW(LPCWSTR lpszStr, LPCWSTR lpszEnd, WORD ch)
{
  TRACE("(%s,%s,%x)\n", debugstr_w(lpszStr), debugstr_w(lpszEnd), ch);

  return SHLWAPI_StrRChrHelperW(lpszStr, lpszEnd, ch, ChrCmpIW);
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
	TRACE("dest=%p len=0x%x strret=%p pidl=%p stub\n",dest,len,src,pidl);

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
	    lstrcpynA((LPSTR)dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSET:
	    lstrcpynA((LPSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	    break;

	  default:
	    FIXME("unknown type!\n");
	    return FALSE;
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
	TRACE("dest=%p len=0x%x strret=%p pidl=%p stub\n",dest,len,src,pidl);

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
	    lstrcpynW((LPWSTR)dest, src->u.pOleStr, len);
	    CoTaskMemFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTR:
              if (!MultiByteToWideChar( CP_ACP, 0, src->u.cStr, -1, dest, len ) && len)
                  dest[len-1] = 0;
	    break;

	  case STRRET_OFFSET:
	    if (pidl)
	    {
              if (!MultiByteToWideChar( CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->u.uOffset, -1,
                                        dest, len ) && len)
                  dest[len-1] = 0;
	    }
	    break;

	  default:
	    FIXME("unknown type!\n");
	    return FALSE;
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
    hRet = SHStrDupW(lpStrRet->u.pOleStr, ppszName);
    CoTaskMemFree(lpStrRet->u.pOleStr);
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
  char szBuff[256], *szOut = szBuff + sizeof(szBuff) - 1;
  LONGLONG ulKB = (llBytes + 1023) >> 10;

  TRACE("(%lld,%p,%d)\n", llBytes, lpszDest, cchMax);

  *szOut-- = '\0';
  *szOut-- = 'B';
  *szOut-- = 'K';
  *szOut-- = ' ';

  do
  {
    LONGLONG ulNextDigit = ulKB % 10;
    *szOut-- = '0' + ulNextDigit;
    ulKB = (ulKB - ulNextDigit) / 10;
  } while (ulKB > 0);

  strncpy(lpszDest, szOut + 1, cchMax);
  return lpszDest;
}

/*************************************************************************
 * StrFormatKBSizeW	[SHLWAPI.@]
 *
 * See StrFormatKBSizeA.
 */
LPWSTR WINAPI StrFormatKBSizeW(LONGLONG llBytes, LPWSTR lpszDest, UINT cchMax)
{
  WCHAR szBuff[256], *szOut = szBuff + sizeof(szBuff)/sizeof(WCHAR) - 1;
  LONGLONG ulKB = (llBytes + 1023) >> 10;

  TRACE("(%lld,%p,%d)\n", llBytes, lpszDest, cchMax);

  *szOut-- = '\0';
  *szOut-- = 'B';
  *szOut-- = 'K';
  *szOut-- = ' ';

  do
  {
    LONGLONG ulNextDigit = ulKB % 10;
    *szOut-- = '0' + ulNextDigit;
    ulKB = (ulKB - ulNextDigit) / 10;
  } while (ulKB > 0);

  strncpyW(lpszDest, szOut + 1, cchMax);
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
    while (*lpszRead && StrChrW(lpszTrim, *lpszRead))
      lpszRead = CharNextW(lpszRead); /* Skip leading matches */

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
        lpszRead = CharPrevW(lpszStr, lpszRead); /* Skip trailing matches */

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
static HRESULT WINAPI _SHStrDupAA(LPCSTR src, LPSTR * dest)
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
    len = MultiByteToWideChar(0, 0, lpszStr, -1, 0, 0) * sizeof(WCHAR);
    *lppszDest = CoTaskMemAlloc(len);
  }
  else
    *lppszDest = NULL;

  if (*lppszDest)
  {
    MultiByteToWideChar(0, 0, lpszStr, -1, *lppszDest, len);
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
static HRESULT WINAPI _SHStrDupAW(LPCWSTR src, LPSTR * dest)
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
inline static LPWSTR SHLWAPI_WriteReverseNum(LPWSTR lpszOut, DWORD dwNum)
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
inline static int SHLWAPI_FormatSignificant(LPWSTR lpszNum, int dwDigits)
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
static int WINAPI SHLWAPI_WriteTimeClass(LPWSTR lpszOut, DWORD dwValue,
                                         LPCWSTR lpszClass, int iDigits)
{
  WCHAR szBuff[64], *szOut = szBuff + 32;

  szOut = SHLWAPI_WriteReverseNum(szOut, dwValue);
  iDigits = SHLWAPI_FormatSignificant(szOut + 1, iDigits);
  *szOut = ' ';
  strcpyW(szBuff + 32, lpszClass);
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

  TRACE("(%p,%d,%ld,%d)\n", lpszStr, cchMax, dwMS, iDigits);

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
  static const WCHAR szHr[] = {' ','h','r','\0'};
  static const WCHAR szMin[] = {' ','m','i','n','\0'};
  static const WCHAR szSec[] = {' ','s','e','c','\0'};
  INT iRet = 0;

  TRACE("(%p,%d,%ld,%d)\n", lpszStr, cchMax, dwMS, iDigits);

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
      iDigits = SHLWAPI_WriteTimeClass(szCopy, dwHours, szHr, iDigits);

    if (dwMinutes && iDigits)
      iDigits = SHLWAPI_WriteTimeClass(szCopy, dwMinutes, szMin, iDigits);

    if (iDigits) /* Always write seconds if we have significant digits */
      SHLWAPI_WriteTimeClass(szCopy, dwMS, szSec, iDigits);

    strncpyW(lpszStr, szCopy, cchMax);
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
  DWORD dwFlags = LOCALE_USE_CP_ACP;
  int iRet;

  TRACE("(%d,%s,%s,%d)\n", bCase,
        debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);

  /* FIXME: These flags are undocumented and unknown by our CompareString.
   *        We need defines for them.
   */
  dwFlags |= bCase ? 0x10000000 : 0x10000001;

  iRet = CompareStringA(GetThreadLocale(),
                        dwFlags, lpszStr, iLen, lpszComp, iLen);

  if (!iRet)
    iRet = CompareStringA(2048, dwFlags, lpszStr, iLen, lpszComp, iLen);

  return iRet == 2 ? TRUE : FALSE;
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
  int iRet;

  TRACE("(%d,%s,%s,%d)\n", bCase,
        debugstr_w(lpszStr),debugstr_w(lpszComp), iLen);

  /* FIXME: These flags are undocumented and unknown by our CompareString.
   *        We need defines for them.
   */
  dwFlags = bCase ? 0x10000000 : 0x10000001;

  iRet = CompareStringW(GetThreadLocale(),
                        dwFlags, lpszStr, iLen, lpszComp, iLen);

  if (!iRet)
    iRet = CompareStringW(2048, dwFlags, lpszStr, iLen, lpszComp, iLen);

  return iRet == 2 ? TRUE : FALSE;
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
 *  Success: A pointer to the last character written.
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
        iDiff = SHLWAPI_ChrCmpHelperA(*lpszStr,*lpszComp,NORM_IGNORECASE);
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
  LPCSTR   lpszFormat;
  CHAR     wPrefix;
} SHLWAPI_BYTEFORMATS;

/*************************************************************************
 * StrFormatByteSize64A	[SHLWAPI.@]
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
LPSTR WINAPI StrFormatByteSize64A(LONGLONG llBytes, LPSTR lpszDest, UINT cchMax)
{
  static const char szBytes[] = "%ld bytes";
  static const char sz3_0[] = "%3.0f";
  static const char sz3_1[] = "%3.1f";
  static const char sz3_2[] = "%3.2f";

#define KB ((ULONGLONG)1024)
#define MB (KB*KB)
#define GB (KB*KB*KB)
#define TB (KB*KB*KB*KB)
#define PB (KB*KB*KB*KB*KB)

  static const SHLWAPI_BYTEFORMATS bfFormats[] =
  {
    { 10*KB, 10.24, 100.0, sz3_2, 'K' }, /* 10 KB */
    { 100*KB, 102.4, 10.0, sz3_1, 'K' }, /* 100 KB */
    { 1000*KB, 1024.0, 1.0, sz3_0, 'K' }, /* 1000 KB */
    { 10*MB, 10485.76, 100.0, sz3_2, 'M' }, /* 10 MB */
    { 100*MB, 104857.6, 10.0, sz3_1, 'M' }, /* 100 MB */
    { 1000*MB, 1048576.0, 1.0, sz3_0, 'M' }, /* 1000 MB */
    { 10*GB, 10737418.24, 100.0, sz3_2, 'G' }, /* 10 GB */
    { 100*GB, 107374182.4, 10.0, sz3_1, 'G' }, /* 100 GB */
    { 1000*GB, 1073741824.0, 1.0, sz3_0, 'G' }, /* 1000 GB */
    { 10*TB, 10485.76, 100.0, sz3_2, 'T' }, /* 10 TB */
    { 100*TB, 104857.6, 10.0, sz3_1, 'T' }, /* 100 TB */
    { 1000*TB, 1048576.0, 1.0, sz3_0, 'T' }, /* 1000 TB */
    { 10*PB, 10737418.24, 100.00, sz3_2, 'P' }, /* 10 PB */
    { 100*PB, 107374182.4, 10.00, sz3_1, 'P' }, /* 100 PB */
    { 1000*PB, 1073741824.0, 1.00, sz3_0, 'P' }, /* 1000 PB */
    { 0, 10995116277.76, 100.00, sz3_2, 'E' } /* EB's, catch all */
  };
  char szBuff[32];
  char szAdd[4];
  double dBytes;
  UINT i = 0;

  TRACE("(%lld,%p,%d)\n", llBytes, lpszDest, cchMax);

  if (!lpszDest || !cchMax)
    return lpszDest;

  if (llBytes < 1024)  /* 1K */
  {
    snprintf (lpszDest, cchMax, szBytes, (long)llBytes);
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
    dBytes = (double)(llBytes >> 20) + 0.001; /* Scale down by I MB */
  else
    dBytes = (double)llBytes + 0.00001;

  dBytes = floor(dBytes / bfFormats[i].dDivisor) / bfFormats[i].dNormaliser;

  sprintf(szBuff, bfFormats[i].lpszFormat, dBytes);
  szAdd[0] = ' ';
  szAdd[1] = bfFormats[i].wPrefix;
  szAdd[2] = 'B';
  szAdd[3] = '\0';
  strcat(szBuff, szAdd);
  strncpy(lpszDest, szBuff, cchMax);
  return lpszDest;
}

/*************************************************************************
 * StrFormatByteSizeW	[SHLWAPI.@]
 *
 * See StrFormatByteSize64A.
 */
LPWSTR WINAPI StrFormatByteSizeW(LONGLONG llBytes, LPWSTR lpszDest,
                                 UINT cchMax)
{
  char szBuff[32];

  StrFormatByteSize64A(llBytes, szBuff, sizeof(szBuff));

  if (lpszDest)
    MultiByteToWideChar(CP_ACP, 0, szBuff, -1, lpszDest, cchMax);
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
  TRACE("(%ld,%p,%d)\n", dwBytes, lpszDest, cchMax);

  return StrFormatByteSize64A(dwBytes, lpszDest, cchMax);
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
    if (lpszTmp && *lpszTmp)
    {
      if (*lpszTmp != '&')
        ch =  *lpszTmp;

      while (lpszIter && *lpszIter)
      {
        lpszTmp = CharNextA(lpszIter);
        *lpszIter = *lpszTmp;
        lpszIter = lpszTmp;
      }
    }
  }

  return ch;
}
