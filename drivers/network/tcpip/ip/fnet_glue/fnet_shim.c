/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FNET side of the TCP glue; only FNET and kernel headers here
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#include <ntifs.h>
#include <stdio.h>
#include <fnet.h>
#include <stack/fnet_prot.h>
#include <stack/fnet_netif_prv.h>
#include <stack/fnet_netbuf_prv.h>
#include <stack/fnet_ip4_prv.h>
#include <stack/fnet_socket_prv.h>
#include <stack/fnet_tcp.h>
#include <fnet_ros_port.h>
#include "fnet_shim.h"

#define FNET_SHIM_NETIF_TAG 'iTNF'

typedef struct _FNET_SHIM_NETIF
{
    fnet_netif_t Netif;
    PVOID IfContext;
    PFNET_SHIM_OUTPUT Output;
} FNET_SHIM_NETIF, *PFNET_SHIM_NETIF;

static PFNET_SHIM_EVENT ShimEventRoutine;
static ULONG ShimNetifNumber;

static
fnet_return_t
ShimNetifInit(fnet_netif_t *netif)
{
    FNET_COMP_UNUSED_ARG(netif);
    return FNET_OK;
}

static
void
ShimNetifOutput(fnet_netif_t *netif, fnet_ip4_addr_t dest_ip_addr, fnet_netbuf_t *nb)
{
    PFNET_SHIM_NETIF Shim = netif->netif_prv;
    fnet_ip4_header_t Header;

    FNET_COMP_UNUSED_ARG(dest_ip_addr);

    /* FNET must only ever emit TCP; ReactOS owns ICMP and everything else */
    if (nb->total_length >= sizeof(Header))
    {
        _fnet_netbuf_to_buf(nb, 0, sizeof(Header), &Header);
        if (Header.protocol == FNET_PROT_TCP)
            Shim->Output(Shim->IfContext, nb, (ULONG)nb->total_length);
        else
            DbgPrintEx(DPFLTR_TCPIP_ID, DPFLTR_WARNING_LEVEL, "TCPIP: dropping FNET output with IP protocol %u\n", Header.protocol);
    }

    _fnet_netbuf_free_chain(nb);
}

static const fnet_netif_api_t ShimNetifApi =
{
    .netif_type = FNET_NETIF_TYPE_OTHER,
    .netif_init = ShimNetifInit,
    .netif_output_ip4 = ShimNetifOutput,
};

/* The loopback type makes FNET accept any destination address, as ReactOS loops back traffic to its own addresses */
static const fnet_netif_api_t ShimLoopbackApi =
{
    .netif_type = FNET_NETIF_TYPE_LOOPBACK,
    .netif_init = ShimNetifInit,
    .netif_output_ip4 = ShimNetifOutput,
};

PVOID
FnetShimNetifCreate(PVOID IfContext, ULONG Mtu, BOOLEAN Loopback, PFNET_SHIM_OUTPUT Output)
{
    PFNET_SHIM_NETIF Shim;

    ASSERT(FnetPortInsideFnet());

    Shim = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Shim), FNET_SHIM_NETIF_TAG);
    if (!Shim)
        return NULL;

    RtlZeroMemory(Shim, sizeof(*Shim));
    Shim->IfContext = IfContext;
    Shim->Output = Output;
    Shim->Netif.netif_mtu = Mtu;
    Shim->Netif.netif_prv = Shim;
    Shim->Netif.netif_api = Loopback ? &ShimLoopbackApi : &ShimNetifApi;
    _snprintf(Shim->Netif.netif_name, sizeof(Shim->Netif.netif_name) - 1, "ros%lu", ShimNetifNumber++);

    if (fnet_netif_init(&Shim->Netif, NULL, 0) != FNET_OK)
    {
        ExFreePoolWithTag(Shim, FNET_SHIM_NETIF_TAG);
        return NULL;
    }
    return Shim;
}

VOID
FnetShimNetifRelease(PVOID Netif)
{
    PFNET_SHIM_NETIF Shim = Netif;

    fnet_netif_release(&Shim->Netif);
    ExFreePoolWithTag(Shim, FNET_SHIM_NETIF_TAG);
}

