/*
 * String manipulation functions
 *
 * Copyright 1998 Eric Kohl
 *           1998 Juergen Schmied <j.schmied@metronet.de>
 *           2000 Eric Kohl for CodeWeavers
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
 *
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h> /* atoi */

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/unicode.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

/*************************************************************************
 * COMCTL32_ChrCmpHelperA
 *
 * Internal helper for ChrCmpA/COMCTL32_ChrCmpIA.
 *
 * NOTES
 *  Both this function and its Unicode counterpart are very inneficient. To
 *  fix this, CompareString must be completely implemented and optimised
 *  first. Then the core character test can be taken out of that function and
 *  placed here, so that it need never be called at all. Until then, do not
 *  attempt to optimise this code unless you are willing to test that it
 *  still performs correctly.
 */
static BOOL COMCTL32_ChrCmpHelperA(WORD ch1, WORD ch2, DWORD dwFlags)
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
 * COMCTL32_ChrCmpHelperW
 *
 * Internal helper for COMCTL32_ChrCmpW/ChrCmpIW.
 */
static BOOL COMCTL32_ChrCmpHelperW(WCHAR ch1, WCHAR ch2, DWORD dwFlags)
{
  WCHAR str1[2], str2[2];

  str1[0] = ch1;
  str1[1] = '\0';
  str2[0] = ch2;
  str2[1] = '\0';
  return CompareStringW(GetThreadLocale(), dwFlags, str1, 2, str2, 2) - 2;
}

/*************************************************************************
 * COMCTL32_ChrCmpA (internal)
 *
 * Internal helper function.
 */
static BOOL COMCTL32_ChrCmpA(WORD ch1, WORD ch2)
{
  return COMCTL32_ChrCmpHelperA(ch1, ch2, 0);
}

/*************************************************************************
 * COMCTL32_ChrCmpIA	(internal)
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
static BOOL COMCTL32_ChrCmpIA(WORD ch1, WORD ch2)
{
  TRACE("(%d,%d)\n", ch1, ch2);

  return COMCTL32_ChrCmpHelperA(ch1, ch2, NORM_IGNORECASE);
}

/*************************************************************************
 * COMCTL32_ChrCmpW
 *
 * Internal helper function.
 */
static BOOL COMCTL32_ChrCmpW(WCHAR ch1, WCHAR ch2)
{
  return COMCTL32_ChrCmpHelperW(ch1, ch2, 0);
}

/**************************************************************************
 * StrChrA [COMCTL32.350]
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
      if (!COMCTL32_ChrCmpA(*lpszStr, ch))
        return (LPSTR)lpszStr;
      lpszStr = CharNextA(lpszStr);
    }
  }
  return NULL;
}

/*************************************************************************
 * COMCTL32_StrStrHelperA
 *
 * Internal implementation of StrStrA/StrStrIA
 */
static LPSTR COMCTL32_StrStrHelperA(LPCSTR lpszStr, LPCSTR lpszSearch,
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
 * COMCTL32_StrStrHelperW
 *
 * Internal implementation of StrStrW/StrStrIW
 */
static LPWSTR COMCTL32_StrStrHelperW(LPCWSTR lpszStr, LPCWSTR lpszSearch,
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

/**************************************************************************
 * StrStrIA [COMCTL32.355]
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

  return COMCTL32_StrStrHelperA(lpszStr, lpszSearch, strncasecmp);
}

/**************************************************************************
 * StrToIntA [COMCTL32.357]
 *
 * Read a signed integer from a string.
 *
 * PARAMS
 *  lpszStr [I] String to read integer from
 *
 * RETURNS
 *   The signed integer value represented by the string, or 0 if no integer is
 *   present.
 */
INT WINAPI StrToIntA (LPSTR lpszStr)
{
    return atoi(lpszStr);
}

/**************************************************************************
 * StrStrIW [COMCTL32.363]
 *
 * See StrStrIA.
 */
LPWSTR WINAPI StrStrIW(LPCWSTR lpszStr, LPCWSTR lpszSearch)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSearch));

  return COMCTL32_StrStrHelperW(lpszStr, lpszSearch, (int (*)(LPCWSTR,LPCWSTR,int)) wcsnicmp);
}

/**************************************************************************
 * StrToIntW [COMCTL32.365]
 *
 * See StrToIntA.
 */
INT WINAPI StrToIntW (LPWSTR lpString)
{
    return atoiW(lpString);
}

/*************************************************************************
 * COMCTL32_StrSpnHelperA (internal)
 *
 * Internal implementation of StrSpnA/StrCSpnA/StrCSpnIA
 */
