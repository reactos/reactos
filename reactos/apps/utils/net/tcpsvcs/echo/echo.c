#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "echo.h"
#include "../skelserver/skelserver.h"

DWORD WINAPI EchoHandler(VOID* Sock_)
{
    DWORD Retval = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!EchoIncomingPackets(Sock)) {
        _tprintf(_T("Echo incoming packets failed\n"));
        Retval = 3;
    }

    _tprintf(_T("Shutting connection down...\n"));
    if (ShutdownConnection(Sock)) {
        _tprintf(_T("Connection is down.\n"));
    }
    else
    {
        _tprintf(_T("Connection shutdown failed\n"));
        Retval = 3;
    }

    return Retval;
}



BOOL EchoIncomingPackets(SOCKET Sock)
{
    TCHAR ReadBuffer[BUF];
    INT Temp;
    INT ReadBytes;
    INT SentBytes;

    do {
        ReadBytes = recv(Sock, ReadBuffer, BUF, 0);
        if (ReadBytes > 0) {
            _tprintf(_T("Received %d bytes from client\n"), ReadBytes);

            SentBytes = 0;
            while (SentBytes < ReadBytes) {
                Temp = send(Sock, ReadBuffer + SentBytes,
                        ReadBytes - SentBytes, 0);
                if (Temp > 0) {
                    _tprintf(_T("Sent %d bytes back to client\n"), Temp);
                    SentBytes += Temp;
                }
                else if (Temp == SOCKET_ERROR) {
                    return FALSE;
                }
                else {
                    /* Client closed connection before we could reply to
                       all the data it sent, so quit early. */
                    _tprintf(_T("Peer unexpectedly dropped connection!\n"));
                    return FALSE;
                }
            }
        }
        else if (ReadBytes == SOCKET_ERROR) {
            return FALSE;
        }
    } while (ReadBytes != 0);

    _tprintf(("Connection closed by peer.\n"));
    return TRUE;
}

