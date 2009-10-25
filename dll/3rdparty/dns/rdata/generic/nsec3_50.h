/*
 * Copyright (C) 2008  Internet Systems Consortium, Inc. ("ISC")
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


#ifndef GENERIC_NSEC3_50_H
#define GENERIC_NSEC3_50_H 1

/* $Id: nsec3_50.h,v 1.4 2008/09/25 04:02:39 tbox Exp $ */

/*!
 * \brief Per RFC 5155 */

#include <isc/iterated_hash.h>

typedef struct dns_rdata_nsec3 {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	dns_hash_t		hash;
	unsigned char		flags;
	dns_iterations_t	iterations;
	unsigned char		salt_length;
	unsigned char		next_length;
	isc_uint16_t		len;
	unsigned char		*salt;
	unsigned char		*next;
	unsigned char		*typebits;
} dns_rdata_nsec3_t;

/*
 * The corresponding NSEC3 interval is OPTOUT indicating possible
 * insecure delegations.
 */
#define DNS_NSEC3FLAG_OPTOUT 0x01U

/*%
 * Non-standard, NSEC3PARAM only.
 *
 * Create a corresponding NSEC3 chain.
 * Once the NSEC3 chain is complete this flag will be removed to signal
 * that there is a complete chain.
 *
 * This flag is automatically set when a NSEC3PARAM record is added to
 * the zone via UPDATE.
 *
 * NSEC3PARAM records with this flag set are supposed to be ignored by
 * RFC 5155 compliant nameservers.
 */
#define DNS_NSEC3FLAG_CREATE 0x80U

/*%
 * Non-standard, NSEC3PARAM only.
 *
 * The corresponding NSEC3 set is to be removed once the NSEC chain
 * has been generated.
 *
 * This flag is automatically set when the last active NSEC3PARAM record
 * is removed from the zone via UPDATE.
 *
 * NSEC3PARAM records with this flag set are supposed to be ignored by
 * RFC 5155 compliant nameservers.
 */
#define DNS_NSEC3FLAG_REMOVE 0x40U

/*%
 * Non-standard, NSEC3PARAM only.
 *
 * Used to identify NSEC3PARAM records added in this UPDATE request.
 */
#define DNS_NSEC3FLAG_UPDATE 0x20U

/*%
 * Non-standard, NSEC3PARAM only.
 *
 * Prevent the creation of a NSEC chain before the last NSEC3 chain
 * is removed.  This will normally only be set when the zone is
 * transitioning from secure with NSEC3 chains to insecure.
 */
#define DNS_NSEC3FLAG_NONSEC 0x10U

#endif /* GENERIC_NSEC3_50_H */
