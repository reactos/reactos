/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001, 2003  Internet Software Consortium.
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

/* $Id: resolver.h,v 1.60.56.3 2009/01/29 22:40:35 jinmei Exp $ */

#ifndef DNS_RESOLVER_H
#define DNS_RESOLVER_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/resolver.h
 *
 * \brief
 * This is the BIND 9 resolver, the module responsible for resolving DNS
 * requests by iteratively querying authoritative servers and following
 * referrals.  This is a "full resolver", not to be confused with
 * the stub resolvers most people associate with the word "resolver".
 * The full resolver is part of the caching name server or resolver
 * daemon the stub resolver talks to.
 *
 * MP:
 *\li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	RFCs:	1034, 1035, 2181, TBS
 *\li	Drafts:	TBS
 */

#include <isc/lang.h>
#include <isc/socket.h>

#include <dns/types.h>
#include <dns/fixedname.h>

ISC_LANG_BEGINDECLS

/*%
 * A dns_fetchevent_t is sent when a 'fetch' completes.  Any of 'db',
 * 'node', 'rdataset', and 'sigrdataset' may be bound.  It is the
 * receiver's responsibility to detach before freeing the event.
 * \brief
 * 'rdataset', 'sigrdataset', 'client' and 'id' are the values that were
 * supplied when dns_resolver_createfetch() was called.  They are returned
 *  to the caller so that they may be freed.
 */
typedef struct dns_fetchevent {
	ISC_EVENT_COMMON(struct dns_fetchevent);
	dns_fetch_t *			fetch;
	isc_result_t			result;
	dns_rdatatype_t			qtype;
	dns_db_t *			db;
	dns_dbnode_t *			node;
	dns_rdataset_t *		rdataset;
	dns_rdataset_t *		sigrdataset;
	dns_fixedname_t			foundname;
	isc_sockaddr_t *		client;
	dns_messageid_t			id;
} dns_fetchevent_t;

/*
 * Options that modify how a 'fetch' is done.
 */
#define DNS_FETCHOPT_TCP		0x01	     /*%< Use TCP. */
#define DNS_FETCHOPT_UNSHARED		0x02	     /*%< See below. */
#define DNS_FETCHOPT_RECURSIVE		0x04	     /*%< Set RD? */
#define DNS_FETCHOPT_NOEDNS0		0x08	     /*%< Do not use EDNS. */
#define DNS_FETCHOPT_FORWARDONLY	0x10	     /*%< Only use forwarders. */
#define DNS_FETCHOPT_NOVALIDATE		0x20	     /*%< Disable validation. */
#define DNS_FETCHOPT_EDNS512		0x40	     /*%< Advertise a 512 byte
							  UDP buffer. */
#define DNS_FETCHOPT_WANTNSID           0x80         /*%< Request NSID */

#define	DNS_FETCHOPT_EDNSVERSIONSET	0x00800000
#define	DNS_FETCHOPT_EDNSVERSIONMASK	0xff000000
#define	DNS_FETCHOPT_EDNSVERSIONSHIFT	24

/*
 * Upper bounds of class of query RTT (ms).  Corresponds to
 * dns_resstatscounter_queryrttX statistics counters.
 */
#define DNS_RESOLVER_QRYRTTCLASS0	10
#define DNS_RESOLVER_QRYRTTCLASS0STR	"10"
#define DNS_RESOLVER_QRYRTTCLASS1	100
#define DNS_RESOLVER_QRYRTTCLASS1STR	"100"
#define DNS_RESOLVER_QRYRTTCLASS2	500
#define DNS_RESOLVER_QRYRTTCLASS2STR	"500"
#define DNS_RESOLVER_QRYRTTCLASS3	800
#define DNS_RESOLVER_QRYRTTCLASS3STR	"800"
#define DNS_RESOLVER_QRYRTTCLASS4	1600
#define DNS_RESOLVER_QRYRTTCLASS4STR	"1600"

