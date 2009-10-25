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

/* $Id: dispatch.h,v 1.60.82.2 2009/01/29 23:47:44 tbox Exp $ */

#ifndef DNS_DISPATCH_H
#define DNS_DISPATCH_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/dispatch.h
 * \brief
 * DNS Dispatch Management
 * 	Shared UDP and single-use TCP dispatches for queries and responses.
 *
 * MP:
 *
 *\li     	All locking is performed internally to each dispatch.
 * 	Restrictions apply to dns_dispatch_removeresponse().
 *
 * Reliability:
 *
 * Resources:
 *
 * Security:
 *
 *\li	Depends on the isc_socket_t and dns_message_t for prevention of
 *	buffer overruns.
 *
 * Standards:
 *
 *\li	None.
 */

/***
 *** Imports
 ***/

#include <isc/buffer.h>
#include <isc/lang.h>
#include <isc/socket.h>
#include <isc/types.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*%
 * This event is sent to a task when a response comes in.
 * No part of this structure should ever be modified by the caller,
 * other than parts of the buffer.  The holy parts of the buffer are
 * the base and size of the buffer.  All other parts of the buffer may
 * be used.  On event delivery the used region contains the packet.
 *
 * "id" is the received message id,
 *
 * "addr" is the host that sent it to us,
 *
 * "buffer" holds state on the received data.
 *
 * The "free" routine for this event will clean up itself as well as
 * any buffer space allocated from common pools.
 */

struct dns_dispatchevent {
	ISC_EVENT_COMMON(dns_dispatchevent_t);	/*%< standard event common */
	isc_result_t		result;		/*%< result code */
	isc_int32_t		id;		/*%< message id */
	isc_sockaddr_t		addr;		/*%< address recv'd from */
	struct in6_pktinfo	pktinfo;	/*%< reply info for v6 */
	isc_buffer_t	        buffer;		/*%< data buffer */
	isc_uint32_t		attributes;	/*%< mirrored from socket.h */
};

/*@{*/
/*%
 * Attributes for added dispatchers.
 *
 * Values with the mask 0xffff0000 are application defined.
 * Values with the mask 0x0000ffff are library defined.
 *
 * Insane values (like setting both TCP and UDP) are not caught.  Don't
 * do that.
 *
 * _PRIVATE
 *	The dispatcher cannot be shared.
 *
 * _TCP, _UDP
 *	The dispatcher is a TCP or UDP socket.
 *
 * _IPV4, _IPV6
 *	The dispatcher uses an IPv4 or IPv6 socket.
 *
 * _NOLISTEN
 *	The dispatcher should not listen on the socket.
 *
 * _MAKEQUERY
 *	The dispatcher can be used to issue queries to other servers, and
 *	accept replies from them.
 *
 * _RANDOMPORT
 *	Previously used to indicate that the port of a dispatch UDP must be
 *	chosen randomly.  This behavior now always applies and the attribute
 *	is obsoleted.
 *
 * _EXCLUSIVE
 *	A separate socket will be used on-demand for each transaction.
 */
#define DNS_DISPATCHATTR_PRIVATE	0x00000001U
#define DNS_DISPATCHATTR_TCP		0x00000002U
#define DNS_DISPATCHATTR_UDP		0x00000004U
#define DNS_DISPATCHATTR_IPV4		0x00000008U
#define DNS_DISPATCHATTR_IPV6		0x00000010U
#define DNS_DISPATCHATTR_NOLISTEN	0x00000020U
#define DNS_DISPATCHATTR_MAKEQUERY	0x00000040U
#define DNS_DISPATCHATTR_CONNECTED	0x00000080U
/*#define DNS_DISPATCHATTR_RANDOMPORT	0x00000100U*/
#define DNS_DISPATCHATTR_EXCLUSIVE	0x00000200U
/*@}*/

