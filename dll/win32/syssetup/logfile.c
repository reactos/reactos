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

/* GLOBALS ******************************************************************/

HANDLE hLogFile = NULL;

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


BOOL WINAPI
SYSSETUP_LogItem(IN const LPSTR lpFileName,
                 IN DWORD dwLineNumber,
                 IN DWORD dwSeverity,
                 IN LPWSTR lpMessageText)
{
    LPCSTR lpSeverityString;
    LPSTR lpMessageString;
    DWORD dwMessageLength;
    DWORD dwMessageSize;
    DWORD dwWritten;
    CHAR Buffer[6];
    CHAR TimeBuffer[30];
    SYSTEMTIME stTime;

    /* Get the severity code string */
    switch (dwSeverity)
    {
        case SYSSETUP_SEVERITY_INFORMATION:
            lpSeverityString = "Information : ";
            break;

        case SYSSETUP_SEVERITY_WARNING:
            lpSeverityString = "Warning : ";
            break;

        case SYSSETUP_SEVERITY_ERROR:
            lpSeverityString = "Error : ";
            break;

        case SYSSETUP_SEVERITY_FATAL_ERROR:
            lpSeverityString = "Fatal error : ";
            break;

        default:
            lpSeverityString = "Unknown : ";
            break;
    }

    /* Get length of the converted ansi string */
    dwMessageLength = wcslen(lpMessageText) * sizeof(WCHAR);
    RtlUnicodeToMultiByteSize(&dwMessageSize,
                              lpMessageText,
                              dwMessageLength);

    /* Allocate message string buffer */
    lpMessageString = (LPSTR) HeapAlloc(GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        dwMessageSize);
    if (!lpMessageString)
        return FALSE;

    /* Convert unicode to ansi */
    RtlUnicodeToMultiByteN(lpMessageString,
                           dwMessageSize,
                           NULL,
                           lpMessageText,
                           dwMessageLength);

    /* Set file pointer to the end of the file */
    SetFilePointer(hLogFile,
                   0,
                   NULL,
                   FILE_END);

    /* Write Time/Date */
    GetLocalTime(&stTime);

    snprintf(TimeBuffer, sizeof(TimeBuffer),
             "%02d/%02d/%02d %02d:%02d:%02d.%03d",
             stTime.wMonth,
             stTime.wDay,
             stTime.wYear,
             stTime.wHour,
             stTime.wMinute,
             stTime.wSecond,
             stTime.wMilliseconds);

    WriteFile(hLogFile,
              TimeBuffer,
              strlen(TimeBuffer),
              &dwWritten,
              NULL);

    /* Write comma */
    WriteFile(hLogFile, ",", 1, &dwWritten, NULL);

    /* Write file name */
    WriteFile(hLogFile,
              lpFileName,
              strlen(lpFileName),
              &dwWritten,
              NULL);

    /* Write comma */
    WriteFile(hLogFile, ",", 1, &dwWritten, NULL);

    /* Write line number */
    snprintf(Buffer, sizeof(Buffer), "%lu", dwLineNumber);
    WriteFile(hLogFile,
              Buffer,
              strlen(Buffer),
              &dwWritten,
              NULL);

    /* Write comma */
    WriteFile(hLogFile, ",", 1, &dwWritten, NULL);

    /* Write severity code */
    WriteFile(hLogFile,
              lpSeverityString,
              strlen(lpSeverityString),
              &dwWritten,
              NULL);

    /* Write message string */
    WriteFile(hLogFile,
              lpMessageString,
              dwMessageSize,
              &dwWritten,
              NULL);

    /* Write newline */
    WriteFile(hLogFile, "\r\n", 2, &dwWritten, NULL);

    HeapFree(GetProcessHeap(),
             0,
             lpMessageString);

    return TRUE;
}

/* EOF */
