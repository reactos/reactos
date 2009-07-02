/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/tcp.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Art Yerkes (arty@users.sf.net)
 * REVISIONS:
 *   CSH 01/08-2000  Created
 *   arty 12/21/2004 Added accept
 */

#include "precomp.h"

LONG TCP_IPIdentification = 0;
static BOOLEAN TCPInitialized = FALSE;
static NPAGED_LOOKASIDE_LIST TCPSegmentList;
LIST_ENTRY SignalledConnections;
LIST_ENTRY SleepingThreadsList;
FAST_MUTEX SleepingThreadsLock;
RECURSIVE_MUTEX TCPLock;
PORT_SET TCPPorts;

static VOID HandleSignalledConnection( PCONNECTION_ENDPOINT Connection,
                                       ULONG NewState ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    PMDL Mdl;

    TI_DbgPrint(MID_TRACE,("Handling signalled state on %x (%x)\n",
                           Connection, Connection->SocketContext));

    /* Things that can happen when we try the initial connection */
    if( NewState & SEL_CONNECT ) {
        while( !IsListEmpty( &Connection->ConnectRequest ) ) {
            Entry = RemoveHeadList( &Connection->ConnectRequest );
            TI_DbgPrint(DEBUG_TCP, ("Connect Event\n"));

            Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
            Complete = Bucket->Request.RequestNotifyObject;
            TI_DbgPrint(DEBUG_TCP,
                        ("Completing Request %x\n", Bucket->Request.RequestContext));

            if( (NewState & (SEL_CONNECT | SEL_FIN)) ==
                (SEL_CONNECT | SEL_FIN) )
                Status = STATUS_CONNECTION_REFUSED;
            else
                Status = STATUS_SUCCESS;

            Complete( Bucket->Request.RequestContext, Status, 0 );

            /* Frees the bucket allocated in TCPConnect */
            exFreePool( Bucket );
        }
    }

    if( NewState & SEL_ACCEPT ) {
        /* Handle readable on a listening socket --
         * TODO: Implement filtering
         */

        TI_DbgPrint(DEBUG_TCP,("Accepting new connection on %x (Queue: %s)\n",
                               Connection,
                               IsListEmpty(&Connection->ListenRequest) ?
                               "empty" : "nonempty"));

        while( !IsListEmpty( &Connection->ListenRequest ) ) {
            PIO_STACK_LOCATION IrpSp;

            Entry = RemoveHeadList( &Connection->ListenRequest );
            Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
            Complete = Bucket->Request.RequestNotifyObject;

            Irp = Bucket->Request.RequestContext;
            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            TI_DbgPrint(DEBUG_TCP,("Getting the socket\n"));
            Status = TCPServiceListeningSocket
                ( Connection->AddressFile->Listener,
                  Bucket->AssociatedEndpoint,
                  (PTDI_REQUEST_KERNEL)&IrpSp->Parameters );

            TI_DbgPrint(DEBUG_TCP,("Socket: Status: %x\n"));

            if( Status == STATUS_PENDING ) {
                InsertHeadList( &Connection->ListenRequest, &Bucket->Entry );
                break;
            } else {
                Complete( Bucket->Request.RequestContext, Status, 0 );
                exFreePool( Bucket );
            }
        }
    }

    /* Things that happen after we're connected */
    if( NewState & SEL_READ ) {
        TI_DbgPrint(DEBUG_TCP,("Readable: irp list %s\n",
                               IsListEmpty(&Connection->ReceiveRequest) ?
                               "empty" : "nonempty"));

        while( !IsListEmpty( &Connection->ReceiveRequest ) ) {
            OSK_UINT RecvLen = 0, Received = 0;
            PVOID RecvBuffer = 0;

            Entry = RemoveHeadList( &Connection->ReceiveRequest );
            Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
            Complete = Bucket->Request.RequestNotifyObject;

            Irp = Bucket->Request.RequestContext;
            Mdl = Irp->MdlAddress;

            TI_DbgPrint(DEBUG_TCP,
                        ("Getting the user buffer from %x\n", Mdl));

            NdisQueryBuffer( Mdl, &RecvBuffer, &RecvLen );

            TI_DbgPrint(DEBUG_TCP,
                        ("Reading %d bytes to %x\n", RecvLen, RecvBuffer));

            TI_DbgPrint(DEBUG_TCP, ("Connection: %x\n", Connection));
            TI_DbgPrint
                (DEBUG_TCP,
                 ("Connection->SocketContext: %x\n",
                  Connection->SocketContext));
            TI_DbgPrint(DEBUG_TCP, ("RecvBuffer: %x\n", RecvBuffer));

            Status = TCPTranslateError
                ( OskitTCPRecv( Connection->SocketContext,
                                RecvBuffer,
                                RecvLen,
                                &Received,
                                0 ) );

            TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", Received));

            if( Status == STATUS_SUCCESS ) {
                TI_DbgPrint(DEBUG_TCP,("Received %d bytes with status %x\n",
                                       Received, Status));

                Complete( Bucket->Request.RequestContext,
                          STATUS_SUCCESS, Received );
                exFreePool( Bucket );
            } else if( Status == STATUS_PENDING ) {
                InsertHeadList
                    ( &Connection->ReceiveRequest, &Bucket->Entry );
                break;
            } else {
                TI_DbgPrint(DEBUG_TCP,
                            ("Completing Receive request: %x %x\n",
                             Bucket->Request, Status));
                Complete( Bucket->Request.RequestContext, Status, 0 );
                exFreePool( Bucket );
            }
        }
    }
    if( NewState & SEL_WRITE ) {
        TI_DbgPrint(DEBUG_TCP,("Writeable: irp list %s\n",
                               IsListEmpty(&Connection->SendRequest) ?
                               "empty" : "nonempty"));

        while( !IsListEmpty( &Connection->SendRequest ) ) {
            OSK_UINT SendLen = 0, Sent = 0;
            PVOID SendBuffer = 0;

            Entry = RemoveHeadList( &Connection->SendRequest );
            Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
            Complete = Bucket->Request.RequestNotifyObject;

            Irp = Bucket->Request.RequestContext;
            Mdl = Irp->MdlAddress;

            TI_DbgPrint(DEBUG_TCP,
                        ("Getting the user buffer from %x\n", Mdl));

            NdisQueryBuffer( Mdl, &SendBuffer, &SendLen );

            TI_DbgPrint(DEBUG_TCP,
                        ("Writing %d bytes to %x\n", SendLen, SendBuffer));

            TI_DbgPrint(DEBUG_TCP, ("Connection: %x\n", Connection));
            TI_DbgPrint
                (DEBUG_TCP,
                 ("Connection->SocketContext: %x\n",
                  Connection->SocketContext));

            Status = TCPTranslateError
                ( OskitTCPSend( Connection->SocketContext,
                                SendBuffer,
                                SendLen,
                                &Sent,
                                0 ) );

            TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", Sent));

            if( Status == STATUS_SUCCESS ) {
                TI_DbgPrint(DEBUG_TCP,("Sent %d bytes with status %x\n",
                                       Sent, Status));

                Complete( Bucket->Request.RequestContext,
                          STATUS_SUCCESS, Sent );
                exFreePool( Bucket );
            } else if( Status == STATUS_PENDING ) {
                InsertHeadList
                    ( &Connection->SendRequest, &Bucket->Entry );
                break;
            } else {
                TI_DbgPrint(DEBUG_TCP,
                            ("Completing Send request: %x %x\n",
                             Bucket->Request, Status));
                Complete( Bucket->Request.RequestContext, Status, 0 );
                exFreePool( Bucket );
            }
        }
    }

    if( NewState & SEL_FIN ) {
        TI_DbgPrint(DEBUG_TCP, ("EOF From socket\n"));

        while (!IsListEmpty(&Connection->ReceiveRequest))
        {
           DISCONNECT_TYPE DisType;
           PIO_STACK_LOCATION IrpSp;
           Entry = RemoveHeadList(&Connection->ReceiveRequest);
           Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
           Complete = Bucket->Request.RequestNotifyObject;
           IrpSp = IoGetCurrentIrpStackLocation((PIRP)Bucket->Request.RequestContext);

           /* We have to notify oskittcp of the abortion */
           DisType.Type = TDI_DISCONNECT_RELEASE | TDI_DISCONNECT_ABORT;
       DisType.Context = Connection;
       DisType.Irp = (PIRP)Bucket->Request.RequestContext;
       DisType.FileObject = IrpSp->FileObject;

           ChewCreate(NULL, sizeof(DISCONNECT_TYPE),
                      DispDoDisconnect, &DisType);
        }

        while (!IsListEmpty(&Connection->SendRequest))
        {
           DISCONNECT_TYPE DisType;
           PIO_STACK_LOCATION IrpSp;
           Entry = RemoveHeadList(&Connection->SendRequest);
           Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
           Complete = Bucket->Request.RequestNotifyObject;
           IrpSp = IoGetCurrentIrpStackLocation((PIRP)Bucket->Request.RequestContext);

           /* We have to notify oskittcp of the abortion */
           DisType.Type = TDI_DISCONNECT_RELEASE;
       DisType.Context = Connection;
       DisType.Irp = (PIRP)Bucket->Request.RequestContext;
       DisType.FileObject = IrpSp->FileObject;

           ChewCreate(NULL, sizeof(DISCONNECT_TYPE),
                      DispDoDisconnect, &DisType);
        }

        while (!IsListEmpty(&Connection->ListenRequest))
        {
           Entry = RemoveHeadList(&Connection->ListenRequest);
           Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
           Complete = Bucket->Request.RequestNotifyObject;

           /* We have to notify oskittcp of the abortion */
           TCPAbortListenForSocket(Connection->AddressFile->Listener,
                               Connection);

           Complete( Bucket->Request.RequestContext, STATUS_CANCELLED, 0 );
        }

        while (!IsListEmpty(&Connection->ConnectRequest))
        {
           Entry = RemoveHeadList(&Connection->ConnectRequest);
           Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
           Complete = Bucket->Request.RequestNotifyObject;

           Complete( Bucket->Request.RequestContext, STATUS_CANCELLED, 0 );
        }
    }

    Connection->Signalled = FALSE;
}

