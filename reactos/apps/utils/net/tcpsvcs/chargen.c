#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "tcpsvcs.h"

DWORD WINAPI ChargenHandler(VOID* Sock_)
{
    DWORD Retval = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!GenerateChars(Sock))
    {
        _tprintf(_T("Char generation failed\n"));
        Retval = 3;
    }

    _tprintf(_T("Shutting connection down...\n"));
    if (ShutdownConnection(Sock, FALSE))
        _tprintf(_T("Connection is down.\n"));
    else
    {
        _tprintf(_T("Connection shutdown failed\n"));
        Retval = 3;
    }
    _tprintf(_T("Terminating chargen thread\n"));
    ExitThread(0);

    return Retval;
}


BOOL GenerateChars(SOCKET Sock)
{
    int i;
    int charIndex; /* internal loop */
    int loopIndex; /* line loop */
    char ring[END-START];
    char *endring;
    char Line[LINESIZ];

    /* fill ring with printable characters */
    for (charIndex=0, i=START; i<=END; charIndex++, i++)
        ring[charIndex] = i;
    /* establish the end character in the ring */
    endring = &ring[charIndex];

    /* where we will start output from */
    loopIndex = 0;
    while (1)
    {
        /* if the loop index is equal to the last char,
         * start the loop again from the beginning */
        if (loopIndex == END-START)
            loopIndex = 0;

        /* start printing from char controled by loopIndex */
        charIndex = loopIndex;
        for (i=0; i < LINESIZ - 2; i++)
        {
            Line[i] = ring[charIndex];

            if (ring[charIndex] == *endring)
                charIndex = 0;
            else
                charIndex++;
        }

        Line[LINESIZ - 2] = L'\r';
        Line[LINESIZ - 1] = L'\n';

        if (!SendLine(Sock, Line))
            break;

        /* increment loop index to start printing from next char in ring */
        loopIndex++;
    }

    return TRUE;
}

BOOL SendLine(SOCKET Sock, TCHAR* Line)
{
    INT RetVal;
    INT SentBytes;
    INT LineSize;

    LineSize = sizeof(TCHAR) * LINESIZ;

    SentBytes = 0;
    RetVal = send(Sock, Line, LineSize, 0);
    /*FIXME: need to establish if peer closes connection,
             not just report a socket error */
    if (RetVal > 0)
    {
        if (RetVal != LineSize)
        {
            _tprintf(("Not sent enough\n"));
            return FALSE;
        }
        SentBytes += RetVal;
        return TRUE;
    }
    else if (RetVal == SOCKET_ERROR)
    {
        _tprintf(("Socket error\n"));
        return FALSE;
    }
    else
        _tprintf(("unknown error\n"));
        //WSAGetLastError()

    _tprintf(("Connection closed by peer.\n"));
    return TRUE;
}