VOID
FnetShimNetifSetAddress(PVOID Netif, ULONG Address, ULONG Mask)
{
    PFNET_SHIM_NETIF Shim = Netif;

    fnet_netif_set_ip4_addr(&Shim->Netif, (fnet_ip4_addr_t)Address, (fnet_ip4_addr_t)Mask);
}

VOID
FnetShimNetifSetDefault(PVOID Netif)
{
    PFNET_SHIM_NETIF Shim = Netif;

    fnet_netif_set_default(&Shim->Netif);
}

VOID
FnetShimNetifSetMtu(PVOID Netif, ULONG Mtu)
{
    PFNET_SHIM_NETIF Shim = Netif;

    Shim->Netif.netif_mtu = Mtu;
}

ULONG
FnetShimNetifScopeId(PVOID Netif)
{
    PFNET_SHIM_NETIF Shim = Netif;

    return (ULONG)fnet_netif_get_scope_id(&Shim->Netif);
}

BOOLEAN
FnetShimNetifInput(PVOID Netif, const VOID *Packet, ULONG Length)
{
    PFNET_SHIM_NETIF Shim = Netif;
    fnet_netbuf_t *nb;

    nb = _fnet_netbuf_from_buf(Packet, Length, FNET_TRUE);
    if (!nb)
        return FALSE;

    _fnet_ip4_input(&Shim->Netif, nb);
    return TRUE;
}

VOID
FnetShimCopyPacket(PVOID Packet, PVOID Destination, ULONG Length)
{
    _fnet_netbuf_to_buf((fnet_netbuf_t *)Packet, 0, Length, Destination);
}

static
FNET_SHIM_STATUS
ShimTranslate(fnet_error_t Error)
{
    switch (Error)
    {
        case FNET_ERR_OK: return FnetShimOk;
        case FNET_ERR_INPROGRESS: return FnetShimInProgress;
        case FNET_ERR_AGAIN: return FnetShimWouldBlock;
        case FNET_ERR_NOMEM: return FnetShimNoMemory;
        case FNET_ERR_ADDRINUSE: return FnetShimAddressInUse;
        case FNET_ERR_ADDRNOTAVAIL: return FnetShimAddressNotAvailable;
        case FNET_ERR_CONNRESET: return FnetShimConnectionReset;
        case FNET_ERR_CONNABORTED: return FnetShimConnectionAborted;
        case FNET_ERR_TIMEDOUT: return FnetShimTimedOut;
        case FNET_ERR_NOTCONN: return FnetShimNotConnected;
        case FNET_ERR_NETUNREACH:
        case FNET_ERR_HOSTUNREACH: return FnetShimUnreachable;
        case FNET_ERR_INVAL: return FnetShimInvalid;
        default: return FnetShimError;
    }
}

static
void
ShimEventThunk(fnet_socket_t s, void *cookie)
{
    FNET_COMP_UNUSED_ARG(s);
    ShimEventRoutine(cookie);
}

static
VOID
ShimFillAddress(struct fnet_sockaddr *Storage, ULONG Address, USHORT Port, ULONG ScopeId)
{
    struct fnet_sockaddr_in *In = (struct fnet_sockaddr_in *)Storage;

    RtlZeroMemory(Storage, sizeof(*Storage));
    In->sin_family = AF_INET;
    In->sin_port = Port;
    In->sin_addr.s_addr = (fnet_ip4_addr_t)Address;
    In->sin_scope_id = (fnet_scope_id_t)ScopeId;
}

PVOID
FnetShimSocketCreate(PFNET_SHIM_EVENT Event, PVOID Cookie)
{
    fnet_socket_t Socket;

    ShimEventRoutine = Event;

    Socket = fnet_socket(AF_INET, SOCK_STREAM, 0);
    if (Socket)
        fnet_socket_set_event_callback(Socket, ShimEventThunk, Cookie);
    return Socket;
}

