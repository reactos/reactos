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
#define SEL_CONNECT 1
#define SEL_FIN     2
#define SEL_RST     4
#define SEL_ABRT    8
#define SEL_READ    16
#define SEL_WRITE   32
#define SEL_ACCEPT  64
#define SEL_OOB     128
#define SEL_ERROR   256
#define SEL_FINOUT  512

typedef void (*OSKITTCP_SOCKET_STATE)
    ( void *ClientData,
      void *WhichSocket,
      void *WhichConnection,
      OSK_UINT SelFlags,
      OSK_UINT SocketState );
typedef int (*OSKITTCP_SEND_PACKET)
    ( void *ClientData,
      void *WhichSocket,
      void *WhichConnection,
      OSK_PCHAR Data,
      OSK_UINT Len );

typedef struct _OSKITTCP_EVENT_HANDLERS {
    void *ClientData;
    OSKITTCP_SOCKET_STATE SocketState;
    OSKITTCP_SEND_PACKET PacketSend;
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
extern int OskitTCPReceive( void *socket, 
			    void *Addr,
			    OSK_PCHAR Data,
			    OSK_UINT Len,
			    OSK_UINT *OutLen,
			    OSK_UINT Flags );
#undef errno

#define malloc(x,...) fbsd_malloc(x,__FILE__,__LINE__)
#define free(x,...) fbsd_free(x,__FILE__,__LINE__)

/* Error codes */
#include <oskiterrno.h>

#define SOCK_MAXADDRLEN 255

#define OSK_MSG_OOB      0x01
#define OSK_MSG_PEEK     0x02
#define OSK_MSG_DONTWAIT 0x80

#endif/*OSKITTCP_H*/
