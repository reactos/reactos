/*
 *  ReactOS Services
 *  Copyright (C) 2005 ReactOS Team
 *
 * LICENCE:     GPL - See COPYING in the top level directory
 * PROJECT:     ReactOS simple TCP/IP services
 * FILE:        apps/utils/net/tcpsvcs/chargen.c
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

extern BOOL bShutDown;

DWORD WINAPI ChargenHandler(VOID* Sock_)
{
    INT RetVal = 0;
    SOCKET Sock = (SOCKET)Sock_;

    if (!GenerateChars(Sock))
    {
        LogEvent(_T("Chargen: Char generation failed\n"), 0, FALSE);
        RetVal = 1;
    }

    LogEvent(_T("Chargen: Shutting connection down...\n"), 0, FALSE);
    if (ShutdownConnection(Sock, FALSE))
        LogEvent(_T("Chargen: Connection is down.\n"), 0, FALSE);
    else
    {
        LogEvent(_T("Chargen: Connection shutdown failed\n"), 0, FALSE);
        RetVal = 1;
    }
    
    LogEvent(_T("Chargen: Terminating thread\n"), 0, FALSE);
    ExitThread(RetVal);

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
        ring[charIndex] = (char)i;
    /* save the address of the end character in the ring */
    endring = &ring[charIndex];

    /* where we will start output from */
    loopIndex = 0;
    while (! bShutDown)
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

        if (! SendLine(Sock, Line))
            break;

        /* increment loop index to start printing from next char in ring */
        loopIndex++;
    }
    
    if (bShutDown)
        return FALSE;
    else
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
            LogEvent(_T("Chargen: Not sent enough bytes\n"), 0, FALSE);
            return FALSE;
        }
        SentBytes += RetVal;
        return TRUE;
    }
    else if (RetVal == SOCKET_ERROR)
    {
        LogEvent(_T("Chargen: Socket error\n"), 0, FALSE);
        return FALSE;
    }
    else
        LogEvent(_T("Chargen: unknown error\n"), 0, FALSE);
        // return FALSE;

    LogEvent(_T("Chargen: Connection closed by peer.\n"), 0, FALSE);
    return TRUE;
}
