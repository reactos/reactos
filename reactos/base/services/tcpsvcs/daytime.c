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

    LogEvent(_T("DayTime: Shutting connection down...\n"), 0, FALSE);
    if (ShutdownConnection(Sock, FALSE))
        LogEvent(_T("DayTime: Connection is down.\n"), 0, FALSE);
    else
    {
        LogEvent(_T("DayTime: Connection shutdown failed\n"), 0, FALSE);
        RetVal = 1;
    }
    
    LogEvent(_T("DayTime: Terminating thread\n"), 0, FALSE);
    ExitThread(RetVal);
}


BOOL SendTime(SOCKET Sock, TCHAR *time)
{
    INT StringSize = (INT)_tcsclen(time);
    INT RetVal = send(Sock, time, sizeof(TCHAR) * StringSize, 0);
    
    if (RetVal == SOCKET_ERROR)
        return FALSE;

    LogEvent(_T("DayTime: Connection closed by peer.\n"), 0, FALSE);
    return TRUE;
}
