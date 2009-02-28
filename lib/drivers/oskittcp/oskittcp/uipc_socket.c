/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
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
 *	@(#)uipc_socket.c	8.3 (Berkeley) 4/15/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/kernel.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <oskittcp.h>

/*
 * Exported to userland via sysctl
 */
int somaxconn = SOMAXCONN;

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 */
/*ARGSUSED*/
int
socreate(dom, aso, type, proto)
	int dom;
	struct socket **aso;
	register int type;
	int proto;
{
#ifndef __REACTOS__
	struct proc *p = curproc;		/* XXX */
#endif
	register struct protosw *prp;
	register struct socket *so;
	register int error;

	if (proto)
		prp = pffindproto(dom, proto, type);
	else
		prp = pffindtype(dom, type);
	if (prp == 0 || prp->pr_usrreq == 0)
		return (EPROTONOSUPPORT);
	if (prp->pr_type != type)
		return (EPROTOTYPE);
	MALLOC(so, struct socket *, sizeof(*so), M_SOCKET, M_WAIT);
	bzero((caddr_t)so, sizeof(*so));
	so->so_type = type;
#ifndef __REACTOS__
	if (p->p_ucred->cr_uid == 0)
		so->so_state = SS_PRIV;
#endif
	so->so_proto = prp;
	error =
	    (*prp->pr_usrreq)(so, PRU_ATTACH,
		(struct mbuf *)0, (struct mbuf *)proto, (struct mbuf *)0);
	if (error) {
		so->so_state |= SS_NOFDREF;
		sofree(so);
		return (error);
	}
	*aso = so;
	return (0);
}

int
sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *)0, nam, (struct mbuf *)0);
	splx(s);
	return (error);
}

int
solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	int s = splnet(), error;

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		splx(s);
		return (error);
	}
	if (so->so_q == 0)
		so->so_options |= SO_ACCEPTCONN;
	if (backlog < 0 || backlog > somaxconn)
		backlog = somaxconn;
	so->so_qlimit = backlog;
	splx(s);
	return (0);
}

void
sofree(so)
	register struct socket *so;
{

	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
		return;
	if (so->so_head) {
		if (!soqremque(so, 0) && !soqremque(so, 1))
			panic("sofree dq");
		so->so_head = 0;
	}
	sbrelease(&so->so_snd);
	sorflush(so);
	FREE(so, M_SOCKET);
}

/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 */
int
soclose(so)
	register struct socket *so;
{
	int s = splnet();		/* conservative */
	int error = 0;

	if (so->so_options & SO_ACCEPTCONN) {
		while (so->so_q0)
			(void) soabort(so->so_q0);
		while (so->so_q)
			(void) soabort(so->so_q);
	}
	if (so->so_pcb == 0)
		goto discard;
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so);
			if (error)
				goto drop;
		}
		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & SS_NBIO))
				goto drop;
			while (so->so_state & SS_ISCONNECTED) {
				error = tsleep((caddr_t)&so->so_timeo,
				    PSOCK | PCATCH, netcls, so->so_linger);
				if (error)
					break;
			}
		}
	}
drop:
	if (so->so_pcb) {
		int error2 =
		    (*so->so_proto->pr_usrreq)(so, PRU_DETACH,
			(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
		if (error == 0)
			error = error2;
	}
discard:
	if (so->so_state & SS_NOFDREF)
		panic("soclose: NOFDREF");
	so->so_state |= SS_NOFDREF;
	sofree(so);
	splx(s);
	return (error);
}

/*
 * Must be called at splnet...
 */
int
soabort(so)
	struct socket *so;
{

	return (
	    (*so->so_proto->pr_usrreq)(so, PRU_ABORT,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
}

int
soaccept(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_NOFDREF) == 0)
		panic("soaccept: !NOFDREF");
	so->so_state &= ~SS_NOFDREF;
	error = (*so->so_proto->pr_usrreq)(so, PRU_ACCEPT,
	    (struct mbuf *)0, nam, (struct mbuf *)0);
	splx(s);
	return (error);
}

int
soconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s;
	int error;

	if (so->so_options & SO_ACCEPTCONN)
		return (EOPNOTSUPP);
	s = splnet();
	/*
	 * If protocol is connection-based, can only connect once.
	 * Otherwise, if connected, try to disconnect first.
	 * This allows user to disconnect by connecting to, e.g.,
	 * a null address.
	 */
	if (so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING) &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) ||
	    (error = sodisconnect(so))))
		error = EISCONN;
	else
		error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
		    (struct mbuf *)0, nam, (struct mbuf *)0);
	splx(s);
	return (error);
}

