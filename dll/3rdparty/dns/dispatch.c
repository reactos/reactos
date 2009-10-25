/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: dispatch.c,v 1.155.12.7 2009/04/28 21:39:45 jinmei Exp $ */

/*! \file */

#include <config.h>

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <isc/entropy.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/portset.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/dispatch.h>
#include <dns/events.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/portlist.h>
#include <dns/stats.h>
#include <dns/tcpmsg.h>
#include <dns/types.h>

typedef ISC_LIST(dns_dispentry_t)	dns_displist_t;

typedef struct dispsocket		dispsocket_t;
typedef ISC_LIST(dispsocket_t)		dispsocketlist_t;

typedef struct dispportentry		dispportentry_t;
typedef ISC_LIST(dispportentry_t)	dispportlist_t;

/* ARC4 Random generator state */
typedef struct arc4ctx {
	isc_uint8_t	i;
	isc_uint8_t	j;
	isc_uint8_t	s[256];
	int		count;
	isc_entropy_t	*entropy;	/*%< entropy source for ARC4 */
	isc_mutex_t	*lock;
} arc4ctx_t;

typedef struct dns_qid {
	unsigned int	magic;
	unsigned int	qid_nbuckets;	/*%< hash table size */
	unsigned int	qid_increment;	/*%< id increment on collision */
	isc_mutex_t	lock;
	dns_displist_t	*qid_table;	/*%< the table itself */
	dispsocketlist_t *sock_table;	/*%< socket table */
} dns_qid_t;

struct dns_dispatchmgr {
	/* Unlocked. */
	unsigned int			magic;
	isc_mem_t		       *mctx;
	dns_acl_t		       *blackhole;
	dns_portlist_t		       *portlist;
	isc_stats_t		       *stats;
	isc_entropy_t		       *entropy; /*%< entropy source */

	/* Locked by "lock". */
	isc_mutex_t			lock;
	unsigned int			state;
	ISC_LIST(dns_dispatch_t)	list;

	/* Locked by arc4_lock. */
	isc_mutex_t			arc4_lock;
	arc4ctx_t			arc4ctx;    /*%< ARC4 context for QID */

	/* locked by buffer lock */
	dns_qid_t			*qid;
	isc_mutex_t			buffer_lock;
	unsigned int			buffers;    /*%< allocated buffers */
	unsigned int			buffersize; /*%< size of each buffer */
	unsigned int			maxbuffers; /*%< max buffers */

	/* Locked internally. */
	isc_mutex_t			pool_lock;
	isc_mempool_t		       *epool;	/*%< memory pool for events */
	isc_mempool_t		       *rpool;	/*%< memory pool for replies */
	isc_mempool_t		       *dpool;  /*%< dispatch allocations */
	isc_mempool_t		       *bpool;	/*%< memory pool for buffers */
	isc_mempool_t		       *spool;	/*%< memory pool for dispsocs */

	/*%
	 * Locked by qid->lock if qid exists; otherwise, can be used without
	 * being locked.
	 * Memory footprint considerations: this is a simple implementation of
	 * available ports, i.e., an ordered array of the actual port numbers.
	 * This will require about 256KB of memory in the worst case (128KB for
	 * each of IPv4 and IPv6).  We could reduce it by representing it as a
	 * more sophisticated way such as a list (or array) of ranges that are
	 * searched to identify a specific port.  Our decision here is the saved
	 * memory isn't worth the implementation complexity, considering the
	 * fact that the whole BIND9 process (which is mainly named) already
	 * requires a pretty large memory footprint.  We may, however, have to
	 * revisit the decision when we want to use it as a separate module for
	 * an environment where memory requirement is severer.
	 */
	in_port_t	*v4ports;	/*%< available ports for IPv4 */
	unsigned int	nv4ports;	/*%< # of available ports for IPv4 */
	in_port_t	*v6ports;	/*%< available ports for IPv4 */
	unsigned int	nv6ports;	/*%< # of available ports for IPv4 */
};

#define MGR_SHUTTINGDOWN		0x00000001U
#define MGR_IS_SHUTTINGDOWN(l)	(((l)->state & MGR_SHUTTINGDOWN) != 0)

#define IS_PRIVATE(d)	(((d)->attributes & DNS_DISPATCHATTR_PRIVATE) != 0)

struct dns_dispentry {
	unsigned int			magic;
	dns_dispatch_t		       *disp;
	dns_messageid_t			id;
	in_port_t			port;
	unsigned int			bucket;
	isc_sockaddr_t			host;
	isc_task_t		       *task;
	isc_taskaction_t		action;
	void			       *arg;
	isc_boolean_t			item_out;
	dispsocket_t			*dispsocket;
	ISC_LIST(dns_dispatchevent_t)	items;
	ISC_LINK(dns_dispentry_t)	link;
};

/*%
 * Maximum number of dispatch sockets that can be pooled for reuse.  The
 * appropriate value may vary, but experiments have shown a busy caching server
 * may need more than 1000 sockets concurrently opened.  The maximum allowable
 * number of dispatch sockets (per manager) will be set to the double of this
 * value.
 */
#ifndef DNS_DISPATCH_POOLSOCKS
#define DNS_DISPATCH_POOLSOCKS			2048
#endif

/*%
 * Quota to control the number of dispatch sockets.  If a dispatch has more
 * than the quota of sockets, new queries will purge oldest ones, so that
 * a massive number of outstanding queries won't prevent subsequent queries
 * (especially if the older ones take longer time and result in timeout).
 */
#ifndef DNS_DISPATCH_SOCKSQUOTA
#define DNS_DISPATCH_SOCKSQUOTA			3072
#endif

struct dispsocket {
	unsigned int			magic;
	isc_socket_t			*socket;
	dns_dispatch_t			*disp;
	isc_sockaddr_t			host;
	in_port_t			localport; /* XXX: should be removed later */
	dispportentry_t			*portentry;
	dns_dispentry_t			*resp;
	isc_task_t			*task;
	ISC_LINK(dispsocket_t)		link;
	unsigned int			bucket;
	ISC_LINK(dispsocket_t)		blink;
};

/*%
 * A port table entry.  We remember every port we first open in a table with a
 * reference counter so that we can 'reuse' the same port (with different
 * destination addresses) using the SO_REUSEADDR socket option.
 */
struct dispportentry {
	in_port_t			port;
	unsigned int			refs;
	ISC_LINK(struct dispportentry)	link;
};

#ifndef DNS_DISPATCH_PORTTABLESIZE
#define DNS_DISPATCH_PORTTABLESIZE	1024
#endif

#define INVALID_BUCKET		(0xffffdead)

/*%
 * Number of tasks for each dispatch that use separate sockets for different
 * transactions.  This must be a power of 2 as it will divide 32 bit numbers
 * to get an uniformly random tasks selection.  See get_dispsocket().
 */
#define MAX_INTERNAL_TASKS	64

struct dns_dispatch {
	/* Unlocked. */
	unsigned int		magic;		/*%< magic */
	dns_dispatchmgr_t      *mgr;		/*%< dispatch manager */
	int			ntasks;
	/*%
	 * internal task buckets.  We use multiple tasks to distribute various
	 * socket events well when using separate dispatch sockets.  We use the
	 * 1st task (task[0]) for internal control events.
	 */
	isc_task_t	       *task[MAX_INTERNAL_TASKS];
	isc_socket_t	       *socket;		/*%< isc socket attached to */
	isc_sockaddr_t		local;		/*%< local address */
	in_port_t		localport;	/*%< local UDP port */
	unsigned int		maxrequests;	/*%< max requests */
	isc_event_t	       *ctlevent;

	/*% Locked by mgr->lock. */
	ISC_LINK(dns_dispatch_t) link;

	/* Locked by "lock". */
	isc_mutex_t		lock;		/*%< locks all below */
	isc_sockettype_t	socktype;
	unsigned int		attributes;
	unsigned int		refcount;	/*%< number of users */
	dns_dispatchevent_t    *failsafe_ev;	/*%< failsafe cancel event */
	unsigned int		shutting_down : 1,
				shutdown_out : 1,
				connected : 1,
				tcpmsg_valid : 1,
				recv_pending : 1; /*%< is a recv() pending? */
	isc_result_t		shutdown_why;
	ISC_LIST(dispsocket_t)	activesockets;
	ISC_LIST(dispsocket_t)	inactivesockets;
	unsigned int		nsockets;
	unsigned int		requests;	/*%< how many requests we have */
	unsigned int		tcpbuffers;	/*%< allocated buffers */
	dns_tcpmsg_t		tcpmsg;		/*%< for tcp streams */
	dns_qid_t		*qid;
	arc4ctx_t		arc4ctx;	/*%< for QID/UDP port num */
	dispportlist_t		*port_table;	/*%< hold ports 'owned' by us */
	isc_mempool_t		*portpool;	/*%< port table entries  */
};

#define QID_MAGIC		ISC_MAGIC('Q', 'i', 'd', ' ')
#define VALID_QID(e)		ISC_MAGIC_VALID((e), QID_MAGIC)

#define RESPONSE_MAGIC		ISC_MAGIC('D', 'r', 's', 'p')
#define VALID_RESPONSE(e)	ISC_MAGIC_VALID((e), RESPONSE_MAGIC)

#define DISPSOCK_MAGIC		ISC_MAGIC('D', 's', 'o', 'c')
#define VALID_DISPSOCK(e)	ISC_MAGIC_VALID((e), DISPSOCK_MAGIC)

#define DISPATCH_MAGIC		ISC_MAGIC('D', 'i', 's', 'p')
#define VALID_DISPATCH(e)	ISC_MAGIC_VALID((e), DISPATCH_MAGIC)

#define DNS_DISPATCHMGR_MAGIC	ISC_MAGIC('D', 'M', 'g', 'r')
#define VALID_DISPATCHMGR(e)	ISC_MAGIC_VALID((e), DNS_DISPATCHMGR_MAGIC)

#define DNS_QID(disp) ((disp)->socktype == isc_sockettype_tcp) ? \
		       (disp)->qid : (disp)->mgr->qid
#define DISP_ARC4CTX(disp) ((disp)->socktype == isc_sockettype_udp) ? \
			(&(disp)->arc4ctx) : (&(disp)->mgr->arc4ctx)

/*%
 * Locking a query port buffer is a bit tricky.  We access the buffer without
 * locking until qid is created.  Technically, there is a possibility of race
 * between the creation of qid and access to the port buffer; in practice,
 * however, this should be safe because qid isn't created until the first
 * dispatch is created and there should be no contending situation until then.
 */
#define PORTBUFLOCK(mgr) if ((mgr)->qid != NULL) LOCK(&((mgr)->qid->lock))
#define PORTBUFUNLOCK(mgr) if ((mgr)->qid != NULL) UNLOCK((&(mgr)->qid->lock))

/*
 * Statics.
 */
static dns_dispentry_t *entry_search(dns_qid_t *, isc_sockaddr_t *,
				     dns_messageid_t, in_port_t, unsigned int);
static isc_boolean_t destroy_disp_ok(dns_dispatch_t *);
static void destroy_disp(isc_task_t *task, isc_event_t *event);
static void destroy_dispsocket(dns_dispatch_t *, dispsocket_t **);
static void deactivate_dispsocket(dns_dispatch_t *, dispsocket_t *);
static void udp_exrecv(isc_task_t *, isc_event_t *);
static void udp_shrecv(isc_task_t *, isc_event_t *);
static void udp_recv(isc_event_t *, dns_dispatch_t *, dispsocket_t *);
static void tcp_recv(isc_task_t *, isc_event_t *);
static isc_result_t startrecv(dns_dispatch_t *, dispsocket_t *);
static isc_uint32_t dns_hash(dns_qid_t *, isc_sockaddr_t *, dns_messageid_t,
			     in_port_t);
static void free_buffer(dns_dispatch_t *disp, void *buf, unsigned int len);
static void *allocate_udp_buffer(dns_dispatch_t *disp);
static inline void free_event(dns_dispatch_t *disp, dns_dispatchevent_t *ev);
static inline dns_dispatchevent_t *allocate_event(dns_dispatch_t *disp);
static void do_cancel(dns_dispatch_t *disp);
static dns_dispentry_t *linear_first(dns_qid_t *disp);
static dns_dispentry_t *linear_next(dns_qid_t *disp,
				    dns_dispentry_t *resp);
static void dispatch_free(dns_dispatch_t **dispp);
static isc_result_t get_udpsocket(dns_dispatchmgr_t *mgr,
				  dns_dispatch_t *disp,
				  isc_socketmgr_t *sockmgr,
				  isc_sockaddr_t *localaddr,
				  isc_socket_t **sockp);
static isc_result_t dispatch_createudp(dns_dispatchmgr_t *mgr,
				       isc_socketmgr_t *sockmgr,
				       isc_taskmgr_t *taskmgr,
				       isc_sockaddr_t *localaddr,
				       unsigned int maxrequests,
				       unsigned int attributes,
				       dns_dispatch_t **dispp);
static isc_boolean_t destroy_mgr_ok(dns_dispatchmgr_t *mgr);
static void destroy_mgr(dns_dispatchmgr_t **mgrp);
static isc_result_t qid_allocate(dns_dispatchmgr_t *mgr, unsigned int buckets,
				 unsigned int increment, dns_qid_t **qidp,
				 isc_boolean_t needaddrtable);
static void qid_destroy(isc_mem_t *mctx, dns_qid_t **qidp);
static isc_result_t open_socket(isc_socketmgr_t *mgr, isc_sockaddr_t *local,
				unsigned int options, isc_socket_t **sockp);
static isc_boolean_t portavailable(dns_dispatchmgr_t *mgr, isc_socket_t *sock,
				   isc_sockaddr_t *sockaddrp);

#define LVL(x) ISC_LOG_DEBUG(x)

static void
mgr_log(dns_dispatchmgr_t *mgr, int level, const char *fmt, ...)
     ISC_FORMAT_PRINTF(3, 4);

static void
mgr_log(dns_dispatchmgr_t *mgr, int level, const char *fmt, ...) {
	char msgbuf[2048];
	va_list ap;

	if (! isc_log_wouldlog(dns_lctx, level))
		return;

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	isc_log_write(dns_lctx,
		      DNS_LOGCATEGORY_DISPATCH, DNS_LOGMODULE_DISPATCH,
		      level, "dispatchmgr %p: %s", mgr, msgbuf);
}