/*
 * XXXRTH  Should this API be made semi-private?  (I.e.
 * _dns_resolver_create()).
 */

#define DNS_RESOLVER_CHECKNAMES		0x01
#define DNS_RESOLVER_CHECKNAMESFAIL	0x02

isc_result_t
dns_resolver_create(dns_view_t *view,
		    isc_taskmgr_t *taskmgr, unsigned int ntasks,
		    isc_socketmgr_t *socketmgr,
		    isc_timermgr_t *timermgr,
		    unsigned int options,
		    dns_dispatchmgr_t *dispatchmgr,
		    dns_dispatch_t *dispatchv4,
		    dns_dispatch_t *dispatchv6,
		    dns_resolver_t **resp);

/*%<
 * Create a resolver.
 *
 * Notes:
 *
 *\li	Generally, applications should not create a resolver directly, but
 *	should instead call dns_view_createresolver().
 *
 * Requires:
 *
 *\li	'view' is a valid view.
 *
 *\li	'taskmgr' is a valid task manager.
 *
 *\li	'ntasks' > 0.
 *
 *\li	'socketmgr' is a valid socket manager.
 *
 *\li	'timermgr' is a valid timer manager.
 *
 *\li	'dispatchv4' is a valid dispatcher with an IPv4 UDP socket, or is NULL.
 *
 *\li	'dispatchv6' is a valid dispatcher with an IPv6 UDP socket, or is NULL.
 *
 *\li	resp != NULL && *resp == NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS				On success.
 *
 *\li	Anything else				Failure.
 */

void
dns_resolver_freeze(dns_resolver_t *res);
/*%<
 * Freeze resolver.
 *
 * Notes:
 *
 *\li	Certain configuration changes cannot be made after the resolver
 *	is frozen.  Fetches cannot be created until the resolver is frozen.
 *
 * Requires:
 *
 *\li	'res' is a valid, unfrozen resolver.
 *
 * Ensures:
 *
 *\li	'res' is frozen.
 */

void
dns_resolver_prime(dns_resolver_t *res);
/*%<
 * Prime resolver.
 *
 * Notes:
 *
 *\li	Resolvers which have a forwarding policy other than dns_fwdpolicy_only
 *	need to be primed with the root nameservers, otherwise the root
 *	nameserver hints data may be used indefinitely.  This function requests
 *	that the resolver start a priming fetch, if it isn't already priming.
 *
 * Requires:
 *
 *\li	'res' is a valid, frozen resolver.
 */


void
dns_resolver_whenshutdown(dns_resolver_t *res, isc_task_t *task,
			  isc_event_t **eventp);
/*%<
 * Send '*eventp' to 'task' when 'res' has completed shutdown.
 *
 * Notes:
 *
 *\li	It is not safe to detach the last reference to 'res' until
 *	shutdown is complete.
 *
 * Requires:
 *
 *\li	'res' is a valid resolver.
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
dns_resolver_shutdown(dns_resolver_t *res);
/*%<
 * Start the shutdown process for 'res'.
 *
 * Notes:
 *
 *\li	This call has no effect if the resolver is already shutting down.
 *
 * Requires:
 *
 *\li	'res' is a valid resolver.
 */

void
dns_resolver_attach(dns_resolver_t *source, dns_resolver_t **targetp);

void
dns_resolver_detach(dns_resolver_t **resp);

isc_result_t
dns_resolver_createfetch(dns_resolver_t *res, dns_name_t *name,
			 dns_rdatatype_t type,
			 dns_name_t *domain, dns_rdataset_t *nameservers,
			 dns_forwarders_t *forwarders,
			 unsigned int options, isc_task_t *task,
			 isc_taskaction_t action, void *arg,
			 dns_rdataset_t *rdataset,
			 dns_rdataset_t *sigrdataset,
			 dns_fetch_t **fetchp);

