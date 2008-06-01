/*
 * PROJECT:     ws2_32.dll API tests
 * LICENSE:     GPLv2 or any later version
 * FILE:        apitests/ws2_32/tests/recv.c
 * PURPOSE:     Tests for the recv function
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "../ws2_32.h"

#define RECV_BUF   4

/* For valid test results, the ReactOS Website needs to return at least 8 bytes on a "GET / HTTP/1.0" request.
   Also the first 4 bytes and the last 4 bytes need to be different.
   Both factors usually apply on standard HTTP responses. */
INT
Test_recv(PTESTINFO pti)
{
    const char szDummyBytes[RECV_BUF] = {0xFF, 0x00, 0xFF, 0x00};

    char szBuf1[RECV_BUF];
    char szBuf2[RECV_BUF];
    int iResult;
    SOCKET sck;
    WSADATA wdata;

    /* Start up Winsock */
    TEST(WSAStartup(MAKEWORD(2, 2), &wdata) == 0);

    /* If we call recv without a socket, it should return with an error and do nothing. */
    memcpy(szBuf1, szDummyBytes, RECV_BUF);
    TEST(recv(0, szBuf1, RECV_BUF, 0) == SOCKET_ERROR);
    TEST(!memcmp(szBuf1, szDummyBytes, RECV_BUF));

    /* Create the socket */
    iResult = CreateSocket(pti, &sck);
    if(iResult != APISTATUS_NORMAL)
        return iResult;

    /* Now we can pass at least a socket, but we have no connection yet. Should return with an error and do nothing. */
    memcpy(szBuf1, szDummyBytes, RECV_BUF);
    TEST(recv(sck, szBuf1, RECV_BUF, 0) == SOCKET_ERROR);
    TEST(!memcmp(szBuf1, szDummyBytes, RECV_BUF));

    /* Connect to "www.reactos.org" */
    iResult = ConnectToReactOSWebsite(pti, sck);
    if(iResult != APISTATUS_NORMAL)
        return iResult;

    /* Send the GET request */
    iResult = GetRequestAndWait(pti, sck);
    if(iResult != APISTATUS_NORMAL)
        return iResult;

    /* Receive the data.
       MSG_PEEK will not change the internal number of bytes read, so that a subsequent request should return the same bytes again. */
    SCKTEST(recv(sck, szBuf1, RECV_BUF, MSG_PEEK));
    SCKTEST(recv(sck, szBuf2, RECV_BUF, 0));
    TEST(!memcmp(szBuf1, szBuf2, RECV_BUF));

    /* The last recv() call moved the internal file pointer, so that the next request should return different data. */
    SCKTEST(recv(sck, szBuf1, RECV_BUF, 0));
    TEST(memcmp(szBuf1, szBuf2, RECV_BUF));

    closesocket(sck);
    WSACleanup();

    return APISTATUS_NORMAL;
}
