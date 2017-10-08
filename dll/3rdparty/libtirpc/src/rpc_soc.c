
/*
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 */

#if defined(PORTMAP) || defined (_WIN32)
/*
 * rpc_soc.c
 *
 * The backward compatibility routines for the earlier implementation
 * of RPC, where the only transports supported were tcp/ip and udp/ip.
 * Based on berkeley socket abstraction, now implemented on the top
 * of TLI/Streams
 */
#include <wintirpc.h>
//#include <pthread.h>
#include <reentrant.h>
#include <sys/types.h>
//#include <sys/socket.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <rpc/nettype.h>
//#include <syslog.h>
//#include <netinet/in.h>
//#include <netdb.h>
#include <errno.h>
//#include <syslog.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#include "rpc_com.h"

extern mutex_t	rpcsoc_lock;

static CLIENT *clnt_com_create(struct sockaddr_in *, rpcprog_t, rpcvers_t,
    int *, u_int, u_int, char *);
static SVCXPRT *svc_com_create(SOCKET, u_int, u_int, char *);
static bool_t rpc_wrap_bcast(char *, struct netbuf *, struct netconfig *);

/* XXX */
#define IN4_LOCALHOST_STRING    "127.0.0.1"
#define IN6_LOCALHOST_STRING    "::1"

/*
 * A common clnt create routine
 */
static CLIENT *
clnt_com_create(raddr, prog, vers, sockp, sendsz, recvsz, tp)
	struct sockaddr_in *raddr;
	rpcprog_t prog;
	rpcvers_t vers;
#ifndef __REACTOS__
	SOCKET *sockp;
#else
    int *sockp;
#endif
	u_int sendsz;
	u_int recvsz;
	char *tp;
{
	CLIENT *cl;
	int madefd = FALSE;
	SOCKET fd = *sockp;
	struct netconfig *nconf;
	struct netbuf bindaddr;

	mutex_lock(&rpcsoc_lock);
	if ((nconf = __rpc_getconfip(tp)) == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		mutex_unlock(&rpcsoc_lock);
		return (NULL);
	}
	if (fd == RPC_ANYSOCK) {
		fd = __rpc_nconf2fd(nconf);
		if (fd == -1)
			goto syserror;
		madefd = TRUE;
	}

	if (raddr->sin_port == 0) {
		u_int proto;
		u_short sport;

		mutex_unlock(&rpcsoc_lock);	/* pmap_getport is recursive */
		proto = strcmp(tp, "udp") == 0 ? IPPROTO_UDP : IPPROTO_TCP;
		sport = pmap_getport(raddr, (u_long)prog, (u_long)vers,
		    proto);
		if (sport == 0) {
			goto err;
		}
		raddr->sin_port = htons(sport);
		mutex_lock(&rpcsoc_lock);	/* pmap_getport is recursive */
	}

	/* Transform sockaddr_in to netbuf */
	bindaddr.maxlen = bindaddr.len =  sizeof (struct sockaddr_in);
	bindaddr.buf = raddr;

	bindresvport(fd, NULL);
	cl = clnt_tli_create(fd, nconf, &bindaddr, prog, vers,
				sendsz, recvsz, NULL, NULL, NULL);
	if (cl) {
		if (madefd == TRUE) {
			/*
			 * The fd should be closed while destroying the handle.
			 */
			(void) CLNT_CONTROL(cl, CLSET_FD_CLOSE, NULL);
			*sockp = fd;
		}
		(void) freenetconfigent(nconf);
		mutex_unlock(&rpcsoc_lock);
		return (cl);
	}
	goto err;

syserror:
	rpc_createerr.cf_stat = RPC_SYSTEMERROR;
	rpc_createerr.cf_error.re_errno = errno;

err:	if (madefd == TRUE)
		(void)closesocket(fd);
	(void) freenetconfigent(nconf);
	mutex_unlock(&rpcsoc_lock);
	return (NULL);
}

CLIENT *
clntudp_bufcreate(raddr, prog, vers, wait, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	struct timeval wait;
	int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *cl;

	cl = clnt_com_create(raddr, (rpcprog_t)prog, (rpcvers_t)vers, sockp,
	    sendsz, recvsz, "udp");
	if (cl == NULL) {
		return (NULL);
	}
	(void) CLNT_CONTROL(cl, CLSET_RETRY_TIMEOUT, &wait);
	return (cl);
}

