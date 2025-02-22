/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL - See COPYING in the top level directory
 * PURPOSE:     Test for recv
 * COPYRIGHT:   Copyright 2008 Colin Finck (colin@reactos.org)
 *              Copyright 2010 Timo Kreuzer (timo.kreuzer@reactos.org)
 *              Copyright 2012 Cameron Gutman (cameron.gutman@reactos.org)
 *              Copyright 2023 Thomas Faber (thomas.faber@reactos.org)
 */

#include "ws2_32.h"

#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>

#define RECV_BUF   4

/* For valid test results, the ReactOS Website needs to return at least 8 bytes on a "GET / HTTP/1.0" request.
   Also the first 4 bytes and the last 4 bytes need to be different.
   Both factors usually apply on standard HTTP responses. */

int Test_recv()
{
    const char szDummyBytes[RECV_BUF] = {0xFF, 0x00, 0xFF, 0x00};

    char szBuf1[RECV_BUF];
    char szBuf2[RECV_BUF];
    int iResult;
    SOCKET sck;
    WSADATA wdata;
    NTSTATUS status;
    IO_STATUS_BLOCK readIosb;
    HANDLE readEvent;
    LARGE_INTEGER readOffset;

    /* Start up Winsock */
    iResult = WSAStartup(MAKEWORD(2, 2), &wdata);
    ok(iResult == 0, "WSAStartup failed, iResult == %d\n", iResult);

    /* If we call recv without a socket, it should return with an error and do nothing. */
    memcpy(szBuf1, szDummyBytes, RECV_BUF);
    iResult = recv(0, szBuf1, RECV_BUF, 0);
    ok(iResult == SOCKET_ERROR, "iRseult = %d\n", iResult);
    ok(!memcmp(szBuf1, szDummyBytes, RECV_BUF), "not equal\n");

    /* Create the socket */
    if (!CreateSocket(&sck))
    {
        ok(0, "CreateSocket failed. Aborting test.\n");
        return 0;
    }

    /* Now we can pass at least a socket, but we have no connection yet. Should return with an error and do nothing. */
    memcpy(szBuf1, szDummyBytes, RECV_BUF);
    iResult = recv(sck, szBuf1, RECV_BUF, 0);
    ok(iResult == SOCKET_ERROR, "iResult = %d\n", iResult);
    ok(!memcmp(szBuf1, szDummyBytes, RECV_BUF), "not equal\n");

    /* Connect to "www.reactos.org" */
    if (!ConnectToReactOSWebsite(sck))
    {
        ok(0, "ConnectToReactOSWebsite failed. Aborting test.\n");
        return 0;
    }

    /* Send the GET request */
    if (!GetRequestAndWait(sck))
    {
        ok(0, "GetRequestAndWait failed. Aborting test.\n");
        return 0;
    }

    /* Receive the data.
       MSG_PEEK will not change the internal number of bytes read, so that a subsequent request should return the same bytes again. */
    SCKTEST(recv(sck, szBuf1, RECV_BUF, MSG_PEEK));
    SCKTEST(recv(sck, szBuf2, RECV_BUF, 0));
    ok(!memcmp(szBuf1, szBuf2, RECV_BUF), "not equal\n");

    /* The last recv() call moved the internal file pointer, so that the next request should return different data. */
    SCKTEST(recv(sck, szBuf1, RECV_BUF, 0));
    ok(memcmp(szBuf1, szBuf2, RECV_BUF), "equal\n");

    /* Create an event for NtReadFile */
    readOffset.QuadPart = 0LL;
    memcpy(szBuf1, szBuf2, RECV_BUF);
    status = NtCreateEvent(&readEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);
    if (status != 0)
    {
        ok(0, "Failed to create event\n");
        return 0;
    }

    /* Try reading the socket using the NT file API */
    status = NtReadFile((HANDLE)sck,
                        readEvent,
                        NULL,
                        NULL,
                        &readIosb,
                        szBuf1,
                        RECV_BUF,
                        &readOffset,
                        NULL);
    if (status == STATUS_PENDING)
    {
        WaitForSingleObject(readEvent, INFINITE);
        status = readIosb.Status;
    }

    ok(status == 0, "Read failed with status 0x%x\n", (unsigned int)status);
    ok(memcmp(szBuf2, szBuf1, RECV_BUF), "equal\n");
    ok(readIosb.Information == RECV_BUF, "Short read\n");

    NtClose(readEvent);
    closesocket(sck);
    WSACleanup();
    return 1;
}

