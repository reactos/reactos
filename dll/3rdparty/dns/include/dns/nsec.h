/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: nsec.h,v 1.12 2008/09/25 04:02:39 tbox Exp $ */

#ifndef DNS_NSEC_H
#define DNS_NSEC_H 1

/*! \file dns/nsec.h */

#include <isc/lang.h>

#include <dns/types.h>
#include <dns/name.h>

#define DNS_NSEC_BUFFERSIZE (DNS_NAME_MAXWIRE + 8192 + 512)

ISC_LANG_BEGINDECLS

isc_result_t
dns_nsec_buildrdata(dns_db_t *db, dns_dbversion_t *version,
		    dns_dbnode_t *node, dns_name_t *target,
		    unsigned char *buffer, dns_rdata_t *rdata);
/*%<
 * Build the rdata of a NSEC record.
 *
 * Requires:
 *\li	buffer	Points to a temporary buffer of at least
 * 		DNS_NSEC_BUFFERSIZE bytes.
 *\li	rdata	Points to an initialized dns_rdata_t.
 *
 * Ensures:
 *  \li    *rdata	Contains a valid NSEC rdata.  The 'data' member refers
 *		to 'buffer'.
 */

isc_result_t
dns_nsec_build(dns_db_t *db, dns_dbversion_t *version, dns_dbnode_t *node,
	       dns_name_t *target, dns_ttl_t ttl);
/*%<
 * Build a NSEC record and add it to a database.
 */

isc_boolean_t
dns_nsec_typepresent(dns_rdata_t *nsec, dns_rdatatype_t type);
/*%<
 * Determine if a type is marked as present in an NSEC record.
 *
 * Requires:
 *\li	'nsec' points to a valid rdataset of type NSEC
 */

isc_result_t
dns_nsec_nseconly(dns_db_t *db, dns_dbversion_t *version,
		  isc_boolean_t *answer);
/*
 * Report whether the DNSKEY RRset has a NSEC only algorithm.  Unknown
 * algorithms are assumed to support NSEC3.
 *
 * Requires:
 * 	'answer' to be non NULL.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_NSEC_H */