int
soconnect2(so1, so2)
	register struct socket *so1;
	struct socket *so2;
{
	int s = splnet();
	int error;

	error = (*so1->so_proto->pr_usrreq)(so1, PRU_CONNECT2,
	    (struct mbuf *)0, (struct mbuf *)so2, (struct mbuf *)0);
	splx(s);
	return (error);
}

int
sodisconnect(so)
	register struct socket *so;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
bad:
	splx(s);
	return (error);
}

#define	SBLOCKWAIT(f)	(((f) & MSG_DONTWAIT) ? M_NOWAIT : M_WAITOK)
/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 * Otherwise, if nonblocking, send as much as possible.
 * The data to be sent is described by "uio" if nonzero,
 * otherwise by the mbuf chain "top" (which must be null
 * if uio is not).  Data provided in mbuf chain must be small
 * enough to send all at once.
 *
 * Returns nonzero on error, timeout or signal; callers
 * must check for short counts if EINTR/ERESTART are returned.
 * Data and control buffers are freed on return.
 */
int
sosend(so, addr, uio, top, control, flags)
	register struct socket *so;
	struct mbuf *addr;
	struct uio *uio;
	struct mbuf *top;
	struct mbuf *control;
	int flags;
{
#ifndef __REACTOS__
	struct proc *p = curproc;		/* XXX */
#endif
	struct mbuf **mp;
	register struct mbuf *m;
	register long space, len, resid;
	int clen = 0, error, s, dontroute, mlen;
	int atomic = sosendallatonce(so) || top;

	if (uio)
		resid = uio->uio_resid;
	else
		resid = top->m_pkthdr.len;
	/*
	 * In theory resid should be unsigned.
	 * However, space must be signed, as it might be less than 0
	 * if we over-committed, and we must use a signed comparison
	 * of space and resid.  On the other hand, a negative resid
	 * causes us to loop sending 0-length segments to the protocol.
	 */
	if (resid < 0)
		return (EINVAL);
	dontroute =
	    (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);
#ifndef __REACTOS__
	p->p_stats->p_ru.ru_msgsnd++;
#endif
	if (control)
		clen = control->m_len;
#define	snderr(errno)	{ error = errno; splx(s); goto release; }

restart:
	error = sblock(&so->so_snd, SBLOCKWAIT(flags));
	if (error)
		goto out;
	do {
		s = splnet();
		if (so->so_state & SS_CANTSENDMORE)
			snderr(EPIPE);
		if (so->so_error)
			snderr(so->so_error);
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			/*
			 * `sendto' and `sendmsg' is allowed on a connection-
			 * based socket if it supports implied connect.
			 * Return ENOTCONN if not connected and no address is
			 * supplied.
			 */
			if ((so->so_proto->pr_flags & PR_CONNREQUIRED) &&
			    (so->so_proto->pr_flags & PR_IMPLOPCL) == 0) {
				if ((so->so_state & SS_ISCONFIRMING) == 0 &&
				    !(resid == 0 && clen != 0))
					snderr(ENOTCONN);
			} else if (addr == 0)
			    snderr(so->so_proto->pr_flags & PR_CONNREQUIRED ?
				   ENOTCONN : EDESTADDRREQ);
		}
		space = sbspace(&so->so_snd);
		if (flags & MSG_OOB)
			space += 1024;
		if ((atomic && resid > so->so_snd.sb_hiwat) ||
		    clen > so->so_snd.sb_hiwat)
			snderr(EMSGSIZE);
		if (space < resid + clen && uio &&
		    (atomic || space < so->so_snd.sb_lowat || space < clen)) {
			if (so->so_state & SS_NBIO)
				snderr(EWOULDBLOCK);
			sbunlock(so, &so->so_snd);
			error = sbwait(&so->so_snd);
			splx(s);
			if (error)
				goto out;
			goto restart;
		}
		splx(s);
		mp = &top;
		space -= clen;
		do {
		    if (uio == NULL) {
			/*
			 * Data is prepackaged in "top".
			 */
			resid = 0;
			if (flags & MSG_EOR)
				top->m_flags |= M_EOR;
		    } else do {
			if (top == 0) {
				MGETHDR(m, M_WAIT, MT_DATA);
				mlen = MHLEN;
				m->m_pkthdr.len = 0;
				m->m_pkthdr.rcvif = (struct ifnet *)0;
			} else {
				MGET(m, M_WAIT, MT_DATA);
				mlen = MLEN;
			}
			if (resid >= MINCLSIZE) {
				MCLGET(m, M_WAIT);
				if ((m->m_flags & M_EXT) == 0)
					goto nopages;
				mlen = MCLBYTES;
				len = min(min(mlen, resid), space);
			} else {
nopages:
				len = min(min(mlen, resid), space);
				/*
				 * For datagram protocols, leave room
				 * for protocol headers in first mbuf.
				 */
				if (atomic && top == 0 && len < mlen)
					MH_ALIGN(m, len);
			}
			space -= len;
			error = uiomove(mtod(m, caddr_t), (int)len, uio);
			resid = uio->uio_resid;
			m->m_len = len;
			*mp = m;
			top->m_pkthdr.len += len;
			if (error)
				goto release;
			mp = &m->m_next;
			if (resid <= 0) {
				if (flags & MSG_EOR)
					top->m_flags |= M_EOR;
				break;
			}
		    } while (space > 0 && atomic);
		    if (dontroute)
			    so->so_options |= SO_DONTROUTE;
		    s = splnet();				/* XXX */
		    error = (*so->so_proto->pr_usrreq)(so,
			(flags & MSG_OOB) ? PRU_SENDOOB :
			/*
			 * If the user set MSG_EOF, the protocol
			 * understands this flag and nothing left to
			 * send then use PRU_SEND_EOF instead of PRU_SEND.
			 */
			((flags & MSG_EOF) &&
			 (so->so_proto->pr_flags & PR_IMPLOPCL) &&
			 (resid <= 0)) ?
				PRU_SEND_EOF : PRU_SEND,
			top, addr, control);
		    splx(s);
		    if (dontroute)
			    so->so_options &= ~SO_DONTROUTE;
		    clen = 0;
		    control = 0;
		    top = 0;
		    mp = &top;
		    if (error)
			goto release;
		} while (resid && space > 0);
	} while (resid);