static void Test_Overread(void)
{
    WSADATA wsaData;
    SOCKET ListeningSocket = INVALID_SOCKET;
    SOCKET ServerSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;
    SOCKADDR_IN address;
    char buffer[32];
    int ret;
    int error;
    int len;
    struct
    {
        char buffer[32];
        DWORD flags;
        WSAOVERLAPPED overlapped;
    } receivers[4] = { 0 };
    DWORD bytesTransferred;
    DWORD flags;
    WSABUF wsaBuf;
    size_t i;

    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0)
    {
        skip("Failed to initialize WinSock, error %d\n", ret);
        goto Exit;
    }

    ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListeningSocket == INVALID_SOCKET)
    {
        skip("Failed to create listening socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    /* Bind to random port */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    ret = bind(ListeningSocket, (SOCKADDR *)&address, sizeof(address));
    if (ret == SOCKET_ERROR)
    {
        skip("Failed to bind socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    ret = listen(ListeningSocket, 1);
    if (ret == SOCKET_ERROR)
    {
        skip("Failed to listen on socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    len = sizeof(address);
    ret = getsockname(ListeningSocket, (SOCKADDR *)&address, &len);
    if (ret == SOCKET_ERROR)
    {
        skip("Failed to get listening port on socket, error %d\n", WSAGetLastError());
        goto Exit;
    }
    ok(len == sizeof(address), "getsocketname length %d\n", len);

    /**************************************************************************
     * Test 1: non-overlapped client socket
     *************************************************************************/
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET)
    {
        skip("Failed to create client socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    ret = connect(ClientSocket, (SOCKADDR *)&address, sizeof(address));
    if (ret == SOCKET_ERROR)
    {
        skip("Failed to connect to socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    ServerSocket = accept(ListeningSocket, NULL, NULL);
    if (ServerSocket == INVALID_SOCKET)
    {
        skip("Failed to accept client socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    ret = send(ServerSocket, "blah", 4, 0);
    if (ret == SOCKET_ERROR)
    {
        skip("Failed to send to socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    ret = closesocket(ServerSocket);
    ServerSocket = INVALID_SOCKET;
    ok(ret == 0, "Failed to close socket with %d\n", WSAGetLastError());

    memset(buffer, 0, sizeof(buffer));
    ret = recv(ClientSocket, buffer, sizeof(buffer), 0);
    error = WSAGetLastError();
    ok(ret == 4, "recv (1) returned %d\n", ret);
    ok(error == NO_ERROR, "recv (1) returned error %d\n", error);
    buffer[4] = 0;
    ok(!strcmp(buffer, "blah"), "recv (1) returned data: %s\n", buffer);

    ret = recv(ClientSocket, buffer, sizeof(buffer), 0);
    error = WSAGetLastError();
    ok(ret == 0, "recv (2) returned %d\n", ret);
    ok(error == NO_ERROR, "recv (2) returned error %d\n", error);

    ret = recv(ClientSocket, buffer, sizeof(buffer), 0);
    error = WSAGetLastError();
    ok(ret == 0, "recv (3) returned %d\n", ret);
    ok(error == NO_ERROR, "recv (3) returned error %d\n", error);

    closesocket(ClientSocket);

    /**************************************************************************
     * Test 2: overlapped client socket with multiple pending receives
     *************************************************************************/
    ClientSocket = WSASocketW(AF_INET,
                              SOCK_STREAM,
                              IPPROTO_TCP,
                              NULL,
                              0,
                              WSA_FLAG_OVERLAPPED);
    if (ClientSocket == INVALID_SOCKET)
    {
        skip("Failed to create overlapped client socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    ret = connect(ClientSocket, (SOCKADDR *)&address, sizeof(address));
    if (ret == SOCKET_ERROR)
    {
        skip("Failed to connect to socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    ServerSocket = accept(ListeningSocket, NULL, NULL);
    if (ServerSocket == INVALID_SOCKET)
    {
        skip("Failed to accept client socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    /* Start overlapping receive calls */
    for (i = 0; i < RTL_NUMBER_OF(receivers); i++)
    {
        wsaBuf.len = sizeof(receivers[i].buffer);
        wsaBuf.buf = receivers[i].buffer;
        receivers[i].flags = 0;
        receivers[i].overlapped.hEvent = WSACreateEvent();
        ret = WSARecv(ClientSocket,
                      &wsaBuf,
                      1,
                      NULL,
                      &receivers[i].flags,
                      &receivers[i].overlapped,
                      NULL);
        error = WSAGetLastError();
        ok(ret == SOCKET_ERROR, "[%Iu] WSARecv returned %d\n", i, ret);
        ok(error == WSA_IO_PENDING, "[%Iu] WSARecv returned error %d\n", i, error);
    }

    /* They should all be pending */
    for (i = 0; i < RTL_NUMBER_OF(receivers); i++)
    {
        ret = WSAGetOverlappedResult(ClientSocket,
                                     &receivers[i].overlapped,
                                     &bytesTransferred,
                                     FALSE,
                                     &flags);
        error = WSAGetLastError();
        ok(ret == FALSE, "[%Iu] WSAGetOverlappedResult returned %d\n", i, ret);
        ok(error == WSA_IO_INCOMPLETE, "[%Iu] WSAGetOverlappedResult returned error %d\n", i, error);
    }

    /* Sending some data should complete the first receive */
    ret = send(ServerSocket, "blah", 4, 0);
    if (ret == SOCKET_ERROR)
    {
        skip("Failed to send to socket, error %d\n", WSAGetLastError());
        goto Exit;
    }

    flags = 0x55555555;
    bytesTransferred = 0x55555555;
    ret = WSAGetOverlappedResult(ClientSocket,
                                 &receivers[0].overlapped,
                                 &bytesTransferred,
                                 FALSE,
                                 &flags);
    error = WSAGetLastError();
    ok(ret == TRUE, "WSAGetOverlappedResult returned %d\n", ret);
    ok(flags == 0, "WSAGetOverlappedResult returned flags 0x%lx\n", flags);
    ok(bytesTransferred == 4, "WSAGetOverlappedResult returned %lu bytes\n", bytesTransferred);
    receivers[0].buffer[4] = 0;
    ok(!strcmp(receivers[0].buffer, "blah"), "WSARecv returned data: %s\n", receivers[0].buffer);

    /* Others should still be in progress */
    for (i = 1; i < RTL_NUMBER_OF(receivers); i++)
    {
        ret = WSAGetOverlappedResult(ClientSocket,
                                     &receivers[i].overlapped,
                                     &bytesTransferred,
                                     FALSE,
                                     &flags);
        error = WSAGetLastError();
        ok(ret == FALSE, "[%Iu] WSAGetOverlappedResult returned %d\n", i, ret);
        ok(error == WSA_IO_INCOMPLETE, "[%Iu] WSAGetOverlappedResult returned error %d\n", i, error);
    }

    /* Closing the server end should make all receives complete */
    ret = closesocket(ServerSocket);
    ServerSocket = INVALID_SOCKET;
    ok(ret == 0, "Failed to close socket with %d\n", WSAGetLastError());

    for (i = 1; i < RTL_NUMBER_OF(receivers); i++)
    {
        flags = 0x55555555;
        bytesTransferred = 0x55555555;
        ret = WSAGetOverlappedResult(ClientSocket,
                                     &receivers[i].overlapped,
                                     &bytesTransferred,
                                     FALSE,
                                     &flags);
        error = WSAGetLastError();
        ok(ret == TRUE, "[%Iu] WSAGetOverlappedResult returned %d\n", i, ret);
        ok(flags == 0, "[%Iu] WSAGetOverlappedResult returned flags 0x%lx\n", i, flags);
        ok(bytesTransferred == 0, "[%Iu] WSAGetOverlappedResult returned %lu bytes\n", i, bytesTransferred);
    }

    /* Start two more receives -- they should immediately return success */
    ret = recv(ClientSocket, receivers[0].buffer, sizeof(receivers[0].buffer), 0);
    error = WSAGetLastError();
    ok(ret == 0, "recv (N+1) returned %d\n", ret);
    ok(error == NO_ERROR, "recv (N+1) returned error %d\n", error);

    ret = recv(ClientSocket, receivers[0].buffer, sizeof(receivers[0].buffer), 0);
    error = WSAGetLastError();
    ok(ret == 0, "recv (N+2) returned %d\n", ret);
    ok(error == NO_ERROR, "recv (N+2) returned error %d\n", error);

Exit:
    for (i = 0; i < RTL_NUMBER_OF(receivers); i++)
    {
        if (receivers[i].overlapped.hEvent != NULL)
        {
            WSACloseEvent(receivers[i].overlapped.hEvent);
        }
    }

    if (ListeningSocket != INVALID_SOCKET)
    {
        ret = closesocket(ListeningSocket);
        ok(ret == 0, "closesocket (1) failed with %d\n", WSAGetLastError());
    }
    if (ClientSocket != INVALID_SOCKET)
    {
        ret = closesocket(ClientSocket);
        ok(ret == 0, "closesocket (2) failed with %d\n", WSAGetLastError());
    }
    if (ServerSocket != INVALID_SOCKET)
    {
        ret = closesocket(ServerSocket);
        ok(ret == 0, "closesocket (3) failed with %d\n", WSAGetLastError());
    }
    ret = WSACleanup();
    ok(ret == 0, "WSACleanup failed with %d\n", WSAGetLastError());
}

START_TEST(recv)
{
    Test_recv();
    Test_Overread();
}

