/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/services/tcpsvcs/daytime.c
 * PURPOSE:     Sends the current date and time to the client
 * COPYRIGHT:   Copyright 2005 - 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

#include <time.h>

static BOOL
SendTime(SOCKET sock, CHAR *time)
{
    DWORD stringSize = strlen(time) + 1;
    if (send(sock, time, stringSize, 0) == SOCKET_ERROR)
    {
        LogEvent(L"DayTime: Error sending data", WSAGetLastError(), 0, LOG_ERROR);
        return FALSE;
    }

    return TRUE;
}


DWORD WINAPI
DaytimeHandler(VOID* Sock_)
{
    struct tm *localTime;
    time_t aclock;
    CHAR *pszTime;
    DWORD retVal = 0;
    SOCKET Sock = (SOCKET)Sock_;

    time(&aclock);
    localTime = localtime(&aclock);
    if (localTime)
    {
        pszTime = asctime(localTime);
        if (!SendTime(Sock, pszTime))
            retVal = 1;
    }

    LogEvent(L"DayTime: Shutting connection down", 0, 0, LOG_FILE);
    if (ShutdownConnection(Sock, FALSE))
        LogEvent(L"DayTime: Connection is down", 0, 0, LOG_FILE);
    else
    {
        LogEvent(L"DayTime: Connection shutdown failed", 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(L"DayTime: Terminating thread", 0, 0, LOG_FILE);
    ExitThread(retVal);
}
