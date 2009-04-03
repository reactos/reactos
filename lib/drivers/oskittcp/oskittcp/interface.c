#include <oskittcp.h>
#include <oskitdebug.h>
#include <net/raw_cb.h>

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

struct linker_set domain_set;

OSKITTCP_EVENT_HANDLERS OtcpEvent = { 0 };

//OSK_UINT OskitDebugTraceLevel = OSK_DEBUG_ULTRA;
OSK_UINT OskitDebugTraceLevel = 0;

/* SPL */
unsigned cpl;
unsigned net_imask;
unsigned volatile ipending;
struct timeval boottime;

void clock_init();
int _snprintf(char * buf, size_t cnt, const char *fmt, ...);

void *fbsd_malloc( unsigned int bytes, char *file, unsigned line, ... ) {
    if( !OtcpEvent.TCPMalloc ) panic("no malloc");
    return OtcpEvent.TCPMalloc
	( OtcpEvent.ClientData, (OSK_UINT)bytes, (OSK_PCHAR)file, line );
}

void fbsd_free( void *data, char *file, unsigned line, ... ) {
    if( !OtcpEvent.TCPFree ) panic("no free");
    OtcpEvent.TCPFree( OtcpEvent.ClientData, data, (OSK_PCHAR)file, line );
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

void TimerOskitTCP( int FastTimer, int SlowTimer ) {
    if ( SlowTimer ) {
        tcp_slowtimo();
    }
    if ( FastTimer ) {
        tcp_fasttimo();
    }
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
			_snprintf ( line, sizeof(line)-1, "%08x:                                                                  \n", &Data[i] );
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
	so->so_error = 0;
        so->so_q = so->so_q0 = NULL;
        so->so_qlen = 0;
        so->so_head = NULL;
	*aso = so;
    }
    return error;
}

int OskitTCPRecv( void *connection,
		  OSK_PCHAR Data,
		  OSK_UINT Len,
		  OSK_UINT *OutLen,
		  OSK_UINT Flags ) {
    struct uio uio = { 0 };
    struct iovec iov = { 0 };
    int error = 0;
    int tcp_flags = 0;

    *OutLen = 0;

    OS_DbgPrint(OSK_MID_TRACE,
                ("so->so_state %x\n", ((struct socket *)connection)->so_state));

    if( Flags & OSK_MSG_OOB )      tcp_flags |= MSG_OOB;
    if( Flags & OSK_MSG_DONTWAIT ) tcp_flags |= MSG_DONTWAIT;
    if( Flags & OSK_MSG_PEEK )     tcp_flags |= MSG_PEEK;

    uio.uio_resid = Len;
    uio.uio_iov = &iov;
    uio.uio_rw = UIO_READ;
    uio.uio_iovcnt = 1;
    iov.iov_len = Len;
    iov.iov_base = (char *)Data;

    OS_DbgPrint(OSK_MID_TRACE,("Reading %d bytes from TCP:\n", Len));

    error = soreceive( connection, NULL, &uio, NULL, NULL /* SCM_RIGHTS */,
		       &tcp_flags );

    if( error == 0 ) {
	*OutLen = Len - uio.uio_resid;
    }

    return error;
}

int OskitTCPBind( void *socket, void *connection,
		  void *nam, OSK_UINT namelen ) {
    int error = EFAULT;
    struct socket *so = socket;
    struct mbuf sabuf;
    struct sockaddr addr;

    OS_DbgPrint(OSK_MID_TRACE,("Called, socket = %08x\n", socket));

    if( nam )
	addr = *((struct sockaddr *)nam);

    RtlZeroMemory(&sabuf, sizeof(sabuf));
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
    int error = EFAULT;
    struct mbuf sabuf;
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

    RtlZeroMemory(&sabuf, sizeof(sabuf));
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

int OskitTCPShutdown( void *socket, int disconn_type ) {
    return soshutdown( socket, disconn_type );
}

int OskitTCPClose( void *socket ) {
    struct socket *so = socket;
    so->so_connection = 0;
    soclose( so );
    return 0;
}

int OskitTCPSend( void *socket, OSK_PCHAR Data, OSK_UINT Len,
		  OSK_UINT *OutLen, OSK_UINT flags ) {
    int error;
    struct uio uio;
    struct iovec iov;

    iov.iov_len = Len;
    iov.iov_base = (char *)Data;
    uio.uio_iov = &iov;
    uio.uio_iovcnt = 1;
    uio.uio_offset = 0;
    uio.uio_resid = Len;
    uio.uio_segflg = UIO_SYSSPACE;
    uio.uio_rw = UIO_WRITE;
    uio.uio_procp = NULL;

    error = sosend( socket, NULL, &uio, NULL, NULL, 0 );
    if (OSK_EWOULDBLOCK == error) {
	((struct socket *) socket)->so_snd.sb_flags |= SB_WAIT;
    }
    *OutLen = uio.uio_offset;

    return error;
}

int OskitTCPAccept( void *socket,
		    void **new_socket,
		    void *AddrOut,
		    OSK_UINT AddrLen,
		    OSK_UINT *OutAddrLen,
		    OSK_UINT FinishAccepting ) {
    struct socket *head = (void *)socket;
    struct sockaddr *name = (struct sockaddr *)AddrOut;
    struct socket **newso = (struct socket **)new_socket;
    struct socket *so = socket;
    struct sockaddr_in sa;
    struct mbuf mnam;
    struct inpcb *inp;
    int namelen = 0, error = 0, s;

    OS_DbgPrint(OSK_MID_TRACE,("OSKITTCP: Doing accept (Finish %d)\n",
			       FinishAccepting));

    *OutAddrLen = AddrLen;

    if (name)
	/* that's a copyin actually */
	namelen = *OutAddrLen;

    s = splnet();

#if 0
    if ((head->so_options & SO_ACCEPTCONN) == 0) {
	splx(s);
	OS_DbgPrint(OSK_MID_TRACE,("OSKITTCP: head->so_options = %x, wanted bit %x\n",
				   head->so_options, SO_ACCEPTCONN));
	error = EINVAL;
	goto out;
    }
#endif

    OS_DbgPrint(OSK_MID_TRACE,("head->so_q = %x, head->so_state = %x\n",
			       head->so_q, head->so_state));

    if ((head->so_state & SS_NBIO) && head->so_q == NULL) {
	splx(s);
	error = EWOULDBLOCK;
	goto out;
    }

    OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
    while (head->so_q == NULL && head->so_error == 0) {
	if (head->so_state & SS_CANTRCVMORE) {
	    head->so_error = ECONNABORTED;
	    break;
	}
	OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
	error = tsleep((caddr_t)&head->so_timeo, PSOCK | PCATCH,
		       "accept", 0);
	if (error) {
	    splx(s);
	    goto out;
	}
	OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
    }
    OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));

