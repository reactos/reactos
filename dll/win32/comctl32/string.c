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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h> /* atoi */

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"

#include "comctl32.h"


#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

/*************************************************************************
 * COMCTL32_ChrCmpHelperA
 *
 * Internal helper for ChrCmpA/COMCTL32_ChrCmpIA.
 *
 * NOTES
 *  Both this function and its Unicode counterpart are very inefficient. To
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

  return CompareStringA(GetThreadLocale(), dwFlags, str1, -1, str2, -1) - CSTR_EQUAL;
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
 * COMCTL32_ChrCmpIW
 *
 * Internal helper function.
 */
static inline BOOL COMCTL32_ChrCmpIW(WCHAR ch1, WCHAR ch2)
{
  return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, &ch1, 1, &ch2, 1) - CSTR_EQUAL;
}

/**************************************************************************
 * Str_GetPtrA [COMCTL32.233]
 *
 * Copies a string into a destination buffer.
 *
 * PARAMS
 *     lpSrc   [I] Source string
 *     lpDest  [O] Destination buffer
 *     nMaxLen [I] Size of buffer in characters
 *
 * RETURNS
 *     The number of characters copied.
 */
INT WINAPI Str_GetPtrA (LPCSTR lpSrc, LPSTR lpDest, INT nMaxLen)
{
    INT len;

    TRACE("(%p %p %d)\n", lpSrc, lpDest, nMaxLen);

    if ((!lpDest || nMaxLen == 0) && lpSrc)
        return (strlen(lpSrc) + 1);

    if (nMaxLen == 0)
        return 0;

    if (lpSrc == NULL) {
        lpDest[0] = '\0';
        return 0;
    }

    len = strlen(lpSrc) + 1;
    if (len >= nMaxLen)
        len = nMaxLen;

    RtlMoveMemory (lpDest, lpSrc, len - 1);
    lpDest[len - 1] = '\0';

    return len;
}

/**************************************************************************
 * Str_SetPtrA [COMCTL32.234]
 *
 * Makes a copy of a string, allocating memory if necessary.
 *
 * PARAMS
 *     lppDest [O] Pointer to destination string
 *     lpSrc   [I] Source string
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     Set lpSrc to NULL to free the memory allocated by a previous call
 *     to this function.
 */
BOOL WINAPI Str_SetPtrA (LPSTR *lppDest, LPCSTR lpSrc)
{
    TRACE("(%p %p)\n", lppDest, lpSrc);

    if (lpSrc) {
        LPSTR ptr = ReAlloc (*lppDest, strlen (lpSrc) + 1);
        if (!ptr)
            return FALSE;
        strcpy (ptr, lpSrc);
        *lppDest = ptr;
    }
    else {
        Free (*lppDest);
        *lppDest = NULL;
    }

    return TRUE;
}

/**************************************************************************
 * Str_GetPtrW [COMCTL32.235]
 *
 * See Str_GetPtrA.
 */
INT WINAPI Str_GetPtrW (LPCWSTR lpSrc, LPWSTR lpDest, INT nMaxLen)
{
    INT len;

    TRACE("(%p %p %d)\n", lpSrc, lpDest, nMaxLen);

    if (!lpDest && lpSrc)
        return lstrlenW (lpSrc);

    if (nMaxLen == 0)
        return 0;

    if (lpSrc == NULL) {
        lpDest[0] = '\0';
        return 0;
    }

    len = lstrlenW (lpSrc);
    if (len >= nMaxLen)
        len = nMaxLen - 1;

    RtlMoveMemory (lpDest, lpSrc, len*sizeof(WCHAR));
    lpDest[len] = '\0';

    return len;
}

/**************************************************************************
 * Str_SetPtrW [COMCTL32.236]
 *
 * See Str_SetPtrA.
 */
BOOL WINAPI Str_SetPtrW (LPWSTR *lppDest, LPCWSTR lpSrc)
{
    TRACE("(%p %s)\n", lppDest, debugstr_w(lpSrc));

    if (lpSrc) {
        INT len = lstrlenW (lpSrc) + 1;
        LPWSTR ptr = ReAlloc (*lppDest, len * sizeof(WCHAR));
        if (!ptr)
            return FALSE;
        lstrcpyW (ptr, lpSrc);
        *lppDest = ptr;
    }
    else {
        Free (*lppDest);
        *lppDest = NULL;
    }

    return TRUE;
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

/**************************************************************************
 * StrCmpNIA [COMCTL32.353]
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
INT WINAPI StrCmpNIA(LPCSTR lpszStr, LPCSTR lpszComp, INT iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);
  return CompareStringA(GetThreadLocale(), NORM_IGNORECASE, lpszStr, iLen, lpszComp, iLen) - CSTR_EQUAL;
}

/*************************************************************************
 * StrCmpNIW	[COMCTL32.361]
 *
 * See StrCmpNIA.
 */
INT WINAPI StrCmpNIW(LPCWSTR lpszStr, LPCWSTR lpszComp, INT iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszComp), iLen);
  return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, lpszStr, iLen, lpszComp, iLen) - CSTR_EQUAL;
}

/*************************************************************************
 * COMCTL32_StrStrHelperA
 *
 * Internal implementation of StrStrA/StrStrIA
 */
