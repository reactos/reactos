#include "lwip/sys.h"
#include "lwip/tcpip.h"

#include "rosip.h"

#include <debug.h>

/* The way that lwIP does multi-threading is really not ideal for our purposes but
 * we best go along with it unless we want another unstable TCP library. lwIP uses
 * a thread called the "tcpip thread" which is the only one allowed to call raw API
 * functions. Since this is the case, for each of our LibTCP* functions, we queue a request
 * for a callback to "tcpip thread" which calls our LibTCP*Callback functions. Yes, this is
 * a lot of unnecessary thread swapping and it could definitely be faster, but I don't want
 * to going messing around in lwIP because I have no desire to create another mess like oskittcp */

extern KEVENT TerminationEvent;

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
InternalSendEventHandler(void *arg, struct tcp_pcb *pcb, u16_t space)
{
    DbgPrint("SendEvent(0x%x, 0x%x, %d)\n", arg, pcb, (unsigned int)space);
    
    /* Make sure the socket didn't get closed */
    if (!arg) return ERR_OK;
    
    TCPSendEventHandler(arg, space);
    
    return ERR_OK;
}

static
err_t
InternalRecvEventHandler(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    u32_t len;

    DbgPrint("RecvEvent(0x%x, 0x%x, 0x%x, %d)\n", arg, pcb, p, (unsigned int)err);
    
    /* Make sure the socket didn't get closed */
    if (!arg)
    {
        if (p) pbuf_free(p);
        return ERR_OK;
    }
    
    if (!p)
    {
        TCPFinEventHandler(arg, ERR_OK);
    }
    else
    {
        DbgPrint("RECV - p:0x%x p->payload:0x%x p->len:%d p->tot_len:%d\n", p, p->payload, p->len, p->tot_len);

        if (err == ERR_OK)
        {
            len = TCPRecvEventHandler(arg, p);
            if (len != 0)
            {
                tcp_recved(pcb, len);
                
                pbuf_free(p);
                
                return ERR_OK;
            }
            else
            {
                /* We want lwIP to store the pbuf on its queue for later */
                return ERR_TIMEOUT;
            }
        }
        else
        {
            pbuf_free(p);
        }
    }
    
    return ERR_OK;
}

static
err_t
InternalAcceptEventHandler(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    DbgPrint("AcceptEvent(0x%x, 0x%x, %d)\n", arg, newpcb, (unsigned int)err);
    
    /* Make sure the socket didn't get closed */
    if (!arg) return ERR_ABRT;
    
    TCPAcceptEventHandler(arg, newpcb);
    
    /* Set in LibTCPAccept (called from TCPAcceptEventHandler) */
    if (newpcb->callback_arg)
        return ERR_OK;
    else
        return ERR_ABRT;
}

static
err_t
InternalConnectEventHandler(void *arg, struct tcp_pcb *pcb, err_t err)
{
    DbgPrint("ConnectEvent(0x%x, 0x%x, %d)\n", arg, pcb, (unsigned int)err);
    
    /* Make sure the socket didn't get closed */
    if (!arg) return ERR_OK;
    
    TCPConnectEventHandler(arg, err);
    
    tcp_recv(pcb, InternalRecvEventHandler);
    
    return ERR_OK;
}

