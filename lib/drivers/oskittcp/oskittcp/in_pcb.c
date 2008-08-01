/*
 * Copyright (c) 1982, 1986, 1991, 1993, 1995
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
 *	@(#)in_pcb.c	8.2 (Berkeley) 1/4/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#include <oskittcp.h>

struct	in_addr zeroin_addr;

int
in_pcballoc(so, pcbinfo)
	struct socket *so;
	struct inpcbinfo *pcbinfo;
{
	register struct inpcb *inp;
	int s;

	MALLOC(inp, struct inpcb *, sizeof(*inp), M_PCB, M_NOWAIT);
	if (inp == NULL)
		return (ENOBUFS);
	bzero((caddr_t)inp, sizeof(*inp));
	inp->inp_pcbinfo = pcbinfo;
	inp->inp_socket = so;
	s = splnet();
	LIST_INSERT_HEAD(pcbinfo->listhead, inp, inp_list);
	in_pcbinshash(inp);
	splx(s);
	so->so_pcb = (caddr_t)inp;
	return (0);
}

int
in_pcbbind(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct socket *so = inp->inp_socket;
	struct inpcbhead *head = inp->inp_pcbinfo->listhead;
	unsigned short *lastport = &inp->inp_pcbinfo->lastport;
	struct sockaddr_in *sin;
#ifndef __REACTOS__
	struct proc *p = curproc;		/* XXX */
#endif
	u_short lport = 0;
	int wild = 0, reuseport = (so->so_options & SO_REUSEPORT);
#ifndef __REACTOS__
	int error;
#endif

	OS_DbgPrint(OSK_MID_TRACE,("Called\n"));

	if( nam ) OskitDumpBuffer( nam->m_data, nam->m_len );

#ifndef __REACTOS__
	if (in_ifaddr == 0) {
	    OS_DbgPrint(OSK_MID_TRACE,("Leaving EADDRNOTAVAIL\n"));
	    return (EADDRNOTAVAIL);
	}
#endif
	if (inp->inp_lport || inp->inp_laddr.s_addr != INADDR_ANY)
		return (EINVAL);
	if ((so->so_options & (SO_REUSEADDR|SO_REUSEPORT)) == 0 &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
	     (so->so_options & SO_ACCEPTCONN) == 0))
		wild = INPLOOKUP_WILDCARD;
	if (nam) {
		sin = mtod(nam, struct sockaddr_in *);
		if (nam->m_len != sizeof (*sin)) {
		    OS_DbgPrint(OSK_MID_TRACE,("Leaving EINVAL\n"));
		    return (EINVAL);
		}
#ifdef notdef
		/*
		 * We should check the family, but old programs
		 * incorrectly fail to initialize it.
		 */
		if (sin->sin_family != AF_INET) {
		    OS_DbgPrint(OSK_MID_TRACE,("Leaving EAFNOSUPPORT\n"));
		    return (EAFNOSUPPORT);
		}
#endif
		lport = sin->sin_port;
		if (IN_MULTICAST(ntohl(sin->sin_addr.s_addr))) {
			/*
			 * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
			 * allow complete duplication of binding if
			 * SO_REUSEPORT is set, or if SO_REUSEADDR is set
			 * and a multicast address is bound on both
			 * new and duplicated sockets.
			 */
			if (so->so_options & SO_REUSEADDR)
				reuseport = SO_REUSEADDR|SO_REUSEPORT;
		} else if (sin->sin_addr.s_addr != INADDR_ANY) {
			sin->sin_port = 0;		/* yech... */
			OS_DbgPrint(OSK_MID_TRACE,("Calling ifwithaddr\n"));
			if (ifa_ifwithaddr((struct sockaddr *)sin) == 0) {
			    OS_DbgPrint(OSK_MID_TRACE,
					("Leaving EADDRNOTAVAIL\n"));
			    return (EADDRNOTAVAIL);
			}
			OS_DbgPrint(OSK_MID_TRACE,("Yep, we have that addr\n"));
		}
		if (lport) {
			struct inpcb *t;

#ifndef __REACTOS__
			/* GROSS */
			if (ntohs(lport) < IPPORT_RESERVED &&
			    (error = suser(p->p_ucred, &p->p_acflag))) {
			    OS_DbgPrint(OSK_MID_TRACE,
					("Leaving EACCESS\n"));
			    return (EACCES);
			}
#endif
			t = in_pcblookup(head, zeroin_addr, 0,
			    sin->sin_addr, lport, wild);
			if (t && (reuseport & t->inp_socket->so_options) == 0)
			{
			    OS_DbgPrint(OSK_MID_TRACE,
					("Leaving EADDRINUSE\n"));
			    return (EADDRINUSE);
			}
		}
		inp->inp_laddr = sin->sin_addr;
	}
	if (lport == 0)
		do {
		    ++*lastport;
		    OS_DbgPrint(OSK_MID_TRACE,("Finding port %d\n",
					       *lastport));
		    if (*lastport < IPPORT_RESERVED ||
			*lastport > IPPORT_USERRESERVED)
			*lastport = IPPORT_RESERVED;
		    lport = htons(*lastport);
		} while (in_pcblookup(head,
				      zeroin_addr, 0, inp->inp_laddr,
				      lport, wild));
	inp->inp_lport = lport;
	in_pcbrehash(inp);

	OS_DbgPrint(OSK_MID_TRACE,("Returning success\n"));
	return (0);
}

