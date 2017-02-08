/*
 * PROJECT:         ReactOS Subst Command
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/subst/subst.c
 * PURPOSE:         Maps a path with a drive letter
 * PROGRAMMERS:     Sam Arun Raj
 *                  Peter Hater
 *                  Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>

#include <conutils.h>

#include "resource.h"

/* FUNCTIONS ****************************************************************/

VOID PrintError(IN DWORD ErrCode)
{
    // DWORD dwLength = 0;
    PWSTR pMsgBuf  = NULL;

#if 0
    if (ErrCode == ERROR_SUCCESS)
        return;
#endif

    /* Retrieve the message string without appending extra newlines */
    // dwLength =
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                   NULL,
                   ErrCode,
                   LANG_USER_DEFAULT,
                   (PWSTR)&pMsgBuf,
                   0, NULL);
    if (pMsgBuf /* && dwLength */)
    {
        ConResPrintf(StdErr, IDS_FAILED_WITH_ERRORCODE,
                     ErrCode, pMsgBuf);
        LocalFree(pMsgBuf);
    }
}

ULONG QuerySubstedDrive(IN WCHAR DriveLetter,
                        IN OUT PWSTR* TargetPath OPTIONAL,
                        IN OUT PULONG Size)
{
    ULONG Result = ERROR_INVALID_DRIVE;
    WCHAR Drive[] = L"A:";
    DWORD dwSize, CharCount = 0;
    PWSTR lpTargetPath = NULL, tmp;

    Drive[0] = DriveLetter;

    /* Check whether the user has given a pointer to a target path buffer */
    if (!TargetPath)
    {
        /* No, therefore use a local buffer */
        dwSize = MAX_PATH;
        lpTargetPath = (PWSTR)HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
        if (!lpTargetPath)
            return ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        /* Just use the user-given pointer to a buffer; Size should point to a valid ULONG */
        if (!Size)
            return ERROR_INVALID_PARAMETER;

        lpTargetPath = *TargetPath;
        dwSize = *Size;
    }

Retry:
    /* Try querying DOS device information */
    CharCount = QueryDosDeviceW(Drive, lpTargetPath, dwSize);
    if (!CharCount)
        Result = GetLastError();

    if (!CharCount && (Result == ERROR_INSUFFICIENT_BUFFER))
    {
        /* Reallocate the buffer with double size */
        dwSize *= 2;
        tmp = (PWSTR)HeapReAlloc(GetProcessHeap(), 0, lpTargetPath, dwSize * sizeof(WCHAR));
        if (!tmp)
        {
            /* Memory problem, bail out */
            CharCount = 0;
            Result = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            /* Retry again */
            lpTargetPath = tmp;
            goto Retry;
        }
    }

    if (CharCount)
    {
        if ( wcsncmp(lpTargetPath, L"\\??\\", 4) == 0 &&
             ( (lpTargetPath[4] >= L'A' && lpTargetPath[4] <= L'Z') ||
               (lpTargetPath[4] >= L'a' && lpTargetPath[4] <= L'z') ) )
        {
            /* The drive exists and is SUBSTed */
            Result = ERROR_IS_SUBSTED;
        }
#if 0
        else
        {
            /* The drive exists but is not SUBSTed */
            Result = ERROR_INVALID_DRIVE;
        }
#endif
    }

    if (!TargetPath)
    {
        /* Free the local buffer */
        HeapFree(GetProcessHeap(), 0, lpTargetPath);
    }
    else
    {
        /* Update the user-given pointers */
        *TargetPath = lpTargetPath;
        *Size = dwSize;
    }

    return Result;
}

VOID DumpSubstedDrives(VOID)
{
    WCHAR DriveLetter;
    PWSTR lpTargetPath = NULL;
    DWORD dwSize;
    UCHAR i = 0;

    dwSize = MAX_PATH;
    lpTargetPath = (PWSTR)HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
    if (!lpTargetPath)
        return;

    while (i < 26)
    {
        DriveLetter = L'A' + i;
        if (QuerySubstedDrive(DriveLetter, &lpTargetPath, &dwSize) == ERROR_IS_SUBSTED)
        {
            ConPrintf(StdOut, L"%c:\\: => %s\n", DriveLetter, lpTargetPath + 4);
        }

        i++;
    }

    HeapFree(GetProcessHeap(), 0, lpTargetPath);
}

