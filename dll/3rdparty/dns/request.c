/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2002  Internet Software Consortium.
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

/* $Id: request.c,v 1.82.72.2 2009/01/18 23:47:40 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/compress.h>
#include <dns/dispatch.h>
#include <dns/events.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/request.h>
#include <dns/result.h>
#include <dns/tsig.h>

#define REQUESTMGR_MAGIC	ISC_MAGIC('R', 'q', 'u', 'M')
#define VALID_REQUESTMGR(mgr)	ISC_MAGIC_VALID(mgr, REQUESTMGR_MAGIC)

#define REQUEST_MAGIC		ISC_MAGIC('R', 'q', 'u', '!')
#define VALID_REQUEST(request)	ISC_MAGIC_VALID(request, REQUEST_MAGIC)

typedef ISC_LIST(dns_request_t) dns_requestlist_t;

#define DNS_REQUEST_NLOCKS 7

struct dns_requestmgr {
	unsigned int			magic;
	isc_mutex_t			lock;
	isc_mem_t		       *mctx;

	/* locked */
	isc_int32_t			eref;
	isc_int32_t			iref;
	isc_timermgr_t		       *timermgr;
	isc_socketmgr_t		       *socketmgr;
	isc_taskmgr_t		       *taskmgr;
	dns_dispatchmgr_t	       *dispatchmgr;
	dns_dispatch_t		       *dispatchv4;
	dns_dispatch_t		       *dispatchv6;
	isc_boolean_t			exiting;
	isc_eventlist_t			whenshutdown;
	unsigned int			hash;
	isc_mutex_t			locks[DNS_REQUEST_NLOCKS];
	dns_requestlist_t 		requests;
};

struct dns_request {
	unsigned int			magic;
	unsigned int			hash;
	isc_mem_t		       *mctx;
	isc_int32_t			flags;
	ISC_LINK(dns_request_t) 	link;
	isc_buffer_t		       *query;
	isc_buffer_t		       *answer;
	dns_requestevent_t	       *event;
	dns_dispatch_t		       *dispatch;
	dns_dispentry_t		       *dispentry;
	isc_timer_t		       *timer;
	dns_requestmgr_t	       *requestmgr;
	isc_buffer_t		       *tsig;
	dns_tsigkey_t		       *tsigkey;
	isc_event_t			ctlevent;
	isc_boolean_t			canceling; /* ctlevent outstanding */
	isc_sockaddr_t			destaddr;
	unsigned int			udpcount;
};

#define DNS_REQUEST_F_CONNECTING 0x0001
#define DNS_REQUEST_F_SENDING 0x0002
#define DNS_REQUEST_F_CANCELED 0x0004	/*%< ctlevent received, or otherwise
					   synchronously canceled */
#define DNS_REQUEST_F_TIMEDOUT 0x0008	/*%< canceled due to a timeout */
#define DNS_REQUEST_F_TCP 0x0010	/*%< This request used TCP */
#define DNS_REQUEST_CANCELED(r) \
	(((r)->flags & DNS_REQUEST_F_CANCELED) != 0)
#define DNS_REQUEST_CONNECTING(r) \
	(((r)->flags & DNS_REQUEST_F_CONNECTING) != 0)
#define DNS_REQUEST_SENDING(r) \
	(((r)->flags & DNS_REQUEST_F_SENDING) != 0)
#define DNS_REQUEST_TIMEDOUT(r) \
	(((r)->flags & DNS_REQUEST_F_TIMEDOUT) != 0)


/***
 *** Forward
 ***/

static void mgr_destroy(dns_requestmgr_t *requestmgr);
static void mgr_shutdown(dns_requestmgr_t *requestmgr);
static unsigned int mgr_gethash(dns_requestmgr_t *requestmgr);
static void send_shutdown_events(dns_requestmgr_t *requestmgr);

static isc_result_t req_render(dns_message_t *message, isc_buffer_t **buffer,
			       unsigned int options, isc_mem_t *mctx);
static void req_senddone(isc_task_t *task, isc_event_t *event);
static void req_response(isc_task_t *task, isc_event_t *event);
static void req_timeout(isc_task_t *task, isc_event_t *event);
static isc_socket_t * req_getsocket(dns_request_t *request);
static void req_connected(isc_task_t *task, isc_event_t *event);
static void req_sendevent(dns_request_t *request, isc_result_t result);
static void req_cancel(dns_request_t *request);
static void req_destroy(dns_request_t *request);
static void req_log(int level, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);
static void do_cancel(isc_task_t *task, isc_event_t *event);

/***
 *** Public
 ***/

isc_result_t
dns_requestmgr_create(isc_mem_t *mctx,
		      isc_timermgr_t *timermgr,
		      isc_socketmgr_t *socketmgr,
		      isc_taskmgr_t *taskmgr,
		      dns_dispatchmgr_t *dispatchmgr,
		      dns_dispatch_t *dispatchv4,
		      dns_dispatch_t *dispatchv6,
		      dns_requestmgr_t **requestmgrp)
{
	dns_requestmgr_t *requestmgr;
	isc_socket_t *socket;
	isc_result_t result;
	int i;
	unsigned int dispattr;

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_create");

	REQUIRE(requestmgrp != NULL && *requestmgrp == NULL);
	REQUIRE(timermgr != NULL);
	REQUIRE(socketmgr != NULL);
	REQUIRE(taskmgr != NULL);
	REQUIRE(dispatchmgr != NULL);
	UNUSED(socket);
	if (dispatchv4 != NULL) {
		dispattr = dns_dispatch_getattributes(dispatchv4);
		REQUIRE((dispattr & DNS_DISPATCHATTR_UDP) != 0);
	}
	if (dispatchv6 != NULL) {
		dispattr = dns_dispatch_getattributes(dispatchv6);
		REQUIRE((dispattr & DNS_DISPATCHATTR_UDP) != 0);
	}

	requestmgr = isc_mem_get(mctx, sizeof(*requestmgr));
	if (requestmgr == NULL)
		return (ISC_R_NOMEMORY);

	result = isc_mutex_init(&requestmgr->lock);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, requestmgr, sizeof(*requestmgr));
		return (result);
	}
	for (i = 0; i < DNS_REQUEST_NLOCKS; i++) {
		result = isc_mutex_init(&requestmgr->locks[i]);
		if (result != ISC_R_SUCCESS) {
			while (--i >= 0)
				DESTROYLOCK(&requestmgr->locks[i]);
			DESTROYLOCK(&requestmgr->lock);
			isc_mem_put(mctx, requestmgr, sizeof(*requestmgr));
			return (result);
		}
	}
	requestmgr->timermgr = timermgr;
	requestmgr->socketmgr = socketmgr;
	requestmgr->taskmgr = taskmgr;
	requestmgr->dispatchmgr = dispatchmgr;
	requestmgr->dispatchv4 = NULL;
	if (dispatchv4 != NULL)
		dns_dispatch_attach(dispatchv4, &requestmgr->dispatchv4);
	requestmgr->dispatchv6 = NULL;
	if (dispatchv6 != NULL)
		dns_dispatch_attach(dispatchv6, &requestmgr->dispatchv6);
	requestmgr->mctx = NULL;
	isc_mem_attach(mctx, &requestmgr->mctx);
	requestmgr->eref = 1;	/* implicit attach */
	requestmgr->iref = 0;
	ISC_LIST_INIT(requestmgr->whenshutdown);
	ISC_LIST_INIT(requestmgr->requests);
	requestmgr->exiting = ISC_FALSE;
	requestmgr->hash = 0;
	requestmgr->magic = REQUESTMGR_MAGIC;

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_create: %p", requestmgr);

	*requestmgrp = requestmgr;
	return (ISC_R_SUCCESS);
}