isc_result_t
dns_dispatchmgr_create(isc_mem_t *mctx, isc_entropy_t *entropy,
		       dns_dispatchmgr_t **mgrp);
/*%<
 * Creates a new dispatchmgr object.
 *
 * Requires:
 *\li	"mctx" be a valid memory context.
 *
 *\li	mgrp != NULL && *mgrp == NULL
 *
 *\li	"entropy" may be NULL, in which case an insecure random generator
 *	will be used.  If it is non-NULL, it must be a valid entropy
 *	source.
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- all ok
 *
 *\li	anything else	-- failure
 */


void
dns_dispatchmgr_destroy(dns_dispatchmgr_t **mgrp);
/*%<
 * Destroys the dispatchmgr when it becomes empty.  This could be
 * immediately.
 *
 * Requires:
 *\li	mgrp != NULL && *mgrp is a valid dispatchmgr.
 */


void
dns_dispatchmgr_setblackhole(dns_dispatchmgr_t *mgr, dns_acl_t *blackhole);
/*%<
 * Sets the dispatcher's "blackhole list," a list of addresses that will
 * be ignored by all dispatchers created by the dispatchmgr.
 *
 * Requires:
 * \li	mgrp is a valid dispatchmgr
 * \li	blackhole is a valid acl
 */


dns_acl_t *
dns_dispatchmgr_getblackhole(dns_dispatchmgr_t *mgr);
/*%<
 * Gets a pointer to the dispatcher's current blackhole list,
 * without incrementing its reference count.
 *
 * Requires:
 *\li 	mgr is a valid dispatchmgr
 * Returns:
 *\li	A pointer to the current blackhole list, or NULL.
 */

void
dns_dispatchmgr_setblackportlist(dns_dispatchmgr_t *mgr,
				 dns_portlist_t *portlist);
/*%<
 * This function is deprecated.  Use dns_dispatchmgr_setavailports() instead.
 *
 * Requires:
 *\li	mgr is a valid dispatchmgr
 */

dns_portlist_t *
dns_dispatchmgr_getblackportlist(dns_dispatchmgr_t *mgr);
/*%<
 * This function is deprecated and always returns NULL.
 *
 * Requires:
 *\li	mgr is a valid dispatchmgr
 */

isc_result_t
dns_dispatchmgr_setavailports(dns_dispatchmgr_t *mgr, isc_portset_t *v4portset,
			      isc_portset_t *v6portset);
/*%<
 * Sets a list of UDP ports that can be used for outgoing UDP messages.
 *
 * Requires:
 *\li	mgr is a valid dispatchmgr
 *\li	v4portset is NULL or a valid port set
 *\li	v6portset is NULL or a valid port set
 */

void
dns_dispatchmgr_setstats(dns_dispatchmgr_t *mgr, isc_stats_t *stats);
/*%<
 * Sets statistics counter for the dispatchmgr.  This function is expected to
 * be called only on zone creation (when necessary).
 * Once installed, it cannot be removed or replaced.  Also, there is no
 * interface to get the installed stats from the zone; the caller must keep the
 * stats to reference (e.g. dump) it later.
 *
 * Requires:
 *\li	mgr is a valid dispatchmgr with no managed dispatch.
 *\li	stats is a valid statistics supporting resolver statistics counters
 *	(see dns/stats.h).
 */

isc_result_t
dns_dispatch_getudp(dns_dispatchmgr_t *mgr, isc_socketmgr_t *sockmgr,
		    isc_taskmgr_t *taskmgr, isc_sockaddr_t *localaddr,
		    unsigned int buffersize,
		    unsigned int maxbuffers, unsigned int maxrequests,
		    unsigned int buckets, unsigned int increment,
		    unsigned int attributes, unsigned int mask,
		    dns_dispatch_t **dispp);