static inline void
inc_stats(dns_dispatchmgr_t *mgr, isc_statscounter_t counter) {
	if (mgr->stats != NULL)
		isc_stats_increment(mgr->stats, counter);
}

static void
dispatch_log(dns_dispatch_t *disp, int level, const char *fmt, ...)
     ISC_FORMAT_PRINTF(3, 4);

static void
dispatch_log(dns_dispatch_t *disp, int level, const char *fmt, ...) {
	char msgbuf[2048];
	va_list ap;

	if (! isc_log_wouldlog(dns_lctx, level))
		return;

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	isc_log_write(dns_lctx,
		      DNS_LOGCATEGORY_DISPATCH, DNS_LOGMODULE_DISPATCH,
		      level, "dispatch %p: %s", disp, msgbuf);
}

static void
request_log(dns_dispatch_t *disp, dns_dispentry_t *resp,
	    int level, const char *fmt, ...)
     ISC_FORMAT_PRINTF(4, 5);

static void
request_log(dns_dispatch_t *disp, dns_dispentry_t *resp,
	    int level, const char *fmt, ...)
{
	char msgbuf[2048];
	char peerbuf[256];
	va_list ap;

	if (! isc_log_wouldlog(dns_lctx, level))
		return;

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	if (VALID_RESPONSE(resp)) {
		isc_sockaddr_format(&resp->host, peerbuf, sizeof(peerbuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DISPATCH,
			      DNS_LOGMODULE_DISPATCH, level,
			      "dispatch %p response %p %s: %s", disp, resp,
			      peerbuf, msgbuf);
	} else {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DISPATCH,
			      DNS_LOGMODULE_DISPATCH, level,
			      "dispatch %p req/resp %p: %s", disp, resp,
			      msgbuf);
	}
}

/*%
 * ARC4 random number generator derived from OpenBSD.
 * Only dispatch_arc4random() and dispatch_arc4uniformrandom() are expected
 * to be called from general dispatch routines; the rest of them are subroutines
 * for these two.
 *
 * The original copyright follows:
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
static void
dispatch_arc4init(arc4ctx_t *actx, isc_entropy_t *entropy, isc_mutex_t *lock) {
	int n;
	for (n = 0; n < 256; n++)
		actx->s[n] = n;
	actx->i = 0;
	actx->j = 0;
	actx->count = 0;
	actx->entropy = entropy; /* don't have to attach */
	actx->lock = lock;
}

static void
dispatch_arc4addrandom(arc4ctx_t *actx, unsigned char *dat, int datlen) {
	int n;
	isc_uint8_t si;

	actx->i--;
	for (n = 0; n < 256; n++) {
		actx->i = (actx->i + 1);
		si = actx->s[actx->i];
		actx->j = (actx->j + si + dat[n % datlen]);
		actx->s[actx->i] = actx->s[actx->j];
		actx->s[actx->j] = si;
	}
	actx->j = actx->i;
}

static inline isc_uint8_t
dispatch_arc4get8(arc4ctx_t *actx) {
	isc_uint8_t si, sj;

	actx->i = (actx->i + 1);
	si = actx->s[actx->i];
	actx->j = (actx->j + si);
	sj = actx->s[actx->j];
	actx->s[actx->i] = sj;
	actx->s[actx->j] = si;

	return (actx->s[(si + sj) & 0xff]);
}

static inline isc_uint16_t
dispatch_arc4get16(arc4ctx_t *actx) {
	isc_uint16_t val;

	val = dispatch_arc4get8(actx) << 8;
	val |= dispatch_arc4get8(actx);

	return (val);
}

static void
dispatch_arc4stir(arc4ctx_t *actx) {
	int i;
	union {
		unsigned char rnd[128];
		isc_uint32_t rnd32[32];
	} rnd;
	isc_result_t result;

	if (actx->entropy != NULL) {
		/*
		 * We accept any quality of random data to avoid blocking.
		 */
		result = isc_entropy_getdata(actx->entropy, rnd.rnd,
					     sizeof(rnd), NULL, 0);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	} else {
		for (i = 0; i < 32; i++)
			isc_random_get(&rnd.rnd32[i]);
	}
	dispatch_arc4addrandom(actx, rnd.rnd, sizeof(rnd.rnd));

	/*
	 * Discard early keystream, as per recommendations in:
	 * http://www.wisdom.weizmann.ac.il/~itsik/RC4/Papers/Rc4_ksa.ps
	 */
	for (i = 0; i < 256; i++)
		(void)dispatch_arc4get8(actx);

	/*
	 * Derived from OpenBSD's implementation.  The rationale is not clear,
	 * but should be conservative enough in safety, and reasonably large
	 * for efficiency.
	 */
	actx->count = 1600000;
}

static isc_uint16_t
dispatch_arc4random(arc4ctx_t *actx) {
	isc_uint16_t result;

	if (actx->lock != NULL)
		LOCK(actx->lock);

	actx->count -= sizeof(isc_uint16_t);
	if (actx->count <= 0)
		dispatch_arc4stir(actx);
	result = dispatch_arc4get16(actx);

	if (actx->lock != NULL)
		UNLOCK(actx->lock);

	return (result);
}

static isc_uint16_t
dispatch_arc4uniformrandom(arc4ctx_t *actx, isc_uint16_t upper_bound) {
	isc_uint16_t min, r;

	if (upper_bound < 2)
		return (0);

	/*
	 * Ensure the range of random numbers [min, 0xffff] be a multiple of
	 * upper_bound and contain at least a half of the 16 bit range.
	 */

	if (upper_bound > 0x8000)
		min = 1 + ~upper_bound; /* 0x8000 - upper_bound */
	else
		min = (isc_uint16_t)(0x10000 % (isc_uint32_t)upper_bound);

	/*
	 * This could theoretically loop forever but each retry has
	 * p > 0.5 (worst case, usually far better) of selecting a
	 * number inside the range we need, so it should rarely need
	 * to re-roll.
	 */
	for (;;) {
		r = dispatch_arc4random(actx);
		if (r >= min)
			break;
	}

	return (r % upper_bound);
}

/*
 * Return a hash of the destination and message id.
 */
static isc_uint32_t
dns_hash(dns_qid_t *qid, isc_sockaddr_t *dest, dns_messageid_t id,
	 in_port_t port)
{
	unsigned int ret;

	ret = isc_sockaddr_hash(dest, ISC_TRUE);
	ret ^= (id << 16) | port;
	ret %= qid->qid_nbuckets;

	INSIST(ret < qid->qid_nbuckets);

	return (ret);
}

/*
 * Find the first entry in 'qid'.  Returns NULL if there are no entries.
 */
static dns_dispentry_t *
linear_first(dns_qid_t *qid) {
	dns_dispentry_t *ret;
	unsigned int bucket;

	bucket = 0;

	while (bucket < qid->qid_nbuckets) {
		ret = ISC_LIST_HEAD(qid->qid_table[bucket]);
		if (ret != NULL)
			return (ret);
		bucket++;
	}

	return (NULL);
}

/*
 * Find the next entry after 'resp' in 'qid'.  Return NULL if there are
 * no more entries.
 */
static dns_dispentry_t *
linear_next(dns_qid_t *qid, dns_dispentry_t *resp) {
	dns_dispentry_t *ret;
	unsigned int bucket;

	ret = ISC_LIST_NEXT(resp, link);
	if (ret != NULL)
		return (ret);

	bucket = resp->bucket;
	bucket++;
	while (bucket < qid->qid_nbuckets) {
		ret = ISC_LIST_HEAD(qid->qid_table[bucket]);
		if (ret != NULL)
			return (ret);
		bucket++;
	}

	return (NULL);
}

/*
 * The dispatch must be locked.
 */
static isc_boolean_t
destroy_disp_ok(dns_dispatch_t *disp)
{
	if (disp->refcount != 0)
		return (ISC_FALSE);

	if (disp->recv_pending != 0)
		return (ISC_FALSE);

	if (!ISC_LIST_EMPTY(disp->activesockets))
		return (ISC_FALSE);

	if (disp->shutting_down == 0)
		return (ISC_FALSE);

	return (ISC_TRUE);
}

/*
 * Called when refcount reaches 0 (and safe to destroy).
 *
 * The dispatcher must not be locked.
 * The manager must be locked.
 */
static void
destroy_disp(isc_task_t *task, isc_event_t *event) {
	dns_dispatch_t *disp;
	dns_dispatchmgr_t *mgr;
	isc_boolean_t killmgr;
	dispsocket_t *dispsocket;
	int i;

	INSIST(event->ev_type == DNS_EVENT_DISPATCHCONTROL);

	UNUSED(task);

	disp = event->ev_arg;
	mgr = disp->mgr;

	LOCK(&mgr->lock);
	ISC_LIST_UNLINK(mgr->list, disp, link);

	dispatch_log(disp, LVL(90),
		     "shutting down; detaching from sock %p, task %p",
		     disp->socket, disp->task[0]); /* XXXX */

	if (disp->socket != NULL)
		isc_socket_detach(&disp->socket);
	while ((dispsocket = ISC_LIST_HEAD(disp->inactivesockets)) != NULL) {
		ISC_LIST_UNLINK(disp->inactivesockets, dispsocket, link);
		destroy_dispsocket(disp, &dispsocket);
	}
	for (i = 0; i < disp->ntasks; i++)
		isc_task_detach(&disp->task[i]);
	isc_event_free(&event);

	dispatch_free(&disp);

	killmgr = destroy_mgr_ok(mgr);
	UNLOCK(&mgr->lock);
	if (killmgr)
		destroy_mgr(&mgr);
}

/*%
 * Manipulate port table per dispatch: find an entry for a given port number,
 * create a new entry, and decrement a given entry with possible clean-up.
 */
static dispportentry_t *
port_search(dns_dispatch_t *disp, in_port_t port) {
	dispportentry_t *portentry;

	REQUIRE(disp->port_table != NULL);

	portentry = ISC_LIST_HEAD(disp->port_table[port %
						   DNS_DISPATCH_PORTTABLESIZE]);
	while (portentry != NULL) {
		if (portentry->port == port)
			return (portentry);
		portentry = ISC_LIST_NEXT(portentry, link);
	}

	return (NULL);
}

static dispportentry_t *
new_portentry(dns_dispatch_t *disp, in_port_t port) {
	dispportentry_t *portentry;

	REQUIRE(disp->port_table != NULL);

	portentry = isc_mempool_get(disp->portpool);
	if (portentry == NULL)
		return (portentry);

	portentry->port = port;
	portentry->refs = 0;
	ISC_LINK_INIT(portentry, link);
	ISC_LIST_APPEND(disp->port_table[port % DNS_DISPATCH_PORTTABLESIZE],
			portentry, link);

	return (portentry);
}

static void
deref_portentry(dns_dispatch_t *disp, dispportentry_t **portentryp) {
	dispportentry_t *portentry = *portentryp;

	REQUIRE(disp->port_table != NULL);
	REQUIRE(portentry != NULL && portentry->refs > 0);

	portentry->refs--;
	if (portentry->refs == 0) {
		ISC_LIST_UNLINK(disp->port_table[portentry->port %
						 DNS_DISPATCH_PORTTABLESIZE],
				portentry, link);
		isc_mempool_put(disp->portpool, portentry);
	}

	*portentryp = NULL;
}

/*%
 * Find a dispsocket for socket address 'dest', and port number 'port'.
 * Return NULL if no such entry exists.
 */
static dispsocket_t *
socket_search(dns_qid_t *qid, isc_sockaddr_t *dest, in_port_t port,
	      unsigned int bucket)
{
	dispsocket_t *dispsock;

	REQUIRE(bucket < qid->qid_nbuckets);

	dispsock = ISC_LIST_HEAD(qid->sock_table[bucket]);

	while (dispsock != NULL) {
		if (isc_sockaddr_equal(dest, &dispsock->host) &&
		    dispsock->portentry->port == port)
			return (dispsock);
		dispsock = ISC_LIST_NEXT(dispsock, blink);
	}

	return (NULL);
}

/*%
 * Make a new socket for a single dispatch with a random port number.
 * The caller must hold the disp->lock and qid->lock.
 */
static isc_result_t
get_dispsocket(dns_dispatch_t *disp, isc_sockaddr_t *dest,
	       isc_socketmgr_t *sockmgr, dns_qid_t *qid,
	       dispsocket_t **dispsockp, in_port_t *portp)
{
	int i;
	isc_uint32_t r;
	dns_dispatchmgr_t *mgr = disp->mgr;
	isc_socket_t *sock = NULL;
	isc_result_t result = ISC_R_FAILURE;
	in_port_t port;
	isc_sockaddr_t localaddr;
	unsigned int bucket = 0;
	dispsocket_t *dispsock;
	unsigned int nports;
	in_port_t *ports;
	unsigned int bindoptions;
	dispportentry_t *portentry = NULL;

	if (isc_sockaddr_pf(&disp->local) == AF_INET) {
		nports = disp->mgr->nv4ports;
		ports = disp->mgr->v4ports;
	} else {
		nports = disp->mgr->nv6ports;
		ports = disp->mgr->v6ports;
	}
	if (nports == 0)
		return (ISC_R_ADDRNOTAVAIL);

	dispsock = ISC_LIST_HEAD(disp->inactivesockets);
	if (dispsock != NULL) {
		ISC_LIST_UNLINK(disp->inactivesockets, dispsock, link);
		sock = dispsock->socket;
		dispsock->socket = NULL;
	} else {
		dispsock = isc_mempool_get(mgr->spool);
		if (dispsock == NULL)
			return (ISC_R_NOMEMORY);

		disp->nsockets++;
		dispsock->socket = NULL;
		dispsock->disp = disp;
		dispsock->resp = NULL;
		dispsock->portentry = NULL;
		isc_random_get(&r);
		dispsock->task = NULL;
		isc_task_attach(disp->task[r % disp->ntasks], &dispsock->task);
		ISC_LINK_INIT(dispsock, link);
		ISC_LINK_INIT(dispsock, blink);
		dispsock->magic = DISPSOCK_MAGIC;
	}

	/*
	 * Pick up a random UDP port and open a new socket with it.  Avoid
	 * choosing ports that share the same destination because it will be
	 * very likely to fail in bind(2) or connect(2).
	 */
	localaddr = disp->local;
	for (i = 0; i < 64; i++) {
		port = ports[dispatch_arc4uniformrandom(DISP_ARC4CTX(disp),
							nports)];
		isc_sockaddr_setport(&localaddr, port);

		bucket = dns_hash(qid, dest, 0, port);
		if (socket_search(qid, dest, port, bucket) != NULL)
			continue;
		bindoptions = 0;
		portentry = port_search(disp, port);
		if (portentry != NULL)
			bindoptions |= ISC_SOCKET_REUSEADDRESS;
		result = open_socket(sockmgr, &localaddr, bindoptions, &sock);
		if (result == ISC_R_SUCCESS) {
			if (portentry == NULL) {
				portentry = new_portentry(disp, port);
				if (portentry == NULL) {
					result = ISC_R_NOMEMORY;
					break;
				}
			}
			portentry->refs++;
			break;
		} else if (result != ISC_R_ADDRINUSE)
			break;
	}

	if (result == ISC_R_SUCCESS) {
		dispsock->socket = sock;
		dispsock->host = *dest;
		dispsock->portentry = portentry;
		dispsock->bucket = bucket;
		ISC_LIST_APPEND(qid->sock_table[bucket], dispsock, blink);
		*dispsockp = dispsock;
		*portp = port;
	} else {
		/*
		 * We could keep it in the inactive list, but since this should
		 * be an exceptional case and might be resource shortage, we'd
		 * rather destroy it.
		 */
		if (sock != NULL)
			isc_socket_detach(&sock);
		destroy_dispsocket(disp, &dispsock);
	}

	return (result);
}

