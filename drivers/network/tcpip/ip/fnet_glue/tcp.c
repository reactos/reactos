/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FNET TCP glue: sockets, events and the data path
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#include "precomp.h"
#include "fnet_shim.h"

NTSTATUS TCPTranslateError(const err_t err);

struct tcp_pcb
{
    LONG RefCount;
    PVOID Socket;                       /* FNET socket; used only inside FNET, created on bind */
    PCONNECTION_ENDPOINT Connection;    /* Owner; NULL once detached; under GlueLock */
    LIST_ENTRY ListEntry;               /* GluePcbList; under GlueLock */
    ULONG BoundAddress;
    LONG UpcallQueued;
    FNET_PORT_WORK_ITEM UpcallItem;
    BOOLEAN Listening;
    BOOLEAN Connecting;
    BOOLEAN Established;
    BOOLEAN FinReported;
    BOOLEAN AbortPending;
    BOOLEAN NoDelay;
    BOOLEAN KeepAlive;
    ULONG KeepIdleSeconds;
    ULONG KeepIntervalSeconds;
};

static KSPIN_LOCK GlueLock;
static LIST_ENTRY GluePcbList;

static VOID GlueUpcall(PVOID Context);

static
err_t
GlueError(FNET_SHIM_STATUS Status)
{
    switch (Status)
    {
        case FnetShimOk: return ERR_OK;
        case FnetShimInProgress: return ERR_INPROGRESS;
        case FnetShimWouldBlock: return ERR_INPROGRESS;
        case FnetShimNoMemory: return ERR_MEM;
        case FnetShimAddressInUse: return ERR_USE;
        case FnetShimAddressNotAvailable: return ERR_VAL;
        case FnetShimConnectionReset: return ERR_RST;
        case FnetShimConnectionAborted: return ERR_ABRT;
        case FnetShimTimedOut: return ERR_TIMEOUT;
        case FnetShimNotConnected: return ERR_CONN;
        case FnetShimUnreachable: return ERR_RTE;
        case FnetShimInvalid: return ERR_ARG;
        default: return ERR_IF;
    }
}

/* After a reset AFD must report WSAECONNRESET; msafd maps the ERR_RST status to WSAECONNREFUSED, which only fits a refused connect */
static
err_t
GlueDataError(FNET_SHIM_STATUS Status)
{
    return (Status == FnetShimConnectionReset) ? ERR_CLSD : GlueError(Status);
}

static
PTCP_PCB
PcbAllocate(VOID)
{
    PTCP_PCB Pcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Pcb), FNET_GLUE_PCB_TAG);

    if (Pcb)
    {
        RtlZeroMemory(Pcb, sizeof(*Pcb));
        Pcb->RefCount = 1;
        InitializeListHead(&Pcb->ListEntry);
    }
    return Pcb;
}

static
VOID
PcbReference(PTCP_PCB Pcb)
{
    InterlockedIncrement(&Pcb->RefCount);
}

static
VOID
PcbDereference(PTCP_PCB Pcb)
{
    if (InterlockedDecrement(&Pcb->RefCount) == 0)
    {
        ASSERT(Pcb->Socket == NULL);
        ASSERT(Pcb->Connection == NULL);
        ExFreePoolWithTag(Pcb, FNET_GLUE_PCB_TAG);
    }
}

static
VOID
PcbAttach(PTCP_PCB Pcb, PCONNECTION_ENDPOINT Connection)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&GlueLock, &OldIrql);
    Pcb->Connection = Connection;
    InsertTailList(&GluePcbList, &Pcb->ListEntry);
    KeReleaseSpinLock(&GlueLock, OldIrql);
}

/* Returns a referenced owner, or NULL once detached */
static
PCONNECTION_ENDPOINT
PcbGetConnection(PTCP_PCB Pcb)
{
    PCONNECTION_ENDPOINT Connection;
    KIRQL OldIrql;

    KeAcquireSpinLock(&GlueLock, &OldIrql);
    Connection = Pcb->Connection;
    if (Connection)
        ReferenceObject(Connection);
    KeReleaseSpinLock(&GlueLock, OldIrql);
    return Connection;
}

