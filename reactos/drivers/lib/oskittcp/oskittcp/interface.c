#include <oskittcp.h>
#include <oskitdebug.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/uio.h>

#ifdef WIN32
#define snprintf _snprintf
#endif//WIN32

struct linker_set domain_set;

OSKITTCP_EVENT_HANDLERS OtcpEvent = { 0 };

OSK_UINT OskitDebugTraceLevel = OSK_DEBUG_ULTRA;

/* SPL */
unsigned cpl;
unsigned net_imask;
unsigned volatile ipending;
struct timeval boottime;

void *fbsd_malloc( unsigned int bytes, ... ) {
    if( !OtcpEvent.TCPMalloc ) panic("no malloc");
    return OtcpEvent.TCPMalloc
	( OtcpEvent.ClientData, (OSK_UINT)bytes, "*", 0 );
}

void fbsd_free( void *data, ... ) {
    if( !OtcpEvent.TCPFree ) panic("no free");
    OtcpEvent.TCPFree( OtcpEvent.ClientData, data, "*", 0 );
}

void InitOskitTCP() {
    OS_DbgPrint(OSK_MID_TRACE,("Init Called\n"));
    OS_DbgPrint(OSK_MID_TRACE,("MB Init\n"));
    mbinit();
    OS_DbgPrint(OSK_MID_TRACE,("Rawip Init\n"));
    rip_init();
    raw_init();
    OS_DbgPrint(OSK_MID_TRACE,("Route Init\n"));
    route_init();
    OS_DbgPrint(OSK_MID_TRACE,("Init fake freebsd scheduling\n"));
    init_freebsd_sched();
    OS_DbgPrint(OSK_MID_TRACE,("Init clock\n"));
    clock_init();
    OS_DbgPrint(OSK_MID_TRACE,("Init TCP\n"));
    tcp_init();
    OS_DbgPrint(OSK_MID_TRACE,("Init routing\n"));
    domaininit();
    OS_DbgPrint(OSK_MID_TRACE,("Init Finished\n"));
    tcp_iss = 1024;
}

void DeinitOskitTCP() {
}

void TimerOskitTCP() {
    tcp_slowtimo();
    tcp_fasttimo();
}

void RegisterOskitTCPEventHandlers( POSKITTCP_EVENT_HANDLERS EventHandlers ) {
    memcpy( &OtcpEvent, EventHandlers, sizeof(OtcpEvent) );
    if( OtcpEvent.PacketSend ) 
	OS_DbgPrint(OSK_MID_TRACE,("SendPacket handler registered: %x\n",
				   OtcpEvent.PacketSend));
}

void OskitDumpBuffer( OSK_PCHAR Data, OSK_UINT Len )
{
	unsigned int i;
	char line[81];
	static const char* hex = "0123456789abcdef";

	for ( i = 0; i < Len; i++ )
	{
		int align = i & 0xf;
		int align3 = (align<<1) + align;
		unsigned char c = Data[i];
		if ( !align )
		{
			if ( i ) DbgPrint( line );
			snprintf ( line, sizeof(line)-1, "%08x:                                                                  \n", &Data[i] );
			line[sizeof(line)-1] = '\0';
		}

		line[10+align3] = hex[(c>>4)&0xf];
		line[11+align3] = hex[c&0xf];
		if ( !isprint(c) )
			c = '.';
		line[59+align] = c;
	}
	if ( Len & 0xf )
		DbgPrint ( line );
}

/* From uipc_syscalls.c */

int OskitTCPSocket( void *context,
		    void **aso, 
		    int domain, 
		    int type, 
		    int proto ) 
{
    struct socket *so;
    int error = socreate(domain, &so, type, proto);
    if( !error ) {
	so->so_connection = context;
	so->so_state = SS_NBIO;
	*aso = so;
    }
    return error;
}

int OskitTCPRecv( void *connection,
		  OSK_PCHAR Data,
		  OSK_UINT Len,
		  OSK_UINT *OutLen,
		  OSK_UINT Flags ) {
    char *output_ptr = Data;
    struct uio uio = { 0 };
    struct iovec iov = { 0 };
    int error = 0;
    int tcp_flags = 0;
    int tocopy = 0;

    *OutLen = 0;

    if( Flags & OSK_MSG_OOB )      tcp_flags |= MSG_OOB;
    if( Flags & OSK_MSG_DONTWAIT ) tcp_flags |= MSG_DONTWAIT;
    if( Flags & OSK_MSG_PEEK )     tcp_flags |= MSG_PEEK;

    uio.uio_resid = Len;
    uio.uio_iov = &iov;
    uio.uio_rw = UIO_READ;
    uio.uio_iovcnt = 1;
    iov.iov_len = Len;
    iov.iov_base = Data;

    OS_DbgPrint(OSK_MID_TRACE,("Reading %d bytes from TCP:\n", Len));
	
    error = soreceive( connection, NULL, &uio, NULL, NULL /* SCM_RIGHTS */, 
		       &tcp_flags );

    if( error == 0 ) {
	*OutLen = Len - uio.uio_resid;
    }

    return error;
}
		  