/*%
 * Destroy a dedicated dispatch socket.
 */
static void
destroy_dispsocket(dns_dispatch_t *disp, dispsocket_t **dispsockp) {
	dispsocket_t *dispsock;
	dns_qid_t *qid;

	/*
	 * The dispatch must be locked.
	 */

	REQUIRE(dispsockp != NULL && *dispsockp != NULL);
	dispsock = *dispsockp;
	REQUIRE(!ISC_LINK_LINKED(dispsock, link));

	disp->nsockets--;
	dispsock->magic = 0;
	if (dispsock->portentry != NULL)
		deref_portentry(disp, &dispsock->portentry);
	if (dispsock->socket != NULL)
		isc_socket_detach(&dispsock->socket);
	if (ISC_LINK_LINKED(dispsock, blink)) {
		qid = DNS_QID(disp);
		LOCK(&qid->lock);
		ISC_LIST_UNLINK(qid->sock_table[dispsock->bucket], dispsock,
				blink);
		UNLOCK(&qid->lock);
	}
	if (dispsock->task != NULL)
		isc_task_detach(&dispsock->task);
	isc_mempool_put(disp->mgr->spool, dispsock);

	*dispsockp = NULL;
}

/*%
 * Deactivate a dedicated dispatch socket.  Move it to the inactive list for
 * future reuse unless the total number of sockets are exceeding the maximum.
 */
static void
deactivate_dispsocket(dns_dispatch_t *disp, dispsocket_t *dispsock) {
	isc_result_t result;
	dns_qid_t *qid;

	/*
	 * The dispatch must be locked.
	 */
	ISC_LIST_UNLINK(disp->activesockets, dispsock, link);
	if (dispsock->resp != NULL) {
		INSIST(dispsock->resp->dispsocket == dispsock);
		dispsock->resp->dispsocket = NULL;
	}

	INSIST(dispsock->portentry != NULL);
	deref_portentry(disp, &dispsock->portentry);

	if (disp->nsockets > DNS_DISPATCH_POOLSOCKS)
		destroy_dispsocket(disp, &dispsock);
	else {
		result = isc_socket_close(dispsock->socket);

		qid = DNS_QID(disp);
		LOCK(&qid->lock);
		ISC_LIST_UNLINK(qid->sock_table[dispsock->bucket], dispsock,
				blink);
		UNLOCK(&qid->lock);

		if (result == ISC_R_SUCCESS)
			ISC_LIST_APPEND(disp->inactivesockets, dispsock, link);
		else {
			/*
			 * If the underlying system does not allow this
			 * optimization, destroy this temporary structure (and
			 * create a new one for a new transaction).
			 */
			INSIST(result == ISC_R_NOTIMPLEMENTED);
			destroy_dispsocket(disp, &dispsock);
		}
	}
}

/*
 * Find an entry for query ID 'id', socket address 'dest', and port number
 * 'port'.
 * Return NULL if no such entry exists.
 */
static dns_dispentry_t *
entry_search(dns_qid_t *qid, isc_sockaddr_t *dest, dns_messageid_t id,
	     in_port_t port, unsigned int bucket)
{
	dns_dispentry_t *res;

	REQUIRE(bucket < qid->qid_nbuckets);

	res = ISC_LIST_HEAD(qid->qid_table[bucket]);

	while (res != NULL) {
		if (res->id == id && isc_sockaddr_equal(dest, &res->host) &&
		    res->port == port) {
			return (res);
		}
		res = ISC_LIST_NEXT(res, link);
	}

	return (NULL);
}

static void
free_buffer(dns_dispatch_t *disp, void *buf, unsigned int len) {
	INSIST(buf != NULL && len != 0);


	switch (disp->socktype) {
	case isc_sockettype_tcp:
		INSIST(disp->tcpbuffers > 0);
		disp->tcpbuffers--;
		isc_mem_put(disp->mgr->mctx, buf, len);
		break;
	case isc_sockettype_udp:
		LOCK(&disp->mgr->buffer_lock);
		INSIST(disp->mgr->buffers > 0);
		INSIST(len == disp->mgr->buffersize);
		disp->mgr->buffers--;
		isc_mempool_put(disp->mgr->bpool, buf);
		UNLOCK(&disp->mgr->buffer_lock);
		break;
	default:
		INSIST(0);
		break;
	}
}

static void *
allocate_udp_buffer(dns_dispatch_t *disp) {
	void *temp;

	LOCK(&disp->mgr->buffer_lock);
	temp = isc_mempool_get(disp->mgr->bpool);

	if (temp != NULL)
		disp->mgr->buffers++;
	UNLOCK(&disp->mgr->buffer_lock);

	return (temp);
}

static inline void
free_event(dns_dispatch_t *disp, dns_dispatchevent_t *ev) {
	if (disp->failsafe_ev == ev) {
		INSIST(disp->shutdown_out == 1);
		disp->shutdown_out = 0;

		return;
	}

	isc_mempool_put(disp->mgr->epool, ev);
}

static inline dns_dispatchevent_t *
allocate_event(dns_dispatch_t *disp) {
	dns_dispatchevent_t *ev;

	ev = isc_mempool_get(disp->mgr->epool);
	if (ev == NULL)
		return (NULL);
	ISC_EVENT_INIT(ev, sizeof(*ev), 0, NULL, 0,
		       NULL, NULL, NULL, NULL, NULL);

	return (ev);
}

static void
udp_exrecv(isc_task_t *task, isc_event_t *ev) {
	dispsocket_t *dispsock = ev->ev_arg;

	UNUSED(task);

	REQUIRE(VALID_DISPSOCK(dispsock));
	udp_recv(ev, dispsock->disp, dispsock);
}

static void
udp_shrecv(isc_task_t *task, isc_event_t *ev) {
	dns_dispatch_t *disp = ev->ev_arg;

	UNUSED(task);

	REQUIRE(VALID_DISPATCH(disp));
	udp_recv(ev, disp, NULL);
}

/*
 * General flow:
 *
 * If I/O result == CANCELED or error, free the buffer.
 *
 * If query, free the buffer, restart.
 *
 * If response:
 *	Allocate event, fill in details.
 *		If cannot allocate, free buffer, restart.
 *	find target.  If not found, free buffer, restart.
 *	if event queue is not empty, queue.  else, send.
 *	restart.
 */
static void
udp_recv(isc_event_t *ev_in, dns_dispatch_t *disp, dispsocket_t *dispsock) {
	isc_socketevent_t *ev = (isc_socketevent_t *)ev_in;
	dns_messageid_t id;
	isc_result_t dres;
	isc_buffer_t source;
	unsigned int flags;
	dns_dispentry_t *resp = NULL;
	dns_dispatchevent_t *rev;
	unsigned int bucket;
	isc_boolean_t killit;
	isc_boolean_t queue_response;
	dns_dispatchmgr_t *mgr;
	dns_qid_t *qid;
	isc_netaddr_t netaddr;
	int match;
	int result;
	isc_boolean_t qidlocked = ISC_FALSE;

	LOCK(&disp->lock);

	mgr = disp->mgr;
	qid = mgr->qid;

	dispatch_log(disp, LVL(90),
		     "got packet: requests %d, buffers %d, recvs %d",
		     disp->requests, disp->mgr->buffers, disp->recv_pending);

	if (dispsock == NULL && ev->ev_type == ISC_SOCKEVENT_RECVDONE) {
		/*
		 * Unless the receive event was imported from a listening
		 * interface, in which case the event type is
		 * DNS_EVENT_IMPORTRECVDONE, receive operation must be pending.
		 */
		INSIST(disp->recv_pending != 0);
		disp->recv_pending = 0;
	}

	if (dispsock != NULL &&
	    (ev->result == ISC_R_CANCELED || dispsock->resp == NULL)) {
		/*
		 * dispsock->resp can be NULL if this transaction was canceled
		 * just after receiving a response.  Since this socket is
		 * exclusively used and there should be at most one receive
		 * event the canceled event should have been no effect.  So
		 * we can (and should) deactivate the socket right now.
		 */
		deactivate_dispsocket(disp, dispsock);
		dispsock = NULL;
	}

	if (disp->shutting_down) {
		/*
		 * This dispatcher is shutting down.
		 */
		free_buffer(disp, ev->region.base, ev->region.length);

		isc_event_free(&ev_in);
		ev = NULL;

		killit = destroy_disp_ok(disp);
		UNLOCK(&disp->lock);
		if (killit)
			isc_task_send(disp->task[0], &disp->ctlevent);

		return;
	}

	if ((disp->attributes & DNS_DISPATCHATTR_EXCLUSIVE) != 0) {
		if (dispsock != NULL) {
			resp = dispsock->resp;
			id = resp->id;
			if (ev->result != ISC_R_SUCCESS) {
				/*
				 * This is most likely a network error on a
				 * connected socket.  It makes no sense to
				 * check the address or parse the packet, but it
				 * will help to return the error to the caller.
				 */
				goto sendresponse;
			}
		} else {
			free_buffer(disp, ev->region.base, ev->region.length);

			UNLOCK(&disp->lock);
			isc_event_free(&ev_in);
			return;
		}
	} else if (ev->result != ISC_R_SUCCESS) {
		free_buffer(disp, ev->region.base, ev->region.length);

		if (ev->result != ISC_R_CANCELED)
			dispatch_log(disp, ISC_LOG_ERROR,
				     "odd socket result in udp_recv(): %s",
				     isc_result_totext(ev->result));

		UNLOCK(&disp->lock);
		isc_event_free(&ev_in);
		return;
	}

	/*
	 * If this is from a blackholed address, drop it.
	 */
	isc_netaddr_fromsockaddr(&netaddr, &ev->address);
	if (disp->mgr->blackhole != NULL &&
	    dns_acl_match(&netaddr, NULL, disp->mgr->blackhole,
			  NULL, &match, NULL) == ISC_R_SUCCESS &&
	    match > 0)
	{
		if (isc_log_wouldlog(dns_lctx, LVL(10))) {
			char netaddrstr[ISC_NETADDR_FORMATSIZE];
			isc_netaddr_format(&netaddr, netaddrstr,
					   sizeof(netaddrstr));
			dispatch_log(disp, LVL(10),
				     "blackholed packet from %s",
				     netaddrstr);
		}
		free_buffer(disp, ev->region.base, ev->region.length);
		goto restart;
	}

	/*
	 * Peek into the buffer to see what we can see.
	 */
	isc_buffer_init(&source, ev->region.base, ev->region.length);
	isc_buffer_add(&source, ev->n);
	dres = dns_message_peekheader(&source, &id, &flags);
	if (dres != ISC_R_SUCCESS) {
		free_buffer(disp, ev->region.base, ev->region.length);
		dispatch_log(disp, LVL(10), "got garbage packet");
		goto restart;
	}

	dispatch_log(disp, LVL(92),
		     "got valid DNS message header, /QR %c, id %u",
		     ((flags & DNS_MESSAGEFLAG_QR) ? '1' : '0'), id);

	/*
	 * Look at flags.  If query, drop it. If response,
	 * look to see where it goes.
	 */
	queue_response = ISC_FALSE;
	if ((flags & DNS_MESSAGEFLAG_QR) == 0) {
		/* query */
		free_buffer(disp, ev->region.base, ev->region.length);
		goto restart;
	}

	/*
	 * Search for the corresponding response.  If we are using an exclusive
	 * socket, we've already identified it and we can skip the search; but
	 * the ID and the address must match the expected ones.
	 */
	if (resp == NULL) {
		bucket = dns_hash(qid, &ev->address, id, disp->localport);
		LOCK(&qid->lock);
		qidlocked = ISC_TRUE;
		resp = entry_search(qid, &ev->address, id, disp->localport,
				    bucket);
		dispatch_log(disp, LVL(90),
			     "search for response in bucket %d: %s",
			     bucket, (resp == NULL ? "not found" : "found"));

		if (resp == NULL) {
			inc_stats(mgr, dns_resstatscounter_mismatch);
			free_buffer(disp, ev->region.base, ev->region.length);
			goto unlock;
		}
	} else if (resp->id != id || !isc_sockaddr_equal(&ev->address,
							 &resp->host)) {
		dispatch_log(disp, LVL(90),
			     "response to an exclusive socket doesn't match");
		inc_stats(mgr, dns_resstatscounter_mismatch);
		free_buffer(disp, ev->region.base, ev->region.length);
		goto unlock;
	}

	/*
	 * Now that we have the original dispatch the query was sent
	 * from check that the address and port the response was
	 * sent to make sense.
	 */
	if (disp != resp->disp) {
		isc_sockaddr_t a1;
		isc_sockaddr_t a2;

		/*
		 * Check that the socket types and ports match.
		 */
		if (disp->socktype != resp->disp->socktype ||
		    isc_sockaddr_getport(&disp->local) !=
		    isc_sockaddr_getport(&resp->disp->local)) {
			free_buffer(disp, ev->region.base, ev->region.length);
			goto unlock;
		}

		/*
		 * If both dispatches are bound to an address then fail as
		 * the addresses can't be equal (enforced by the IP stack).
		 *
		 * Note under Linux a packet can be sent out via IPv4 socket
		 * and the response be received via a IPv6 socket.
		 *
		 * Requests sent out via IPv6 should always come back in
		 * via IPv6.
		 */
		if (isc_sockaddr_pf(&resp->disp->local) == PF_INET6 &&
		    isc_sockaddr_pf(&disp->local) != PF_INET6) {
			free_buffer(disp, ev->region.base, ev->region.length);
			goto unlock;
		}
		isc_sockaddr_anyofpf(&a1, isc_sockaddr_pf(&resp->disp->local));
		isc_sockaddr_anyofpf(&a2, isc_sockaddr_pf(&disp->local));
		if (!isc_sockaddr_eqaddr(&a1, &resp->disp->local) &&
		    !isc_sockaddr_eqaddr(&a2, &disp->local)) {
			free_buffer(disp, ev->region.base, ev->region.length);
			goto unlock;
		}
	}

  sendresponse:
	queue_response = resp->item_out;
	rev = allocate_event(resp->disp);
	if (rev == NULL) {
		free_buffer(disp, ev->region.base, ev->region.length);
		goto unlock;
	}

	/*
	 * At this point, rev contains the event we want to fill in, and
	 * resp contains the information on the place to send it to.
	 * Send the event off.
	 */
	isc_buffer_init(&rev->buffer, ev->region.base, ev->region.length);
	isc_buffer_add(&rev->buffer, ev->n);
	rev->result = ev->result;
	rev->id = id;
	rev->addr = ev->address;
	rev->pktinfo = ev->pktinfo;
	rev->attributes = ev->attributes;
	if (queue_response) {
		ISC_LIST_APPEND(resp->items, rev, ev_link);
	} else {
		ISC_EVENT_INIT(rev, sizeof(*rev), 0, NULL,
			       DNS_EVENT_DISPATCH,
			       resp->action, resp->arg, resp, NULL, NULL);
		request_log(disp, resp, LVL(90),
			    "[a] Sent event %p buffer %p len %d to task %p",
			    rev, rev->buffer.base, rev->buffer.length,
			    resp->task);
		resp->item_out = ISC_TRUE;
		isc_task_send(resp->task, ISC_EVENT_PTR(&rev));
	}
 unlock:
	if (qidlocked)
		UNLOCK(&qid->lock);

	/*
	 * Restart recv() to get the next packet.
	 */
 restart:
	result = startrecv(disp, dispsock);
	if (result != ISC_R_SUCCESS && dispsock != NULL) {
		/*
		 * XXX: wired. There seems to be no recovery process other than
		 * deactivate this socket anyway (since we cannot start
		 * receiving, we won't be able to receive a cancel event
		 * from the user).
		 */
		deactivate_dispsocket(disp, dispsock);
	}
	UNLOCK(&disp->lock);

	isc_event_free(&ev_in);
}

