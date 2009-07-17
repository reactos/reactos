/*
 * USER string functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 * Copyright 1996 Alexandre Julliard
 * Copyright 1996 Marcus Meissner
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

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"

#include "wine/exception.h"
#include "wine/unicode.h"


/***********************************************************************
 *           CharNextA   (USER32.@)
 */
LPSTR WINAPI CharNextA( LPCSTR ptr )
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByte( ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}


/***********************************************************************
 *           CharNextExA   (USER32.@)
 */
LPSTR WINAPI CharNextExA( WORD codepage, LPCSTR ptr, DWORD flags )
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByteEx( codepage, ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}


/***********************************************************************
 *           CharNextExW   (USER32.@)
 */
LPWSTR WINAPI CharNextExW( WORD codepage, LPCWSTR ptr, DWORD flags )
{
    /* doesn't make sense, there are no codepages for Unicode */
    return NULL;
}


/***********************************************************************
 *           CharNextW   (USER32.@)
 */
LPWSTR WINAPI CharNextW(LPCWSTR x)
{
    if (*x) x++;

    return (LPWSTR)x;
}


/***********************************************************************
 *           CharPrevA   (USER32.@)
 */
LPSTR WINAPI CharPrevA( LPCSTR start, LPCSTR ptr )
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextA( start );
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}


/***********************************************************************
 *           CharPrevExA   (USER32.@)
 */
LPSTR WINAPI CharPrevExA( WORD codepage, LPCSTR start, LPCSTR ptr, DWORD flags )
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextExA( codepage, start, flags );
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}


/***********************************************************************
 *           CharPrevExW   (USER32.@)
 */
LPSTR WINAPI CharPrevExW( WORD codepage, LPCWSTR start, LPCWSTR ptr, DWORD flags )
{
    /* doesn't make sense, there are no codepages for Unicode */
    return NULL;
}


/***********************************************************************
 *           CharPrevW   (USER32.@)
 */
LPWSTR WINAPI CharPrevW(LPCWSTR start,LPCWSTR x)
{
    if (x>start) return (LPWSTR)(x-1);
    else return (LPWSTR)x;
}


/***********************************************************************
 *           CharToOemA   (USER32.@)
 */
BOOL WINAPI CharToOemA( LPCSTR s, LPSTR d )
{
    if ( !s || !d ) return TRUE;
    return CharToOemBuffA( s, d, strlen( s ) + 1 );
}


/***********************************************************************
 *           CharToOemBuffA   (USER32.@)
 */
