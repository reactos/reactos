/*
 *  ATTRIB.C - attrib internal command.
 *
 *
 *  History:
 *
 *    04-Dec-1998 Eric Kohl
 *        started
 *
 *    09-Dec-1998 Eric Kohl
 *        implementation works, except recursion ("attrib /s").
 *
 *    05-Jan-1999 Eric Kohl
 *        major rewrite.
 *        fixed recursion ("attrib /s").
 *        started directory support ("attrib /s /d").
 *        updated help text.
 *
 *    14-Jan-1999 Eric Kohl
 *        Unicode ready!
 *
 *    19-Jan-1999 Eric Kohl
 *        Redirection ready!
 *
 *    21-Jan-1999 Eric Kohl
 *        Added check for invalid filenames.
 *
 *    23-Jan-1999 Eric Kohl
 *        Added handling of multiple filenames.
 *
 *    02-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winuser.h>

#include <conutils.h>

#include "resource.h"

CON_SCREEN StdOutScreen = INIT_CON_SCREEN(StdOut);

static
VOID
ErrorMessage(
    DWORD dwErrorCode,
    LPWSTR szFormat,
    ...)
{
    WCHAR szMsg[RC_STRING_MAX_SIZE];
    WCHAR  szMessage[1024];
    LPWSTR szError;
    va_list arg_ptr;

    if (dwErrorCode == ERROR_SUCCESS)
        return;

    if (szFormat)
    {
        va_start(arg_ptr, szFormat);
        vswprintf(szMessage, szFormat, arg_ptr);
        va_end(arg_ptr);
    }

    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR)&szError, 0, NULL))
    {
        ConPrintf(StdOut, L"%s %s\n", szError, szMessage);
        if (szError)
            LocalFree(szError);
        return;
    }

    /* Fall back just in case the error is not defined */
    LoadStringW(GetModuleHandle(NULL), STRING_CONSOLE_ERROR, szMsg, ARRAYSIZE(szMsg));
    if (szFormat)
        ConPrintf(StdOut, L"%s -- %s\n", szMsg, szMessage);
    else
        ConPrintf(StdOut, L"%s\n", szMsg);
}

