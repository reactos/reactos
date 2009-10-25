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

/* $Id: resolver.c,v 1.384.14.14 2009/06/02 23:47:13 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/platform.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/random.h>
#include <isc/task.h>
#include <isc/stats.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/adb.h>
#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dispatch.h>
#include <dns/ds.h>
#include <dns/events.h>
#include <dns/forward.h>
#include <dns/keytable.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/ncache.h>
#include <dns/opcode.h>
#include <dns/peer.h>
#include <dns/rbt.h>
#include <dns/rcode.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/rootns.h>
#include <dns/stats.h>
#include <dns/tsig.h>
#include <dns/validator.h>

#define DNS_RESOLVER_TRACE
#ifdef DNS_RESOLVER_TRACE
#define RTRACE(m)       isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "res %p: %s", res, (m))
#define RRTRACE(r, m)   isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "res %p: %s", (r), (m))
#define FCTXTRACE(m)    isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fctx %p(%s'): %s", fctx, fctx->info, (m))
#define FCTXTRACE2(m1, m2) \
			isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fctx %p(%s): %s %s", \
				      fctx, fctx->info, (m1), (m2))
#define FTRACE(m)       isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "fetch %p (fctx %p(%s)): %s", \
				      fetch, fetch->private, \
				      fetch->private->info, (m))
#define QTRACE(m)       isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_RESOLVER, \
				      DNS_LOGMODULE_RESOLVER, \
				      ISC_LOG_DEBUG(3), \
				      "resquery %p (fctx %p(%s)): %s", \
				      query, query->fctx, \
				      query->fctx->info, (m))
#else
#define RTRACE(m)
#define RRTRACE(r, m)
#define FCTXTRACE(m)
#define FTRACE(m)
#define QTRACE(m)
#endif

/*%
 * Maximum EDNS0 input packet size.
 */
#define RECV_BUFFER_SIZE                4096            /* XXXRTH  Constant. */

/*%
 * This defines the maximum number of timeouts we will permit before we
 * disable EDNS0 on the query.
 */
#define MAX_EDNS0_TIMEOUTS      3

typedef struct fetchctx fetchctx_t;

typedef struct query {
	/* Locked by task event serialization. */
	unsigned int			magic;
	fetchctx_t *			fctx;
	isc_mem_t *			mctx;
	dns_dispatchmgr_t *		dispatchmgr;
	dns_dispatch_t *		dispatch;
	isc_boolean_t			exclusivesocket;
	dns_adbaddrinfo_t *		addrinfo;
	isc_socket_t *			tcpsocket;
	isc_time_t			start;
	dns_messageid_t			id;
	dns_dispentry_t *		dispentry;
	ISC_LINK(struct query)		link;
	isc_buffer_t			buffer;
	isc_buffer_t			*tsig;
	dns_tsigkey_t			*tsigkey;
	unsigned int			options;
	unsigned int			attributes;
	unsigned int			sends;
	unsigned int			connects;
	unsigned char			data[512];
} resquery_t;

#define QUERY_MAGIC			ISC_MAGIC('Q', '!', '!', '!')
#define VALID_QUERY(query)		ISC_MAGIC_VALID(query, QUERY_MAGIC)

#define RESQUERY_ATTR_CANCELED          0x02

#define RESQUERY_CONNECTING(q)          ((q)->connects > 0)
#define RESQUERY_CANCELED(q)            (((q)->attributes & \
					  RESQUERY_ATTR_CANCELED) != 0)
#define RESQUERY_SENDING(q)             ((q)->sends > 0)

typedef enum {
	fetchstate_init = 0,            /*%< Start event has not run yet. */
	fetchstate_active,
	fetchstate_done                 /*%< FETCHDONE events posted. */
} fetchstate;

typedef enum {
	badns_unreachable = 0,
	badns_response,
	badns_validation
} badnstype_t;

struct fetchctx {
	/*% Not locked. */
	unsigned int			magic;
	dns_resolver_t *		res;
	dns_name_t			name;
	dns_rdatatype_t			type;
	unsigned int			options;
	unsigned int			bucketnum;
	char *			info;
	/*% Locked by appropriate bucket lock. */
	fetchstate			state;
	isc_boolean_t			want_shutdown;
	isc_boolean_t			cloned;
	isc_boolean_t			spilled;
	unsigned int			references;
	isc_event_t			control_event;
	ISC_LINK(struct fetchctx)       link;
	ISC_LIST(dns_fetchevent_t)      events;
	/*% Locked by task event serialization. */
	dns_name_t			domain;
	dns_rdataset_t			nameservers;
	unsigned int			attributes;
	isc_timer_t *			timer;
	isc_time_t			expires;
	isc_interval_t			interval;
	dns_message_t *			qmessage;
	dns_message_t *			rmessage;
	ISC_LIST(resquery_t)		queries;
	dns_adbfindlist_t		finds;
	dns_adbfind_t *			find;
	dns_adbfindlist_t		altfinds;
	dns_adbfind_t *			altfind;
	dns_adbaddrinfolist_t		forwaddrs;
	dns_adbaddrinfolist_t		altaddrs;
	isc_sockaddrlist_t		forwarders;
	dns_fwdpolicy_t			fwdpolicy;
	isc_sockaddrlist_t		bad;
	isc_sockaddrlist_t		edns;
	isc_sockaddrlist_t		edns512;
	dns_validator_t			*validator;
	ISC_LIST(dns_validator_t)       validators;
	dns_db_t *			cache;
	dns_adb_t *			adb;

	/*%
	 * The number of events we're waiting for.
	 */
	unsigned int			pending;

	/*%
	 * The number of times we've "restarted" the current
	 * nameserver set.  This acts as a failsafe to prevent
	 * us from pounding constantly on a particular set of
	 * servers that, for whatever reason, are not giving
	 * us useful responses, but are responding in such a
	 * way that they are not marked "bad".
	 */
	unsigned int			restarts;

	/*%
	 * The number of timeouts that have occurred since we
	 * last successfully received a response packet.  This
	 * is used for EDNS0 black hole detection.
	 */
	unsigned int			timeouts;

	/*%
	 * Look aside state for DS lookups.
	 */
	dns_name_t 			nsname;
	dns_fetch_t *			nsfetch;
	dns_rdataset_t			nsrrset;

	/*%
	 * Number of queries that reference this context.
	 */
	unsigned int			nqueries;

	/*%
	 * The reason to print when logging a successful
	 * response to a query.
	 */
	const char *			reason;

	/*%
	 * Random numbers to use for mixing up server addresses.
	 */
	isc_uint32_t                    rand_buf;
	isc_uint32_t                    rand_bits;

	/*%
	 * Fetch-local statistics for detailed logging.
	 */
	isc_result_t			result; /*%< fetch result  */
	isc_result_t			vresult; /*%< validation result  */
	int				exitline;
	isc_time_t			start;
	isc_uint64_t			duration;
	isc_boolean_t			logged;
	unsigned int			querysent;
	unsigned int			referrals;
	unsigned int			lamecount;
	unsigned int			neterr;
	unsigned int			badresp;
	unsigned int			adberr;
	unsigned int			findfail;
	unsigned int			valfail;
	isc_boolean_t			timeout;
};

#define FCTX_MAGIC			ISC_MAGIC('F', '!', '!', '!')
#define VALID_FCTX(fctx)		ISC_MAGIC_VALID(fctx, FCTX_MAGIC)

#define FCTX_ATTR_HAVEANSWER            0x0001
#define FCTX_ATTR_GLUING                0x0002
#define FCTX_ATTR_ADDRWAIT              0x0004
#define FCTX_ATTR_SHUTTINGDOWN          0x0008
#define FCTX_ATTR_WANTCACHE             0x0010
#define FCTX_ATTR_WANTNCACHE            0x0020
#define FCTX_ATTR_NEEDEDNS0             0x0040
#define FCTX_ATTR_TRIEDFIND             0x0080
#define FCTX_ATTR_TRIEDALT              0x0100

#define HAVE_ANSWER(f)          (((f)->attributes & FCTX_ATTR_HAVEANSWER) != \
				 0)
#define GLUING(f)               (((f)->attributes & FCTX_ATTR_GLUING) != \
				 0)
#define ADDRWAIT(f)             (((f)->attributes & FCTX_ATTR_ADDRWAIT) != \
				 0)
#define SHUTTINGDOWN(f)         (((f)->attributes & FCTX_ATTR_SHUTTINGDOWN) \
				 != 0)
#define WANTCACHE(f)            (((f)->attributes & FCTX_ATTR_WANTCACHE) != 0)
#define WANTNCACHE(f)           (((f)->attributes & FCTX_ATTR_WANTNCACHE) != 0)
#define NEEDEDNS0(f)            (((f)->attributes & FCTX_ATTR_NEEDEDNS0) != 0)
#define TRIEDFIND(f)            (((f)->attributes & FCTX_ATTR_TRIEDFIND) != 0)
#define TRIEDALT(f)             (((f)->attributes & FCTX_ATTR_TRIEDALT) != 0)

typedef struct {
	dns_adbaddrinfo_t *		addrinfo;
	fetchctx_t *			fctx;
} dns_valarg_t;

struct dns_fetch {
	unsigned int			magic;
	fetchctx_t *			private;
};

#define DNS_FETCH_MAGIC			ISC_MAGIC('F', 't', 'c', 'h')
#define DNS_FETCH_VALID(fetch)		ISC_MAGIC_VALID(fetch, DNS_FETCH_MAGIC)

typedef struct fctxbucket {
	isc_task_t *			task;
	isc_mutex_t			lock;
	ISC_LIST(fetchctx_t)		fctxs;
	isc_boolean_t			exiting;
	isc_mem_t *			mctx;
} fctxbucket_t;

typedef struct alternate {
	isc_boolean_t			isaddress;
	union   {
		isc_sockaddr_t		addr;
		struct {
			dns_name_t      name;
			in_port_t       port;
		} _n;
	} _u;
	ISC_LINK(struct alternate)      link;
} alternate_t;

struct dns_resolver {
	/* Unlocked. */
	unsigned int			magic;
	isc_mem_t *			mctx;
	isc_mutex_t			lock;
	isc_mutex_t			nlock;
	isc_mutex_t			primelock;
	dns_rdataclass_t		rdclass;
	isc_socketmgr_t *		socketmgr;
	isc_timermgr_t *		timermgr;
	isc_taskmgr_t *			taskmgr;
	dns_view_t *			view;
	isc_boolean_t			frozen;
	unsigned int			options;
	dns_dispatchmgr_t *		dispatchmgr;
	dns_dispatch_t *		dispatchv4;
	isc_boolean_t			exclusivev4;
	dns_dispatch_t *		dispatchv6;
	isc_boolean_t			exclusivev6;
	unsigned int			ndisps;
	unsigned int			nbuckets;
	fctxbucket_t *			buckets;
	isc_uint32_t			lame_ttl;
	ISC_LIST(alternate_t)		alternates;
	isc_uint16_t			udpsize;
#if USE_ALGLOCK
	isc_rwlock_t			alglock;
#endif
	dns_rbt_t *			algorithms;
#if USE_MBSLOCK
	isc_rwlock_t			mbslock;
#endif
	dns_rbt_t *			mustbesecure;
	unsigned int			spillatmax;
	unsigned int			spillatmin;
	isc_timer_t *			spillattimer;
	isc_boolean_t			zero_no_soa_ttl;

	/* Locked by lock. */
	unsigned int			references;
	isc_boolean_t			exiting;
	isc_eventlist_t			whenshutdown;
	unsigned int			activebuckets;
	isc_boolean_t			priming;
	unsigned int			spillat;	/* clients-per-query */
	unsigned int			nextdisp;
	/* Locked by primelock. */
	dns_fetch_t *			primefetch;
	/* Locked by nlock. */
	unsigned int			nfctx;
};

#define RES_MAGIC			ISC_MAGIC('R', 'e', 's', '!')
#define VALID_RESOLVER(res)		ISC_MAGIC_VALID(res, RES_MAGIC)

/*%
 * Private addrinfo flags.  These must not conflict with DNS_FETCHOPT_NOEDNS0,
 * which we also use as an addrinfo flag.
 */
#define FCTX_ADDRINFO_MARK              0x0001
#define FCTX_ADDRINFO_FORWARDER         0x1000
#define FCTX_ADDRINFO_TRIED             0x2000
#define UNMARKED(a)                     (((a)->flags & FCTX_ADDRINFO_MARK) \
					 == 0)
#define ISFORWARDER(a)                  (((a)->flags & \
					 FCTX_ADDRINFO_FORWARDER) != 0)
#define TRIED(a)                        (((a)->flags & \
					 FCTX_ADDRINFO_TRIED) != 0)

#define NXDOMAIN(r) (((r)->attributes & DNS_RDATASETATTR_NXDOMAIN) != 0)

static void destroy(dns_resolver_t *res);
static void empty_bucket(dns_resolver_t *res);
static isc_result_t resquery_send(resquery_t *query);
static void resquery_response(isc_task_t *task, isc_event_t *event);
static void resquery_connected(isc_task_t *task, isc_event_t *event);
static void fctx_try(fetchctx_t *fctx, isc_boolean_t retrying);
static isc_boolean_t fctx_destroy(fetchctx_t *fctx);
static isc_result_t ncache_adderesult(dns_message_t *message,
				      dns_db_t *cache, dns_dbnode_t *node,
				      dns_rdatatype_t covers,
				      isc_stdtime_t now, dns_ttl_t maxttl,
				      isc_boolean_t optout,
				      dns_rdataset_t *ardataset,
				      isc_result_t *eresultp);
static void validated(isc_task_t *task, isc_event_t *event);
static void maybe_destroy(fetchctx_t *fctx);
static void add_bad(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo,
		    isc_result_t reason, badnstype_t badtype);

/*%
 * Increment resolver-related statistics counters.
 */
static inline void
inc_stats(dns_resolver_t *res, isc_statscounter_t counter) {
	if (res->view->resstats != NULL)
		isc_stats_increment(res->view->resstats, counter);
}

static isc_result_t
valcreate(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo, dns_name_t *name,
	  dns_rdatatype_t type, dns_rdataset_t *rdataset,
	  dns_rdataset_t *sigrdataset, unsigned int valoptions,
	  isc_task_t *task)
{
	dns_validator_t *validator = NULL;
	dns_valarg_t *valarg;
	isc_result_t result;

	valarg = isc_mem_get(fctx->res->buckets[fctx->bucketnum].mctx,
			     sizeof(*valarg));
	if (valarg == NULL)
		return (ISC_R_NOMEMORY);

	valarg->fctx = fctx;
	valarg->addrinfo = addrinfo;

	if (!ISC_LIST_EMPTY(fctx->validators))
		INSIST((valoptions & DNS_VALIDATOR_DEFER) != 0);

	result = dns_validator_create(fctx->res->view, name, type, rdataset,
				      sigrdataset, fctx->rmessage,
				      valoptions, task, validated, valarg,
				      &validator);
	if (result == ISC_R_SUCCESS) {
		inc_stats(fctx->res, dns_resstatscounter_val);
		if ((valoptions & DNS_VALIDATOR_DEFER) == 0) {
			INSIST(fctx->validator == NULL);
			fctx->validator  = validator;
		}
		ISC_LIST_APPEND(fctx->validators, validator, link);
	} else
		isc_mem_put(fctx->res->buckets[fctx->bucketnum].mctx,
			    valarg, sizeof(*valarg));
	return (result);
}

static isc_boolean_t
rrsig_fromchildzone(fetchctx_t *fctx, dns_rdataset_t *rdataset) {
	dns_namereln_t namereln;
	dns_rdata_rrsig_t rrsig;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	int order;
	isc_result_t result;
	unsigned int labels;

	for (result = dns_rdataset_first(rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset)) {
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &rrsig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		namereln = dns_name_fullcompare(&rrsig.signer, &fctx->domain,
						&order, &labels);
		if (namereln == dns_namereln_subdomain)
			return (ISC_TRUE);
		dns_rdata_reset(&rdata);
	}
	return (ISC_FALSE);
}

static isc_boolean_t
fix_mustbedelegationornxdomain(dns_message_t *message, fetchctx_t *fctx) {
	dns_name_t *name;
	dns_name_t *domain = &fctx->domain;
	dns_rdataset_t *rdataset;
	dns_rdatatype_t type;
	isc_result_t result;
	isc_boolean_t keep_auth = ISC_FALSE;

	if (message->rcode == dns_rcode_nxdomain)
		return (ISC_FALSE);

	/*
	 * A DS RRset can appear anywhere in a zone, even for a delegation-only
	 * zone.  So a response to an explicit query for this type should be
	 * excluded from delegation-only fixup.
	 *
	 * SOA, NS, and DNSKEY can only exist at a zone apex, so a postive
	 * response to a query for these types can never violate the
	 * delegation-only assumption: if the query name is below a
	 * zone cut, the response should normally be a referral, which should
	 * be accepted; if the query name is below a zone cut but the server
	 * happens to have authority for the zone of the query name, the
	 * response is a (non-referral) answer.  But this does not violate
	 * delegation-only because the query name must be in a different zone
	 * due to the "apex-only" nature of these types.  Note that if the
	 * remote server happens to have authority for a child zone of a
	 * delegation-only zone, we may still incorrectly "fix" the response
	 * with NXDOMAIN for queries for other types.  Unfortunately it's
	 * generally impossible to differentiate this case from violation of
	 * the delegation-only assumption.  Once the resolver learns the
	 * correct zone cut, possibly via a separate query for an "apex-only"
	 * type, queries for other types will be resolved correctly.
	 *
	 * A query for type ANY will be accepted if it hits an exceptional
	 * type above in the answer section as it should be from a child
	 * zone.
	 *
	 * Also accept answers with RRSIG records from the child zone.
	 * Direct queries for RRSIG records should not be answered from
	 * the parent zone.
	 */

	if (message->counts[DNS_SECTION_ANSWER] != 0 &&
	    (fctx->type == dns_rdatatype_ns ||
	     fctx->type == dns_rdatatype_ds ||
	     fctx->type == dns_rdatatype_soa ||
	     fctx->type == dns_rdatatype_any ||
	     fctx->type == dns_rdatatype_rrsig ||
	     fctx->type == dns_rdatatype_dnskey)) {
		result = dns_message_firstname(message, DNS_SECTION_ANSWER);
		while (result == ISC_R_SUCCESS) {
			name = NULL;
			dns_message_currentname(message, DNS_SECTION_ANSWER,
						&name);
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				if (!dns_name_equal(name, &fctx->name))
					continue;
				type = rdataset->type;
				/*
				 * RRsig from child?
				 */
				if (type == dns_rdatatype_rrsig &&
				    rrsig_fromchildzone(fctx, rdataset))
					return (ISC_FALSE);
				/*
				 * Direct query for apex records or DS.
				 */
				if (fctx->type == type &&
				    (type == dns_rdatatype_ds ||
				     type == dns_rdatatype_ns ||
				     type == dns_rdatatype_soa ||
				     type == dns_rdatatype_dnskey))
					return (ISC_FALSE);
				/*
				 * Indirect query for apex records or DS.
				 */
				if (fctx->type == dns_rdatatype_any &&
				    (type == dns_rdatatype_ns ||
				     type == dns_rdatatype_ds ||
				     type == dns_rdatatype_soa ||
				     type == dns_rdatatype_dnskey))
					return (ISC_FALSE);
			}
			result = dns_message_nextname(message,
						      DNS_SECTION_ANSWER);
		}
	}

	/*
	 * A NODATA response to a DS query?
	 */
	if (fctx->type == dns_rdatatype_ds &&
	    message->counts[DNS_SECTION_ANSWER] == 0)
		return (ISC_FALSE);

	/* Look for referral or indication of answer from child zone? */
	if (message->counts[DNS_SECTION_AUTHORITY] == 0)
		goto munge;

	result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			type = rdataset->type;
			if (type == dns_rdatatype_soa &&
			    dns_name_equal(name, domain))
				keep_auth = ISC_TRUE;

			if (type != dns_rdatatype_ns &&
			    type != dns_rdatatype_soa &&
			    type != dns_rdatatype_rrsig)
				continue;

			if (type == dns_rdatatype_rrsig) {
				if (rrsig_fromchildzone(fctx, rdataset))
					return (ISC_FALSE);
				else
					continue;
			}

			/* NS or SOA records. */
			if (dns_name_equal(name, domain)) {
				/*
				 * If a query for ANY causes a negative
				 * response, we can be sure that this is
				 * an empty node.  For other type of queries
				 * we cannot differentiate an empty node
				 * from a node that just doesn't have that
				 * type of record.  We only accept the former
				 * case.
				 */
				if (message->counts[DNS_SECTION_ANSWER] == 0 &&
				    fctx->type == dns_rdatatype_any)
					return (ISC_FALSE);
			} else if (dns_name_issubdomain(name, domain)) {
				/* Referral or answer from child zone. */
				return (ISC_FALSE);
			}
		}
		result = dns_message_nextname(message, DNS_SECTION_AUTHORITY);
	}

 munge:
	message->rcode = dns_rcode_nxdomain;
	message->counts[DNS_SECTION_ANSWER] = 0;
	if (!keep_auth)
		message->counts[DNS_SECTION_AUTHORITY] = 0;
	message->counts[DNS_SECTION_ADDITIONAL] = 0;
	return (ISC_TRUE);
}

static inline isc_result_t
fctx_starttimer(fetchctx_t *fctx) {
	/*
	 * Start the lifetime timer for fctx.
	 *
	 * This is also used for stopping the idle timer; in that
	 * case we must purge events already posted to ensure that
	 * no further idle events are delivered.
	 */
	return (isc_timer_reset(fctx->timer, isc_timertype_once,
				&fctx->expires, NULL, ISC_TRUE));
}

static inline void
fctx_stoptimer(fetchctx_t *fctx) {
	isc_result_t result;

	/*
	 * We don't return a result if resetting the timer to inactive fails
	 * since there's nothing to be done about it.  Resetting to inactive
	 * should never fail anyway, since the code as currently written
	 * cannot fail in that case.
	 */
	result = isc_timer_reset(fctx->timer, isc_timertype_inactive,
				  NULL, NULL, ISC_TRUE);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_timer_reset(): %s",
				 isc_result_totext(result));
	}
}


static inline isc_result_t
fctx_startidletimer(fetchctx_t *fctx, isc_interval_t *interval) {
	/*
	 * Start the idle timer for fctx.  The lifetime timer continues
	 * to be in effect.
	 */
	return (isc_timer_reset(fctx->timer, isc_timertype_once,
				&fctx->expires, interval, ISC_FALSE));
}

/*
 * Stopping the idle timer is equivalent to calling fctx_starttimer(), but
 * we use fctx_stopidletimer for readability in the code below.
 */
#define fctx_stopidletimer      fctx_starttimer


static inline void
resquery_destroy(resquery_t **queryp) {
	resquery_t *query;

	REQUIRE(queryp != NULL);
	query = *queryp;
	REQUIRE(!ISC_LINK_LINKED(query, link));

	INSIST(query->tcpsocket == NULL);

	query->fctx->nqueries--;
	if (SHUTTINGDOWN(query->fctx))
		maybe_destroy(query->fctx);     /* Locks bucket. */
	query->magic = 0;
	isc_mem_put(query->mctx, query, sizeof(*query));
	*queryp = NULL;
}

static void
fctx_cancelquery(resquery_t **queryp, dns_dispatchevent_t **deventp,
		 isc_time_t *finish, isc_boolean_t no_response)
{
	fetchctx_t *fctx;
	resquery_t *query;
	unsigned int rtt, rttms;
	unsigned int factor;
	dns_adbfind_t *find;
	dns_adbaddrinfo_t *addrinfo;
	isc_socket_t *socket;

	query = *queryp;
	fctx = query->fctx;

	FCTXTRACE("cancelquery");

	REQUIRE(!RESQUERY_CANCELED(query));

	query->attributes |= RESQUERY_ATTR_CANCELED;

	/*
	 * Should we update the RTT?
	 */
	if (finish != NULL || no_response) {
		if (finish != NULL) {
			/*
			 * We have both the start and finish times for this
			 * packet, so we can compute a real RTT.
			 */
			rtt = (unsigned int)isc_time_microdiff(finish,
							       &query->start);
			factor = DNS_ADB_RTTADJDEFAULT;

			rttms = rtt / 1000;
			if (rttms < DNS_RESOLVER_QRYRTTCLASS0) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt0);
			} else if (rttms < DNS_RESOLVER_QRYRTTCLASS1) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt1);
			} else if (rttms < DNS_RESOLVER_QRYRTTCLASS2) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt2);
			} else if (rttms < DNS_RESOLVER_QRYRTTCLASS3) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt3);
			} else if (rttms < DNS_RESOLVER_QRYRTTCLASS4) {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt4);
			} else {
				inc_stats(fctx->res,
					  dns_resstatscounter_queryrtt5);
			}
		} else {
			/*
			 * We don't have an RTT for this query.  Maybe the
			 * packet was lost, or maybe this server is very
			 * slow.  We don't know.  Increase the RTT.
			 */
			INSIST(no_response);
			rtt = query->addrinfo->srtt + 200000;
			if (rtt > 10000000)
				rtt = 10000000;
			/*
			 * Replace the current RTT with our value.
			 */
			factor = DNS_ADB_RTTADJREPLACE;
		}
		dns_adb_adjustsrtt(fctx->adb, query->addrinfo, rtt, factor);
	}

	/* Remember that the server has been tried. */
	if (!TRIED(query->addrinfo)) {
		dns_adb_changeflags(fctx->adb, query->addrinfo,
				    FCTX_ADDRINFO_TRIED, FCTX_ADDRINFO_TRIED);
	}

	/*
	 * Age RTTs of servers not tried.
	 */
	factor = DNS_ADB_RTTADJAGE;
	if (finish != NULL)
		for (addrinfo = ISC_LIST_HEAD(fctx->forwaddrs);
		     addrinfo != NULL;
		     addrinfo = ISC_LIST_NEXT(addrinfo, publink))
			if (UNMARKED(addrinfo))
				dns_adb_adjustsrtt(fctx->adb, addrinfo,
						   0, factor);

	if (finish != NULL && TRIEDFIND(fctx))
		for (find = ISC_LIST_HEAD(fctx->finds);
		     find != NULL;
		     find = ISC_LIST_NEXT(find, publink))
			for (addrinfo = ISC_LIST_HEAD(find->list);
			     addrinfo != NULL;
			     addrinfo = ISC_LIST_NEXT(addrinfo, publink))
				if (UNMARKED(addrinfo))
					dns_adb_adjustsrtt(fctx->adb, addrinfo,
							   0, factor);

	if (finish != NULL && TRIEDALT(fctx)) {
		for (addrinfo = ISC_LIST_HEAD(fctx->altaddrs);
		     addrinfo != NULL;
		     addrinfo = ISC_LIST_NEXT(addrinfo, publink))
			if (UNMARKED(addrinfo))
				dns_adb_adjustsrtt(fctx->adb, addrinfo,
						   0, factor);
		for (find = ISC_LIST_HEAD(fctx->altfinds);
		     find != NULL;
		     find = ISC_LIST_NEXT(find, publink))
			for (addrinfo = ISC_LIST_HEAD(find->list);
			     addrinfo != NULL;
			     addrinfo = ISC_LIST_NEXT(addrinfo, publink))
				if (UNMARKED(addrinfo))
					dns_adb_adjustsrtt(fctx->adb, addrinfo,
							   0, factor);
	}

	/*
	 * Check for any outstanding socket events.  If they exist, cancel
	 * them and let the event handlers finish the cleanup.  The resolver
	 * only needs to worry about managing the connect and send events;
	 * the dispatcher manages the recv events.
	 */
	if (RESQUERY_CONNECTING(query)) {
		/*
		 * Cancel the connect.
		 */
		if (query->tcpsocket != NULL) {
			isc_socket_cancel(query->tcpsocket, NULL,
					  ISC_SOCKCANCEL_CONNECT);
		} else if (query->dispentry != NULL) {
			INSIST(query->exclusivesocket);
			socket = dns_dispatch_getentrysocket(query->dispentry);
			if (socket != NULL)
				isc_socket_cancel(socket, NULL,
						  ISC_SOCKCANCEL_CONNECT);
		}
	} else if (RESQUERY_SENDING(query)) {
		/*
		 * Cancel the pending send.
		 */
		if (query->exclusivesocket && query->dispentry != NULL)
			socket = dns_dispatch_getentrysocket(query->dispentry);
		else
			socket = dns_dispatch_getsocket(query->dispatch);
		if (socket != NULL)
			isc_socket_cancel(socket, NULL, ISC_SOCKCANCEL_SEND);
	}

	if (query->dispentry != NULL)
		dns_dispatch_removeresponse(&query->dispentry, deventp);

	ISC_LIST_UNLINK(fctx->queries, query, link);

	if (query->tsig != NULL)
		isc_buffer_free(&query->tsig);

	if (query->tsigkey != NULL)
		dns_tsigkey_detach(&query->tsigkey);

	if (query->dispatch != NULL)
		dns_dispatch_detach(&query->dispatch);

	if (! (RESQUERY_CONNECTING(query) || RESQUERY_SENDING(query)))
		/*
		 * It's safe to destroy the query now.
		 */
		resquery_destroy(&query);
}

static void
fctx_cancelqueries(fetchctx_t *fctx, isc_boolean_t no_response) {
	resquery_t *query, *next_query;

	FCTXTRACE("cancelqueries");

	for (query = ISC_LIST_HEAD(fctx->queries);
	     query != NULL;
	     query = next_query) {
		next_query = ISC_LIST_NEXT(query, link);
		fctx_cancelquery(&query, NULL, NULL, no_response);
	}
}

static void
fctx_cleanupfinds(fetchctx_t *fctx) {
	dns_adbfind_t *find, *next_find;

	REQUIRE(ISC_LIST_EMPTY(fctx->queries));

	for (find = ISC_LIST_HEAD(fctx->finds);
	     find != NULL;
	     find = next_find) {
		next_find = ISC_LIST_NEXT(find, publink);
		ISC_LIST_UNLINK(fctx->finds, find, publink);
		dns_adb_destroyfind(&find);
	}
	fctx->find = NULL;
}

static void
fctx_cleanupaltfinds(fetchctx_t *fctx) {
	dns_adbfind_t *find, *next_find;

	REQUIRE(ISC_LIST_EMPTY(fctx->queries));

	for (find = ISC_LIST_HEAD(fctx->altfinds);
	     find != NULL;
	     find = next_find) {
		next_find = ISC_LIST_NEXT(find, publink);
		ISC_LIST_UNLINK(fctx->altfinds, find, publink);
		dns_adb_destroyfind(&find);
	}
	fctx->altfind = NULL;
}

static void
fctx_cleanupforwaddrs(fetchctx_t *fctx) {
	dns_adbaddrinfo_t *addr, *next_addr;

	REQUIRE(ISC_LIST_EMPTY(fctx->queries));

	for (addr = ISC_LIST_HEAD(fctx->forwaddrs);
	     addr != NULL;
	     addr = next_addr) {
		next_addr = ISC_LIST_NEXT(addr, publink);
		ISC_LIST_UNLINK(fctx->forwaddrs, addr, publink);
		dns_adb_freeaddrinfo(fctx->adb, &addr);
	}
}

