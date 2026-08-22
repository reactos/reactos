/* Internet TCP/IP and IPX Connection For DirectPlay
 *
 * Copyright 2008 Ismael Barros Barros
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

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "dpwsockx_dll.h"
#include "wine/debug.h"
#include "dplay.h"
#include "wine/dplaysp.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

#define DPWS_PORT           47624
#define DPWS_START_TCP_PORT 2300
#define DPWS_END_TCP_PORT   2350
#define DPWS_START_UDP_PORT 2350
#define DPWS_END_UDP_PORT   2400

static void DPWS_MessageBodyReceiveCompleted( DPWS_IN_CONNECTION *connection );

static int DPWS_CompareConnections( const void *key, const struct rb_entry *entry )
{
    DPWS_OUT_CONNECTION *connection = RB_ENTRY_VALUE( entry, DPWS_OUT_CONNECTION, entry );
    DPWS_CONNECTION_KEY *connection_key = (DPWS_CONNECTION_KEY *)key;

    if ( connection_key->addr.sin_port < connection->key.addr.sin_port )
        return -1;
    if ( connection_key->addr.sin_port > connection->key.addr.sin_port )
        return 1;

    if ( connection_key->addr.sin_addr.s_addr < connection->key.addr.sin_addr.s_addr )
        return -1;
    if ( connection_key->addr.sin_addr.s_addr > connection->key.addr.sin_addr.s_addr )
        return 1;

    if ( connection_key->guaranteed < connection->key.guaranteed )
        return -1;
    if ( connection_key->guaranteed > connection->key.guaranteed )
        return 1;

    return 0;
}

static int DPWS_CompareConnectionRefs( const void *key, const struct rb_entry *entry )
{
    DPWS_CONNECTION_REF *connectionRef = RB_ENTRY_VALUE( entry, DPWS_CONNECTION_REF, entry );

    return DPWS_CompareConnections( key, &connectionRef->connection->entry );
}

static HRESULT DPWS_BindToFreePort( SOCKET sock, SOCKADDR_IN *addr, int startPort, int endPort )
{
    int port;

    memset( addr, 0, sizeof( *addr ) );
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl( INADDR_ANY );

    for ( port = startPort; port < endPort; ++port )
    {
        addr->sin_port = htons( port );

        if ( SOCKET_ERROR != bind( sock, (SOCKADDR *) addr, sizeof( *addr ) ) )
            return DP_OK;

        if ( WSAGetLastError() == WSAEADDRINUSE )
            continue;

        ERR( "bind() failed\n" );
        return DPERR_UNAVAILABLE;
    }

    ERR( "no free ports\n" );
    return DPERR_UNAVAILABLE;
}

static void DPWS_RemoveInConnection( DPWS_IN_CONNECTION *connection )
{
    list_remove( &connection->entry );
    closesocket( connection->tcpSock );
    free( connection->buffer );
    free( connection );
}

static void WINAPI DPWS_TcpReceiveCompleted( DWORD error, DWORD transferred,
                                             WSAOVERLAPPED *overlapped, DWORD flags )
{
    DPWS_IN_CONNECTION *connection = (DPWS_IN_CONNECTION *)overlapped->hEvent;

    if ( error != ERROR_SUCCESS )
    {
        ERR( "WSARecv() failed\n" );
        DPWS_RemoveInConnection( connection );
        return;
    }

    if ( !transferred )
    {
        DPWS_RemoveInConnection( connection );
        return;
    }

    if ( transferred < connection->wsaBuffer.len )
    {
        connection->wsaBuffer.len -= transferred;
        connection->wsaBuffer.buf += transferred;

        if ( SOCKET_ERROR == WSARecv( connection->tcpSock, &connection->wsaBuffer, 1, &transferred,
                                      &flags, &connection->overlapped, DPWS_TcpReceiveCompleted ) )
        {
            if ( WSAGetLastError() != WSA_IO_PENDING )
            {
                ERR( "WSARecv() failed\n" );
                DPWS_RemoveInConnection( connection );
                return;
            }
        }
        return;
    }

    connection->completionRoutine( connection );
}

static HRESULT DPWS_TcpReceive( DPWS_IN_CONNECTION *connection, void *data, DWORD size,
                                DPWS_COMPLETION_ROUTINE *completionRoutine )
{
    DWORD transferred;
    DWORD flags = 0;

    connection->wsaBuffer.len = size;
    connection->wsaBuffer.buf = data;

    connection->completionRoutine = completionRoutine;

    if ( SOCKET_ERROR == WSARecv( connection->tcpSock, &connection->wsaBuffer, 1, &transferred,
                                  &flags, &connection->overlapped, DPWS_TcpReceiveCompleted ) )
    {
        if ( WSAGetLastError() != WSA_IO_PENDING )
        {
            ERR( "WSARecv() failed\n" );
            return DPERR_UNAVAILABLE;
        }
    }

    return DP_OK;
}

static void DPWS_HeaderReceiveCompleted( DPWS_IN_CONNECTION *connection )
{
    int messageBodySize;
    int messageSize;

    messageSize = DPSP_MSG_SIZE( connection->header.mixed );
    if ( messageSize < sizeof( DPSP_MSG_HEADER ))
    {
        ERR( "message is too short: %d\n", messageSize );
        DPWS_RemoveInConnection( connection );
        return;
    }
    messageBodySize = messageSize - sizeof( DPSP_MSG_HEADER );

    if ( messageBodySize > DPWS_GUARANTEED_MAXBUFFERSIZE )
    {
        ERR( "message is too long: %d\n", messageSize );
        DPWS_RemoveInConnection( connection );
        return;
    }

    if ( connection->bufferSize < messageBodySize )
    {
        int newSize = max( connection->bufferSize * 2, messageBodySize );
        char *newBuffer = malloc( newSize );
        if ( !newBuffer )
        {
            ERR( "failed to allocate required memory.\n" );
            DPWS_RemoveInConnection( connection );
            return;
        }
        free( connection->buffer );
        connection->buffer = newBuffer;
        connection->bufferSize = newSize;
    }

    if ( FAILED( DPWS_TcpReceive( connection, connection->buffer, messageBodySize,
                                  DPWS_MessageBodyReceiveCompleted ) ) )
    {
        DPWS_RemoveInConnection( connection );
        return;
    }
}

static void DPWS_MessageBodyReceiveCompleted( DPWS_IN_CONNECTION *connection )
{
    int messageBodySize;
    int messageSize;

    if ( connection->header.SockAddr.sin_addr.s_addr == INADDR_ANY )
        connection->header.SockAddr.sin_addr = connection->addr.sin_addr;

    messageSize = DPSP_MSG_SIZE( connection->header.mixed );
    messageBodySize = messageSize - sizeof( DPSP_MSG_HEADER );

    IDirectPlaySP_HandleMessage( connection->sp, connection->buffer, messageBodySize,
                                 &connection->header );

    if ( FAILED( DPWS_TcpReceive( connection, &connection->header, sizeof( DPSP_MSG_HEADER ),
                                  DPWS_HeaderReceiveCompleted ) ) )
    {
        DPWS_RemoveInConnection( connection );
        return;
    }
}

static void WINAPI DPWS_UdpReceiveCompleted( DWORD error, DWORD transferred,
                                             WSAOVERLAPPED *overlapped, DWORD flags )
{
    DPWS_DATA *dpwsData = (DPWS_DATA *)overlapped->hEvent;
    DPSP_MSG_HEADER *header;
    int messageBodySize;
    char *messageBody;

    if ( error != ERROR_SUCCESS )
    {
        ERR( "WSARecv() failed\n" );
        return;
    }

    messageBody = dpwsData->buffer;
    messageBodySize = transferred;
    header = NULL;

    if ( transferred >= sizeof( DPSP_MSG_HEADER ) + sizeof( DWORD ) )
    {
        DWORD signature = *(DWORD *)&dpwsData->buffer[ sizeof( DPSP_MSG_HEADER ) ];
        if ( signature == 0x79616c70 )
        {
            int messageSize;

            header = (DPSP_MSG_HEADER *)dpwsData->buffer;
            messageSize = DPSP_MSG_SIZE( header->mixed );

            if ( messageSize < sizeof( DPSP_MSG_HEADER ))
            {
                ERR( "message is too short: %d\n", messageSize );
                return;
            }
            if ( messageSize > transferred )
            {
                ERR( "truncated message\n" );
                return;
            }

            messageBody = dpwsData->buffer + sizeof( DPSP_MSG_HEADER );
            messageBodySize = messageSize - sizeof( DPSP_MSG_HEADER );
        }
    }

    IDirectPlaySP_HandleMessage( dpwsData->lpISP, messageBody, messageBodySize, header );

    if ( SOCKET_ERROR == WSARecv( dpwsData->udpSock, &dpwsData->wsaBuffer, 1, &transferred, &flags,
                                  &dpwsData->overlapped, DPWS_UdpReceiveCompleted ) )
    {
        if ( WSAGetLastError() != WSA_IO_PENDING )
        {
            ERR( "WSARecv() failed\n" );
            return;
        }
    }
}

static DWORD WINAPI DPWS_ThreadProc( void *param )
{
    DPWS_DATA *dpwsData = (DPWS_DATA *)param;
    DWORD transferred;
    DWORD flags = 0;

    SetThreadDescription( GetCurrentThread(), L"dpwsockx" );

    if ( SOCKET_ERROR == WSARecv( dpwsData->udpSock, &dpwsData->wsaBuffer, 1, &transferred, &flags,
                                  &dpwsData->overlapped, DPWS_UdpReceiveCompleted ) )
    {
        if ( WSAGetLastError() != WSA_IO_PENDING )
        {
            ERR( "WSARecv() failed\n" );
            return 0;
        }
    }

    for ( ;; )
    {
        DPWS_IN_CONNECTION *connection;
        WSANETWORKEVENTS networkEvents;
        WSAEVENT events[2];
        SOCKADDR_IN addr;
        DWORD waitResult;
        int addrSize;
        SOCKET sock;

        events[ 0 ] = dpwsData->stopEvent;
        events[ 1 ] = dpwsData->acceptEvent;
        waitResult = WSAWaitForMultipleEvents( ARRAYSIZE( events ), events, FALSE, WSA_INFINITE,
                                               TRUE );
        if ( waitResult == WSA_WAIT_FAILED )
        {
            ERR( "WSAWaitForMultipleEvents() failed\n" );
            break;
        }
        if ( waitResult == WSA_WAIT_IO_COMPLETION )
            continue;
        if ( waitResult == WSA_WAIT_EVENT_0 )
            break;

        if ( SOCKET_ERROR == WSAEnumNetworkEvents( dpwsData->tcpSock, dpwsData->acceptEvent,
                                                   &networkEvents ) )
        {
            ERR( "WSAEnumNetworkEvents() failed\n" );
            break;
        }

        addrSize = sizeof( addr );
        sock = accept( dpwsData->tcpSock, (SOCKADDR *)&addr, &addrSize );
        if ( sock == INVALID_SOCKET )
        {
            if ( WSAGetLastError() == WSAEWOULDBLOCK )
                continue;
            ERR( "accept() failed\n" );
            break;
        }

        connection = calloc( 1, sizeof( DPWS_IN_CONNECTION ) );
        if ( !connection )
        {
            ERR( "failed to allocate required memory.\n" );
            closesocket( sock );
            continue;
        }

        connection->addr = addr;
        connection->tcpSock = sock;
        connection->overlapped.hEvent = connection;
        connection->sp = dpwsData->lpISP;

        list_add_tail( &dpwsData->inConnections, &connection->entry );

        if ( FAILED( DPWS_TcpReceive( connection, &connection->header, sizeof( DPSP_MSG_HEADER ),
                                      DPWS_HeaderReceiveCompleted ) ) )
        {
            DPWS_RemoveInConnection( connection );
            continue;
        }
    }

    return 0;
}

static HRESULT DPWS_Start( DPWS_DATA *dpwsData )
{
    HRESULT hr;

    if ( dpwsData->started )
        return S_OK;

    dpwsData->tcpSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( dpwsData->tcpSock == INVALID_SOCKET )
    {
        ERR( "socket() failed\n" );
        return DPERR_UNAVAILABLE;
    }

    hr = DPWS_BindToFreePort( dpwsData->tcpSock, &dpwsData->tcpAddr, DPWS_START_TCP_PORT,
                              DPWS_END_TCP_PORT );
    if ( FAILED( hr ) )
    {
        closesocket( dpwsData->tcpSock );
        return hr;
    }

    if ( SOCKET_ERROR == listen( dpwsData->tcpSock, SOMAXCONN ) )
    {
        ERR( "listen() failed\n" );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    dpwsData->acceptEvent = WSACreateEvent();
    if ( !dpwsData->acceptEvent )
    {
        ERR( "WSACreateEvent() failed\n" );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    if ( SOCKET_ERROR == WSAEventSelect( dpwsData->tcpSock, dpwsData->acceptEvent, FD_ACCEPT ) )
    {
        ERR( "WSAEventSelect() failed\n" );
        WSACloseEvent( dpwsData->acceptEvent );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    list_init( &dpwsData->inConnections );

    dpwsData->udpSock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( dpwsData->udpSock == INVALID_SOCKET )
    {
        ERR( "socket() failed\n" );
        WSACloseEvent( dpwsData->acceptEvent );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    hr = DPWS_BindToFreePort( dpwsData->udpSock, &dpwsData->udpAddr, DPWS_START_UDP_PORT,
                              DPWS_END_UDP_PORT );
    if ( FAILED( hr ) )
    {
        closesocket( dpwsData->udpSock );
        WSACloseEvent( dpwsData->acceptEvent );
        closesocket( dpwsData->tcpSock );
        return hr;
    }

    dpwsData->overlapped.hEvent = (HANDLE)dpwsData;
    dpwsData->wsaBuffer.len = sizeof( dpwsData->buffer );
    dpwsData->wsaBuffer.buf = dpwsData->buffer;

    rb_init( &dpwsData->connections, DPWS_CompareConnections );

    dpwsData->stopEvent = WSACreateEvent();
    if ( !dpwsData->stopEvent )
    {
        ERR( "WSACreateEvent() failed\n" );
        closesocket( dpwsData->udpSock );
        WSACloseEvent( dpwsData->acceptEvent );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    InitializeCriticalSection( &dpwsData->sendCs );

    dpwsData->thread = CreateThread( NULL, 0, DPWS_ThreadProc, dpwsData, 0, NULL );
    if ( !dpwsData->thread )
    {
        ERR( "CreateThread() failed\n" );
        DeleteCriticalSection( &dpwsData->sendCs );
        WSACloseEvent( dpwsData->stopEvent );
        closesocket( dpwsData->udpSock );
        WSACloseEvent( dpwsData->acceptEvent );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    dpwsData->started = TRUE;

    return S_OK;
}

static void DPWS_Stop( DPWS_DATA *dpwsData )
{
    DPWS_OUT_CONNECTION *outConnection2;
    DPWS_OUT_CONNECTION *outConnection;
    DPWS_IN_CONNECTION *inConnection2;
    DPWS_IN_CONNECTION *inConnection;

    if ( !dpwsData->started )
        return;

    dpwsData->started = FALSE;

    WSASetEvent( dpwsData->stopEvent );
    WaitForSingleObject( dpwsData->thread, INFINITE );
    CloseHandle( dpwsData->thread );

    RB_FOR_EACH_ENTRY_DESTRUCTOR( outConnection, outConnection2, &dpwsData->connections,
                                  DPWS_OUT_CONNECTION, entry )
    {
        rb_remove( &dpwsData->connections, &outConnection->entry );
        if ( outConnection->tcpSock != INVALID_SOCKET )
            closesocket( outConnection->tcpSock );
        free( outConnection );
    }

    LIST_FOR_EACH_ENTRY_SAFE( inConnection, inConnection2, &dpwsData->inConnections,
                              DPWS_IN_CONNECTION, entry )
        DPWS_RemoveInConnection( inConnection );

    DeleteCriticalSection( &dpwsData->sendCs );
    WSACloseEvent( dpwsData->stopEvent );
    closesocket( dpwsData->udpSock );
    WSACloseEvent( dpwsData->acceptEvent );
    closesocket( dpwsData->tcpSock );
}

static HRESULT WINAPI DPWSCB_EnumSessions( LPDPSP_ENUMSESSIONSDATA data )
{
    DPSP_MSG_HEADER *header = (DPSP_MSG_HEADER *) data->lpMessage;
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    SOCKADDR_IN addr;
    BOOL trueflag = TRUE;
    SOCKET sock;
    HRESULT hr;

    TRACE( "(%p,%ld,%p,%u)\n",
           data->lpMessage, data->dwMessageSize,
           data->lpISP, data->bReturnStatus );

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = DPWS_Start( dpwsData );
    if ( FAILED (hr) )
        return hr;

    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( sock == INVALID_SOCKET )
    {
        ERR( "socket() failed\n" );
        return DPERR_UNAVAILABLE;
    }

    if ( SOCKET_ERROR == setsockopt( sock, SOL_SOCKET, SO_BROADCAST,
                                     (const char *) &trueflag,
                                     sizeof( trueflag ) ) )
    {
        ERR( "setsockopt() failed\n" );
        closesocket( sock );
        return DPERR_UNAVAILABLE;
    }

    memset( header, 0, sizeof( DPSP_MSG_HEADER ) );
    header->mixed = DPSP_MSG_MAKE_MIXED( data->dwMessageSize, DPSP_MSG_TOKEN_REMOTE );
    header->SockAddr.sin_family = AF_INET;
    header->SockAddr.sin_port = dpwsData->tcpAddr.sin_port;

    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl( INADDR_BROADCAST );
    addr.sin_port = htons( DPWS_PORT );

    if ( SOCKET_ERROR == sendto( sock, data->lpMessage, data->dwMessageSize, 0, (SOCKADDR *) &addr,
                                 sizeof( addr ) ) )
    {
        ERR( "sendto() failed\n" );
        closesocket( sock );
        return DPERR_UNAVAILABLE;
    }

    closesocket( sock );

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_Reply( LPDPSP_REPLYDATA data )
{
    FIXME( "(%p,%p,%ld,%ld,%p) stub\n",
           data->lpSPMessageHeader, data->lpMessage, data->dwMessageSize,
           data->idNameServer, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_Send( LPDPSP_SENDDATA data )
{
    FIXME( "(0x%08lx,%ld,%ld,%p,%ld,%u,%p) stub\n",
           data->dwFlags, data->idPlayerTo, data->idPlayerFrom,
           data->lpMessage, data->dwMessageSize,
           data->bSystemMessage, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_AddPlayerToGroup( DPSP_ADDPLAYERTOGROUPDATA *data )
{
    DPWS_CONNECTION_REF *playerConnectionRef;
    DPWS_CONNECTION_REF *groupConnectionRef;
    struct rb_entry *entry;
    DPWS_PLAYER *player;
    DPWS_PLAYER *group;
    DWORD playerSize;
    DWORD groupSize;
    HRESULT hr;

    TRACE( "(%ld,%ld,%p)\n", data->idPlayer, data->idGroup, data->lpISP );

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idPlayer, (void **) &player, &playerSize,
                                        DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idGroup, (void **) &group, &groupSize,
                                        DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    RB_FOR_EACH_ENTRY( playerConnectionRef, &player->connectionRefs, DPWS_CONNECTION_REF, entry )
    {
        entry = rb_get( &group->connectionRefs, &playerConnectionRef->connection->key );
        if ( entry )
        {
            groupConnectionRef = RB_ENTRY_VALUE( entry, DPWS_CONNECTION_REF, entry );
            ++groupConnectionRef->ref;
            continue;
        }
        groupConnectionRef = calloc( 1, sizeof( DPWS_CONNECTION_REF ) );
        if ( !groupConnectionRef )
            return DPERR_OUTOFMEMORY;
        groupConnectionRef->ref = 1;
        groupConnectionRef->connection = playerConnectionRef->connection;
        rb_put( &group->connectionRefs, &groupConnectionRef->connection->key,
                &groupConnectionRef->entry );
    }

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_CreateGroup( DPSP_CREATEGROUPDATA *data )
{
    DPWS_PLAYER groupPlaceholder = { 0 };
    DPWS_PLAYER *group;
    DWORD groupSize;
    HRESULT hr;

    TRACE( "(%ld,0x%08lx,%p,%p)\n", data->idGroup, data->dwFlags, data->lpSPMessageHeader,
           data->lpISP );

    hr = IDirectPlaySP_SetSPPlayerData( data->lpISP, data->idGroup, &groupPlaceholder,
                                        sizeof( groupPlaceholder ), DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idGroup, (void **) &group, &groupSize,
                                        DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    rb_init( &group->connectionRefs, DPWS_CompareConnectionRefs );

    return DP_OK;
}

static HRESULT DPWS_GetConnection( DPWS_DATA *dpwsData, SOCKADDR_IN *addr, BOOL guaranteed,
                                   DPWS_OUT_CONNECTION **connection )
{
    DPWS_CONNECTION_KEY key;
    struct rb_entry *entry;

    key.addr = *addr;
    key.guaranteed = guaranteed;

    entry = rb_get( &dpwsData->connections, &key );
    if ( entry )
    {
        *connection = RB_ENTRY_VALUE( entry, DPWS_OUT_CONNECTION, entry );
        return DP_OK;
    }

    *connection = calloc( 1, sizeof( DPWS_OUT_CONNECTION ) );
    if ( !*connection )
        return DPERR_OUTOFMEMORY;

    (*connection)->key = key;
    (*connection)->tcpSock = INVALID_SOCKET;

    rb_put( &dpwsData->connections, &(*connection)->key, &(*connection)->entry );

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_CreatePlayer( LPDPSP_CREATEPLAYERDATA data )
{
    DPWS_CONNECTION_REF *tcpConnectionRef;
    DPWS_CONNECTION_REF *udpConnectionRef;
    DPWS_PLAYER playerPlaceholder = { 0 };
    DPWS_PLAYERDATA *playerData;
    DWORD playerDataSize;
    DPWS_PLAYER *player;
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    DWORD playerSize;
    HRESULT hr;

    TRACE( "(%ld,0x%08lx,%p,%p)\n",
           data->idPlayer, data->dwFlags,
           data->lpSPMessageHeader, data->lpISP );

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    if ( !data->lpSPMessageHeader )
    {
        DPWS_PLAYERDATA playerDataPlaceholder = { 0 };
        hr = IDirectPlaySP_SetSPPlayerData( data->lpISP, data->idPlayer,
                                            (void *) &playerDataPlaceholder,
                                            sizeof( playerDataPlaceholder ), DPSET_REMOTE );
        if ( FAILED( hr ) )
            return hr;
    }

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idPlayer, (void **) &playerData,
                                        &playerDataSize, DPSET_REMOTE );
    if ( FAILED( hr ) )
        return hr;
    if ( playerDataSize != sizeof( DPWS_PLAYERDATA ) )
        return DPERR_GENERIC;

    if ( data->lpSPMessageHeader )
    {
        DPSP_MSG_HEADER *header = data->lpSPMessageHeader;
        if ( !playerData->tcpAddr.sin_addr.s_addr )
            playerData->tcpAddr.sin_addr = header->SockAddr.sin_addr;
        if ( !playerData->udpAddr.sin_addr.s_addr )
            playerData->udpAddr.sin_addr = header->SockAddr.sin_addr;
    }
    else
    {
        playerData->tcpAddr.sin_family = AF_INET;
        playerData->tcpAddr.sin_port = dpwsData->tcpAddr.sin_port;
        playerData->udpAddr.sin_family = AF_INET;
        playerData->udpAddr.sin_port = dpwsData->udpAddr.sin_port;
    }

    hr = IDirectPlaySP_SetSPPlayerData( data->lpISP, data->idPlayer, (void *) &playerPlaceholder,
                                        sizeof( playerPlaceholder ), DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idPlayer,
                                        (void **) &player, &playerSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    rb_init( &player->connectionRefs, DPWS_CompareConnectionRefs );

    if ( data->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL )
        return DP_OK;

    tcpConnectionRef = calloc( 1, sizeof( DPWS_CONNECTION_REF ) );
    if ( !tcpConnectionRef )
    {
        free( player );
        return DPERR_OUTOFMEMORY;
    }

    tcpConnectionRef->ref = 1;

    hr = DPWS_GetConnection( dpwsData, &playerData->tcpAddr, TRUE, &tcpConnectionRef->connection );
    if ( FAILED( hr ) )
    {
        free( tcpConnectionRef );
        free( player );
        return hr;
    }

    udpConnectionRef = calloc( 1, sizeof( DPWS_CONNECTION_REF ) );
    if ( !udpConnectionRef )
    {
        free( tcpConnectionRef );
        free( player );
        return DPERR_OUTOFMEMORY;
    }

    udpConnectionRef->ref = 1;

    hr = DPWS_GetConnection( dpwsData, &playerData->udpAddr, FALSE, &udpConnectionRef->connection );
    if ( FAILED( hr ) )
    {
        free( udpConnectionRef );
        free( tcpConnectionRef );
        free( player );
        return hr;
    }

    rb_put( &player->connectionRefs, &tcpConnectionRef->connection->key, &tcpConnectionRef->entry );
    rb_put( &player->connectionRefs, &udpConnectionRef->connection->key, &udpConnectionRef->entry );

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_DeleteGroup( DPSP_DELETEGROUPDATA *data )
{
    DPWS_CONNECTION_REF *connectionRef2;
    DPWS_CONNECTION_REF *connectionRef;
    DPWS_PLAYER *group;
    DWORD groupSize;
    HRESULT hr;

    TRACE( "(%ld,0x%08lx,%p)\n", data->idGroup, data->dwFlags, data->lpISP );

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idGroup, (void **) &group, &groupSize,
                                        DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    RB_FOR_EACH_ENTRY_DESTRUCTOR( connectionRef, connectionRef2, &group->connectionRefs,
                                  DPWS_CONNECTION_REF, entry )
    {
        rb_remove( &group->connectionRefs, &connectionRef->entry );
        free( connectionRef );
    }

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_DeletePlayer( LPDPSP_DELETEPLAYERDATA data )
{
    DPWS_CONNECTION_REF *connectionRef2;
    DPWS_CONNECTION_REF *connectionRef;
    DPWS_PLAYER *player;
    DWORD playerSize;
    HRESULT hr;

    TRACE( "(%ld,0x%08lx,%p)\n",
           data->idPlayer, data->dwFlags, data->lpISP );

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idPlayer, (void **) &player, &playerSize,
                                        DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    RB_FOR_EACH_ENTRY_DESTRUCTOR( connectionRef, connectionRef2, &player->connectionRefs,
                                  DPWS_CONNECTION_REF, entry )
    {
        rb_remove( &player->connectionRefs, &connectionRef->entry );
        free( connectionRef );
    }

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_GetAddress( LPDPSP_GETADDRESSDATA data )
{
    FIXME( "(%ld,0x%08lx,%p,%p,%p) stub\n",
           data->idPlayer, data->dwFlags, data->lpAddress,
           data->lpdwAddressSize, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_GetCaps( LPDPSP_GETCAPSDATA data )
{
    TRACE( "(%ld,%p,0x%08lx,%p)\n",
           data->idPlayer, data->lpCaps, data->dwFlags, data->lpISP );

    data->lpCaps->dwFlags = ( DPCAPS_ASYNCSUPPORTED |
                              DPCAPS_GUARANTEEDOPTIMIZED |
                              DPCAPS_GUARANTEEDSUPPORTED );

    data->lpCaps->dwMaxQueueSize    = DPWS_MAXQUEUESIZE;
    data->lpCaps->dwHundredBaud     = DPWS_HUNDREDBAUD;
    data->lpCaps->dwLatency         = DPWS_LATENCY;
    data->lpCaps->dwMaxLocalPlayers = DPWS_MAXLOCALPLAYERS;
    data->lpCaps->dwHeaderLength    = sizeof(DPSP_MSG_HEADER);
    data->lpCaps->dwTimeout         = DPWS_TIMEOUT;

    if ( data->dwFlags & DPGETCAPS_GUARANTEED )
    {
        data->lpCaps->dwMaxBufferSize = DPWS_GUARANTEED_MAXBUFFERSIZE;
        data->lpCaps->dwMaxPlayers    = DPWS_GUARANTEED_MAXPLAYERS;
    }
    else
    {
        data->lpCaps->dwMaxBufferSize = DPWS_MAXBUFFERSIZE;
        data->lpCaps->dwMaxPlayers    = DPWS_MAXPLAYERS;
    }

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_Open( LPDPSP_OPENDATA data )
{
    DPSP_MSG_HEADER *header = (DPSP_MSG_HEADER *) data->lpSPMessageHeader;
    SOCKADDR_IN nameserverAddr;
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    HRESULT hr;

    TRACE( "(%u,%p,%p,%u,0x%08lx,0x%08lx)\n",
           data->bCreate, data->lpSPMessageHeader, data->lpISP,
           data->bReturnStatus, data->dwOpenFlags, data->dwSessionFlags );

    if ( data->bCreate )
    {
        FIXME( "session creation is not yet supported\n" );
        return DPERR_UNSUPPORTED;
    }

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **)&dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = DPWS_Start( dpwsData );
    if ( FAILED (hr) )
        return hr;

    memset( &nameserverAddr, 0, sizeof( nameserverAddr ) );
    nameserverAddr.sin_family = AF_INET;
    nameserverAddr.sin_addr = header->SockAddr.sin_addr;
    nameserverAddr.sin_port = header->SockAddr.sin_port;

    rb_init( &dpwsData->nameserver.connectionRefs, DPWS_CompareConnectionRefs );

    hr = DPWS_GetConnection( dpwsData, &nameserverAddr, TRUE,
                             &dpwsData->nameserverConnectionRef.connection );
    if ( FAILED( hr ) )
        return hr;

    rb_put( &dpwsData->nameserver.connectionRefs,
            &dpwsData->nameserverConnectionRef.connection->key,
            &dpwsData->nameserverConnectionRef.entry );

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_RemovePlayerFromGroup( DPSP_REMOVEPLAYERFROMGROUPDATA *data )
{
    DPWS_CONNECTION_REF *playerConnectionRef;
    DPWS_CONNECTION_REF *groupConnectionRef;
    struct rb_entry *entry;
    DPWS_PLAYER *player;
    DPWS_PLAYER *group;
    DWORD playerSize;
    DWORD groupSize;
    HRESULT hr;

    TRACE( "(%ld,%ld,%p)\n", data->idPlayer, data->idGroup, data->lpISP );

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idPlayer, (void **) &player, &playerSize,
                                        DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idGroup, (void **) &group, &groupSize,
                                        DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    RB_FOR_EACH_ENTRY( playerConnectionRef, &player->connectionRefs, DPWS_CONNECTION_REF, entry )
    {
        entry = rb_get( &group->connectionRefs, &playerConnectionRef->connection->key );
        if ( !entry )
            continue;
        groupConnectionRef = RB_ENTRY_VALUE( entry, DPWS_CONNECTION_REF, entry );
        --groupConnectionRef->ref;
        if ( groupConnectionRef->ref )
            continue;
        rb_remove( &player->connectionRefs, &groupConnectionRef->entry );
        free( groupConnectionRef );
    }

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_CloseEx( LPDPSP_CLOSEDATA data )
{
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    HRESULT hr;

    TRACE( "(%p)\n", data->lpISP );

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    DPWS_Stop( dpwsData );

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_ShutdownEx( LPDPSP_SHUTDOWNDATA data )
{
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    HRESULT hr;

    TRACE( "(%p)\n", data->lpISP );

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    DPWS_Stop( dpwsData );

    WSACleanup();

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_GetAddressChoices( LPDPSP_GETADDRESSCHOICESDATA data )
{
    FIXME( "(%p,%p,%p) stub\n",
           data->lpAddress, data->lpdwAddressSize, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT DPWS_SendImpl( DPWS_DATA *dpwsData, DPWS_PLAYER *player, SGBUFFER *buffers,
                              DWORD bufferCount, DWORD messageSize, BOOL guaranteed, BOOL system )
{
    DPWS_CONNECTION_REF *connectionRef;
    DPSP_MSG_HEADER header = { 0 };
    HRESULT sendResult;

    if ( guaranteed || system )
    {
        header.mixed = DPSP_MSG_MAKE_MIXED( messageSize, DPSP_MSG_TOKEN_REMOTE );
        header.SockAddr.sin_family = AF_INET;
        header.SockAddr.sin_port = dpwsData->tcpAddr.sin_port;

        buffers[ 0 ].pData = (unsigned char *) &header;
    }

    EnterCriticalSection( &dpwsData->sendCs );

    if ( guaranteed )
    {
        RB_FOR_EACH_ENTRY( connectionRef, &player->connectionRefs, DPWS_CONNECTION_REF, entry )
        {
            DPWS_OUT_CONNECTION *connection = connectionRef->connection;

            if ( !connection->key.guaranteed )
                continue;
            if ( connection->tcpSock != INVALID_SOCKET )
                continue;

            connection->tcpSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
            if ( connection->tcpSock == INVALID_SOCKET )
            {
                ERR( "socket() failed\n" );
                LeaveCriticalSection( &dpwsData->sendCs );
                return DPERR_UNAVAILABLE;
            }

            if ( SOCKET_ERROR == connect( connection->tcpSock, (SOCKADDR *) &connection->key.addr,
                                          sizeof( connection->key.addr ) ) )
            {
                ERR( "connect() failed\n" );
                closesocket( connection->tcpSock );
                connection->tcpSock = INVALID_SOCKET;
                LeaveCriticalSection( &dpwsData->sendCs );
                return DPERR_UNAVAILABLE;
            }
        }
    }

    sendResult = DP_OK;

    RB_FOR_EACH_ENTRY( connectionRef, &player->connectionRefs, DPWS_CONNECTION_REF, entry )
    {
        DPWS_OUT_CONNECTION *connection = connectionRef->connection;
        DWORD transferred;

        if ( connection->key.guaranteed != guaranteed )
            continue;

        if ( guaranteed )
        {
            if ( SOCKET_ERROR == WSASend( connection->tcpSock, (WSABUF *) buffers, bufferCount,
                                          &transferred, 0, &connection->overlapped, NULL ) )
            {
                if ( WSAGetLastError() != WSA_IO_PENDING )
                {
                    ERR( "WSASend() failed\n" );
                    LeaveCriticalSection( &dpwsData->sendCs );
                    return DPERR_UNAVAILABLE;
                }
            }
        }
        else
        {
            if ( SOCKET_ERROR == WSASendTo( dpwsData->udpSock, (WSABUF *) buffers, bufferCount,
                                            &transferred, 0, (SOCKADDR *) &connection->key.addr,
                                            sizeof( connection->key.addr ), &connection->overlapped,
                                            NULL ) )
            {
                if ( WSAGetLastError() != WSA_IO_PENDING )
                {
                    ERR( "WSASendTo() failed\n" );
                    LeaveCriticalSection( &dpwsData->sendCs );
                    return DPERR_UNAVAILABLE;
                }
            }
        }
    }

    RB_FOR_EACH_ENTRY( connectionRef, &player->connectionRefs, DPWS_CONNECTION_REF, entry )
    {
        DPWS_OUT_CONNECTION *connection = connectionRef->connection;
        DWORD transferred;
        SOCKET sock;

        if ( connection->key.guaranteed != guaranteed )
            continue;

        sock = guaranteed ? connection->tcpSock : dpwsData->udpSock;

        if ( !WSAGetOverlappedResult( sock, &connection->overlapped, &transferred, TRUE, NULL ) )
        {
            ERR( "WSAGetOverlappedResult() failed\n" );
            sendResult = DPERR_UNAVAILABLE;
            continue;
        }

        if ( guaranteed && transferred < messageSize )
        {
            ERR( "lost connection\n" );
            closesocket( connection->tcpSock );
            connection->tcpSock = INVALID_SOCKET;
            sendResult = DPERR_CONNECTIONLOST;
            continue;
        }
    }

    LeaveCriticalSection( &dpwsData->sendCs );

    return sendResult;
}

static HRESULT WINAPI DPWSCB_SendEx( LPDPSP_SENDEXDATA data )
{
    DPWS_PLAYER *player;
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    HRESULT hr;

    TRACE( "(%p,0x%08lx,%ld,%ld,%p,%ld,%ld,%ld,%ld,%p,%p,%u)\n",
           data->lpISP, data->dwFlags, data->idPlayerTo, data->idPlayerFrom,
           data->lpSendBuffers, data->cBuffers, data->dwMessageSize,
           data->dwPriority, data->dwTimeout, data->lpDPContext,
           data->lpdwSPMsgID, data->bSystemMessage );

    if ( data->dwFlags & DPSEND_ASYNC )
    {
        FIXME("asynchronous send is not yet supported\n");
        return DPERR_UNSUPPORTED;
    }

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    if ( data->idPlayerTo )
    {
        DWORD playerSize;
        hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idPlayerTo, (void **) &player,
                                            &playerSize, DPSET_LOCAL );
        if ( FAILED( hr ) )
            return hr;
    }
    else
    {
        player = &dpwsData->nameserver;
    }

    return DPWS_SendImpl( dpwsData, player, data->lpSendBuffers, data->cBuffers,
                          data->dwMessageSize, !!( data->dwFlags & DPSEND_GUARANTEED ),
                          data->bSystemMessage );
}

static HRESULT WINAPI DPWSCB_SendToGroupEx( LPDPSP_SENDTOGROUPEXDATA data )
{
    DPWS_DATA *dpwsData;
    DPWS_PLAYER *group;
    DWORD dpwsDataSize;
    DWORD groupSize;
    HRESULT hr;

    TRACE( "(%p,0x%08lx,%ld,%ld,%p,%ld,%ld,%ld,%ld,%p,%p)\n",
           data->lpISP, data->dwFlags, data->idGroupTo, data->idPlayerFrom,
           data->lpSendBuffers, data->cBuffers, data->dwMessageSize,
           data->dwPriority, data->dwTimeout, data->lpDPContext,
           data->lpdwSPMsgID );

    if ( data->dwFlags & DPSEND_ASYNC )
    {
        FIXME("asynchronous send is not yet supported\n");
        return DPERR_UNSUPPORTED;
    }

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idGroupTo, (void **) &group, &groupSize,
                                        DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    return DPWS_SendImpl( dpwsData, group, data->lpSendBuffers, data->cBuffers, data->dwMessageSize,
                          !!( data->dwFlags & DPSEND_GUARANTEED ), FALSE );
}

static HRESULT WINAPI DPWSCB_Cancel( LPDPSP_CANCELDATA data )
{
    FIXME( "(%p,0x%08lx,%p,%ld,0x%08lx,0x%08lx) stub\n",
           data->lpISP, data->dwFlags, data->lprglpvSPMsgID, data->cSPMsgID,
           data->dwMinPriority, data->dwMaxPriority );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_GetMessageQueue( LPDPSP_GETMESSAGEQUEUEDATA data )
{
    FIXME( "(%p,0x%08lx,%ld,%ld,%p,%p) stub\n",
           data->lpISP, data->dwFlags, data->idFrom, data->idTo,
           data->lpdwNumMsgs, data->lpdwNumBytes );
    return DPERR_UNSUPPORTED;
}

static void setup_callbacks( LPDPSP_SPCALLBACKS lpCB )
{
    lpCB->EnumSessions           = DPWSCB_EnumSessions;
    lpCB->Reply                  = DPWSCB_Reply;
    lpCB->Send                   = DPWSCB_Send;
    lpCB->AddPlayerToGroup       = DPWSCB_AddPlayerToGroup;
    lpCB->CreateGroup            = DPWSCB_CreateGroup;
    lpCB->CreatePlayer           = DPWSCB_CreatePlayer;
    lpCB->DeleteGroup            = DPWSCB_DeleteGroup;
    lpCB->DeletePlayer           = DPWSCB_DeletePlayer;
    lpCB->GetAddress             = DPWSCB_GetAddress;
    lpCB->GetCaps                = DPWSCB_GetCaps;
    lpCB->Open                   = DPWSCB_Open;
    lpCB->RemovePlayerFromGroup  = DPWSCB_RemovePlayerFromGroup;
    lpCB->CloseEx                = DPWSCB_CloseEx;
    lpCB->ShutdownEx             = DPWSCB_ShutdownEx;
    lpCB->GetAddressChoices      = DPWSCB_GetAddressChoices;
    lpCB->SendEx                 = DPWSCB_SendEx;
    lpCB->SendToGroupEx          = DPWSCB_SendToGroupEx;
    lpCB->Cancel                 = DPWSCB_Cancel;
    lpCB->GetMessageQueue        = DPWSCB_GetMessageQueue;

    lpCB->Close                  = NULL;
    lpCB->SendToGroup            = NULL;
    lpCB->Shutdown               = NULL;
}



/******************************************************************
 *		SPInit (DPWSOCKX.1)
 */
