#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>
#include "tcpsvcs.h"

//these need putting in an RC file.
TCHAR Quotes[][MAX_QUOTE_BUF] =
{
    _T("\"I have a penchant for mischief, property damage, stalking and cheesecake, of course\" - kjk hyperion"),
    _T("\"Wow! I fixed a setmenu bug.\" - jimtabor"),
    _T("\"if the code is broken though, your free to call it ur own\" - Alex Ionescu"),
    _T("\"i don't know about any bug; none exist; ReactOS is prefect\" - filip2307"),
    _T("\"if you were kernel code, cutler would rewrite you.\" - Alex Ionescu"),
    _T("\"Looks like Hartmut is cleaning out his WC. working copy, that is\" - WaxDragon")
    _T("\"don't question it ... it's clearly an optimization\" - arty")
};

DWORD WINAPI QotdHandler(VOID* Sock_)
{
    DWORD Retval = 0;
    SOCKET Sock;
    INT NumOfQuotes;

    Sock = (SOCKET)Sock_;
    NumOfQuotes = sizeof(Quotes) / MAX_QUOTE_BUF;

    SendQuote(Sock, Quotes[1]);

    _tprintf(_T("Shutting connection down...\n"));
    if (ShutdownConnection(Sock, FALSE))
        _tprintf(_T("Connection is down.\n"));
    else
    {
        _tprintf(_T("Connection shutdown failed\n"));
        Retval = 3;
    }
    _tprintf(_T("Terminating thread\n"));
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
