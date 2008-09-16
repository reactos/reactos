/*
 * PROJECT:     ReactOS simple TCP/IP services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        /base/services/tcpsvcs/chargen.c
 * PURPOSE:     Sends continous lines of chars to the client
 * COPYRIGHT:   Copyright 2005 - 2008 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tcpsvcs.h"

/* printable ASCII's characters for chargen */
#define ASCII_START 32
#define ASCII_END 126
#define NUM_CHARS ASCII_END - ASCII_START

/* number of chars to put on a line */
#define LINESIZE 74 // 72 + CR + NL

static BOOL
SendLine(SOCKET sock, LPSTR lpLine)
{
    BOOL bRet = FALSE;

    /*FIXME: need to establish if peer closes connection, not just report a socket error */
    INT retVal = send(sock, lpLine, LINESIZE, 0);
    if (retVal > 0)
    {
        if (retVal == LINESIZE)
        {
            bRet = TRUE;
        }
        else
        {
            LogEvent(L"Chargen: Not sent enough bytes", 0, 0, LOG_FILE);
        }
    }
    else if (retVal == SOCKET_ERROR)
    {
        LogEvent(L"Chargen: Socket error\n", WSAGetLastError(), 0, LOG_ERROR);
    }
    else
    {
        LogEvent(L"Chargen: unknown error\n", WSAGetLastError(), 0, LOG_ERROR);
    }

    return bRet;;
}

static BOOL
GenerateChars(SOCKET sock)
{
    CHAR chars[NUM_CHARS];
    CHAR line[LINESIZE];
    INT charIndex;
    INT loopIndex;
    INT i;

    /* fill the array with printable characters */
    for (charIndex = 0, i = ASCII_START; i <= ASCII_END; charIndex++, i++)
        chars[charIndex] = (char)i;

    loopIndex = 0;
    while (!bShutdown)
    {
        /* reset the loop when we hit the last char */
        if (loopIndex == NUM_CHARS)
            loopIndex = 0;

        /* fill a line array to send */
        charIndex = loopIndex;
        for (i=0; i < LINESIZE - 2; i++)
        {
            line[i] = chars[charIndex];

            /* if we hit the end char, reset it */
            if (chars[charIndex] == chars[NUM_CHARS - 1])
                charIndex = 0;
            else
                charIndex++;
        }
        line[LINESIZE - 2] = '\r';
        line[LINESIZE - 1] = '\n';

        if (!SendLine(sock, line))
            break;

        /* start printing from next char in the array */
        loopIndex++;
    }

    return TRUE;
}

DWORD WINAPI
ChargenHandler(VOID* sock_)
{
    INT retVal = 0;
    SOCKET sock = (SOCKET)sock_;

    if (!GenerateChars(sock))
    {
        LogEvent(L"Chargen: Char generation failed", 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(L"Chargen: Shutting connection down...", 0, 0, LOG_FILE);
    if (ShutdownConnection(sock, FALSE))
    {
        LogEvent(L"Chargen: Connection is down", 0, 0, LOG_FILE);
    }
    else
    {
        LogEvent(L"Chargen: Connection shutdown failed", 0, 0, LOG_FILE);
        retVal = 1;
    }

    LogEvent(L"Chargen: Terminating thread", 0, 0, LOG_FILE);
    ExitThread(retVal);
}