isc_result_t
dns_resolver_createfetch2(dns_resolver_t *res, dns_name_t *name,
			  dns_rdatatype_t type,
			  dns_name_t *domain, dns_rdataset_t *nameservers,
			  dns_forwarders_t *forwarders,
			  isc_sockaddr_t *client, isc_uint16_t id,
			  unsigned int options, isc_task_t *task,
			  isc_taskaction_t action, void *arg,
			  dns_rdataset_t *rdataset,
			  dns_rdataset_t *sigrdataset,
			  dns_fetch_t **fetchp);
/*%<
 * Recurse to answer a question.
 *
 * Notes:
 *
 *\li	This call starts a query for 'name', type 'type'.
 *
 *\li	The 'domain' is a parent domain of 'name' for which
 *	a set of name servers 'nameservers' is known.  If no
 *	such name server information is available, set
 * 	'domain' and 'nameservers' to NULL.
 *
 *\li	'forwarders' is unimplemented, and subject to change when
 *	we figure out how selective forwarding will work.
 *
 *\li	When the fetch completes (successfully or otherwise), a
 *	#DNS_EVENT_FETCHDONE event with action 'action' and arg 'arg' will be
 *	posted to 'task'.
 *
 *\li	The values of 'rdataset' and 'sigrdataset' will be returned in
 *	the FETCHDONE event.
 *
 *\li	'client' and 'id' are used for duplicate query detection.  '*client'
 *	must remain stable until after 'action' has been called or
 *	dns_resolver_cancelfetch() is called.
 *
 * Requires:
 *
 *\li	'res' is a valid resolver that has been frozen.
 *
 *\li	'name' is a valid name.
 *
 *\li	'type' is not a meta type other than ANY.
 *
 *\li	'domain' is a valid name or NULL.
 *
 *\li	'nameservers' is a valid NS rdataset (whose owner name is 'domain')
 *	iff. 'domain' is not NULL.
 *
 *\li	'forwarders' is NULL.
 *
 *\li	'client' is a valid sockaddr or NULL.
 *
 *\li	'options' contains valid options.
 *
 *\li	'rdataset' is a valid, disassociated rdataset.
 *
 *\li	'sigrdataset' is NULL, or is a valid, disassociated rdataset.
 *
 *\li	fetchp != NULL && *fetchp == NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS					Success
 *\li	#DNS_R_DUPLICATE
 *\li	#DNS_R_DROP
 *
 *\li	Many other values are possible, all of which indicate failure.
 */

void
dns_resolver_cancelfetch(dns_fetch_t *fetch);
/*%<
 * Cancel 'fetch'.
 *
 * Notes:
 *
 *\li	If 'fetch' has not completed, post its FETCHDONE event with a
 *	result code of #ISC_R_CANCELED.
 *
 * Requires:
 *
 *\li	'fetch' is a valid fetch.
 */

void
dns_resolver_destroyfetch(dns_fetch_t **fetchp);
/*%<
 * Destroy 'fetch'.
 *
 * Requires:
 *
 *\li	'*fetchp' is a valid fetch.
 *
 *\li	The caller has received the FETCHDONE event (either because the
 *	fetch completed or because dns_resolver_cancelfetch() was called).
 *
 * Ensures:
 *
 *\li	*fetchp == NULL.
 */

void
dns_resolver_logfetch(dns_fetch_t *fetch, isc_log_t *lctx,
		      isc_logcategory_t *category, isc_logmodule_t *module,
		      int level, isc_boolean_t duplicateok);
/*%<
 * Dump a log message on internal state at the completion of given 'fetch'.
 * 'lctx', 'category', 'module', and 'level' are used to write the log message.
 * By default, only one log message is written even if the corresponding fetch
 * context serves multiple clients; if 'duplicateok' is true the suppression
 * is disabled and the message can be written every time this function is
 * called.
 *
 * Requires:
 *
 *\li	'fetch' is a valid fetch, and has completed.
 */

dns_dispatchmgr_t *
dns_resolver_dispatchmgr(dns_resolver_t *resolver);

dns_dispatch_t *
dns_resolver_dispatchv4(dns_resolver_t *resolver);

