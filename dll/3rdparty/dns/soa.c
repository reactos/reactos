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

/* $Id: soa.c,v 1.8 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/util.h>

#include <dns/rdata.h>
#include <dns/soa.h>

static inline isc_uint32_t
decode_uint32(unsigned char *p) {
	return ((p[0] << 24) +
		(p[1] << 16) +
		(p[2] <<  8) +
		(p[3] <<  0));
}

static inline void
encode_uint32(isc_uint32_t val, unsigned char *p) {
	p[0] = (isc_uint8_t)(val >> 24);
	p[1] = (isc_uint8_t)(val >> 16);
	p[2] = (isc_uint8_t)(val >>  8);
	p[3] = (isc_uint8_t)(val >>  0);
}

static isc_uint32_t
soa_get(dns_rdata_t *rdata, int offset) {
	INSIST(rdata->type == dns_rdatatype_soa);
	/*
	 * Locate the field within the SOA RDATA based
	 * on its position relative to the end of the data.
	 *
	 * This is a bit of a kludge, but the alternative approach of
	 * using dns_rdata_tostruct() and dns_rdata_fromstruct() would
	 * involve a lot of unnecessary work (like building domain
	 * names and allocating temporary memory) when all we really
	 * want to do is to get 32 bits of fixed-sized data.
	 */
	INSIST(rdata->length >= 20);
	INSIST(offset >= 0 && offset <= 16);
	return (decode_uint32(rdata->data + rdata->length - 20 + offset));
}

isc_uint32_t
dns_soa_getserial(dns_rdata_t *rdata) {
	return soa_get(rdata, 0);
}
isc_uint32_t
dns_soa_getrefresh(dns_rdata_t *rdata) {
	return soa_get(rdata, 4);
}
isc_uint32_t
dns_soa_getretry(dns_rdata_t *rdata) {
	return soa_get(rdata, 8);
}
isc_uint32_t
dns_soa_getexpire(dns_rdata_t *rdata) {
	return soa_get(rdata, 12);
}
isc_uint32_t
dns_soa_getminimum(dns_rdata_t *rdata) {
	return soa_get(rdata, 16);
}

static void
soa_set(dns_rdata_t *rdata, isc_uint32_t val, int offset) {
	INSIST(rdata->type == dns_rdatatype_soa);
	INSIST(rdata->length >= 20);
	INSIST(offset >= 0 && offset <= 16);
	encode_uint32(val, rdata->data + rdata->length - 20 + offset);
}

void
dns_soa_setserial(isc_uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 0);
}
void
dns_soa_setrefresh(isc_uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 4);
}
void
dns_soa_setretry(isc_uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 8);
}
void
dns_soa_setexpire(isc_uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 12);
}
void
dns_soa_setminimum(isc_uint32_t val, dns_rdata_t *rdata) {
	soa_set(rdata, val, 16);
}
