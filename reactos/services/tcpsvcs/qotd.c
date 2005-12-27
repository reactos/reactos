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

LPCTSTR FilePath = _T("\\drivers\\etc\\quotes");

DWORD WINAPI QotdHandler(VOID* Sock_)
{
    FILE *fp;
    SOCKET Sock;
    TCHAR Sys[MAX_PATH];
    TCHAR Quote[60][BUFSIZ]; // need to set this dynamically
    INT QuoteToPrint;
    INT NumQuotes;

    Sock = (SOCKET)Sock_;

    if(! GetSystemDirectory(Sys, MAX_PATH))
    {
    	LogEvent(_T("QOTD: Getting system path failed.\n"), 0, TRUE);
    	ExitThread(1);
    }
    
    _tcscat(Sys, FilePath);

    LogEvent(_T("QOTD: Opening quotes file\n"), 0, FALSE);
    if ((fp = _tfopen(Sys, "r")) == NULL)
    {
		TCHAR buf[256];

		_stprintf(buf, _T("QOTD: Error opening quote file : %s\n"), Sys);
        LogEvent(buf, 0, TRUE);
        LogEvent(_T("QOTD: Terminating thread\n"), 0, FALSE);
        ExitThread(1);
    }

    /* read all quotes in the file into an array */
    NumQuotes = 0;
    while (_fgetts(Quote[NumQuotes], QBUFSIZ, fp) != NULL)
        NumQuotes++;

    LogEvent(_T("QOTD: Closing quotes file\n"), 0, FALSE);
    fclose(fp);

    /* randomise the quote */
    srand((unsigned int) time(0));
    QuoteToPrint = rand() % NumQuotes;

    if (!SendQuote(Sock, Quote[QuoteToPrint]))
        LogEvent(_T("QOTD: Error sending data\n"), 0, TRUE);


    LogEvent(_T("QOTD: Shutting connection down...\n"), 0, FALSE);
    if (ShutdownConnection(Sock, FALSE))
        LogEvent(_T("QOTD: Connection is down\n"), 0, FALSE);
    else
    {
        LogEvent(_T("QOTD: Connection shutdown failed\n"), 0, FALSE);
        LogEvent(_T("QOTD: Terminating thread\n"), 0, FALSE);
        ExitThread(1);
    }
    
    LogEvent(_T("QOTD: Terminating thread\n"), 0, FALSE);
    ExitThread(0);

    //return Retval;
}


BOOL SendQuote(SOCKET Sock, TCHAR* Quote)
{
    INT StringSize;
    INT RetVal;

    StringSize = (INT)_tcsclen(Quote);
    RetVal = send(Sock, Quote, sizeof(TCHAR) * StringSize, 0);

    if (RetVal == SOCKET_ERROR)
        return FALSE;

    LogEvent(_T("QOTD: Connection closed by peer\n"), 0, FALSE);
    return TRUE;
}
