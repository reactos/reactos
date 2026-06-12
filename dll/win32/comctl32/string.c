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
