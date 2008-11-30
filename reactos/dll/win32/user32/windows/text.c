/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>

/* FUNCTIONS *****************************************************************/

static WORD
GetC1Type(WCHAR Ch)
{
    WORD CharType;

    if (! GetStringTypeW(CT_CTYPE1, &Ch, 1, &CharType))
    {
        return 0;
    }

    return CharType;
}

/*
 * @implemented
 */
LPSTR
WINAPI
CharLowerA(LPSTR str)
{
    if (!HIWORD(str))
    {
        char ch = LOWORD(str);
        CharLowerBuffA( &ch, 1 );
        return (LPSTR)(UINT_PTR)(BYTE)ch;
    }

    _SEH_TRY
    {
        CharLowerBuffA( str, strlen(str) );
    }
    _SEH_HANDLE
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    _SEH_END;

    return str;
}

/*
 * @implemented
 */
DWORD
WINAPI
CharLowerBuffA(LPSTR str, DWORD len)
{
    DWORD lenW;
    WCHAR *strW;
    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
    if (strW) {
        MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
        CharLowerBuffW(strW, lenW);
        len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
        HeapFree(GetProcessHeap(), 0, strW);
        return len;
    }
    return 0;
}

/*
 * @implemented
 */
DWORD
WINAPI
CharLowerBuffW(LPWSTR str, DWORD len)
{
    DWORD ret = len;
    if (!str) return 0; /* YES */
    for (; len; len--, str++) *str = towlower(*str);
    return ret;
}

/*
 * @implemented
 */
LPWSTR
WINAPI
CharLowerW(LPWSTR x)
{
    if (HIWORD(x)) return strlwrW(x);
    else return (LPWSTR)((UINT_PTR)tolowerW(LOWORD(x)));
}

/*
 * @implemented
 */
LPWSTR
WINAPI
CharPrevW(LPCWSTR start, LPCWSTR x)
{
    if (x > start) return (LPWSTR)(x-1);
    else return (LPWSTR)x;
}

/*
 * @implemented
 */
LPSTR
WINAPI
CharNextA(LPCSTR ptr)
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByte(ptr[0]) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}

/*
 * @implemented
 */
LPSTR
WINAPI
CharNextExA(WORD codepage, LPCSTR ptr, DWORD flags)
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByteEx(codepage, ptr[0]) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}

/*
 * @implemented
 */
LPWSTR
WINAPI
CharNextW(LPCWSTR x)
{
    if (*x) x++;
    return (LPWSTR)x;
}

/*
 * @implemented
 */
LPSTR
WINAPI
CharPrevA(LPCSTR start, LPCSTR ptr)
{
    while (*start && (start < ptr)) {
        LPCSTR next = CharNextA(start);
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}

/*
 * @implemented
 */
LPSTR WINAPI CharPrevExA( WORD codepage, LPCSTR start, LPCSTR ptr, DWORD flags )
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextExA( codepage, start, flags );
        if (next > ptr) break;
        start = next;
    }
    return (LPSTR)start;
}

/*
 * @implemented
 */
BOOL
WINAPI
CharToOemA(LPCSTR s, LPSTR d)
{
    if (!s || !d) return TRUE;
    return CharToOemBuffA(s, d, strlen(s) + 1);
}

/*
 * @implemented
 */
