/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/discard.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "tcpsvcs.h"

extern BOOL bShutDown;

DWORD WINAPI DiscardHandler(VOID* Sock_)
{
    DWORD RetVal = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!RecieveIncomingPackets(Sock))
    {
        LogEvent(_T("Discard: RecieveIncomingPackets failed\n"), 0, FALSE);
        RetVal = 1;
    }

    LogEvent(_T("Discard: Shutting connection down...\n"), 0, FALSE);
    if (ShutdownConnection(Sock, TRUE))
        LogEvent(_T("Discard: Connection is down.\n"), 0, FALSE);
    else
    {
        LogEvent(_T("Discard: Connection shutdown failed\n"), 0, FALSE);
        RetVal = 1;
    }

    LogEvent(_T("Discard: Terminating thread\n"), 0, FALSE);
    ExitThread(RetVal);
}



BOOL RecieveIncomingPackets(SOCKET Sock)
{
    char ReadBuffer[BUF];
    TCHAR buf[256];
    INT ReadBytes;

    do
    {
        ReadBytes = recv(Sock, ReadBuffer, BUF, 0);
        if (ReadBytes > 0)
        {
            _stprintf(buf, _T("Received %d bytes from client\n"), ReadBytes);
            LogEvent(buf, 0, FALSE);
        }
        else if (ReadBytes == SOCKET_ERROR)
        {
            _stprintf(buf, _T("Socket Error: %d\n"), WSAGetLastError());
            LogEvent(buf, 0, TRUE);
            return FALSE;
        }
    } while ((ReadBytes > 0) && (! bShutDown));

    if (! bShutDown)
        LogEvent(_T("Discard: Connection closed by peer.\n"), 0, FALSE);

    return TRUE;
}
