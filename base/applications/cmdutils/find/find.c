/*
 * PROJECT:     ReactOS Find Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Prints all lines of a file that contain a string.
 * COPYRIGHT:   Copyright 1994-2002 Jim Hall (jhall@freedos.org)
 *              Copyright 2019 Paweł Cholewa (DaMcpg@protonmail.com)
 *              Copyright 2019 Hermes Belusca-Maito
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winuser.h>

#include <conutils.h>
#include <strsafe.h>

#include "resource.h"

#define FIND_LINE_BUFFER_SIZE 4096

static BOOL bInvertSearch = FALSE;
static BOOL bCountLines = FALSE;
static BOOL bDisplayLineNumbers = FALSE;
static BOOL bIgnoreCase = FALSE;
static BOOL bDoNotSkipOfflineFiles = FALSE;

/**
 * @name StrStrCase
 * @implemented
 *
 * Locates a substring inside a NULL-terminated wide string.
 *
 * @param[in] pszStr
 *     The NULL-terminated string to be scanned.
 *
 * @param[in] pszSearch
 *     The NULL-terminated string to search for.
 *
 * @param[in] bIgnoreCase
 *     TRUE if case has to be ignored, FALSE otherwise.
 *
 * @return
 *     Returns a pointer to the first occurrence of pszSearch in pszStr,
 *     or NULL if pszSearch does not appear in pszStr. If pszSearch points
 *     to a string of zero length, the function returns pszStr.
 */
static PWSTR
StrStrCase(
    IN PCWSTR pszStr,
    IN PCWSTR pszSearch,
    IN BOOL bIgnoreCase)
{
    if (bIgnoreCase)
    {
        LCID LocaleId;
        INT i, cch1, cch2;

        LocaleId = GetThreadLocale();

        cch1 = wcslen(pszStr);
        cch2 = wcslen(pszSearch);

        if (cch2 == 0)
            return (PWSTR)pszStr;

        for (i = 0; i <= cch1 - cch2; ++i)
        {
            if (CompareStringW(LocaleId /* LOCALE_SYSTEM_DEFAULT */,
                               NORM_IGNORECASE /* | NORM_LINGUISTIC_CASING */,
                               pszStr + i, cch2, pszSearch, cch2) == CSTR_EQUAL)
            {
                return (PWSTR)(pszStr + i);
            }
        }
        return NULL;
    }
    else
    {
        return wcsstr(pszStr, pszSearch);
    }
}

/**
 * @name FindString
 * @implemented
 *
 * Prints all lines of the stream that contain a string.
 *
 * @param[in] pStream
 *     The stream to read from.
 *
 * @param[in] pszFilePath
 *     The file name to print out. Can be NULL.
 *
 * @param[in] pszSearchString
 *     The NULL-terminated string to search for.
 *
 * @return
 *     0 if the string was found at least once, 1 otherwise.
 */
static int
FindString(
    IN FILE* pStream,
    IN PCWSTR pszFilePath OPTIONAL,
    IN PCWSTR pszSearchString)
{
    LONG lLineCount = 0;
    LONG lLineNumber = 0;
    BOOL bSubstringFound;
    int iReturnValue = 1;
    WCHAR szLineBuffer[FIND_LINE_BUFFER_SIZE];

    if (pszFilePath != NULL)
    {
        /* Print the file's header */
        ConPrintf(StdOut, L"\n---------- %s%s",
                  pszFilePath, bCountLines ? L": " : L"\n");
    }

    /* Loop through every line in the file */
    // FIXME: What if the string we search for crosses the boundary of our szLineBuffer ?
    while (fgetws(szLineBuffer, _countof(szLineBuffer), pStream) != NULL)
    {
        ++lLineNumber;

        bSubstringFound = (StrStrCase(szLineBuffer, pszSearchString, bIgnoreCase) != NULL);

        /* Check if this line can be counted */
        if (bSubstringFound != bInvertSearch)
        {
            iReturnValue = 0;

            if (bCountLines)
            {
                ++lLineCount;
            }
            else
            {
                /* Display the line number if needed */
                if (bDisplayLineNumbers)
                {
                    ConPrintf(StdOut, L"[%ld]", lLineNumber);
                }
                ConPrintf(StdOut, L"%s", szLineBuffer);
            }
        }
    }

    if (bCountLines)
    {
        /* Print the matching line count */
        ConPrintf(StdOut, L"%ld\n", lLineCount);
    }
#if 0
    else if (pszFilePath != NULL && iReturnValue == 0)
    {
        /* Print a newline for formatting */
        ConPrintf(StdOut, L"\n");
    }
#endif

    return iReturnValue;
}

