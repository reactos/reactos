#ifndef OSKITTCP_H
#define OSKITTCP_H

#include <roscfg.h>
#ifdef KERNEL
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>

struct connect_args {
    int s;
    caddr_t name;
    int namelen;
};
#endif

#include <oskittypes.h>

#define IPHDR_SIZE 20

typedef void (*OSKITTCP_SOCKET_DATA_AVAILABLE)
    ( void *ClientData,
      void *WhichSocket,
      void *WhichConnection,
      OSK_PCHAR Data,
      OSK_UINT Len );
typedef void (*OSKITTCP_SOCKET_CONNECT_INDICATION)
    ( void *ClientData, 
      void *WhichSocket, 
      void *WhichConnection );
typedef void (*OSKITTCP_SOCKET_CLOSE_INDICATION)
    ( void *WhichSocket );
typedef void (*OSKITTCP_SOCKET_PENDING_CONNECT_INDICATION)
    ( void *WhichSocket );
typedef void (*OSKITTCP_SOCKET_RESET_INDICATION)
    ( void *WhichSocket );
typedef int (*OSKITTCP_SEND_PACKET)
    ( void *ClientData,
      void *WhichSocket,
      void *WhichConnection,
      OSK_PCHAR Data,
      OSK_UINT Len );
typedef int (*OSKITTCP_NEED_BIND)
    ( void *ClientData,
      void *WhichSocket, 
      void *WhichConnection,
      struct sockaddr *address, 
      OSK_UINT addrlen, 
      OSK_UINT reuseport );

typedef struct _OSKITTCP_EVENT_HANDLERS {
    void *ClientData;
    OSKITTCP_SOCKET_DATA_AVAILABLE SocketDataAvailable;
    OSKITTCP_SOCKET_CONNECT_INDICATION SocketConnectIndication;
    OSKITTCP_SOCKET_CLOSE_INDICATION SocketCloseIndication;
    OSKITTCP_SOCKET_PENDING_CONNECT_INDICATION SocketPendingConnectIndication;
    OSKITTCP_SOCKET_RESET_INDICATION SocketResetIndication;
    OSKITTCP_SEND_PACKET PacketSend;
    OSKITTCP_NEED_BIND Bind;
} OSKITTCP_EVENT_HANDLERS, *POSKITTCP_EVENT_HANDLERS;

extern OSKITTCP_EVENT_HANDLERS OtcpEvent;

extern void InitOskitTCP();
extern void DeinitOskitTCP();
extern void TimerOskitTCP();
extern int  OskitTCPSocket( void *Connection, void **ConnectionContext,
			    int Af, int Type, int Proto );
extern void RegisterOskitTCPEventHandlers
( POSKITTCP_EVENT_HANDLERS EventHandlers );
extern void OskitTCPReceiveDatagram( OSK_PCHAR Data, OSK_UINT Len,
				     OSK_UINT IpHeaderLen );

#undef errno

#define malloc(x,...) fbsd_malloc(x,__FILE__,__LINE__)
#define free(x,...) fbsd_free(x,__FILE__,__LINE__)

/* Error codes */
#include <oskiterrno.h>

#define SOCK_MAXADDRLEN 255

#endif/*OSKITTCP_H*/
