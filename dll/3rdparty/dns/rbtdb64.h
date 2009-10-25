/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: rbtdb64.h,v 1.17 2007/06/19 23:47:16 tbox Exp $ */

#ifndef DNS_RBTDB64_H
#define DNS_RBTDB64_H 1

#include <isc/lang.h>

/*****
 ***** Module Info
 *****/

/*! \file
 * \brief
 * DNS Red-Black Tree DB Implementation with 64-bit version numbers
 */

#include <dns/db.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_rbtdb64_create(isc_mem_t *mctx, dns_name_t *base, dns_dbtype_t type,
		   dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
		   void *driverarg, dns_db_t **dbp);

ISC_LANG_ENDDECLS

#endif /* DNS_RBTDB64_H */