static void
fctx_cleanupaltaddrs(fetchctx_t *fctx) {
	dns_adbaddrinfo_t *addr, *next_addr;

	REQUIRE(ISC_LIST_EMPTY(fctx->queries));

	for (addr = ISC_LIST_HEAD(fctx->altaddrs);
	     addr != NULL;
	     addr = next_addr) {
		next_addr = ISC_LIST_NEXT(addr, publink);
		ISC_LIST_UNLINK(fctx->altaddrs, addr, publink);
		dns_adb_freeaddrinfo(fctx->adb, &addr);
	}
}

static inline void
fctx_stopeverything(fetchctx_t *fctx, isc_boolean_t no_response) {
	FCTXTRACE("stopeverything");
	fctx_cancelqueries(fctx, no_response);
	fctx_cleanupfinds(fctx);
	fctx_cleanupaltfinds(fctx);
	fctx_cleanupforwaddrs(fctx);
	fctx_cleanupaltaddrs(fctx);
	fctx_stoptimer(fctx);
}

static inline void
fctx_sendevents(fetchctx_t *fctx, isc_result_t result, int line) {
	dns_fetchevent_t *event, *next_event;
	isc_task_t *task;
	unsigned int count = 0;
	isc_interval_t i;
	isc_boolean_t logit = ISC_FALSE;
	isc_time_t now;
	unsigned int old_spillat;
	unsigned int new_spillat = 0;	/* initialized to silence
					   compiler warnings */

	/*
	 * Caller must be holding the appropriate bucket lock.
	 */
	REQUIRE(fctx->state == fetchstate_done);

	FCTXTRACE("sendevents");

	/*
	 * Keep some record of fetch result for logging later (if required).
	 */
	fctx->result = result;
	fctx->exitline = line;
	TIME_NOW(&now);
	fctx->duration = isc_time_microdiff(&now, &fctx->start);

	for (event = ISC_LIST_HEAD(fctx->events);
	     event != NULL;
	     event = next_event) {
		next_event = ISC_LIST_NEXT(event, ev_link);
		ISC_LIST_UNLINK(fctx->events, event, ev_link);
		task = event->ev_sender;
		event->ev_sender = fctx;
		if (!HAVE_ANSWER(fctx))
			event->result = result;

		INSIST(result != ISC_R_SUCCESS ||
		       dns_rdataset_isassociated(event->rdataset) ||
		       fctx->type == dns_rdatatype_any ||
		       fctx->type == dns_rdatatype_rrsig ||
		       fctx->type == dns_rdatatype_sig);

		/*
		 * Negative results must be indicated in event->result.
		 */
		if (dns_rdataset_isassociated(event->rdataset) &&
		    event->rdataset->type == dns_rdatatype_none) {
			INSIST(event->result == DNS_R_NCACHENXDOMAIN ||
			       event->result == DNS_R_NCACHENXRRSET);
		}

		isc_task_sendanddetach(&task, ISC_EVENT_PTR(&event));
		count++;
	}

	if ((fctx->attributes & FCTX_ATTR_HAVEANSWER) != 0 &&
	    fctx->spilled &&
	    (count < fctx->res->spillatmax || fctx->res->spillatmax == 0)) {
		LOCK(&fctx->res->lock);
		if (count == fctx->res->spillat && !fctx->res->exiting) {
			old_spillat = fctx->res->spillat;
			fctx->res->spillat += 5;
			if (fctx->res->spillat > fctx->res->spillatmax &&
			    fctx->res->spillatmax != 0)
				fctx->res->spillat = fctx->res->spillatmax;
			new_spillat = fctx->res->spillat;
			if (new_spillat != old_spillat) {
				logit = ISC_TRUE;
			}
			isc_interval_set(&i, 20 * 60, 0);
			result = isc_timer_reset(fctx->res->spillattimer,
						 isc_timertype_ticker, NULL,
						 &i, ISC_TRUE);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
		}
		UNLOCK(&fctx->res->lock);
		if (logit)
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
				      "clients-per-query increased to %u",
				      new_spillat);
	}
}

static inline void
log_edns(fetchctx_t *fctx) {
	char domainbuf[DNS_NAME_FORMATSIZE];

	if (fctx->reason == NULL)
		return;

	dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_EDNS_DISABLED,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
		      "success resolving '%s' (in '%s'?) after %s",
		      fctx->info, domainbuf, fctx->reason);

	fctx->reason = NULL;
}

static void
fctx_done(fetchctx_t *fctx, isc_result_t result, int line) {
	dns_resolver_t *res;
	isc_boolean_t no_response;

	REQUIRE(line >= 0);

	FCTXTRACE("done");

	res = fctx->res;

	if (result == ISC_R_SUCCESS) {
		/*%
		 * Log any deferred EDNS timeout messages.
		 */
		log_edns(fctx);
		no_response = ISC_TRUE;
	 } else
		no_response = ISC_FALSE;

	fctx->reason = NULL;
	fctx_stopeverything(fctx, no_response);

	LOCK(&res->buckets[fctx->bucketnum].lock);

	fctx->state = fetchstate_done;
	fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
	fctx_sendevents(fctx, result, line);

	UNLOCK(&res->buckets[fctx->bucketnum].lock);
}

static void
process_sendevent(resquery_t *query, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;
	isc_boolean_t retry = ISC_FALSE;
	isc_result_t result;
	fetchctx_t *fctx;

	fctx = query->fctx;

	if (RESQUERY_CANCELED(query)) {
		if (query->sends == 0 && query->connects == 0) {
			/*
			 * This query was canceled while the
			 * isc_socket_sendto/connect() was in progress.
			 */
			if (query->tcpsocket != NULL)
				isc_socket_detach(&query->tcpsocket);
			resquery_destroy(&query);
		}
	} else {
		switch (sevent->result) {
		case ISC_R_SUCCESS:
			break;

		case ISC_R_HOSTUNREACH:
		case ISC_R_NETUNREACH:
		case ISC_R_NOPERM:
		case ISC_R_ADDRNOTAVAIL:
		case ISC_R_CONNREFUSED:

			/*
			 * No route to remote.
			 */
			add_bad(fctx, query->addrinfo, sevent->result,
				badns_unreachable);
			fctx_cancelquery(&query, NULL, NULL, ISC_TRUE);
			retry = ISC_TRUE;
			break;

		default:
			fctx_cancelquery(&query, NULL, NULL, ISC_FALSE);
			break;
		}
	}

	isc_event_free(&event);

	if (retry) {
		/*
		 * Behave as if the idle timer has expired.  For TCP
		 * this may not actually reflect the latest timer.
		 */
		fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
		result = fctx_stopidletimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else
			fctx_try(fctx, ISC_TRUE);
	}
}

static void
resquery_udpconnected(isc_task_t *task, isc_event_t *event) {
	resquery_t *query = event->ev_arg;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_CONNECT);

	QTRACE("udpconnected");

	UNUSED(task);

	INSIST(RESQUERY_CONNECTING(query));

	query->connects--;

	process_sendevent(query, event);
}

static void
resquery_senddone(isc_task_t *task, isc_event_t *event) {
	resquery_t *query = event->ev_arg;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_SENDDONE);

	QTRACE("senddone");

	/*
	 * XXXRTH
	 *
	 * Currently we don't wait for the senddone event before retrying
	 * a query.  This means that if we get really behind, we may end
	 * up doing extra work!
	 */

	UNUSED(task);

	INSIST(RESQUERY_SENDING(query));

	query->sends--;

	process_sendevent(query, event);
}

static inline isc_result_t
fctx_addopt(dns_message_t *message, unsigned int version,
	    isc_uint16_t udpsize, isc_boolean_t request_nsid)
{
	dns_rdataset_t *rdataset;
	dns_rdatalist_t *rdatalist;
	dns_rdata_t *rdata;
	isc_result_t result;

	rdatalist = NULL;
	result = dns_message_gettemprdatalist(message, &rdatalist);
	if (result != ISC_R_SUCCESS)
		return (result);
	rdata = NULL;
	result = dns_message_gettemprdata(message, &rdata);
	if (result != ISC_R_SUCCESS)
		return (result);
	rdataset = NULL;
	result = dns_message_gettemprdataset(message, &rdataset);
	if (result != ISC_R_SUCCESS)
		return (result);
	dns_rdataset_init(rdataset);

	rdatalist->type = dns_rdatatype_opt;
	rdatalist->covers = 0;

	/*
	 * Set Maximum UDP buffer size.
	 */
	rdatalist->rdclass = udpsize;

	/*
	 * Set EXTENDED-RCODE and Z to 0, DO to 1.
	 */
	rdatalist->ttl = (version << 16);
	rdatalist->ttl |= DNS_MESSAGEEXTFLAG_DO;

	/*
	 * Set EDNS options if applicable
	 */
	if (request_nsid) {
		/* Send empty NSID option (RFC5001) */
		unsigned char data[4];
		isc_buffer_t buf;

		isc_buffer_init(&buf, data, sizeof(data));
		isc_buffer_putuint16(&buf, DNS_OPT_NSID);
		isc_buffer_putuint16(&buf, 0);
		rdata->data = data;
		rdata->length = sizeof(data);
	} else {
		rdata->data = NULL;
		rdata->length = 0;
	}

	rdata->rdclass = rdatalist->rdclass;
	rdata->type = rdatalist->type;
	rdata->flags = 0;

	ISC_LIST_INIT(rdatalist->rdata);
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	RUNTIME_CHECK(dns_rdatalist_tordataset(rdatalist, rdataset) == ISC_R_SUCCESS);

	return (dns_message_setopt(message, rdataset));
}

static inline void
fctx_setretryinterval(fetchctx_t *fctx, unsigned int rtt) {
	unsigned int seconds;
	unsigned int us;

	/*
	 * We retry every .8 seconds the first two times through the address
	 * list, and then we do exponential back-off.
	 */
	if (fctx->restarts < 3)
		us = 800000;
	else
		us = (800000 << (fctx->restarts - 2));

	/*
	 * Double the round-trip time.
	 */
	rtt *= 2;

	/*
	 * Always wait for at least the doubled round-trip time.
	 */
	if (us < rtt)
		us = rtt;

	/*
	 * But don't ever wait for more than 10 seconds.
	 */
	if (us > 10000000)
		us = 10000000;

	seconds = us / 1000000;
	us -= seconds * 1000000;
	isc_interval_set(&fctx->interval, seconds, us * 1000);
}

static isc_result_t
fctx_query(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo,
	   unsigned int options)
{
	dns_resolver_t *res;
	isc_task_t *task;
	isc_result_t result;
	resquery_t *query;
	isc_sockaddr_t addr;
	isc_boolean_t have_addr = ISC_FALSE;
	unsigned int srtt;

	FCTXTRACE("query");

	res = fctx->res;
	task = res->buckets[fctx->bucketnum].task;

	srtt = addrinfo->srtt;
	if (ISFORWARDER(addrinfo) && srtt < 1000000)
		srtt = 1000000;

	fctx_setretryinterval(fctx, srtt);
	result = fctx_startidletimer(fctx, &fctx->interval);
	if (result != ISC_R_SUCCESS)
		return (result);

	INSIST(ISC_LIST_EMPTY(fctx->validators));

	dns_message_reset(fctx->rmessage, DNS_MESSAGE_INTENTPARSE);

	query = isc_mem_get(res->buckets[fctx->bucketnum].mctx,
			    sizeof(*query));
	if (query == NULL) {
		result = ISC_R_NOMEMORY;
		goto stop_idle_timer;
	}
	query->mctx = res->buckets[fctx->bucketnum].mctx;
	query->options = options;
	query->attributes = 0;
	query->sends = 0;
	query->connects = 0;
	/*
	 * Note that the caller MUST guarantee that 'addrinfo' will remain
	 * valid until this query is canceled.
	 */
	query->addrinfo = addrinfo;
	TIME_NOW(&query->start);

	/*
	 * If this is a TCP query, then we need to make a socket and
	 * a dispatch for it here.  Otherwise we use the resolver's
	 * shared dispatch.
	 */
	query->dispatchmgr = res->dispatchmgr;
	query->dispatch = NULL;
	query->exclusivesocket = ISC_FALSE;
	query->tcpsocket = NULL;
	if (res->view->peers != NULL) {
		dns_peer_t *peer = NULL;
		isc_netaddr_t dstip;
		isc_netaddr_fromsockaddr(&dstip, &addrinfo->sockaddr);
		result = dns_peerlist_peerbyaddr(res->view->peers,
						 &dstip, &peer);
		if (result == ISC_R_SUCCESS) {
			result = dns_peer_getquerysource(peer, &addr);
			if (result == ISC_R_SUCCESS)
				have_addr = ISC_TRUE;
		}
	}

	if ((query->options & DNS_FETCHOPT_TCP) != 0) {
		int pf;

		pf = isc_sockaddr_pf(&addrinfo->sockaddr);
		if (!have_addr) {
			switch (pf) {
			case PF_INET:
				result =
				  dns_dispatch_getlocaladdress(res->dispatchv4,
							       &addr);
				break;
			case PF_INET6:
				result =
				  dns_dispatch_getlocaladdress(res->dispatchv6,
							       &addr);
				break;
			default:
				result = ISC_R_NOTIMPLEMENTED;
				break;
			}
			if (result != ISC_R_SUCCESS)
				goto cleanup_query;
		}
		isc_sockaddr_setport(&addr, 0);

		result = isc_socket_create(res->socketmgr, pf,
					   isc_sockettype_tcp,
					   &query->tcpsocket);
		if (result != ISC_R_SUCCESS)
			goto cleanup_query;

#ifndef BROKEN_TCP_BIND_BEFORE_CONNECT
		result = isc_socket_bind(query->tcpsocket, &addr, 0);
		if (result != ISC_R_SUCCESS)
			goto cleanup_socket;
#endif

		/*
		 * A dispatch will be created once the connect succeeds.
		 */
	} else {
		if (have_addr) {
			unsigned int attrs, attrmask;
			attrs = DNS_DISPATCHATTR_UDP;
			switch (isc_sockaddr_pf(&addr)) {
			case AF_INET:
				attrs |= DNS_DISPATCHATTR_IPV4;
				break;
			case AF_INET6:
				attrs |= DNS_DISPATCHATTR_IPV6;
				break;
			default:
				result = ISC_R_NOTIMPLEMENTED;
				goto cleanup_query;
			}
			attrmask = DNS_DISPATCHATTR_UDP;
			attrmask |= DNS_DISPATCHATTR_TCP;
			attrmask |= DNS_DISPATCHATTR_IPV4;
			attrmask |= DNS_DISPATCHATTR_IPV6;
			result = dns_dispatch_getudp(res->dispatchmgr,
						     res->socketmgr,
						     res->taskmgr, &addr,
						     4096, 1000, 32768, 16411,
						     16433, attrs, attrmask,
						     &query->dispatch);
			if (result != ISC_R_SUCCESS)
				goto cleanup_query;
		} else {
			switch (isc_sockaddr_pf(&addrinfo->sockaddr)) {
			case PF_INET:
				dns_dispatch_attach(res->dispatchv4,
						    &query->dispatch);
				query->exclusivesocket = res->exclusivev4;
				break;
			case PF_INET6:
				dns_dispatch_attach(res->dispatchv6,
						    &query->dispatch);
				query->exclusivesocket = res->exclusivev6;
				break;
			default:
				result = ISC_R_NOTIMPLEMENTED;
				goto cleanup_query;
			}
		}
		/*
		 * We should always have a valid dispatcher here.  If we
		 * don't support a protocol family, then its dispatcher
		 * will be NULL, but we shouldn't be finding addresses for
		 * protocol types we don't support, so the dispatcher
		 * we found should never be NULL.
		 */
		INSIST(query->dispatch != NULL);
	}

	query->dispentry = NULL;
	query->fctx = fctx;
	query->tsig = NULL;
	query->tsigkey = NULL;
	ISC_LINK_INIT(query, link);
	query->magic = QUERY_MAGIC;

	if ((query->options & DNS_FETCHOPT_TCP) != 0) {
		/*
		 * Connect to the remote server.
		 *
		 * XXXRTH  Should we attach to the socket?
		 */
		result = isc_socket_connect(query->tcpsocket,
					    &addrinfo->sockaddr, task,
					    resquery_connected, query);
		if (result != ISC_R_SUCCESS)
			goto cleanup_socket;
		query->connects++;
		QTRACE("connecting via TCP");
	} else {
		result = resquery_send(query);
		if (result != ISC_R_SUCCESS)
			goto cleanup_dispatch;
	}
	fctx->querysent++;

	ISC_LIST_APPEND(fctx->queries, query, link);
	query->fctx->nqueries++;
	if (isc_sockaddr_pf(&addrinfo->sockaddr) == PF_INET)
		inc_stats(res, dns_resstatscounter_queryv4);
	else
		inc_stats(res, dns_resstatscounter_queryv6);
	if (res->view->resquerystats != NULL)
		dns_rdatatypestats_increment(res->view->resquerystats,
					     fctx->type);

	return (ISC_R_SUCCESS);

 cleanup_socket:
	isc_socket_detach(&query->tcpsocket);

 cleanup_dispatch:
	if (query->dispatch != NULL)
		dns_dispatch_detach(&query->dispatch);

 cleanup_query:
	query->magic = 0;
	isc_mem_put(res->buckets[fctx->bucketnum].mctx,
		    query, sizeof(*query));

 stop_idle_timer:
	RUNTIME_CHECK(fctx_stopidletimer(fctx) == ISC_R_SUCCESS);

	return (result);
}

static isc_boolean_t
triededns(fetchctx_t *fctx, isc_sockaddr_t *address) {
	isc_sockaddr_t *sa;

	for (sa = ISC_LIST_HEAD(fctx->edns);
	     sa != NULL;
	     sa = ISC_LIST_NEXT(sa, link)) {
		if (isc_sockaddr_equal(sa, address))
			return (ISC_TRUE);
	}

	return (ISC_FALSE);
}

static void
add_triededns(fetchctx_t *fctx, isc_sockaddr_t *address) {
	isc_sockaddr_t *sa;

	if (triededns(fctx, address))
		return;

	sa = isc_mem_get(fctx->res->buckets[fctx->bucketnum].mctx,
			 sizeof(*sa));
	if (sa == NULL)
		return;

	*sa = *address;
	ISC_LIST_INITANDAPPEND(fctx->edns, sa, link);
}

static isc_boolean_t
triededns512(fetchctx_t *fctx, isc_sockaddr_t *address) {
	isc_sockaddr_t *sa;

	for (sa = ISC_LIST_HEAD(fctx->edns512);
	     sa != NULL;
	     sa = ISC_LIST_NEXT(sa, link)) {
		if (isc_sockaddr_equal(sa, address))
			return (ISC_TRUE);
	}

	return (ISC_FALSE);
}

static void
add_triededns512(fetchctx_t *fctx, isc_sockaddr_t *address) {
	isc_sockaddr_t *sa;

	if (triededns512(fctx, address))
		return;

	sa = isc_mem_get(fctx->res->buckets[fctx->bucketnum].mctx,
			 sizeof(*sa));
	if (sa == NULL)
		return;

	*sa = *address;
	ISC_LIST_INITANDAPPEND(fctx->edns512, sa, link);
}

