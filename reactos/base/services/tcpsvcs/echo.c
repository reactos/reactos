/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/echo.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "tcpsvcs.h"

extern BOOL bShutDown;

BOOL EchoIncomingPackets(SOCKET Sock)
{
    char ReadBuffer[BUF];
    TCHAR buf[256]; // temp for holding LogEvent text
    INT Temp;
    INT ReadBytes;
    INT SentBytes;

    do {
        ReadBytes = recv(Sock, ReadBuffer, BUF, 0);
        if (ReadBytes > 0)
        {
            _stprintf(buf, _T("Received %d bytes from client\n"), ReadBytes);
            LogEvent(buf, 0, FALSE);

            SentBytes = 0;
            while (SentBytes < ReadBytes)
            {
                Temp = send(Sock, ReadBuffer + SentBytes,
                        ReadBytes - SentBytes, 0);
                if (Temp > 0)
                {
                    _stprintf(buf, _T("Sent %d bytes back to client\n"), Temp);
                    LogEvent(buf, 0, FALSE);
                    SentBytes += Temp;
                }
                else if (Temp == SOCKET_ERROR)
                    return FALSE;
                else
                {
                    /* Client closed connection before we could reply to
                       all the data it sent, so quit early. */
                    _stprintf(buf, _T("Peer unexpectedly dropped connection!\n"));
                    LogEvent(buf, 0, FALSE);
                    return FALSE;
                }
            }
        }
        else if (ReadBytes == SOCKET_ERROR)
            return FALSE;

    } while ((ReadBytes != 0) && (! bShutDown));

    if (! bShutDown)
        LogEvent(_T("Echo: Connection closed by peer.\n"), 0, FALSE);

	if (bShutDown)
		LogEvent(_T("Echo: thread recieved shutdown signal\n"), 0, FALSE);

    return TRUE;
}

DWORD WINAPI EchoHandler(VOID* Sock_)
{
    DWORD RetVal = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!EchoIncomingPackets(Sock)) {
        LogEvent(_T("Echo: EchoIncomingPackets failed\n"), 0, FALSE);
        RetVal = 1;
    }

    LogEvent(_T("Echo: Shutting connection down...\n"), 0, FALSE);

    if (ShutdownConnection(Sock, TRUE))
        LogEvent(_T("Echo: Connection is down\n"), 0, FALSE);
    else
    {
        LogEvent(_T("Echo: Connection shutdown failed\n"), 0, FALSE);
        RetVal = 1;
    }

    LogEvent(_T("Echo: Terminating thread\n"), 0, FALSE);
    ExitThread(RetVal);
}
