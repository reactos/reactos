/*
 *  LABEL.C - label internal command.
 *
 *
 *  History:
 *
 *    10-Dec-1998 (Eric Kohl)
 *        Started.
 *
 *    11-Dec-1998 (Eric Kohl)
 *        Finished.
 *
 *    19-Jan-1998 (Eric Kohl)
 *        Unicode ready!
 *
 *    28-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winnls.h>
#include <winuser.h>

#include <conutils.h>

#include "resource.h"

#define MAX_LABEL_LENGTH 32
#define MAX_DRIVE_LENGTH  2


static
VOID
ConFormatMessage(PCON_STREAM Stream, DWORD MessageId, ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, MessageId);
    ConMsgPrintfV(Stream,
                  FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  MessageId,
                  LANG_USER_DEFAULT,
                  &arg_ptr);
    va_end(arg_ptr);
}


static
VOID
ConInString(LPWSTR lpInput, DWORD dwLength)
{
    DWORD dwOldMode;
    DWORD dwRead = 0;
    HANDLE hFile;
    LPWSTR p;
    PCHAR pBuf;

    pBuf = (PCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength - 1);

    hFile = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hFile, &dwOldMode);

    SetConsoleMode(hFile, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    ReadFile(hFile, (PVOID)pBuf, dwLength - 1, &dwRead, NULL);

    MultiByteToWideChar(GetConsoleCP(), 0, pBuf, dwRead, lpInput, dwLength - 1);
    HeapFree(GetProcessHeap(), 0, pBuf);

    for (p = lpInput; *p; p++)
    {
        if (*p == L'\x0d')
        {
            *p = UNICODE_NULL;
            break;
        }
    }

    SetConsoleMode(hFile, dwOldMode);
}


static
BOOL
IsValidPathName(LPCWSTR pszPath)
{
    WCHAR szOldPath[MAX_PATH];
    BOOL  bResult;

    GetCurrentDirectoryW(MAX_PATH, szOldPath);
    bResult = SetCurrentDirectoryW(pszPath);

    SetCurrentDirectoryW(szOldPath);

    return bResult;
}


static
BOOL
PromptYesNo(VOID)
{
    WCHAR szOptions[4];
    WCHAR szInput[16];
    BOOL bResult = FALSE;

    LoadString(GetModuleHandle(NULL), STRING_LABEL_OPTIONS, szOptions, ARRAYSIZE(szOptions));

    for (;;)
    {
        ConPuts(StdOut, L"\n");
        ConResPuts(StdOut, STRING_LABEL_PROMPT);

        ConInString(szInput, ARRAYSIZE(szInput));

        if (towupper(szInput[0]) == szOptions[0])
        {
            bResult = TRUE;
            break;
        }
        else if (towupper(szInput[0]) == szOptions[1])
        {
            bResult = FALSE;
            break;
        }

        ConPuts(StdOut, L"\n");
    }

    ConPuts(StdOut, L"\n");

    return bResult;
}


int wmain(int argc, WCHAR *argv[])
{
    WCHAR szRootPath[] = L" :\\";
    WCHAR szBuffer[80];
    WCHAR szLabel[80];
    WCHAR szOldLabel[80];
    DWORD dwSerialNr;
    INT len, i;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* set empty label string */
    szLabel[0] = UNICODE_NULL;

    /* print help */
    if (argc > 1 && wcscmp(argv[1], L"/?") == 0)
    {
        ConResPuts(StdOut, STRING_LABEL_HELP);
        return 0;
    }

    if (argc > 1)
    {
        len = 0;
        for (i = 1; i < argc; i++)
        {
            if (i > 1)
                len++;
            len += wcslen(argv[i]);
        }

        if (len > MAX_LABEL_LENGTH + MAX_DRIVE_LENGTH)
        {
            ConResPuts(StdOut, STRING_ERROR_INVALID_LABEL);
            return 1;
        }

        for (i = 1; i < argc; i++)
        {
            if (i > 1)
                wcscat(szBuffer, L" ");
            wcscat(szBuffer, argv[i]);
        }
    }

    if (wcslen(szBuffer) > 0)
    {
        if (szBuffer[1] == L':')
        {
            szRootPath[0] = towupper(szBuffer[0]);
            wcscpy(szLabel, &szBuffer[2]);
        }
        else
        {
            wcscpy(szLabel, szBuffer);
        }
    }

    if (wcslen(szLabel) > MAX_LABEL_LENGTH)
    {
        ConResPuts(StdOut, STRING_ERROR_INVALID_LABEL);
        return 1;
    }

    if (szRootPath[0] == L' ')
    {
        /* get label of current drive */
        WCHAR szCurPath[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, szCurPath);
        szRootPath[0] = szCurPath[0];
    }

    /* check root path */
    if (!IsValidPathName(szRootPath))
    {
        ConResPuts(StdErr, STRING_ERROR_INVALID_DRIVE);
        return 1;
    }

    if (wcslen(szLabel) == 0)
    {
        GetVolumeInformationW(szRootPath, szOldLabel, ARRAYSIZE(szOldLabel), &dwSerialNr,
                              NULL, NULL, NULL, 0);

        /* print drive info */
        if (szOldLabel[0] != UNICODE_NULL)
        {
            ConResPrintf(StdOut, STRING_LABEL_TEXT1, towupper(szRootPath[0]), szOldLabel);
        }
        else
        {
            ConResPrintf(StdOut, STRING_LABEL_TEXT2, towupper(szRootPath[0]));
        }

        /* print the volume serial number */
        ConResPrintf(StdOut, STRING_LABEL_TEXT3, HIWORD(dwSerialNr), LOWORD(dwSerialNr));

        ConResPuts(StdOut, STRING_LABEL_TEXT4);

        ConInString(szLabel, ARRAYSIZE(szLabel));
        ConPuts(StdOut, L"\n");

        if (wcslen(szLabel) == 0)
        {
            if (PromptYesNo() == FALSE)
                return 0;
        }
    }

    if (!SetVolumeLabelW(szRootPath, szLabel))
    {
        ConFormatMessage(StdOut, GetLastError());
        return 1;
    }

    return 0;
}

/* EOF */
