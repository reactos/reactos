/*
 * Copyright (C) 2008, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: nsec3.h,v 1.5.48.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_NSEC3_H
#define DNS_NSEC3_H 1

#include <isc/lang.h>
#include <isc/iterated_hash.h>

#include <dns/db.h>
#include <dns/diff.h>
#include <dns/name.h>
#include <dns/rdatastruct.h>
#include <dns/types.h>

/*
 * hash = 1, flags =1, iterations = 2, salt length = 1, salt = 255 (max)
 * hash length = 1, hash = 255 (max), bitmap = 8192 + 512 (max)
 */
#define DNS_NSEC3_BUFFERSIZE (6 + 255 + 255 + 8192 + 512)
/*
 * hash = 1, flags = 1, iterations = 2, salt length = 1, salt = 255 (max)
 */
#define DNS_NSEC3PARAM_BUFFERSIZE (5 + 255)

/*
 * Test "unknown" algorithm.  Is mapped to dns_hash_sha1.
 */
#define DNS_NSEC3_UNKNOWNALG 245U

ISC_LANG_BEGINDECLS

isc_result_t
dns_nsec3_buildrdata(dns_db_t *db, dns_dbversion_t *version,
		     dns_dbnode_t *node, unsigned int hashalg,
		     unsigned int optin, unsigned int iterations,
		     const unsigned char *salt, size_t salt_length,
		     const unsigned char *nexthash, size_t hash_length,
		     unsigned char *buffer, dns_rdata_t *rdata);
/*%<
 * Build the rdata of a NSEC3 record for the data at 'node'.
 * Note: 'node' is not the node where the NSEC3 record will be stored.
 *
 * Requires:
 *	buffer	Points to a temporary buffer of at least
 * 		DNS_NSEC_BUFFERSIZE bytes.
 *	rdata	Points to an initialized dns_rdata_t.
 *
 * Ensures:
 *      *rdata	Contains a valid NSEC3 rdata.  The 'data' member refers
 *		to 'buffer'.
 */

isc_boolean_t
dns_nsec3_typepresent(dns_rdata_t *nsec, dns_rdatatype_t type);
/*%<
 * Determine if a type is marked as present in an NSEC3 record.
 *
 * Requires:
 *	'nsec' points to a valid rdataset of type NSEC3
 */

isc_result_t
dns_nsec3_hashname(dns_fixedname_t *result,
		   unsigned char rethash[NSEC3_MAX_HASH_LENGTH],
		   size_t *hash_length, dns_name_t *name, dns_name_t *origin,
		   dns_hash_t hashalg, unsigned int iterations,
		   const unsigned char *salt, size_t saltlength);
/*%<
 * Make a hashed domain name from an unhashed one. If rethash is not NULL
 * the raw hash is stored there.
 */

unsigned int
dns_nsec3_hashlength(dns_hash_t hash);
/*%<
 * Return the length of the hash produced by the specified algorithm
 * or zero when unknown.
 */

isc_boolean_t
dns_nsec3_supportedhash(dns_hash_t hash);
/*%<
 * Return whether we support this hash algorithm or not.
 */

isc_result_t
dns_nsec3_addnsec3(dns_db_t *db, dns_dbversion_t *version,
		   dns_name_t *name, const dns_rdata_nsec3param_t *nsec3param,
		   dns_ttl_t nsecttl, isc_boolean_t unsecure, dns_diff_t *diff);

isc_result_t
dns_nsec3_addnsec3s(dns_db_t *db, dns_dbversion_t *version,
		    dns_name_t *name, dns_ttl_t nsecttl,
		    isc_boolean_t unsecure, dns_diff_t *diff);
/*%<
 * Add NSEC3 records for 'name', recording the change in 'diff'.
 * Adjust previous NSEC3 records, if any, to reflect the addition.
 * The existing NSEC3 records are removed.
 *
 * dns_nsec3_addnsec3() will only add records to the chain identified by
 * 'nsec3param'.
 *
 * 'unsecure' should be set to reflect if this is a potentially
 * unsecure delegation (no DS record).
 *
 * dns_nsec3_addnsec3s() will examine the NSEC3PARAM RRset to determine which
 * chains to be updated.  NSEC3PARAM records with the DNS_NSEC3FLAG_CREATE
 * will be preferentially chosen over NSEC3PARAM records without
 * DNS_NSEC3FLAG_CREATE set.  NSEC3PARAM records with DNS_NSEC3FLAG_REMOVE
 * set will be ignored by dns_nsec3_addnsec3s().  If DNS_NSEC3FLAG_CREATE
 * is set then the new NSEC3 will have OPTOUT set to match the that in the
 * NSEC3PARAM record otherwise OPTOUT will be inherited from the previous
 * record in the chain.
 *
 * Requires:
 *	'db' to be valid.
 *	'version' to be valid or NULL.
 *	'name' to be valid.
 *	'nsec3param' to be valid.
 *	'diff' to be valid.
 */

isc_result_t
dns_nsec3_delnsec3(dns_db_t *db, dns_dbversion_t *version, dns_name_t *name,
		   const dns_rdata_nsec3param_t *nsec3param, dns_diff_t *diff);

isc_result_t
dns_nsec3_delnsec3s(dns_db_t *db, dns_dbversion_t *version, dns_name_t *name,
		    dns_diff_t *diff);
/*%<
 * Remove NSEC3 records for 'name', recording the change in 'diff'.
 * Adjust previous NSEC3 records, if any, to reflect the removal.
 *
 * dns_nsec3_delnsec3() performs the above for the chain identified by
 * 'nsec3param'.
 *
 * dns_nsec3_delnsec3s() examines the NSEC3PARAM RRset in a similar manner
 * to dns_nsec3_addnsec3s().  Unlike dns_nsec3_addnsec3s() updated NSEC3
 * records have the OPTOUT flag preserved.
 *
 * Requires:
 *	'db' to be valid.
 *	'version' to be valid or NULL.
 *	'name' to be valid.
 *	'nsec3param' to be valid.
 *	'diff' to be valid.
 */

isc_result_t
dns_nsec3_active(dns_db_t *db, dns_dbversion_t *version,
		 isc_boolean_t complete, isc_boolean_t *answer);
/*%<
 * Check if there are any complete/to be built NSEC3 chains.
 * If 'complete' is ISC_TRUE only complete chains will be recognized.
 *
 * Requires:
 *	'db' to be valid.
 *	'version' to be valid or NULL.
 *	'answer' to be non NULL.
 */

isc_result_t
dns_nsec3_maxiterations(dns_db_t *db, dns_dbversion_t *version,
			isc_mem_t *mctx, unsigned int *iterationsp);
/*%<
 * Find the maximum permissible number of iterations allowed based on
 * the key strength.
 *
 * Requires:
 *	'db' to be valid.
 *	'version' to be valid or NULL.
 *	'mctx' to be valid.
 *	'iterationsp' to be non NULL.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_NSEC3_H */