/*%<
 * Attach to existing dns_dispatch_t if one is found with dns_dispatchmgr_find,
 * otherwise create a new UDP dispatch.
 *
 * Requires:
 *\li	All pointer parameters be valid for their respective types.
 *
 *\li	dispp != NULL && *disp == NULL
 *
 *\li	512 <= buffersize <= 64k
 *
 *\li	maxbuffers > 0
 *
 *\li	buckets < 2097169
 *
 *\li	increment > buckets
 *
 *\li	(attributes & DNS_DISPATCHATTR_TCP) == 0
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- success.
 *
 *\li	Anything else	-- failure.
 */

isc_result_t
dns_dispatch_createtcp(dns_dispatchmgr_t *mgr, isc_socket_t *sock,
		       isc_taskmgr_t *taskmgr, unsigned int buffersize,
		       unsigned int maxbuffers, unsigned int maxrequests,
		       unsigned int buckets, unsigned int increment,
		       unsigned int attributes, dns_dispatch_t **dispp);
/*%<
 * Create a new dns_dispatch and attach it to the provided isc_socket_t.
 *
 * For all dispatches, "buffersize" is the maximum packet size we will
 * accept.
 *
 * "maxbuffers" and "maxrequests" control the number of buffers in the
 * overall system and the number of buffers which can be allocated to
 * requests.
 *
 * "buckets" is the number of buckets to use, and should be prime.
 *
 * "increment" is used in a collision avoidance function, and needs to be
 * a prime > buckets, and not 2.
 *
 * Requires:
 *
 *\li	mgr is a valid dispatch manager.
 *
 *\li	sock is a valid.
 *
 *\li	task is a valid task that can be used internally to this dispatcher.
 *
 * \li	512 <= buffersize <= 64k
 *
 *\li	maxbuffers > 0.
 *
 *\li	maxrequests <= maxbuffers.
 *
 *\li	buckets < 2097169 (the next prime after 65536 * 32)
 *
 *\li	increment > buckets (and prime).
 *
 *\li	attributes includes #DNS_DISPATCHATTR_TCP and does not include
 *	#DNS_DISPATCHATTR_UDP.
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- success.
 *
 *\li	Anything else	-- failure.
 */

void
dns_dispatch_attach(dns_dispatch_t *disp, dns_dispatch_t **dispp);
/*%<
 * Attach to a dispatch handle.
 *
 * Requires:
 *\li	disp is valid.
 *
 *\li	dispp != NULL && *dispp == NULL
 */

void
dns_dispatch_detach(dns_dispatch_t **dispp);
/*%<
 * Detaches from the dispatch.
 *
 * Requires:
 *\li	dispp != NULL and *dispp be a valid dispatch.
 */

void
dns_dispatch_starttcp(dns_dispatch_t *disp);
/*%<
 * Start processing of a TCP dispatch once the socket connects.
 *
 * Requires:
 *\li	'disp' is valid.
 */

isc_result_t
dns_dispatch_addresponse2(dns_dispatch_t *disp, isc_sockaddr_t *dest,
			  isc_task_t *task, isc_taskaction_t action, void *arg,
			  isc_uint16_t *idp, dns_dispentry_t **resp,
			  isc_socketmgr_t *sockmgr);

isc_result_t
dns_dispatch_addresponse(dns_dispatch_t *disp, isc_sockaddr_t *dest,
			 isc_task_t *task, isc_taskaction_t action, void *arg,
			 isc_uint16_t *idp, dns_dispentry_t **resp);
