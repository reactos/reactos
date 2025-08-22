/*
 * Unit test suite for protocol functions
 *
 * Copyright 2004 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#include <mswsock.h>
#include <iphlpapi.h>

#include "wine/test.h"

static void (WINAPI *pFreeAddrInfoExW)(ADDRINFOEXW *ai);
static int (WINAPI *pGetAddrInfoExW)(const WCHAR *name, const WCHAR *servname, DWORD namespace,
        GUID *namespace_id, const ADDRINFOEXW *hints, ADDRINFOEXW **result,
        struct timeval *timeout, OVERLAPPED *overlapped,
        LPLOOKUPSERVICE_COMPLETION_ROUTINE completion_routine, HANDLE *handle);
static int   (WINAPI *pGetAddrInfoExOverlappedResult)(OVERLAPPED *overlapped);
static int (WINAPI *pGetHostNameW)(WCHAR *name, int len);
static const char *(WINAPI *p_inet_ntop)(int family, void *addr, char *string, ULONG size);
static const WCHAR *(WINAPI *pInetNtopW)(int family, void *addr, WCHAR *string, ULONG size);
static int (WINAPI *p_inet_pton)(int family, const char *string, void *addr);
static int (WINAPI *pInetPtonW)(int family, WCHAR *string, void *addr);
static int (WINAPI *pWSCGetApplicationCategory)(LPCWSTR path, DWORD path_len, LPCWSTR extra, DWORD extra_len, DWORD *category, INT *err);
static int (WINAPI *pWSCGetProviderInfo)(GUID *provider, WSC_PROVIDER_INFO_TYPE type, BYTE *info, size_t *size, DWORD flags, INT *err);

/* TCP and UDP over IP fixed set of service flags */
#define TCPIP_SERVICE_FLAGS (XP1_GUARANTEED_DELIVERY \
                           | XP1_GUARANTEED_ORDER    \
                           | XP1_GRACEFUL_CLOSE      \
                           | XP1_EXPEDITED_DATA      \
                           | XP1_IFS_HANDLES)

#define UDPIP_SERVICE_FLAGS (XP1_CONNECTIONLESS      \
                           | XP1_MESSAGE_ORIENTED    \
                           | XP1_SUPPORT_BROADCAST   \
                           | XP1_SUPPORT_MULTIPOINT  \
                           | XP1_IFS_HANDLES)

static void test_service_flags(int family, int version, int socktype, int protocol, DWORD testflags)
{
    DWORD expectedflags = 0;
    if (socktype == SOCK_STREAM && protocol == IPPROTO_TCP)
        expectedflags = TCPIP_SERVICE_FLAGS;
    if (socktype == SOCK_DGRAM && protocol == IPPROTO_UDP)
        expectedflags = UDPIP_SERVICE_FLAGS;

    /* check if standard TCP and UDP protocols are offering the correct service flags */
    if ((family == AF_INET || family == AF_INET6) && version == 2 && expectedflags)
    {
        /* QOS may or may not be installed */
        testflags &= ~XP1_QOS_SUPPORTED;
        ok(expectedflags == testflags,
           "Incorrect flags, expected 0x%lx, received 0x%lx\n",
           expectedflags, testflags);
    }
}

static void test_WSAEnumProtocolsA(void)
{
    INT ret, i, j, found;
    DWORD len = 0, error;
    WSAPROTOCOL_INFOA info, *buffer;
    INT ptest[] = {0xdead, IPPROTO_TCP, 0xcafe, IPPROTO_UDP, 0xbeef, 0};

    ret = WSAEnumProtocolsA( NULL, NULL, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsA() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS, "Expected 10055, received %ld\n", error);

    len = 0;

    ret = WSAEnumProtocolsA( NULL, &info, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsA() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS, "Expected 10055, received %ld\n", error);

    buffer = malloc( len );

    if (buffer)
    {
        ret = WSAEnumProtocolsA( NULL, buffer, &len );
        ok( ret != SOCKET_ERROR, "WSAEnumProtocolsA() failed unexpectedly: %d\n",
            WSAGetLastError() );

        for (i = 0; i < ret; i++)
        {
            ok( strlen( buffer[i].szProtocol ), "No protocol name found\n" );
            ok( !(buffer[i].dwProviderFlags & PFL_HIDDEN), "Found a protocol with PFL_HIDDEN.\n" );
            test_service_flags( buffer[i].iAddressFamily, buffer[i].iVersion,
                                buffer[i].iSocketType, buffer[i].iProtocol,
                                buffer[i].dwServiceFlags1);
        }

        free( buffer );
    }

    /* Test invalid protocols in the list */
    ret = WSAEnumProtocolsA( ptest, NULL, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsA() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS || broken(error == WSAEFAULT) /* NT4 */,
       "Expected 10055, received %ld\n", error);

    buffer = malloc( len );

    if (buffer)
    {
        ret = WSAEnumProtocolsA( ptest, buffer, &len );
        ok( ret != SOCKET_ERROR, "WSAEnumProtocolsA() failed unexpectedly: %d\n",
            WSAGetLastError() );
        ok( ret >= 2, "Expected at least 2 items, received %d\n", ret);

        for (i = found = 0; i < ret; i++)
            for (j = 0; j < ARRAY_SIZE(ptest); j++)
                if (buffer[i].iProtocol == ptest[j])
                {
                    found |= 1 << j;
                    break;
                }
        ok(found == 0x0A, "Expected 2 bits represented as 0xA, received 0x%x\n", found);

        free( buffer );
    }
}

static void test_WSAEnumProtocolsW(void)
{
    INT ret, i, j, found;
    DWORD len = 0, error;
    WSAPROTOCOL_INFOW info, *buffer;
    INT ptest[] = {0xdead, IPPROTO_TCP, 0xcafe, IPPROTO_UDP, 0xbeef, 0};

    ret = WSAEnumProtocolsW( NULL, NULL, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsW() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS, "Expected 10055, received %ld\n", error);

    len = 0;

    ret = WSAEnumProtocolsW( NULL, &info, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsW() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS, "Expected 10055, received %ld\n", error);

    buffer = malloc( len );

    if (buffer)
    {
        ret = WSAEnumProtocolsW( NULL, buffer, &len );
        ok( ret != SOCKET_ERROR, "WSAEnumProtocolsW() failed unexpectedly: %d\n",
            WSAGetLastError() );

        for (i = 0; i < ret; i++)
        {
            ok( lstrlenW( buffer[i].szProtocol ), "No protocol name found\n" );
            ok( !(buffer[i].dwProviderFlags & PFL_HIDDEN), "Found a protocol with PFL_HIDDEN.\n" );
            test_service_flags( buffer[i].iAddressFamily, buffer[i].iVersion,
                                buffer[i].iSocketType, buffer[i].iProtocol,
                                buffer[i].dwServiceFlags1);
        }

        free( buffer );
    }

    /* Test invalid protocols in the list */
    ret = WSAEnumProtocolsW( ptest, NULL, &len );
    ok( ret == SOCKET_ERROR, "WSAEnumProtocolsW() succeeded unexpectedly\n");
    error = WSAGetLastError();
    ok( error == WSAENOBUFS || broken(error == WSAEFAULT) /* NT4 */,
       "Expected 10055, received %ld\n", error);

    buffer = malloc( len );

    if (buffer)
    {
        ret = WSAEnumProtocolsW( ptest, buffer, &len );
        ok( ret != SOCKET_ERROR, "WSAEnumProtocolsW() failed unexpectedly: %d\n",
            WSAGetLastError() );
        ok( ret >= 2, "Expected at least 2 items, received %d\n", ret);

        for (i = found = 0; i < ret; i++)
            for (j = 0; j < ARRAY_SIZE(ptest); j++)
                if (buffer[i].iProtocol == ptest[j])
                {
                    found |= 1 << j;
                    break;
                }
        ok(found == 0x0A, "Expected 2 bits represented as 0xA, received 0x%x\n", found);

        free( buffer );
    }
}

struct protocol
{
    int prot;
    const char *names[2];
    BOOL missing_from_xp;
};

static const struct protocol protocols[] =
{
    {   0, { "ip", "IP" }},
    {   1, { "icmp", "ICMP" }},
    {   3, { "ggp", "GGP" }},
    {   6, { "tcp", "TCP" }},
    {   8, { "egp", "EGP" }},
    {  12, { "pup", "PUP" }},
    {  17, { "udp", "UDP" }},
    {  20, { "hmp", "HMP" }},
    {  22, { "xns-idp", "XNS-IDP" }},
    {  27, { "rdp", "RDP" }},
    {  41, { "ipv6", "IPv6" }, TRUE},
    {  43, { "ipv6-route", "IPv6-Route" }, TRUE},
    {  44, { "ipv6-frag", "IPv6-Frag" }, TRUE},
    {  50, { "esp", "ESP" }, TRUE},
    {  51, { "ah", "AH" }, TRUE},
    {  58, { "ipv6-icmp", "IPv6-ICMP" }, TRUE},
    {  59, { "ipv6-nonxt", "IPv6-NoNxt" }, TRUE},
    {  60, { "ipv6-opts", "IPv6-Opts" }, TRUE},
    {  66, { "rvd", "RVD" }},
};

static const struct protocol *find_protocol(int number)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(protocols); i++)
    {
        if (protocols[i].prot == number)
            return &protocols[i];
    }
    return NULL;
}

static void test_getprotobyname(void)
{
    struct protoent *ent;
    char all_caps_name[16];
    int i, j;

    for (i = 0; i < ARRAY_SIZE(protocols); i++)
    {
        for (j = 0; j < ARRAY_SIZE(protocols[0].names); j++)
        {
            ent = getprotobyname(protocols[i].names[j]);
            ok((ent && ent->p_proto == protocols[i].prot) || broken(!ent && protocols[i].missing_from_xp),
               "Expected %s to be protocol number %d, got %d\n",
               wine_dbgstr_a(protocols[i].names[j]), protocols[i].prot, ent ? ent->p_proto : -1);
        }

        for (j = 0; protocols[i].names[0][j]; j++)
            all_caps_name[j] = toupper(protocols[i].names[0][j]);
        all_caps_name[j] = 0;
        ent = getprotobyname(all_caps_name);
        ok((ent && ent->p_proto == protocols[i].prot) || broken(!ent && protocols[i].missing_from_xp),
           "Expected %s to be protocol number %d, got %d\n",
           wine_dbgstr_a(all_caps_name), protocols[i].prot, ent ? ent->p_proto : -1);
    }
}

static void test_getprotobynumber(void)
{
    struct protoent *ent;
    const struct protocol *ref;
    int i;

    for (i = -1; i <= 256; i++)
    {
        ent = getprotobynumber(i);
        ref = find_protocol(i);

        if (!ref)
        {
            ok(!ent, "Expected protocol number %d to be undefined, got %s\n",
               i, wine_dbgstr_a(ent ? ent->p_name : NULL));
            continue;
        }

        ok((ent && ent->p_name && !strcmp(ent->p_name, ref->names[0])) ||
           broken(!ent && ref->missing_from_xp),
           "Expected protocol number %d to be %s, got %s\n",
           i, ref->names[0], wine_dbgstr_a(ent ? ent->p_name : NULL));

        ok((ent && ent->p_aliases && ent->p_aliases[0] &&
            !strcmp(ent->p_aliases[0], ref->names[1])) ||
           broken(!ent && ref->missing_from_xp),
           "Expected protocol number %d alias 0 to be %s, got %s\n",
           i, ref->names[0], wine_dbgstr_a(ent && ent->p_aliases ? ent->p_aliases[0] : NULL));
    }
}

#define NUM_THREADS 3      /* Number of threads to run getservbyname */
#define NUM_QUERIES 250    /* Number of getservbyname queries per thread */

static DWORD WINAPI do_getservbyname( void *param )
{
    struct
    {
        const char *name;
        const char *proto;
        int port;
    } serv[2] =
    {
        {"domain", "udp", 53},
        {"telnet", "tcp", 23},
    };

    HANDLE *starttest = param;
    int i, j;
    struct servent *pserv[2];

    ok( WaitForSingleObject( *starttest, 30 * 1000 ) != WAIT_TIMEOUT,
         "test_getservbyname: timeout waiting for start signal\n" );

    /* ensure that necessary buffer resizes are completed */
    for (j = 0; j < 2; j++)
        pserv[j] = getservbyname( serv[j].name, serv[j].proto );

    for (i = 0; i < NUM_QUERIES / 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            pserv[j] = getservbyname( serv[j].name, serv[j].proto );
            ok( pserv[j] != NULL || broken(pserv[j] == NULL) /* win8, fixed in win81 */,
                 "getservbyname could not retrieve information for %s: %d\n", serv[j].name, WSAGetLastError() );
            if ( !pserv[j] ) continue;
            ok( pserv[j]->s_port == htons(serv[j].port),
                 "getservbyname returned the wrong port for %s: %d\n", serv[j].name, ntohs(pserv[j]->s_port) );
            ok( !strcmp( pserv[j]->s_proto, serv[j].proto ),
                 "getservbyname returned the wrong protocol for %s: %s\n", serv[j].name, pserv[j]->s_proto );
            ok( !strcmp( pserv[j]->s_name, serv[j].name ),
                 "getservbyname returned the wrong name for %s: %s\n", serv[j].name, pserv[j]->s_name );
        }

        ok( pserv[0] == pserv[1] || broken(pserv[0] != pserv[1]) /* win8, fixed in win81 */,
             "getservbyname: winsock resized servent buffer when not necessary\n" );
    }

    return 0;
}

static void test_getservbyname(void)
{
    int i;
    HANDLE starttest, thread[NUM_THREADS];

    starttest = CreateEventA( NULL, 1, 0, "test_getservbyname_starttest" );

    /* create threads */
    for (i = 0; i < NUM_THREADS; i++)
        thread[i] = CreateThread( NULL, 0, do_getservbyname, &starttest, 0, NULL );

    /* signal threads to start */
    SetEvent( starttest );

    for (i = 0; i < NUM_THREADS; i++)
        WaitForSingleObject( thread[i], 30 * 1000 );
}