/* Returns the connection's pcb referenced, or NULL once another thread detached it */
static
PTCP_PCB
GlueGetPcb(PCONNECTION_ENDPOINT Connection)
{
    PTCP_PCB Pcb;
    KIRQL OldIrql;

    KeAcquireSpinLock(&GlueLock, &OldIrql);
    Pcb = Connection->SocketContext;
    if (Pcb)
        PcbReference(Pcb);
    KeReleaseSpinLock(&GlueLock, OldIrql);
    return Pcb;
}

/* Inside FNET. FNET events only queue an upcall; IRPs are completed outside FNET. */
static
VOID
GlueQueueUpcall(PTCP_PCB Pcb)
{
    if (InterlockedExchange(&Pcb->UpcallQueued, 1))
        return;

    PcbReference(Pcb);
    FnetPortDeferUpcall(&Pcb->UpcallItem, GlueUpcall, Pcb);
}

static
VOID
GlueSocketEvent(PVOID Cookie)
{
    GlueQueueUpcall((PTCP_PCB)Cookie);
}

static
VOID
GlueKickRoutine(PVOID Context)
{
    GlueQueueUpcall((PTCP_PCB)Context);
}

/* Makes the next upcall look at the socket again */
static
VOID
GlueKick(PTCP_PCB Pcb)
{
    FnetPortRunSync(GlueKickRoutine, Pcb);
}

typedef struct _GLUE_CLOSE_CALL
{
    PTCP_PCB Pcb;
    BOOLEAN Abortive;
} GLUE_CLOSE_CALL;

static
VOID
GlueCloseRoutine(PVOID Context)
{
    GLUE_CLOSE_CALL *Call = Context;

    if (Call->Pcb->Socket)
    {
        FnetShimSocketClose(Call->Pcb->Socket, Call->Abortive);
        Call->Pcb->Socket = NULL;
    }
}

/* Unlinks pcb and connection, closes the FNET socket, drops the owner reference; FALSE if already detached */
static
BOOLEAN
GlueDetach(PTCP_PCB Pcb, PCONNECTION_ENDPOINT Connection, BOOLEAN Abortive)
{
    GLUE_CLOSE_CALL Call;
    KIRQL OldIrql;

    KeAcquireSpinLock(&GlueLock, &OldIrql);
    if (Pcb->Connection != Connection)
    {
        KeReleaseSpinLock(&GlueLock, OldIrql);
        return FALSE;
    }
    Pcb->Connection = NULL;
    RemoveEntryList(&Pcb->ListEntry);
    InitializeListHead(&Pcb->ListEntry);
    if (Connection->SocketContext == Pcb)
        Connection->SocketContext = NULL;
    KeReleaseSpinLock(&GlueLock, OldIrql);

    Call.Pcb = Pcb;
    Call.Abortive = Abortive;
    FnetPortRunSync(GlueCloseRoutine, &Call);

    PcbDereference(Pcb);
    return TRUE;
}

/* Same steps as the lwIP glue's error callback: one last read, then tear down */
static
VOID
GlueFail(PTCP_PCB Pcb, PCONNECTION_ENDPOINT Connection, err_t Error)
{
    /* Hand data that is already buffered to waiting reads first */
    TCPRecvEventHandler(Connection);

    PcbReference(Pcb);
    if (!GlueDetach(Pcb, Connection, TRUE))
    {
        PcbDereference(Pcb);
        return;
    }
    PcbDereference(Pcb);

    LockObject(Connection);
    Connection->ReceiveShutdown = TRUE;
    Connection->ReceiveShutdownStatus = TCPTranslateError(Error);
    UnlockObject(Connection);

    TCPRecvEventHandler(Connection);
    TCPFinEventHandler(Connection, Error);
}

typedef struct _GLUE_STATE_CALL
{
    PTCP_PCB Pcb;
    BOOLEAN HaveSocket;
    FNET_SHIM_SOCKET_STATE State;
} GLUE_STATE_CALL;

static
VOID
GlueStateRoutine(PVOID Context)
{
    GLUE_STATE_CALL *Call = Context;

    Call->HaveSocket = (Call->Pcb->Socket != NULL);
    if (Call->HaveSocket)
        FnetShimSocketGetState(Call->Pcb->Socket, &Call->State);
}

