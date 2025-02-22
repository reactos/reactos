/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for getaddrinfo
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "ws2_32.h"

START_TEST(getnameinfo)
{
    WSADATA WsaData;
    int Error, MinSize;
    PCHAR NodeBuffer, ServiceBuffer;
    CHAR TestBuf[NI_MAXHOST];
    WCHAR TestBufW[NI_MAXHOST];
    SOCKADDR_IN LocalAddr;

    /* not yet initialized */
    StartSeh()
        Error = getnameinfo(NULL, 0, NULL, 0, NULL, 0, 0);
        ok_dec(Error, WSANOTINITIALISED);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        NodeBuffer = InvalidPointer;
        ServiceBuffer = InvalidPointer;
        Error = getnameinfo(NULL, 0, NodeBuffer, 0, ServiceBuffer, 0, 0);
        ok_dec(Error, WSANOTINITIALISED);
        ok_ptr(NodeBuffer, InvalidPointer);
        ok_ptr(ServiceBuffer, InvalidPointer);
    EndSeh(STATUS_SUCCESS);

    LocalAddr.sin_family = AF_INET;
    LocalAddr.sin_port = 80;
    LocalAddr.sin_addr.S_un.S_addr = ntohl(INADDR_LOOPBACK);
    Error = getnameinfo((PSOCKADDR)&LocalAddr, sizeof(LocalAddr), TestBuf, sizeof(TestBuf), NULL, 0, 0);
    ok_dec(Error, WSANOTINITIALISED);

    Error = WSAStartup(MAKEWORD(2, 2), &WsaData);
    ok_dec(Error, 0);

    StartSeh()
        Error = getnameinfo(NULL, 0, NULL, 0, NULL, 0, 0);
        ok_dec(Error, WSAEFAULT);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        NodeBuffer = InvalidPointer;
        ServiceBuffer = InvalidPointer;
        Error = getnameinfo(NULL, 0, NodeBuffer, 0, ServiceBuffer, 0, 0);
        ok_dec(Error, WSAEFAULT);
        ok_ptr(NodeBuffer, InvalidPointer);
        ok_ptr(ServiceBuffer, InvalidPointer);
    EndSeh(STATUS_SUCCESS);

    /* initialize LocalAddress for tests */
    Error = getnameinfo((PSOCKADDR)&LocalAddr, sizeof(LocalAddr), TestBuf, sizeof(TestBuf), NULL, 0, 0);
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);

    /* test minimal size */
    MinSize = sizeof(LocalAddr);
    Error = getnameinfo((PSOCKADDR)&LocalAddr, MinSize, TestBuf, sizeof(TestBuf), NULL, 0, 0);
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);

    Error = GetNameInfoA((PSOCKADDR)&LocalAddr, MinSize, TestBuf, sizeof(TestBuf), NULL, 0, 0);
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);

    Error = GetNameInfoW((PSOCKADDR)&LocalAddr, MinSize, TestBufW, sizeof(TestBufW), NULL, 0, 0);
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);

    MinSize--;
    Error = getnameinfo((PSOCKADDR)&LocalAddr, MinSize, TestBuf, sizeof(TestBuf), NULL, 0, 0);
    ok_dec(Error, WSAEFAULT);

    Error = GetNameInfoA((PSOCKADDR)&LocalAddr, MinSize, TestBuf, sizeof(TestBuf), NULL, 0, 0);
    ok_dec(Error, WSAEFAULT);

    Error = GetNameInfoW((PSOCKADDR)&LocalAddr, MinSize, TestBufW, sizeof(TestBufW), NULL, 0, 0);
    ok_dec(Error, WSAEFAULT);

    Error = WSACleanup();
    ok_dec(Error, 0);

    /* not initialized anymore */
    Error = getnameinfo((PSOCKADDR)&LocalAddr, sizeof(LocalAddr), NodeBuffer, 0, ServiceBuffer, 0, 0);
    ok_dec(Error, WSANOTINITIALISED);
}