VOID DrainSignals() {
    PCONNECTION_ENDPOINT Connection;
    PLIST_ENTRY ListEntry;

    while( !IsListEmpty( &SignalledConnections ) ) {
        ListEntry = RemoveHeadList( &SignalledConnections );
        Connection = CONTAINING_RECORD( ListEntry, CONNECTION_ENDPOINT,
                                        SignalList );
        HandleSignalledConnection( Connection, Connection->SignalState );
    }
}

PCONNECTION_ENDPOINT TCPAllocateConnectionEndpoint( PVOID ClientContext ) {
    PCONNECTION_ENDPOINT Connection =
        exAllocatePool(NonPagedPool, sizeof(CONNECTION_ENDPOINT));
    if (!Connection)
        return Connection;

    TI_DbgPrint(DEBUG_CPOINT, ("Connection point file object allocated at (0x%X).\n", Connection));

    RtlZeroMemory(Connection, sizeof(CONNECTION_ENDPOINT));

    /* Initialize spin lock that protects the connection endpoint file object */
    TcpipInitializeSpinLock(&Connection->Lock);
    InitializeListHead(&Connection->ConnectRequest);
    InitializeListHead(&Connection->ListenRequest);
    InitializeListHead(&Connection->ReceiveRequest);
    InitializeListHead(&Connection->SendRequest);

    /* Save client context pointer */
    Connection->ClientContext = ClientContext;

    return Connection;
}

