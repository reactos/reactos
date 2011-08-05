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

#include "rosip.h"

static const char * const tcp_state_str[] = {
  "CLOSED",      
  "LISTEN",      
  "SYN_SENT",    
  "SYN_RCVD",    
  "ESTABLISHED", 
  "FIN_WAIT_1",  
  "FIN_WAIT_2",  
  "CLOSE_WAIT",  
  "CLOSING",     
  "LAST_ACK",    
  "TIME_WAIT"   
};

static
VOID
BucketCompletionWorker(PVOID Context)
{
    PTDI_BUCKET Bucket = (PTDI_BUCKET)Context;
    PTCP_COMPLETION_ROUTINE Complete;
    
    Complete = (PTCP_COMPLETION_ROUTINE)Bucket->Request.RequestNotifyObject;
    
    Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
    
    DereferenceObject(Bucket->AssociatedEndpoint);

    ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
}

static
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
    
    ReferenceObject(Connection);
        
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ReceiveRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        TI_DbgPrint(DEBUG_TCP,
                    ("Completing Receive request: %x %x\n",
                     Bucket->Request, Status));
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        CompleteBucket(Connection, Bucket, FALSE);
    }

    DereferenceObject(Connection);
}

VOID
FlushAllQueues(PCONNECTION_ENDPOINT Connection, NTSTATUS Status)
{
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    
    ReferenceObject(Connection);
        
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ReceiveRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        TI_DbgPrint(DEBUG_TCP,
                    ("Completing Receive request: %x %x\n",
                     Bucket->Request, Status));
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        CompleteBucket(Connection, Bucket, FALSE);
    }

    /* We completed the reads successfully but we need to return failure now */
    if (Status == STATUS_SUCCESS)
    {
        Status = STATUS_FILE_CLOSED;
    }
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ListenRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        DereferenceObject(Bucket->AssociatedEndpoint);
        CompleteBucket(Connection, Bucket, FALSE);
    }
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->SendRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );    
        
        TI_DbgPrint(DEBUG_TCP,
                    ("Completing Send request: %x %x\n",
                     Bucket->Request, Status));
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        CompleteBucket(Connection, Bucket, FALSE);
    }
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ConnectRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        CompleteBucket(Connection, Bucket, FALSE);
    }
    
    DereferenceObject(Connection);
}

VOID
TCPFinEventHandler(void *arg, const err_t err)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg, LastConnection;
    const NTSTATUS Status = TCPTranslateError(err);
    KIRQL OldIrql;

    ASSERT(Connection->AddressFile);

    /* Check if this was a partial socket closure */
    if (err == ERR_OK && Connection->SocketContext)
    {
        /* Just flush the receive queue and get out of here */
        FlushReceiveQueue(Connection, STATUS_SUCCESS);
    }
    else
    {
        /* First off all, remove the PCB pointer */
        Connection->SocketContext = NULL;

        /* Complete all outstanding requests now */
        FlushAllQueues(Connection, Status);

        LockObject(Connection, &OldIrql);

        LockObjectAtDpcLevel(Connection->AddressFile);

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

        UnlockObjectFromDpcLevel(Connection->AddressFile);

        /* Remove the address file from this connection */
        DereferenceObject(Connection->AddressFile);
        Connection->AddressFile = NULL;

        UnlockObject(Connection, OldIrql);
    }
}
    
