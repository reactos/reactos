/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/event.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "precomp.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/api.h"

#include <lwip_glue/lwip_glue.h>

extern NPAGED_LOOKASIDE_LIST TdiBucketLookasideList;

static
VOID
BucketCompletionWorker(PVOID Context)
{
    PTDI_BUCKET Bucket = (PTDI_BUCKET)Context;
    PTCP_COMPLETION_ROUTINE Complete;

    Complete = (PTCP_COMPLETION_ROUTINE)Bucket->Request.RequestNotifyObject;

    Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);

    DereferenceObject(Bucket->AssociatedEndpoint);

    ExFreeToNPagedLookasideList(&TdiBucketLookasideList, Bucket);
}

VOID
CompleteBucket(PCONNECTION_ENDPOINT Connection, PTDI_BUCKET Bucket, const BOOLEAN Synchronous)
{
    ReferenceObject(Connection);
    Bucket->AssociatedEndpoint = Connection;
    if (Synchronous)
    {
        BucketCompletionWorker(Bucket);
    }
    else
    {
        ChewCreate(BucketCompletionWorker, Bucket);
    }
}

VOID
FlushReceiveQueue(PCONNECTION_ENDPOINT Connection, const NTSTATUS Status)
{
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    ASSERT_TCPIP_OBJECT_LOCKED(Connection);

    while (!IsListEmpty(&Connection->ReceiveRequest))
    {
        Entry = RemoveHeadList(&Connection->ReceiveRequest);

        Bucket = CONTAINING_RECORD(Entry, TDI_BUCKET, Entry);

        Bucket->Information = 0;
        Bucket->Status = Status;

        CompleteBucket(Connection, Bucket, FALSE);
    }
}

VOID
FlushSendQueue(PCONNECTION_ENDPOINT Connection, const NTSTATUS Status)
{
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    ASSERT_TCPIP_OBJECT_LOCKED(Connection);

    while (!IsListEmpty(&Connection->SendRequest))
    {
        Entry = RemoveHeadList(&Connection->SendRequest);

        Bucket = CONTAINING_RECORD(Entry, TDI_BUCKET, Entry);

        Bucket->Information = 0;
        Bucket->Status = Status;

        CompleteBucket(Connection, Bucket, FALSE);
    }
}

VOID
FlushShutdownQueue(PCONNECTION_ENDPOINT Connection, const NTSTATUS Status)
{
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    ASSERT_TCPIP_OBJECT_LOCKED(Connection);

    while (!IsListEmpty(&Connection->ShutdownRequest))
    {
        Entry = RemoveHeadList(&Connection->ShutdownRequest);

        Bucket = CONTAINING_RECORD(Entry, TDI_BUCKET, Entry);

        Bucket->Information = 0;
        Bucket->Status = Status;

        CompleteBucket(Connection, Bucket, FALSE);
    }
}

VOID
FlushConnectQueue(PCONNECTION_ENDPOINT Connection, const NTSTATUS Status)
{
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    ASSERT_TCPIP_OBJECT_LOCKED(Connection);

    while (!IsListEmpty(&Connection->ConnectRequest))
    {
        Entry = RemoveHeadList(&Connection->ConnectRequest);
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

        Bucket->Status = Status;
        Bucket->Information = 0;

        CompleteBucket(Connection, Bucket, FALSE);
    }
}

VOID
FlushListenQueue(PCONNECTION_ENDPOINT Connection, const NTSTATUS Status)
{
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    ASSERT_TCPIP_OBJECT_LOCKED(Connection);

    while (!IsListEmpty(&Connection->ListenRequest))
    {
        Entry = RemoveHeadList(&Connection->ListenRequest);
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

        Bucket->Status = Status;
        Bucket->Information = 0;

        DereferenceObject(Bucket->AssociatedEndpoint);
        CompleteBucket(Connection, Bucket, FALSE);
    }
}

VOID
FlushAllQueues(PCONNECTION_ENDPOINT Connection, NTSTATUS Status)
{
    // flush receive queue
    FlushReceiveQueue(Connection, Status);

    /* We completed the reads successfully but we need to return failure now */
    if (Status == STATUS_SUCCESS)
    {
        Status = STATUS_FILE_CLOSED;
    }

    // flush listen queue
    FlushListenQueue(Connection, Status);

    // flush send queue
    FlushSendQueue(Connection, Status);

    // flush connect queue
    FlushConnectQueue(Connection, Status);

    // flush shutdown queue
    FlushShutdownQueue(Connection, Status);
}

VOID
TCPFinEventHandler(void *arg, const err_t err)
{
   PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg, LastConnection;
   const NTSTATUS Status = TCPTranslateError(err);

   ASSERT(Connection->SocketContext == NULL);
   ASSERT(Connection->AddressFile);
   ASSERT(err != ERR_OK);

   LockObject(Connection);

   /* Complete all outstanding requests now */
   FlushAllQueues(Connection, Status);

   LockObject(Connection->AddressFile);

   /* Unlink this connection from the address file */
   if (Connection->AddressFile->Connection == Connection)
   {
       Connection->AddressFile->Connection = Connection->Next;
       DereferenceObject(Connection);
   }
   else if (Connection->AddressFile->Listener == Connection)
   {
       Connection->AddressFile->Listener = NULL;
       DereferenceObject(Connection);
   }
   else
   {
       LastConnection = Connection->AddressFile->Connection;
       while (LastConnection->Next != Connection && LastConnection->Next != NULL)
           LastConnection = LastConnection->Next;
       if (LastConnection->Next == Connection)
       {
           LastConnection->Next = Connection->Next;
           DereferenceObject(Connection);
       }
   }

   UnlockObject(Connection->AddressFile);

   /* Remove the address file from this connection */
   DereferenceObject(Connection->AddressFile);
   Connection->AddressFile = NULL;

   UnlockObject(Connection);
}

