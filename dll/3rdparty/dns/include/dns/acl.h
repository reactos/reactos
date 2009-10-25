/*
 * Copyright (C) 2004-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: acl.h,v 1.31.206.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_ACL_H
#define DNS_ACL_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/acl.h
 * \brief
 * Address match list handling.
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/netaddr.h>
#include <isc/refcount.h>

#include <dns/name.h>
#include <dns/types.h>
#include <dns/iptable.h>

/***
 *** Types
 ***/

typedef enum {
	dns_aclelementtype_ipprefix,
	dns_aclelementtype_keyname,
	dns_aclelementtype_nestedacl,
	dns_aclelementtype_localhost,
	dns_aclelementtype_localnets,
	dns_aclelementtype_any
} dns_aclelemettype_t;

typedef struct dns_aclipprefix dns_aclipprefix_t;

struct dns_aclipprefix {
	isc_netaddr_t address; /* IP4/IP6 */
	unsigned int prefixlen;
};

struct dns_aclelement {
	dns_aclelemettype_t	type;
	isc_boolean_t		negative;
	dns_name_t		keyname;
	dns_acl_t		*nestedacl;
	int			node_num;
};

struct dns_acl {
	unsigned int		magic;
	isc_mem_t		*mctx;
	isc_refcount_t		refcount;
	dns_iptable_t		*iptable;
#define node_count		iptable->radix->num_added_node
	dns_aclelement_t	*elements;
	isc_boolean_t 		has_negatives;
	unsigned int 		alloc;		/*%< Elements allocated */
	unsigned int 		length;		/*%< Elements initialized */
	char 			*name;		/*%< Temporary use only */
	ISC_LINK(dns_acl_t) 	nextincache;	/*%< Ditto */
};

struct dns_aclenv {
	dns_acl_t *localhost;
	dns_acl_t *localnets;
	isc_boolean_t match_mapped;
};

#define DNS_ACL_MAGIC		ISC_MAGIC('D','a','c','l')
#define DNS_ACL_VALID(a)	ISC_MAGIC_VALID(a, DNS_ACL_MAGIC)

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
dns_acl_create(isc_mem_t *mctx, int n, dns_acl_t **target);
/*%<
 * Create a new ACL, including an IP table and an array with room
 * for 'n' ACL elements.  The elements are uninitialized and the
 * length is 0.
 */

isc_result_t
dns_acl_any(isc_mem_t *mctx, dns_acl_t **target);
/*%<
 * Create a new ACL that matches everything.
 */

isc_result_t
dns_acl_none(isc_mem_t *mctx, dns_acl_t **target);
/*%<
 * Create a new ACL that matches nothing.
 */

isc_boolean_t
dns_acl_isany(dns_acl_t *acl);
/*%<
 * Test whether ACL is set to "{ any; }"
 */

isc_boolean_t
dns_acl_isnone(dns_acl_t *acl);
/*%<
 * Test whether ACL is set to "{ none; }"
 */

isc_result_t
dns_acl_merge(dns_acl_t *dest, dns_acl_t *source, isc_boolean_t pos);
/*%<
 * Merge the contents of one ACL into another.  Call dns_iptable_merge()
 * for the IP tables, then concatenate the element arrays.
 *
 * If pos is set to false, then the nested ACL is to be negated.  This
 * means reverse the sense of each *positive* element or IP table node,
 * but leave negatives alone, so as to prevent a double-negative causing
 * an unexpected positive match in the parent ACL.
 */

void
dns_acl_attach(dns_acl_t *source, dns_acl_t **target);

void
dns_acl_detach(dns_acl_t **aclp);

isc_boolean_t
dns_acl_isinsecure(const dns_acl_t *a);
/*%<
 * Return #ISC_TRUE iff the acl 'a' is considered insecure, that is,
 * if it contains IP addresses other than those of the local host.
 * This is intended for applications such as printing warning
 * messages for suspect ACLs; it is not intended for making access
 * control decisions.  We make no guarantee that an ACL for which
 * this function returns #ISC_FALSE is safe.
 */

isc_result_t
dns_aclenv_init(isc_mem_t *mctx, dns_aclenv_t *env);
/*%<
 * Initialize ACL environment, setting up localhost and localnets ACLs
 */

void
dns_aclenv_copy(dns_aclenv_t *t, dns_aclenv_t *s);

void
dns_aclenv_destroy(dns_aclenv_t *env);

isc_result_t
dns_acl_match(const isc_netaddr_t *reqaddr,
	      const dns_name_t *reqsigner,
	      const dns_acl_t *acl,
	      const dns_aclenv_t *env,
	      int *match,
	      const dns_aclelement_t **matchelt);
/*%<
 * General, low-level ACL matching.  This is expected to
 * be useful even for weird stuff like the topology and sortlist statements.
 *
 * Match the address 'reqaddr', and optionally the key name 'reqsigner',
 * against 'acl'.  'reqsigner' may be NULL.
 *
 * If there is a match, '*match' will be set to an integer whose absolute
 * value corresponds to the order in which the matching value was inserted
 * into the ACL.  For a positive match, this value will be positive; for a
 * negative match, it will be negative.
 *
 * If there is no match, *match will be set to zero.
 *
 * If there is a match in the element list (either positive or negative)
 * and 'matchelt' is non-NULL, *matchelt will be pointed to the matching
 * element.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		Always succeeds.
 */

isc_boolean_t
dns_aclelement_match(const isc_netaddr_t *reqaddr,
		     const dns_name_t *reqsigner,
		     const dns_aclelement_t *e,
		     const dns_aclenv_t *env,
		     const dns_aclelement_t **matchelt);
/*%<
 * Like dns_acl_match, but matches against the single ACL element 'e'
 * rather than a complete ACL, and returns ISC_TRUE iff it matched.
 *
 * To determine whether the match was positive or negative, the
 * caller should examine e->negative.  Since the element 'e' may be
 * a reference to a named ACL or a nested ACL, a matching element
 * returned through 'matchelt' is not necessarily 'e' itself.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_ACL_H */
