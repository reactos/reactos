/*
 * Copyright (c) 1988, 1991, 1993
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
 *	@(#)rtsock.c	8.5 (Berkeley) 11/2/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>

#include <net/if.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <oskittcp.h>

struct	sockaddr route_dst = { 2, PF_ROUTE, };
struct	sockaddr route_src = { 2, PF_ROUTE, };
struct	sockproto route_proto = { PF_ROUTE, };

struct walkarg {
	int	w_op, w_arg, w_given, w_needed, w_tmemsize;
	caddr_t	w_where, w_tmem;
};

static struct mbuf *
		rt_msg1 __P((int, struct rt_addrinfo *));
static int	rt_msg2 __P((int,
		    struct rt_addrinfo *, caddr_t, struct walkarg *));
static void	rt_xaddrs __P((caddr_t, caddr_t, struct rt_addrinfo *));

/* Sleazy use of local variables throughout file, warning!!!! */
#define dst	info.rti_info[RTAX_DST]
#define gate	info.rti_info[RTAX_GATEWAY]
#define netmask	info.rti_info[RTAX_NETMASK]
#define genmask	info.rti_info[RTAX_GENMASK]
#define ifpaddr	info.rti_info[RTAX_IFP]
#define ifaaddr	info.rti_info[RTAX_IFA]
#define brdaddr	info.rti_info[RTAX_BRD]

/*ARGSUSED*/
int
route_usrreq(so, req, m, nam, control)
	register struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
{
	register int error = 0;
	register struct rawcb *rp = sotorawcb(so);
	int s;

	if (req == PRU_ATTACH) {
		MALLOC(rp, struct rawcb *, sizeof(*rp), M_PCB, M_WAITOK);
		so->so_pcb = (caddr_t)rp;
		if (so->so_pcb)
			bzero(so->so_pcb, sizeof(*rp));

	}
	if (req == PRU_DETACH && rp) {
		int af = rp->rcb_proto.sp_protocol;
		if (af == AF_INET)
			route_cb.ip_count--;
		else if (af == AF_NS)
			route_cb.ns_count--;
		else if (af == AF_ISO)
			route_cb.iso_count--;
		route_cb.any_count--;
	}
	s = splnet();
	error = raw_usrreq(so, req, m, nam, control);
	rp = sotorawcb(so);
	if (req == PRU_ATTACH && rp) {
		int af = rp->rcb_proto.sp_protocol;
		if (error) {
			free((caddr_t)rp, M_PCB);
			splx(s);
			return (error);
		}
		if (af == AF_INET)
			route_cb.ip_count++;
		else if (af == AF_NS)
			route_cb.ns_count++;
		else if (af == AF_ISO)
			route_cb.iso_count++;
		rp->rcb_faddr = &route_src;
		route_cb.any_count++;
		soisconnected(so);
		so->so_options |= SO_USELOOPBACK;
	}
	splx(s);
	return (error);
}

