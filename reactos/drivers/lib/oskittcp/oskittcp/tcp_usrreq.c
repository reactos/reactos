/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	From: @(#)tcp_usrreq.c	8.2 (Berkeley) 1/3/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <vm/vm.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#ifdef TCPDEBUG
#include <netinet/tcp_debug.h>
#endif

/*
 * TCP protocol interface to socket abstraction.
 */
extern	char *tcpstates[];

/*
 * Process a TCP user request for TCP tb.  If this is a send request
 * then m is the mbuf chain of send data.  If this is a timer expiration
 * (called from the software clock routine), then timertype tells which timer.
 */
/*ARGSUSED*/
int
tcp_usrreq(so, req, m, nam, control)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
{
    register struct inpcb *inp;
    register struct tcpcb *tp = 0;
    struct sockaddr_in *sinp;
    int s;
    int error = 0;

    if (req == PRU_CONTROL)
	return (in_control(so, (u_long)m, (caddr_t)nam,
			   (struct ifnet *)control));
    if (control && control->m_len) {
	m_freem(control);
	if (m)
	    m_freem(m);
	return (EINVAL);
    }
    
    s = splnet();
    inp = sotoinpcb(so);

    /*
     * When a TCP is attached to a socket, then there will be
     * a (struct inpcb) pointed at by the socket, and this
     * structure will point at a subsidary (struct tcpcb).
     */
    if (inp == 0 && req != PRU_ATTACH) {
	splx(s);
	/* safer version of fix for mbuf leak */
	if (m && (req == PRU_SEND || req == PRU_SENDOOB))
	    m_freem(m);
    }

    if( inp )
	tp = intotcpcb(inp);
    
    switch (req) {
	/*
	 * TCP attaches to socket via PRU_ATTACH, reserving space,
	 * and an internet control block.
	 */
    case PRU_ATTACH:
	if (inp) {
	    error = EISCONN;
	    break;
	}
	error = tcp_attach(so);
	if (error)
	    break;
	if ((so->so_options & SO_LINGER) && so->so_linger == 0)
	    so->so_linger = TCP_LINGERTIME * hz;
	tp = sototcpcb(so);
	break;
	
	/*
	 * PRU_DETACH detaches the TCP protocol from the socket.
	 * If the protocol state is non-embryonic, then can't
	 * do this directly: have to initiate a PRU_DISCONNECT,
	 * which may finish later; embryonic TCB's can just
	 * be discarded here.
	 */
    case PRU_DETACH:
	if (tp->t_state > TCPS_LISTEN)
	    tp = tcp_disconnect(tp);
	else
	    tp = tcp_close(tp);
	break;
	
	/*
	 * Give the socket an address.
	 */
    case PRU_BIND:
	/*
	 * Must check for multicast addresses and disallow binding
	 * to them.
	 */
	sinp = mtod(nam, struct sockaddr_in *);
	error = in_pcbbind(inp, nam);
	if (error)
	    break;
	break;
	
	/*
	 * Prepare to accept connections.
	 */
    case PRU_LISTEN:
	if (inp->inp_lport == 0)
	    error = in_pcbbind(inp, NULL);
	if (error == 0)
	    tp->t_state = TCPS_LISTEN;
	break;
	
	/*
	 * Initiate connection to peer.
	 * Create a template for use in transmissions on this connection.
	 * Enter SYN_SENT state, and mark socket as connecting.
	 * Start keep-alive timer, and seed output sequence space.
	 * Send initial segment on connection.
	 */
    case PRU_CONNECT:
	/*
	 * Must disallow TCP ``connections'' to multicast addresses.
	 */
	sinp = mtod(nam, struct sockaddr_in *);
	if ((error = tcp_connect(tp, nam)) != 0) {
	    OS_DbgPrint(OSK_MID_TRACE,("TC: %d\n", error));
	    break;
	}
	error = tcp_output(tp);
	OS_DbgPrint(OSK_MID_TRACE,("TO: %d\n", error));
	break;
	
	/*
	 * Create a TCP connection between two sockets.
	 */
    case PRU_CONNECT2:
	error = EOPNOTSUPP;
	break;
	
	/*
	 * Initiate disconnect from peer.
	 * If connection never passed embryonic stage, just drop;
	 * else if don't need to let data drain, then can just drop anyways,
	 * else have to begin TCP shutdown process: mark socket disconnecting,
	 * drain unread data, state switch to reflect user close, and
	 * send segment (e.g. FIN) to peer.  Socket will be really disconnected
	 * when peer sends FIN and acks ours.
	 *
	 * SHOULD IMPLEMENT LATER PRU_CONNECT VIA REALLOC TCPCB.
	 */
    case PRU_DISCONNECT:
	tp = tcp_disconnect(tp);
	break;
	
	/*
	 * Accept a connection.  Essentially all the work is
	 * done at higher levels; just return the address
	 * of the peer, storing through addr.
	 */
    case PRU_ACCEPT:
	in_setpeeraddr(inp, nam);
	break;
	
	/*
	 * Mark the connection as being incapable of further output.
	 */
    case PRU_SHUTDOWN:
	socantsendmore(so);
	tp = tcp_usrclosed(tp);
	if (tp)
	    error = tcp_output(tp);
	break;
	
	/*
	 * After a receive, possibly send window update to peer.
	 */
    case PRU_RCVD:
	(void) tcp_output(tp);
	break;
	
	/*
	 * Do a send by putting data in output queue and updating urgent
	 * marker if URG set.  Possibly send more data.
	 */
    case PRU_SEND_EOF:
    case PRU_SEND:
	sbappend(so, &so->so_snd, m);
	if (nam && tp->t_state < TCPS_SYN_SENT) {
	    /*
	     * Do implied connect if not yet connected,
	     * initialize window to default value, and
	     * initialize maxseg/maxopd using peer's cached
	     * MSS.
	     */
	    error = tcp_connect(tp, nam);
	    if (error)
		break;
	    tp->snd_wnd = TTCP_CLIENT_SND_WND;
	    tcp_mss(tp, -1);
	}
	
	if (req == PRU_SEND_EOF) {
	    /*
	     * Close the send side of the connection after
	     * the data is sent.
	     */
	    socantsendmore(so);
	    tp = tcp_usrclosed(tp);
	}
	if (tp != NULL)
	    error = tcp_output(tp);
	break;
	
	/*
	 * Abort the TCP.
	 */
    case PRU_ABORT:
	tp = tcp_drop(tp, ECONNABORTED);
	break;
	
    case PRU_SENSE:
	((struct stat *) m)->st_blksize = so->so_snd.sb_hiwat;
	(void) splx(s);
	return (0);
	
    case PRU_RCVOOB:
	if ((so->so_oobmark == 0 &&
	     (so->so_state & SS_RCVATMARK) == 0) ||
	    so->so_options & SO_OOBINLINE ||
	    tp->t_oobflags & TCPOOB_HADDATA) {
	    error = EINVAL;
	    break;
	}
	if ((tp->t_oobflags & TCPOOB_HAVEDATA) == 0) {
	    error = EWOULDBLOCK;
	    break;
	}
	m->m_len = 1;
	*mtod(m, caddr_t) = tp->t_iobc;
	if (((int)nam & MSG_PEEK) == 0)
	    tp->t_oobflags ^= (TCPOOB_HAVEDATA | TCPOOB_HADDATA);
	break;
	
    case PRU_SENDOOB:
	if (sbspace(&so->so_snd) < -512) {
	    m_freem(m);
	    error = ENOBUFS;
	    break;
	}
	/*
	 * According to RFC961 (Assigned Protocols),
	 * the urgent pointer points to the last octet
	 * of urgent data.  We continue, however,
	 * to consider it to indicate the first octet
	 * of data past the urgent section.
	 * Otherwise, snd_up should be one lower.
	 */
	sbappend(so, &so->so_snd, m);
	if (nam && tp->t_state < TCPS_SYN_SENT) {
	    /*
	     * Do implied connect if not yet connected,
	     * initialize window to default value, and
	     * initialize maxseg/maxopd using peer's cached
	     * MSS.
	     */
	    error = tcp_connect(tp, nam);
	    if (error)
		break;
	    tp->snd_wnd = TTCP_CLIENT_SND_WND;
	    tcp_mss(tp, -1);
	}
	tp->snd_up = tp->snd_una + so->so_snd.sb_cc;
	tp->t_force = 1;
	error = tcp_output(tp);
	tp->t_force = 0;
	break;

    case PRU_SOCKADDR:
	in_setsockaddr(inp, nam);
	break;

    case PRU_PEERADDR:
	in_setpeeraddr(inp, nam);
	break;

	/*
	 * TCP slow timer went off; going through this
	 * routine for tracing's sake.
	 */
    case PRU_SLOWTIMO:
	tp = tcp_timers(tp, (int)nam);
#ifdef TCPDEBUG
	req |= (int)nam << 8;		/* for debug's sake */
#endif
	break;
	
    default:
	panic("tcp_usrreq");
    }
#ifdef TCPDEBUG
    if (tp && (so->so_options & SO_DEBUG))
	tcp_trace(TA_USER, ostate, tp, (struct tcpiphdr *)0, req);
#endif
    splx(s);
    return (error);
}

/*
 * Common subroutine to open a TCP connection to remote host specified
 * by struct sockaddr_in in mbuf *nam.  Call in_pcbbind to assign a local
 * port number if needed.  Call in_pcbladdr to do the routing and to choose
 * a local host address (interface).  If there is an existing incarnation
 * of the same connection in TIME-WAIT state and if the remote host was
 * sending CC options and if the connection duration was < MSL, then
 * truncate the previous TIME-WAIT state and proceed.
 * Initialize connection parameters and enter SYN-SENT state.
 */ 
int
tcp_connect(tp, nam)
	register struct tcpcb *tp;
	struct mbuf *nam;
{
    struct inpcb *inp/* = tp->t_inpcb */, *oinp;
    struct socket *so;
    struct tcpcb *otp;
    struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
    struct sockaddr_in ifaddr;
    int error;
    struct rmxp_tao *taop;
    struct rmxp_tao tao_noncached;

    if( !tp ) 
	panic( "No tcpcb provided.\n" );

    if( !tp->t_inpcb )
	panic( "No inpcb provided.\n" );

    inp = tp->t_inpcb;

    if( !inp->inp_socket )
	panic( "No socket provided.\n" );

    so = inp->inp_socket;

    if (inp->inp_lport == 0) {
	error = in_pcbbind(inp, NULL);
	if (error) {
	    OS_DbgPrint(OSK_MID_TRACE,("error %d\n", error));
	    return error;
	}
    }
    /*
     * Cannot simply call in_pcbconnect, because there might be an
     * earlier incarnation of this same connection still in
     * TIME_WAIT state, creating an ADDRINUSE error.
     */
    error = in_pcbladdr(inp, nam, &ifaddr);
    
    if (error) {
	OS_DbgPrint(OSK_MID_TRACE,("error %d\n", error));	    
	return error;
    }
    
    oinp = in_pcblookup(inp->inp_pcbinfo->listhead,
			sin->sin_addr, sin->sin_port,
			inp->inp_laddr.s_addr != INADDR_ANY ? inp->inp_laddr
			: ifaddr.sin_addr,
			inp->inp_lport,  0);
    
    if (oinp) {
    
	if (oinp != inp && (otp = intotcpcb(oinp)) != NULL &&
	    otp->t_state == TCPS_TIME_WAIT &&
	    otp->t_duration < TCPTV_MSL &&
	    (otp->t_flags & TF_RCVD_CC))
	    otp = tcp_close(otp);
	else {
    
	    OS_DbgPrint(OSK_MID_TRACE,("error EADDRINUSE\n"));
	    return EADDRINUSE;
	}
    
    }
    
    if (inp->inp_laddr.s_addr == INADDR_ANY)
	inp->inp_laddr = ifaddr.sin_addr;
    
    inp->inp_faddr = sin->sin_addr;
    inp->inp_fport = sin->sin_port;
    
    in_pcbrehash(inp);
    
    
    tp->t_template = tcp_template(tp);
    
    if (tp->t_template == 0) {
	in_pcbdisconnect(inp);
	OS_DbgPrint(OSK_MID_TRACE,("error ENOBUFS\n"));
	return ENOBUFS;
    }
    
    
    /* Compute window scaling to request.  */
    while (tp->request_r_scale < TCP_MAX_WINSHIFT &&
	   (TCP_MAXWIN << tp->request_r_scale) < so->so_rcv.sb_hiwat)
	tp->request_r_scale++;
    
    
    soisconnecting(so);
    
    tcpstat.tcps_connattempt++;
    tp->t_state = TCPS_SYN_SENT;
    tp->t_timer[TCPT_KEEP] = tcp_keepinit;
    tp->rcv_nxt = 0;
    tp->iss = tcp_iss; tcp_iss += TCP_ISSINCR/2;
    
    tcp_sendseqinit(tp);
    
    
    /*
     * Generate a CC value for this connection and
     * check whether CC or CCnew should be used.
     */
    if ((taop = tcp_gettaocache(tp->t_inpcb)) == NULL) {
	taop = &tao_noncached;
	bzero(taop, sizeof(*taop));
    }

    tp->cc_send = CC_INC(tcp_ccgen);
    if (taop->tao_ccsent != 0 &&
	CC_GEQ(tp->cc_send, taop->tao_ccsent)) {
	taop->tao_ccsent = tp->cc_send;
    } else {
	taop->tao_ccsent = 0;
	tp->t_flags |= TF_SENDCCNEW;
    }

    return 0;
}

int
tcp_ctloutput(op, so, level, optname, mp)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	int error = 0, s;
	struct inpcb *inp;
	register struct tcpcb *tp;
	register struct mbuf *m;
	register int i;

	s = splnet();
	inp = sotoinpcb(so);
	if (inp == NULL) {
		splx(s);
		if (op == PRCO_SETOPT && *mp)
			(void) m_free(*mp);
		return (ECONNRESET);
	}
	if (level != IPPROTO_TCP) {
		error = ip_ctloutput(op, so, level, optname, mp);
		splx(s);
		return (error);
	}
	tp = intotcpcb(inp);

	switch (op) {

	case PRCO_SETOPT:
		m = *mp;
		switch (optname) {

		case TCP_NODELAY:
			if (m == NULL || m->m_len < sizeof (int))
				error = EINVAL;
			else if (*mtod(m, int *))
				tp->t_flags |= TF_NODELAY;
			else
				tp->t_flags &= ~TF_NODELAY;
			break;

		case TCP_MAXSEG:
			if (m && (i = *mtod(m, int *)) > 0 && i <= tp->t_maxseg)
				tp->t_maxseg = i;
			else
				error = EINVAL;
			break;

		case TCP_NOOPT:
			if (m == NULL || m->m_len < sizeof (int))
				error = EINVAL;
			else if (*mtod(m, int *))
				tp->t_flags |= TF_NOOPT;
			else
				tp->t_flags &= ~TF_NOOPT;
			break;

		case TCP_NOPUSH:
			if (m == NULL || m->m_len < sizeof (int))
				error = EINVAL;
			else if (*mtod(m, int *))
				tp->t_flags |= TF_NOPUSH;
			else
				tp->t_flags &= ~TF_NOPUSH;
			break;

		default:
			error = ENOPROTOOPT;
			break;
		}
		if (m)
			(void) m_free(m);
		break;

	case PRCO_GETOPT:
		*mp = m = m_get(M_WAIT, MT_SOOPTS);
		m->m_len = sizeof(int);

		switch (optname) {
		case TCP_NODELAY:
			*mtod(m, int *) = tp->t_flags & TF_NODELAY;
			break;
		case TCP_MAXSEG:
			*mtod(m, int *) = tp->t_maxseg;
			break;
		case TCP_NOOPT:
			*mtod(m, int *) = tp->t_flags & TF_NOOPT;
			break;
		case TCP_NOPUSH:
			*mtod(m, int *) = tp->t_flags & TF_NOPUSH;
			break;
		default:
			error = ENOPROTOOPT;
			break;
		}
		break;
	}
	splx(s);
	return (error);
}

