/*
 *  TYPE.C - type internal command.
 *
 *  History:
 *
 *    07/08/1998 (John P. Price)
 *        started.
 *
 *    07/12/98 (Rob Lake)
 *        Changed error messages
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    07-Jan-1999 (Eric Kohl)
 *        Added support for quoted arguments (type "test file.dat").
 *        Cleaned up.
 *
 *    19-Jan-1999 (Eric Kohl)
 *        Unicode and redirection ready!
 *
 *    19-Jan-1999 (Paolo Pantaleo <paolopan@freemail.it>)
 *        Added multiple file support (copied from y.c)
 *
 *    30-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_TYPE

static BOOL
FileGetString(
    IN HANDLE hFile,
    OUT LPTSTR lpBuffer,
    IN LONG nBufferLength)
{
    PCHAR pString;
    DWORD dwRead;
    LONG len = 0;

#ifdef _UNICODE
    pString = cmd_alloc(nBufferLength);
#else
    pString = lpBuffer;
#endif

    if (ReadFile(hFile, pString, nBufferLength - 1, &dwRead, NULL))
    {
        /* Break at new line*/
        PCHAR end = memchr(pString, '\n', dwRead);
        len = dwRead;
        if (end)
        {
            len = (LONG)(end - pString) + 1;
            SetFilePointer(hFile, len - dwRead, NULL, FILE_CURRENT);
        }
    }

    if (!len)
    {
#ifdef _UNICODE
        cmd_free(pString);
#endif
        return FALSE;
    }

    pString[len++] = '\0';
#ifdef _UNICODE
    MultiByteToWideChar(OutputCodePage, 0, pString, -1, lpBuffer, len);
    cmd_free(pString);
#endif
    return TRUE;
}

static BOOL
DoTypeFile(
    IN LPTSTR FileName,
    IN HANDLE hConsoleOut,
    IN BOOL bNoFileName,
    IN BOOL bPaging)
{
    HANDLE hFile;
    BOOL   bIsFile;
    DWORD  dwFileSize;
    DWORD  dwFilePos;
    DWORD  dwRet;
    LPTSTR errmsg;
    TCHAR  buff[256];

    hFile = CreateFile(FileName,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        // FIXME: Use ErrorMessage() ?
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_IGNORE_INSERTS |
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&errmsg,
                      0,
                      NULL);
        ConErrPrintf(_T("%s - %s"), FileName, errmsg);
        LocalFree(errmsg);
        nErrorLevel = 1;
        return TRUE;
    }

    /*
     * When reading from a file, retrieve its original size, so that
     * we can stop reading it once we are beyond its original ending.
     * This allows avoiding an infinite read loop in case the output
     * of the file is redirected back to it.
     * If we read from somewhere else (device, ...) don't do anything;
     * we will stop when ReadFile() fails (e.g. when Ctrl-Z is seen...).
     */
    bIsFile = ((GetFileType(hFile) & ~FILE_TYPE_REMOTE) == FILE_TYPE_DISK);
    if (bIsFile)
    {
        dwFileSize = GetFileSize(hFile, NULL);
        if ((dwFileSize == INVALID_FILE_SIZE) &&
            (GetLastError() != ERROR_SUCCESS))
        {
            WARN("Error when retrieving file size, or size too large (%d)\n",
                 dwFileSize);
            dwFileSize = 0;
        }
        dwFilePos = SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        if ((dwFilePos == INVALID_SET_FILE_POINTER) &&
            (GetLastError() != ERROR_SUCCESS))
        {
            WARN("Error when setting file pointer\n");
            dwFilePos = 0;
        }
    }
    else
    {
        dwFileSize = dwFilePos = 0;
    }

    /*
     * Display the file name on StdErr if required, so that if StdOut
     * alone is redirected, we can obtain the file contents only.
     */
    if (!bNoFileName)
        ConErrPrintf(_T("\n%s\n\n\n"), FileName);

    if (bPaging)
    {
        while (FileGetString(hFile, buff, ARRAYSIZE(buff)))
        {
            if (!ConOutPrintfPaging(FALSE, _T("%s"), buff))
            {
                bCtrlBreak = FALSE;
                CloseHandle(hFile);
                nErrorLevel = 1;
                return FALSE;
            }

            /*
             * If we read from a file, check where we are and stop
             * once we are beyond the original end of the file.
             */
            if (bIsFile)
            {
                dwFilePos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
                if ((dwFilePos == INVALID_SET_FILE_POINTER) &&
                    (GetLastError() != ERROR_SUCCESS))
                {
                    WARN("Error when getting file pointer\n");
                    break;
                }
                if (dwFilePos >= dwFileSize)
                    break;
            }
        }
    }
    else
    {
        while (ReadFile(hFile, buff, sizeof(buff), &dwRet, NULL) && dwRet > 0)
        {
            WriteFile(hConsoleOut, buff, dwRet, &dwRet, NULL);
            if (bCtrlBreak)
            {
                bCtrlBreak = FALSE;
                CloseHandle(hFile);
                nErrorLevel = 1;
                return FALSE;
            }

            /*
             * If we read from a file, check where we are and stop
             * once we are beyond the original end of the file.
             */
            if (bIsFile)
            {
                dwFilePos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
                if ((dwFilePos == INVALID_SET_FILE_POINTER) &&
                    (GetLastError() != ERROR_SUCCESS))
                {
                    WARN("Error when getting file pointer\n");
                    break;
                }
                if (dwFilePos >= dwFileSize)
                    break;
            }
        }
    }

    CloseHandle(hFile);
    return TRUE;
}

