#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include "tcpsvcs.h"

DWORD WINAPI ChargenHandler(VOID* Sock_)
{
    DWORD Retval = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!GenerateChars(Sock)) {
        _tprintf(_T("Char generation failed\n"));
        Retval = 3;
    }

    _tprintf(_T("Shutting connection down...\n"));
    if (ShutdownConnection(Sock, FALSE)) {
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


BOOL GenerateChars(SOCKET Sock)
{
    int i,
        charIndex, /* internal loop */
        loopIndex; /* line loop */
    char ring[END-START];
    char *endring;
    BOOL bLoop = TRUE;

    /* fill ring with printable characters */
    for (charIndex=0, i=START; i<=END; charIndex++, i++)
        ring[charIndex] = i;
    /* establish the end character in the ring */
    endring = &ring[charIndex];

    /* where we will start output from */
    loopIndex = 0;

    while (bLoop)
    {
        /* if the loop index is equal to number of chars previously
         * printed, start the loop from the beginning */
        if (loopIndex == END-START)
            loopIndex = 0;

        /* start printing from char controled by loopIndex */
        charIndex = loopIndex;
        for (i=0; i<LINESIZ; i++)
        {
            /* FIXME: Should send lines instead of chars to improve efficiency
             * TCP will wait until it fills a packet anway before putting on
             * the wire, so it's pointless to keep polling send */
            if (!SendChar(Sock, ring[charIndex]))
            {
                return FALSE;
            }
            /* if current char equal last char, reset */
            if (ring[charIndex] == *endring)
                charIndex = 0;
            else
                charIndex++;
        }

        if (bLoop)
            if ((!SendChar(Sock, L'\r')) ||   (!SendChar(Sock, L'\n')))
                return FALSE;

        /* increment loop index to start printing from next char in ring */
        loopIndex++;
    }

    return TRUE;
}

BOOL SendChar(SOCKET Sock, TCHAR c)
{
    INT RetVal;
    INT SentBytes;

    SentBytes = 0;
    RetVal = send(Sock, &c, sizeof(TCHAR), 0);
    /*FIXME: need to establish if peer closes connection,
             not just report a socket error */
    if (RetVal > 0)
    {
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