/*
 *   Transform old in_pcbconnect() into an inner subroutine for new
 *   in_pcbconnect(): Do some validity-checking on the remote
 *   address (in mbuf 'nam') and then determine local host address
 *   (i.e., which interface) to use to access that remote host.
 *
 *   This preserves definition of in_pcbconnect(), while supporting a
 *   slightly different version for T/TCP.  (This is more than
 *   a bit of a kludge, but cleaning up the internal interfaces would
 *   have forced minor changes in every protocol).
 */

int
in_pcbladdr(inp, nam, plocal_sin)
	register struct inpcb *inp;
	struct mbuf *nam;
	struct sockaddr_in **plocal_sin;
{
	struct in_ifaddr *ia;
#ifndef __REACTOS__
	struct sockaddr_in *ifaddr = 0;
#endif
	register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

	OS_DbgPrint(OSK_MID_TRACE,("Called\n"));

	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	if (sin->sin_port == 0)
		return (EADDRNOTAVAIL);
	if (in_ifaddr) {
		/*
		 * If the destination address is INADDR_ANY,
		 * use the primary local address.
		 * If the supplied address is INADDR_BROADCAST,
		 * and the primary interface supports broadcast,
		 * choose the broadcast address for that interface.
		 */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
#define sintosa(sin)	((struct sockaddr *)(sin))
#define ifatoia(ifa)	((struct in_ifaddr *)(ifa))
		if (sin->sin_addr.s_addr == INADDR_ANY)
		    sin->sin_addr = IA_SIN(in_ifaddr)->sin_addr;
#ifndef __REACTOS__
		else if (sin->sin_addr.s_addr == (u_long)INADDR_BROADCAST &&
			 (in_ifaddr->ia_ifp->if_flags & IFF_BROADCAST))
		    sin->sin_addr = satosin(&in_ifaddr->ia_broadaddr)->sin_addr;
#endif
	}
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		register struct route *ro;

		ia = (struct in_ifaddr *)0;
		/*
		 * If route is known or can be allocated now,
		 * our src addr is taken from the i/f, else punt.
		 */
		ro = &inp->inp_route;
		if (ro->ro_rt &&
		    (satosin(&ro->ro_dst)->sin_addr.s_addr !=
			sin->sin_addr.s_addr ||
		    inp->inp_socket->so_options & SO_DONTROUTE)) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)0;
		}
		if ((inp->inp_socket->so_options & SO_DONTROUTE) == 0 && /*XXX*/
		    (ro->ro_rt == (struct rtentry *)0 ||
		    ro->ro_rt->rt_ifp == (struct ifnet *)0)) {
			/* No route yet, so try to acquire one */
			ro->ro_dst.sa_family = AF_INET;
			ro->ro_dst.sa_len = sizeof(struct sockaddr_in);
			((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
				sin->sin_addr;
			rtalloc(ro);
		}
		/*
		 * If we found a route, use the address
		 * corresponding to the outgoing interface
		 * unless it is the loopback (in case a route
		 * to our address on another net goes to loopback).
		 */
		if (ro->ro_rt && !(ro->ro_rt->rt_ifp->if_flags & IFF_LOOPBACK))
			ia = ifatoia(ro->ro_rt->rt_ifa);
		if (ia == 0) {
			u_short fport = sin->sin_port;

			sin->sin_port = 0;
			ia = ifatoia(ifa_ifwithdstaddr(sintosa(sin)));

			if (ia == 0)
				ia = ifatoia(ifa_ifwithnet(sintosa(sin)));
			sin->sin_port = fport;
			if (ia == 0)
				ia = in_ifaddr;
			if (ia == 0)
				return (EADDRNOTAVAIL);
		}
		/*
		 * If the destination address is multicast and an outgoing
		 * interface has been set as a multicast option, use the
		 * address of that interface as our source address.
		 */
#ifndef __REACTOS__
		if (IN_MULTICAST(ntohl(sin->sin_addr.s_addr)) &&
		    inp->inp_moptions != NULL) {
			struct ip_moptions *imo;
			struct ifnet *ifp;

			imo = inp->inp_moptions;
			if (imo->imo_multicast_ifp != NULL) {
			    ifp = imo->imo_multicast_ifp;
			    for (ia = in_ifaddr; ia; ia = ia->ia_next)
				if (ia->ia_ifp == ifp)
				    break;
			    if (ia == 0)
				return (EADDRNOTAVAIL);
			}
		}
#endif
		/*
	 * Don't do pcblookup call here; return interface in plocal_sin
	 * and exit to caller, that will do the lookup.
	 */
		*plocal_sin = (struct sockaddr_in *)ia->ia_ifa.ifa_addr;
		OS_DbgPrint(OSK_MID_TRACE,("plocal sin %x\n",
					   (*plocal_sin)->sin_addr.s_addr));

	}
	return(0);
}