typedef struct _GLUE_ACCEPT_CALL
{
    PTCP_PCB Listener;
    PTCP_PCB NewPcb;
} GLUE_ACCEPT_CALL;

static
VOID
GlueAcceptRoutine(PVOID Context)
{
    GLUE_ACCEPT_CALL *Call = Context;

    if (Call->Listener->Socket)
        Call->NewPcb->Socket = FnetShimSocketAccept(Call->Listener->Socket, GlueSocketEvent, Call->NewPcb);
}

/* Pairs connections waiting in the FNET backlog with queued listen IRPs */
static
VOID
GlueAcceptIncoming(PTCP_PCB Listener, PCONNECTION_ENDPOINT ListenConnection)
{
    GLUE_ACCEPT_CALL Call;
    GLUE_CLOSE_CALL Close;
    BOOLEAN Waiting;

    for (;;)
    {
        LockObject(ListenConnection);
        Waiting = !IsListEmpty(&ListenConnection->ListenRequest);
        UnlockObject(ListenConnection);

        /* Without a listen IRP the connection stays in the FNET backlog until TCPAccept */
        if (!Waiting)
            break;

        Call.Listener = Listener;
        Call.NewPcb = PcbAllocate();
        if (!Call.NewPcb)
            break;

        FnetPortRunSync(GlueAcceptRoutine, &Call);
        if (!Call.NewPcb->Socket)
        {
            PcbDereference(Call.NewPcb);
            break;
        }
        Call.NewPcb->Established = TRUE;

        TCPAcceptEventHandler(ListenConnection, Call.NewPcb);

        /* LibTCPAccept attaches it; if no listen IRP took it, refuse like the lwIP glue does */
        if (!Call.NewPcb->Connection)
        {
            Close.Pcb = Call.NewPcb;
            Close.Abortive = TRUE;
            FnetPortRunSync(GlueCloseRoutine, &Close);
            PcbDereference(Call.NewPcb);
        }
    }
}

static
VOID
GlueDispatch(PTCP_PCB Pcb, PCONNECTION_ENDPOINT Connection, PFNET_SHIM_SOCKET_STATE State)
{
    BOOLEAN Finish;

    if (Pcb->AbortPending)
    {
        GlueFail(Pcb, Connection, ERR_ABRT);
        return;
    }

    if (Pcb->Listening)
    {
        if (State->Readable)
            GlueAcceptIncoming(Pcb, Connection);
        return;
    }

    if (Pcb->Connecting)
    {
        if (State->Error)
        {
            Pcb->Connecting = FALSE;
            GlueFail(Pcb, Connection, GlueError(State->SocketError));
            return;
        }
        if (State->Connecting)
            return;

        Pcb->Connecting = FALSE;
        Pcb->Established = TRUE;
        TCPConnectEventHandler(Connection, ERR_OK);
    }

    if (!Pcb->Established)
        return;

    if (State->ReceiveCount)
        TCPRecvEventHandler(Connection);

    if (State->Error)
    {
        GlueFail(Pcb, Connection, GlueDataError(State->SocketError));
        return;
    }

    /* Graceful close from the peer, once everything before the FIN has been read */
    if (State->FinReceived && State->ReceiveCount == 0 && !Pcb->FinReported)
    {
        Pcb->FinReported = TRUE;

        LockObject(Connection);
        if (!Connection->ReceiveShutdown)
        {
            Connection->ReceiveShutdown = TRUE;
            Connection->ReceiveShutdownStatus = STATUS_SUCCESS;
        }
        Finish = Connection->SendShutdown;
        UnlockObject(Connection);

        TCPRecvEventHandler(Connection);

        /* Both directions done: the socket is finished, as in the lwIP glue */
        if (Finish)
        {
            if (GlueDetach(Pcb, Connection, FALSE))
                TCPFinEventHandler(Connection, ERR_CLSD);
            return;
        }
    }

    if (State->SendSpace &&
        (!IsListEmpty(&Connection->SendRequest) || !IsListEmpty(&Connection->ShutdownRequest)))
    {
        TCPSendEventHandler(Connection, (u16_t)min(State->SendSpace, 0xFFFF));
    }
}

