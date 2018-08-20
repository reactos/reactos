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
 * clnt_tcp.c, Implements a TCP/IP based, client side RPC.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * TCP based RPC supports 'batched calls'.
 * A sequence of calls may be batched-up in a send buffer.  The rpc call
 * return immediately to the client even though the call was not necessarily
 * sent.  The batching occurs if the results' xdr routine is NULL (0) AND
 * the rpc timeout value is zero (see clnt.h, rpc).
 *
 * Clients should NOT casually batch calls that in fact return results; that is,
 * the server side should be aware that a call is batched and not produce any
 * return message.  Batched calls that produce many result messages can
 * deadlock (netlock) the client and the server....
 *
 * Now go hang yourself.
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
//#include <sys/poll.h>
//#include <sys/syslog.h>
//#include <sys/un.h>
//#include <sys/uio.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>
#include <assert.h>
//#include <err.h>
#include <errno.h>
//#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <signal.h>
#include <time.h>

#include <rpc/rpc.h>
#include "rpc_com.h"

#define MCALL_MSG_SIZE 24

#define CMGROUP_MAX    16
#define SCM_CREDS      0x03            /* process creds (struct cmsgcred) */

/*
 * Credentials structure, used to verify the identity of a peer
 * process that has sent us a message. This is allocated by the
 * peer process but filled in by the kernel. This prevents the
 * peer from lying about its identity. (Note that cmcred_groups[0]
 * is the effective GID.)
 */
struct cmsgcred {
        pid_t   cmcred_pid;             /* PID of sending process */
        uid_t   cmcred_uid;             /* real UID of sending process */
        uid_t   cmcred_euid;            /* effective UID of sending process */
        gid_t   cmcred_gid;             /* real GID of sending process */
        short   cmcred_ngroups;         /* number or groups */
        gid_t   cmcred_groups[CMGROUP_MAX];     /* groups */
};

struct cmessage {
        struct cmsghdr cmsg;
        struct cmsgcred cmcred;
};

static enum clnt_stat clnt_vc_call(CLIENT *, rpcproc_t, xdrproc_t, void *,
    xdrproc_t, void *, struct timeval);
static void clnt_vc_geterr(CLIENT *, struct rpc_err *);
static bool_t clnt_vc_freeres(CLIENT *, xdrproc_t, void *);
static void clnt_vc_abort(CLIENT *);
static bool_t clnt_vc_control(CLIENT *, u_int, void *);
static void clnt_vc_destroy(CLIENT *);
static struct clnt_ops *clnt_vc_ops(void);
static bool_t time_not_ok(struct timeval *);
static int read_vc(void *, void *, int);
static int write_vc(void *, void *, int);

struct ct_data {
	int		ct_fd;		/* connection's fd */
	bool_t		ct_closeit;	/* close it on destroy */
	struct timeval	ct_wait;	/* wait interval in milliseconds */
	bool_t          ct_waitset;	/* wait set by clnt_control? */
	struct netbuf	ct_addr;	/* remote addr */
	struct rpc_err	ct_error;
	union {
		char	ct_mcallc[MCALL_MSG_SIZE];	/* marshalled callmsg */
		u_int32_t ct_mcalli;
	} ct_u;
	u_int		ct_mpos;	/* pos after marshal */
	XDR		ct_xdrs;	/* XDR stream */
    struct rpc_msg reply_msg;
    bool_t use_stored_reply_msg;
};

/*
 *      This machinery implements per-fd locks for MT-safety.  It is not
 *      sufficient to do per-CLIENT handle locks for MT-safety because a
 *      user may create more than one CLIENT handle with the same fd behind
 *      it.  Therfore, we allocate an array of flags (vc_fd_locks), protected
 *      by the clnt_fd_lock mutex, and an array (vc_cv) of condition variables
 *      similarly protected.  Vc_fd_lock[fd] == 1 => a call is active on some
 *      CLIENT handle created for that fd.
 *      The current implementation holds locks across the entire RPC and reply.
 *      Yes, this is silly, and as soon as this code is proven to work, this
 *      should be the first thing fixed.  One step at a time.
 */
static int      *vc_fd_locks;
extern mutex_t  clnt_fd_lock;
static cond_t   *vc_cv;
#ifndef _WIN32
#define release_fd_lock(fd, mask) {	\
	mutex_lock(&clnt_fd_lock);	\
	vc_fd_locks[fd] = 0;		\
	mutex_unlock(&clnt_fd_lock);	\
	thr_sigsetmask(SIG_SETMASK, &(mask), (sigset_t *) NULL);	\
	cond_signal(&vc_cv[fd]);	\
}
#else
/* XXX Need Windows signal/event stuff XXX */
#define release_fd_lock(fd, mask) {	\
	mutex_lock(&clnt_fd_lock);	\
	vc_fd_locks[WINSOCK_HANDLE_HASH(fd)] = 0;		\
	mutex_unlock(&clnt_fd_lock);	\
	\
	cond_broadcast(&vc_cv[WINSOCK_HANDLE_HASH(fd)]);	\
}
#endif

