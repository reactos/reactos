#include "lwip/sys.h"
#include "lwip/tcpip.h"

#include "rosip.h"

#include <debug.h>

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

/* The way that lwIP does multi-threading is really not ideal for our purposes but
 * we best go along with it unless we want another unstable TCP library. lwIP uses
 * a thread called the "tcpip thread" which is the only one allowed to call raw API
 * functions. Since this is the case, for each of our LibTCP* functions, we queue a request
 * for a callback to "tcpip thread" which calls our LibTCP*Callback functions. Yes, this is
 * a lot of unnecessary thread swapping and it could definitely be faster, but I don't want
 * to going messing around in lwIP because I have no desire to create another mess like oskittcp */

extern KEVENT TerminationEvent;

static
void
LibTCPEmptyQueue(PCONNECTION_ENDPOINT Connection)
{
    PLIST_ENTRY Entry;
    PQUEUE_ENTRY qp = NULL;

    ReferenceObject(Connection);

    
    while (!IsListEmpty(&Connection->PacketQueue))
    {
        Entry = RemoveHeadList(&Connection->PacketQueue);
        qp = CONTAINING_RECORD(Entry, QUEUE_ENTRY, ListEntry);

        /* We're in the tcpip thread here so this is safe */
        pbuf_free(qp->p);

        ExFreePoolWithTag(qp, LWIP_TAG);
    }

    DereferenceObject(Connection);
}

void LibTCPEnqueuePacket(PCONNECTION_ENDPOINT Connection, struct pbuf *p)
{
    PQUEUE_ENTRY qp;

    qp = (PQUEUE_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(QUEUE_ENTRY), LWIP_TAG);
    qp->p = p;

    ExInterlockedInsertTailList(&Connection->PacketQueue, &qp->ListEntry, &Connection->Lock);
}

PQUEUE_ENTRY LibTCPDequeuePacket(PCONNECTION_ENDPOINT Connection)
{
    PLIST_ENTRY Entry;
    PQUEUE_ENTRY qp = NULL;

    Entry = ExInterlockedRemoveHeadList(&Connection->PacketQueue, &Connection->Lock);
    
    qp = CONTAINING_RECORD(Entry, QUEUE_ENTRY, ListEntry);

    return qp;
}

NTSTATUS LibTCPGetDataFromConnectionQueue(PCONNECTION_ENDPOINT Connection, PUCHAR RecvBuffer, UINT RecvLen, UINT *Received)
{
    PQUEUE_ENTRY qp;
    struct pbuf* p;
    NTSTATUS Status = STATUS_PENDING;

    if (!IsListEmpty(&Connection->PacketQueue))
    {
        qp = LibTCPDequeuePacket(Connection);
        p = qp->p;

        RecvLen = MIN(p->tot_len, RecvLen);

        for ((*Received) = 0; (*Received) < RecvLen; (*Received) += p->len, p = p->next)
        {
            RtlCopyMemory(RecvBuffer + (*Received), p->payload, p->len);
        }

        /* Use this special pbuf free callback function because we're outside tcpip thread */
        pbuf_free_callback(qp->p);

        ExFreePoolWithTag(qp, LWIP_TAG);

        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_PENDING;
    }

    return Status;
}

static
BOOLEAN
WaitForEventSafely(PRKEVENT Event)
{
    PVOID WaitObjects[] = {Event, &TerminationEvent};
    
    if (KeWaitForMultipleObjects(2,
                                 WaitObjects,
                                 WaitAny,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 NULL,
                                 NULL) == STATUS_WAIT_0)
    {
        /* Signalled by the caller's event */
        return TRUE;
    }
    else /* if KeWaitForMultipleObjects() == STATUS_WAIT_1 */
    {
        /* Signalled by our termination event */
        return FALSE;
    }
}

static
err_t
InternalSendEventHandler(void *arg, PTCP_PCB pcb, const u16_t space)
{
    /* Make sure the socket didn't get closed */
    if (!arg) return ERR_OK;
    
    TCPSendEventHandler(arg, space);
    
    return ERR_OK;
}

