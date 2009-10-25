/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: keytable.h,v 1.16 2007/06/19 23:47:16 tbox Exp $ */

#ifndef DNS_KEYTABLE_H
#define DNS_KEYTABLE_H 1

/*****
 ***** Module Info
 *****/

/*! \file
 * \brief
 * The keytable module provides services for storing and retrieving DNSSEC
 * trusted keys, as well as the ability to find the deepest matching key
 * for a given domain name.
 *
 * MP:
 *\li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 */

#include <isc/lang.h>

#include <dns/types.h>

#include <dst/dst.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_keytable_create(isc_mem_t *mctx, dns_keytable_t **keytablep);
/*%<
 * Create a keytable.
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context.
 *
 *\li	keytablep != NULL && *keytablep == NULL
 *
 * Ensures:
 *
 *\li	On success, *keytablep is a valid, empty key table.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */


void
dns_keytable_attach(dns_keytable_t *source, dns_keytable_t **targetp);
/*%<
 * Attach *targetp to source.
 *
 * Requires:
 *
 *\li	'source' is a valid keytable.
 *
 *\li	'targetp' points to a NULL dns_keytable_t *.
 *
 * Ensures:
 *
 *\li	*targetp is attached to source.
 */

void
dns_keytable_detach(dns_keytable_t **keytablep);
/*%<
 * Detach *keytablep from its keytable.
 *
 * Requires:
 *
 *\li	'keytablep' points to a valid keytable.
 *
 * Ensures:
 *
 *\li	*keytablep is NULL.
 *
 *\li	If '*keytablep' is the last reference to the keytable,
 *		all resources used by the keytable will be freed
 */

isc_result_t
dns_keytable_add(dns_keytable_t *keytable, dst_key_t **keyp);
/*%<
 * Add '*keyp' to 'keytable'.
 *
 * Notes:
 *
 *\li	Ownership of *keyp is transferred to the keytable.
 *
 * Requires:
 *
 *\li	keyp != NULL && *keyp is a valid dst_key_t *.
 *
 * Ensures:
 *
 *\li	On success, *keyp == NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */

isc_result_t
dns_keytable_findkeynode(dns_keytable_t *keytable, dns_name_t *name,
			 dns_secalg_t algorithm, dns_keytag_t tag,
			 dns_keynode_t **keynodep);
/*%<
 * Search for a key named 'name', matching 'algorithm' and 'tag' in
 * 'keytable'.  This finds the first instance which matches.  Use
 * dns_keytable_findnextkeynode() to find other instances.
 *
 * Requires:
 *
 *\li	'keytable' is a valid keytable.
 *
 *\li	'name' is a valid absolute name.
 *
 *\li	keynodep != NULL && *keynodep == NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_PARTIALMATCH	the name existed in the keytable.
 *\li	ISC_R_NOTFOUND
 *
 *\li	Any other result indicates an error.
 */

isc_result_t
dns_keytable_findnextkeynode(dns_keytable_t *keytable, dns_keynode_t *keynode,
		                             dns_keynode_t **nextnodep);
/*%<
 * Search for the next key with the same properties as 'keynode' in
 * 'keytable' as found by dns_keytable_findkeynode().
 *
 * Requires:
 *
 *\li	'keytable' is a valid keytable.
 *
 *\li	'keynode' is a valid keynode.
 *
 *\li	nextnodep != NULL && *nextnodep == NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOTFOUND
 *
 *\li	Any other result indicates an error.
 */

isc_result_t
dns_keytable_finddeepestmatch(dns_keytable_t *keytable, dns_name_t *name,
			      dns_name_t *foundname);
/*%<
 * Search for the deepest match of 'name' in 'keytable'.
 *
 * Requires:
 *
 *\li	'keytable' is a valid keytable.
 *
 *\li	'name' is a valid absolute name.
 *
 *\li	'foundname' is a name with a dedicated buffer.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOTFOUND
 *
 *\li	Any other result indicates an error.
 */

void
dns_keytable_detachkeynode(dns_keytable_t *keytable,
			   dns_keynode_t **keynodep);
/*%<
 * Give back a keynode found via dns_keytable_findkeynode().
 *
 * Requires:
 *
 *\li	'keytable' is a valid keytable.
 *
 *\li	*keynodep is a valid keynode returned by a call to
 *	dns_keytable_findkeynode().
 *
 * Ensures:
 *
 *\li	*keynodep == NULL
 */

isc_result_t
dns_keytable_issecuredomain(dns_keytable_t *keytable, dns_name_t *name,
			    isc_boolean_t *wantdnssecp);
/*%<
 * Is 'name' at or beneath a trusted key?
 *
 * Requires:
 *
 *\li	'keytable' is a valid keytable.
 *
 *\li	'name' is a valid absolute name.
 *
 *\li	'*wantsdnssecp' is a valid isc_boolean_t.
 *
 * Ensures:
 *
 *\li	On success, *wantsdnssecp will be ISC_TRUE if and only if 'name'
 *	is at or beneath a trusted key.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result is an error.
 */

dst_key_t *
dns_keynode_key(dns_keynode_t *keynode);
/*%<
 * Get the DST key associated with keynode.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_KEYTABLE_H */