release:
	sbunlock(so, &so->so_snd);
out:
	if (top)
		m_freem(top);
	if (control)
		m_freem(control);
	return (error);
}

/*
 * Implement receive operations on a socket.
 * We depend on the way that records are added to the sockbuf
 * by sbappend*.  In particular, each record (mbufs linked through m_next)
 * must begin with an address if the protocol so specifies,
 * followed by an optional mbuf or mbufs containing ancillary data,
 * and then zero or more mbufs of data.
 * In order to avoid blocking network interrupts for the entire time here,
 * we splx() while doing the actual copy to user space.
 * Although the sockbuf is locked, new data may still be appended,
 * and thus we must maintain consistency of the sockbuf during that time.
 *
 * The caller may receive the data as a single mbuf chain by supplying
 * an mbuf **mp0 for use in returning the chain.  The uio is then used
 * only for the count in uio_resid.
 */
int
soreceive(so, paddr, uio, mp0, controlp, flagsp)
	register struct socket *so;
	struct mbuf **paddr;
	struct uio *uio;
	struct mbuf **mp0;
	struct mbuf **controlp;
	int *flagsp;
{
	register struct mbuf *m, **mp;
	register int flags, len, error, s, offset;
	struct protosw *pr = so->so_proto;
	struct mbuf *nextrecord;
	int moff, type = 0;
	int orig_resid = uio->uio_resid;

	mp = mp0;
	if (paddr)
		*paddr = 0;
	if (controlp)
		*controlp = 0;
	if (flagsp)
		flags = *flagsp &~ MSG_EOR;
	else
		flags = 0;
	if (flags & MSG_OOB) {
		m = m_get(M_WAIT, MT_DATA);
		error = (*pr->pr_usrreq)(so, PRU_RCVOOB,
		    m, (struct mbuf *)(flags & MSG_PEEK), (struct mbuf *)0);
		if (error)
			goto bad;
		do {
			error = uiomove(mtod(m, caddr_t),
			    (int) min(uio->uio_resid, m->m_len), uio);
			m = m_free(m);
		} while (uio->uio_resid && error == 0 && m);
bad:
		if (m)
			m_freem(m);
		return (error);
	}
	if (mp)
		*mp = (struct mbuf *)0;
	if (so->so_state & SS_ISCONFIRMING && uio->uio_resid)
		(*pr->pr_usrreq)(so, PRU_RCVD, (struct mbuf *)0,
		    (struct mbuf *)0, (struct mbuf *)0);

restart:
	error = sblock(&so->so_rcv, SBLOCKWAIT(flags));
	if (error)
		return (error);
	s = splnet();

	m = so->so_rcv.sb_mb;
	/*
	 * If we have less data than requested, block awaiting more
	 * (subject to any timeout) if:
	 *   1. the current count is less than the low water mark, or
	 *   2. MSG_WAITALL is set, and it is possible to do the entire
	 *	receive operation at once if we block (resid <= hiwat).
	 *   3. MSG_DONTWAIT is not set
	 * If MSG_WAITALL is set but resid is larger than the receive buffer,
	 * we have to do the receive in sections, and thus risk returning
	 * a short count if a timeout or signal occurs after we start.
	 */
	if (m == 0 || (((flags & MSG_DONTWAIT) == 0 &&
	    so->so_rcv.sb_cc < uio->uio_resid) &&
	    (so->so_rcv.sb_cc < so->so_rcv.sb_lowat ||
	    ((flags & MSG_WAITALL) && uio->uio_resid <= so->so_rcv.sb_hiwat)) &&
	    m->m_nextpkt == 0 && (pr->pr_flags & PR_ATOMIC) == 0)) {
#ifdef DIAGNOSTIC
		if (m == 0 && so->so_rcv.sb_cc)
			panic("receive 1");
#endif
		if (so->so_error) {
			if (m)
				goto dontblock;
			error = so->so_error;
			if ((flags & MSG_PEEK) == 0)
				so->so_error = 0;
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE) {
			if (m)
				goto dontblock;
			else
				goto release;
		}
		for (; m; m = m->m_next)
			if (m->m_type == MT_OOBDATA  || (m->m_flags & M_EOR)) {
				m = so->so_rcv.sb_mb;
				goto dontblock;
			}
		if ((so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING)) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED)) {
		    printf("so: %x\n", so);
		    __asm__("int3");
			error = ENOTCONN;
			goto release;
		}
		if (uio->uio_resid == 0)
			goto release;
		if ((so->so_state & SS_NBIO) || (flags & MSG_DONTWAIT)) {
			error = EWOULDBLOCK;
			goto release;
		}
		sbunlock(so, &so->so_rcv);
		error = sbwait(&so->so_rcv);
		splx(s);
		if (error)
			return (error);
		goto restart;
	}