static int
getsockaddr(namp, uaddr, len)
/* [<][>][^][v][top][bottom][index][help] */
    struct sockaddr **namp;
caddr_t uaddr;
size_t len;
{
    struct sockaddr *sa;
    int error;

    if (len > SOCK_MAXADDRLEN)
	return ENAMETOOLONG;
    MALLOC(sa, struct sockaddr *, len, M_SONAME, M_WAITOK);
    error = copyin(uaddr, sa, len);
    if (error) {
	FREE(sa, M_SONAME);
    } else {
	*namp = sa;
    }
    return error;
}

int OskitTCPBind( void *socket, void *connection,
		  void *nam, OSK_UINT namelen ) {
    int error = EFAULT;
    struct socket *so = socket;
    struct mbuf sabuf = { 0 };
    struct sockaddr addr;

    OS_DbgPrint(OSK_MID_TRACE,("Called, socket = %08x\n", socket));

    if( nam )
	addr = *((struct sockaddr *)nam);

    sabuf.m_data = (void *)&addr;
    sabuf.m_len = sizeof(addr);
    
    addr.sa_family = addr.sa_len;
    addr.sa_len = sizeof(struct sockaddr);

    error = sobind(so, &sabuf);

    OS_DbgPrint(OSK_MID_TRACE,("Ending: %08x\n", error));
    return (error);    
}

int OskitTCPConnect( void *socket, void *connection, 
		     void *nam, OSK_UINT namelen ) {
    struct socket *so = socket;
    struct connect_args _uap = {
	0, nam, namelen
    }, *uap = &_uap;
    int error = EFAULT, s;
    struct mbuf sabuf = { 0 };
    struct sockaddr addr;

    OS_DbgPrint(OSK_MID_TRACE,("Called, socket = %08x\n", socket));

    so->so_connection = connection;

    if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
	error = EALREADY;
	goto done;
    }

    OS_DbgPrint(OSK_MIN_TRACE,("Nam: %x\n", nam));
    if( nam )
	addr = *((struct sockaddr *)nam);

    sabuf.m_data = (void *)&addr;
    sabuf.m_len = sizeof(addr);
    
    addr.sa_family = addr.sa_len;
    addr.sa_len = sizeof(struct sockaddr);

    error = soconnect(so, &sabuf);

    if (error)
	goto bad;

    if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
	error = EINPROGRESS;
	goto done;
    }

bad:
    so->so_state &= ~SS_ISCONNECTING;

    if (error == ERESTART)
	error = EINTR;

done:
    OS_DbgPrint(OSK_MID_TRACE,("Ending: %08x\n", error));
    return (error);    
}

int OskitTCPClose( void *socket ) {
    struct socket *so = socket;
    so->so_connection = 0;
    soclose( so );
    return 0;
}

int OskitTCPSend( void *socket, OSK_PCHAR Data, OSK_UINT Len, 
		  OSK_UINT *OutLen, OSK_UINT flags ) {
    struct mbuf* m = m_devget( Data, Len, 0, NULL, NULL );
    int error = 0;
	if ( !m )
		return ENOBUFS;
    error = sosend( socket, NULL, NULL, m, NULL, 0 );
    *OutLen = Len;
    return error;
}

int OskitTCPAccept( void *socket, 
		    void *AddrOut, 
		    OSK_UINT AddrLen,
		    OSK_UINT *OutAddrLen ) {
    struct mbuf nam;
    int error;

    nam.m_data = AddrOut;
    nam.m_len  = AddrLen;

    return soaccept( socket, &nam );
}

/* The story so far
 * 
 * We have a packet.  While we store the fields we want in host byte order
 * outside the original packet, the bsd stack modifies them in place.
 */

void OskitTCPReceiveDatagram( OSK_PCHAR Data, OSK_UINT Len, 
			      OSK_UINT IpHeaderLen ) {
    struct mbuf *Ip = m_devget( Data, Len, 0, NULL, NULL );
    struct ip *iph;
    
    if( !Ip ) return; /* drop the segment */

    //memcpy( Ip->m_data, Data, Len );
    Ip->m_pkthdr.len = IpHeaderLen;

    /* Do the transformations on the header that tcp_input expects */
    iph = mtod(Ip, struct ip *);
    NTOHS(iph->ip_len);
    iph->ip_len -= sizeof(struct ip);

    OS_DbgPrint(OSK_MAX_TRACE, 
		("OskitTCPReceiveDatagram: %d (%d header) Bytes\n", Len,
		 IpHeaderLen));

    tcp_input(Ip, IpHeaderLen);

    /* The buffer Ip is freed by tcp_input */
}