/*
 * General flow:
 *
 * If I/O result == CANCELED, EOF, or error, notify everyone as the
 * various queues drain.
 *
 * If query, restart.
 *
 * If response:
 *	Allocate event, fill in details.
 *		If cannot allocate, restart.
 *	find target.  If not found, restart.
 *	if event queue is not empty, queue.  else, send.
 *	restart.
 */
static void
tcp_recv(isc_task_t *task, isc_event_t *ev_in) {
	dns_dispatch_t *disp = ev_in->ev_arg;
	dns_tcpmsg_t *tcpmsg = &disp->tcpmsg;
	dns_messageid_t id;
	isc_result_t dres;
	unsigned int flags;
	dns_dispentry_t *resp;
	dns_dispatchevent_t *rev;
	unsigned int bucket;
	isc_boolean_t killit;
	isc_boolean_t queue_response;
	dns_qid_t *qid;
	int level;
	char buf[ISC_SOCKADDR_FORMATSIZE];

	UNUSED(task);

	REQUIRE(VALID_DISPATCH(disp));

	qid = disp->qid;

	dispatch_log(disp, LVL(90),
		     "got TCP packet: requests %d, buffers %d, recvs %d",
		     disp->requests, disp->tcpbuffers, disp->recv_pending);

	LOCK(&disp->lock);

	INSIST(disp->recv_pending != 0);
	disp->recv_pending = 0;

	if (disp->refcount == 0) {
		/*
		 * This dispatcher is shutting down.  Force cancelation.
		 */
		tcpmsg->result = ISC_R_CANCELED;
	}

	if (tcpmsg->result != ISC_R_SUCCESS) {
		switch (tcpmsg->result) {
		case ISC_R_CANCELED:
			break;

		case ISC_R_EOF:
			dispatch_log(disp, LVL(90), "shutting down on EOF");
			do_cancel(disp);
			break;

		case ISC_R_CONNECTIONRESET:
			level = ISC_LOG_INFO;
			goto logit;

		default:
			level = ISC_LOG_ERROR;
		logit:
			isc_sockaddr_format(&tcpmsg->address, buf, sizeof(buf));
			dispatch_log(disp, level, "shutting down due to TCP "
				     "receive error: %s: %s", buf,
				     isc_result_totext(tcpmsg->result));
			do_cancel(disp);
			break;
		}

		/*
		 * The event is statically allocated in the tcpmsg
		 * structure, and destroy_disp() frees the tcpmsg, so we must
		 * free the event *before* calling destroy_disp().
		 */
		isc_event_free(&ev_in);

		disp->shutting_down = 1;
		disp->shutdown_why = tcpmsg->result;

		/*
		 * If the recv() was canceled pass the word on.
		 */
		killit = destroy_disp_ok(disp);
		UNLOCK(&disp->lock);
		if (killit)
			isc_task_send(disp->task[0], &disp->ctlevent);
		return;
	}

	dispatch_log(disp, LVL(90), "result %d, length == %d, addr = %p",
		     tcpmsg->result,
		     tcpmsg->buffer.length, tcpmsg->buffer.base);

	/*
	 * Peek into the buffer to see what we can see.
	 */
	dres = dns_message_peekheader(&tcpmsg->buffer, &id, &flags);
	if (dres != ISC_R_SUCCESS) {
		dispatch_log(disp, LVL(10), "got garbage packet");
		goto restart;
	}

	dispatch_log(disp, LVL(92),
		     "got valid DNS message header, /QR %c, id %u",
		     ((flags & DNS_MESSAGEFLAG_QR) ? '1' : '0'), id);

	/*
	 * Allocate an event to send to the query or response client, and
	 * allocate a new buffer for our use.
	 */

	/*
	 * Look at flags.  If query, drop it. If response,
	 * look to see where it goes.
	 */
	queue_response = ISC_FALSE;
	if ((flags & DNS_MESSAGEFLAG_QR) == 0) {
		/*
		 * Query.
		 */
		goto restart;
	}

	/*
	 * Response.
	 */
	bucket = dns_hash(qid, &tcpmsg->address, id, disp->localport);
	LOCK(&qid->lock);
	resp = entry_search(qid, &tcpmsg->address, id, disp->localport, bucket);
	dispatch_log(disp, LVL(90),
		     "search for response in bucket %d: %s",
		     bucket, (resp == NULL ? "not found" : "found"));

	if (resp == NULL)
		goto unlock;
	queue_response = resp->item_out;
	rev = allocate_event(disp);
	if (rev == NULL)
		goto unlock;

	/*
	 * At this point, rev contains the event we want to fill in, and
	 * resp contains the information on the place to send it to.
	 * Send the event off.
	 */
	dns_tcpmsg_keepbuffer(tcpmsg, &rev->buffer);
	disp->tcpbuffers++;
	rev->result = ISC_R_SUCCESS;
	rev->id = id;
	rev->addr = tcpmsg->address;
	if (queue_response) {
		ISC_LIST_APPEND(resp->items, rev, ev_link);
	} else {
		ISC_EVENT_INIT(rev, sizeof(*rev), 0, NULL, DNS_EVENT_DISPATCH,
			       resp->action, resp->arg, resp, NULL, NULL);
		request_log(disp, resp, LVL(90),
			    "[b] Sent event %p buffer %p len %d to task %p",
			    rev, rev->buffer.base, rev->buffer.length,
			    resp->task);
		resp->item_out = ISC_TRUE;
		isc_task_send(resp->task, ISC_EVENT_PTR(&rev));
	}
 unlock:
	UNLOCK(&qid->lock);

	/*
	 * Restart recv() to get the next packet.
	 */
 restart:
	(void)startrecv(disp, NULL);

	UNLOCK(&disp->lock);

	isc_event_free(&ev_in);
}

/*
 * disp must be locked.
 */
static isc_result_t
startrecv(dns_dispatch_t *disp, dispsocket_t *dispsock) {
	isc_result_t res;
	isc_region_t region;
	isc_socket_t *socket;

	if (disp->shutting_down == 1)
		return (ISC_R_SUCCESS);

	if ((disp->attributes & DNS_DISPATCHATTR_NOLISTEN) != 0)
		return (ISC_R_SUCCESS);

	if (disp->recv_pending != 0 && dispsock == NULL)
		return (ISC_R_SUCCESS);

	if (disp->mgr->buffers >= disp->mgr->maxbuffers)
		return (ISC_R_NOMEMORY);

	if ((disp->attributes & DNS_DISPATCHATTR_EXCLUSIVE) != 0 &&
	    dispsock == NULL)
		return (ISC_R_SUCCESS);

	if (dispsock != NULL)
		socket = dispsock->socket;
	else
		socket = disp->socket;
	INSIST(socket != NULL);

	switch (disp->socktype) {
		/*
		 * UDP reads are always maximal.
		 */
	case isc_sockettype_udp:
		region.length = disp->mgr->buffersize;
		region.base = allocate_udp_buffer(disp);
		if (region.base == NULL)
			return (ISC_R_NOMEMORY);
		if (dispsock != NULL) {
			res = isc_socket_recv(socket, &region, 1,
					      dispsock->task, udp_exrecv,
					      dispsock);
			if (res != ISC_R_SUCCESS) {
				free_buffer(disp, region.base, region.length);
				return (res);
			}
		} else {
			res = isc_socket_recv(socket, &region, 1,
					      disp->task[0], udp_shrecv, disp);
			if (res != ISC_R_SUCCESS) {
				free_buffer(disp, region.base, region.length);
				disp->shutdown_why = res;
				disp->shutting_down = 1;
				do_cancel(disp);
				return (ISC_R_SUCCESS); /* recover by cancel */
			}
			INSIST(disp->recv_pending == 0);
			disp->recv_pending = 1;
		}
		break;

	case isc_sockettype_tcp:
		res = dns_tcpmsg_readmessage(&disp->tcpmsg, disp->task[0],
					     tcp_recv, disp);
		if (res != ISC_R_SUCCESS) {
			disp->shutdown_why = res;
			disp->shutting_down = 1;
			do_cancel(disp);
			return (ISC_R_SUCCESS); /* recover by cancel */
		}
		INSIST(disp->recv_pending == 0);
		disp->recv_pending = 1;
		break;
	default:
		INSIST(0);
		break;
	}

	return (ISC_R_SUCCESS);
}

/*
 * Mgr must be locked when calling this function.
 */
static isc_boolean_t
destroy_mgr_ok(dns_dispatchmgr_t *mgr) {
	mgr_log(mgr, LVL(90),
		"destroy_mgr_ok: shuttingdown=%d, listnonempty=%d, "
		"epool=%d, rpool=%d, dpool=%d",
		MGR_IS_SHUTTINGDOWN(mgr), !ISC_LIST_EMPTY(mgr->list),
		isc_mempool_getallocated(mgr->epool),
		isc_mempool_getallocated(mgr->rpool),
		isc_mempool_getallocated(mgr->dpool));
	if (!MGR_IS_SHUTTINGDOWN(mgr))
		return (ISC_FALSE);
	if (!ISC_LIST_EMPTY(mgr->list))
		return (ISC_FALSE);
	if (isc_mempool_getallocated(mgr->epool) != 0)
		return (ISC_FALSE);
	if (isc_mempool_getallocated(mgr->rpool) != 0)
		return (ISC_FALSE);
	if (isc_mempool_getallocated(mgr->dpool) != 0)
		return (ISC_FALSE);

	return (ISC_TRUE);
}

/*
 * Mgr must be unlocked when calling this function.
 */
static void
destroy_mgr(dns_dispatchmgr_t **mgrp) {
	isc_mem_t *mctx;
	dns_dispatchmgr_t *mgr;

	mgr = *mgrp;
	*mgrp = NULL;

	mctx = mgr->mctx;

	mgr->magic = 0;
	mgr->mctx = NULL;
	DESTROYLOCK(&mgr->lock);
	mgr->state = 0;

	DESTROYLOCK(&mgr->arc4_lock);

	isc_mempool_destroy(&mgr->epool);
	isc_mempool_destroy(&mgr->rpool);
	isc_mempool_destroy(&mgr->dpool);
	isc_mempool_destroy(&mgr->bpool);
	isc_mempool_destroy(&mgr->spool);

	DESTROYLOCK(&mgr->pool_lock);

	if (mgr->entropy != NULL)
		isc_entropy_detach(&mgr->entropy);
	if (mgr->qid != NULL)
		qid_destroy(mctx, &mgr->qid);

	DESTROYLOCK(&mgr->buffer_lock);

	if (mgr->blackhole != NULL)
		dns_acl_detach(&mgr->blackhole);

	if (mgr->stats != NULL)
		isc_stats_detach(&mgr->stats);

	if (mgr->v4ports != NULL) {
		isc_mem_put(mctx, mgr->v4ports,
			    mgr->nv4ports * sizeof(in_port_t));
	}
	if (mgr->v6ports != NULL) {
		isc_mem_put(mctx, mgr->v6ports,
			    mgr->nv6ports * sizeof(in_port_t));
	}
	isc_mem_put(mctx, mgr, sizeof(dns_dispatchmgr_t));
	isc_mem_detach(&mctx);
}

