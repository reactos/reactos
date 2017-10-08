/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/services/tcpsvcs/skelserver.c
 * PURPOSE:     Sets up a server and listens for connections
 * COPYRIGHT:   Copyright 2005 - 2008 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "tcpsvcs.h"

#define BUF 1024

static SOCKET
SetUpListener(USHORT Port)
{
    SOCKET sock;
    SOCKADDR_IN server;
    BOOL bSetup = FALSE;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock != INVALID_SOCKET)
    {
        ZeroMemory(&server, sizeof(SOCKADDR_IN));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = Port;

        if (bind(sock, (SOCKADDR*)&server, sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
        {
            if (listen(sock, SOMAXCONN) != SOCKET_ERROR)
            {
                bSetup = TRUE;
            }
            else
            {
                LogEvent(L"listen() failed", WSAGetLastError(), 0, LOG_ERROR);
            }
        }
        else
        {
            LogEvent(L"bind() failed", WSAGetLastError(), 0, LOG_ERROR);
        }
    }
    else
    {
        LogEvent(L"socket() failed", WSAGetLastError(), 0, LOG_ERROR);
    }

    return bSetup ? sock : INVALID_SOCKET;
}


static VOID
AcceptConnections(SOCKET listeningSocket,
                  LPTHREAD_START_ROUTINE lpService,
                  LPWSTR lpName)
{
    SOCKADDR_IN client;
    SOCKET sock;
    HANDLE hThread;
    TIMEVAL timeVal;
    FD_SET readFD;
    WCHAR logBuf[256];
    INT timeOut = 2000;

    timeVal.tv_sec  = timeOut / 1000;
    timeVal.tv_usec = timeOut % 1000;

    while (!bShutdown)
    {
        INT selRet = 0;

        FD_ZERO(&readFD);
        FD_SET(listeningSocket, &readFD);

        selRet = select(0, &readFD, NULL, NULL, &timeVal);
        if (selRet > 0)
        {
            if (!bShutdown || FD_ISSET(listeningSocket, &readFD))
            {
                INT addrSize = sizeof(SOCKADDR_IN);

                sock = accept(listeningSocket, (SOCKADDR*)&client, &addrSize);
                if (sock != INVALID_SOCKET)
                {
                    swprintf(logBuf,
                             L"Accepted connection to %s server from %S:%d",
                             lpName,
                             inet_ntoa(client.sin_addr),
                             ntohs(client.sin_port));
                    LogEvent(logBuf, 0, 0, LOG_FILE);

                    swprintf(logBuf, L"Creating worker thread for %s", lpName);
                    LogEvent(logBuf, 0, 0, LOG_FILE);

                    if (!bShutdown)
                    {
                        hThread = CreateThread(0, 0, lpService, (void*)sock, 0, NULL);
                        if (hThread != NULL)
                        {
                            CloseHandle(hThread);
                        }
                        else
                        {
                            swprintf(logBuf, L"Failed to start worker thread for the %s server",
                                     lpName);
                            LogEvent(logBuf, 0, 0, LOG_FILE);
                        }
                    }
                }
                else
                {
                    LogEvent(L"accept failed", WSAGetLastError(), 0, LOG_ERROR);
                }
            }
        }
        else if (selRet == SOCKET_ERROR)
        {
            LogEvent(L"select failed", WSAGetLastError(), 0, LOG_ERROR);
        }
    }
}

BOOL
ShutdownConnection(SOCKET sock,
                   BOOL bRec)
{
    WCHAR logBuf[256];

    /* Disallow any further data sends.  This will tell the other side
       that we want to go away now.  If we skip this step, we don't
       shut the connection down nicely. */
    if (shutdown(sock, SD_SEND) == SOCKET_ERROR)
    {
        LogEvent(L"Error in shutdown()", WSAGetLastError(), 0, LOG_ERROR);
        return FALSE;
    }

      /* Receive any extra data still sitting on the socket
         before we close it */
    if (bRec)
    {
        CHAR readBuffer[BUF];
        INT ret;

        do
        {
            ret = recv(sock, readBuffer, BUF, 0);
            if (ret >= 0)
            {
                swprintf(logBuf, L"FYI, received %d unexpected bytes during shutdown", ret);
                LogEvent(logBuf, 0, 0, LOG_FILE);
            }
        } while (ret > 0);
    }

    closesocket(sock);

    return TRUE;
}


DWORD WINAPI
StartServer(LPVOID lpParam)
{
    SOCKET listeningSocket;
    PSERVICES pServices;
    TCHAR logBuf[256];

    pServices = (PSERVICES)lpParam;

    swprintf(logBuf, L"Starting %s server", pServices->lpName);
    LogEvent(logBuf, 0, 0, LOG_FILE);

    if (!bShutdown)
    {
        listeningSocket = SetUpListener(htons(pServices->Port));
        if (!bShutdown && listeningSocket != INVALID_SOCKET)
        {
            swprintf(logBuf,
                     L"%s is waiting for connections on port %d",
                     pServices->lpName,
                     pServices->Port);
            LogEvent(logBuf, 0, 0, LOG_FILE);

            AcceptConnections(listeningSocket, pServices->lpService, pServices->lpName);
        }
        else
        {
            LogEvent(L"Socket error when setting up listener", 0, 0, LOG_FILE);
        }
    }

    swprintf(logBuf, L"Exiting %s thread", pServices->lpName);
    LogEvent(logBuf, 0, 0, LOG_FILE);
    ExitThread(0);
}
