/*
 *  ReactOS Services
 *  Copyright (C) 2005 ReactOS Team
 *
 * LICENCE:     GPL - See COPYING in the top level directory
 * PROJECT:     ReactOS simple TCP/IP services
 * FILE:        apps/utils/net/tcpsvcs/skelserver.c
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

DWORD WINAPI StartServer(LPVOID lpParam)
{
	SOCKET ListeningSocket;
    const TCHAR* HostIP = "127.0.0.1";
    PSERVICES pServices;

    pServices = (PSERVICES)lpParam;

    ListeningSocket = SetUpListener(HostIP, htons(pServices->Port));
    if (ListeningSocket == INVALID_SOCKET)
    {
        _tprintf(_T("error setting up socket\n"));
        return 3;
    }

    _tprintf(_T("%s is waiting for connections on port %d...\n"),
        pServices->Name, pServices->Port);
    while (1)
    {
        AcceptConnections(ListeningSocket, pServices->Service, pServices->Name);
        printf("Acceptor restarting...\n");
    }

    /* won't see this yet as we kill the service with ctrl+c */
    _tprintf(_T("Detaching Winsock2...\n"));
    WSACleanup();
    return 0;
}


SOCKET SetUpListener(const char* ServAddr, int Port)
{
    SOCKET Sock;
    SOCKADDR_IN Server;

    Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (Sock != INVALID_SOCKET)
    {
        Server.sin_family = AF_INET;
        Server.sin_addr.s_addr = htonl(INADDR_ANY);
        Server.sin_port = Port;
        if (bind(Sock, (SOCKADDR*)&Server, sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
        {
            listen(Sock, SOMAXCONN);
            return Sock;
        }
        else
            printf("bind() failed\n");

    }
    return INVALID_SOCKET;
}




VOID AcceptConnections(SOCKET ListeningSocket,
    LPTHREAD_START_ROUTINE Service, TCHAR *Name)
{
    SOCKADDR_IN Client;
    SOCKET Sock;
    INT nAddrSize = sizeof(Client);
    DWORD ThreadID;

    while (1)
    {
        Sock = accept(ListeningSocket, (SOCKADDR*)&Client, &nAddrSize);
        if (Sock != INVALID_SOCKET)
        {
            _tprintf(_T("Accepted connection to %s server from %s:%d\n"),
                Name, inet_ntoa(Client.sin_addr), ntohs(Client.sin_port));
            _tprintf(_T("Creating new thread for %s\n"), Name);
            CreateThread(0, 0, Service, (void*)Sock, 0, &ThreadID);
        }
        else
        {
            _tprintf(_T("accept() failed\n"));
            return;
        }
    }
}

BOOL ShutdownConnection(SOCKET Sock, BOOL bRec)
{
    /* Disallow any further data sends.  This will tell the other side
       that we want to go away now.  If we skip this step, we don't
       shut the connection down nicely. */
    if (shutdown(Sock, SD_SEND) == SOCKET_ERROR)
    {
        _tprintf(_T("Error in shutdown"));
        return FALSE;
    }

      /* Receive any extra data still sitting on the socket.  After all
         data is received, this call will block until the remote host
         acknowledges the TCP control packet sent by the shutdown above.
         Then we'll get a 0 back from recv, signalling that the remote
         host has closed its side of the connection. */
    if (bRec)
    {
        char ReadBuffer[BUF];
        int NewBytes = recv(Sock, ReadBuffer, BUF, 0);
        if (NewBytes == SOCKET_ERROR)
            return FALSE;
        else if (NewBytes != 0)
            _tprintf(_T("FYI, received %d unexpected bytes during shutdown\n"), NewBytes);
    }

    /* Close the socket. */
    if (closesocket(Sock) == SOCKET_ERROR)
        return FALSE;

    return TRUE;
}