static
INT
PrintAttribute(
    LPWSTR pszPath,
    LPWSTR pszFile,
    BOOL bRecurse)
{
    WIN32_FIND_DATAW findData;
    HANDLE hFind;
    WCHAR  szFullName[MAX_PATH];
    LPWSTR pszFileName;

    /* prepare full file name buffer */
    wcscpy(szFullName, pszPath);
    pszFileName = szFullName + wcslen(szFullName);

    /* display all subdirectories */
    if (bRecurse)
    {
        /* append file name */
        wcscpy(pszFileName, pszFile);

        hFind = FindFirstFileW(szFullName, &findData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            ErrorMessage(GetLastError(), pszFile);
            return 1;
        }

        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            if (!wcscmp(findData.cFileName, L".") ||
                !wcscmp(findData.cFileName, L".."))
                continue;

            wcscpy(pszFileName, findData.cFileName);
            wcscat(pszFileName, L"\\");
            PrintAttribute(szFullName, pszFile, bRecurse);
        }
        while(FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    /* append file name */
    wcscpy(pszFileName, pszFile);

    /* display current directory */
    hFind = FindFirstFileW(szFullName, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ErrorMessage(GetLastError(), pszFile);
        return 1;
    }

    do
    {
        wcscpy(pszFileName, findData.cFileName);

        ConPrintf(StdOut,
                  L"%c  %c%c%c     %s\n",
                  (findData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) ? L'A' : L' ',
                  (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ? L'S' : L' ',
                  (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? L'H' : L' ',
                  (findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? L'R' : L' ',
                  szFullName);
    }
    while(FindNextFileW(hFind, &findData));
    FindClose(hFind);

    return 0;
}


static
BOOL
ChangeAttribute(
    LPWSTR pszPath,
    LPWSTR pszFile,
    DWORD dwMask,
    DWORD dwAttrib,
    BOOL bRecurse,
    BOOL bDirectories)
{
    WIN32_FIND_DATAW findData;
    HANDLE hFind;
    DWORD  dwAttribute;
    WCHAR  szFullName[MAX_PATH];
    LPWSTR pszFileName;
    BOOL bWildcard = (wcschr(pszFile, L'*') || wcschr(pszFile, L'?'));

    /* prepare full file name buffer */
    wcscpy(szFullName, pszPath);
    pszFileName = szFullName + wcslen(szFullName);

    /* append file name */
    wcscpy(pszFileName, pszFile);

    hFind = FindFirstFileW(szFullName, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    dwAttribute = findData.dwFileAttributes;

    if (!bWildcard)
    {
        FindClose(hFind);
        if (dwAttribute & FILE_ATTRIBUTE_DIRECTORY)
        {
            dwAttribute = (dwAttribute & ~dwMask) | dwAttrib;
            SetFileAttributes(szFullName, dwAttribute);
            if (bRecurse)
            {
                if (bDirectories)
                {
                    ChangeAttribute(szFullName, L"*", dwMask, dwAttrib,
                                    bRecurse, bDirectories);
                }
                else
                {
                    if (!ChangeAttribute(szFullName, L"*", dwMask, dwAttrib,
                                         bRecurse, FALSE))
                    {
                        return FALSE;
                    }
                }
            }
            else
            {
                if (!bDirectories)
                {
                    ChangeAttribute(szFullName, L"*", dwMask, dwAttrib,
                                    bRecurse, FALSE);
                }
            }
            return TRUE;
        }
        else
        {
            dwAttribute = (dwAttribute & ~dwMask) | dwAttrib;
            SetFileAttributes(szFullName, dwAttribute);
            return TRUE;
        }
    }
    else
    {
        if ((dwAttribute & FILE_ATTRIBUTE_DIRECTORY) && (!bRecurse || !bDirectories))
            return FALSE;

        do
        {
            dwAttribute = findData.dwFileAttributes;
            if (dwAttribute & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!bDirectories)
                    continue;

                if (!wcscmp(findData.cFileName, L".") ||
                    !wcscmp(findData.cFileName, L".."))
                    continue;

                wcscpy(pszFileName, findData.cFileName);
                dwAttribute = (dwAttribute & ~dwMask) | dwAttrib;
                SetFileAttributes(szFullName, dwAttribute);

                if (bRecurse)
                {
                    ChangeAttribute(szFullName, findData.cFileName, dwMask,
                                    dwAttrib, bRecurse, FALSE);
                }
            }
            else
            {
                wcscpy(pszFileName, findData.cFileName);
                dwAttribute = (dwAttribute & ~dwMask) | dwAttrib;
                SetFileAttributes(szFullName, dwAttribute);
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
    }

    return TRUE;
}

int wmain(int argc, WCHAR *argv[])
{
    INT    i;
    WCHAR  szPath[MAX_PATH];
    WCHAR  szFileName [MAX_PATH];
    BOOL   bRecurse = FALSE;
    BOOL   bDirectories = FALSE;
    DWORD  dwAttrib = 0;
    DWORD  dwMask = 0;
    LPWSTR p;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Print help */
    if (argc > 1 && wcscmp(argv[1], L"/?") == 0)
    {
        ConResPuts(StdOut, STRING_ATTRIB_HELP);
        return 0;
    }

    /* check for options */
    for (i = 1; i < argc; i++)
    {
        if (wcsicmp(argv[i], L"/s") == 0)
            bRecurse = TRUE;
        else if (wcsicmp(argv[i], L"/d") == 0)
            bDirectories = TRUE;
    }

    /* create attributes and mask */
    for (i = 1; i < argc; i++)
    {
        if (*argv[i] == L'+')
        {
            if (wcslen(argv[i]) != 2)
            {
                ConResPrintf(StdOut, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                return -1;
            }

            switch (towupper(argv[i][1]))
            {
                case L'A':
                    dwMask   |= FILE_ATTRIBUTE_ARCHIVE;
                    dwAttrib |= FILE_ATTRIBUTE_ARCHIVE;
                    break;

                case L'H':
                    dwMask   |= FILE_ATTRIBUTE_HIDDEN;
                    dwAttrib |= FILE_ATTRIBUTE_HIDDEN;
                    break;

                case L'R':
                    dwMask   |= FILE_ATTRIBUTE_READONLY;
                    dwAttrib |= FILE_ATTRIBUTE_READONLY;
                    break;

                case L'S':
                    dwMask   |= FILE_ATTRIBUTE_SYSTEM;
                    dwAttrib |= FILE_ATTRIBUTE_SYSTEM;
                    break;

                default:
                    ConResPrintf(StdOut, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                    return -1;
            }
        }
        else if (*argv[i] == L'-')
        {
            if (wcslen(argv[i]) != 2)
            {
                ConResPrintf(StdOut, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                return -1;
            }

            switch (towupper(argv[i][1]))
            {
                case L'A':
                    dwMask   |= FILE_ATTRIBUTE_ARCHIVE;
                    dwAttrib &= ~FILE_ATTRIBUTE_ARCHIVE;
                    break;

                case L'H':
                    dwMask   |= FILE_ATTRIBUTE_HIDDEN;
                    dwAttrib &= ~FILE_ATTRIBUTE_HIDDEN;
                    break;

                case L'R':
                    dwMask   |= FILE_ATTRIBUTE_READONLY;
                    dwAttrib &= ~FILE_ATTRIBUTE_READONLY;
                    break;

                case L'S':
                    dwMask   |= FILE_ATTRIBUTE_SYSTEM;
                    dwAttrib &= ~FILE_ATTRIBUTE_SYSTEM;
                    break;

                default:
                    ConResPrintf(StdOut, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                    return -1;
            }
        }
    }

    if (argc == 1)
    {
        DWORD len;

        len = GetCurrentDirectory(MAX_PATH, szPath);
        if (szPath[len-1] != L'\\')
        {
            szPath[len] = L'\\';
            szPath[len + 1] = UNICODE_NULL;
        }
        wcscpy(szFileName, L"*.*");
        PrintAttribute(szPath, szFileName, bRecurse);
        return 0;
    }

    /* get full file name */
    for (i = 1; i < argc; i++)
    {
        if (*argv[i] == L'+' || *argv[i] == L'-' || *argv[i] == L'/')
            continue;

        GetFullPathNameW(argv[i], MAX_PATH, szPath, &p);
        wcscpy(szFileName, p);
        *p = 0;

        if (dwMask == 0)
        {
            PrintAttribute(szPath, szFileName, bRecurse);
        }
        else if (!ChangeAttribute(szPath, szFileName, dwMask,
                                  dwAttrib, bRecurse, bDirectories))
        {
            ConResPrintf(StdOut, STRING_FILE_NOT_FOUND, argv[i]);
        }
    }

    return 0;
}
