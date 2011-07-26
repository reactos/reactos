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
PORT_SET TCPPorts;

#include "lwip/pbuf.h"
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lwip/arch.h"

#include "rosip.h"

VOID ConnectionFree(PVOID Object)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)Object;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_TCP, ("Freeing TCP Endpoint\n"));
    
    DbgPrint("CONNECTION ENDPOINT: Freeing 0x%x\n", Object);

    TcpipAcquireSpinLock(&ConnectionEndpointListLock, &OldIrql);
    RemoveEntryList(&Connection->ListEntry);
    TcpipReleaseSpinLock(&ConnectionEndpointListLock, OldIrql);

    ExFreePoolWithTag( Connection, CONN_ENDPT_TAG );
}

PCONNECTION_ENDPOINT TCPAllocateConnectionEndpoint( PVOID ClientContext )
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)
        ExAllocatePoolWithTag(NonPagedPool, sizeof(CONNECTION_ENDPOINT),
                              CONN_ENDPT_TAG);
    if (!Connection)
        return Connection;

    TI_DbgPrint(DEBUG_CPOINT, ("Connection point file object allocated at (0x%X).\n", Connection));

    RtlZeroMemory(Connection, sizeof(CONNECTION_ENDPOINT));

    /* Initialize spin lock that protects the connection endpoint file object */
    KeInitializeSpinLock(&Connection->Lock);
    InitializeListHead(&Connection->ConnectRequest);
    InitializeListHead(&Connection->ListenRequest);
    InitializeListHead(&Connection->ReceiveRequest);
    InitializeListHead(&Connection->SendRequest);
    InitializeListHead(&Connection->ShutdownRequest);
    InitializeListHead(&Connection->PacketQueue);

    /* Save client context pointer */
    Connection->ClientContext = ClientContext;

    Connection->RefCount = 1;
    Connection->Free = ConnectionFree;

    /* Add connection endpoint to global list */
    ExInterlockedInsertTailList(&ConnectionEndpointListHead,
                                &Connection->ListEntry,
                                &ConnectionEndpointListLock);

    return Connection;
}

NTSTATUS TCPSocket( PCONNECTION_ENDPOINT Connection,
                    UINT Family, UINT Type, UINT Proto )
{
    NTSTATUS Status;
    KIRQL OldIrql;

    LockObject(Connection, &OldIrql);

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPSocket] Called: Connection %x, Family %d, Type %d, "
                           "Proto %d, sizeof(CONNECTION_ENDPOINT) = %d\n",
                           Connection, Family, Type, Proto, sizeof(CONNECTION_ENDPOINT)));
    DbgPrint("[IP, TCPSocket] Called: Connection 0x%x, Family %d, Type %d, "
                           "Proto %d, sizeof(CONNECTION_ENDPOINT) = %d\n",
                           Connection, Family, Type, Proto, sizeof(CONNECTION_ENDPOINT));

    Connection->SocketContext = LibTCPSocket(Connection);
    if (Connection->SocketContext)
        Status = STATUS_SUCCESS;
    else
        Status = STATUS_INSUFFICIENT_RESOURCES;

    DbgPrint("[IP, TCPSocket] Connection->SocketContext = 0x%x\n", Connection->SocketContext);

    UnlockObject(Connection, OldIrql);

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPSocket] Leaving. Status = 0x%x\n", Status));
    DbgPrint("[IP, TCPSocket] Leaving. Status = 0x%x\n", Status);

    return Status;
}

NTSTATUS TCPClose
( PCONNECTION_ENDPOINT Connection )
{
    KIRQL OldIrql;
    PVOID Socket;

    LockObject(Connection, &OldIrql);

    DbgPrint("[IP, TCPClose] Called for Connection( 0x%x )->SocketConext( 0x%x )\n", Connection, Connection->SocketContext);

    Socket = Connection->SocketContext;
    //Connection->SocketContext = NULL;

    /* We should not be associated to an address file at this point */
    ASSERT(!Connection->AddressFile);

    /* Don't try to close again if the other side closed us already */
    if (Connection->SocketContext)
    {
        FlushAllQueues(Connection, STATUS_CANCELLED);

        DbgPrint("[IP, TCPClose] Socket (pcb) = 0x%x\n", Socket);

        LibTCPClose(Connection, FALSE);
    }

    DbgPrint("[IP, TCPClose] Leaving. Connection->RefCount = %d\n", Connection->RefCount);

    UnlockObject(Connection, OldIrql);

    DereferenceObject(Connection);

    return STATUS_SUCCESS;
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
    DbgPrint("[IP, TCPReceive] Called. Got packet from network stack\n");

    TI_DbgPrint(DEBUG_TCP,("Sending packet %d (%d) to lwIP\n",
                           IPPacket->TotalSize,
                           IPPacket->HeaderSize));
    
    LibIPInsertPacket(Interface->TCPContext, IPPacket->Header, IPPacket->TotalSize);

    DbgPrint("[IP, TCPReceive] Leaving\n");
}

