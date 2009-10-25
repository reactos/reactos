/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2002  Internet Software Consortium.
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

/* $Id: ncache.h,v 1.25 2008/09/25 04:02:39 tbox Exp $ */

#ifndef DNS_NCACHE_H
#define DNS_NCACHE_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/ncache.h
 *\brief
 * DNS Ncache
 *
 * XXX TBS XXX
 *
 * MP:
 *\li	The caller must ensure any required synchronization.
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
 *\li	RFC2308
 */

#include <isc/lang.h>
#include <isc/stdtime.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*%
 * _OMITDNSSEC:
 *      Omit DNSSEC records when rendering.
 */
#define DNS_NCACHETOWIRE_OMITDNSSEC   0x0001

isc_result_t
dns_ncache_add(dns_message_t *message, dns_db_t *cache, dns_dbnode_t *node,
	       dns_rdatatype_t covers, isc_stdtime_t now, dns_ttl_t maxttl,
	       dns_rdataset_t *addedrdataset);
isc_result_t
dns_ncache_addoptout(dns_message_t *message, dns_db_t *cache,
		     dns_dbnode_t *node, dns_rdatatype_t covers,
		     isc_stdtime_t now, dns_ttl_t maxttl,
		     isc_boolean_t optout, dns_rdataset_t *addedrdataset);
/*%<
 * Convert the authority data from 'message' into a negative cache
 * rdataset, and store it in 'cache' at 'node' with a TTL limited to
 * 'maxttl'.
 *
 * The 'covers' argument is the RR type whose nonexistence we are caching,
 * or dns_rdatatype_any when caching a NXDOMAIN response.
 *
 * 'optout' indicates a DNS_RATASETATTR_OPTOUT should be set.
 *
 * Note:
 *\li	If 'addedrdataset' is not NULL, then it will be attached to the added
 *	rdataset.  See dns_db_addrdataset() for more details.
 *
 * Requires:
 *\li	'message' is a valid message with a properly formatting negative cache
 *	authority section.
 *
 *\li	The requirements of dns_db_addrdataset() apply to 'cache', 'node',
 *	'now', and 'addedrdataset'.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOSPACE
 *
 *\li	Any result code of dns_db_addrdataset() is a possible result code
 *	of dns_ncache_add().
 */

isc_result_t
dns_ncache_towire(dns_rdataset_t *rdataset, dns_compress_t *cctx,
		  isc_buffer_t *target, unsigned int options,
		  unsigned int *countp);
/*%<
 * Convert the negative caching rdataset 'rdataset' to wire format,
 * compressing names as specified in 'cctx', and storing the result in
 * 'target'.  If 'omit_dnssec' is set, DNSSEC records will not
 * be added to 'target'.
 *
 * Notes:
 *\li	The number of RRs added to target will be added to *countp.
 *
 * Requires:
 *\li	'rdataset' is a valid negative caching rdataset.
 *
 *\li	'rdataset' is not empty.
 *
 *\li	'countp' is a valid pointer.
 *
 * Ensures:
 *\li	On a return of ISC_R_SUCCESS, 'target' contains a wire format
 *	for the data contained in 'rdataset'.  Any error return leaves
 *	the buffer unchanged.
 *
 *\li	*countp has been incremented by the number of RRs added to
 *	target.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		- all ok
 *\li	#ISC_R_NOSPACE		- 'target' doesn't have enough room
 *
 *\li	Any error returned by dns_rdata_towire(), dns_rdataset_next(),
 *	dns_name_towire().
 */

isc_result_t
dns_ncache_getrdataset(dns_rdataset_t *ncacherdataset, dns_name_t *name,
		       dns_rdatatype_t type, dns_rdataset_t *rdataset);
/*%<
 * Search the negative caching rdataset for an rdataset with the
 * specified name and type.
 *
 * Requires:
 *\li	'ncacherdataset' is a valid negative caching rdataset.
 *
 *\li	'ncacherdataset' is not empty.
 *
 *\li	'name' is a valid name.
 *
 *\li	'type' is not SIG, or a meta-RR type.
 *
 *\li	'rdataset' is a valid disassociated rdataset.
 *
 * Ensures:
 *\li	On a return of ISC_R_SUCCESS, 'rdataset' is bound to the found
 *	rdataset.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		- the rdataset was found.
 *\li	#ISC_R_NOTFOUND		- the rdataset was not found.
 *
 */

void
dns_ncache_current(dns_rdataset_t *ncacherdataset, dns_name_t *found,
		   dns_rdataset_t *rdataset);

/*%<
 * Extract the current rdataset and name from a ncache entry.
 *
 * Requires:
 * \li	'ncacherdataset' to be valid and to be a negative cache entry
 * \li	'found' to be valid.
 * \li	'rdataset' to be unassociated.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_NCACHE_H */