dontblock:
	if (uio->uio_procp)
		uio->uio_procp->p_stats->p_ru.ru_msgrcv++;
	nextrecord = m->m_nextpkt;
	if (pr->pr_flags & PR_ADDR) {
#ifdef DIAGNOSTIC
		if (m->m_type != MT_SONAME)
			panic("receive 1a");
#endif
		orig_resid = 0;
		if (flags & MSG_PEEK) {
			if (paddr)
				*paddr = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (paddr) {
				*paddr = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = 0;
				m = so->so_rcv.sb_mb;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
			}
		}
	}
	while (m && m->m_type == MT_CONTROL && error == 0) {
		if (flags & MSG_PEEK) {
			if (controlp)
				*controlp = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
#ifndef __REACTOS__
			if (controlp) {
				if (pr->pr_domain->dom_externalize &&
				    mtod(m, struct cmsghdr *)->cmsg_type ==
				    SCM_RIGHTS)
				   error = (*pr->pr_domain->dom_externalize)(m);
				*controlp = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = 0;
				m = so->so_rcv.sb_mb;
			} else
#endif
			{
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
			}
		}
		if (controlp) {
			orig_resid = 0;
			controlp = &(*controlp)->m_next;
		}
	}
	if (m) {
		if ((flags & MSG_PEEK) == 0)
			m->m_nextpkt = nextrecord;
		type = m->m_type;
		if (type == MT_OOBDATA)
			flags |= MSG_OOB;
	}
	moff = 0;
	offset = 0;
	while (m && uio->uio_resid > 0 && error == 0) {
		if (m->m_type == MT_OOBDATA) {
			if (type != MT_OOBDATA)
				break;
		} else if (type == MT_OOBDATA)
			break;
#ifdef DIAGNOSTIC
		else if (m->m_type != MT_DATA && m->m_type != MT_HEADER)
			panic("receive 3");
#endif
		so->so_state &= ~SS_RCVATMARK;
		len = uio->uio_resid;
		if (so->so_oobmark && len > so->so_oobmark - offset)
			len = so->so_oobmark - offset;
		if (len > m->m_len - moff)
			len = m->m_len - moff;
		/*
		 * If mp is set, just pass back the mbufs.
		 * Otherwise copy them out via the uio, then free.
		 * Sockbuf must be consistent here (points to current mbuf,
		 * it points to next record) when we drop priority;
		 * we must note any additions to the sockbuf when we
		 * block interrupts again.
		 */
		if (mp == 0) {
			splx(s);
			error = uiomove(mtod(m, caddr_t) + moff, (int)len, uio);
			s = splnet();
		} else
			uio->uio_resid -= len;
		if (len == m->m_len - moff) {
			if (m->m_flags & M_EOR)
				flags |= MSG_EOR;
			if (flags & MSG_PEEK) {
				m = m->m_next;
				moff = 0;
			} else {
				nextrecord = m->m_nextpkt;
				sbfree(&so->so_rcv, m);
				if (mp) {
					*mp = m;
					mp = &m->m_next;
					so->so_rcv.sb_mb = m = m->m_next;
					*mp = (struct mbuf *)0;
				} else {
					MFREE(m, so->so_rcv.sb_mb);
					m = so->so_rcv.sb_mb;
				}
				if (m)
					m->m_nextpkt = nextrecord;
			}
		} else {
			if (flags & MSG_PEEK)
				moff += len;
			else {
				if (mp)
					*mp = m_copym(m, 0, len, M_WAIT);
				m->m_data += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
		if (so->so_oobmark) {
			if ((flags & MSG_PEEK) == 0) {
				so->so_oobmark -= len;
				if (so->so_oobmark == 0) {
					so->so_state |= SS_RCVATMARK;
					break;
				}
			} else {
				offset += len;
				if (offset == so->so_oobmark)
					break;
			}
		}
		if (flags & MSG_EOR)
			break;
		/*
		 * If the MSG_WAITALL flag is set (for non-atomic socket),
		 * we must not quit until "uio->uio_resid == 0" or an error
		 * termination.  If a signal/timeout occurs, return
		 * with a short count but without error.
		 * Keep sockbuf locked against other readers.
		 */
		while (flags & MSG_WAITALL && m == 0 && uio->uio_resid > 0 &&
		       !sosendallatonce(so) && !nextrecord) {
			if (so->so_error || so->so_state & SS_CANTRCVMORE)
				break;
			error = sbwait(&so->so_rcv);
			if (error) {
				sbunlock(so, &so->so_rcv);
				splx(s);
				return (0);
			}
			m = so->so_rcv.sb_mb;
			if (m)
				nextrecord = m->m_nextpkt;
		}
	}

	if (m && pr->pr_flags & PR_ATOMIC) {
		flags |= MSG_TRUNC;
		if ((flags & MSG_PEEK) == 0)
			(void) sbdroprecord(&so->so_rcv);
	}
	if ((flags & MSG_PEEK) == 0) {
		if (m == 0)
			so->so_rcv.sb_mb = nextrecord;
		if (pr->pr_flags & PR_WANTRCVD && so->so_pcb)
			(*pr->pr_usrreq)(so, PRU_RCVD, (struct mbuf *)0,
			    (struct mbuf *)flags, (struct mbuf *)0);
	}
	if (orig_resid == uio->uio_resid && orig_resid &&
	    (flags & MSG_EOR) == 0 && (so->so_state & SS_CANTRCVMORE) == 0) {
		sbunlock(so, &so->so_rcv);
		splx(s);
		goto restart;
	}

	if (flagsp)
		*flagsp |= flags;
release:
	sbunlock(so, &so->so_rcv);
	splx(s);
	return (error);
}

int
soshutdown(so, how)
	register struct socket *so;
	register int how;
{
    if (so == NULL)
    {
     	register struct protosw *pr = NULL;

	how++;
	if (how & FREAD)
		sorflush(so);
	if (how & FWRITE)
		return ((*pr->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
	return (0);
    }

	register struct protosw *pr = so->so_proto;

	how++;
	if (how & FREAD)
		sorflush(so);
	if (how & FWRITE)
		return ((*pr->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
	return (0);
}

void
sorflush(so)
	register struct socket *so;
{
	register struct sockbuf *sb = &so->so_rcv;
	register struct protosw *pr = so->so_proto;
	register int s;
	struct sockbuf asb;

	sb->sb_flags |= SB_NOINTR;
	(void) sblock(sb, M_WAITOK);
	s = splimp();
	socantrcvmore(so);
	sbunlock(so, sb);
	asb = *sb;
	bzero((caddr_t)sb, sizeof (*sb));
	splx(s);
	if (pr->pr_flags & PR_RIGHTS && pr->pr_domain->dom_dispose)
		(*pr->pr_domain->dom_dispose)(asb.sb_mb);
	sbrelease(&asb);
}

int
sosetopt(so, level, optname, m0)
	register struct socket *so;
	int level, optname;
	struct mbuf *m0;
{
	int error = 0;
	register struct mbuf *m = m0;

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput)
			return ((*so->so_proto->pr_ctloutput)
				  (PRCO_SETOPT, so, level, optname, &m0));
		error = ENOPROTOOPT;
	} else {
		switch (optname) {

		case SO_LINGER:
			if (m == NULL || m->m_len != sizeof (struct linger)) {
				error = EINVAL;
				goto bad;
			}
			so->so_linger = mtod(m, struct linger *)->l_linger;
			/* fall thru... */

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_OOBINLINE:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			if (*mtod(m, int *))
				so->so_options |= optname;
			else
				so->so_options &= ~optname;
			break;

		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			switch (optname) {

			case SO_SNDBUF:
			case SO_RCVBUF:
				if (sbreserve(optname == SO_SNDBUF ?
				    &so->so_snd : &so->so_rcv,
				    (u_long) *mtod(m, int *)) == 0) {
					error = ENOBUFS;
					goto bad;
				}
				break;

			case SO_SNDLOWAT:
				so->so_snd.sb_lowat = *mtod(m, int *);
				break;
			case SO_RCVLOWAT:
				so->so_rcv.sb_lowat = *mtod(m, int *);
				break;
			}
			break;

		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		    {
			struct timeval *tv;
			short val;

			if (m == NULL || m->m_len < sizeof (*tv)) {
				error = EINVAL;
				goto bad;
			}
			tv = mtod(m, struct timeval *);
			if (tv->tv_sec > SHRT_MAX / hz - hz) {
				error = EDOM;
				goto bad;
			}
			val = tv->tv_sec * hz + tv->tv_usec / tick;

			switch (optname) {

			case SO_SNDTIMEO:
				so->so_snd.sb_timeo = val;
				break;
			case SO_RCVTIMEO:
				so->so_rcv.sb_timeo = val;
				break;
			}
			break;
		    }

		default:
			error = ENOPROTOOPT;
			break;
		}
		if (error == 0 && so->so_proto && so->so_proto->pr_ctloutput) {
			(void) ((*so->so_proto->pr_ctloutput)
				  (PRCO_SETOPT, so, level, optname, &m0));
			m = NULL;	/* freed by protocol */
		}
	}
bad:
	if (m)
		(void) m_free(m);
	return (error);
}

int
sogetopt(so, level, optname, mp)
	register struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	register struct mbuf *m;

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput) {
			return ((*so->so_proto->pr_ctloutput)
				  (PRCO_GETOPT, so, level, optname, mp));
		} else
			return (ENOPROTOOPT);
	} else {
		m = m_get(M_WAIT, MT_SOOPTS);
		m->m_len = sizeof (int);

		switch (optname) {

		case SO_LINGER:
			m->m_len = sizeof (struct linger);
			mtod(m, struct linger *)->l_onoff =
				so->so_options & SO_LINGER;
			mtod(m, struct linger *)->l_linger = so->so_linger;
			break;

		case SO_USELOOPBACK:
		case SO_DONTROUTE:
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_BROADCAST:
		case SO_OOBINLINE:
			*mtod(m, int *) = so->so_options & optname;
			break;

		case SO_TYPE:
			*mtod(m, int *) = so->so_type;
			break;

		case SO_ERROR:
			*mtod(m, int *) = so->so_error;
			so->so_error = 0;
			break;

		case SO_SNDBUF:
			*mtod(m, int *) = so->so_snd.sb_hiwat;
			break;

		case SO_RCVBUF:
			*mtod(m, int *) = so->so_rcv.sb_hiwat;
			break;

		case SO_SNDLOWAT:
			*mtod(m, int *) = so->so_snd.sb_lowat;
			break;

		case SO_RCVLOWAT:
			*mtod(m, int *) = so->so_rcv.sb_lowat;
			break;

		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		    {
			int val = (optname == SO_SNDTIMEO ?
			     so->so_snd.sb_timeo : so->so_rcv.sb_timeo);

			m->m_len = sizeof(struct timeval);
			mtod(m, struct timeval *)->tv_sec = val / hz;
			mtod(m, struct timeval *)->tv_usec =
			    (val % hz) * tick;
			break;
		    }

		default:
			(void)m_free(m);
			return (ENOPROTOOPT);
		}
		*mp = m;
		return (0);
	}
}

void
sohasoutofband(so)
	register struct socket *so;
{
	struct proc *p;

	if (so->so_pgid < 0)
		gsignal(-so->so_pgid, SIGURG);
	else if (so->so_pgid > 0 && (p = pfind(so->so_pgid)) != 0)
		psignal(p, SIGURG);
#ifndef __REACTOS__
	selwakeup(so, &so->so_rcv.sb_sel);
#else
	sorwakeup(so);
#endif
}
