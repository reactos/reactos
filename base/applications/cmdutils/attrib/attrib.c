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
    _In_ DWORD dwErrorCode,
    _In_opt_ PCWSTR pszMsg,
    ...)
{
    INT Len;
    va_list arg_ptr;

    if (dwErrorCode == ERROR_SUCCESS)
        return;

    va_start(arg_ptr, pszMsg);
    Len = ConMsgPrintfV(StdErr,
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        dwErrorCode,
                        LANG_USER_DEFAULT,
                        &arg_ptr);
    va_end(arg_ptr);

    /* Fall back just in case the error is not defined */
    if (Len <= 0)
        ConResPrintf(StdErr, STRING_CONSOLE_ERROR, dwErrorCode);

    /* Display the extra optional message if necessary */
    if (pszMsg)
        ConPrintf(StdErr, L"  %s\n", pszMsg);
}

/* Returns TRUE if anything is printed, FALSE otherwise */
static
BOOL
PrintAttribute(
    LPWSTR pszPath,
    LPWSTR pszFile,
    BOOL   bRecurse,
    BOOL   bDirectories)
{
    WIN32_FIND_DATAW findData;
    HANDLE hFind;
    WCHAR  szFullName[MAX_PATH];
    LPWSTR pszFileName;
    BOOL   bFound = FALSE;
    BOOL   bIsDir;
    BOOL   bExactMatch;
    DWORD  Error;

    /* prepare full file name buffer */
    wcscpy(szFullName, pszPath);
    pszFileName = szFullName + wcslen(szFullName);

    /* display all subdirectories */
    if (bRecurse)
    {
        /* append *.* */
        wcscpy(pszFileName, L"*.*");

        hFind = FindFirstFileW(szFullName, &findData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            Error = GetLastError();
            if ((Error != ERROR_DIRECTORY) && (Error != ERROR_SHARING_VIOLATION)
                  && (Error != ERROR_FILE_NOT_FOUND))
            {
                ErrorMessage(Error, pszFile);
            }
            return FALSE;
        }

        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            if (!wcscmp(findData.cFileName, L".") ||
                !wcscmp(findData.cFileName, L".."))
            {
                continue;
            }

            wcscpy(pszFileName, findData.cFileName);
            wcscat(pszFileName, L"\\");
            bFound |= PrintAttribute(szFullName, pszFile, bRecurse, bDirectories);
        }
        while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    /* append file name */
    wcscpy(pszFileName, pszFile);

    /* search current directory */
    hFind = FindFirstFileW(szFullName, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return bFound;
    }

    do
    {
        bIsDir = findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        bExactMatch = wcsicmp(findData.cFileName, pszFile) == 0;

        if (bIsDir && !bDirectories && !bExactMatch)
            continue;

        if (!wcscmp(findData.cFileName, L".") ||
            !wcscmp(findData.cFileName, L".."))
        {
            continue;
        }

        wcscpy(pszFileName, findData.cFileName);

        ConPrintf(StdOut,
                  L"%c  %c%c%c     %s\n",
                  (findData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) ? L'A' : L' ',
                  (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ? L'S' : L' ',
                  (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? L'H' : L' ',
                  (findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? L'R' : L' ',
                  szFullName);
        bFound = TRUE;
    }
    while (FindNextFileW(hFind, &findData));
    FindClose(hFind);

    return bFound;
}


/* Returns TRUE if anything changed, FALSE otherwise */
static
BOOL
ChangeAttribute(
    LPWSTR pszPath,
    LPWSTR pszFile,
    BOOL  bRecurse,
    BOOL  bDirectories,
    DWORD dwMask,
    DWORD dwAttrib)
{
    WIN32_FIND_DATAW findData;
    HANDLE hFind;
    WCHAR  szFullName[MAX_PATH];
    LPWSTR pszFileName;
    BOOL   bFound = FALSE;
    BOOL   bIsDir;
    BOOL   bExactMatch;
    DWORD  dwAttribute;
    DWORD  Error;

    /* prepare full file name buffer */
    wcscpy(szFullName, pszPath);
    pszFileName = szFullName + wcslen(szFullName);

    /* display all subdirectories */
    if (bRecurse)
    {
        /* append *.* */
        wcscpy(pszFileName, L"*.*");

        hFind = FindFirstFileW(szFullName, &findData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            Error = GetLastError();
            if ((Error != ERROR_DIRECTORY) && (Error != ERROR_SHARING_VIOLATION)
                  && (Error != ERROR_FILE_NOT_FOUND))
            {
                ErrorMessage(Error, pszFile);
            }
            return FALSE;
        }

        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

            if (!wcscmp(findData.cFileName, L".") ||
                !wcscmp(findData.cFileName, L".."))
            {
                continue;
            }

            wcscpy(pszFileName, findData.cFileName);
            wcscat(pszFileName, L"\\");
            bFound |= ChangeAttribute(szFullName, pszFile, bRecurse, bDirectories,
                                      dwMask, dwAttrib);
        }
        while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    /* append file name */
    wcscpy(pszFileName, pszFile);

    /* search current directory */
    hFind = FindFirstFileW(szFullName, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return bFound;
    }

    do
    {
        bIsDir = findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        bExactMatch = wcsicmp(findData.cFileName, pszFile) == 0;

        if (bIsDir && !bDirectories && !bExactMatch)
            continue;

        if (!wcscmp(findData.cFileName, L".") ||
            !wcscmp(findData.cFileName, L".."))
        {
            continue;
        }

        if (bRecurse && bIsDir && !bDirectories)
            continue;

        wcscpy(pszFileName, findData.cFileName);

        dwAttribute = (findData.dwFileAttributes & ~dwMask) | dwAttrib;

        SetFileAttributes(szFullName, dwAttribute);
        bFound = TRUE;
    }
    while (FindNextFileW(hFind, &findData));
    FindClose(hFind);

    return bFound;
}

int wmain(int argc, WCHAR *argv[])
{
    INT i;
    BOOL bRecurse = FALSE;
    BOOL bDirectories = FALSE;
    DWORD dwAttrib = 0;
    DWORD dwMask = 0;
    BOOL bFound = FALSE;
    PWSTR pszFileName;
    WCHAR szFilePath[MAX_PATH + 2] = L""; // + 2 to reserve an extra path separator and a NULL-terminator.

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Check for options and file specifications */
    for (i = 1; i < argc; i++)
    {
        if (*argv[i] == L'/')
        {
            /* Print help and bail out if needed */
            if (wcscmp(argv[i], L"/?") == 0)
            {
                ConResPuts(StdOut, STRING_ATTRIB_HELP);
                return 0;
            }
            else
            /* Retrieve the enumeration modes */
            if (wcsicmp(argv[i], L"/s") == 0)
                bRecurse = TRUE;
            else if (wcsicmp(argv[i], L"/d") == 0)
                bDirectories = TRUE;
            else
            {
                /* Unknown option */
                ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                return -1;
            }
        }
        else
        /* Build attributes and mask */
        if ((*argv[i] == L'+') || (*argv[i] == L'-'))
        {
            BOOL bAdd = (*argv[i] == L'+');

            if (wcslen(argv[i]) != 2)
            {
                ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                return -1;
            }

            switch (towupper(argv[i][1]))
            {
                case L'A':
                    dwMask |= FILE_ATTRIBUTE_ARCHIVE;
                    if (bAdd)
                        dwAttrib |= FILE_ATTRIBUTE_ARCHIVE;
                    else
                        dwAttrib &= ~FILE_ATTRIBUTE_ARCHIVE;
                    break;

                case L'S':
                    dwMask |= FILE_ATTRIBUTE_SYSTEM;
                    if (bAdd)
                        dwAttrib |= FILE_ATTRIBUTE_SYSTEM;
                    else
                        dwAttrib &= ~FILE_ATTRIBUTE_SYSTEM;
                    break;

                case L'H':
                    dwMask |= FILE_ATTRIBUTE_HIDDEN;
                    if (bAdd)
                        dwAttrib |= FILE_ATTRIBUTE_HIDDEN;
                    else
                        dwAttrib &= ~FILE_ATTRIBUTE_HIDDEN;
                    break;

                case L'R':
                    dwMask |= FILE_ATTRIBUTE_READONLY;
                    if (bAdd)
                        dwAttrib |= FILE_ATTRIBUTE_READONLY;
                    else
                        dwAttrib &= ~FILE_ATTRIBUTE_READONLY;
                    break;

                default:
                    ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[i]);
                    return -1;
            }
        }
        else
        {
            /* At least one file specification found */
            bFound = TRUE;
        }
    }

    /* If no file specification was found, operate on all files of the current directory */
    if (!bFound)
    {
        DWORD len = GetCurrentDirectoryW(_countof(szFilePath) - 2, szFilePath);
        if (szFilePath[len - 1] != L'\\')
        {
            szFilePath[len] = L'\\';
            szFilePath[len + 1] = UNICODE_NULL;
        }
        pszFileName = L"*.*";

        if (dwMask == 0)
            bFound = PrintAttribute(szFilePath, pszFileName, bRecurse, bDirectories);
        else
            bFound = ChangeAttribute(szFilePath, pszFileName, bRecurse, bDirectories, dwMask, dwAttrib);

        if (!bFound)
            ConResPrintf(StdOut, STRING_FILE_NOT_FOUND, pszFileName);

        return 0;
    }

    /* Operate on each file specification */
    for (i = 1; i < argc; i++)
    {
        /* Skip options */
        if (*argv[i] == L'/' || *argv[i] == L'+' || *argv[i] == L'-')
            continue;

        GetFullPathNameW(argv[i], _countof(szFilePath) - 2, szFilePath, &pszFileName);
        if (pszFileName)
        {
            /* Move the file part so as to separate and NULL-terminate the directory */
            MoveMemory(pszFileName + 1, pszFileName,
                       sizeof(szFilePath) - (pszFileName -szFilePath + 1) * sizeof(*szFilePath));
            *pszFileName++ = UNICODE_NULL;
        }
        else
        {
            pszFileName = L"";
        }

        if (dwMask == 0)
            bFound = PrintAttribute(szFilePath, pszFileName, bRecurse, bDirectories);
        else
            bFound = ChangeAttribute(szFilePath, pszFileName, bRecurse, bDirectories, dwMask, dwAttrib);

        if (!bFound)
            ConResPrintf(StdOut, STRING_FILE_NOT_FOUND, argv[i]);
    }

    return 0;
}
