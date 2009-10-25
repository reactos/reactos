/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
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

/* $Id: stats.h,v 1.18.56.2 2009/01/29 23:47:44 tbox Exp $ */

#ifndef DNS_STATS_H
#define DNS_STATS_H 1

/*! \file dns/stats.h */

#include <dns/types.h>

/*%
 * Statistics counters.  Used as isc_statscounter_t values.
 */
enum {
	/*%
	 * Resolver statistics counters.
	 */
	dns_resstatscounter_queryv4 = 0,
	dns_resstatscounter_queryv6 = 1,
	dns_resstatscounter_responsev4 = 2,
	dns_resstatscounter_responsev6 = 3,
	dns_resstatscounter_nxdomain = 4,
	dns_resstatscounter_servfail = 5,
	dns_resstatscounter_formerr = 6,
	dns_resstatscounter_othererror = 7,
	dns_resstatscounter_edns0fail = 8,
	dns_resstatscounter_mismatch = 9,
	dns_resstatscounter_truncated = 10,
	dns_resstatscounter_lame = 11,
	dns_resstatscounter_retry = 12,
	dns_resstatscounter_gluefetchv4 = 13,
	dns_resstatscounter_gluefetchv6 = 14,
	dns_resstatscounter_gluefetchv4fail = 15,
	dns_resstatscounter_gluefetchv6fail = 16,
	dns_resstatscounter_val = 17,
	dns_resstatscounter_valsuccess = 18,
	dns_resstatscounter_valnegsuccess = 19,
	dns_resstatscounter_valfail = 20,
	dns_resstatscounter_dispabort = 21,
	dns_resstatscounter_dispsockfail = 22,
	dns_resstatscounter_querytimeout = 23,
	dns_resstatscounter_queryrtt0 = 24,
	dns_resstatscounter_queryrtt1 = 25,
	dns_resstatscounter_queryrtt2 = 26,
	dns_resstatscounter_queryrtt3 = 27,
	dns_resstatscounter_queryrtt4 = 28,
	dns_resstatscounter_queryrtt5 = 29,

	dns_resstatscounter_max = 30,

	/*%
	 * Zone statistics counters.
	 */
	dns_zonestatscounter_notifyoutv4 = 0,
	dns_zonestatscounter_notifyoutv6 = 1,
	dns_zonestatscounter_notifyinv4 = 2,
	dns_zonestatscounter_notifyinv6 = 3,
	dns_zonestatscounter_notifyrej = 4,
	dns_zonestatscounter_soaoutv4 = 5,
	dns_zonestatscounter_soaoutv6 = 6,
	dns_zonestatscounter_axfrreqv4 = 7,
	dns_zonestatscounter_axfrreqv6 = 8,
	dns_zonestatscounter_ixfrreqv4 = 9,
	dns_zonestatscounter_ixfrreqv6 = 10,
	dns_zonestatscounter_xfrsuccess = 11,
	dns_zonestatscounter_xfrfail = 12,

	dns_zonestatscounter_max = 13,

	/*%
	* Query statistics counters (obsolete).
	*/
	dns_statscounter_success = 0,    /*%< Successful lookup */
	dns_statscounter_referral = 1,   /*%< Referral result */
	dns_statscounter_nxrrset = 2,    /*%< NXRRSET result */
	dns_statscounter_nxdomain = 3,   /*%< NXDOMAIN result */
	dns_statscounter_recursion = 4,  /*%< Recursion was used */
	dns_statscounter_failure = 5,    /*%< Some other failure */
	dns_statscounter_duplicate = 6,  /*%< Duplicate query */
	dns_statscounter_dropped = 7	 /*%< Duplicate query (dropped) */
};

#define DNS_STATS_NCOUNTERS 8

#if 0
/*%<
 * Flag(s) for dns_xxxstats_dump().  DNS_STATSDUMP_VERBOSE is obsolete.
 * ISC_STATSDUMP_VERBOSE should be used instead.  These two values are
 * intentionally defined to be the same value to ensure binary compatibility.
 */
#define DNS_STATSDUMP_VERBOSE	0x00000001 /*%< dump 0-value counters */
#endif

/*%<
 * (Obsoleted)
 */
LIBDNS_EXTERNAL_DATA extern const char *dns_statscounter_names[];

/*%
 * Attributes for statistics counters of RRset and Rdatatype types.
 *
 * _OTHERTYPE
 *	The rdata type is not explicitly supported and the corresponding counter
 *	is counted for other such types, too.  When this attribute is set,
 *	the base type is of no use.
 *
 * _NXRRSET
 * 	RRset type counters only.  Indicates the RRset is non existent.
 *
 * _NXDOMAIN
 *	RRset type counters only.  Indicates a non existent name.  When this
 *	attribute is set, the base type is of no use.
 */
#define DNS_RDATASTATSTYPE_ATTR_OTHERTYPE	0x0001
#define DNS_RDATASTATSTYPE_ATTR_NXRRSET		0x0002
#define DNS_RDATASTATSTYPE_ATTR_NXDOMAIN	0x0004

