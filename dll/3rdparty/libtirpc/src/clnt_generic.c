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
 * Copyright (c) 1986-1996,1998 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#include <wintirpc.h>
//#include <pthread.h>
#include <reentrant.h>
#include <sys/types.h>
//#include <sys/fcntl.h>
#include <fcntl.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
//#include <netdb.h>
//#include <syslog.h>
#include <rpc/rpc.h>
#include <rpc/nettype.h>
//#include <unistd.h>
#include "rpc_com.h"

extern bool_t __rpc_is_local_host(const char *);
#if 0	/* WINDOWS */
int __rpc_raise_fd(int);
#endif

#ifndef NETIDLEN
#define	NETIDLEN 32
#endif

/*
 * Generic client creation with version checking the value of
 * vers_out is set to the highest server supported value
 * vers_low <= vers_out <= vers_high  AND an error results
 * if this can not be done.
 *
 * It calls clnt_create_vers_timed() with a NULL value for the timeout
 * pointer, which indicates that the default timeout should be used.
 */
CLIENT *
clnt_create_vers(const char *hostname, const rpcprog_t prog, rpcvers_t *vers_out,
	const rpcvers_t vers_low, const rpcvers_t vers_high, const char *nettype)
{

	return (clnt_create_vers_timed(hostname, prog, vers_out, vers_low,
				vers_high, nettype, NULL));
}

/*
 * This the routine has the same definition as clnt_create_vers(),
 * except it takes an additional timeout parameter - a pointer to
 * a timeval structure.  A NULL value for the pointer indicates
 * that the default timeout value should be used.
 */
CLIENT *
clnt_create_vers_timed(const char *hostname, const rpcprog_t prog,
    rpcvers_t *vers_out, const rpcvers_t vers_low_in, const rpcvers_t vers_high_in,
    const char *nettype, const struct timeval *tp)
{
	CLIENT *clnt;
	struct timeval to;
	enum clnt_stat rpc_stat;
	struct rpc_err rpcerr;
	rpcvers_t vers_high = vers_high_in;
	rpcvers_t vers_low = vers_low_in;

	clnt = clnt_create_timed(hostname, prog, vers_high, nettype, tp);
	if (clnt == NULL) {
		return (NULL);
	}
	to.tv_sec = 10;
	to.tv_usec = 0;
	rpc_stat = clnt_call(clnt, NULLPROC, (xdrproc_t)xdr_void,
			(char *)NULL, (xdrproc_t)xdr_void, (char *)NULL, to);
	if (rpc_stat == RPC_SUCCESS) {
		*vers_out = vers_high;
		return (clnt);
	}
	while (rpc_stat == RPC_PROGVERSMISMATCH && vers_high > vers_low) {
		unsigned int minvers, maxvers;

		clnt_geterr(clnt, &rpcerr);
		minvers = rpcerr.re_vers.low;
		maxvers = rpcerr.re_vers.high;
		if (maxvers < vers_high)
			vers_high = maxvers;
		else
			vers_high--;
		if (minvers > vers_low)
			vers_low = minvers;
		if (vers_low > vers_high) {
			goto error;
		}
		CLNT_CONTROL(clnt, CLSET_VERS, (char *)&vers_high);
		rpc_stat = clnt_call(clnt, NULLPROC, (xdrproc_t)xdr_void,
				(char *)NULL, (xdrproc_t)xdr_void,
				(char *)NULL, to);
		if (rpc_stat == RPC_SUCCESS) {
			*vers_out = vers_high;
			return (clnt);
		}
	}
	clnt_geterr(clnt, &rpcerr);

error:
	rpc_createerr.cf_stat = rpc_stat;
	rpc_createerr.cf_error = rpcerr;
	clnt_destroy(clnt);
	return (NULL);
}

/*
 * Top level client creation routine.
 * Generic client creation: takes (servers name, program-number, nettype) and
 * returns client handle. Default options are set, which the user can
 * change using the rpc equivalent of _ioctl()'s.
 *
 * It tries for all the netids in that particular class of netid until
 * it succeeds.
 * XXX The error message in the case of failure will be the one
 * pertaining to the last create error.
 *
 * It calls clnt_create_timed() with the default timeout.
 */
CLIENT *
clnt_create(const char *hostname, const rpcprog_t prog, const rpcvers_t vers,
    const char *nettype)
{

	return (clnt_create_timed(hostname, prog, vers, nettype, NULL));
}

/*
 * This the routine has the same definition as clnt_create(),
 * except it takes an additional timeout parameter - a pointer to
 * a timeval structure.  A NULL value for the pointer indicates
 * that the default timeout value should be used.
 *
 * This function calls clnt_tp_create_timed().
 */
