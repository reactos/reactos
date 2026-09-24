/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for local delivery of UDP broadcasts
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ws2_32.h"
#include <iphlpapi.h>

static
VOID
TestBroadcastLoopback(
    _In_ ULONG Destination)
{
    SOCKET RecvSock;
    SOCKET SendSock;
    SOCKADDR_IN Addr;
    INT AddrLen = sizeof(Addr);
    BOOL Enable = TRUE;
    CHAR Buffer[16];
    fd_set Fds;
    struct timeval Timeout = { 2, 0 };
    INT Result;

    RecvSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(RecvSock != INVALID_SOCKET, "socket failed, error %d\n", WSAGetLastError());
    if (RecvSock == INVALID_SOCKET)
        return;

    ZeroMemory(&Addr, sizeof(Addr));
    Addr.sin_family = AF_INET;
    Addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Result = bind(RecvSock, (SOCKADDR *)&Addr, sizeof(Addr));
    ok(Result == 0, "bind failed, error %d\n", WSAGetLastError());

    Result = getsockname(RecvSock, (SOCKADDR *)&Addr, &AddrLen);
    ok(Result == 0, "getsockname failed, error %d\n", WSAGetLastError());

    SendSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ok(SendSock != INVALID_SOCKET, "socket failed, error %d\n", WSAGetLastError());
    if (SendSock == INVALID_SOCKET)
    {
        closesocket(RecvSock);
        return;
    }

    Result = setsockopt(SendSock, SOL_SOCKET, SO_BROADCAST, (PCHAR)&Enable, sizeof(Enable));
    ok(Result == 0, "setsockopt failed, error %d\n", WSAGetLastError());

    Addr.sin_addr.s_addr = Destination;
    Result = sendto(SendSock, "ping", 4, 0, (SOCKADDR *)&Addr, sizeof(Addr));
    ok(Result == 4, "sendto returned %d, error %d\n", Result, WSAGetLastError());

    FD_ZERO(&Fds);
    FD_SET(RecvSock, &Fds);
    Result = select(0, &Fds, NULL, NULL, &Timeout);
    ok(Result == 1, "select returned %d for 0x%08lx\n", Result, ntohl(Destination));

    if (Result == 1)
    {
        Result = recv(RecvSock, Buffer, sizeof(Buffer), 0);
        ok(Result == 4 && !memcmp(Buffer, "ping", 4), "recv returned %d\n", Result);
    }

    closesocket(SendSock);
    closesocket(RecvSock);
}

static
ULONG
GetDirectedBroadcast(VOID)
{
    PMIB_IPADDRTABLE Table;
    ULONG TableSize = 0;
    ULONG Broadcast = 0;
    ULONG Index;

    if (GetIpAddrTable(NULL, &TableSize, FALSE) != ERROR_INSUFFICIENT_BUFFER)
        return 0;

    Table = HeapAlloc(GetProcessHeap(), 0, TableSize);
    if (!Table)
        return 0;

    if (GetIpAddrTable(Table, &TableSize, FALSE) == NO_ERROR)
    {
        for (Index = 0; Index < Table->dwNumEntries; Index++)
        {
            if (Table->table[Index].dwAddr == 0 ||
                Table->table[Index].dwAddr == htonl(INADDR_LOOPBACK))
            {
                continue;
            }

            Broadcast = Table->table[Index].dwAddr | ~Table->table[Index].dwMask;
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, Table);
    return Broadcast;
}

START_TEST(broadcast)
{
    WSADATA WsaData;
    ULONG Directed;

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
    {
        skip("WSAStartup failed\n");
        return;
    }

    TestBroadcastLoopback(htonl(INADDR_BROADCAST));

    Directed = GetDirectedBroadcast();
    if (Directed)
        TestBroadcastLoopback(Directed);
    else
        skip("No configured adapter for directed broadcast\n");

    WSACleanup();
}