/*
 * tcp_sendspace and tcp_recvspace are the default send and receive window
 * sizes, respectively.  These are obsolescent (this information should
 * be set by the route).
 */
u_long	tcp_sendspace = 1024*16;
u_long	tcp_recvspace = 1024*16;

/*
 * Attach TCP protocol to socket, allocating
 * internet protocol control block, tcp control block,
 * bufer space, and entering LISTEN state if to accept connections.
 */
int
tcp_attach(so)
	struct socket *so;
{
	register struct tcpcb *tp;
	struct inpcb *inp;
	int error;

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, tcp_sendspace, tcp_recvspace);
		if (error)
			return (error);
	}
	error = in_pcballoc(so, &tcbinfo);
	if (error)
		return (error);
	inp = sotoinpcb(so);
	tp = tcp_newtcpcb(inp);
	if (tp == 0) {
		int nofd = so->so_state & SS_NOFDREF;	/* XXX */

		so->so_state &= ~SS_NOFDREF;	/* don't free the socket yet */
		in_pcbdetach(inp);
		so->so_state |= nofd;
		OS_DbgPrint(OSK_MID_TRACE,("ENOBUFS: no tcpcb allocated\n"));
		return (ENOBUFS);
	}
	tp->t_state = TCPS_CLOSED;
	return (0);
}