static isc_result_t
resquery_send(resquery_t *query) {
	fetchctx_t *fctx;
	isc_result_t result;
	dns_name_t *qname = NULL;
	dns_rdataset_t *qrdataset = NULL;
	isc_region_t r;
	dns_resolver_t *res;
	isc_task_t *task;
	isc_socket_t *socket;
	isc_buffer_t tcpbuffer;
	isc_sockaddr_t *address;
	isc_buffer_t *buffer;
	isc_netaddr_t ipaddr;
	dns_tsigkey_t *tsigkey = NULL;
	dns_peer_t *peer = NULL;
	isc_boolean_t useedns;
	dns_compress_t cctx;
	isc_boolean_t cleanup_cctx = ISC_FALSE;
	isc_boolean_t secure_domain;

	fctx = query->fctx;
	QTRACE("send");

	res = fctx->res;
	task = res->buckets[fctx->bucketnum].task;
	address = NULL;

	if ((query->options & DNS_FETCHOPT_TCP) != 0) {
		/*
		 * Reserve space for the TCP message length.
		 */
		isc_buffer_init(&tcpbuffer, query->data, sizeof(query->data));
		isc_buffer_init(&query->buffer, query->data + 2,
				sizeof(query->data) - 2);
		buffer = &tcpbuffer;
	} else {
		isc_buffer_init(&query->buffer, query->data,
				sizeof(query->data));
		buffer = &query->buffer;
	}

	result = dns_message_gettempname(fctx->qmessage, &qname);
	if (result != ISC_R_SUCCESS)
		goto cleanup_temps;
	result = dns_message_gettemprdataset(fctx->qmessage, &qrdataset);
	if (result != ISC_R_SUCCESS)
		goto cleanup_temps;

	/*
	 * Get a query id from the dispatch.
	 */
	result = dns_dispatch_addresponse2(query->dispatch,
					   &query->addrinfo->sockaddr,
					   task,
					   resquery_response,
					   query,
					   &query->id,
					   &query->dispentry,
					   res->socketmgr);
	if (result != ISC_R_SUCCESS)
		goto cleanup_temps;

	fctx->qmessage->opcode = dns_opcode_query;

	/*
	 * Set up question.
	 */
	dns_name_init(qname, NULL);
	dns_name_clone(&fctx->name, qname);
	dns_rdataset_init(qrdataset);
	dns_rdataset_makequestion(qrdataset, res->rdclass, fctx->type);
	ISC_LIST_APPEND(qname->list, qrdataset, link);
	dns_message_addname(fctx->qmessage, qname, DNS_SECTION_QUESTION);
	qname = NULL;
	qrdataset = NULL;

	/*
	 * Set RD if the client has requested that we do a recursive query,
	 * or if we're sending to a forwarder.
	 */
	if ((query->options & DNS_FETCHOPT_RECURSIVE) != 0 ||
	    ISFORWARDER(query->addrinfo))
		fctx->qmessage->flags |= DNS_MESSAGEFLAG_RD;

	/*
	 * Set CD if the client says don't validate or the question is
	 * under a secure entry point.
	 */
	if ((query->options & DNS_FETCHOPT_NOVALIDATE) != 0) {
		fctx->qmessage->flags |= DNS_MESSAGEFLAG_CD;
	} else if (res->view->enablevalidation) {
		result = dns_keytable_issecuredomain(res->view->secroots,
						     &fctx->name,
						     &secure_domain);
		if (result != ISC_R_SUCCESS)
			secure_domain = ISC_FALSE;
		if (res->view->dlv != NULL)
			secure_domain = ISC_TRUE;
		if (secure_domain)
			fctx->qmessage->flags |= DNS_MESSAGEFLAG_CD;
	}

	/*
	 * We don't have to set opcode because it defaults to query.
	 */
	fctx->qmessage->id = query->id;

	/*
	 * Convert the question to wire format.
	 */
	result = dns_compress_init(&cctx, -1, fctx->res->mctx);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;
	cleanup_cctx = ISC_TRUE;

	result = dns_message_renderbegin(fctx->qmessage, &cctx,
					 &query->buffer);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	result = dns_message_rendersection(fctx->qmessage,
					   DNS_SECTION_QUESTION, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	peer = NULL;
	isc_netaddr_fromsockaddr(&ipaddr, &query->addrinfo->sockaddr);
	(void) dns_peerlist_peerbyaddr(fctx->res->view->peers, &ipaddr, &peer);

	/*
	 * The ADB does not know about servers with "edns no".  Check this,
	 * and then inform the ADB for future use.
	 */
	if ((query->addrinfo->flags & DNS_FETCHOPT_NOEDNS0) == 0 &&
	    peer != NULL &&
	    dns_peer_getsupportedns(peer, &useedns) == ISC_R_SUCCESS &&
	    !useedns)
	{
		query->options |= DNS_FETCHOPT_NOEDNS0;
		dns_adb_changeflags(fctx->adb, query->addrinfo,
				    DNS_FETCHOPT_NOEDNS0,
				    DNS_FETCHOPT_NOEDNS0);
	}

	/* Sync NOEDNS0 flag in addrinfo->flags and options now. */
	if ((query->addrinfo->flags & DNS_FETCHOPT_NOEDNS0) != 0)
		query->options |= DNS_FETCHOPT_NOEDNS0;

	/*
	 * Handle timeouts by reducing the UDP response size to 512 bytes
	 * then if that doesn't work disabling EDNS (includes DO) and CD.
	 *
	 * These timeout can be due to:
	 *	* broken nameservers that don't respond to EDNS queries.
	 *	* broken/misconfigured firewalls and NAT implementations
	 *	  that don't handle IP fragmentation.
	 *	* broken/misconfigured firewalls that don't handle responses
	 *	  greater than 512 bytes.
	 *	* broken/misconfigured firewalls that don't handle EDNS, DO
	 *	  or CD.
	 *	* packet loss / link outage.
	 */
	if (fctx->timeout) {
		if ((triededns512(fctx, &query->addrinfo->sockaddr) ||
		     fctx->timeouts >= (MAX_EDNS0_TIMEOUTS * 2)) &&
		    (query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
			query->options |= DNS_FETCHOPT_NOEDNS0;
			fctx->reason = "disabling EDNS";
		} else if ((triededns(fctx, &query->addrinfo->sockaddr) ||
			    fctx->timeouts >= MAX_EDNS0_TIMEOUTS) &&
			   (query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
			query->options |= DNS_FETCHOPT_EDNS512;
			fctx->reason = "reducing the advertised EDNS UDP "
				       "packet size to 512 octets";
		}
		fctx->timeout = ISC_FALSE;
	}

	/*
	 * Use EDNS0, unless the caller doesn't want it, or we know that
	 * the remote server doesn't like it.
	 */
	if ((query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
		if ((query->addrinfo->flags & DNS_FETCHOPT_NOEDNS0) == 0) {
			unsigned int version = 0;       /* Default version. */
			unsigned int flags;
			isc_uint16_t udpsize = res->udpsize;
			isc_boolean_t reqnsid = res->view->requestnsid;

			flags = query->addrinfo->flags;
			if ((flags & DNS_FETCHOPT_EDNSVERSIONSET) != 0) {
				version = flags & DNS_FETCHOPT_EDNSVERSIONMASK;
				version >>= DNS_FETCHOPT_EDNSVERSIONSHIFT;
			}
			if ((query->options & DNS_FETCHOPT_EDNS512) != 0)
				udpsize = 512;
			else if (peer != NULL)
				(void)dns_peer_getudpsize(peer, &udpsize);

			/* request NSID for current view or peer? */
			if (peer != NULL)
				(void) dns_peer_getrequestnsid(peer, &reqnsid);
			result = fctx_addopt(fctx->qmessage, version,
					     udpsize, reqnsid);
			if (reqnsid && result == ISC_R_SUCCESS) {
				query->options |= DNS_FETCHOPT_WANTNSID;
			} else if (result != ISC_R_SUCCESS) {
				/*
				 * We couldn't add the OPT, but we'll press on.
				 * We're not using EDNS0, so set the NOEDNS0
				 * bit.
				 */
				query->options |= DNS_FETCHOPT_NOEDNS0;
			}
		} else {
			/*
			 * We know this server doesn't like EDNS0, so we
			 * won't use it.  Set the NOEDNS0 bit since we're
			 * not using EDNS0.
			 */
			query->options |= DNS_FETCHOPT_NOEDNS0;
		}
	}

	/*
	 * If we need EDNS0 to do this query and aren't using it, we lose.
	 */
	if (NEEDEDNS0(fctx) && (query->options & DNS_FETCHOPT_NOEDNS0) != 0) {
		result = DNS_R_SERVFAIL;
		goto cleanup_message;
	}

	if ((query->options & DNS_FETCHOPT_NOEDNS0) == 0)
		add_triededns(fctx, &query->addrinfo->sockaddr);

	if ((query->options & DNS_FETCHOPT_EDNS512) != 0)
		add_triededns512(fctx, &query->addrinfo->sockaddr);

	/*
	 * Clear CD if EDNS is not in use.
	 */
	if ((query->options & DNS_FETCHOPT_NOEDNS0) != 0)
		fctx->qmessage->flags &= ~DNS_MESSAGEFLAG_CD;

	/*
	 * Add TSIG record tailored to the current recipient.
	 */
	result = dns_view_getpeertsig(fctx->res->view, &ipaddr, &tsigkey);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND)
		goto cleanup_message;

	if (tsigkey != NULL) {
		result = dns_message_settsigkey(fctx->qmessage, tsigkey);
		dns_tsigkey_detach(&tsigkey);
		if (result != ISC_R_SUCCESS)
			goto cleanup_message;
	}

	result = dns_message_rendersection(fctx->qmessage,
					   DNS_SECTION_ADDITIONAL, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	result = dns_message_renderend(fctx->qmessage);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	dns_compress_invalidate(&cctx);
	cleanup_cctx = ISC_FALSE;

	if (dns_message_gettsigkey(fctx->qmessage) != NULL) {
		dns_tsigkey_attach(dns_message_gettsigkey(fctx->qmessage),
				   &query->tsigkey);
		result = dns_message_getquerytsig(fctx->qmessage,
						  fctx->res->mctx,
						  &query->tsig);
		if (result != ISC_R_SUCCESS)
			goto cleanup_message;
	}

	/*
	 * If using TCP, write the length of the message at the beginning
	 * of the buffer.
	 */
	if ((query->options & DNS_FETCHOPT_TCP) != 0) {
		isc_buffer_usedregion(&query->buffer, &r);
		isc_buffer_putuint16(&tcpbuffer, (isc_uint16_t)r.length);
		isc_buffer_add(&tcpbuffer, r.length);
	}

	/*
	 * We're now done with the query message.
	 */
	dns_message_reset(fctx->qmessage, DNS_MESSAGE_INTENTRENDER);

	if (query->exclusivesocket)
		socket = dns_dispatch_getentrysocket(query->dispentry);
	else
		socket = dns_dispatch_getsocket(query->dispatch);
	/*
	 * Send the query!
	 */
	if ((query->options & DNS_FETCHOPT_TCP) == 0) {
		address = &query->addrinfo->sockaddr;
		if (query->exclusivesocket) {
			result = isc_socket_connect(socket, address, task,
						    resquery_udpconnected,
						    query);
			if (result != ISC_R_SUCCESS)
				goto cleanup_message;
			query->connects++;
		}
	}
	isc_buffer_usedregion(buffer, &r);

	/*
	 * XXXRTH  Make sure we don't send to ourselves!  We should probably
	 *		prune out these addresses when we get them from the ADB.
	 */
	result = isc_socket_sendto(socket, &r, task, resquery_senddone,
				   query, address, NULL);
	if (result != ISC_R_SUCCESS)
		goto cleanup_message;

	query->sends++;

	QTRACE("sent");

	return (ISC_R_SUCCESS);

 cleanup_message:
	if (cleanup_cctx)
		dns_compress_invalidate(&cctx);

	dns_message_reset(fctx->qmessage, DNS_MESSAGE_INTENTRENDER);

	/*
	 * Stop the dispatcher from listening.
	 */
	dns_dispatch_removeresponse(&query->dispentry, NULL);

 cleanup_temps:
	if (qname != NULL)
		dns_message_puttempname(fctx->qmessage, &qname);
	if (qrdataset != NULL)
		dns_message_puttemprdataset(fctx->qmessage, &qrdataset);

	return (result);
}

static void
resquery_connected(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;
	resquery_t *query = event->ev_arg;
	isc_boolean_t retry = ISC_FALSE;
	isc_interval_t interval;
	isc_result_t result;
	unsigned int attrs;
	fetchctx_t *fctx;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_CONNECT);
	REQUIRE(VALID_QUERY(query));

	QTRACE("connected");

	UNUSED(task);

	/*
	 * XXXRTH
	 *
	 * Currently we don't wait for the connect event before retrying
	 * a query.  This means that if we get really behind, we may end
	 * up doing extra work!
	 */

	query->connects--;
	fctx = query->fctx;

	if (RESQUERY_CANCELED(query)) {
		/*
		 * This query was canceled while the connect() was in
		 * progress.
		 */
		isc_socket_detach(&query->tcpsocket);
		resquery_destroy(&query);
	} else {
		switch (sevent->result) {
		case ISC_R_SUCCESS:

			/*
			 * Extend the idle timer for TCP.  20 seconds
			 * should be long enough for a TCP connection to be
			 * established, a single DNS request to be sent,
			 * and the response received.
			 */
			isc_interval_set(&interval, 20, 0);
			result = fctx_startidletimer(query->fctx, &interval);
			if (result != ISC_R_SUCCESS) {
				fctx_cancelquery(&query, NULL, NULL, ISC_FALSE);
				fctx_done(fctx, result, __LINE__);
				break;
			}
			/*
			 * We are connected.  Create a dispatcher and
			 * send the query.
			 */
			attrs = 0;
			attrs |= DNS_DISPATCHATTR_TCP;
			attrs |= DNS_DISPATCHATTR_PRIVATE;
			attrs |= DNS_DISPATCHATTR_CONNECTED;
			if (isc_sockaddr_pf(&query->addrinfo->sockaddr) ==
			    AF_INET)
				attrs |= DNS_DISPATCHATTR_IPV4;
			else
				attrs |= DNS_DISPATCHATTR_IPV6;
			attrs |= DNS_DISPATCHATTR_MAKEQUERY;

			result = dns_dispatch_createtcp(query->dispatchmgr,
						     query->tcpsocket,
						     query->fctx->res->taskmgr,
						     4096, 2, 1, 1, 3, attrs,
						     &query->dispatch);

			/*
			 * Regardless of whether dns_dispatch_create()
			 * succeeded or not, we don't need our reference
			 * to the socket anymore.
			 */
			isc_socket_detach(&query->tcpsocket);

			if (result == ISC_R_SUCCESS)
				result = resquery_send(query);

			if (result != ISC_R_SUCCESS) {
				fctx_cancelquery(&query, NULL, NULL, ISC_FALSE);
				fctx_done(fctx, result, __LINE__);
			}
			break;

		case ISC_R_NETUNREACH:
		case ISC_R_HOSTUNREACH:
		case ISC_R_CONNREFUSED:
		case ISC_R_NOPERM:
		case ISC_R_ADDRNOTAVAIL:
		case ISC_R_CONNECTIONRESET:
			/*
			 * No route to remote.
			 */
			isc_socket_detach(&query->tcpsocket);
			fctx_cancelquery(&query, NULL, NULL, ISC_TRUE);
			retry = ISC_TRUE;
			break;

		default:
			isc_socket_detach(&query->tcpsocket);
			fctx_cancelquery(&query, NULL, NULL, ISC_FALSE);
			break;
		}
	}

	isc_event_free(&event);

	if (retry) {
		/*
		 * Behave as if the idle timer has expired.  For TCP
		 * connections this may not actually reflect the latest timer.
		 */
		fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
		result = fctx_stopidletimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else
			fctx_try(fctx, ISC_TRUE);
	}
}

static void
fctx_finddone(isc_task_t *task, isc_event_t *event) {
	fetchctx_t *fctx;
	dns_adbfind_t *find;
	dns_resolver_t *res;
	isc_boolean_t want_try = ISC_FALSE;
	isc_boolean_t want_done = ISC_FALSE;
	isc_boolean_t bucket_empty = ISC_FALSE;
	unsigned int bucketnum;

	find = event->ev_sender;
	fctx = event->ev_arg;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	UNUSED(task);

	FCTXTRACE("finddone");

	INSIST(fctx->pending > 0);
	fctx->pending--;

	if (ADDRWAIT(fctx)) {
		/*
		 * The fetch is waiting for a name to be found.
		 */
		INSIST(!SHUTTINGDOWN(fctx));
		fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
		if (event->ev_type == DNS_EVENT_ADBMOREADDRESSES)
			want_try = ISC_TRUE;
		else {
			fctx->findfail++;
			if (fctx->pending == 0) {
				/*
				 * We've got nothing else to wait for and don't
				 * know the answer.  There's nothing to do but
				 * fail the fctx.
				 */
				want_done = ISC_TRUE;
			}
		}
	} else if (SHUTTINGDOWN(fctx) && fctx->pending == 0 &&
		   fctx->nqueries == 0 && ISC_LIST_EMPTY(fctx->validators)) {
		bucketnum = fctx->bucketnum;
		LOCK(&res->buckets[bucketnum].lock);
		/*
		 * Note that we had to wait until we had the lock before
		 * looking at fctx->references.
		 */
		if (fctx->references == 0)
			bucket_empty = fctx_destroy(fctx);
		UNLOCK(&res->buckets[bucketnum].lock);
	}

	isc_event_free(&event);
	dns_adb_destroyfind(&find);

	if (want_try)
		fctx_try(fctx, ISC_TRUE);
	else if (want_done)
		fctx_done(fctx, ISC_R_FAILURE, __LINE__);
	else if (bucket_empty)
		empty_bucket(res);
}


static inline isc_boolean_t
bad_server(fetchctx_t *fctx, isc_sockaddr_t *address) {
	isc_sockaddr_t *sa;

	for (sa = ISC_LIST_HEAD(fctx->bad);
	     sa != NULL;
	     sa = ISC_LIST_NEXT(sa, link)) {
		if (isc_sockaddr_equal(sa, address))
			return (ISC_TRUE);
	}

	return (ISC_FALSE);
}

static inline isc_boolean_t
mark_bad(fetchctx_t *fctx) {
	dns_adbfind_t *curr;
	dns_adbaddrinfo_t *addrinfo;
	isc_boolean_t all_bad = ISC_TRUE;

	/*
	 * Mark all known bad servers, so we don't try to talk to them
	 * again.
	 */

	/*
	 * Mark any bad nameservers.
	 */
	for (curr = ISC_LIST_HEAD(fctx->finds);
	     curr != NULL;
	     curr = ISC_LIST_NEXT(curr, publink)) {
		for (addrinfo = ISC_LIST_HEAD(curr->list);
		     addrinfo != NULL;
		     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
			if (bad_server(fctx, &addrinfo->sockaddr))
				addrinfo->flags |= FCTX_ADDRINFO_MARK;
			else
				all_bad = ISC_FALSE;
		}
	}

	/*
	 * Mark any bad forwarders.
	 */
	for (addrinfo = ISC_LIST_HEAD(fctx->forwaddrs);
	     addrinfo != NULL;
	     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
		if (bad_server(fctx, &addrinfo->sockaddr))
			addrinfo->flags |= FCTX_ADDRINFO_MARK;
		else
			all_bad = ISC_FALSE;
	}

	/*
	 * Mark any bad alternates.
	 */
	for (curr = ISC_LIST_HEAD(fctx->altfinds);
	     curr != NULL;
	     curr = ISC_LIST_NEXT(curr, publink)) {
		for (addrinfo = ISC_LIST_HEAD(curr->list);
		     addrinfo != NULL;
		     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
			if (bad_server(fctx, &addrinfo->sockaddr))
				addrinfo->flags |= FCTX_ADDRINFO_MARK;
			else
				all_bad = ISC_FALSE;
		}
	}

	for (addrinfo = ISC_LIST_HEAD(fctx->altaddrs);
	     addrinfo != NULL;
	     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
		if (bad_server(fctx, &addrinfo->sockaddr))
			addrinfo->flags |= FCTX_ADDRINFO_MARK;
		else
			all_bad = ISC_FALSE;
	}

	return (all_bad);
}

static void
add_bad(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo, isc_result_t reason,
	badnstype_t badtype)
{
	char namebuf[DNS_NAME_FORMATSIZE];
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];
	char classbuf[64];
	char typebuf[64];
	char code[64];
	isc_buffer_t b;
	isc_sockaddr_t *sa;
	const char *sep1, *sep2;
	isc_sockaddr_t *address = &addrinfo->sockaddr;

	if (reason == DNS_R_LAME)
		fctx->lamecount++;
	else {
		switch (badtype) {
		case badns_unreachable:
			fctx->neterr++;
			break;
		case badns_response:
			fctx->badresp++;
			break;
		case badns_validation:
			break;	/* counted as 'valfail' */
		}
	}

	if (bad_server(fctx, address)) {
		/*
		 * We already know this server is bad.
		 */
		return;
	}

	FCTXTRACE("add_bad");

	sa = isc_mem_get(fctx->res->buckets[fctx->bucketnum].mctx,
			 sizeof(*sa));
	if (sa == NULL)
		return;
	*sa = *address;
	ISC_LIST_INITANDAPPEND(fctx->bad, sa, link);

	if (reason == DNS_R_LAME)       /* already logged */
		return;

	if (reason == DNS_R_UNEXPECTEDRCODE &&
	    fctx->rmessage->rcode == dns_rcode_servfail &&
	    ISFORWARDER(addrinfo))
		return;

	if (reason == DNS_R_UNEXPECTEDRCODE) {
		isc_buffer_init(&b, code, sizeof(code) - 1);
		dns_rcode_totext(fctx->rmessage->rcode, &b);
		code[isc_buffer_usedlength(&b)] = '\0';
		sep1 = "(";
		sep2 = ") ";
	} else if (reason == DNS_R_UNEXPECTEDOPCODE) {
		isc_buffer_init(&b, code, sizeof(code) - 1);
		dns_opcode_totext((dns_opcode_t)fctx->rmessage->opcode, &b);
		code[isc_buffer_usedlength(&b)] = '\0';
		sep1 = "(";
		sep2 = ") ";
	} else {
		code[0] = '\0';
		sep1 = "";
		sep2 = "";
	}
	dns_name_format(&fctx->name, namebuf, sizeof(namebuf));
	dns_rdatatype_format(fctx->type, typebuf, sizeof(typebuf));
	dns_rdataclass_format(fctx->res->rdclass, classbuf, sizeof(classbuf));
	isc_sockaddr_format(address, addrbuf, sizeof(addrbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_LAME_SERVERS,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
		      "%s %s%s%sresolving '%s/%s/%s': %s",
		      dns_result_totext(reason), sep1, code, sep2,
		      namebuf, typebuf, classbuf, addrbuf);
}

/*
 * Return 'bits' bits of random entropy from fctx->rand_buf,
 * refreshing it by calling isc_random_get() whenever the requested
 * number of bits is greater than the number in the buffer.
 */
static inline isc_uint32_t
random_bits(fetchctx_t *fctx, isc_uint32_t bits) {
	isc_uint32_t ret = 0;

	REQUIRE(VALID_FCTX(fctx));
	REQUIRE(bits <= 32);
	if (bits == 0)
		return (0);

	if (bits >= fctx->rand_bits) {
		/* if rand_bits == 0, this is unnecessary but harmless */
		bits -= fctx->rand_bits;
		ret = fctx->rand_buf << bits;

		/* refresh random buffer now */
		isc_random_get(&fctx->rand_buf);
		fctx->rand_bits = sizeof(fctx->rand_buf) * CHAR_BIT;
	}

	if (bits > 0) {
		isc_uint32_t mask = 0xffffffff;
		if (bits < 32) {
			mask = (1 << bits) - 1;
		}

		ret |= fctx->rand_buf & mask;
		fctx->rand_buf >>= bits;
		fctx->rand_bits -= bits;
	}

	return (ret);
}

/*
 * Add some random jitter to a server's RTT value so that the
 * order of queries will be unpredictable.
 *
 * RTT values of servers which have been tried are fuzzed by 128 ms.
 * Servers that haven't been tried yet have their RTT set to a random
 * value between 0 ms and 7 ms; they should get to go first, but in
 * unpredictable order.
 */
static inline void
randomize_srtt(fetchctx_t *fctx, dns_adbaddrinfo_t *ai) {
	if (TRIED(ai)) {
		ai->srtt >>= 10; /* convert to milliseconds, near enough */
		ai->srtt |= (ai->srtt & 0x80) | random_bits(fctx, 7);
		ai->srtt <<= 10; /* now back to microseconds */
	} else
		ai->srtt = random_bits(fctx, 3) << 10;
}

/*
 * Sort addrinfo list by RTT (with random jitter)
 */
static void
sort_adbfind(fetchctx_t *fctx, dns_adbfind_t *find) {
	dns_adbaddrinfo_t *best, *curr;
	dns_adbaddrinfolist_t sorted;

	/* Add jitter to SRTT values */
	curr = ISC_LIST_HEAD(find->list);
	while (curr != NULL) {
		randomize_srtt(fctx, curr);
		curr = ISC_LIST_NEXT(curr, publink);
	}

	/* Lame N^2 bubble sort. */
	ISC_LIST_INIT(sorted);
	while (!ISC_LIST_EMPTY(find->list)) {
		best = ISC_LIST_HEAD(find->list);
		curr = ISC_LIST_NEXT(best, publink);
		while (curr != NULL) {
			if (curr->srtt < best->srtt)
				best = curr;
			curr = ISC_LIST_NEXT(curr, publink);
		}
		ISC_LIST_UNLINK(find->list, best, publink);
		ISC_LIST_APPEND(sorted, best, publink);
	}
	find->list = sorted;
}

/*
 * Sort a list of finds by server RTT (with random jitter)
 */
static void
sort_finds(fetchctx_t *fctx, dns_adbfindlist_t *findlist) {
	dns_adbfind_t *best, *curr;
	dns_adbfindlist_t sorted;
	dns_adbaddrinfo_t *addrinfo, *bestaddrinfo;

	/* Sort each find's addrinfo list by SRTT (after adding jitter) */
	for (curr = ISC_LIST_HEAD(*findlist);
	     curr != NULL;
	     curr = ISC_LIST_NEXT(curr, publink))
		sort_adbfind(fctx, curr);

	/* Lame N^2 bubble sort. */
	ISC_LIST_INIT(sorted);
	while (!ISC_LIST_EMPTY(*findlist)) {
		best = ISC_LIST_HEAD(*findlist);
		bestaddrinfo = ISC_LIST_HEAD(best->list);
		INSIST(bestaddrinfo != NULL);
		curr = ISC_LIST_NEXT(best, publink);
		while (curr != NULL) {
			addrinfo = ISC_LIST_HEAD(curr->list);
			INSIST(addrinfo != NULL);
			if (addrinfo->srtt < bestaddrinfo->srtt) {
				best = curr;
				bestaddrinfo = addrinfo;
			}
			curr = ISC_LIST_NEXT(curr, publink);
		}
		ISC_LIST_UNLINK(*findlist, best, publink);
		ISC_LIST_APPEND(sorted, best, publink);
	}
	*findlist = sorted;
}

static void
findname(fetchctx_t *fctx, dns_name_t *name, in_port_t port,
	 unsigned int options, unsigned int flags, isc_stdtime_t now,
	 isc_boolean_t *need_alternate)
{
	dns_adbaddrinfo_t *ai;
	dns_adbfind_t *find;
	dns_resolver_t *res;
	isc_boolean_t unshared;
	isc_result_t result;

	res = fctx->res;
	unshared = ISC_TF((fctx->options | DNS_FETCHOPT_UNSHARED) != 0);
	/*
	 * If this name is a subdomain of the query domain, tell
	 * the ADB to start looking using zone/hint data. This keeps us
	 * from getting stuck if the nameserver is beneath the zone cut
	 * and we don't know its address (e.g. because the A record has
	 * expired).
	 */
	if (dns_name_issubdomain(name, &fctx->domain))
		options |= DNS_ADBFIND_STARTATZONE;
	options |= DNS_ADBFIND_GLUEOK;
	options |= DNS_ADBFIND_HINTOK;

	/*
	 * See what we know about this address.
	 */
	find = NULL;
	result = dns_adb_createfind(fctx->adb,
				    res->buckets[fctx->bucketnum].task,
				    fctx_finddone, fctx, name,
				    &fctx->name, fctx->type,
				    options, now, NULL,
				    res->view->dstport, &find);
	if (result != ISC_R_SUCCESS) {
		if (result == DNS_R_ALIAS) {
			/*
			 * XXXRTH  Follow the CNAME/DNAME chain?
			 */
			dns_adb_destroyfind(&find);
			fctx->adberr++;
		}
	} else if (!ISC_LIST_EMPTY(find->list)) {
		/*
		 * We have at least some of the addresses for the
		 * name.
		 */
		INSIST((find->options & DNS_ADBFIND_WANTEVENT) == 0);
		if (flags != 0 || port != 0) {
			for (ai = ISC_LIST_HEAD(find->list);
			     ai != NULL;
			     ai = ISC_LIST_NEXT(ai, publink)) {
				ai->flags |= flags;
				if (port != 0)
					isc_sockaddr_setport(&ai->sockaddr,
							     port);
			}
		}
		if ((flags & FCTX_ADDRINFO_FORWARDER) != 0)
			ISC_LIST_APPEND(fctx->altfinds, find, publink);
		else
			ISC_LIST_APPEND(fctx->finds, find, publink);
	} else {
		/*
		 * We don't know any of the addresses for this
		 * name.
		 */
		if ((find->options & DNS_ADBFIND_WANTEVENT) != 0) {
			/*
			 * We're looking for them and will get an
			 * event about it later.
			 */
			fctx->pending++;
			/*
			 * Bootstrap.
			 */
			if (need_alternate != NULL &&
			    !*need_alternate && unshared &&
			    ((res->dispatchv4 == NULL &&
			      find->result_v6 != DNS_R_NXDOMAIN) ||
			     (res->dispatchv6 == NULL &&
			      find->result_v4 != DNS_R_NXDOMAIN)))
				*need_alternate = ISC_TRUE;
		} else {
			if ((find->options & DNS_ADBFIND_LAMEPRUNED) != 0)
				fctx->lamecount++; /* cached lame server */
			else
				fctx->adberr++; /* unreachable server, etc. */

			/*
			 * If we know there are no addresses for
			 * the family we are using then try to add
			 * an alternative server.
			 */
			if (need_alternate != NULL && !*need_alternate &&
			    ((res->dispatchv4 == NULL &&
			      find->result_v6 == DNS_R_NXRRSET) ||
			     (res->dispatchv6 == NULL &&
			      find->result_v4 == DNS_R_NXRRSET)))
				*need_alternate = ISC_TRUE;
			dns_adb_destroyfind(&find);
		}
	}
}

static isc_result_t
fctx_getaddresses(fetchctx_t *fctx) {
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;
	dns_resolver_t *res;
	isc_stdtime_t now;
	unsigned int stdoptions;
	isc_sockaddr_t *sa;
	dns_adbaddrinfo_t *ai;
	isc_boolean_t all_bad;
	dns_rdata_ns_t ns;
	isc_boolean_t need_alternate = ISC_FALSE;

	FCTXTRACE("getaddresses");

	/*
	 * Don't pound on remote servers.  (Failsafe!)
	 */
	fctx->restarts++;
	if (fctx->restarts > 10) {
		FCTXTRACE("too many restarts");
		return (DNS_R_SERVFAIL);
	}

	res = fctx->res;
	stdoptions = 0;         /* Keep compiler happy. */

	/*
	 * Forwarders.
	 */

	INSIST(ISC_LIST_EMPTY(fctx->forwaddrs));
	INSIST(ISC_LIST_EMPTY(fctx->altaddrs));

	/*
	 * If this fctx has forwarders, use them; otherwise use any
	 * selective forwarders specified in the view; otherwise use the
	 * resolver's forwarders (if any).
	 */
	sa = ISC_LIST_HEAD(fctx->forwarders);
	if (sa == NULL) {
		dns_forwarders_t *forwarders = NULL;
		dns_name_t *name = &fctx->name;
		dns_name_t suffix;
		unsigned int labels;

		/*
		 * DS records are found in the parent server.
		 * Strip label to get the correct forwarder (if any).
		 */
		if (fctx->type == dns_rdatatype_ds &&
		    dns_name_countlabels(name) > 1) {
			dns_name_init(&suffix, NULL);
			labels = dns_name_countlabels(name);
			dns_name_getlabelsequence(name, 1, labels - 1, &suffix);
			name = &suffix;
		}
		result = dns_fwdtable_find(fctx->res->view->fwdtable, name,
					   &forwarders);
		if (result == ISC_R_SUCCESS) {
			sa = ISC_LIST_HEAD(forwarders->addrs);
			fctx->fwdpolicy = forwarders->fwdpolicy;
		}
	}

	while (sa != NULL) {
		if ((isc_sockaddr_pf(sa) == AF_INET &&
			 fctx->res->dispatchv4 == NULL) ||
		    (isc_sockaddr_pf(sa) == AF_INET6 &&
			fctx->res->dispatchv6 == NULL)) {
				sa = ISC_LIST_NEXT(sa, link);
				continue;
		}
		ai = NULL;
		result = dns_adb_findaddrinfo(fctx->adb,
					      sa, &ai, 0);  /* XXXMLG */
		if (result == ISC_R_SUCCESS) {
			dns_adbaddrinfo_t *cur;
			ai->flags |= FCTX_ADDRINFO_FORWARDER;
			cur = ISC_LIST_HEAD(fctx->forwaddrs);
			while (cur != NULL && cur->srtt < ai->srtt)
				cur = ISC_LIST_NEXT(cur, publink);
			if (cur != NULL)
				ISC_LIST_INSERTBEFORE(fctx->forwaddrs, cur,
						      ai, publink);
			else
				ISC_LIST_APPEND(fctx->forwaddrs, ai, publink);
		}
		sa = ISC_LIST_NEXT(sa, link);
	}

	/*
	 * If the forwarding policy is "only", we don't need the addresses
	 * of the nameservers.
	 */
	if (fctx->fwdpolicy == dns_fwdpolicy_only)
		goto out;

	/*
	 * Normal nameservers.
	 */

	stdoptions = DNS_ADBFIND_WANTEVENT | DNS_ADBFIND_EMPTYEVENT;
	if (fctx->restarts == 1) {
		/*
		 * To avoid sending out a flood of queries likely to
		 * result in NXRRSET, we suppress fetches for address
		 * families we don't have the first time through,
		 * provided that we have addresses in some family we
		 * can use.
		 *
		 * We don't want to set this option all the time, since
		 * if fctx->restarts > 1, we've clearly been having trouble
		 * with the addresses we had, so getting more could help.
		 */
		stdoptions |= DNS_ADBFIND_AVOIDFETCHES;
	}
	if (res->dispatchv4 != NULL)
		stdoptions |= DNS_ADBFIND_INET;
	if (res->dispatchv6 != NULL)
		stdoptions |= DNS_ADBFIND_INET6;
	isc_stdtime_get(&now);

	INSIST(ISC_LIST_EMPTY(fctx->finds));
	INSIST(ISC_LIST_EMPTY(fctx->altfinds));

	for (result = dns_rdataset_first(&fctx->nameservers);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(&fctx->nameservers))
	{
		dns_rdataset_current(&fctx->nameservers, &rdata);
		/*
		 * Extract the name from the NS record.
		 */
		result = dns_rdata_tostruct(&rdata, &ns, NULL);
		if (result != ISC_R_SUCCESS)
			continue;

		findname(fctx, &ns.name, 0, stdoptions, 0, now,
			 &need_alternate);
		dns_rdata_reset(&rdata);
		dns_rdata_freestruct(&ns);
	}
	if (result != ISC_R_NOMORE)
		return (result);

	/*
	 * Do we need to use 6 to 4?
	 */
	if (need_alternate) {
		int family;
		alternate_t *a;
		family = (res->dispatchv6 != NULL) ? AF_INET6 : AF_INET;
		for (a = ISC_LIST_HEAD(fctx->res->alternates);
		     a != NULL;
		     a = ISC_LIST_NEXT(a, link)) {
			if (!a->isaddress) {
				findname(fctx, &a->_u._n.name, a->_u._n.port,
					 stdoptions, FCTX_ADDRINFO_FORWARDER,
					 now, NULL);
				continue;
			}
			if (isc_sockaddr_pf(&a->_u.addr) != family)
				continue;
			ai = NULL;
			result = dns_adb_findaddrinfo(fctx->adb, &a->_u.addr,
						      &ai, 0);
			if (result == ISC_R_SUCCESS) {
				dns_adbaddrinfo_t *cur;
				ai->flags |= FCTX_ADDRINFO_FORWARDER;
				cur = ISC_LIST_HEAD(fctx->altaddrs);
				while (cur != NULL && cur->srtt < ai->srtt)
					cur = ISC_LIST_NEXT(cur, publink);
				if (cur != NULL)
					ISC_LIST_INSERTBEFORE(fctx->altaddrs,
							      cur, ai, publink);
				else
					ISC_LIST_APPEND(fctx->altaddrs, ai,
							publink);
			}
		}
	}

 out:
	/*
	 * Mark all known bad servers.
	 */
	all_bad = mark_bad(fctx);

	/*
	 * How are we doing?
	 */
	if (all_bad) {
		/*
		 * We've got no addresses.
		 */
		if (fctx->pending > 0) {
			/*
			 * We're fetching the addresses, but don't have any
			 * yet.   Tell the caller to wait for an answer.
			 */
			result = DNS_R_WAIT;
		} else {
			/*
			 * We've lost completely.  We don't know any
			 * addresses, and the ADB has told us it can't get
			 * them.
			 */
			FCTXTRACE("no addresses");
			result = ISC_R_FAILURE;
		}
	} else {
		/*
		 * We've found some addresses.  We might still be looking
		 * for more addresses.
		 */
		sort_finds(fctx, &fctx->finds);
		sort_finds(fctx, &fctx->altfinds);
		result = ISC_R_SUCCESS;
	}

	return (result);
}

static inline void
possibly_mark(fetchctx_t *fctx, dns_adbaddrinfo_t *addr)
{
	isc_netaddr_t na;
	char buf[ISC_NETADDR_FORMATSIZE];
	isc_sockaddr_t *sa;
	isc_boolean_t aborted = ISC_FALSE;
	isc_boolean_t bogus;
	dns_acl_t *blackhole;
	isc_netaddr_t ipaddr;
	dns_peer_t *peer = NULL;
	dns_resolver_t *res;
	const char *msg = NULL;

	sa = &addr->sockaddr;

	res = fctx->res;
	isc_netaddr_fromsockaddr(&ipaddr, sa);
	blackhole = dns_dispatchmgr_getblackhole(res->dispatchmgr);
	(void) dns_peerlist_peerbyaddr(res->view->peers, &ipaddr, &peer);

	if (blackhole != NULL) {
		int match;

		if (dns_acl_match(&ipaddr, NULL, blackhole,
				  &res->view->aclenv,
				  &match, NULL) == ISC_R_SUCCESS &&
		    match > 0)
			aborted = ISC_TRUE;
	}

	if (peer != NULL &&
	    dns_peer_getbogus(peer, &bogus) == ISC_R_SUCCESS &&
	    bogus)
		aborted = ISC_TRUE;

	if (aborted) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring blackholed / bogus server: ";
	} else if (isc_sockaddr_ismulticast(sa)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring multicast address: ";
	} else if (isc_sockaddr_isexperimental(sa)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring experimental address: ";
	} else if (sa->type.sa.sa_family != AF_INET6) {
		return;
	} else if (IN6_IS_ADDR_V4MAPPED(&sa->type.sin6.sin6_addr)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring IPv6 mapped IPV4 address: ";
	} else if (IN6_IS_ADDR_V4COMPAT(&sa->type.sin6.sin6_addr)) {
		addr->flags |= FCTX_ADDRINFO_MARK;
		msg = "ignoring IPv6 compatibility IPV4 address: ";
	} else
		return;

	if (!isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(3)))
		return;

	isc_netaddr_fromsockaddr(&na, sa);
	isc_netaddr_format(&na, buf, sizeof(buf));
	FCTXTRACE2(msg, buf);
}

static inline dns_adbaddrinfo_t *
fctx_nextaddress(fetchctx_t *fctx) {
	dns_adbfind_t *find, *start;
	dns_adbaddrinfo_t *addrinfo;
	dns_adbaddrinfo_t *faddrinfo;

	/*
	 * Return the next untried address, if any.
	 */

	/*
	 * Find the first unmarked forwarder (if any).
	 */
	for (addrinfo = ISC_LIST_HEAD(fctx->forwaddrs);
	     addrinfo != NULL;
	     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
		if (!UNMARKED(addrinfo))
			continue;
		possibly_mark(fctx, addrinfo);
		if (UNMARKED(addrinfo)) {
			addrinfo->flags |= FCTX_ADDRINFO_MARK;
			fctx->find = NULL;
			return (addrinfo);
		}
	}

	/*
	 * No forwarders.  Move to the next find.
	 */

	fctx->attributes |= FCTX_ATTR_TRIEDFIND;

	find = fctx->find;
	if (find == NULL)
		find = ISC_LIST_HEAD(fctx->finds);
	else {
		find = ISC_LIST_NEXT(find, publink);
		if (find == NULL)
			find = ISC_LIST_HEAD(fctx->finds);
	}

	/*
	 * Find the first unmarked addrinfo.
	 */
	addrinfo = NULL;
	if (find != NULL) {
		start = find;
		do {
			for (addrinfo = ISC_LIST_HEAD(find->list);
			     addrinfo != NULL;
			     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
				if (!UNMARKED(addrinfo))
					continue;
				possibly_mark(fctx, addrinfo);
				if (UNMARKED(addrinfo)) {
					addrinfo->flags |= FCTX_ADDRINFO_MARK;
					break;
				}
			}
			if (addrinfo != NULL)
				break;
			find = ISC_LIST_NEXT(find, publink);
			if (find == NULL)
				find = ISC_LIST_HEAD(fctx->finds);
		} while (find != start);
	}

	fctx->find = find;
	if (addrinfo != NULL)
		return (addrinfo);

	/*
	 * No nameservers left.  Try alternates.
	 */

	fctx->attributes |= FCTX_ATTR_TRIEDALT;

	find = fctx->altfind;
	if (find == NULL)
		find = ISC_LIST_HEAD(fctx->altfinds);
	else {
		find = ISC_LIST_NEXT(find, publink);
		if (find == NULL)
			find = ISC_LIST_HEAD(fctx->altfinds);
	}

	/*
	 * Find the first unmarked addrinfo.
	 */
	addrinfo = NULL;
	if (find != NULL) {
		start = find;
		do {
			for (addrinfo = ISC_LIST_HEAD(find->list);
			     addrinfo != NULL;
			     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
				if (!UNMARKED(addrinfo))
					continue;
				possibly_mark(fctx, addrinfo);
				if (UNMARKED(addrinfo)) {
					addrinfo->flags |= FCTX_ADDRINFO_MARK;
					break;
				}
			}
			if (addrinfo != NULL)
				break;
			find = ISC_LIST_NEXT(find, publink);
			if (find == NULL)
				find = ISC_LIST_HEAD(fctx->altfinds);
		} while (find != start);
	}

	faddrinfo = addrinfo;

	/*
	 * See if we have a better alternate server by address.
	 */

	for (addrinfo = ISC_LIST_HEAD(fctx->altaddrs);
	     addrinfo != NULL;
	     addrinfo = ISC_LIST_NEXT(addrinfo, publink)) {
		if (!UNMARKED(addrinfo))
			continue;
		possibly_mark(fctx, addrinfo);
		if (UNMARKED(addrinfo) &&
		    (faddrinfo == NULL ||
		     addrinfo->srtt < faddrinfo->srtt)) {
			if (faddrinfo != NULL)
				faddrinfo->flags &= ~FCTX_ADDRINFO_MARK;
			addrinfo->flags |= FCTX_ADDRINFO_MARK;
			break;
		}
	}

	if (addrinfo == NULL) {
		addrinfo = faddrinfo;
		fctx->altfind = find;
	}

	return (addrinfo);
}

static void
fctx_try(fetchctx_t *fctx, isc_boolean_t retrying) {
	isc_result_t result;
	dns_adbaddrinfo_t *addrinfo;

	FCTXTRACE("try");

	REQUIRE(!ADDRWAIT(fctx));

	addrinfo = fctx_nextaddress(fctx);
	if (addrinfo == NULL) {
		/*
		 * We have no more addresses.  Start over.
		 */
		fctx_cancelqueries(fctx, ISC_TRUE);
		fctx_cleanupfinds(fctx);
		fctx_cleanupaltfinds(fctx);
		fctx_cleanupforwaddrs(fctx);
		fctx_cleanupaltaddrs(fctx);
		result = fctx_getaddresses(fctx);
		if (result == DNS_R_WAIT) {
			/*
			 * Sleep waiting for addresses.
			 */
			FCTXTRACE("addrwait");
			fctx->attributes |= FCTX_ATTR_ADDRWAIT;
			return;
		} else if (result != ISC_R_SUCCESS) {
			/*
			 * Something bad happened.
			 */
			fctx_done(fctx, result, __LINE__);
			return;
		}

		addrinfo = fctx_nextaddress(fctx);
		/*
		 * While we may have addresses from the ADB, they
		 * might be bad ones.  In this case, return SERVFAIL.
		 */
		if (addrinfo == NULL) {
			fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
			return;
		}
	}

	result = fctx_query(fctx, addrinfo, fctx->options);
	if (result != ISC_R_SUCCESS)
		fctx_done(fctx, result, __LINE__);
	else if (retrying)
		inc_stats(fctx->res, dns_resstatscounter_retry);
}

static isc_boolean_t
fctx_destroy(fetchctx_t *fctx) {
	dns_resolver_t *res;
	unsigned int bucketnum;
	isc_sockaddr_t *sa, *next_sa;

	/*
	 * Caller must be holding the bucket lock.
	 */

	REQUIRE(VALID_FCTX(fctx));
	REQUIRE(fctx->state == fetchstate_done ||
		fctx->state == fetchstate_init);
	REQUIRE(ISC_LIST_EMPTY(fctx->events));
	REQUIRE(ISC_LIST_EMPTY(fctx->queries));
	REQUIRE(ISC_LIST_EMPTY(fctx->finds));
	REQUIRE(ISC_LIST_EMPTY(fctx->altfinds));
	REQUIRE(fctx->pending == 0);
	REQUIRE(fctx->references == 0);
	REQUIRE(ISC_LIST_EMPTY(fctx->validators));

	FCTXTRACE("destroy");

	res = fctx->res;
	bucketnum = fctx->bucketnum;

	ISC_LIST_UNLINK(res->buckets[bucketnum].fctxs, fctx, link);

	/*
	 * Free bad.
	 */
	for (sa = ISC_LIST_HEAD(fctx->bad);
	     sa != NULL;
	     sa = next_sa) {
		next_sa = ISC_LIST_NEXT(sa, link);
		ISC_LIST_UNLINK(fctx->bad, sa, link);
		isc_mem_put(res->buckets[bucketnum].mctx, sa, sizeof(*sa));
	}

	for (sa = ISC_LIST_HEAD(fctx->edns);
	     sa != NULL;
	     sa = next_sa) {
		next_sa = ISC_LIST_NEXT(sa, link);
		ISC_LIST_UNLINK(fctx->edns, sa, link);
		isc_mem_put(res->buckets[bucketnum].mctx, sa, sizeof(*sa));
	}

	for (sa = ISC_LIST_HEAD(fctx->edns512);
	     sa != NULL;
	     sa = next_sa) {
		next_sa = ISC_LIST_NEXT(sa, link);
		ISC_LIST_UNLINK(fctx->edns512, sa, link);
		isc_mem_put(res->buckets[bucketnum].mctx, sa, sizeof(*sa));
	}

	isc_timer_detach(&fctx->timer);
	dns_message_destroy(&fctx->rmessage);
	dns_message_destroy(&fctx->qmessage);
	if (dns_name_countlabels(&fctx->domain) > 0)
		dns_name_free(&fctx->domain, res->buckets[bucketnum].mctx);
	if (dns_rdataset_isassociated(&fctx->nameservers))
		dns_rdataset_disassociate(&fctx->nameservers);
	dns_name_free(&fctx->name, res->buckets[bucketnum].mctx);
	dns_db_detach(&fctx->cache);
	dns_adb_detach(&fctx->adb);
	isc_mem_free(res->buckets[bucketnum].mctx, fctx->info);
	isc_mem_put(res->buckets[bucketnum].mctx, fctx, sizeof(*fctx));

	LOCK(&res->nlock);
	res->nfctx--;
	UNLOCK(&res->nlock);

	if (res->buckets[bucketnum].exiting &&
	    ISC_LIST_EMPTY(res->buckets[bucketnum].fctxs))
		return (ISC_TRUE);

	return (ISC_FALSE);
}

/*
 * Fetch event handlers.
 */

static void
fctx_timeout(isc_task_t *task, isc_event_t *event) {
	fetchctx_t *fctx = event->ev_arg;
	isc_timerevent_t *tevent = (isc_timerevent_t *)event;
	resquery_t *query;

	REQUIRE(VALID_FCTX(fctx));

	UNUSED(task);

	FCTXTRACE("timeout");

	inc_stats(fctx->res, dns_resstatscounter_querytimeout);

	if (event->ev_type == ISC_TIMEREVENT_LIFE) {
		fctx->reason = NULL;
		fctx_done(fctx, ISC_R_TIMEDOUT, __LINE__);
	} else {
		isc_result_t result;

		fctx->timeouts++;
		fctx->timeout = ISC_TRUE;
		/*
		 * We could cancel the running queries here, or we could let
		 * them keep going.  Since we normally use separate sockets for
		 * different queries, we adopt the former approach to reduce
		 * the number of open sockets: cancel the oldest query if it
		 * expired after the query had started (this is usually the
		 * case but is not always so, depending on the task schedule
		 * timing).
		 */
		query = ISC_LIST_HEAD(fctx->queries);
		if (query != NULL &&
		    isc_time_compare(&tevent->due, &query->start) >= 0) {
			fctx_cancelquery(&query, NULL, NULL, ISC_TRUE);
		}
		fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;
		/*
		 * Our timer has triggered.  Reestablish the fctx lifetime
		 * timer.
		 */
		result = fctx_starttimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else
			/*
			 * Keep trying.
			 */
			fctx_try(fctx, ISC_TRUE);
	}

	isc_event_free(&event);
}

static void
fctx_shutdown(fetchctx_t *fctx) {
	isc_event_t *cevent;

	/*
	 * Start the shutdown process for fctx, if it isn't already underway.
	 */

	FCTXTRACE("shutdown");

	/*
	 * The caller must be holding the appropriate bucket lock.
	 */

	if (fctx->want_shutdown)
		return;

	fctx->want_shutdown = ISC_TRUE;

	/*
	 * Unless we're still initializing (in which case the
	 * control event is still outstanding), we need to post
	 * the control event to tell the fetch we want it to
	 * exit.
	 */
	if (fctx->state != fetchstate_init) {
		cevent = &fctx->control_event;
		isc_task_send(fctx->res->buckets[fctx->bucketnum].task,
			      &cevent);
	}
}

static void
fctx_doshutdown(isc_task_t *task, isc_event_t *event) {
	fetchctx_t *fctx = event->ev_arg;
	isc_boolean_t bucket_empty = ISC_FALSE;
	dns_resolver_t *res;
	unsigned int bucketnum;
	dns_validator_t *validator;

	REQUIRE(VALID_FCTX(fctx));

	UNUSED(task);

	res = fctx->res;
	bucketnum = fctx->bucketnum;

	FCTXTRACE("doshutdown");

	/*
	 * An fctx that is shutting down is no longer in ADDRWAIT mode.
	 */
	fctx->attributes &= ~FCTX_ATTR_ADDRWAIT;

	/*
	 * Cancel all pending validators.  Note that this must be done
	 * without the bucket lock held, since that could cause deadlock.
	 */
	validator = ISC_LIST_HEAD(fctx->validators);
	while (validator != NULL) {
		dns_validator_cancel(validator);
		validator = ISC_LIST_NEXT(validator, link);
	}

	if (fctx->nsfetch != NULL)
		dns_resolver_cancelfetch(fctx->nsfetch);

	/*
	 * Shut down anything that is still running on behalf of this
	 * fetch.  To avoid deadlock with the ADB, we must do this
	 * before we lock the bucket lock.
	 */
	fctx_stopeverything(fctx, ISC_FALSE);

	LOCK(&res->buckets[bucketnum].lock);

	fctx->attributes |= FCTX_ATTR_SHUTTINGDOWN;

	INSIST(fctx->state == fetchstate_active ||
	       fctx->state == fetchstate_done);
	INSIST(fctx->want_shutdown);

	if (fctx->state != fetchstate_done) {
		fctx->state = fetchstate_done;
		fctx_sendevents(fctx, ISC_R_CANCELED, __LINE__);
	}

	if (fctx->references == 0 && fctx->pending == 0 &&
	    fctx->nqueries == 0 && ISC_LIST_EMPTY(fctx->validators))
		bucket_empty = fctx_destroy(fctx);

	UNLOCK(&res->buckets[bucketnum].lock);

	if (bucket_empty)
		empty_bucket(res);
}

static void
fctx_start(isc_task_t *task, isc_event_t *event) {
	fetchctx_t *fctx = event->ev_arg;
	isc_boolean_t done = ISC_FALSE, bucket_empty = ISC_FALSE;
	dns_resolver_t *res;
	unsigned int bucketnum;

	REQUIRE(VALID_FCTX(fctx));

	UNUSED(task);

	res = fctx->res;
	bucketnum = fctx->bucketnum;

	FCTXTRACE("start");

	LOCK(&res->buckets[bucketnum].lock);

	INSIST(fctx->state == fetchstate_init);
	if (fctx->want_shutdown) {
		/*
		 * We haven't started this fctx yet, and we've been requested
		 * to shut it down.
		 */
		fctx->attributes |= FCTX_ATTR_SHUTTINGDOWN;
		fctx->state = fetchstate_done;
		fctx_sendevents(fctx, ISC_R_CANCELED, __LINE__);
		/*
		 * Since we haven't started, we INSIST that we have no
		 * pending ADB finds and no pending validations.
		 */
		INSIST(fctx->pending == 0);
		INSIST(fctx->nqueries == 0);
		INSIST(ISC_LIST_EMPTY(fctx->validators));
		if (fctx->references == 0) {
			/*
			 * It's now safe to destroy this fctx.
			 */
			bucket_empty = fctx_destroy(fctx);
		}
		done = ISC_TRUE;
	} else {
		/*
		 * Normal fctx startup.
		 */
		fctx->state = fetchstate_active;
		/*
		 * Reset the control event for later use in shutting down
		 * the fctx.
		 */
		ISC_EVENT_INIT(event, sizeof(*event), 0, NULL,
			       DNS_EVENT_FETCHCONTROL, fctx_doshutdown, fctx,
			       NULL, NULL, NULL);
	}

	UNLOCK(&res->buckets[bucketnum].lock);

	if (!done) {
		isc_result_t result;

		/*
		 * All is well.  Start working on the fetch.
		 */
		result = fctx_starttimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else
			fctx_try(fctx, ISC_FALSE);
	} else if (bucket_empty)
		empty_bucket(res);
}

/*
 * Fetch Creation, Joining, and Cancelation.
 */

static inline isc_result_t
fctx_join(fetchctx_t *fctx, isc_task_t *task, isc_sockaddr_t *client,
	  dns_messageid_t id, isc_taskaction_t action, void *arg,
	  dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
	  dns_fetch_t *fetch)
{
	isc_task_t *clone;
	dns_fetchevent_t *event;

	FCTXTRACE("join");

	/*
	 * We store the task we're going to send this event to in the
	 * sender field.  We'll make the fetch the sender when we actually
	 * send the event.
	 */
	clone = NULL;
	isc_task_attach(task, &clone);
	event = (dns_fetchevent_t *)
		isc_event_allocate(fctx->res->mctx, clone, DNS_EVENT_FETCHDONE,
				   action, arg, sizeof(*event));
	if (event == NULL) {
		isc_task_detach(&clone);
		return (ISC_R_NOMEMORY);
	}
	event->result = DNS_R_SERVFAIL;
	event->qtype = fctx->type;
	event->db = NULL;
	event->node = NULL;
	event->rdataset = rdataset;
	event->sigrdataset = sigrdataset;
	event->fetch = fetch;
	event->client = client;
	event->id = id;
	dns_fixedname_init(&event->foundname);

	/*
	 * Make sure that we can store the sigrdataset in the
	 * first event if it is needed by any of the events.
	 */
	if (event->sigrdataset != NULL)
		ISC_LIST_PREPEND(fctx->events, event, ev_link);
	else
		ISC_LIST_APPEND(fctx->events, event, ev_link);
	fctx->references++;

	fetch->magic = DNS_FETCH_MAGIC;
	fetch->private = fctx;

	return (ISC_R_SUCCESS);
}

static isc_result_t
fctx_create(dns_resolver_t *res, dns_name_t *name, dns_rdatatype_t type,
	    dns_name_t *domain, dns_rdataset_t *nameservers,
	    unsigned int options, unsigned int bucketnum, fetchctx_t **fctxp)
{
	fetchctx_t *fctx;
	isc_result_t result;
	isc_result_t iresult;
	isc_interval_t interval;
	dns_fixedname_t fixed;
	unsigned int findoptions = 0;
	char buf[DNS_NAME_FORMATSIZE + DNS_RDATATYPE_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	dns_name_t suffix;

	/*
	 * Caller must be holding the lock for bucket number 'bucketnum'.
	 */
	REQUIRE(fctxp != NULL && *fctxp == NULL);

	fctx = isc_mem_get(res->buckets[bucketnum].mctx, sizeof(*fctx));
	if (fctx == NULL)
		return (ISC_R_NOMEMORY);
	dns_name_format(name, buf, sizeof(buf));
	dns_rdatatype_format(type, typebuf, sizeof(typebuf));
	strcat(buf, "/");       /* checked */
	strcat(buf, typebuf);   /* checked */
	fctx->info = isc_mem_strdup(res->buckets[bucketnum].mctx, buf);
	if (fctx->info == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_fetch;
	}
	FCTXTRACE("create");
	dns_name_init(&fctx->name, NULL);
	result = dns_name_dup(name, res->buckets[bucketnum].mctx, &fctx->name);
	if (result != ISC_R_SUCCESS)
		goto cleanup_info;
	dns_name_init(&fctx->domain, NULL);
	dns_rdataset_init(&fctx->nameservers);

	fctx->type = type;
	fctx->options = options;
	/*
	 * Note!  We do not attach to the task.  We are relying on the
	 * resolver to ensure that this task doesn't go away while we are
	 * using it.
	 */
	fctx->res = res;
	fctx->references = 0;
	fctx->bucketnum = bucketnum;
	fctx->state = fetchstate_init;
	fctx->want_shutdown = ISC_FALSE;
	fctx->cloned = ISC_FALSE;
	ISC_LIST_INIT(fctx->queries);
	ISC_LIST_INIT(fctx->finds);
	ISC_LIST_INIT(fctx->altfinds);
	ISC_LIST_INIT(fctx->forwaddrs);
	ISC_LIST_INIT(fctx->altaddrs);
	ISC_LIST_INIT(fctx->forwarders);
	fctx->fwdpolicy = dns_fwdpolicy_none;
	ISC_LIST_INIT(fctx->bad);
	ISC_LIST_INIT(fctx->edns);
	ISC_LIST_INIT(fctx->edns512);
	ISC_LIST_INIT(fctx->validators);
	fctx->validator = NULL;
	fctx->find = NULL;
	fctx->altfind = NULL;
	fctx->pending = 0;
	fctx->restarts = 0;
	fctx->querysent = 0;
	fctx->referrals = 0;
	TIME_NOW(&fctx->start);
	fctx->timeouts = 0;
	fctx->lamecount = 0;
	fctx->adberr = 0;
	fctx->neterr = 0;
	fctx->badresp = 0;
	fctx->findfail = 0;
	fctx->valfail = 0;
	fctx->result = ISC_R_FAILURE;
	fctx->vresult = ISC_R_SUCCESS;
	fctx->exitline = -1;	/* sentinel */
	fctx->logged = ISC_FALSE;
	fctx->attributes = 0;
	fctx->spilled = ISC_FALSE;
	fctx->nqueries = 0;
	fctx->reason = NULL;
	fctx->rand_buf = 0;
	fctx->rand_bits = 0;
	fctx->timeout = ISC_FALSE;

	dns_name_init(&fctx->nsname, NULL);
	fctx->nsfetch = NULL;
	dns_rdataset_init(&fctx->nsrrset);

	if (domain == NULL) {
		dns_forwarders_t *forwarders = NULL;
		unsigned int labels;

		/*
		 * DS records are found in the parent server.
		 * Strip label to get the correct forwarder (if any).
		 */
		if (fctx->type == dns_rdatatype_ds &&
		    dns_name_countlabels(name) > 1) {
			dns_name_init(&suffix, NULL);
			labels = dns_name_countlabels(name);
			dns_name_getlabelsequence(name, 1, labels - 1, &suffix);
			name = &suffix;
		}
		dns_fixedname_init(&fixed);
		domain = dns_fixedname_name(&fixed);
		result = dns_fwdtable_find2(fctx->res->view->fwdtable, name,
					    domain, &forwarders);
		if (result == ISC_R_SUCCESS)
			fctx->fwdpolicy = forwarders->fwdpolicy;

		if (fctx->fwdpolicy != dns_fwdpolicy_only) {
			/*
			 * The caller didn't supply a query domain and
			 * nameservers, and we're not in forward-only mode,
			 * so find the best nameservers to use.
			 */
			if (dns_rdatatype_atparent(type))
				findoptions |= DNS_DBFIND_NOEXACT;
			result = dns_view_findzonecut(res->view, name, domain,
						      0, findoptions, ISC_TRUE,
						      &fctx->nameservers,
						      NULL);
			if (result != ISC_R_SUCCESS)
				goto cleanup_name;
			result = dns_name_dup(domain,
					      res->buckets[bucketnum].mctx,
					      &fctx->domain);
			if (result != ISC_R_SUCCESS) {
				dns_rdataset_disassociate(&fctx->nameservers);
				goto cleanup_name;
			}
		} else {
			/*
			 * We're in forward-only mode.  Set the query domain.
			 */
			result = dns_name_dup(domain,
					      res->buckets[bucketnum].mctx,
					      &fctx->domain);
			if (result != ISC_R_SUCCESS)
				goto cleanup_name;
		}
	} else {
		result = dns_name_dup(domain,
				      res->buckets[bucketnum].mctx,
				      &fctx->domain);
		if (result != ISC_R_SUCCESS)
			goto cleanup_name;
		dns_rdataset_clone(nameservers, &fctx->nameservers);
	}

	INSIST(dns_name_issubdomain(&fctx->name, &fctx->domain));

	fctx->qmessage = NULL;
	result = dns_message_create(res->buckets[bucketnum].mctx,
				    DNS_MESSAGE_INTENTRENDER,
				    &fctx->qmessage);

	if (result != ISC_R_SUCCESS)
		goto cleanup_domain;

	fctx->rmessage = NULL;
	result = dns_message_create(res->buckets[bucketnum].mctx,
				    DNS_MESSAGE_INTENTPARSE,
				    &fctx->rmessage);

	if (result != ISC_R_SUCCESS)
		goto cleanup_qmessage;

	/*
	 * Compute an expiration time for the entire fetch.
	 */
	isc_interval_set(&interval, 30, 0);             /* XXXRTH constant */
	iresult = isc_time_nowplusinterval(&fctx->expires, &interval);
	if (iresult != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_time_nowplusinterval: %s",
				 isc_result_totext(iresult));
		result = ISC_R_UNEXPECTED;
		goto cleanup_rmessage;
	}

	/*
	 * Default retry interval initialization.  We set the interval now
	 * mostly so it won't be uninitialized.  It will be set to the
	 * correct value before a query is issued.
	 */
	isc_interval_set(&fctx->interval, 2, 0);

	/*
	 * Create an inactive timer.  It will be made active when the fetch
	 * is actually started.
	 */
	fctx->timer = NULL;
	iresult = isc_timer_create(res->timermgr, isc_timertype_inactive,
				   NULL, NULL,
				   res->buckets[bucketnum].task, fctx_timeout,
				   fctx, &fctx->timer);
	if (iresult != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_timer_create: %s",
				 isc_result_totext(iresult));
		result = ISC_R_UNEXPECTED;
		goto cleanup_rmessage;
	}

	/*
	 * Attach to the view's cache and adb.
	 */
	fctx->cache = NULL;
	dns_db_attach(res->view->cachedb, &fctx->cache);
	fctx->adb = NULL;
	dns_adb_attach(res->view->adb, &fctx->adb);

	ISC_LIST_INIT(fctx->events);
	ISC_LINK_INIT(fctx, link);
	fctx->magic = FCTX_MAGIC;

	ISC_LIST_APPEND(res->buckets[bucketnum].fctxs, fctx, link);

	LOCK(&res->nlock);
	res->nfctx++;
	UNLOCK(&res->nlock);

	*fctxp = fctx;

	return (ISC_R_SUCCESS);

 cleanup_rmessage:
	dns_message_destroy(&fctx->rmessage);

 cleanup_qmessage:
	dns_message_destroy(&fctx->qmessage);

 cleanup_domain:
	if (dns_name_countlabels(&fctx->domain) > 0)
		dns_name_free(&fctx->domain, res->buckets[bucketnum].mctx);
	if (dns_rdataset_isassociated(&fctx->nameservers))
		dns_rdataset_disassociate(&fctx->nameservers);

 cleanup_name:
	dns_name_free(&fctx->name, res->buckets[bucketnum].mctx);

 cleanup_info:
	isc_mem_free(res->buckets[bucketnum].mctx, fctx->info);

 cleanup_fetch:
	isc_mem_put(res->buckets[bucketnum].mctx, fctx, sizeof(*fctx));

	return (result);
}