static int COMCTL32_StrSpnHelperA(LPCSTR lpszStr, LPCSTR lpszMatch,
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

/**************************************************************************
 * StrCSpnA [COMCTL32.356]
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

  return COMCTL32_StrSpnHelperA(lpszStr, lpszMatch, StrChrA, TRUE);
}

/**************************************************************************
 * StrChrW [COMCTL32.358]
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

/**************************************************************************
 * StrCmpNA [COMCTL32.352]
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
  INT iRet;

  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);

  iRet = CompareStringA(GetThreadLocale(), 0, lpszStr, iLen, lpszComp, iLen);
  return iRet == CSTR_LESS_THAN ? -1 : iRet == CSTR_GREATER_THAN ? 1 : 0;
}

/**************************************************************************
 * StrCmpNIA [COMCTL32.353]
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
 */
int WINAPI StrCmpNIA(LPCSTR lpszStr, LPCSTR lpszComp, int iLen)
{
  INT iRet;

  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);

  iRet = CompareStringA(GetThreadLocale(), NORM_IGNORECASE, lpszStr, iLen, lpszComp, iLen);
  return iRet == CSTR_LESS_THAN ? -1 : iRet == CSTR_GREATER_THAN ? 1 : 0;
}

/*************************************************************************
 * StrCmpNIW	[COMCTL32.361]
 *
 * See StrCmpNIA.
 */
INT WINAPI StrCmpNIW(LPCWSTR lpszStr, LPCWSTR lpszComp, int iLen)
{
  INT iRet;

  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszComp), iLen);

  iRet = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, lpszStr, iLen, lpszComp, iLen);
  return iRet == CSTR_LESS_THAN ? -1 : iRet == CSTR_GREATER_THAN ? 1 : 0;
}

/**************************************************************************
 * StrCmpNW [COMCTL32.360]
 *
 * See StrCmpNA.
 */
INT WINAPI StrCmpNW(LPCWSTR lpszStr, LPCWSTR lpszComp, INT iLen)
{
  INT iRet;

  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszComp), iLen);

  iRet = CompareStringW(GetThreadLocale(), 0, lpszStr, iLen, lpszComp, iLen);
  return iRet == CSTR_LESS_THAN ? -1 : iRet == CSTR_GREATER_THAN ? 1 : 0;
}

/**************************************************************************
 * StrRChrA [COMCTL32.351]
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
  LPCSTR lpszRet = NULL;

  TRACE("(%s,%s,%x)\n", debugstr_a(lpszStr), debugstr_a(lpszEnd), ch);

  if (lpszStr)
  {
    WORD ch2;

    if (!lpszEnd)
      lpszEnd = lpszStr + lstrlenA(lpszStr);

    while (*lpszStr && lpszStr <= lpszEnd)
    {
      ch2 = IsDBCSLeadByte(*lpszStr)? *lpszStr << 8 | lpszStr[1] : *lpszStr;

      if (!COMCTL32_ChrCmpA(ch, ch2))
        lpszRet = lpszStr;
      lpszStr = CharNextA(lpszStr);
    }
  }
  return (LPSTR)lpszRet;
}


/**************************************************************************
 * StrRChrW [COMCTL32.359]
 *
 * See StrRChrA.
 */
LPWSTR WINAPI StrRChrW(LPCWSTR lpszStr, LPCWSTR lpszEnd, WORD ch)
{
  LPCWSTR lpszRet = NULL;

  TRACE("(%s,%s,%x)\n", debugstr_w(lpszStr), debugstr_w(lpszEnd), ch);

  if (lpszStr)
  {
    if (!lpszEnd)
      lpszEnd = lpszStr + strlenW(lpszStr);

    while (*lpszStr && lpszStr <= lpszEnd)
    {
      if (!COMCTL32_ChrCmpW(ch, *lpszStr))
        lpszRet = lpszStr;
      lpszStr = CharNextW(lpszStr);
    }
  }
  return (LPWSTR)lpszRet;
}

/**************************************************************************
 * StrStrA [COMCTL32.354]
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

  return COMCTL32_StrStrHelperA(lpszStr, lpszSearch, strncmp);
}

/**************************************************************************
 * StrStrW [COMCTL32.362]
 *
 * See StrStrA.
 */
LPWSTR WINAPI StrStrW(LPCWSTR lpszStr, LPCWSTR lpszSearch)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSearch));

  return COMCTL32_StrStrHelperW(lpszStr, lpszSearch, (int (*)(LPCWSTR,LPCWSTR,int)) wcsncmp);
}