void
dns_requestmgr_whenshutdown(dns_requestmgr_t *requestmgr, isc_task_t *task,
			    isc_event_t **eventp)
{
	isc_task_t *clone;
	isc_event_t *event;

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_whenshutdown");

	REQUIRE(VALID_REQUESTMGR(requestmgr));
	REQUIRE(eventp != NULL);

	event = *eventp;
	*eventp = NULL;

	LOCK(&requestmgr->lock);

	if (requestmgr->exiting) {
		/*
		 * We're already shutdown.  Send the event.
		 */
		event->ev_sender = requestmgr;
		isc_task_send(task, &event);
	} else {
		clone = NULL;
		isc_task_attach(task, &clone);
		event->ev_sender = clone;
		ISC_LIST_APPEND(requestmgr->whenshutdown, event, ev_link);
	}
	UNLOCK(&requestmgr->lock);
}

void
dns_requestmgr_shutdown(dns_requestmgr_t *requestmgr) {

	REQUIRE(VALID_REQUESTMGR(requestmgr));

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_shutdown: %p", requestmgr);

	LOCK(&requestmgr->lock);
	mgr_shutdown(requestmgr);
	UNLOCK(&requestmgr->lock);
}

static void
mgr_shutdown(dns_requestmgr_t *requestmgr) {
	dns_request_t *request;

	/*
	 * Caller holds lock.
	 */
	if (!requestmgr->exiting) {
		requestmgr->exiting = ISC_TRUE;
		for (request = ISC_LIST_HEAD(requestmgr->requests);
		     request != NULL;
		     request = ISC_LIST_NEXT(request, link)) {
			dns_request_cancel(request);
		}
		if (requestmgr->iref == 0) {
			INSIST(ISC_LIST_EMPTY(requestmgr->requests));
			send_shutdown_events(requestmgr);
		}
	}
}

