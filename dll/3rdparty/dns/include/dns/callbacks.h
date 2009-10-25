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

/* $Id: callbacks.h,v 1.24 2007/06/19 23:47:16 tbox Exp $ */

#ifndef DNS_CALLBACKS_H
#define DNS_CALLBACKS_H 1

/*! \file dns/callbacks.h */

/***
 ***	Imports
 ***/

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/***
 ***	Types
 ***/

struct dns_rdatacallbacks {
	/*%
	 * dns_load_master calls this when it has rdatasets to commit.
	 */
	dns_addrdatasetfunc_t add;
	/*%
	 * dns_load_master / dns_rdata_fromtext call this to issue a error.
	 */
	void	(*error)(struct dns_rdatacallbacks *, const char *, ...);
	/*%
	 * dns_load_master / dns_rdata_fromtext call this to issue a warning.
	 */
	void	(*warn)(struct dns_rdatacallbacks *, const char *, ...);
	/*%
	 * Private data handles for use by the above callback functions.
	 */
	void	*add_private;
	void	*error_private;
	void	*warn_private;
};

/***
 ***	Initialization
 ***/

void
dns_rdatacallbacks_init(dns_rdatacallbacks_t *callbacks);
/*%<
 * Initialize 'callbacks'.
 *
 *
 * \li	'error' and 'warn' are set to default callbacks that print the
 *	error message through the DNS library log context.
 *
 *\li	All other elements are initialized to NULL.
 *
 * Requires:
 *  \li    'callbacks' is a valid dns_rdatacallbacks_t,
 */

void
dns_rdatacallbacks_init_stdio(dns_rdatacallbacks_t *callbacks);
/*%<
 * Like dns_rdatacallbacks_init, but logs to stdio.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_CALLBACKS_H */