#define acquire_fd_lock(fd) { \
	mutex_lock(&clnt_fd_lock); \
	while (vc_fd_locks[WINSOCK_HANDLE_HASH(fd)] && \
            vc_fd_locks[WINSOCK_HANDLE_HASH(fd)] != GetCurrentThreadId()) \
		cond_wait(&vc_cv[WINSOCK_HANDLE_HASH(fd)], &clnt_fd_lock); \
	vc_fd_locks[WINSOCK_HANDLE_HASH(fd)] = GetCurrentThreadId(); \
	mutex_unlock(&clnt_fd_lock); \
}

static const char clnt_vc_errstr[] = "%s : %s";
static const char clnt_vc_str[] = "clnt_vc_create";
static const char clnt_read_vc_str[] = "read_vc";
static const char __no_mem_str[] = "out of memory";

/* callback thread */
#define CALLBACK_TIMEOUT 5000
#define	RQCRED_SIZE	400	/* this size is excessive */
static unsigned int WINAPI clnt_cb_thread(void *args) 
{
    int status = NO_ERROR;
    CLIENT *cl = (CLIENT *)args;
	struct ct_data *ct = (struct ct_data *) cl->cl_private;
	XDR *xdrs = &(ct->ct_xdrs);
    long saved_timeout_sec = ct->ct_wait.tv_sec;
    long saved_timeout_usec = ct->ct_wait.tv_usec;
    struct rpc_msg reply_msg;    
    char cred_area[2 * MAX_AUTH_BYTES + RQCRED_SIZE];

    fprintf(stderr/*stdout*/, "%04x: Creating callback thread\n", GetCurrentThreadId());
    while(1) {
        cb_req header;
        void *res = NULL;
        mutex_lock(&clnt_fd_lock);
	    while (vc_fd_locks[WINSOCK_HANDLE_HASH(ct->ct_fd)] || 
                !ct->use_stored_reply_msg ||
                (ct->use_stored_reply_msg && ct->reply_msg.rm_direction != CALL)) {
            if (cl->shutdown)
                break;
		    if (!cond_wait_timed(&vc_cv[WINSOCK_HANDLE_HASH(ct->ct_fd)], &clnt_fd_lock, 
                CALLBACK_TIMEOUT))
                if (!vc_fd_locks[WINSOCK_HANDLE_HASH(ct->ct_fd)])
                    break;
        }
	    vc_fd_locks[WINSOCK_HANDLE_HASH(ct->ct_fd)] = GetCurrentThreadId();
	    mutex_unlock(&clnt_fd_lock);

        if (cl->shutdown) {
            fprintf(stdout, "%04x: callback received shutdown signal\n", GetCurrentThreadId());
            release_fd_lock(ct->ct_fd, mask);
            goto out;
        }

        saved_timeout_sec = ct->ct_wait.tv_sec;
        saved_timeout_usec = ct->ct_wait.tv_usec;
        xdrs->x_op = XDR_DECODE;
        if (ct->use_stored_reply_msg && ct->reply_msg.rm_direction == CALL) {
            goto process_rpc_call;
        } else if (!ct->use_stored_reply_msg) {
            ct->ct_wait.tv_sec = ct->ct_wait.tv_usec = 0;
            __xdrrec_setnonblock(xdrs, 0);
		    if (!xdrrec_skiprecord(xdrs))
                goto skip_process;
            if (!xdr_getxiddir(xdrs, &ct->reply_msg)) {
                goto skip_process;
            }
            if (ct->reply_msg.rm_direction == CALL) {
                goto process_rpc_call;
            } else {
                if (ct->reply_msg.rm_direction == REPLY)
                    ct->use_stored_reply_msg = TRUE;
                goto skip_setlastfrag;
            }
        } else {
            goto skip_setlastfrag;
        }
process_rpc_call:
        //call to get call headers
        ct->use_stored_reply_msg = FALSE;
        ct->reply_msg.rm_call.cb_cred.oa_base = cred_area;
        ct->reply_msg.rm_call.cb_verf.oa_base = &(cred_area[MAX_AUTH_BYTES]);
        if (!xdr_getcallbody(xdrs, &ct->reply_msg)) {
            fprintf(stderr, "%04x: xdr_getcallbody failed\n", GetCurrentThreadId());            
            goto skip_process;
        } else 
            fprintf(stdout, "%04x: callbody: rpcvers %d cb_prog %d cb_vers %d cb_proc %d\n", 
                GetCurrentThreadId(), 
                ct->reply_msg.rm_call.cb_rpcvers, ct->reply_msg.rm_call.cb_prog,
                ct->reply_msg.rm_call.cb_vers, ct->reply_msg.rm_call.cb_proc);
        header.rq_prog = ct->reply_msg.rm_call.cb_prog;
        header.rq_vers = ct->reply_msg.rm_call.cb_vers;
        header.rq_proc = ct->reply_msg.rm_call.cb_proc;
        header.xdr = xdrs;
        status = (*cl->cb_fn)(cl->cb_args, &header, &res);
        if (status) {
            fprintf(stderr, "%04x: callback function failed with %d\n", status);
        }
        
        xdrs->x_op = XDR_ENCODE;
        __xdrrec_setblock(xdrs);
        reply_msg.rm_xid = ct->reply_msg.rm_xid;
        fprintf(stdout, "%04x: cb: replying to xid %d\n", GetCurrentThreadId(), 
            ct->reply_msg.rm_xid);
        ct->reply_msg.rm_xid = 0;
        reply_msg.rm_direction = REPLY;
        reply_msg.rm_reply.rp_stat = MSG_ACCEPTED;
        reply_msg.acpted_rply.ar_verf = _null_auth;
        reply_msg.acpted_rply.ar_stat = status;
        reply_msg.acpted_rply.ar_results.where = NULL;
        reply_msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
        xdr_replymsg(xdrs, &reply_msg);
        if (!status) {
            (*cl->cb_xdr)(xdrs, res); /* encode the results */
            xdrs->x_op = XDR_FREE;
            (*cl->cb_xdr)(xdrs, res); /* free the results */
        }
        if (! xdrrec_endofrecord(xdrs, 1)) {
            fprintf(stderr, "%04x: failed to send REPLY\n", GetCurrentThreadId());
        }
skip_process:
        ct->reply_msg.rm_direction = -1;
        xdrrec_setlastfrag(xdrs);
skip_setlastfrag:
        ct->ct_wait.tv_sec = saved_timeout_sec;
        ct->ct_wait.tv_usec = saved_timeout_usec;
        release_fd_lock(ct->ct_fd, mask);
    }
out:
    return status;
}
/*
 * Create a client handle for a connection.
 * Default options are set, which the user can change using clnt_control()'s.
 * The rpc/vc package does buffering similar to stdio, so the client
 * must pick send and receive buffer sizes, 0 => use the default.
 * NB: fd is copied into a private area.
 * NB: The rpch->cl_auth is set null authentication. Caller may wish to
 * set this something more useful.
 *
 * fd should be an open socket
 */