VOID TCPFreeConnectionEndpoint( PCONNECTION_ENDPOINT Connection ) {
    TI_DbgPrint(DEBUG_TCP, ("Freeing TCP Endpoint\n"));
    exFreePool( Connection );
}

NTSTATUS TCPSocket( PCONNECTION_ENDPOINT Connection,
                    UINT Family, UINT Type, UINT Proto ) {
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_TCP,("Called: Connection %x, Family %d, Type %d, "
                           "Proto %d\n",
                           Connection, Family, Type, Proto));

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );
    Status = TCPTranslateError( OskitTCPSocket( Connection,
                                                &Connection->SocketContext,
                                                Family,
                                                Type,
                                                Proto ) );

    ASSERT_KM_POINTER(Connection->SocketContext);

    TI_DbgPrint(DEBUG_TCP,("Connection->SocketContext %x\n",
                           Connection->SocketContext));

    TcpipRecursiveMutexLeave( &TCPLock );

    return Status;
}

VOID TCPReceive(PIP_INTERFACE Interface, PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues TCP data
 * ARGUMENTS:
 *     IPPacket = Pointer to an IP packet that was received
 * NOTES:
 *     This is the low level interface for receiving TCP data
 */
{
    TI_DbgPrint(DEBUG_TCP,("Sending packet %d (%d) to oskit\n",
                           IPPacket->TotalSize,
                           IPPacket->HeaderSize));

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    OskitTCPReceiveDatagram( IPPacket->Header,
                             IPPacket->TotalSize,
                             IPPacket->HeaderSize );

    DrainSignals();

    TcpipRecursiveMutexLeave( &TCPLock );
}

/* event.c */
int TCPSocketState( void *ClientData,
                    void *WhichSocket,
                    void *WhichConnection,
                    OSK_UINT NewState );

int TCPPacketSend( void *ClientData,
                   OSK_PCHAR Data,
                   OSK_UINT Len );

POSK_IFADDR TCPFindInterface( void *ClientData,
                              OSK_UINT AddrType,
                              OSK_UINT FindType,
                              OSK_SOCKADDR *ReqAddr );

NTSTATUS TCPMemStartup( void );
void *TCPMalloc( void *ClientData,
                 OSK_UINT bytes, OSK_PCHAR file, OSK_UINT line );
void TCPFree( void *ClientData,
              void *data, OSK_PCHAR file, OSK_UINT line );
void TCPMemShutdown( void );

int TCPSleep( void *ClientData, void *token, int priority, char *msg,
              int tmio );

void TCPWakeup( void *ClientData, void *token );

OSKITTCP_EVENT_HANDLERS EventHandlers = {
    NULL,             /* Client Data */
    TCPSocketState,   /* SocketState */
    TCPPacketSend,    /* PacketSend */
    TCPFindInterface, /* FindInterface */
    TCPMalloc,        /* Malloc */
    TCPFree,          /* Free */
    TCPSleep,         /* Sleep */
    TCPWakeup         /* Wakeup */
};

static KEVENT TimerLoopEvent;
static HANDLE TimerThreadHandle;

/*
 * We are running 2 timers here, one with a 200ms interval (fast) and the other
 * with a 500ms interval (slow). So we need to time out at 200, 400, 500, 600,
 * 800, 1000 and process the "fast" events at 200, 400, 600, 800, 1000 and the
 * "slow" events at 500 and 1000.
 */
static VOID NTAPI
TimerThread(PVOID Context)
{
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    unsigned Current, NextFast, NextSlow, Next;

    Current = 0;
    Next = 0;
    NextFast = 0;
    NextSlow = 0;
    while ( 1 ) {
        if (Next == NextFast) {
            NextFast += 2;
        }
        if (Next == NextSlow) {
            NextSlow += 5;
        }
        Next = min(NextFast, NextSlow);
        Timeout.QuadPart = (LONGLONG) (Next - Current) * -1000000; /* 100 ms */
        Status = KeWaitForSingleObject(&TimerLoopEvent, Executive, KernelMode,
                                       FALSE, &Timeout);
        if (Status != STATUS_TIMEOUT) {
            PsTerminateSystemThread(Status);
        }

        TcpipRecursiveMutexEnter( &TCPLock, TRUE );
        TimerOskitTCP( Next == NextFast, Next == NextSlow );
        if (Next == NextSlow) {
            DrainSignals();
        }
        TcpipRecursiveMutexLeave( &TCPLock );

        Current = Next;
        if (10 <= Current) {
            Current = 0;
            Next = 0;
            NextFast = 0;
            NextSlow = 0;
        }
    }
}

static VOID
StartTimer(VOID)
{
    KeInitializeEvent(&TimerLoopEvent, NotificationEvent, FALSE);
    PsCreateSystemThread(&TimerThreadHandle, THREAD_ALL_ACCESS, 0, 0, 0,
                         TimerThread, NULL);
}


NTSTATUS TCPStartup(VOID)
/*
 * FUNCTION: Initializes the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;

    TcpipRecursiveMutexInit( &TCPLock );
    ExInitializeFastMutex( &SleepingThreadsLock );
    InitializeListHead( &SleepingThreadsList );
    InitializeListHead( &SignalledConnections );
    Status = TCPMemStartup();
    if ( ! NT_SUCCESS(Status) ) {
        return Status;
    }

    Status = PortsStartup( &TCPPorts, 1, 0xfffe );
    if( !NT_SUCCESS(Status) ) {
        TCPMemShutdown();
        return Status;
    }

    RegisterOskitTCPEventHandlers( &EventHandlers );
    InitOskitTCP();

    /* Register this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, TCPReceive);

    ExInitializeNPagedLookasideList(
        &TCPSegmentList,                /* Lookaside list */
        NULL,                           /* Allocate routine */
        NULL,                           /* Free routine */
        0,                              /* Flags */
        sizeof(TCP_SEGMENT),            /* Size of each entry */
        TAG('T','C','P','S'),           /* Tag */
        0);                             /* Depth */

    StartTimer();

    TCPInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS TCPShutdown(VOID)