dns_dispatch_t *
dns_resolver_dispatchv6(dns_resolver_t *resolver);

isc_socketmgr_t *
dns_resolver_socketmgr(dns_resolver_t *resolver);

isc_taskmgr_t *
dns_resolver_taskmgr(dns_resolver_t *resolver);

isc_uint32_t
dns_resolver_getlamettl(dns_resolver_t *resolver);
/*%<
 * Get the resolver's lame-ttl.  zero => no lame processing.
 *
 * Requires:
 *\li	'resolver' to be valid.
 */

void
dns_resolver_setlamettl(dns_resolver_t *resolver, isc_uint32_t lame_ttl);
/*%<
 * Set the resolver's lame-ttl.  zero => no lame processing.
 *
 * Requires:
 *\li	'resolver' to be valid.
 */

unsigned int
dns_resolver_nrunning(dns_resolver_t *resolver);
/*%<
 * Return the number of currently running resolutions in this
 * resolver.  This is may be less than the number of outstanding
 * fetches due to multiple identical fetches, or more than the
 * number of of outstanding fetches due to the fact that resolution
 * can continue even though a fetch has been canceled.
 */

isc_result_t
dns_resolver_addalternate(dns_resolver_t *resolver, isc_sockaddr_t *alt,
			  dns_name_t *name, in_port_t port);
/*%<
 * Add alternate addresses to be tried in the event that the nameservers
 * for a zone are not available in the address families supported by the
 * operating system.
 *
 * Require:
 * \li	only one of 'name' or 'alt' to be valid.
 */

void
dns_resolver_setudpsize(dns_resolver_t *resolver, isc_uint16_t udpsize);
/*%<
 * Set the EDNS UDP buffer size advertised by the server.
 */

isc_uint16_t
dns_resolver_getudpsize(dns_resolver_t *resolver);
/*%<
 * Get the current EDNS UDP buffer size.
 */

void
dns_resolver_reset_algorithms(dns_resolver_t *resolver);
/*%<
 * Clear the disabled DNSSEC algorithms.
 */

isc_result_t
dns_resolver_disable_algorithm(dns_resolver_t *resolver, dns_name_t *name,
			       unsigned int alg);
/*%<
 * Mark the give DNSSEC algorithm as disabled and below 'name'.
 * Valid algorithms are less than 256.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_RANGE
 *\li	#ISC_R_NOMEMORY
 */

isc_boolean_t
dns_resolver_algorithm_supported(dns_resolver_t *resolver, dns_name_t *name,
				 unsigned int alg);
/*%<
 * Check if the given algorithm is supported by this resolver.
 * This checks if the algorithm has been disabled via
 * dns_resolver_disable_algorithm() then the underlying
 * crypto libraries if not specifically disabled.
 */

isc_boolean_t
dns_resolver_digest_supported(dns_resolver_t *resolver, unsigned int digest_type);
/*%<
 * Is this digest type supported.
 */

void
dns_resolver_resetmustbesecure(dns_resolver_t *resolver);

isc_result_t
dns_resolver_setmustbesecure(dns_resolver_t *resolver, dns_name_t *name,
			     isc_boolean_t value);

isc_boolean_t
dns_resolver_getmustbesecure(dns_resolver_t *resolver, dns_name_t *name);

void
dns_resolver_setclientsperquery(dns_resolver_t *resolver,
				isc_uint32_t min, isc_uint32_t max);

void
dns_resolver_getclientsperquery(dns_resolver_t *resolver, isc_uint32_t *cur,
				isc_uint32_t *min, isc_uint32_t *max);

isc_boolean_t
dns_resolver_getzeronosoattl(dns_resolver_t *resolver);

void
dns_resolver_setzeronosoattl(dns_resolver_t *resolver, isc_boolean_t state);

unsigned int
dns_resolver_getoptions(dns_resolver_t *resolver);

ISC_LANG_ENDDECLS

#endif /* DNS_RESOLVER_H */