/*%<
 * Conversion macros among dns_rdatatype_t, attributes and isc_statscounter_t.
 */
#define DNS_RDATASTATSTYPE_BASE(type)	((dns_rdatatype_t)((type) & 0xFFFF))
#define DNS_RDATASTATSTYPE_ATTR(type)	((type) >> 16)
#define DNS_RDATASTATSTYPE_VALUE(b, a)	(((a) << 16) | (b))

/*%<
 * Types of dump callbacks.
 */
typedef void (*dns_generalstats_dumper_t)(isc_statscounter_t, isc_uint64_t,
					  void *);
typedef void (*dns_rdatatypestats_dumper_t)(dns_rdatastatstype_t, isc_uint64_t,
					    void *);
typedef void (*dns_opcodestats_dumper_t)(dns_opcode_t, isc_uint64_t, void *);

isc_result_t
dns_generalstats_create(isc_mem_t *mctx, dns_stats_t **statsp, int ncounters);
/*%<
 * Create a statistics counter structure of general type.  It counts a general
 * set of counters indexed by an ID between 0 and ncounters -1.
 * This function is obsolete.  A more general function, isc_stats_create(),
 * should be used.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- all ok
 *
 *\li	anything else	-- failure
 */

isc_result_t
dns_rdatatypestats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per rdatatype.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- all ok
 *
 *\li	anything else	-- failure
 */

isc_result_t
dns_rdatasetstats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per RRset.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- all ok
 *
 *\li	anything else	-- failure
 */

isc_result_t
dns_opcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per opcode.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- all ok
 *
 *\li	anything else	-- failure
 */

void
dns_stats_attach(dns_stats_t *stats, dns_stats_t **statsp);
/*%<
 * Attach to a statistics set.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL
 */

void
dns_stats_detach(dns_stats_t **statsp);
/*%<
 * Detaches from the statistics set.
 *
 * Requires:
 *\li	'statsp' != NULL and '*statsp' is a valid dns_stats_t.
 */

void
dns_generalstats_increment(dns_stats_t *stats, isc_statscounter_t counter);
/*%<
 * Increment the counter-th counter of stats.  This function is obsolete.
 * A more general function, isc_stats_increment(), should be used.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 *
 *\li	counter is less than the maximum available ID for the stats specified
 *	on creation.
 */

void
dns_rdatatypestats_increment(dns_stats_t *stats, dns_rdatatype_t type);
/*%<
 * Increment the statistics counter for 'type'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_rdatatypestats_create().
 */

void
dns_rdatasetstats_increment(dns_stats_t *stats, dns_rdatastatstype_t rrsettype);
/*%<
 * Increment the statistics counter for 'rrsettype'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_rdatasetstats_create().
 */

void
dns_rdatasetstats_decrement(dns_stats_t *stats, dns_rdatastatstype_t rrsettype);
/*%<
 * Decrement the statistics counter for 'rrsettype'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_rdatasetstats_create().
 */

void
dns_opcodestats_increment(dns_stats_t *stats, dns_opcode_t code);
/*%<
 * Increment the statistics counter for 'code'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_opcodestats_create().
 */

void
dns_generalstats_dump(dns_stats_t *stats, dns_generalstats_dumper_t dump_fn,
		      void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with its current value and the given argument
 * arg.  By default counters that have a value of 0 is skipped; if options has
 * the ISC_STATSDUMP_VERBOSE flag, even such counters are dumped.
 *
 * This function is obsolete.  A more general function, isc_stats_dump(),
 * should be used.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

void
dns_rdatatypestats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
			void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with the corresponding type in the form of
 * dns_rdatastatstype_t, the current counter value and the given argument
 * arg.  By default counters that have a value of 0 is skipped; if options has
 * the ISC_STATSDUMP_VERBOSE flag, even such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

void
dns_rdatasetstats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
		       void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with the corresponding type in the form of
 * dns_rdatastatstype_t, the current counter value and the given argument
 * arg.  By default counters that have a value of 0 is skipped; if options has
 * the ISC_STATSDUMP_VERBOSE flag, even such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

void
dns_opcodestats_dump(dns_stats_t *stats, dns_opcodestats_dumper_t dump_fn,
		     void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with the corresponding opcode, the current
 * counter value and the given argument arg.  By default counters that have a
 * value of 0 is skipped; if options has the ISC_STATSDUMP_VERBOSE flag, even
 * such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

isc_result_t
dns_stats_alloccounters(isc_mem_t *mctx, isc_uint64_t **ctrp);
/*%<
 * Allocate an array of query statistics counters from the memory
 * context 'mctx'.
 *
 * This function is obsoleted.  Use dns_xxxstats_create() instead.
 */

void
dns_stats_freecounters(isc_mem_t *mctx, isc_uint64_t **ctrp);
/*%<
 * Free an array of query statistics counters allocated from the memory
 * context 'mctx'.
 *
 * This function is obsoleted.  Use dns_stats_destroy() instead.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_STATS_H */