VOID
FnetShimSocketClose(PVOID Socket, BOOLEAN Abortive)
{
    struct fnet_linger Linger;

    fnet_socket_set_event_callback(Socket, NULL, NULL);

    if (Abortive)
    {
        /* A zero linger makes FNET send RST and drop the connection */
        Linger.l_onoff = FNET_TRUE;
        Linger.l_linger = 0;
        fnet_socket_setopt(Socket, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger));
    }
    fnet_socket_close(Socket);
}

FNET_SHIM_STATUS
FnetShimSocketBind(PVOID Socket, ULONG Address, USHORT PortNetworkOrder)
{
    struct fnet_sockaddr Name;

    ShimFillAddress(&Name, Address, PortNetworkOrder, 0);
    if (fnet_socket_bind(Socket, &Name, sizeof(Name)) != FNET_OK)
        return ShimTranslate(fnet_error_get());
    return FnetShimOk;
}

FNET_SHIM_STATUS
FnetShimSocketListen(PVOID Socket, ULONG Backlog)
{
    if (fnet_socket_listen(Socket, Backlog ? Backlog : 1) != FNET_OK)
        return ShimTranslate(fnet_error_get());
    return FnetShimOk;
}

FNET_SHIM_STATUS
FnetShimSocketConnect(PVOID Socket, ULONG Address, USHORT PortNetworkOrder, ULONG ScopeId)
{
    struct fnet_sockaddr Name;
    fnet_error_t Error;

    ShimFillAddress(&Name, Address, PortNetworkOrder, ScopeId);
    if (fnet_socket_connect(Socket, &Name, sizeof(Name)) == FNET_OK)
        return FnetShimInProgress;

    Error = fnet_error_get();
    return (Error == FNET_ERR_INPROGRESS) ? FnetShimInProgress : ShimTranslate(Error);
}

PVOID
FnetShimSocketAccept(PVOID Socket, PFNET_SHIM_EVENT Event, PVOID Cookie)
{
    struct fnet_sockaddr Peer;
    fnet_size_t PeerLength = sizeof(Peer);
    fnet_socket_t NewSocket;

    ShimEventRoutine = Event;

    NewSocket = fnet_socket_accept(Socket, &Peer, &PeerLength);
    if (NewSocket)
        fnet_socket_set_event_callback(NewSocket, ShimEventThunk, Cookie);
    return NewSocket;
}

FNET_SHIM_STATUS
FnetShimSocketSend(PVOID Socket, const VOID *Data, ULONG Length, PULONG Sent)
{
    FNET_SHIM_SOCKET_STATE State;
    fnet_ssize_t Result;

    *Sent = 0;

    /* FNET itself decides whether the state allows sending (CLOSE_WAIT does, see REACTOS_PATCHES.md) */
    FnetShimSocketGetState(Socket, &State);
    if (State.Error)
        return State.SocketError;
    if (State.SendSpace == 0)
        return (State.Connected || State.FinReceived) ? FnetShimWouldBlock : FnetShimNotConnected;

    /* FNET rejects a send bigger than the whole buffer, so never offer more than the free space */
    Result = fnet_socket_send(Socket, Data, min(Length, State.SendSpace), 0);
    if (Result < 0)
        return ShimTranslate(fnet_error_get());

    /* Zero with buffer space left means FNET could not get heap for the data */
    if (Result == 0)
        return FnetShimNoMemory;

    *Sent = (ULONG)Result;
    return FnetShimOk;
}

FNET_SHIM_STATUS
FnetShimSocketReceive(PVOID Socket, PVOID Buffer, ULONG Length, PULONG Received)
{
    fnet_ssize_t Result;

    *Received = 0;

    Result = fnet_socket_recv(Socket, Buffer, Length, 0);
    if (Result < 0)
        return ShimTranslate(fnet_error_get());

    *Received = (ULONG)Result;
    return (Result > 0) ? FnetShimOk : FnetShimWouldBlock;
}

FNET_SHIM_STATUS
FnetShimSocketShutdown(PVOID Socket, BOOLEAN Receive, BOOLEAN Send)
{
    fnet_sd_flags_t How = 0;

    if (Receive)
        How |= SD_READ;
    if (Send)
        How |= SD_WRITE;

    if (fnet_socket_shutdown(Socket, How) != FNET_OK)
        return ShimTranslate(fnet_error_get());
    return FnetShimOk;
}