/*
 * FUNCTION: Shuts down the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    LARGE_INTEGER WaitForThread;

    if (!TCPInitialized)
        return STATUS_SUCCESS;

    WaitForThread.QuadPart = -2500000; /* 250 ms */
    KeSetEvent(&TimerLoopEvent, IO_NO_INCREMENT, FALSE);
    ZwWaitForSingleObject(TimerThreadHandle, FALSE, &WaitForThread);

    /* Deregister this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, NULL);

    ExDeleteNPagedLookasideList(&TCPSegmentList);

    TCPInitialized = FALSE;

    DeinitOskitTCP();

    PortsShutdown( &TCPPorts );

    TCPMemShutdown();

    return STATUS_SUCCESS;
}

NTSTATUS TCPTranslateError( int OskitError ) {
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    switch( OskitError ) {
    case 0: Status = STATUS_SUCCESS; break;
    case OSK_EADDRNOTAVAIL:
    case OSK_EAFNOSUPPORT: Status = STATUS_INVALID_CONNECTION; break;
    case OSK_ECONNREFUSED:
    case OSK_ECONNRESET: Status = STATUS_REMOTE_NOT_LISTENING; break;
    case OSK_EINPROGRESS:
    case OSK_EAGAIN: Status = STATUS_PENDING; break;
    default: Status = STATUS_INVALID_CONNECTION; break;
    }

    TI_DbgPrint(DEBUG_TCP,("Error %d -> %x\n", OskitError, Status));
    return Status;
}

NTSTATUS TCPConnect
( PCONNECTION_ENDPOINT Connection,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    NTSTATUS Status;
    SOCKADDR_IN AddressToConnect = { 0 }, AddressToBind = { 0 };
    IP_ADDRESS RemoteAddress;
    USHORT RemotePort;
    PTDI_BUCKET Bucket;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(DEBUG_TCP,("TCPConnect: Called\n"));

    Status = AddrBuildAddress
        ((PTRANSPORT_ADDRESS)ConnInfo->RemoteAddress,
         &RemoteAddress,
         &RemotePort);

    if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(DEBUG_TCP, ("Could not AddrBuildAddress in TCPConnect\n"));
        return Status;
    }

    if (!(NCE = RouteGetRouteToDestination(&RemoteAddress)))
    {
        return STATUS_NETWORK_UNREACHABLE;
    }

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    if (Connection->State & SEL_FIN)
    {
        TcpipRecursiveMutexLeave( &TCPLock );
        return STATUS_REMOTE_DISCONNECT;
    }

    /* Freed in TCPSocketState */
    TI_DbgPrint(DEBUG_TCP,
                ("Connecting to address %x:%x\n",
                 RemoteAddress.Address.IPv4Address,
                 RemotePort));

    AddressToConnect.sin_family = AF_INET;
    AddressToBind = AddressToConnect;
    AddressToBind.sin_addr.s_addr = NCE->Interface->Unicast.Address.IPv4Address;

    Status = TCPTranslateError
        ( OskitTCPBind( Connection->SocketContext,
                        Connection,
                        &AddressToBind,
                        sizeof(AddressToBind) ) );

    if (NT_SUCCESS(Status)) {
        memcpy( &AddressToConnect.sin_addr,
                &RemoteAddress.Address.IPv4Address,
                sizeof(AddressToConnect.sin_addr) );
        AddressToConnect.sin_port = RemotePort;

        Status = TCPTranslateError
            ( OskitTCPConnect( Connection->SocketContext,
                               Connection,
                               &AddressToConnect,
                               sizeof(AddressToConnect) ) );

        if (Status == STATUS_PENDING)
        {
            Bucket = exAllocatePool( NonPagedPool, sizeof(*Bucket) );
            if( !Bucket ) return STATUS_NO_MEMORY;
            
            Bucket->Request.RequestNotifyObject = (PVOID)Complete;
            Bucket->Request.RequestContext = Context;
            
            IoMarkIrpPending((PIRP)Context);
			
            InsertTailList( &Connection->ConnectRequest, &Bucket->Entry );
        }
    }

    TcpipRecursiveMutexLeave( &TCPLock );

    return Status;
}

