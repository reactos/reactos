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

#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "tcpsvcs.h"

DWORD WINAPI DiscardHandler(VOID* Sock_)
{
    DWORD RetVal = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!RecieveIncomingPackets(Sock))
    {
        _tprintf(_T("RecieveIncomingPackets failed\n"));
        RetVal = -1;
    }

    _tprintf(_T("Shutting connection down...\n"));
    if (ShutdownConnection(Sock, TRUE))
    {
        _tprintf(_T("Connection is down.\n"));
    }
    else
    {
        _tprintf(_T("Connection shutdown failed\n"));
        RetVal = -1;
    }
    
    _tprintf(_T("Terminating discard thread\n"));
    ExitThread(RetVal);
}



BOOL RecieveIncomingPackets(SOCKET Sock)
{
    TCHAR ReadBuffer[BUF];
    INT ReadBytes;

    do
    {
        ReadBytes = recv(Sock, ReadBuffer, BUF, 0);
        if (ReadBytes > 0)
            _tprintf(_T("Received %d bytes from client\n"), ReadBytes);
        else if (ReadBytes == SOCKET_ERROR)
        {
            _tprintf(("Socket Error: %d\n"), WSAGetLastError());
            return FALSE;
        }
    } while (ReadBytes > 0);

    _tprintf(("Connection closed by peer.\n"));
    return TRUE;
}
