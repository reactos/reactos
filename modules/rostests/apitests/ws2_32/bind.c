/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for bind
 * PROGRAMMER:      Peter Hater
 */

#include "ws2_32.h"

static CHAR LocalAddress[sizeof("255.255.255.255")];
#define PORT 58888

static
VOID
TestBind(IN_ADDR Address)
{
    const UCHAR b1 = Address.S_un.S_un_b.s_b1;
    const UCHAR b2 = Address.S_un.S_un_b.s_b2;
    const UCHAR b3 = Address.S_un.S_un_b.s_b3;
    const UCHAR b4 = Address.S_un.S_un_b.s_b4;

    int Error;
    struct
    {
        INT Type;
        INT Proto;
        struct sockaddr_in Addr;
        INT ExpectedResult;
        INT ExpectedWSAResult;
        struct sockaddr_in ExpectedAddr;
    } Tests[] =
    {
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ b1, b2, b3, b4 }}} }, 0, 0, { AF_INET, PORT, {{{ b1, b2, b3, b4 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ b1, b2, b3, b4 }}} }, 0, 0, { AF_INET, 0, {{{ b1, b2, b3, b4 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ b1, b2, b3, b4 }}} }, 0, 0, { AF_INET, PORT, {{{ b1, b2, b3, b4 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ b1, b2, b3, b4 }}} }, 0, 0,{ AF_INET, 0, {{{ b1, b2, b3, b4 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
    };
    const INT TestCount = _countof(Tests);
    INT i, AddrSize;
    SOCKET Socket;
    struct sockaddr_in Addr;
    BOOL Broadcast = TRUE;

    for (i = 0; i < TestCount; i++)
    {
        trace("%d: %s %d.%d.%d.%d:%d\n", i, Tests[i].Type == SOCK_STREAM ? "TCP" : "UDP", Tests[i].Addr.sin_addr.S_un.S_un_b.s_b1, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b2, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b3, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b4, Tests[i].ExpectedAddr.sin_port);
        Socket = socket(AF_INET, Tests[i].Type, Tests[i].Proto);
        if (Socket == INVALID_SOCKET)
        {
            skip("Failed to create socket with error %d for test %d, skipping\n", WSAGetLastError(), i);
            continue;
        }
        Error = bind(Socket, (const struct sockaddr *) &Tests[i].Addr, sizeof(Tests[i].Addr));
        ok(Error == Tests[i].ExpectedResult, "Error %d differs from expected %d for test %d\n", Error, Tests[i].ExpectedResult, i);
        if (Error)
        {
            ok(WSAGetLastError() == Tests[i].ExpectedWSAResult, "Error %d differs from expected %d for test %d\n", WSAGetLastError(), Tests[i].ExpectedWSAResult, i);
        }
        else
        {
            AddrSize = sizeof(Addr);
            Error = getsockname(Socket, (struct sockaddr *) &Addr, &AddrSize);
            ok(Error == 0, "Unexpected error %d %d on getsockname for test %d\n", Error, WSAGetLastError(), i);
            ok(AddrSize == sizeof(Addr), "Returned size %d differs from expected %d for test %d\n", AddrSize, sizeof(Addr), i);
            ok(Addr.sin_addr.s_addr == Tests[i].ExpectedAddr.sin_addr.s_addr, "Expected address %lx differs from returned address %lx for test %d\n", Tests[i].ExpectedAddr.sin_addr.s_addr, Addr.sin_addr.s_addr, i);
            if (Tests[i].ExpectedAddr.sin_port)
            {
                ok(Addr.sin_port == Tests[i].ExpectedAddr.sin_port, "Returned port %d differs from expected %d for test %d\n", Addr.sin_port, Tests[i].ExpectedAddr.sin_port, i);
            }
            else
            {
                ok(Addr.sin_port != 0, "Port remained zero for test %d\n", i);
            }
        }
        Error = closesocket(Socket);
        ok(Error == 0, "Unexpected error %d %d on closesocket for test %d\n", Error, WSAGetLastError(), i);
    }
    /* Check double bind */
    Socket = socket(AF_INET, Tests[0].Type, Tests[0].Proto);
    ok(Socket != INVALID_SOCKET, "Failed to create socket with error %d for double bind test, next tests might be wrong\n", WSAGetLastError());
    Error = bind(Socket, (const struct sockaddr *) &Tests[0].Addr, sizeof(Tests[0].Addr));
    ok(Error == Tests[0].ExpectedResult, "Error %d differs from expected %d for double bind test\n", Error, Tests[0].ExpectedResult);
    if (Error)
    {
        ok(WSAGetLastError() == Tests[i].ExpectedWSAResult, "Error %d differs from expected %d for double bind test\n", WSAGetLastError(), Tests[0].ExpectedWSAResult);
    }
    else
    {
        AddrSize = sizeof(Addr);
        Error = getsockname(Socket, (struct sockaddr *) &Addr, &AddrSize);
        ok(Error == 0, "Unexpected error %d %d on getsockname for double bind test\n", Error, WSAGetLastError());
        ok(AddrSize == sizeof(Addr), "Returned size %d differs from expected %d for double bind test\n", AddrSize, sizeof(Addr));
        ok(Addr.sin_addr.s_addr == Tests[0].ExpectedAddr.sin_addr.s_addr, "Expected address %lx differs from returned address %lx for double bind test\n", Tests[0].ExpectedAddr.sin_addr.s_addr, Addr.sin_addr.s_addr);
        if (Tests[0].ExpectedAddr.sin_port)
        {
            ok(Addr.sin_port == Tests[0].ExpectedAddr.sin_port, "Returned port %d differs from expected %d for double bind test\n", Addr.sin_port, Tests[0].ExpectedAddr.sin_port);
        }
        else
        {
            ok(Addr.sin_port != 0, "Port remained zero for double bind test\n");
        }
        Error = bind(Socket, (const struct sockaddr *) &Tests[2].Addr, sizeof(Tests[2].Addr));
        ok(Error == SOCKET_ERROR && WSAGetLastError() == WSAEINVAL, "Unexpected result %d expected %d and wsa result %d expected %ld for double bind test\n", Error, SOCKET_ERROR, WSAGetLastError(), WSAEINVAL);
    }
    Error = closesocket(Socket);
    ok(Error == 0, "Unexpected error %d %d on closesocket for double bind test\n", Error, WSAGetLastError());
    /* Check SO_BROADCAST and bind to broadcast address */
    Socket = socket(AF_INET, Tests[10].Type, Tests[10].Proto);
    ok(Socket != INVALID_SOCKET, "Failed to create socket with error %d for broadcast test, next tests might be wrong\n", WSAGetLastError());
    Error = setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (const char *) &Broadcast, sizeof(Broadcast));
    ok(Error == 0, "Unexpected error %d %d on setsockopt for broadcast test\n", Error, WSAGetLastError());
    Error = bind(Socket, (const struct sockaddr *) &Tests[10].Addr, sizeof(Tests[10].Addr));
    ok(Error == 0, "Unexpected error %d %d on bind for broadcast test\n", Error, WSAGetLastError());
    Error = closesocket(Socket);
    ok(Error == 0, "Unexpected error %d %d on closesocket for broadcast test\n", Error, WSAGetLastError());
}

START_TEST(bind)
{
    WSADATA WsaData;
    int Error;
    CHAR LocalHostName[128];
    struct hostent *Hostent;
    IN_ADDR Address;
    SOCKET Socket;
    struct sockaddr_in Addr = { AF_INET };

    /* not yet initialized */
    StartSeh()
        Error = bind(INVALID_SOCKET, NULL, 0);
        ok_dec(Error, -1);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Error = bind(INVALID_SOCKET, InvalidPointer, 0);
        ok_dec(Error, -1);
    EndSeh(STATUS_SUCCESS);

    Error = WSAStartup(MAKEWORD(2, 2), &WsaData);
    ok_dec(Error, 0);

    /* initialize LocalAddress for tests */
    Error = gethostname(LocalHostName, sizeof(LocalHostName));
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);
    trace("Local host name is '%s'\n", LocalHostName);
    Hostent = gethostbyname(LocalHostName);
    ok(Hostent != NULL, "gethostbyname failed with %d\n", WSAGetLastError());
    if (Hostent && Hostent->h_addr_list[0] && Hostent->h_length == sizeof(IN_ADDR))
    {
        memcpy(&Address, Hostent->h_addr_list[0], sizeof(Address));
        strcpy(LocalAddress, inet_ntoa(Address));
    }
    trace("Local address is '%s'\n", LocalAddress);
    ok(LocalAddress[0] != '\0',
       "Could not determine local address. Following test results may be wrong.\n");

    /* parameter tests */
    StartSeh()
        Error = bind(INVALID_SOCKET, NULL, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAENOTSOCK);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Error = bind(INVALID_SOCKET, InvalidPointer, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAENOTSOCK);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Error = bind(Socket, NULL, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Error = bind(Socket, InvalidPointer, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Error = bind(Socket, NULL, sizeof(Addr));
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Error = bind(Socket, InvalidPointer, sizeof(Addr));
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Error = bind(Socket, (const struct sockaddr *) &Addr, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Error = bind(Socket, (const struct sockaddr *) &Addr, sizeof(Addr)-1);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);

    TestBind(Address);
    /* TODO: test IPv6 */

    Error = WSACleanup();
    ok_dec(Error, 0);
}
