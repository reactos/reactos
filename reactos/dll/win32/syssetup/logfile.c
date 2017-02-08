/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Log file functions
 * FILE:              lib/syssetup/logfile.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include <stdarg.h>

/* GLOBALS ******************************************************************/

HANDLE hLogFile = NULL;

#define FORMAT_BUFFER_SIZE 512
#define LINE_BUFFER_SIZE 1024

/* FUNCTIONS ****************************************************************/

BOOL WINAPI
InitializeSetupActionLog (BOOL bDeleteOldLogFile)
{
    WCHAR szFileName[MAX_PATH];

    GetWindowsDirectoryW(szFileName, MAX_PATH);

    if (szFileName[wcslen(szFileName)] != L'\\')
    {
        wcsncat(szFileName,
                L"\\",
                (sizeof(szFileName) / sizeof(szFileName[0])) - wcslen(szFileName));
    }
    wcsncat(szFileName,
            L"setuplog.txt",
            (sizeof(szFileName) / sizeof(szFileName[0])) - wcslen(szFileName));

    if (bDeleteOldLogFile)
    {
        SetFileAttributesW(szFileName, FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(szFileName);
    }

    hLogFile = CreateFileW(szFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (hLogFile == INVALID_HANDLE_VALUE)
    {
        hLogFile = NULL;
        return FALSE;
    }

    return TRUE;
}


VOID WINAPI
TerminateSetupActionLog(VOID)
{
    if (hLogFile != NULL)
    {
        CloseHandle (hLogFile);
        hLogFile = NULL;
    }
}


VOID
CDECL
pSetupDebugPrint(
    IN PCWSTR pszFileName,
    IN INT nLineNumber,
    IN PCWSTR pszTag,
    IN PCWSTR pszMessage,
    ...)
{
    PWSTR pszFormatBuffer = NULL;
    PWSTR pszLineBuffer = NULL;
    PSTR pszOutputBuffer = NULL;
    ULONG ulLineSize, ulOutputSize;
    DWORD dwWritten;
    SYSTEMTIME stTime;
    va_list args;

    if (hLogFile == NULL)
        return;

    GetLocalTime(&stTime);

    if (pszMessage)
    {
        pszFormatBuffer = HeapAlloc(GetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    FORMAT_BUFFER_SIZE * sizeof(WCHAR));
        if (pszFormatBuffer == NULL)
            goto done;

        va_start(args, pszMessage);
        vsnwprintf(pszFormatBuffer,
                   FORMAT_BUFFER_SIZE,
                   pszMessage,
                   args);
        va_end(args);
    }

    pszLineBuffer = HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              LINE_BUFFER_SIZE * sizeof(WCHAR));
    if (pszLineBuffer == NULL)
        goto done;

    _snwprintf(pszLineBuffer,
               LINE_BUFFER_SIZE,
               L"%02d/%02d/%04d %02d:%02d:%02d.%03d, %s, %d, %s, %s\r\n",
               stTime.wMonth,
               stTime.wDay,
               stTime.wYear,
               stTime.wHour,
               stTime.wMinute,
               stTime.wSecond,
               stTime.wMilliseconds,
               pszFileName ? pszFileName : L"",
               nLineNumber,
               pszTag ? pszTag : L"",
               pszFormatBuffer ? pszFormatBuffer : L"");

    /* Get length of the converted ansi string */
    ulLineSize = wcslen(pszLineBuffer) * sizeof(WCHAR);
    RtlUnicodeToMultiByteSize(&ulOutputSize,
                              pszLineBuffer,
                              ulLineSize);

    /* Allocate message string buffer */
    pszOutputBuffer = HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                ulOutputSize);
    if (pszOutputBuffer == NULL)
        goto done;

    /* Convert unicode to ansi */
    RtlUnicodeToMultiByteN(pszOutputBuffer,
                           ulOutputSize,
                           NULL,
                           pszLineBuffer,
                           ulLineSize);

    /* Set file pointer to the end of the file */
    SetFilePointer(hLogFile,
                   0,
                   NULL,
                   FILE_END);

    WriteFile(hLogFile,
              pszOutputBuffer,
              ulOutputSize,
              &dwWritten,
              NULL);

done:
    if (pszOutputBuffer)
        HeapFree(GetProcessHeap(), 0, pszOutputBuffer);

    if (pszLineBuffer)
        HeapFree(GetProcessHeap(), 0, pszLineBuffer);

    if (pszFormatBuffer)
        HeapFree(GetProcessHeap(), 0, pszFormatBuffer);
}

/* EOF */