BOOL
WINAPI
CharToOemBuffA(LPCSTR s, LPSTR d, DWORD len)
{
    WCHAR* bufW;

    bufW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (bufW) {
	    MultiByteToWideChar(CP_ACP, 0, s, len, bufW, len);
        WideCharToMultiByte(CP_OEMCP, 0, bufW, len, d, len, NULL, NULL);
	    HeapFree(GetProcessHeap(), 0, bufW);
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CharToOemBuffW(LPCWSTR s, LPSTR d, DWORD len)
{
    if (!s || !d)
        return TRUE;
    WideCharToMultiByte(CP_OEMCP, 0, s, len, d, len, NULL, NULL);
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CharToOemW(LPCWSTR s, LPSTR d)
{
    return CharToOemBuffW(s, d, wcslen(s) + 1);
}

/*
 * @implemented
 */
LPSTR WINAPI CharUpperA(LPSTR str)
{
    if (!HIWORD(str))
    {
        char ch = LOWORD(str);
        CharUpperBuffA( &ch, 1 );
        return (LPSTR)(UINT_PTR)(BYTE)ch;
    }

    _SEH_TRY
    {
        CharUpperBuffA( str, strlen(str) );
    }
    _SEH_HANDLE
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    _SEH_END;

    return str;
}

/*
 * @implemented
 */
DWORD
WINAPI
CharUpperBuffA(LPSTR str, DWORD len)
{
    DWORD lenW;
    WCHAR* strW;
    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
    if (strW) {
        MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
        CharUpperBuffW(strW, lenW);
        len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
        HeapFree(GetProcessHeap(), 0, strW);
        return len;
    }
    return 0;
}

/*
 * @implemented
 */
DWORD
WINAPI
CharUpperBuffW(LPWSTR str, DWORD len)
{
    DWORD ret = len;
    if (!str) return 0; /* YES */
    for (; len; len--, str++) *str = towupper(*str);
    return ret;
}

/*
 * @implemented
 */
LPWSTR
WINAPI
CharUpperW(LPWSTR x)
{
    if (HIWORD(x)) return struprW(x);
    else return (LPWSTR)((UINT_PTR)toupperW(LOWORD(x)));
}

/*
 * @implemented
 */
BOOL
WINAPI
IsCharAlphaA(CHAR Ch)
{
    WCHAR WCh;

    MultiByteToWideChar(CP_ACP, 0, &Ch, 1, &WCh, 1);
    return IsCharAlphaW(WCh);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsCharAlphaNumericA(CHAR Ch)
{
    WCHAR WCh;

    MultiByteToWideChar(CP_ACP, 0, &Ch, 1, &WCh, 1);
    return IsCharAlphaNumericW(WCh);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsCharAlphaNumericW(WCHAR Ch)
{
    return (GetC1Type(Ch) & (C1_ALPHA|C1_DIGIT)) != 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsCharAlphaW(WCHAR Ch)
{
    return (GetC1Type(Ch) & C1_ALPHA) != 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsCharLowerA(CHAR Ch)
{
    WCHAR WCh;

    MultiByteToWideChar(CP_ACP, 0, &Ch, 1, &WCh, 1);
    return IsCharLowerW(WCh);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsCharLowerW(WCHAR Ch)
{
    return (GetC1Type(Ch) & C1_LOWER) != 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsCharUpperA(CHAR Ch)
{
    WCHAR WCh;

    MultiByteToWideChar(CP_ACP, 0, &Ch, 1, &WCh, 1);
    return IsCharUpperW(WCh);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsCharUpperW(WCHAR Ch)
{
    return (GetC1Type(Ch) & C1_UPPER) != 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
OemToCharA(LPCSTR s, LPSTR d)
{
    return OemToCharBuffA(s, d, strlen(s) + 1);
}

/*
 * @implemented
 */
BOOL WINAPI OemToCharBuffA(LPCSTR s, LPSTR d, DWORD len)
{
    WCHAR* bufW;

    bufW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (bufW) {
        MultiByteToWideChar(CP_OEMCP, 0, s, len, bufW, len);
	    WideCharToMultiByte(CP_ACP, 0, bufW, len, d, len, NULL, NULL);
	    HeapFree(GetProcessHeap(), 0, bufW);
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
OemToCharBuffW(LPCSTR s, LPWSTR d, DWORD len)
{
    MultiByteToWideChar(CP_OEMCP, 0, s, len, d, len);
    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI OemToCharW(LPCSTR s, LPWSTR d)
{
    return OemToCharBuffW(s, d, strlen(s) + 1);
}

/* EOF */