CLIENT *
clnt_vc_create(fd, raddr, prog, vers, sendsz, recvsz, cb_xdr, cb_fn, cb_args)
	int fd;				/* open file descriptor */
	const struct netbuf *raddr;	/* servers address */
	const rpcprog_t prog;			/* program number */
	const rpcvers_t vers;			/* version number */
	u_int sendsz;			/* buffer recv size */
	u_int recvsz;			/* buffer send size */
    int (*cb_xdr)(void *, void *); /* if not NULL, point to function to xdr CB args */
    int (*cb_fn)(void *, void *, void **);   /* if not NULL, pointer to function to handle RPC_CALLs */
    void *cb_args;          /* if not NULL, pointer to pass into cb_fn */
{
	CLIENT *cl;			/* client handle */
	struct ct_data *ct = NULL;	/* client handle */
	struct timeval now;
	struct rpc_msg call_msg;
	static u_int32_t disrupt;
#ifndef _WIN32
	sigset_t mask;
	sigset_t newmask;
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif
	struct sockaddr_storage ss;
	socklen_t slen;
	struct __rpc_sockinfo si;

	if (disrupt == 0)
		disrupt = PtrToUlong(raddr);

	cl = (CLIENT *)mem_alloc(sizeof (*cl));
	ct = (struct ct_data *)mem_alloc(sizeof (*ct));
	if ((cl == (CLIENT *)NULL) || (ct == (struct ct_data *)NULL)) {
//		(void) syslog(LOG_ERR, clnt_vc_errstr,
//		    clnt_vc_str, __no_mem_str);
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		goto err;
	}
	ct->ct_addr.buf = NULL;
#ifndef _WIN32
	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif
	mutex_lock(&clnt_fd_lock);
	if (vc_fd_locks == (int *) NULL) {
		int cv_allocsz, fd_allocsz;
		int dtbsize = __rpc_dtbsize();

		fd_allocsz = dtbsize * sizeof (int);
		vc_fd_locks = (int *) mem_alloc(fd_allocsz);
		if (vc_fd_locks == (int *) NULL) {
			mutex_unlock(&clnt_fd_lock);
//			thr_sigsetmask(SIG_SETMASK, &(mask), NULL);
			goto err;
		} else
			memset(vc_fd_locks, 0, fd_allocsz);

		assert(vc_cv == (cond_t *) NULL);
		cv_allocsz = dtbsize * sizeof (cond_t);
		vc_cv = (cond_t *) mem_alloc(cv_allocsz);
		if (vc_cv == (cond_t *) NULL) {
			mem_free(vc_fd_locks, fd_allocsz);
			vc_fd_locks = (int *) NULL;
			mutex_unlock(&clnt_fd_lock);
//			thr_sigsetmask(SIG_SETMASK, &(mask), NULL);
			goto err;
		} else {
			int i;

			for (i = 0; i < dtbsize; i++)
				cond_init(&vc_cv[i], 0, (void *) 0);
		}
	} else
		assert(vc_cv != (cond_t *) NULL);

	/*
	 * XXX - fvdl connecting while holding a mutex?
	 */
	slen = sizeof ss;
	if (getpeername(fd, (struct sockaddr *)&ss, &slen) == SOCKET_ERROR) {
		errno = WSAGetLastError();
		if (errno != WSAENOTCONN) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
			mutex_unlock(&clnt_fd_lock);
//			thr_sigsetmask(SIG_SETMASK, &(mask), NULL);
			goto err;
		}
		if (connect(fd, (struct sockaddr *)raddr->buf, raddr->len) == SOCKET_ERROR){
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = WSAGetLastError();
			mutex_unlock(&clnt_fd_lock);
//			thr_sigsetmask(SIG_SETMASK, &(mask), NULL);
			goto err;
		}
	}
	mutex_unlock(&clnt_fd_lock);
	if (!__rpc_fd2sockinfo(fd, &si))
		goto err;
