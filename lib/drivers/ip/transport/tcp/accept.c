/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/accept.c
 * PURPOSE:     Transmission Control Protocol Listen/Accept code
 * PROGRAMMERS: Art Yerkes (arty@users.sf.net)
 * REVISIONS:
 *   arty 12/21/2004 Created
 */

#include "precomp.h"

#include "rosip.h"

NTSTATUS TCPCheckPeerForAccept(PVOID Context,
                               PTDI_REQUEST_KERNEL Request)
{
    struct tcp_pcb *newpcb = Context;
    NTSTATUS Status;
    PTDI_CONNECTION_INFORMATION WhoIsConnecting;
    PTA_IP_ADDRESS RemoteAddress;
    struct ip_addr ipaddr;

    DbgPrint("[IP, TCPCheckPeerForAccept] Called\n");
    
    if (Request->RequestFlags & TDI_QUERY_ACCEPT)
        DbgPrint("TDI_QUERY_ACCEPT NOT SUPPORTED!!!\n");

    WhoIsConnecting = (PTDI_CONNECTION_INFORMATION)Request->ReturnConnectionInformation;
    RemoteAddress = (PTA_IP_ADDRESS)WhoIsConnecting->RemoteAddress;
    
    RemoteAddress->TAAddressCount = 1;
    RemoteAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    RemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    
    Status = TCPTranslateError(LibTCPGetPeerName(newpcb,
                                                 &ipaddr,
                                                 &RemoteAddress->Address[0].Address[0].sin_port));
    
    RemoteAddress->Address[0].Address[0].in_addr = ipaddr.addr;
    
    DbgPrint("[IP, TCPCheckPeerForAccept] Leaving. Status %x\n", Status);

    return Status;
}

/* This listen is on a socket we keep as internal.  That socket has the same
 * lifetime as the address file */
NTSTATUS TCPListen( PCONNECTION_ENDPOINT Connection, UINT Backlog )
{
    NTSTATUS Status = STATUS_SUCCESS;
    struct ip_addr AddressToBind;
    KIRQL OldIrql;

    ASSERT(Connection);
    ASSERT_KM_POINTER(Connection->AddressFile);

    LockObject(Connection, &OldIrql);

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPListen] Called\n"));
    DbgPrint("[IP, TCPListen] Called\n");

    TI_DbgPrint(DEBUG_TCP, ("Connection->SocketContext %x\n",
        Connection->SocketContext));
    
    AddressToBind.addr = Connection->AddressFile->Address.Address.IPv4Address;

    Status = TCPTranslateError(LibTCPBind(Connection->SocketContext,
                                          &AddressToBind,
                                          Connection->AddressFile->Port));

    if (NT_SUCCESS(Status))
    {
        Connection->SocketContext = LibTCPListen(Connection->SocketContext, Backlog);
        if (!Connection->SocketContext)
            Status = STATUS_UNSUCCESSFUL;
    }

    UnlockObject(Connection, OldIrql);

    TI_DbgPrint(DEBUG_TCP,("[IP, TCPListen] Leaving. Status = %x\n", Status));
    DbgPrint("[IP, TCPListen] Leaving. Status = %x\n", Status);

    return Status;
}

BOOLEAN TCPAbortListenForSocket( PCONNECTION_ENDPOINT Listener,
                  PCONNECTION_ENDPOINT Connection )
{
    PLIST_ENTRY ListEntry;
    PTDI_BUCKET Bucket;
    KIRQL OldIrql;
    BOOLEAN Found = FALSE;

    DbgPrint("[IP, TCPAbortListenForSocket] Called\n");

    LockObject(Listener, &OldIrql);

    ListEntry = Listener->ListenRequest.Flink;
    while ( ListEntry != &Listener->ListenRequest )
    {
        Bucket = CONTAINING_RECORD(ListEntry, TDI_BUCKET, Entry);

        if (Bucket->AssociatedEndpoint == Connection)
        {
            DereferenceObject(Bucket->AssociatedEndpoint);
            RemoveEntryList( &Bucket->Entry );
            ExFreePoolWithTag( Bucket, TDI_BUCKET_TAG );
            Found = TRUE;
            break;
        }

        ListEntry = ListEntry->Flink;
    }

    UnlockObject(Listener, OldIrql);

    DbgPrint("[IP, TCPAbortListenForSocket] Leaving. Status = %s\n",
        Found == TRUE ? "TRUE" : "FALSE");

    return Found;
}

NTSTATUS TCPAccept ( PTDI_REQUEST Request,
                     PCONNECTION_ENDPOINT Listener,
                     PCONNECTION_ENDPOINT Connection,
                     PTCP_COMPLETION_ROUTINE Complete,
                     PVOID Context )
{
    NTSTATUS Status;
    PTDI_BUCKET Bucket;
    KIRQL OldIrql;

    DbgPrint("[IP, TCPAccept] Called\n");

    LockObject(Listener, &OldIrql);

    Bucket = ExAllocatePoolWithTag( NonPagedPool, sizeof(*Bucket),
                                   TDI_BUCKET_TAG );
    
    if( Bucket )
    {
        Bucket->AssociatedEndpoint = Connection;
        ReferenceObject(Bucket->AssociatedEndpoint);

        Bucket->Request.RequestNotifyObject = Complete;
        Bucket->Request.RequestContext = Context;
        InsertTailList( &Listener->ListenRequest, &Bucket->Entry );
        Status = STATUS_PENDING;
    }
    else
        Status = STATUS_NO_MEMORY;

    UnlockObject(Listener, OldIrql);

    DbgPrint("[IP, TCPAccept] Leaving. Status = %x\n", Status);
    return Status;
}