NTSTATUS TCPDisconnect
( PCONNECTION_ENDPOINT Connection,
  UINT Flags,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_TCP,("started\n"));

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    switch( Flags & (TDI_DISCONNECT_ABORT | TDI_DISCONNECT_RELEASE) ) {
    case 0:
    case TDI_DISCONNECT_ABORT:
        Flags = 0;
        break;

    case TDI_DISCONNECT_ABORT | TDI_DISCONNECT_RELEASE:
        Flags = 2;
        break;

    case TDI_DISCONNECT_RELEASE:
        Flags = 1;
        break;
    }

    Status = TCPTranslateError
        ( OskitTCPShutdown( Connection->SocketContext, Flags ) );

    TcpipRecursiveMutexLeave( &TCPLock );

    TI_DbgPrint(DEBUG_TCP,("finished %x\n", Status));

    return Status;
}

NTSTATUS TCPClose
( PCONNECTION_ENDPOINT Connection ) {
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_TCP,("TCPClose started\n"));

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    /* Make our code remove all pending IRPs */
    Connection->State |= SEL_FIN;
    DrainSignals();

    Status = TCPTranslateError( OskitTCPClose( Connection->SocketContext ) );

    TcpipRecursiveMutexLeave( &TCPLock );

    TI_DbgPrint(DEBUG_TCP,("TCPClose finished %x\n", Status));

    return Status;
}