/*
 * Handle Responses
 */
static inline isc_boolean_t
is_lame(fetchctx_t *fctx) {
	dns_message_t *message = fctx->rmessage;
	dns_name_t *name;
	dns_rdataset_t *rdataset;
	isc_result_t result;

	if (message->rcode != dns_rcode_noerror &&
	    message->rcode != dns_rcode_nxdomain)
		return (ISC_FALSE);

	if (message->counts[DNS_SECTION_ANSWER] != 0)
		return (ISC_FALSE);

	if (message->counts[DNS_SECTION_AUTHORITY] == 0)
		return (ISC_FALSE);

	result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			dns_namereln_t namereln;
			int order;
			unsigned int labels;
			if (rdataset->type != dns_rdatatype_ns)
				continue;
			namereln = dns_name_fullcompare(name, &fctx->domain,
							&order, &labels);
			if (namereln == dns_namereln_equal &&
			    (message->flags & DNS_MESSAGEFLAG_AA) != 0)
				return (ISC_FALSE);
			if (namereln == dns_namereln_subdomain)
				return (ISC_FALSE);
			return (ISC_TRUE);
		}
		result = dns_message_nextname(message, DNS_SECTION_AUTHORITY);
	}

	return (ISC_FALSE);
}

static inline void
log_lame(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char domainbuf[DNS_NAME_FORMATSIZE];
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];

	dns_name_format(&fctx->name, namebuf, sizeof(namebuf));
	dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
	isc_sockaddr_format(&addrinfo->sockaddr, addrbuf, sizeof(addrbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_LAME_SERVERS,
		      DNS_LOGMODULE_RESOLVER, ISC_LOG_INFO,
		      "lame server resolving '%s' (in '%s'?): %s",
		      namebuf, domainbuf, addrbuf);
}

static inline isc_result_t
same_question(fetchctx_t *fctx) {
	isc_result_t result;
	dns_message_t *message = fctx->rmessage;
	dns_name_t *name;
	dns_rdataset_t *rdataset;

	/*
	 * Caller must be holding the fctx lock.
	 */

	/*
	 * XXXRTH  Currently we support only one question.
	 */
	if (message->counts[DNS_SECTION_QUESTION] != 1)
		return (DNS_R_FORMERR);

	result = dns_message_firstname(message, DNS_SECTION_QUESTION);
	if (result != ISC_R_SUCCESS)
		return (result);
	name = NULL;
	dns_message_currentname(message, DNS_SECTION_QUESTION, &name);
	rdataset = ISC_LIST_HEAD(name->list);
	INSIST(rdataset != NULL);
	INSIST(ISC_LIST_NEXT(rdataset, link) == NULL);
	if (fctx->type != rdataset->type ||
	    fctx->res->rdclass != rdataset->rdclass ||
	    !dns_name_equal(&fctx->name, name))
		return (DNS_R_FORMERR);

	return (ISC_R_SUCCESS);
}

static void
clone_results(fetchctx_t *fctx) {
	dns_fetchevent_t *event, *hevent;
	isc_result_t result;
	dns_name_t *name, *hname;

	FCTXTRACE("clone_results");

	/*
	 * Set up any other events to have the same data as the first
	 * event.
	 *
	 * Caller must be holding the appropriate lock.
	 */

	fctx->cloned = ISC_TRUE;
	hevent = ISC_LIST_HEAD(fctx->events);
	if (hevent == NULL)
		return;
	hname = dns_fixedname_name(&hevent->foundname);
	for (event = ISC_LIST_NEXT(hevent, ev_link);
	     event != NULL;
	     event = ISC_LIST_NEXT(event, ev_link)) {
		name = dns_fixedname_name(&event->foundname);
		result = dns_name_copy(hname, name, NULL);
		if (result != ISC_R_SUCCESS)
			event->result = result;
		else
			event->result = hevent->result;
		dns_db_attach(hevent->db, &event->db);
		dns_db_attachnode(hevent->db, hevent->node, &event->node);
		INSIST(hevent->rdataset != NULL);
		INSIST(event->rdataset != NULL);
		if (dns_rdataset_isassociated(hevent->rdataset))
			dns_rdataset_clone(hevent->rdataset, event->rdataset);
		INSIST(! (hevent->sigrdataset == NULL &&
			  event->sigrdataset != NULL));
		if (hevent->sigrdataset != NULL &&
		    dns_rdataset_isassociated(hevent->sigrdataset) &&
		    event->sigrdataset != NULL)
			dns_rdataset_clone(hevent->sigrdataset,
					   event->sigrdataset);
	}
}

#define CACHE(r)        (((r)->attributes & DNS_RDATASETATTR_CACHE) != 0)
#define ANSWER(r)       (((r)->attributes & DNS_RDATASETATTR_ANSWER) != 0)
#define ANSWERSIG(r)    (((r)->attributes & DNS_RDATASETATTR_ANSWERSIG) != 0)
#define EXTERNAL(r)     (((r)->attributes & DNS_RDATASETATTR_EXTERNAL) != 0)
#define CHAINING(r)     (((r)->attributes & DNS_RDATASETATTR_CHAINING) != 0)
#define CHASE(r)        (((r)->attributes & DNS_RDATASETATTR_CHASE) != 0)
#define CHECKNAMES(r)   (((r)->attributes & DNS_RDATASETATTR_CHECKNAMES) != 0)


/*
 * Destroy '*fctx' if it is ready to be destroyed (i.e., if it has
 * no references and is no longer waiting for any events).  If this
 * was the last fctx in the resolver, destroy the resolver.
 *
 * Requires:
 *      '*fctx' is shutting down.
 */
static void
maybe_destroy(fetchctx_t *fctx) {
	unsigned int bucketnum;
	isc_boolean_t bucket_empty = ISC_FALSE;
	dns_resolver_t *res = fctx->res;
	dns_validator_t *validator, *next_validator;

	REQUIRE(SHUTTINGDOWN(fctx));

	if (fctx->pending != 0 || fctx->nqueries != 0)
		return;

	for (validator = ISC_LIST_HEAD(fctx->validators);
	     validator != NULL; validator = next_validator) {
		next_validator = ISC_LIST_NEXT(validator, link);
		dns_validator_cancel(validator);
		/*
		 * If this is a active validator wait for the cancel
		 * to complete before calling dns_validator_destroy().
		 */
		if (validator == fctx->validator)
			continue;
		ISC_LIST_UNLINK(fctx->validators, validator, link);
		dns_validator_destroy(&validator);
	}

	bucketnum = fctx->bucketnum;
	LOCK(&res->buckets[bucketnum].lock);
	if (fctx->references == 0 && ISC_LIST_EMPTY(fctx->validators))
		bucket_empty = fctx_destroy(fctx);
	UNLOCK(&res->buckets[bucketnum].lock);

	if (bucket_empty)
		empty_bucket(res);
}

/*
 * The validator has finished.
 */