static void test_WSALookupService(void)
{
    char buffer[4096], strbuff[128];
    WSAQUERYSETW *qs = NULL;
    HANDLE handle;
    PNLA_BLOB netdata;
    int ret;
    DWORD error, offset, size;

    qs = (WSAQUERYSETW *)buffer;
    memset(qs, 0, sizeof(*qs));

    /* invalid parameter tests */
    ret = WSALookupServiceBeginW(NULL, 0, &handle);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "WSALookupServiceBeginW should have failed\n");
    todo_wine
    ok(error == WSAEFAULT, "expected 10014, got %ld\n", error);

    ret = WSALookupServiceBeginW(qs, 0, NULL);
    error = WSAGetLastError();
    ok(ret == SOCKET_ERROR, "WSALookupServiceBeginW should have failed\n");
    todo_wine
    ok(error == WSAEFAULT, "expected 10014, got %ld\n", error);

    ret = WSALookupServiceBeginW(qs, 0, &handle);
    ok(ret == SOCKET_ERROR, "WSALookupServiceBeginW should have failed\n");
    todo_wine ok(WSAGetLastError() == WSAEINVAL
            || broken(WSAGetLastError() == ERROR_INVALID_PARAMETER)
            || broken(WSAGetLastError() == WSASERVICE_NOT_FOUND) /* win10 1809 */,
            "got error %u\n", WSAGetLastError());

    ret = WSALookupServiceEnd(NULL);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "WSALookupServiceEnd should have failed\n");
    todo_wine
    ok(error == ERROR_INVALID_HANDLE, "expected 6, got %ld\n", error);

    /* standard network list query */
    qs->dwSize = sizeof(*qs);
    handle = (HANDLE)0xdeadbeef;
    ret = WSALookupServiceBeginW(qs, LUP_RETURN_ALL | LUP_DEEP, &handle);
    error = WSAGetLastError();
    if (ret && error == ERROR_INVALID_PARAMETER)
    {
        win_skip("the current WSALookupServiceBeginW test is not supported in win <= 2000\n");
        return;
    }

    todo_wine
    ok(!ret, "WSALookupServiceBeginW failed unexpectedly with error %ld\n", error);
    todo_wine
    ok(handle != (HANDLE)0xdeadbeef, "Handle was not filled\n");

    offset = 0;
    do
    {
        memset(qs, 0, sizeof(*qs));
        size = sizeof(buffer);

        if (WSALookupServiceNextW(handle, 0, &size, qs) == SOCKET_ERROR)
        {
            ok(WSAGetLastError() == WSA_E_NO_MORE, "got error %u\n", WSAGetLastError());
            break;
        }

        if (winetest_debug <= 1) continue;

        WideCharToMultiByte(CP_ACP, 0, qs->lpszServiceInstanceName, -1,
                            strbuff, sizeof(strbuff), NULL, NULL);
        trace("Network Name: %s\n", strbuff);

        /* network data is written in the blob field */
        if (qs->lpBlob)
        {
            /* each network may have multiple NLA_BLOB information structures */
            do
            {
                netdata = (PNLA_BLOB) &qs->lpBlob->pBlobData[offset];
                switch (netdata->header.type)
                {
                    case NLA_RAW_DATA:
                        trace("\tNLA Data Type: NLA_RAW_DATA\n");
                        break;
                    case NLA_INTERFACE:
                        trace("\tNLA Data Type: NLA_INTERFACE\n");
                        trace("\t\tType: %ld\n", netdata->data.interfaceData.dwType);
                        trace("\t\tSpeed: %ld\n", netdata->data.interfaceData.dwSpeed);
                        trace("\t\tAdapter Name: %s\n", netdata->data.interfaceData.adapterName);
                        break;
                    case NLA_802_1X_LOCATION:
                        trace("\tNLA Data Type: NLA_802_1X_LOCATION\n");
                        trace("\t\tInformation: %s\n", netdata->data.locationData.information);
                        break;
                    case NLA_CONNECTIVITY:
                        switch (netdata->data.connectivity.type)
                        {
                            case NLA_NETWORK_AD_HOC:
                                trace("\t\tNetwork Type: AD HOC\n");
                                break;
                            case NLA_NETWORK_MANAGED:
                                trace("\t\tNetwork Type: Managed\n");
                                break;
                            case NLA_NETWORK_UNMANAGED:
                                trace("\t\tNetwork Type: Unmanaged\n");
                                break;
                            case NLA_NETWORK_UNKNOWN:
                                trace("\t\tNetwork Type: Unknown\n");
                        }
                        switch (netdata->data.connectivity.internet)
                        {
                            case NLA_INTERNET_NO:
                                trace("\t\tInternet connectivity: No\n");
                                break;
                            case NLA_INTERNET_YES:
                                trace("\t\tInternet connectivity: Yes\n");
                                break;
                            case NLA_INTERNET_UNKNOWN:
                                trace("\t\tInternet connectivity: Unknown\n");
                                break;
                        }
                        break;
                    case NLA_ICS:
                        trace("\tNLA Data Type: NLA_ICS\n");
                        trace("\t\tSpeed: %ld\n",
                               netdata->data.ICS.remote.speed);
                        trace("\t\tType: %ld\n",
                               netdata->data.ICS.remote.type);
                        trace("\t\tState: %ld\n",
                               netdata->data.ICS.remote.state);
                        WideCharToMultiByte(CP_ACP, 0, netdata->data.ICS.remote.machineName, -1,
                            strbuff, sizeof(strbuff), NULL, NULL);
                        trace("\t\tMachine Name: %s\n", strbuff);
                        WideCharToMultiByte(CP_ACP, 0, netdata->data.ICS.remote.sharedAdapterName, -1,
                            strbuff, sizeof(strbuff), NULL, NULL);
                        trace("\t\tShared Adapter Name: %s\n", strbuff);
                        break;
                    default:
                        trace("\tNLA Data Type: Unknown\n");
                        break;
                }
            }
            while (offset);
        }
    }
    while (1);

    ret = WSALookupServiceEnd(handle);
    ok(!ret, "WSALookupServiceEnd failed unexpectedly\n");
}

#define WM_ASYNCCOMPLETE (WM_USER + 100)
static HWND create_async_message_window(void)
{
    static const char class_name[] = "ws2_32 async message window class";

    WNDCLASSEXA wndclass;
    HWND hWnd;

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = DefWindowProcA;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = GetModuleHandleA(NULL);
    wndclass.hIcon          = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wndclass.hIconSm        = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wndclass.hCursor        = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszClassName  = class_name;
    wndclass.lpszMenuName   = NULL;

    RegisterClassExA(&wndclass);

    hWnd = CreateWindowA(class_name, "ws2_32 async message window", WS_OVERLAPPEDWINDOW,
                        0, 0, 500, 500, NULL, NULL, GetModuleHandleA(NULL), NULL);
    ok(!!hWnd, "failed to create window\n");

    return hWnd;
}

