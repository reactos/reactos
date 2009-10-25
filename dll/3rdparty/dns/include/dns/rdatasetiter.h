/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
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

/* $Id: rdatasetiter.h,v 1.21 2007/06/19 23:47:17 tbox Exp $ */

#ifndef DNS_RDATASETITER_H
#define DNS_RDATASETITER_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/rdatasetiter.h
 * \brief
 * The DNS Rdataset Iterator interface allows iteration of all of the
 * rdatasets at a node.
 *
 * The dns_rdatasetiter_t type is like a "virtual class".  To actually use
 * it, an implementation of the class is required.  This implementation is
 * supplied by the database.
 *
 * It is the client's responsibility to call dns_rdataset_disassociate()
 * on all rdatasets returned.
 *
 * XXX more XXX
 *
 * MP:
 *\li	The iterator itself is not locked.  The caller must ensure
 *	synchronization.
 *
 *\li	The iterator methods ensure appropriate database locking.
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
 *\li	None.
 */

/*****
 ***** Imports
 *****/

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/stdtime.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*****
 ***** Types
 *****/

typedef struct dns_rdatasetitermethods {
	void		(*destroy)(dns_rdatasetiter_t **iteratorp);
	isc_result_t	(*first)(dns_rdatasetiter_t *iterator);
	isc_result_t	(*next)(dns_rdatasetiter_t *iterator);
	void		(*current)(dns_rdatasetiter_t *iterator,
				   dns_rdataset_t *rdataset);
} dns_rdatasetitermethods_t;

#define DNS_RDATASETITER_MAGIC	     ISC_MAGIC('D','N','S','i')
#define DNS_RDATASETITER_VALID(i)    ISC_MAGIC_VALID(i, DNS_RDATASETITER_MAGIC)

/*%
 * This structure is actually just the common prefix of a DNS db
 * implementation's version of a dns_rdatasetiter_t.
 * \brief
 * Direct use of this structure by clients is forbidden.  DB implementations
 * may change the structure.  'magic' must be #DNS_RDATASETITER_MAGIC for
 * any of the dns_rdatasetiter routines to work.  DB implementations must
 * maintain all DB rdataset iterator invariants.
 */
struct dns_rdatasetiter {
	/* Unlocked. */
	unsigned int			magic;
	dns_rdatasetitermethods_t *	methods;
	dns_db_t *			db;
	dns_dbnode_t *			node;
	dns_dbversion_t *		version;
	isc_stdtime_t			now;
};

void
dns_rdatasetiter_destroy(dns_rdatasetiter_t **iteratorp);
/*%<
 * Destroy '*iteratorp'.
 *
 * Requires:
 *
 *\li	'*iteratorp' is a valid iterator.
 *
 * Ensures:
 *
 *\li	All resources used by the iterator are freed.
 *
 *\li	*iteratorp == NULL.
 */

isc_result_t
dns_rdatasetiter_first(dns_rdatasetiter_t *iterator);
/*%<
 * Move the rdataset cursor to the first rdataset at the node (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOMORE			There are no rdatasets at the node.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_rdatasetiter_next(dns_rdatasetiter_t *iterator);
/*%<
 * Move the rdataset cursor to the next rdataset at the node (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOMORE			There are no more rdatasets at the
 *					node.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

void
dns_rdatasetiter_current(dns_rdatasetiter_t *iterator,
			 dns_rdataset_t *rdataset);
/*%<
 * Return the current rdataset.
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 *\li	'rdataset' is a valid, disassociated rdataset.
 *
 *\li	The rdataset cursor of 'iterator' is at a valid location (i.e. the
 *	result of last call to a cursor movement command was #ISC_R_SUCCESS).
 */

ISC_LANG_ENDDECLS

#endif /* DNS_RDATASETITER_H */
