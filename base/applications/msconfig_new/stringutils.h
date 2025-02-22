/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/stringutils.h
 * PURPOSE:     ANSI & UNICODE String Utility Functions
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef __STRINGUTILS_H__
#define __STRINGUTILS_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//
// String formatting
//
LPTSTR FormatStringV(LPCTSTR str, va_list args);
LPTSTR FormatString(LPCTSTR str, ...);

//
// String handling (ANSI <-> Unicode UTF16)
//
LPSTR UnicodeToAnsi(LPCWSTR strW);
LPWSTR AnsiToUnicode(LPCSTR strA);
LPSTR DuplicateStringA(LPCSTR str);
LPWSTR DuplicateStringW(LPCWSTR str);
LPSTR DuplicateStringAEx(LPCSTR str, size_t numOfChars);
LPWSTR DuplicateStringWEx(LPCWSTR str, size_t numOfChars);

//
// Conversion macros ANSI <-> Unicode
//
#ifdef UNICODE
    #define NewAnsiString(x)        UnicodeToAnsi(x)
    #define NewPortableString(x)    AnsiToUnicode(x)
    #define DuplicateString(x)      DuplicateStringW(x)
    #define DuplicateStringEx(x, y) DuplicateStringWEx((x), (y))
#else
    #define NewAnsiString(x)        DuplicateStringA(x)
    #define NewPortableString(x)    DuplicateString(x)
    #define DuplicateString(x)      DuplicateStringA(x)
    #define DuplicateStringEx(x, y) DuplicateStringAEx((x), (y))
#endif

//
// String search functions
//
#define FindSubStr(str, strSearch) _tcsstr((str), (strSearch))
LPTSTR FindSubStrI(LPCTSTR str, LPCTSTR strSearch);

LPTSTR AppendPathSeparator(LPTSTR lpszPath);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __STRINGUTILS_H__

/* EOF */
