/*
 * Copyright 2000 Juergen Schmied
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wine/unicode.h>
#include <wine/debug.h>

#include "shell32_main.h"
#include "undocshell.h"
#include "shlwapi_undoc.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/************************* STRRET functions ****************************/

static const char *debugstr_strret(STRRET *s)
{
    switch (s->uType)
    {
        case STRRET_WSTR:
            return "STRRET_WSTR";
        case STRRET_CSTR:
            return "STRRET_CSTR";
        case STRRET_OFFSET:
            return "STRRET_OFFSET";
        default:
            return "STRRET_???";
    }
}

BOOL WINAPI StrRetToStrNA(LPSTR dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{
    TRACE("dest=%p len=0x%x strret=%p(%s) pidl=%p\n", dest, len, src, debugstr_strret(src), pidl);

    if (!dest)
        return FALSE;

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
            lstrcpynA(dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
            break;
        default:
            FIXME("unknown type %u!\n", src->uType);
            if (len)
                *dest = '\0';
            return FALSE;
    }
    TRACE("-- %s\n", debugstr_a(dest) );
    return TRUE;
}

/************************************************************************/

BOOL WINAPI StrRetToStrNW(LPWSTR dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{
    TRACE("dest=%p len=0x%x strret=%p(%s) pidl=%p\n", dest, len, src, debugstr_strret(src), pidl);

    if (!dest)
        return FALSE;

    switch (src->uType)
    {
        case STRRET_WSTR:
            lstrcpynW(dest, src->u.pOleStr, len);
            CoTaskMemFree(src->u.pOleStr);
            break;
        case STRRET_CSTR:
            if (!MultiByteToWideChar(CP_ACP, 0, src->u.cStr, -1, dest, len) && len)
                dest[len-1] = 0;
            break;
        case STRRET_OFFSET:
            if (!MultiByteToWideChar(CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->u.uOffset, -1, dest, len)
                    && len)
                dest[len-1] = 0;
            break;
        default:
            FIXME("unknown type %u!\n", src->uType);
            if (len)
                *dest = '\0';
            return FALSE;
    }
    return TRUE;
}


/*************************************************************************
 * StrRetToStrN    [SHELL32.96]
 *
 * converts a STRRET to a normal string
 *
 * NOTES
 *  the pidl is for STRRET OFFSET
 */
BOOL WINAPI StrRetToStrNAW(LPVOID dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{
    if(SHELL_OsIsUnicode())
        return StrRetToStrNW(dest, len, src, pidl);
    else
        return StrRetToStrNA(dest, len, src, pidl);
}

/************************* OLESTR functions ****************************/

/************************************************************************
 *	StrToOleStr			[SHELL32.163]
 *
 */
static int StrToOleStrA (LPWSTR lpWideCharStr, LPCSTR lpMultiByteString)
{
	TRACE("(%p, %p %s)\n",
	lpWideCharStr, lpMultiByteString, debugstr_a(lpMultiByteString));

	return MultiByteToWideChar(CP_ACP, 0, lpMultiByteString, -1, lpWideCharStr, MAX_PATH);

}
static int StrToOleStrW (LPWSTR lpWideCharStr, LPCWSTR lpWString)
{
	TRACE("(%p, %p %s)\n",
	lpWideCharStr, lpWString, debugstr_w(lpWString));

	strcpyW (lpWideCharStr, lpWString );
	return strlenW(lpWideCharStr);
}

BOOL WINAPI StrToOleStrAW (LPWSTR lpWideCharStr, LPCVOID lpString)
{
	if (SHELL_OsIsUnicode())
	  return StrToOleStrW (lpWideCharStr, lpString);
	return StrToOleStrA (lpWideCharStr, lpString);
}

/*************************************************************************
 * StrToOleStrN					[SHELL32.79]
 *  lpMulti, nMulti, nWide [IN]
 *  lpWide [OUT]
 */
static BOOL StrToOleStrNA (LPWSTR lpWide, INT nWide, LPCSTR lpStrA, INT nStr)
{
	TRACE("(%p, %x, %s, %x)\n", lpWide, nWide, debugstr_an(lpStrA,nStr), nStr);
	return MultiByteToWideChar (CP_ACP, 0, lpStrA, nStr, lpWide, nWide);
}
static BOOL StrToOleStrNW (LPWSTR lpWide, INT nWide, LPCWSTR lpStrW, INT nStr)
{
	TRACE("(%p, %x, %s, %x)\n", lpWide, nWide, debugstr_wn(lpStrW, nStr), nStr);

	if (lstrcpynW (lpWide, lpStrW, nWide))
	{ return lstrlenW (lpWide);
	}
	return FALSE;
}

BOOL WINAPI StrToOleStrNAW (LPWSTR lpWide, INT nWide, LPCVOID lpStr, INT nStr)
{
	if (SHELL_OsIsUnicode())
	  return StrToOleStrNW (lpWide, nWide, lpStr, nStr);
	return StrToOleStrNA (lpWide, nWide, lpStr, nStr);
}

/*************************************************************************
 * OleStrToStrN					[SHELL32.78]
 */
static BOOL OleStrToStrNA (LPSTR lpStr, INT nStr, LPCWSTR lpOle, INT nOle)
{
	TRACE("(%p, %x, %s, %x)\n", lpStr, nStr, debugstr_wn(lpOle,nOle), nOle);
	return WideCharToMultiByte (CP_ACP, 0, lpOle, nOle, lpStr, nStr, NULL, NULL);
}

static BOOL OleStrToStrNW (LPWSTR lpwStr, INT nwStr, LPCWSTR lpOle, INT nOle)
{
	TRACE("(%p, %x, %s, %x)\n", lpwStr, nwStr, debugstr_wn(lpOle,nOle), nOle);

	if (lstrcpynW ( lpwStr, lpOle, nwStr))
	{ return lstrlenW (lpwStr);
	}
        return FALSE;
}

BOOL WINAPI OleStrToStrNAW (LPVOID lpOut, INT nOut, LPCVOID lpIn, INT nIn)
{
	if (SHELL_OsIsUnicode())
	  return OleStrToStrNW (lpOut, nOut, lpIn, nIn);
	return OleStrToStrNA (lpOut, nOut, lpIn, nIn);
}


/*************************************************************************
 * CheckEscapesA             [SHELL32.@]
 *
 * Checks a string for special characters which are not allowed in a path
 * and encloses it in quotes if that is the case.
 *
 * PARAMS
 *  string     [I/O] string to check and on return eventually quoted
 *  len        [I]   length of string
 */
VOID WINAPI CheckEscapesA(
	LPSTR	string,         /* [I/O]   string to check ??*/
	DWORD	len)            /* [I]      is 0 */
{
    LPWSTR wString;
    TRACE("(%s %d)\n", debugstr_a(string), len);

    if (!string || !string[0])
        return;

    wString = LocalAlloc(LPTR, len * sizeof(WCHAR));
    if (!wString)
        return;

    SHAnsiToUnicode(string, wString, len);
    CheckEscapesW(wString, len);
    SHUnicodeToAnsi(wString, string, len);
    LocalFree(wString);
}

/*************************************************************************
 * CheckEscapesW             [SHELL32.@]
 *
 * See CheckEscapesA.
 */
VOID WINAPI CheckEscapesW(
	LPWSTR	string,
	DWORD	len)
{
	DWORD size = lstrlenW(string);
	LPWSTR s, d;

	TRACE("(%s %d) stub\n", debugstr_w(string), len);

	if (StrPBrkW(string, L" \",;^") && size + 2 <= len)
	{
	  s = &string[size - 1];
	  d = &string[size + 2];
	  *d-- = 0;
	  *d-- = '"';
	  for (;d > string;)
	    *d-- = *s--;
	  *d = '"';
	}
}