static
VOID
GlueUpcall(PVOID Context)
{
    PTCP_PCB Pcb = Context;
    PCONNECTION_ENDPOINT Connection;
    GLUE_STATE_CALL Call;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(!FnetPortInsideFnet());

    /* Events from here on queue a fresh upcall */
    InterlockedExchange(&Pcb->UpcallQueued, 0);

    Connection = PcbGetConnection(Pcb);
    if (Connection)
    {
        Call.Pcb = Pcb;
        FnetPortRunSync(GlueStateRoutine, &Call);
        if (Call.HaveSocket)
            GlueDispatch(Pcb, Connection, &Call.State);
        DereferenceObject(Connection);
    }

    PcbDereference(Pcb);
}

PTCP_PCB
LibTCPSocket(void *arg)
{
    PTCP_PCB Pcb = PcbAllocate();

    /* The FNET socket comes later, in bind, so idle connection objects cost no FNET socket */
    if (Pcb)
        PcbAttach(Pcb, (PCONNECTION_ENDPOINT)arg);
    return Pcb;
}

VOID
LibTCPFreeSocket(PTCP_PCB pcb)
{
    PCONNECTION_ENDPOINT Connection = PcbGetConnection(pcb);

    if (Connection)
    {
        GlueDetach(pcb, Connection, FALSE);
        DereferenceObject(Connection);
    }
}

typedef struct _GLUE_BIND_CALL
{
    PTCP_PCB Pcb;
    ULONG Address;
    USHORT Port;
    FNET_SHIM_STATUS Status;
} GLUE_BIND_CALL;

static
VOID
GlueApplyOptions(PTCP_PCB Pcb)
{
    if (Pcb->NoDelay)
        FnetShimSocketSetNoDelay(Pcb->Socket, TRUE);
    if (Pcb->KeepAlive)
        FnetShimSocketSetKeepAlive(Pcb->Socket, TRUE, Pcb->KeepIdleSeconds, Pcb->KeepIntervalSeconds);
}

static
VOID
GlueBindRoutine(PVOID Context)
{
    GLUE_BIND_CALL *Call = Context;

    if (!Call->Pcb->Socket)
    {
        Call->Pcb->Socket = FnetShimSocketCreate(GlueSocketEvent, Call->Pcb);
        if (!Call->Pcb->Socket)
        {
            Call->Status = FnetShimNoMemory;
            return;
        }
        GlueApplyOptions(Call->Pcb);
    }
    Call->Status = FnetShimSocketBind(Call->Pcb->Socket, Call->Address, Call->Port);
}

err_t
LibTCPBind(PCONNECTION_ENDPOINT Connection, ip4_addr_t *const ipaddr, const u16_t port)
{
    PTCP_PCB Pcb = Connection->SocketContext;
    GLUE_BIND_CALL Call;

    if (!Pcb)
        return ERR_CLSD;

    Call.Pcb = Pcb;
    Call.Address = ipaddr->addr;
    Call.Port = port;
    FnetPortRunSync(GlueBindRoutine, &Call);

    if (Call.Status == FnetShimOk)
        Pcb->BoundAddress = ipaddr->addr;
    return GlueError(Call.Status);
}

typedef struct _GLUE_LISTEN_CALL
{
    PTCP_PCB Pcb;
    ULONG Backlog;
    FNET_SHIM_STATUS Status;
} GLUE_LISTEN_CALL;

static
VOID
GlueListenRoutine(PVOID Context)
{
    GLUE_LISTEN_CALL *Call = Context;

    Call->Status = Call->Pcb->Socket ? FnetShimSocketListen(Call->Pcb->Socket, Call->Backlog)
                                     : FnetShimNotConnected;
    if (Call->Status == FnetShimOk)
        Call->Pcb->Listening = TRUE;
}

PTCP_PCB
LibTCPListen(PCONNECTION_ENDPOINT Connection, const u8_t backlog)
{
    PTCP_PCB Pcb = Connection->SocketContext;
    GLUE_LISTEN_CALL Call;

    if (!Pcb)
        return NULL;

    Call.Pcb = Pcb;
    Call.Backlog = backlog;
    FnetPortRunSync(GlueListenRoutine, &Call);
    return (Call.Status == FnetShimOk) ? Pcb : NULL;
}

