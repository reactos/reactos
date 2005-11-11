/*
 *  ReactOS Services
 *  Copyright (C) 2005 ReactOS Team
 *
 * LICENCE:     GPL - See COPYING in the top level directory
 * PROJECT:     ReactOS simple TCP/IP services
 * FILE:        apps/utils/net/tcpsvcs/discard.c
  * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *   GM 04/10/05 Created
 *
 */

#include "tcpsvcs.h"

DWORD WINAPI DiscardHandler(VOID* Sock_)
{
    DWORD RetVal = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!RecieveIncomingPackets(Sock))
    {
        LogEvent(_T("Discard: RecieveIncomingPackets failed\n"), 0, FALSE);
        RetVal = -1;
    }

    LogEvent(_T("Discard: Shutting connection down...\n"), 0, FALSE);
    if (ShutdownConnection(Sock, TRUE))
        LogEvent(_T("Discard: Connection is down.\n"), 0, FALSE);
    else
    {
        LogEvent(_T("Discard: Connection shutdown failed\n"), 0, FALSE);
        RetVal = -1;
    }
    
    LogEvent(_T("Discard: Terminating thread\n"), 0, FALSE);
    ExitThread(RetVal);
}



BOOL RecieveIncomingPackets(SOCKET Sock)
{
    TCHAR ReadBuffer[BUF];
    TCHAR temp[512]; // temp for holding LogEvent text
    INT ReadBytes;

    do
    {
        ReadBytes = recv(Sock, ReadBuffer, BUF, 0);
        if (ReadBytes > 0)
        {
            _stprintf(temp, _T("Received %d bytes from client\n"), ReadBytes);
            LogEvent(temp, 0, FALSE);
        }
        else if (ReadBytes == SOCKET_ERROR)
        {
            _stprintf(temp, ("Socket Error: %d\n"), WSAGetLastError());
            LogEvent(temp, 0, TRUE);
            return FALSE;
        }
    } while (ReadBytes > 0);

    LogEvent(_T("Discard: Connection closed by peer.\n"), 0, FALSE);
    return TRUE;
}
