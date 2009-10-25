/*
 * Copyright (C) 2004-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: request.h,v 1.27.332.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_REQUEST_H
#define DNS_REQUEST_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/request.h
 *
 * \brief
 * The request module provides simple request/response services useful for
 * sending SOA queries, DNS Notify messages, and dynamic update requests.
 *
 * MP:
 *\li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 */

#include <isc/lang.h>
#include <isc/event.h>

#include <dns/types.h>

#define DNS_REQUESTOPT_TCP 0x00000001U

typedef struct dns_requestevent {
	ISC_EVENT_COMMON(struct dns_requestevent);
	isc_result_t result;
	dns_request_t *request;
} dns_requestevent_t;

ISC_LANG_BEGINDECLS

isc_result_t
dns_requestmgr_create(isc_mem_t *mctx, isc_timermgr_t *timermgr,
		      isc_socketmgr_t *socketmgr, isc_taskmgr_t *taskmgr,
		      dns_dispatchmgr_t *dispatchmgr,
		      dns_dispatch_t *dispatchv4, dns_dispatch_t *dispatchv6,
		      dns_requestmgr_t **requestmgrp);
/*%<
 * Create a request manager.
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context.
 *
 *\li	'timermgr' is a valid timer manager.
 *
 *\li	'socketmgr' is a valid socket manager.
 *
 *\li	'taskmgr' is a valid task manager.
 *
 *\li	'dispatchv4' is a valid dispatcher with an IPv4 UDP socket, or is NULL.
 *
 *\li	'dispatchv6' is a valid dispatcher with an IPv6 UDP socket, or is NULL.
 *
 *\li	requestmgrp != NULL && *requestmgrp == NULL
 *
 * Ensures:
 *
 *\li	On success, *requestmgrp is a valid request manager.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */

void
dns_requestmgr_whenshutdown(dns_requestmgr_t *requestmgr, isc_task_t *task,
			    isc_event_t **eventp);
/*%<
 * Send '*eventp' to 'task' when 'requestmgr' has completed shutdown.
 *
 * Notes:
 *
 *\li	It is not safe to detach the last reference to 'requestmgr' until
 *	shutdown is complete.
 *
 * Requires:
 *
 *\li	'requestmgr' is a valid request manager.
 *
 *\li	'task' is a valid task.
 *
 *\li	*eventp is a valid event.
 *
 * Ensures:
 *
 *\li	*eventp == NULL.
 */

void
dns_requestmgr_shutdown(dns_requestmgr_t *requestmgr);
/*%<
 * Start the shutdown process for 'requestmgr'.
 *
 * Notes:
 *
 *\li	This call has no effect if the request manager is already shutting
 *	down.
 *
 * Requires:
 *
 *\li	'requestmgr' is a valid requestmgr.
 */

void
dns_requestmgr_attach(dns_requestmgr_t *source, dns_requestmgr_t **targetp);
/*%<
 *	Attach to the request manager.  dns_requestmgr_shutdown() must not
 *	have been called on 'source' prior to calling dns_requestmgr_attach().
 *
 * Requires:
 *
 *\li	'source' is a valid requestmgr.
 *
 *\li	'targetp' to be non NULL and '*targetp' to be NULL.
 */

void
dns_requestmgr_detach(dns_requestmgr_t **requestmgrp);
/*%<
 *	Detach from the given requestmgr.  If this is the final detach
 *	requestmgr will be destroyed.  dns_requestmgr_shutdown() must
 *	be called before the final detach.
 *
 * Requires:
 *
 *\li	'*requestmgrp' is a valid requestmgr.
 *
 * Ensures:
 *\li	'*requestmgrp' is NULL.
 */

isc_result_t
dns_request_create(dns_requestmgr_t *requestmgr, dns_message_t *message,
		   isc_sockaddr_t *address, unsigned int options,
		   dns_tsigkey_t *key,
		   unsigned int timeout, isc_task_t *task,
		   isc_taskaction_t action, void *arg,
		   dns_request_t **requestp);
/*%<
 * Create and send a request.
 *
 * Notes:
 *
 *\li	'message' will be rendered and sent to 'address'.  If the
 *	#DNS_REQUESTOPT_TCP option is set, TCP will be used.  The request
 *	will timeout after 'timeout' seconds.
 *
 *\li	When the request completes, successfully, due to a timeout, or
 *	because it was canceled, a completion event will be sent to 'task'.
 *
 * Requires:
 *
 *\li	'message' is a valid DNS message.
 *
 *\li	'address' is a valid sockaddr.
 *
 *\li	'timeout' > 0
 *
 *\li	'task' is a valid task.
 *
 *\li	requestp != NULL && *requestp == NULL
 */

/*% See dns_request_createvia3() */
isc_result_t
dns_request_createvia(dns_requestmgr_t *requestmgr, dns_message_t *message,
		      isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		      unsigned int options, dns_tsigkey_t *key,
		      unsigned int timeout, isc_task_t *task,
		      isc_taskaction_t action, void *arg,
		      dns_request_t **requestp);

/*% See dns_request_createvia3() */
isc_result_t
dns_request_createvia2(dns_requestmgr_t *requestmgr, dns_message_t *message,
		       isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		       unsigned int options, dns_tsigkey_t *key,
		       unsigned int timeout, unsigned int udptimeout,
		       isc_task_t *task, isc_taskaction_t action, void *arg,
		       dns_request_t **requestp);