NTSTATUS TCPReceiveData
( PCONNECTION_ENDPOINT Connection,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  PULONG BytesReceived,
  ULONG ReceiveFlags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    PVOID DataBuffer;
    UINT DataLen, Received = 0;
    NTSTATUS Status;
    PTDI_BUCKET Bucket;

    TI_DbgPrint(DEBUG_TCP,("Called for %d bytes (on socket %x)\n",
                           ReceiveLength, Connection->SocketContext));

    ASSERT_KM_POINTER(Connection->SocketContext);

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    /* Closing */
    if (Connection->State & SEL_FIN)
    {
        TcpipRecursiveMutexLeave( &TCPLock );
        *BytesReceived = 0;
        return STATUS_REMOTE_DISCONNECT;
    }

    NdisQueryBuffer( Buffer, &DataBuffer, &DataLen );

    TI_DbgPrint(DEBUG_TCP,("TCP>|< Got an MDL %x (%x:%d)\n", Buffer, DataBuffer, DataLen));

    Status = TCPTranslateError
        ( OskitTCPRecv
          ( Connection->SocketContext,
            DataBuffer,
            DataLen,
            &Received,
            ReceiveFlags ) );

    TI_DbgPrint(DEBUG_TCP,("OskitTCPReceive: %x, %d\n", Status, Received));

    /* Keep this request around ... there was no data yet */
    if( Status == STATUS_PENDING ) {
        /* Freed in TCPSocketState */
        Bucket = exAllocatePool( NonPagedPool, sizeof(*Bucket) );
        if( !Bucket ) {
            TI_DbgPrint(DEBUG_TCP,("Failed to allocate bucket\n"));
            TcpipRecursiveMutexLeave( &TCPLock );
            return STATUS_NO_MEMORY;
        }

        Bucket->Request.RequestNotifyObject = Complete;
        Bucket->Request.RequestContext = Context;
        *BytesReceived = 0;

        IoMarkIrpPending((PIRP)Context);

        InsertTailList( &Connection->ReceiveRequest, &Bucket->Entry );
        TI_DbgPrint(DEBUG_TCP,("Queued read irp\n"));
    } else {
        TI_DbgPrint(DEBUG_TCP,("Got status %x, bytes %d\n", Status, Received));
        *BytesReceived = Received;
    }

    TcpipRecursiveMutexLeave( &TCPLock );

    TI_DbgPrint(DEBUG_TCP,("Status %x\n", Status));

    return Status;
}