int wmain(int argc, WCHAR* argv[])
{
    int i;
    int iReturnValue = 2;
    int iSearchedStringIndex = -1;
    BOOL bFoundFileParameter = FALSE;
    HANDLE hFindFile;
    WIN32_FIND_DATAW FindData;
    FILE* pOpenedFile;
    PWCHAR ptr;
    WCHAR szFullFilePath[MAX_PATH];

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (argc == 1)
    {
        /* If no argument were provided by the user, display program usage and exit */
        ConResPuts(StdOut, IDS_USAGE);
        return 0;
    }

    /* Parse the command line arguments */
    for (i = 1; i < argc; ++i)
    {
        /* Check if this argument contains a switch */
        if (wcslen(argv[i]) == 2 && argv[i][0] == L'/')
        {
            switch (towupper(argv[i][1]))
            {
                case L'?':
                    ConResPuts(StdOut, IDS_USAGE);
                    return 0;
                case L'V':
                    bInvertSearch = TRUE;
                    break;
                case L'C':
                    bCountLines = TRUE;
                    break;
                case L'N':
                    bDisplayLineNumbers = TRUE;
                    break;
                case L'I':
                    bIgnoreCase = TRUE;
                    break;
                default:
                    /* Report invalid switch error */
                    ConResPuts(StdErr, IDS_INVALID_SWITCH);
                    return 2;
            }
        }
        else if (wcslen(argv[i]) > 2 && argv[i][0] == L'/')
        {
            /* Check if this parameter is /OFF or /OFFLINE */
            if (_wcsicmp(argv[i], L"/off") == 0 || _wcsicmp(argv[i], L"/offline") == 0)
            {
                bDoNotSkipOfflineFiles = TRUE;
            }
            else
            {
                /* Report invalid switch error */
                ConResPuts(StdErr, IDS_INVALID_SWITCH);
                return 2;
            }
        }
        else
        {
            if (iSearchedStringIndex == -1)
            {
                iSearchedStringIndex = i;
            }
            else
            {
                /* There's a file specified in the parameters, no need to read from stdin */
                bFoundFileParameter = TRUE;
            }
        }
    }

    if (iSearchedStringIndex == -1)
    {
        /* User didn't provide the string to search for, display program usage and exit */
        ConResPuts(StdErr, IDS_USAGE);
        return 2;
    }

    if (bFoundFileParameter)
    {
        /* After the command line arguments were parsed, iterate through them again to get the filenames */
        for (i = 1; i < argc; ++i)
        {
            /* If the value is a switch or the searched string, continue */
            if ((wcslen(argv[i]) > 0 && argv[i][0] == L'/') || i == iSearchedStringIndex)
            {
                continue;
            }

            hFindFile = FindFirstFileW(argv[i], &FindData);
            if (hFindFile == INVALID_HANDLE_VALUE)
            {
                ConResPrintf(StdErr, IDS_NO_SUCH_FILE, argv[i]);
                continue;
            }

            do
            {
                /* Check if the file contains offline attribute and should be skipped */
                if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) && !bDoNotSkipOfflineFiles)
                {
                    continue;
                }

                /* Skip directory */
                // FIXME: Implement recursivity?
                if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    continue;
                }

                /*
                 * Build the full file path from the file specification pattern.
                 *
                 * Note that we could use GetFullPathName() instead, however
                 * we want to keep compatibility with Windows' find.exe utility
                 * that does not use this function as it keeps the file name
                 * directly based on the pattern.
                 */
                ptr = wcsrchr(argv[i], L'\\');    // Check for last directory.
                if (!ptr)
                    ptr = wcsrchr(argv[i], L':'); // Check for drive.
                if (ptr)
                {
                    /* The pattern contains a drive or directory part: keep it and concatenate the full file name */
                    StringCchCopyNW(szFullFilePath, _countof(szFullFilePath),
                                    argv[i], ptr + 1 - argv[i]);
                    StringCchCatW(szFullFilePath, _countof(szFullFilePath),
                                  FindData.cFileName);
                }
                else
                {
                    /* The pattern does not contain any drive or directory part: just copy the full file name */
                    StringCchCopyW(szFullFilePath, _countof(szFullFilePath),
                                   FindData.cFileName);
                }

                // FIXME: Windows' find.exe supports searching inside binary files.
                pOpenedFile = _wfopen(szFullFilePath, L"r");
                if (pOpenedFile == NULL)
                {
                    ConResPrintf(StdErr, IDS_CANNOT_OPEN, szFullFilePath);
                    continue;
                }

                /* NOTE: Convert the file path to uppercase for formatting */
                if (FindString(pOpenedFile, _wcsupr(szFullFilePath), argv[iSearchedStringIndex]) == 0)
                {
                    iReturnValue = 0;
                }
                else if (iReturnValue != 0)
                {
                    iReturnValue = 1;
                }

                fclose(pOpenedFile);
            } while (FindNextFileW(hFindFile, &FindData));

            FindClose(hFindFile);
        }
    }
    else
    {
        FindString(stdin, NULL, argv[iSearchedStringIndex]);
    }

    return iReturnValue;
}