static
err_t
InternalRecvEventHandler(void *arg, PTCP_PCB pcb, struct pbuf *p, const err_t err)
{
    u32_t len;

    /* Make sure the socket didn't get closed */
    if (!arg)
    {
        if (p)
            pbuf_free(p);

        return ERR_OK;
    }
    
    if (p)
    {
        len = TCPRecvEventHandler(arg, p);
        if (len == p->tot_len)
        {
            tcp_recved(pcb, len);

            pbuf_free(p);

            return ERR_OK;
        }
        else if (len != 0)
        {
            DbgPrint("UNTESTED CASE: NOT ALL DATA TAKEN! EXTRA DATA MAY BE LOST!\n");
            
            tcp_recved(pcb, len);
            
            /* Possible memory leak of pbuf here? */
            
            return ERR_OK;
        }
        else
        {
            LibTCPEnqueuePacket((PCONNECTION_ENDPOINT)arg, p);

            tcp_recved(pcb, p->tot_len);

            return ERR_OK;
        }
    }
    else if (err == ERR_OK)
    {
        /* Complete pending reads with 0 bytes to indicate a graceful closure,
         * but note that send is still possible in this state so we don't close the
         * whole socket here (by calling tcp_close()) as that would violate TCP specs
         */
        TCPFinEventHandler(arg, ERR_OK);
    }
    
    return ERR_OK;
}

static
err_t
InternalAcceptEventHandler(void *arg, PTCP_PCB newpcb, const err_t err)
{
    /* Make sure the socket didn't get closed */
    if (!arg)
        return ERR_ABRT;
    
    TCPAcceptEventHandler(arg, newpcb);
    
    /* Set in LibTCPAccept (called from TCPAcceptEventHandler) */
    if (newpcb->callback_arg)
        return ERR_OK;
    else
        return ERR_ABRT;
}

static
err_t
InternalConnectEventHandler(void *arg, PTCP_PCB pcb, const err_t err)
{
    /* Make sure the socket didn't get closed */
    if (!arg)
        return ERR_OK;
    
    TCPConnectEventHandler(arg, err);
    
    return ERR_OK;
}

static
void
InternalErrorEventHandler(void *arg, const err_t err)
{
    /* Make sure the socket didn't get closed */
    if (!arg) return;
    
    TCPFinEventHandler(arg, err);
}

struct socket_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    PVOID Arg;
    
    /* Output */
    struct tcp_pcb *NewPcb;
};

static
void
LibTCPSocketCallback(void *arg)
{
    struct socket_callback_msg *msg = arg;
    
    ASSERT(msg);
    
    msg->NewPcb = tcp_new();
    
    if (msg->NewPcb)
    {
        tcp_arg(msg->NewPcb, msg->Arg);
        tcp_err(msg->NewPcb, InternalErrorEventHandler);
    }
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

struct tcp_pcb *
LibTCPSocket(void *arg)
{
    struct socket_callback_msg *msg = ExAllocatePool(NonPagedPool, sizeof(struct socket_callback_msg));
    struct tcp_pcb *ret;
    
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Arg = arg;
        
        tcpip_callback_with_block(LibTCPSocketCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->NewPcb;
        else
            ret = NULL;
        
        ExFreePool(msg);
        
        return ret;
    }
    
    return NULL;
}

struct bind_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    PCONNECTION_ENDPOINT Connection;
    struct ip_addr *IpAddress;
    u16_t Port;
    
    /* Output */
    err_t Error;
};

static
void
LibTCPBindCallback(void *arg)
{
    struct bind_callback_msg *msg = arg;
    
    ASSERT(msg);
    
    if (!msg->Connection->SocketContext)
    {
        msg->Error = ERR_CLSD;
        goto done;
    }
    
    msg->Error = tcp_bind((PTCP_PCB)msg->Connection->SocketContext,
                            msg->IpAddress,
                            ntohs(msg->Port));
    
done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPBind(PCONNECTION_ENDPOINT Connection, struct ip_addr *const ipaddr, const u16_t port)
{
    struct bind_callback_msg *msg;
    err_t ret;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct bind_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Connection = Connection;
        msg->IpAddress = ipaddr;
        msg->Port = port;
        
        tcpip_callback_with_block(LibTCPBindCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        return ret;
    }
    
    return ERR_MEM;
}

struct listen_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    PCONNECTION_ENDPOINT Connection;
    u8_t Backlog;
    
    /* Output */
    PTCP_PCB NewPcb;
};