FNET_SHIM_STATUS
FnetShimSocketGetName(PVOID Socket, BOOLEAN Peer, PULONG Address, PUSHORT PortNetworkOrder)
{
    struct fnet_sockaddr Name;
    fnet_size_t NameLength = sizeof(Name);
    fnet_return_t Result;

    Result = Peer ? fnet_socket_getpeername(Socket, &Name, &NameLength)
                  : fnet_socket_getname(Socket, &Name, &NameLength);
    if (Result != FNET_OK)
        return ShimTranslate(fnet_error_get());

    *Address = (ULONG)((struct fnet_sockaddr_in *)&Name)->sin_addr.s_addr;
    *PortNetworkOrder = ((struct fnet_sockaddr_in *)&Name)->sin_port;
    return FnetShimOk;
}

VOID
FnetShimSocketGetState(PVOID Socket, PFNET_SHIM_SOCKET_STATE State)
{
    fnet_socket_if_t *Sock = *(fnet_socket_if_t **)Socket;
    fnet_socket_poll_t Poll;
    fnet_socket_state_t SocketState = SS_CLOSED;
    fnet_error_t PendingError = FNET_ERR_OK;
    fnet_uint32_t Value = 0;
    fnet_uint32_t SendMax = 0;
    fnet_size_t Length;

    RtlZeroMemory(State, sizeof(*State));

    /* Read the real error first: polling a closed socket records FNET_ERR_NOTCONN over the RST or timeout reason */
    if (Sock && Sock != (fnet_socket_if_t *)FNET_SOCKET_DESC_RESERVED)
        PendingError = (Sock->options.local_error != FNET_ERR_OK) ? Sock->options.local_error : Sock->options.error;

    Poll.s = Socket;
    Poll.events = FNET_SOCKET_EVENT_ALL;
    Poll.events_occurred = FNET_SOCKET_EVENT_NONE;
    fnet_socket_poll(&Poll, 1);
    State->Readable = (Poll.events_occurred & FNET_SOCKET_EVENT_IN) != 0;
    State->Writable = (Poll.events_occurred & FNET_SOCKET_EVENT_OUT) != 0;

    Length = sizeof(SocketState);
    fnet_socket_getopt(Socket, SOL_SOCKET, SO_STATE, &SocketState, &Length);
    State->Connecting = (SocketState == SS_CONNECTING);
    State->Connected = (SocketState == SS_CONNECTED);
    State->Listening = (SocketState == SS_LISTENING);

    if (PendingError != FNET_ERR_OK && PendingError != FNET_ERR_NOTCONN)
    {
        State->Error = TRUE;
        State->SocketError = ShimTranslate(PendingError);
    }
    else if (SocketState == SS_CLOSED && (Poll.events_occurred & FNET_SOCKET_EVENT_ERR))
    {
        /* Closed with no recorded reason */
        State->Error = TRUE;
        State->SocketError = FnetShimConnectionAborted;
    }

    if (!State->Listening)
    {
        Length = sizeof(Value);
        if (fnet_socket_getopt(Socket, IPPROTO_TCP, TCP_FINRCVD, &Value, &Length) == FNET_OK)
            State->FinReceived = (Value != 0);

        Length = sizeof(Value);
        if (fnet_socket_getopt(Socket, SOL_SOCKET, SO_RCVNUM, &Value, &Length) == FNET_OK)
            State->ReceiveCount = Value;

        Length = sizeof(SendMax);
        if (fnet_socket_getopt(Socket, SOL_SOCKET, SO_SNDBUF, &SendMax, &Length) == FNET_OK)
        {
            Length = sizeof(Value);
            if (fnet_socket_getopt(Socket, SOL_SOCKET, SO_SNDNUM, &Value, &Length) == FNET_OK && SendMax > Value)
                State->SendSpace = SendMax - Value;
        }
    }
}