NTSTATUS TCPSendData
( PCONNECTION_ENDPOINT Connection,
  PCHAR BufferData,
  ULONG SendLength,
  PULONG BytesSent,
  ULONG Flags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    UINT Sent = 0;
    NTSTATUS Status;
    PTDI_BUCKET Bucket;

    TI_DbgPrint(DEBUG_TCP,("Called for %d bytes (on socket %x)\n",
                           SendLength, Connection->SocketContext));

    ASSERT_KM_POINTER(Connection->SocketContext);

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    TI_DbgPrint(DEBUG_TCP,("Connection = %x\n", Connection));
    TI_DbgPrint(DEBUG_TCP,("Connection->SocketContext = %x\n",
                           Connection->SocketContext));

    /* Closing */
    if (Connection->State & SEL_FIN)
    {
        TcpipRecursiveMutexLeave( &TCPLock );
        *BytesSent = 0;
        return STATUS_REMOTE_DISCONNECT;
    }

    Status = TCPTranslateError
        ( OskitTCPSend( Connection->SocketContext,
                        (OSK_PCHAR)BufferData, SendLength,
                        &Sent, 0 ) );

    TI_DbgPrint(DEBUG_TCP,("OskitTCPSend: %x, %d\n", Status, Sent));

    /* Keep this request around ... there was no data yet */
    if( Status == STATUS_PENDING ) {
        /* Freed in TCPSocketState */
        Bucket = exAllocatePool( NonPagedPool, sizeof(*Bucket) );
        if( !Bucket ) {
            TI_DbgPrint(DEBUG_TCP,("Failed to allocate bucket\n"));
            TcpipRecursiveMutexLeave( &TCPLock );
            return STATUS_NO_MEMORY;
        }
        
        Bucket->Request.RequestNotifyObject = Complete;
        Bucket->Request.RequestContext = Context;
        *BytesSent = 0;

        IoMarkIrpPending((PIRP)Context);
        
        InsertTailList( &Connection->SendRequest, &Bucket->Entry );
        TI_DbgPrint(DEBUG_TCP,("Queued write irp\n"));
    } else {
        TI_DbgPrint(DEBUG_TCP,("Got status %x, bytes %d\n", Status, Sent));
        *BytesSent = Sent;
    }
    
    TcpipRecursiveMutexLeave( &TCPLock );
    
    TI_DbgPrint(DEBUG_TCP,("Status %x\n", Status));

    return Status;
}

