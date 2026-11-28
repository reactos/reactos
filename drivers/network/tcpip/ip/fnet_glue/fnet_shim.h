/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Neutral interface to FNET, so FNET and ReactOS headers never meet
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#pragma once

/* Every function here runs inside FNET */

typedef enum _FNET_SHIM_STATUS
{
    FnetShimOk = 0,
    FnetShimInProgress,
    FnetShimWouldBlock,
    FnetShimNoMemory,
    FnetShimAddressInUse,
    FnetShimAddressNotAvailable,
    FnetShimConnectionReset,
    FnetShimConnectionAborted,
    FnetShimTimedOut,
    FnetShimNotConnected,
    FnetShimUnreachable,
    FnetShimInvalid,
    FnetShimError
} FNET_SHIM_STATUS;

typedef struct _FNET_SHIM_SOCKET_STATE
{
    BOOLEAN Readable;
    BOOLEAN Writable;
    BOOLEAN Error;
    BOOLEAN Connecting;
    BOOLEAN Connected;
    BOOLEAN Listening;
    BOOLEAN FinReceived;
    ULONG ReceiveCount;
    ULONG SendSpace;
    FNET_SHIM_STATUS SocketError;
} FNET_SHIM_SOCKET_STATE, *PFNET_SHIM_SOCKET_STATE;

/* Netif output: copy TotalLength bytes with FnetShimCopyPacket, then return */
typedef VOID (*PFNET_SHIM_OUTPUT)(PVOID IfContext, PVOID Packet, ULONG TotalLength);

/* Socket event callback, called inside FNET; may only record and queue */
typedef VOID (*PFNET_SHIM_EVENT)(PVOID Cookie);

PVOID FnetShimNetifCreate(PVOID IfContext, ULONG Mtu, BOOLEAN Loopback, PFNET_SHIM_OUTPUT Output);
VOID FnetShimNetifRelease(PVOID Netif);
VOID FnetShimNetifSetAddress(PVOID Netif, ULONG Address, ULONG Mask);
VOID FnetShimNetifSetDefault(PVOID Netif);
VOID FnetShimNetifSetMtu(PVOID Netif, ULONG Mtu);
ULONG FnetShimNetifScopeId(PVOID Netif);
BOOLEAN FnetShimNetifInput(PVOID Netif, const VOID *Packet, ULONG Length);
VOID FnetShimCopyPacket(PVOID Packet, PVOID Destination, ULONG Length);

PVOID FnetShimSocketCreate(PFNET_SHIM_EVENT Event, PVOID Cookie);
VOID FnetShimSocketClose(PVOID Socket, BOOLEAN Abortive);
FNET_SHIM_STATUS FnetShimSocketBind(PVOID Socket, ULONG Address, USHORT PortNetworkOrder);
FNET_SHIM_STATUS FnetShimSocketListen(PVOID Socket, ULONG Backlog);
FNET_SHIM_STATUS FnetShimSocketConnect(PVOID Socket, ULONG Address, USHORT PortNetworkOrder, ULONG ScopeId);
PVOID FnetShimSocketAccept(PVOID Socket, PFNET_SHIM_EVENT Event, PVOID Cookie);
FNET_SHIM_STATUS FnetShimSocketSend(PVOID Socket, const VOID *Data, ULONG Length, PULONG Sent);
FNET_SHIM_STATUS FnetShimSocketReceive(PVOID Socket, PVOID Buffer, ULONG Length, PULONG Received);
FNET_SHIM_STATUS FnetShimSocketShutdown(PVOID Socket, BOOLEAN Receive, BOOLEAN Send);
FNET_SHIM_STATUS FnetShimSocketGetName(PVOID Socket, BOOLEAN Peer, PULONG Address, PUSHORT PortNetworkOrder);
VOID FnetShimSocketGetState(PVOID Socket, PFNET_SHIM_SOCKET_STATE State);
/* Returns a MIB_TCP_STATE value */
ULONG FnetShimSocketGetTcpState(PVOID Socket);
VOID FnetShimSocketSetMss(PVOID Socket, ULONG Mss);
VOID FnetShimSocketSetNoDelay(PVOID Socket, BOOLEAN Set);
VOID FnetShimSocketSetKeepAlive(PVOID Socket, BOOLEAN Set, ULONG IdleSeconds, ULONG IntervalSeconds);