static void
validated(isc_task_t *task, isc_event_t *event) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_result_t eresult = ISC_R_SUCCESS;
	isc_stdtime_t now;
	fetchctx_t *fctx;
	dns_validatorevent_t *vevent;
	dns_fetchevent_t *hevent;
	dns_rdataset_t *ardataset = NULL;
	dns_rdataset_t *asigrdataset = NULL;
	dns_dbnode_t *node = NULL;
	isc_boolean_t negative;
	isc_boolean_t chaining;
	isc_boolean_t sentresponse;
	isc_uint32_t ttl;
	dns_dbnode_t *nsnode = NULL;
	dns_name_t *name;
	dns_rdataset_t *rdataset;
	dns_rdataset_t *sigrdataset;
	dns_valarg_t *valarg;
	dns_adbaddrinfo_t *addrinfo;

	UNUSED(task); /* for now */

	REQUIRE(event->ev_type == DNS_EVENT_VALIDATORDONE);
	valarg = event->ev_arg;
	fctx = valarg->fctx;
	addrinfo = valarg->addrinfo;
	REQUIRE(VALID_FCTX(fctx));
	REQUIRE(!ISC_LIST_EMPTY(fctx->validators));

	vevent = (dns_validatorevent_t *)event;

	FCTXTRACE("received validation completion event");

	ISC_LIST_UNLINK(fctx->validators, vevent->validator, link);
	fctx->validator = NULL;

	/*
	 * Destroy the validator early so that we can
	 * destroy the fctx if necessary.
	 */
	dns_validator_destroy(&vevent->validator);
	isc_mem_put(fctx->res->buckets[fctx->bucketnum].mctx,
		    valarg, sizeof(*valarg));

	negative = ISC_TF(vevent->rdataset == NULL);

	sentresponse = ISC_TF((fctx->options & DNS_FETCHOPT_NOVALIDATE) != 0);

	/*
	 * If shutting down, ignore the results.  Check to see if we're
	 * done waiting for validator completions and ADB pending events; if
	 * so, destroy the fctx.
	 */
	if (SHUTTINGDOWN(fctx) && !sentresponse) {
		maybe_destroy(fctx);    /* Locks bucket. */
		goto cleanup_event;
	}

	LOCK(&fctx->res->buckets[fctx->bucketnum].lock);

	/*
	 * If chaining, we need to make sure that the right result code is
	 * returned, and that the rdatasets are bound.
	 */
	if (vevent->result == ISC_R_SUCCESS &&
	    !negative &&
	    vevent->rdataset != NULL &&
	    CHAINING(vevent->rdataset))
	{
		if (vevent->rdataset->type == dns_rdatatype_cname)
			eresult = DNS_R_CNAME;
		else {
			INSIST(vevent->rdataset->type == dns_rdatatype_dname);
			eresult = DNS_R_DNAME;
		}
		chaining = ISC_TRUE;
	} else
		chaining = ISC_FALSE;

	/*
	 * Either we're not shutting down, or we are shutting down but want
	 * to cache the result anyway (if this was a validation started by
	 * a query with cd set)
	 */

	hevent = ISC_LIST_HEAD(fctx->events);
	if (hevent != NULL) {
		if (!negative && !chaining &&
		    (fctx->type == dns_rdatatype_any ||
		     fctx->type == dns_rdatatype_rrsig ||
		     fctx->type == dns_rdatatype_sig)) {
			/*
			 * Don't bind rdatasets; the caller
			 * will iterate the node.
			 */
		} else {
			ardataset = hevent->rdataset;
			asigrdataset = hevent->sigrdataset;
		}
	}

	if (vevent->result != ISC_R_SUCCESS) {
		FCTXTRACE("validation failed");
		inc_stats(fctx->res, dns_resstatscounter_valfail);
		fctx->valfail++;
		fctx->vresult = vevent->result;
		result = ISC_R_NOTFOUND;
		if (vevent->rdataset != NULL)
			result = dns_db_findnode(fctx->cache, vevent->name,
						 ISC_TRUE, &node);
		if (result == ISC_R_SUCCESS)
			(void)dns_db_deleterdataset(fctx->cache, node, NULL,
						    vevent->type, 0);
		if (result == ISC_R_SUCCESS && vevent->sigrdataset != NULL)
			(void)dns_db_deleterdataset(fctx->cache, node, NULL,
						    dns_rdatatype_rrsig,
						    vevent->type);
		if (result == ISC_R_SUCCESS)
			dns_db_detachnode(fctx->cache, &node);
		result = vevent->result;
		add_bad(fctx, addrinfo, result, badns_validation);
		isc_event_free(&event);
		UNLOCK(&fctx->res->buckets[fctx->bucketnum].lock);
		INSIST(fctx->validator == NULL);
		fctx->validator = ISC_LIST_HEAD(fctx->validators);
		if (fctx->validator != NULL) {
			dns_validator_send(fctx->validator);
		} else if (sentresponse)
			fctx_done(fctx, result, __LINE__); /* Locks bucket. */
		else
			fctx_try(fctx, ISC_TRUE);       /* Locks bucket. */
		return;
	}

	isc_stdtime_get(&now);

	if (negative) {
		dns_rdatatype_t covers;
		FCTXTRACE("nonexistence validation OK");

		inc_stats(fctx->res, dns_resstatscounter_valnegsuccess);

		if (fctx->rmessage->rcode == dns_rcode_nxdomain)
			covers = dns_rdatatype_any;
		else
			covers = fctx->type;

		result = dns_db_findnode(fctx->cache, vevent->name, ISC_TRUE,
					 &node);
		if (result != ISC_R_SUCCESS)
			goto noanswer_response;

		/*
		 * If we are asking for a SOA record set the cache time
		 * to zero to facilitate locating the containing zone of
		 * a arbitrary zone.
		 */
		ttl = fctx->res->view->maxncachettl;
		if (fctx->type == dns_rdatatype_soa &&
		    covers == dns_rdatatype_any &&
		    fctx->res->zero_no_soa_ttl)
			ttl = 0;

		result = ncache_adderesult(fctx->rmessage, fctx->cache, node,
					   covers, now, ttl, vevent->optout,
					   ardataset, &eresult);
		if (result != ISC_R_SUCCESS)
			goto noanswer_response;
		goto answer_response;
	} else
		inc_stats(fctx->res, dns_resstatscounter_valsuccess);

	FCTXTRACE("validation OK");

	if (vevent->proofs[DNS_VALIDATOR_NOQNAMEPROOF] != NULL) {

		result = dns_rdataset_addnoqname(vevent->rdataset,
				   vevent->proofs[DNS_VALIDATOR_NOQNAMEPROOF]);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		INSIST(vevent->sigrdataset != NULL);
		vevent->sigrdataset->ttl = vevent->rdataset->ttl;
		if (vevent->proofs[DNS_VALIDATOR_CLOSESTENCLOSER] != NULL) {
			result = dns_rdataset_addclosest(vevent->rdataset,
				 vevent->proofs[DNS_VALIDATOR_CLOSESTENCLOSER]);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
		}
	}

	/*
	 * The data was already cached as pending data.
	 * Re-cache it as secure and bind the cached
	 * rdatasets to the first event on the fetch
	 * event list.
	 */
	result = dns_db_findnode(fctx->cache, vevent->name, ISC_TRUE, &node);
	if (result != ISC_R_SUCCESS)
		goto noanswer_response;

	result = dns_db_addrdataset(fctx->cache, node, NULL, now,
				    vevent->rdataset, 0, ardataset);
	if (result != ISC_R_SUCCESS &&
	    result != DNS_R_UNCHANGED)
		goto noanswer_response;
	if (ardataset != NULL && ardataset->type == 0) {
		if (NXDOMAIN(ardataset))
			eresult = DNS_R_NCACHENXDOMAIN;
		else
			eresult = DNS_R_NCACHENXRRSET;
	} else if (vevent->sigrdataset != NULL) {
		result = dns_db_addrdataset(fctx->cache, node, NULL, now,
					    vevent->sigrdataset, 0,
					    asigrdataset);
		if (result != ISC_R_SUCCESS &&
		    result != DNS_R_UNCHANGED)
			goto noanswer_response;
	}

	if (sentresponse) {
		/*
		 * If we only deferred the destroy because we wanted to cache
		 * the data, destroy now.
		 */
		dns_db_detachnode(fctx->cache, &node);
		UNLOCK(&fctx->res->buckets[fctx->bucketnum].lock);
		if (SHUTTINGDOWN(fctx))
			maybe_destroy(fctx);    /* Locks bucket. */
		goto cleanup_event;
	}

	if (!ISC_LIST_EMPTY(fctx->validators)) {
		INSIST(!negative);
		INSIST(fctx->type == dns_rdatatype_any ||
		       fctx->type == dns_rdatatype_rrsig ||
		       fctx->type == dns_rdatatype_sig);
		/*
		 * Don't send a response yet - we have
		 * more rdatasets that still need to
		 * be validated.
		 */
		dns_db_detachnode(fctx->cache, &node);
		UNLOCK(&fctx->res->buckets[fctx->bucketnum].lock);
		dns_validator_send(ISC_LIST_HEAD(fctx->validators));
		goto cleanup_event;
	}

 answer_response:
	/*
	 * Cache any NS/NSEC records that happened to be validated.
	 */
	result = dns_message_firstname(fctx->rmessage, DNS_SECTION_AUTHORITY);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(fctx->rmessage, DNS_SECTION_AUTHORITY,
					&name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			if ((rdataset->type != dns_rdatatype_ns &&
			     rdataset->type != dns_rdatatype_nsec) ||
			    rdataset->trust != dns_trust_secure)
				continue;
			for (sigrdataset = ISC_LIST_HEAD(name->list);
			     sigrdataset != NULL;
			     sigrdataset = ISC_LIST_NEXT(sigrdataset, link)) {
				if (sigrdataset->type != dns_rdatatype_rrsig ||
				    sigrdataset->covers != rdataset->type)
					continue;
				break;
			}
			if (sigrdataset == NULL ||
			    sigrdataset->trust != dns_trust_secure)
				continue;
			result = dns_db_findnode(fctx->cache, name, ISC_TRUE,
						 &nsnode);
			if (result != ISC_R_SUCCESS)
				continue;

			result = dns_db_addrdataset(fctx->cache, nsnode, NULL,
						    now, rdataset, 0, NULL);
			if (result == ISC_R_SUCCESS)
				result = dns_db_addrdataset(fctx->cache, nsnode,
							    NULL, now,
							    sigrdataset, 0,
							    NULL);
			dns_db_detachnode(fctx->cache, &nsnode);
		}
		result = dns_message_nextname(fctx->rmessage,
					      DNS_SECTION_AUTHORITY);
	}

	result = ISC_R_SUCCESS;

	/*
	 * Respond with an answer, positive or negative,
	 * as opposed to an error.  'node' must be non-NULL.
	 */

	fctx->attributes |= FCTX_ATTR_HAVEANSWER;

	if (hevent != NULL) {
		hevent->result = eresult;
		RUNTIME_CHECK(dns_name_copy(vevent->name,
			      dns_fixedname_name(&hevent->foundname), NULL)
			      == ISC_R_SUCCESS);
		dns_db_attach(fctx->cache, &hevent->db);
		dns_db_transfernode(fctx->cache, &node, &hevent->node);
		clone_results(fctx);
	}

 noanswer_response:
	if (node != NULL)
		dns_db_detachnode(fctx->cache, &node);

	UNLOCK(&fctx->res->buckets[fctx->bucketnum].lock);

	fctx_done(fctx, result, __LINE__); /* Locks bucket. */

 cleanup_event:
	INSIST(node == NULL);
	isc_event_free(&event);
}

static inline isc_result_t
cache_name(fetchctx_t *fctx, dns_name_t *name, dns_adbaddrinfo_t *addrinfo,
	   isc_stdtime_t now)
{
	dns_rdataset_t *rdataset, *sigrdataset;
	dns_rdataset_t *addedrdataset, *ardataset, *asigrdataset;
	dns_rdataset_t *valrdataset = NULL, *valsigrdataset = NULL;
	dns_dbnode_t *node, **anodep;
	dns_db_t **adbp;
	dns_name_t *aname;
	dns_resolver_t *res;
	isc_boolean_t need_validation, secure_domain, have_answer;
	isc_result_t result, eresult;
	dns_fetchevent_t *event;
	unsigned int options;
	isc_task_t *task;
	isc_boolean_t fail;
	unsigned int valoptions = 0;

	/*
	 * The appropriate bucket lock must be held.
	 */

	res = fctx->res;
	need_validation = ISC_FALSE;
	secure_domain = ISC_FALSE;
	have_answer = ISC_FALSE;
	eresult = ISC_R_SUCCESS;
	task = res->buckets[fctx->bucketnum].task;

	/*
	 * Is DNSSEC validation required for this name?
	 */
	if (res->view->enablevalidation) {
		result = dns_keytable_issecuredomain(res->view->secroots, name,
						     &secure_domain);
		if (result != ISC_R_SUCCESS)
			return (result);

		if (!secure_domain && res->view->dlv != NULL) {
			valoptions = DNS_VALIDATOR_DLV;
			secure_domain = ISC_TRUE;
		}
	}

	if ((fctx->options & DNS_FETCHOPT_NOVALIDATE) != 0)
		need_validation = ISC_FALSE;
	else
		need_validation = secure_domain;

	adbp = NULL;
	aname = NULL;
	anodep = NULL;
	ardataset = NULL;
	asigrdataset = NULL;
	event = NULL;
	if ((name->attributes & DNS_NAMEATTR_ANSWER) != 0 &&
	    !need_validation) {
		have_answer = ISC_TRUE;
		event = ISC_LIST_HEAD(fctx->events);
		if (event != NULL) {
			adbp = &event->db;
			aname = dns_fixedname_name(&event->foundname);
			result = dns_name_copy(name, aname, NULL);
			if (result != ISC_R_SUCCESS)
				return (result);
			anodep = &event->node;
			/*
			 * If this is an ANY, SIG or RRSIG query, we're not
			 * going to return any rdatasets, unless we encountered
			 * a CNAME or DNAME as "the answer".  In this case,
			 * we're going to return DNS_R_CNAME or DNS_R_DNAME
			 * and we must set up the rdatasets.
			 */
			if ((fctx->type != dns_rdatatype_any &&
			     fctx->type != dns_rdatatype_rrsig &&
			     fctx->type != dns_rdatatype_sig) ||
			    (name->attributes & DNS_NAMEATTR_CHAINING) != 0) {
				ardataset = event->rdataset;
				asigrdataset = event->sigrdataset;
			}
		}
	}

	/*
	 * Find or create the cache node.
	 */
	node = NULL;
	result = dns_db_findnode(fctx->cache, name, ISC_TRUE, &node);
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * Cache or validate each cacheable rdataset.
	 */
	fail = ISC_TF((fctx->res->options & DNS_RESOLVER_CHECKNAMESFAIL) != 0);
	for (rdataset = ISC_LIST_HEAD(name->list);
	     rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link)) {
		if (!CACHE(rdataset))
			continue;
		if (CHECKNAMES(rdataset)) {
			char namebuf[DNS_NAME_FORMATSIZE];
			char typebuf[DNS_RDATATYPE_FORMATSIZE];
			char classbuf[DNS_RDATATYPE_FORMATSIZE];

			dns_name_format(name, namebuf, sizeof(namebuf));
			dns_rdatatype_format(rdataset->type, typebuf,
					     sizeof(typebuf));
			dns_rdataclass_format(rdataset->rdclass, classbuf,
					      sizeof(classbuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
				      "check-names %s %s/%s/%s",
				      fail ? "failure" : "warning",
				      namebuf, typebuf, classbuf);
			if (fail) {
				if (ANSWER(rdataset)) {
					dns_db_detachnode(fctx->cache, &node);
					return (DNS_R_BADNAME);
				}
				continue;
			}
		}

		/*
		 * Enforce the configure maximum cache TTL.
		 */
		if (rdataset->ttl > res->view->maxcachettl)
			rdataset->ttl = res->view->maxcachettl;

		/*
		 * If this rrset is in a secure domain, do DNSSEC validation
		 * for it, unless it is glue.
		 */
		if (secure_domain && rdataset->trust != dns_trust_glue) {
			/*
			 * RRSIGs are validated as part of validating the
			 * type they cover.
			 */
			if (rdataset->type == dns_rdatatype_rrsig)
				continue;
			/*
			 * Find the SIG for this rdataset, if we have it.
			 */
			for (sigrdataset = ISC_LIST_HEAD(name->list);
			     sigrdataset != NULL;
			     sigrdataset = ISC_LIST_NEXT(sigrdataset, link)) {
				if (sigrdataset->type == dns_rdatatype_rrsig &&
				    sigrdataset->covers == rdataset->type)
					break;
			}
			if (sigrdataset == NULL) {
				if (!ANSWER(rdataset) && need_validation) {
					/*
					 * Ignore non-answer rdatasets that
					 * are missing signatures.
					 */
					continue;
				}
			}

			/*
			 * Normalize the rdataset and sigrdataset TTLs.
			 */
			if (sigrdataset != NULL) {
				rdataset->ttl = ISC_MIN(rdataset->ttl,
							sigrdataset->ttl);
				sigrdataset->ttl = rdataset->ttl;
			}

			/*
			 * Cache this rdataset/sigrdataset pair as
			 * pending data.
			 */
			rdataset->trust = dns_trust_pending;
			if (sigrdataset != NULL)
				sigrdataset->trust = dns_trust_pending;
			if (!need_validation || !ANSWER(rdataset)) {
				addedrdataset = ardataset;
				result = dns_db_addrdataset(fctx->cache, node,
							    NULL, now, rdataset,
							    0, addedrdataset);
				if (result == DNS_R_UNCHANGED) {
					result = ISC_R_SUCCESS;
					if (!need_validation &&
					    ardataset != NULL &&
					    ardataset->type == 0) {
						/*
						 * The answer in the cache is
						 * better than the answer we
						 * found, and is a negative
						 * cache entry, so we must set
						 * eresult appropriately.
						 */
						if (NXDOMAIN(ardataset))
							eresult =
							   DNS_R_NCACHENXDOMAIN;
						else
							eresult =
							   DNS_R_NCACHENXRRSET;
						/*
						 * We have a negative response
						 * from the cache so don't
						 * attempt to add the RRSIG
						 * rrset.
						 */
						continue;
					}
				}
				if (result != ISC_R_SUCCESS)
					break;
				if (sigrdataset != NULL) {
					addedrdataset = asigrdataset;
					result = dns_db_addrdataset(fctx->cache,
								node, NULL, now,
								sigrdataset, 0,
								addedrdataset);
					if (result == DNS_R_UNCHANGED)
						result = ISC_R_SUCCESS;
					if (result != ISC_R_SUCCESS)
						break;
				} else if (!ANSWER(rdataset))
					continue;
			}

			if (ANSWER(rdataset) && need_validation) {
				if (fctx->type != dns_rdatatype_any &&
				    fctx->type != dns_rdatatype_rrsig &&
				    fctx->type != dns_rdatatype_sig) {
					/*
					 * This is The Answer.  We will
					 * validate it, but first we cache
					 * the rest of the response - it may
					 * contain useful keys.
					 */
					INSIST(valrdataset == NULL &&
					       valsigrdataset == NULL);
					valrdataset = rdataset;
					valsigrdataset = sigrdataset;
				} else {
					/*
					 * This is one of (potentially)
					 * multiple answers to an ANY
					 * or SIG query.  To keep things
					 * simple, we just start the
					 * validator right away rather
					 * than caching first and
					 * having to remember which
					 * rdatasets needed validation.
					 */
					result = valcreate(fctx, addrinfo,
							   name, rdataset->type,
							   rdataset,
							   sigrdataset,
							   valoptions, task);
					/*
					 * Defer any further validations.
					 * This prevents multiple validators
					 * from manipulating fctx->rmessage
					 * simultaneously.
					 */
					valoptions |= DNS_VALIDATOR_DEFER;
				}
			} else if (CHAINING(rdataset)) {
				if (rdataset->type == dns_rdatatype_cname)
					eresult = DNS_R_CNAME;
				else {
					INSIST(rdataset->type ==
					       dns_rdatatype_dname);
					eresult = DNS_R_DNAME;
				}
			}
		} else if (!EXTERNAL(rdataset)) {
			/*
			 * It's OK to cache this rdataset now.
			 */
			if (ANSWER(rdataset))
				addedrdataset = ardataset;
			else if (ANSWERSIG(rdataset))
				addedrdataset = asigrdataset;
			else
				addedrdataset = NULL;
			if (CHAINING(rdataset)) {
				if (rdataset->type == dns_rdatatype_cname)
					eresult = DNS_R_CNAME;
				else {
					INSIST(rdataset->type ==
					       dns_rdatatype_dname);
					eresult = DNS_R_DNAME;
				}
			}
			if (rdataset->trust == dns_trust_glue &&
			    (rdataset->type == dns_rdatatype_ns ||
			     (rdataset->type == dns_rdatatype_rrsig &&
			      rdataset->covers == dns_rdatatype_ns))) {
				/*
				 * If the trust level is 'dns_trust_glue'
				 * then we are adding data from a referral
				 * we got while executing the search algorithm.
				 * New referral data always takes precedence
				 * over the existing cache contents.
				 */
				options = DNS_DBADD_FORCE;
			} else
				options = 0;
			/*
			 * Now we can add the rdataset.
			 */
			result = dns_db_addrdataset(fctx->cache,
						    node, NULL, now,
						    rdataset,
						    options,
						    addedrdataset);
			if (result == DNS_R_UNCHANGED) {
				if (ANSWER(rdataset) &&
				    ardataset != NULL &&
				    ardataset->type == 0) {
					/*
					 * The answer in the cache is better
					 * than the answer we found, and is
					 * a negative cache entry, so we
					 * must set eresult appropriately.
					 */
					if (NXDOMAIN(ardataset))
						eresult = DNS_R_NCACHENXDOMAIN;
					else
						eresult = DNS_R_NCACHENXRRSET;
				}
				result = ISC_R_SUCCESS;
			} else if (result != ISC_R_SUCCESS)
				break;
		}
	}

	if (valrdataset != NULL)
		result = valcreate(fctx, addrinfo, name, fctx->type,
				   valrdataset, valsigrdataset, valoptions,
				   task);

	if (result == ISC_R_SUCCESS && have_answer) {
		fctx->attributes |= FCTX_ATTR_HAVEANSWER;
		if (event != NULL) {
			/*
			 * Negative results must be indicated in event->result.
			 */
			if (dns_rdataset_isassociated(event->rdataset) &&
			    event->rdataset->type == dns_rdatatype_none) {
				INSIST(eresult == DNS_R_NCACHENXDOMAIN ||
				       eresult == DNS_R_NCACHENXRRSET);
			}
			event->result = eresult;
			dns_db_attach(fctx->cache, adbp);
			dns_db_transfernode(fctx->cache, &node, anodep);
			clone_results(fctx);
		}
	}

	if (node != NULL)
		dns_db_detachnode(fctx->cache, &node);

	return (result);
}

static inline isc_result_t
cache_message(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo, isc_stdtime_t now)
{
	isc_result_t result;
	dns_section_t section;
	dns_name_t *name;

	FCTXTRACE("cache_message");

	fctx->attributes &= ~FCTX_ATTR_WANTCACHE;

	LOCK(&fctx->res->buckets[fctx->bucketnum].lock);

	for (section = DNS_SECTION_ANSWER;
	     section <= DNS_SECTION_ADDITIONAL;
	     section++) {
		result = dns_message_firstname(fctx->rmessage, section);
		while (result == ISC_R_SUCCESS) {
			name = NULL;
			dns_message_currentname(fctx->rmessage, section,
						&name);
			if ((name->attributes & DNS_NAMEATTR_CACHE) != 0) {
				result = cache_name(fctx, name, addrinfo, now);
				if (result != ISC_R_SUCCESS)
					break;
			}
			result = dns_message_nextname(fctx->rmessage, section);
		}
		if (result != ISC_R_NOMORE)
			break;
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

	UNLOCK(&fctx->res->buckets[fctx->bucketnum].lock);

	return (result);
}

/*
 * Do what dns_ncache_addoptout() does, and then compute an appropriate eresult.
 */
static isc_result_t
ncache_adderesult(dns_message_t *message, dns_db_t *cache, dns_dbnode_t *node,
		  dns_rdatatype_t covers, isc_stdtime_t now, dns_ttl_t maxttl,
		  isc_boolean_t optout, dns_rdataset_t *ardataset,
		  isc_result_t *eresultp)
{
	isc_result_t result;
	dns_rdataset_t rdataset;

	if (ardataset == NULL) {
		dns_rdataset_init(&rdataset);
		ardataset = &rdataset;
	}
	result = dns_ncache_addoptout(message, cache, node, covers, now,
				     maxttl, optout, ardataset);
	if (result == DNS_R_UNCHANGED || result == ISC_R_SUCCESS) {
		/*
		 * If the cache now contains a negative entry and we
		 * care about whether it is DNS_R_NCACHENXDOMAIN or
		 * DNS_R_NCACHENXRRSET then extract it.
		 */
		if (ardataset->type == 0) {
			/*
			 * The cache data is a negative cache entry.
			 */
			if (NXDOMAIN(ardataset))
				*eresultp = DNS_R_NCACHENXDOMAIN;
			else
				*eresultp = DNS_R_NCACHENXRRSET;
		} else {
			/*
			 * Either we don't care about the nature of the
			 * cache rdataset (because no fetch is interested
			 * in the outcome), or the cache rdataset is not
			 * a negative cache entry.  Whichever case it is,
			 * we can return success.
			 *
			 * XXXRTH  There's a CNAME/DNAME problem here.
			 */
			*eresultp = ISC_R_SUCCESS;
		}
		result = ISC_R_SUCCESS;
	}
	if (ardataset == &rdataset && dns_rdataset_isassociated(ardataset))
		dns_rdataset_disassociate(ardataset);

	return (result);
}

static inline isc_result_t
ncache_message(fetchctx_t *fctx, dns_adbaddrinfo_t *addrinfo,
	       dns_rdatatype_t covers, isc_stdtime_t now)
{
	isc_result_t result, eresult;
	dns_name_t *name;
	dns_resolver_t *res;
	dns_db_t **adbp;
	dns_dbnode_t *node, **anodep;
	dns_rdataset_t *ardataset;
	isc_boolean_t need_validation, secure_domain;
	dns_name_t *aname;
	dns_fetchevent_t *event;
	isc_uint32_t ttl;
	unsigned int valoptions = 0;

	FCTXTRACE("ncache_message");

	fctx->attributes &= ~FCTX_ATTR_WANTNCACHE;

	res = fctx->res;
	need_validation = ISC_FALSE;
	secure_domain = ISC_FALSE;
	eresult = ISC_R_SUCCESS;
	name = &fctx->name;
	node = NULL;

	/*
	 * XXXMPA remove when we follow cnames and adjust the setting
	 * of FCTX_ATTR_WANTNCACHE in noanswer_response().
	 */
	INSIST(fctx->rmessage->counts[DNS_SECTION_ANSWER] == 0);

	/*
	 * Is DNSSEC validation required for this name?
	 */
	if (fctx->res->view->enablevalidation) {
		result = dns_keytable_issecuredomain(res->view->secroots, name,
						     &secure_domain);
		if (result != ISC_R_SUCCESS)
			return (result);

		if (!secure_domain && res->view->dlv != NULL) {
			valoptions = DNS_VALIDATOR_DLV;
			secure_domain = ISC_TRUE;
		}
	}

	if ((fctx->options & DNS_FETCHOPT_NOVALIDATE) != 0)
		need_validation = ISC_FALSE;
	else
		need_validation = secure_domain;

	if (secure_domain) {
		/*
		 * Mark all rdatasets as pending.
		 */
		dns_rdataset_t *trdataset;
		dns_name_t *tname;

		result = dns_message_firstname(fctx->rmessage,
					       DNS_SECTION_AUTHORITY);
		while (result == ISC_R_SUCCESS) {
			tname = NULL;
			dns_message_currentname(fctx->rmessage,
						DNS_SECTION_AUTHORITY,
						&tname);
			for (trdataset = ISC_LIST_HEAD(tname->list);
			     trdataset != NULL;
			     trdataset = ISC_LIST_NEXT(trdataset, link))
				trdataset->trust = dns_trust_pending;
			result = dns_message_nextname(fctx->rmessage,
						      DNS_SECTION_AUTHORITY);
		}
		if (result != ISC_R_NOMORE)
			return (result);

	}

	if (need_validation) {
		/*
		 * Do negative response validation.
		 */
		result = valcreate(fctx, addrinfo, name, fctx->type,
				   NULL, NULL, valoptions,
				   res->buckets[fctx->bucketnum].task);
		/*
		 * If validation is necessary, return now.  Otherwise continue
		 * to process the message, letting the validation complete
		 * in its own good time.
		 */
		return (result);
	}

	LOCK(&res->buckets[fctx->bucketnum].lock);

	adbp = NULL;
	aname = NULL;
	anodep = NULL;
	ardataset = NULL;
	if (!HAVE_ANSWER(fctx)) {
		event = ISC_LIST_HEAD(fctx->events);
		if (event != NULL) {
			adbp = &event->db;
			aname = dns_fixedname_name(&event->foundname);
			result = dns_name_copy(name, aname, NULL);
			if (result != ISC_R_SUCCESS)
				goto unlock;
			anodep = &event->node;
			ardataset = event->rdataset;
		}
	} else
		event = NULL;

	result = dns_db_findnode(fctx->cache, name, ISC_TRUE, &node);
	if (result != ISC_R_SUCCESS)
		goto unlock;

	/*
	 * If we are asking for a SOA record set the cache time
	 * to zero to facilitate locating the containing zone of
	 * a arbitrary zone.
	 */
	ttl = fctx->res->view->maxncachettl;
	if (fctx->type == dns_rdatatype_soa &&
	    covers == dns_rdatatype_any &&
	    fctx->res->zero_no_soa_ttl)
		ttl = 0;

	result = ncache_adderesult(fctx->rmessage, fctx->cache, node,
				   covers, now, ttl, ISC_FALSE,
				   ardataset, &eresult);
	if (result != ISC_R_SUCCESS)
		goto unlock;

	if (!HAVE_ANSWER(fctx)) {
		fctx->attributes |= FCTX_ATTR_HAVEANSWER;
		if (event != NULL) {
			event->result = eresult;
			dns_db_attach(fctx->cache, adbp);
			dns_db_transfernode(fctx->cache, &node, anodep);
			clone_results(fctx);
		}
	}

 unlock:
	UNLOCK(&res->buckets[fctx->bucketnum].lock);

	if (node != NULL)
		dns_db_detachnode(fctx->cache, &node);

	return (result);
}

static inline void
mark_related(dns_name_t *name, dns_rdataset_t *rdataset,
	     isc_boolean_t external, isc_boolean_t gluing)
{
	name->attributes |= DNS_NAMEATTR_CACHE;
	if (gluing) {
		rdataset->trust = dns_trust_glue;
		/*
		 * Glue with 0 TTL causes problems.  We force the TTL to
		 * 1 second to prevent this.
		 */
		if (rdataset->ttl == 0)
			rdataset->ttl = 1;
	} else
		rdataset->trust = dns_trust_additional;
	/*
	 * Avoid infinite loops by only marking new rdatasets.
	 */
	if (!CACHE(rdataset)) {
		name->attributes |= DNS_NAMEATTR_CHASE;
		rdataset->attributes |= DNS_RDATASETATTR_CHASE;
	}
	rdataset->attributes |= DNS_RDATASETATTR_CACHE;
	if (external)
		rdataset->attributes |= DNS_RDATASETATTR_EXTERNAL;
}

static isc_result_t
check_related(void *arg, dns_name_t *addname, dns_rdatatype_t type) {
	fetchctx_t *fctx = arg;
	isc_result_t result;
	dns_name_t *name;
	dns_rdataset_t *rdataset;
	isc_boolean_t external;
	dns_rdatatype_t rtype;
	isc_boolean_t gluing;

	REQUIRE(VALID_FCTX(fctx));

	if (GLUING(fctx))
		gluing = ISC_TRUE;
	else
		gluing = ISC_FALSE;
	name = NULL;
	rdataset = NULL;
	result = dns_message_findname(fctx->rmessage, DNS_SECTION_ADDITIONAL,
				      addname, dns_rdatatype_any, 0, &name,
				      NULL);
	if (result == ISC_R_SUCCESS) {
		external = ISC_TF(!dns_name_issubdomain(name, &fctx->domain));
		if (type == dns_rdatatype_a) {
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				if (rdataset->type == dns_rdatatype_rrsig)
					rtype = rdataset->covers;
				else
					rtype = rdataset->type;
				if (rtype == dns_rdatatype_a ||
				    rtype == dns_rdatatype_aaaa)
					mark_related(name, rdataset, external,
						     gluing);
			}
		} else {
			result = dns_message_findtype(name, type, 0,
						      &rdataset);
			if (result == ISC_R_SUCCESS) {
				mark_related(name, rdataset, external, gluing);
				/*
				 * Do we have its SIG too?
				 */
				rdataset = NULL;
				result = dns_message_findtype(name,
						      dns_rdatatype_rrsig,
						      type, &rdataset);
				if (result == ISC_R_SUCCESS)
					mark_related(name, rdataset, external,
						     gluing);
			}
		}
	}

	return (ISC_R_SUCCESS);
}