/*
 * Initiate (or continue) disconnect.
 * If embryonic state, just send reset (once).
 * If in ``let data drain'' option and linger null, just drop.
 * Otherwise (hard), mark socket disconnecting and drop
 * current input data; switch states based on user close, and
 * send segment to peer (with FIN).
 */
struct tcpcb *
tcp_disconnect(tp)
	register struct tcpcb *tp;
{
	struct socket *so = tp->t_inpcb->inp_socket;

	if (tp->t_state < TCPS_ESTABLISHED)
		tp = tcp_close(tp);
	else if ((so->so_options & SO_LINGER) && so->so_linger == 0)
		tp = tcp_drop(tp, 0);
	else {
		soisdisconnecting(so);
		sbflush(&so->so_rcv);
		tp = tcp_usrclosed(tp);
		if (tp)
			(void) tcp_output(tp);
	}
	return (tp);
}

/*
 * User issued close, and wish to trail through shutdown states:
 * if never received SYN, just forget it.  If got a SYN from peer,
 * but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
 * If already got a FIN from peer, then almost done; go to LAST_ACK
 * state.  In all other cases, have already sent FIN to peer (e.g.
 * after PRU_SHUTDOWN), and just have to play tedious game waiting
 * for peer to send FIN or not respond to keep-alives, etc.
 * We can let the user exit from the close as soon as the FIN is acked.
 */
