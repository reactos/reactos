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
/* $Id: text.c,v 1.7 2003/07/10 21:04:32 chorns Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#define __NTAPP__

#include <windows.h>
#include <user32.h>

//#include <kernel32/winnls.h>
#include <ntos/rtl.h>

#include <debug.h>


const unsigned short wctype_table[] =
{
};

/* the character type contains the C1_* flags in the low 12 bits */
/* and the C2_* type in the high 4 bits */
static inline unsigned short get_char_typeW(WCHAR ch)
{
    extern const unsigned short wctype_table[];
    return wctype_table[wctype_table[ch >> 8] + (ch & 0xff)];
}


/* FUNCTIONS *****************************************************************/

//LPSTR STDCALL CharLowerA(LPSTR lpsz)
/*
 * @implemented
 */
LPSTR
WINAPI
CharLowerA(LPSTR x)
{
    if (!HIWORD(x)) return (LPSTR)tolower((char)(int)x);
/*
    __TRY
    {
        LPSTR s = x;
        while (*s)
        {
            *s=tolower(*s);
            s++;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
 */
    return x;
}

//DWORD STDCALL CharLowerBuffA(LPSTR lpsz, DWORD cchLength)
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

//DWORD STDCALL CharLowerBuffW(LPWSTR lpsz, DWORD cchLength)
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

//LPWSTR STDCALL CharLowerW(LPWSTR lpsz)
/*
 * @implemented
 */
LPWSTR
WINAPI
CharLowerW(LPWSTR x)
{
    if (HIWORD(x)) {
        return _wcslwr(x);
    } else {
        return (LPWSTR)(INT)towlower((WORD)(((DWORD)(x)) & 0xFFFF));
    }
}

//LPWSTR STDCALL CharPrevW(LPCWSTR lpszStart, LPCWSTR lpszCurrent)
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

//LPSTR STDCALL CharNextA(LPCSTR lpsz)
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

//LPSTR STDCALL CharNextExA(WORD CodePage, LPCSTR lpCurrentChar, DWORD dwFlags)
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

//LPWSTR STDCALL CharNextW(LPCWSTR lpsz)
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

//LPSTR STDCALL CharPrevA(LPCSTR lpszStart, LPCSTR lpszCurrent)
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

//LPSTR STDCALL CharPrevExA(WORD CodePage, LPCSTR lpStart, LPCSTR lpCurrentChar, DWORD dwFlags)
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

//WINBOOL STDCALL CharToOemA(LPCSTR lpszSrc, LPSTR lpszDst)
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

//WINBOOL STDCALL CharToOemBuffA(LPCSTR lpszSrc, LPSTR lpszDst, DWORD cchDstLength)
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

//WINBOOL STDCALL CharToOemBuffW(LPCWSTR lpszSrc, LPSTR lpszDst, DWORD cchDstLength)
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

//WINBOOL STDCALL CharToOemW(LPCWSTR lpszSrc, LPSTR lpszDst)
/*
 * @implemented
 */
BOOL
WINAPI
CharToOemW(LPCWSTR s, LPSTR d)
{
    return CharToOemBuffW(s, d, wcslen(s) + 1);
}

//LPSTR STDCALL CharUpperA(LPSTR lpsz)
/*
 * @implemented
 */
LPSTR WINAPI CharUpperA(LPSTR x)
{
    if (!HIWORD(x)) return (LPSTR)toupper((char)(int)x);
/*
    __TRY
    {
        LPSTR s = x;
        while (*s)
        {
            *s=toupper(*s);
            s++;
        }
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return x;
 */
  return (LPSTR)NULL;
}

//DWORD STDCALL CharUpperBuffA(LPSTR lpsz, DWORD cchLength)
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

//DWORD STDCALL CharUpperBuffW(LPWSTR lpsz, DWORD cchLength)
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

//LPWSTR STDCALL CharUpperW(LPWSTR lpsz)
/*
 * @implemented
 */
LPWSTR
WINAPI
CharUpperW(LPWSTR x)
{
    if (HIWORD(x)) return _wcsupr(x);
    else return (LPWSTR)(UINT)towlower((WORD)(((DWORD)(x)) & 0xFFFF));
}

//WINBOOL STDCALL IsCharAlphaA(CHAR ch)
/*
 * @implemented
 */
BOOL
WINAPI
IsCharAlphaA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharAlphaW(wch);
}

const char IsCharAlphaNumericA_lookup_table[] = { 
    0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0xff,  0x03,
    0xfe,  0xff,  0xff,  0x07,  0xfe,  0xff,  0xff,  0x07,
    0x08,  0x54,  0x00,  0xd4,  0x00,  0x00,  0x0c,  0x02,
    0xff,  0xff,  0x7f,  0xff,  0xff,  0xff,  0x7f,  0xff
};

/*
 * @implemented
 */
WINBOOL
STDCALL
IsCharAlphaNumericA(CHAR ch)
{
//    return (IsCharAlphaNumericA_lookup_table[ch / 8] & (1 << (ch % 8))) ? 1 : 0;

    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wch, 1);
    return IsCharAlphaNumericW(wch);
  //return FALSE;
}

/*
 * @implemented
 */
WINBOOL
STDCALL
IsCharAlphaNumericW(WCHAR ch)
{
    return (get_char_typeW(ch) & (C1_ALPHA|C1_DIGIT)) != 0;
//  return FALSE;
}

//WINBOOL STDCALL IsCharAlphaW(WCHAR ch)
/*
 * @implemented
 */
BOOL
WINAPI
IsCharAlphaW(WCHAR x)
{
    return (get_char_typeW(x) & C1_ALPHA) != 0;
}

//WINBOOL STDCALL IsCharLowerA(CHAR ch)
/*
 * @implemented
 */
BOOL
WINAPI
IsCharLowerA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharLowerW(wch);
}

//WINBOOL STDCALL IsCharLowerW(WCHAR ch)
/*
 * @implemented
 */
BOOL
WINAPI
IsCharLowerW(WCHAR x)
{
    return (get_char_typeW(x) & C1_LOWER) != 0;
}

//WINBOOL STDCALL IsCharUpperA(CHAR ch)
/*
 * @implemented
 */
BOOL
WINAPI
IsCharUpperA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharUpperW(wch);
}

//WINBOOL STDCALL IsCharUpperW(WCHAR ch)
/*
 * @implemented
 */
BOOL
WINAPI
IsCharUpperW(WCHAR x)
{
    return (get_char_typeW(x) & C1_UPPER) != 0;
}

//WINBOOL STDCALL OemToCharA(LPCSTR lpszSrc, LPSTR lpszDst)
/*
 * @implemented
 */
BOOL
WINAPI
OemToCharA(LPCSTR s, LPSTR d)
{
    return OemToCharBuffA(s, d, strlen(s) + 1);
}

//WINBOOL STDCALL OemToCharBuffA(LPCSTR lpszSrc, LPSTR lpszDst, DWORD cchDstLength)
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

//WINBOOL STDCALL OemToCharBuffW(LPCSTR lpszSrc, LPWSTR lpszDst, DWORD cchDstLength)
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

//WINBOOL STDCALL OemToCharW(LPCSTR lpszSrc, LPWSTR lpszDst)
/*
 * @implemented
 */
BOOL WINAPI OemToCharW(LPCSTR s, LPWSTR d)
{
    return OemToCharBuffW(s, d, strlen(s) + 1);
}
