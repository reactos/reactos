/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for listen
 * COPYRIGHT:   Copyright 2026 Tomas Srnka <tomas.srnka@e2b.dev>
 */

#include "ws2_32.h"

typedef struct _LISTEN_CONTEXT
{
    SOCKET Listener;
    struct sockaddr_in Address;
    BOOL Connect;
    SOCKET Client;
    HANDLE Listening;
    HANDLE Accepted;
} LISTEN_CONTEXT, *PLISTEN_CONTEXT;

static SOCKET ConnectWithTimeout(const struct sockaddr_in *Address)
{
    SOCKET Socket;
    u_long NonBlocking = 1;
    fd_set WriteSet, ExceptSet;
    struct timeval Timeout = { 5, 0 };
    int Result;

    Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Socket == INVALID_SOCKET)
        return INVALID_SOCKET;

    if (ioctlsocket(Socket, FIONBIO, &NonBlocking) == SOCKET_ERROR)
    {
        closesocket(Socket);
        return INVALID_SOCKET;
    }

    Result = connect(Socket, (const struct sockaddr *)Address, sizeof(*Address));
    if (Result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
    {
        closesocket(Socket);
        return INVALID_SOCKET;
    }

    FD_ZERO(&WriteSet);
    FD_SET(Socket, &WriteSet);
    FD_ZERO(&ExceptSet);
    FD_SET(Socket, &ExceptSet);
    Result = select(0, NULL, &WriteSet, &ExceptSet, &Timeout);
    if (Result != 1 || !FD_ISSET(Socket, &WriteSet))
    {
        closesocket(Socket);
        return INVALID_SOCKET;
    }

    NonBlocking = 0;
    if (ioctlsocket(Socket, FIONBIO, &NonBlocking) == SOCKET_ERROR)
    {
        closesocket(Socket);
        return INVALID_SOCKET;
    }

    return Socket;
}

static SOCKET AcceptWithTimeout(SOCKET Listener)
{
    fd_set ReadSet;
    struct timeval Timeout = { 5, 0 };

    FD_ZERO(&ReadSet);
    FD_SET(Listener, &ReadSet);
    if (select(0, &ReadSet, NULL, NULL, &Timeout) != 1)
        return INVALID_SOCKET;

    return accept(Listener, NULL, NULL);
}

static void ExchangeByte(SOCKET Client, SOCKET Server, PCSTR What)
{
    fd_set ReadSet;
    struct timeval Timeout = { 5, 0 };
    char Byte = 'x';
    int Result;

    Result = send(Client, &Byte, 1, 0);
    ok(Result == 1, "%s: send returned %d, error %d\n", What, Result, WSAGetLastError());

    Byte = 0;
    FD_ZERO(&ReadSet);
    FD_SET(Server, &ReadSet);
    Result = select(0, &ReadSet, NULL, NULL, &Timeout);
    ok(Result == 1, "%s: select returned %d\n", What, Result);
    if (Result == 1)
    {
        Result = recv(Server, &Byte, 1, 0);
        ok(Result == 1 && Byte == 'x', "%s: recv returned %d, byte %d\n", What, Result, Byte);
    }

    closesocket(Server);
    closesocket(Client);
}

static void CheckConnection(SOCKET Listener, SOCKET Client, PCSTR What)
{
    SOCKET Server;

    ok(Client != INVALID_SOCKET, "%s: connect failed\n", What);
    if (Client == INVALID_SOCKET)
        return;

    Server = AcceptWithTimeout(Listener);
    ok(Server != INVALID_SOCKET, "%s: nothing to accept, error %d\n", What, WSAGetLastError());
    if (Server == INVALID_SOCKET)
    {
        closesocket(Client);
        return;
    }

    ExchangeByte(Client, Server, What);
}

static DWORD WINAPI ListenThread(PVOID Param)
{
    PLISTEN_CONTEXT Context = Param;

    if (listen(Context->Listener, SOMAXCONN) == SOCKET_ERROR)
        return WSAGetLastError();

    SetEvent(Context->Listening);

    if (Context->Connect)
    {
        /* Exit only once the connection made here has been accepted */
        Context->Client = ConnectWithTimeout(&Context->Address);
        WaitForSingleObject(Context->Accepted, 10000);
    }

    return 0;
}

