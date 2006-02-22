#ifndef OSKITTCP_H
#define OSKITTCP_H

#ifdef linux
#include <netinet/in.h>
#endif

#ifndef _MSC_VER
#include <roscfg.h>
#endif/*_MSC_VER*/
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

typedef int (*OSKITTCP_SOCKET_STATE)
    ( void *ClientData,
      void *WhichSocket,
      void *WhichConnection,
      OSK_UINT NewState );

typedef int (*OSKITTCP_SEND_PACKET)
    ( void *ClientData,
      OSK_PCHAR Data,
      OSK_UINT Len );

typedef struct ifaddr *(*OSKITTCP_FIND_INTERFACE)
    ( void *ClientData,
      OSK_UINT AddrType,
      OSK_UINT FindType,
      struct sockaddr *ReqAddr );

typedef void *(*OSKITTCP_MALLOC)
    ( void *ClientData,
      OSK_UINT Bytes,
      OSK_PCHAR File,
      OSK_UINT Line );

typedef void (*OSKITTCP_FREE)
    ( void *ClientData,
      void *data,
      OSK_PCHAR File,
      OSK_UINT Line );

typedef int (*OSKITTCP_SLEEP)
    ( void *ClientData, void *token, int priority, char *msg, int tmio );

typedef void (*OSKITTCP_WAKEUP)( void *ClientData, void *token );

typedef struct _OSKITTCP_EVENT_HANDLERS {
    void *ClientData;
    OSKITTCP_SOCKET_STATE SocketState;
    OSKITTCP_SEND_PACKET PacketSend;
    OSKITTCP_FIND_INTERFACE FindInterface;
    OSKITTCP_MALLOC TCPMalloc;
    OSKITTCP_FREE TCPFree;
    OSKITTCP_SLEEP Sleep;
    OSKITTCP_WAKEUP Wakeup;
} OSKITTCP_EVENT_HANDLERS, *POSKITTCP_EVENT_HANDLERS;

extern OSKITTCP_EVENT_HANDLERS OtcpEvent;

extern void InitOskitTCP();
extern void DeinitOskitTCP();
extern void TimerOskitTCP( int FastTimer, int SlowTimer );
extern void OskitDumpBuffer( OSK_PCHAR Data, OSK_UINT Len );
extern int  OskitTCPShutdown( void *socket, int disconn_type );
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
extern int OskitTCPSend( void *socket,
			 OSK_PCHAR Data,
			 OSK_UINT Len,
			 OSK_UINT *OutLen,
			 OSK_UINT Flags );

extern int OskitTCPConnect( void *socket, void *connection,
			    void *nam, OSK_UINT namelen );
extern int OskitTCPClose( void *socket );

extern int OskitTCPBind( void *socket, void *connection,
			 void *nam, OSK_UINT namelen );

extern int OskitTCPAccept( void *socket, void **new_socket,
			   void *addr_out,
			   OSK_UINT addr_len,
			   OSK_UINT *out_addr_len,
			   OSK_UINT finish_accept );

extern int OskitTCPListen( void *socket, int backlog );

extern int OskitTCPRecv( void *connection,
			 OSK_PCHAR Data,
			 OSK_UINT Len,
			 OSK_UINT *OutLen,
			 OSK_UINT Flags );

void OskitTCPGetAddress( void *socket,
			 OSK_UINT *LocalAddress,
			 OSK_UI16 *LocalPort,
			 OSK_UINT *RemoteAddress,
			 OSK_UI16 *RemotePort );

#undef errno

void *fbsd_malloc( unsigned int bytes, char *file, unsigned line, ... );
void fbsd_free( void *data, char *file, unsigned line, ... );
#if 0
#define malloc(x) fbsd_malloc(x,__FILE__,__LINE__)
#define free(x) fbsd_free(x,__FILE__,__LINE__)
#endif
#define kern_malloc(x,y,z) kern_malloc_needs_definition(x,y,z)
#define kern_free(x,y,z) kern_free_needs_definition(x,w,z)

/* Error codes */
#include <oskiterrno.h>

#define SOCK_MAXADDRLEN 255

#define OSK_MSG_OOB      0x01
#define OSK_MSG_PEEK     0x02
#define OSK_MSG_DONTWAIT 0x80

#endif/*OSKITTCP_H*/
