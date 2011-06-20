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
    DbgPrint("[lwIP, InternalSendEventHandler] SendEvent (0x%x, 0x%x, %d)\n",
        arg, pcb, (unsigned int)space);
    
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

    DbgPrint("[lwIP, InternalRecvEventHandler] RecvEvent (0x%x, 0x%x, 0x%x, %d)\n",
        arg, pcb, p, (unsigned int)err);
    
    /* Make sure the socket didn't get closed */
    if (!arg)
    {
        if (p) pbuf_free(p);
        return ERR_OK;
    }
    
    if (p)
    {
        DbgPrint("[lwIP, InternalRecvEventHandler] RECV - p:0x%x p->payload:0x%x p->len:%d p->tot_len:%d\n",
            p, p->payload, p->len, p->tot_len);

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
            /* We want lwIP to store the pbuf on its queue for later */
            return ERR_TIMEOUT;
        }
    }
    else if (err == ERR_OK)
    {
        TCPFinEventHandler(arg, ERR_OK);
        tcp_close(pcb);
    }
    
    return ERR_OK;
}

static
err_t
InternalAcceptEventHandler(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    DbgPrint("[lwIP, InternalAcceptEventHandler] AcceptEvent arg = 0x%x, newpcb = 0x%x, err = %d\n",
        arg, newpcb, (unsigned int)err);
    
    /* Make sure the socket didn't get closed */
    if (!arg)
        return ERR_ABRT;

    DbgPrint("[lwIP, InternalAcceptEventHandler] newpcb->state = %s\n",
                tcp_state_str[newpcb->state]);
    
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
    DbgPrint("[lwIP, InternalConnectEventHandler] ConnectEvent(0x%x, 0x%x, %d)\n",
        arg, pcb, (unsigned int)err);
    
    /* Make sure the socket didn't get closed */
    if (!arg)
        return ERR_OK;
    
    TCPConnectEventHandler(arg, err);
    
    tcp_recv(pcb, InternalRecvEventHandler);
    
    return ERR_OK;
}

static
void
InternalErrorEventHandler(void *arg, err_t err)
{
    DbgPrint("[lwIP, InternalErrorEventHandler] ErrorEvent(0x%x, %d)\n",
        arg, (unsigned int)err);
    
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

    DbgPrint("[lwIP, LibTCPSocketCallback] Called\n");
    
    msg->NewPcb = tcp_new();

    DbgPrint("[lwIP, LibTCPSocketCallback] Assigned new pcb = 0x%x\n", msg->NewPcb);
    
    if (msg->NewPcb)
    {
        tcp_arg((struct tcp_pcb*)msg->NewPcb, msg->Arg);
        tcp_err((struct tcp_pcb*)msg->NewPcb, InternalErrorEventHandler);
    }
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

struct tcp_pcb *
LibTCPSocket(void *arg)
{
    struct socket_callback_msg *msg = ExAllocatePool(NonPagedPool, sizeof(struct socket_callback_msg));
    void *ret;

    DbgPrint("[lwIP, LibTCPSocket] Called\n");
    
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        msg->Arg = arg;
        
        tcpip_callback_with_block(LibTCPSocketCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->NewPcb;
        else
            ret = NULL;
        
        DbgPrint("[lwIP, LibTCPSocket] Connection( 0x%x )->SocketContext = pcb( 0x%x )\n", arg, ret);

        DbgPrint("[lwIP, LibTCPSocket] Done\n");
        
        ExFreePool(msg);
        
        return (struct tcp_pcb*)ret;
    }

    DbgPrint("[lwIP, LibTCPSocket] Done\n");
    
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

    DbgPrint("[lwIP, LibTCPBind] Called\n");
    
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
        
        DbgPrint("[lwIP, LibTCPBind] pcb = 0x%x\n", pcb);

        DbgPrint("[lwIP, LibTCPBind] Done\n");
        
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

    DbgPrint("[lwIP, LibTCPListenCallback] Called\n");
    
    p = msg->Pcb->callback_arg;
    msg->NewPcb = tcp_listen_with_backlog(msg->Pcb, msg->Backlog);
    
    if (msg->NewPcb)
    {
        tcp_arg(msg->NewPcb, p);
        tcp_accept(msg->NewPcb, InternalAcceptEventHandler);
        tcp_err(msg->NewPcb, InternalErrorEventHandler);
    }

    DbgPrint("[lwIP, LibTCPListenCallback] Done\n");
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

struct tcp_pcb *
LibTCPListen(struct tcp_pcb *pcb, u8_t backlog)
{
    struct listen_callback_msg *msg;
    void *ret;

    DbgPrint("[lwIP, LibTCPListen] Called on pcb = 0x%x\n", pcb);
    
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
        
        DbgPrint("[lwIP, LibTCPListen] pcb = 0x%x, newpcb = 0x%x, sizeof(pcb) = %d \n",
            pcb, ret, sizeof(struct tcp_pcb));

        DbgPrint("[lwIP, LibTCPListen] Done\n");
        
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

    DbgPrint("[lwIP, LibTCPConnectCallback] Called\n");
    
    ASSERT(arg);
    
    msg->Error = tcp_connect(msg->Pcb, msg->IpAddress, ntohs(msg->Port), InternalConnectEventHandler);
    if (msg->Error == ERR_OK)
        msg->Error = ERR_INPROGRESS;
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);

    DbgPrint("[lwIP, LibTCPConnectCallback] Done\n");
}

err_t
LibTCPConnect(struct tcp_pcb *pcb, struct ip_addr *ipaddr, u16_t port)
{
    struct connect_callback_msg *msg;
    err_t ret;

    DbgPrint("[lwIP, LibTCPConnect] Called\n");
    
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

        DbgPrint("[lwIP, LibTCPConnect] pcb = 0x%x\n", pcb);

        DbgPrint("[lwIP, LibTCPConnect] Done\n");
        
        return ret;
    }
    
    return ERR_MEM;
}

