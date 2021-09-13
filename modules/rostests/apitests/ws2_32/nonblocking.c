/*
* PROJECT:         ReactOS api tests
* LICENSE:         GPL - See COPYING in the top level directory
* PURPOSE:         Test for nonblocking sockets
* PROGRAMMERS:     Peter Hater
*/

#include "ws2_32.h"

#define SVR_PORT 5000
#define WAIT_TIMEOUT_ 10000
#define EXIT_FLAGS (FD_ACCEPT|FD_CONNECT)

START_TEST(nonblocking)
{
    WSADATA    WsaData;
    SOCKET     ServerSocket = INVALID_SOCKET,
               ClientSocket = INVALID_SOCKET;
    struct sockaddr_in server_addr_in;
    struct sockaddr_in addr_remote;
    struct sockaddr_in addr_con_loc;
    int nConRes, err;
    int addrsize;
    SOCKET sockaccept;
    ULONG ulValue = 1;
    DWORD dwFlags = 0, dwLen, dwAddrLen;
    fd_set readfds, writefds, exceptfds;
    struct timeval tval = { 0 };
    char address[100];

    if (!winetest_interactive)
    {
        skip("ROSTESTS-247: Skipping ws2_32_apitest:nonblocking because it times out on testbot\n");
        return;
    }

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
    {
        skip("WSAStartup failed\n");
        return;
    }

    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (ServerSocket == INVALID_SOCKET)
    {
        skip("ERROR: Server socket creation failed\n");
        return;
    }
    if (ClientSocket == INVALID_SOCKET)
    {
        skip("ERROR: Client socket creation failed\n");
        closesocket(ServerSocket);
        return;
    }
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_addr.s_addr = INADDR_ANY;
    server_addr_in.sin_port   = htons(SVR_PORT);

    // Server initialization.
    trace("Initializing server and client connections ...\n");
    err = bind(ServerSocket, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in));
    ok(err == 0, "ERROR: server bind failed\n");
    err = ioctlsocket(ServerSocket, FIONBIO, &ulValue);
    ok(err == 0, "ERROR: server ioctlsocket FIONBIO failed\n");

    // Client initialization.
    err = ioctlsocket(ClientSocket, FIONBIO, &ulValue);
    ok(err == 0, "ERROR: client ioctlsocket FIONBIO failed\n");

    // listen
    trace("Starting server listening mode ...\n");
    err = listen(ServerSocket, 2);
    ok(err == 0, "ERROR: cannot initialize server listen\n");

    trace("Starting client to server connection ...\n");
    // connect
    server_addr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr_in.sin_port = htons(SVR_PORT);
    nConRes = connect(ClientSocket, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in));
    ok(nConRes == SOCKET_ERROR, "ERROR: client connect() result is not SOCKET_ERROR\n");
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "ERROR: client connect() last error is not WSAEWOULDBLOCK\n");
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(ServerSocket, &readfds);

    while (dwFlags != EXIT_FLAGS)
    {
        addrsize = sizeof(addr_con_loc);
        err = getsockname(ClientSocket, (struct sockaddr*)&addr_con_loc, &addrsize);
        if (err == 0)
        {// client connected
            dwLen = sizeof(addr_con_loc);
            dwAddrLen = sizeof(address);
            err = WSAAddressToStringA((PSOCKADDR)&addr_con_loc, dwLen, NULL, address, &dwAddrLen);
            if (err == 0)
            {
                trace("Event FD_CONNECT...\n");
                dwFlags |= FD_CONNECT;
                err = recv(ClientSocket, address, dwAddrLen, 0);
                ok(err == -1, "ERROR: error reading data from connected socket, error %d\n", WSAGetLastError());
                ok(WSAGetLastError() == WSAEWOULDBLOCK, "ERROR: client connect() last error is not WSAEWOULDBLOCK\n");
                err = send(ClientSocket, address, dwAddrLen, 0);
                ok(err == dwAddrLen, "ERROR: error writing data to connected socket, error %d %d\n", err, WSAGetLastError());
            }
            else
            {
                trace("WSAAddressToStringA failed %d\n", WSAGetLastError());
            }
        }

        err = select(1, &readfds, &writefds, &exceptfds, &tval);
        if (err == 1 && FD_ISSET(ServerSocket, &readfds))
        {// connection ready to be accepted
            trace("Event FD_ACCEPT...\n");
            addrsize = sizeof(addr_remote);
            sockaccept = accept(ServerSocket, (struct sockaddr*)&addr_remote, &addrsize);
            ok(sockaccept != INVALID_SOCKET, "ERROR: Connection accept function failed, error %d\n", WSAGetLastError());
            dwFlags |= FD_ACCEPT;
            dwLen = sizeof(addr_remote);
            dwAddrLen = sizeof(address);
            err = WSAAddressToStringA((PSOCKADDR)&addr_remote, dwLen, NULL, address, &dwAddrLen);
            ok(err == 0, "WSAAddressToStringA, error %d\n", WSAGetLastError());
            ok(dwAddrLen > 7, "len <= 7\n");
            err = send(sockaccept, address, dwAddrLen, 0);
            ok(err == dwAddrLen, "ERROR: error sending data on accepted socket, error %d\n", WSAGetLastError());
        }
    }

    closesocket(sockaccept);
    closesocket(ServerSocket);
    closesocket(ClientSocket);

    WSACleanup();
}
