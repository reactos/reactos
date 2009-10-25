/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: zt.h,v 1.38 2007/06/19 23:47:17 tbox Exp $ */

#ifndef DNS_ZT_H
#define DNS_ZT_H 1

/*! \file dns/zt.h */

#include <isc/lang.h>

#include <dns/types.h>

#define DNS_ZTFIND_NOEXACT		0x01

ISC_LANG_BEGINDECLS

isc_result_t
dns_zt_create(isc_mem_t *mctx, dns_rdataclass_t rdclass, dns_zt_t **zt);
/*%<
 * Creates a new zone table.
 *
 * Requires:
 * \li	'mctx' to be initialized.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS on success.
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_zt_mount(dns_zt_t *zt, dns_zone_t *zone);
/*%<
 * Mounts the zone on the zone table.
 *
 * Requires:
 * \li	'zt' to be valid
 * \li	'zone' to be valid
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_EXISTS
 * \li	#ISC_R_NOSPACE
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_zt_unmount(dns_zt_t *zt, dns_zone_t *zone);
/*%<
 * Unmount the given zone from the table.
 *
 * Requires:
 * 	'zt' to be valid
 * \li	'zone' to be valid
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_zt_find(dns_zt_t *zt, dns_name_t *name, unsigned int options,
	    dns_name_t *foundname, dns_zone_t **zone);
/*%<
 * Find the best match for 'name' in 'zt'.  If foundname is non NULL
 * then the name of the zone found is returned.
 *
 * Notes:
 * \li	If the DNS_ZTFIND_NOEXACT is set, the best partial match (if any)
 *	to 'name' will be returned.
 *
 * Requires:
 * \li	'zt' to be valid
 * \li	'name' to be valid
 * \li	'foundname' to be initialized and associated with a fixedname or NULL
 * \li	'zone' to be non NULL and '*zone' to be NULL
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_PARTIALMATCH
 * \li	#ISC_R_NOTFOUND
 * \li	#ISC_R_NOSPACE
 */

void
dns_zt_detach(dns_zt_t **ztp);
/*%<
 * Detach the given zonetable, if the reference count goes to zero the
 * zonetable will be freed.  In either case 'ztp' is set to NULL.
 *
 * Requires:
 * \li	'*ztp' to be valid
 */

void
dns_zt_flushanddetach(dns_zt_t **ztp);
/*%<
 * Detach the given zonetable, if the reference count goes to zero the
 * zonetable will be flushed and then freed.  In either case 'ztp' is
 * set to NULL.
 *
 * Requires:
 * \li	'*ztp' to be valid
 */

void
dns_zt_attach(dns_zt_t *zt, dns_zt_t **ztp);
/*%<
 * Attach 'zt' to '*ztp'.
 *
 * Requires:
 * \li	'zt' to be valid
 * \li	'*ztp' to be NULL
 */

isc_result_t
dns_zt_load(dns_zt_t *zt, isc_boolean_t stop);

isc_result_t
dns_zt_loadnew(dns_zt_t *zt, isc_boolean_t stop);
/*%<
 * Load all zones in the table.  If 'stop' is ISC_TRUE,
 * stop on the first error and return it.  If 'stop'
 * is ISC_FALSE, ignore errors.
 *
 * dns_zt_loadnew() only loads zones that are not yet loaded.
 * dns_zt_load() also loads zones that are already loaded and
 * and whose master file has changed since the last load.
 *
 * Requires:
 * \li	'zt' to be valid
 */

isc_result_t
dns_zt_freezezones(dns_zt_t *zt, isc_boolean_t freeze);
/*%<
 * Freeze/thaw updates to master zones.
 * Any pending updates will be flushed.
 * Zones will be reloaded on thaw.
 */

isc_result_t
dns_zt_apply(dns_zt_t *zt, isc_boolean_t stop,
	     isc_result_t (*action)(dns_zone_t *, void *), void *uap);

isc_result_t
dns_zt_apply2(dns_zt_t *zt, isc_boolean_t stop, isc_result_t *sub,
	      isc_result_t (*action)(dns_zone_t *, void *), void *uap);
/*%<
 * Apply a given 'action' to all zone zones in the table.
 * If 'stop' is 'ISC_TRUE' then walking the zone tree will stop if
 * 'action' does not return ISC_R_SUCCESS.
 *
 * Requires:
 * \li	'zt' to be valid.
 * \li	'action' to be non NULL.
 *
 * Returns:
 * \li	ISC_R_SUCCESS if action was applied to all nodes.  If 'stop' is
 *	ISC_FALSE and 'sub' is non NULL then the first error (if any)
 *	reported by 'action' is returned in '*sub';
 *	any error code from 'action'.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_ZT_H */
