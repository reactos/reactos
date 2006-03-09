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
 *	@(#)raw_ip.c	8.7 (Berkeley) 5/15/95
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#ifndef __REACTOS__
#include <netinet/ip_mroute.h>
#endif

#include <netinet/ip_fw.h>
#include <oskittcp.h>

static struct inpcbhead ripcb;
static struct inpcbinfo ripcbinfo;

/*
 * Nominal space allocated to a raw ip socket.
 */
#define	RIPSNDQ		8192
#define	RIPRCVQ		8192

/*
 * Raw interface to IP protocol.
 */

/*
 * Initialize raw connection block q.
 */
void
rip_init()
{
	LIST_INIT(&ripcb);
	ripcbinfo.listhead = &ripcb;
	/*
	 * XXX We don't use the hash list for raw IP, but it's easier
	 * to allocate a one entry hash list than it is to check all
	 * over the place for hashbase == NULL.
	 */
	ripcbinfo.hashbase = phashinit(1, M_PCB, &ripcbinfo.hashsize);
}

static struct	sockaddr_in ripsrc = { sizeof(ripsrc), AF_INET };
/*
 * Setup generic address and protocol structures
 * for raw_input routine, then pass them along with
 * mbuf chain.
 */
void
rip_input(m)
	struct mbuf *m;
{
	register struct ip *ip = mtod(m, struct ip *);
	register struct inpcb *inp;
	struct socket *last = 0;

	ripsrc.sin_addr = ip->ip_src;
	OS_DbgPrint(OSK_MID_TRACE,("ripsrc: %x\n", ip->ip_src));
	for (inp = ripcb.lh_first; inp != NULL; inp = inp->inp_list.le_next) {
	    OS_DbgPrint(OSK_MID_TRACE,("Looking at %d %x -> %x\n",
				       inp->inp_ip.ip_p,
				       inp->inp_laddr.s_addr,
				       inp->inp_faddr.s_addr));
		if (inp->inp_ip.ip_p && inp->inp_ip.ip_p != ip->ip_p)
			continue;
		if (inp->inp_laddr.s_addr &&
                  inp->inp_laddr.s_addr != ip->ip_dst.s_addr)
			continue;
		if (inp->inp_faddr.s_addr &&
                  inp->inp_faddr.s_addr != ip->ip_src.s_addr)
			continue;
		if (last) {
			struct mbuf *n = m_copy(m, 0, (int)M_COPYALL);
			if (n) {
				if (sbappendaddr(&last->so_rcv,
				    (struct sockaddr *)&ripsrc, n,
				    (struct mbuf *)0) == 0)
					/* should notify about lost packet */
					m_freem(n);
				else
					sorwakeup(last);
			}
		}
		last = inp->inp_socket;
	}
	if (last) {
		if (sbappendaddr(&last->so_rcv, (struct sockaddr *)&ripsrc,
		    m, (struct mbuf *)0) == 0)
			m_freem(m);
		else
			sorwakeup(last);
	} else {
		m_freem(m);
              ipstat.ips_noproto++;
              ipstat.ips_delivered--;
      }
}

/*
 * Generate IP header and pass packet to ip_output.
 * Tack on options user may have setup with control call.
 */
int
rip_output(m, so, dst)
	register struct mbuf *m;
	struct socket *so;
	u_long dst;
{
	register struct ip *ip;
	register struct inpcb *inp = sotoinpcb(so);
#ifndef __REACTOS__
	struct mbuf *opts;
#endif
	int flags = (so->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST;

	/*
	 * If the user handed us a complete IP packet, use it.
	 * Otherwise, allocate an mbuf for a header and fill it in.
	 */
	if ((inp->inp_flags & INP_HDRINCL) == 0) {
		M_PREPEND(m, sizeof(struct ip), M_WAIT);
		ip = mtod(m, struct ip *);
		ip->ip_tos = 0;
		ip->ip_off = 0;
		ip->ip_p = inp->inp_ip.ip_p;
		ip->ip_len = m->m_pkthdr.len;
		ip->ip_src = inp->inp_laddr;
		ip->ip_dst.s_addr = dst;
		ip->ip_ttl = MAXTTL;
	} else {
		ip = mtod(m, struct ip *);
		/* don't allow both user specified and setsockopt options,
		   and don't allow packet length sizes that will crash */
		if (((ip->ip_hl != (sizeof (*ip) >> 2)) && inp->inp_options) ||
		     (ip->ip_len > m->m_pkthdr.len)) {
			m_freem(m);
			return EINVAL;
		}
		if (ip->ip_id == 0)
			ip->ip_id = htons(ip_id++);
		/* XXX prevent ip_output from overwriting header fields */
		flags |= IP_RAWOUTPUT;
		ipstat.ips_rawout++;
	}
	return (ip_output(m, inp->inp_options, &inp->inp_route, flags,
			  inp->inp_moptions));
}

/*
 * Raw IP socket option processing.
 */
int
rip_ctloutput(op, so, level, optname, m)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **m;
{
	register int error = EINVAL;
#ifndef __REACTOS__
	register struct inpcb *inp = sotoinpcb(so);

	if (level != IPPROTO_IP) {
		if (op == PRCO_SETOPT && *m)
			(void)m_free(*m);
		return (EINVAL);
	}

	switch (optname) {

	case IP_HDRINCL:
		error = 0;
		if (op == PRCO_SETOPT) {
			if (m == 0 || *m == 0 || (*m)->m_len < sizeof (int))
				error = EINVAL;
			else if (*mtod(*m, int *))
				inp->inp_flags |= INP_HDRINCL;
			else
				inp->inp_flags &= ~INP_HDRINCL;
			if (*m)
				(void)m_free(*m);
		} else {
			*m = m_get(M_WAIT, MT_SOOPTS);
			(*m)->m_len = sizeof (int);
			*mtod(*m, int *) = inp->inp_flags & INP_HDRINCL;
		}
		return (error);

	case IP_FW_GET:
		if (ip_fw_ctl_ptr==NULL || op == PRCO_SETOPT) {
			if (*m) (void)m_free(*m);
			return(EINVAL);
		}
		return (*ip_fw_ctl_ptr)(optname, m);
	case IP_FW_ADD:
	case IP_FW_DEL:
	case IP_FW_FLUSH:
	case IP_FW_ZERO:
		if (ip_fw_ctl_ptr==NULL || op != PRCO_SETOPT) {
			if (*m) (void)m_free(*m);
			return(EINVAL);
		}

		return (*ip_fw_ctl_ptr)(optname, m);
		return(error);

	case IP_RSVP_ON:
		return ip_rsvp_init(so);
		break;

	case IP_RSVP_OFF:
		return ip_rsvp_done();
		break;

	case IP_RSVP_VIF_ON:
		return ip_rsvp_vif_init(so, *m);

	case IP_RSVP_VIF_OFF:
		return ip_rsvp_vif_done(so, *m);

	case MRT_INIT:
	case MRT_DONE:
	case MRT_ADD_VIF:
	case MRT_DEL_VIF:
	case MRT_ADD_MFC:
	case MRT_DEL_MFC:
	case MRT_VERSION:
	case MRT_ASSERT:
		if (op == PRCO_SETOPT) {
			error = ip_mrouter_set(optname, so, *m);
			if (*m)
				(void)m_free(*m);
		} else if (op == PRCO_GETOPT) {
			error = ip_mrouter_get(optname, so, m);
		} else
			error = EINVAL;
		return (error);
	}
	return (ip_ctloutput(op, so, level, optname, m));
#else
	return error;
#endif
}

static u_long	rip_sendspace = RIPSNDQ; /* XXX sysctl ? */
static u_long	rip_recvspace = RIPRCVQ; /* XXX sysctl ? */

/*ARGSUSED*/
int
rip_usrreq(so, req, m, nam, control)
	register struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
{
	register int error = 0;
	register struct inpcb *inp = sotoinpcb(so);

	if (req == PRU_CONTROL)
		return (in_control(so, (u_long)m, (caddr_t)nam,
			(struct ifnet *)control));

	switch (req) {

	case PRU_ATTACH:
		if (inp)
			panic("rip_attach");
		if ((so->so_state & SS_PRIV) == 0) {
			error = EACCES;
			break;
		}
		if ((error = soreserve(so, rip_sendspace, rip_recvspace)) ||
		    (error = in_pcballoc(so, &ripcbinfo)))
			break;
		inp = (struct inpcb *)so->so_pcb;
		inp->inp_ip.ip_p = (int)nam;
		break;

	case PRU_DISCONNECT:
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			error = ENOTCONN;
			break;
		}
		/* FALLTHROUGH */
	case PRU_ABORT:
		soisdisconnected(so);
		/* FALLTHROUGH */
	case PRU_DETACH:
		if (inp == 0)
			panic("rip_detach");
#ifndef __REACTOS__
		if (so == ip_mrouter)
			ip_mrouter_done();
		ip_rsvp_force_done(so);
		if (so == ip_rsvpd)
			ip_rsvp_done();
#endif
		in_pcbdetach(inp);
		break;

	case PRU_BIND:
	    {
		struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);

		if (nam->m_len != sizeof(*addr)) {
			error = EINVAL;
			break;
		}
		if ((ifnet == 0) ||
		    ((addr->sin_family != AF_INET) &&
		     (addr->sin_family != AF_IMPLINK)) ||
		    (addr->sin_addr.s_addr &&
		     ifa_ifwithaddr((struct sockaddr *)addr) == 0)) {
			error = EADDRNOTAVAIL;
			break;
		}
		inp->inp_laddr = addr->sin_addr;
		break;
	    }
	case PRU_CONNECT:
	    {
		struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);

		if (nam->m_len != sizeof(*addr)) {
			error = EINVAL;
			break;
		}
		if (ifnet == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		if ((addr->sin_family != AF_INET) &&
		     (addr->sin_family != AF_IMPLINK)) {
			error = EAFNOSUPPORT;
			break;
		}
		inp->inp_faddr = addr->sin_addr;
		soisconnected(so);
		break;
	    }

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	/*
	 * Mark the connection as being incapable of further input.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	/*
	 * Ship a packet out.  The appropriate raw output
	 * routine handles any massaging necessary.
	 */
	case PRU_SEND:
	    {
		register u_long dst;

		if (so->so_state & SS_ISCONNECTED) {
			if (nam) {
				error = EISCONN;
				break;
			}
			dst = inp->inp_faddr.s_addr;
		} else {
			if (nam == NULL) {
				error = ENOTCONN;
				break;
			}
			dst = mtod(nam, struct sockaddr_in *)->sin_addr.s_addr;
		}
		error = rip_output(m, so, dst);
		m = NULL;
		break;
	    }

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	/*
	 * Not supported.
	 */
	case PRU_RCVOOB:
	case PRU_RCVD:
	case PRU_LISTEN:
	case PRU_ACCEPT:
	case PRU_SENDOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	default:
		panic("rip_usrreq");
	}
	if (m != NULL)
		m_freem(m);
	return (error);
}