static isc_result_t
open_socket(isc_socketmgr_t *mgr, isc_sockaddr_t *local,
	    unsigned int options, isc_socket_t **sockp)
{
	isc_socket_t *sock;
	isc_result_t result;

	sock = *sockp;
	if (sock == NULL) {
		result = isc_socket_create(mgr, isc_sockaddr_pf(local),
					   isc_sockettype_udp, &sock);
		if (result != ISC_R_SUCCESS)
			return (result);
		isc_socket_setname(sock, "dispatcher", NULL);
	} else {
		result = isc_socket_open(sock);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

#ifndef ISC_ALLOW_MAPPED
	isc_socket_ipv6only(sock, ISC_TRUE);
#endif
	result = isc_socket_bind(sock, local, options);
	if (result != ISC_R_SUCCESS) {
		if (*sockp == NULL)
			isc_socket_detach(&sock);
		else
			isc_socket_close(sock);
		return (result);
	}

	*sockp = sock;
	return (ISC_R_SUCCESS);
}

/*%
 * Create a temporary port list to set the initial default set of dispatch
 * ports: [1024, 65535].  This is almost meaningless as the application will
 * normally set the ports explicitly, but is provided to fill some minor corner
 * cases.
 */
static isc_result_t
create_default_portset(isc_mem_t *mctx, isc_portset_t **portsetp) {
	isc_result_t result;

	result = isc_portset_create(mctx, portsetp);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_portset_addrange(*portsetp, 1024, 65535);

	return (ISC_R_SUCCESS);
}

/*
 * Publics.
 */

isc_result_t
dns_dispatchmgr_create(isc_mem_t *mctx, isc_entropy_t *entropy,
		       dns_dispatchmgr_t **mgrp)
{
	dns_dispatchmgr_t *mgr;
	isc_result_t result;
	isc_portset_t *v4portset = NULL;
	isc_portset_t *v6portset = NULL;

	REQUIRE(mctx != NULL);
	REQUIRE(mgrp != NULL && *mgrp == NULL);

	mgr = isc_mem_get(mctx, sizeof(dns_dispatchmgr_t));
	if (mgr == NULL)
		return (ISC_R_NOMEMORY);

	mgr->mctx = NULL;
	isc_mem_attach(mctx, &mgr->mctx);

	mgr->blackhole = NULL;
	mgr->stats = NULL;

	result = isc_mutex_init(&mgr->lock);
	if (result != ISC_R_SUCCESS)
		goto deallocate;

	result = isc_mutex_init(&mgr->arc4_lock);
	if (result != ISC_R_SUCCESS)
		goto kill_lock;

	result = isc_mutex_init(&mgr->buffer_lock);
	if (result != ISC_R_SUCCESS)
		goto kill_arc4_lock;

	result = isc_mutex_init(&mgr->pool_lock);
	if (result != ISC_R_SUCCESS)
		goto kill_buffer_lock;

	mgr->epool = NULL;
	if (isc_mempool_create(mgr->mctx, sizeof(dns_dispatchevent_t),
			       &mgr->epool) != ISC_R_SUCCESS) {
		result = ISC_R_NOMEMORY;
		goto kill_pool_lock;
	}

	mgr->rpool = NULL;
	if (isc_mempool_create(mgr->mctx, sizeof(dns_dispentry_t),
			       &mgr->rpool) != ISC_R_SUCCESS) {
		result = ISC_R_NOMEMORY;
		goto kill_epool;
	}

	mgr->dpool = NULL;
	if (isc_mempool_create(mgr->mctx, sizeof(dns_dispatch_t),
			       &mgr->dpool) != ISC_R_SUCCESS) {
		result = ISC_R_NOMEMORY;
		goto kill_rpool;
	}

	isc_mempool_setname(mgr->epool, "dispmgr_epool");
	isc_mempool_setfreemax(mgr->epool, 1024);
	isc_mempool_associatelock(mgr->epool, &mgr->pool_lock);

	isc_mempool_setname(mgr->rpool, "dispmgr_rpool");
	isc_mempool_setfreemax(mgr->rpool, 1024);
	isc_mempool_associatelock(mgr->rpool, &mgr->pool_lock);

	isc_mempool_setname(mgr->dpool, "dispmgr_dpool");
	isc_mempool_setfreemax(mgr->dpool, 1024);
	isc_mempool_associatelock(mgr->dpool, &mgr->pool_lock);

	mgr->buffers = 0;
	mgr->buffersize = 0;
	mgr->maxbuffers = 0;
	mgr->bpool = NULL;
	mgr->spool = NULL;
	mgr->entropy = NULL;
	mgr->qid = NULL;
	mgr->state = 0;
	ISC_LIST_INIT(mgr->list);
	mgr->v4ports = NULL;
	mgr->v6ports = NULL;
	mgr->nv4ports = 0;
	mgr->nv6ports = 0;
	mgr->magic = DNS_DISPATCHMGR_MAGIC;

	result = create_default_portset(mctx, &v4portset);
	if (result == ISC_R_SUCCESS) {
		result = create_default_portset(mctx, &v6portset);
		if (result == ISC_R_SUCCESS) {
			result = dns_dispatchmgr_setavailports(mgr,
							       v4portset,
							       v6portset);
		}
	}
	if (v4portset != NULL)
		isc_portset_destroy(mctx, &v4portset);
	if (v6portset != NULL)
		isc_portset_destroy(mctx, &v6portset);
	if (result != ISC_R_SUCCESS)
		goto kill_dpool;

	if (entropy != NULL)
		isc_entropy_attach(entropy, &mgr->entropy);

	dispatch_arc4init(&mgr->arc4ctx, mgr->entropy, &mgr->arc4_lock);

	*mgrp = mgr;
	return (ISC_R_SUCCESS);

 kill_dpool:
	isc_mempool_destroy(&mgr->dpool);
 kill_rpool:
	isc_mempool_destroy(&mgr->rpool);
 kill_epool:
	isc_mempool_destroy(&mgr->epool);
 kill_pool_lock:
	DESTROYLOCK(&mgr->pool_lock);
 kill_buffer_lock:
	DESTROYLOCK(&mgr->buffer_lock);
 kill_arc4_lock:
	DESTROYLOCK(&mgr->arc4_lock);
 kill_lock:
	DESTROYLOCK(&mgr->lock);
 deallocate:
	isc_mem_put(mctx, mgr, sizeof(dns_dispatchmgr_t));
	isc_mem_detach(&mctx);

	return (result);
}

void
dns_dispatchmgr_setblackhole(dns_dispatchmgr_t *mgr, dns_acl_t *blackhole) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	if (mgr->blackhole != NULL)
		dns_acl_detach(&mgr->blackhole);
	dns_acl_attach(blackhole, &mgr->blackhole);
}

dns_acl_t *
dns_dispatchmgr_getblackhole(dns_dispatchmgr_t *mgr) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	return (mgr->blackhole);
}

void
dns_dispatchmgr_setblackportlist(dns_dispatchmgr_t *mgr,
				 dns_portlist_t *portlist)
{
	REQUIRE(VALID_DISPATCHMGR(mgr));
	UNUSED(portlist);

	/* This function is deprecated: use dns_dispatchmgr_setavailports(). */
	return;
}

dns_portlist_t *
dns_dispatchmgr_getblackportlist(dns_dispatchmgr_t *mgr) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	return (NULL);		/* this function is deprecated */
}

isc_result_t
dns_dispatchmgr_setavailports(dns_dispatchmgr_t *mgr, isc_portset_t *v4portset,
			      isc_portset_t *v6portset)
{
	in_port_t *v4ports, *v6ports, p;
	unsigned int nv4ports, nv6ports, i4, i6;

	REQUIRE(VALID_DISPATCHMGR(mgr));

	nv4ports = isc_portset_nports(v4portset);
	nv6ports = isc_portset_nports(v6portset);

	v4ports = NULL;
	if (nv4ports != 0) {
		v4ports = isc_mem_get(mgr->mctx, sizeof(in_port_t) * nv4ports);
		if (v4ports == NULL)
			return (ISC_R_NOMEMORY);
	}
	v6ports = NULL;
	if (nv6ports != 0) {
		v6ports = isc_mem_get(mgr->mctx, sizeof(in_port_t) * nv6ports);
		if (v6ports == NULL) {
			if (v4ports != NULL) {
				isc_mem_put(mgr->mctx, v4ports,
					    sizeof(in_port_t) *
					    isc_portset_nports(v4portset));
			}
			return (ISC_R_NOMEMORY);
		}
	}

	p = 0;
	i4 = 0;
	i6 = 0;
	do {
		if (isc_portset_isset(v4portset, p)) {
			INSIST(i4 < nv4ports);
			v4ports[i4++] = p;
		}
		if (isc_portset_isset(v6portset, p)) {
			INSIST(i6 < nv6ports);
			v6ports[i6++] = p;
		}
	} while (p++ < 65535);
	INSIST(i4 == nv4ports && i6 == nv6ports);

	PORTBUFLOCK(mgr);
	if (mgr->v4ports != NULL) {
		isc_mem_put(mgr->mctx, mgr->v4ports,
			    mgr->nv4ports * sizeof(in_port_t));
	}
	mgr->v4ports = v4ports;
	mgr->nv4ports = nv4ports;

	if (mgr->v6ports != NULL) {
		isc_mem_put(mgr->mctx, mgr->v6ports,
			    mgr->nv6ports * sizeof(in_port_t));
	}
	mgr->v6ports = v6ports;
	mgr->nv6ports = nv6ports;
	PORTBUFUNLOCK(mgr);

	return (ISC_R_SUCCESS);
}

static isc_result_t
dns_dispatchmgr_setudp(dns_dispatchmgr_t *mgr,
		       unsigned int buffersize, unsigned int maxbuffers,
		       unsigned int maxrequests, unsigned int buckets,
		       unsigned int increment)
{
	isc_result_t result;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(buffersize >= 512 && buffersize < (64 * 1024));
	REQUIRE(maxbuffers > 0);
	REQUIRE(buckets < 2097169);  /* next prime > 65536 * 32 */
	REQUIRE(increment > buckets);

	/*
	 * Keep some number of items around.  This should be a config
	 * option.  For now, keep 8, but later keep at least two even
	 * if the caller wants less.  This allows us to ensure certain
	 * things, like an event can be "freed" and the next allocation
	 * will always succeed.
	 *
	 * Note that if limits are placed on anything here, we use one
	 * event internally, so the actual limit should be "wanted + 1."
	 *
	 * XXXMLG
	 */

	if (maxbuffers < 8)
		maxbuffers = 8;

	LOCK(&mgr->buffer_lock);

	/* Create or adjust buffer pool */
	if (mgr->bpool != NULL) {
		isc_mempool_setmaxalloc(mgr->bpool, maxbuffers);
		mgr->maxbuffers = maxbuffers;
	} else {
		result = isc_mempool_create(mgr->mctx, buffersize, &mgr->bpool);
		if (result != ISC_R_SUCCESS) {
			UNLOCK(&mgr->buffer_lock);
			return (result);
		}
		isc_mempool_setname(mgr->bpool, "dispmgr_bpool");
		isc_mempool_setmaxalloc(mgr->bpool, maxbuffers);
		isc_mempool_associatelock(mgr->bpool, &mgr->pool_lock);
	}

	/* Create or adjust socket pool */
	if (mgr->spool != NULL) {
		isc_mempool_setmaxalloc(mgr->spool, DNS_DISPATCH_POOLSOCKS * 2);
		UNLOCK(&mgr->buffer_lock);
		return (ISC_R_SUCCESS);
	}
	result = isc_mempool_create(mgr->mctx, sizeof(dispsocket_t),
				    &mgr->spool);
	if (result != ISC_R_SUCCESS) {
		UNLOCK(&mgr->buffer_lock);
		goto cleanup;
	}
	isc_mempool_setname(mgr->spool, "dispmgr_spool");
	isc_mempool_setmaxalloc(mgr->spool, maxrequests);
	isc_mempool_associatelock(mgr->spool, &mgr->pool_lock);

	result = qid_allocate(mgr, buckets, increment, &mgr->qid, ISC_TRUE);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	mgr->buffersize = buffersize;
	mgr->maxbuffers = maxbuffers;
	UNLOCK(&mgr->buffer_lock);
	return (ISC_R_SUCCESS);

 cleanup:
	isc_mempool_destroy(&mgr->bpool);
	if (mgr->spool != NULL)
		isc_mempool_destroy(&mgr->spool);
	UNLOCK(&mgr->buffer_lock);
	return (result);
}

void
dns_dispatchmgr_destroy(dns_dispatchmgr_t **mgrp) {
	dns_dispatchmgr_t *mgr;
	isc_boolean_t killit;

	REQUIRE(mgrp != NULL);
	REQUIRE(VALID_DISPATCHMGR(*mgrp));

	mgr = *mgrp;
	*mgrp = NULL;

	LOCK(&mgr->lock);
	mgr->state |= MGR_SHUTTINGDOWN;

	killit = destroy_mgr_ok(mgr);
	UNLOCK(&mgr->lock);

	mgr_log(mgr, LVL(90), "destroy: killit=%d", killit);

	if (killit)
		destroy_mgr(&mgr);
}

void
dns_dispatchmgr_setstats(dns_dispatchmgr_t *mgr, isc_stats_t *stats) {
	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(ISC_LIST_EMPTY(mgr->list));
	REQUIRE(mgr->stats == NULL);

	isc_stats_attach(stats, &mgr->stats);
}

static int
port_cmp(const void *key, const void *ent) {
	in_port_t p1 = *(const in_port_t *)key;
	in_port_t p2 = *(const in_port_t *)ent;

	if (p1 < p2)
		return (-1);
	else if (p1 == p2)
		return (0);
	else
		return (1);
}