//	thr_sigsetmask(SIG_SETMASK, &(mask), NULL);

	ct->ct_closeit = FALSE;

	/*
	 * Set up private data struct
	 */
	ct->ct_fd = fd;
	ct->ct_wait.tv_usec = 0;
	ct->ct_waitset = FALSE;
	ct->ct_addr.buf = malloc(raddr->maxlen);
	if (ct->ct_addr.buf == NULL)
		goto err;
	memcpy(ct->ct_addr.buf, raddr->buf, raddr->len);
	ct->ct_addr.len = raddr->len;
	ct->ct_addr.maxlen = raddr->maxlen;
    ct->use_stored_reply_msg = FALSE;

	/*
	 * Initialize call message
	 */
	(void)gettimeofday(&now, NULL);
	call_msg.rm_xid = ((u_int32_t)++disrupt) ^ __RPC_GETXID(&now);
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = (u_int32_t)prog;
	call_msg.rm_call.cb_vers = (u_int32_t)vers;

	/*
	 * pre-serialize the static part of the call msg and stash it away
	 */
	xdrmem_create(&(ct->ct_xdrs), ct->ct_u.ct_mcallc, MCALL_MSG_SIZE,
	    XDR_ENCODE);
	if (! xdr_callhdr(&(ct->ct_xdrs), &call_msg)) {
		if (ct->ct_closeit) {
			(void)closesocket(fd);
		}
		goto err;
	}
	ct->ct_mpos = XDR_GETPOS(&(ct->ct_xdrs));
	XDR_DESTROY(&(ct->ct_xdrs));

	/*
	 * Create a client handle which uses xdrrec for serialization
	 * and authnone for authentication.
	 */
	cl->cl_ops = clnt_vc_ops();
	cl->cl_private = ct;
	cl->cl_auth = authnone_create();
	sendsz = __rpc_get_t_size(si.si_af, si.si_proto, (int)sendsz);
	recvsz = __rpc_get_t_size(si.si_af, si.si_proto, (int)recvsz);
	xdrrec_create(&(ct->ct_xdrs), sendsz, recvsz,
	    cl->cl_private, read_vc, write_vc);

    if (cb_xdr && cb_fn && cb_args) {
        cl->cb_xdr = cb_xdr;
        cl->cb_fn = cb_fn;
        cl->cb_args = cb_args;
        cl->cb_thread = (HANDLE)_beginthreadex(NULL,
            0, clnt_cb_thread, cl, 0, NULL);
        if (cl->cb_thread == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "_beginthreadex failed %d\n", GetLastError());
            goto err;
        } else
            fprintf(stdout, "%04x: started the callback thread %04zx\n", 
                GetCurrentThreadId(), (ULONG_PTR)cl->cb_thread);
    } else
        cl->cb_thread = INVALID_HANDLE_VALUE;
	return (cl);

err:
	if (cl) {
		if (ct) {
			if (ct->ct_addr.len)
				mem_free(ct->ct_addr.buf, ct->ct_addr.len);
			mem_free(ct, sizeof (struct ct_data));
		}
		if (cl)
			mem_free(cl, sizeof (CLIENT));
	}
	return ((CLIENT *)NULL);
}