VOID
TCPAcceptEventHandler(void *arg, PTCP_PCB newpcb)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;
    KIRQL OldIrql;
        
    ReferenceObject(Connection);
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ListenRequest, &Connection->Lock)))
    {
        PIO_STACK_LOCATION IrpSp;
        
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
            LockObject(Bucket->AssociatedEndpoint, &OldIrql);

            /* sanity assert...this should never be in anything else but a CLOSED state */
            ASSERT( ((PTCP_PCB)Bucket->AssociatedEndpoint->SocketContext)->state == CLOSED );
            
            /*  free socket context created in FileOpenConnection, as we're using a new one */
            LibTCPClose(Bucket->AssociatedEndpoint, TRUE, FALSE);

            /* free previously created socket context (we don't use it, we use newpcb) */
            Bucket->AssociatedEndpoint->SocketContext = newpcb;
            
            LibTCPAccept(newpcb, (PTCP_PCB)Connection->SocketContext, Bucket->AssociatedEndpoint);

            UnlockObject(Bucket->AssociatedEndpoint, OldIrql);
        }
        
        DereferenceObject(Bucket->AssociatedEndpoint);
        
        CompleteBucket(Connection, Bucket, FALSE);
    }
    
    DereferenceObject(Connection);
}

VOID
TCPSendEventHandler(void *arg, u16_t space)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;
    PMDL Mdl;
    
    ReferenceObject(Connection);

    while ((Entry = ExInterlockedRemoveHeadList(&Connection->SendRequest, &Connection->Lock)))
    {
        UINT SendLen = 0;
        PVOID SendBuffer = 0;
        
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
                                              SendLen, TRUE));
        
        TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", SendLen));
        
        if( Status == STATUS_PENDING )
        {
            ExInterlockedInsertHeadList(&Connection->SendRequest,
                                        &Bucket->Entry,
                                        &Connection->Lock);
            break;
        }
        else
        {
            TI_DbgPrint(DEBUG_TCP,
                        ("Completing Send request: %x %x\n",
                         Bucket->Request, Status));
            
            Bucket->Status = Status;
            Bucket->Information = (Bucket->Status == STATUS_SUCCESS) ? SendLen : 0;
                        
            CompleteBucket(Connection, Bucket, FALSE);
        }
    }
    
    DereferenceObject(Connection);
}

u32_t
TCPRecvEventHandler(void *arg, struct pbuf *p)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    PMDL Mdl;
    UINT Received = 0;
    UINT RecvLen;
    PUCHAR RecvBuffer;
    
    ASSERT(p);
    
    ReferenceObject(Connection);
        
    if ((Entry = ExInterlockedRemoveHeadList(&Connection->ReceiveRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Irp = Bucket->Request.RequestContext;
        Mdl = Irp->MdlAddress;
        
        TI_DbgPrint(DEBUG_TCP,
                    ("[IP, TCPRecvEventHandler] Getting the user buffer from %x\n", Mdl));
        
        NdisQueryBuffer( Mdl, &RecvBuffer, &RecvLen );
        
        TI_DbgPrint(DEBUG_TCP,
                    ("[IP, TCPRecvEventHandler] Reading %d bytes to %x\n", RecvLen, RecvBuffer));
        
        TI_DbgPrint(DEBUG_TCP, ("Connection: %x\n", Connection));
        TI_DbgPrint(DEBUG_TCP, ("[IP, TCPRecvEventHandler] Connection->SocketContext: %x\n", Connection->SocketContext));
        TI_DbgPrint(DEBUG_TCP, ("[IP, TCPRecvEventHandler] RecvBuffer: %x\n", RecvBuffer));
        
        RecvLen = MIN(p->tot_len, RecvLen);
        
        for (Received = 0; Received < RecvLen; Received += p->len, p = p->next)
        {
            RtlCopyMemory(RecvBuffer + Received, p->payload, p->len);
        }
        
        TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", Received));
        
        Bucket->Status = STATUS_SUCCESS;
        Bucket->Information = Received;
        
        CompleteBucket(Connection, Bucket, FALSE);
    }

    DereferenceObject(Connection);
    
    return Received;
}

VOID
TCPConnectEventHandler(void *arg, err_t err)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
        
    ReferenceObject(Connection);
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ConnectRequest, &Connection->Lock)))
    {
        
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = TCPTranslateError(err);
        Bucket->Information = 0;
                
        CompleteBucket(Connection, Bucket, FALSE);
    }
    
    DereferenceObject(Connection);
}