static
void
LibTCPListenCallback(void *arg)
{
    struct listen_callback_msg *msg = arg;
    
    ASSERT(msg);
    
    if (!msg->Connection->SocketContext)
    {
        msg->NewPcb = NULL;
        goto done;
    }

    msg->NewPcb = tcp_listen_with_backlog((PTCP_PCB)msg->Connection->SocketContext, msg->Backlog);
    
    if (msg->NewPcb)
    {
        tcp_accept(msg->NewPcb, InternalAcceptEventHandler);
    }
    
done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

PTCP_PCB
LibTCPListen(PCONNECTION_ENDPOINT Connection, const u8_t backlog)
{
    struct listen_callback_msg *msg;
    PTCP_PCB ret;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct listen_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Connection = Connection;
        msg->Backlog = backlog;
        
        tcpip_callback_with_block(LibTCPListenCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->NewPcb;
        else
            ret = NULL;
        
        ExFreePool(msg);
        
        return ret;
    }

    return NULL;
}

struct send_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    PCONNECTION_ENDPOINT Connection;
    void *Data;
    u16_t DataLength;
    
    /* Output */
    err_t Error;
};

static
void
LibTCPSendCallback(void *arg)
{
    struct send_callback_msg *msg = arg;
    
    ASSERT(msg);
    
    if (!msg->Connection->SocketContext)
    {
        msg->Error = ERR_CLSD;
        goto done;
    }
    
    if (tcp_sndbuf((PTCP_PCB)msg->Connection->SocketContext) < msg->DataLength)
    {
        msg->Error = ERR_INPROGRESS;
    }
    else
    {
        msg->Error = tcp_write((PTCP_PCB)msg->Connection->SocketContext,
                                msg->Data,
                                msg->DataLength,
                                TCP_WRITE_FLAG_COPY);
        
        tcp_output((PTCP_PCB)msg->Connection->SocketContext);
    }
    
done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPSend(PCONNECTION_ENDPOINT Connection, void *const dataptr, const u16_t len, const int safe)
{
    err_t ret;
    struct send_callback_msg *msg;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct send_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Connection = Connection;
        msg->Data = dataptr;
        msg->DataLength = len;
        
        if (safe)
            LibTCPSendCallback(msg);
        else
            tcpip_callback_with_block(LibTCPSendCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        return ret;
    }

    return ERR_MEM;
}

struct connect_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    PCONNECTION_ENDPOINT Connection;
    struct ip_addr *IpAddress;
    u16_t Port;
    
    /* Output */
    err_t Error;
};

static
void
LibTCPConnectCallback(void *arg)
{
    struct connect_callback_msg *msg = arg;
    
    ASSERT(arg);
    
    if (!msg->Connection->SocketContext)
    {
        msg->Error = ERR_CLSD;
        goto done;
    }
    
    tcp_recv((PTCP_PCB)msg->Connection->SocketContext, InternalRecvEventHandler);
    tcp_sent((PTCP_PCB)msg->Connection->SocketContext, InternalSendEventHandler);

    err_t Error = tcp_connect((PTCP_PCB)msg->Connection->SocketContext,
                                msg->IpAddress, ntohs(msg->Port),
                                InternalConnectEventHandler);

    msg->Error = Error == ERR_OK ? ERR_INPROGRESS : Error;
    
done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPConnect(PCONNECTION_ENDPOINT Connection, struct ip_addr *const ipaddr, const u16_t port)
{
    struct connect_callback_msg *msg;
    err_t ret;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct connect_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Connection = Connection;
        msg->IpAddress = ipaddr;
        msg->Port = port;
        
        tcpip_callback_with_block(LibTCPConnectCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
        {
            ret = msg->Error;
        }
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        return ret;
    }
    
    return ERR_MEM;
}

struct shutdown_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    PCONNECTION_ENDPOINT Connection;
    int shut_rx;
    int shut_tx;
    
    /* Output */
    err_t Error;
};