NTSTATUS TCPStartup(VOID)
/*
 * FUNCTION: Initializes the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;

    Status = PortsStartup( &TCPPorts, 1, 0xfffe );
    if( !NT_SUCCESS(Status) )
    {
        return Status;
    }
    
    /* Initialize our IP library */
    LibIPInitialize();
    
    /* Register this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, TCPReceive);
    
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
    if (!TCPInitialized)
        return STATUS_SUCCESS;
    
    LibIPShutdown();

    /* Deregister this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, NULL);

    TCPInitialized = FALSE;

    PortsShutdown( &TCPPorts );

    return STATUS_SUCCESS;
}

NTSTATUS TCPTranslateError( err_t err )
{
    NTSTATUS Status;

    switch (err)
    {
        case ERR_OK: Status = STATUS_SUCCESS; break; //0
        case ERR_MEM: Status = STATUS_INSUFFICIENT_RESOURCES; break; //-1
        case ERR_BUF: Status = STATUS_BUFFER_TOO_SMALL; break; //-2
        case ERR_TIMEOUT: Status = STATUS_TIMEOUT; break; // -3
        case ERR_RTE: Status = STATUS_HOST_UNREACHABLE; break; //-4
        case ERR_ABRT: Status = STATUS_LOCAL_DISCONNECT; break; //-5
        case ERR_RST: Status = STATUS_REMOTE_DISCONNECT; break; //-6
        case ERR_CLSD: Status = STATUS_FILE_CLOSED; break; //-7
        case ERR_CONN: Status = STATUS_UNSUCCESSFUL; break; //-8 (FIXME)
        case ERR_VAL: Status = STATUS_INVALID_PARAMETER; break; //-9
        case ERR_ARG: Status = STATUS_INVALID_PARAMETER; break; //-10
        case ERR_USE: Status = STATUS_ADDRESS_ALREADY_EXISTS; break; //-11
        case ERR_IF: Status = STATUS_NETWORK_UNREACHABLE; break; //-12
        case ERR_ISCONN: Status = STATUS_UNSUCCESSFUL; break; //-13 (FIXME)
        case ERR_INPROGRESS: Status = STATUS_PENDING; break; //-14
        default:
            DbgPrint("Invalid error value: %d\n", err);
            ASSERT(FALSE);
            Status = STATUS_UNSUCCESSFUL;
            break;
    }
    
    DbgPrint("TCPTranslateError: %d -> %x\n", (unsigned int)err, Status);
    
    return Status;
}

NTSTATUS TCPConnect
( PCONNECTION_ENDPOINT Connection,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context )
{
    NTSTATUS Status;
    struct ip_addr bindaddr, connaddr;
    IP_ADDRESS RemoteAddress;
    USHORT RemotePort;
    TA_IP_ADDRESS LocalAddress;
    PTDI_BUCKET Bucket;
    PNEIGHBOR_CACHE_ENTRY NCE;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPConnect] Called\n"));

    Status = AddrBuildAddress
        ((PTRANSPORT_ADDRESS)ConnInfo->RemoteAddress,
         &RemoteAddress,
         &RemotePort);

    if (!NT_SUCCESS(Status))
    {
        TI_DbgPrint(DEBUG_TCP, ("Could not AddrBuildAddress in TCPConnect\n"));
        return Status;
    }

    /* Freed in TCPSocketState */
    TI_DbgPrint(DEBUG_TCP,
                ("Connecting to address %x:%x\n",
                 RemoteAddress.Address.IPv4Address,
                 RemotePort));

    LockObject(Connection, &OldIrql);

    if (!Connection->AddressFile)
    {
        UnlockObject(Connection, OldIrql);
        return STATUS_INVALID_PARAMETER;
    }

    if (AddrIsUnspecified(&Connection->AddressFile->Address))
    {
        if (!(NCE = RouteGetRouteToDestination(&RemoteAddress)))
        {
            UnlockObject(Connection, OldIrql);
            return STATUS_NETWORK_UNREACHABLE;
        }

        bindaddr.addr = NCE->Interface->Unicast.Address.IPv4Address;
    }
    else
    {
        bindaddr.addr = Connection->AddressFile->Address.Address.IPv4Address;
    }

    Status = TCPTranslateError(LibTCPBind(Connection,
                                          &bindaddr,
                                          Connection->AddressFile->Port));
    
    DbgPrint("LibTCPBind: 0x%x\n", Status);

    if (NT_SUCCESS(Status))
    {
        /* Check if we had an unspecified port */
        if (!Connection->AddressFile->Port)
        {
            /* We did, so we need to copy back the port */
            Status = TCPGetSockAddress(Connection, (PTRANSPORT_ADDRESS)&LocalAddress, FALSE);
            if (NT_SUCCESS(Status))
            {
                /* Allocate the port in the port bitmap */
                Connection->AddressFile->Port = TCPAllocatePort(LocalAddress.Address[0].Address[0].sin_port);
                    
                /* This should never fail */
                ASSERT(Connection->AddressFile->Port != 0xFFFF);
            }
        }

        if (NT_SUCCESS(Status))
        {
            connaddr.addr = RemoteAddress.Address.IPv4Address;

            Bucket = ExAllocatePoolWithTag( NonPagedPool, sizeof(*Bucket), TDI_BUCKET_TAG );
            if( !Bucket )
            {
                UnlockObject(Connection, OldIrql);
                return STATUS_NO_MEMORY;
            }
            
            Bucket->Request.RequestNotifyObject = (PVOID)Complete;
            Bucket->Request.RequestContext = Context;
			
            InsertTailList( &Connection->ConnectRequest, &Bucket->Entry );
        
            Status = TCPTranslateError(LibTCPConnect(Connection,
                                                     &connaddr,
                                                     RemotePort));
        
            DbgPrint("LibTCPConnect: 0x%x\n", Status);
        }
    }

    UnlockObject(Connection, OldIrql);

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPConnect] Leaving. Status = 0x%x\n", Status));

    return Status;
}