VOID TCPTimeout(VOID) {
    /* Now handled by TimerThread */
}

UINT TCPAllocatePort( UINT HintPort ) {
    if( HintPort ) {
        if( AllocatePort( &TCPPorts, HintPort ) ) return HintPort;
        else {
            TI_DbgPrint
                (MID_TRACE,("We got a hint port but couldn't allocate it\n"));
            return (UINT)-1;
        }
    } else return AllocatePortFromRange( &TCPPorts, 1024, 5000 );
}

VOID TCPFreePort( UINT Port ) {
    DeallocatePort( &TCPPorts, Port );
}

NTSTATUS TCPGetSockAddress
( PCONNECTION_ENDPOINT Connection,
  PTRANSPORT_ADDRESS Address,
  BOOLEAN GetRemote ) {
    OSK_UINT LocalAddress, RemoteAddress;
    OSK_UI16 LocalPort, RemotePort;
    PTA_IP_ADDRESS AddressIP = (PTA_IP_ADDRESS)Address;

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    OskitTCPGetAddress
        ( Connection->SocketContext,
          &LocalAddress, &LocalPort,
          &RemoteAddress, &RemotePort );

    AddressIP->TAAddressCount = 1;
    AddressIP->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    AddressIP->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    AddressIP->Address[0].Address[0].sin_port = GetRemote ? RemotePort : LocalPort;
    AddressIP->Address[0].Address[0].in_addr = GetRemote ? RemoteAddress : LocalAddress;

    TcpipRecursiveMutexLeave( &TCPLock );

    return STATUS_SUCCESS;
}

VOID TCPRemoveIRP( PCONNECTION_ENDPOINT Endpoint, PIRP Irp ) {
    PLIST_ENTRY Entry;
    PLIST_ENTRY ListHead[4];
    KIRQL OldIrql;
    PTDI_BUCKET Bucket;
    UINT i = 0;

    ListHead[0] = &Endpoint->SendRequest;
    ListHead[1] = &Endpoint->ReceiveRequest;
    ListHead[2] = &Endpoint->ConnectRequest;
    ListHead[3] = &Endpoint->ListenRequest;

    TcpipAcquireSpinLock( &Endpoint->Lock, &OldIrql );

    for( i = 0; i < 4; i++ )
    {
        for( Entry = ListHead[i]->Flink;
             Entry != ListHead[i];
             Entry = Entry->Flink )
        {
            Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
            if( Bucket->Request.RequestContext == Irp )
            {
                RemoveEntryList( &Bucket->Entry );
                exFreePool( Bucket );
                break;
            }
        }
    }

    TcpipReleaseSpinLock( &Endpoint->Lock, OldIrql );
}

/* EOF */