INT cmd_type(LPTSTR param)
{
    INT argc, i;
    LPTSTR* argv;
    LPTSTR errmsg;
    HANDLE hConsoleOut;
    BOOL bNoFileName = FALSE;
    BOOL bPaging = FALSE;
    BOOL bFileFound;
    DWORD dwLastError;
    UINT nFileSpecs = 0;
    HANDLE hFind;
    WIN32_FIND_DATA FindData;

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_TYPE_HELP1);
        return 0;
    }

    if (!*param)
    {
        error_req_param_missing();
        return 1;
    }

    /* Parse the command line. We will manually expand any file specification present. */
    argv = split(param, &argc, FALSE, FALSE);

    /* Loop through the options, count also the specified number of file specifications */
    for (i = 0; i < argc; ++i)
    {
        if (argv[i][0] == _T('/'))
        {
            if (_tcslen(argv[i]) == 2)
            {
                switch (_totupper(argv[i][1]))
                {
                case _T('N'):
                    bNoFileName = TRUE;
                    continue;

                case _T('P'):
                    bPaging = TRUE;
                    continue;
                }
            }

            // error_invalid_switch(argv[i] + 1);
            ConErrResPrintf(STRING_TYPE_ERROR1, argv[i] + 1);
            nErrorLevel = 1;
            goto Quit;
        }

        /* This should be a file specification */
        ++nFileSpecs;
    }

    nErrorLevel = 0;

    hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Reset paging state */
    if (bPaging)
        ConOutPrintfPaging(TRUE, _T(""));

    /* Now loop through the files */
    for (i = 0; i < argc; ++i)
    {
        /* Skip the options */
        if (argv[i][0] == _T('/'))
            continue;

        /* If wildcards are present in this file specification, perform a file enumeration */
        if (_tcschr(argv[i], _T('*')) || _tcschr(argv[i], _T('?')))
        {
            dwLastError = ERROR_SUCCESS;
            bFileFound = FALSE;

            hFind = FindFirstFile(argv[i], &FindData);

            if (hFind != INVALID_HANDLE_VALUE)
            {
                /* Loop through all the files */
                do
                {
                    /* Ignore any directory silently */
                    if (!_tcscmp(FindData.cFileName, _T("."))  ||
                        !_tcscmp(FindData.cFileName, _T("..")) ||
                        (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        continue;
                    }

                    bFileFound = TRUE;
                    if (!DoTypeFile(FindData.cFileName, hConsoleOut, bNoFileName, bPaging))
                    {
                        FindClose(hFind);
                        goto Quit;
                    }

                } while (FindNextFile(hFind, &FindData));

                FindClose(hFind);
            }

            /*
             * Return an error if the file specification could not be resolved,
             * or no actual files were encountered (but only directories).
             */
            if (hFind == INVALID_HANDLE_VALUE)
                dwLastError = GetLastError();
            else if (!bFileFound)
                dwLastError = ERROR_FILE_NOT_FOUND;

            if (dwLastError != ERROR_SUCCESS)
            {
                // FIXME: Use ErrorMessage() ?
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                              FORMAT_MESSAGE_IGNORE_INSERTS |
                              FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              dwLastError,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPTSTR)&errmsg,
                              0,
                              NULL);
                ConErrPrintf(_T("%s - %s"), argv[i], errmsg);
                LocalFree(errmsg);
                nErrorLevel = 1;
            }
        }
        else
        {
            if (!DoTypeFile(argv[i], hConsoleOut, (bNoFileName || (nFileSpecs <= 1)), bPaging))
                goto Quit;
        }

        /* Continue with the next file specification */
    }

Quit:
    freep(argv);
    return nErrorLevel;
}

#endif
