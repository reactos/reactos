#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "tcpsvcs.h"

DWORD WINAPI DiscardHandler(VOID* Sock_)
{
    DWORD Retval = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!RecieveIncomingPackets(Sock))
    {
        _tprintf(_T("RecieveIncomingPackets failed\n"));
        Retval = 3;
    }

    _tprintf(_T("Shutting connection down...\n"));
    if (ShutdownConnection(Sock, TRUE))
    {
        _tprintf(_T("Connection is down.\n"));
    }
    else
    {
        _tprintf(_T("Connection shutdown failed\n"));
        Retval = 3;
    }
    _tprintf(_T("Terminating thread\n"));
    ExitThread(0);

    return Retval;
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
