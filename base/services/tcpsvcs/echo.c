/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/services/tcpsvcs/echo.c
 * PURPOSE:     Returns whatever input the client sends
 * COPYRIGHT:   Copyright 2005 - 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

#define RECV_BUF 1024

static BOOL
EchoIncomingPackets(SOCKET sock)
{
    CHAR readBuffer[RECV_BUF];
    WCHAR logBuf[256];
    INT totalSentBytes;
    INT readBytes;
    INT retVal;

    do
    {
        readBytes = recv(sock, readBuffer, RECV_BUF, 0);
        if (readBytes > 0)
        {
            swprintf(logBuf, L"Received %d bytes from client", readBytes);
            LogEvent(logBuf, 0, 0, LOG_FILE);

            totalSentBytes = 0;
            while (!bShutdown && totalSentBytes < readBytes)
            {
                retVal = send(sock, readBuffer + totalSentBytes, readBytes - totalSentBytes, 0);
                if (retVal > 0)
                {
                    swprintf(logBuf, L"Sent %d bytes back to client", retVal);
                    LogEvent(logBuf, 0, 0, LOG_FILE);
                    totalSentBytes += retVal;
                }
                else if (retVal == SOCKET_ERROR)
                {
                    LogEvent(L"Echo: socket error", WSAGetLastError(), 0, LOG_ERROR);
                    return FALSE;
                }
                else
                {
                    /* Client closed connection before we could reply to
                       all the data it sent, so quit early. */
                    LogEvent(L"Peer unexpectedly dropped connection!", 0, 0, LOG_FILE);
                    return FALSE;
                }
            }
        }
        else if (readBytes == SOCKET_ERROR)
        {
            LogEvent(L"Echo: socket error", WSAGetLastError(), 0, LOG_ERROR);
            return FALSE;
        }
    } while ((readBytes != 0) && (!bShutdown));

    if (!bShutdown)
        LogEvent(L"Echo: Connection closed by peer", 0, 0, LOG_FILE);

    return TRUE;
}

DWORD WINAPI
EchoHandler(VOID* sock_)
{
    DWORD retVal = 0;
    SOCKET sock = (SOCKET)sock_;

    if (!EchoIncomingPackets(sock))
    {
        LogEvent(L"Echo: EchoIncomingPackets failed", 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(L"Echo: Shutting connection down", 0, 0, LOG_FILE);

    if (ShutdownConnection(sock, TRUE))
    {
        LogEvent(L"Echo: Connection is down", 0, 0, LOG_FILE);
    }
    else
    {
        LogEvent(L"Echo: Connection shutdown failed", 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(L"Echo: Terminating thread", 0, 0, LOG_FILE);
    ExitThread(retVal);
}
