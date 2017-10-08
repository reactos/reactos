/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         User mode part of the TcpIp.sys test suite
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <kmt_test.h>
#include <winsock2.h>

#include "tcpip.h"

static
void
LoadTcpIpTestDriver(void)
{
    /* Start the special-purpose driver */
    KmtLoadDriver(L"TcpIp", FALSE);
    KmtOpenDriver();
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
    LoadTcpIpTestDriver();

    ok(KmtSendToDriver(IOCTL_TEST_TDI) == ERROR_SUCCESS, "\n");

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
    HANDLE ReadyToConnectEvent = (HANDLE)Parameter;

    /* Initialize winsock */
    WinsockVersion = MAKEWORD(2, 0);
    Error = WSAStartup(WinsockVersion, &WsaData);
    ok(Error == 0, "");

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok_bool_true(ListenSocket != INVALID_SOCKET, "socket failed");

    ZeroMemory(&ListenAddress, sizeof(ListenAddress));
    ListenAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    ListenAddress.sin_port = htons(TEST_CONNECT_SERVER_PORT);
    ListenAddress.sin_family = AF_INET;

    Error = bind(ListenSocket, (struct sockaddr*)&ListenAddress, sizeof(ListenAddress));
    ok(Error == 0, "");

    Error = listen(ListenSocket, 1);
    ok(Error == 0, "");

    SetEvent(ReadyToConnectEvent);

    AcceptAddressLength = sizeof(AcceptAddress);
    AcceptSocket = accept(ListenSocket, (struct sockaddr*)&AcceptAddress, &AcceptAddressLength);
    ok(AcceptSocket != INVALID_SOCKET, "\n");
    ok_eq_long(AcceptAddressLength, sizeof(AcceptAddress));
    ok_eq_hex(AcceptAddress.sin_addr.S_un.S_addr, inet_addr("127.0.0.1"));
    ok_eq_hex(AcceptAddress.sin_port, ntohs(TEST_CONNECT_CLIENT_PORT));

    return 0;
}

START_TEST(TcpIpConnect)
{
    HANDLE AcceptThread;
    HANDLE ReadyToConnectEvent;

    ReadyToConnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ok(ReadyToConnectEvent != NULL, "\n");

    AcceptThread = CreateThread(NULL, 0, AcceptProc, (PVOID)ReadyToConnectEvent, 0, NULL);
    ok(AcceptThread != NULL, "");

    WaitForSingleObject(ReadyToConnectEvent, INFINITE);

    LoadTcpIpTestDriver();

    ok(KmtSendToDriver(IOCTL_TEST_CONNECT) == ERROR_SUCCESS, "\n");

    WaitForSingleObject(AcceptThread, INFINITE);

    UnloadTcpIpTestDriver();

    WSACleanup();
}
