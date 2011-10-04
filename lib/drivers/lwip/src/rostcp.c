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
extern NPAGED_LOOKASIDE_LIST MessageLookasideList;
extern NPAGED_LOOKASIDE_LIST QueueEntryLookasideList;

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

        ExFreeToNPagedLookasideList(&QueueEntryLookasideList, qp);
    }

    DereferenceObject(Connection);
}

void LibTCPEnqueuePacket(PCONNECTION_ENDPOINT Connection, struct pbuf *p)
{
    PQUEUE_ENTRY qp;

    qp = (PQUEUE_ENTRY)ExAllocateFromNPagedLookasideList(&QueueEntryLookasideList);
    qp->p = p;

    ExInterlockedInsertTailList(&Connection->PacketQueue, &qp->ListEntry, &Connection->Lock);
}

PQUEUE_ENTRY LibTCPDequeuePacket(PCONNECTION_ENDPOINT Connection)
{
    PLIST_ENTRY Entry;
    PQUEUE_ENTRY qp = NULL;

    if (IsListEmpty(&Connection->PacketQueue)) return NULL;

    Entry = RemoveHeadList(&Connection->PacketQueue);

    qp = CONTAINING_RECORD(Entry, QUEUE_ENTRY, ListEntry);

    return qp;
}

NTSTATUS LibTCPGetDataFromConnectionQueue(PCONNECTION_ENDPOINT Connection, PUCHAR RecvBuffer, UINT RecvLen, UINT *Received)
{
    PQUEUE_ENTRY qp;
    struct pbuf* p;
    NTSTATUS Status = STATUS_PENDING;
    UINT ReadLength, ExistingDataLength;
    KIRQL OldIrql;

    (*Received) = 0;

    LockObject(Connection, &OldIrql);

    if (!IsListEmpty(&Connection->PacketQueue))
    {
        while ((qp = LibTCPDequeuePacket(Connection)) != NULL)
        {
            p = qp->p;
            ExistingDataLength = (*Received);

            Status = STATUS_SUCCESS;

            ReadLength = MIN(p->tot_len, RecvLen);
            if (ReadLength != p->tot_len)
            {
                if (ExistingDataLength)
                {
                    /* The packet was too big but we used some data already so give it another shot later */
                    InsertHeadList(&Connection->PacketQueue, &qp->ListEntry);
                    break;
                }
                else
                {
                    /* The packet is just too big to fit fully in our buffer, even when empty so
                     * return an informative status but still copy all the data we can fit.
                     */
                    Status = STATUS_BUFFER_OVERFLOW;
                }
            }

            UnlockObject(Connection, OldIrql);

            /* Return to a lower IRQL because the receive buffer may be pageable memory */
            for (; (*Received) < ReadLength + ExistingDataLength; (*Received) += p->len, p = p->next)
            {
                RtlCopyMemory(RecvBuffer + (*Received), p->payload, p->len);
            }

            LockObject(Connection, &OldIrql);

            RecvLen -= ReadLength;

            /* Use this special pbuf free callback function because we're outside tcpip thread */
            pbuf_free_callback(qp->p);

            ExFreeToNPagedLookasideList(&QueueEntryLookasideList, qp);

            if (!RecvLen)
                break;

            if (Status != STATUS_SUCCESS)
                break;
        }
    }
    else
    {
        if (Connection->ReceiveShutdown)
            Status = STATUS_SUCCESS;
        else
            Status = STATUS_PENDING;
    }

    UnlockObject(Connection, OldIrql);

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
    PCONNECTION_ENDPOINT Connection = arg;

    /* Make sure the socket didn't get closed */
    if (!arg)
    {
        if (p)
            pbuf_free(p);

        return ERR_OK;
    }

    if (p)
    {
        LibTCPEnqueuePacket(Connection, p);

        tcp_recved(pcb, p->tot_len);

        TCPRecvEventHandler(arg);
    }
    else if (err == ERR_OK)
    {
        /* Complete pending reads with 0 bytes to indicate a graceful closure,
         * but note that send is still possible in this state so we don't close the
         * whole socket here (by calling tcp_close()) as that would violate TCP specs
         */
        Connection->ReceiveShutdown = TRUE;
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

static
void
LibTCPSocketCallback(void *arg)
{
    struct lwip_callback_msg *msg = arg;

    ASSERT(msg);

    msg->Output.Socket.NewPcb = tcp_new();

    if (msg->Output.Socket.NewPcb)
    {
        tcp_arg(msg->Output.Socket.NewPcb, msg->Input.Socket.Arg);
        tcp_err(msg->Output.Socket.NewPcb, InternalErrorEventHandler);
    }

    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

struct tcp_pcb *
LibTCPSocket(void *arg)
{
    struct lwip_callback_msg *msg = ExAllocateFromNPagedLookasideList(&MessageLookasideList);
    struct tcp_pcb *ret;

    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Input.Socket.Arg = arg;

        tcpip_callback_with_block(LibTCPSocketCallback, msg, 1);

        if (WaitForEventSafely(&msg->Event))
            ret = msg->Output.Socket.NewPcb;
        else
            ret = NULL;

        ExFreeToNPagedLookasideList(&MessageLookasideList, msg);

        return ret;
    }

    return NULL;
}

static
void
LibTCPBindCallback(void *arg)
{
    struct lwip_callback_msg *msg = arg;

    ASSERT(msg);

    if (!msg->Input.Bind.Connection->SocketContext)
    {
        msg->Output.Bind.Error = ERR_CLSD;
        goto done;
    }

    msg->Output.Bind.Error = tcp_bind((PTCP_PCB)msg->Input.Bind.Connection->SocketContext,
                                      msg->Input.Bind.IpAddress,
                                      ntohs(msg->Input.Bind.Port));

done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPBind(PCONNECTION_ENDPOINT Connection, struct ip_addr *const ipaddr, const u16_t port)
{
    struct lwip_callback_msg *msg;
    err_t ret;

    msg = ExAllocateFromNPagedLookasideList(&MessageLookasideList);
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Input.Bind.Connection = Connection;
        msg->Input.Bind.IpAddress = ipaddr;
        msg->Input.Bind.Port = port;

        tcpip_callback_with_block(LibTCPBindCallback, msg, 1);

        if (WaitForEventSafely(&msg->Event))
            ret = msg->Output.Bind.Error;
        else
            ret = ERR_CLSD;

        ExFreeToNPagedLookasideList(&MessageLookasideList, msg);

        return ret;
    }

    return ERR_MEM;
}

static
void
LibTCPListenCallback(void *arg)
{
    struct lwip_callback_msg *msg = arg;

    ASSERT(msg);

    if (!msg->Input.Listen.Connection->SocketContext)
    {
        msg->Output.Listen.NewPcb = NULL;
        goto done;
    }

    msg->Output.Listen.NewPcb = tcp_listen_with_backlog((PTCP_PCB)msg->Input.Listen.Connection->SocketContext, msg->Input.Listen.Backlog);

    if (msg->Output.Listen.NewPcb)
    {
        tcp_accept(msg->Output.Listen.NewPcb, InternalAcceptEventHandler);
    }

done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

PTCP_PCB
LibTCPListen(PCONNECTION_ENDPOINT Connection, const u8_t backlog)
{
    struct lwip_callback_msg *msg;
    PTCP_PCB ret;

    msg = ExAllocateFromNPagedLookasideList(&MessageLookasideList);
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Input.Listen.Connection = Connection;
        msg->Input.Listen.Backlog = backlog;

        tcpip_callback_with_block(LibTCPListenCallback, msg, 1);

        if (WaitForEventSafely(&msg->Event))
            ret = msg->Output.Listen.NewPcb;
        else
            ret = NULL;

        ExFreeToNPagedLookasideList(&MessageLookasideList, msg);

        return ret;
    }

    return NULL;
}

static
void
LibTCPSendCallback(void *arg)
{
    struct lwip_callback_msg *msg = arg;

    ASSERT(msg);

    if (!msg->Input.Send.Connection->SocketContext)
    {
        msg->Output.Send.Error = ERR_CLSD;
        goto done;
    }

    if (msg->Input.Send.Connection->SendShutdown)
    {
        msg->Output.Send.Error = ERR_CLSD;
        goto done;
    }

    msg->Output.Send.Error = tcp_write((PTCP_PCB)msg->Input.Send.Connection->SocketContext,
                                       msg->Input.Send.Data,
                                       msg->Input.Send.DataLength,
                                       TCP_WRITE_FLAG_COPY);
    if (msg->Output.Send.Error == ERR_MEM)
    {
        /* No buffer space so return pending */
        msg->Output.Send.Error = ERR_INPROGRESS;
    }
    else if (msg->Output.Send.Error == ERR_OK)
    {
        /* Queued successfully so try to send it */
        tcp_output((PTCP_PCB)msg->Input.Send.Connection->SocketContext);
    }

done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPSend(PCONNECTION_ENDPOINT Connection, void *const dataptr, const u16_t len, const int safe)
{
    err_t ret;
    struct lwip_callback_msg *msg;

    msg = ExAllocateFromNPagedLookasideList(&MessageLookasideList);
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Input.Send.Connection = Connection;
        msg->Input.Send.Data = dataptr;
        msg->Input.Send.DataLength = len;

        if (safe)
            LibTCPSendCallback(msg);
        else
            tcpip_callback_with_block(LibTCPSendCallback, msg, 1);

        if (WaitForEventSafely(&msg->Event))
            ret = msg->Output.Send.Error;
        else
            ret = ERR_CLSD;

        ExFreeToNPagedLookasideList(&MessageLookasideList, msg);

        return ret;
    }

    return ERR_MEM;
}

static
void
LibTCPConnectCallback(void *arg)
{
    struct lwip_callback_msg *msg = arg;
    err_t Error;

    ASSERT(arg);

    if (!msg->Input.Connect.Connection->SocketContext)
    {
        msg->Output.Connect.Error = ERR_CLSD;
        goto done;
    }

    tcp_recv((PTCP_PCB)msg->Input.Connect.Connection->SocketContext, InternalRecvEventHandler);
    tcp_sent((PTCP_PCB)msg->Input.Connect.Connection->SocketContext, InternalSendEventHandler);

    Error = tcp_connect((PTCP_PCB)msg->Input.Connect.Connection->SocketContext,
                        msg->Input.Connect.IpAddress, ntohs(msg->Input.Connect.Port),
                        InternalConnectEventHandler);

    msg->Output.Connect.Error = Error == ERR_OK ? ERR_INPROGRESS : Error;

done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPConnect(PCONNECTION_ENDPOINT Connection, struct ip_addr *const ipaddr, const u16_t port)
{
    struct lwip_callback_msg *msg;
    err_t ret;

    msg = ExAllocateFromNPagedLookasideList(&MessageLookasideList);
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Input.Connect.Connection = Connection;
        msg->Input.Connect.IpAddress = ipaddr;
        msg->Input.Connect.Port = port;

        tcpip_callback_with_block(LibTCPConnectCallback, msg, 1);

        if (WaitForEventSafely(&msg->Event))
        {
            ret = msg->Output.Connect.Error;
        }
        else
            ret = ERR_CLSD;

        ExFreeToNPagedLookasideList(&MessageLookasideList, msg);

        return ret;
    }

    return ERR_MEM;
}

static
void
LibTCPShutdownCallback(void *arg)
{
    struct lwip_callback_msg *msg = arg;
    PTCP_PCB pcb = msg->Input.Shutdown.Connection->SocketContext;

    if (!msg->Input.Shutdown.Connection->SocketContext)
    {
        msg->Output.Shutdown.Error = ERR_CLSD;
        goto done;
    }

    if (pcb->state == CLOSE_WAIT)
    {
        /* This case actually results in a socket closure later (lwIP bug?) */
        msg->Input.Shutdown.Connection->SocketContext = NULL;
    }

    msg->Output.Shutdown.Error = tcp_shutdown(pcb, msg->Input.Shutdown.shut_rx, msg->Input.Shutdown.shut_tx);
    if (msg->Output.Shutdown.Error)
    {
        msg->Input.Shutdown.Connection->SocketContext = pcb;
    }
    else
    {
        if (msg->Input.Shutdown.shut_rx)
            msg->Input.Shutdown.Connection->ReceiveShutdown = TRUE;

        if (msg->Input.Shutdown.shut_tx)
            msg->Input.Shutdown.Connection->SendShutdown = TRUE;
    }

done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPShutdown(PCONNECTION_ENDPOINT Connection, const int shut_rx, const int shut_tx)
{
    struct lwip_callback_msg *msg;
    err_t ret;

    msg = ExAllocateFromNPagedLookasideList(&MessageLookasideList);
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);

        msg->Input.Shutdown.Connection = Connection;
        msg->Input.Shutdown.shut_rx = shut_rx;
        msg->Input.Shutdown.shut_tx = shut_tx;

        tcpip_callback_with_block(LibTCPShutdownCallback, msg, 1);

        if (WaitForEventSafely(&msg->Event))
            ret = msg->Output.Shutdown.Error;
        else
            ret = ERR_CLSD;

        ExFreeToNPagedLookasideList(&MessageLookasideList, msg);

        return ret;
    }

    return ERR_MEM;
}

static
void
LibTCPCloseCallback(void *arg)
{
    struct lwip_callback_msg *msg = arg;
    PTCP_PCB pcb = msg->Input.Close.Connection->SocketContext;

    /* Empty the queue even if we're already "closed" */
    LibTCPEmptyQueue(msg->Input.Close.Connection);

    if (!msg->Input.Close.Connection->SocketContext)
    {
        msg->Output.Close.Error = ERR_OK;
        goto done;
    }

    /* Clear the PCB pointer */
    msg->Input.Close.Connection->SocketContext = NULL;

    switch (pcb->state)
    {
        case CLOSED:
        case LISTEN:
        case SYN_SENT:
           msg->Output.Close.Error = tcp_close(pcb);

           if (!msg->Output.Close.Error && msg->Input.Close.Callback)
               TCPFinEventHandler(msg->Input.Close.Connection, ERR_OK);
           break;

        default:
           if (msg->Input.Close.Connection->SendShutdown &&
               msg->Input.Close.Connection->ReceiveShutdown)
           {
               /* Abort the connection */
               tcp_abort(pcb);

               /* Aborts always succeed */
               msg->Output.Close.Error = ERR_OK;
           }
           else
           {
               /* Start the graceful close process (or send RST for pending data) */
               msg->Output.Close.Error = tcp_close(pcb);
           }
           break;
    }

    if (msg->Output.Close.Error)
    {
        /* Restore the PCB pointer */
        msg->Input.Close.Connection->SocketContext = pcb;
    }

done:
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPClose(PCONNECTION_ENDPOINT Connection, const int safe, const int callback)
{
    err_t ret;
    struct lwip_callback_msg *msg;

    msg = ExAllocateFromNPagedLookasideList(&MessageLookasideList);
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);

        msg->Input.Close.Connection = Connection;
        msg->Input.Close.Callback = callback;

        if (safe)
            LibTCPCloseCallback(msg);
        else
            tcpip_callback_with_block(LibTCPCloseCallback, msg, 1);

        if (WaitForEventSafely(&msg->Event))
            ret = msg->Output.Close.Error;
        else
            ret = ERR_CLSD;

        ExFreeToNPagedLookasideList(&MessageLookasideList, msg);

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