static LPSTR COMCTL32_StrStrHelperA(LPCSTR lpszStr, LPCSTR lpszSearch,
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

  return COMCTL32_StrStrHelperA(lpszStr, lpszSearch, StrCmpNIA);
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
INT WINAPI StrToIntA (LPCSTR lpszStr)
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
  int iLen;
  LPCWSTR end;

  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSearch));

  if (!lpszStr || !lpszSearch || !*lpszSearch)
    return NULL;

  iLen = lstrlenW(lpszSearch);
  end = lpszStr + lstrlenW(lpszStr);

  while (lpszStr + iLen <= end)
  {
    if (!StrCmpNIW(lpszStr, lpszSearch, iLen))
      return (LPWSTR)lpszStr;
    lpszStr++;
  }
  return NULL;
}

/**************************************************************************
 * StrToIntW [COMCTL32.365]
 *
 * See StrToIntA.
 */
INT WINAPI StrToIntW (LPCWSTR lpString)
{
    return wcstol(lpString, NULL, 10);
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
    lpszRet = wcschr(lpszStr, ch);
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

/**************************************************************************
 * StrCmpNW [COMCTL32.360]
 *
 * See StrCmpNA.
 */
INT WINAPI StrCmpNW(LPCWSTR lpszStr, LPCWSTR lpszComp, INT iLen)
{
  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszComp), iLen);
  return CompareStringW(GetThreadLocale(), 0, lpszStr, iLen, lpszComp, iLen) - CSTR_EQUAL;
}

/**************************************************************************
 * StrRChrA [COMCTL32.351]
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
LPWSTR WINAPI StrRChrW(LPCWSTR str, LPCWSTR end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + lstrlenW(str);
    while (str < end)
    {
        if (*str == ch) ret = (WCHAR *)str;
        str++;
    }
    return ret;
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

  return COMCTL32_StrStrHelperA(lpszStr, lpszSearch, StrCmpNA);
}

/**************************************************************************
 * StrStrW [COMCTL32.362]
 *
 * See StrStrA.
 */
LPWSTR WINAPI StrStrW(LPCWSTR lpszStr, LPCWSTR lpszSearch)
{
    if (!lpszStr || !lpszSearch) return NULL;
    return wcsstr( lpszStr, lpszSearch );
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
    ch = towupper(ch);
    while (*lpszStr)
    {
      if (towupper(*lpszStr) == ch)
        return (LPWSTR)lpszStr;
      lpszStr++;
    }
    lpszStr = NULL;
  }
  return (LPWSTR)lpszStr;
}

/*************************************************************************
 * StrRStrIA	[COMCTL32.372]
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

  iLen = lstrlenW(lpszSearch);

  if (!lpszEnd)
    lpszEnd = lpszStr + lstrlenW(lpszStr);
  else /* reproduce the broken behaviour on Windows */
    lpszEnd += min(iLen - 1, lstrlenW(lpszEnd));


  while (lpszStr + iLen <= lpszEnd && *lpszStr)
  {
    if (!COMCTL32_ChrCmpIW(*lpszSearch, *lpszStr))
    {
      if (!StrCmpNIW(lpszStr, lpszSearch, iLen))
        lpszRet = (LPWSTR)lpszStr;
    }
    lpszStr++;
  }
  return lpszRet;
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

/**************************************************************************
 * StrRChrIA	[COMCTL32.368]
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
LPWSTR WINAPI StrRChrIW(LPCWSTR str, LPCWSTR end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + lstrlenW(str);
    while (str < end)
    {
        if (!COMCTL32_ChrCmpIW(*str, ch)) ret = (WCHAR *)str;
        str++;
    }
    return ret;
}

/*************************************************************************
 * StrCSpnW	[COMCTL32.364]
 *
 * See StrCSpnA.
 */
int WINAPI StrCSpnW(LPCWSTR lpszStr, LPCWSTR lpszMatch)
{
    if (!lpszStr || !lpszMatch) return 0;
    return wcscspn( lpszStr, lpszMatch );
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
  DWORD dwFlags;
  int iRet;

  TRACE("(%d,%s,%s,%d)\n", bCase,
        debugstr_a(lpszStr), debugstr_a(lpszComp), iLen);

  /* FIXME: This flag is undocumented and unknown by our CompareString.
   */
  dwFlags = LOCALE_RETURN_GENITIVE_NAMES;
  if (!bCase) dwFlags |= NORM_IGNORECASE;

  iRet = CompareStringA(GetThreadLocale(),
                        dwFlags, lpszStr, iLen, lpszComp, iLen);

  if (!iRet)
    iRet = CompareStringA(2048, dwFlags, lpszStr, iLen, lpszComp, iLen);

  return iRet == CSTR_EQUAL;
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

  /* FIXME: This flag is undocumented and unknown by our CompareString.
   */
  dwFlags = LOCALE_RETURN_GENITIVE_NAMES;
  if (!bCase) dwFlags |= NORM_IGNORECASE;

  iRet = CompareStringW(GetThreadLocale(),
                        dwFlags, lpszStr, iLen, lpszComp, iLen);

  if (!iRet)
    iRet = CompareStringW(2048, dwFlags, lpszStr, iLen, lpszComp, iLen);

  return iRet == CSTR_EQUAL;
}