BOOL WINAPI CharToOemBuffA( LPCSTR s, LPSTR d, DWORD len )
{
    WCHAR *bufW;

    bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( bufW )
    {
	MultiByteToWideChar( CP_ACP, 0, s, len, bufW, len );
	WideCharToMultiByte( CP_OEMCP, 0, bufW, len, d, len, NULL, NULL );
	HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuffW   (USER32.@)
 */
BOOL WINAPI CharToOemBuffW( LPCWSTR s, LPSTR d, DWORD len )
{
   if ( !s || !d ) return TRUE;
    WideCharToMultiByte( CP_OEMCP, 0, s, len, d, len, NULL, NULL );
    return TRUE;
}


/***********************************************************************
 *           CharToOemW   (USER32.@)
 */
BOOL WINAPI CharToOemW( LPCWSTR s, LPSTR d )
{
    return CharToOemBuffW( s, d, strlenW( s ) + 1 );
}


/***********************************************************************
 *           OemToCharA   (USER32.@)
 */
BOOL WINAPI OemToCharA( LPCSTR s, LPSTR d )
{
    return OemToCharBuffA( s, d, strlen( s ) + 1 );
}


/***********************************************************************
 *           OemToCharBuffA   (USER32.@)
 */
BOOL WINAPI OemToCharBuffA( LPCSTR s, LPSTR d, DWORD len )
{
    WCHAR *bufW;

    bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( bufW )
    {
	MultiByteToWideChar( CP_OEMCP, 0, s, len, bufW, len );
	WideCharToMultiByte( CP_ACP, 0, bufW, len, d, len, NULL, NULL );
	HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuffW   (USER32.@)
 */
BOOL WINAPI OemToCharBuffW( LPCSTR s, LPWSTR d, DWORD len )
{
    MultiByteToWideChar( CP_OEMCP, 0, s, len, d, len );
    return TRUE;
}


/***********************************************************************
 *           OemToCharW   (USER32.@)
 */
BOOL WINAPI OemToCharW( LPCSTR s, LPWSTR d )
{
    return OemToCharBuffW( s, d, strlen( s ) + 1 );
}


/***********************************************************************
 *           CharLowerA   (USER32.@)
 */
LPSTR WINAPI CharLowerA(LPSTR str)
{
    if (!HIWORD(str))
    {
        char ch = LOWORD(str);
        CharLowerBuffA( &ch, 1 );
        return (LPSTR)(UINT_PTR)(BYTE)ch;
    }

    __TRY
    {
        CharLowerBuffA( str, strlen(str) );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return str;
}


/***********************************************************************
 *           CharUpperA   (USER32.@)
 */
LPSTR WINAPI CharUpperA(LPSTR str)
{
    if (!HIWORD(str))
    {
        char ch = LOWORD(str);
        CharUpperBuffA( &ch, 1 );
        return (LPSTR)(UINT_PTR)(BYTE)ch;
    }

    __TRY
    {
        CharUpperBuffA( str, strlen(str) );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return str;
}


/***********************************************************************
 *           CharLowerW   (USER32.@)
 */
LPWSTR WINAPI CharLowerW(LPWSTR x)
{
    if (HIWORD(x)) return strlwrW(x);
    else return (LPWSTR)((UINT_PTR)tolowerW(LOWORD(x)));
}


/***********************************************************************
 *           CharUpperW   (USER32.@)
 */
LPWSTR WINAPI CharUpperW(LPWSTR x)
{
    if (HIWORD(x)) return struprW(x);
    else return (LPWSTR)((UINT_PTR)toupperW(LOWORD(x)));
}


/***********************************************************************
 *           CharLowerBuffA   (USER32.@)
 */
DWORD WINAPI CharLowerBuffA( LPSTR str, DWORD len )
{
    DWORD lenW;
    WCHAR buffer[32];
    WCHAR *strW = buffer;

    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    if (lenW > sizeof(buffer)/sizeof(WCHAR))
    {
        strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        if (!strW) return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
    CharLowerBuffW(strW, lenW);
    len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
    if (strW != buffer) HeapFree(GetProcessHeap(), 0, strW);
    return len;
}


/***********************************************************************
 *           CharLowerBuffW   (USER32.@)
 */
DWORD WINAPI CharLowerBuffW( LPWSTR str, DWORD len )
{
    DWORD ret = len;
    if (!str) return 0; /* YES */
    for (; len; len--, str++) *str = tolowerW(*str);
    return ret;
}


/***********************************************************************
 *           CharUpperBuffA   (USER32.@)
 */
DWORD WINAPI CharUpperBuffA( LPSTR str, DWORD len )
{
    DWORD lenW;
    WCHAR buffer[32];
    WCHAR *strW = buffer;

    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    if (lenW > sizeof(buffer)/sizeof(WCHAR))
    {
        strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        if (!strW) return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
    CharUpperBuffW(strW, lenW);
    len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
    if (strW != buffer) HeapFree(GetProcessHeap(), 0, strW);
    return len;
}


/***********************************************************************
 *           CharUpperBuffW   (USER32.@)
 */
DWORD WINAPI CharUpperBuffW( LPWSTR str, DWORD len )
{
    DWORD ret = len;
    if (!str) return 0; /* YES */
    for (; len; len--, str++) *str = toupperW(*str);
    return ret;
}


/***********************************************************************
 *           IsCharLower    (USER.436)
 *           IsCharLowerA   (USER32.@)
 */
BOOL WINAPI IsCharLowerA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharLowerW(wch);
}


/***********************************************************************
 *           IsCharLowerW   (USER32.@)
 */
BOOL WINAPI IsCharLowerW(WCHAR x)
{
    return (get_char_typeW(x) & C1_LOWER) != 0;
}


/***********************************************************************
 *           IsCharUpper    (USER.435)
 *           IsCharUpperA   (USER32.@)
 */
BOOL WINAPI IsCharUpperA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharUpperW(wch);
}


/***********************************************************************
 *           IsCharUpperW   (USER32.@)
 */
BOOL WINAPI IsCharUpperW(WCHAR x)
{
    return (get_char_typeW(x) & C1_UPPER) != 0;
}


/***********************************************************************
 *           IsCharAlphaNumeric    (USER.434)
 *           IsCharAlphaNumericA   (USER32.@)
 */
BOOL WINAPI IsCharAlphaNumericA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharAlphaNumericW(wch);
}


/***********************************************************************
 *           IsCharAlphaNumericW   (USER32.@)
 */
BOOL WINAPI IsCharAlphaNumericW(WCHAR x)
{
    return (get_char_typeW(x) & (C1_ALPHA|C1_DIGIT)) != 0;
}


/***********************************************************************
 *           IsCharAlpha    (USER.433)
 *           IsCharAlphaA   (USER32.@)
 */
BOOL WINAPI IsCharAlphaA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharAlphaW(wch);
}


/***********************************************************************
 *           IsCharAlphaW   (USER32.@)
 */
BOOL WINAPI IsCharAlphaW(WCHAR x)
{
    return (get_char_typeW(x) & C1_ALPHA) != 0;
}