/*ARGSUSED*/
int
route_output(m, so)
	register struct mbuf *m;
	struct socket *so;
{
	register struct rt_msghdr *rtm = 0;
	register struct rtentry *rt = 0;
	struct rtentry *saved_nrt = 0;
	struct radix_node_head *rnh;
	struct rt_addrinfo info;
	int len, error = 0;
	struct ifnet *ifp = 0;
	struct ifaddr *ifa = 0;

#define senderr(e) { error = e; goto flush;}
	if (m == 0 || ((m->m_len < sizeof(long)) &&
		       (m = m_pullup(m, sizeof(long))) == 0))
		return (ENOBUFS);
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("route_output");
	len = m->m_pkthdr.len;
	if (len < sizeof(*rtm) ||
	    len != mtod(m, struct rt_msghdr *)->rtm_msglen) {
		dst = 0;
		senderr(EINVAL);
	}
	R_Malloc(rtm, struct rt_msghdr *, len);
	if (rtm == 0) {
		dst = 0;
		senderr(ENOBUFS);
	}
	m_copydata(m, 0, len, (caddr_t)rtm);
	if (rtm->rtm_version != RTM_VERSION) {
		dst = 0;
		senderr(EPROTONOSUPPORT);
	}
	rtm->rtm_pid = curproc->p_pid;
	info.rti_addrs = rtm->rtm_addrs;
	rt_xaddrs((caddr_t)(rtm + 1), len + (caddr_t)rtm, &info);
	if (dst == 0)
		senderr(EINVAL);
	if (genmask) {
		struct radix_node *t;
		t = rn_addmask((caddr_t)genmask, 0, 1);
		if (t && Bcmp(genmask, t->rn_key, *(u_char *)genmask) == 0)
			genmask = (struct sockaddr *)(t->rn_key);
		else
			senderr(ENOBUFS);
	}
	switch (rtm->rtm_type) {

	case RTM_ADD:
		if (gate == 0)
			senderr(EINVAL);
		error = rtrequest(RTM_ADD, dst, gate, netmask,
					rtm->rtm_flags, &saved_nrt);
		if (error == 0 && saved_nrt) {
			rt_setmetrics(rtm->rtm_inits,
				&rtm->rtm_rmx, &saved_nrt->rt_rmx);
			saved_nrt->rt_refcnt--;
			saved_nrt->rt_genmask = genmask;
		}
		break;

	case RTM_DELETE:
		error = rtrequest(RTM_DELETE, dst, gate, netmask,
				rtm->rtm_flags, &saved_nrt);
		if (error == 0) {
			if ((rt = saved_nrt))
				rt->rt_refcnt++;
			goto report;
		}
		break;

	case RTM_GET:
	case RTM_CHANGE:
	case RTM_LOCK:
		if ((rnh = rt_tables[dst->sa_family]) == 0) {
			senderr(EAFNOSUPPORT);
		} else if ((rt = (struct rtentry *)
				rnh->rnh_lookup(dst, netmask, rnh)))
			rt->rt_refcnt++;
		else
			senderr(ESRCH);
		switch(rtm->rtm_type) {

		case RTM_GET:
		report:
			dst = rt_key(rt);
			gate = rt->rt_gateway;
			netmask = rt_mask(rt);
			genmask = rt->rt_genmask;
			if (rtm->rtm_addrs & (RTA_IFP | RTA_IFA)) {
				ifp = rt->rt_ifp;
				if (ifp) {
					ifpaddr = ifp->if_addrlist->ifa_addr;
					ifaaddr = rt->rt_ifa->ifa_addr;
					rtm->rtm_index = ifp->if_index;
				} else {
					ifpaddr = 0;
					ifaaddr = 0;
			    }
			}
			len = rt_msg2(rtm->rtm_type, &info, (caddr_t)0,
				(struct walkarg *)0);
			if (len > rtm->rtm_msglen) {
				struct rt_msghdr *new_rtm;
				R_Malloc(new_rtm, struct rt_msghdr *, len);
				if (new_rtm == 0)
					senderr(ENOBUFS);
				Bcopy(rtm, new_rtm, rtm->rtm_msglen);
				Free(rtm); rtm = new_rtm;
			}
			(void)rt_msg2(rtm->rtm_type, &info, (caddr_t)rtm,
				(struct walkarg *)0);
			rtm->rtm_flags = rt->rt_flags;
			rtm->rtm_rmx = rt->rt_rmx;
			rtm->rtm_addrs = info.rti_addrs;
			break;

		case RTM_CHANGE:
			if (gate && rt_setgate(rt, rt_key(rt), gate))
				senderr(EDQUOT);

			/*
			 * If they tried to change things but didn't specify
			 * the required gateway, then just use the old one.
			 * This can happen if the user tries to change the
			 * flags on the default route without changing the
			 * default gateway.  Changing flags still doesn't work.
			 */
			if ((rt->rt_flags & RTF_GATEWAY) && !gate)
				gate = rt->rt_gateway;

#ifndef __REACTOS__
			/* new gateway could require new ifaddr, ifp;
			   flags may also be different; ifp may be specified
			   by ll sockaddr when protocol address is ambiguous */
			if (ifpaddr && (ifa = ifa_ifwithnet(ifpaddr)) &&
			    (ifp = ifa->ifa_ifp))
				ifa = ifaof_ifpforaddr(ifaaddr ? ifaaddr : gate,
						       ifp);
#endif

			else if ((ifaaddr && (ifa = ifa_ifwithaddr(ifaaddr))) ||
				 (ifa = ifa_ifwithroute(rt->rt_flags,
							rt_key(rt), gate)))
#ifndef __REACTOS__
			    ifp = ifa->ifa_ifp;
#else
			;
#endif
			if (ifa) {
				register struct ifaddr *oifa = rt->rt_ifa;
				if (oifa != ifa) {
#ifndef __REACTOS__
				    if (oifa && oifa->ifa_rtrequest)
					oifa->ifa_rtrequest(RTM_DELETE,
								rt, gate);
#endif
				    IFAFREE(rt->rt_ifa);
				    rt->rt_ifa = ifa;
				    ifa->ifa_refcnt++;
				    rt->rt_ifp = ifp;
				}
			}
			rt_setmetrics(rtm->rtm_inits, &rtm->rtm_rmx,
					&rt->rt_rmx);
#ifndef __REACTOS__
			if (rt->rt_ifa && rt->rt_ifa->ifa_rtrequest)
			       rt->rt_ifa->ifa_rtrequest(RTM_ADD, rt, gate);
#endif
			if (genmask)
				rt->rt_genmask = genmask;
			/*
			 * Fall into
			 */
		case RTM_LOCK:
			rt->rt_rmx.rmx_locks &= ~(rtm->rtm_inits);
			rt->rt_rmx.rmx_locks |=
				(rtm->rtm_inits & rtm->rtm_rmx.rmx_locks);
			break;
		}
		break;

	default:
		senderr(EOPNOTSUPP);
	}

flush:
	if (rtm) {
		if (error)
			rtm->rtm_errno = error;
		else
			rtm->rtm_flags |= RTF_DONE;
	}
	if (rt)
		rtfree(rt);
    {
	register struct rawcb *rp = 0;
	/*
	 * Check to see if we don't want our own messages.
	 */
	if ((so->so_options & SO_USELOOPBACK) == 0) {
		if (route_cb.any_count <= 1) {
			if (rtm)
				Free(rtm);
			m_freem(m);
			return (error);
		}
		/* There is another listener, so construct message */
		rp = sotorawcb(so);
	}
	if (rtm) {
		m_copyback(m, 0, rtm->rtm_msglen, (caddr_t)rtm);
		Free(rtm);
	}
	if (rp)
		rp->rcb_proto.sp_family = 0; /* Avoid us */
	if (dst)
		route_proto.sp_protocol = dst->sa_family;
	raw_input(m, &route_proto, &route_src, &route_dst);
	if (rp)
		rp->rcb_proto.sp_family = PF_ROUTE;
    }
	return (error);
}