NTSTATUS TCPDisconnect
( PCONNECTION_ENDPOINT Connection,
  UINT Flags,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPDisconnect] Called\n"));

    LockObject(Connection, &OldIrql);

    if (Connection->SocketContext)
    {
        if (Flags & TDI_DISCONNECT_RELEASE)
        {
            Status = TCPTranslateError(LibTCPShutdown(Connection, 0, 1));
        }

        if ((Flags & TDI_DISCONNECT_ABORT) || !Flags)
        {
            Status = TCPTranslateError(LibTCPShutdown(Connection, 1, 1));
        }
    }
    else
    {
        /* We already got closed by the other side so just return success */
        DbgPrint("[IP, TCPDisconnect] Socket was alraedy clsoed on the other side\n");
        Status = STATUS_SUCCESS;
    }
    
    DbgPrint("LibTCPShutdown: %x\n", Status);

    UnlockObject(Connection, OldIrql);

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPDisconnect] Leaving. Status = 0x%x\n", Status));

    return Status;
}

NTSTATUS TCPReceiveData
( PCONNECTION_ENDPOINT Connection,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  PULONG BytesReceived,
  ULONG ReceiveFlags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context )
{
    PTDI_BUCKET Bucket;
    KIRQL OldIrql;
    PUCHAR DataBuffer;
    UINT DataLen, Received;
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPReceiveData] Called for %d bytes (on socket %x)\n",
                           ReceiveLength, Connection->SocketContext));

    DbgPrint("[IP, TCPReceiveData] Called for %d bytes (on Connection->SocketContext = 0x%x)\n",
                           ReceiveLength, Connection->SocketContext);

    NdisQueryBuffer(Buffer, &DataBuffer, &DataLen);
    
    Status = LibTCPGetDataFromConnectionQueue(Connection, DataBuffer, DataLen, &Received);

    if (Status == STATUS_PENDING)
    {
        LockObject(Connection, &OldIrql);
    
        /* Freed in TCPSocketState */
        Bucket = ExAllocatePoolWithTag( NonPagedPool, sizeof(*Bucket), TDI_BUCKET_TAG );
        if (!Bucket)
        {
            TI_DbgPrint(DEBUG_TCP,("[IP, TCPReceiveData] Failed to allocate bucket\n"));
            UnlockObject(Connection, OldIrql);

            return STATUS_NO_MEMORY;
        }
    
        Bucket->Request.RequestNotifyObject = Complete;
        Bucket->Request.RequestContext = Context;

        InsertTailList( &Connection->ReceiveRequest, &Bucket->Entry );
        TI_DbgPrint(DEBUG_TCP,("[IP, TCPReceiveData] Queued read irp\n"));

        UnlockObject(Connection, OldIrql);

        TI_DbgPrint(DEBUG_TCP,("[IP, TCPReceiveData] Leaving. Status = STATUS_PENDING\n"));

        (*BytesReceived) = 0;
    }
    else
    {
        (*BytesReceived) = Received;
    }

    DbgPrint("[IP, TCPReceiveData] Leaving. Status = %s\n",
        Status == STATUS_PENDING? "STATUS_PENDING" : "STATUS_SUCCESS");

    return Status;
}