static enum clnt_stat
clnt_vc_call(cl, proc, xdr_args, args_ptr, xdr_results, results_ptr, timeout)
	CLIENT *cl;
	rpcproc_t proc;
	xdrproc_t xdr_args;
	void *args_ptr;
	xdrproc_t xdr_results;
	void *results_ptr;
	struct timeval timeout;
{
	struct ct_data *ct = (struct ct_data *) cl->cl_private;
	XDR *xdrs = &(ct->ct_xdrs);
	u_int32_t x_id;
	u_int32_t *msg_x_id = &ct->ct_u.ct_mcalli;    /* yuk */
	bool_t shipnow;
	static int refreshes = 2;
    u_int seq = -1;
    time_t start_send, time_now;
#ifndef _WIN32
	sigset_t mask, newmask;
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif
    enum clnt_stat status;

	assert(cl != NULL);

#ifndef _WIN32
	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif

    acquire_fd_lock(ct->ct_fd);

	if (!ct->ct_waitset) {
		/* If time is not within limits, we ignore it. */
		if (time_not_ok(&timeout) == FALSE)
			ct->ct_wait = timeout;
	}

	shipnow =
	    (xdr_results == NULL && timeout.tv_sec == 0
	    && timeout.tv_usec == 0) ? FALSE : TRUE;

call_again:
    __xdrrec_setblock(xdrs);
	xdrs->x_op = XDR_ENCODE;
	ct->ct_error.re_status = RPC_SUCCESS;
	x_id = ntohl(--(*msg_x_id));

	if ((! XDR_PUTBYTES(xdrs, ct->ct_u.ct_mcallc, ct->ct_mpos)) ||
	    (! XDR_PUTINT32(xdrs, (int32_t *)&proc)) ||
	    (! AUTH_MARSHALL(cl->cl_auth, xdrs, &seq)) ||
	    (! AUTH_WRAP(cl->cl_auth, xdrs, xdr_args, args_ptr))) {
		if (ct->ct_error.re_status == RPC_SUCCESS)
			ct->ct_error.re_status = RPC_CANTENCODEARGS;
		(void)xdrrec_endofrecord(xdrs, TRUE);
        goto out;
	}

	if (! xdrrec_endofrecord(xdrs, shipnow)) {
        ct->ct_error.re_status = RPC_CANTSEND;
        goto out;
	}
	if (! shipnow) {
		release_fd_lock(ct->ct_fd, mask);
		return (RPC_SUCCESS);
	}

#ifdef NO_CB_4_KRB5P
    if (cl->cb_thread != INVALID_HANDLE_VALUE)
        release_fd_lock(ct->ct_fd, mask);
#endif
	/*
	 * Keep receiving until we get a valid transaction id
	 */

    time(&start_send);
	while (TRUE) {
#ifdef NO_CB_4_KRB5P
        if (cl->cb_thread != INVALID_HANDLE_VALUE) {
            mutex_lock(&clnt_fd_lock);
	        while ((vc_fd_locks[WINSOCK_HANDLE_HASH(ct->ct_fd)] && 
                    vc_fd_locks[WINSOCK_HANDLE_HASH(ct->ct_fd)] != GetCurrentThreadId()) || 
                    (ct->reply_msg.rm_xid && ct->reply_msg.rm_xid != x_id))
		        cond_wait(&vc_cv[WINSOCK_HANDLE_HASH(ct->ct_fd)], &clnt_fd_lock);
	        vc_fd_locks[WINSOCK_HANDLE_HASH(ct->ct_fd)] = GetCurrentThreadId();
	        mutex_unlock(&clnt_fd_lock);
        }
#endif
        __xdrrec_setnonblock(xdrs, 0);
        xdrs->x_op = XDR_DECODE;
		ct->reply_msg.acpted_rply.ar_verf = _null_auth;
		ct->reply_msg.acpted_rply.ar_results.where = NULL;
		ct->reply_msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
        if (!ct->use_stored_reply_msg) {
		    if (!xdrrec_skiprecord(xdrs)) {
                if (ct->ct_error.re_status != RPC_CANTRECV) {
                    time(&time_now);
                    if (time_now - start_send >= timeout.tv_sec) {
                        ct->ct_error.re_status = RPC_TIMEDOUT;
                        goto out;
                    }
#ifdef NO_CB_4_KRB5P
                    if (cl->cb_thread != INVALID_HANDLE_VALUE)  
#endif
			            release_fd_lock(ct->ct_fd, mask);
                    SwitchToThread();
			        continue;
                }
                goto out;
		    }
            if (!xdr_getxiddir(xdrs, &ct->reply_msg)) {
			    if (ct->ct_error.re_status == RPC_SUCCESS) {
#ifdef NO_CB_4_KRB5P
                    if (cl->cb_thread != INVALID_HANDLE_VALUE)  
#endif
                        release_fd_lock(ct->ct_fd, mask);
                    SwitchToThread();
                    continue;
                }
                goto out;
            } 

            if (ct->reply_msg.rm_direction != REPLY) {
                if (cl->cb_thread == INVALID_HANDLE_VALUE) {
                    ct->reply_msg.rm_xid = 0;
                } else {
                    ct->use_stored_reply_msg = TRUE;
                }
                release_fd_lock(ct->ct_fd, mask);
                SwitchToThread();
                continue;
            }
        }
		if (ct->reply_msg.rm_xid == x_id) {
            ct->use_stored_reply_msg = FALSE;
            ct->reply_msg.rm_xid = 0;
            if (!xdr_getreplyunion(xdrs, &ct->reply_msg))
                goto out;
			break;
        }
        else {
            time(&time_now);
            if (time_now - start_send >= timeout.tv_sec) {
                ct->ct_error.re_status = RPC_TIMEDOUT;
                goto out;
            }
            ct->use_stored_reply_msg = TRUE;
#ifdef NO_CB_4_KRB5P
            if (cl->cb_thread != INVALID_HANDLE_VALUE)  
#endif
                release_fd_lock(ct->ct_fd, mask);
            SwitchToThread();
        }
	}

