/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock Helper DLL for TCP/IP
 * FILE:        include/wshtcpip.h
 * PURPOSE:     WinSock Helper DLL for TCP/IP header
 */
#ifndef __WSHTCPIP_H
#define __WSHTCPIP_H

#define WIN32_NO_STATUS
#include <wsahelp.h>
#include <tdiinfo.h>
#include <tcpioctl.h>
#include <tdilib.h>
#include <ws2tcpip.h>
#include <rtlfuncs.h>

#define EXPORT WINAPI

#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_RAW_IP_DEVICE_NAME   L"\\Device\\RawIp"

typedef enum _SOCKET_STATE {
    SocketStateCreated,
    SocketStateBound,
    SocketStateListening,
    SocketStateConnected
} SOCKET_STATE, *PSOCKET_STATE;

typedef struct _QUEUED_REQUEST {
    PTCP_REQUEST_SET_INFORMATION_EX Info;
    PVOID Next;
} QUEUED_REQUEST, *PQUEUED_REQUEST;

typedef struct _SOCKET_CONTEXT {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
    DWORD Flags;
    DWORD AddrFileEntityType;
    DWORD AddrFileInstance;
    SOCKET_STATE SocketState;
    PQUEUED_REQUEST RequestQueue;
} SOCKET_CONTEXT, *PSOCKET_CONTEXT;

#endif /* __WSHTCPIP_H */

/* EOF */