CLIENT *
clnt_create_timed(const char *hostname, const rpcprog_t prog, const rpcvers_t vers,
    const char *netclass, const struct timeval *tp)
{
	struct netconfig *nconf;
	CLIENT *clnt = NULL;
	void *handle;
	enum clnt_stat	save_cf_stat = RPC_SUCCESS;
	struct rpc_err	save_cf_error;
	char nettype_array[NETIDLEN];
	char *nettype = &nettype_array[0];

	if (netclass == NULL)
		nettype = NULL;
	else {
		size_t len = strlen(netclass);
		if (len >= sizeof (nettype_array)) {
			rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			return (NULL);
		}
		strcpy(nettype, netclass);
	}

	if ((handle = __rpc_setconf((char *)nettype)) == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		return (NULL);
	}
	rpc_createerr.cf_stat = RPC_SUCCESS;
	while (clnt == NULL) {
		if ((nconf = __rpc_getconf(handle)) == NULL) {
			if (rpc_createerr.cf_stat == RPC_SUCCESS)
				rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			break;
		}
#ifdef CLNT_DEBUG
		printf("trying netid %s\n", nconf->nc_netid);
#endif
		clnt = clnt_tp_create_timed(hostname, prog, vers, nconf, tp);
		if (clnt)
			break;
		else {
			/*
			 *	Since we didn't get a name-to-address
			 *	translation failure here, we remember
			 *	this particular error.  The object of
			 *	this is to enable us to return to the
			 *	caller a more-specific error than the
			 *	unhelpful ``Name to address translation
			 *	failed'' which might well occur if we
			 *	merely returned the last error (because
			 *	the local loopbacks are typically the
			 *	last ones in /etc/netconfig and the most
			 *	likely to be unable to translate a host
			 *	name).  We also check for a more
			 *	meaningful error than ``unknown host
			 *	name'' for the same reasons.
			 */
			if (rpc_createerr.cf_stat != RPC_N2AXLATEFAILURE &&
			    rpc_createerr.cf_stat != RPC_UNKNOWNHOST) {
				save_cf_stat = rpc_createerr.cf_stat;
				save_cf_error = rpc_createerr.cf_error;
			}
		}
	}

	/*
	 *	Attempt to return an error more specific than ``Name to address
	 *	translation failed'' or ``unknown host name''
	 */
	if ((rpc_createerr.cf_stat == RPC_N2AXLATEFAILURE ||
	     rpc_createerr.cf_stat == RPC_UNKNOWNHOST) &&
	    (save_cf_stat != RPC_SUCCESS)) {
		rpc_createerr.cf_stat = save_cf_stat;
		rpc_createerr.cf_error = save_cf_error;
	}
	__rpc_endconf(handle);
	return (clnt);
}

/*
 * Generic client creation: takes (servers name, program-number, netconf) and
 * returns client handle. Default options are set, which the user can
 * change using the rpc equivalent of _ioctl()'s : clnt_control()
 * It finds out the server address from rpcbind and calls clnt_tli_create().
 *
 * It calls clnt_tp_create_timed() with the default timeout.
 */
CLIENT *
clnt_tp_create(const char *hostname, const rpcprog_t prog, const rpcvers_t vers,
    const struct netconfig *nconf)
{
	return (clnt_tp_create_timed(hostname, prog, vers, nconf, NULL));
}

/*
 * This has the same definition as clnt_tp_create(), except it
 * takes an additional parameter - a pointer to a timeval structure.
 * A NULL value for the timeout pointer indicates that the default
 * value for the timeout should be used.
 */
CLIENT *
clnt_tp_create_timed(const char *hostname, const rpcprog_t prog, const rpcvers_t vers,
    const struct netconfig *nconf, const struct timeval *tp)
{
	struct netbuf *svcaddr;			/* servers address */
	CLIENT *cl = NULL;			/* client handle */

	if (nconf == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		return (NULL);
	}

	/*
	 * Get the address of the server
	 */
	if ((svcaddr = __rpcb_findaddr_timed(prog, vers,
			(struct netconfig *)nconf, (char *)hostname,
			&cl, (struct timeval *)tp)) == NULL) {
		/* appropriate error number is set by rpcbind libraries */
		return (NULL);
	}
	if (cl == NULL) {
		cl = clnt_tli_create(RPC_ANYFD, nconf, svcaddr,
					prog, vers, 0, 0, NULL, NULL, NULL);
	} else {
		/* Reuse the CLIENT handle and change the appropriate fields */
		if (CLNT_CONTROL(cl, CLSET_SVC_ADDR, (void *)svcaddr) == TRUE) {
			if (cl->cl_netid == NULL)
				cl->cl_netid = strdup(nconf->nc_netid);
			if (cl->cl_tp == NULL)
				cl->cl_tp = strdup(nconf->nc_device);
			(void) CLNT_CONTROL(cl, CLSET_PROG, (void *)&prog);
			(void) CLNT_CONTROL(cl, CLSET_VERS, (void *)&vers);
		} else {
			CLNT_DESTROY(cl);
			cl = clnt_tli_create(RPC_ANYFD, nconf, svcaddr,
					prog, vers, 0, 0, NULL, NULL, NULL);
		}
	}
	free(svcaddr->buf);
	free(svcaddr);
	return (cl);
}