struct shutdown_callback_msg
{
    /* Synchronization */
    KEVENT Event;
    
    /* Input */
    struct tcp_pcb *Pcb;
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
    
    msg->Error = tcp_shutdown(msg->Pcb, msg->shut_rx, msg->shut_tx);
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPShutdown(struct tcp_pcb *pcb, int shut_rx, int shut_tx)
{
    struct shutdown_callback_msg *msg;
    err_t ret;
    
    DbgPrint("[lwIP, LibTCPShutdown] Called\n");
    
    if (!pcb)
    {
        DbgPrint("[lwIP, LibTCPShutdown] Done... NO pcb\n");
        return ERR_CLSD;
    }
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct shutdown_callback_msg));
    if (msg)
    {
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
        
        msg->Pcb = pcb;
        msg->shut_rx = shut_rx;
        msg->shut_tx = shut_tx;
                
        tcpip_callback_with_block(LibTCPShutdownCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        DbgPrint("[lwIP, LibTCPShutdown] pcb = 0x%x\n", pcb);
        
        DbgPrint("[lwIP, LibTCPShutdown] Done\n");
        
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

    DbgPrint("[lwIP, LibTCPClose] Called on pcb = 0x%x\n", pcb);
    
    if (!pcb)
    {
        DbgPrint("[lwIP, LibTCPClose] Done... NO pcb\n");
        return ERR_CLSD;
    }

    DbgPrint("[lwIP, LibTCPClose] Removing pcb callbacks\n");

    DbgPrint("[lwIP, LibTCPClose] pcb->state = %s\n", tcp_state_str[pcb->state]);

    tcp_arg(pcb, NULL);
    tcp_recv(pcb, NULL);
    tcp_sent(pcb, NULL);
    tcp_err(pcb, NULL);
    tcp_accept(pcb, NULL);

    DbgPrint("[lwIP, LibTCPClose] Attempting to allocate memory for msg\n");
    
    msg = ExAllocatePool(NonPagedPool, sizeof(struct close_callback_msg));
    if (msg)
    {
        DbgPrint("[lwIP, LibTCPClose] Initializing msg->Event\n");
        KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);

        DbgPrint("[lwIP, LibTCPClose] Initializing msg->pcb = 0x%x\n", pcb);
        msg->Pcb = pcb;

        DbgPrint("[lwIP, LibTCPClose] Attempting to call LibTCPCloseCallback\n");
        
        tcpip_callback_with_block(LibTCPCloseCallback, msg, 1);
        
        if (WaitForEventSafely(&msg->Event))
            ret = msg->Error;
        else
            ret = ERR_CLSD;
        
        ExFreePool(msg);
        
        DbgPrint("[lwIP, LibTCPClose] pcb = 0x%x\n", pcb);

        DbgPrint("[lwIP, LibTCPClose] Done\n");
        
        return ret;
    }

    DbgPrint("[lwIP, LibTCPClose] Failed to allocate memory\n");
    
    return ERR_MEM;
}

void
LibTCPAccept(struct tcp_pcb *pcb, struct tcp_pcb *listen_pcb, void *arg)
{
    DbgPrint("[lwIP, LibTCPAccept] Called. (pcb, arg) = (0x%x, 0x%x)\n", pcb, arg);
    
    ASSERT(arg);
    
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, InternalRecvEventHandler);
    tcp_sent(pcb, InternalSendEventHandler);
    tcp_arg(pcb, arg);
    
    tcp_accepted(listen_pcb);

    DbgPrint("[lwIP, LibTCPAccept] Done\n");
}

err_t
LibTCPGetHostName(struct tcp_pcb *pcb, struct ip_addr *ipaddr, u16_t *port)
{
    DbgPrint("[lwIP, LibTCPGetHostName] Called. pcb = (0x%x)\n", pcb);
    
    if (!pcb)
        return ERR_CLSD;
    
    *ipaddr = pcb->local_ip;
    *port = pcb->local_port;
    
    DbgPrint("[lwIP, LibTCPGetHostName] Got host port: %d\n", *port);

    DbgPrint("[lwIP, LibTCPGetHostName] Done\n");
    
    return ERR_OK;
}

err_t
LibTCPGetPeerName(struct tcp_pcb *pcb, struct ip_addr *ipaddr, u16_t *port)
{
    DbgPrint("[lwIP, LibTCPGetPeerName] pcb = (0x%x)\n", pcb);
    
    if (!pcb)
        return ERR_CLSD;
    
    *ipaddr = pcb->remote_ip;
    *port = pcb->remote_port;
    
    DbgPrint("[lwIP, LibTCPGetPeerName] Got remote port: %d\n", *port);
    
    return ERR_OK;
}
