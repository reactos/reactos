#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>
#include "tcpsvcs.h"

DWORD WINAPI QotdHandler(VOID* Sock_)
{
    DWORD Retval = 0;
    SOCKET Sock;
    INT NumOfQuotes;
    INT QuoteToPrint;
    TCHAR Quote[160];

    Sock = (SOCKET)Sock_;

    NumOfQuotes = 70; // need to emurate the rc file to discover
                      // how many quotes are in there.

    /* randomise the quote */
    srand((unsigned int) time(0));
    QuoteToPrint = rand() % NumOfQuotes;

    LoadString(NULL, QuoteToPrint, Quote, sizeof(Quote)/sizeof(TCHAR));

    SendQuote(Sock, Quote);

    _tprintf(_T("Shutting connection down...\n"));
    if (ShutdownConnection(Sock, FALSE))
        _tprintf(_T("Connection is down.\n"));
    else
    {
        _tprintf(_T("Connection shutdown failed\n"));
        Retval = 3;
    }
    _tprintf(_T("Terminating qotd thread\n"));
    ExitThread(0);

    return Retval;
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