	/*
	 * process header
	 */
	_seterr_reply(&ct->reply_msg, &(ct->ct_error));
	if (ct->ct_error.re_status == RPC_SUCCESS) {
		if (! AUTH_VALIDATE(cl->cl_auth,
		    &ct->reply_msg.acpted_rply.ar_verf, seq)) {
			ct->ct_error.re_status = RPC_AUTHERROR;
			ct->ct_error.re_why = AUTH_INVALIDRESP;
        }
        else if (! AUTH_UNWRAP(cl->cl_auth, xdrs, xdr_results, results_ptr, seq)) {
			if (ct->ct_error.re_status == RPC_SUCCESS)
				ct->ct_error.re_status = RPC_CANTDECODERES;
		}
		/* free verifier ... */
		if (ct->reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
			xdrs->x_op = XDR_FREE;
			(void)xdr_opaque_auth(xdrs,
			    &(ct->reply_msg.acpted_rply.ar_verf));
		}
	}  /* end successful completion */
	else {
		if (ct->reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
			xdrs->x_op = XDR_FREE;
			(void)xdr_opaque_auth(xdrs,
			    &(ct->reply_msg.acpted_rply.ar_verf));
		}
		/* maybe our credentials need to be refreshed ... */
		if (refreshes-- > 0 && AUTH_REFRESH(cl->cl_auth, &ct->reply_msg))
			goto call_again;
	}  /* end of unsuccessful completion */
    ct->reply_msg.rm_direction = -1;
out:
    status = ct->ct_error.re_status;
	release_fd_lock(ct->ct_fd, mask);
	return status;
}

static void
clnt_vc_geterr(cl, errp)
	CLIENT *cl;
	struct rpc_err *errp;
{
	struct ct_data *ct;

	assert(cl != NULL);
	assert(errp != NULL);

	ct = (struct ct_data *) cl->cl_private;
	*errp = ct->ct_error;
}

static bool_t
clnt_vc_freeres(cl, xdr_res, res_ptr)
	CLIENT *cl;
	xdrproc_t xdr_res;
	void *res_ptr;
{
	struct ct_data *ct;
	XDR *xdrs;
	bool_t dummy;
#ifndef _WIN32
	sigset_t mask;
	sigset_t newmask;
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif

	assert(cl != NULL);

	ct = (struct ct_data *)cl->cl_private;
	xdrs = &(ct->ct_xdrs);

#ifndef _WIN32
	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif
	mutex_lock(&clnt_fd_lock);
	while (vc_fd_locks[WINSOCK_HANDLE_HASH(ct->ct_fd)])
		cond_wait(&vc_cv[WINSOCK_HANDLE_HASH(ct->ct_fd)], &clnt_fd_lock);
	xdrs->x_op = XDR_FREE;
	dummy = (*xdr_res)(xdrs, res_ptr);
	mutex_unlock(&clnt_fd_lock);
//	thr_sigsetmask(SIG_SETMASK, &(mask), NULL);
	cond_signal(&vc_cv[WINSOCK_HANDLE_HASH(ct->ct_fd)]);

	return dummy;
}

/*ARGSUSED*/
static void
clnt_vc_abort(cl)
	CLIENT *cl;
{
}