void
LibTCPAcceptQueued(PCONNECTION_ENDPOINT Listener)
{
    PTCP_PCB Pcb = GlueGetPcb(Listener);

    if (!Pcb)
        return;
    if (Pcb->Listening)
        GlueKick(Pcb);
    PcbDereference(Pcb);
}

void
LibTCPKickReceive(PCONNECTION_ENDPOINT Connection)
{
    PTCP_PCB Pcb = GlueGetPcb(Connection);

    if (!Pcb)
        return;
    if (Pcb->Established)
        GlueKick(Pcb);
    PcbDereference(Pcb);
}

void
LibTCPAccept(PTCP_PCB pcb, PTCP_PCB listen_pcb, void *arg)
{
    UNREFERENCED_PARAMETER(listen_pcb);
    ASSERT(arg);

    PcbAttach(pcb, (PCONNECTION_ENDPOINT)arg);

    /* Data or a FIN may have arrived before the connection was attached */
    GlueKick(pcb);
}

typedef struct _GLUE_CONNECT_CALL
{
    PTCP_PCB Pcb;
    ULONG Address;
    USHORT Port;
    ULONG ScopeId;
    ULONG Mss;
    FNET_SHIM_STATUS Status;
} GLUE_CONNECT_CALL;

static
VOID
GlueConnectRoutine(PVOID Context)
{
    GLUE_CONNECT_CALL *Call = Context;

    if (!Call->Pcb->Socket)
    {
        Call->Status = FnetShimNotConnected;
        return;
    }

    /* The MSS we advertise comes from the interface ReactOS routes through, not from FNET's own route */
    if (Call->Mss)
        FnetShimSocketSetMss(Call->Pcb->Socket, Call->Mss);

    Call->Pcb->Connecting = TRUE;
    Call->Status = FnetShimSocketConnect(Call->Pcb->Socket, Call->Address, Call->Port, Call->ScopeId);
    if (Call->Status != FnetShimInProgress)
        Call->Pcb->Connecting = FALSE;
}

err_t
LibTCPConnect(PCONNECTION_ENDPOINT Connection, ip4_addr_t *const ipaddr, const u16_t port)
{
    PTCP_PCB Pcb = Connection->SocketContext;
    GLUE_CONNECT_CALL Call;
    IP_ADDRESS Remote;
    PNEIGHBOR_CACHE_ENTRY NCE;

    if (!Pcb)
        return ERR_CLSD;

    /* ReactOS routing picks the interface; FNET must not choose its own */
    Remote.Type = IP_ADDRESS_V4;
    Remote.Address.IPv4Address = ipaddr->addr;
    NCE = RouteGetRouteToDestination(&Remote);
    if (!NCE)
        return ERR_RTE;

    Call.Pcb = Pcb;
    Call.Address = ipaddr->addr;
    Call.Port = port;
    Call.ScopeId = FnetGlueInterfaceScopeId(NCE->Interface);
    Call.Mss = (FnetGlueInterfaceMtu(NCE->Interface) > 40) ? FnetGlueInterfaceMtu(NCE->Interface) - 40 : 0;
    FnetPortRunSync(GlueConnectRoutine, &Call);

    return (Call.Status == FnetShimInProgress) ? ERR_INPROGRESS : GlueError(Call.Status);
}

typedef struct _GLUE_SEND_CALL
{
    PTCP_PCB Pcb;
    PVOID Data;
    ULONG Length;
    ULONG Sent;
    FNET_SHIM_STATUS Status;
} GLUE_SEND_CALL;

static
VOID
GlueSendRoutine(PVOID Context)
{
    GLUE_SEND_CALL *Call = Context;

    Call->Status = Call->Pcb->Socket ? FnetShimSocketSend(Call->Pcb->Socket, Call->Data, Call->Length, &Call->Sent)
                                     : FnetShimNotConnected;
}

