/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: rdatalist_p.h,v 1.11 2008/09/25 04:02:38 tbox Exp $ */

#ifndef DNS_RDATALIST_P_H
#define DNS_RDATALIST_P_H

/*! \file */

#include <isc/result.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

void
isc__rdatalist_disassociate(dns_rdataset_t *rdatasetp);

isc_result_t
isc__rdatalist_first(dns_rdataset_t *rdataset);

isc_result_t
isc__rdatalist_next(dns_rdataset_t *rdataset);

void
isc__rdatalist_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata);

void
isc__rdatalist_clone(dns_rdataset_t *source, dns_rdataset_t *target);

unsigned int
isc__rdatalist_count(dns_rdataset_t *rdataset);

isc_result_t
isc__rdatalist_addnoqname(dns_rdataset_t *rdataset, dns_name_t *name);

isc_result_t
isc__rdatalist_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
			  dns_rdataset_t *neg, dns_rdataset_t *negsig);

isc_result_t
isc__rdatalist_addclosest(dns_rdataset_t *rdataset, dns_name_t *name);

isc_result_t
isc__rdatalist_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
			  dns_rdataset_t *neg, dns_rdataset_t *negsig);

ISC_LANG_ENDDECLS

#endif /* DNS_RDATALIST_P_H */