VOID
TCPAcceptEventHandler(void *arg, PTCP_PCB newpcb)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;

    LockObject(Connection);

    while (!IsListEmpty(&Connection->ListenRequest))
    {
        PIO_STACK_LOCATION IrpSp;

        Entry = RemoveHeadList(&Connection->ListenRequest);

        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

        Irp = Bucket->Request.RequestContext;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        TI_DbgPrint(DEBUG_TCP,("[IP, TCPAcceptEventHandler] Getting the socket\n"));

        Status = TCPCheckPeerForAccept(newpcb,
                                       (PTDI_REQUEST_KERNEL)&IrpSp->Parameters);

        TI_DbgPrint(DEBUG_TCP,("Socket: Status: %x\n", Status));

        Bucket->Status = Status;
        Bucket->Information = 0;

        if (Status == STATUS_SUCCESS)
        {
            LockObject(Bucket->AssociatedEndpoint);

            /* sanity assert...this should never be in anything else but a CLOSED state */
            ASSERT( ((PTCP_PCB)Bucket->AssociatedEndpoint->SocketContext)->state == CLOSED );

            /*  free socket context created in FileOpenConnection, as we're using a new one */
            LibTCPClose(Bucket->AssociatedEndpoint, TRUE, FALSE);

            /* free previously created socket context (we don't use it, we use newpcb) */
            Bucket->AssociatedEndpoint->SocketContext = newpcb;

            UnlockObject(Bucket->AssociatedEndpoint);

            LibTCPAccept(newpcb, (PTCP_PCB)Connection->SocketContext, Bucket->AssociatedEndpoint);
        }

        DereferenceObject(Bucket->AssociatedEndpoint);

        CompleteBucket(Connection, Bucket, FALSE);

        if (Status == STATUS_SUCCESS)
        {
            break;
        }
    }

    UnlockObject(Connection);
}

VOID
TCPSendEventHandler(void *arg, const u16_t space)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;
    PMDL Mdl;
    ULONG BytesSent;

    ReferenceObject(Connection);
    LockObject(Connection);

    while (!IsListEmpty(&Connection->SendRequest))
    {
        UINT SendLen = 0;
        PVOID SendBuffer = 0;

        Entry = RemoveHeadList(&Connection->SendRequest);

        UnlockObject(Connection);

        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

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

        Status = TCPTranslateError(LibTCPSend(Connection,
                                              SendBuffer,
                                              SendLen, &BytesSent, TRUE));

        TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", BytesSent));

        if( Status == STATUS_PENDING )
        {
            LockObject(Connection);
            InsertHeadList(&Connection->SendRequest, &Bucket->Entry);
            break;
        }
        else
        {
            TI_DbgPrint(DEBUG_TCP,
                        ("Completing Send request: %x %x\n",
                         Bucket->Request, Status));

            Bucket->Status = Status;
            Bucket->Information = (Bucket->Status == STATUS_SUCCESS) ? BytesSent : 0;

            CompleteBucket(Connection, Bucket, FALSE);
        }

        LockObject(Connection);
    }

    //  If we completed all outstanding send requests then finish all pending shutdown requests,
    //  cancel the timer and dereference the connection
    if (IsListEmpty(&Connection->SendRequest))
    {
        FlushShutdownQueue(Connection, STATUS_SUCCESS);

        if (KeCancelTimer(&Connection->DisconnectTimer))
        {
            DereferenceObject(Connection);
        }
    }

    UnlockObject(Connection);

    DereferenceObject(Connection);
}

VOID
TCPRecvEventHandler(void *arg)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    PMDL Mdl;
    UINT Received;
    UINT RecvLen;
    PUCHAR RecvBuffer;
    NTSTATUS Status;

    LockObject(Connection);

    while(!IsListEmpty(&Connection->ReceiveRequest))
    {
        Entry = RemoveHeadList(&Connection->ReceiveRequest);
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

        Irp = Bucket->Request.RequestContext;
        Mdl = Irp->MdlAddress;

        NdisQueryBuffer( Mdl, &RecvBuffer, &RecvLen );

        Status = LibTCPGetDataFromConnectionQueue(Connection, RecvBuffer, RecvLen, &Received);
        if (Status == STATUS_PENDING)
        {
            InsertHeadList(&Connection->ReceiveRequest, &Bucket->Entry);
            break;
        }

        Bucket->Status = Status;
        Bucket->Information = Received;

        CompleteBucket(Connection, Bucket, FALSE);
    }
    UnlockObject(Connection);
}

VOID
TCPConnectEventHandler(void *arg, const err_t err)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    LockObject(Connection);

    while (!IsListEmpty(&Connection->ConnectRequest))
    {
        Entry = RemoveHeadList(&Connection->ConnectRequest);

        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

        Bucket->Status = TCPTranslateError(err);
        Bucket->Information = 0;

        CompleteBucket(Connection, Bucket, FALSE);
    }

    UnlockObject(Connection);
}