/*
 * Outer subroutine:
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
int
in_pcbconnect(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in *ifaddr;
	register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
	int error;

	/*
	 *   Call inner routine, to assign local interface address.
	 */
	if ((error = in_pcbladdr(inp, nam, &ifaddr)))
		return(error);

	if (in_pcblookuphash(inp->inp_pcbinfo, sin->sin_addr, sin->sin_port,
	    inp->inp_laddr.s_addr ? inp->inp_laddr : ifaddr->sin_addr,
	    inp->inp_lport) != NULL)
		return (EADDRINUSE);
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		if (inp->inp_lport == 0)
			(void)in_pcbbind(inp, (struct mbuf *)0);
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	in_pcbrehash(inp);
	return (0);
}

void
in_pcbdisconnect(inp)
	struct inpcb *inp;
{

	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;
	in_pcbrehash(inp);
	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
}

void
in_pcbdetach(inp)
	struct inpcb *inp;
{
	struct socket *so = inp->inp_socket;
	int s;

	so->so_pcb = 0;
	sofree(so);
	if (inp->inp_options)
		(void)m_free(inp->inp_options);
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt);
	ip_freemoptions(inp->inp_moptions);
	s = splnet();
	LIST_REMOVE(inp, inp_hash);
	LIST_REMOVE(inp, inp_list);
	splx(s);
	FREE(inp, M_PCB);
}