CLIENT *
clntudp_create(raddr, program, version, wait, sockp)
	struct sockaddr_in *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	int *sockp;
{
	return clntudp_bufcreate(raddr, program, version, wait, sockp, UDPMSGSIZE, UDPMSGSIZE);
}

CLIENT *
clnttcp_create(raddr, prog, vers, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	return clnt_com_create(raddr, (rpcprog_t)prog, (rpcvers_t)vers, sockp,
	    sendsz, recvsz, "tcp");
}

/* IPv6 version of clnt*_*create */

#ifdef INET6_NOT_USED

CLIENT *
clntudp6_bufcreate(raddr, prog, vers, wait, sockp, sendsz, recvsz)
	struct sockaddr_in6 *raddr;
	u_long prog;
	u_long vers;
	struct timeval wait;
	int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *cl;

	cl = clnt_com_create(raddr, (rpcprog_t)prog, (rpcvers_t)vers, sockp,
	    sendsz, recvsz, "udp6");
	if (cl == NULL) {
		return (NULL);
	}
	(void) CLNT_CONTROL(cl, CLSET_RETRY_TIMEOUT, &wait);
	return (cl);
}

CLIENT *
clntudp6_create(raddr, program, version, wait, sockp)
	struct sockaddr_in6 *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	int *sockp;
{
	return clntudp6_bufcreate(raddr, program, version, wait, sockp, UDPMSGSIZE, UDPMSGSIZE);
}

CLIENT *
clnttcp6_create(raddr, prog, vers, sockp, sendsz, recvsz)
	struct sockaddr_in6 *raddr;
	u_long prog;
	u_long vers;
	int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	return clnt_com_create(raddr, (rpcprog_t)prog, (rpcvers_t)vers, sockp,
	    sendsz, recvsz, "tcp6");
}

#endif

CLIENT *
clntraw_create(prog, vers)
	u_long prog;
	u_long vers;
{
	return clnt_raw_create((rpcprog_t)prog, (rpcvers_t)vers);
}

/*
 * A common server create routine
 */
static SVCXPRT *
svc_com_create(fd, sendsize, recvsize, netid)
	SOCKET fd;
	u_int sendsize;
	u_int recvsize;
	char *netid;
{
	struct netconfig *nconf;
	SVCXPRT *svc;
	int madefd = FALSE;
	int port;
	struct sockaddr_in sin;

	if ((nconf = __rpc_getconfip(netid)) == NULL) {
		//(void) syslog(LOG_ERR, "Could not get %s transport", netid);
		return (NULL);
	}
	if (fd == RPC_ANYSOCK) {
		fd = __rpc_nconf2fd(nconf);
		if (fd == -1) {
			(void) freenetconfigent(nconf);
			//(void) syslog(LOG_ERR,
			//"svc%s_create: could not open connection", netid);
			return (NULL);
		}
		madefd = TRUE;
	}

	memset(&sin, 0, sizeof sin);
	sin.sin_family = AF_INET;
	bindresvport(fd, &sin);
	listen(fd, SOMAXCONN);
	svc = svc_tli_create(fd, nconf, NULL, sendsize, recvsize);
	(void) freenetconfigent(nconf);
	if (svc == NULL) {
		if (madefd)
			(void)closesocket(fd);
		return (NULL);
	}
	port = (((struct sockaddr_in *)svc->xp_ltaddr.buf)->sin_port);
	svc->xp_port = ntohs((u_short)port);
	return (svc);
}

SVCXPRT *
svctcp_create(fd, sendsize, recvsize)
#ifndef __REACTOS__
	SOCKET fd;
#else
    int fd;
#endif
	u_int sendsize;
	u_int recvsize;
{

	return svc_com_create(fd, sendsize, recvsize, "tcp");
}



SVCXPRT *
svcudp_bufcreate(fd, sendsz, recvsz)
#ifndef __REACTOS__
	SOCKET fd;
#else
    int fd;
#endif
	u_int sendsz, recvsz;
{

	return svc_com_create(fd, sendsz, recvsz, "udp");
}



SVCXPRT *
svcfd_create(fd, sendsize, recvsize)
#ifndef __REACTOS__
	SOCKET fd;
#else
    int fd;
#endif
	u_int sendsize;
	u_int recvsize;
{

	return svc_fd_create(fd, sendsize, recvsize);
}


SVCXPRT *
svcudp_create(fd)
#ifndef __REACTOS__
	SOCKET fd;
