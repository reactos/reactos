#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>
#include "tcpsvcs.h"

DWORD WINAPI DaytimeHandler(VOID* Sock_)
{
    struct tm *newtime;
    time_t aclock;
    TCHAR *pszTime;
    DWORD Retval = 0;
    SOCKET Sock = (SOCKET)Sock_;
    
    time(&aclock);
    newtime = localtime(&aclock);
    pszTime = _tasctime(newtime);
    
    SendTime(Sock, pszTime);

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


BOOL SendTime(SOCKET Sock, TCHAR *time)
{
    INT StringSize = strlen(time);
    INT RetVal = send(Sock, time, sizeof(TCHAR) * StringSize, 0);
    
    if (RetVal == SOCKET_ERROR)
        return FALSE;

    _tprintf(("Connection closed by peer.\n"));
    return TRUE;
}
