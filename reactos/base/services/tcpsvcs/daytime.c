/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/daytime.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "tcpsvcs.h"

BOOL SendTime(SOCKET Sock, CHAR *time)
{
    INT StringSize = (INT)strlen(time);
    INT RetVal = send(Sock, time, sizeof(CHAR) * StringSize, 0);

    if (RetVal == SOCKET_ERROR)
        return FALSE;

    LogEvent(_T("DayTime: Connection closed by peer.\n"), 0, FALSE);
    return TRUE;
}


DWORD WINAPI DaytimeHandler(VOID* Sock_)
{
    struct tm *newtime;
    time_t aclock;
    CHAR *pszTime;
    DWORD RetVal = 0;
    SOCKET Sock = (SOCKET)Sock_;

    time(&aclock);
    newtime = localtime(&aclock);
    pszTime = asctime(newtime);

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