static bool_t
clnt_vc_control(cl, request, info)
	CLIENT *cl;
	u_int request;
	void *info;
{
	struct ct_data *ct;
	void *infop = info;
#ifndef _WIN32
	sigset_t mask;
	sigset_t newmask;
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif

	assert(cl != NULL);

	ct = (struct ct_data *)cl->cl_private;

#ifndef _WIN32
	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif
    acquire_fd_lock(ct->ct_fd);

	switch (request) {
	case CLSET_FD_CLOSE:
		ct->ct_closeit = TRUE;
		release_fd_lock(ct->ct_fd, mask);
		return (TRUE);
	case CLSET_FD_NCLOSE:
		ct->ct_closeit = FALSE;
		release_fd_lock(ct->ct_fd, mask);
		return (TRUE);
	default:
		break;
	}

	/* for other requests which use info */
	if (info == NULL) {
		release_fd_lock(ct->ct_fd, mask);
		return (FALSE);
	}
	switch (request) {
	case CLSET_TIMEOUT:
		if (time_not_ok((struct timeval *)info)) {
			release_fd_lock(ct->ct_fd, mask);
			return (FALSE);
		}
		ct->ct_wait = *(struct timeval *)infop;
		ct->ct_waitset = TRUE;
		break;
	case CLGET_TIMEOUT:
		*(struct timeval *)infop = ct->ct_wait;
		break;
	case CLGET_SERVER_ADDR:
		(void) memcpy(info, ct->ct_addr.buf, (size_t)ct->ct_addr.len);
		break;
	case CLGET_FD:
		*(int *)info = ct->ct_fd;
		break;
	case CLGET_SVC_ADDR:
		/* The caller should not free this memory area */
		*(struct netbuf *)info = ct->ct_addr;
		break;
	case CLSET_SVC_ADDR:		/* set to new address */
		release_fd_lock(ct->ct_fd, mask);
		return (FALSE);
	case CLGET_XID:
		/*
		 * use the knowledge that xid is the
		 * first element in the call structure
		 * This will get the xid of the PREVIOUS call
		 */
		*(u_int32_t *)info =
		    ntohl(*(u_int32_t *)(void *)&ct->ct_u.ct_mcalli);
		break;
	case CLSET_XID:
		/* This will set the xid of the NEXT call */
		*(u_int32_t *)(void *)&ct->ct_u.ct_mcalli =
		    htonl(*((u_int32_t *)info) + 1);
		/* increment by 1 as clnt_vc_call() decrements once */
		break;
	case CLGET_VERS:
		/*
		 * This RELIES on the information that, in the call body,
		 * the version number field is the fifth field from the
		 * begining of the RPC header. MUST be changed if the
		 * call_struct is changed
		 */
		*(u_int32_t *)info =
		    ntohl(*(u_int32_t *)(void *)(ct->ct_u.ct_mcallc +
		    4 * BYTES_PER_XDR_UNIT));
		break;

	case CLSET_VERS:
		*(u_int32_t *)(void *)(ct->ct_u.ct_mcallc +
		    4 * BYTES_PER_XDR_UNIT) =
		    htonl(*(u_int32_t *)info);
		break;

	case CLGET_PROG:
		/*
		 * This RELIES on the information that, in the call body,
		 * the program number field is the fourth field from the
		 * begining of the RPC header. MUST be changed if the
		 * call_struct is changed
		 */
		*(u_int32_t *)info =
		    ntohl(*(u_int32_t *)(void *)(ct->ct_u.ct_mcallc +
		    3 * BYTES_PER_XDR_UNIT));
		break;

	case CLSET_PROG:
		*(u_int32_t *)(void *)(ct->ct_u.ct_mcallc +
		    3 * BYTES_PER_XDR_UNIT) =
		    htonl(*(u_int32_t *)info);
		break;

	default:
		release_fd_lock(ct->ct_fd, mask);
		return (FALSE);
	}
	release_fd_lock(ct->ct_fd, mask);
	return (TRUE);
}


static void
clnt_vc_destroy(cl)
	CLIENT *cl;
{
	struct ct_data *ct = (struct ct_data *) cl->cl_private;
	int ct_fd = ct->ct_fd;
#ifndef _WIN32
	sigset_t mask;
	sigset_t newmask;
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif

	assert(cl != NULL);

	ct = (struct ct_data *) cl->cl_private;

#ifndef _WIN32
	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif
	mutex_lock(&clnt_fd_lock);
	while (vc_fd_locks[WINSOCK_HANDLE_HASH(ct_fd)])
		cond_wait(&vc_cv[WINSOCK_HANDLE_HASH(ct_fd)], &clnt_fd_lock);

    if (cl->cb_thread != INVALID_HANDLE_VALUE) {
        int status;
        fprintf(stdout, "%04x: sending shutdown to callback thread %04zx\n", 
            GetCurrentThreadId(), (ULONG_PTR)cl->cb_thread);
        cl->shutdown = 1;
        mutex_unlock(&clnt_fd_lock);
        cond_signal(&vc_cv[WINSOCK_HANDLE_HASH(ct_fd)]);
        status = WaitForSingleObject(cl->cb_thread, INFINITE);
        fprintf(stdout, "%04x: terminated callback thread\n", GetCurrentThreadId());
        mutex_lock(&clnt_fd_lock);
        while (vc_fd_locks[WINSOCK_HANDLE_HASH(ct_fd)])
            cond_wait(&vc_cv[WINSOCK_HANDLE_HASH(ct_fd)], &clnt_fd_lock);
    }

	if (ct->ct_closeit && ct->ct_fd != -1) {
		(void)closesocket(ct->ct_fd);
	}
	XDR_DESTROY(&(ct->ct_xdrs));
	if (ct->ct_addr.buf)
		free(ct->ct_addr.buf);
	mem_free(ct, sizeof(struct ct_data));
	if (cl->cl_netid && cl->cl_netid[0])
		mem_free(cl->cl_netid, strlen(cl->cl_netid) +1);
	if (cl->cl_tp && cl->cl_tp[0])
		mem_free(cl->cl_tp, strlen(cl->cl_tp) +1);
	mem_free(cl, sizeof(CLIENT));
	mutex_unlock(&clnt_fd_lock);
//	thr_sigsetmask(SIG_SETMASK, &(mask), NULL);
	cond_signal(&vc_cv[WINSOCK_HANDLE_HASH(ct_fd)]);
}

