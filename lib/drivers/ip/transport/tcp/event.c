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

VOID
CompleteBucket(PCONNECTION_ENDPOINT Connection, PTDI_BUCKET Bucket, BOOLEAN Synchronous)
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
FlushAllQueues(PCONNECTION_ENDPOINT Connection, NTSTATUS Status)
{
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    
    ReferenceObject(Connection);
    
    DbgPrint("[IP, FlushAllQueues] Flushing recv/all with status: 0x%x for Connection = 0x%x\n", Status, Connection);
    
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

    // Calling with Status == STATUS_SUCCESS means that we got a graceful closure
    // so we don't want to kill everything else since send is still valid in this state
    //
    if (Status == STATUS_SUCCESS)
    {
        DbgPrint("[IP, FlushAllQueues] Flushed recv only after graceful closure\n");
        DereferenceObject(Connection);
        return;
    }
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ListenRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = Status;
        Bucket->Information = 0;

        DbgPrint("[IP, FlushAllQueues] Completing Listen request for Connection = 0x%x\n", Bucket->AssociatedEndpoint);
        
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

        DbgPrint("[IP, FlushAllQueues] Completing Connection request for Connection = 0x%x\n", Bucket->AssociatedEndpoint);
        
        CompleteBucket(Connection, Bucket, FALSE);
    }
    
    DereferenceObject(Connection);

    DbgPrint("[IP, FlushAllQueues] Leaving\n");
}

VOID
TCPFinEventHandler(void *arg, err_t err)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    const NTSTATUS status = TCPTranslateError(err);

    DbgPrint("[IP, TCPFinEventHandler] Called for Connection( 0x%x )-> SocketContext = pcb (0x%x)\n", Connection, Connection->SocketContext);

    /* Only clear the pointer if the shutdown was caused by an error */
    if ((err != ERR_OK))
    {
        /* We're already closed by the error so we don't want to call lwip_close */
        DbgPrint("[IP, TCPFinEventHandler] MAKING Connection( 0x%x )-> SocketContext = pcb (0x%x) NULL\n", Connection, Connection->SocketContext);

        Connection->SocketContext = NULL;
    }

    FlushAllQueues(Connection, status);

    DbgPrint("[IP, TCPFinEventHandler] Done\n");
}
    
VOID
TCPAcceptEventHandler(void *arg, struct tcp_pcb *newpcb)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;
    KIRQL OldIrql;
    
    DbgPrint("[IP, TCPAcceptEventHandler] Called\n");
    
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
        
        DbgPrint("[IP, TCPAcceptEventHandler] Completing accept event %x\n", Status);
        
        if (Status == STATUS_SUCCESS)
        {
            DbgPrint("[IP, TCPAcceptEventHandler] newpcb->state = %s, listen_pcb->state = %s, newpcb = 0x%x\n",
                tcp_state_str[newpcb->state],
                tcp_state_str[((struct tcp_pcb*)Connection->SocketContext)->state],
                newpcb);

            LockObject(Bucket->AssociatedEndpoint, &OldIrql);

            /* sanity assert...this should never be in anything else but a CLOSED state */
            ASSERT(((struct tcp_pcb*)Bucket->AssociatedEndpoint->SocketContext)->state == CLOSED);
            
            /*  free socket context created in FileOpenConnection, as we're using a new one */
            LibTCPClose(Bucket->AssociatedEndpoint, TRUE);

            /* free previously created socket context (we don't use it, we use newpcb) */
            Bucket->AssociatedEndpoint->SocketContext = newpcb;
            
            LibTCPAccept(newpcb, (PTCP_PCB)Connection->SocketContext, Bucket->AssociatedEndpoint);

            DbgPrint("[IP, TCPAcceptEventHandler] Trying to unlock Bucket->AssociatedEndpoint\n");
            UnlockObject(Bucket->AssociatedEndpoint, OldIrql);
        }
        
        DereferenceObject(Bucket->AssociatedEndpoint);

        DbgPrint("[IP, TCPAcceptEventHandler] Done!\n");
        
        CompleteBucket(Connection, Bucket, FALSE);
    }
    
    DereferenceObject(Connection);
}

VOID
TCPSendEventHandler(void *arg, u16_t space)
{
    PCONNECTION_ENDPOINT Connection = arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;
    PMDL Mdl;
    
    DbgPrint("[IP, TCPSendEventHandler] Called\n");
    
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
        DbgPrint("[IP, TCPSendEventHandler] Sending out &d bytes on pcb = 0x%x\n",
            Connection->SocketContext);
        
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
            
            DbgPrint("Completing send req %x\n", Status);
            
            CompleteBucket(Connection, Bucket, FALSE);
        }
    }
    
    DereferenceObject(Connection);

    DbgPrint("[IP, TCPSendEventHandler] Leaving\n");
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
    
    DbgPrint("[IP, TCPRecvEventHandler] Called\n");
    
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
            DbgPrint("[IP, TCPRecvEventHandler] 0x%x: Copying %d bytes to 0x%x from 0x%x\n",
                p, p->len, ((PUCHAR)RecvBuffer) + Received, p->payload);
            
            RtlCopyMemory(RecvBuffer + Received, p->payload, p->len);
        }
        
        TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", Received));
        
        Bucket->Status = STATUS_SUCCESS;
        Bucket->Information = Received;
        
        CompleteBucket(Connection, Bucket, FALSE);
    }

    DereferenceObject(Connection);

    DbgPrint("[IP, TCPRecvEventHandler] Leaving\n");
    
    return Received;
}

VOID
TCPConnectEventHandler(void *arg, err_t err)
{
    PCONNECTION_ENDPOINT Connection = (PCONNECTION_ENDPOINT)arg;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    
    DbgPrint("[IP, TCPConnectEventHandler] Called\n");
    
    ReferenceObject(Connection);
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ConnectRequest, &Connection->Lock)))
    {
        
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = TCPTranslateError(err);
        Bucket->Information = 0;
        
        DbgPrint("[IP, TCPConnectEventHandler] Completing connection request! (0x%x)\n", err);
        
        CompleteBucket(Connection, Bucket, FALSE);
    }

    DbgPrint("[IP, TCPConnectEventHandler] Done\n");
    
    DereferenceObject(Connection);
}