#else
    int fd;
#endif
{

	return svc_com_create(fd, UDPMSGSIZE, UDPMSGSIZE, "udp");
}


SVCXPRT *
svcraw_create()
{

	return svc_raw_create();
}


/* IPV6 version */
#ifdef INET6_NOT_USED
SVCXPRT *
svcudp6_bufcreate(fd, sendsz, recvsz)
	int fd;
	u_int sendsz, recvsz;
{
	return svc_com_create(fd, sendsz, recvsz, "udp6");
}


SVCXPRT *
svctcp6_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	return svc_com_create(fd, sendsize, recvsize, "tcp6");
}


SVCXPRT *
svcudp6_create(fd)
	int fd;
{
	return svc_com_create(fd, UDPMSGSIZE, UDPMSGSIZE, "udp6");
}
#endif

int
get_myaddress(addr)
	struct sockaddr_in *addr;
{

	memset((void *) addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(PMAPPORT);
	addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	return (0);
}

/*
 * For connectionless "udp" transport. Obsoleted by rpc_call().
 */
int
callrpc(host, prognum, versnum, procnum, inproc, in, outproc, out)
	const char *host;
	int prognum, versnum, procnum;
	xdrproc_t inproc, outproc;
	void *in, *out;
{

	return (int)rpc_call(host, (rpcprog_t)prognum, (rpcvers_t)versnum,
	    (rpcproc_t)procnum, inproc, in, outproc, out, "udp");
}

/*
 * For connectionless kind of transport. Obsoleted by rpc_reg()
 */
int
registerrpc(prognum, versnum, procnum, progname, inproc, outproc)
	int prognum, versnum, procnum;
	char *(*progname)(char [UDPMSGSIZE]);
	xdrproc_t inproc, outproc;
{

	return rpc_reg((rpcprog_t)prognum, (rpcvers_t)versnum,
	    (rpcproc_t)procnum, progname, inproc, outproc, "udp");
}

/*
 * All the following clnt_broadcast stuff is convulated; it supports
 * the earlier calling style of the callback function
 */
extern thread_key_t	clnt_broadcast_key;

/*
 * Need to translate the netbuf address into sockaddr_in address.
 * Dont care about netid here.
 */
/* ARGSUSED */
static bool_t
rpc_wrap_bcast(resultp, addr, nconf)
	char *resultp;		/* results of the call */
	struct netbuf *addr;	/* address of the guy who responded */
	struct netconfig *nconf; /* Netconf of the transport */
{
	resultproc_t clnt_broadcast_result;

	if (strcmp(nconf->nc_netid, "udp"))
		return (FALSE);
	clnt_broadcast_result = (resultproc_t)thr_getspecific(clnt_broadcast_key);
	return (*clnt_broadcast_result)(resultp,
				(struct sockaddr_in *)addr->buf);
}

/*
 * Broadcasts on UDP transport. Obsoleted by rpc_broadcast().
 */
enum clnt_stat
clnt_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp, eachresult)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	void	       *argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	void	       *resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
{
	extern mutex_t tsd_lock;

	if (clnt_broadcast_key == -1) {
		mutex_lock(&tsd_lock);
		if (clnt_broadcast_key == -1)
			thr_keycreate(&clnt_broadcast_key, free);
		mutex_unlock(&tsd_lock);
	}
	thr_setspecific(clnt_broadcast_key, (void *) eachresult);
	return rpc_broadcast((rpcprog_t)prog, (rpcvers_t)vers,
	    (rpcproc_t)proc, xargs, argsp, xresults, resultsp,
	    (resultproc_t) rpc_wrap_bcast, "udp");
}

#ifndef _WIN32
/*
 * Create the client des authentication object. Obsoleted by
 * authdes_seccreate().
 */
AUTH *
authdes_create(servername, window, syncaddr, ckey)
	char *servername;		/* network name of server */
	u_int window;			/* time to live */
	struct sockaddr *syncaddr;	/* optional hostaddr to sync with */
	des_block *ckey;		/* optional conversation key to use */
{
	AUTH *dummy;
	AUTH *nauth;
	char hostname[NI_MAXHOST];

	if (syncaddr) {
		/*
		 * Change addr to hostname, because that is the way
		 * new interface takes it.
		 */
		if (getnameinfo(syncaddr, sizeof(syncaddr), hostname,
		    sizeof hostname, NULL, 0, 0) != 0)
			goto fallback;

		nauth = authdes_seccreate(servername, window, hostname, ckey);
		return (nauth);
	}
fallback:
	dummy = authdes_seccreate(servername, window, NULL, ckey);
	return (dummy);
}
#endif

