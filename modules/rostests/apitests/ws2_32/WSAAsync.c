/*
* PROJECT:         ReactOS api tests
* LICENSE:         GPL - See COPYING in the top level directory
* PURPOSE:         Test for WSAAsync
* PROGRAMMERS:     Miroslav Mastny
*/

#include "ws2_32.h"

#define SVR_PORT 5000
#define WAIT_TIMEOUT_ 10000
#define EXIT_FLAGS (FD_ACCEPT|FD_CONNECT)
#define MAX_LOOPCOUNT 9u

START_TEST(WSAAsync)
{
    WSADATA    WsaData;
    SOCKET     ServerSocket = INVALID_SOCKET,
               ClientSocket = INVALID_SOCKET;
    WSAEVENT   ServerEvent = WSA_INVALID_EVENT,
               ClientEvent = WSA_INVALID_EVENT;
    struct hostent *ent = NULL;
    struct sockaddr_in server_addr_in;
    struct sockaddr_in addr_remote;
    struct sockaddr_in addr_con_loc;
    int nConRes, nSockNameRes;
    int addrsize, len;
    WSAEVENT fEvents[2];
    SOCKET fSockets[2];
    SOCKET sockaccept;
    WSANETWORKEVENTS WsaNetworkEvents;
    ULONG ulValue = 1;
    DWORD dwWait;
    DWORD dwFlags = 0;
    struct fd_set select_rfds;
    struct fd_set select_wfds;
    struct fd_set select_efds;
    struct timeval timeval;
    BOOL ConnectSent = FALSE;
    unsigned int Addr_con_locLoopCount = 0,
                 ServerSocketLoopCount = 0;

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
    {
        skip("WSAStartup failed\n");
        return;
    }

    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ServerEvent  = WSACreateEvent();
    ClientEvent  = WSACreateEvent();

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
    if (ServerEvent == WSA_INVALID_EVENT)
    {
        skip("ERROR: Server WSAEvent creation failed\n");
        closesocket(ClientSocket);
        closesocket(ServerSocket);
        return;
    }
    if (ClientEvent == WSA_INVALID_EVENT)
    {
        skip("ERROR: Client WSAEvent creation failed\n");
        WSACloseEvent(ServerEvent);
        closesocket(ClientSocket);
        closesocket(ServerSocket);
        return;
    }
    ent = gethostbyname("127.0.0.1");
    if (ent == NULL)
    {
        ok(ent != NULL, "ERROR: gethostbyname '127.0.0.1' failed, trying 'localhost'\n");
        ent = gethostbyname("localhost");

        if (ent == NULL)
        {
            skip("ERROR: gethostbyname 'localhost' failed\n");
            goto done;
        }
    }

    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_port   = htons(SVR_PORT);
    memcpy(&server_addr_in.sin_addr.S_un.S_addr, ent->h_addr_list[0], 4);

    // Server initialization.
    trace("Initializing server and client connections ...\n");
    ok(bind(ServerSocket, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in)) == 0, "ERROR: server bind failed\n");
    ok(ioctlsocket(ServerSocket, FIONBIO, &ulValue) == 0, "ERROR: server ioctlsocket FIONBIO failed\n");
    ok(WSAEventSelect(ServerSocket, ServerEvent, FD_ACCEPT | FD_CLOSE) == 0, "ERROR: server accept EventSelect failed\n");

    // Client initialization.
    ok(WSAEventSelect(ClientSocket, ClientEvent, FD_CONNECT | FD_CLOSE) == 0, "ERROR: client EventSelect failed\n");
    ok(ioctlsocket(ClientSocket, FIONBIO, &ulValue) == 0, "ERROR: client ioctlsocket FIONBIO failed\n");

    // listen
    trace("Starting server listening mode ...\n");
    ok(listen(ServerSocket, SOMAXCONN) == 0, "ERROR: cannot initialize server listen\n");

    trace("Starting client to server connection ...\n");
    // connect
    nConRes = connect(ClientSocket, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in));
    ok(nConRes == SOCKET_ERROR, "ERROR: client connect() result is not SOCKET_ERROR\n");
    ok(WSAGetLastError() == WSAEWOULDBLOCK, "ERROR: client connect() last error is not WSAEWOULDBLOCK\n");

    fSockets[0] = ServerSocket;
    fSockets[1] = ClientSocket;

    fEvents[0] = ServerEvent;
    fEvents[1] = ClientEvent;

    while (dwFlags != EXIT_FLAGS)
    {
        dwWait = WaitForMultipleObjects(2, fEvents, FALSE, WAIT_TIMEOUT_);

        if (dwWait != WAIT_OBJECT_0 && // server socket event
            dwWait != WAIT_OBJECT_0+1) // client socket event
        {
            ok(FALSE, "Unknown event received %lu\n", dwWait);
            skip("ERROR: Connection timeout\n");
            break;
        }

        WSAEnumNetworkEvents(fSockets[dwWait-WAIT_OBJECT_0], fEvents[dwWait-WAIT_OBJECT_0], &WsaNetworkEvents);

        if ((WsaNetworkEvents.lNetworkEvents & FD_ACCEPT) != 0)
        {// connection accepted
            trace("Event FD_ACCEPT...\n");
            ok(WsaNetworkEvents.iErrorCode[FD_ACCEPT_BIT] == 0, "Error on accept %d\n", WsaNetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
            if (WsaNetworkEvents.iErrorCode[FD_ACCEPT_BIT] == 0)
            {
                addrsize = sizeof(addr_remote);
                sockaccept = accept(fSockets[dwWait - WAIT_OBJECT_0], (struct sockaddr*)&addr_remote, &addrsize);
                ok(sockaccept != INVALID_SOCKET, "ERROR: Connection accept function failed, error %d\n", WSAGetLastError());
                dwFlags |= FD_ACCEPT;
            }
        }

        if ((WsaNetworkEvents.lNetworkEvents & FD_CONNECT) != 0)
        {// client connected
            trace("Event FD_CONNECT...\n");
            ok(WsaNetworkEvents.iErrorCode[FD_CONNECT_BIT] == 0, "Error on connect %d\n", WsaNetworkEvents.iErrorCode[FD_CONNECT_BIT]);
            if (WsaNetworkEvents.iErrorCode[FD_CONNECT_BIT] == 0)
            {
                len = sizeof(addr_con_loc);
                ok(getsockname(fSockets[dwWait - WAIT_OBJECT_0], (struct sockaddr*)&addr_con_loc, &len) == 0, "\n");
                dwFlags |= FD_CONNECT;
            }
        }
    }
    closesocket(sockaccept);
    closesocket(ServerSocket);
    closesocket(ClientSocket);

    /* same test but with waiting select and getsockname to return proper values */
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
    ent = gethostbyname("127.0.0.1");
    if (ent == NULL)
    {
        ok(ent != NULL, "ERROR: gethostbyname '127.0.0.1' failed, trying 'localhost'\n");
        ent = gethostbyname("localhost");

        if (ent == NULL)
        {
            skip("ERROR: gethostbyname 'localhost' failed\n");
            goto done;
        }
    }

    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_port = htons(SVR_PORT);
    memcpy(&server_addr_in.sin_addr.S_un.S_addr, ent->h_addr_list[0], 4);

    // Server initialization.
    trace("Initializing server and client connections ...\n");
    ok(bind(ServerSocket, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in)) == 0, "ERROR: server bind failed\n");
    ok(ioctlsocket(ServerSocket, FIONBIO, &ulValue) == 0, "ERROR: server ioctlsocket FIONBIO failed\n");

    // Client initialization.
    ok(ioctlsocket(ClientSocket, FIONBIO, &ulValue) == 0, "ERROR: client ioctlsocket FIONBIO failed\n");

    // listen
    trace("Starting server listening mode ...\n");
    ok(listen(ServerSocket, SOMAXCONN) == 0, "ERROR: cannot initialize server listen\n");

    memset(&timeval, 0, sizeof(timeval));
    timeval.tv_usec = WAIT_TIMEOUT_;
    dwFlags = 0;

    while (dwFlags != EXIT_FLAGS)
    {
        len = sizeof(addr_con_loc);
        nSockNameRes = getsockname(ClientSocket, (struct sockaddr*)&addr_con_loc, &len);
        if (dwFlags == 0 && !ConnectSent)
        {
            ok(nSockNameRes == SOCKET_ERROR, "ERROR: getsockname function failed, expected %d error %d\n", SOCKET_ERROR, nSockNameRes);
            ok(WSAGetLastError() == WSAEINVAL, "ERROR: getsockname function failed, expected %ld error %d\n", WSAEINVAL, WSAGetLastError());
            trace("Starting client to server connection ...\n");
            // connect
            nConRes = connect(ClientSocket, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in));
            ok(nConRes == SOCKET_ERROR, "ERROR: client connect() result is not SOCKET_ERROR\n");
            ok(WSAGetLastError() == WSAEWOULDBLOCK, "ERROR: client connect() last error is not WSAEWOULDBLOCK\n");
            ConnectSent = TRUE;
            continue;
        }
        else
        {
            if (nSockNameRes != 0)
                ok(FALSE, "ERROR: getsockname function failed, expected 0 error %d\n", nSockNameRes);
            if (len != sizeof(addr_con_loc))
                ok(FALSE, "ERROR: getsockname function wrong size, expected %Iu returned %d\n", sizeof(addr_con_loc), len);

            if (addr_con_loc.sin_addr.s_addr == 0ul)
            {
                if (++Addr_con_locLoopCount >= MAX_LOOPCOUNT)
                {
                    ok(FALSE, "Giving up, on getsockname() (%u/%u), as addr_con_loc is not set yet\n",
                       Addr_con_locLoopCount, MAX_LOOPCOUNT);
                    goto done;
                }

                trace("Looping, for getsockname() (%u/%u), as addr_con_loc is not set yet\n",
                      Addr_con_locLoopCount, MAX_LOOPCOUNT);
                Sleep(1);
                continue;
            }

            if (addr_con_loc.sin_addr.s_addr != server_addr_in.sin_addr.s_addr)
                ok(FALSE, "ERROR: getsockname function wrong addr, expected %08lx returned %08lx\n", server_addr_in.sin_addr.s_addr, addr_con_loc.sin_addr.s_addr);
        }
        if ((dwFlags & FD_ACCEPT) != 0)
        {// client connected
            trace("Select CONNECT...\n");
            dwFlags |= FD_CONNECT;
        }

        FD_ZERO(&select_rfds);
        FD_ZERO(&select_wfds);
        FD_ZERO(&select_efds);
        FD_SET(ServerSocket, &select_rfds);
        FD_SET(ClientSocket, &select_rfds);
        FD_SET(ServerSocket, &select_wfds);
        FD_SET(ClientSocket, &select_wfds);
        FD_SET(ServerSocket, &select_efds);
        FD_SET(ClientSocket, &select_efds);
        if ((dwFlags & FD_ACCEPT) != 0)
        {
            FD_SET(sockaccept, &select_rfds);
            FD_SET(sockaccept, &select_wfds);
            FD_SET(sockaccept, &select_efds);
        }
        if (select(0, &select_rfds, &select_wfds, &select_efds, &timeval) != 0)
        {// connection accepted
            if (dwFlags == (FD_ACCEPT | FD_CONNECT))
            {
                trace("Select ACCEPT&CONNECT...\n");
                ok(FD_ISSET(ClientSocket, &select_wfds), "ClientSocket is not writable\n");
                ok(FD_ISSET(sockaccept, &select_wfds), "sockaccept is not writable\n");
                ok(!FD_ISSET(ServerSocket, &select_rfds), "ServerSocket is readable\n");
            }
            if (dwFlags == FD_ACCEPT)
            {
                trace("Select ACCEPT...\n");
                ok(!FD_ISSET(ClientSocket, &select_wfds), "ClientSocket is writable\n");
                ok(FD_ISSET(sockaccept, &select_wfds), "sockaccept is not writable\n");
                ok(FD_ISSET(ServerSocket, &select_rfds), "ServerSocket is not readable\n");
            }
            if (dwFlags == 0)
            {
                if (FD_ISSET(ServerSocket, &select_rfds))
                {
                    trace("Select ACCEPT...\n");
                    addrsize = sizeof(addr_remote);
                    sockaccept = accept(ServerSocket, (struct sockaddr*)&addr_remote, &addrsize);
                    ok(sockaccept != INVALID_SOCKET, "ERROR: Connection accept function failed, error %d\n", WSAGetLastError());
                    dwFlags |= FD_ACCEPT;
                }
                else
                {
                    if (++ServerSocketLoopCount >= MAX_LOOPCOUNT)
                    {
                        ok(FALSE, "Giving up, on select() (%u/%u), as ServerSocket is not readable yet\n",
                           ServerSocketLoopCount, MAX_LOOPCOUNT);
                        goto done;
                    }

                    trace("Looping, for select() (%u/%u), as ServerSocket is not readable yet\n",
                          ServerSocketLoopCount, MAX_LOOPCOUNT);
                    Sleep(1);
                    continue;
                }
            }
        }
    }

done:
    WSACloseEvent(ServerEvent);
    WSACloseEvent(ClientEvent);
    closesocket(sockaccept);
    closesocket(ServerSocket);
    closesocket(ClientSocket);

    WSACleanup();
}
