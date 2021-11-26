/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for WSASocket
 * PROGRAMMERS:     Peter Hater
 */

#include "ws2_32.h"

void Test_CloseDuplicatedSocket()
{
    char szBuf[10];
    int err;
    SOCKET sck, dup_sck;
    WSAPROTOCOL_INFOW ProtocolInfo;
    struct sockaddr_in to = { AF_INET, 2222, {{{ 0x7f, 0x00, 0x00, 0x01 }}} };

    /* Create the socket */
    sck = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sck == INVALID_SOCKET)
    {
        skip("socket failed %d. Aborting test.\n", WSAGetLastError());
        return;
    }

    err = sendto(sck, szBuf, _countof(szBuf), 0, (struct sockaddr *)&to, sizeof(to));
    ok(err == _countof(szBuf), "sendto err = %d %d\n", err, WSAGetLastError());

    err = WSADuplicateSocketW(sck, GetCurrentProcessId(), &ProtocolInfo);
    ok(err == 0, "WSADuplicateSocketW err = %d %d\n", err, WSAGetLastError());

    dup_sck = WSASocketW(0, 0, 0, &ProtocolInfo, 0, 0);
    if (dup_sck == INVALID_SOCKET)
    {
        skip("WSASocketW failed %d. Aborting test.\n", WSAGetLastError());
        closesocket(sck);
        return;
    }

    err = sendto(dup_sck, szBuf, _countof(szBuf), 0, (struct sockaddr *)&to, sizeof(to));
    ok(err == _countof(szBuf), "sendto err = %d %d\n", err, WSAGetLastError());

    err = closesocket(sck);
    ok(err == 0, "closesocket sck err = %d %d\n", err, WSAGetLastError());

    err = sendto(dup_sck, szBuf, _countof(szBuf), 0, (struct sockaddr *)&to, sizeof(to));
    ok(err == _countof(szBuf), "sendto err = %d %d\n", err, WSAGetLastError());

    err = closesocket(dup_sck);
    ok(err == 0, "closesocket dup_sck err = %d %d\n", err, WSAGetLastError());
    return;
}

// 100 ms
#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 100000

// 250 ms
#define TIME_SLEEP1 250

#define THREAD_PROC_LOOPS 5

#define LISTEN_PORT 22222
#define LISTEN_BACKLOG 5

DWORD WINAPI thread_proc(void* param)
{
    fd_set read, write, except;
    struct timeval tval;
    SOCKET sock = (SOCKET)param;
    int i;

    tval.tv_sec = TIMEOUT_SEC;
    tval.tv_usec = TIMEOUT_USEC;

    for (i = 0; i < THREAD_PROC_LOOPS; ++i)
    {
        FD_ZERO(&read); FD_ZERO(&write); FD_ZERO(&except);
        // write will be empty
        FD_SET(sock, &read); FD_SET(sock, &except);

        select(0, &read, &write, &except, &tval);
    }

    return 0;
}

void Test_CloseWhileSelectSameSocket()
{
    int err;
    SOCKET sock;
    struct sockaddr_in addrin;
    HANDLE hthread;

    /* Create the socket */
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        skip("socket failed %d. Aborting test.\n", WSAGetLastError());
        return;
    }

    memset(&addrin, 0, sizeof(struct sockaddr_in));
    addrin.sin_family = AF_INET;
    addrin.sin_addr.s_addr = inet_addr("127.0.0.1");
    addrin.sin_port = htons(LISTEN_PORT);

    err = bind(sock, (struct sockaddr*)(&addrin), sizeof(struct sockaddr_in));
    ok(err == 0, "bind err = %d %d\n", err, WSAGetLastError());
    err = listen(sock, LISTEN_BACKLOG);
    ok(err == 0, "listen err = %d %d\n", err, WSAGetLastError());

    hthread = CreateThread(NULL, 0, thread_proc, (void*)sock, 0, NULL);
    ok(hthread != NULL, "CreateThread %ld\n", GetLastError());

    Sleep(TIME_SLEEP1);
    err = closesocket(sock);
    ok(err == 0, "closesocket err = %d %d\n", err, WSAGetLastError());

    WaitForSingleObject(hthread, INFINITE);
    CloseHandle(hthread);
    return;
}

void Test_CloseWhileSelectDuplicatedSocket()
{
    int err;
    SOCKET sock, dup_sock;
    WSAPROTOCOL_INFOW ProtocolInfo;
    struct sockaddr_in addrin;
    HANDLE hthread;

    /* Create the socket */
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        skip("socket failed %d. Aborting test.\n", WSAGetLastError());
        return;
    }

    memset(&addrin, 0, sizeof(struct sockaddr_in));
    addrin.sin_family = AF_INET;
    addrin.sin_addr.s_addr = inet_addr("127.0.0.1");
    addrin.sin_port = htons(LISTEN_PORT);

    err = bind(sock, (struct sockaddr*)(&addrin), sizeof(struct sockaddr_in));
    ok(err == 0, "bind err = %d %d\n", err, WSAGetLastError());
    err = listen(sock, LISTEN_BACKLOG);
    ok(err == 0, "listen err = %d %d\n", err, WSAGetLastError());

    err = WSADuplicateSocketW(sock, GetCurrentProcessId(), &ProtocolInfo);
    ok(err == 0, "WSADuplicateSocketW err = %d %d\n", err, WSAGetLastError());

    dup_sock = WSASocketW(0, 0, 0, &ProtocolInfo, 0, 0);
    if (dup_sock == INVALID_SOCKET)
    {
        skip("WSASocketW failed %d. Aborting test.\n", WSAGetLastError());
        closesocket(sock);
        return;
    }

    hthread = CreateThread(NULL, 0, thread_proc, (void*)dup_sock, 0, NULL);
    ok(hthread != NULL, "CreateThread %ld\n", GetLastError());

    err = closesocket(sock);
    ok(err == 0, "closesocket err = %d %d\n", err, WSAGetLastError());

    Sleep(TIME_SLEEP1);
    err = closesocket(dup_sock);
    ok(err == 0, "closesocket err = %d %d\n", err, WSAGetLastError());

    WaitForSingleObject(hthread, INFINITE);
    CloseHandle(hthread);
    return;
}

START_TEST(close)
{
    int err;
    WSADATA wdata;

    /* Start up Winsock */
    err = WSAStartup(MAKEWORD(2, 2), &wdata);
    ok(err == 0, "WSAStartup failed, iResult == %d %d\n", err, WSAGetLastError());

    Test_CloseDuplicatedSocket();
    Test_CloseWhileSelectSameSocket();
    Test_CloseWhileSelectDuplicatedSocket();

    WSACleanup();
}