/* The listen stays in effect after the thread that called listen() exits */
static void Test_ListenThreadExit(BOOL Connect)
{
    PLISTEN_CONTEXT Context;
    int AddressLength = sizeof(Context->Address);
    SOCKET Server = INVALID_SOCKET;
    HANDLE Thread;
    DWORD Wait, ExitCode = 0;

    /* A thread that did not exit may still use the context, so it is freed only after the exit */
    Context = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*Context));
    if (!Context)
    {
        skip("HeapAlloc failed\n");
        return;
    }

    Context->Connect = Connect;
    Context->Client = INVALID_SOCKET;
    Context->Listening = CreateEventW(NULL, TRUE, FALSE, NULL);
    Context->Accepted = CreateEventW(NULL, TRUE, FALSE, NULL);
    Context->Listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Context->Listener == INVALID_SOCKET)
    {
        skip("socket failed with %d\n", WSAGetLastError());
        CloseHandle(Context->Listening);
        CloseHandle(Context->Accepted);
        HeapFree(GetProcessHeap(), 0, Context);
        return;
    }

    Context->Address.sin_family = AF_INET;
    Context->Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(Context->Listener, (struct sockaddr *)&Context->Address, sizeof(Context->Address)) ||
        getsockname(Context->Listener, (struct sockaddr *)&Context->Address, &AddressLength))
    {
        skip("bind failed with %d\n", WSAGetLastError());
        closesocket(Context->Listener);
        CloseHandle(Context->Listening);
        CloseHandle(Context->Accepted);
        HeapFree(GetProcessHeap(), 0, Context);
        return;
    }

    Thread = CreateThread(NULL, 0, ListenThread, Context, 0, NULL);
    ok(Thread != NULL, "CreateThread failed with %lu\n", GetLastError());
    if (!Thread)
    {
        closesocket(Context->Listener);
        CloseHandle(Context->Listening);
        CloseHandle(Context->Accepted);
        HeapFree(GetProcessHeap(), 0, Context);
        return;
    }

    if (Connect)
    {
        Wait = WaitForSingleObject(Context->Listening, 10000);
        ok(Wait == WAIT_OBJECT_0, "listen() did not return, wait returned %lu\n", Wait);
        Server = AcceptWithTimeout(Context->Listener);
        ok(Server != INVALID_SOCKET, "Connected before exit: nothing to accept, error %d\n",
           WSAGetLastError());
        SetEvent(Context->Accepted);
    }

    Wait = WaitForSingleObject(Thread, 10000);
    ok(Wait == WAIT_OBJECT_0, "The listening thread did not exit, wait returned %lu\n", Wait);
    if (Wait != WAIT_OBJECT_0)
    {
        /* The thread may still use the context, so it is not freed */
        if (Server != INVALID_SOCKET)
            closesocket(Server);
        closesocket(Context->Listener);
        CloseHandle(Thread);
        return;
    }

    ok(GetExitCodeThread(Thread, &ExitCode) && ExitCode == 0, "listen failed with %lu\n", ExitCode);
    CloseHandle(Thread);

    if (Connect)
    {
        ok(Context->Client != INVALID_SOCKET, "Connected before exit: connect failed\n");
        if (Server != INVALID_SOCKET && Context->Client != INVALID_SOCKET)
        {
            ExchangeByte(Context->Client, Server, "Connected before exit");
        }
        else
        {
            if (Server != INVALID_SOCKET)
                closesocket(Server);
            if (Context->Client != INVALID_SOCKET)
                closesocket(Context->Client);
        }
    }

    CheckConnection(Context->Listener, ConnectWithTimeout(&Context->Address), "First after exit");
    CheckConnection(Context->Listener, ConnectWithTimeout(&Context->Address), "Second after exit");

    closesocket(Context->Listener);
    CloseHandle(Context->Listening);
    CloseHandle(Context->Accepted);
    HeapFree(GetProcessHeap(), 0, Context);
}

START_TEST(listen)
{
    WSADATA WsaData;

    if (WSAStartup(MAKEWORD(2, 2), &WsaData))
    {
        skip("WSAStartup failed\n");
        return;
    }

    Test_ListenThreadExit(FALSE);
    Test_ListenThreadExit(TRUE);

    WSACleanup();
}