struct tcpcb *
tcp_usrclosed(tp)
	register struct tcpcb *tp;
{

	switch (tp->t_state) {

	case TCPS_CLOSED:
	case TCPS_LISTEN:
		tp->t_state = TCPS_CLOSED;
		tp = tcp_close(tp);
		break;

	case TCPS_SYN_SENT:
	case TCPS_SYN_RECEIVED:
		tp->t_flags |= TF_NEEDFIN;
		break;

	case TCPS_ESTABLISHED:
		tp->t_state = TCPS_FIN_WAIT_1;
		break;

	case TCPS_CLOSE_WAIT:
		tp->t_state = TCPS_LAST_ACK;
		break;
	}
	if (tp && tp->t_state >= TCPS_FIN_WAIT_2) {
		soisdisconnected(tp->t_inpcb->inp_socket);
		/* To prevent the connection hanging in FIN_WAIT_2 forever. */
		if (tp->t_state == TCPS_FIN_WAIT_2)
			tp->t_timer[TCPT_2MSL] = tcp_maxidle;
	}
	return (tp);
}

/*
 * Sysctl for tcp variables.
 */
int
tcp_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
    /* All sysctl names at this level are terminal. */
    if (namelen != 1)
	return (ENOTDIR);
    
    switch (name[0]) {
    case TCPCTL_DO_RFC1323:
	return (sysctl_int(oldp, oldlenp, newp, newlen,
			   &tcp_do_rfc1323));
    case TCPCTL_DO_RFC1644:
	return (sysctl_int(oldp, oldlenp, newp, newlen,
			   &tcp_do_rfc1644));
    case TCPCTL_MSSDFLT:
	return (sysctl_int(oldp, oldlenp, newp, newlen,
			   &tcp_mssdflt));
    case TCPCTL_STATS:
	return (sysctl_rdstruct(oldp, oldlenp, newp, &tcpstat,
				sizeof tcpstat));
    case TCPCTL_RTTDFLT:
	return (sysctl_int(oldp, oldlenp, newp, newlen, &tcp_rttdflt));
    case TCPCTL_KEEPIDLE:
	return (sysctl_int(oldp, oldlenp, newp, newlen,
			   &tcp_keepidle));
    case TCPCTL_KEEPINTVL:
	return (sysctl_int(oldp, oldlenp, newp, newlen,
			   &tcp_keepintvl));
    case TCPCTL_SENDSPACE:
	return (sysctl_int(oldp, oldlenp, newp, newlen,
			   (int *)&tcp_sendspace)); /* XXX */
    case TCPCTL_RECVSPACE:
	return (sysctl_int(oldp, oldlenp, newp, newlen,
			   (int *)&tcp_recvspace)); /* XXX */
    case TCPCTL_KEEPINIT:
	return (sysctl_int(oldp, oldlenp, newp, newlen,
			   &tcp_keepinit));
    default:
	return (ENOPROTOOPT);
    }
	/* NOTREACHED */
}