static
void
InternalErrorEventHandler(void *arg, err_t err)
{
    DbgPrint("ErrorEvent(0x%x, %d)\n", arg, (unsigned int)err);
    
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
    PVOID NewPcb;
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
    void *ret;
    
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Arg = arg;
        
        tcpip_callback_with_block(LibTCPSocketCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->NewPcb;
        else
            ret = NULL;
        
        DbgPrint("LibTCPSocket(0x%x) = 0x%x\n", arg, ret);
        
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
    struct tcp_pcb *Pcb;
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
    
    msg->Error = tcp_bind(msg->Pcb, msg->IpAddress, ntohs(msg->Port));
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPBind(struct tcp_pcb *pcb, struct ip_addr *ipaddr, u16_t port)
{
    struct bind_callback_msg *msg;
    err_t ret;
    
    if (!pcb)
        return ERR_CLSD;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct bind_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Pcb = pcb;
        msg->IpAddress = ipaddr;
        msg->Port = port;
        
        tcpip_callback_with_block(LibTCPBindCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        DbgPrint("LibTCPBind(0x%x)\n", pcb);
        
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
    struct tcp_pcb *Pcb;
    u8_t Backlog;
    
    /* Output */
    struct tcp_pcb *NewPcb;
};

static
void
LibTCPListenCallback(void *arg)
{
    struct listen_callback_msg *msg = arg;
    void *p;
    
    ASSERT(msg);
    
    p = msg->Pcb->callback_arg;
    msg->NewPcb = tcp_listen_with_backlog(msg->Pcb, msg->Backlog);
    
    if (msg->NewPcb)
    {
        tcp_arg(msg->NewPcb, p);
        tcp_accept(msg->NewPcb, InternalAcceptEventHandler);
        tcp_err(msg->NewPcb, InternalErrorEventHandler);
    }
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

struct tcp_pcb *
LibTCPListen(struct tcp_pcb *pcb, u8_t backlog)
{
    struct listen_callback_msg *msg;
    void *ret;
    
    if (!pcb)
        return NULL;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct listen_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Pcb = pcb;
        msg->Backlog = backlog;
        
        tcpip_callback_with_block(LibTCPListenCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->NewPcb;
        else
            ret = NULL;
        
        DbgPrint("LibTCPListen(0x%x,0x%x)\n", pcb, ret);
        
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
    struct tcp_pcb *Pcb;
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
    
    if (tcp_sndbuf(msg->Pcb) < msg->DataLength)
    {
        msg->Error = ERR_INPROGRESS;
    }
    else
    {
        tcp_sent(msg->Pcb, InternalSendEventHandler);
        
        msg->Error = tcp_write(msg->Pcb, msg->Data, msg->DataLength, TCP_WRITE_FLAG_COPY);
        
        tcp_output(msg->Pcb);
    }
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPSend(struct tcp_pcb *pcb, void *dataptr, u16_t len)
{
    struct send_callback_msg *msg;
    err_t ret;
    
    if (!pcb)
        return ERR_CLSD;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct send_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Pcb = pcb;
        msg->Data = dataptr;
        msg->DataLength = len;
        
        tcpip_callback_with_block(LibTCPSendCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        DbgPrint("LibTCPSend(0x%x)\n", pcb);
        
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
    struct tcp_pcb *Pcb;
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
    
    msg->Error = tcp_connect(msg->Pcb, msg->IpAddress, ntohs(msg->Port), InternalConnectEventHandler);
    if (msg->Error == ERR_OK)
        msg->Error = ERR_INPROGRESS;
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPConnect(struct tcp_pcb *pcb, struct ip_addr *ipaddr, u16_t port)
{
    struct connect_callback_msg *msg;
    err_t ret;
    
    if (!pcb)
        return ERR_CLSD;
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct connect_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Pcb = pcb;
        msg->IpAddress = ipaddr;
        msg->Port = port;
        
        tcpip_callback_with_block(LibTCPConnectCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        DbgPrint("LibTCPConnect(0x%x)\n", pcb);
        
        return ret;
    }
    
    return ERR_MEM;
}

struct close_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    struct tcp_pcb *Pcb;
    
    /* Output */
    err_t Error;
};

static
void
LibTCPCloseCallback(void *arg)
{
    struct close_callback_msg *msg = arg;
    
    msg->Error = tcp_close(msg->Pcb);
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPClose(struct tcp_pcb *pcb)
{
    struct close_callback_msg *msg;
    err_t ret;
    
    if (!pcb)
        return ERR_CLSD;
    
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, NULL);
    tcp_sent(pcb, NULL);
    tcp_err(pcb, NULL);
    tcp_accept(pcb, NULL);
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct close_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Pcb = pcb;
        
        tcpip_callback_with_block(LibTCPCloseCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        DbgPrint("LibTCPClose(0x%x)\n", pcb);
        
        return ret;
    }
    
    return ERR_MEM;
}

void
LibTCPAccept(struct tcp_pcb *pcb, void *arg)
{
    DbgPrint("LibTCPAccept(0x%x, 0x%x)\n", pcb, arg);
    
    ASSERT(arg);
    
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, InternalRecvEventHandler);
    tcp_sent(pcb, InternalSendEventHandler);
    tcp_arg(pcb, arg);
    
    tcp_accepted(pcb);
}

err_t
LibTCPGetHostName(struct tcp_pcb *pcb, struct ip_addr *ipaddr, u16_t *port)
{
    DbgPrint("LibTCPGetHostName(0x%x)\n", pcb);
    
    if (!pcb)
        return ERR_CLSD;
    
    *ipaddr = pcb->local_ip;
    *port = pcb->local_port;
    
    DbgPrint("Got host port: %d\n", *port);
    
    return ERR_OK;
}

err_t
LibTCPGetPeerName(struct tcp_pcb *pcb, struct ip_addr *ipaddr, u16_t *port)
{
    DbgPrint("LibTCPGetPeerName(0x%x)\n", pcb);
    
    if (!pcb)
        return ERR_CLSD;
    
    *ipaddr = pcb->remote_ip;
    *port = pcb->remote_port;
    
    DbgPrint("Got remote port: %d\n", *port);
    
    return ERR_OK;
}