static isc_boolean_t
portavailable(dns_dispatchmgr_t *mgr, isc_socket_t *sock,
	      isc_sockaddr_t *sockaddrp)
{
	isc_sockaddr_t sockaddr;
	isc_result_t result;
	in_port_t *ports, port;
	unsigned int nports;
	isc_boolean_t available = ISC_FALSE;

	REQUIRE(sock != NULL || sockaddrp != NULL);

	PORTBUFLOCK(mgr);
	if (sock != NULL) {
		sockaddrp = &sockaddr;
		result = isc_socket_getsockname(sock, sockaddrp);
		if (result != ISC_R_SUCCESS)
			goto unlock;
	}

	if (isc_sockaddr_pf(sockaddrp) == AF_INET) {
		ports = mgr->v4ports;
		nports = mgr->nv4ports;
	} else {
		ports = mgr->v6ports;
		nports = mgr->nv6ports;
	}
	if (ports == NULL)
		goto unlock;

	port = isc_sockaddr_getport(sockaddrp);
	if (bsearch(&port, ports, nports, sizeof(in_port_t), port_cmp) != NULL)
		available = ISC_TRUE;

unlock:
	PORTBUFUNLOCK(mgr);
	return (available);
}

#define ATTRMATCH(_a1, _a2, _mask) (((_a1) & (_mask)) == ((_a2) & (_mask)))

static isc_boolean_t
local_addr_match(dns_dispatch_t *disp, isc_sockaddr_t *addr) {
	isc_sockaddr_t sockaddr;
	isc_result_t result;

	REQUIRE(disp->socket != NULL);

	if (addr == NULL)
		return (ISC_TRUE);

	/*
	 * Don't match wildcard ports unless the port is available in the
	 * current configuration.
	 */
	if (isc_sockaddr_getport(addr) == 0 &&
	    isc_sockaddr_getport(&disp->local) == 0 &&
	    !portavailable(disp->mgr, disp->socket, NULL)) {
		return (ISC_FALSE);
	}

	/*
	 * Check if we match the binding <address,port>.
	 * Wildcard ports match/fail here.
	 */
	if (isc_sockaddr_equal(&disp->local, addr))
		return (ISC_TRUE);
	if (isc_sockaddr_getport(addr) == 0)
		return (ISC_FALSE);

	/*
	 * Check if we match a bound wildcard port <address,port>.
	 */
	if (!isc_sockaddr_eqaddr(&disp->local, addr))
		return (ISC_FALSE);
	result = isc_socket_getsockname(disp->socket, &sockaddr);
	if (result != ISC_R_SUCCESS)
		return (ISC_FALSE);

	return (isc_sockaddr_equal(&sockaddr, addr));
}

/*
 * Requires mgr be locked.
 *
 * No dispatcher can be locked by this thread when calling this function.
 *
 *
 * NOTE:
 *	If a matching dispatcher is found, it is locked after this function
 *	returns, and must be unlocked by the caller.
 */
static isc_result_t
dispatch_find(dns_dispatchmgr_t *mgr, isc_sockaddr_t *local,
	      unsigned int attributes, unsigned int mask,
	      dns_dispatch_t **dispp)
{
	dns_dispatch_t *disp;
	isc_result_t result;

	/*
	 * Make certain that we will not match a private or exclusive dispatch.
	 */
	attributes &= ~(DNS_DISPATCHATTR_PRIVATE|DNS_DISPATCHATTR_EXCLUSIVE);
	mask |= (DNS_DISPATCHATTR_PRIVATE|DNS_DISPATCHATTR_EXCLUSIVE);

	disp = ISC_LIST_HEAD(mgr->list);
	while (disp != NULL) {
		LOCK(&disp->lock);
		if ((disp->shutting_down == 0)
		    && ATTRMATCH(disp->attributes, attributes, mask)
		    && local_addr_match(disp, local))
			break;
		UNLOCK(&disp->lock);
		disp = ISC_LIST_NEXT(disp, link);
	}

	if (disp == NULL) {
		result = ISC_R_NOTFOUND;
		goto out;
	}

	*dispp = disp;
	result = ISC_R_SUCCESS;
 out:

	return (result);
}

static isc_result_t
qid_allocate(dns_dispatchmgr_t *mgr, unsigned int buckets,
	     unsigned int increment, dns_qid_t **qidp,
	     isc_boolean_t needsocktable)
{
	dns_qid_t *qid;
	unsigned int i;
	isc_result_t result;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(buckets < 2097169);  /* next prime > 65536 * 32 */
	REQUIRE(increment > buckets);
	REQUIRE(qidp != NULL && *qidp == NULL);

	qid = isc_mem_get(mgr->mctx, sizeof(*qid));
	if (qid == NULL)
		return (ISC_R_NOMEMORY);

	qid->qid_table = isc_mem_get(mgr->mctx,
				     buckets * sizeof(dns_displist_t));
	if (qid->qid_table == NULL) {
		isc_mem_put(mgr->mctx, qid, sizeof(*qid));
		return (ISC_R_NOMEMORY);
	}

	qid->sock_table = NULL;
	if (needsocktable) {
		qid->sock_table = isc_mem_get(mgr->mctx, buckets *
					      sizeof(dispsocketlist_t));
		if (qid->sock_table == NULL) {
			isc_mem_put(mgr->mctx, qid, sizeof(*qid));
			isc_mem_put(mgr->mctx, qid->qid_table,
				    buckets * sizeof(dns_displist_t));
			return (ISC_R_NOMEMORY);
		}
	}

	result = isc_mutex_init(&qid->lock);
	if (result != ISC_R_SUCCESS) {
		if (qid->sock_table != NULL) {
			isc_mem_put(mgr->mctx, qid->sock_table,
				    buckets * sizeof(dispsocketlist_t));
		}
		isc_mem_put(mgr->mctx, qid->qid_table,
			    buckets * sizeof(dns_displist_t));
		isc_mem_put(mgr->mctx, qid, sizeof(*qid));
		return (result);
	}

	for (i = 0; i < buckets; i++) {
		ISC_LIST_INIT(qid->qid_table[i]);
		if (qid->sock_table != NULL)
			ISC_LIST_INIT(qid->sock_table[i]);
	}

	qid->qid_nbuckets = buckets;
	qid->qid_increment = increment;
	qid->magic = QID_MAGIC;
	*qidp = qid;
	return (ISC_R_SUCCESS);
}

static void
qid_destroy(isc_mem_t *mctx, dns_qid_t **qidp) {
	dns_qid_t *qid;

	REQUIRE(qidp != NULL);
	qid = *qidp;

	REQUIRE(VALID_QID(qid));

	*qidp = NULL;
	qid->magic = 0;
	isc_mem_put(mctx, qid->qid_table,
		    qid->qid_nbuckets * sizeof(dns_displist_t));
	if (qid->sock_table != NULL) {
		isc_mem_put(mctx, qid->sock_table,
			    qid->qid_nbuckets * sizeof(dispsocketlist_t));
	}
	DESTROYLOCK(&qid->lock);
	isc_mem_put(mctx, qid, sizeof(*qid));
}

/*
 * Allocate and set important limits.
 */
static isc_result_t
dispatch_allocate(dns_dispatchmgr_t *mgr, unsigned int maxrequests,
		  dns_dispatch_t **dispp)
{
	dns_dispatch_t *disp;
	isc_result_t result;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(dispp != NULL && *dispp == NULL);

	/*
	 * Set up the dispatcher, mostly.  Don't bother setting some of
	 * the options that are controlled by tcp vs. udp, etc.
	 */

	disp = isc_mempool_get(mgr->dpool);
	if (disp == NULL)
		return (ISC_R_NOMEMORY);

	disp->magic = 0;
	disp->mgr = mgr;
	disp->maxrequests = maxrequests;
	disp->attributes = 0;
	ISC_LINK_INIT(disp, link);
	disp->refcount = 1;
	disp->recv_pending = 0;
	memset(&disp->local, 0, sizeof(disp->local));
	disp->localport = 0;
	disp->shutting_down = 0;
	disp->shutdown_out = 0;
	disp->connected = 0;
	disp->tcpmsg_valid = 0;
	disp->shutdown_why = ISC_R_UNEXPECTED;
	disp->requests = 0;
	disp->tcpbuffers = 0;
	disp->qid = NULL;
	ISC_LIST_INIT(disp->activesockets);
	ISC_LIST_INIT(disp->inactivesockets);
	disp->nsockets = 0;
	dispatch_arc4init(&disp->arc4ctx, mgr->entropy, NULL);
	disp->port_table = NULL;
	disp->portpool = NULL;

	result = isc_mutex_init(&disp->lock);
	if (result != ISC_R_SUCCESS)
		goto deallocate;

	disp->failsafe_ev = allocate_event(disp);
	if (disp->failsafe_ev == NULL) {
		result = ISC_R_NOMEMORY;
		goto kill_lock;
	}

	disp->magic = DISPATCH_MAGIC;

	*dispp = disp;
	return (ISC_R_SUCCESS);

	/*
	 * error returns
	 */
 kill_lock:
	DESTROYLOCK(&disp->lock);
 deallocate:
	isc_mempool_put(mgr->dpool, disp);

	return (result);
}


/*
 * MUST be unlocked, and not used by anything.
 */
static void
dispatch_free(dns_dispatch_t **dispp)
{
	dns_dispatch_t *disp;
	dns_dispatchmgr_t *mgr;
	int i;

	REQUIRE(VALID_DISPATCH(*dispp));
	disp = *dispp;
	*dispp = NULL;

	mgr = disp->mgr;
	REQUIRE(VALID_DISPATCHMGR(mgr));

	if (disp->tcpmsg_valid) {
		dns_tcpmsg_invalidate(&disp->tcpmsg);
		disp->tcpmsg_valid = 0;
	}

	INSIST(disp->tcpbuffers == 0);
	INSIST(disp->requests == 0);
	INSIST(disp->recv_pending == 0);
	INSIST(ISC_LIST_EMPTY(disp->activesockets));
	INSIST(ISC_LIST_EMPTY(disp->inactivesockets));

	isc_mempool_put(mgr->epool, disp->failsafe_ev);
	disp->failsafe_ev = NULL;

	if (disp->qid != NULL)
		qid_destroy(mgr->mctx, &disp->qid);

	if (disp->port_table != NULL) {
		for (i = 0; i < DNS_DISPATCH_PORTTABLESIZE; i++)
			INSIST(ISC_LIST_EMPTY(disp->port_table[i]));
		isc_mem_put(mgr->mctx, disp->port_table,
			    sizeof(disp->port_table[0]) *
			    DNS_DISPATCH_PORTTABLESIZE);
	}

	if (disp->portpool != NULL)
		isc_mempool_destroy(&disp->portpool);

	disp->mgr = NULL;
	DESTROYLOCK(&disp->lock);
	disp->magic = 0;
	isc_mempool_put(mgr->dpool, disp);
}

isc_result_t
dns_dispatch_createtcp(dns_dispatchmgr_t *mgr, isc_socket_t *sock,
		       isc_taskmgr_t *taskmgr, unsigned int buffersize,
		       unsigned int maxbuffers, unsigned int maxrequests,
		       unsigned int buckets, unsigned int increment,
		       unsigned int attributes, dns_dispatch_t **dispp)
{
	isc_result_t result;
	dns_dispatch_t *disp;

	UNUSED(maxbuffers);
	UNUSED(buffersize);

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(isc_socket_gettype(sock) == isc_sockettype_tcp);
	REQUIRE((attributes & DNS_DISPATCHATTR_TCP) != 0);
	REQUIRE((attributes & DNS_DISPATCHATTR_UDP) == 0);

	attributes |= DNS_DISPATCHATTR_PRIVATE;  /* XXXMLG */

	LOCK(&mgr->lock);

	/*
	 * dispatch_allocate() checks mgr for us.
	 * qid_allocate() checks buckets and increment for us.
	 */
	disp = NULL;
	result = dispatch_allocate(mgr, maxrequests, &disp);
	if (result != ISC_R_SUCCESS) {
		UNLOCK(&mgr->lock);
		return (result);
	}

	result = qid_allocate(mgr, buckets, increment, &disp->qid, ISC_FALSE);
	if (result != ISC_R_SUCCESS)
		goto deallocate_dispatch;

	disp->socktype = isc_sockettype_tcp;
	disp->socket = NULL;
	isc_socket_attach(sock, &disp->socket);

	disp->ntasks = 1;
	disp->task[0] = NULL;
	result = isc_task_create(taskmgr, 0, &disp->task[0]);
	if (result != ISC_R_SUCCESS)
		goto kill_socket;

	disp->ctlevent = isc_event_allocate(mgr->mctx, disp,
					    DNS_EVENT_DISPATCHCONTROL,
					    destroy_disp, disp,
					    sizeof(isc_event_t));
	if (disp->ctlevent == NULL) {
		result = ISC_R_NOMEMORY;
		goto kill_task;
	}

	isc_task_setname(disp->task[0], "tcpdispatch", disp);

	dns_tcpmsg_init(mgr->mctx, disp->socket, &disp->tcpmsg);
	disp->tcpmsg_valid = 1;

	disp->attributes = attributes;

	/*
	 * Append it to the dispatcher list.
	 */
	ISC_LIST_APPEND(mgr->list, disp, link);
	UNLOCK(&mgr->lock);

	mgr_log(mgr, LVL(90), "created TCP dispatcher %p", disp);
	dispatch_log(disp, LVL(90), "created task %p", disp->task[0]);

	*dispp = disp;

	return (ISC_R_SUCCESS);

	/*
	 * Error returns.
	 */
 kill_task:
	isc_task_detach(&disp->task[0]);
 kill_socket:
	isc_socket_detach(&disp->socket);
 deallocate_dispatch:
	dispatch_free(&disp);

	UNLOCK(&mgr->lock);

	return (result);
}