static void
requestmgr_attach(dns_requestmgr_t *source, dns_requestmgr_t **targetp) {

	/*
	 * Locked by caller.
	 */

	REQUIRE(VALID_REQUESTMGR(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	REQUIRE(!source->exiting);

	source->iref++;
	*targetp = source;

	req_log(ISC_LOG_DEBUG(3), "requestmgr_attach: %p: eref %d iref %d",
		source, source->eref, source->iref);
}

static void
requestmgr_detach(dns_requestmgr_t **requestmgrp) {
	dns_requestmgr_t *requestmgr;
	isc_boolean_t need_destroy = ISC_FALSE;

	REQUIRE(requestmgrp != NULL);
	requestmgr = *requestmgrp;
	REQUIRE(VALID_REQUESTMGR(requestmgr));

	*requestmgrp = NULL;
	LOCK(&requestmgr->lock);
	INSIST(requestmgr->iref > 0);
	requestmgr->iref--;

	req_log(ISC_LOG_DEBUG(3), "requestmgr_detach: %p: eref %d iref %d",
		requestmgr, requestmgr->eref, requestmgr->iref);

	if (requestmgr->iref == 0 && requestmgr->exiting) {
		INSIST(ISC_LIST_HEAD(requestmgr->requests) == NULL);
		send_shutdown_events(requestmgr);
		if (requestmgr->eref == 0)
			need_destroy = ISC_TRUE;
	}
	UNLOCK(&requestmgr->lock);

	if (need_destroy)
		mgr_destroy(requestmgr);
}

void
dns_requestmgr_attach(dns_requestmgr_t *source, dns_requestmgr_t **targetp) {

	REQUIRE(VALID_REQUESTMGR(source));
	REQUIRE(targetp != NULL && *targetp == NULL);
	REQUIRE(!source->exiting);

	LOCK(&source->lock);
	source->eref++;
	*targetp = source;
	UNLOCK(&source->lock);

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_attach: %p: eref %d iref %d",
		source, source->eref, source->iref);
}

void
dns_requestmgr_detach(dns_requestmgr_t **requestmgrp) {
	dns_requestmgr_t *requestmgr;
	isc_boolean_t need_destroy = ISC_FALSE;

	REQUIRE(requestmgrp != NULL);
	requestmgr = *requestmgrp;
	REQUIRE(VALID_REQUESTMGR(requestmgr));

	LOCK(&requestmgr->lock);
	INSIST(requestmgr->eref > 0);
	requestmgr->eref--;

	req_log(ISC_LOG_DEBUG(3), "dns_requestmgr_detach: %p: eref %d iref %d",
		requestmgr, requestmgr->eref, requestmgr->iref);

	if (requestmgr->eref == 0 && requestmgr->iref == 0) {
		INSIST(requestmgr->exiting &&
		       ISC_LIST_HEAD(requestmgr->requests) == NULL);
		need_destroy = ISC_TRUE;
	}
	UNLOCK(&requestmgr->lock);

	if (need_destroy)
		mgr_destroy(requestmgr);

	*requestmgrp = NULL;
}

static void
send_shutdown_events(dns_requestmgr_t *requestmgr) {
	isc_event_t *event, *next_event;
	isc_task_t *etask;

	req_log(ISC_LOG_DEBUG(3), "send_shutdown_events: %p", requestmgr);

	/*
	 * Caller must be holding the manager lock.
	 */
	for (event = ISC_LIST_HEAD(requestmgr->whenshutdown);
	     event != NULL;
	     event = next_event) {
		next_event = ISC_LIST_NEXT(event, ev_link);
		ISC_LIST_UNLINK(requestmgr->whenshutdown, event, ev_link);
		etask = event->ev_sender;
		event->ev_sender = requestmgr;
		isc_task_sendanddetach(&etask, &event);
	}
}

static void
mgr_destroy(dns_requestmgr_t *requestmgr) {
	int i;
	isc_mem_t *mctx;

	req_log(ISC_LOG_DEBUG(3), "mgr_destroy");

	REQUIRE(requestmgr->eref == 0);
	REQUIRE(requestmgr->iref == 0);

	DESTROYLOCK(&requestmgr->lock);
	for (i = 0; i < DNS_REQUEST_NLOCKS; i++)
		DESTROYLOCK(&requestmgr->locks[i]);
	if (requestmgr->dispatchv4 != NULL)
		dns_dispatch_detach(&requestmgr->dispatchv4);
	if (requestmgr->dispatchv6 != NULL)
		dns_dispatch_detach(&requestmgr->dispatchv6);
	requestmgr->magic = 0;
	mctx = requestmgr->mctx;
	isc_mem_put(mctx, requestmgr, sizeof(*requestmgr));
	isc_mem_detach(&mctx);
}

static unsigned int
mgr_gethash(dns_requestmgr_t *requestmgr) {
	req_log(ISC_LOG_DEBUG(3), "mgr_gethash");
	/*
	 * Locked by caller.
	 */
	requestmgr->hash++;
	return (requestmgr->hash % DNS_REQUEST_NLOCKS);
}

static inline isc_result_t
req_send(dns_request_t *request, isc_task_t *task, isc_sockaddr_t *address) {
	isc_region_t r;
	isc_socket_t *socket;
	isc_result_t result;
	unsigned int dispattr;

	req_log(ISC_LOG_DEBUG(3), "req_send: request %p", request);

	REQUIRE(VALID_REQUEST(request));
	dispattr = dns_dispatch_getattributes(request->dispatch);
	socket = req_getsocket(request);
	isc_buffer_usedregion(request->query, &r);
	/*
	 * We could connect the socket when we are using an exclusive dispatch
	 * as we do in resolver.c, but we prefer implementation simplicity
	 * at this moment.
	 */
	result = isc_socket_sendto(socket, &r, task, req_senddone,
				  request, address, NULL);
	if (result == ISC_R_SUCCESS)
		request->flags |= DNS_REQUEST_F_SENDING;
	return (result);
}

static isc_result_t
new_request(isc_mem_t *mctx, dns_request_t **requestp) {
	dns_request_t *request;

	request = isc_mem_get(mctx, sizeof(*request));
	if (request == NULL)
		return (ISC_R_NOMEMORY);

	/*
	 * Zero structure.
	 */
	request->magic = 0;
	request->mctx = NULL;
	request->flags = 0;
	ISC_LINK_INIT(request, link);
	request->query = NULL;
	request->answer = NULL;
	request->event = NULL;
	request->dispatch = NULL;
	request->dispentry = NULL;
	request->timer = NULL;
	request->requestmgr = NULL;
	request->tsig = NULL;
	request->tsigkey = NULL;
	ISC_EVENT_INIT(&request->ctlevent, sizeof(request->ctlevent), 0, NULL,
		       DNS_EVENT_REQUESTCONTROL, do_cancel, request, NULL,
		       NULL, NULL);
	request->canceling = ISC_FALSE;
	request->udpcount = 0;

	isc_mem_attach(mctx, &request->mctx);

	request->magic = REQUEST_MAGIC;
	*requestp = request;
	return (ISC_R_SUCCESS);
}


static isc_boolean_t
isblackholed(dns_dispatchmgr_t *dispatchmgr, isc_sockaddr_t *destaddr) {
	dns_acl_t *blackhole;
	isc_netaddr_t netaddr;
	int match;
	isc_boolean_t drop = ISC_FALSE;
	char netaddrstr[ISC_NETADDR_FORMATSIZE];

	blackhole = dns_dispatchmgr_getblackhole(dispatchmgr);
	if (blackhole != NULL) {
		isc_netaddr_fromsockaddr(&netaddr, destaddr);
		if (dns_acl_match(&netaddr, NULL, blackhole,
				  NULL, &match, NULL) == ISC_R_SUCCESS &&
		    match > 0)
			drop = ISC_TRUE;
	}
	if (drop) {
		isc_netaddr_format(&netaddr, netaddrstr, sizeof(netaddrstr));
		req_log(ISC_LOG_DEBUG(10), "blackholed address %s", netaddrstr);
	}
	return (drop);
}

static isc_result_t
create_tcp_dispatch(dns_requestmgr_t *requestmgr, isc_sockaddr_t *srcaddr,
		    isc_sockaddr_t *destaddr, dns_dispatch_t **dispatchp)
{
	isc_result_t result;
	isc_socket_t *socket = NULL;
	isc_sockaddr_t src;
	unsigned int attrs;
	isc_sockaddr_t bind_any;

	result = isc_socket_create(requestmgr->socketmgr,
				   isc_sockaddr_pf(destaddr),
				   isc_sockettype_tcp, &socket);
	if (result != ISC_R_SUCCESS)
		return (result);
#ifndef BROKEN_TCP_BIND_BEFORE_CONNECT
	if (srcaddr == NULL) {
		isc_sockaddr_anyofpf(&bind_any,
				     isc_sockaddr_pf(destaddr));
		result = isc_socket_bind(socket, &bind_any, 0);
	} else {
		src = *srcaddr;
		isc_sockaddr_setport(&src, 0);
		result = isc_socket_bind(socket, &src, 0);
	}
	if (result != ISC_R_SUCCESS)
		goto cleanup;
#endif
	attrs = 0;
	attrs |= DNS_DISPATCHATTR_TCP;
	attrs |= DNS_DISPATCHATTR_PRIVATE;
	if (isc_sockaddr_pf(destaddr) == AF_INET)
		attrs |= DNS_DISPATCHATTR_IPV4;
	else
		attrs |= DNS_DISPATCHATTR_IPV6;
	attrs |= DNS_DISPATCHATTR_MAKEQUERY;
	result = dns_dispatch_createtcp(requestmgr->dispatchmgr,
					socket, requestmgr->taskmgr,
					4096, 2, 1, 1, 3, attrs,
					dispatchp);
cleanup:
	isc_socket_detach(&socket);
	return (result);
}

static isc_result_t
find_udp_dispatch(dns_requestmgr_t *requestmgr, isc_sockaddr_t *srcaddr,
		  isc_sockaddr_t *destaddr, dns_dispatch_t **dispatchp)
{
	dns_dispatch_t *disp = NULL;
	unsigned int attrs, attrmask;

	if (srcaddr == NULL) {
		switch (isc_sockaddr_pf(destaddr)) {
		case PF_INET:
			disp = requestmgr->dispatchv4;
			break;

		case PF_INET6:
			disp = requestmgr->dispatchv6;
			break;

		default:
			return (ISC_R_NOTIMPLEMENTED);
		}
		if (disp == NULL)
			return (ISC_R_FAMILYNOSUPPORT);
		dns_dispatch_attach(disp, dispatchp);
		return (ISC_R_SUCCESS);
	}
	attrs = 0;
	attrs |= DNS_DISPATCHATTR_UDP;
	switch (isc_sockaddr_pf(srcaddr)) {
	case PF_INET:
		attrs |= DNS_DISPATCHATTR_IPV4;
		break;

	case PF_INET6:
		attrs |= DNS_DISPATCHATTR_IPV6;
		break;

	default:
		return (ISC_R_NOTIMPLEMENTED);
	}
	attrmask = 0;
	attrmask |= DNS_DISPATCHATTR_UDP;
	attrmask |= DNS_DISPATCHATTR_TCP;
	attrmask |= DNS_DISPATCHATTR_IPV4;
	attrmask |= DNS_DISPATCHATTR_IPV6;
	return (dns_dispatch_getudp(requestmgr->dispatchmgr,
				    requestmgr->socketmgr,
				    requestmgr->taskmgr,
				    srcaddr, 4096,
				    1000, 32768, 16411, 16433,
				    attrs, attrmask,
				    dispatchp));
}

static isc_result_t
get_dispatch(isc_boolean_t tcp, dns_requestmgr_t *requestmgr,
	     isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
	     dns_dispatch_t **dispatchp)
{
	isc_result_t result;
	if (tcp)
		result = create_tcp_dispatch(requestmgr, srcaddr,
					     destaddr, dispatchp);
	else
		result = find_udp_dispatch(requestmgr, srcaddr,
					   destaddr, dispatchp);
	return (result);
}

static isc_result_t
set_timer(isc_timer_t *timer, unsigned int timeout, unsigned int udpresend) {
	isc_time_t expires;
	isc_interval_t interval;
	isc_result_t result;
	isc_timertype_t timertype;

	isc_interval_set(&interval, timeout, 0);
	result = isc_time_nowplusinterval(&expires, &interval);
	isc_interval_set(&interval, udpresend, 0);

	timertype = udpresend != 0 ? isc_timertype_limited : isc_timertype_once;
	if (result == ISC_R_SUCCESS)
		result = isc_timer_reset(timer, timertype, &expires,
					 &interval, ISC_FALSE);
	return (result);
}

isc_result_t
dns_request_createraw(dns_requestmgr_t *requestmgr, isc_buffer_t *msgbuf,
		      isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		      unsigned int options, unsigned int timeout,
		      isc_task_t *task, isc_taskaction_t action, void *arg,
		      dns_request_t **requestp)
{
	return(dns_request_createraw3(requestmgr, msgbuf, srcaddr, destaddr,
				      options, timeout, 0, 0, task, action,
				      arg, requestp));
}

isc_result_t
dns_request_createraw2(dns_requestmgr_t *requestmgr, isc_buffer_t *msgbuf,
		       isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		       unsigned int options, unsigned int timeout,
		       unsigned int udptimeout, isc_task_t *task,
		       isc_taskaction_t action, void *arg,
		       dns_request_t **requestp)
{
	unsigned int udpretries = 0;

	if (udptimeout != 0)
		udpretries = timeout / udptimeout;

	return (dns_request_createraw3(requestmgr, msgbuf, srcaddr, destaddr,
				       options, timeout, udptimeout,
				       udpretries, task, action, arg,
				       requestp));
}

isc_result_t
dns_request_createraw3(dns_requestmgr_t *requestmgr, isc_buffer_t *msgbuf,
		       isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		       unsigned int options, unsigned int timeout,
		       unsigned int udptimeout, unsigned int udpretries,
		       isc_task_t *task, isc_taskaction_t action, void *arg,
		       dns_request_t **requestp)
{
	dns_request_t *request = NULL;
	isc_task_t *tclone = NULL;
	isc_socket_t *socket = NULL;
	isc_result_t result;
	isc_mem_t *mctx;
	dns_messageid_t	id;
	isc_boolean_t tcp = ISC_FALSE;
	isc_region_t r;

	REQUIRE(VALID_REQUESTMGR(requestmgr));
	REQUIRE(msgbuf != NULL);
	REQUIRE(destaddr != NULL);
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);
	REQUIRE(requestp != NULL && *requestp == NULL);
	REQUIRE(timeout > 0);
	if (srcaddr != NULL)
		REQUIRE(isc_sockaddr_pf(srcaddr) == isc_sockaddr_pf(destaddr));

	mctx = requestmgr->mctx;

	req_log(ISC_LOG_DEBUG(3), "dns_request_createraw");

	if (isblackholed(requestmgr->dispatchmgr, destaddr))
		return (DNS_R_BLACKHOLED);

	request = NULL;
	result = new_request(mctx, &request);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (udptimeout == 0 && udpretries != 0) {
		udptimeout = timeout / (udpretries + 1);
		if (udptimeout == 0)
			udptimeout = 1;
	}
	request->udpcount = udpretries;

	/*
	 * Create timer now.  We will set it below once.
	 */
	result = isc_timer_create(requestmgr->timermgr, isc_timertype_inactive,
				  NULL, NULL, task, req_timeout, request,
				  &request->timer);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	request->event = (dns_requestevent_t *)
		isc_event_allocate(mctx, task, DNS_EVENT_REQUESTDONE,
				   action, arg, sizeof(dns_requestevent_t));
	if (request->event == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	isc_task_attach(task, &tclone);
	request->event->ev_sender = task;
	request->event->request = request;
	request->event->result = ISC_R_FAILURE;

	isc_buffer_usedregion(msgbuf, &r);
	if (r.length < DNS_MESSAGE_HEADERLEN || r.length > 65535) {
		result = DNS_R_FORMERR;
		goto cleanup;
	}

	if ((options & DNS_REQUESTOPT_TCP) != 0 || r.length > 512)
		tcp = ISC_TRUE;

	result = get_dispatch(tcp, requestmgr, srcaddr, destaddr,
			      &request->dispatch);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = dns_dispatch_addresponse2(request->dispatch, destaddr, task,
					   req_response, request, &id,
					   &request->dispentry,
					   requestmgr->socketmgr);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	socket = req_getsocket(request);
	INSIST(socket != NULL);

	result = isc_buffer_allocate(mctx, &request->query,
				     r.length + (tcp ? 2 : 0));
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	if (tcp)
		isc_buffer_putuint16(request->query, (isc_uint16_t)r.length);
	result = isc_buffer_copyregion(request->query, &r);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	/* Add message ID. */
	isc_buffer_usedregion(request->query, &r);
	if (tcp)
		isc_region_consume(&r, 2);
	r.base[0] = (id>>8) & 0xff;
	r.base[1] = id & 0xff;

	LOCK(&requestmgr->lock);
	if (requestmgr->exiting) {
		UNLOCK(&requestmgr->lock);
		result = ISC_R_SHUTTINGDOWN;
		goto cleanup;
	}
	requestmgr_attach(requestmgr, &request->requestmgr);
	request->hash = mgr_gethash(requestmgr);
	ISC_LIST_APPEND(requestmgr->requests, request, link);
	UNLOCK(&requestmgr->lock);

	result = set_timer(request->timer, timeout, tcp ? 0 : udptimeout);
	if (result != ISC_R_SUCCESS)
		goto unlink;

	request->destaddr = *destaddr;
	if (tcp) {
		result = isc_socket_connect(socket, destaddr, task,
					    req_connected, request);
		if (result != ISC_R_SUCCESS)
			goto unlink;
		request->flags |= DNS_REQUEST_F_CONNECTING|DNS_REQUEST_F_TCP;
	} else {
		result = req_send(request, task, destaddr);
		if (result != ISC_R_SUCCESS)
			goto unlink;
	}

	req_log(ISC_LOG_DEBUG(3), "dns_request_createraw: request %p",
		request);
	*requestp = request;
	return (ISC_R_SUCCESS);

 unlink:
	LOCK(&requestmgr->lock);
	ISC_LIST_UNLINK(requestmgr->requests, request, link);
	UNLOCK(&requestmgr->lock);

 cleanup:
	if (tclone != NULL)
		isc_task_detach(&tclone);
	req_destroy(request);
	req_log(ISC_LOG_DEBUG(3), "dns_request_createraw: failed %s",
		dns_result_totext(result));
	return (result);
}

isc_result_t
dns_request_create(dns_requestmgr_t *requestmgr, dns_message_t *message,
		   isc_sockaddr_t *address, unsigned int options,
		   dns_tsigkey_t *key,
		   unsigned int timeout, isc_task_t *task,
		   isc_taskaction_t action, void *arg,
		   dns_request_t **requestp)
{
	return (dns_request_createvia3(requestmgr, message, NULL, address,
				       options, key, timeout, 0, 0, task,
				       action, arg, requestp));
}

isc_result_t
dns_request_createvia(dns_requestmgr_t *requestmgr, dns_message_t *message,
		      isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		      unsigned int options, dns_tsigkey_t *key,
		      unsigned int timeout, isc_task_t *task,
		      isc_taskaction_t action, void *arg,
		      dns_request_t **requestp)
{
	return(dns_request_createvia3(requestmgr, message, srcaddr, destaddr,
				      options, key, timeout, 0, 0, task,
				      action, arg, requestp));
}

isc_result_t
dns_request_createvia2(dns_requestmgr_t *requestmgr, dns_message_t *message,
		       isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		       unsigned int options, dns_tsigkey_t *key,
		       unsigned int timeout, unsigned int udptimeout,
		       isc_task_t *task, isc_taskaction_t action, void *arg,
		       dns_request_t **requestp)
{
	unsigned int udpretries = 0;

	if (udptimeout != 0)
		udpretries = timeout / udptimeout;
	return (dns_request_createvia3(requestmgr, message, srcaddr, destaddr,
				       options, key, timeout, udptimeout,
				       udpretries, task, action, arg,
				       requestp));
}

isc_result_t
dns_request_createvia3(dns_requestmgr_t *requestmgr, dns_message_t *message,
		       isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		       unsigned int options, dns_tsigkey_t *key,
		       unsigned int timeout, unsigned int udptimeout,
		       unsigned int udpretries, isc_task_t *task,
		       isc_taskaction_t action, void *arg,
		       dns_request_t **requestp)
{
	dns_request_t *request = NULL;
	isc_task_t *tclone = NULL;
	isc_socket_t *socket = NULL;
	isc_result_t result;
	isc_mem_t *mctx;
	dns_messageid_t	id;
	isc_boolean_t tcp;
	isc_boolean_t setkey = ISC_TRUE;

	REQUIRE(VALID_REQUESTMGR(requestmgr));
	REQUIRE(message != NULL);
	REQUIRE(destaddr != NULL);
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);
	REQUIRE(requestp != NULL && *requestp == NULL);
	REQUIRE(timeout > 0);
	if (srcaddr != NULL)
		REQUIRE(isc_sockaddr_pf(srcaddr) == isc_sockaddr_pf(destaddr));

	mctx = requestmgr->mctx;

	req_log(ISC_LOG_DEBUG(3), "dns_request_createvia");

	if (isblackholed(requestmgr->dispatchmgr, destaddr))
		return (DNS_R_BLACKHOLED);

	request = NULL;
	result = new_request(mctx, &request);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (udptimeout == 0 && udpretries != 0) {
		udptimeout = timeout / (udpretries + 1);
		if (udptimeout == 0)
			udptimeout = 1;
	}
	request->udpcount = udpretries;

	/*
	 * Create timer now.  We will set it below once.
	 */
	result = isc_timer_create(requestmgr->timermgr, isc_timertype_inactive,
				  NULL, NULL, task, req_timeout, request,
				  &request->timer);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	request->event = (dns_requestevent_t *)
		isc_event_allocate(mctx, task, DNS_EVENT_REQUESTDONE,
				   action, arg, sizeof(dns_requestevent_t));
	if (request->event == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	isc_task_attach(task, &tclone);
	request->event->ev_sender = task;
	request->event->request = request;
	request->event->result = ISC_R_FAILURE;
	if (key != NULL)
		dns_tsigkey_attach(key, &request->tsigkey);

 use_tcp:
	tcp = ISC_TF((options & DNS_REQUESTOPT_TCP) != 0);
	result = get_dispatch(tcp, requestmgr, srcaddr, destaddr,
			      &request->dispatch);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = dns_dispatch_addresponse2(request->dispatch, destaddr, task,
					   req_response, request, &id,
					   &request->dispentry,
					   requestmgr->socketmgr);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	socket = req_getsocket(request);
	INSIST(socket != NULL);

	message->id = id;
	if (setkey) {
		result = dns_message_settsigkey(message, request->tsigkey);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}
	result = req_render(message, &request->query, options, mctx);
	if (result == DNS_R_USETCP &&
	    (options & DNS_REQUESTOPT_TCP) == 0) {
		/*
		 * Try again using TCP.
		 */
		dns_message_renderreset(message);
		dns_dispatch_removeresponse(&request->dispentry, NULL);
		dns_dispatch_detach(&request->dispatch);
		socket = NULL;
		options |= DNS_REQUESTOPT_TCP;
		setkey = ISC_FALSE;
		goto use_tcp;
	}
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = dns_message_getquerytsig(message, mctx, &request->tsig);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	LOCK(&requestmgr->lock);
	if (requestmgr->exiting) {
		UNLOCK(&requestmgr->lock);
		result = ISC_R_SHUTTINGDOWN;
		goto cleanup;
	}
	requestmgr_attach(requestmgr, &request->requestmgr);
	request->hash = mgr_gethash(requestmgr);
	ISC_LIST_APPEND(requestmgr->requests, request, link);
	UNLOCK(&requestmgr->lock);

	result = set_timer(request->timer, timeout, tcp ? 0 : udptimeout);
	if (result != ISC_R_SUCCESS)
		goto unlink;

	request->destaddr = *destaddr;
	if (tcp) {
		result = isc_socket_connect(socket, destaddr, task,
					    req_connected, request);
		if (result != ISC_R_SUCCESS)
			goto unlink;
		request->flags |= DNS_REQUEST_F_CONNECTING|DNS_REQUEST_F_TCP;
	} else {
		result = req_send(request, task, destaddr);
		if (result != ISC_R_SUCCESS)
			goto unlink;
	}

	req_log(ISC_LOG_DEBUG(3), "dns_request_createvia: request %p",
		request);
	*requestp = request;
	return (ISC_R_SUCCESS);

 unlink:
	LOCK(&requestmgr->lock);
	ISC_LIST_UNLINK(requestmgr->requests, request, link);
	UNLOCK(&requestmgr->lock);

 cleanup:
	if (tclone != NULL)
		isc_task_detach(&tclone);
	req_destroy(request);
	req_log(ISC_LOG_DEBUG(3), "dns_request_createvia: failed %s",
		dns_result_totext(result));
	return (result);
}

static isc_result_t
req_render(dns_message_t *message, isc_buffer_t **bufferp,
	   unsigned int options, isc_mem_t *mctx)
{
	isc_buffer_t *buf1 = NULL;
	isc_buffer_t *buf2 = NULL;
	isc_result_t result;
	isc_region_t r;
	isc_boolean_t tcp = ISC_FALSE;
	dns_compress_t cctx;
	isc_boolean_t cleanup_cctx = ISC_FALSE;

	REQUIRE(bufferp != NULL && *bufferp == NULL);

	req_log(ISC_LOG_DEBUG(3), "request_render");

	/*
	 * Create buffer able to hold largest possible message.
	 */
	result = isc_buffer_allocate(mctx, &buf1, 65535);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_compress_init(&cctx, -1, mctx);
	if (result != ISC_R_SUCCESS)
		return (result);
	cleanup_cctx = ISC_TRUE;

	/*
	 * Render message.
	 */
	result = dns_message_renderbegin(message, &cctx, buf1);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_rendersection(message, DNS_SECTION_QUESTION, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_rendersection(message, DNS_SECTION_ANSWER, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_rendersection(message, DNS_SECTION_AUTHORITY, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_rendersection(message, DNS_SECTION_ADDITIONAL, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_renderend(message);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	dns_compress_invalidate(&cctx);
	cleanup_cctx = ISC_FALSE;

	/*
	 * Copy rendered message to exact sized buffer.
	 */
	isc_buffer_usedregion(buf1, &r);
	if ((options & DNS_REQUESTOPT_TCP) != 0) {
		tcp = ISC_TRUE;
	} else if (r.length > 512) {
		result = DNS_R_USETCP;
		goto cleanup;
	}
	result = isc_buffer_allocate(mctx, &buf2, r.length + (tcp ? 2 : 0));
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	if (tcp)
		isc_buffer_putuint16(buf2, (isc_uint16_t)r.length);
	result = isc_buffer_copyregion(buf2, &r);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	/*
	 * Cleanup and return.
	 */
	isc_buffer_free(&buf1);
	*bufferp = buf2;
	return (ISC_R_SUCCESS);

 cleanup:
	dns_message_renderreset(message);
	if (buf1 != NULL)
		isc_buffer_free(&buf1);
	if (buf2 != NULL)
		isc_buffer_free(&buf2);
	if (cleanup_cctx)
		dns_compress_invalidate(&cctx);
	return (result);
}


/*
 * If this request is no longer waiting for events,
 * send the completion event.  This will ultimately
 * cause the request to be destroyed.
 *
 * Requires:
 *	'request' is locked by the caller.
 */
static void
send_if_done(dns_request_t *request, isc_result_t result) {
	if (!DNS_REQUEST_CONNECTING(request) &&
	    !DNS_REQUEST_SENDING(request) &&
	    !request->canceling)
		req_sendevent(request, result);
}

/*
 * Handle the control event.
 */
static void
do_cancel(isc_task_t *task, isc_event_t *event) {
	dns_request_t *request = event->ev_arg;
	UNUSED(task);
	INSIST(event->ev_type == DNS_EVENT_REQUESTCONTROL);
	LOCK(&request->requestmgr->locks[request->hash]);
	request->canceling = ISC_FALSE;
	if (!DNS_REQUEST_CANCELED(request))
		req_cancel(request);
	send_if_done(request, ISC_R_CANCELED);
	UNLOCK(&request->requestmgr->locks[request->hash]);
}

void
dns_request_cancel(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));

	req_log(ISC_LOG_DEBUG(3), "dns_request_cancel: request %p", request);

	REQUIRE(VALID_REQUEST(request));

	LOCK(&request->requestmgr->locks[request->hash]);
	if (!request->canceling && !DNS_REQUEST_CANCELED(request)) {
		isc_event_t *ev =  &request->ctlevent;
		isc_task_send(request->event->ev_sender, &ev);
		request->canceling = ISC_TRUE;
	}
	UNLOCK(&request->requestmgr->locks[request->hash]);
}

isc_result_t
dns_request_getresponse(dns_request_t *request, dns_message_t *message,
			unsigned int options)
{
	isc_result_t result;

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(request->answer != NULL);

	req_log(ISC_LOG_DEBUG(3), "dns_request_getresponse: request %p",
		request);

	result = dns_message_setquerytsig(message, request->tsig);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = dns_message_settsigkey(message, request->tsigkey);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = dns_message_parse(message, request->answer, options);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (request->tsigkey != NULL)
		result = dns_tsig_verify(request->answer, message, NULL, NULL);
	return (result);
}

isc_boolean_t
dns_request_usedtcp(dns_request_t *request) {
	REQUIRE(VALID_REQUEST(request));

	return (ISC_TF((request->flags & DNS_REQUEST_F_TCP) != 0));
}

void
dns_request_destroy(dns_request_t **requestp) {
	dns_request_t *request;

	REQUIRE(requestp != NULL && VALID_REQUEST(*requestp));

	request = *requestp;

	req_log(ISC_LOG_DEBUG(3), "dns_request_destroy: request %p", request);

	LOCK(&request->requestmgr->lock);
	LOCK(&request->requestmgr->locks[request->hash]);
	ISC_LIST_UNLINK(request->requestmgr->requests, request, link);
	INSIST(!DNS_REQUEST_CONNECTING(request));
	INSIST(!DNS_REQUEST_SENDING(request));
	UNLOCK(&request->requestmgr->locks[request->hash]);
	UNLOCK(&request->requestmgr->lock);

	/*
	 * These should have been cleaned up by req_cancel() before
	 * the completion event was sent.
	 */
	INSIST(!ISC_LINK_LINKED(request, link));
	INSIST(request->dispentry == NULL);
	INSIST(request->dispatch == NULL);
	INSIST(request->timer == NULL);

	req_destroy(request);

	*requestp = NULL;
}

/***
 *** Private: request.
 ***/

static isc_socket_t *
req_getsocket(dns_request_t *request) {
	unsigned int dispattr;
	isc_socket_t *socket;

	dispattr = dns_dispatch_getattributes(request->dispatch);
	if ((dispattr & DNS_DISPATCHATTR_EXCLUSIVE) != 0) {
		INSIST(request->dispentry != NULL);
		socket = dns_dispatch_getentrysocket(request->dispentry);
	} else
		socket = dns_dispatch_getsocket(request->dispatch);

	return (socket);
}

static void
req_connected(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;
	isc_result_t result;
	dns_request_t *request = event->ev_arg;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_CONNECT);
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(DNS_REQUEST_CONNECTING(request));

	req_log(ISC_LOG_DEBUG(3), "req_connected: request %p", request);

	LOCK(&request->requestmgr->locks[request->hash]);
	request->flags &= ~DNS_REQUEST_F_CONNECTING;

	if (DNS_REQUEST_CANCELED(request)) {
		/*
		 * Send delayed event.
		 */
		if (DNS_REQUEST_TIMEDOUT(request))
			send_if_done(request, ISC_R_TIMEDOUT);
		else
			send_if_done(request, ISC_R_CANCELED);
	} else {
		dns_dispatch_starttcp(request->dispatch);
		result = sevent->result;
		if (result == ISC_R_SUCCESS)
			result = req_send(request, task, NULL);

		if (result != ISC_R_SUCCESS) {
			req_cancel(request);
			send_if_done(request, ISC_R_CANCELED);
		}
	}
	UNLOCK(&request->requestmgr->locks[request->hash]);
	isc_event_free(&event);
}

static void
req_senddone(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;
	dns_request_t *request = event->ev_arg;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_SENDDONE);
	REQUIRE(VALID_REQUEST(request));
	REQUIRE(DNS_REQUEST_SENDING(request));

	req_log(ISC_LOG_DEBUG(3), "req_senddone: request %p", request);

	UNUSED(task);

	LOCK(&request->requestmgr->locks[request->hash]);
	request->flags &= ~DNS_REQUEST_F_SENDING;

	if (DNS_REQUEST_CANCELED(request)) {
		/*
		 * Send delayed event.
		 */
		if (DNS_REQUEST_TIMEDOUT(request))
			send_if_done(request, ISC_R_TIMEDOUT);
		else
			send_if_done(request, ISC_R_CANCELED);
	} else if (sevent->result != ISC_R_SUCCESS) {
			req_cancel(request);
			send_if_done(request, ISC_R_CANCELED);
	}
	UNLOCK(&request->requestmgr->locks[request->hash]);

	isc_event_free(&event);
}

static void
req_response(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	dns_request_t *request = event->ev_arg;
	dns_dispatchevent_t *devent = (dns_dispatchevent_t *)event;
	isc_region_t r;

	REQUIRE(VALID_REQUEST(request));
	REQUIRE(event->ev_type == DNS_EVENT_DISPATCH);

	UNUSED(task);

	req_log(ISC_LOG_DEBUG(3), "req_response: request %p: %s", request,
		dns_result_totext(devent->result));

	LOCK(&request->requestmgr->locks[request->hash]);
	result = devent->result;
	if (result != ISC_R_SUCCESS)
		goto done;

	/*
	 * Copy buffer to request.
	 */
	isc_buffer_usedregion(&devent->buffer, &r);
	result = isc_buffer_allocate(request->mctx, &request->answer,
				     r.length);
	if (result != ISC_R_SUCCESS)
		goto done;
	result = isc_buffer_copyregion(request->answer, &r);
	if (result != ISC_R_SUCCESS)
		isc_buffer_free(&request->answer);
 done:
	/*
	 * Cleanup.
	 */
	dns_dispatch_removeresponse(&request->dispentry, &devent);
	req_cancel(request);
	/*
	 * Send completion event.
	 */
	send_if_done(request, result);
	UNLOCK(&request->requestmgr->locks[request->hash]);
}

static void
req_timeout(isc_task_t *task, isc_event_t *event) {
	dns_request_t *request = event->ev_arg;
	isc_result_t result;

	REQUIRE(VALID_REQUEST(request));

	req_log(ISC_LOG_DEBUG(3), "req_timeout: request %p", request);

	UNUSED(task);
	LOCK(&request->requestmgr->locks[request->hash]);
	if (event->ev_type == ISC_TIMEREVENT_TICK &&
	    request->udpcount-- != 0) {
		if (! DNS_REQUEST_SENDING(request)) {
			result = req_send(request, task, &request->destaddr);
			if (result != ISC_R_SUCCESS) {
				req_cancel(request);
				send_if_done(request, result);
			}
		}
	} else {
		request->flags |= DNS_REQUEST_F_TIMEDOUT;
		req_cancel(request);
		send_if_done(request, ISC_R_TIMEDOUT);
	}
	UNLOCK(&request->requestmgr->locks[request->hash]);
	isc_event_free(&event);
}

static void
req_sendevent(dns_request_t *request, isc_result_t result) {
	isc_task_t *task;

	REQUIRE(VALID_REQUEST(request));

	req_log(ISC_LOG_DEBUG(3), "req_sendevent: request %p", request);

	/*
	 * Lock held by caller.
	 */
	task = request->event->ev_sender;
	request->event->ev_sender = request;
	request->event->result = result;
	isc_task_sendanddetach(&task, (isc_event_t **)&request->event);
}

static void
req_destroy(dns_request_t *request) {
	isc_mem_t *mctx;

	REQUIRE(VALID_REQUEST(request));

	req_log(ISC_LOG_DEBUG(3), "req_destroy: request %p", request);

	request->magic = 0;
	if (request->query != NULL)
		isc_buffer_free(&request->query);
	if (request->answer != NULL)
		isc_buffer_free(&request->answer);
	if (request->event != NULL)
		isc_event_free((isc_event_t **)&request->event);
	if (request->dispentry != NULL)
		dns_dispatch_removeresponse(&request->dispentry, NULL);
	if (request->dispatch != NULL)
		dns_dispatch_detach(&request->dispatch);
	if (request->timer != NULL)
		isc_timer_detach(&request->timer);
	if (request->tsig != NULL)
		isc_buffer_free(&request->tsig);
	if (request->tsigkey != NULL)
		dns_tsigkey_detach(&request->tsigkey);
	if (request->requestmgr != NULL)
		requestmgr_detach(&request->requestmgr);
	mctx = request->mctx;
	isc_mem_put(mctx, request, sizeof(*request));
	isc_mem_detach(&mctx);
}

/*
 * Stop the current request.  Must be called from the request's task.
 */
static void
req_cancel(dns_request_t *request) {
	isc_socket_t *socket;
	unsigned int dispattr;

	REQUIRE(VALID_REQUEST(request));

	req_log(ISC_LOG_DEBUG(3), "req_cancel: request %p", request);

	/*
	 * Lock held by caller.
	 */
	request->flags |= DNS_REQUEST_F_CANCELED;

	if (request->timer != NULL)
		isc_timer_detach(&request->timer);
	dispattr = dns_dispatch_getattributes(request->dispatch);
	socket = NULL;
	if (DNS_REQUEST_CONNECTING(request) || DNS_REQUEST_SENDING(request)) {
		if ((dispattr & DNS_DISPATCHATTR_EXCLUSIVE) != 0) {
			if (request->dispentry != NULL) {
				socket = dns_dispatch_getentrysocket(
					request->dispentry);
			}
		} else
			socket = dns_dispatch_getsocket(request->dispatch);
		if (DNS_REQUEST_CONNECTING(request) && socket != NULL)
			isc_socket_cancel(socket, NULL, ISC_SOCKCANCEL_CONNECT);
		if (DNS_REQUEST_SENDING(request) && socket != NULL)
			isc_socket_cancel(socket, NULL, ISC_SOCKCANCEL_SEND);
	}
	if (request->dispentry != NULL)
		dns_dispatch_removeresponse(&request->dispentry, NULL);
	dns_dispatch_detach(&request->dispatch);
}

static void
req_log(int level, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		       DNS_LOGMODULE_REQUEST, level, fmt, ap);
	va_end(ap);
}