err_t
LibTCPSend(PCONNECTION_ENDPOINT Connection, void *const dataptr, const ULONG len, ULONG *sent, const int safe)
{
    PTCP_PCB Pcb;
    GLUE_SEND_CALL Call;

    UNREFERENCED_PARAMETER(safe);
    *sent = 0;

    if (Connection->SendShutdown)
        return ERR_CLSD;
    Pcb = GlueGetPcb(Connection);
    if (!Pcb)
        return ERR_CLSD;

    Call.Pcb = Pcb;
    Call.Data = dataptr;
    Call.Length = len;
    Call.Sent = 0;
    FnetPortRunSync(GlueSendRoutine, &Call);
    PcbDereference(Pcb);

    /* Like the lwIP glue: complete with what fit, pend only when nothing fit */
    *sent = Call.Sent;
    return GlueDataError(Call.Status);
}

typedef struct _GLUE_RECEIVE_CALL
{
    PTCP_PCB Pcb;
    PVOID Buffer;
    ULONG Length;
    ULONG Received;
    BOOLEAN FinReceived;
    FNET_SHIM_STATUS Status;
} GLUE_RECEIVE_CALL;

static
VOID
GlueReceiveRoutine(PVOID Context)
{
    GLUE_RECEIVE_CALL *Call = Context;
    FNET_SHIM_SOCKET_STATE State;

    Call->Received = 0;
    Call->FinReceived = FALSE;

    if (!Call->Pcb->Socket)
    {
        Call->Status = FnetShimNotConnected;
        return;
    }

    /* Reading is what reopens the window, so data waits in FNET until someone takes it */
    Call->Status = FnetShimSocketReceive(Call->Pcb->Socket, Call->Buffer, Call->Length, &Call->Received);
    if (Call->Received == 0)
    {
        FnetShimSocketGetState(Call->Pcb->Socket, &State);
        Call->FinReceived = State.FinReceived;
    }
}

NTSTATUS
LibTCPGetDataFromConnectionQueue(PCONNECTION_ENDPOINT Connection, PUCHAR RecvBuffer, UINT RecvLen, UINT *Received)
{
    PTCP_PCB Pcb = GlueGetPcb(Connection);
    GLUE_RECEIVE_CALL Call;
    NTSTATUS Status;

    *Received = 0;

    LockObject(Connection);

    if (Pcb && Pcb->Established && RecvLen)
    {
        Call.Pcb = Pcb;
        Call.Buffer = RecvBuffer;
        Call.Length = RecvLen;
        FnetPortRunSync(GlueReceiveRoutine, &Call);

        if (Call.Received)
        {
            *Received = Call.Received;
            UnlockObject(Connection);
            PcbDereference(Pcb);
            return STATUS_SUCCESS;
        }

        if (Call.FinReceived && !Connection->ReceiveShutdown)
        {
            Connection->ReceiveShutdown = TRUE;
            Connection->ReceiveShutdownStatus = STATUS_SUCCESS;

            /* Let the upcall finish the close if our side is already shut */
            GlueKick(Pcb);
        }
    }

    if (Connection->ReceiveShutdown)
        Status = Connection->ReceiveShutdownStatus;
    else
        Status = STATUS_PENDING;

    UnlockObject(Connection);
    if (Pcb)
        PcbDereference(Pcb);
    return Status;
}

typedef struct _GLUE_SHUTDOWN_CALL
{
    PTCP_PCB Pcb;
    BOOLEAN Receive;
    BOOLEAN Send;
    FNET_SHIM_STATUS Status;
} GLUE_SHUTDOWN_CALL;

static
VOID
GlueShutdownRoutine(PVOID Context)
{
    GLUE_SHUTDOWN_CALL *Call = Context;

    Call->Status = Call->Pcb->Socket ? FnetShimSocketShutdown(Call->Pcb->Socket, Call->Receive, Call->Send)
                                     : FnetShimNotConnected;
}

