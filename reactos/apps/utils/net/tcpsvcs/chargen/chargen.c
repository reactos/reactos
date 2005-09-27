#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "chargen.h"
#include "../skelserver/skelserver.h"

DWORD WINAPI ChargenHandler(VOID* Sock_)
{
    DWORD Retval = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!GenerateChars(Sock)) {
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


BOOL GenerateChars(SOCKET Sock)
{
    int i,
        charIndex, /* internal loop */
        loopIndex; /* line loop */
    char ring[END-START];
    char *endring;

    /* fill ring with printable characters */
    for (charIndex=0, i=START; i<=END; charIndex++, i++)
        ring[charIndex] = i;
    /* establish the end character in the ring */
    endring = &ring[charIndex];

    /* where we will start output from */
    loopIndex = 0;

    while (1)
    {
        /* if the loop index is equal to number of chars previously
         * printed, start the loop from the beginning */
        if (loopIndex == END-START)
            loopIndex = 0;

        /* start printing from char controled by loopIndex */
        charIndex = loopIndex;
        for (i=0; i<LINESIZ; i++)
        {
            SendChar(Sock, ring[charIndex]);
            /* if current char equal last char, reset */
            if (ring[charIndex] == *endring)
                charIndex = 0;
            else
                charIndex++;
        }
        SendChar(Sock, L'\r');
        SendChar(Sock, L'\n');

        /* increment loop index to start printing from next char in ring */
        loopIndex++;
    }

    return 0;
}

BOOL SendChar(SOCKET Sock, TCHAR c)
{
    INT Temp;
    INT SentBytes;

    SentBytes = 0;
    Temp = send(Sock, &c, sizeof(TCHAR), 0);
    if (Temp > 0) {
        SentBytes += Temp;
    }
    else if (Temp == SOCKET_ERROR) {
        return FALSE;
    }
    else
    {
        /* Client closed connection before we could reply to
           all the data it sent, so quit early. */
        _tprintf(_T("Peer unexpectedly dropped connection!\n"));
        return FALSE;
    }

    _tprintf(("Connection closed by peer.\n"));
    return TRUE;
}





