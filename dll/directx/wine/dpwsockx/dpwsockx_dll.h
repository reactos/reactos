/*
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

#ifndef __WINE_DPWSOCKX_DLL_H
#define __WINE_DPWSOCKX_DLL_H

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "winnt.h"
#include "wine/dplaysp.h"
#include "wine/list.h"
#include "wine/rbtree.h"

#define DPWS_MAXQUEUESIZE             0
#define DPWS_HUNDREDBAUD              0
#define DPWS_LATENCY                  500
#define DPWS_MAXLOCALPLAYERS          65536
#define DPWS_TIMEOUT                  5000
#define DPWS_MAXBUFFERSIZE            65479
#define DPWS_MAXPLAYERS               65536
#define DPWS_GUARANTEED_MAXBUFFERSIZE 1048547
#define DPWS_GUARANTEED_MAXPLAYERS    64

#include "pshpack1.h"

typedef struct tagDPSP_MSG_HEADER
{
    DWORD       mixed;
    SOCKADDR_IN SockAddr;
} DPSP_MSG_HEADER, *LPDPSP_MSG_HEADER;
typedef const DPSP_MSG_HEADER* LPCDPSP_MSG_HEADER;

typedef struct
{
    SOCKADDR_IN tcpAddr;
    SOCKADDR_IN udpAddr;
} DPWS_PLAYERDATA;

#include "poppack.h"

typedef struct tagDPWS_IN_CONNECTION DPWS_IN_CONNECTION;
typedef void DPWS_COMPLETION_ROUTINE( DPWS_IN_CONNECTION *connection );

struct tagDPWS_IN_CONNECTION
{
    struct list              entry;
    SOCKADDR_IN              addr;

    SOCKET                   tcpSock;
    WSAOVERLAPPED            overlapped;
    WSABUF                   wsaBuffer;
    DPWS_COMPLETION_ROUTINE *completionRoutine;

    DPSP_MSG_HEADER          header;
    char                    *buffer;
    DWORD                    bufferSize;

    IDirectPlaySP           *sp;
};

typedef struct
{
    SOCKADDR_IN addr;
    BOOL        guaranteed;
} DPWS_CONNECTION_KEY;

typedef struct
{
    struct rb_entry     entry;
    DPWS_CONNECTION_KEY key;

    SOCKET              tcpSock;
    WSAOVERLAPPED       overlapped;
} DPWS_OUT_CONNECTION;

typedef struct
{
    struct rb_entry      entry;
    ULONG                ref;
    DPWS_OUT_CONNECTION *connection;
} DPWS_CONNECTION_REF;

typedef struct
{
    struct rb_entry      entry;
    DPID                 id;
    struct rb_tree       connectionRefs;
} DPWS_PLAYER;

typedef struct tagDPWS_DATA
{
    LPDIRECTPLAYSP        lpISP;

    SOCKET                tcpSock;
    SOCKADDR_IN           tcpAddr;
    WSAEVENT              acceptEvent;
    struct list           inConnections;

    SOCKET                udpSock;
    SOCKADDR_IN           udpAddr;
    WSAOVERLAPPED         overlapped;
    WSABUF                wsaBuffer;
    char                  buffer[ 65536 ];

    CRITICAL_SECTION      sendCs;
    DPWS_PLAYER           nameserver;
    DPWS_CONNECTION_REF   nameserverConnectionRef;
    struct rb_tree        connections;

    BOOL                  started;
    HANDLE                thread;
    WSAEVENT              stopEvent;
} DPWS_DATA, *LPDPWS_DATA;

#define DPSP_MSG_TOKEN_REMOTE    0xFAB00000
#define DPSP_MSG_TOKEN_FORWARDED 0xCAB00000
#define DPSP_MSG_TOKEN_SERVER    0xBAB00000

#define DPSP_MSG_MAKE_MIXED(s,t) ((s) | (t))
#define DPSP_MSG_SIZE(m)         ((m) & 0x000FFFFF)
#define DPSP_MSG_TOKEN(m)        ((m) & 0xFFF00000)



HRESULT WINAPI SPInit( LPSPINITDATA );

#endif	/* __WINE_DPWSOCKX_DLL_H */