void
rt_setmetrics(which, in, out)
	u_long which;
	register struct rt_metrics *in, *out;
{
#define metric(f, e) if (which & (f)) out->e = in->e;
	metric(RTV_RPIPE, rmx_recvpipe);
	metric(RTV_SPIPE, rmx_sendpipe);
	metric(RTV_SSTHRESH, rmx_ssthresh);
	metric(RTV_RTT, rmx_rtt);
	metric(RTV_RTTVAR, rmx_rttvar);
	metric(RTV_HOPCOUNT, rmx_hopcount);
	metric(RTV_MTU, rmx_mtu);
	metric(RTV_EXPIRE, rmx_expire);
#undef metric
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

static void
rt_xaddrs(cp, cplim, rtinfo)
	register caddr_t cp, cplim;
	register struct rt_addrinfo *rtinfo;
{
	register struct sockaddr *sa;
	register int i;

	bzero(rtinfo->rti_info, sizeof(rtinfo->rti_info));
	for (i = 0; (i < RTAX_MAX) && (cp < cplim); i++) {
		if ((rtinfo->rti_addrs & (1 << i)) == 0)
			continue;
		rtinfo->rti_info[i] = sa = (struct sockaddr *)cp;
		ADVANCE(cp, sa);
	}
}

static struct mbuf *
rt_msg1(type, rtinfo)
	int type;
	register struct rt_addrinfo *rtinfo;
{
	register struct rt_msghdr *rtm;
	register struct mbuf *m;
	register int i;
	register struct sockaddr *sa;
	int len, dlen;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (m);
	switch (type) {

	case RTM_DELADDR:
	case RTM_NEWADDR:
		len = sizeof(struct ifa_msghdr);
		break;

	case RTM_IFINFO:
		len = sizeof(struct if_msghdr);
		break;

	default:
		len = sizeof(struct rt_msghdr);
	}
	if (len > MHLEN)
		panic("rt_msg1");
	m->m_pkthdr.len = m->m_len = len;
	m->m_pkthdr.rcvif = 0;
	rtm = mtod(m, struct rt_msghdr *);
	bzero((caddr_t)rtm, len);
	for (i = 0; i < RTAX_MAX; i++) {
		if ((sa = rtinfo->rti_info[i]) == NULL)
			continue;
		rtinfo->rti_addrs |= (1 << i);
		dlen = ROUNDUP(sa->sa_len);
		m_copyback(m, len, dlen, (caddr_t)sa);
		len += dlen;
	}
	if (m->m_pkthdr.len != len) {
		m_freem(m);
		return (NULL);
	}
	rtm->rtm_msglen = len;
	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_type = type;
	return (m);
}

static int
rt_msg2(type, rtinfo, cp, w)
	int type;
	register struct rt_addrinfo *rtinfo;
	caddr_t cp;
	struct walkarg *w;
{
	register int i;
	int len, dlen, second_time = 0;
	caddr_t cp0;

	rtinfo->rti_addrs = 0;
again:
	switch (type) {

	case RTM_DELADDR:
	case RTM_NEWADDR:
		len = sizeof(struct ifa_msghdr);
		break;

	case RTM_IFINFO:
		len = sizeof(struct if_msghdr);
		break;

	default:
		len = sizeof(struct rt_msghdr);
	}
	cp0 = cp;
	if (cp0)
		cp += len;
	for (i = 0; i < RTAX_MAX; i++) {
		register struct sockaddr *sa;

		if ((sa = rtinfo->rti_info[i]) == 0)
			continue;
		rtinfo->rti_addrs |= (1 << i);
		dlen = ROUNDUP(sa->sa_len);
		if (cp) {
			bcopy((caddr_t)sa, cp, (unsigned)dlen);
			cp += dlen;
		}
		len += dlen;
	}
	if (cp == 0 && w != NULL && !second_time) {
		register struct walkarg *rw = w;

		rw->w_needed += len;
		if (rw->w_needed <= 0 && rw->w_where) {
			if (rw->w_tmemsize < len) {
				if (rw->w_tmem)
					free(rw->w_tmem, M_RTABLE);
				rw->w_tmem = (caddr_t)
					malloc(len, M_RTABLE, M_NOWAIT);
				if (rw->w_tmem)
					rw->w_tmemsize = len;
			}
			if (rw->w_tmem) {
				cp = rw->w_tmem;
				second_time = 1;
				goto again;
			} else
				rw->w_where = 0;
		}
	}
	if (cp) {
		register struct rt_msghdr *rtm = (struct rt_msghdr *)cp0;

		rtm->rtm_version = RTM_VERSION;
		rtm->rtm_type = type;
		rtm->rtm_msglen = len;
	}
	return (len);
}

/*
 * This routine is called to generate a message from the routing
 * socket indicating that a redirect has occured, a routing lookup
 * has failed, or that a protocol has detected timeouts to a particular
 * destination.
 */
void
rt_missmsg(type, rtinfo, flags, error)
	int type, flags, error;
	register struct rt_addrinfo *rtinfo;
{
	register struct rt_msghdr *rtm;
	register struct mbuf *m;
	struct sockaddr *sa = rtinfo->rti_info[RTAX_DST];

	if (route_cb.any_count == 0)
		return;
	m = rt_msg1(type, rtinfo);
	if (m == 0)
		return;
	rtm = mtod(m, struct rt_msghdr *);
	rtm->rtm_flags = RTF_DONE | flags;
	rtm->rtm_errno = error;
	rtm->rtm_addrs = rtinfo->rti_addrs;
	route_proto.sp_protocol = sa ? sa->sa_family : 0;
	raw_input(m, &route_proto, &route_src, &route_dst);
}

/*
 * This routine is called to generate a message from the routing
 * socket indicating that the status of a network interface has changed.
 */
void
rt_ifmsg(ifp)
	register struct ifnet *ifp;
{
	register struct if_msghdr *ifm;
	struct mbuf *m;
	struct rt_addrinfo info;

	if (route_cb.any_count == 0)
		return;
	bzero((caddr_t)&info, sizeof(info));
	m = rt_msg1(RTM_IFINFO, &info);
	if (m == 0)
		return;
	ifm = mtod(m, struct if_msghdr *);
	ifm->ifm_index = ifp->if_index;
	ifm->ifm_flags = (u_short)ifp->if_flags;
	ifm->ifm_data = ifp->if_data;
	ifm->ifm_addrs = 0;
	route_proto.sp_protocol = 0;
	raw_input(m, &route_proto, &route_src, &route_dst);
}

/*
 * This is called to generate messages from the routing socket
 * indicating a network interface has had addresses associated with it.
 * if we ever reverse the logic and replace messages TO the routing
 * socket indicate a request to configure interfaces, then it will
 * be unnecessary as the routing socket will automatically generate
 * copies of it.
 */
void
rt_newaddrmsg(cmd, ifa, error, rt)
	int cmd, error;
	register struct ifaddr *ifa;
	register struct rtentry *rt;
{
	struct rt_addrinfo info;
	struct sockaddr *sa = 0;
	int pass;
	struct mbuf *m = 0;
#ifndef __REACTOS__
	struct ifnet *ifp = ifa->ifa_ifp;
#endif

	if (route_cb.any_count == 0)
		return;
	for (pass = 1; pass < 3; pass++) {
		bzero((caddr_t)&info, sizeof(info));
		if ((cmd == RTM_ADD && pass == 1) ||
		    (cmd == RTM_DELETE && pass == 2)) {
			register struct ifa_msghdr *ifam;
			int ncmd = cmd == RTM_ADD ? RTM_NEWADDR : RTM_DELADDR;

			ifaaddr = sa = ifa->ifa_addr;
#ifndef __REACTOS__
			ifpaddr = ifp->if_addrlist->ifa_addr;
#endif
			netmask = ifa->ifa_netmask;
			brdaddr = ifa->ifa_dstaddr;
			if ((m = rt_msg1(ncmd, &info)) == NULL)
				continue;
			ifam = mtod(m, struct ifa_msghdr *);
#ifndef __REACTOS__
			ifam->ifam_index = ifp->if_index;
#endif
			ifam->ifam_metric = ifa->ifa_metric;
			ifam->ifam_flags = ifa->ifa_flags;
			ifam->ifam_addrs = info.rti_addrs;
		}
		if ((cmd == RTM_ADD && pass == 2) ||
		    (cmd == RTM_DELETE && pass == 1)) {
			register struct rt_msghdr *rtm;

			if (rt == 0)
				continue;
			netmask = rt_mask(rt);
			dst = sa = rt_key(rt);
			gate = rt->rt_gateway;
			if ((m = rt_msg1(cmd, &info)) == NULL)
				continue;
			rtm = mtod(m, struct rt_msghdr *);
#ifndef __REACTOS__
			rtm->rtm_index = ifp->if_index;
#endif
			rtm->rtm_flags |= rt->rt_flags;
			rtm->rtm_errno = error;
			rtm->rtm_addrs = info.rti_addrs;
		}
		route_proto.sp_protocol = sa ? sa->sa_family : 0;
		raw_input(m, &route_proto, &route_src, &route_dst);
	}
}

/*
 * This is used in dumping the kernel table via sysctl().
 */
int
sysctl_dumpentry(rn, w)
	struct radix_node *rn;
	register struct walkarg *w;
{
	register struct rtentry *rt = (struct rtentry *)rn;
	int error = 0, size;
	struct rt_addrinfo info;

	if (w->w_op == NET_RT_FLAGS && !(rt->rt_flags & w->w_arg))
		return 0;
	bzero((caddr_t)&info, sizeof(info));
	dst = rt_key(rt);
	gate = rt->rt_gateway;
	netmask = rt_mask(rt);
	genmask = rt->rt_genmask;
	size = rt_msg2(RTM_GET, &info, 0, w);
	if (w->w_where && w->w_tmem) {
		register struct rt_msghdr *rtm = (struct rt_msghdr *)w->w_tmem;

		rtm->rtm_flags = rt->rt_flags;
		rtm->rtm_use = rt->rt_use;
		rtm->rtm_rmx = rt->rt_rmx;
		rtm->rtm_index = rt->rt_ifp->if_index;
		rtm->rtm_errno = rtm->rtm_pid = rtm->rtm_seq = 0;
		rtm->rtm_addrs = info.rti_addrs;
		error = copyout((caddr_t)rtm, w->w_where, size);
		if (error)
			w->w_where = NULL;
		else
			w->w_where += size;
	}
	return (error);
}

int
sysctl_iflist(af, w)
	int	af;
	register struct	walkarg *w;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	struct	rt_addrinfo info;
	int	len, error = 0;

	bzero((caddr_t)&info, sizeof(info));
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (w->w_arg && w->w_arg != ifp->if_index)
			continue;
		ifa = ifp->if_addrlist;
		ifpaddr = ifa->ifa_addr;
		len = rt_msg2(RTM_IFINFO, &info, (caddr_t)0, w);
		ifpaddr = 0;
		if (w->w_where && w->w_tmem) {
			register struct if_msghdr *ifm;

			ifm = (struct if_msghdr *)w->w_tmem;
			ifm->ifm_index = ifp->if_index;
			ifm->ifm_flags = (u_short)ifp->if_flags;
			ifm->ifm_data = ifp->if_data;
			ifm->ifm_addrs = info.rti_addrs;
			error = copyout((caddr_t)ifm, w->w_where, len);
			if (error)
				return (error);
			w->w_where += len;
		}
#ifndef __REACTOS__
		while ((ifa = ifa->ifa_next) != 0) {
			if (af && af != ifa->ifa_addr->sa_family)
				continue;
			ifaaddr = ifa->ifa_addr;
			netmask = ifa->ifa_netmask;
			brdaddr = ifa->ifa_dstaddr;
			len = rt_msg2(RTM_NEWADDR, &info, 0, w);
			if (w->w_where && w->w_tmem) {
				register struct ifa_msghdr *ifam;

				ifam = (struct ifa_msghdr *)w->w_tmem;
				ifam->ifam_index = ifa->ifa_ifp->if_index;
				ifam->ifam_flags = ifa->ifa_flags;
				ifam->ifam_metric = ifa->ifa_metric;
				ifam->ifam_addrs = info.rti_addrs;
				error = copyout(w->w_tmem, w->w_where, len);
				if (error)
					return (error);
				w->w_where += len;
			}
		}
#endif
		ifaaddr = netmask = brdaddr = 0;
	}
	return (0);
}

