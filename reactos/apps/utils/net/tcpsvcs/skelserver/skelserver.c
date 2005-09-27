#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "../tcpsvcs.h"
#include "skelserver.h"


DWORD WINAPI StartServer(LPVOID lpParam)
{
    const TCHAR* HostIP = "127.0.0.1";
    DWORD RetVal;
    WSADATA wsaData;
    PMYDATA pData;

    pData = (PMYDATA)lpParam;

    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        _tprintf(_T("WSAStartup() failed : %lu\n"), RetVal);
        return -1;
    }

    SOCKET ListeningSocket = SetUpListener(HostIP, htons(pData->Port));
    if (ListeningSocket == INVALID_SOCKET)
    {
        _tprintf(_T("error setting up socket\n"));
        return 3;
    }

    printf("Waiting for connections...\n");
    while (1)
    {
        AcceptConnections(ListeningSocket, pData->Service);
        printf("Acceptor restarting...\n");
    }

    WSACleanup();
    return 0;
}


SOCKET SetUpListener(const char* ServAddr, int Port)
{
    SOCKET Sock;
    SOCKADDR_IN Server;
    DWORD InterfaceAddr = inet_addr(ServAddr);

    if (InterfaceAddr != INADDR_NONE)
    {
        Sock = socket(AF_INET, SOCK_STREAM, 0);
        if (Sock != INVALID_SOCKET)
        {
            Server.sin_family = AF_INET;
            Server.sin_addr.s_addr = InterfaceAddr;
            Server.sin_port = Port;
            if (bind(Sock, (SOCKADDR*)&Server, sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
            {
                listen(Sock, SOMAXCONN);
                return Sock;
            }
            else
                printf("bind() failed\n");

        }
    }
    return INVALID_SOCKET;
}




VOID AcceptConnections(SOCKET ListeningSocket, LPTHREAD_START_ROUTINE Service)
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
            _tprintf(_T("Accepted connection from %s:%d\n"),
                inet_ntoa(Client.sin_addr), ntohs(Client.sin_port));

            CreateThread(0, 0, Service, (void*)Sock, 0, &ThreadID);
        }
        else
        {
            _tprintf(_T("accept() failed\n"));
            return;
        }
    }
}

BOOL ShutdownConnection(SOCKET Sock)
{
    /* Disallow any further data sends.  This will tell the other side
       that we want to go away now.  If we skip this step, we don't
       shut the connection down nicely. */
    if (shutdown(Sock, SD_SEND) == SOCKET_ERROR)
        return FALSE;

      /* Receive any extra data still sitting on the socket.  After all
         data is received, this call will block until the remote host
         acknowledges the TCP control packet sent by the shutdown above.
         Then we'll get a 0 back from recv, signalling that the remote
         host has closed its side of the connection. */
    while (1)
    {
        char ReadBuffer[BUF];
        int NewBytes = recv(Sock, ReadBuffer, BUF, 0);
        if (NewBytes == SOCKET_ERROR)
            return FALSE;
        else if (NewBytes != 0)
            _tprintf(_T("FYI, received %d unexpected bytes during shutdown\n"), NewBytes);
        else
            break;
    }

    /* Close the socket. */
    if (closesocket(Sock) == SOCKET_ERROR)
        return FALSE;

    return TRUE;
}