isc_result_t
dns_dispatch_getudp(dns_dispatchmgr_t *mgr, isc_socketmgr_t *sockmgr,
		    isc_taskmgr_t *taskmgr, isc_sockaddr_t *localaddr,
		    unsigned int buffersize,
		    unsigned int maxbuffers, unsigned int maxrequests,
		    unsigned int buckets, unsigned int increment,
		    unsigned int attributes, unsigned int mask,
		    dns_dispatch_t **dispp)
{
	isc_result_t result;
	dns_dispatch_t *disp = NULL;

	REQUIRE(VALID_DISPATCHMGR(mgr));
	REQUIRE(sockmgr != NULL);
	REQUIRE(localaddr != NULL);
	REQUIRE(taskmgr != NULL);
	REQUIRE(buffersize >= 512 && buffersize < (64 * 1024));
	REQUIRE(maxbuffers > 0);
	REQUIRE(buckets < 2097169);  /* next prime > 65536 * 32 */
	REQUIRE(increment > buckets);
	REQUIRE(dispp != NULL && *dispp == NULL);
	REQUIRE((attributes & DNS_DISPATCHATTR_TCP) == 0);

	result = dns_dispatchmgr_setudp(mgr, buffersize, maxbuffers,
					maxrequests, buckets, increment);
	if (result != ISC_R_SUCCESS)
		return (result);

	LOCK(&mgr->lock);

	if ((attributes & DNS_DISPATCHATTR_EXCLUSIVE) != 0) {
		REQUIRE(isc_sockaddr_getport(localaddr) == 0);
		goto createudp;
	}

	/*
	 * See if we have a dispatcher that matches.
	 */
	result = dispatch_find(mgr, localaddr, attributes, mask, &disp);
	if (result == ISC_R_SUCCESS) {
		disp->refcount++;

		if (disp->maxrequests < maxrequests)
			disp->maxrequests = maxrequests;

		if ((disp->attributes & DNS_DISPATCHATTR_NOLISTEN) == 0 &&
		    (attributes & DNS_DISPATCHATTR_NOLISTEN) != 0)
		{
			disp->attributes |= DNS_DISPATCHATTR_NOLISTEN;
			if (disp->recv_pending != 0)
				isc_socket_cancel(disp->socket, disp->task[0],
						  ISC_SOCKCANCEL_RECV);
		}

		UNLOCK(&disp->lock);
		UNLOCK(&mgr->lock);

		*dispp = disp;

		return (ISC_R_SUCCESS);
	}

 createudp:
	/*
	 * Nope, create one.
	 */
	result = dispatch_createudp(mgr, sockmgr, taskmgr, localaddr,
				    maxrequests, attributes, &disp);
	if (result != ISC_R_SUCCESS) {
		UNLOCK(&mgr->lock);
		return (result);
	}

	UNLOCK(&mgr->lock);
	*dispp = disp;
	return (ISC_R_SUCCESS);
}

/*
 * mgr should be locked.
 */

#ifndef DNS_DISPATCH_HELD
#define DNS_DISPATCH_HELD 20U
#endif

static isc_result_t
get_udpsocket(dns_dispatchmgr_t *mgr, dns_dispatch_t *disp,
	      isc_socketmgr_t *sockmgr, isc_sockaddr_t *localaddr,
	      isc_socket_t **sockp)
{
	unsigned int i, j;
	isc_socket_t *held[DNS_DISPATCH_HELD];
	isc_sockaddr_t localaddr_bound;
	isc_socket_t *sock = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	isc_boolean_t anyport;

	INSIST(sockp != NULL && *sockp == NULL);

	localaddr_bound = *localaddr;
	anyport = ISC_TF(isc_sockaddr_getport(localaddr) == 0);

	if (anyport) {
		unsigned int nports;
		in_port_t *ports;

		/*
		 * If no port is specified, we first try to pick up a random
		 * port by ourselves.
		 */
		if (isc_sockaddr_pf(&disp->local) == AF_INET) {
			nports = disp->mgr->nv4ports;
			ports = disp->mgr->v4ports;
		} else {
			nports = disp->mgr->nv6ports;
			ports = disp->mgr->v6ports;
		}
		if (nports == 0)
			return (ISC_R_ADDRNOTAVAIL);

		for (i = 0; i < 1024; i++) {
			in_port_t prt;

			prt = ports[dispatch_arc4uniformrandom(
					DISP_ARC4CTX(disp),
					nports)];
			isc_sockaddr_setport(&localaddr_bound, prt);
			result = open_socket(sockmgr, &localaddr_bound,
					     0, &sock);
			if (result == ISC_R_SUCCESS ||
			    result != ISC_R_ADDRINUSE) {
				disp->localport = prt;
				*sockp = sock;
				return (result);
			}
		}

		/*
		 * If this fails 1024 times, we then ask the kernel for
		 * choosing one.
		 */
	} else {
		/* Allow to reuse address for non-random ports. */
		result = open_socket(sockmgr, localaddr,
				     ISC_SOCKET_REUSEADDRESS, &sock);

		if (result == ISC_R_SUCCESS)
			*sockp = sock;

		return (result);
	}

	memset(held, 0, sizeof(held));
	i = 0;

	for (j = 0; j < 0xffffU; j++) {
		result = open_socket(sockmgr, localaddr, 0, &sock);
		if (result != ISC_R_SUCCESS)
			goto end;
		else if (!anyport)
			break;
		else if (portavailable(mgr, sock, NULL))
			break;
		if (held[i] != NULL)
			isc_socket_detach(&held[i]);
		held[i++] = sock;
		sock = NULL;
		if (i == DNS_DISPATCH_HELD)
			i = 0;
	}
	if (j == 0xffffU) {
		mgr_log(mgr, ISC_LOG_ERROR,
			"avoid-v%s-udp-ports: unable to allocate "
			"an available port",
			isc_sockaddr_pf(localaddr) == AF_INET ? "4" : "6");
		result = ISC_R_FAILURE;
		goto end;
	}
	*sockp = sock;

end:
	for (i = 0; i < DNS_DISPATCH_HELD; i++) {
		if (held[i] != NULL)
			isc_socket_detach(&held[i]);
	}

	return (result);
}

static isc_result_t
dispatch_createudp(dns_dispatchmgr_t *mgr, isc_socketmgr_t *sockmgr,
		   isc_taskmgr_t *taskmgr,
		   isc_sockaddr_t *localaddr,
		   unsigned int maxrequests,
		   unsigned int attributes,
		   dns_dispatch_t **dispp)
{
	isc_result_t result;
	dns_dispatch_t *disp;
	isc_socket_t *sock = NULL;
	int i = 0;

	/*
	 * dispatch_allocate() checks mgr for us.
	 */
	disp = NULL;
	result = dispatch_allocate(mgr, maxrequests, &disp);
	if (result != ISC_R_SUCCESS)
		return (result);

	if ((attributes & DNS_DISPATCHATTR_EXCLUSIVE) == 0) {
		result = get_udpsocket(mgr, disp, sockmgr, localaddr, &sock);
		if (result != ISC_R_SUCCESS)
			goto deallocate_dispatch;
	} else {
		isc_sockaddr_t sa_any;

		/*
		 * For dispatches using exclusive sockets with a specific
		 * source address, we only check if the specified address is
		 * available on the system.  Query sockets will be created later
		 * on demand.
		 */
		isc_sockaddr_anyofpf(&sa_any, isc_sockaddr_pf(localaddr));
		if (!isc_sockaddr_eqaddr(&sa_any, localaddr)) {
			result = open_socket(sockmgr, localaddr, 0, &sock);
			if (sock != NULL)
				isc_socket_detach(&sock);
			if (result != ISC_R_SUCCESS)
				goto deallocate_dispatch;
		}

		disp->port_table = isc_mem_get(mgr->mctx,
					       sizeof(disp->port_table[0]) *
					       DNS_DISPATCH_PORTTABLESIZE);
		if (disp->port_table == NULL)
			goto deallocate_dispatch;
		for (i = 0; i < DNS_DISPATCH_PORTTABLESIZE; i++)
			ISC_LIST_INIT(disp->port_table[i]);

		result = isc_mempool_create(mgr->mctx, sizeof(dispportentry_t),
					    &disp->portpool);
		if (result != ISC_R_SUCCESS)
			goto deallocate_dispatch;
		isc_mempool_setname(disp->portpool, "disp_portpool");
		isc_mempool_setfreemax(disp->portpool, 128);
	}
	disp->socktype = isc_sockettype_udp;
	disp->socket = sock;
	disp->local = *localaddr;

	if ((attributes & DNS_DISPATCHATTR_EXCLUSIVE) != 0)
		disp->ntasks = MAX_INTERNAL_TASKS;
	else
		disp->ntasks = 1;
	for (i = 0; i < disp->ntasks; i++) {
		disp->task[i] = NULL;
		result = isc_task_create(taskmgr, 0, &disp->task[i]);
		if (result != ISC_R_SUCCESS) {
			while (--i >= 0)
				isc_task_destroy(&disp->task[i]);
			goto kill_socket;
		}
		isc_task_setname(disp->task[i], "udpdispatch", disp);
	}

	disp->ctlevent = isc_event_allocate(mgr->mctx, disp,
					    DNS_EVENT_DISPATCHCONTROL,
					    destroy_disp, disp,
					    sizeof(isc_event_t));
	if (disp->ctlevent == NULL) {
		result = ISC_R_NOMEMORY;
		goto kill_task;
	}

	attributes &= ~DNS_DISPATCHATTR_TCP;
	attributes |= DNS_DISPATCHATTR_UDP;
	disp->attributes = attributes;

	/*
	 * Append it to the dispatcher list.
	 */
	ISC_LIST_APPEND(mgr->list, disp, link);

	mgr_log(mgr, LVL(90), "created UDP dispatcher %p", disp);
	dispatch_log(disp, LVL(90), "created task %p", disp->task[0]); /* XXX */
	if (disp->socket != NULL)
		dispatch_log(disp, LVL(90), "created socket %p", disp->socket);

	*dispp = disp;
	return (result);

	/*
	 * Error returns.
	 */
 kill_task:
	for (i = 0; i < disp->ntasks; i++)
		isc_task_detach(&disp->task[i]);
 kill_socket:
	if (disp->socket != NULL)
		isc_socket_detach(&disp->socket);
 deallocate_dispatch:
	dispatch_free(&disp);

	return (result);
}

void
dns_dispatch_attach(dns_dispatch_t *disp, dns_dispatch_t **dispp) {
	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE(dispp != NULL && *dispp == NULL);

	LOCK(&disp->lock);
	disp->refcount++;
	UNLOCK(&disp->lock);

	*dispp = disp;
}

/*
 * It is important to lock the manager while we are deleting the dispatch,
 * since dns_dispatch_getudp will call dispatch_find, which returns to
 * the caller a dispatch but does not attach to it until later.  _getudp
 * locks the manager, however, so locking it here will keep us from attaching
 * to a dispatcher that is in the process of going away.
 */
void
dns_dispatch_detach(dns_dispatch_t **dispp) {
	dns_dispatch_t *disp;
	dispsocket_t *dispsock;
	isc_boolean_t killit;

	REQUIRE(dispp != NULL && VALID_DISPATCH(*dispp));

	disp = *dispp;
	*dispp = NULL;

	LOCK(&disp->lock);

	INSIST(disp->refcount > 0);
	disp->refcount--;
	killit = ISC_FALSE;
	if (disp->refcount == 0) {
		if (disp->recv_pending > 0)
			isc_socket_cancel(disp->socket, disp->task[0],
					  ISC_SOCKCANCEL_RECV);
		for (dispsock = ISC_LIST_HEAD(disp->activesockets);
		     dispsock != NULL;
		     dispsock = ISC_LIST_NEXT(dispsock, link)) {
			isc_socket_cancel(dispsock->socket, dispsock->task,
					  ISC_SOCKCANCEL_RECV);
		}
		disp->shutting_down = 1;
	}

	dispatch_log(disp, LVL(90), "detach: refcount %d", disp->refcount);

	killit = destroy_disp_ok(disp);
	UNLOCK(&disp->lock);
	if (killit)
		isc_task_send(disp->task[0], &disp->ctlevent);
}