static void
chase_additional(fetchctx_t *fctx) {
	isc_boolean_t rescan;
	dns_section_t section = DNS_SECTION_ADDITIONAL;
	isc_result_t result;

 again:
	rescan = ISC_FALSE;

	for (result = dns_message_firstname(fctx->rmessage, section);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(fctx->rmessage, section)) {
		dns_name_t *name = NULL;
		dns_rdataset_t *rdataset;
		dns_message_currentname(fctx->rmessage, DNS_SECTION_ADDITIONAL,
					&name);
		if ((name->attributes & DNS_NAMEATTR_CHASE) == 0)
			continue;
		name->attributes &= ~DNS_NAMEATTR_CHASE;
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			if (CHASE(rdataset)) {
				rdataset->attributes &= ~DNS_RDATASETATTR_CHASE;
				(void)dns_rdataset_additionaldata(rdataset,
								  check_related,
								  fctx);
				rescan = ISC_TRUE;
			}
		}
	}
	if (rescan)
		goto again;
}

static inline isc_result_t
cname_target(dns_rdataset_t *rdataset, dns_name_t *tname) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_cname_t cname;

	result = dns_rdataset_first(rdataset);
	if (result != ISC_R_SUCCESS)
		return (result);
	dns_rdataset_current(rdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &cname, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);
	dns_name_init(tname, NULL);
	dns_name_clone(&cname.cname, tname);
	dns_rdata_freestruct(&cname);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
dname_target(dns_rdataset_t *rdataset, dns_name_t *qname, dns_name_t *oname,
	     dns_fixedname_t *fixeddname)
{
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned int nlabels;
	int order;
	dns_namereln_t namereln;
	dns_rdata_dname_t dname;
	dns_fixedname_t prefix;

	/*
	 * Get the target name of the DNAME.
	 */

	result = dns_rdataset_first(rdataset);
	if (result != ISC_R_SUCCESS)
		return (result);
	dns_rdataset_current(rdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &dname, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * Get the prefix of qname.
	 */
	namereln = dns_name_fullcompare(qname, oname, &order, &nlabels);
	if (namereln != dns_namereln_subdomain) {
		dns_rdata_freestruct(&dname);
		return (DNS_R_FORMERR);
	}
	dns_fixedname_init(&prefix);
	dns_name_split(qname, nlabels, dns_fixedname_name(&prefix), NULL);
	dns_fixedname_init(fixeddname);
	result = dns_name_concatenate(dns_fixedname_name(&prefix),
				      &dname.dname,
				      dns_fixedname_name(fixeddname), NULL);
	dns_rdata_freestruct(&dname);
	return (result);
}

/*
 * Handle a no-answer response (NXDOMAIN, NXRRSET, or referral).
 * If bind8_ns_resp is ISC_TRUE, this is a suspected BIND 8
 * response to an NS query that should be treated as a referral
 * even though the NS records occur in the answer section
 * rather than the authority section.
 */
static isc_result_t
noanswer_response(fetchctx_t *fctx, dns_name_t *oqname,
		  isc_boolean_t bind8_ns_resp)
{
	isc_result_t result;
	dns_message_t *message;
	dns_name_t *name, *qname, *ns_name, *soa_name, *ds_name;
	dns_rdataset_t *rdataset, *ns_rdataset;
	isc_boolean_t aa, negative_response;
	dns_rdatatype_t type;
	dns_section_t section =
		bind8_ns_resp ? DNS_SECTION_ANSWER : DNS_SECTION_AUTHORITY;

	FCTXTRACE("noanswer_response");

	message = fctx->rmessage;

	/*
	 * Setup qname.
	 */
	if (oqname == NULL) {
		/*
		 * We have a normal, non-chained negative response or
		 * referral.
		 */
		if ((message->flags & DNS_MESSAGEFLAG_AA) != 0)
			aa = ISC_TRUE;
		else
			aa = ISC_FALSE;
		qname = &fctx->name;
	} else {
		/*
		 * We're being invoked by answer_response() after it has
		 * followed a CNAME/DNAME chain.
		 */
		qname = oqname;
		aa = ISC_FALSE;
		/*
		 * If the current qname is not a subdomain of the query
		 * domain, there's no point in looking at the authority
		 * section without doing DNSSEC validation.
		 *
		 * Until we do that validation, we'll just return success
		 * in this case.
		 */
		if (!dns_name_issubdomain(qname, &fctx->domain))
			return (ISC_R_SUCCESS);
	}

	/*
	 * We have to figure out if this is a negative response, or a
	 * referral.
	 */

	/*
	 * Sometimes we can tell if its a negative response by looking at
	 * the message header.
	 */
	negative_response = ISC_FALSE;
	if (message->rcode == dns_rcode_nxdomain ||
	    (message->counts[DNS_SECTION_ANSWER] == 0 &&
	     message->counts[DNS_SECTION_AUTHORITY] == 0))
		negative_response = ISC_TRUE;

	/*
	 * Process the authority section.
	 */
	ns_name = NULL;
	ns_rdataset = NULL;
	soa_name = NULL;
	ds_name = NULL;
	result = dns_message_firstname(message, section);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, section, &name);
		if (dns_name_issubdomain(name, &fctx->domain)) {
			/*
			 * Look for NS/SOA RRsets first.
			 */
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				type = rdataset->type;
				if (type == dns_rdatatype_rrsig)
					type = rdataset->covers;
				if (((type == dns_rdatatype_ns ||
				      type == dns_rdatatype_soa) &&
				     !dns_name_issubdomain(qname, name)))
					return (DNS_R_FORMERR);
				if (type == dns_rdatatype_ns) {
					/*
					 * NS or RRSIG NS.
					 *
					 * Only one set of NS RRs is allowed.
					 */
					if (rdataset->type ==
					    dns_rdatatype_ns) {
						if (ns_name != NULL &&
						    name != ns_name)
							return (DNS_R_FORMERR);
						ns_name = name;
						ns_rdataset = rdataset;
					}
					name->attributes |=
						DNS_NAMEATTR_CACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_CACHE;
					rdataset->trust = dns_trust_glue;
				}
				if (type == dns_rdatatype_soa) {
					/*
					 * SOA, or RRSIG SOA.
					 *
					 * Only one SOA is allowed.
					 */
					if (rdataset->type ==
					    dns_rdatatype_soa) {
						if (soa_name != NULL &&
						    name != soa_name)
							return (DNS_R_FORMERR);
						soa_name = name;
					}
					name->attributes |=
						DNS_NAMEATTR_NCACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_NCACHE;
					if (aa)
						rdataset->trust =
						    dns_trust_authauthority;
					else
						rdataset->trust =
							dns_trust_additional;
				}
			}
		}
		result = dns_message_nextname(message, section);
		if (result == ISC_R_NOMORE)
			break;
		else if (result != ISC_R_SUCCESS)
			return (result);
	}

	/*
	 * A negative response has a SOA record (Type 2)
	 * and a optional NS RRset (Type 1) or it has neither
	 * a SOA or a NS RRset (Type 3, handled above) or
	 * rcode is NXDOMAIN (handled above) in which case
	 * the NS RRset is allowed (Type 4).
	 */
	if (soa_name != NULL)
		negative_response = ISC_TRUE;

	result = dns_message_firstname(message, section);
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, section, &name);
		if (dns_name_issubdomain(name, &fctx->domain)) {
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				type = rdataset->type;
				if (type == dns_rdatatype_rrsig)
					type = rdataset->covers;
				if (type == dns_rdatatype_nsec ||
				    type == dns_rdatatype_nsec3) {
					/*
					 * NSEC or RRSIG NSEC.
					 */
					if (negative_response) {
						name->attributes |=
							DNS_NAMEATTR_NCACHE;
						rdataset->attributes |=
							DNS_RDATASETATTR_NCACHE;
					} else if (type == dns_rdatatype_nsec) {
						name->attributes |=
							DNS_NAMEATTR_CACHE;
						rdataset->attributes |=
							DNS_RDATASETATTR_CACHE;
					}
					if (aa)
						rdataset->trust =
						    dns_trust_authauthority;
					else
						rdataset->trust =
							dns_trust_additional;
					/*
					 * No additional data needs to be
					 * marked.
					 */
				} else if (type == dns_rdatatype_ds) {
					/*
					 * DS or SIG DS.
					 *
					 * These should only be here if
					 * this is a referral, and there
					 * should only be one DS.
					 */
					if (ns_name == NULL)
						return (DNS_R_FORMERR);
					if (rdataset->type ==
					    dns_rdatatype_ds) {
						if (ds_name != NULL &&
						    name != ds_name)
							return (DNS_R_FORMERR);
						ds_name = name;
					}
					name->attributes |=
						DNS_NAMEATTR_CACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_CACHE;
					if (aa)
						rdataset->trust =
						    dns_trust_authauthority;
					else
						rdataset->trust =
							dns_trust_additional;
				}
			}
		}
		result = dns_message_nextname(message, section);
		if (result == ISC_R_NOMORE)
			break;
		else if (result != ISC_R_SUCCESS)
			return (result);
	}

	/*
	 * Trigger lookups for DNS nameservers.
	 */
	if (negative_response && message->rcode == dns_rcode_noerror &&
	    fctx->type == dns_rdatatype_ds && soa_name != NULL &&
	    dns_name_equal(soa_name, qname) &&
	    !dns_name_equal(qname, dns_rootname))
		return (DNS_R_CHASEDSSERVERS);

	/*
	 * Did we find anything?
	 */
	if (!negative_response && ns_name == NULL) {
		/*
		 * Nope.
		 */
		if (oqname != NULL) {
			/*
			 * We've already got a partial CNAME/DNAME chain,
			 * and haven't found else anything useful here, but
			 * no error has occurred since we have an answer.
			 */
			return (ISC_R_SUCCESS);
		} else {
			/*
			 * The responder is insane.
			 */
			return (DNS_R_FORMERR);
		}
	}

	/*
	 * If we found both NS and SOA, they should be the same name.
	 */
	if (ns_name != NULL && soa_name != NULL && ns_name != soa_name)
		return (DNS_R_FORMERR);

	/*
	 * Do we have a referral?  (We only want to follow a referral if
	 * we're not following a chain.)
	 */
	if (!negative_response && ns_name != NULL && oqname == NULL) {
		/*
		 * We already know ns_name is a subdomain of fctx->domain.
		 * If ns_name is equal to fctx->domain, we're not making
		 * progress.  We return DNS_R_FORMERR so that we'll keep
		 * trying other servers.
		 */
		if (dns_name_equal(ns_name, &fctx->domain))
			return (DNS_R_FORMERR);

		/*
		 * If the referral name is not a parent of the query
		 * name, consider the responder insane.
		 */
		if (! dns_name_issubdomain(&fctx->name, ns_name)) {
			FCTXTRACE("referral to non-parent");
			return (DNS_R_FORMERR);
		}

		/*
		 * Mark any additional data related to this rdataset.
		 * It's important that we do this before we change the
		 * query domain.
		 */
		INSIST(ns_rdataset != NULL);
		fctx->attributes |= FCTX_ATTR_GLUING;
		(void)dns_rdataset_additionaldata(ns_rdataset, check_related,
						  fctx);
		fctx->attributes &= ~FCTX_ATTR_GLUING;
		/*
		 * NS rdatasets with 0 TTL cause problems.
		 * dns_view_findzonecut() will not find them when we
		 * try to follow the referral, and we'll SERVFAIL
		 * because the best nameservers are now above QDOMAIN.
		 * We force the TTL to 1 second to prevent this.
		 */
		if (ns_rdataset->ttl == 0)
			ns_rdataset->ttl = 1;
		/*
		 * Set the current query domain to the referral name.
		 *
		 * XXXRTH  We should check if we're in forward-only mode, and
		 *		if so we should bail out.
		 */
		INSIST(dns_name_countlabels(&fctx->domain) > 0);
		dns_name_free(&fctx->domain,
			      fctx->res->buckets[fctx->bucketnum].mctx);
		if (dns_rdataset_isassociated(&fctx->nameservers))
			dns_rdataset_disassociate(&fctx->nameservers);
		dns_name_init(&fctx->domain, NULL);
		result = dns_name_dup(ns_name,
				      fctx->res->buckets[fctx->bucketnum].mctx,
				      &fctx->domain);
		if (result != ISC_R_SUCCESS)
			return (result);
		fctx->attributes |= FCTX_ATTR_WANTCACHE;
		return (DNS_R_DELEGATION);
	}

	/*
	 * Since we're not doing a referral, we don't want to cache any
	 * NS RRs we may have found.
	 */
	if (ns_name != NULL)
		ns_name->attributes &= ~DNS_NAMEATTR_CACHE;

	if (negative_response && oqname == NULL)
		fctx->attributes |= FCTX_ATTR_WANTNCACHE;

	return (ISC_R_SUCCESS);
}

static isc_result_t
answer_response(fetchctx_t *fctx) {
	isc_result_t result;
	dns_message_t *message;
	dns_name_t *name, *qname, tname;
	dns_rdataset_t *rdataset;
	isc_boolean_t done, external, chaining, aa, found, want_chaining;
	isc_boolean_t have_answer, found_cname, found_type, wanted_chaining;
	unsigned int aflag;
	dns_rdatatype_t type;
	dns_fixedname_t dname, fqname;

	FCTXTRACE("answer_response");

	message = fctx->rmessage;

	/*
	 * Examine the answer section, marking those rdatasets which are
	 * part of the answer and should be cached.
	 */

	done = ISC_FALSE;
	found_cname = ISC_FALSE;
	found_type = ISC_FALSE;
	chaining = ISC_FALSE;
	have_answer = ISC_FALSE;
	want_chaining = ISC_FALSE;
	if ((message->flags & DNS_MESSAGEFLAG_AA) != 0)
		aa = ISC_TRUE;
	else
		aa = ISC_FALSE;
	qname = &fctx->name;
	type = fctx->type;
	result = dns_message_firstname(message, DNS_SECTION_ANSWER);
	while (!done && result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_ANSWER, &name);
		external = ISC_TF(!dns_name_issubdomain(name, &fctx->domain));
		if (dns_name_equal(name, qname)) {
			wanted_chaining = ISC_FALSE;
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				found = ISC_FALSE;
				want_chaining = ISC_FALSE;
				aflag = 0;
				if (rdataset->type == dns_rdatatype_nsec3) {
					/*
					 * NSEC3 records are not allowed to
					 * appear in the answer section.
					 */
					return (DNS_R_FORMERR);
				}
				if (rdataset->type == type && !found_cname) {
					/*
					 * We've found an ordinary answer.
					 */
					found = ISC_TRUE;
					found_type = ISC_TRUE;
					done = ISC_TRUE;
					aflag = DNS_RDATASETATTR_ANSWER;
				} else if (type == dns_rdatatype_any) {
					/*
					 * We've found an answer matching
					 * an ANY query.  There may be
					 * more.
					 */
					found = ISC_TRUE;
					aflag = DNS_RDATASETATTR_ANSWER;
				} else if (rdataset->type == dns_rdatatype_rrsig
					   && rdataset->covers == type
					   && !found_cname) {
					/*
					 * We've found a signature that
					 * covers the type we're looking for.
					 */
					found = ISC_TRUE;
					found_type = ISC_TRUE;
					aflag = DNS_RDATASETATTR_ANSWERSIG;
				} else if (rdataset->type ==
					   dns_rdatatype_cname
					   && !found_type) {
					/*
					 * We're looking for something else,
					 * but we found a CNAME.
					 *
					 * Getting a CNAME response for some
					 * query types is an error.
					 */
					if (type == dns_rdatatype_rrsig ||
					    type == dns_rdatatype_dnskey ||
					    type == dns_rdatatype_nsec)
						return (DNS_R_FORMERR);
					found = ISC_TRUE;
					found_cname = ISC_TRUE;
					want_chaining = ISC_TRUE;
					aflag = DNS_RDATASETATTR_ANSWER;
					result = cname_target(rdataset,
							      &tname);
					if (result != ISC_R_SUCCESS)
						return (result);
				} else if (rdataset->type == dns_rdatatype_rrsig
					   && rdataset->covers ==
					   dns_rdatatype_cname
					   && !found_type) {
					/*
					 * We're looking for something else,
					 * but we found a SIG CNAME.
					 */
					found = ISC_TRUE;
					found_cname = ISC_TRUE;
					aflag = DNS_RDATASETATTR_ANSWERSIG;
				}

				if (found) {
					/*
					 * We've found an answer to our
					 * question.
					 */
					name->attributes |=
						DNS_NAMEATTR_CACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_CACHE;
					rdataset->trust = dns_trust_answer;
					if (!chaining) {
						/*
						 * This data is "the" answer
						 * to our question only if
						 * we're not chaining (i.e.
						 * if we haven't followed
						 * a CNAME or DNAME).
						 */
						INSIST(!external);
						if (aflag ==
						    DNS_RDATASETATTR_ANSWER)
							have_answer = ISC_TRUE;
						name->attributes |=
							DNS_NAMEATTR_ANSWER;
						rdataset->attributes |= aflag;
						if (aa)
							rdataset->trust =
							  dns_trust_authanswer;
					} else if (external) {
						/*
						 * This data is outside of
						 * our query domain, and
						 * may only be cached if it
						 * comes from a secure zone
						 * and validates.
						 */
						rdataset->attributes |=
						    DNS_RDATASETATTR_EXTERNAL;
					}

					/*
					 * Mark any additional data related
					 * to this rdataset.
					 */
					(void)dns_rdataset_additionaldata(
							rdataset,
							check_related,
							fctx);

					/*
					 * CNAME chaining.
					 */
					if (want_chaining) {
						wanted_chaining = ISC_TRUE;
						name->attributes |=
							DNS_NAMEATTR_CHAINING;
						rdataset->attributes |=
						    DNS_RDATASETATTR_CHAINING;
						qname = &tname;
					}
				}
				/*
				 * We could add an "else" clause here and
				 * log that we're ignoring this rdataset.
				 */
			}
			/*
			 * If wanted_chaining is true, we've done
			 * some chaining as the result of processing
			 * this node, and thus we need to set
			 * chaining to true.
			 *
			 * We don't set chaining inside of the
			 * rdataset loop because doing that would
			 * cause us to ignore the signatures of
			 * CNAMEs.
			 */
			if (wanted_chaining)
				chaining = ISC_TRUE;
		} else {
			/*
			 * Look for a DNAME (or its SIG).  Anything else is
			 * ignored.
			 */
			wanted_chaining = ISC_FALSE;
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				isc_boolean_t found_dname = ISC_FALSE;
				found = ISC_FALSE;
				aflag = 0;
				if (rdataset->type == dns_rdatatype_dname) {
					/*
					 * We're looking for something else,
					 * but we found a DNAME.
					 *
					 * If we're not chaining, then the
					 * DNAME should not be external.
					 */
					if (!chaining && external)
						return (DNS_R_FORMERR);
					found = ISC_TRUE;
					want_chaining = ISC_TRUE;
					aflag = DNS_RDATASETATTR_ANSWER;
					result = dname_target(rdataset,
							      qname, name,
							      &dname);
					if (result == ISC_R_NOSPACE) {
						/*
						 * We can't construct the
						 * DNAME target.  Do not
						 * try to continue.
						 */
						want_chaining = ISC_FALSE;
					} else if (result != ISC_R_SUCCESS)
						return (result);
					else
						found_dname = ISC_TRUE;
				} else if (rdataset->type == dns_rdatatype_rrsig
					   && rdataset->covers ==
					   dns_rdatatype_dname) {
					/*
					 * We've found a signature that
					 * covers the DNAME.
					 */
					found = ISC_TRUE;
					aflag = DNS_RDATASETATTR_ANSWERSIG;
				}

				if (found) {
					/*
					 * We've found an answer to our
					 * question.
					 */
					name->attributes |=
						DNS_NAMEATTR_CACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_CACHE;
					rdataset->trust = dns_trust_answer;
					if (!chaining) {
						/*
						 * This data is "the" answer
						 * to our question only if
						 * we're not chaining.
						 */
						INSIST(!external);
						if (aflag ==
						    DNS_RDATASETATTR_ANSWER)
							have_answer = ISC_TRUE;
						name->attributes |=
							DNS_NAMEATTR_ANSWER;
						rdataset->attributes |= aflag;
						if (aa)
							rdataset->trust =
							  dns_trust_authanswer;
					} else if (external) {
						rdataset->attributes |=
						    DNS_RDATASETATTR_EXTERNAL;
					}

					/*
					 * DNAME chaining.
					 */
					if (found_dname) {
						/*
						 * Copy the dname into the
						 * qname fixed name.
						 *
						 * Although we check for
						 * failure of the copy
						 * operation, in practice it
						 * should never fail since
						 * we already know that the
						 * result fits in a fixedname.
						 */
						dns_fixedname_init(&fqname);
						result = dns_name_copy(
						  dns_fixedname_name(&dname),
						  dns_fixedname_name(&fqname),
						  NULL);
						if (result != ISC_R_SUCCESS)
							return (result);
						wanted_chaining = ISC_TRUE;
						name->attributes |=
							DNS_NAMEATTR_CHAINING;
						rdataset->attributes |=
						    DNS_RDATASETATTR_CHAINING;
						qname = dns_fixedname_name(
								   &fqname);
					}
				}
			}
			if (wanted_chaining)
				chaining = ISC_TRUE;
		}
		result = dns_message_nextname(message, DNS_SECTION_ANSWER);
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * We should have found an answer.
	 */
	if (!have_answer)
		return (DNS_R_FORMERR);

	/*
	 * This response is now potentially cacheable.
	 */
	fctx->attributes |= FCTX_ATTR_WANTCACHE;

	/*
	 * Did chaining end before we got the final answer?
	 */
	if (chaining) {
		/*
		 * Yes.  This may be a negative reply, so hand off
		 * authority section processing to the noanswer code.
		 * If it isn't a noanswer response, no harm will be
		 * done.
		 */
		return (noanswer_response(fctx, qname, ISC_FALSE));
	}

	/*
	 * We didn't end with an incomplete chain, so the rcode should be
	 * "no error".
	 */
	if (message->rcode != dns_rcode_noerror)
		return (DNS_R_FORMERR);

	/*
	 * Examine the authority section (if there is one).
	 *
	 * We expect there to be only one owner name for all the rdatasets
	 * in this section, and we expect that it is not external.
	 */
	done = ISC_FALSE;
	result = dns_message_firstname(message, DNS_SECTION_AUTHORITY);
	while (!done && result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(message, DNS_SECTION_AUTHORITY, &name);
		external = ISC_TF(!dns_name_issubdomain(name, &fctx->domain));
		if (!external) {
			/*
			 * We expect to find NS or SIG NS rdatasets, and
			 * nothing else.
			 */
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				if (rdataset->type == dns_rdatatype_ns ||
				    (rdataset->type == dns_rdatatype_rrsig &&
				     rdataset->covers == dns_rdatatype_ns)) {
					name->attributes |=
						DNS_NAMEATTR_CACHE;
					rdataset->attributes |=
						DNS_RDATASETATTR_CACHE;
					if (aa && !chaining)
						rdataset->trust =
						    dns_trust_authauthority;
					else
						rdataset->trust =
						    dns_trust_additional;

					/*
					 * Mark any additional data related
					 * to this rdataset.
					 */
					(void)dns_rdataset_additionaldata(
							rdataset,
							check_related,
							fctx);
					done = ISC_TRUE;
				}
			}
		}
		result = dns_message_nextname(message, DNS_SECTION_AUTHORITY);
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

	return (result);
}

static void
resume_dslookup(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *fevent;
	dns_resolver_t *res;
	fetchctx_t *fctx;
	isc_result_t result;
	isc_boolean_t bucket_empty = ISC_FALSE;
	isc_boolean_t locked = ISC_FALSE;
	unsigned int bucketnum;
	dns_rdataset_t nameservers;
	dns_fixedname_t fixed;
	dns_name_t *domain;

	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	fevent = (dns_fetchevent_t *)event;
	fctx = event->ev_arg;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	UNUSED(task);
	FCTXTRACE("resume_dslookup");

	if (fevent->node != NULL)
		dns_db_detachnode(fevent->db, &fevent->node);
	if (fevent->db != NULL)
		dns_db_detach(&fevent->db);

	dns_rdataset_init(&nameservers);

	bucketnum = fctx->bucketnum;
	if (fevent->result == ISC_R_CANCELED) {
		dns_resolver_destroyfetch(&fctx->nsfetch);
		fctx_done(fctx, ISC_R_CANCELED, __LINE__);
	} else if (fevent->result == ISC_R_SUCCESS) {

		FCTXTRACE("resuming DS lookup");

		dns_resolver_destroyfetch(&fctx->nsfetch);
		if (dns_rdataset_isassociated(&fctx->nameservers))
			dns_rdataset_disassociate(&fctx->nameservers);
		dns_rdataset_clone(fevent->rdataset, &fctx->nameservers);
		dns_name_free(&fctx->domain,
			      fctx->res->buckets[bucketnum].mctx);
		dns_name_init(&fctx->domain, NULL);
		result = dns_name_dup(&fctx->nsname,
				      fctx->res->buckets[bucketnum].mctx,
				      &fctx->domain);
		if (result != ISC_R_SUCCESS) {
			fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
			goto cleanup;
		}
		/*
		 * Try again.
		 */
		fctx_try(fctx, ISC_TRUE);
	} else {
		unsigned int n;
		dns_rdataset_t *nsrdataset = NULL;

		/*
		 * Retrieve state from fctx->nsfetch before we destroy it.
		 */
		dns_fixedname_init(&fixed);
		domain = dns_fixedname_name(&fixed);
		dns_name_copy(&fctx->nsfetch->private->domain, domain, NULL);
		if (dns_name_equal(&fctx->nsname, domain)) {
			fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
			dns_resolver_destroyfetch(&fctx->nsfetch);
			goto cleanup;
		}
		if (dns_rdataset_isassociated(
		    &fctx->nsfetch->private->nameservers)) {
			dns_rdataset_clone(
			    &fctx->nsfetch->private->nameservers,
			    &nameservers);
			nsrdataset = &nameservers;
		} else
			domain = NULL;
		dns_resolver_destroyfetch(&fctx->nsfetch);
		n = dns_name_countlabels(&fctx->nsname);
		dns_name_getlabelsequence(&fctx->nsname, 1, n - 1,
					  &fctx->nsname);

		if (dns_rdataset_isassociated(fevent->rdataset))
			dns_rdataset_disassociate(fevent->rdataset);
		FCTXTRACE("continuing to look for parent's NS records");
		result = dns_resolver_createfetch(fctx->res, &fctx->nsname,
						  dns_rdatatype_ns, domain,
						  nsrdataset, NULL, 0, task,
						  resume_dslookup, fctx,
						  &fctx->nsrrset, NULL,
						  &fctx->nsfetch);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		else {
			LOCK(&res->buckets[bucketnum].lock);
			locked = ISC_TRUE;
			fctx->references++;
		}
	}

 cleanup:
	if (dns_rdataset_isassociated(&nameservers))
		dns_rdataset_disassociate(&nameservers);
	if (dns_rdataset_isassociated(fevent->rdataset))
		dns_rdataset_disassociate(fevent->rdataset);
	INSIST(fevent->sigrdataset == NULL);
	isc_event_free(&event);
	if (!locked)
		LOCK(&res->buckets[bucketnum].lock);
	fctx->references--;
	if (fctx->references == 0)
		bucket_empty = fctx_destroy(fctx);
	UNLOCK(&res->buckets[bucketnum].lock);
	if (bucket_empty)
		empty_bucket(res);
}

static inline void
checknamessection(dns_message_t *message, dns_section_t section) {
	isc_result_t result;
	dns_name_t *name;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdataset_t *rdataset;

	for (result = dns_message_firstname(message, section);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(message, section))
	{
		name = NULL;
		dns_message_currentname(message, section, &name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset)) {
				dns_rdataset_current(rdataset, &rdata);
				if (!dns_rdata_checkowner(name, rdata.rdclass,
							  rdata.type,
							  ISC_FALSE) ||
				    !dns_rdata_checknames(&rdata, name, NULL))
				{
					rdataset->attributes |=
						DNS_RDATASETATTR_CHECKNAMES;
				}
				dns_rdata_reset(&rdata);
			}
		}
	}
}

static void
checknames(dns_message_t *message) {

	checknamessection(message, DNS_SECTION_ANSWER);
	checknamessection(message, DNS_SECTION_AUTHORITY);
	checknamessection(message, DNS_SECTION_ADDITIONAL);
}

/*
 * Log server NSID at log level 'level'
 */