static void wait_for_async_message(HWND hwnd, HANDLE handle)
{
    BOOL ret;
    MSG msg;

    while ((ret = GetMessageA(&msg, 0, 0, 0)) &&
           !(msg.hwnd == hwnd && msg.message == WM_ASYNCCOMPLETE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ok(ret, "did not expect WM_QUIT message\n");
    ok(msg.wParam == (WPARAM)handle, "expected wParam = %p, got %Ix\n", handle, msg.wParam);
}

static void test_WSAAsyncGetServByPort(void)
{
    HWND hwnd = create_async_message_window();
    HANDLE ret;
    char buffer[MAXGETHOSTSTRUCT];

    /* FIXME: The asynchronous window messages should be tested. */

    /* Parameters are not checked when initiating the asynchronous operation.  */
    ret = WSAAsyncGetServByPort(NULL, 0, 0, NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByPort returned NULL\n");

    ret = WSAAsyncGetServByPort(hwnd, WM_ASYNCCOMPLETE, 0, NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByPort returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByPort(hwnd, WM_ASYNCCOMPLETE, htons(80), NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByPort returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByPort(hwnd, WM_ASYNCCOMPLETE, htons(80), NULL, buffer, MAXGETHOSTSTRUCT);
    ok(ret != NULL, "WSAAsyncGetServByPort returned NULL\n");
    wait_for_async_message(hwnd, ret);

    DestroyWindow(hwnd);
}

static void test_WSAAsyncGetServByName(void)
{
    HWND hwnd = create_async_message_window();
    HANDLE ret;
    char buffer[MAXGETHOSTSTRUCT];

    /* FIXME: The asynchronous window messages should be tested. */

    /* Parameters are not checked when initiating the asynchronous operation.  */
    ret = WSAAsyncGetServByName(hwnd, WM_ASYNCCOMPLETE, "", NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByName returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByName(hwnd, WM_ASYNCCOMPLETE, "", "", buffer, MAXGETHOSTSTRUCT);
    ok(ret != NULL, "WSAAsyncGetServByName returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByName(hwnd, WM_ASYNCCOMPLETE, "http", NULL, NULL, 0);
    ok(ret != NULL, "WSAAsyncGetServByName returned NULL\n");
    wait_for_async_message(hwnd, ret);

    ret = WSAAsyncGetServByName(hwnd, WM_ASYNCCOMPLETE, "http", "tcp", buffer, MAXGETHOSTSTRUCT);
    ok(ret != NULL, "WSAAsyncGetServByName returned NULL\n");
    wait_for_async_message(hwnd, ret);

    DestroyWindow(hwnd);
}

static DWORD WINAPI inet_ntoa_thread_proc(void *param)
{
    ULONG addr;
    const char *str;
    HANDLE *event = param;

    addr = inet_addr("4.3.2.1");
    ok(addr == htonl(0x04030201), "expected 0x04030201, got %08lx\n", addr);
    str = inet_ntoa(*(struct in_addr *)&addr);
    ok(!strcmp(str, "4.3.2.1"), "expected 4.3.2.1, got %s\n", str);

    SetEvent(event[0]);
    WaitForSingleObject(event[1], 3000);

    return 0;
}

static void test_inet_ntoa(void)
{
    ULONG addr;
    const char *str;
    HANDLE thread, event[2];
    DWORD tid;

    addr = inet_addr("1.2.3.4");
    ok(addr == htonl(0x01020304), "expected 0x01020304, got %08lx\n", addr);
    str = inet_ntoa(*(struct in_addr *)&addr);
    ok(!strcmp(str, "1.2.3.4"), "expected 1.2.3.4, got %s\n", str);

    event[0] = CreateEventW(NULL, TRUE, FALSE, NULL);
    event[1] = CreateEventW(NULL, TRUE, FALSE, NULL);

    thread = CreateThread(NULL, 0, inet_ntoa_thread_proc, event, 0, &tid);
    WaitForSingleObject(event[0], 3000);

    ok(!strcmp(str, "1.2.3.4"), "expected 1.2.3.4, got %s\n", str);

    SetEvent(event[1]);
    WaitForSingleObject(thread, 3000);

    CloseHandle(event[0]);
    CloseHandle(event[1]);
    CloseHandle(thread);
}

static void test_inet_addr(void)
{
    static const struct
    {
        const char *input;
        u_long addr;
    }
    tests[] =
    {
        {"1.2.3.4",                0x04030201},
        {"1 2 3 4",                0x01000000},
        {"1.2.3. 4",               0xffffffff},
        {"1.2.3 .4",               0x03000201},
        {"1.2.3 \xfe\xff",         0x03000201},
        {"3.4.5.6.7",              0xffffffff},
        {"3.4.5.6. 7",             0xffffffff},
        {"3.4.5.6  7",             0x06050403},
        {" 3.4.5.6",               0xffffffff},
        {"\t3.4.5.6",              0xffffffff},
        {"3.4.5.6 ",               0x06050403},
        {"3.4.5.6  ",              0x06050403},
        {"3. 4.5.6",               0xffffffff},
        {"3 .4.5.6",               0x03000000},
        {"1.2.3",                  0x03000201},
        {".1.2.3",                 0xffffffff},
        {"0.0.0.0",                0x00000000},
        {"",                       0xffffffff},
        {" 0",                     0xffffffff},
        {"0xa1a2b3b4 ",            0xb4b3a2a1},
        {".",                      0xffffffff},
        {" ",                      0x00000000},
        {"\t",                     0xffffffff},
        {"  ",                     0xffffffff},
        {"127.127.127.255",        0xff7f7f7f},
        {"127.127.127.255:123",    0xffffffff},
        {"127.127.127.256",        0xffffffff},
        {"a",                      0xffffffff},
        {"1.2.3.0xaA",             0xaa030201},
        {"1.1.1.0x",               0xffffffff},
        {"1.2.3.010",              0x08030201},
        {"1.2.3.00",               0x00030201},
        {"1.2.3.0a",               0xffffffff},
        {"1.1.1.0o10",             0xffffffff},
        {"1.1.1.0b10",             0xffffffff},
        {"1.1.1.-2",               0xffffffff},
        {"1",                      0x01000000},
        {"1.2",                    0x02000001},
        {"1.2.3",                  0x03000201},
        {"203569230",              0x4e38220c},
        {"[0.1.2.3]",              0xffffffff},
        {"0x00010203",             0x03020100},
        {"0x2134",                 0x34210000},
        {"1234BEEF",               0xffffffff},
        {"017700000001",           0x0100007f},
        {"0777",                   0xff010000},
        {"2607:f0d0:1002:51::4",   0xffffffff},
        {"::177.32.45.20",         0xffffffff},
        {"::1/128",                0xffffffff},
        {"::1",                    0xffffffff},
        {":1",                     0xffffffff},
    };
    u_long addr, expected;
    unsigned int i;
    char str[32];

    WSASetLastError(0xdeadbeef);
    addr = inet_addr(NULL);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());
    ok(addr == 0xffffffff, "got addr %#08lx\n", addr);
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context( "Address %s, i %u", debugstr_a(tests[i].input), i );
        WSASetLastError(0xdeadbeef);
        addr = inet_addr(tests[i].input);
        ok(WSAGetLastError() == 0xdeadbeef, "got error %u\n", WSAGetLastError());
        ok(addr == tests[i].addr, "got addr %#08lx\n", addr);
        winetest_pop_context();
    }

    strcpy(str, "1.2.3");
    str[6] = 0;
    for (i = 1; i < 256; ++i)
    {
        if (isdigit(i))
            continue;
        str[5] = i;
        expected = isspace(i) ? 0x03000201 : 0xffffffff;
        addr = inet_addr(str);
        ok(addr == expected, "got addr %#08lx, expected %#08lx, i %u\n", addr, expected, i);
    }
}

static void test_inet_pton(void)
{
    static const struct
    {
        char input[32];
        int ret;
        DWORD addr;
    }
    ipv4_tests[] =
    {
        {"",                       0, 0xdeadbeef},
        {" ",                      0, 0xdeadbeef},
        {"1.1.1.1",                1, 0x01010101},
        {"0.0.0.0",                1, 0x00000000},
        {"127.127.127.255",        1, 0xff7f7f7f},
        {"127.127.127.255:123",    0, 0xff7f7f7f},
        {"127.127.127.256",        0, 0xdeadbeef},
        {"a",                      0, 0xdeadbeef},
        {"1.1.1.0xaA",             0, 0xdeadbeef},
        {"1.1.1.0x",               0, 0xdeadbeef},
        {"1.1.1.010",              0, 0xdeadbeef},
        {"1.1.1.00",               0, 0xdeadbeef},
        {"1.1.1.0a",               0, 0x00010101},
        {"1.1.1.0o10",             0, 0x00010101},
        {"1.1.1.0b10",             0, 0x00010101},
        {"1.1.1.-2",               0, 0xdeadbeef},
        {"1",                      0, 0xdeadbeef},
        {"1.2",                    0, 0xdeadbeef},
        {"1.2.3",                  0, 0xdeadbeef},
        {"203569230",              0, 0xdeadbeef},
        {"3.4.5.6.7",              0, 0xdeadbeef},
        {" 3.4.5.6",               0, 0xdeadbeef},
        {"\t3.4.5.6",              0, 0xdeadbeef},
        {"3.4.5.6 ",               0, 0x06050403},
        {"3. 4.5.6",               0, 0xdeadbeef},
        {"[0.1.2.3]",              0, 0xdeadbeef},
        {"0x00010203",             0, 0xdeadbeef},
        {"0x2134",                 0, 0xdeadbeef},
        {"1234BEEF",               0, 0xdeadbeef},
        {"017700000001",           0, 0xdeadbeef},
        {"0777",                   0, 0xdeadbeef},
        {"2607:f0d0:1002:51::4",   0, 0xdeadbeef},
        {"::177.32.45.20",         0, 0xdeadbeef},
        {"::1/128",                0, 0xdeadbeef},
        {"::1",                    0, 0xdeadbeef},
        {":1",                     0, 0xdeadbeef},
    };

    static const struct
    {
        char input[64];
        int ret;
        unsigned short addr[8];
        int broken;
        int broken_ret;
    }
    ipv6_tests[] =
    {
        {"0000:0000:0000:0000:0000:0000:0000:0000",        1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0000:0000:0000:0000:0000:0000:0000:0001",        1, {0, 0, 0, 0, 0, 0, 0, 0x100}},
        {"0:0:0:0:0:0:0:0",                                1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:0:0:0:0:0:0:1",                                1, {0, 0, 0, 0, 0, 0, 0, 0x100}},
        {"0:0:0:0:0:0:0::",                                1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:0:0:0:0:0:13.1.68.3",                          1, {0, 0, 0, 0, 0, 0, 0x10d, 0x344}},
        {"0:0:0:0:0:0::",                                  1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:0:0:0:0::",                                    1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:0:0:0:0:FFFF:129.144.52.38",                   1, {0, 0, 0, 0, 0, 0xffff, 0x9081, 0x2634}},
        {"0::",                                            1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"0:1:2:3:4:5:6:7",                                1, {0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700}},
        {"1080:0:0:0:8:800:200c:417a",                     1, {0x8010, 0, 0, 0, 0x800, 0x8, 0x0c20, 0x7a41}},
        {"0:a:b:c:d:e:f::",                                1, {0, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00, 0xf00, 0}},
        {"1111:2222:3333:4444:5555:6666:123.123.123.123",  1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111:2222:3333:4444:5555:6666:7777:8888",        1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111:2222:3333:4444:0x5555:6666:7777:8888",      0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:x555:6666:7777:8888",        0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:0r5555:6666:7777:8888",      0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:r5555:6666:7777:8888",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555:6666:7777::",           1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0}},
        {"1111:2222:3333:4444:5555:6666::",                1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0}},
        {"1111:2222:3333:4444:5555:6666::8888",            1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x8888}},
        {"1111:2222:3333:4444:5555:6666::7777:8888",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x7777}},
        {"1111:2222:3333:4444:5555:6666:7777::8888",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0}},
        {"1111:2222:3333:4444:5555::",                     1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0}},
        {"1111:2222:3333:4444:5555::123.123.123.123",      1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0x7b7b, 0x7b7b}},
        {"1111:2222:3333:4444:5555::0x1.123.123.123",      0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x100}},
        {"1111:2222:3333:4444:5555::0x88",                 0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800}},
        {"1111:2222:3333:4444:5555::0X88",                 0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800}},
        {"1111:2222:3333:4444:5555::0X",                   0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0}},
        {"1111:2222:3333:4444:5555::0X88:7777",            0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8800}},
        {"1111:2222:3333:4444:5555::0x8888",               0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8888}},
        {"1111:2222:3333:4444:5555::0x80000000",           0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0xffff}},
        {"1111:2222:3333:4444::5555:0x012345678",          0, {0x1111, 0x2222, 0x3333, 0x4444, 0, 0, 0x5555, 0x7856}},
        {"1111:2222:3333:4444::5555:0x123456789",          0, {0x1111, 0x2222, 0x3333, 0x4444, 0, 0, 0x5555, 0xffff}},
        {"1111:2222:3333:4444:5555:6666:0x12345678",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555:6666:7777:0x80000000",  0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xffff}},
        {"1111:2222:3333:4444:5555:6666:7777:0x012345678", 0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x7856}},
        {"1111:2222:3333:4444:5555:6666:7777:0x123456789", 0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xffff}},
        {"111:222:333:444:555:666:777:0x123456789abcdef0", 0, {0x1101, 0x2202, 0x3303, 0x4404, 0x5505, 0x6606, 0x7707, 0xffff}},
        {"1111:2222:3333:4444:5555::08888",                0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555::08888::",              0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555:6666:7777:fffff:",      0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0xabab}},
        {"1111:2222:3333:4444:5555:6666::fffff:",          0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555::fffff",                0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444::fffff",                     0, {0x1111, 0x2222, 0x3333, 0x4444, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333::fffff",                          0, {0x1111, 0x2222, 0x3333, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555::7777:8888",            1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0x7777, 0x8888}},
        {"1111:2222:3333:4444:5555::8888",                 1, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0, 0, 0x8888}},
        {"1111::",                                         1, {0x1111, 0, 0, 0, 0, 0, 0, 0}},
        {"1111::123.123.123.123",                          1, {0x1111, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b}},
        {"1111::3333:4444:5555:6666:123.123.123.123",      1, {0x1111, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111::3333:4444:5555:6666:7777:8888",            1, {0x1111, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111::4444:5555:6666:123.123.123.123",           1, {0x1111, 0, 0, 0x4444, 0x5555, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111::4444:5555:6666:7777:8888",                 1, {0x1111, 0, 0, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111::5555:6666:123.123.123.123",                1, {0x1111, 0, 0, 0, 0x5555, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111::5555:6666:7777:8888",                      1, {0x1111, 0, 0, 0, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111::6666:123.123.123.123",                     1, {0x1111, 0, 0, 0, 0, 0x6666, 0x7b7b, 0x7b7b}},
        {"1111::6666:7777:8888",                           1, {0x1111, 0, 0, 0, 0, 0x6666, 0x7777, 0x8888}},
        {"1111::7777:8888",                                1, {0x1111, 0, 0, 0, 0, 0, 0x7777, 0x8888}},
        {"1111::8888",                                     1, {0x1111, 0, 0, 0, 0, 0, 0, 0x8888}},
        {"1:2:3:4:5:6:1.2.3.4",                            1, {0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x201, 0x403}},
        {"1:2:3:4:5:6:7:8",                                1, {0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800}},
        {"1:2:3:4:5:6::",                                  1, {0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0, 0}},
        {"1:2:3:4:5:6::8",                                 1, {0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0, 0x800}},
        {"2001:0000:1234:0000:0000:C1C0:ABCD:0876",        1, {0x120, 0, 0x3412, 0, 0, 0xc0c1, 0xcdab, 0x7608}},
        {"2001:0000:4136:e378:8000:63bf:3fff:fdd2",        1, {0x120, 0, 0x3641, 0x78e3, 0x80, 0xbf63, 0xff3f, 0xd2fd}},
        {"2001:0db8:0:0:0:0:1428:57ab",                    1, {0x120, 0xb80d, 0, 0, 0, 0, 0x2814, 0xab57}},
        {"2001:0db8:1234:ffff:ffff:ffff:ffff:ffff",        1, {0x120, 0xb80d, 0x3412, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}},
        {"2001::CE49:7601:2CAD:DFFF:7C94:FFFE",            1, {0x120, 0, 0x49ce, 0x176, 0xad2c, 0xffdf, 0x947c, 0xfeff}},
        {"2001:db8:85a3::8a2e:370:7334",                   1, {0x120, 0xb80d, 0xa385, 0, 0, 0x2e8a, 0x7003, 0x3473}},
        {"3ffe:0b00:0000:0000:0001:0000:0000:000a",        1, {0xfe3f, 0xb, 0, 0, 0x100, 0, 0, 0xa00}},
        {"::",                                             1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::%16",                                          0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::/16",                                          0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::01234",                                        0, {0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::0",                                            1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0",                                          1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0",                                        1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0:0",                                      1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0:0:0",                                    1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0:0:0:0",                                  1, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::0:0:0:0:0:0:0",                                1, {0, 0, 0, 0, 0, 0, 0, 0}, 1},
        {"::0:a:b:c:d:e:f",                                1, {0, 0, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00, 0xf00}, 1},
        {"::123.123.123.123",                              1, {0, 0, 0, 0, 0, 0, 0x7b7b, 0x7b7b}},
        {"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",        1, {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}},
        {"':10.0.0.1",                                     0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"-1",                                             0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"02001:0000:1234:0000:0000:C1C0:ABCD:0876",       0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:00000:1234:0000:0000:C1C0:ABCD:0876",       0, {0x120, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:0000:01234:0000:0000:C1C0:ABCD:0876",       0, {0x120, 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:0000::01234.0",                             0, {0x120, 0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:0::b.0",                                    0, {0x120, 0, 0, 0, 0, 0, 0, 0xb00}},
        {"2001::0:b.0",                                    0, {0x120, 0, 0, 0, 0, 0, 0, 0xb00}},
        {"1.2.3.4",                                        0, {0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1.2.3.4:1111::5555",                             0, {0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1.2.3.4::5555",                                  0, {0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"11112222:3333:4444:5555:6666:1.2.3.4",           0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"11112222:3333:4444:5555:6666:7777:8888",         0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111",                                           0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"0x1111",                                         0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:22223333:4444:5555:6666:1.2.3.4",           0, {0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:22223333:4444:5555:6666:7777:8888",         0, {0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:123456789:4444:5555:6666:7777:8888",        0, {0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:1234567890abcdef0:4444:5555:6666:7777:888", 0, {0x1111, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:",                                     0, {0x1111, 0x2222, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:1.2.3.4",                              0, {0x1111, 0x2222, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333",                                 0, {0x1111, 0x2222, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111:2222:3333:4444:5555:6666::1.2.3.4",         0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0, 0x100}},
        {"1111:2222:3333:4444:5555:6666:7777:1.2.3.4",     0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x100}},
        {"1111:2222:3333:4444:5555:6666:7777:8888:",       0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111:2222:3333:4444:5555:6666:7777:8888:1.2.3.4",0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111:2222:3333:4444:5555:6666:7777:8888:9999",   0, {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888}},
        {"1111:2222:::",                                   0, {0x1111, 0x2222, 0, 0, 0, 0, 0, 0}},
        {"1111::5555:",                                    0, {0x1111, 0x5555, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1111::3333:4444:5555:6666:7777::",               0, {0x1111, 0, 0, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777}},
        {"1111:2222:::4444:5555:6666:1.2.3.4",             0, {0x1111, 0x2222, 0, 0, 0, 0, 0, 0}},
        {"1111::3333::5555:6666:1.2.3.4",                  0, {0x1111, 0, 0, 0, 0, 0, 0, 0x3333}},
        {"12345::6:7:8",                                   0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::001.2.3.4",                                   1, {0x100, 0, 0, 0, 0, 0, 0x201, 0x403}},
        {"1::1.002.3.4",                                   1, {0x100, 0, 0, 0, 0, 0, 0x201, 0x403}},
        {"1::0001.2.3.4",                                  0, {0x100, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.0002.3.4",                                  0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.256.4",                                   0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.4294967296.4",                            0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.18446744073709551616.4",                  0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.256",                                   0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.4294967296",                            0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.18446744073709551616",                  0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.300",                                   0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.300.",                                  0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2::1",                                      0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.4::1",                                  0, {0x100, 0, 0, 0, 0, 0, 0x201, 0x403}},
        {"1::1.",                                          0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2",                                         0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.",                                        0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3",                                       0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.",                                      0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.3.4",                                     1, {0x100, 0, 0, 0, 0, 0, 0x201, 0x403}},
        {"1::1.2.3.900",                                   0, {0x100, 0x201, 0xab03, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2.300.4",                                   0, {0x100, 0x201, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.256.3.4",                                   0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.256:3.4",                                   0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1.2a.3.4",                                    0, {0x100, 0xab01, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::256.2.3.4",                                   0, {0x100, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"1::1a.2.3.4",                                    0, {0x100, 0, 0, 0, 0, 0, 0, 0x1a00}},
        {"1::2::3",                                        0, {0x100, 0, 0, 0, 0, 0, 0, 0x200}},
        {"2001:0000:1234: 0000:0000:C1C0:ABCD:0876",       0, {0x120, 0, 0x3412, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"2001:0000:1234:0000:0000:C1C0:ABCD:0876  0",     0, {0x120, 0, 0x3412, 0, 0, 0xc0c1, 0xcdab, 0x7608}},
        {"2001:1:1:1:1:1:255Z255X255Y255",                 0, {0x120, 0x100, 0x100, 0x100, 0x100, 0x100, 0xabab, 0xabab}},
        {"2001::FFD3::57ab",                               0, {0x120, 0, 0, 0, 0, 0, 0, 0xd3ff}},
        {":",                                              0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {":1111:2222:3333:4444:5555:6666:1.2.3.4",         0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {":1111:2222:3333:4444:5555:6666:7777:8888",       0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {":1111::",                                        0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::-1",                                           0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::12345678",                                     0, {0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::123456789",                                    0, {0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::1234567890abcdef0",                            0, {0, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"::0x80000000",                                   0, {0, 0, 0, 0, 0, 0, 0, 0xffff}},
        {"::0x012345678",                                  0, {0, 0, 0, 0, 0, 0, 0, 0x7856}},
        {"::0x123456789",                                  0, {0, 0, 0, 0, 0, 0, 0, 0xffff}},
        {"::0x1234567890abcdef0",                          0, {0, 0, 0, 0, 0, 0, 0, 0xffff}},
        {"::.",                                            0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::..",                                           0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"::...",                                          0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {"XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:1.2.3.4",          0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
        {"[::]",                                           0, {0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab, 0xabab}},
    };

    BYTE buffer[32];
    int i, ret;

    /* inet_ntop and inet_pton became available in Vista and Win2008 */
    if (!p_inet_ntop)
    {
        win_skip("inet_ntop is not available\n");
        return;
    }

    WSASetLastError(0xdeadbeef);
    ret = p_inet_pton(AF_UNSPEC, NULL, buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = p_inet_pton(AF_INET, NULL, buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pInetPtonW(AF_UNSPEC, NULL, buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = pInetPtonW(AF_INET, NULL, buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = p_inet_pton(AF_UNSPEC, "127.0.0.1", buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEAFNOSUPPORT, "got error %u\n", WSAGetLastError());

    WSASetLastError(0xdeadbeef);
    ret = p_inet_pton(AF_UNSPEC, "2607:f0d0:1002:51::4", buffer);
    ok(ret == -1, "got %d\n", ret);
    ok(WSAGetLastError() == WSAEAFNOSUPPORT, "got error %u\n", WSAGetLastError());

    for (i = 0; i < ARRAY_SIZE(ipv4_tests); ++i)
    {
        WCHAR inputW[32];
        DWORD addr;

        winetest_push_context( "Address %s", debugstr_a(ipv4_tests[i].input) );

        WSASetLastError(0xdeadbeef);
        addr = 0xdeadbeef;
        ret = p_inet_pton(AF_INET, ipv4_tests[i].input, &addr);
        ok(ret == ipv4_tests[i].ret, "got %d\n", ret);
        ok(WSAGetLastError() == 0xdeadbeef, "got error %u\n", WSAGetLastError());
        ok(addr == ipv4_tests[i].addr, "got addr %#08lx\n", addr);

        MultiByteToWideChar(CP_ACP, 0, ipv4_tests[i].input, -1, inputW, ARRAY_SIZE(inputW));
        WSASetLastError(0xdeadbeef);
        addr = 0xdeadbeef;
        ret = pInetPtonW(AF_INET, inputW, &addr);
        ok(ret == ipv4_tests[i].ret, "got %d\n", ret);
        ok(WSAGetLastError() == (ret ? 0xdeadbeef : WSAEINVAL), "got error %u\n", WSAGetLastError());
        ok(addr == ipv4_tests[i].addr, "got addr %#08lx\n", addr);

        WSASetLastError(0xdeadbeef);
        addr = inet_addr(ipv4_tests[i].input);
        ok(addr == ipv4_tests[i].ret ? ipv4_tests[i].addr : INADDR_NONE, "got addr %#08lx\n", addr);
        ok(WSAGetLastError() == 0xdeadbeef, "got error %u\n", WSAGetLastError());

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(ipv6_tests); ++i)
    {
        unsigned short addr[8];
        WCHAR inputW[64];

        winetest_push_context( "Address %s", debugstr_a(ipv6_tests[i].input) );

        WSASetLastError(0xdeadbeef);
        memset(addr, 0xab, sizeof(addr));
        ret = p_inet_pton(AF_INET6, ipv6_tests[i].input, addr);
        if (ipv6_tests[i].broken)
            ok(ret == ipv6_tests[i].ret || broken(ret == ipv6_tests[i].broken_ret), "got %d\n", ret);
        else
            ok(ret == ipv6_tests[i].ret, "got %d\n", ret);
        ok(WSAGetLastError() == 0xdeadbeef, "got error %u\n", WSAGetLastError());
        if (ipv6_tests[i].broken)
            ok(!memcmp(addr, ipv6_tests[i].addr, sizeof(addr)) || broken(memcmp(addr, ipv6_tests[i].addr, sizeof(addr))),
               "address didn't match\n");
        else
            ok(!memcmp(addr, ipv6_tests[i].addr, sizeof(addr)), "address didn't match\n");

        MultiByteToWideChar(CP_ACP, 0, ipv6_tests[i].input, -1, inputW, ARRAY_SIZE(inputW));
        WSASetLastError(0xdeadbeef);
        memset(addr, 0xab, sizeof(addr));
        ret = pInetPtonW(AF_INET6, inputW, addr);
        if (ipv6_tests[i].broken)
            ok(ret == ipv6_tests[i].ret || broken(ret == ipv6_tests[i].broken_ret), "got %d\n", ret);
        else
            ok(ret == ipv6_tests[i].ret, "got %d\n", ret);
        ok(WSAGetLastError() == (ret ? 0xdeadbeef : WSAEINVAL), "got error %u\n", WSAGetLastError());
        if (ipv6_tests[i].broken)
            ok(!memcmp(addr, ipv6_tests[i].addr, sizeof(addr)) || broken(memcmp(addr, ipv6_tests[i].addr, sizeof(addr))),
               "address didn't match\n");
        else
            ok(!memcmp(addr, ipv6_tests[i].addr, sizeof(addr)), "address didn't match\n");

        winetest_pop_context();
    }
}

static void test_addr_to_print(void)
{
    char dst[16];
    char dst6[64];
    const char *pdst;
    struct in_addr in;
    struct in6_addr in6;

    u_long addr0_Num = 0x00000000;
    const char *addr0_Str = "0.0.0.0";
    u_long addr1_Num = 0x20201015;
    const char *addr1_Str = "21.16.32.32";
    u_char addr2_Num[16] = {0,0,0,0,0,0,0,0,0,0,0xff,0xfe,0xcC,0x98,0xbd,0x74};
    const char *addr2_Str = "::fffe:cc98:bd74";
    u_char addr3_Num[16] = {0x20,0x30,0xa4,0xb1};
    const char *addr3_Str = "2030:a4b1::";
    u_char addr4_Num[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0xcC,0x98,0xbd,0x74};
    const char *addr4_Str = "::204.152.189.116";

    /* Test IPv4 addresses */
    in.s_addr = addr0_Num;

    pdst = inet_ntoa(*((struct in_addr *)&in.s_addr));
    ok(pdst != NULL, "inet_ntoa failed %s\n", dst);
    ok(!strcmp(pdst, addr0_Str),"Address %s != %s\n", pdst, addr0_Str);

    /* Test that inet_ntoa and inet_ntop return the same value */
    in.S_un.S_addr = addr1_Num;
    pdst = inet_ntoa(*((struct in_addr *)&in.s_addr));
    ok(pdst != NULL, "inet_ntoa failed %s\n", dst);
    ok(!strcmp(pdst, addr1_Str),"Address %s != %s\n", pdst, addr1_Str);

    /* inet_ntop became available in Vista and Win2008 */
    if (!p_inet_ntop)
    {
        win_skip("InetNtop not present, not executing tests\n");
        return;
    }

    /* Second part of test */
    pdst = p_inet_ntop(AF_INET, &in.s_addr, dst, sizeof(dst));
    ok(pdst != NULL, "InetNtop failed %s\n", dst);
    ok(!strcmp(pdst, addr1_Str),"Address %s != %s\n", pdst, addr1_Str);

    /* Test invalid parm conditions */
    pdst = p_inet_ntop(1, (void *)&in.s_addr, dst, sizeof(dst));
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == WSAEAFNOSUPPORT, "Should be WSAEAFNOSUPPORT\n");

    /* Test Null destination */
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET, &in.s_addr, NULL, sizeof(dst));
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test zero length passed */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET, &in.s_addr, dst, 0);
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test length one shorter than the address length */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET, &in.s_addr, dst, 6);
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test longer length is ok */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET, &in.s_addr, dst, sizeof(dst)+1);
    ok(pdst != NULL, "The pointer should  be returned (%p)\n", pdst);
    ok(!strcmp(pdst, addr1_Str),"Address %s != %s\n", pdst, addr1_Str);

    /* Test the IPv6 addresses */

    /* Test an zero prefixed IPV6 address */
    memcpy(in6.u.Byte, addr2_Num, sizeof(addr2_Num));
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, sizeof(dst6));
    ok(pdst != NULL, "InetNtop failed %s\n", dst6);
    ok(!strcmp(pdst, addr2_Str),"Address %s != %s\n", pdst, addr2_Str);

    /* Test an zero suffixed IPV6 address */
    memcpy(in6.s6_addr, addr3_Num, sizeof(addr3_Num));
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, sizeof(dst6));
    ok(pdst != NULL, "InetNtop failed %s\n", dst6);
    ok(!strcmp(pdst, addr3_Str),"Address %s != %s\n", pdst, addr3_Str);

    /* Test the IPv6 address contains the IPv4 address in IPv4 notation */
    memcpy(in6.s6_addr, addr4_Num, sizeof(addr4_Num));
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, sizeof(dst6));
    ok(pdst != NULL, "InetNtop failed %s\n", dst6);
    ok(!strcmp(pdst, addr4_Str),"Address %s != %s\n", pdst, addr4_Str);

    /* Test invalid parm conditions */
    memcpy(in6.u.Byte, addr2_Num, sizeof(addr2_Num));

    /* Test Null destination */
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, NULL, sizeof(dst6));
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test zero length passed */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, 0);
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test length one shorter than the address length */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, 16);
    ok(pdst == NULL, "The pointer should not be returned (%p)\n", pdst);
    ok(WSAGetLastError() == STATUS_INVALID_PARAMETER || WSAGetLastError() == WSAEINVAL /* Win7 */,
       "Should be STATUS_INVALID_PARAMETER or WSAEINVAL not 0x%x\n", WSAGetLastError());

    /* Test longer length is ok */
    WSASetLastError(0);
    pdst = NULL;
    pdst = p_inet_ntop(AF_INET6, &in6.s6_addr, dst6, 18);
    ok(pdst != NULL, "The pointer should be returned (%p)\n", pdst);
}

static void test_WSAAddressToString(void)
{
    static struct
    {
        ULONG address;
        USHORT port;
        char output[32];
    }
    ipv4_tests[] =
    {
        { 0, 0, "0.0.0.0" },
        { 0xffffffff, 0, "255.255.255.255" },
        { 0, 0xffff, "0.0.0.0:65535" },
        { 0xffffffff, 0xffff, "255.255.255.255:65535" },
    };
    static struct
    {
        USHORT address[8];
        ULONG scope;
        USHORT port;
        char output[64];
    }
    ipv6_tests[] =
    {
        { { 0, 0, 0, 0, 0, 0, 0, 0x100 }, 0, 0, "::1" },
        { { 0xab20, 0, 0, 0, 0, 0, 0, 0x100 }, 0, 0, "20ab::1" },
        { { 0xab20, 0, 0, 0, 0, 0, 0, 0x120 }, 0, 0xfa81, "[20ab::2001]:33274" },
        { { 0xab20, 0, 0, 0, 0, 0, 0, 0x120 }, 0x1234, 0xfa81, "[20ab::2001%4660]:33274" },
        { { 0xab20, 0, 0, 0, 0, 0, 0, 0x120 }, 0x1234, 0, "20ab::2001%4660" },
    };
    SOCKADDR_IN sockaddr;
    SOCKADDR_IN6 sockaddr6;
    char output[64];
    WCHAR outputW[64], expected_outputW[64];
    unsigned int i;
    SOCKET v6;
    INT ret;
    DWORD len;

    len = 0;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = 0;
    sockaddr.sin_port = 0;
    WSASetLastError( 0xdeadbeef );
    ret = WSAAddressToStringA( (SOCKADDR *)&sockaddr, sizeof(sockaddr), NULL, output, &len );
    ok( ret == SOCKET_ERROR, "WSAAddressToStringA() returned %d, expected SOCKET_ERROR\n", ret );
    ok( WSAGetLastError() == WSAEFAULT, "WSAAddressToStringA() gave error %d, expected WSAEFAULT\n", WSAGetLastError() );
    ok( len == 8, "WSAAddressToStringA() gave length %ld, expected 8\n", len );

    len = 0;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = 0;
    sockaddr.sin_port = 0;
    WSASetLastError( 0xdeadbeef );
    ret = WSAAddressToStringW( (SOCKADDR *)&sockaddr, sizeof(sockaddr), NULL, NULL, &len );
    ok( ret == SOCKET_ERROR, "got %d\n", ret );
    ok( WSAGetLastError() == WSAEFAULT, "got %08x\n", WSAGetLastError() );
    ok( len == 8, "got %lu\n", len );

    len = ARRAY_SIZE(outputW);
    memset( outputW, 0, sizeof(outputW) );
    ret = WSAAddressToStringW( (SOCKADDR *)&sockaddr, sizeof(sockaddr), NULL, outputW, &len );
    ok( !ret, "WSAAddressToStringW() returned %d\n", ret );
    ok( len == 8, "got %lu\n", len );
    ok( !wcscmp(outputW, L"0.0.0.0"), "got %s\n", wine_dbgstr_w(outputW) );

    for (i = 0; i < ARRAY_SIZE(ipv4_tests); ++i)
    {
        winetest_push_context( "Test %u", i );

        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = ipv4_tests[i].address;
        sockaddr.sin_port = ipv4_tests[i].port;

        len = sizeof(output);
        memset( output, 0, len );
        ret = WSAAddressToStringA( (SOCKADDR *)&sockaddr, sizeof(sockaddr), NULL, output, &len );
        ok( !ret, "got error %d\n", WSAGetLastError() );
        ok( !strcmp( output, ipv4_tests[i].output ), "got string %s\n", debugstr_a(output) );
        ok( len == strlen(ipv4_tests[i].output) + 1, "got len %lu\n", len );

        len = sizeof(outputW);
        memset( outputW, 0, len );
        ret = WSAAddressToStringW( (SOCKADDR *)&sockaddr, sizeof(sockaddr), NULL, outputW, &len );
        MultiByteToWideChar( CP_ACP, 0, ipv4_tests[i].output, -1,
                             expected_outputW, ARRAY_SIZE(expected_outputW) );
        ok( !ret, "got error %d\n", WSAGetLastError() );
        ok( !wcscmp( outputW, expected_outputW ), "got string %s\n", debugstr_w(outputW) );
        ok( len == wcslen(expected_outputW) + 1, "got len %lu\n", len );

        winetest_pop_context();
    }

    v6 = socket( AF_INET6, SOCK_STREAM, IPPROTO_TCP );
    if (v6 == -1 && WSAGetLastError() == WSAEAFNOSUPPORT)
    {
        skip( "IPv6 is not supported\n" );
        return;
    }
    closesocket( v6 );

    for (i = 0; i < ARRAY_SIZE(ipv6_tests); ++i)
    {
        winetest_push_context( "Test %u", i );

        sockaddr6.sin6_family = AF_INET6;
        sockaddr6.sin6_scope_id = ipv6_tests[i].scope;
        sockaddr6.sin6_port = ipv6_tests[i].port;
        memcpy( sockaddr6.sin6_addr.s6_addr, ipv6_tests[i].address, sizeof(ipv6_tests[i].address) );

        len = sizeof(output);
        ret = WSAAddressToStringA( (SOCKADDR *)&sockaddr6, sizeof(sockaddr6), NULL, output, &len );
        ok( !ret, "got error %d\n", WSAGetLastError() );
        ok( !strcmp( output, ipv6_tests[i].output ), "got string %s\n", debugstr_a(output) );
        ok( len == strlen(ipv6_tests[i].output) + 1, "got len %lu\n", len );

        len = sizeof(outputW);
        ret = WSAAddressToStringW( (SOCKADDR *)&sockaddr6, sizeof(sockaddr6), NULL, outputW, &len );
        MultiByteToWideChar( CP_ACP, 0, ipv6_tests[i].output, -1,
                             expected_outputW, ARRAY_SIZE(expected_outputW) );
        ok( !ret, "got error %d\n", WSAGetLastError() );
        ok( !wcscmp( outputW, expected_outputW ), "got string %s\n", debugstr_w(outputW) );
        ok( len == wcslen(expected_outputW) + 1, "got len %lu\n", len );

        winetest_pop_context();
    }
}

static void test_WSAStringToAddress(void)
{
    static struct
    {
        char input[32];
        ULONG address;
        USHORT port;
        int error;
    }
    ipv4_tests[] =
    {
        { "",                       0,          0, WSAEINVAL },
        { " ",                      0,          0, WSAEINVAL },
        { "1.1.1.1",                0x01010101 },
        { "0.0.0.0",                0 },
        { "127.127.127.127",        0x7f7f7f7f },
        { "127.127.127.127:",       0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:0",      0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:123",    0x7f7f7f7f, 123 },
        { "127.127.127.127:65535",  0x7f7f7f7f, 65535 },
        { "127.127.127.127:65537",  0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127::1",     0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:0xAbCd", 0x7f7f7f7f, 43981 },
        { "127.127.127.127:0XaBcD", 0x7f7f7f7f, 43981 },
        { "127.127.127.127:0x",     0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:0x10000",0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:010",    0x7f7f7f7f, 8 },
        { "127.127.127.127:007",    0x7f7f7f7f, 7 },
        { "127.127.127.127:08",     0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:008",    0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:0a",     0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:0o10",   0x7f7f7f7f, 0, WSAEINVAL },
        { "127.127.127.127:0b10",   0x7f7f7f7f, 0, WSAEINVAL },
        { "255.255.255.255",        0xffffffff },
        { "255.255.255.255:123",    0xffffffff, 123 },
        { "255.255.255.255:65535",  0xffffffff, 65535 },
        { "255.255.255.256",        0,          0, WSAEINVAL },
        { "a",                      0,          0, WSAEINVAL },
        { "1.1.1.0xaA",             0x010101aa },
        { "1.1.1.0XaA",             0x010101aa },
        { "1.1.1.0x",               0,          0, WSAEINVAL },
        { "1.1.1.0xff",             0x010101ff },
        { "1.1.1.0x100",            0,          0, WSAEINVAL },
        { "1.1.1.010",              0x01010108 },
        { "1.1.1.00",               0x01010100 },
        { "1.1.1.007",              0x01010107 },
        { "1.1.1.08",               0,          0, WSAEINVAL },
        { "1.1.1.008",              0x01010100, 0, WSAEINVAL },
        { "1.1.1.0a",               0x01010100, 0, WSAEINVAL },
        { "1.1.1.0o10",             0x01010100, 0, WSAEINVAL },
        { "1.1.1.0b10",             0x01010100, 0, WSAEINVAL },
        { "1.1.1.-2",               0,          0, WSAEINVAL },
        { "1",                      0x00000001 },
        { "-1",                     0,          0, WSAEINVAL },
        { "1:1",                    0x00000001, 1 },
        { "1.2",                    0x01000002 },
        { "1.2:1",                  0x01000002, 1 },
        { "1000.2000",              0,          0, WSAEINVAL },
        { "1.2.",                   0,          0, WSAEINVAL },
        { "1..2",                   0,          0, WSAEINVAL },
        { "1...2",                  0,          0, WSAEINVAL },
        { "1.2.3",                  0x01020003 },
        { "1.2.3.",                 0,          0, WSAEINVAL },
        { "1.2.3:1",                0x01020003, 1 },
        { "203569230",              0x0c22384e },
        { "203569230:123",          0x0c22384e, 123 },
        { "2001::1",                0x000007d1, 0, WSAEINVAL },
        { "1.223756",               0x01036a0c },
        { "3.4.756",                0x030402f4 },
        { "756.3.4",                0,          0, WSAEINVAL },
        { "3.756.4",                0,          0, WSAEINVAL },
        { "3.4.756.1",              0,          0, WSAEINVAL },
        { "3.4.65536",              0,          0, WSAEINVAL },
        { "3.4.5.6.7",              0,          0, WSAEINVAL },
        { "3.4.5.+6",               0,          0, WSAEINVAL },
        { " 3.4.5.6",               0,          0, WSAEINVAL },
        { "\t3.4.5.6",              0,          0, WSAEINVAL },
        { "3.4.5.6 ",               0x03040506, 0, WSAEINVAL },
        { "3.4.5.6:7 ",             0x03040506, 0, WSAEINVAL },
        { "3.4.5.6: 7",             0x03040506, 0, WSAEINVAL },
        { "3. 4.5.6",               0,          0, WSAEINVAL },
        { ".",                      0,          0, WSAEINVAL },
        { "[0.1.2.3]",              0,          0, WSAEINVAL },
        { "0x00010203",             0x00010203 },
        { "0X00010203",             0x00010203 },
        { "0x00010203:10",          0x00010203, 10 },
        { "0x1234",                 0x00001234 },
        { "0x123456789",            0x23456789 },
        { "0x00010Q03",             0x00000010, 0, WSAEINVAL },
        { "x00010203",              0,          0, WSAEINVAL },
        { "1234BEEF",               0x000004d2, 0, WSAEINVAL },
        { "017700000001",           0x7f000001 },
        { "0777",                   0x000001ff },
        { "0777:10",                0x000001ff, 10 },
        { "::1",                    0,          0, WSAEINVAL },
        { ":1",                     0,          0, WSAEINVAL },
    };
    static struct
    {
        char input[64];
        USHORT address[8];
        USHORT port;
        int error;
        int broken;
        int broken_error;
    }
    ipv6_tests[] =
    {
        { "::1", { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1]", { 0, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "[::1]:65535", { 0, 0, 0, 0, 0, 0, 0, 0x100 }, 0xffff },
        { "2001::1", { 0x120, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "::1]:65535", { 0, 0, 0, 0, 0, 0, 0, 0x100 }, 0, WSAEINVAL },
        { "001::1", { 0x100, 0, 0, 0, 0, 0, 0, 0x100 } },
        { "::1:2:3:4:5:6:7", { 0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700 }, 0, 0, 1, WSAEINVAL },
        { "1.2.3.4", { 0x201, 0x3, 0, 0, 0, 0, 0, 0 }, 0, WSAEINVAL },
        { "1:2:3:", { 0x100, 0x200, 0x300, 0, 0, 0, 0 }, 0, WSAEINVAL },
        { "", { 0, 0, 0, 0, 0, 0, 0, 0 }, 0, WSAEINVAL },
    };

    int len, ret, expected_len;
    WCHAR inputW[64];
    SOCKADDR_IN sockaddr;
    SOCKADDR_IN6 sockaddr6;
    unsigned int i, j;

    len = 0;
    WSASetLastError( 0 );
    ret = WSAStringToAddressA( ipv4_tests[0].input, AF_INET, NULL, (SOCKADDR *)&sockaddr, &len );
    ok( ret == SOCKET_ERROR, "WSAStringToAddressA() returned %d, expected SOCKET_ERROR\n", ret );
    ok( WSAGetLastError() == WSAEFAULT, "WSAStringToAddress() gave error %d, expected WSAEFAULT\n", WSAGetLastError() );
    ok( len >= sizeof(sockaddr) || broken(len == 0) /* xp */,
        "WSAStringToAddress() gave length %d, expected at least %Id\n", len, sizeof(sockaddr) );

    for (i = 0; i < 2; i++)
    {
        winetest_push_context( i ? "unicode" : "ascii" );

        for (j = 0; j < ARRAY_SIZE(ipv4_tests); j++)
        {
            len = sizeof(sockaddr) + 10;
            expected_len = ipv4_tests[j].error ? len : sizeof(sockaddr);
            memset( &sockaddr, 0xab, sizeof(sockaddr) );

            winetest_push_context( "addr %s", debugstr_a(ipv4_tests[j].input) );

            WSASetLastError( 0 );
            if (i == 0)
            {
                ret = WSAStringToAddressA( ipv4_tests[j].input, AF_INET, NULL, (SOCKADDR *)&sockaddr, &len );
            }
            else
            {
                MultiByteToWideChar( CP_ACP, 0, ipv4_tests[j].input, -1, inputW, ARRAY_SIZE(inputW) );
                ret = WSAStringToAddressW( inputW, AF_INET, NULL, (SOCKADDR *)&sockaddr, &len );
            }
            ok( ret == (ipv4_tests[j].error ? SOCKET_ERROR : 0), "got %d\n", ret );
            ok( WSAGetLastError() == ipv4_tests[j].error, "got error %d\n", WSAGetLastError() );
            ok( sockaddr.sin_family == (ipv4_tests[j].error ? 0 : AF_INET),
                "got family %#x\n", sockaddr.sin_family );
            ok( sockaddr.sin_addr.s_addr == htonl( ipv4_tests[j].address ),
                "got addr %08lx\n", sockaddr.sin_addr.s_addr );
            ok( sockaddr.sin_port == htons( ipv4_tests[j].port ), "got port %u\n", sockaddr.sin_port );
            ok( len == expected_len, "got len %d\n", len );

            winetest_pop_context();
        }

        for (j = 0; j < ARRAY_SIZE(ipv6_tests); j++)
        {
            len = sizeof(sockaddr6) + 10;
            expected_len = ipv6_tests[j].error ? len : sizeof(sockaddr6);
            memset( &sockaddr6, 0xab, sizeof(sockaddr6) );

            WSASetLastError( 0 );
            if (i == 0)
            {
                ret = WSAStringToAddressA( ipv6_tests[j].input, AF_INET6, NULL, (SOCKADDR *)&sockaddr6, &len );
            }
            else
            {
                MultiByteToWideChar( CP_ACP, 0, ipv6_tests[j].input, -1, inputW, ARRAY_SIZE(inputW) );
                ret = WSAStringToAddressW( inputW, AF_INET6, NULL, (SOCKADDR *)&sockaddr6, &len );
            }
            if (j == 0 && ret == SOCKET_ERROR)
            {
                win_skip("IPv6 not supported\n");
                break;
            }

            winetest_push_context( "addr %s", debugstr_a(ipv6_tests[j].input) );

            if (ipv6_tests[j].broken)
            {
                ok( ret == (ipv6_tests[j].error ? SOCKET_ERROR : 0) ||
                    broken(ret == (ipv6_tests[j].broken_error ? SOCKET_ERROR : 0)), "got %d\n", ret );
                ok( WSAGetLastError() == ipv6_tests[j].error ||
                    broken(WSAGetLastError() == ipv6_tests[j].broken_error), "got error %d\n", WSAGetLastError() );
                ok( !memcmp( &sockaddr6.sin6_addr, ipv6_tests[j].address, sizeof(sockaddr6.sin6_addr) ) ||
                    broken(memcmp( &sockaddr6.sin6_addr, ipv6_tests[j].address, sizeof(sockaddr6.sin6_addr) )),
                    "got addr %x:%x:%x:%x:%x:%x:%x:%x\n",
                    sockaddr6.sin6_addr.s6_words[0], sockaddr6.sin6_addr.s6_words[1],
                    sockaddr6.sin6_addr.s6_words[2], sockaddr6.sin6_addr.s6_words[3],
                    sockaddr6.sin6_addr.s6_words[4], sockaddr6.sin6_addr.s6_words[5],
                    sockaddr6.sin6_addr.s6_words[6], sockaddr6.sin6_addr.s6_words[7] );
            }
            else
            {
                ok( ret == (ipv6_tests[j].error ? SOCKET_ERROR : 0), "got %d\n", ret );
                ok( WSAGetLastError() == ipv6_tests[j].error, "got error %d\n", WSAGetLastError() );
                ok( !memcmp( &sockaddr6.sin6_addr, ipv6_tests[j].address, sizeof(sockaddr6.sin6_addr) ),
                    "got addr %x:%x:%x:%x:%x:%x:%x:%x\n",
                    sockaddr6.sin6_addr.s6_words[0], sockaddr6.sin6_addr.s6_words[1],
                    sockaddr6.sin6_addr.s6_words[2], sockaddr6.sin6_addr.s6_words[3],
                    sockaddr6.sin6_addr.s6_words[4], sockaddr6.sin6_addr.s6_words[5],
                    sockaddr6.sin6_addr.s6_words[6], sockaddr6.sin6_addr.s6_words[7] );
                ok( sockaddr6.sin6_family == (ipv6_tests[j].error ? 0 : AF_INET6),
                    "got family %#x\n", sockaddr6.sin6_family );
                ok( len == expected_len, "got len %d\n", len );
            }
            ok( !sockaddr6.sin6_scope_id, "got scope id %lu\n", sockaddr6.sin6_scope_id );
            ok( sockaddr6.sin6_port == ipv6_tests[j].port, "got port %u\n", sockaddr6.sin6_port );
            ok( !sockaddr6.sin6_flowinfo, "got flowinfo %lu\n", sockaddr6.sin6_flowinfo );

            winetest_pop_context();
        }

        winetest_pop_context();
    }
}

/* Tests used in both getaddrinfo and GetAddrInfoW */
static const struct addr_hint_tests
{
    int family, socktype, protocol;
    DWORD error;
}
hinttests[] =
{
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0}, /* 0 */
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_UDP, 0},
    {AF_UNSPEC, SOCK_STREAM, IPPROTO_IPV6,0},
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_TCP, 0},
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_UDP, 0},
    {AF_UNSPEC, SOCK_DGRAM,  IPPROTO_IPV6,0},
    {AF_INET,   SOCK_STREAM, IPPROTO_TCP, 0},
    {AF_INET,   SOCK_STREAM, IPPROTO_UDP, 0},
    {AF_INET,   SOCK_STREAM, IPPROTO_IPV6,0},
    {AF_INET,   SOCK_DGRAM,  IPPROTO_TCP, 0},
    {AF_INET,   SOCK_DGRAM,  IPPROTO_UDP, 0}, /* 10 */
    {AF_INET,   SOCK_DGRAM,  IPPROTO_IPV6,0},
    {AF_UNSPEC, 0,           IPPROTO_TCP, 0},
    {AF_UNSPEC, 0,           IPPROTO_UDP, 0},
    {AF_UNSPEC, 0,           IPPROTO_IPV6,0},
    {AF_UNSPEC, SOCK_STREAM, 0,           0},
    {AF_UNSPEC, SOCK_DGRAM,  0,           0},
    {AF_INET,   0,           IPPROTO_TCP, 0},
    {AF_INET,   0,           IPPROTO_UDP, 0},
    {AF_INET,   0,           IPPROTO_IPV6,0},
    {AF_INET,   SOCK_STREAM, 0,           0}, /* 20 */
    {AF_INET,   SOCK_DGRAM,  0,           0},
    {AF_UNSPEC, 999,         IPPROTO_TCP, WSAESOCKTNOSUPPORT},
    {AF_UNSPEC, 999,         IPPROTO_UDP, WSAESOCKTNOSUPPORT},
    {AF_UNSPEC, 999,         IPPROTO_IPV6,WSAESOCKTNOSUPPORT},
    {AF_INET,   999,         IPPROTO_TCP, WSAESOCKTNOSUPPORT},
    {AF_INET,   999,         IPPROTO_UDP, WSAESOCKTNOSUPPORT},
    {AF_INET,   999,         IPPROTO_IPV6,WSAESOCKTNOSUPPORT},
    {AF_UNSPEC, SOCK_STREAM, 999,         0},
    {AF_UNSPEC, SOCK_STREAM, 999,         0},
    {AF_INET,   SOCK_DGRAM,  999,         0}, /* 30 */
    {AF_INET,   SOCK_DGRAM,  999,         0},
};

static void compare_addrinfow(ADDRINFOW *a, ADDRINFOW *b)
{
    for (; a && b; a = a->ai_next, b = b->ai_next)
    {
        ok(a->ai_flags == b->ai_flags,
           "Wrong flags %d != %d\n", a->ai_flags, b->ai_flags);
        ok(a->ai_family == b->ai_family,
           "Wrong family %d != %d\n", a->ai_family, b->ai_family);
        ok(a->ai_socktype == b->ai_socktype,
           "Wrong socktype %d != %d\n", a->ai_socktype, b->ai_socktype);
        ok(a->ai_protocol == b->ai_protocol,
           "Wrong protocol %d != %d\n", a->ai_protocol, b->ai_protocol);
        ok(a->ai_addrlen == b->ai_addrlen,
           "Wrong addrlen %Iu != %Iu\n", a->ai_addrlen, b->ai_addrlen);
        ok(!memcmp(a->ai_addr, b->ai_addr, min(a->ai_addrlen, b->ai_addrlen)),
           "Wrong address data\n");
        if (a->ai_canonname && b->ai_canonname)
        {
            ok(!lstrcmpW(a->ai_canonname, b->ai_canonname), "Wrong canonical name '%s' != '%s'\n",
               wine_dbgstr_w(a->ai_canonname), wine_dbgstr_w(b->ai_canonname));
        }
        else
            ok(!a->ai_canonname && !b->ai_canonname, "Expected both names absent (%p != %p)\n",
               a->ai_canonname, b->ai_canonname);
    }
    ok(!a && !b, "Expected both addresses null (%p != %p)\n", a, b);
}

static void test_GetAddrInfoW(void)
{
    static const WCHAR port[] = {'8','0',0};
    static const WCHAR empty[] = {0};
    static const WCHAR localhost[] = {'l','o','c','a','l','h','o','s','t',0};
    static const WCHAR nxdomain[] =
        {'n','x','d','o','m','a','i','n','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};
    static const WCHAR zero[] = {'0',0};
    int i, ret;
    ADDRINFOW *result, *result2, *p, hint;
    WCHAR name[256];
    DWORD size = ARRAY_SIZE(name);
    /* te su to.winehq.org written in katakana */
    static const WCHAR idn_domain[] =
        {0x30C6,0x30B9,0x30C8,'.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR idn_punycode[] =
        {'x','n','-','-','z','c','k','z','a','h','.','w','i','n','e','h','q','.','o','r','g',0};

    memset(&hint, 0, sizeof(ADDRINFOW));
    name[0] = 0;
    GetComputerNameExW( ComputerNamePhysicalDnsHostname, name, &size );

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = GetAddrInfoW(NULL, NULL, NULL, &result);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = GetAddrInfoW(empty, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(NULL, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    result2 = NULL;
    ret = GetAddrInfoW(NULL, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    result = NULL;
    ret = GetAddrInfoW(empty, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    result2 = NULL;
    ret = GetAddrInfoW(empty, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    result = NULL;
    ret = GetAddrInfoW(localhost, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(localhost, empty, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(localhost, zero, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(localhost, port, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    ret = GetAddrInfoW(localhost, NULL, &hint, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    result = NULL;
    SetLastError(0xdeadbeef);
    ret = GetAddrInfoW(localhost, port, &hint, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    FreeAddrInfoW(result);

    /* try to get information from the computer name, result is the same
     * as if requesting with an empty host name. */
    ret = GetAddrInfoW(name, NULL, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = GetAddrInfoW(empty, NULL, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    ret = GetAddrInfoW(name, empty, NULL, &result);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = GetAddrInfoW(empty, empty, NULL, &result2);
    ok(!ret, "GetAddrInfoW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    compare_addrinfow(result, result2);
    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = GetAddrInfoW(NULL, NULL, NULL, &result);
    if (ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = (ADDRINFOW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = GetAddrInfoW(nxdomain, NULL, NULL, &result);
    if (ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    for (i = 0; i < ARRAY_SIZE(hinttests); i++)
    {
        hint.ai_family = hinttests[i].family;
        hint.ai_socktype = hinttests[i].socktype;
        hint.ai_protocol = hinttests[i].protocol;

        result = NULL;
        SetLastError(0xdeadbeef);
        ret = GetAddrInfoW(localhost, NULL, &hint, &result);
        ok(ret == hinttests[i].error, "test %d: wrong ret %d\n", i, ret);
        if (!ret)
        {
            for (p = result; p; p = p->ai_next)
            {
                /* when AF_UNSPEC is used the return will be either AF_INET or AF_INET6 */
                if (hinttests[i].family == AF_UNSPEC)
                    ok(p->ai_family == AF_INET || p->ai_family == AF_INET6,
                       "test %d: expected AF_INET or AF_INET6, got %d\n",
                       i, p->ai_family);
                else
                    ok(p->ai_family == hinttests[i].family,
                       "test %d: expected family %d, got %d\n",
                       i, hinttests[i].family, p->ai_family);

                ok(p->ai_socktype == hinttests[i].socktype,
                   "test %d: expected type %d, got %d\n",
                   i, hinttests[i].socktype, p->ai_socktype);
                ok(p->ai_protocol == hinttests[i].protocol,
                   "test %d: expected protocol %d, got %d\n",
                   i, hinttests[i].protocol, p->ai_protocol);
            }
            FreeAddrInfoW(result);
        }
        else
        {
            ok(WSAGetLastError() == hinttests[i].error, "test %d: wrong error %d\n", i, WSAGetLastError());
        }
    }

    /* Test IDN resolution (Internationalized Domain Names) present since Windows 8 */
    result = NULL;
    ret = GetAddrInfoW(idn_punycode, NULL, NULL, &result);
    ok(!ret, "got %d expected success\n", ret);
    ok(result != NULL, "got %p\n", result);
    FreeAddrInfoW(result);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result = NULL;
    ret = GetAddrInfoW(idn_punycode, NULL, &hint, &result);
    ok(!ret, "got %d expected success\n", ret);
    ok(result != NULL, "got %p\n", result);

    result2 = NULL;
    ret = GetAddrInfoW(idn_domain, NULL, NULL, &result2);
    if (broken(ret == WSAHOST_NOT_FOUND))
    {
        FreeAddrInfoW(result);
        win_skip("IDN resolution not supported in Win <= 7\n");
        return;
    }

    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);
    FreeAddrInfoW(result2);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result2 = NULL;
    ret = GetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);

    /* ensure manually resolved punycode and unicode hosts result in same data */
    compare_addrinfow(result, result2);

    FreeAddrInfoW(result);
    FreeAddrInfoW(result2);

    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = 0;

    result2 = NULL;
    ret = GetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(!ret, "got %d expected success\n", ret);
    ok(result2 != NULL, "got %p\n", result2);
    FreeAddrInfoW(result2);

    /* Disable IDN resolution and test again*/
    hint.ai_family = AF_INET;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_flags = AI_DISABLE_IDN_ENCODING;

    SetLastError(0xdeadbeef);
    result2 = NULL;
    ret = GetAddrInfoW(idn_domain, NULL, &hint, &result2);
    ok(ret == WSAHOST_NOT_FOUND || ret == WSATRY_AGAIN, "got %d\n", ret);
    ok(WSAGetLastError() == ret, "got %d\n", WSAGetLastError());
    ok(result2 == NULL, "got %p\n", result2);
}

static struct completion_routine_test
{
    WSAOVERLAPPED  *overlapped;
    DWORD           error;
    DWORD           error2;
    ADDRINFOEXW   **result;
    HANDLE          event;
    DWORD           called;
} completion_routine_test;

static void CALLBACK completion_routine(DWORD error, DWORD byte_count, WSAOVERLAPPED *overlapped)
{
    struct completion_routine_test *test = &completion_routine_test;

    ok(error == test->error || (test->error2 && error == test->error2), "got %lu\n", error);
    ok(!byte_count, "got %lu\n", byte_count);
    ok(overlapped == test->overlapped, "got %p\n", overlapped);
    ok(overlapped->Internal == test->error, "got %Iu\n", overlapped->Internal);
    ok(overlapped->Pointer == test->result, "got %p\n", overlapped->Pointer);
    ok(overlapped->hEvent == NULL, "got %p\n", overlapped->hEvent);

    test->called++;
    SetEvent(test->event);
}

static void test_GetAddrInfoExW(void)
{
    static const WCHAR empty[] = {0};
    static const WCHAR localhost[] = {'l','o','c','a','l','h','o','s','t',0};
    static const WCHAR winehq[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR nxdomain[] = {'n','x','d','o','m','a','i','n','.','w','i','n','e','h','q','.','o','r','g',0};
    ADDRINFOEXW *result, hints;
    OVERLAPPED overlapped;
    HANDLE event;
    int ret;

    if (!pGetAddrInfoExW || !pGetAddrInfoExOverlappedResult)
    {
        win_skip("GetAddrInfoExW and/or GetAddrInfoExOverlappedResult not present\n");
        return;
    }

    event = WSACreateEvent();

    result = (ADDRINFOEXW *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(NULL, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(empty, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(!ret, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    pFreeAddrInfoExW(result);

    result = NULL;
    ret = pGetAddrInfoExW(localhost, NULL, NS_DNS, NULL, NULL, &result, NULL, NULL, NULL, NULL);
    ok(!ret, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    pFreeAddrInfoExW(result);

    ret = pGetAddrInfoExOverlappedResult(NULL);
    ok(ret == WSAEINVAL, "overlapped result is %d\n", ret);

    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    overlapped.hEvent = event;
    ResetEvent(event);
    ret = pGetAddrInfoExW(localhost, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    /* result pointer is cleared by GetAddrInfoExW(), but may be set to the
     * actual addrinfo before we can verify that */
    ok(result != (void *)0xdeadbeef, "result was not changed\n");
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    pFreeAddrInfoExW(result);

    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    ResetEvent(event);
    overlapped.hEvent = event;
    WSASetLastError(0xdeadbeef);
    ret = pGetAddrInfoExW(winehq, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == ERROR_IO_PENDING, "expected 11001, got %d\n", WSAGetLastError());
    ret = overlapped.Internal;
    ok(ret == WSAEINPROGRESS || ret == ERROR_SUCCESS, "overlapped.Internal = %u\n", ret);
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    ok(overlapped.hEvent == event, "hEvent changed %p\n", overlapped.hEvent);
    ok(overlapped.Internal == ERROR_SUCCESS, "overlapped.Internal = %Ix\n", overlapped.Internal);
    ok(overlapped.Pointer == &result, "overlapped.Pointer != &result\n");
    ok(result != NULL, "result == NULL\n");
    ok(!result->ai_blob, "ai_blob != NULL\n");
    ok(!result->ai_bloblen, "ai_bloblen != 0\n");
    ok(!result->ai_provider, "ai_provider = %s\n", wine_dbgstr_guid(result->ai_provider));
    pFreeAddrInfoExW(result);

    /* hints */
    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    ResetEvent(event);
    overlapped.hEvent = event;
    WSASetLastError(0xdeadbeef);
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_ALL | AI_V4MAPPED;
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    ret = pGetAddrInfoExW(winehq, NULL, NS_ALL, NULL, &hints, &result, NULL, &overlapped, NULL, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == ERROR_IO_PENDING, "expected 11001, got %d\n", WSAGetLastError());
    ret = overlapped.Internal;
    ok(ret == WSAEINPROGRESS || ret == ERROR_SUCCESS, "overlapped.Internal = %u\n", ret);
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    ok(result != NULL, "result == NULL\n");
    pFreeAddrInfoExW(result);

    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    ResetEvent(event);
    overlapped.hEvent = event;
    ret = pGetAddrInfoExW(NULL, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, NULL, NULL);
    todo_wine
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    todo_wine
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);
    ret = WaitForSingleObject(event, 0);
    todo_wine_if(ret != WAIT_TIMEOUT) /* Remove when abowe todo_wines are fixed */
    ok(ret == WAIT_TIMEOUT, "wait failed\n");

    /* event + completion routine */
    result = (void *)0xdeadbeef;
    memset(&overlapped, 0xcc, sizeof(overlapped));
    overlapped.hEvent = event;
    ResetEvent(event);
    ret = pGetAddrInfoExW(localhost, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, completion_routine, NULL);
    ok(ret == WSAEINVAL, "GetAddrInfoExW failed with %d\n", WSAGetLastError());

    /* completion routine, existing domain */
    result = (void *)0xdeadbeef;
    overlapped.hEvent = NULL;
    completion_routine_test.overlapped = &overlapped;
    completion_routine_test.error = ERROR_SUCCESS;
    completion_routine_test.error2 = ERROR_SUCCESS;
    completion_routine_test.result = &result;
    completion_routine_test.event = event;
    completion_routine_test.called = 0;
    ResetEvent(event);
    ret = pGetAddrInfoExW(winehq, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, completion_routine, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(!result, "result != NULL\n");
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(!ret, "overlapped result is %d\n", ret);
    ok(overlapped.hEvent == NULL, "hEvent changed %p\n", overlapped.hEvent);
    ok(overlapped.Internal == ERROR_SUCCESS, "overlapped.Internal = %Ix\n", overlapped.Internal);
    ok(overlapped.Pointer == &result, "overlapped.Pointer != &result\n");
    ok(completion_routine_test.called == 1, "got %lu\n", completion_routine_test.called);
    pFreeAddrInfoExW(result);

    /* completion routine, non-existing domain */
    result = (void *)0xdeadbeef;
    completion_routine_test.overlapped = &overlapped;
    completion_routine_test.error = WSAHOST_NOT_FOUND;
    completion_routine_test.error2 = WSANO_DATA;
    completion_routine_test.called = 0;
    ResetEvent(event);
    ret = pGetAddrInfoExW(nxdomain, NULL, NS_DNS, NULL, NULL, &result, NULL, &overlapped, completion_routine, NULL);
    ok(ret == ERROR_IO_PENDING, "GetAddrInfoExW failed with %d\n", WSAGetLastError());
    ok(!result, "result != NULL\n");
    ok(WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, "wait failed\n");
    ret = pGetAddrInfoExOverlappedResult(&overlapped);
    ok(ret == WSAHOST_NOT_FOUND, "overlapped result is %d\n", ret);
    ok(overlapped.hEvent == NULL, "hEvent changed %p\n", overlapped.hEvent);
    ok(overlapped.Internal == WSAHOST_NOT_FOUND, "overlapped.Internal = %Ix\n", overlapped.Internal);
    ok(overlapped.Pointer == &result, "overlapped.Pointer != &result\n");
    ok(completion_routine_test.called == 1, "got %lu\n", completion_routine_test.called);
    ok(result == NULL, "got %p\n", result);

    WSACloseEvent(event);
}

static void verify_ipv6_addrinfo(ADDRINFOA *result, const char *expect)
{
    SOCKADDR_IN6 *sockaddr6;
    char buffer[256];
    const char *ret;

    ok(result->ai_family == AF_INET6, "ai_family == %d\n", result->ai_family);
    ok(result->ai_addrlen >= sizeof(struct sockaddr_in6), "ai_addrlen == %d\n", (int)result->ai_addrlen);
    ok(result->ai_addr != NULL, "ai_addr == NULL\n");

    sockaddr6 = (SOCKADDR_IN6 *)result->ai_addr;
    ok(sockaddr6->sin6_family == AF_INET6, "ai_addr->sin6_family == %d\n", sockaddr6->sin6_family);
    ok(sockaddr6->sin6_port == 0, "ai_addr->sin6_port == %d\n", sockaddr6->sin6_port);

    memset(buffer, 0, sizeof(buffer));
    ret = p_inet_ntop(AF_INET6, &sockaddr6->sin6_addr, buffer, sizeof(buffer));
    ok(ret != NULL, "inet_ntop failed (%d)\n", WSAGetLastError());
    ok(!strcmp(buffer, expect), "ai_addr->sin6_addr == '%s' (expected '%s')\n", buffer, expect);
}

static void compare_addrinfo(ADDRINFO *a, ADDRINFO *b)
{
    for (; a && b ; a = a->ai_next, b = b->ai_next)
    {
        ok(a->ai_flags == b->ai_flags,
           "Wrong flags %d != %d\n", a->ai_flags, b->ai_flags);
        ok(a->ai_family == b->ai_family,
           "Wrong family %d != %d\n", a->ai_family, b->ai_family);
        ok(a->ai_socktype == b->ai_socktype,
           "Wrong socktype %d != %d\n", a->ai_socktype, b->ai_socktype);
        ok(a->ai_protocol == b->ai_protocol,
           "Wrong protocol %d != %d\n", a->ai_protocol, b->ai_protocol);
        ok(a->ai_addrlen == b->ai_addrlen,
           "Wrong addrlen %Iu != %Iu\n", a->ai_addrlen, b->ai_addrlen);
        ok(!memcmp(a->ai_addr, b->ai_addr, min(a->ai_addrlen, b->ai_addrlen)),
           "Wrong address data\n");
        if (a->ai_canonname && b->ai_canonname)
        {
            ok(!strcmp(a->ai_canonname, b->ai_canonname), "Wrong canonical name '%s' != '%s'\n",
               a->ai_canonname, b->ai_canonname);
        }
        else
            ok(!a->ai_canonname && !b->ai_canonname, "Expected both names absent (%p != %p)\n",
               a->ai_canonname, b->ai_canonname);
    }
    ok(!a && !b, "Expected both addresses null (%p != %p)\n", a, b);
}

static BOOL ipv6_found(ADDRINFOA *addr)
{
    ADDRINFOA *p;
    for (p = addr; p; p = p->ai_next)
    {
        if (p->ai_family == AF_INET6)
            return TRUE;
    }
    return FALSE;
}

static void test_getaddrinfo(void)
{
    int i, ret;
    ADDRINFOA *result, *result2, *p, hint;
    SOCKADDR_IN *sockaddr;
    CHAR name[256], *ip;
    DWORD size = sizeof(name);
    BOOL has_ipv6_getaddrinfo = TRUE;
    BOOL has_ipv6_addr;

    memset(&hint, 0, sizeof(ADDRINFOA));
    GetComputerNameExA( ComputerNamePhysicalDnsHostname, name, &size );

    result = (ADDRINFOA *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo(NULL, NULL, NULL, &result);
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo("", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo(NULL, "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");

    result2 = NULL;
    ret = getaddrinfo(NULL, "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "getaddrinfo failed\n");
    compare_addrinfo(result, result2);
    freeaddrinfo(result);
    freeaddrinfo(result2);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo("", "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    ok(result != NULL, "getaddrinfo failed\n");

    result2 = NULL;
    ret = getaddrinfo("", "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result2 != NULL, "getaddrinfo failed\n");
    compare_addrinfo(result, result2);
    freeaddrinfo(result);
    freeaddrinfo(result2);

    result = NULL;
    ret = getaddrinfo("localhost", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("localhost", "", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("localhost", "0", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("localhost", "80", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("localhost", NULL, &hint, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    freeaddrinfo(result);

    result = NULL;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo("localhost", "80", &hint, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == 0, "expected 0, got %d\n", WSAGetLastError());
    freeaddrinfo(result);

    hint.ai_flags = AI_NUMERICHOST;
    result = (void *)0xdeadbeef;
    ret = getaddrinfo("localhost", "80", &hint, &result);
    ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected WSAHOST_NOT_FOUND, got %d\n", WSAGetLastError());
    ok(!result, "result = %p\n", result);
    hint.ai_flags = 0;

    /* try to get information from the computer name, result is the same
     * as if requesting with an empty host name. */
    ret = getaddrinfo(name, NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = getaddrinfo("", NULL, NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    compare_addrinfo(result, result2);
    freeaddrinfo(result);
    freeaddrinfo(result2);

    ret = getaddrinfo(name, "", NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");

    ret = getaddrinfo("", "", NULL, &result2);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(result != NULL, "GetAddrInfoW failed\n");
    compare_addrinfo(result, result2);
    freeaddrinfo(result);
    freeaddrinfo(result2);

    result = (ADDRINFOA *)0xdeadbeef;
    WSASetLastError(0xdeadbeef);
    ret = getaddrinfo("nxdomain.codeweavers.com", NULL, NULL, &result);
    if (ret == 0)
    {
        skip("nxdomain returned success. Broken ISP redirects?\n");
        return;
    }
    ok(ret == WSAHOST_NOT_FOUND, "got %d expected WSAHOST_NOT_FOUND\n", ret);
    ok(WSAGetLastError() == WSAHOST_NOT_FOUND, "expected 11001, got %d\n", WSAGetLastError());
    ok(result == NULL, "got %p\n", result);

    /* Test IPv4 address conversion */
    result = NULL;
    ret = getaddrinfo("192.168.1.253", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", ret);
    ok(result->ai_family == AF_INET, "ai_family == %d\n", result->ai_family);
    ok(result->ai_addrlen >= sizeof(struct sockaddr_in), "ai_addrlen == %d\n", (int)result->ai_addrlen);
    ok(result->ai_addr != NULL, "ai_addr == NULL\n");
    sockaddr = (SOCKADDR_IN *)result->ai_addr;
    ok(sockaddr->sin_family == AF_INET, "ai_addr->sin_family == %d\n", sockaddr->sin_family);
    ok(sockaddr->sin_port == 0, "ai_addr->sin_port == %d\n", sockaddr->sin_port);

    ip = inet_ntoa(sockaddr->sin_addr);
    ok(strcmp(ip, "192.168.1.253") == 0, "sockaddr->ai_addr == '%s'\n", ip);
    freeaddrinfo(result);

    /* Test IPv4 address conversion with port */
    result = NULL;
    hint.ai_flags = AI_NUMERICHOST;
    ret = getaddrinfo("192.168.1.253:1024", NULL, &hint, &result);
    hint.ai_flags = 0;
    ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo returned unexpected result: %d\n", ret);
    ok(result == NULL, "expected NULL, got %p\n", result);

    /* Test IPv6 address conversion */
    result = NULL;
    SetLastError(0xdeadbeef);
    ret = getaddrinfo("2a00:2039:dead:beef:cafe::6666", NULL, NULL, &result);

    if (result != NULL)
    {
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "2a00:2039:dead:beef:cafe::6666");
        freeaddrinfo(result);

        /* Test IPv6 address conversion with brackets */
        result = NULL;
        ret = getaddrinfo("[beef::cafe]", NULL, NULL, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        freeaddrinfo(result);

        /* Test IPv6 address conversion with brackets and hints */
        memset(&hint, 0, sizeof(ADDRINFOA));
        hint.ai_flags = AI_NUMERICHOST;
        hint.ai_family = AF_INET6;
        result = NULL;
        ret = getaddrinfo("[beef::cafe]", NULL, &hint, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        freeaddrinfo(result);

        memset(&hint, 0, sizeof(ADDRINFOA));
        hint.ai_flags = AI_NUMERICHOST;
        hint.ai_family = AF_INET;
        result = NULL;
        ret = getaddrinfo("[beef::cafe]", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);

        /* Test IPv6 address conversion with brackets and port */
        result = NULL;
        ret = getaddrinfo("[beef::cafe]:10239", NULL, NULL, &result);
        ok(!ret, "getaddrinfo failed with %d\n", ret);
        verify_ipv6_addrinfo(result, "beef::cafe");
        freeaddrinfo(result);

        /* Test IPv6 address conversion with unmatched brackets */
        result = NULL;
        hint.ai_flags = AI_NUMERICHOST;
        ret = getaddrinfo("[beef::cafe", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);

        ret = getaddrinfo("beef::cafe]", NULL, &hint, &result);
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);
    }
    else
    {
        todo_wine
        ok(ret == WSAHOST_NOT_FOUND, "getaddrinfo failed with %d\n", ret);
        skip("getaddrinfo does not support IPV6\n");
        has_ipv6_getaddrinfo = FALSE;
    }

    hint.ai_flags = 0;

    for (i = 0; i < ARRAY_SIZE(hinttests); i++)
    {
        hint.ai_family = hinttests[i].family;
        hint.ai_socktype = hinttests[i].socktype;
        hint.ai_protocol = hinttests[i].protocol;

        result = NULL;
        SetLastError(0xdeadbeef);
        ret = getaddrinfo("localhost", NULL, &hint, &result);
        ok(ret == hinttests[i].error, "test %d: wrong ret %d\n", i, ret);
        if (!ret)
        {
            for (p = result; p; p = p->ai_next)
            {
                /* when AF_UNSPEC is used the return will be either AF_INET or AF_INET6 */
                if (hinttests[i].family == AF_UNSPEC)
                    ok(p->ai_family == AF_INET || p->ai_family == AF_INET6,
                       "test %d: expected AF_INET or AF_INET6, got %d\n",
                       i, p->ai_family);
                else
                    ok(p->ai_family == hinttests[i].family,
                       "test %d: expected family %d, got %d\n",
                       i, hinttests[i].family, p->ai_family);

                ok(p->ai_socktype == hinttests[i].socktype,
                   "test %d: expected type %d, got %d\n",
                   i, hinttests[i].socktype, p->ai_socktype);
                ok(p->ai_protocol == hinttests[i].protocol,
                   "test %d: expected protocol %d, got %d\n",
                   i, hinttests[i].protocol, p->ai_protocol);
            }
            freeaddrinfo(result);
        }
        else
        {
            ok(WSAGetLastError() == hinttests[i].error, "test %d: wrong error %d\n", i, WSAGetLastError());
        }
    }

    memset(&hint, 0, sizeof(hint));
    ret = getaddrinfo(NULL, "nonexistentservice", &hint, &result);
    ok(ret == WSATYPE_NOT_FOUND, "got %d\n", ret);

    result = NULL;
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags    = AI_ALL | AI_DNS_ONLY;
    hint.ai_family   = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;
    ret = getaddrinfo("winehq.org", NULL, &hint, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    ok(!result->ai_flags, "ai_flags == %08x\n", result->ai_flags);
    ok(result->ai_family == AF_INET, "ai_family == %d\n", result->ai_family);
    ok(result->ai_socktype == SOCK_STREAM, "ai_socktype == %d\n", result->ai_socktype);
    ok(result->ai_protocol == IPPROTO_TCP, "ai_protocol == %d\n", result->ai_protocol);
    ok(!result->ai_canonname, "ai_canonname == %s\n", result->ai_canonname);
    ok(result->ai_addrlen == sizeof(struct sockaddr_in), "ai_addrlen == %d\n", (int)result->ai_addrlen);
    ok(result->ai_addr != NULL, "ai_addr == NULL\n");
    sockaddr = (SOCKADDR_IN *)result->ai_addr;
    ok(sockaddr->sin_family == AF_INET, "ai_addr->sin_family == %d\n", sockaddr->sin_family);
    ok(sockaddr->sin_port == 0, "ai_addr->sin_port == %d\n", sockaddr->sin_port);
    freeaddrinfo(result);

    /* Check whether we have global IPv6 address */
    result = NULL;
    ret = getaddrinfo("", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    has_ipv6_addr = FALSE;
    for (p = result; p; p = p->ai_next)
    {
        if (p->ai_family == AF_INET6)
        {
            IN6_ADDR *a = &((SOCKADDR_IN6 *)p->ai_addr)->sin6_addr;
            if (!IN6_IS_ADDR_LINKLOCAL(a) && !IN6_IS_ADDR_LOOPBACK(a) && !IN6_IS_ADDR_UNSPECIFIED(a))
            {
                has_ipv6_addr = TRUE;
                break;
            }
        }
    }
    freeaddrinfo(result);

    result = NULL;
    ret = getaddrinfo("www.kernel.org", NULL, NULL, &result);
    ok(!ret, "getaddrinfo failed with %d\n", WSAGetLastError());
    if (!has_ipv6_addr)
    {
        todo_wine_if(has_ipv6_getaddrinfo)
        ok(!ipv6_found(result), "IPv6 address is returned.\n");
    }
    freeaddrinfo(result);

    for (i = 0; i < ARRAY_SIZE(hinttests); i++)
    {
        if (hinttests[i].family != AF_UNSPEC || hinttests[i].error) continue;
        winetest_push_context("Test %u", i);

        hint.ai_flags = 0;
        hint.ai_family = hinttests[i].family;
        hint.ai_socktype = hinttests[i].socktype;
        hint.ai_protocol = hinttests[i].protocol;

        result = NULL;
        ret = getaddrinfo("www.kernel.org", NULL, &hint, &result);
        ok(!ret, "Got unexpected ret %d\n", ret);
        if (!has_ipv6_addr)
            todo_wine ok(!ipv6_found(result), "IPv6 address is returned.\n");
        freeaddrinfo(result);

        hint.ai_family = AF_INET6;
        result = NULL;
        ret = getaddrinfo("www.kernel.org", NULL, &hint, &result);
        if (!has_ipv6_addr)
            todo_wine ok(ret == WSANO_DATA, "Got unexpected ret %d\n", ret);
        freeaddrinfo(result);

        winetest_pop_context();
    }
}

static void test_dns(void)
{
    struct hostent *h;
    union
    {
        char *chr;
        void *mem;
    } addr;
    char **ptr;
    int count;

    h = gethostbyname("");
    ok(h != NULL, "gethostbyname(\"\") failed with %d\n", h_errno);

    /* Use an address with valid alias names if possible */
    h = gethostbyname("source.winehq.org");
    if (!h)
    {
        skip("Can't test the hostent structure because gethostbyname failed\n");
        return;
    }

    /* The returned struct must be allocated in a very strict way. First we need to
     * count how many aliases there are because they must be located right after
     * the struct hostent size. Knowing the amount of aliases we know the exact
     * location of the first IP returned. Rule valid for >= XP, for older OS's
     * it's somewhat the opposite. */
    addr.mem = h + 1;
    if (h->h_addr_list == addr.mem) /* <= W2K */
    {
        win_skip("Skipping hostent tests since this OS is unsupported\n");
        return;
    }

    ok(h->h_aliases == addr.mem,
       "hostent->h_aliases should be in %p, it is in %p\n", addr.mem, h->h_aliases);

    for (ptr = h->h_aliases, count = 1; *ptr; ptr++) count++;
    addr.chr += sizeof(*ptr) * count;
    ok(h->h_addr_list == addr.mem,
       "hostent->h_addr_list should be in %p, it is in %p\n", addr.mem, h->h_addr_list);

    for (ptr = h->h_addr_list, count = 1; *ptr; ptr++) count++;

    addr.chr += sizeof(*ptr) * count;
    ok(h->h_addr_list[0] == addr.mem,
       "hostent->h_addr_list[0] should be in %p, it is in %p\n", addr.mem, h->h_addr_list[0]);
}

static void test_gethostbyname(void)
{
    struct hostent *he;
    struct in_addr **addr_list;
    char name[256], first_ip[16];
    int ret, i, count;
    MIB_IPFORWARDTABLE *routes = NULL;
    IP_ADAPTER_INFO *adapters = NULL, *k;
    DWORD adap_size = 0, route_size = 0;
    BOOL found_default = FALSE;
    BOOL local_ip = FALSE;

    ret = gethostname(name, sizeof(name));
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());

    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());
    addr_list = (struct in_addr **)he->h_addr_list;
    strcpy(first_ip, inet_ntoa(*addr_list[0]));

    if (winetest_debug > 1) trace("List of local IPs:\n");
    for (count = 0; addr_list[count] != NULL; count++)
    {
        char *ip = inet_ntoa(*addr_list[count]);
        if (!strcmp(ip, "127.0.0.1"))
            local_ip = TRUE;
        if (winetest_debug > 1) trace("%s\n", ip);
    }

    if (local_ip)
    {
        ok(count == 1, "expected 127.0.0.1 to be the only IP returned\n");
        skip("Only the loopback address is present, skipping tests\n");
        return;
    }

    ret = GetAdaptersInfo(NULL, &adap_size);
    ok(ret  == ERROR_BUFFER_OVERFLOW, "GetAdaptersInfo failed with a different error: %d\n", ret);
    ret = GetIpForwardTable(NULL, &route_size, FALSE);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "GetIpForwardTable failed with a different error: %d\n", ret);

    adapters = malloc(adap_size);
    routes = malloc(route_size);

    ret = GetAdaptersInfo(adapters, &adap_size);
    ok(ret  == NO_ERROR, "GetAdaptersInfo failed, error: %d\n", ret);
    ret = GetIpForwardTable(routes, &route_size, FALSE);
    ok(ret == NO_ERROR, "GetIpForwardTable failed, error: %d\n", ret);

    /* This test only has meaning if there is more than one IP configured */
    if (adapters->Next == NULL && count == 1)
    {
        skip("Only one IP is present, skipping tests\n");
        goto cleanup;
    }

    for (i = 0; !found_default && i < routes->dwNumEntries; i++)
    {
        /* default route (ip 0.0.0.0) ? */
        if (routes->table[i].dwForwardDest) continue;

        for (k = adapters; k != NULL; k = k->Next)
        {
            char *ip;

            if (k->Index != routes->table[i].dwForwardIfIndex) continue;

            /* the first IP returned from gethostbyname must be a default route */
            ip = k->IpAddressList.IpAddress.String;
            if (!strcmp(first_ip, ip))
            {
                found_default = TRUE;
                break;
            }
        }
    }
    ok(found_default, "failed to find the first IP from gethostbyname!\n");

cleanup:
    free(adapters);
    free(routes);
}

static void test_gethostbyname_hack(void)
{
    struct hostent *he;
    char name[256];
    static BYTE loopback[] = {127, 0, 0, 1};
    static BYTE magic_loopback[] = {127, 12, 34, 56};
    int ret;

    ret = gethostname(name, 256);
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());

    he = gethostbyname("localhost");
    ok(he != NULL, "gethostbyname(\"localhost\") failed: %d\n", h_errno);
    if (he->h_length != 4)
    {
        skip("h_length is %d, not IPv4, skipping test.\n", he->h_length);
        return;
    }
    ok(!memcmp(he->h_addr_list[0], loopback, he->h_length),
       "gethostbyname(\"localhost\") returned %u.%u.%u.%u\n",
       he->h_addr_list[0][0], he->h_addr_list[0][1], he->h_addr_list[0][2],
       he->h_addr_list[0][3]);

    if (!strcmp(name, "localhost"))
    {
        skip("hostname seems to be \"localhost\", skipping test.\n");
        return;
    }

    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, h_errno);
    if (he->h_length != 4)
    {
        skip("h_length is %d, not IPv4, skipping test.\n", he->h_length);
        return;
    }

    if (he->h_addr_list[0][0] == 127)
    {
        ok(memcmp(he->h_addr_list[0], magic_loopback, he->h_length) == 0,
           "gethostbyname(\"%s\") returned %u.%u.%u.%u not 127.12.34.56\n",
           name, he->h_addr_list[0][0], he->h_addr_list[0][1],
           he->h_addr_list[0][2], he->h_addr_list[0][3]);
    }

    gethostbyname("nonexistent.winehq.org");
    /* Don't check for the return value, as some braindead ISPs will kindly
     * resolve nonexistent host names to addresses of the ISP's spam pages. */
}

static void test_gethostname(void)
{
    struct hostent *he;
    char name[256];
    int ret, len;

    WSASetLastError(0xdeadbeef);
    ret = gethostname(NULL, 256);
    ok(ret == -1, "gethostname() returned %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "gethostname with null buffer "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    ret = gethostname(name, sizeof(name));
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());
    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());

    len = strlen(name);
    WSASetLastError(0xdeadbeef);
    strcpy(name, "deadbeef");
    ret = gethostname(name, len);
    ok(ret == -1, "gethostname() returned %d\n", ret);
    ok(!strcmp(name, "deadbeef"), "name changed unexpected!\n");
    ok(WSAGetLastError() == WSAEFAULT, "gethostname with insufficient length "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    len++;
    ret = gethostname(name, len);
    ok(ret == 0, "gethostname() call failed: %d\n", WSAGetLastError());
    he = gethostbyname(name);
    ok(he != NULL, "gethostbyname(\"%s\") failed: %d\n", name, WSAGetLastError());
}

static void test_GetHostNameW(void)
{
    WCHAR name[256];
    int ret, len;

    if (!pGetHostNameW)
    {
        win_skip("GetHostNameW() not present\n");
        return;
    }

    WSASetLastError(0xdeadbeef);
    ret = pGetHostNameW(NULL, 256);
    ok(ret == -1, "GetHostNameW() returned %d\n", ret);
    ok(WSAGetLastError() == WSAEFAULT, "GetHostNameW with null buffer "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    ret = pGetHostNameW(name, sizeof(name));
    ok(ret == 0, "GetHostNameW() call failed: %d\n", WSAGetLastError());

    len = wcslen(name);
    WSASetLastError(0xdeadbeef);
    wcscpy(name, L"deadbeef");
    ret = pGetHostNameW(name, len);
    ok(ret == -1, "GetHostNameW() returned %d\n", ret);
    ok(!wcscmp(name, L"deadbeef"), "name changed unexpected!\n");
    ok(WSAGetLastError() == WSAEFAULT, "GetHostNameW with insufficient length "
            "failed with %d, expected %d\n", WSAGetLastError(), WSAEFAULT);

    len++;
    ret = pGetHostNameW(name, len);
    ok(ret == 0, "GetHostNameW() call failed: %d\n", WSAGetLastError());
}

static void test_WSAEnumNameSpaceProvidersA(void)
{
    WSANAMESPACE_INFOA *name = NULL;
    DWORD ret, error, len = 0;

    SetLastError(0xdeadbeef);
    ret = WSAEnumNameSpaceProvidersA(&len, name);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %lu\n", ret);
    todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %lu\n", error);

    /* Invalid parameter tests */
    SetLastError(0xdeadbeef);
    ret = WSAEnumNameSpaceProvidersA(NULL, name);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %lu\n", ret);
    todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = WSAEnumNameSpaceProvidersA(NULL, NULL);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %lu\n", ret);
    todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = WSAEnumNameSpaceProvidersA(&len, NULL);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %lu\n", ret);
    todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %lu\n", error);

    name = malloc(len);

    ret = WSAEnumNameSpaceProvidersA(&len, name);
    todo_wine
    ok(ret > 0, "Expected more than zero name space providers\n");

    free(name);
}

static void test_WSAEnumNameSpaceProvidersW(void)
{
    WSANAMESPACE_INFOW *name = NULL;
    DWORD ret, error, len = 0, i;

    SetLastError(0xdeadbeef);
    ret = WSAEnumNameSpaceProvidersW(&len, name);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %lu\n", ret);
    todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %lu\n", error);

    /* Invalid parameter tests */
    SetLastError(0xdeadbeef);
    ret = WSAEnumNameSpaceProvidersW(NULL, name);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %lu\n", ret);
    todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = WSAEnumNameSpaceProvidersW(NULL, NULL);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %lu\n", ret);
    todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = WSAEnumNameSpaceProvidersW(&len, NULL);
    error = WSAGetLastError();
    todo_wine
    ok(ret == SOCKET_ERROR, "Expected failure, got %lu\n", ret);
    todo_wine
    ok(error == WSAEFAULT, "Expected 10014, got %lu\n", error);

    name = malloc(len);

    ret = WSAEnumNameSpaceProvidersW(&len, name);
    todo_wine
    ok(ret > 0, "Expected more than zero name space providers\n");

    if (winetest_debug > 1)
    {
        for (i = 0; i < ret; i++)
        {
            trace("Name space Identifier (%p): %s\n", name[i].lpszIdentifier,
                   wine_dbgstr_w(name[i].lpszIdentifier));
            switch (name[i].dwNameSpace)
            {
                case NS_DNS:
                    trace("\tName space ID: NS_DNS (%lu)\n", name[i].dwNameSpace);
                    break;
                case NS_NLA:
                    trace("\tName space ID: NS_NLA (%lu)\n", name[i].dwNameSpace);
                    break;
                default:
                    trace("\tName space ID: Unknown (%lu)\n", name[i].dwNameSpace);
                    break;
            }
            trace("\tActive:  %d\n", name[i].fActive);
            trace("\tVersion: %ld\n", name[i].dwVersion);
        }
    }

    free(name);
}

static void test_WSCGetApplicationCategory(void)
{
    int ret;
    int errcode;
    DWORD category;

    if (!pWSCGetApplicationCategory)
    {
        win_skip("WSCGetApplicationCategory is not available.\n");
        return;
    }

    errcode = 0xdeadbeef;
    ret = pWSCGetApplicationCategory(NULL, 0, NULL, 0, NULL, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    ok(errcode == WSAEINVAL, "got %d, expected WSAEINVAL\n", errcode);

    errcode = 0xdeadbeef;
    ret = pWSCGetApplicationCategory(L"", 0, NULL, 0, NULL, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    todo_wine ok(errcode == WSAEINVAL, "got %d, expected WSAEINVAL\n", errcode);

    errcode = 0xdeadbeef;
    ret = pWSCGetApplicationCategory(L"", 0, L"", 0, NULL, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    todo_wine ok(errcode == WSAEINVAL, "got %d, expected WSAEINVAL\n", errcode);

    errcode = 0xdeadbeef;
    ret = pWSCGetApplicationCategory(L"", 0, NULL, 0, &category, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    todo_wine ok(errcode == WSAEINVAL, "got %d, expected WSAEINVAL\n", errcode);

    errcode = 0xdeadbeef;
    ret = pWSCGetApplicationCategory(L"", 0, L"", 0, &category, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    todo_wine ok(errcode == WSAEINVAL, "got %d, expected WSAEINVAL\n", errcode);
}

static void test_WSCGetProviderInfo(void)
{
    int ret;
    int errcode;
    GUID provider = {0};
    BYTE info[1];
    size_t len = 0;

    if (!pWSCGetProviderInfo)
    {
        win_skip("WSCGetProviderInfo is not available.\n");
        return;
    }

    ret = pWSCGetProviderInfo(NULL, -1, NULL, NULL, 0, NULL);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);

    errcode = 0xdeadbeef;
    ret = pWSCGetProviderInfo(NULL, ProviderInfoLspCategories, info, &len, 0, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    ok(errcode == WSAEFAULT, "got %d, expected WSAEFAULT\n", errcode);

    errcode = 0xdeadbeef;
    ret = pWSCGetProviderInfo(&provider, -1, info, &len, 0, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    ok(errcode == WSANO_RECOVERY, "got %d, expected WSANO_RECOVERY\n", errcode);

    errcode = 0xdeadbeef;
    ret = pWSCGetProviderInfo(&provider, ProviderInfoLspCategories, NULL, &len, 0, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    ok(errcode == WSANO_RECOVERY, "got %d, expected WSANO_RECOVERY\n", errcode);

    errcode = 0xdeadbeef;
    ret = pWSCGetProviderInfo(&provider, ProviderInfoLspCategories, info, NULL, 0, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    ok(errcode == WSANO_RECOVERY, "got %d, expected WSANO_RECOVERY\n", errcode);

    errcode = 0xdeadbeef;
    ret = pWSCGetProviderInfo(&provider, ProviderInfoLspCategories, info, &len, 0, &errcode);
    ok(ret == SOCKET_ERROR, "got %d, expected SOCKET_ERROR\n", ret);
    ok(errcode == WSANO_RECOVERY, "got %d, expected WSANO_RECOVERY\n", errcode);
}

static void test_WSCGetProviderPath(void)
{
    GUID provider = {0};
    WCHAR buffer[256];
    INT ret, err, len;

    ret = WSCGetProviderPath(NULL, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);

    ret = WSCGetProviderPath(&provider, NULL, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);

    ret = WSCGetProviderPath(NULL, buffer, NULL, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);

    len = -1;
    ret = WSCGetProviderPath(NULL, NULL, &len, NULL);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(len == -1, "Got unexpected len %d.\n", len);

    err = 0;
    ret = WSCGetProviderPath(NULL, NULL, NULL, &err);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(err == WSAEFAULT, "Got unexpected error %d.\n", err);

    err = 0;
    ret = WSCGetProviderPath(&provider, NULL, NULL, &err);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(err == WSAEFAULT, "Got unexpected error %d.\n", err);

    err = 0;
    len = -1;
    ret = WSCGetProviderPath(&provider, NULL, &len, &err);
    ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    ok(err == WSAEINVAL, "Got unexpected error %d.\n", err);
    ok(len == -1, "Got unexpected len %d.\n", len);

    err = 0;
    len = 256;
    ret = WSCGetProviderPath(&provider, NULL, &len, &err);
    todo_wine ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    todo_wine ok(err == WSAEINVAL, "Got unexpected error %d.\n", err);
    ok(len == 256, "Got unexpected len %d.\n", len);

    /* Valid pointers and length but invalid GUID */
    err = 0;
    len = 256;
    ret = WSCGetProviderPath(&provider, buffer, &len, &err);
    todo_wine ok(ret == SOCKET_ERROR, "Got unexpected ret %d.\n", ret);
    todo_wine ok(err == WSAEINVAL, "Got unexpected error %d.\n", err);
    ok(len == 256, "Got unexpected len %d.\n", len);
}

static void test_startup(void)
{
    unsigned int i;
    WSADATA data;
    int ret;

    static const struct
    {
        WORD version;
        WORD ret_version;
    }
    tests[] =
    {
        {MAKEWORD(0, 0), MAKEWORD(2, 2)},
        {MAKEWORD(0, 1), MAKEWORD(2, 2)},
        {MAKEWORD(1, 0), MAKEWORD(1, 0)},
        {MAKEWORD(1, 1), MAKEWORD(1, 1)},
        {MAKEWORD(1, 2), MAKEWORD(1, 1)},
        {MAKEWORD(1, 0xff), MAKEWORD(1, 1)},
        {MAKEWORD(2, 0), MAKEWORD(2, 0)},
        {MAKEWORD(2, 1), MAKEWORD(2, 1)},
        {MAKEWORD(2, 2), MAKEWORD(2, 2)},
        {MAKEWORD(2, 3), MAKEWORD(2, 2)},
        {MAKEWORD(2, 0xff), MAKEWORD(2, 2)},
        {MAKEWORD(3, 0), MAKEWORD(2, 2)},
        {MAKEWORD(0xff, 0), MAKEWORD(2, 2)},
    };

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Version %#x", tests[i].version);

        memset(&data, 0xcc, sizeof(data));
        data.lpVendorInfo = (void *)0xdeadbeef;
        ret = WSAStartup(tests[i].version, &data);
        ok(ret == (LOBYTE(tests[i].version) ? 0 : WSAVERNOTSUPPORTED), "got %d\n", ret);
        ok(data.wVersion == tests[i].ret_version, "got version %#x\n", data.wVersion);
        if (!ret)
        {
            ret = WSAStartup(tests[i].version, &data);
            ok(!ret, "got %d\n", ret);

            WSASetLastError(0xdeadbeef);
            ret = WSACleanup();
            ok(!ret, "got %d\n", ret);
            todo_wine ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());

            WSASetLastError(0xdeadbeef);
            ret = WSACleanup();
            ok(!ret, "got %d\n", ret);
            todo_wine ok(!WSAGetLastError(), "got error %u\n", WSAGetLastError());
        }
        ok(data.lpVendorInfo == (void *)0xdeadbeef, "got vendor info %p\n", data.lpVendorInfo);
        ok(data.wHighVersion == 0x202, "got maximum version %#x\n", data.wHighVersion);
        ok(!strcmp(data.szDescription, "WinSock 2.0"), "got description %s\n", debugstr_a(data.szDescription));
        ok(!strcmp(data.szSystemStatus, "Running"), "got status %s\n", debugstr_a(data.szSystemStatus));
        ok(data.iMaxSockets == (LOBYTE(tests[i].version) == 1 ? 32767 : 0), "got maximum sockets %u\n", data.iMaxSockets);
        ok(data.iMaxUdpDg == (LOBYTE(tests[i].version) == 1 ? 65467 : 0), "got maximum datagram size %u\n", data.iMaxUdpDg);

        WSASetLastError(0xdeadbeef);
        ret = WSACleanup();
        ok(ret == -1, "got %d\n", ret);
        ok(WSAGetLastError() == WSANOTINITIALISED, "got error %u\n", WSAGetLastError());

        ret = WSAStartup(tests[i].version, NULL);
        ok(ret == (LOBYTE(tests[i].version) ? WSAEFAULT : WSAVERNOTSUPPORTED), "got %d\n", ret);

        winetest_pop_context();
    }
}

START_TEST( protocol )
{
    WSADATA data;
    int ret;

    pFreeAddrInfoExW = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "FreeAddrInfoExW");
    pGetAddrInfoExOverlappedResult = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "GetAddrInfoExOverlappedResult");
    pGetAddrInfoExW = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "GetAddrInfoExW");
    pGetHostNameW = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "GetHostNameW");
    p_inet_ntop = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "inet_ntop");
    pInetNtopW = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "InetNtopW");
    p_inet_pton = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "inet_pton");
    pInetPtonW = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "InetPtonW");
    pWSCGetApplicationCategory = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "WSCGetApplicationCategory");
    pWSCGetProviderInfo = (void *)GetProcAddress(GetModuleHandleA("ws2_32"), "WSCGetProviderInfo");

    ret = WSAStartup(0x202, &data);
    ok(!ret, "got %d\n", ret);

    test_WSAEnumProtocolsA();
    test_WSAEnumProtocolsW();
    test_getprotobyname();
    test_getprotobynumber();

    test_getservbyname();
    test_WSALookupService();

    test_inet_ntoa();
    test_inet_addr();
    test_inet_pton();
    test_addr_to_print();
    test_WSAAddressToString();
    test_WSAStringToAddress();

    test_GetAddrInfoW();
    test_GetAddrInfoExW();
    test_getaddrinfo();

    test_dns();
    test_gethostbyname();
    test_gethostbyname_hack();
    test_gethostname();
    test_GetHostNameW();

    test_WSAEnumNameSpaceProvidersA();
    test_WSAEnumNameSpaceProvidersW();
    test_WSCGetApplicationCategory();
    test_WSCGetProviderInfo();
    test_WSCGetProviderPath();

    WSACleanup();

    /* These tests are finnicky. If WSAStartup() is ever called with a
     * version below 2.2, it causes getaddrinfo() to behave differently. */

    test_startup();

    /* And if WSAAsyncGetServBy*() is ever called, it somehow causes
     * WSAStartup() to succeed with 0.1 instead of failing. */

    ret = WSAStartup(0x202, &data);
    ok(!ret, "got %d\n", ret);

    test_WSAAsyncGetServByPort();
    test_WSAAsyncGetServByName();
}