static
void
LibTCPShutdownCallback(void *arg)
{
    struct shutdown_callback_msg *msg = arg;
    PTCP_PCB pcb = msg->Connection->SocketContext;

    if (!msg->Connection->SocketContext)
    {
        msg->Error = ERR_CLSD;
        goto done;
    }

    if (pcb->state == CLOSE_WAIT)
    {
        /* This case actually results in a socket closure later (lwIP bug?) */
        msg->Connection->SocketContext = NULL;
    }

    msg->Error = tcp_shutdown(pcb, msg->shut_rx, msg->shut_tx);
    if (msg->Error)
    {
        msg->Connection->SocketContext = pcb;
    }
    
done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPShutdown(PCONNECTION_ENDPOINT Connection, const int shut_rx, const int shut_tx)
{
    struct shutdown_callback_msg *msg;
    err_t ret;

    msg = ExAllocatePool(NonPagedPool, sizeof(struct shutdown_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        
        msg->Connection = Connection;
        msg->shut_rx = shut_rx;
        msg->shut_tx = shut_tx;
                
        tcpip_callback_with_block(LibTCPShutdownCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        return ret;
    }
    
    return ERR_MEM;
}

struct close_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    PCONNECTION_ENDPOINT Connection;
    int Callback;
    
    /* Output */
    err_t Error;
};

static
void
LibTCPCloseCallback(void *arg)
{
    struct close_callback_msg *msg = arg;
    PTCP_PCB pcb = msg->Connection->SocketContext;
    int state;

    if (!msg->Connection->SocketContext)
    {
        msg->Error = ERR_OK;
        goto done;
    }

    LibTCPEmptyQueue(msg->Connection);

    /* Clear the PCB pointer */
    msg->Connection->SocketContext = NULL;

    /* Save the old PCB state */
    state = pcb->state;

    msg->Error = tcp_close(pcb);
    if (!msg->Error)
    {
        if (msg->Callback)
        {
            /* Call the FIN handler in the cases where it will not be called by lwIP */
            switch (state)
            {
                case CLOSED:
                case LISTEN:
                case SYN_SENT:
                   TCPFinEventHandler(msg->Connection, ERR_OK);
                   break;

                default:
                   break;
            }
        }
    }
    else
    {
        /* Restore the PCB pointer */
        msg->Connection->SocketContext = pcb;
    }

done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPClose(PCONNECTION_ENDPOINT Connection, const int safe, const int callback)
{
    err_t ret;
    struct close_callback_msg *msg;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct close_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        
        msg->Connection = Connection;
        msg->Callback = callback;
        
        if (safe)
            LibTCPCloseCallback(msg);
        else
            tcpip_callback_with_block(LibTCPCloseCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        return ret;
    }

    return ERR_MEM;
}

void
LibTCPAccept(PTCP_PCB pcb, struct tcp_pcb *listen_pcb, void *arg)
{
    ASSERT(arg);
    
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, InternalRecvEventHandler);
    tcp_sent(pcb, InternalSendEventHandler);
    tcp_err(pcb, InternalErrorEventHandler);
    tcp_arg(pcb, arg);
    
    tcp_accepted(listen_pcb);
}

err_t
LibTCPGetHostName(PTCP_PCB pcb, struct ip_addr *const ipaddr, u16_t *const port)
{
    if (!pcb)
        return ERR_CLSD;
    
    *ipaddr = pcb->local_ip;
    *port = pcb->local_port;
    
    return ERR_OK;
}

err_t
LibTCPGetPeerName(PTCP_PCB pcb, struct ip_addr * const ipaddr, u16_t * const port)
{    
    if (!pcb)
        return ERR_CLSD;
    
    *ipaddr = pcb->remote_ip;
    *port = pcb->remote_port;
    
    return ERR_OK;
}