#if 0
    if (head->so_error) {
	OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
	error = head->so_error;
	head->so_error = 0;
	splx(s);
	goto out;
    }
    OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
#endif

    /*
     * At this point we know that there is at least one connection
     * ready to be accepted. Remove it from the queue.
     */
    so = head->so_q;

    inp = so ? (struct inpcb *)so->so_pcb : NULL;
    if( inp ) {
        ((struct sockaddr_in *)AddrOut)->sin_addr.s_addr =
            inp->inp_faddr.s_addr;
        ((struct sockaddr_in *)AddrOut)->sin_port = inp->inp_fport;
    }

    OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
    if( FinishAccepting ) {
	head->so_q = so->so_q;
	head->so_qlen--;

	*newso = so;

	/*so->so_state &= ~SS_COMP;*/

	mnam.m_data = (char *)&sa;
	mnam.m_len = sizeof(sa);

	(void) soaccept(so, &mnam);

	so->so_state = SS_NBIO | SS_ISCONNECTED;
        so->so_q = so->so_q0 = NULL;
        so->so_qlen = 0;
        so->so_head = 0;

	OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
	if (name) {
	    /* check sa_len before it is destroyed */
	    memcpy( AddrOut, &sa, AddrLen < sizeof(sa) ? AddrLen : sizeof(sa) );
	    OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
	    *OutAddrLen = namelen;	/* copyout actually */
	}
	OS_DbgPrint(OSK_MID_TRACE,("error = %d\n", error));
	splx(s);
    }
out:
    OS_DbgPrint(OSK_MID_TRACE,("OSKITTCP: Returning %d\n", error));
    return (error);
}

/* The story so far
 *
 * We have a packet.  While we store the fields we want in host byte order
 * outside the original packet, the bsd stack modifies them in place.
 */

void OskitTCPReceiveDatagram( OSK_PCHAR Data, OSK_UINT Len,
			      OSK_UINT IpHeaderLen ) {
    struct mbuf *Ip = m_devget( (char *)Data, Len, 0, NULL, NULL );
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
    int error;

    OS_DbgPrint(OSK_MID_TRACE,("Called, socket = %08x\n", socket));
    error = solisten( socket, backlog );
    OS_DbgPrint(OSK_MID_TRACE,("Ending: %08x\n", error));

    return error;
}

void OskitTCPSetAddress( void *socket,
			 OSK_UINT LocalAddress,
			 OSK_UI16 LocalPort,
			 OSK_UINT RemoteAddress,
			 OSK_UI16 RemotePort ) {
    struct socket *so = socket;
    struct inpcb *inp = (struct inpcb *)so->so_pcb;
    inp->inp_laddr.s_addr = LocalAddress;
    inp->inp_lport = LocalPort;
    inp->inp_faddr.s_addr = RemoteAddress;
    inp->inp_fport = RemotePort;
}

void OskitTCPGetAddress( void *socket,
			 OSK_UINT *LocalAddress,
			 OSK_UI16 *LocalPort,
			 OSK_UINT *RemoteAddress,
			 OSK_UI16 *RemotePort ) {
    struct socket *so = socket;
    struct inpcb *inp = so ? (struct inpcb *)so->so_pcb : NULL;
    if( inp ) {
	*LocalAddress = inp->inp_laddr.s_addr;
	*LocalPort = inp->inp_lport;
	*RemoteAddress = inp->inp_faddr.s_addr;
	*RemotePort = inp->inp_fport;
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
       sin = (struct sockaddr_in *)&ifaddr->ifa_addr;

       OS_DbgPrint(OSK_MID_TRACE,("ifaddr->addr = %x\n",
                                  sin->sin_addr.s_addr));
    }

    return ifaddr;
}

