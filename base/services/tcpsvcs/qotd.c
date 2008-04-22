/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/qotd.c
 * PURPOSE:     Sends a random quote to the client
 * COPYRIGHT:   Copyright 2005 - 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

static LPCTSTR lpFilePath = _T("\\drivers\\etc\\quotes");

static BOOL
SendQuote(SOCKET sock, char* Quote)
{
    INT strSize = strlen(Quote);
    if (send(sock, Quote, strSize, 0) == SOCKET_ERROR)
        return FALSE;

    return TRUE;
}

static BOOL
RetrieveQuote(SOCKET sock)
{
    HANDLE hFile;
    TCHAR lpFullPath[MAX_PATH + 20];
    DWORD dwBytesRead;
    LPSTR lpQuotes;
    LPSTR lpStr;
    DWORD quoteNum;
    DWORD NumQuotes = 0;
    INT i;

    if(!GetSystemDirectory(lpFullPath, MAX_PATH))
    {
        LogEvent(_T("QOTD: Getting system path failed"), GetLastError(), 0, LOG_FILE);
        return FALSE;
    }
    _tcscat(lpFullPath, lpFilePath);


    LogEvent(_T("QOTD: Opening quotes file"), 0, 0, LOG_FILE);
    hFile = CreateFile(lpFullPath,
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        LogEvent(_T("QOTD: Error opening quotes file"), GetLastError(), 0, LOG_FILE);
    }
    else
    {
        DWORD dwSize = GetFileSize(hFile, NULL);
        lpQuotes = (LPSTR)HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (!lpQuotes) 
        {
            CloseHandle(hFile);
            return FALSE;
        }

        ReadFile(hFile,
                 lpQuotes,
                 dwSize,
                 &dwBytesRead,
                 NULL);
        CloseHandle(hFile);

        lpQuotes[dwSize] = 0;

        if (dwBytesRead != dwSize)
            return FALSE;

        lpStr = lpQuotes;
        while (*lpStr)
        {
            if (*lpStr == '%')
                NumQuotes++;
            lpStr++;
        }

        /* pick a random quote */
        srand((unsigned int) GetTickCount());
        quoteNum = rand() % NumQuotes;

        /* retrieve the full quote */
        lpStr = lpQuotes;
        for (i = 1; i <= quoteNum; i++)
        {
            /* move past proceding quote */
            lpStr++;

            if (i == quoteNum)
            {
                LPSTR lpStart = lpStr;

                while (*lpStr != '%' && *lpStr != '\0')
                    lpStr++;

                *lpStr = 0;

                /* send the quote */
                if (!SendQuote(sock, lpStart))
                    LogEvent(_T("QOTD: Error sending data"), 0, 0, LOG_FILE);
                break;
            }
            else
            {
                while (*lpStr != '%' && *lpStr != '\0')
                    lpStr++;

                /* move past % and RN */
                lpStr += 3;
            }
        }

        return TRUE;
    }

    return FALSE;
}


DWORD WINAPI
QotdHandler(VOID* sock_)
{
    SOCKET sock = (SOCKET)sock_;
    DWORD retVal = 0;

    if (!RetrieveQuote(sock))
    {
        LogEvent(_T("QOTD: Error retrieving quote"), 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(_T("QOTD: Shutting connection down"), 0, 0, LOG_FILE);
    if (ShutdownConnection(sock, FALSE))
    {
        LogEvent(_T("QOTD: Connection is down"), 0, 0, LOG_FILE);
    }
    else
    {
        LogEvent(_T("QOTD: Connection shutdown failed"), 0, 0, LOG_FILE);
        LogEvent(_T("QOTD: Terminating thread"), 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(_T("QOTD: Terminating thread"), 0, 0, LOG_FILE);
    ExitThread(retVal);
}