/*
 * Create a client handle for a unix connection. Obsoleted by clnt_vc_create()
 */
CLIENT *
clntunix_create(raddr, prog, vers, sockp, sendsz, recvsz)
	struct sockaddr_un *raddr;
	u_long prog;
	u_long vers;
#ifndef __REACTOS__
	SOCKET *sockp;
#else
    int *sockp;
#endif
	u_int sendsz;
	u_int recvsz;
{
	struct netbuf *svcaddr;
	struct netconfig *nconf;
	CLIENT *cl;
	int len;

	cl = NULL;
	nconf = NULL;
	svcaddr = NULL;
	if (((svcaddr = malloc(sizeof(struct netbuf))) == NULL ) ||
	    ((svcaddr->buf = malloc(sizeof(struct sockaddr_un))) == NULL)) {
		if (svcaddr != NULL)
			free(svcaddr);
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		return(cl);
	}
	if (*sockp == SOCKET_ERROR) {
		*sockp = socket(AF_UNIX, SOCK_STREAM, 0);
		len = SUN_LEN(raddr);
		if ((*sockp == INVALID_SOCKET) || (connect(*sockp,
		    (struct sockaddr *)raddr, len) == SOCKET_ERROR)) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
			if (*sockp != INVALID_SOCKET)
				(void)closesocket(*sockp);
			goto done;
		}
	}
	svcaddr->buf = raddr;
	svcaddr->len = sizeof(raddr);
	svcaddr->maxlen = sizeof (struct sockaddr_un);
	cl = clnt_vc_create(*sockp, svcaddr, prog,
	    vers, sendsz, recvsz, NULL, NULL, NULL);
done:
	free(svcaddr->buf);
	free(svcaddr);
	return(cl);
}

/*
 * Creates, registers, and returns a (rpc) unix based transporter.
 * Obsoleted by svc_vc_create().
 */
SVCXPRT *
svcunix_create(sock, sendsize, recvsize, path)
#ifndef __REACTOS__
	SOCKET sock;
#else
    int sock;
#endif
	u_int sendsize;
	u_int recvsize;
	char *path;
{
	struct netconfig *nconf;
	void *localhandle;
	struct sockaddr_un sun;
	struct sockaddr *sa;
	struct t_bind taddr;
	SVCXPRT *xprt;
	int addrlen;

	xprt = (SVCXPRT *)NULL;
	localhandle = setnetconfig();
	while ((nconf = getnetconfig(localhandle)) != NULL) {
		if (nconf->nc_protofmly != NULL &&
		    strcmp(nconf->nc_protofmly, NC_LOOPBACK) == 0)
			break;
	}
	if (nconf == NULL)
		return(xprt);

	if ((sock = __rpc_nconf2fd(nconf)) == SOCKET_ERROR)
		goto done;

	memset(&sun, 0, sizeof sun);
	sun.sun_family = AF_UNIX;
	strncpy(sun.sun_path, path, sizeof(sun.sun_path));
	addrlen = sizeof(struct sockaddr_un);
	sa = (struct sockaddr *)&sun;

	if (bind(sock, sa, addrlen) == SOCKET_ERROR)
		goto done;

	taddr.addr.len = taddr.addr.maxlen = addrlen;
	taddr.addr.buf = malloc(addrlen);
	if (taddr.addr.buf == NULL)
		goto done;
	memcpy(taddr.addr.buf, sa, addrlen);

	if (nconf->nc_semantics != NC_TPI_CLTS) {
		if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
			free(taddr.addr.buf);
			goto done;
		}
	}

	xprt = (SVCXPRT *)svc_tli_create(sock, nconf, &taddr, sendsize, recvsize);

done:
	endnetconfig(localhandle);
	return(xprt);
}

/*
 * Like svunix_create(), except the routine takes any *open* UNIX file
 * descriptor as its first input. Obsoleted by svc_fd_create();
 */
SVCXPRT *
svcunixfd_create(fd, sendsize, recvsize)
#ifndef __REACTOS__
	SOCKET fd;
#else
    int fd;
#endif
	u_int sendsize;
	u_int recvsize;
{
 	return (svc_fd_create(fd, sendsize, recvsize));
}

#endif /* PORTMAP */