static isc_result_t
log_nsid(dns_rdataset_t *opt, resquery_t *query, int level, isc_mem_t *mctx)
{
	static const char hex[17] = "0123456789abcdef";
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];
	isc_uint16_t optcode, nsid_len, buflen, i;
	isc_result_t result;
	isc_buffer_t nsidbuf;
	dns_rdata_t rdata;
	unsigned char *p, *buf, *nsid;

	/* Extract rdata from OPT rdataset */
	result = dns_rdataset_first(opt);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_FAILURE);

	dns_rdata_init(&rdata);
	dns_rdataset_current(opt, &rdata);
	if (rdata.length < 4)
		return (ISC_R_FAILURE);

	/* Check for NSID */
	isc_buffer_init(&nsidbuf, rdata.data, rdata.length);
	isc_buffer_add(&nsidbuf, rdata.length);
	optcode = isc_buffer_getuint16(&nsidbuf);
	nsid_len = isc_buffer_getuint16(&nsidbuf);
	if (optcode != DNS_OPT_NSID || nsid_len == 0)
		return (ISC_R_FAILURE);

	/* Allocate buffer for storing hex version of the NSID */
	buflen = nsid_len * 2 + 1;
	buf = isc_mem_get(mctx, buflen);
	if (buf == NULL)
		return (ISC_R_NOSPACE);

	/* Convert to hex */
	p = buf;
	nsid = rdata.data + 4;
	for (i = 0; i < nsid_len; i++) {
		*p++ = hex[(nsid[0] >> 4) & 0xf];
		*p++ = hex[nsid[0] & 0xf];
		nsid++;
	}
	*p = '\0';

	isc_sockaddr_format(&query->addrinfo->sockaddr, addrbuf,
			    sizeof(addrbuf));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
		      DNS_LOGMODULE_RESOLVER, level,
		      "received NSID '%s' from %s", buf, addrbuf);

	/* Clean up */
	isc_mem_put(mctx, buf, buflen);
	return (ISC_R_SUCCESS);
}

static void
log_packet(dns_message_t *message, int level, isc_mem_t *mctx) {
	isc_buffer_t buffer;
	char *buf = NULL;
	int len = 1024;
	isc_result_t result;

	if (! isc_log_wouldlog(dns_lctx, level))
		return;

	/*
	 * Note that these are multiline debug messages.  We want a newline
	 * to appear in the log after each message.
	 */

	do {
		buf = isc_mem_get(mctx, len);
		if (buf == NULL)
			break;
		isc_buffer_init(&buffer, buf, len);
		result = dns_message_totext(message, &dns_master_style_debug,
					    0, &buffer);
		if (result == ISC_R_NOSPACE) {
			isc_mem_put(mctx, buf, len);
			len += 1024;
		} else if (result == ISC_R_SUCCESS)
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, level,
				      "received packet:\n%.*s",
				      (int)isc_buffer_usedlength(&buffer),
				      buf);
	} while (result == ISC_R_NOSPACE);

	if (buf != NULL)
		isc_mem_put(mctx, buf, len);
}

static void
resquery_response(isc_task_t *task, isc_event_t *event) {
	isc_result_t result = ISC_R_SUCCESS;
	resquery_t *query = event->ev_arg;
	dns_dispatchevent_t *devent = (dns_dispatchevent_t *)event;
	isc_boolean_t keep_trying, get_nameservers, resend;
	isc_boolean_t truncated;
	dns_message_t *message;
	dns_rdataset_t *opt;
	fetchctx_t *fctx;
	dns_name_t *fname;
	dns_fixedname_t foundname;
	isc_stdtime_t now;
	isc_time_t tnow, *finish;
	dns_adbaddrinfo_t *addrinfo;
	unsigned int options;
	unsigned int findoptions;
	isc_result_t broken_server;
	badnstype_t broken_type = badns_response;

	REQUIRE(VALID_QUERY(query));
	fctx = query->fctx;
	options = query->options;
	REQUIRE(VALID_FCTX(fctx));
	REQUIRE(event->ev_type == DNS_EVENT_DISPATCH);

	QTRACE("response");

	if (isc_sockaddr_pf(&query->addrinfo->sockaddr) == PF_INET)
		inc_stats(fctx->res, dns_resstatscounter_responsev4);
	else
		inc_stats(fctx->res, dns_resstatscounter_responsev6);

	(void)isc_timer_touch(fctx->timer);

	keep_trying = ISC_FALSE;
	broken_server = ISC_R_SUCCESS;
	get_nameservers = ISC_FALSE;
	resend = ISC_FALSE;
	truncated = ISC_FALSE;
	finish = NULL;

	if (fctx->res->exiting) {
		result = ISC_R_SHUTTINGDOWN;
		goto done;
	}

	fctx->timeouts = 0;
	fctx->timeout = ISC_FALSE;

	/*
	 * XXXRTH  We should really get the current time just once.  We
	 *		need a routine to convert from an isc_time_t to an
	 *		isc_stdtime_t.
	 */
	TIME_NOW(&tnow);
	finish = &tnow;
	isc_stdtime_get(&now);

	/*
	 * Did the dispatcher have a problem?
	 */
	if (devent->result != ISC_R_SUCCESS) {
		if (devent->result == ISC_R_EOF &&
		    (query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
			/*
			 * The problem might be that they
			 * don't understand EDNS0.  Turn it
			 * off and try again.
			 */
			options |= DNS_FETCHOPT_NOEDNS0;
			resend = ISC_TRUE;
			/*
			 * Remember that they don't like EDNS0.
			 */
			dns_adb_changeflags(fctx->adb,
					    query->addrinfo,
					    DNS_FETCHOPT_NOEDNS0,
					    DNS_FETCHOPT_NOEDNS0);
		} else {
			/*
			 * There's no hope for this query.
			 */
			keep_trying = ISC_TRUE;

			/*
			 * If this is a network error on an exclusive query
			 * socket, mark the server as bad so that we won't try
			 * it for this fetch again.
			 */
			if (query->exclusivesocket &&
			    (devent->result == ISC_R_HOSTUNREACH ||
			     devent->result == ISC_R_NETUNREACH ||
			     devent->result == ISC_R_CONNREFUSED ||
			     devent->result == ISC_R_CANCELED)) {
				    broken_server = devent->result;
				    broken_type = badns_unreachable;
			}
		}
		goto done;
	}

	message = fctx->rmessage;

	if (query->tsig != NULL) {
		result = dns_message_setquerytsig(message, query->tsig);
		if (result != ISC_R_SUCCESS)
			goto done;
	}

	if (query->tsigkey) {
		result = dns_message_settsigkey(message, query->tsigkey);
		if (result != ISC_R_SUCCESS)
			goto done;
	}

	result = dns_message_parse(message, &devent->buffer, 0);
	if (result != ISC_R_SUCCESS) {
		switch (result) {
		case ISC_R_UNEXPECTEDEND:
			if (!message->question_ok ||
			    (message->flags & DNS_MESSAGEFLAG_TC) == 0 ||
			    (options & DNS_FETCHOPT_TCP) != 0) {
				/*
				 * Either the message ended prematurely,
				 * and/or wasn't marked as being truncated,
				 * and/or this is a response to a query we
				 * sent over TCP.  In all of these cases,
				 * something is wrong with the remote
				 * server and we don't want to retry using
				 * TCP.
				 */
				if ((query->options & DNS_FETCHOPT_NOEDNS0)
				    == 0) {
					/*
					 * The problem might be that they
					 * don't understand EDNS0.  Turn it
					 * off and try again.
					 */
					options |= DNS_FETCHOPT_NOEDNS0;
					resend = ISC_TRUE;
					/*
					 * Remember that they don't like EDNS0.
					 */
					dns_adb_changeflags(
							fctx->adb,
							query->addrinfo,
							DNS_FETCHOPT_NOEDNS0,
							DNS_FETCHOPT_NOEDNS0);
					inc_stats(fctx->res,
						 dns_resstatscounter_edns0fail);
				} else {
					broken_server = result;
					keep_trying = ISC_TRUE;
				}
				goto done;
			}
			/*
			 * We defer retrying via TCP for a bit so we can
			 * check out this message further.
			 */
			truncated = ISC_TRUE;
			break;
		case DNS_R_FORMERR:
			if ((query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
				/*
				 * The problem might be that they
				 * don't understand EDNS0.  Turn it
				 * off and try again.
				 */
				options |= DNS_FETCHOPT_NOEDNS0;
				resend = ISC_TRUE;
				/*
				 * Remember that they don't like EDNS0.
				 */
				dns_adb_changeflags(fctx->adb,
						    query->addrinfo,
						    DNS_FETCHOPT_NOEDNS0,
						    DNS_FETCHOPT_NOEDNS0);
				inc_stats(fctx->res,
						 dns_resstatscounter_edns0fail);
			} else {
				broken_server = DNS_R_UNEXPECTEDRCODE;
				keep_trying = ISC_TRUE;
			}
			goto done;
		default:
			/*
			 * Something bad has happened.
			 */
			goto done;
		}
	}


	/*
	 * Log the incoming packet.
	 */
	log_packet(message, ISC_LOG_DEBUG(10), fctx->res->mctx);

	/*
	 * Did we request NSID?  If so, and if the response contains
	 * NSID data, log it at INFO level.
	 */
	opt = dns_message_getopt(message);
	if (opt != NULL && (query->options & DNS_FETCHOPT_WANTNSID) != 0)
		log_nsid(opt, query, ISC_LOG_INFO, fctx->res->mctx);

	/*
	 * If the message is signed, check the signature.  If not, this
	 * returns success anyway.
	 */
	result = dns_message_checksig(message, fctx->res->view);
	if (result != ISC_R_SUCCESS)
		goto done;

	/*
	 * The dispatcher should ensure we only get responses with QR set.
	 */
	INSIST((message->flags & DNS_MESSAGEFLAG_QR) != 0);
	/*
	 * INSIST() that the message comes from the place we sent it to,
	 * since the dispatch code should ensure this.
	 *
	 * INSIST() that the message id is correct (this should also be
	 * ensured by the dispatch code).
	 */


	/*
	 * Deal with truncated responses by retrying using TCP.
	 */
	if ((message->flags & DNS_MESSAGEFLAG_TC) != 0)
		truncated = ISC_TRUE;

	if (truncated) {
		inc_stats(fctx->res, dns_resstatscounter_truncated);
		if ((options & DNS_FETCHOPT_TCP) != 0) {
			broken_server = DNS_R_TRUNCATEDTCP;
			keep_trying = ISC_TRUE;
		} else {
			options |= DNS_FETCHOPT_TCP;
			resend = ISC_TRUE;
		}
		goto done;
	}

	/*
	 * Is it a query response?
	 */
	if (message->opcode != dns_opcode_query) {
		/* XXXRTH Log */
		broken_server = DNS_R_UNEXPECTEDOPCODE;
		keep_trying = ISC_TRUE;
		goto done;
	}

	/*
	 * Update statistics about erroneous responses.
	 */
	if (message->rcode != dns_rcode_noerror) {
		switch (message->rcode) {
		case dns_rcode_nxdomain:
			inc_stats(fctx->res, dns_resstatscounter_nxdomain);
			break;
		case dns_rcode_servfail:
			inc_stats(fctx->res, dns_resstatscounter_servfail);
			break;
		case dns_rcode_formerr:
			inc_stats(fctx->res, dns_resstatscounter_formerr);
			break;
		default:
			inc_stats(fctx->res, dns_resstatscounter_othererror);
			break;
		}
	}

	/*
	 * Is the remote server broken, or does it dislike us?
	 */
	if (message->rcode != dns_rcode_noerror &&
	    message->rcode != dns_rcode_nxdomain) {
		if (((message->rcode == dns_rcode_formerr ||
		     message->rcode == dns_rcode_notimp) ||
		    (message->rcode == dns_rcode_servfail &&
		     dns_message_getopt(message) == NULL)) &&
		    (query->options & DNS_FETCHOPT_NOEDNS0) == 0) {
			/*
			 * It's very likely they don't like EDNS0.
			 * If the response code is SERVFAIL, also check if the
			 * response contains an OPT RR and don't cache the
			 * failure since it can be returned for various other
			 * reasons.
			 *
			 * XXXRTH  We should check if the question
			 *		we're asking requires EDNS0, and
			 *		if so, we should bail out.
			 */
			options |= DNS_FETCHOPT_NOEDNS0;
			resend = ISC_TRUE;
			/*
			 * Remember that they don't like EDNS0.
			 */
			if (message->rcode != dns_rcode_servfail)
				dns_adb_changeflags(fctx->adb, query->addrinfo,
						    DNS_FETCHOPT_NOEDNS0,
						    DNS_FETCHOPT_NOEDNS0);
			inc_stats(fctx->res, dns_resstatscounter_edns0fail);
		} else if (message->rcode == dns_rcode_formerr) {
			if (ISFORWARDER(query->addrinfo)) {
				/*
				 * This forwarder doesn't understand us,
				 * but other forwarders might.  Keep trying.
				 */
				broken_server = DNS_R_REMOTEFORMERR;
				keep_trying = ISC_TRUE;
			} else {
				/*
				 * The server doesn't understand us.  Since
				 * all servers for a zone need similar
				 * capabilities, we assume that we will get
				 * FORMERR from all servers, and thus we
				 * cannot make any more progress with this
				 * fetch.
				 */
				result = DNS_R_FORMERR;
			}
		} else if (message->rcode == dns_rcode_yxdomain) {
			/*
			 * DNAME mapping failed because the new name
			 * was too long.  There's no chance of success
			 * for this fetch.
			 */
			result = DNS_R_YXDOMAIN;
		} else if (message->rcode == dns_rcode_badvers) {
			unsigned int flags, mask;
			unsigned int version;

			resend = ISC_TRUE;
			version = (opt->ttl >> 16) & 0xff;
			flags = (version << DNS_FETCHOPT_EDNSVERSIONSHIFT) |
				DNS_FETCHOPT_EDNSVERSIONSET;
			mask = DNS_FETCHOPT_EDNSVERSIONMASK |
			       DNS_FETCHOPT_EDNSVERSIONSET;
			switch (version) {
			case 0:
				dns_adb_changeflags(fctx->adb, query->addrinfo,
						    flags, mask);
				break;
			default:
				broken_server = DNS_R_BADVERS;
				keep_trying = ISC_TRUE;
				break;
			}
		} else {
			/*
			 * XXXRTH log.
			 */
			broken_server = DNS_R_UNEXPECTEDRCODE;
			INSIST(broken_server != ISC_R_SUCCESS);
			keep_trying = ISC_TRUE;
		}
		goto done;
	}

	/*
	 * Is the question the same as the one we asked?
	 */
	result = same_question(fctx);
	if (result != ISC_R_SUCCESS) {
		/* XXXRTH Log */
		if (result == DNS_R_FORMERR)
			keep_trying = ISC_TRUE;
		goto done;
	}

	/*
	 * Is the server lame?
	 */
	if (fctx->res->lame_ttl != 0 && !ISFORWARDER(query->addrinfo) &&
	    is_lame(fctx)) {
		inc_stats(fctx->res, dns_resstatscounter_lame);
		log_lame(fctx, query->addrinfo);
		result = dns_adb_marklame(fctx->adb, query->addrinfo,
					  &fctx->name, fctx->type,
					  now + fctx->res->lame_ttl);
		if (result != ISC_R_SUCCESS)
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
				      DNS_LOGMODULE_RESOLVER, ISC_LOG_ERROR,
				      "could not mark server as lame: %s",
				      isc_result_totext(result));
		broken_server = DNS_R_LAME;
		keep_trying = ISC_TRUE;
		goto done;
	}

	/*
	 * Enforce delegations only zones like NET and COM.
	 */
	if (!ISFORWARDER(query->addrinfo) &&
	    dns_view_isdelegationonly(fctx->res->view, &fctx->domain) &&
	    !dns_name_equal(&fctx->domain, &fctx->name) &&
	    fix_mustbedelegationornxdomain(message, fctx)) {
		char namebuf[DNS_NAME_FORMATSIZE];
		char domainbuf[DNS_NAME_FORMATSIZE];
		char addrbuf[ISC_SOCKADDR_FORMATSIZE];
		char classbuf[64];
		char typebuf[64];

		dns_name_format(&fctx->name, namebuf, sizeof(namebuf));
		dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
		dns_rdatatype_format(fctx->type, typebuf, sizeof(typebuf));
		dns_rdataclass_format(fctx->res->rdclass, classbuf,
				      sizeof(classbuf));
		isc_sockaddr_format(&query->addrinfo->sockaddr, addrbuf,
				    sizeof(addrbuf));

		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DELEGATION_ONLY,
			     DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
			     "enforced delegation-only for '%s' (%s/%s/%s) "
			     "from %s",
			     domainbuf, namebuf, typebuf, classbuf, addrbuf);
	}

	if ((fctx->res->options & DNS_RESOLVER_CHECKNAMES) != 0)
		checknames(message);

	/*
	 * Clear cache bits.
	 */
	fctx->attributes &= ~(FCTX_ATTR_WANTNCACHE | FCTX_ATTR_WANTCACHE);

	/*
	 * Did we get any answers?
	 */
	if (message->counts[DNS_SECTION_ANSWER] > 0 &&
	    (message->rcode == dns_rcode_noerror ||
	     message->rcode == dns_rcode_nxdomain)) {
		/*
		 * We've got answers.  However, if we sent
		 * a BIND 8 server an NS query, it may have
		 * incorrectly responded with a non-authoritative
		 * answer instead of a referral.  Since this
		 * answer lacks the SIGs necessary to do DNSSEC
		 * validation, we must invoke the following special
		 * kludge to treat it as a referral.
		 */
		if (fctx->type == dns_rdatatype_ns &&
		    (message->flags & DNS_MESSAGEFLAG_AA) == 0 &&
		    !ISFORWARDER(query->addrinfo))
		{
			result = noanswer_response(fctx, NULL, ISC_TRUE);
			if (result != DNS_R_DELEGATION) {
				/*
				 * The answer section must have contained
				 * something other than the NS records
				 * we asked for.  Since AA is not set
				 * and the server is not a forwarder,
				 * it is technically lame and it's easier
				 * to treat it as such than to figure out
				 * some more elaborate course of action.
				 */
				broken_server = DNS_R_LAME;
				keep_trying = ISC_TRUE;
				goto done;
			}
			goto force_referral;
		}
		result = answer_response(fctx);
		if (result != ISC_R_SUCCESS) {
			if (result == DNS_R_FORMERR)
				keep_trying = ISC_TRUE;
			goto done;
		}
	} else if (message->counts[DNS_SECTION_AUTHORITY] > 0 ||
		   message->rcode == dns_rcode_noerror ||
		   message->rcode == dns_rcode_nxdomain) {
		/*
		 * NXDOMAIN, NXRDATASET, or referral.
		 */
		result = noanswer_response(fctx, NULL, ISC_FALSE);
		if (result == DNS_R_CHASEDSSERVERS) {
		} else if (result == DNS_R_DELEGATION) {
		force_referral:
			/*
			 * We don't have the answer, but we know a better
			 * place to look.
			 */
			get_nameservers = ISC_TRUE;
			keep_trying = ISC_TRUE;
			/*
			 * We have a new set of name servers, and it
			 * has not experienced any restarts yet.
			 */
			fctx->restarts = 0;

			/*
			 * Update local statistics counters collected for each
			 * new zone.
			 */
			fctx->referrals++;
			fctx->querysent = 0;
			fctx->lamecount = 0;
			fctx->neterr = 0;
			fctx->badresp = 0;
			fctx->adberr = 0;

			result = ISC_R_SUCCESS;
		} else if (result != ISC_R_SUCCESS) {
			/*
			 * Something has gone wrong.
			 */
			if (result == DNS_R_FORMERR)
				keep_trying = ISC_TRUE;
			goto done;
		}
	} else {
		/*
		 * The server is insane.
		 */
		/* XXXRTH Log */
		broken_server = DNS_R_UNEXPECTEDRCODE;
		keep_trying = ISC_TRUE;
		goto done;
	}

	/*
	 * Follow additional section data chains.
	 */
	chase_additional(fctx);

	/*
	 * Cache the cacheable parts of the message.  This may also cause
	 * work to be queued to the DNSSEC validator.
	 */
	if (WANTCACHE(fctx)) {
		result = cache_message(fctx, query->addrinfo, now);
		if (result != ISC_R_SUCCESS)
			goto done;
	}

	/*
	 * Ncache the negatively cacheable parts of the message.  This may
	 * also cause work to be queued to the DNSSEC validator.
	 */
	if (WANTNCACHE(fctx)) {
		dns_rdatatype_t covers;
		if (message->rcode == dns_rcode_nxdomain)
			covers = dns_rdatatype_any;
		else
			covers = fctx->type;

		/*
		 * Cache any negative cache entries in the message.
		 */
		result = ncache_message(fctx, query->addrinfo, covers, now);
	}

 done:
	/*
	 * Remember the query's addrinfo, in case we need to mark the
	 * server as broken.
	 */
	addrinfo = query->addrinfo;

	/*
	 * Cancel the query.
	 *
	 * XXXRTH  Don't cancel the query if waiting for validation?
	 */
	fctx_cancelquery(&query, &devent, finish, ISC_FALSE);

	if (keep_trying) {
		if (result == DNS_R_FORMERR)
			broken_server = DNS_R_FORMERR;
		if (broken_server != ISC_R_SUCCESS) {
			/*
			 * Add this server to the list of bad servers for
			 * this fctx.
			 */
			add_bad(fctx, addrinfo, broken_server, broken_type);
		}

		if (get_nameservers) {
			dns_name_t *name;
			dns_fixedname_init(&foundname);
			fname = dns_fixedname_name(&foundname);
			if (result != ISC_R_SUCCESS) {
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}
			findoptions = 0;
			if (dns_rdatatype_atparent(fctx->type))
				findoptions |= DNS_DBFIND_NOEXACT;
			if ((options & DNS_FETCHOPT_UNSHARED) == 0)
				name = &fctx->name;
			else
				name = &fctx->domain;
			result = dns_view_findzonecut(fctx->res->view,
						      name, fname,
						      now, findoptions,
						      ISC_TRUE,
						      &fctx->nameservers,
						      NULL);
			if (result != ISC_R_SUCCESS) {
				FCTXTRACE("couldn't find a zonecut");
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}
			if (!dns_name_issubdomain(fname, &fctx->domain)) {
				/*
				 * The best nameservers are now above our
				 * QDOMAIN.
				 */
				FCTXTRACE("nameservers now above QDOMAIN");
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}
			dns_name_free(&fctx->domain,
				      fctx->res->buckets[fctx->bucketnum].mctx);
			dns_name_init(&fctx->domain, NULL);
			result = dns_name_dup(fname,
					      fctx->res->buckets[fctx->bucketnum].mctx,
					      &fctx->domain);
			if (result != ISC_R_SUCCESS) {
				fctx_done(fctx, DNS_R_SERVFAIL, __LINE__);
				return;
			}
			fctx_cancelqueries(fctx, ISC_TRUE);
			fctx_cleanupfinds(fctx);
			fctx_cleanupaltfinds(fctx);
			fctx_cleanupforwaddrs(fctx);
			fctx_cleanupaltaddrs(fctx);
		}
		/*
		 * Try again.
		 */
		fctx_try(fctx, !get_nameservers);
	} else if (resend) {
		/*
		 * Resend (probably with changed options).
		 */
		FCTXTRACE("resend");
		inc_stats(fctx->res, dns_resstatscounter_retry);
		result = fctx_query(fctx, addrinfo, options);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
	} else if (result == ISC_R_SUCCESS && !HAVE_ANSWER(fctx)) {
		/*
		 * All has gone well so far, but we are waiting for the
		 * DNSSEC validator to validate the answer.
		 */
		FCTXTRACE("wait for validator");
		fctx_cancelqueries(fctx, ISC_TRUE);
		/*
		 * We must not retransmit while the validator is working;
		 * it has references to the current rmessage.
		 */
		result = fctx_stopidletimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
	} else if (result == DNS_R_CHASEDSSERVERS) {
		unsigned int n;
		add_bad(fctx, addrinfo, result, broken_type);
		fctx_cancelqueries(fctx, ISC_TRUE);
		fctx_cleanupfinds(fctx);
		fctx_cleanupforwaddrs(fctx);

		n = dns_name_countlabels(&fctx->name);
		dns_name_getlabelsequence(&fctx->name, 1, n - 1, &fctx->nsname);

		FCTXTRACE("suspending DS lookup to find parent's NS records");

		result = dns_resolver_createfetch(fctx->res, &fctx->nsname,
						  dns_rdatatype_ns,
						  NULL, NULL, NULL, 0, task,
						  resume_dslookup, fctx,
						  &fctx->nsrrset, NULL,
						  &fctx->nsfetch);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
		LOCK(&fctx->res->buckets[fctx->bucketnum].lock);
		fctx->references++;
		UNLOCK(&fctx->res->buckets[fctx->bucketnum].lock);
		result = fctx_stopidletimer(fctx);
		if (result != ISC_R_SUCCESS)
			fctx_done(fctx, result, __LINE__);
	} else {
		/*
		 * We're done.
		 */
		fctx_done(fctx, result, __LINE__);
	}
}


/***
 *** Resolver Methods
 ***/

static void
destroy(dns_resolver_t *res) {
	unsigned int i;
	alternate_t *a;

	REQUIRE(res->references == 0);
	REQUIRE(!res->priming);
	REQUIRE(res->primefetch == NULL);

	RTRACE("destroy");

	INSIST(res->nfctx == 0);

	DESTROYLOCK(&res->primelock);
	DESTROYLOCK(&res->nlock);
	DESTROYLOCK(&res->lock);
	for (i = 0; i < res->nbuckets; i++) {
		INSIST(ISC_LIST_EMPTY(res->buckets[i].fctxs));
		isc_task_shutdown(res->buckets[i].task);
		isc_task_detach(&res->buckets[i].task);
		DESTROYLOCK(&res->buckets[i].lock);
		isc_mem_detach(&res->buckets[i].mctx);
	}
	isc_mem_put(res->mctx, res->buckets,
		    res->nbuckets * sizeof(fctxbucket_t));
	if (res->dispatchv4 != NULL)
		dns_dispatch_detach(&res->dispatchv4);
	if (res->dispatchv6 != NULL)
		dns_dispatch_detach(&res->dispatchv6);
	while ((a = ISC_LIST_HEAD(res->alternates)) != NULL) {
		ISC_LIST_UNLINK(res->alternates, a, link);
		if (!a->isaddress)
			dns_name_free(&a->_u._n.name, res->mctx);
		isc_mem_put(res->mctx, a, sizeof(*a));
	}
	dns_resolver_reset_algorithms(res);
	dns_resolver_resetmustbesecure(res);
#if USE_ALGLOCK
	isc_rwlock_destroy(&res->alglock);
#endif
#if USE_MBSLOCK
	isc_rwlock_destroy(&res->mbslock);
#endif
	isc_timer_detach(&res->spillattimer);
	res->magic = 0;
	isc_mem_put(res->mctx, res, sizeof(*res));
}

static void
send_shutdown_events(dns_resolver_t *res) {
	isc_event_t *event, *next_event;
	isc_task_t *etask;

	/*
	 * Caller must be holding the resolver lock.
	 */

	for (event = ISC_LIST_HEAD(res->whenshutdown);
	     event != NULL;
	     event = next_event) {
		next_event = ISC_LIST_NEXT(event, ev_link);
		ISC_LIST_UNLINK(res->whenshutdown, event, ev_link);
		etask = event->ev_sender;
		event->ev_sender = res;
		isc_task_sendanddetach(&etask, &event);
	}
}

static void
empty_bucket(dns_resolver_t *res) {
	RTRACE("empty_bucket");

	LOCK(&res->lock);

	INSIST(res->activebuckets > 0);
	res->activebuckets--;
	if (res->activebuckets == 0)
		send_shutdown_events(res);

	UNLOCK(&res->lock);
}

static void
spillattimer_countdown(isc_task_t *task, isc_event_t *event) {
	dns_resolver_t *res = event->ev_arg;
	isc_result_t result;
	unsigned int count;
	isc_boolean_t logit = ISC_FALSE;

	REQUIRE(VALID_RESOLVER(res));

	UNUSED(task);

	LOCK(&res->lock);
	INSIST(!res->exiting);
	if (res->spillat > res->spillatmin) {
		res->spillat--;
		logit = ISC_TRUE;
	}
	if (res->spillat <= res->spillatmin) {
		result = isc_timer_reset(res->spillattimer,
					 isc_timertype_inactive, NULL,
					 NULL, ISC_TRUE);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}
	count = res->spillat;
	UNLOCK(&res->lock);
	if (logit)
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
			      DNS_LOGMODULE_RESOLVER, ISC_LOG_NOTICE,
			      "clients-per-query decreased to %u", count);

	isc_event_free(&event);
}

