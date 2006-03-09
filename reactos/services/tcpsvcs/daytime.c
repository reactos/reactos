/*
 *  ReactOS Services
 *  Copyright (C) 2005 ReactOS Team
 *
 * LICENCE:     GPL - See COPYING in the top level directory
 * PROJECT:     ReactOS simple TCP/IP services
 * FILE:        apps/utils/net/tcpsvcs/daytime.c
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

DWORD WINAPI DaytimeHandler(VOID* Sock_)
{
    struct tm *newtime;
    time_t aclock;
    TCHAR *pszTime;
    DWORD RetVal = 0;
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
        RetVal = -1;
    }
    
    _tprintf(_T("Terminating daytime thread\n"));
    ExitThread(RetVal);
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
