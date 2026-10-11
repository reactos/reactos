/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         User mode part of the TcpIp.sys test suite
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <kmt_test.h>
#include <winsock2.h>

#include "tcpip.h"

typedef struct _ACCEPT_CONTEXT
{
    HANDLE ReadyToConnectEvent;
    SOCKET ListenSocket;
    BOOL WinsockStarted;
    BOOL ListenerReady;
} ACCEPT_CONTEXT, *PACCEPT_CONTEXT;

static
DWORD
LoadTcpIpTestDriver(void)
{
    DWORD Error;

    /* Start the special-purpose driver */
    Error = KmtLoadAndOpenDriver(L"TcpIp", FALSE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return Error;

    return ERROR_SUCCESS;
}

static
void
UnloadTcpIpTestDriver(void)
{
    /* Stop the driver. */
    KmtCloseDriver();
    KmtUnloadDriver();
}

START_TEST(TcpIpTdi)
{
    DWORD Error;

    Error = LoadTcpIpTestDriver();
    ok_eq_int(Error, 0);
    if (Error)
        return;

    Error = KmtSendToDriver(IOCTL_TEST_TDI);
    ok_eq_ulong(Error, ERROR_SUCCESS);

    UnloadTcpIpTestDriver();
}

static
DWORD
WINAPI
AcceptProc(
    _In_ LPVOID Parameter)
{
    WORD WinsockVersion;
    WSADATA WsaData;
    int Error;
    SOCKET ListenSocket, AcceptSocket;
    struct sockaddr_in ListenAddress, AcceptAddress;
    int AcceptAddressLength;
    PACCEPT_CONTEXT AcceptContext = Parameter;

    /* Initialize winsock */
    WinsockVersion = MAKEWORD(2, 0);
    Error = WSAStartup(WinsockVersion, &WsaData);
    if (skip(Error == 0, "WSAStartup failed with %d\n", Error))
    {
        SetEvent(AcceptContext->ReadyToConnectEvent);
        return 0;
    }
    AcceptContext->WinsockStarted = TRUE;

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (skip(ListenSocket != INVALID_SOCKET, "socket failed\n"))
    {
        SetEvent(AcceptContext->ReadyToConnectEvent);
        return 0;
    }
    AcceptContext->ListenSocket = ListenSocket;

    ZeroMemory(&ListenAddress, sizeof(ListenAddress));
    ListenAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    ListenAddress.sin_port = htons(TEST_CONNECT_SERVER_PORT);
    ListenAddress.sin_family = AF_INET;

    Error = bind(ListenSocket, (struct sockaddr*)&ListenAddress, sizeof(ListenAddress));
    if (skip(Error == 0, "bind failed with %d\n", Error))
    {
        SetEvent(AcceptContext->ReadyToConnectEvent);
        closesocket(ListenSocket);
        AcceptContext->ListenSocket = INVALID_SOCKET;
        return 0;
    }

    Error = listen(ListenSocket, 1);
    if (skip(Error == 0, "listen failed with %d\n", Error))
    {
        SetEvent(AcceptContext->ReadyToConnectEvent);
        closesocket(ListenSocket);
        AcceptContext->ListenSocket = INVALID_SOCKET;
        return 0;
    }

    AcceptContext->ListenerReady = TRUE;
    SetEvent(AcceptContext->ReadyToConnectEvent);

    AcceptAddressLength = sizeof(AcceptAddress);
    AcceptSocket = accept(ListenSocket, (struct sockaddr*)&AcceptAddress, &AcceptAddressLength);
    ok(AcceptSocket != INVALID_SOCKET, "accept failed\n");
    if (AcceptSocket != INVALID_SOCKET)
    {
        ok_eq_long(AcceptAddressLength, sizeof(AcceptAddress));
        ok_eq_hex(AcceptAddress.sin_addr.S_un.S_addr, inet_addr("127.0.0.1"));
        ok_eq_hex(AcceptAddress.sin_port, htons(TEST_CONNECT_CLIENT_PORT));
        closesocket(AcceptSocket);
    }

    if (AcceptContext->ListenSocket != INVALID_SOCKET)
    {
        closesocket(ListenSocket);
        AcceptContext->ListenSocket = INVALID_SOCKET;
    }

    return 0;
}

START_TEST(TcpIpConnect)
{
    HANDLE AcceptThread;
    ACCEPT_CONTEXT AcceptContext;
    DWORD Error;

    ZeroMemory(&AcceptContext, sizeof(AcceptContext));
    AcceptContext.ListenSocket = INVALID_SOCKET;

    AcceptContext.ReadyToConnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ok(AcceptContext.ReadyToConnectEvent != NULL, "CreateEvent failed\n");
    if (!AcceptContext.ReadyToConnectEvent)
        return;

    AcceptThread = CreateThread(NULL, 0, AcceptProc, &AcceptContext, 0, NULL);
    ok(AcceptThread != NULL, "CreateThread failed\n");
    if (!AcceptThread)
    {
        CloseHandle(AcceptContext.ReadyToConnectEvent);
        return;
    }

    Error = WaitForSingleObject(AcceptContext.ReadyToConnectEvent, 10 * 1000);
    if (skip(Error == WAIT_OBJECT_0 && AcceptContext.ListenerReady, "TCP listener did not become ready\n"))
    {
        if (AcceptContext.ListenSocket != INVALID_SOCKET)
        {
            closesocket(AcceptContext.ListenSocket);
            AcceptContext.ListenSocket = INVALID_SOCKET;
        }
        WaitForSingleObject(AcceptThread, 10 * 1000);
        CloseHandle(AcceptThread);
        CloseHandle(AcceptContext.ReadyToConnectEvent);
        if (AcceptContext.WinsockStarted)
            WSACleanup();
        return;
    }

    Error = LoadTcpIpTestDriver();
    if (Error)
    {
        if (AcceptContext.ListenSocket != INVALID_SOCKET)
        {
            closesocket(AcceptContext.ListenSocket);
            AcceptContext.ListenSocket = INVALID_SOCKET;
        }
        WaitForSingleObject(AcceptThread, 10 * 1000);
        CloseHandle(AcceptThread);
        CloseHandle(AcceptContext.ReadyToConnectEvent);
        if (AcceptContext.WinsockStarted)
            WSACleanup();
        return;
    }

    Error = KmtSendToDriver(IOCTL_TEST_CONNECT);
    ok_eq_ulong(Error, ERROR_SUCCESS);

    Error = WaitForSingleObject(AcceptThread, 10 * 1000);
    ok(Error == WAIT_OBJECT_0, "AcceptThread timed out\n");

    UnloadTcpIpTestDriver();

    CloseHandle(AcceptThread);
    CloseHandle(AcceptContext.ReadyToConnectEvent);
    if (AcceptContext.WinsockStarted)
        WSACleanup();
}
