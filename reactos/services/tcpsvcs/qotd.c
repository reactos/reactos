/*
 *  ReactOS Services
 *  Copyright (C) 2005 ReactOS Team
 *
 * LICENCE:     GPL - See COPYING in the top level directory
 * PROJECT:     ReactOS simple TCP/IP services
 * FILE:        apps/utils/net/tcpsvcs/qotd.c
  * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *   GM 04/10/05 Created
 *
 */

#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>
#include "tcpsvcs.h"

#define QBUFSIZ 160
#define NUMQUOTES 60

LPCTSTR FilePath = _T("\\drivers\\etc\\quotes");

DWORD WINAPI QotdHandler(VOID* Sock_)
{
    FILE *fp;
    SOCKET Sock;
    TCHAR Sys[MAX_PATH];
    TCHAR Quote[NUMQUOTES][BUFSIZ]; // need to set this dynamically
    INT QuoteToPrint;
    INT i = 0;

    Sock = (SOCKET)Sock_;

    if(! GetSystemDirectory(Sys, MAX_PATH))
    	_tprintf(_T("Getting system path failed. Error: %lu\n"), GetLastError());
    
    _tcscat(Sys, FilePath);

    _tprintf(_T("Opening quotes file\n"));
    if ((fp = _tfopen(Sys, "r")) == NULL)
    {
        _tprintf(_T("Error opening file: %lu\n"), GetLastError());
        _tprintf(_T("Terminating qotd thread\n"));
        ExitThread(-1);
    }

    while (_fgetts(Quote[i], QBUFSIZ, fp) != NULL)
        i++;

    _tprintf(_T("Closing quotes file\n"));
    fclose(fp);

    /* randomise the quote */
    srand((unsigned int) time(0));
    QuoteToPrint = rand() % NUMQUOTES;

    if (!SendQuote(Sock, Quote[QuoteToPrint]))
    {
        _tprintf(_T("Error sending data. Error: %x\n"), WSAGetLastError());
    }

    _tprintf(_T("Shutting connection down...\n"));
    if (ShutdownConnection(Sock, FALSE))
        _tprintf(_T("Connection is down.\n"));
    else
    {
        _tprintf(_T("Connection shutdown failed\n"));
        _tprintf(_T("Terminating qotd thread\n"));
        ExitThread(-1);
    }
    
    _tprintf(_T("Terminating qotd thread\n"));
    ExitThread(0);

    //return Retval;
}


BOOL SendQuote(SOCKET Sock, TCHAR* Quote)
{
    INT StringSize;
    INT RetVal;

    StringSize = strlen(Quote);
    RetVal = send(Sock, Quote, sizeof(TCHAR) * StringSize, 0);

    if (RetVal == SOCKET_ERROR)
        return FALSE;

    _tprintf(("Connection closed by peer.\n"));
    return TRUE;
}