NTSTATUS TCPSendData
( PCONNECTION_ENDPOINT Connection,
  PCHAR BufferData,
  ULONG SendLength,
  PULONG BytesSent,
  ULONG Flags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context )
{
    NTSTATUS Status;
    PTDI_BUCKET Bucket;
    KIRQL OldIrql;

    LockObject(Connection, &OldIrql);

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPSendData] Called for %d bytes (on socket %x)\n",
                           SendLength, Connection->SocketContext));

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPSendData] Connection = %x\n", Connection));
    TI_DbgPrint(DEBUG_TCP,("[IP, TCPSendData] Connection->SocketContext = %x\n",
                           Connection->SocketContext));
    DbgPrint("[IP, TCPSendData] Called\n");

    Status = TCPTranslateError(LibTCPSend(Connection,
                                          BufferData,
                                          SendLength,
                                          FALSE));
    
    DbgPrint("[IP, TCPSendData] LibTCPSend: 0x%x\n", Status);

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPSendData] Send: %x, %d\n", Status, SendLength));

    /* Keep this request around ... there was no data yet */
    if( Status == STATUS_PENDING )
    {
        /* Freed in TCPSocketState */
        Bucket = ExAllocatePoolWithTag( NonPagedPool, sizeof(*Bucket), TDI_BUCKET_TAG );
        if( !Bucket )
        {
            UnlockObject(Connection, OldIrql);
            TI_DbgPrint(DEBUG_TCP,("[IP, TCPSendData] Failed to allocate bucket\n"));
            return STATUS_NO_MEMORY;
        }
        
        Bucket->Request.RequestNotifyObject = Complete;
        Bucket->Request.RequestContext = Context;
        *BytesSent = 0;
        
        InsertTailList( &Connection->SendRequest, &Bucket->Entry );
        TI_DbgPrint(DEBUG_TCP,("[IP, TCPSendData] Queued write irp\n"));
    }
    else
    {
        TI_DbgPrint(DEBUG_TCP,("[IP, TCPSendData] Got status %x, bytes %d\n",
                    Status, SendLength));
        *BytesSent = SendLength;
    }

    UnlockObject(Connection, OldIrql);

    TI_DbgPrint(DEBUG_TCP, ("[IP, TCPSendData] Leaving. Status = %x\n", Status));

    DbgPrint("[IP, TCPSendData] Leaving. Status = %x\n", Status);

    return Status;
}

UINT TCPAllocatePort( UINT HintPort )
{
    if( HintPort )
    {
        if( AllocatePort( &TCPPorts, HintPort ) )
            return HintPort;
        else
        {
            TI_DbgPrint
                (MID_TRACE,("We got a hint port but couldn't allocate it\n"));
            return (UINT)-1;
        }
    }
    else
        return AllocatePortFromRange( &TCPPorts, 1024, 5000 );
}

VOID TCPFreePort( UINT Port )
{
    DeallocatePort( &TCPPorts, Port );
}

NTSTATUS TCPGetSockAddress
( PCONNECTION_ENDPOINT Connection,
  PTRANSPORT_ADDRESS Address,
  BOOLEAN GetRemote )
{
    PTA_IP_ADDRESS AddressIP = (PTA_IP_ADDRESS)Address;
    struct ip_addr ipaddr;
    NTSTATUS Status;
    KIRQL OldIrql;
    
    AddressIP->TAAddressCount = 1;
    AddressIP->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    AddressIP->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;

    LockObject(Connection, &OldIrql);

    if (GetRemote)
    {
        Status = TCPTranslateError(LibTCPGetPeerName(Connection->SocketContext,
                                    &ipaddr,
                                    &AddressIP->Address[0].Address[0].sin_port));
    }
    else
    {
        Status = TCPTranslateError(LibTCPGetHostName(Connection->SocketContext,
                                    &ipaddr,
                                    &AddressIP->Address[0].Address[0].sin_port));
    }

    UnlockObject(Connection, OldIrql);
    
    AddressIP->Address[0].Address[0].in_addr = ipaddr.addr;

    DbgPrint("LibTCPGetXXXName: 0x%x\n", Status);

    return Status;
}

BOOLEAN TCPRemoveIRP( PCONNECTION_ENDPOINT Endpoint, PIRP Irp )
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY ListHead[4];
    KIRQL OldIrql;
    PTDI_BUCKET Bucket;
    UINT i = 0;
    BOOLEAN Found = FALSE;

    ListHead[0] = &Endpoint->SendRequest;
    ListHead[1] = &Endpoint->ReceiveRequest;
    ListHead[2] = &Endpoint->ConnectRequest;
    ListHead[3] = &Endpoint->ListenRequest;

    LockObject(Endpoint, &OldIrql);

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
                ExFreePoolWithTag( Bucket, TDI_BUCKET_TAG );
                Found = TRUE;
                break;
            }
        }
    }

    UnlockObject(Endpoint, OldIrql);

    return Found;
}

/* EOF */
