/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        base/applications/network/ipv6xchg/ipv6xchg.c
 * PURPOSE:     Exchange one TCP payload and one UDP payload over IPv6
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * Peer address fe80::2 and port 5470 are the echo target in
 * virtual-machines/veros-ntfs/Send-Ipv6Peer.ps1. Link-local form is
 * RFC 4291 Section 2.5.6 (https://www.rfc-editor.org/rfc/rfc4291).
 * SOCKADDR_IN6 is sdk/include/psdk/ws2tcpip.h. DbgPrint matches
 * base/applications/network/ping/ping.c. Serial accept lines are
 * "IPV6 XCHG tcp" and "IPV6 XCHG udp".
 */

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <winbase.h>
#include <stdio.h>
#include <string.h>

ULONG DbgPrint(const char *Format, ...);

#define XCHG_PORT 5470
static const char Payload[] = "IPV6XCHG";

static DWORD WINAPI
Watchdog(PVOID Parameter)
{
    UNREFERENCED_PARAMETER(Parameter);
    Sleep(45000);
    DbgPrint("IPV6 XCHG timeout\n");
    ExitProcess(1);
    return 0;
}

static void
FillPeer(SOCKADDR_IN6 *Addr)
{
    memset(Addr, 0, sizeof(*Addr));
    Addr->sin6_family = AF_INET6;
    Addr->sin6_port = htons(XCHG_PORT);
    Addr->sin6_addr.u.Byte[0] = 0xfe;
    Addr->sin6_addr.u.Byte[1] = 0x80;
    Addr->sin6_addr.u.Byte[15] = 0x02;
}

static SOCKET UdpSock;

static DWORD WINAPI
UdpRecv(PVOID Parameter)
{
    char Buf[32];
    SOCKADDR_IN6 From;
    int FromLen = sizeof(From);
    int Got;

    UNREFERENCED_PARAMETER(Parameter);
    Got = recvfrom(UdpSock, Buf, sizeof(Buf), 0, (SOCKADDR *)&From, &FromLen);
    if (Got == (int)(sizeof(Payload) - 1) &&
        memcmp(Buf, Payload, sizeof(Payload) - 1) == 0)
    {
        DbgPrint("IPV6 XCHG udp\n");
    }
    else
    {
        DbgPrint("IPV6 XCHG udp fail %d\n", Got < 0 ? WSAGetLastError() : Got);
    }
    return 0;
}

static void
ExchangeUdp(void)
{
    SOCKADDR_IN6 Peer;
    HANDLE Thread;

    UdpSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (UdpSock == INVALID_SOCKET)
    {
        DbgPrint("IPV6 XCHG udp socket %d\n", WSAGetLastError());
        return;
    }

    /* The UDP path drops a datagram that arrives with no receive pending. */
    Thread = CreateThread(NULL, 0, UdpRecv, NULL, 0, NULL);
    if (!Thread)
    {
        DbgPrint("IPV6 XCHG udp fail %lu\n", GetLastError());
        closesocket(UdpSock);
        return;
    }
    Sleep(500);

    FillPeer(&Peer);
    if (sendto(UdpSock, Payload, sizeof(Payload) - 1, 0,
               (SOCKADDR *)&Peer, sizeof(Peer)) == SOCKET_ERROR)
    {
        DbgPrint("IPV6 XCHG udp send %d\n", WSAGetLastError());
        closesocket(UdpSock);
        WaitForSingleObject(Thread, 2000);
        CloseHandle(Thread);
        return;
    }

    if (WaitForSingleObject(Thread, 8000) != WAIT_OBJECT_0)
        DbgPrint("IPV6 XCHG udp fail 0\n");
    closesocket(UdpSock);
    WaitForSingleObject(Thread, 2000);
    CloseHandle(Thread);
}

static void
ExchangeTcp(void)
{
    SOCKET Sock;
    SOCKADDR_IN6 Peer;
    char Buf[32];
    int Got;
    int Total;

    Sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (Sock == INVALID_SOCKET)
    {
        DbgPrint("IPV6 XCHG tcp socket %d\n", WSAGetLastError());
        return;
    }

    FillPeer(&Peer);
    if (connect(Sock, (SOCKADDR *)&Peer, sizeof(Peer)) == SOCKET_ERROR)
    {
        DbgPrint("IPV6 XCHG tcp connect %d\n", WSAGetLastError());
        closesocket(Sock);
        return;
    }

    if (send(Sock, Payload, sizeof(Payload) - 1, 0) == SOCKET_ERROR)
    {
        DbgPrint("IPV6 XCHG tcp fail %d\n", WSAGetLastError());
        closesocket(Sock);
        return;
    }

    Total = 0;
    while (Total < (int)(sizeof(Payload) - 1))
    {
        Got = recv(Sock, Buf + Total, (int)(sizeof(Payload) - 1) - Total, 0);
        if (Got <= 0)
        {
            DbgPrint("IPV6 XCHG tcp fail %d\n", WSAGetLastError());
            closesocket(Sock);
            return;
        }
        Total += Got;
    }

    if (memcmp(Buf, Payload, sizeof(Payload) - 1) == 0)
        DbgPrint("IPV6 XCHG tcp\n");
    else
        DbgPrint("IPV6 XCHG tcp fail 0\n");

    closesocket(Sock);
}

int
wmain(void)
{
    WSADATA Wsa;

    DbgPrint("IPV6 XCHG start\n");
    CreateThread(NULL, 0, Watchdog, NULL, 0, NULL);

    if (WSAStartup(MAKEWORD(2, 2), &Wsa) != 0)
    {
        DbgPrint("IPV6 XCHG udp fail %d\n", WSAGetLastError());
        DbgPrint("IPV6 XCHG tcp fail %d\n", WSAGetLastError());
        return 1;
    }

    ExchangeUdp();
    ExchangeTcp();
    WSACleanup();
    return 0;
}