/*%<
 * Add a response entry for this dispatch.
 *
 * "*idp" is filled in with the assigned message ID, and *resp is filled in
 * to contain the magic token used to request event flow stop.
 *
 * Arranges for the given task to get a callback for response packets.  When
 * the event is delivered, it must be returned using dns_dispatch_freeevent()
 * or through dns_dispatch_removeresponse() for another to be delivered.
 *
 * Requires:
 *\li	"idp" be non-NULL.
 *
 *\li	"task" "action" and "arg" be set as appropriate.
 *
 *\li	"dest" be non-NULL and valid.
 *
 *\li	"resp" be non-NULL and *resp be NULL
 *
 *\li	"sockmgr" be NULL or a valid socket manager.  If 'disp' has
 *	the DNS_DISPATCHATTR_EXCLUSIVE attribute, this must not be NULL,
 *	which also means dns_dispatch_addresponse() cannot be used.
 *
 * Ensures:
 *
 *\li	&lt;id, dest> is a unique tuple.  That means incoming messages
 *	are identifiable.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS		-- all is well.
 *\li	ISC_R_NOMEMORY		-- memory could not be allocated.
 *\li	ISC_R_NOMORE		-- no more message ids can be allocated
 *				   for this destination.
 */


void
dns_dispatch_removeresponse(dns_dispentry_t **resp,
			    dns_dispatchevent_t **sockevent);
/*%<
 * Stops the flow of responses for the provided id and destination.
 * If "sockevent" is non-NULL, the dispatch event and associated buffer is
 * also returned to the system.
 *
 * Requires:
 *\li	"resp" != NULL and "*resp" contain a value previously allocated
 *	by dns_dispatch_addresponse();
 *
 *\li	May only be called from within the task given as the 'task'
 * 	argument to dns_dispatch_addresponse() when allocating '*resp'.
 */

isc_socket_t *
dns_dispatch_getentrysocket(dns_dispentry_t *resp);

isc_socket_t *
dns_dispatch_getsocket(dns_dispatch_t *disp);
/*%<
 * Return the socket associated with this dispatcher.
 *
 * Requires:
 *\li	disp is valid.
 *
 * Returns:
 *\li	The socket the dispatcher is using.
 */

isc_result_t
dns_dispatch_getlocaladdress(dns_dispatch_t *disp, isc_sockaddr_t *addrp);
/*%<
 * Return the local address for this dispatch.
 * This currently only works for dispatches using UDP sockets.
 *
 * Requires:
 *\li	disp is valid.
 *\li	addrp to be non null.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOTIMPLEMENTED
 */

void
dns_dispatch_cancel(dns_dispatch_t *disp);
/*%<
 * cancel outstanding clients
 *
 * Requires:
 *\li	disp is valid.
 */

unsigned int
dns_dispatch_getattributes(dns_dispatch_t *disp);
/*%<
 * Return the attributes (DNS_DISPATCHATTR_xxx) of this dispatch.  Only the
 * non-changeable attributes are expected to be referenced by the caller.
 *
 * Requires:
 *\li	disp is valid.
 */

void
dns_dispatch_changeattributes(dns_dispatch_t *disp,
			      unsigned int attributes, unsigned int mask);
/*%<
 * Set the bits described by "mask" to the corresponding values in
 * "attributes".
 *
 * That is:
 *
 * \code
 *	new = (old & ~mask) | (attributes & mask)
 * \endcode
 *
 * This function has a side effect when #DNS_DISPATCHATTR_NOLISTEN changes.
 * When the flag becomes off, the dispatch will start receiving on the
 * corresponding socket.  When the flag becomes on, receive events on the
 * corresponding socket will be canceled.
 *
 * Requires:
 *\li	disp is valid.
 *
 *\li	attributes are reasonable for the dispatch.  That is, setting the UDP
 *	attribute on a TCP socket isn't reasonable.
 */

void
dns_dispatch_importrecv(dns_dispatch_t *disp, isc_event_t *event);
/*%<
 * Inform the dispatcher of a socket receive.  This is used for sockets
 * shared between dispatchers and clients.  If the dispatcher fails to copy
 * or send the event, nothing happens.
 *
 * Requires:
 *\li 	disp is valid, and the attribute DNS_DISPATCHATTR_NOLISTEN is set.
 * 	event != NULL
 */

ISC_LANG_ENDDECLS

#endif /* DNS_DISPATCH_H */