int OskitTCPListen( void *socket, int backlog ) {
    return solisten( socket, backlog );
}

void OskitTCPSetAddress( void *socket, 
			 OSK_UINT LocalAddress,
			 OSK_UI16 LocalPort,
			 OSK_UINT RemoteAddress,
			 OSK_UI16 RemotePort ) {
    struct socket *so = socket;
    struct inpcb *inp = so->so_pcb;
    inp->inp_laddr.s_addr = LocalAddress;
    inp->inp_lport = LocalPort;
    inp->inp_faddr.s_addr = RemoteAddress;
    inp->inp_fport = RemotePort;
    DbgPrint("OSKIT: SET ADDR %x:%x -> %x:%x\n", 
	     LocalAddress, LocalPort,
	     RemoteAddress, RemotePort);
}

void OskitTCPGetAddress( void *socket, 
			 OSK_UINT *LocalAddress,
			 OSK_UI16 *LocalPort,
			 OSK_UINT *RemoteAddress,
			 OSK_UI16 *RemotePort ) {
    struct socket *so = socket;
    struct inpcb *inp = so ? so->so_pcb : 0;
    if( inp ) {
	*LocalAddress = inp->inp_laddr.s_addr;
	*LocalPort = inp->inp_lport;
	*RemoteAddress = inp->inp_faddr.s_addr;
	*RemotePort = inp->inp_fport;
	DbgPrint("OSKIT: GET ADDR %x:%x -> %x:%x\n", 
		 *LocalAddress, *LocalPort,
		 *RemoteAddress, *RemotePort);
    }
}

struct ifaddr *ifa_iffind(struct sockaddr *addr, int type)
{
    if( OtcpEvent.FindInterface ) 
	return OtcpEvent.FindInterface( OtcpEvent.ClientData,
					PF_INET,
					type,
					addr );
    else
	return NULL;
}

void oskittcp_die( const char *file, int line ) {
    DbgPrint("\n\n*** OSKITTCP: Panic Called at %s:%d ***\n", file, line);
    *((int *)0) = 0;
}

/* Stuff supporting the BSD network-interface interface */
struct ifaddr **ifnet_addrs;
struct	ifnet *ifnet;

void
ifinit()
{
}


void
if_attach(ifp)
	struct ifnet *ifp;
{
    panic("if_attach\n");
}

struct ifnet *
ifunit(char *name)
{
	return 0;
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
void
if_slowtimo(arg)
	void *arg;
{
#if 0
	register struct ifnet *ifp;
	int s = splimp();

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp->if_unit);
	}
	splx(s);
	timeout(if_slowtimo, (void *)0, hz / IFNET_SLOWHZ);
#endif
}

/*
 * Locate an interface based on a complete address.
 */

/*ARGSUSED*/
struct ifaddr *ifa_ifwithaddr(addr)
    struct sockaddr *addr;
{
    struct ifaddr *ifaddr = ifa_ifwithnet( addr );
    struct sockaddr_in *addr_in;
    struct sockaddr_in *faddr_in;

    if( !ifaddr ) {
	OS_DbgPrint(OSK_MID_TRACE,("No ifaddr\n"));
	return NULL;
    } else {
	OS_DbgPrint(OSK_MID_TRACE,("ifaddr @ %x\n", ifaddr));
    }

    addr_in = (struct sockaddr_in *)addr;
    faddr_in = (struct sockaddr_in *)ifaddr->ifa_addr;

    if( faddr_in->sin_addr.s_addr == addr_in->sin_addr.s_addr )
	return ifaddr;
    else
	return NULL;
}

/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr)
	register struct sockaddr *addr;
{
    OS_DbgPrint(OSK_MID_TRACE,("Called\n"));
    return ifa_iffind(addr, IFF_POINTOPOINT);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is most specific found.
 */
struct ifaddr *ifa_ifwithnet(addr)
	struct sockaddr *addr;
{
    struct sockaddr_in *sin;
    struct ifaddr *ifaddr = ifa_iffind(addr, IFF_UNICAST);

    if( ifaddr )
    {
       sin = (struct sockaddr *)&ifaddr->ifa_addr;

       OS_DbgPrint(OSK_MID_TRACE,("ifaddr->addr = %x\n", 
                                  sin->sin_addr.s_addr));
    }

    return ifaddr;
}

