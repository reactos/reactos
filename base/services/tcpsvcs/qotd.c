/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/qotd.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "tcpsvcs.h"

#define QBUFSIZ 60

LPCTSTR FilePath = _T("\\drivers\\etc\\quotes"); /* 19 chars */

BOOL SendQuote(SOCKET Sock, char* Quote)
{
    INT StringSize;
    INT RetVal;

    StringSize = (INT)strlen(Quote);
    RetVal = send(Sock, Quote, sizeof(char) * StringSize, 0);

    if (RetVal == SOCKET_ERROR)
        return FALSE;

    LogEvent(_T("QOTD: Connection closed by peer\n"), 0, FALSE);
    return TRUE;
}


DWORD WINAPI QotdHandler(VOID* Sock_)
{
    FILE *fp;
    SOCKET Sock;
    TCHAR Sys[MAX_PATH + 20];
    char Quote[QBUFSIZ][BUFSIZ]; // need to set this dynamically
    INT QuoteToPrint;
    INT NumQuotes;

    Sock = (SOCKET)Sock_;

    if(! GetSystemDirectory(Sys, MAX_PATH))
    {
    	LogEvent(_T("QOTD: Getting system path failed.\n"), 0, TRUE);
    	ExitThread(1);
    }

    _tcsncat(Sys, FilePath, _tcslen(FilePath));

    LogEvent(_T("QOTD: Opening quotes file\n"), 0, FALSE);
    if ((fp = _tfopen(Sys, _T("r"))) == NULL)
    {
		TCHAR buf[320];

		_sntprintf(buf, 320, _T("QOTD: Error opening quote file : %s\n"), Sys);
        LogEvent(buf, 0, TRUE);
        LogEvent(_T("QOTD: Terminating thread\n"), 0, FALSE);
        ExitThread(1);
    }

    /* read all quotes in the file into an array */
    NumQuotes = 0;
    while ((fgets(Quote[NumQuotes], QBUFSIZ, fp) != NULL) &&
           (NumQuotes != QBUFSIZ))
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

}
