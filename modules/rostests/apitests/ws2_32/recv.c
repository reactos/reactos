/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for recv
 * PROGRAMMERS:     Colin Finck
 */

#include <apitest.h>

#include <stdio.h>
#include <ntstatus.h>
#include <wine/winternl.h>
#include "ws2_32.h"

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

START_TEST(recv)
{
    Test_recv();
}