isc_result_t
dns_dispatch_addresponse2(dns_dispatch_t *disp, isc_sockaddr_t *dest,
			  isc_task_t *task, isc_taskaction_t action, void *arg,
			  dns_messageid_t *idp, dns_dispentry_t **resp,
			  isc_socketmgr_t *sockmgr)
{
	dns_dispentry_t *res;
	unsigned int bucket;
	in_port_t localport = 0;
	dns_messageid_t id;
	int i;
	isc_boolean_t ok;
	dns_qid_t *qid;
	dispsocket_t *dispsocket = NULL;
	isc_result_t result;

	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE(task != NULL);
	REQUIRE(dest != NULL);
	REQUIRE(resp != NULL && *resp == NULL);
	REQUIRE(idp != NULL);
	if ((disp->attributes & DNS_DISPATCHATTR_EXCLUSIVE) != 0)
		REQUIRE(sockmgr != NULL);

	LOCK(&disp->lock);

	if (disp->shutting_down == 1) {
		UNLOCK(&disp->lock);
		return (ISC_R_SHUTTINGDOWN);
	}

	if (disp->requests >= disp->maxrequests) {
		UNLOCK(&disp->lock);
		return (ISC_R_QUOTA);
	}

	if ((disp->attributes & DNS_DISPATCHATTR_EXCLUSIVE) != 0 &&
	    disp->nsockets > DNS_DISPATCH_SOCKSQUOTA) {
		dispsocket_t *oldestsocket;
		dns_dispentry_t *oldestresp;
		dns_dispatchevent_t *rev;

		/*
		 * Kill oldest outstanding query if the number of sockets
		 * exceeds the quota to keep the room for new queries.
		 */
		oldestsocket = ISC_LIST_HEAD(disp->activesockets);
		oldestresp = oldestsocket->resp;
		if (oldestresp != NULL && !oldestresp->item_out) {
			rev = allocate_event(oldestresp->disp);
			if (rev != NULL) {
				rev->buffer.base = NULL;
				rev->result = ISC_R_CANCELED;
				rev->id = oldestresp->id;
				ISC_EVENT_INIT(rev, sizeof(*rev), 0,
					       NULL, DNS_EVENT_DISPATCH,
					       oldestresp->action,
					       oldestresp->arg, oldestresp,
					       NULL, NULL);
				oldestresp->item_out = ISC_TRUE;
				isc_task_send(oldestresp->task,
					      ISC_EVENT_PTR(&rev));
				inc_stats(disp->mgr,
					  dns_resstatscounter_dispabort);
			}
		}

		/*
		 * Move this entry to the tail so that it won't (easily) be
		 * examined before actually being canceled.
		 */
		ISC_LIST_UNLINK(disp->activesockets, oldestsocket, link);
		ISC_LIST_APPEND(disp->activesockets, oldestsocket, link);
	}

	qid = DNS_QID(disp);
	LOCK(&qid->lock);

	if ((disp->attributes & DNS_DISPATCHATTR_EXCLUSIVE) != 0) {
		/*
		 * Get a separate UDP socket with a random port number.
		 */
		result = get_dispsocket(disp, dest, sockmgr, qid, &dispsocket,
					&localport);
		if (result != ISC_R_SUCCESS) {
			UNLOCK(&qid->lock);
			UNLOCK(&disp->lock);
			inc_stats(disp->mgr, dns_resstatscounter_dispsockfail);
			return (result);
		}
	} else {
		localport = disp->localport;
	}

	/*
	 * Try somewhat hard to find an unique ID.
	 */
	id = (dns_messageid_t)dispatch_arc4random(DISP_ARC4CTX(disp));
	bucket = dns_hash(qid, dest, id, localport);
	ok = ISC_FALSE;
	for (i = 0; i < 64; i++) {
		if (entry_search(qid, dest, id, localport, bucket) == NULL) {
			ok = ISC_TRUE;
			break;
		}
		id += qid->qid_increment;
		id &= 0x0000ffff;
		bucket = dns_hash(qid, dest, id, localport);
	}

	if (!ok) {
		UNLOCK(&qid->lock);
		UNLOCK(&disp->lock);
		return (ISC_R_NOMORE);
	}

	res = isc_mempool_get(disp->mgr->rpool);
	if (res == NULL) {
		UNLOCK(&qid->lock);
		UNLOCK(&disp->lock);
		if (dispsocket != NULL)
			destroy_dispsocket(disp, &dispsocket);
		return (ISC_R_NOMEMORY);
	}

	disp->refcount++;
	disp->requests++;
	res->task = NULL;
	isc_task_attach(task, &res->task);
	res->disp = disp;
	res->id = id;
	res->port = localport;
	res->bucket = bucket;
	res->host = *dest;
	res->action = action;
	res->arg = arg;
	res->dispsocket = dispsocket;
	if (dispsocket != NULL)
		dispsocket->resp = res;
	res->item_out = ISC_FALSE;
	ISC_LIST_INIT(res->items);
	ISC_LINK_INIT(res, link);
	res->magic = RESPONSE_MAGIC;
	ISC_LIST_APPEND(qid->qid_table[bucket], res, link);
	UNLOCK(&qid->lock);

	request_log(disp, res, LVL(90),
		    "attached to task %p", res->task);

	if (((disp->attributes & DNS_DISPATCHATTR_UDP) != 0) ||
	    ((disp->attributes & DNS_DISPATCHATTR_CONNECTED) != 0)) {
		result = startrecv(disp, dispsocket);
		if (result != ISC_R_SUCCESS) {
			LOCK(&qid->lock);
			ISC_LIST_UNLINK(qid->qid_table[bucket], res, link);
			UNLOCK(&qid->lock);

			if (dispsocket != NULL)
				destroy_dispsocket(disp, &dispsocket);

			disp->refcount--;
			disp->requests--;

			UNLOCK(&disp->lock);
			isc_task_detach(&res->task);
			isc_mempool_put(disp->mgr->rpool, res);
			return (result);
		}
	}

	if (dispsocket != NULL)
		ISC_LIST_APPEND(disp->activesockets, dispsocket, link);

	UNLOCK(&disp->lock);

	*idp = id;
	*resp = res;

	if ((disp->attributes & DNS_DISPATCHATTR_EXCLUSIVE) != 0)
		INSIST(res->dispsocket != NULL);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_dispatch_addresponse(dns_dispatch_t *disp, isc_sockaddr_t *dest,
			 isc_task_t *task, isc_taskaction_t action, void *arg,
			 dns_messageid_t *idp, dns_dispentry_t **resp)
{
	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE((disp->attributes & DNS_DISPATCHATTR_EXCLUSIVE) == 0);

	return (dns_dispatch_addresponse2(disp, dest, task, action, arg,
					  idp, resp, NULL));
}

void
dns_dispatch_starttcp(dns_dispatch_t *disp) {

	REQUIRE(VALID_DISPATCH(disp));

	dispatch_log(disp, LVL(90), "starttcp %p", disp->task[0]);

	LOCK(&disp->lock);
	disp->attributes |= DNS_DISPATCHATTR_CONNECTED;
	(void)startrecv(disp, NULL);
	UNLOCK(&disp->lock);
}

void
dns_dispatch_removeresponse(dns_dispentry_t **resp,
			    dns_dispatchevent_t **sockevent)
{
	dns_dispatchmgr_t *mgr;
	dns_dispatch_t *disp;
	dns_dispentry_t *res;
	dispsocket_t *dispsock;
	dns_dispatchevent_t *ev;
	unsigned int bucket;
	isc_boolean_t killit;
	unsigned int n;
	isc_eventlist_t events;
	dns_qid_t *qid;

	REQUIRE(resp != NULL);
	REQUIRE(VALID_RESPONSE(*resp));

	res = *resp;
	*resp = NULL;

	disp = res->disp;
	REQUIRE(VALID_DISPATCH(disp));
	mgr = disp->mgr;
	REQUIRE(VALID_DISPATCHMGR(mgr));

	qid = DNS_QID(disp);

	if (sockevent != NULL) {
		REQUIRE(*sockevent != NULL);
		ev = *sockevent;
		*sockevent = NULL;
	} else {
		ev = NULL;
	}

	LOCK(&disp->lock);

	INSIST(disp->requests > 0);
	disp->requests--;
	INSIST(disp->refcount > 0);
	disp->refcount--;
	killit = ISC_FALSE;
	if (disp->refcount == 0) {
		if (disp->recv_pending > 0)
			isc_socket_cancel(disp->socket, disp->task[0],
					  ISC_SOCKCANCEL_RECV);
		for (dispsock = ISC_LIST_HEAD(disp->activesockets);
		     dispsock != NULL;
		     dispsock = ISC_LIST_NEXT(dispsock, link)) {
			isc_socket_cancel(dispsock->socket, dispsock->task,
					  ISC_SOCKCANCEL_RECV);
		}
		disp->shutting_down = 1;
	}

	bucket = res->bucket;

	LOCK(&qid->lock);
	ISC_LIST_UNLINK(qid->qid_table[bucket], res, link);
	UNLOCK(&qid->lock);

	if (ev == NULL && res->item_out) {
		/*
		 * We've posted our event, but the caller hasn't gotten it
		 * yet.  Take it back.
		 */
		ISC_LIST_INIT(events);
		n = isc_task_unsend(res->task, res, DNS_EVENT_DISPATCH,
				    NULL, &events);
		/*
		 * We had better have gotten it back.
		 */
		INSIST(n == 1);
		ev = (dns_dispatchevent_t *)ISC_LIST_HEAD(events);
	}

	if (ev != NULL) {
		REQUIRE(res->item_out == ISC_TRUE);
		res->item_out = ISC_FALSE;
		if (ev->buffer.base != NULL)
			free_buffer(disp, ev->buffer.base, ev->buffer.length);
		free_event(disp, ev);
	}

	request_log(disp, res, LVL(90), "detaching from task %p", res->task);
	isc_task_detach(&res->task);

	if (res->dispsocket != NULL) {
		isc_socket_cancel(res->dispsocket->socket,
				  res->dispsocket->task, ISC_SOCKCANCEL_RECV);
		res->dispsocket->resp = NULL;
	}

	/*
	 * Free any buffered requests as well
	 */
	ev = ISC_LIST_HEAD(res->items);
	while (ev != NULL) {
		ISC_LIST_UNLINK(res->items, ev, ev_link);
		if (ev->buffer.base != NULL)
			free_buffer(disp, ev->buffer.base, ev->buffer.length);
		free_event(disp, ev);
		ev = ISC_LIST_HEAD(res->items);
	}
	res->magic = 0;
	isc_mempool_put(disp->mgr->rpool, res);
	if (disp->shutting_down == 1)
		do_cancel(disp);
	else
		(void)startrecv(disp, NULL);

	killit = destroy_disp_ok(disp);
	UNLOCK(&disp->lock);
	if (killit)
		isc_task_send(disp->task[0], &disp->ctlevent);
}

static void
do_cancel(dns_dispatch_t *disp) {
	dns_dispatchevent_t *ev;
	dns_dispentry_t *resp;
	dns_qid_t *qid;

	if (disp->shutdown_out == 1)
		return;

	qid = DNS_QID(disp);

	/*
	 * Search for the first response handler without packets outstanding
	 * unless a specific hander is given.
	 */
	LOCK(&qid->lock);
	for (resp = linear_first(qid);
	     resp != NULL && resp->item_out;
	     /* Empty. */)
		resp = linear_next(qid, resp);

	/*
	 * No one to send the cancel event to, so nothing to do.
	 */
	if (resp == NULL)
		goto unlock;

	/*
	 * Send the shutdown failsafe event to this resp.
	 */
	ev = disp->failsafe_ev;
	ISC_EVENT_INIT(ev, sizeof(*ev), 0, NULL, DNS_EVENT_DISPATCH,
		       resp->action, resp->arg, resp, NULL, NULL);
	ev->result = disp->shutdown_why;
	ev->buffer.base = NULL;
	ev->buffer.length = 0;
	disp->shutdown_out = 1;
	request_log(disp, resp, LVL(10),
		    "cancel: failsafe event %p -> task %p",
		    ev, resp->task);
	resp->item_out = ISC_TRUE;
	isc_task_send(resp->task, ISC_EVENT_PTR(&ev));
 unlock:
	UNLOCK(&qid->lock);
}

isc_socket_t *
dns_dispatch_getsocket(dns_dispatch_t *disp) {
	REQUIRE(VALID_DISPATCH(disp));

	return (disp->socket);
}

isc_socket_t *
dns_dispatch_getentrysocket(dns_dispentry_t *resp) {
	REQUIRE(VALID_RESPONSE(resp));

	if (resp->dispsocket != NULL)
		return (resp->dispsocket->socket);
	else
		return (NULL);
}

isc_result_t
dns_dispatch_getlocaladdress(dns_dispatch_t *disp, isc_sockaddr_t *addrp) {

	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE(addrp != NULL);

	if (disp->socktype == isc_sockettype_udp) {
		*addrp = disp->local;
		return (ISC_R_SUCCESS);
	}
	return (ISC_R_NOTIMPLEMENTED);
}

void
dns_dispatch_cancel(dns_dispatch_t *disp) {
	REQUIRE(VALID_DISPATCH(disp));

	LOCK(&disp->lock);

	if (disp->shutting_down == 1) {
		UNLOCK(&disp->lock);
		return;
	}

	disp->shutdown_why = ISC_R_CANCELED;
	disp->shutting_down = 1;
	do_cancel(disp);

	UNLOCK(&disp->lock);

	return;
}

unsigned int
dns_dispatch_getattributes(dns_dispatch_t *disp) {
	REQUIRE(VALID_DISPATCH(disp));

	/*
	 * We don't bother locking disp here; it's the caller's responsibility
	 * to use only non volatile flags.
	 */
	return (disp->attributes);
}

void
dns_dispatch_changeattributes(dns_dispatch_t *disp,
			      unsigned int attributes, unsigned int mask)
{
	REQUIRE(VALID_DISPATCH(disp));
	/* Exclusive attribute can only be set on creation */
	REQUIRE((attributes & DNS_DISPATCHATTR_EXCLUSIVE) == 0);
	/* Also, a dispatch with randomport specified cannot start listening */
	REQUIRE((disp->attributes & DNS_DISPATCHATTR_EXCLUSIVE) == 0 ||
		(attributes & DNS_DISPATCHATTR_NOLISTEN) == 0);

	/* XXXMLG
	 * Should check for valid attributes here!
	 */

	LOCK(&disp->lock);

	if ((mask & DNS_DISPATCHATTR_NOLISTEN) != 0) {
		if ((disp->attributes & DNS_DISPATCHATTR_NOLISTEN) != 0 &&
		    (attributes & DNS_DISPATCHATTR_NOLISTEN) == 0) {
			disp->attributes &= ~DNS_DISPATCHATTR_NOLISTEN;
			(void)startrecv(disp, NULL);
		} else if ((disp->attributes & DNS_DISPATCHATTR_NOLISTEN)
			   == 0 &&
			   (attributes & DNS_DISPATCHATTR_NOLISTEN) != 0) {
			disp->attributes |= DNS_DISPATCHATTR_NOLISTEN;
			if (disp->recv_pending != 0)
				isc_socket_cancel(disp->socket, disp->task[0],
						  ISC_SOCKCANCEL_RECV);
		}
	}

	disp->attributes &= ~mask;
	disp->attributes |= (attributes & mask);
	UNLOCK(&disp->lock);
}

void
dns_dispatch_importrecv(dns_dispatch_t *disp, isc_event_t *event) {
	void *buf;
	isc_socketevent_t *sevent, *newsevent;

	REQUIRE(VALID_DISPATCH(disp));
	REQUIRE((disp->attributes & DNS_DISPATCHATTR_NOLISTEN) != 0);
	REQUIRE(event != NULL);

	sevent = (isc_socketevent_t *)event;

	INSIST(sevent->n <= disp->mgr->buffersize);
	newsevent = (isc_socketevent_t *)
		    isc_event_allocate(disp->mgr->mctx, NULL,
				      DNS_EVENT_IMPORTRECVDONE, udp_shrecv,
				      disp, sizeof(isc_socketevent_t));
	if (newsevent == NULL)
		return;

	buf = allocate_udp_buffer(disp);
	if (buf == NULL) {
		isc_event_free(ISC_EVENT_PTR(&newsevent));
		return;
	}
	memcpy(buf, sevent->region.base, sevent->n);
	newsevent->region.base = buf;
	newsevent->region.length = disp->mgr->buffersize;
	newsevent->n = sevent->n;
	newsevent->result = sevent->result;
	newsevent->address = sevent->address;
	newsevent->timestamp = sevent->timestamp;
	newsevent->pktinfo = sevent->pktinfo;
	newsevent->attributes = sevent->attributes;

	isc_task_send(disp->task[0], ISC_EVENT_PTR(&newsevent));
}

#if 0
void
dns_dispatchmgr_dump(dns_dispatchmgr_t *mgr) {
	dns_dispatch_t *disp;
	char foo[1024];

	disp = ISC_LIST_HEAD(mgr->list);
	while (disp != NULL) {
		isc_sockaddr_format(&disp->local, foo, sizeof(foo));
		printf("\tdispatch %p, addr %s\n", disp, foo);
		disp = ISC_LIST_NEXT(disp, link);
	}
}
#endif
