/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001, 2003  Internet Software Consortium.
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

/* $Id: zonekey.c,v 1.9 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/keyvalues.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/types.h>
#include <dns/zonekey.h>

isc_boolean_t
dns_zonekey_iszonekey(dns_rdata_t *keyrdata) {
	isc_result_t result;
	dns_rdata_dnskey_t key;
	isc_boolean_t iszonekey = ISC_TRUE;

	REQUIRE(keyrdata != NULL);

	result = dns_rdata_tostruct(keyrdata, &key, NULL);
	if (result != ISC_R_SUCCESS)
		return (ISC_FALSE);

	if ((key.flags & DNS_KEYTYPE_NOAUTH) != 0)
		iszonekey = ISC_FALSE;
	if ((key.flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE)
		iszonekey = ISC_FALSE;
	if (key.protocol != DNS_KEYPROTO_DNSSEC &&
	key.protocol != DNS_KEYPROTO_ANY)
		iszonekey = ISC_FALSE;
	
	return (iszonekey);
}
