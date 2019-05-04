/*
 * PROJECT:     ReactOS Find Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Prints all lines of a file that contain a string.
 * COPYRIGHT:   Copyright 1994-2002 Jim Hall (jhall@freedos.org)
 *              Copyright 2019 Paweł Cholewa (DaMcpg@protonmail.com)
 */

#include <windows.h>
#include <stdio.h>
#include <conutils.h>
#include <shlwapi.h> /* StrStrW and StrStrIW */

#include "resource.h"

#define FIND_LINE_BUFFER_SIZE 4096

static BOOL bInvertSearch = FALSE;
static BOOL bCountLines = FALSE;
static BOOL bDisplayLineNumbers = FALSE;
static BOOL bIgnoreCase = FALSE;
static BOOL bDoNotSkipOfflineFiles = FALSE;

/**
 * @name FindString
 * @implemented
 * 
 * Prints all lines of the stream that contain a string.
 * 
 * @param pStream
 * Stream to read from.
 * 
 * @param szFilePath
 * Filename to print in console. Can be NULL.
 * 
 * @param szSearchedString
 * String to search for.
 * 
 * @return
 * 0 if the string was found at least once, 1 otherwise.
 *
 */
static int FindString(FILE* pStream, LPWSTR szFilePath, LPWSTR szSearchedString)
{
    WCHAR szLineBuffer[FIND_LINE_BUFFER_SIZE];
    LONG lLineCount = 0;
    LONG lLineNumber = 0;
    BOOL bSubstringFound;
    int iReturnValue = 1;

    if (szFilePath != NULL)
    {
        /* Convert the filename to uppercase (for formatting) */
        CharUpperW(szFilePath);

        /* Print the file's header */
        ConPrintf(StdOut, L"\n---------- %s", szFilePath);

        if (bCountLines)
        {
            ConPrintf(StdOut, L": ");
        }
        else
        {
            ConPrintf(StdOut, L"\n");
        }
    }

    /* Loop through every line in the file */
    while (fgetws(szLineBuffer, sizeof(szLineBuffer), pStream) != NULL)
    {
        lLineCount++;

        if (bIgnoreCase)
        {
            bSubstringFound = StrStrIW(szLineBuffer, szSearchedString) != NULL;
        }
        else
        {
            bSubstringFound = StrStrW(szLineBuffer, szSearchedString) != NULL;
        }
        

        /* Check if this line can be counted */
        if (bSubstringFound != bInvertSearch)
        {
            iReturnValue = 0;

            if (bCountLines)
            {
                lLineNumber++;
            }
            else
            {
                /* Display the line on the screen */
                if (bDisplayLineNumbers)
                {
                    ConPrintf(StdOut, L"[%ld]", lLineCount);
                }
                ConPrintf(StdOut, L"%s", szLineBuffer);
            }
        }
    }

    if (bCountLines)
    {
        /* Print the matching line count */
        ConPrintf(StdOut, L"%ld\n", lLineNumber);
    }
    else if (szFilePath != NULL && iReturnValue == 0)
    {
        /* Print a newline for formatting */
        ConPrintf(StdOut, L"\n");
    }

    return iReturnValue;
}

int wmain(int argc, WCHAR* argv[])
{
    int i;
    int iReturnValue = 2;
    int iSearchedStringIndex = -1;

    BOOL bFoundFileParameter = FALSE;

    HANDLE hFindFileHandle;
    WIN32_FIND_DATAW FindData;

    FILE* pOpenedFile;

    ConInitStdStreams();

    if (argc == 1)
    {
        /* If no argument were provided by the user, display program usage and exit */
        ConResPuts(StdOut, IDS_USAGE);
        return 0;
    }

    /* Parse the command line arguments */
    for (i = 1; i < argc; i++)
    {
        /* Check if this argument contains a switch */
        if (lstrlenW(argv[i]) == 2 && argv[i][0] == L'/')
        {
            switch (argv[i][1])
            {
                case L'?':
                    ConResPuts(StdOut, IDS_USAGE);
                    return 0;
                case L'v':
                case L'V':
                    bInvertSearch = TRUE;
                    break;
                case L'c':
                case L'C':
                    bCountLines = TRUE;
                    break;
                case L'n':
                case L'N':
                    bDisplayLineNumbers = TRUE;
                    break;
                case L'i':
                case L'I':
                    bIgnoreCase = TRUE;
                    break;
                default:
                    /* Report invalid switch error */
                    ConResPuts(StdOut, IDS_INVALID_SWITCH);
                    return 2;
            }
        }
        else if (lstrlenW(argv[i]) > 2 && argv[i][0] == L'/')
        {
            /* Check if this parameter is /OFF or /OFFLINE */
            if (lstrcmpiW(argv[i], L"/off") == 0 || lstrcmpiW(argv[i], L"/offline") == 0)
            {
                bDoNotSkipOfflineFiles = TRUE;
            }
            else
            {
                /* Report invalid switch error */
                ConResPuts(StdOut, IDS_INVALID_SWITCH);
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
        ConResPuts(StdOut, IDS_USAGE);
        return 2;
    }

    if (bFoundFileParameter)
    {
        /* After the command line arguments were parsed, iterate through them again to get the filenames */
        for (i = 1; i < argc; i++)
        {
            /* If the value is a switch or the searched string, continue */
            if ((lstrlenW(argv[i]) > 0 && argv[i][0] == L'/') || i == iSearchedStringIndex)
            {
                continue;
            }

            hFindFileHandle = FindFirstFileW(argv[i], &FindData);
            if (hFindFileHandle == INVALID_HANDLE_VALUE)
            {
                ConResPrintf(StdOut, IDS_NO_SUCH_FILE, argv[i]);
                continue;
            }
            
            do
            {
                /* Check if the file contains offline attribute and should be skipped */
                if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) && !bDoNotSkipOfflineFiles)
                {
                    continue;
                }

                pOpenedFile = _wfopen(FindData.cFileName, L"r");
                if (pOpenedFile == NULL)
                {
                    ConResPrintf(StdOut, IDS_CANNOT_OPEN, FindData.cFileName);
                    continue;
                }

                if (FindString(pOpenedFile, FindData.cFileName, argv[iSearchedStringIndex]) == 0)
                {
                    iReturnValue = 0;
                }
                else if (iReturnValue != 0)
                {
                    iReturnValue = 1;
                }

                fclose(pOpenedFile);
            } while (FindNextFileW(hFindFileHandle, &FindData));

            FindClose(hFindFileHandle);
        }
    }
    else
    {
        FindString(stdin, NULL, argv[iSearchedStringIndex]);
    }
    
    return iReturnValue;
}
