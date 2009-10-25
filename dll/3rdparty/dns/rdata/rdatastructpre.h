/*
 * Copyright (C) 2004, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: rdatastructpre.h,v 1.16 2007/06/19 23:47:17 tbox Exp $ */

#ifndef DNS_RDATASTRUCT_H
#define DNS_RDATASTRUCT_H 1

#include <isc/lang.h>
#include <isc/sockaddr.h>

#include <dns/name.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

typedef struct dns_rdatacommon {
	dns_rdataclass_t			rdclass;
	dns_rdatatype_t				rdtype;
	ISC_LINK(struct dns_rdatacommon)	link;
} dns_rdatacommon_t;

#define DNS_RDATACOMMON_INIT(_data, _rdtype, _rdclass) \
	do { \
		(_data)->common.rdtype = (_rdtype); \
		(_data)->common.rdclass = (_rdclass); \
		ISC_LINK_INIT(&(_data)->common, link); \
	} while (0)