err_t
LibTCPShutdown(PCONNECTION_ENDPOINT Connection, const int shut_rx, const int shut_tx)
{
    PTCP_PCB Pcb = GlueGetPcb(Connection);
    GLUE_SHUTDOWN_CALL Call;
    BOOLEAN Finish, Detached = FALSE;

    if (!Pcb)
        return ERR_CLSD;

    if (shut_rx && shut_tx)
    {
        /* TDI abort: reset the connection */
        Detached = GlueDetach(Pcb, Connection, TRUE);
    }
    else if (shut_rx || shut_tx)
    {
        Call.Pcb = Pcb;
        Call.Receive = !!shut_rx;
        Call.Send = !!shut_tx;
        FnetPortRunSync(GlueShutdownRoutine, &Call);
        if (Call.Status != FnetShimOk)
        {
            PcbDereference(Pcb);
            return GlueDataError(Call.Status);
        }
    }
    else
    {
        PcbDereference(Pcb);
        DbgPrint("Requested socket shutdown(0, 0) !\n");
        return ERR_OK;
    }

    /* Upcalls read these under the same lock when the peer's FIN arrives */
    LockObject(Connection);
    if (shut_rx)
    {
        Connection->ReceiveShutdown = TRUE;
        Connection->ReceiveShutdownStatus = STATUS_FILE_CLOSED;
    }
    if (shut_tx)
        Connection->SendShutdown = TRUE;
    Finish = Connection->ReceiveShutdown && Connection->SendShutdown;
    UnlockObject(Connection);

    /* Both directions done; only the thread that detaches reports it, the upcall may have won */
    if (Finish && !Detached)
        Detached = GlueDetach(Pcb, Connection, FALSE);
    if (Finish && Detached)
        TCPFinEventHandler(Connection, ERR_CLSD);

    PcbDereference(Pcb);
    return ERR_OK;
}

err_t
LibTCPClose(PCONNECTION_ENDPOINT Connection, const int safe, const int callback)
{
    PTCP_PCB Pcb;

    UNREFERENCED_PARAMETER(safe);

    if (Connection->Closing)
        return ERR_OK;

    if (callback)
        Connection->Closing = TRUE;

    /* Already gone, the client just does not know yet */
    Pcb = GlueGetPcb(Connection);
    if (!Pcb)
        return ERR_OK;

    /* Graceful: FNET sends FIN and finishes the connection on its own; an upcall that detached first already reported it */
    if (GlueDetach(Pcb, Connection, FALSE) && callback)
        TCPFinEventHandler(Connection, ERR_CLSD);

    PcbDereference(Pcb);
    return ERR_OK;
}

typedef struct _GLUE_NAME_CALL
{
    PTCP_PCB Pcb;
    BOOLEAN Peer;
    ULONG Address;
    USHORT Port;
    FNET_SHIM_STATUS Status;
} GLUE_NAME_CALL;

static
VOID
GlueNameRoutine(PVOID Context)
{
    GLUE_NAME_CALL *Call = Context;

    Call->Status = Call->Pcb->Socket ? FnetShimSocketGetName(Call->Pcb->Socket, Call->Peer, &Call->Address, &Call->Port)
                                     : FnetShimNotConnected;
}

static
err_t
GlueGetName(PTCP_PCB pcb, BOOLEAN Peer, ip4_addr_t *const ipaddr, u16_t *const port)
{
    GLUE_NAME_CALL Call;

    if (!pcb)
        return ERR_CLSD;

    Call.Pcb = pcb;
    Call.Peer = Peer;
    FnetPortRunSync(GlueNameRoutine, &Call);
    if (Call.Status != FnetShimOk)
        return ERR_CLSD;

    /* The lwIP glue hands out the port in host order; keep that so callers see the same values */
    ipaddr->addr = Call.Address;
    *port = RtlUshortByteSwap(Call.Port);
    return ERR_OK;
}

err_t
LibTCPGetHostName(PTCP_PCB pcb, ip4_addr_t *const ipaddr, u16_t *const port)
{
    return GlueGetName(pcb, FALSE, ipaddr, port);
}

err_t
LibTCPGetPeerName(PTCP_PCB pcb, ip4_addr_t *const ipaddr, u16_t *const port)
{
    return GlueGetName(pcb, TRUE, ipaddr, port);
}

static
VOID
GlueOptionsRoutine(PVOID Context)
{
    PTCP_PCB Pcb = Context;

    if (Pcb->Socket)
    {
        FnetShimSocketSetNoDelay(Pcb->Socket, Pcb->NoDelay);
        FnetShimSocketSetKeepAlive(Pcb->Socket, Pcb->KeepAlive, Pcb->KeepIdleSeconds, Pcb->KeepIntervalSeconds);
    }
}