VOID
FnetShimSocketSetMss(PVOID Socket, ULONG Mss)
{
    fnet_uint32_t Value = Mss;

    fnet_socket_setopt(Socket, IPPROTO_TCP, TCP_MSS, &Value, sizeof(Value));
}

VOID
FnetShimSocketSetNoDelay(PVOID Socket, BOOLEAN Set)
{
    fnet_uint32_t Value = Set ? 1 : 0;

    fnet_socket_setopt(Socket, IPPROTO_TCP, TCP_NODELAY, &Value, sizeof(Value));
}

VOID
FnetShimSocketSetKeepAlive(PVOID Socket, BOOLEAN Set, ULONG IdleSeconds, ULONG IntervalSeconds)
{
    fnet_uint32_t Value = Set ? 1 : 0;

    fnet_socket_setopt(Socket, SOL_SOCKET, SO_KEEPALIVE, &Value, sizeof(Value));
    if (IdleSeconds)
    {
        Value = IdleSeconds;
        fnet_socket_setopt(Socket, IPPROTO_TCP, TCP_KEEPIDLE, &Value, sizeof(Value));
    }
    if (IntervalSeconds)
    {
        Value = IntervalSeconds;
        fnet_socket_setopt(Socket, IPPROTO_TCP, TCP_KEEPINTVL, &Value, sizeof(Value));
    }
    /* Same probe count lwIP used */
    Value = 10;
    fnet_socket_setopt(Socket, IPPROTO_TCP, TCP_KEEPCNT, &Value, sizeof(Value));
}

/* MIB_TCP_STATE values from tcpmib.h, which is a user mode header */
enum
{
    MIB_TCP_STATE_CLOSED = 1,
    MIB_TCP_STATE_LISTEN,
    MIB_TCP_STATE_SYN_SENT,
    MIB_TCP_STATE_SYN_RCVD,
    MIB_TCP_STATE_ESTAB,
    MIB_TCP_STATE_FIN_WAIT1,
    MIB_TCP_STATE_FIN_WAIT2,
    MIB_TCP_STATE_CLOSE_WAIT,
    MIB_TCP_STATE_CLOSING,
    MIB_TCP_STATE_LAST_ACK,
    MIB_TCP_STATE_TIME_WAIT
};

ULONG
FnetShimSocketGetTcpState(PVOID Socket)
{
    /* A descriptor points at the table slot holding the socket, as in _fnet_socket_desc_find */
    fnet_socket_if_t *Sock = *(fnet_socket_if_t **)Socket;
    fnet_tcp_control_t *Cb;

    if (!Sock || Sock == (fnet_socket_if_t *)FNET_SOCKET_DESC_RESERVED || !Sock->protocol_control)
        return MIB_TCP_STATE_CLOSED;

    if (Sock->state == SS_LISTENING)
        return MIB_TCP_STATE_LISTEN;

    Cb = (fnet_tcp_control_t *)Sock->protocol_control;
    switch (Cb->tcpcb_connection_state)
    {
        case FNET_TCP_CS_SYN_SENT: return MIB_TCP_STATE_SYN_SENT;
        case FNET_TCP_CS_SYN_RCVD: return MIB_TCP_STATE_SYN_RCVD;
        case FNET_TCP_CS_LISTENING: return MIB_TCP_STATE_LISTEN;
        case FNET_TCP_CS_ESTABLISHED: return MIB_TCP_STATE_ESTAB;
        case FNET_TCP_CS_FIN_WAIT_1: return MIB_TCP_STATE_FIN_WAIT1;
        case FNET_TCP_CS_FIN_WAIT_2: return MIB_TCP_STATE_FIN_WAIT2;
        case FNET_TCP_CS_CLOSE_WAIT: return MIB_TCP_STATE_CLOSE_WAIT;
        case FNET_TCP_CS_CLOSING: return MIB_TCP_STATE_CLOSING;
        case FNET_TCP_CS_LAST_ACK: return MIB_TCP_STATE_LAST_ACK;
        case FNET_TCP_CS_TIME_WAIT: return MIB_TCP_STATE_TIME_WAIT;
        default: return MIB_TCP_STATE_CLOSED;
    }
}