int
sysctl_rtable(name, namelen, where, given, new, newlen)
	int	*name;
	int	namelen;
	caddr_t	where;
	size_t	*given;
	caddr_t	*new;
	size_t	newlen;
{
	register struct radix_node_head *rnh;
	int	i, s, error = EINVAL;
	u_char  af;
	struct	walkarg w;

	if (new)
		return (EPERM);
	if (namelen != 3)
		return (EINVAL);
	af = name[0];
	Bzero(&w, sizeof(w));
	w.w_where = where;
	w.w_given = *given;
	w.w_needed = 0 - w.w_given;
	w.w_op = name[1];
	w.w_arg = name[2];

	s = splnet();
	switch (w.w_op) {

	case NET_RT_DUMP:
	case NET_RT_FLAGS:
		for (i = 1; i <= AF_MAX; i++)
			if ((rnh = rt_tables[i]) && (af == 0 || af == i) &&
			    (error = rnh->rnh_walktree(rnh,
							sysctl_dumpentry, &w)))
				break;
		break;

	case NET_RT_IFLIST:
		error = sysctl_iflist(af, &w);
	}
	splx(s);
	if (w.w_tmem)
		free(w.w_tmem, M_RTABLE);
	w.w_needed += w.w_given;
	if (where) {
		*given = w.w_where - where;
		if (*given < w.w_needed)
			return (ENOMEM);
	} else {
		*given = (11 * w.w_needed) / 10;
	}
	return (error);
}

/*
 * Definitions of protocols supported in the ROUTE domain.
 */

extern	struct domain routedomain;		/* or at least forward */

struct protosw routesw[] = {
{ SOCK_RAW,	&routedomain,	0,		PR_ATOMIC|PR_ADDR,
  raw_input,	route_output,	raw_ctlinput,	0,
  route_usrreq,
  raw_init,	0,		0,		0,
  sysctl_rtable,
}
};

struct domain routedomain =
    { PF_ROUTE, "route", route_init, 0, 0,
      routesw, &routesw[sizeof(routesw)/sizeof(routesw[0])] };

DOMAIN_SET(route);