isc_result_t
dns_request_createvia3(dns_requestmgr_t *requestmgr, dns_message_t *message,
		       isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		       unsigned int options, dns_tsigkey_t *key,
		       unsigned int timeout, unsigned int udptimeout,
		       unsigned int udpretries, isc_task_t *task,
		       isc_taskaction_t action, void *arg,
		       dns_request_t **requestp);
/*%<
 * Create and send a request.
 *
 * Notes:
 *
 *\li	'message' will be rendered and sent to 'address'.  If the
 *	#DNS_REQUESTOPT_TCP option is set, TCP will be used.  The request
 *	will timeout after 'timeout' seconds.  UDP requests will be resent
 *	at 'udptimeout' intervals if non-zero or 'udpretries' is non-zero.
 *
 *\li	When the request completes, successfully, due to a timeout, or
 *	because it was canceled, a completion event will be sent to 'task'.
 *
 * Requires:
 *
 *\li	'message' is a valid DNS message.
 *
 *\li	'dstaddr' is a valid sockaddr.
 *
 *\li	'srcaddr' is a valid sockaddr or NULL.
 *
 *\li	'srcaddr' and 'dstaddr' are the same protocol family.
 *
 *\li	'timeout' > 0
 *
 *\li	'task' is a valid task.
 *
 *\li	requestp != NULL && *requestp == NULL
 */

/*% See dns_request_createraw3() */
isc_result_t
dns_request_createraw(dns_requestmgr_t *requestmgr, isc_buffer_t *msgbuf,
		      isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		      unsigned int options, unsigned int timeout,
		      isc_task_t *task, isc_taskaction_t action, void *arg,
		      dns_request_t **requestp);

/*% See dns_request_createraw3() */
isc_result_t
dns_request_createraw2(dns_requestmgr_t *requestmgr, isc_buffer_t *msgbuf,
		       isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		       unsigned int options, unsigned int timeout,
		       unsigned int udptimeout, isc_task_t *task,
		       isc_taskaction_t action, void *arg,
		       dns_request_t **requestp);

isc_result_t
dns_request_createraw3(dns_requestmgr_t *requestmgr, isc_buffer_t *msgbuf,
		       isc_sockaddr_t *srcaddr, isc_sockaddr_t *destaddr,
		       unsigned int options, unsigned int timeout,
		       unsigned int udptimeout, unsigned int udpretries,
		       isc_task_t *task, isc_taskaction_t action, void *arg,
		       dns_request_t **requestp);
/*!<
 * \brief Create and send a request.
 *
 * Notes:
 *
 *\li	'msgbuf' will be sent to 'destaddr' after setting the id.  If the
 *	#DNS_REQUESTOPT_TCP option is set, TCP will be used.  The request
 *	will timeout after 'timeout' seconds.   UDP requests will be resent
 *	at 'udptimeout' intervals if non-zero or if 'udpretries' is not zero.
 *
 *\li	When the request completes, successfully, due to a timeout, or
 *	because it was canceled, a completion event will be sent to 'task'.
 *
 * Requires:
 *
 *\li	'msgbuf' is a valid DNS message in compressed wire format.
 *
 *\li	'destaddr' is a valid sockaddr.
 *
 *\li	'srcaddr' is a valid sockaddr or NULL.
 *
 *\li	'srcaddr' and 'dstaddr' are the same protocol family.
 *
 *\li	'timeout' > 0
 *
 *\li	'task' is a valid task.
 *
 *\li	requestp != NULL && *requestp == NULL
 */

void
dns_request_cancel(dns_request_t *request);
/*%<
 * Cancel 'request'.
 *
 * Requires:
 *
 *\li	'request' is a valid request.
 *
 * Ensures:
 *
 *\li	If the completion event for 'request' has not yet been sent, it
 *	will be sent, and the result code will be ISC_R_CANCELED.
 */

isc_result_t
dns_request_getresponse(dns_request_t *request, dns_message_t *message,
			unsigned int options);
/*%<
 * Get the response to 'request' by filling in 'message'.
 *
 * 'options' is passed to dns_message_parse().  See dns_message_parse()
 * for more details.
 *
 * Requires:
 *
 *\li	'request' is a valid request for which the caller has received the
 *	completion event.
 *
 *\li	The result code of the completion event was #ISC_R_SUCCESS.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any result that dns_message_parse() can return.
 */

isc_boolean_t
dns_request_usedtcp(dns_request_t *request);
/*%<
 * Return whether this query used TCP or not.  Setting #DNS_REQUESTOPT_TCP
 * in the call to dns_request_create() will cause the function to return
 * #ISC_TRUE, otherwise the result is based on the query message size.
 *
 * Requires:
 *\li	'request' is a valid request.
 *
 * Returns:
 *\li	ISC_TRUE	if TCP was used.
 *\li	ISC_FALSE	if UDP was used.
 */

void
dns_request_destroy(dns_request_t **requestp);
/*%<
 * Destroy 'request'.
 *
 * Requires:
 *
 *\li	'request' is a valid request for which the caller has received the
 *	completion event.
 *
 * Ensures:
 *
 *\li	*requestp == NULL
 */

ISC_LANG_ENDDECLS

#endif /* DNS_REQUEST_H */