/*
 * Generic client creation:  returns client handle.
 * Default options are set, which the user can
 * change using the rpc equivalent of _ioctl()'s : clnt_control().
 * If fd is RPC_ANYFD, it will be opened using nconf.
 * It will be bound if not so.
 * If sizes are 0; appropriate defaults will be chosen.
 */
CLIENT *
clnt_tli_create(const SOCKET fd_in, const struct netconfig *nconf,
	struct netbuf *svcaddr, const rpcprog_t prog, const rpcvers_t vers,
	const uint sendsz, const uint recvsz,
    int (*callback_xdr)(void *, void *),
    int (*callback_function)(void *, void *, void **), 
    void *callback_args)
{
	CLIENT *cl;			/* client handle */
	bool_t madefd = FALSE;		/* whether fd opened here */
	long servtype;
	BOOL one = TRUE;
	struct __rpc_sockinfo si;
	extern int __rpc_minfd;
	SOCKET fd = fd_in;

	if (fd == RPC_ANYFD) {
		if (nconf == NULL) {
			rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			return (NULL);
		}

		fd = __rpc_nconf2fd(nconf);

		if (fd == INVALID_SOCKET)
			goto err;
#if 0
		if (fd < __rpc_minfd)
			fd = __rpc_raise_fd(fd);
#endif
		madefd = TRUE;
		servtype = nconf->nc_semantics;
		bindresvport(fd, NULL);
		if (!__rpc_fd2sockinfo(fd, &si))
			goto err;
	} else {
		if (!__rpc_fd2sockinfo(fd, &si))
			goto err;
		servtype = __rpc_socktype2seman(si.si_socktype);
		if (servtype == -1) {
			rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			return (NULL);
		}
	}

	if (si.si_af != ((struct sockaddr *)svcaddr->buf)->sa_family) {
		rpc_createerr.cf_stat = RPC_UNKNOWNHOST;	/* XXX */
		goto err1;
	}

	switch (servtype) {
	case NC_TPI_COTS:
		cl = clnt_vc_create(fd, svcaddr, prog, vers, sendsz, recvsz, 
            callback_xdr, callback_function, callback_args);
		break;
	case NC_TPI_COTS_ORD:
		if (nconf &&
		    ((strcmp(nconf->nc_protofmly, "inet") == 0) ||
		     (strcmp(nconf->nc_protofmly, "inet6") == 0))) {
			setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&one,
			    sizeof (one));
		}
		cl = clnt_vc_create(fd, svcaddr, prog, vers, sendsz, recvsz,
            callback_xdr, callback_function, callback_args);
		break;
	case NC_TPI_CLTS:
		cl = clnt_dg_create(fd, svcaddr, prog, vers, sendsz, recvsz);
		break;
	default:
		goto err;
	}

	if (cl == NULL)
		goto err1; /* borrow errors from clnt_dg/vc creates */
	if (nconf) {
		cl->cl_netid = strdup(nconf->nc_netid);
		cl->cl_tp = strdup(nconf->nc_device);
	} else {
		cl->cl_netid = "";
		cl->cl_tp = "";
	}
	if (madefd) {
		(void) CLNT_CONTROL(cl, CLSET_FD_CLOSE, NULL);
/*		(void) CLNT_CONTROL(cl, CLSET_POP_TIMOD, NULL);  */
	};

	return (cl);

err:
	rpc_createerr.cf_stat = RPC_SYSTEMERROR;
	rpc_createerr.cf_error.re_errno = errno;
err1:	if (madefd)
		(void)closesocket(fd);
	return (NULL);
}

#if 0	/* WINDOWS */
/*
 *  To avoid conflicts with the "magic" file descriptors (0, 1, and 2),
 *  we try to not use them.  The __rpc_raise_fd() routine will dup
 *  a descriptor to a higher value.  If we fail to do it, we continue
 *  to use the old one (and hope for the best).
 */
int __rpc_minfd = 3;

int
__rpc_raise_fd(int fd)
{
	int nfd;

	if (fd >= __rpc_minfd)
		return (fd);

	if ((nfd = fcntl(fd, F_DUPFD, __rpc_minfd)) == -1)
		return (fd);

	if (fsync(nfd) == -1) {
		closesocket(nfd);
		return (fd);
	}

	if (closesocket(fd) == -1) {
		/* this is okay, we will syslog an error, then use the new fd */
		(void) syslog(LOG_ERR,
			"could not close() fd %d; mem & fd leak", fd);
	}

	return (nfd);
}
#endif