isc_result_t
dns_resolver_create(dns_view_t *view,
		    isc_taskmgr_t *taskmgr, unsigned int ntasks,
		    isc_socketmgr_t *socketmgr,
		    isc_timermgr_t *timermgr,
		    unsigned int options,
		    dns_dispatchmgr_t *dispatchmgr,
		    dns_dispatch_t *dispatchv4,
		    dns_dispatch_t *dispatchv6,
		    dns_resolver_t **resp)
{
	dns_resolver_t *res;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int i, buckets_created = 0;
	isc_task_t *task = NULL;
	char name[16];
	unsigned dispattr;

	/*
	 * Create a resolver.
	 */

	REQUIRE(DNS_VIEW_VALID(view));
	REQUIRE(ntasks > 0);
	REQUIRE(resp != NULL && *resp == NULL);
	REQUIRE(dispatchmgr != NULL);
	REQUIRE(dispatchv4 != NULL || dispatchv6 != NULL);

	res = isc_mem_get(view->mctx, sizeof(*res));
	if (res == NULL)
		return (ISC_R_NOMEMORY);
	RTRACE("create");
	res->mctx = view->mctx;
	res->rdclass = view->rdclass;
	res->socketmgr = socketmgr;
	res->timermgr = timermgr;
	res->taskmgr = taskmgr;
	res->dispatchmgr = dispatchmgr;
	res->view = view;
	res->options = options;
	res->lame_ttl = 0;
	ISC_LIST_INIT(res->alternates);
	res->udpsize = RECV_BUFFER_SIZE;
	res->algorithms = NULL;
	res->mustbesecure = NULL;
	res->spillatmin = res->spillat = 10;
	res->spillatmax = 100;
	res->spillattimer = NULL;
	res->zero_no_soa_ttl = ISC_FALSE;
	res->ndisps = 0;
	res->nextdisp = 0; /* meaningless at this point, but init it */
	res->nbuckets = ntasks;
	res->activebuckets = ntasks;
	res->buckets = isc_mem_get(view->mctx,
				   ntasks * sizeof(fctxbucket_t));
	if (res->buckets == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_res;
	}
	for (i = 0; i < ntasks; i++) {
		result = isc_mutex_init(&res->buckets[i].lock);
		if (result != ISC_R_SUCCESS)
			goto cleanup_buckets;
		res->buckets[i].task = NULL;
		result = isc_task_create(taskmgr, 0, &res->buckets[i].task);
		if (result != ISC_R_SUCCESS) {
			DESTROYLOCK(&res->buckets[i].lock);
			goto cleanup_buckets;
		}
		res->buckets[i].mctx = NULL;
		snprintf(name, sizeof(name), "res%u", i);
#ifdef ISC_PLATFORM_USETHREADS
		/*
		 * Use a separate memory context for each bucket to reduce
		 * contention among multiple threads.  Do this only when
		 * enabling threads because it will be require more memory.
		 */
		result = isc_mem_create(0, 0, &res->buckets[i].mctx);
		if (result != ISC_R_SUCCESS) {
			isc_task_detach(&res->buckets[i].task);
			DESTROYLOCK(&res->buckets[i].lock);
			goto cleanup_buckets;
		}
		isc_mem_setname(res->buckets[i].mctx, name, NULL);
#else
		isc_mem_attach(view->mctx, &res->buckets[i].mctx);
#endif
		isc_task_setname(res->buckets[i].task, name, res);
		ISC_LIST_INIT(res->buckets[i].fctxs);
		res->buckets[i].exiting = ISC_FALSE;
		buckets_created++;
	}

	res->dispatchv4 = NULL;
	if (dispatchv4 != NULL) {
		dns_dispatch_attach(dispatchv4, &res->dispatchv4);
		dispattr = dns_dispatch_getattributes(dispatchv4);
		res->exclusivev4 =
			ISC_TF((dispattr & DNS_DISPATCHATTR_EXCLUSIVE) != 0);
	}

	res->dispatchv6 = NULL;
	if (dispatchv6 != NULL) {
		dns_dispatch_attach(dispatchv6, &res->dispatchv6);
		dispattr = dns_dispatch_getattributes(dispatchv6);
		res->exclusivev6 =
			ISC_TF((dispattr & DNS_DISPATCHATTR_EXCLUSIVE) != 0);
	}

	res->references = 1;
	res->exiting = ISC_FALSE;
	res->frozen = ISC_FALSE;
	ISC_LIST_INIT(res->whenshutdown);
	res->priming = ISC_FALSE;
	res->primefetch = NULL;
	res->nfctx = 0;

	result = isc_mutex_init(&res->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_dispatches;

	result = isc_mutex_init(&res->nlock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_lock;

	result = isc_mutex_init(&res->primelock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_nlock;

	task = NULL;
	result = isc_task_create(taskmgr, 0, &task);
	if (result != ISC_R_SUCCESS)
		goto cleanup_primelock;

	result = isc_timer_create(timermgr, isc_timertype_inactive, NULL, NULL,
				  task, spillattimer_countdown, res,
				  &res->spillattimer);
	isc_task_detach(&task);
	if (result != ISC_R_SUCCESS)
		goto cleanup_primelock;

#if USE_ALGLOCK
	result = isc_rwlock_init(&res->alglock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_spillattimer;
#endif
#if USE_MBSLOCK
	result = isc_rwlock_init(&res->mbslock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_alglock;
#endif

	res->magic = RES_MAGIC;

	*resp = res;

	return (ISC_R_SUCCESS);

#if USE_MBSLOCK
 cleanup_alglock:
#if USE_ALGLOCK
	isc_rwlock_destroy(&res->alglock);
#endif
#endif
#if USE_ALGLOCK || USE_MBSLOCK
 cleanup_spillattimer:
	isc_timer_detach(&res->spillattimer);
#endif

 cleanup_primelock:
	DESTROYLOCK(&res->primelock);

 cleanup_nlock:
	DESTROYLOCK(&res->nlock);

 cleanup_lock:
	DESTROYLOCK(&res->lock);

 cleanup_dispatches:
	if (res->dispatchv6 != NULL)
		dns_dispatch_detach(&res->dispatchv6);
	if (res->dispatchv4 != NULL)
		dns_dispatch_detach(&res->dispatchv4);

 cleanup_buckets:
	for (i = 0; i < buckets_created; i++) {
		isc_mem_detach(&res->buckets[i].mctx);
		DESTROYLOCK(&res->buckets[i].lock);
		isc_task_shutdown(res->buckets[i].task);
		isc_task_detach(&res->buckets[i].task);
	}
	isc_mem_put(view->mctx, res->buckets,
		    res->nbuckets * sizeof(fctxbucket_t));

 cleanup_res:
	isc_mem_put(view->mctx, res, sizeof(*res));

	return (result);
}

static void
prime_done(isc_task_t *task, isc_event_t *event) {
	dns_resolver_t *res;
	dns_fetchevent_t *fevent;
	dns_fetch_t *fetch;
	dns_db_t *db = NULL;

	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	fevent = (dns_fetchevent_t *)event;
	res = event->ev_arg;
	REQUIRE(VALID_RESOLVER(res));

	UNUSED(task);

	LOCK(&res->lock);

	INSIST(res->priming);
	res->priming = ISC_FALSE;
	LOCK(&res->primelock);
	fetch = res->primefetch;
	res->primefetch = NULL;
	UNLOCK(&res->primelock);

	UNLOCK(&res->lock);

	if (fevent->result == ISC_R_SUCCESS &&
	    res->view->cache != NULL && res->view->hints != NULL) {
		dns_cache_attachdb(res->view->cache, &db);
		dns_root_checkhints(res->view, res->view->hints, db);
		dns_db_detach(&db);
	}

	if (fevent->node != NULL)
		dns_db_detachnode(fevent->db, &fevent->node);
	if (fevent->db != NULL)
		dns_db_detach(&fevent->db);
	if (dns_rdataset_isassociated(fevent->rdataset))
		dns_rdataset_disassociate(fevent->rdataset);
	INSIST(fevent->sigrdataset == NULL);

	isc_mem_put(res->mctx, fevent->rdataset, sizeof(*fevent->rdataset));

	isc_event_free(&event);
	dns_resolver_destroyfetch(&fetch);
}

void
dns_resolver_prime(dns_resolver_t *res) {
	isc_boolean_t want_priming = ISC_FALSE;
	dns_rdataset_t *rdataset;
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(res));
	REQUIRE(res->frozen);

	RTRACE("dns_resolver_prime");

	LOCK(&res->lock);

	if (!res->exiting && !res->priming) {
		INSIST(res->primefetch == NULL);
		res->priming = ISC_TRUE;
		want_priming = ISC_TRUE;
	}

	UNLOCK(&res->lock);

	if (want_priming) {
		/*
		 * To avoid any possible recursive locking problems, we
		 * start the priming fetch like any other fetch, and holding
		 * no resolver locks.  No one else will try to start it
		 * because we're the ones who set res->priming to true.
		 * Any other callers of dns_resolver_prime() while we're
		 * running will see that res->priming is already true and
		 * do nothing.
		 */
		RTRACE("priming");
		rdataset = isc_mem_get(res->mctx, sizeof(*rdataset));
		if (rdataset == NULL) {
			LOCK(&res->lock);
			INSIST(res->priming);
			INSIST(res->primefetch == NULL);
			res->priming = ISC_FALSE;
			UNLOCK(&res->lock);
			return;
		}
		dns_rdataset_init(rdataset);
		LOCK(&res->primelock);
		result = dns_resolver_createfetch(res, dns_rootname,
						  dns_rdatatype_ns,
						  NULL, NULL, NULL, 0,
						  res->buckets[0].task,
						  prime_done,
						  res, rdataset, NULL,
						  &res->primefetch);
		UNLOCK(&res->primelock);
		if (result != ISC_R_SUCCESS) {
			LOCK(&res->lock);
			INSIST(res->priming);
			res->priming = ISC_FALSE;
			UNLOCK(&res->lock);
		}
	}
}

void
dns_resolver_freeze(dns_resolver_t *res) {

	/*
	 * Freeze resolver.
	 */

	REQUIRE(VALID_RESOLVER(res));
	REQUIRE(!res->frozen);

	res->frozen = ISC_TRUE;
}

void
dns_resolver_attach(dns_resolver_t *source, dns_resolver_t **targetp) {
	REQUIRE(VALID_RESOLVER(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	RRTRACE(source, "attach");
	LOCK(&source->lock);
	REQUIRE(!source->exiting);

	INSIST(source->references > 0);
	source->references++;
	INSIST(source->references != 0);
	UNLOCK(&source->lock);

	*targetp = source;
}

void
dns_resolver_whenshutdown(dns_resolver_t *res, isc_task_t *task,
			  isc_event_t **eventp)
{
	isc_task_t *clone;
	isc_event_t *event;

	REQUIRE(VALID_RESOLVER(res));
	REQUIRE(eventp != NULL);

	event = *eventp;
	*eventp = NULL;

	LOCK(&res->lock);

	if (res->exiting && res->activebuckets == 0) {
		/*
		 * We're already shutdown.  Send the event.
		 */
		event->ev_sender = res;
		isc_task_send(task, &event);
	} else {
		clone = NULL;
		isc_task_attach(task, &clone);
		event->ev_sender = clone;
		ISC_LIST_APPEND(res->whenshutdown, event, ev_link);
	}

	UNLOCK(&res->lock);
}

void
dns_resolver_shutdown(dns_resolver_t *res) {
	unsigned int i;
	fetchctx_t *fctx;
	isc_socket_t *sock;
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(res));

	RTRACE("shutdown");

	LOCK(&res->lock);

	if (!res->exiting) {
		RTRACE("exiting");
		res->exiting = ISC_TRUE;

		for (i = 0; i < res->nbuckets; i++) {
			LOCK(&res->buckets[i].lock);
			for (fctx = ISC_LIST_HEAD(res->buckets[i].fctxs);
			     fctx != NULL;
			     fctx = ISC_LIST_NEXT(fctx, link))
				fctx_shutdown(fctx);
			if (res->dispatchv4 != NULL && !res->exclusivev4) {
				sock = dns_dispatch_getsocket(res->dispatchv4);
				isc_socket_cancel(sock, res->buckets[i].task,
						  ISC_SOCKCANCEL_ALL);
			}
			if (res->dispatchv6 != NULL && !res->exclusivev6) {
				sock = dns_dispatch_getsocket(res->dispatchv6);
				isc_socket_cancel(sock, res->buckets[i].task,
						  ISC_SOCKCANCEL_ALL);
			}
			res->buckets[i].exiting = ISC_TRUE;
			if (ISC_LIST_EMPTY(res->buckets[i].fctxs)) {
				INSIST(res->activebuckets > 0);
				res->activebuckets--;
			}
			UNLOCK(&res->buckets[i].lock);
		}
		if (res->activebuckets == 0)
			send_shutdown_events(res);
		result = isc_timer_reset(res->spillattimer,
					 isc_timertype_inactive, NULL,
					 NULL, ISC_TRUE);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	UNLOCK(&res->lock);
}

void
dns_resolver_detach(dns_resolver_t **resp) {
	dns_resolver_t *res;
	isc_boolean_t need_destroy = ISC_FALSE;

	REQUIRE(resp != NULL);
	res = *resp;
	REQUIRE(VALID_RESOLVER(res));

	RTRACE("detach");

	LOCK(&res->lock);

	INSIST(res->references > 0);
	res->references--;
	if (res->references == 0) {
		INSIST(res->exiting && res->activebuckets == 0);
		need_destroy = ISC_TRUE;
	}

	UNLOCK(&res->lock);

	if (need_destroy)
		destroy(res);

	*resp = NULL;
}

static inline isc_boolean_t
fctx_match(fetchctx_t *fctx, dns_name_t *name, dns_rdatatype_t type,
	   unsigned int options)
{
	if (fctx->type != type || fctx->options != options)
		return (ISC_FALSE);
	return (dns_name_equal(&fctx->name, name));
}

static inline void
log_fetch(dns_name_t *name, dns_rdatatype_t type) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	int level = ISC_LOG_DEBUG(1);

	if (! isc_log_wouldlog(dns_lctx, level))
		return;

	dns_name_format(name, namebuf, sizeof(namebuf));
	dns_rdatatype_format(type, typebuf, sizeof(typebuf));

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_RESOLVER,
		      DNS_LOGMODULE_RESOLVER, level,
		      "createfetch: %s %s", namebuf, typebuf);
}

isc_result_t
dns_resolver_createfetch(dns_resolver_t *res, dns_name_t *name,
			 dns_rdatatype_t type,
			 dns_name_t *domain, dns_rdataset_t *nameservers,
			 dns_forwarders_t *forwarders,
			 unsigned int options, isc_task_t *task,
			 isc_taskaction_t action, void *arg,
			 dns_rdataset_t *rdataset,
			 dns_rdataset_t *sigrdataset,
			 dns_fetch_t **fetchp)
{
	return (dns_resolver_createfetch2(res, name, type, domain,
					  nameservers, forwarders, NULL, 0,
					  options, task, action, arg,
					  rdataset, sigrdataset, fetchp));
}

isc_result_t
dns_resolver_createfetch2(dns_resolver_t *res, dns_name_t *name,
			  dns_rdatatype_t type,
			  dns_name_t *domain, dns_rdataset_t *nameservers,
			  dns_forwarders_t *forwarders,
			  isc_sockaddr_t *client, dns_messageid_t id,
			  unsigned int options, isc_task_t *task,
			  isc_taskaction_t action, void *arg,
			  dns_rdataset_t *rdataset,
			  dns_rdataset_t *sigrdataset,
			  dns_fetch_t **fetchp)
{
	dns_fetch_t *fetch;
	fetchctx_t *fctx = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int bucketnum;
	isc_boolean_t new_fctx = ISC_FALSE;
	isc_event_t *event;
	unsigned int count = 0;
	unsigned int spillat;
	unsigned int spillatmin;

	UNUSED(forwarders);

	REQUIRE(VALID_RESOLVER(res));
	REQUIRE(res->frozen);
	/* XXXRTH  Check for meta type */
	if (domain != NULL) {
		REQUIRE(DNS_RDATASET_VALID(nameservers));
		REQUIRE(nameservers->type == dns_rdatatype_ns);
	} else
		REQUIRE(nameservers == NULL);
	REQUIRE(forwarders == NULL);
	REQUIRE(!dns_rdataset_isassociated(rdataset));
	REQUIRE(sigrdataset == NULL ||
		!dns_rdataset_isassociated(sigrdataset));
	REQUIRE(fetchp != NULL && *fetchp == NULL);

	log_fetch(name, type);

	/*
	 * XXXRTH  use a mempool?
	 */
	fetch = isc_mem_get(res->mctx, sizeof(*fetch));
	if (fetch == NULL)
		return (ISC_R_NOMEMORY);

	bucketnum = dns_name_fullhash(name, ISC_FALSE) % res->nbuckets;

	LOCK(&res->lock);
	spillat = res->spillat;
	spillatmin = res->spillatmin;
	UNLOCK(&res->lock);
	LOCK(&res->buckets[bucketnum].lock);

	if (res->buckets[bucketnum].exiting) {
		result = ISC_R_SHUTTINGDOWN;
		goto unlock;
	}

	if ((options & DNS_FETCHOPT_UNSHARED) == 0) {
		for (fctx = ISC_LIST_HEAD(res->buckets[bucketnum].fctxs);
		     fctx != NULL;
		     fctx = ISC_LIST_NEXT(fctx, link)) {
			if (fctx_match(fctx, name, type, options))
				break;
		}
	}

	/*
	 * Is this a duplicate?
	 */
	if (fctx != NULL && client != NULL) {
		dns_fetchevent_t *fevent;
		for (fevent = ISC_LIST_HEAD(fctx->events);
		     fevent != NULL;
		     fevent = ISC_LIST_NEXT(fevent, ev_link)) {
			if (fevent->client != NULL && fevent->id == id &&
			    isc_sockaddr_equal(fevent->client, client)) {
				result = DNS_R_DUPLICATE;
				goto unlock;
			}
			count++;
		}
	}
	if (count >= spillatmin && spillatmin != 0) {
		INSIST(fctx != NULL);
		if (count >= spillat)
			fctx->spilled = ISC_TRUE;
		if (fctx->spilled) {
			result = DNS_R_DROP;
			goto unlock;
		}
	}

	/*
	 * If we didn't have a fetch, would attach to a done fetch, this
	 * fetch has already cloned its results, or if the fetch has gone
	 * "idle" (no one was interested in it), we need to start a new
	 * fetch instead of joining with the existing one.
	 */
	if (fctx == NULL ||
	    fctx->state == fetchstate_done ||
	    fctx->cloned ||
	    ISC_LIST_EMPTY(fctx->events)) {
		fctx = NULL;
		result = fctx_create(res, name, type, domain, nameservers,
				     options, bucketnum, &fctx);
		if (result != ISC_R_SUCCESS)
			goto unlock;
		new_fctx = ISC_TRUE;
	}

	result = fctx_join(fctx, task, client, id, action, arg,
			   rdataset, sigrdataset, fetch);
	if (new_fctx) {
		if (result == ISC_R_SUCCESS) {
			/*
			 * Launch this fctx.
			 */
			event = &fctx->control_event;
			ISC_EVENT_INIT(event, sizeof(*event), 0, NULL,
				       DNS_EVENT_FETCHCONTROL,
				       fctx_start, fctx, NULL,
				       NULL, NULL);
			isc_task_send(res->buckets[bucketnum].task, &event);
		} else {
			/*
			 * We don't care about the result of fctx_destroy()
			 * since we know we're not exiting.
			 */
			(void)fctx_destroy(fctx);
		}
	}

 unlock:
	UNLOCK(&res->buckets[bucketnum].lock);

	if (result == ISC_R_SUCCESS) {
		FTRACE("created");
		*fetchp = fetch;
	} else
		isc_mem_put(res->mctx, fetch, sizeof(*fetch));

	return (result);
}

void
dns_resolver_cancelfetch(dns_fetch_t *fetch) {
	fetchctx_t *fctx;
	dns_resolver_t *res;
	dns_fetchevent_t *event, *next_event;
	isc_task_t *etask;

	REQUIRE(DNS_FETCH_VALID(fetch));
	fctx = fetch->private;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	FTRACE("cancelfetch");

	LOCK(&res->buckets[fctx->bucketnum].lock);

	/*
	 * Find the completion event for this fetch (as opposed
	 * to those for other fetches that have joined the same
	 * fctx) and send it with result = ISC_R_CANCELED.
	 */
	event = NULL;
	if (fctx->state != fetchstate_done) {
		for (event = ISC_LIST_HEAD(fctx->events);
		     event != NULL;
		     event = next_event) {
			next_event = ISC_LIST_NEXT(event, ev_link);
			if (event->fetch == fetch) {
				ISC_LIST_UNLINK(fctx->events, event, ev_link);
				break;
			}
		}
	}
	if (event != NULL) {
		etask = event->ev_sender;
		event->ev_sender = fctx;
		event->result = ISC_R_CANCELED;
		isc_task_sendanddetach(&etask, ISC_EVENT_PTR(&event));
	}
	/*
	 * The fctx continues running even if no fetches remain;
	 * the answer is still cached.
	 */

	UNLOCK(&res->buckets[fctx->bucketnum].lock);
}

void
dns_resolver_destroyfetch(dns_fetch_t **fetchp) {
	dns_fetch_t *fetch;
	dns_resolver_t *res;
	dns_fetchevent_t *event, *next_event;
	fetchctx_t *fctx;
	unsigned int bucketnum;
	isc_boolean_t bucket_empty = ISC_FALSE;

	REQUIRE(fetchp != NULL);
	fetch = *fetchp;
	REQUIRE(DNS_FETCH_VALID(fetch));
	fctx = fetch->private;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	FTRACE("destroyfetch");

	bucketnum = fctx->bucketnum;
	LOCK(&res->buckets[bucketnum].lock);

	/*
	 * Sanity check: the caller should have gotten its event before
	 * trying to destroy the fetch.
	 */
	event = NULL;
	if (fctx->state != fetchstate_done) {
		for (event = ISC_LIST_HEAD(fctx->events);
		     event != NULL;
		     event = next_event) {
			next_event = ISC_LIST_NEXT(event, ev_link);
			RUNTIME_CHECK(event->fetch != fetch);
		}
	}

	INSIST(fctx->references > 0);
	fctx->references--;
	if (fctx->references == 0) {
		/*
		 * No one cares about the result of this fetch anymore.
		 */
		if (fctx->pending == 0 && fctx->nqueries == 0 &&
		    ISC_LIST_EMPTY(fctx->validators) &&
		    SHUTTINGDOWN(fctx)) {
			/*
			 * This fctx is already shutdown; we were just
			 * waiting for the last reference to go away.
			 */
			bucket_empty = fctx_destroy(fctx);
		} else {
			/*
			 * Initiate shutdown.
			 */
			fctx_shutdown(fctx);
		}
	}

	UNLOCK(&res->buckets[bucketnum].lock);

	isc_mem_put(res->mctx, fetch, sizeof(*fetch));
	*fetchp = NULL;

	if (bucket_empty)
		empty_bucket(res);
}

void
dns_resolver_logfetch(dns_fetch_t *fetch, isc_log_t *lctx,
		      isc_logcategory_t *category, isc_logmodule_t *module,
		      int level, isc_boolean_t duplicateok)
{
	fetchctx_t *fctx;
	dns_resolver_t *res;
	char domainbuf[DNS_NAME_FORMATSIZE];

	REQUIRE(DNS_FETCH_VALID(fetch));
	fctx = fetch->private;
	REQUIRE(VALID_FCTX(fctx));
	res = fctx->res;

	LOCK(&res->buckets[fctx->bucketnum].lock);

	INSIST(fctx->exitline >= 0);
	if (!fctx->logged || duplicateok) {
		dns_name_format(&fctx->domain, domainbuf, sizeof(domainbuf));
		isc_log_write(lctx, category, module, level,
			      "fetch completed at %s:%d for %s in "
			      "%" ISC_PRINT_QUADFORMAT "u."
			      "%06" ISC_PRINT_QUADFORMAT "u: %s/%s "
			      "[domain:%s,referral:%u,restart:%u,qrysent:%u,"
			      "timeout:%u,lame:%u,neterr:%u,badresp:%u,"
			      "adberr:%u,findfail:%u,valfail:%u]",
			      __FILE__, fctx->exitline, fctx->info,
			      fctx->duration / 1000000,
			      fctx->duration % 1000000,
			      isc_result_totext(fctx->result),
			      isc_result_totext(fctx->vresult), domainbuf,
			      fctx->referrals, fctx->restarts,
			      fctx->querysent, fctx->timeouts, fctx->lamecount,
			      fctx->neterr, fctx->badresp, fctx->adberr,
			      fctx->findfail, fctx->valfail);
		fctx->logged = ISC_TRUE;
	}

	UNLOCK(&res->buckets[fctx->bucketnum].lock);
}

dns_dispatchmgr_t *
dns_resolver_dispatchmgr(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->dispatchmgr);
}

dns_dispatch_t *
dns_resolver_dispatchv4(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->dispatchv4);
}

dns_dispatch_t *
dns_resolver_dispatchv6(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->dispatchv6);
}

isc_socketmgr_t *
dns_resolver_socketmgr(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->socketmgr);
}

isc_taskmgr_t *
dns_resolver_taskmgr(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->taskmgr);
}

isc_uint32_t
dns_resolver_getlamettl(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->lame_ttl);
}

void
dns_resolver_setlamettl(dns_resolver_t *resolver, isc_uint32_t lame_ttl) {
	REQUIRE(VALID_RESOLVER(resolver));
	resolver->lame_ttl = lame_ttl;
}

unsigned int
dns_resolver_nrunning(dns_resolver_t *resolver) {
	unsigned int n;
	LOCK(&resolver->nlock);
	n = resolver->nfctx;
	UNLOCK(&resolver->nlock);
	return (n);
}

isc_result_t
dns_resolver_addalternate(dns_resolver_t *resolver, isc_sockaddr_t *alt,
			  dns_name_t *name, in_port_t port) {
	alternate_t *a;
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(resolver));
	REQUIRE(!resolver->frozen);
	REQUIRE((alt == NULL) ^ (name == NULL));

	a = isc_mem_get(resolver->mctx, sizeof(*a));
	if (a == NULL)
		return (ISC_R_NOMEMORY);
	if (alt != NULL) {
		a->isaddress = ISC_TRUE;
		a->_u.addr = *alt;
	} else {
		a->isaddress = ISC_FALSE;
		a->_u._n.port = port;
		dns_name_init(&a->_u._n.name, NULL);
		result = dns_name_dup(name, resolver->mctx, &a->_u._n.name);
		if (result != ISC_R_SUCCESS) {
			isc_mem_put(resolver->mctx, a, sizeof(*a));
			return (result);
		}
	}
	ISC_LINK_INIT(a, link);
	ISC_LIST_APPEND(resolver->alternates, a, link);

	return (ISC_R_SUCCESS);
}

void
dns_resolver_setudpsize(dns_resolver_t *resolver, isc_uint16_t udpsize) {
	REQUIRE(VALID_RESOLVER(resolver));
	resolver->udpsize = udpsize;
}

isc_uint16_t
dns_resolver_getudpsize(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));
	return (resolver->udpsize);
}

static void
free_algorithm(void *node, void *arg) {
	unsigned char *algorithms = node;
	isc_mem_t *mctx = arg;

	isc_mem_put(mctx, algorithms, *algorithms);
}

void
dns_resolver_reset_algorithms(dns_resolver_t *resolver) {

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	if (resolver->algorithms != NULL)
		dns_rbt_destroy(&resolver->algorithms);
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
}

isc_result_t
dns_resolver_disable_algorithm(dns_resolver_t *resolver, dns_name_t *name,
			       unsigned int alg)
{
	unsigned int len, mask;
	unsigned char *new;
	unsigned char *algorithms;
	isc_result_t result;
	dns_rbtnode_t *node = NULL;

	REQUIRE(VALID_RESOLVER(resolver));
	if (alg > 255)
		return (ISC_R_RANGE);

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	if (resolver->algorithms == NULL) {
		result = dns_rbt_create(resolver->mctx, free_algorithm,
					resolver->mctx, &resolver->algorithms);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}

	len = alg/8 + 2;
	mask = 1 << (alg%8);

	result = dns_rbt_addnode(resolver->algorithms, name, &node);

	if (result == ISC_R_SUCCESS || result == ISC_R_EXISTS) {
		algorithms = node->data;
		if (algorithms == NULL || len > *algorithms) {
			new = isc_mem_get(resolver->mctx, len);
			if (new == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}
			memset(new, 0, len);
			if (algorithms != NULL)
				memcpy(new, algorithms, *algorithms);
			new[len-1] |= mask;
			*new = len;
			node->data = new;
			if (algorithms != NULL)
				isc_mem_put(resolver->mctx, algorithms,
					    *algorithms);
		} else
			algorithms[len-1] |= mask;
	}
	result = ISC_R_SUCCESS;
 cleanup:
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_write);
#endif
	return (result);
}

isc_boolean_t
dns_resolver_algorithm_supported(dns_resolver_t *resolver, dns_name_t *name,
				 unsigned int alg)
{
	unsigned int len, mask;
	unsigned char *algorithms;
	void *data = NULL;
	isc_result_t result;
	isc_boolean_t found = ISC_FALSE;

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_ALGLOCK
	RWLOCK(&resolver->alglock, isc_rwlocktype_read);
#endif
	if (resolver->algorithms == NULL)
		goto unlock;
	result = dns_rbt_findname(resolver->algorithms, name, 0, NULL, &data);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		len = alg/8 + 2;
		mask = 1 << (alg%8);
		algorithms = data;
		if (len <= *algorithms && (algorithms[len-1] & mask) != 0)
			found = ISC_TRUE;
	}
 unlock:
#if USE_ALGLOCK
	RWUNLOCK(&resolver->alglock, isc_rwlocktype_read);
#endif
	if (found)
		return (ISC_FALSE);
	return (dst_algorithm_supported(alg));
}

isc_boolean_t
dns_resolver_digest_supported(dns_resolver_t *resolver, unsigned int digest) {

	UNUSED(resolver);
	return (dns_ds_digest_supported(digest));
}

void
dns_resolver_resetmustbesecure(dns_resolver_t *resolver) {

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_MBSLOCK
	RWLOCK(&resolver->mbslock, isc_rwlocktype_write);
#endif
	if (resolver->mustbesecure != NULL)
		dns_rbt_destroy(&resolver->mustbesecure);
#if USE_MBSLOCK
	RWUNLOCK(&resolver->mbslock, isc_rwlocktype_write);
#endif
}

static isc_boolean_t yes = ISC_TRUE, no = ISC_FALSE;

isc_result_t
dns_resolver_setmustbesecure(dns_resolver_t *resolver, dns_name_t *name,
			     isc_boolean_t value)
{
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_MBSLOCK
	RWLOCK(&resolver->mbslock, isc_rwlocktype_write);
#endif
	if (resolver->mustbesecure == NULL) {
		result = dns_rbt_create(resolver->mctx, NULL, NULL,
					&resolver->mustbesecure);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
	}
	result = dns_rbt_addname(resolver->mustbesecure, name,
				 value ? &yes : &no);
 cleanup:
#if USE_MBSLOCK
	RWUNLOCK(&resolver->mbslock, isc_rwlocktype_write);
#endif
	return (result);
}

isc_boolean_t
dns_resolver_getmustbesecure(dns_resolver_t *resolver, dns_name_t *name) {
	void *data = NULL;
	isc_boolean_t value = ISC_FALSE;
	isc_result_t result;

	REQUIRE(VALID_RESOLVER(resolver));

#if USE_MBSLOCK
	RWLOCK(&resolver->mbslock, isc_rwlocktype_read);
#endif
	if (resolver->mustbesecure == NULL)
		goto unlock;
	result = dns_rbt_findname(resolver->mustbesecure, name, 0, NULL, &data);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH)
		value = *(isc_boolean_t*)data;
 unlock:
#if USE_MBSLOCK
	RWUNLOCK(&resolver->mbslock, isc_rwlocktype_read);
#endif
	return (value);
}

void
dns_resolver_getclientsperquery(dns_resolver_t *resolver, isc_uint32_t *cur,
				isc_uint32_t *min, isc_uint32_t *max)
{
	REQUIRE(VALID_RESOLVER(resolver));

	LOCK(&resolver->lock);
	if (cur != NULL)
		*cur = resolver->spillat;
	if (min != NULL)
		*min = resolver->spillatmin;
	if (max != NULL)
		*max = resolver->spillatmax;
	UNLOCK(&resolver->lock);
}

void
dns_resolver_setclientsperquery(dns_resolver_t *resolver, isc_uint32_t min,
				isc_uint32_t max)
{
	REQUIRE(VALID_RESOLVER(resolver));

	LOCK(&resolver->lock);
	resolver->spillatmin = resolver->spillat = min;
	resolver->spillatmax = max;
	UNLOCK(&resolver->lock);
}

isc_boolean_t
dns_resolver_getzeronosoattl(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));

	return (resolver->zero_no_soa_ttl);
}

void
dns_resolver_setzeronosoattl(dns_resolver_t *resolver, isc_boolean_t state) {
	REQUIRE(VALID_RESOLVER(resolver));

	resolver->zero_no_soa_ttl = state;
}

unsigned int
dns_resolver_getoptions(dns_resolver_t *resolver) {
	REQUIRE(VALID_RESOLVER(resolver));

	return (resolver->options);
}
