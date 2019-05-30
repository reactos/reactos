/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for WSARecv
 * PROGRAMMERS:     Peter Hater
 */

#include "ws2_32.h"

#define RECV_BUF   4
#define WSARecv_TIMEOUT 2000

static int count = 0;

void
CALLBACK completion(
    DWORD dwError,
    DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags)
{
    //trace("completion called dwFlags %ld cbTransferred %ld lpOverlapped %p dwFlags %ld\n", dwError, cbTransferred, lpOverlapped, dwFlags);
    count++;
    ok(count == 1, "completion sould be called only once count = %d\n", count);
    ok(dwError == 0, "dwError = %ld\n", dwError);
    ok(cbTransferred == RECV_BUF, "cbTransferred %ld != %d\n", cbTransferred, RECV_BUF);
    ok(lpOverlapped != NULL, "lpOverlapped %p\n", lpOverlapped);
    if (lpOverlapped)
    {
        ok(lpOverlapped->hEvent != INVALID_HANDLE_VALUE, "lpOverlapped->hEvent %p\n", lpOverlapped->hEvent);
        if (lpOverlapped->hEvent != INVALID_HANDLE_VALUE)
            WSASetEvent(lpOverlapped->hEvent);
    }
}

void Test_WSARecv()
{
    const char szDummyBytes[RECV_BUF] = { 0xFF, 0x00, 0xFF, 0x00 };

    char szBuf[RECV_BUF];
    char szRecvBuf[RECV_BUF];
    int iResult, err;
    SOCKET sck;
    WSADATA wdata;
    WSABUF buffers;
    DWORD dwRecv, dwSent, dwFlags;
    WSAOVERLAPPED overlapped;
    char szGetRequest[] = "GET / HTTP/1.0\r\n\r\n";
    struct fd_set readable;

    /* Start up Winsock */
    iResult = WSAStartup(MAKEWORD(2, 2), &wdata);
    ok(iResult == 0, "WSAStartup failed, iResult == %d\n", iResult);

    /* If we call recv without a socket, it should return with an error and do nothing. */
    memcpy(szBuf, szDummyBytes, RECV_BUF);
    buffers.buf = szBuf;
    buffers.len = sizeof(szBuf);
    dwFlags = 0;
    dwRecv = 0;
    iResult = WSARecv(0, &buffers, 1, &dwRecv, &dwFlags, NULL, NULL);
    ok(iResult == SOCKET_ERROR, "iRseult = %d\n", iResult);
    ok(!memcmp(szBuf, szDummyBytes, RECV_BUF), "not equal\n");

    /* Create the socket */
    sck = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if(sck == INVALID_SOCKET)
    {
        WSACleanup();
        skip("CreateSocket failed. Aborting test.\n");
        return;
    }

    /* Now we can pass at least a socket, but we have no connection yet. Should return with an error and do nothing. */
    memcpy(szBuf, szDummyBytes, RECV_BUF);
    buffers.buf = szBuf;
    buffers.len = sizeof(szBuf);
    dwFlags = 0;
    dwRecv = 0;
    iResult = WSARecv(sck, &buffers, 1, &dwRecv, &dwFlags, NULL, NULL);
    ok(iResult == SOCKET_ERROR, "iResult = %d\n", iResult);
    ok(!memcmp(szBuf, szDummyBytes, RECV_BUF), "not equal\n");

    /* Connect to "www.reactos.org" */
    if (!ConnectToReactOSWebsite(sck))
    {
        WSACleanup();
        skip("ConnectToReactOSWebsite failed. Aborting test.\n");
        return;
    }

    /* prepare overlapped */
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = WSACreateEvent();

    /* Send the GET request */
    buffers.buf = szGetRequest;
    buffers.len = lstrlenA(szGetRequest);
    dwSent = 0;
    iResult = WSASend(sck, &buffers, 1, &dwSent, 0, &overlapped, NULL);
    err = WSAGetLastError();
    ok(iResult == 0 || (iResult == SOCKET_ERROR && err == WSA_IO_PENDING), "iResult = %d, %d\n", iResult, err);
    if (err == WSA_IO_PENDING)
    {
        iResult = WSAWaitForMultipleEvents(1, &overlapped.hEvent, TRUE, WSARecv_TIMEOUT, TRUE);
        ok(iResult == WSA_WAIT_EVENT_0, "WSAWaitForMultipleEvents failed %d\n", iResult);
        ok(WSAGetOverlappedResult(sck, &overlapped, &dwSent, TRUE, &dwFlags), "WSAGetOverlappedResult failed %d\n", WSAGetLastError());
    }
    ok(dwSent == strlen(szGetRequest), "dwSent %ld != %d\n", dwSent, strlen(szGetRequest));
#if 0 /* break windows too */
    /* Shutdown the SEND connection */
    iResult = shutdown(sck, SD_SEND);
    ok(iResult != SOCKET_ERROR, "iResult = %d\n", iResult);
#endif
    /* Wait until we're ready to read */
    FD_ZERO(&readable);
    FD_SET(sck, &readable);

    iResult = select(0, &readable, NULL, NULL, NULL);
    ok(iResult != SOCKET_ERROR, "iResult = %d\n", iResult);

    /* Receive the data. */
    buffers.buf = szBuf;
    buffers.len = sizeof(szBuf);
    dwRecv = sizeof(szBuf);
    iResult = WSARecv(sck, &buffers, 1, &dwRecv, &dwFlags, NULL, NULL);
    ok(iResult != SOCKET_ERROR, "iResult = %d\n", iResult);
    ok(dwRecv == sizeof(szBuf), "dwRecv %ld != %d\n", dwRecv, sizeof(szBuf));
    /* MSG_PEEK is invalid for overlapped (MSDN), but passes??? */
    buffers.buf = szRecvBuf;
    buffers.len = sizeof(szRecvBuf);
    dwFlags = MSG_PEEK;
    dwRecv = sizeof(szRecvBuf);
    ok(overlapped.hEvent != NULL, "WSACreateEvent failed %d\n", WSAGetLastError());
    iResult = WSARecv(sck, &buffers, 1, &dwRecv, &dwFlags, &overlapped, NULL);
    err = WSAGetLastError();
    ok(iResult == 0 || (iResult == SOCKET_ERROR && err == WSA_IO_PENDING), "iResult = %d, %d\n", iResult, err);
    if (err == WSA_IO_PENDING)
    {
        iResult = WSAWaitForMultipleEvents(1, &overlapped.hEvent, TRUE, WSARecv_TIMEOUT, TRUE);
        ok(iResult == WSA_WAIT_EVENT_0, "WSAWaitForMultipleEvents failed %d\n", iResult);
        ok(WSAGetOverlappedResult(sck, &overlapped, &dwRecv, TRUE, &dwFlags), "WSAGetOverlappedResult failed %d\n", WSAGetLastError());
    }
    ok(dwRecv == sizeof(szRecvBuf), "dwRecv %ld != %d\n", dwRecv, sizeof(szRecvBuf));
    /* normal overlapped, no completion */
    buffers.buf = szBuf;
    buffers.len = sizeof(szBuf);
    dwFlags = 0;
    dwRecv = sizeof(szBuf);
    WSAResetEvent(overlapped.hEvent);
    iResult = WSARecv(sck, &buffers, 1, &dwRecv, &dwFlags, &overlapped, NULL);
    err = WSAGetLastError();
    ok(iResult == 0 || (iResult == SOCKET_ERROR && err == WSA_IO_PENDING), "iResult = %d, %d\n", iResult, err);
    if (err == WSA_IO_PENDING)
    {
        iResult = WSAWaitForMultipleEvents(1, &overlapped.hEvent, TRUE, WSARecv_TIMEOUT, TRUE);
        ok(iResult == WSA_WAIT_EVENT_0, "WSAWaitForMultipleEvents failed %d\n", iResult);
        ok(WSAGetOverlappedResult(sck, &overlapped, &dwRecv, TRUE, &dwFlags), "WSAGetOverlappedResult failed %d\n", WSAGetLastError());
    }
    ok(dwRecv == sizeof(szBuf), "dwRecv %ld != %d\n", dwRecv, sizeof(szBuf));
    ok(memcmp(szRecvBuf, szBuf, sizeof(szBuf)) == 0, "MSG_PEEK shouldn't have moved the pointer\n");
    /* overlapped with completion */
    dwFlags = 0;
    dwRecv = sizeof(szBuf);
    WSAResetEvent(overlapped.hEvent);
    iResult = WSARecv(sck, &buffers, 1, &dwRecv, &dwFlags, &overlapped, &completion);
    err = WSAGetLastError();
    ok(iResult == 0 || (iResult == SOCKET_ERROR && err == WSA_IO_PENDING), "iResult = %d, %d\n", iResult, err);
    if (err == WSA_IO_PENDING)
    {
        iResult = WSAWaitForMultipleEvents(1, &overlapped.hEvent, TRUE, WSARecv_TIMEOUT, TRUE);
        ok(iResult == WSA_WAIT_EVENT_0, "WSAWaitForMultipleEvents failed %d\n", iResult);
        ok(WSAGetOverlappedResult(sck, &overlapped, &dwRecv, TRUE, &dwFlags), "WSAGetOverlappedResult failed %d\n", WSAGetLastError());
    }
    ok(WSACloseEvent(overlapped.hEvent), "WSAGetOverlappedResult failed %d\n", WSAGetLastError());
    ok(dwRecv == sizeof(szBuf), "dwRecv %ld != %d\n", dwRecv, sizeof(szBuf));
    /* no overlapped with completion */
    dwFlags = 0;
    dwRecv = sizeof(szBuf);
    /* call doesn't fail, but completion is not called */
    iResult = WSARecv(sck, &buffers, 1, &dwRecv, &dwFlags, NULL, &completion);
    err = WSAGetLastError();
    ok(iResult == 0 || (iResult == SOCKET_ERROR && err == WSA_IO_PENDING), "iResult = %d, %d\n", iResult, err);
    ok(WSAGetLastError() == 0, "WSAGetLastError failed %d\n", WSAGetLastError());
    ok(dwRecv == sizeof(szBuf), "dwRecv %ld != %d and 0\n", dwRecv, sizeof(szBuf));

    closesocket(sck);
    WSACleanup();
    return;
}

START_TEST(WSARecv)
{
    Test_WSARecv();
}