void
in_setsockaddr(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;

	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

void
in_setpeeraddr(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;

	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  The local address and/or port numbers
 * may be specified to limit the search.  The "usual action" will be
 * taken, depending on the ctlinput cmd.  The caller must filter any
 * cmds that are uninteresting (e.g., no error in the map).
 * Call the protocol specific routine (if any) to report
 * any errors for each matching socket.
 *
 * Must be called at splnet.
 */
void
in_pcbnotify(head, dst, fport_arg, laddr, lport_arg, cmd, notify)
	struct inpcbhead *head;
	struct sockaddr *dst;
	u_int fport_arg, lport_arg;
	struct in_addr laddr;
	int cmd;
	void (*notify) __P((struct inpcb *, int));
{
	register struct inpcb *inp, *oinp;
	struct in_addr faddr;
	u_short fport = fport_arg, lport = lport_arg;
	int errno, s;

	if ((unsigned)cmd > PRC_NCMDS || dst->sa_family != AF_INET)
		return;
	faddr = ((struct sockaddr_in *)dst)->sin_addr;
	if (faddr.s_addr == INADDR_ANY)
		return;

	/*
	 * Redirects go to all references to the destination,
	 * and use in_rtchange to invalidate the route cache.
	 * Dead host indications: notify all references to the destination.
	 * Otherwise, if we have knowledge of the local port and address,
	 * deliver only to that socket.
	 */
	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
		fport = 0;
		lport = 0;
		laddr.s_addr = 0;
		if (cmd != PRC_HOSTDEAD)
			notify = in_rtchange;
	}
	errno = inetctlerrmap[cmd];
	s = splnet();
	for (inp = head->lh_first; inp != NULL;) {
		if (inp->inp_faddr.s_addr != faddr.s_addr ||
		    inp->inp_socket == 0 ||
		    (lport && inp->inp_lport != lport) ||
		    (laddr.s_addr && inp->inp_laddr.s_addr != laddr.s_addr) ||
		    (fport && inp->inp_fport != fport)) {
			inp = inp->inp_list.le_next;
			continue;
		}
		oinp = inp;
		inp = inp->inp_list.le_next;
		if (notify)
			(*notify)(oinp, errno);
	}
	splx(s);
}

/*
 * Check for alternatives when higher level complains
 * about service problems.  For now, invalidate cached
 * routing information.  If the route was created dynamically
 * (by a redirect), time to try a default gateway again.
 */
void
in_losing(inp)
	struct inpcb *inp;
{
	register struct rtentry *rt;
	struct rt_addrinfo info;

	if ((rt = inp->inp_route.ro_rt)) {
		inp->inp_route.ro_rt = 0;
		bzero((caddr_t)&info, sizeof(info));
		info.rti_info[RTAX_DST] =
			(struct sockaddr *)&inp->inp_route.ro_dst;
		info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
		info.rti_info[RTAX_NETMASK] = rt_mask(rt);
		rt_missmsg(RTM_LOSING, &info, rt->rt_flags, 0);
		if (rt->rt_flags & RTF_DYNAMIC)
			(void) rtrequest(RTM_DELETE, rt_key(rt),
				rt->rt_gateway, rt_mask(rt), rt->rt_flags,
				(struct rtentry **)0);
		else
		/*
		 * A new route can be allocated
		 * the next time output is attempted.
		 */
			rtfree(rt);
	}
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
void
in_rtchange(inp, errno)
	register struct inpcb *inp;
	int errno;
{
	if (inp->inp_route.ro_rt) {
		rtfree(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
}

struct inpcb *
in_pcblookup(head, faddr, fport_arg, laddr, lport_arg, flags)
	struct inpcbhead *head;
	struct in_addr faddr, laddr;
	u_int fport_arg, lport_arg;
	int flags;
{
	register struct inpcb *inp, *match = NULL;
	int matchwild = 3, wildcard;
	u_short fport = fport_arg, lport = lport_arg;
	int s;

	s = splnet();

	for (inp = head->lh_first; inp != NULL; inp = inp->inp_list.le_next) {
		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
			    inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0) {
				break;
			}
		}
	}
	splx(s);
	return (match);
}

/*
 * Lookup PCB in hash list.
 */
struct inpcb *
in_pcblookuphash(pcbinfo, faddr, fport_arg, laddr, lport_arg)
	struct inpcbinfo *pcbinfo;
	struct in_addr faddr, laddr;
	u_int fport_arg, lport_arg;
{
	struct inpcbhead *head;
	register struct inpcb *inp;
	u_short fport = fport_arg, lport = lport_arg;
	int s;

	s = splnet();
	/*
	 * First look for an exact match.
	 */
	head = &pcbinfo->hashbase[(faddr.s_addr + lport + fport) % pcbinfo->hashsize];

	for (inp = head->lh_first; inp != NULL; inp = inp->inp_hash.le_next) {
		if (inp->inp_faddr.s_addr != faddr.s_addr ||
		    inp->inp_fport != fport ||
		    inp->inp_lport != lport ||
		    inp->inp_laddr.s_addr != laddr.s_addr)
			continue;
		/*
		 * Move PCB to head of this hash chain so that it can be
		 * found more quickly in the future.
		 */
		if (inp != head->lh_first) {
			LIST_REMOVE(inp, inp_hash);
			LIST_INSERT_HEAD(head, inp, inp_hash);
		}
		break;
	}
	splx(s);
	return (inp);
}

/*
 * Insert PCB into hash chain. Must be called at splnet.
 */
void
in_pcbinshash(inp)
	struct inpcb *inp;
{
	struct inpcbhead *head;

	head = &inp->inp_pcbinfo->hashbase[(inp->inp_faddr.s_addr +
		inp->inp_lport + inp->inp_fport) % inp->inp_pcbinfo->hashsize];

	LIST_INSERT_HEAD(head, inp, inp_hash);
}

void
in_pcbrehash(inp)
	struct inpcb *inp;
{
	struct inpcbhead *head;
	int s;

	s = splnet();
	LIST_REMOVE(inp, inp_hash);

	head = &inp->inp_pcbinfo->hashbase[(inp->inp_faddr.s_addr +
		inp->inp_lport + inp->inp_fport) % inp->inp_pcbinfo->hashsize];

	LIST_INSERT_HEAD(head, inp, inp_hash);
	splx(s);
}