HRESULT WINAPI SPInit( LPSPINITDATA lpspData )
{
    WSADATA wsaData;
    DPWS_DATA dpwsData;

    TRACE( "Initializing library for %s (%s)\n",
           wine_dbgstr_guid(lpspData->lpGuid), debugstr_w(lpspData->lpszName) );

    /* We only support TCP/IP service */
    if ( !IsEqualGUID(lpspData->lpGuid, &DPSPGUID_TCPIP) )
    {
        return DPERR_UNAVAILABLE;
    }

    /* Assign callback functions */
    setup_callbacks( lpspData->lpCB );

    /* Load Winsock 2.0 DLL */
    if ( WSAStartup( MAKEWORD(2, 0), &wsaData ) != 0 )
    {
        ERR( "WSAStartup() failed\n" );
        return DPERR_UNAVAILABLE;
    }

    /* Initialize internal data */
    memset( &dpwsData, 0, sizeof(DPWS_DATA) );
    dpwsData.lpISP = lpspData->lpISP;
    IDirectPlaySP_SetSPData( lpspData->lpISP, &dpwsData, sizeof(DPWS_DATA),
                             DPSET_LOCAL );

    /* dplay needs to know the size of the header */
    lpspData->dwSPHeaderSize = sizeof(DPSP_MSG_HEADER);

    return DP_OK;
}
