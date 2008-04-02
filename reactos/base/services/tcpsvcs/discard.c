/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/discard.c
 * PURPOSE:     Recieves input from a client and discards it
 * COPYRIGHT:   Copyright 2005 - 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

#define BUFSIZE 1024

static BOOL
RecieveIncomingPackets(SOCKET sock)
{
    char readBuffer[BUFSIZE];
    INT readBytes;

    do
    {
        readBytes = recv(sock, readBuffer, BUFSIZE, 0);
        if (readBytes > 0)
        {
            TCHAR logBuf[256];

            _stprintf(logBuf, _T("Discard: Received %d bytes from client"), readBytes);
            LogEvent(logBuf, 0, 0, LOG_FILE);
        }
        else if (readBytes == SOCKET_ERROR)
        {
            LogEvent(_T("Discard: Socket Error"), WSAGetLastError(), 0, LOG_ERROR);
            return FALSE;
        }
    } while ((readBytes > 0) && (!bShutdown));

    if (!bShutdown)
        LogEvent(_T("Discard: Connection closed by peer"), 0, 0, LOG_FILE);

    return TRUE;
}

DWORD WINAPI
DiscardHandler(VOID* sock_)
{
    DWORD retVal = 0;
    SOCKET sock = (SOCKET)sock_;

    if (!RecieveIncomingPackets(sock))
    {
        LogEvent(_T("Discard: RecieveIncomingPackets failed"), 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(_T("Discard: Shutting connection down"), 0, 0, LOG_FILE);
    if (ShutdownConnection(sock, TRUE))
    {
        LogEvent(_T("Discard: Connection is down."), 0, 0, LOG_FILE);
    }
    else
    {
        LogEvent(_T("Discard: Connection shutdown failed"), 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(_T("Discard: Terminating thread"), 0, 0, LOG_FILE);
    ExitThread(retVal);
}