/*
 * Interface between xdr serializer and tcp connection.
 * Behaves like the system calls, read & write, but keeps some error state
 * around for the rpc level.
 */
static int
read_vc(ctp, buf, len)
	void *ctp;
	void *buf;
	int len;
{
	/*
	struct sockaddr sa;
	socklen_t sal;
	*/
	struct ct_data *ct = (struct ct_data *)ctp;
	struct pollfd fd;
	int milliseconds = ct->ct_wait.tv_usec;

	if (len == 0)
		return (0);
	fd.fd = ct->ct_fd;
	fd.events = POLLIN;
	for (;;) {
#ifndef __REACTOS__
		switch (poll(&fd, 1, milliseconds)) {
#else
		/* ReactOS: use select instead of poll */
		fd_set infd;
		struct timeval timeout;

		FD_ZERO(&infd);
		FD_SET(ct->ct_fd, &infd);

		timeout.tv_sec = 0;
		timeout.tv_usec = milliseconds * 1000;

		switch (select(0, &infd, NULL, NULL, &timeout)) {
#endif

		case 0:
			ct->ct_error.re_status = RPC_TIMEDOUT;
			return (-1);

		case SOCKET_ERROR:
			errno = WSAGetLastError();
			if (errno == WSAEINTR)
				continue;
			ct->ct_error.re_status = RPC_CANTRECV;
			ct->ct_error.re_errno = errno;
			return (-2);
		}
		break;
	}

	len = recv(ct->ct_fd, buf, (size_t)len, 0);
	errno = WSAGetLastError();

	switch (len) {
	case 0:
		/* premature eof */
		ct->ct_error.re_errno = WSAECONNRESET;
		ct->ct_error.re_status = RPC_CANTRECV;
		len = -1;  /* it's really an error */
		break;

	case SOCKET_ERROR:
		ct->ct_error.re_errno = errno;
		ct->ct_error.re_status = RPC_CANTRECV;
		break;
	}
	return (len);
}

static int
#ifndef __REACTOS__
write_vc(ctp, buf, len)
#else
write_vc(ctp, ptr, len)
#endif
	void *ctp;
#ifndef __REACTOS__
	char *buf;
#else
    void *ptr;
#endif
	int len;
{
	struct ct_data *ct = (struct ct_data *)ctp;
	int i = 0, cnt;
#ifdef __REACTOS__
    char *buf = ptr;
#endif

	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
	    if ((i = send(ct->ct_fd, buf, (size_t)cnt, 0)) == SOCKET_ERROR) {
		ct->ct_error.re_errno = WSAGetLastError();
		ct->ct_error.re_status = RPC_CANTSEND;
		return (-1);
	    }
	}
	return (len);
}

static struct clnt_ops *
clnt_vc_ops()
{
	static struct clnt_ops ops;
	extern mutex_t  ops_lock;
#ifndef _WIN32
	sigset_t mask, newmask;

	/* VARIABLES PROTECTED BY ops_lock: ops */

	sigfillset(&newmask);
	thr_sigsetmask(SIG_SETMASK, &newmask, &mask);
#else
	/* XXX Need Windows signal/event stuff XXX */
#endif
	mutex_lock(&ops_lock);
	if (ops.cl_call == NULL) {
		ops.cl_call = clnt_vc_call;
		ops.cl_abort = clnt_vc_abort;
		ops.cl_geterr = clnt_vc_geterr;
		ops.cl_freeres = clnt_vc_freeres;
		ops.cl_destroy = clnt_vc_destroy;
		ops.cl_control = clnt_vc_control;
	}
	mutex_unlock(&ops_lock);
//	thr_sigsetmask(SIG_SETMASK, &(mask), NULL);
	return (&ops);
}

/*
 * Make sure that the time is not garbage.   -1 value is disallowed.
 * Note this is different from time_not_ok in clnt_dg.c
 */
static bool_t
time_not_ok(t)
	struct timeval *t;
{
	return (t->tv_sec <= -1 || t->tv_sec > 100000000 ||
		t->tv_usec <= -1 || t->tv_usec > 1000000);
}
