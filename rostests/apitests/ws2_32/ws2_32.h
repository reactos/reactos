/*
 * PROJECT:     ws2_32.dll API tests
 * LICENSE:     GPLv2 or any later version
 * FILE:        apitests/ws2_32/ws2_32.h
 * PURPOSE:     Main header file
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#ifndef _WS2_32_APITESTS_H
#define _WS2_32_APITESTS_H

#include <winsock2.h>

#include "../apitest.h"

/* Simple macro for executing a socket command and doing cleanup operations in case of a failure */
#define SCKTEST(_cmd_) \
    iResult = _cmd_; \
    TEST(iResult != SOCKET_ERROR); \
    if(iResult == SOCKET_ERROR) \
    { \
        printf("Winsock error code is %u\n", WSAGetLastError()); \
        closesocket(sck); \
        WSACleanup(); \
        return APISTATUS_ASSERTION_FAILED; \
    }

/* helpers.c */
int CreateSocket(PTESTINFO pti, SOCKET* sck);
int ConnectToReactOSWebsite(PTESTINFO pti, SOCKET sck);
int GetRequestAndWait(PTESTINFO pti, SOCKET sck);

/* ws2_32.c */
extern HANDLE g_hHeap;

#endif