/*************************************************************************
 * StrChrIA	[COMCTL32.366]
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
      if (!COMCTL32_ChrCmpIA(*lpszStr, ch))
        return (LPSTR)lpszStr;
      lpszStr = CharNextA(lpszStr);
    }
  }
  return NULL;
}

/*************************************************************************
 * StrChrIW	[COMCTL32.367]
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
 * StrRStrIA	[COMCTL32.372]
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
    if (!COMCTL32_ChrCmpIA(ch1, ch2))
    {
      if (!StrCmpNIA(lpszStr, lpszSearch, iLen))
        lpszRet = (LPSTR)lpszStr;
    }
    lpszStr = CharNextA(lpszStr);
  }
  return lpszRet;
}

/*************************************************************************
 * StrRStrIW	[COMCTL32.373]
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
    if (!COMCTL32_ChrCmpIA(*lpszSearch, *lpszStr))
    {
      if (!StrCmpNIW(lpszStr, lpszSearch, iLen))
        lpszRet = (LPWSTR)lpszStr;
    }
    lpszStr = CharNextW(lpszStr);
  }
  return lpszRet;
}

/*************************************************************************
 * COMCTL32_StrSpnHelperW
 *
 * Internal implementation of StrSpnW/StrCSpnW/StrCSpnIW
 */
static int COMCTL32_StrSpnHelperW(LPCWSTR lpszStr, LPCWSTR lpszMatch,
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
 * StrCSpnIA	[COMCTL32.374]
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

  return COMCTL32_StrSpnHelperA(lpszStr, lpszMatch, StrChrIA, TRUE);
}

/*************************************************************************
 * StrCSpnIW	[COMCTL32.375]
 *
 * See StrCSpnIA.
 */
int WINAPI StrCSpnIW(LPCWSTR lpszStr, LPCWSTR lpszMatch)
{
  TRACE("(%s,%s)\n",debugstr_w(lpszStr), debugstr_w(lpszMatch));

  return COMCTL32_StrSpnHelperW(lpszStr, lpszMatch, StrChrIW, TRUE);
}

/**************************************************************************
 * StrRChrIA	[COMCTL32.368]
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
  LPCSTR lpszRet = NULL;

  TRACE("(%s,%s,%x)\n", debugstr_a(lpszStr), debugstr_a(lpszEnd), ch);

  if (lpszStr)
  {
    WORD ch2;

    if (!lpszEnd)
      lpszEnd = lpszStr + lstrlenA(lpszStr);

    while (*lpszStr && lpszStr <= lpszEnd)
    {
      ch2 = IsDBCSLeadByte(*lpszStr)? *lpszStr << 8 | lpszStr[1] : *lpszStr;

      if (ch == ch2)
        lpszRet = lpszStr;
      lpszStr = CharNextA(lpszStr);
    }
  }
  return (LPSTR)lpszRet;
}

/**************************************************************************
 * StrRChrIW	[COMCTL32.369]
 *
 * See StrRChrIA.
 */
LPWSTR WINAPI StrRChrIW(LPCWSTR lpszStr, LPCWSTR lpszEnd, WORD ch)
{
  LPCWSTR lpszRet = NULL;

  TRACE("(%s,%s,%x)\n", debugstr_w(lpszStr), debugstr_w(lpszEnd), ch);

  if (lpszStr)
  {
    if (!lpszEnd)
      lpszEnd = lpszStr + strlenW(lpszStr);

    while (*lpszStr && lpszStr <= lpszEnd)
    {
      if (ch == *lpszStr)
        lpszRet = lpszStr;
      lpszStr = CharNextW(lpszStr);
    }
  }
  return (LPWSTR)lpszRet;
}

/*************************************************************************
 * StrSpnW	[COMCTL32.364]
 *
 * See StrSpnA.
 */
int WINAPI StrSpnW(LPCWSTR lpszStr, LPCWSTR lpszMatch)
{
  TRACE("(%s,%s)\n",debugstr_w(lpszStr), debugstr_w(lpszMatch));

  return COMCTL32_StrSpnHelperW(lpszStr, lpszMatch, StrChrW, FALSE);
}

/*************************************************************************
 * StrCSpnW	[COMCTL32.@]
 *
 * See StrCSpnA.
 */
int WINAPI StrCSpnW(LPCWSTR lpszStr, LPCWSTR lpszMatch)
{
  TRACE("(%s,%s)\n",debugstr_w(lpszStr), debugstr_w(lpszMatch));

  return COMCTL32_StrSpnHelperW(lpszStr, lpszMatch, StrChrW, TRUE);
}

/*************************************************************************
 * IntlStrEqWorkerA	[COMCTL32.376]
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
BOOL WINAPI IntlStrEqWorkerA(BOOL bCase, LPCSTR lpszStr, LPCSTR lpszComp,
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
 * IntlStrEqWorkerW	[COMCTL32.377]
 *
 * See IntlStrEqWorkerA.
 */
BOOL WINAPI IntlStrEqWorkerW(BOOL bCase, LPCWSTR lpszStr, LPCWSTR lpszComp,
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
