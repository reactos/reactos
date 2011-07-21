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
InternalSendEventHandler(void *arg, struct tcp_pcb *pcb, const u16_t space)
{
    DbgPrint("[lwIP, InternalSendEventHandler] SendEvent (0x%x, 0x%x, %d)\n",
        arg, pcb, (unsigned int)space);

    ASSERT(pcb->sent != 0);
    
    /* Make sure the socket didn't get closed */
    if (!arg) return ERR_OK;
    
    TCPSendEventHandler(arg, space);

    DbgPrint("[lwIP, InternalSendEventHandler] Done\n");
    
    return ERR_OK;
}

static
err_t
InternalRecvEventHandler(void *arg, struct tcp_pcb *pcb, struct pbuf *p, const err_t err)
{
    u32_t len;

    DbgPrint("[lwIP, InternalRecvEventHandler] RecvEvent (0x%x, pcb = 0x%x, pbuf = 0x%x, err = %d)\n",
        arg, pcb, p, (unsigned int)err);

    ASSERT(pcb->recv != NULL);
    
    /* Make sure the socket didn't get closed */
    if (!arg)
    {
        if (p)
            pbuf_free(p);

        DbgPrint("[lwIP, InternalRecvEventHandler] Done ERR_OK 0 - socket got closed on us\n");

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

            DbgPrint("[lwIP, InternalRecvEventHandler] Done ERR_OK 1\n");

            return ERR_OK;
        }
        else if (len != 0)
        {
            DbgPrint("UNTESTED CASE: NOT ALL DATA TAKEN! EXTRA DATA MAY BE LOST!\n");
            
            tcp_recved(pcb, len);
            
            /* Possible memory leak of pbuf here? */
            DbgPrint("[lwIP, InternalRecvEventHandler] Done ERR_OK 2\n");
            
            return ERR_OK;
        }
        else
        {
            /* We want lwIP to store the pbuf on its queue for later */
            DbgPrint("[lwIP, InternalRecvEventHandler] Done ERR_TIMEOUT\n");
            return ERR_TIMEOUT;
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

    DbgPrint("[lwIP, InternalRecvEventHandler] Done ERR_OK 3\n");
    
    return ERR_OK;
}

static
err_t
InternalAcceptEventHandler(void *arg, struct tcp_pcb *newpcb, const err_t err)
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
InternalConnectEventHandler(void *arg, struct tcp_pcb *pcb, const err_t err)
{
    DbgPrint("[lwIP, InternalConnectEventHandler] ConnectEvent (0x%x, pcb = 0x%x, err = %d)\n",
        arg, pcb, (unsigned int)err);
    
    /* Make sure the socket didn't get closed */
    if (!arg)
        return ERR_OK;
    
    TCPConnectEventHandler(arg, err);

    DbgPrint("[lwIP, InternalConnectEventHandler] Done\n");
    
    return ERR_OK;
}

static
void
InternalErrorEventHandler(void *arg, const err_t err)
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
    struct tcp_pcb *NewPcb;
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
        
        return ret;
    }

    DbgPrint("[lwIP, LibTCPSocket] Done\n");
    
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
    
    msg->Error = tcp_bind((PTCP_PCB)msg->Connection->SocketContext,
                            msg->IpAddress,
                            ntohs(msg->Port));
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPBind(PCONNECTION_ENDPOINT Connection, struct ip_addr *const ipaddr, const u16_t port)
{
    struct bind_callback_msg *msg;
    err_t ret;
    
    if (!Connection->SocketContext)
        return ERR_CLSD;

    DbgPrint("[lwIP, LibTCPBind] Called\n");
    
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
        
        DbgPrint("[lwIP, LibTCPBind] pcb = 0x%x\n", Connection->SocketContext);

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

    DbgPrint("[lwIP, LibTCPListenCallback] Called\n");

    msg->NewPcb = tcp_listen_with_backlog((PTCP_PCB)msg->Connection->SocketContext, msg->Backlog);
    
    if (msg->NewPcb)
    {
        tcp_accept(msg->NewPcb, InternalAcceptEventHandler);
    }

    DbgPrint("[lwIP, LibTCPListenCallback] Done\n");
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

PTCP_PCB
LibTCPListen(PCONNECTION_ENDPOINT Connection, const u8_t backlog)
{
    struct listen_callback_msg *msg;
    PTCP_PCB ret;

    DbgPrint("[lwIP, LibTCPListen] Called on pcb = 0x%x\n", Connection->SocketContext);
    
    if (!Connection->SocketContext)
        return NULL;
    
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
        
        DbgPrint("[lwIP, LibTCPListen] pcb = 0x%x, newpcb = 0x%x, sizeof(pcb) = %d \n",
            Connection->SocketContext, ret, sizeof(struct tcp_pcb));

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
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPSend(PCONNECTION_ENDPOINT Connection, void *const dataptr, const u16_t len, const int safe)
{
    err_t ret;
    
    if (!Connection->SocketContext)
        return ERR_CLSD;

    /*  
        If  we're being called from a handler it means we're in the conetxt of teh tcpip
        main thread. Therefore we don't have to queue our request via a callback and we
        can execute immediately.
    */
    if (safe)
    {
        if (tcp_sndbuf((PTCP_PCB)Connection->SocketContext) < len)
        {
            ret = ERR_INPROGRESS;
        }
        else
        {
            ret = tcp_write((PTCP_PCB)Connection->SocketContext, dataptr, len, TCP_WRITE_FLAG_COPY);
            tcp_output((PTCP_PCB)Connection->SocketContext);
        }

        return ret;
    }
    else
    {
        struct send_callback_msg *msg;

        msg = ExAllocatePool(NonPagedPool, sizeof(struct send_callback_msg));
        if (msg)
        {
            KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);
            msg->Connection = Connection;
            msg->Data = dataptr;
            msg->DataLength = len;
        
            tcpip_callback_with_block(LibTCPSendCallback, msg, 1);
        
            if (WaitForEventSafely(&msg->Event))
                ret = msg->Error;
            else
                ret = ERR_CLSD;
        
            DbgPrint("[lwIP, LibTCPSend] pcb = 0x%x\n", Connection->SocketContext);
        
            ExFreePool(msg);
        
            return ret;
        }
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

    DbgPrint("[lwIP, LibTCPConnectCallback] Called\n");
    
    ASSERT(arg);
    
    tcp_recv((PTCP_PCB)msg->Connection->SocketContext, InternalRecvEventHandler);
    tcp_sent((PTCP_PCB)msg->Connection->SocketContext, InternalSendEventHandler);

    err_t Error = tcp_connect((PTCP_PCB)msg->Connection->SocketContext,
                                msg->IpAddress, ntohs(msg->Port),
                                InternalConnectEventHandler);

    msg->Error = Error == ERR_OK ? ERR_INPROGRESS : Error;
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);

    DbgPrint("[lwIP, LibTCPConnectCallback] Done\n");
}

err_t
LibTCPConnect(PCONNECTION_ENDPOINT Connection, struct ip_addr *const ipaddr, const u16_t port)
{
    struct connect_callback_msg *msg;
    err_t ret;

    DbgPrint("[lwIP, LibTCPConnect] Called\n");
    
    if (!Connection->SocketContext)
        return ERR_CLSD;
    
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

        DbgPrint("[lwIP, LibTCPConnect] pcb = 0x%x\n", Connection->SocketContext);

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

    /*
        We check here if the pcb is in state ESTABLISHED or SYN_RECV because otherwise
        it means lwIP will take care of it anyway and if it does so before us it will
        cause memory corruption.
    */
    if ((((PTCP_PCB)msg->Connection->SocketContext)->state == ESTABLISHED) ||
        (((PTCP_PCB)msg->Connection->SocketContext)->state == SYN_RCVD))
    {
        msg->Error = 
            tcp_shutdown((PTCP_PCB)msg->Connection->SocketContext,
                msg->shut_rx, msg->shut_tx);
    }
    else
        msg->Error = ERR_OK;
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPShutdown(PCONNECTION_ENDPOINT Connection, const int shut_rx, const int shut_tx)
{
    struct shutdown_callback_msg *msg;
    err_t ret;
    
    DbgPrint("[lwIP, LibTCPShutdown] Called on pcb = 0x%x, rx = %d, tx = %d\n",
        Connection->SocketContext, shut_rx, shut_tx);
    
    if (!Connection->SocketContext)
    {
        DbgPrint("[lwIP, LibTCPShutdown] Done... NO pcb\n");
        return ERR_CLSD;
    }

    DbgPrint("[lwIP, LibTCPShutdown] pcb->state = %s\n",
        tcp_state_str[((PTCP_PCB)Connection->SocketContext)->state]);
    
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
    PCONNECTION_ENDPOINT Connection;
    
    /* Output */
    err_t Error;
};

static
void
CloseCallbacks(struct tcp_pcb *pcb)
{
    tcp_arg(pcb, NULL);
    /*  
        if this pcb is not in LISTEN state than it has
        valid recv, send and err callbacks to cancel
    */
    if (pcb->state != LISTEN)
    {
        tcp_recv(pcb, NULL);
        //tcp_sent(pcb, NULL);
        //tcp_err(pcb, NULL);
    }

    tcp_accept(pcb, NULL);
}

static
void
LibTCPCloseCallback(void *arg)
{
    struct close_callback_msg *msg = arg;

    DbgPrint("[lwIP, LibTCPCloseCallback] pcb = 0x%x\n", (PTCP_PCB)msg->Connection->SocketContext);

    if (!msg->Connection->SocketContext)
    {
        DbgPrint("[lwIP, LibTCPCloseCallback] NULL pcb...bail, bail!!!\n");
        
        ASSERT(FALSE);

        msg->Error = ERR_OK;
        return;
    }

    CloseCallbacks((PTCP_PCB)msg->Connection->SocketContext);

    if (((PTCP_PCB)msg->Connection->SocketContext)->state == LISTEN)
    {
        DbgPrint("[lwIP, LibTCPCloseCallback] Closing a listener\n");
        msg->Error = tcp_close((PTCP_PCB)msg->Connection->SocketContext);
    }
    else
    {
        DbgPrint("[lwIP, LibTCPCloseCallback] Aborting a connection\n");
        tcp_abort((PTCP_PCB)msg->Connection->SocketContext);
        msg->Error = ERR_OK;
    }
    
    KeSetEvent(&msg->Event, IO_NO_INCREMENT, FALSE);
}

err_t
LibTCPClose(PCONNECTION_ENDPOINT Connection, const int safe)
{
    err_t ret;

    DbgPrint("[lwIP, LibTCPClose] Called on pcb = 0x%x\n", Connection->SocketContext);
    
    if (!Connection->SocketContext)
    {
        DbgPrint("[lwIP, LibTCPClose] Done... NO pcb\n");
        return ERR_CLSD;
    }

    DbgPrint("[lwIP, LibTCPClose] pcb->state = %s\n",
        tcp_state_str[((PTCP_PCB)Connection->SocketContext)->state]);

    /*  
        If  we're being called from a handler it means we're in the conetxt of teh tcpip
        main thread. Therefore we don't have to queue our request via a callback and we
        can execute immediately.
    */
    if (safe)
    {
        CloseCallbacks((PTCP_PCB)Connection->SocketContext);
        if ( ((PTCP_PCB)Connection->SocketContext)->state == LISTEN )
        {
            DbgPrint("[lwIP, LibTCPClose] Closing a listener\n");
            ret = tcp_close((PTCP_PCB)Connection->SocketContext);
        }
        else
        {
            DbgPrint("[lwIP, LibTCPClose] Aborting a connection\n");
            tcp_abort((PTCP_PCB)Connection->SocketContext);
            ret = ERR_OK;
        }

        return ret;
    }
    else
    {
        struct close_callback_msg *msg;

        msg = ExAllocatePool(NonPagedPool, sizeof(struct close_callback_msg));
        if (msg)
        {
            KeInitializeEvent(&msg->Event, NotificationEvent, FALSE);

            msg->Connection = Connection;
        
            tcpip_callback_with_block(LibTCPCloseCallback, msg, 1);
        
            if (WaitForEventSafely(&msg->Event))
                ret = msg->Error;
            else
                ret = ERR_CLSD;
        
            ExFreePool(msg);

            DbgPrint("[lwIP, LibTCPClose] Done\n");
        
            return ret;
        }
    }

    DbgPrint("[lwIP, LibTCPClose] Failed to allocate memory\n");
    
    return ERR_MEM;
}

void
LibTCPAccept(PTCP_PCB pcb, struct tcp_pcb *listen_pcb, void *arg)
{
    DbgPrint("[lwIP, LibTCPAccept] Called. (pcb, arg) = (0x%x, 0x%x)\n", pcb, arg);
    
    ASSERT(arg);
    
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, InternalRecvEventHandler);
    tcp_sent(pcb, InternalSendEventHandler);
    tcp_err(pcb, InternalErrorEventHandler);
    tcp_arg(pcb, arg);
    
    tcp_accepted(listen_pcb);

    DbgPrint("[lwIP, LibTCPAccept] Done\n");
}

err_t
LibTCPGetHostName(PTCP_PCB pcb, struct ip_addr *const ipaddr, u16_t *const port)
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
LibTCPGetPeerName(PTCP_PCB pcb, struct ip_addr * const ipaddr, u16_t * const port)
{
    DbgPrint("[lwIP, LibTCPGetPeerName] pcb = (0x%x)\n", pcb);
    
    if (!pcb)
        return ERR_CLSD;
    
    *ipaddr = pcb->remote_ip;
    *port = pcb->remote_port;
    
    DbgPrint("[lwIP, LibTCPGetPeerName] Got remote port: %d\n", *port);
    
    return ERR_OK;
}
