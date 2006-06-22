/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/timedate/ntpclient.c
 * PURPOSE:     Queries the NTP server
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include <timedate.h>

#define TIMEOUT 4000 /* 4 second timeout */

SOCKET Sock;
SOCKADDR_IN myAddr, ntpAddr;

BOOL
InitialiseConnection(LPSTR lpAddress)
{
    WSADATA wsaData;
    HOSTENT *he;
    INT Ret;

    Ret = WSAStartup(MAKEWORD(2, 2),
                     &wsaData);
    if (Ret != 0)
        return FALSE;

    Sock = socket(AF_INET,
                  SOCK_DGRAM,
                  0);
    if (Sock == INVALID_SOCKET)
        return FALSE;

    /* setup server info */
    he = gethostbyname(lpAddress);
    if (he != NULL)
    {
        /* setup server socket info */
        ZeroMemory(&ntpAddr, sizeof(SOCKADDR_IN));
        ntpAddr.sin_family = AF_INET; //he->h_addrtype;
        ntpAddr.sin_port = htons(NTPPORT);
        ntpAddr.sin_addr = *((struct in_addr *)he->h_addr);
    }
    else
        return FALSE;

    return TRUE;
}

VOID
DestroyConnection()
{
    WSACleanup();
}

/* send some data to wake the server up */
BOOL
SendData()
{
    CHAR Packet[] = "";
    INT Ret;

    Ret = sendto(Sock,
                 Packet,
                 sizeof(Packet),
                 0,
                 (SOCKADDR *)&ntpAddr,
                 sizeof(SOCKADDR_IN));

    if (Ret == SOCKET_ERROR)
        return FALSE;

    return TRUE;
}


ULONG
RecieveData(VOID)
{
    TIMEVAL timeVal;
    FD_SET readFDS;
    INT Ret;
    ULONG ulTime = 0;

    /* monitor socket for incomming connections */
    FD_ZERO(&readFDS);
    FD_SET(Sock, &readFDS);

    /* set timeout values */
    timeVal.tv_sec  = TIMEOUT / 1000;
    timeVal.tv_usec = TIMEOUT % 1000;

    /* check for data on the socket for TIMEOUT millisecs*/
    Ret = select(0, &readFDS, NULL, NULL, &timeVal);

    if ((Ret != SOCKET_ERROR) && (Ret != 0))
    {
        Ret = recvfrom(Sock,
                       (char *)&ulTime,
                       4,
                       0,
                       NULL,
                       NULL);
        if (Ret != SOCKET_ERROR)
            ulTime = ntohl(ulTime);
    }

    return ulTime;
}
