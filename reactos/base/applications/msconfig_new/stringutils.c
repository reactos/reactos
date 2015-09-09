/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/stringutils.c
 * PURPOSE:     ANSI & UNICODE String Utility Functions
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include "utils.h"
#include "stringutils.h"

//
// String formatting
//
LPTSTR FormatStringV(LPCTSTR str, va_list args)
{
    LPTSTR lpszString = NULL;

    if (str)
    {
        size_t strLenPlusNull = _vsctprintf(str, args) + 1;
        lpszString = (LPTSTR)MemAlloc(0, strLenPlusNull * sizeof(TCHAR));
        if (lpszString)
            StringCchVPrintf(lpszString, strLenPlusNull, str, args);
    }

    return lpszString;
}

LPTSTR FormatString(LPCTSTR str, ...)
{
    LPTSTR lpszString;
    va_list args;

    va_start(args, str);
    lpszString = FormatStringV(str, args);
    va_end(args);

    return lpszString;
}

//
// String handling (ANSI <-> Unicode UTF16)
//
LPSTR UnicodeToAnsi(LPCWSTR strW)
{
    LPSTR strA = NULL;

    if (strW)
    {
        int iNeededChars = WideCharToMultiByte(CP_ACP,
                                               WC_COMPOSITECHECK /* | WC_NO_BEST_FIT_CHARS */,
                                               strW, -1, NULL, 0, NULL, NULL);

        strA = (LPSTR)MemAlloc(0, iNeededChars * sizeof(CHAR));
        if (strA)
        {
            WideCharToMultiByte(CP_ACP,
                                WC_COMPOSITECHECK /* | WC_NO_BEST_FIT_CHARS */,
                                strW, -1, strA, iNeededChars, NULL, NULL);
        }
    }

    return strA;
}

LPWSTR AnsiToUnicode(LPCSTR strA)
{
    LPWSTR strW = NULL;

    if (strA)
    {
        int iNeededChars = MultiByteToWideChar(CP_ACP,
                                               MB_PRECOMPOSED,
                                               strA, -1, NULL, 0);

        strW = (LPWSTR)MemAlloc(0, iNeededChars * sizeof(WCHAR));
        if (strW)
        {
            MultiByteToWideChar(CP_ACP,
                                MB_PRECOMPOSED,
                                strA, -1, strW, iNeededChars);
        }
    }

    return strW;
}

LPSTR DuplicateStringA(LPCSTR str)
{
    LPSTR dupStr = NULL;

    if (str)
    {
        size_t strSizePlusNull = strlen(str) + 1;

        dupStr = (LPSTR)MemAlloc(0, strSizePlusNull * sizeof(CHAR));
        if (dupStr)
            StringCchCopyA(dupStr, strSizePlusNull, str);
    }

    return dupStr;
}

LPWSTR DuplicateStringW(LPCWSTR str)
{
    LPWSTR dupStr = NULL;

    if (str)
    {
        size_t strSizePlusNull = wcslen(str) + 1;

        dupStr = (LPWSTR)MemAlloc(0, strSizePlusNull * sizeof(WCHAR));
        if (dupStr)
            StringCchCopyW(dupStr, strSizePlusNull, str);
    }

    return dupStr;
}

LPSTR DuplicateStringAEx(LPCSTR str, size_t numOfChars)
{
    LPSTR dupStr = NULL;

    if (str)
    {
        size_t strSize = min(strlen(str), numOfChars);

        dupStr = (LPSTR)MemAlloc(0, (strSize + 1) * sizeof(CHAR));
        if (dupStr)
        {
            StringCchCopyNA(dupStr, strSize + 1, str, strSize);
            dupStr[strSize] = '\0';
        }
    }

    return dupStr;
}

LPWSTR DuplicateStringWEx(LPCWSTR str, size_t numOfChars)
{
    LPWSTR dupStr = NULL;

    if (str)
    {
        size_t strSize = min(wcslen(str), numOfChars);

        dupStr = (LPWSTR)MemAlloc(0, (strSize + 1) * sizeof(WCHAR));
        if (dupStr)
        {
            StringCchCopyNW(dupStr, strSize + 1, str, strSize);
            dupStr[strSize] = L'\0';
        }
    }

    return dupStr;
}

//
// String search functions
//
/***
*wchar_t *wcsstr(string1, string2) - search for string2 in string1
*       (wide strings)
*
*Purpose:
*       finds the first occurrence of string2 in string1 (wide strings)
*
*Entry:
*       wchar_t *string1 - string to search in
*       wchar_t *string2 - string to search for
*
*Exit:
*       returns a pointer to the first occurrence of string2 in
*       string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/
LPTSTR FindSubStrI(LPCTSTR str, LPCTSTR strSearch)
{
    LPTSTR cp = (LPTSTR)str;
    LPTSTR s1, s2;

    if (!*strSearch)
        return (LPTSTR)str;

    while (*cp)
    {
        s1 = cp;
        s2 = (LPTSTR)strSearch;

        while (*s1 && *s2 && (_totupper(*s1) == _totupper(*s2)))
            ++s1, ++s2;

        if (!*s2)
            return cp;

        ++cp;
    }

    return NULL;
}

/*************************************************************************
 * AppendPathSeparator
 *
 * Append a backslash ('\') to a path if one doesn't exist.
 *
 * PARAMS
 *  lpszPath [I/O] The path to append a backslash to.
 *
 * RETURNS
 *  Success: The position of the last backslash in the path.
 *  Failure: NULL, if lpszPath is NULL or the path is too large.
 */
LPTSTR AppendPathSeparator(LPTSTR lpszPath)
{
    size_t iLen = 0;

    if (!lpszPath || (iLen = _tcslen(lpszPath)) >= MAX_PATH)
        return NULL;

    if (iLen >= 1)
    {
        lpszPath += iLen - 1;
        if (*lpszPath++ != _T('\\'))
        {
            *lpszPath++ = _T('\\');
            *lpszPath   = _T('\0');
        }
    }

    return lpszPath;
}
