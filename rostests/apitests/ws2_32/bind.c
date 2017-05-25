/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for bind
 * PROGRAMMER:      Peter Hater
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <ws2tcpip.h>
#include <ndk/umtypes.h>

CHAR LocalAddress[sizeof("255.255.255.255")];
#define PORT 58888

static
VOID
TestBind(IN_ADDR Address)
{
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
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, Address }, 0, 0, { AF_INET, PORT, Address } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, Address }, 0, 0, { AF_INET, 0, Address } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, Address }, 0, 0, { AF_INET, PORT, Address } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, Address }, 0, 0,{ AF_INET, 0, Address } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
    };
    const INT TestCount = _countof(Tests);
    INT i, AddrSize;
    SOCKET Socket;
    struct sockaddr_in Addr;
#if 0
    BOOL Broadcast = FALSE;
#endif

    for (i = 0; i < TestCount; i++)
    {
        trace("%d: %s %d.%d.%d.%d:%d\n", i, Tests[i].Type == SOCK_STREAM ? "TCP" : "UDP", Tests[i].Addr.sin_addr.S_un.S_un_b.s_b1, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b2, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b3, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b4, Tests[i].ExpectedAddr.sin_port);
        Socket = socket(AF_INET, Tests[i].Type, Tests[i].Proto);
        if (Socket == INVALID_SOCKET)
        {
            skip("Failed to create socket, skipping");
            continue;
        }
        Error = bind(Socket, (const struct sockaddr *) &Tests[i].Addr, sizeof(Tests[i].Addr));
        ok_dec(Error, Tests[i].ExpectedResult);
        if (Error)
        {
            ok_dec(WSAGetLastError(), Tests[i].ExpectedWSAResult);
        }
        else
        {
            ok_dec(WSAGetLastError(), 0);
            Error = getsockname(Socket, (struct sockaddr *) &Addr, &AddrSize);
            ok(Error == 0, "Error getsockname for test %d, Error %d\n", i, Error);
            ok_dec(WSAGetLastError(), 0);
            ok_dec(AddrSize, sizeof(Addr));
            ok_dec(Addr.sin_addr.s_addr, Tests[i].ExpectedAddr.sin_addr.s_addr);
            if (Tests[i].ExpectedAddr.sin_port)
            {
                ok_dec(Addr.sin_port, Tests[i].ExpectedAddr.sin_port);
            }
            else
            {
                ok(Addr.sin_port != 0, "Port remained zero\n");
            }
        }
        Error = closesocket(Socket);
        ok_dec(Error, 0);
    }
#if 0
    /* Check double bind */
    Socket = socket(AF_INET, Tests[0].Type, Tests[0].Proto);
    Error = bind(Socket, (const struct sockaddr *) &Tests[0].Addr, sizeof(Tests[0].Addr));
    ok_dec(Error, Tests[0].ExpectedResult);
    if (Error)
    {
        ok_dec(WSAGetLastError(), Tests[0].ExpectedWSAResult);
    }
    else
    {
        ok_dec(WSAGetLastError(), 0);
        Error = getsockname(Socket, (struct sockaddr *) &Addr, &AddrSize);
        ok_dec(Error, 0);
        ok_dec(AddrSize, sizeof(Addr));
        ok_dec(Addr.sin_addr.s_addr, Tests[0].ExpectedAddr.sin_addr.s_addr);
        if (Tests[0].ExpectedAddr.sin_port)
        {
            ok_dec(Addr.sin_port, Tests[0].ExpectedAddr.sin_port);
        }
        else
        {
            ok(Addr.sin_port != 0, "Port remained zero\n");
        }
        Error = bind(Socket, (const struct sockaddr *) &Tests[2].Addr, sizeof(Tests[2].Addr));
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEINVAL);
    }
    closesocket(Socket);
    /* Check disabled SO_BROADCAST and bind to broadcast address */
    Socket = socket(AF_INET, Tests[10].Type, Tests[10].Proto);
    Error = setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (const char *) &Broadcast, sizeof(Broadcast));
    ok_dec(Error, 0);
    // FIXME: should be made properly broadcast address
    Tests[10].Addr.sin_addr.S_un.S_un_b.s_b4 = 0xff;
    Error = bind(Socket, (const struct sockaddr *) &Tests[10].Addr, sizeof(Tests[10].Addr));
    ok_dec(Error, SOCKET_ERROR);
    ok_dec(WSAGetLastError(), WSAEACCES);
    closesocket(Socket);
#endif
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
    EndSeh(STATUS_ACCESS_VIOLATION);
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
