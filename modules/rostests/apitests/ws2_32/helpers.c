/*
 * PROJECT:     ws2_32.dll API tests
 * LICENSE:     GPLv2 or any later version
 * FILE:        apitests/ws2_32/helpers.c
 * PURPOSE:     Helper functions for the socket tests
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "ws2_32.h"

int CreateSocket(SOCKET* psck)
{
    /* Create the socket */
    *psck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(*psck != INVALID_SOCKET, "*psck = %d\n", *psck);

    if(*psck == INVALID_SOCKET)
    {
        printf("Winsock error code is %u\n", WSAGetLastError());
        WSACleanup();
        return 0;
    }

    return 1;
}

int ConnectToReactOSWebsite(SOCKET sck)
{
    int iResult;
    struct hostent* host;
    struct sockaddr_in sa;

    /* Connect to "www.reactos.org" */
    host = gethostbyname("test.winehq.org");

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = *(u_long*)host->h_addr_list[0];
    sa.sin_port = htons(80);

    SCKTEST(connect(sck, (struct sockaddr *)&sa, sizeof(sa)));

    return 1;
}

int GetRequestAndWait(SOCKET sck)
{
    const char szGetRequest[] = "GET / HTTP/1.0\r\n\r\n";
    int iResult;
    struct fd_set readable;

    /* Send the GET request */
    SCKTEST(send(sck, szGetRequest, lstrlenA(szGetRequest), 0));
    ok(iResult == strlen(szGetRequest), "iResult = %d\n", iResult);
#if 0 /* breaks windows too */
    /* Shutdown the SEND connection */
    SCKTEST(shutdown(sck, SD_SEND));
#endif
    /* Wait until we're ready to read */
    FD_ZERO(&readable);
    FD_SET(sck, &readable);

    SCKTEST(select(0, &readable, NULL, NULL, NULL));

    return 1;
}
