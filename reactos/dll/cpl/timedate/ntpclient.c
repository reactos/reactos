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

typedef struct _INFO
{
    SOCKET Sock;
    SOCKADDR_IN myAddr;
    SOCKADDR_IN ntpAddr;
    NTPPACKET SendPacket;
    NTPPACKET RecvPacket;
} INFO, *PINFO;


static BOOL
InitConnection(PINFO pInfo,
               LPSTR lpAddress)
{
    WSADATA wsaData;
    HOSTENT *he;
    INT Ret;

    Ret = WSAStartup(MAKEWORD(2, 2),
                     &wsaData);
    if (Ret != 0)
        return FALSE;

    pInfo->Sock = socket(AF_INET,
                         SOCK_DGRAM,
                         0);
    if (pInfo->Sock == INVALID_SOCKET)
        return FALSE;

    /* setup server info */
    he = gethostbyname(lpAddress);
    if (he != NULL)
    {
        /* setup server socket info */
        ZeroMemory(&pInfo->ntpAddr, sizeof(SOCKADDR_IN));
        pInfo->ntpAddr.sin_family = AF_INET; //he->h_addrtype;
        pInfo->ntpAddr.sin_port = htons(NTPPORT);
        pInfo->ntpAddr.sin_addr = *((struct in_addr *)he->h_addr);
    }
    else
        return FALSE;

    return TRUE;
}


static VOID
DestroyConnection(VOID)
{
    WSACleanup();
}


static BOOL
GetTransmitTime(PTIMEPACKET ptp)
{
    return TRUE;
}


/* send some data to wake the server up */
static BOOL
SendData(PINFO pInfo)
{
    TIMEPACKET tp = {0,};
    INT Ret;

    ZeroMemory(&pInfo->SendPacket, sizeof(pInfo->SendPacket));
    pInfo->SendPacket.LiVnMode = 27;
    if (!GetTransmitTime(&tp))
        return FALSE;
    pInfo->SendPacket.TransmitTimestamp = tp;

    Ret = sendto(pInfo->Sock,
                 (char *)&pInfo->SendPacket,
                 sizeof(pInfo->SendPacket),
                 0,
                 (SOCKADDR *)&pInfo->ntpAddr,
                 sizeof(SOCKADDR_IN));

    if (Ret == SOCKET_ERROR)
        return FALSE;

    return TRUE;
}


static ULONG
RecieveData(PINFO pInfo)
{
    TIMEVAL timeVal;
    FD_SET readFDS;
    INT Ret;
    ULONG ulTime = 0;

    /* monitor socket for incomming connections */
    FD_ZERO(&readFDS);
    FD_SET(pInfo->Sock, &readFDS);

    /* set timeout values */
    timeVal.tv_sec  = TIMEOUT / 1000;
    timeVal.tv_usec = TIMEOUT % 1000;

    /* check for data on the socket for TIMEOUT millisecs*/
    Ret = select(0, &readFDS, NULL, NULL, &timeVal);

    if ((Ret != SOCKET_ERROR) && (Ret != 0))
    {

        Ret = recvfrom(pInfo->Sock,
                       (char *)&pInfo->RecvPacket,
                       sizeof(pInfo->RecvPacket),
                       0,
                       NULL,
                       NULL);
        if (Ret != SOCKET_ERROR)
            ulTime = ntohl(ulTime);
    }

    return ulTime;
}


ULONG
GetServerTime(LPWSTR lpAddress)
{
    PINFO pInfo;
    LPSTR lpAddr;
    DWORD dwSize = wcslen(lpAddress) + 1;
    ULONG ulTime = 0;

    pInfo = (PINFO)HeapAlloc(GetProcessHeap(),
                             0,
                             sizeof(INFO));
    lpAddr = (LPSTR)HeapAlloc(GetProcessHeap(),
                              0,
                              dwSize);

    if (pInfo && lpAddr)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                lpAddress,
                                -1,
                                lpAddr,
                                dwSize,
                                NULL,
                                NULL))
        {
            if (InitConnection(pInfo, lpAddr))
            {
                if (SendData(pInfo))
                {
                    ulTime = RecieveData(pInfo);
                }
            }

            DestroyConnection();
        }
    }

    if (pInfo)
        HeapFree(GetProcessHeap(), 0, pInfo);
    if (lpAddr)
        HeapFree(GetProcessHeap(), 0, lpAddr);

    return ulTime;
}
