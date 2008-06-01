/*
 * PROJECT:     ws2_32.dll API tests
 * LICENSE:     GPLv2 or any later version
 * FILE:        apitests/ws2_32/tests/ioctlsocket.c
 * PURPOSE:     Tests for the ioctlsocket function
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "../ws2_32.h"

/* For valid test results, the ReactOS Website needs to return at least 2 bytes on a "GET / HTTP/1.0" request. */
INT
Test_ioctlsocket(PTESTINFO pti)
{
    LPSTR pszBuf;
    int iResult;
    SOCKET sck;
    ULONG BytesAvailable;
    ULONG BytesToRead;
    WSADATA wdata;

    /* Start up Winsock */
    TEST(WSAStartup(MAKEWORD(2, 2), &wdata) == 0);

    /* If we call ioctlsocket without a socket, it should return with an error and do nothing. */
    BytesAvailable = 0xdeadbeef;
    TEST(ioctlsocket(0, FIONREAD, &BytesAvailable) == SOCKET_ERROR);
    TEST(BytesAvailable == 0xdeadbeef);

    /* Create the socket */
    iResult = CreateSocket(pti, &sck);
    if(iResult != APISTATUS_NORMAL)
        return iResult;

    /* Now we can pass at least a socket, but we have no connection yet. The function should return 0. */
    BytesAvailable = 0xdeadbeef;
    TEST(ioctlsocket(sck, FIONREAD, &BytesAvailable) == 0);
    TEST(BytesAvailable == 0);

    /* Connect to "www.reactos.org" */
    iResult = ConnectToReactOSWebsite(pti, sck);
    if(iResult != APISTATUS_NORMAL)
        return iResult;

    /* Even with a connection, there shouldn't be any bytes available. */
    TEST(ioctlsocket(sck, FIONREAD, &BytesAvailable) == 0);
    TEST(BytesAvailable == 0);

    /* Send the GET request */
    iResult = GetRequestAndWait(pti, sck);
    if(iResult != APISTATUS_NORMAL)
        return iResult;

    /* Try ioctlsocket with FIONREAD. There should be bytes available now. */
    SCKTEST(ioctlsocket(sck, FIONREAD, &BytesAvailable));
    TEST(BytesAvailable != 0);

    /* Get half of the data */
    BytesToRead = BytesAvailable / 2;
    pszBuf = (LPSTR) HeapAlloc(g_hHeap, 0, BytesToRead);
    SCKTEST(recv(sck, pszBuf, BytesToRead, 0));
    HeapFree(g_hHeap, 0, pszBuf);

    BytesToRead = BytesAvailable - BytesToRead;

    /* Now try ioctlsocket again. BytesAvailable should be at the value saved in BytesToRead now. */
    SCKTEST(ioctlsocket(sck, FIONREAD, &BytesAvailable));
    TEST(BytesAvailable == BytesToRead);

    /* Read those bytes */
    pszBuf = (LPSTR) HeapAlloc(g_hHeap, 0, BytesToRead);
    SCKTEST(recv(sck, pszBuf, BytesToRead, 0));
    HeapFree(g_hHeap, 0, pszBuf);

    /* Try it for the last time. BytesAvailable should be at 0 now. */
    SCKTEST(ioctlsocket(sck, FIONREAD, &BytesAvailable));
    TEST(BytesAvailable == 0);

    closesocket(sck);
    WSACleanup();

    return APISTATUS_NORMAL;
}