void
LibTCPSetNoDelay(PTCP_PCB pcb, BOOLEAN Set)
{
    if (!pcb)
        return;
    pcb->NoDelay = Set;
    FnetPortRunSync(GlueOptionsRoutine, pcb);
}

void
LibTCPSetKeepAlive(PTCP_PCB pcb, BOOLEAN Set)
{
    if (!pcb)
        return;
    pcb->KeepAlive = Set;
    FnetPortRunSync(GlueOptionsRoutine, pcb);
}

void
LibTcpSetKeepAliveValues(PTCP_PCB pcb, u32_t KeepAliveTime, u32_t KeepAliveInterval)
{
    if (!pcb)
        return;

    /* Callers pass milliseconds; FNET counts seconds. Round up so short values do not become 0. */
    pcb->KeepIdleSeconds = KeepAliveTime ? max((KeepAliveTime + 999) / 1000, 1) : 0;
    pcb->KeepIntervalSeconds = KeepAliveInterval ? max((KeepAliveInterval + 999) / 1000, 1) : 0;
    FnetPortRunSync(GlueOptionsRoutine, pcb);
}

typedef struct _GLUE_STATUS_CALL
{
    PTCP_PCB Pcb;
    ULONG State;
} GLUE_STATUS_CALL;

static
VOID
GlueStatusRoutine(PVOID Context)
{
    GLUE_STATUS_CALL *Call = Context;

    /* MIB_TCP_STATE_CLOSED when there is no FNET socket yet */
    Call->State = Call->Pcb->Socket ? FnetShimSocketGetTcpState(Call->Pcb->Socket) : 1;
}

void
LibTCPGetSocketStatus(PTCP_PCB pcb, PULONG State)
{
    GLUE_STATUS_CALL Call;

    Call.Pcb = pcb;
    Call.State = 1;
    if (pcb)
        FnetPortRunSync(GlueStatusRoutine, &Call);
    *State = Call.State;
}

void
LibTCPDumpPcb(PVOID SocketContext)
{
    PTCP_PCB Pcb = SocketContext;
    ULONG State;

    LibTCPGetSocketStatus(Pcb, &State);
    DbgPrint("\tFNET socket %p, MIB state %lu, bound %lx\n", Pcb->Socket, State, Pcb->BoundAddress);
}

/* An interface lost Address: connections bound to it are aborted, as lwIP does on an address change */
VOID
FnetGlueAbortAddress(ULONG Address)
{
    PLIST_ENTRY Entry;
    PTCP_PCB Pcb;
    KIRQL OldIrql;
    ULONG Count = 0;

    if (!Address)
        return;

    for (;;)
    {
        Pcb = NULL;

        KeAcquireSpinLock(&GlueLock, &OldIrql);
        for (Entry = GluePcbList.Flink; Entry != &GluePcbList; Entry = Entry->Flink)
        {
            PTCP_PCB Candidate = CONTAINING_RECORD(Entry, struct tcp_pcb, ListEntry);

            if (Candidate->BoundAddress == Address && !Candidate->Listening && !Candidate->AbortPending)
            {
                Candidate->AbortPending = TRUE;
                PcbReference(Candidate);
                Pcb = Candidate;
                break;
            }
        }
        KeReleaseSpinLock(&GlueLock, OldIrql);

        if (!Pcb)
            break;

        GlueKick(Pcb);
        PcbDereference(Pcb);
        Count++;
    }

    if (Count)
        TI_DbgPrint(MIN_TRACE, ("Aborting %lu connections bound to a removed address\n", Count));
}

void
LibIPInitialize(void)
{
    NTSTATUS Status;

    KeInitializeSpinLock(&GlueLock);
    InitializeListHead(&GluePcbList);

    Status = FnetPortInitialize();
    if (!NT_SUCCESS(Status))
        DbgPrint("TCPIP: FNET failed to start (0x%lx), TCP is unavailable\n", Status);
}

void
LibIPShutdown(void)
{
    FnetPortShutdown();
}