INT DeleteSubst(IN PWSTR Drive)
{
    DWORD dwResult;

    if ((wcslen(Drive) != 2) || (Drive[1] != L':'))
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto Quit;
    }

    if (QuerySubstedDrive(Drive[0], NULL, NULL) != ERROR_IS_SUBSTED)
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto Quit;
    }

    if (!DefineDosDeviceW(DDD_REMOVE_DEFINITION, Drive, NULL))
        dwResult = GetLastError();
    else
        dwResult = ERROR_SUCCESS;

Quit:
    switch (dwResult)
    {
        case ERROR_SUCCESS:
            break;

        // case ERROR_INVALID_DRIVE:
        case ERROR_INVALID_PARAMETER:
        {
            ConResPrintf(StdErr, IDS_INVALID_PARAMETER2, Drive);
            return 1;
        }

        case ERROR_ACCESS_DENIED:
        {
            ConResPrintf(StdErr, IDS_ACCESS_DENIED, Drive);
            return 1;
        }

        default:
        {
            PrintError(GetLastError());
            return 1;
        }
    }

    return 0;
}

INT AddSubst(IN PWSTR Drive, IN PWSTR Path)
{
    DWORD dwResult, dwPathAttr;

    if ((wcslen(Drive) != 2) || (Drive[1] != L':'))
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto Quit;
    }

    /*
     * Even if DefineDosDevice allows to map files to drive letters (yes yes!!)
     * it is not the purpose of SUBST to allow that. Therefore check whether
     * the given path exists and really is a path to a directory, and if not,
     * just fail with an error.
     */
    dwPathAttr = GetFileAttributesW(Path);
    if ( (dwPathAttr == INVALID_FILE_ATTRIBUTES) ||
        !(dwPathAttr & FILE_ATTRIBUTE_DIRECTORY) )
    {
        dwResult = ERROR_PATH_NOT_FOUND;
        goto Quit;
    }

    /*
     * QuerySubstedDrive (via QueryDosDevice) returns ERROR_FILE_NOT_FOUND only
     * if there is no already existing drive mapping. For all other results
     * (existing drive, be it already subst'ed or not, or other errors...)
     * no attempt at defining a drive mapping should be done.
     */
    dwResult = QuerySubstedDrive(Drive[0], NULL, NULL);
    if (dwResult != ERROR_FILE_NOT_FOUND)
        goto Quit;

    if (!DefineDosDeviceW(0, Drive, Path))
        dwResult = GetLastError();
    else
        dwResult = ERROR_SUCCESS;

Quit:
    switch (dwResult)
    {
        case ERROR_SUCCESS:
            break;

        case ERROR_INVALID_DRIVE:
        case ERROR_INVALID_PARAMETER:
        {
            ConResPrintf(StdErr, IDS_INVALID_PARAMETER2, Drive);
            return 1;
        }

        case ERROR_IS_SUBSTED:
        {
            ConResPuts(StdErr, IDS_DRIVE_ALREADY_SUBSTED);
            return 1;
        }

        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        {
            ConResPrintf(StdErr, IDS_PATH_NOT_FOUND, Path);
            return 1;
        }

        case ERROR_ACCESS_DENIED:
        {
            ConResPrintf(StdErr, IDS_ACCESS_DENIED, Path);
            return 1;
        }

        default:
        {
            PrintError(GetLastError());
            return 1;
        }
    }

    return 0;
}

int wmain(int argc, WCHAR* argv[])
{
    INT i;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    for (i = 0; i < argc; i++)
    {
        if (!_wcsicmp(argv[i], L"/?"))
        {
            ConResPuts(StdOut, IDS_USAGE);
            return 0;
        }
    }

    if (argc < 3)
    {
        if (argc >= 2)
        {
            ConResPrintf(StdErr, IDS_INVALID_PARAMETER, argv[1]);
            return 1;
        }
        DumpSubstedDrives();
        return 0;
    }

    if (argc > 3)
    {
        ConResPrintf(StdErr, IDS_INCORRECT_PARAMETER_COUNT, argv[3]);
        return 1;
    }

    if (!_wcsicmp(argv[1], L"/D"))
        return DeleteSubst(argv[2]);
    if (!_wcsicmp(argv[2], L"/D"))
        return DeleteSubst(argv[1]);
    return AddSubst(argv[1], argv[2]);
}

/* EOF */
