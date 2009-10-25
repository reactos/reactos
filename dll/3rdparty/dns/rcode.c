/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2003  Internet Software Consortium.
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

/* $Id: rcode.c,v 1.8 2008/09/25 04:02:38 tbox Exp $ */

#include <config.h>
#include <ctype.h>

#include <isc/buffer.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/stdio.h>
#include <isc/stdlib.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/cert.h>
#include <dns/keyflags.h>
#include <dns/keyvalues.h>
#include <dns/rcode.h>
#include <dns/rdataclass.h>
#include <dns/result.h>
#include <dns/secalg.h>
#include <dns/secproto.h>

#define RETERR(x) \
	do { \
		isc_result_t _r = (x); \
		if (_r != ISC_R_SUCCESS) \
			return (_r); \
	} while (0)

#define NUMBERSIZE sizeof("037777777777") /* 2^32-1 octal + NUL */

#define RCODENAMES \
	/* standard rcodes */ \
	{ dns_rcode_noerror, "NOERROR", 0}, \
	{ dns_rcode_formerr, "FORMERR", 0}, \
	{ dns_rcode_servfail, "SERVFAIL", 0}, \
	{ dns_rcode_nxdomain, "NXDOMAIN", 0}, \
	{ dns_rcode_notimp, "NOTIMP", 0}, \
	{ dns_rcode_refused, "REFUSED", 0}, \
	{ dns_rcode_yxdomain, "YXDOMAIN", 0}, \
	{ dns_rcode_yxrrset, "YXRRSET", 0}, \
	{ dns_rcode_nxrrset, "NXRRSET", 0}, \
	{ dns_rcode_notauth, "NOTAUTH", 0}, \
	{ dns_rcode_notzone, "NOTZONE", 0},

#define ERCODENAMES \
	/* extended rcodes */ \
	{ dns_rcode_badvers, "BADVERS", 0}, \
	{ 0, NULL, 0 }

#define TSIGRCODENAMES \
	/* extended rcodes */ \
	{ dns_tsigerror_badsig, "BADSIG", 0}, \
	{ dns_tsigerror_badkey, "BADKEY", 0}, \
	{ dns_tsigerror_badtime, "BADTIME", 0}, \
	{ dns_tsigerror_badmode, "BADMODE", 0}, \
	{ dns_tsigerror_badname, "BADNAME", 0}, \
	{ dns_tsigerror_badalg, "BADALG", 0}, \
	{ dns_tsigerror_badtrunc, "BADTRUNC", 0}, \
	{ 0, NULL, 0 }

/* RFC2538 section 2.1 */

#define CERTNAMES \
	{ 1, "PKIX", 0}, \
	{ 2, "SPKI", 0}, \
	{ 3, "PGP", 0}, \
	{ 253, "URI", 0}, \
	{ 254, "OID", 0}, \
	{ 0, NULL, 0}

/* RFC2535 section 7, RFC3110 */

#define SECALGNAMES \
	{ DNS_KEYALG_RSAMD5, "RSAMD5", 0 }, \
	{ DNS_KEYALG_RSAMD5, "RSA", 0 }, \
	{ DNS_KEYALG_DH, "DH", 0 }, \
	{ DNS_KEYALG_DSA, "DSA", 0 }, \
	{ DNS_KEYALG_NSEC3DSA, "NSEC3DSA", 0 }, \
	{ DNS_KEYALG_ECC, "ECC", 0 }, \
	{ DNS_KEYALG_RSASHA1, "RSASHA1", 0 }, \
	{ DNS_KEYALG_NSEC3RSASHA1, "NSEC3RSASHA1", 0 }, \
	{ DNS_KEYALG_INDIRECT, "INDIRECT", 0 }, \
	{ DNS_KEYALG_PRIVATEDNS, "PRIVATEDNS", 0 }, \
	{ DNS_KEYALG_PRIVATEOID, "PRIVATEOID", 0 }, \
	{ 0, NULL, 0}

/* RFC2535 section 7.1 */

#define SECPROTONAMES \
	{   0,    "NONE", 0 }, \
	{   1,    "TLS", 0 }, \
	{   2,    "EMAIL", 0 }, \
	{   3,    "DNSSEC", 0 }, \
	{   4,    "IPSEC", 0 }, \
	{ 255,    "ALL", 0 }, \
	{ 0, NULL, 0}

#define HASHALGNAMES \
	{ 1, "SHA-1", 0 }, \
	{ 0, NULL, 0 }

struct tbl {
	unsigned int    value;
	const char      *name;
	int             flags;
};

static struct tbl rcodes[] = { RCODENAMES ERCODENAMES };
static struct tbl tsigrcodes[] = { RCODENAMES TSIGRCODENAMES };
static struct tbl certs[] = { CERTNAMES };
static struct tbl secalgs[] = { SECALGNAMES };
static struct tbl secprotos[] = { SECPROTONAMES };
static struct tbl hashalgs[] = { HASHALGNAMES };

static struct keyflag {
	const char *name;
	unsigned int value;
	unsigned int mask;
} keyflags[] = {
	{ "NOCONF", 0x4000, 0xC000 },
	{ "NOAUTH", 0x8000, 0xC000 },
	{ "NOKEY",  0xC000, 0xC000 },
	{ "FLAG2",  0x2000, 0x2000 },
	{ "EXTEND", 0x1000, 0x1000 },
	{ "FLAG4",  0x0800, 0x0800 },
	{ "FLAG5",  0x0400, 0x0400 },
	{ "USER",   0x0000, 0x0300 },
	{ "ZONE",   0x0100, 0x0300 },
	{ "HOST",   0x0200, 0x0300 },
	{ "NTYP3",  0x0300, 0x0300 },
	{ "FLAG8",  0x0080, 0x0080 },
	{ "FLAG9",  0x0040, 0x0040 },
	{ "FLAG10", 0x0020, 0x0020 },
	{ "FLAG11", 0x0010, 0x0010 },
	{ "SIG0",   0x0000, 0x000F },
	{ "SIG1",   0x0001, 0x000F },
	{ "SIG2",   0x0002, 0x000F },
	{ "SIG3",   0x0003, 0x000F },
	{ "SIG4",   0x0004, 0x000F },
	{ "SIG5",   0x0005, 0x000F },
	{ "SIG6",   0x0006, 0x000F },
	{ "SIG7",   0x0007, 0x000F },
	{ "SIG8",   0x0008, 0x000F },
	{ "SIG9",   0x0009, 0x000F },
	{ "SIG10",  0x000A, 0x000F },
	{ "SIG11",  0x000B, 0x000F },
	{ "SIG12",  0x000C, 0x000F },
	{ "SIG13",  0x000D, 0x000F },
	{ "SIG14",  0x000E, 0x000F },
	{ "SIG15",  0x000F, 0x000F },
	{ "KSK",  DNS_KEYFLAG_KSK, DNS_KEYFLAG_KSK },
	{ NULL,     0, 0 }
};

static isc_result_t
str_totext(const char *source, isc_buffer_t *target) {
	unsigned int l;
	isc_region_t region;

	isc_buffer_availableregion(target, &region);
	l = strlen(source);

	if (l > region.length)
		return (ISC_R_NOSPACE);

	memcpy(region.base, source, l);
	isc_buffer_add(target, l);
	return (ISC_R_SUCCESS);
}

static isc_result_t
maybe_numeric(unsigned int *valuep, isc_textregion_t *source,
	      unsigned int max, isc_boolean_t hex_allowed)
{
	isc_result_t result;
	isc_uint32_t n;
	char buffer[NUMBERSIZE];

	if (! isdigit(source->base[0] & 0xff) ||
	    source->length > NUMBERSIZE - 1)
		return (ISC_R_BADNUMBER);

	/*
	 * We have a potential number.  Try to parse it with
	 * isc_parse_uint32().  isc_parse_uint32() requires
	 * null termination, so we must make a copy.
	 */
	strncpy(buffer, source->base, NUMBERSIZE);
	INSIST(buffer[source->length] == '\0');

	result = isc_parse_uint32(&n, buffer, 10);
	if (result == ISC_R_BADNUMBER && hex_allowed)
		result = isc_parse_uint32(&n, buffer, 16);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (n > max)
		return (ISC_R_RANGE);
	*valuep = n;
	return (ISC_R_SUCCESS);
}

static isc_result_t
dns_mnemonic_fromtext(unsigned int *valuep, isc_textregion_t *source,
		      struct tbl *table, unsigned int max)
{
	isc_result_t result;
	int i;

	result = maybe_numeric(valuep, source, max, ISC_FALSE);
	if (result != ISC_R_BADNUMBER)
		return (result);

	for (i = 0; table[i].name != NULL; i++) {
		unsigned int n;
		n = strlen(table[i].name);
		if (n == source->length &&
		    strncasecmp(source->base, table[i].name, n) == 0) {
			*valuep = table[i].value;
			return (ISC_R_SUCCESS);
		}
	}
	return (DNS_R_UNKNOWN);
}

static isc_result_t
dns_mnemonic_totext(unsigned int value, isc_buffer_t *target,
		    struct tbl *table)
{
	int i = 0;
	char buf[sizeof("4294967296")];
	while (table[i].name != NULL) {
		if (table[i].value == value) {
			return (str_totext(table[i].name, target));
		}
		i++;
	}
	snprintf(buf, sizeof(buf), "%u", value);
	return (str_totext(buf, target));
}

isc_result_t
dns_rcode_fromtext(dns_rcode_t *rcodep, isc_textregion_t *source) {
	unsigned int value;
	RETERR(dns_mnemonic_fromtext(&value, source, rcodes, 0xffff));
	*rcodep = value;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_rcode_totext(dns_rcode_t rcode, isc_buffer_t *target) {
	return (dns_mnemonic_totext(rcode, target, rcodes));
}

isc_result_t
dns_tsigrcode_fromtext(dns_rcode_t *rcodep, isc_textregion_t *source) {
	unsigned int value;
	RETERR(dns_mnemonic_fromtext(&value, source, tsigrcodes, 0xffff));
	*rcodep = value;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_tsigrcode_totext(dns_rcode_t rcode, isc_buffer_t *target) {
	return (dns_mnemonic_totext(rcode, target, tsigrcodes));
}

isc_result_t
dns_cert_fromtext(dns_cert_t *certp, isc_textregion_t *source) {
	unsigned int value;
	RETERR(dns_mnemonic_fromtext(&value, source, certs, 0xffff));
	*certp = value;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_cert_totext(dns_cert_t cert, isc_buffer_t *target) {
	return (dns_mnemonic_totext(cert, target, certs));
}

isc_result_t
dns_secalg_fromtext(dns_secalg_t *secalgp, isc_textregion_t *source) {
	unsigned int value;
	RETERR(dns_mnemonic_fromtext(&value, source, secalgs, 0xff));
	*secalgp = value;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_secalg_totext(dns_secalg_t secalg, isc_buffer_t *target) {
	return (dns_mnemonic_totext(secalg, target, secalgs));
}

isc_result_t
dns_secproto_fromtext(dns_secproto_t *secprotop, isc_textregion_t *source) {
	unsigned int value;
	RETERR(dns_mnemonic_fromtext(&value, source, secprotos, 0xff));
	*secprotop = value;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_secproto_totext(dns_secproto_t secproto, isc_buffer_t *target) {
	return (dns_mnemonic_totext(secproto, target, secprotos));
}

isc_result_t
dns_hashalg_fromtext(unsigned char *hashalg, isc_textregion_t *source) {
	unsigned int value;
	RETERR(dns_mnemonic_fromtext(&value, source, hashalgs, 0xff));
	*hashalg = value;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_keyflags_fromtext(dns_keyflags_t *flagsp, isc_textregion_t *source)
{
	isc_result_t result;
	char *text, *end;
	unsigned int value, mask;

	result = maybe_numeric(&value, source, 0xffff, ISC_TRUE);
	if (result == ISC_R_SUCCESS) {
		*flagsp = value;
		return (ISC_R_SUCCESS);
	}
	if (result != ISC_R_BADNUMBER)
		return (result);

	text = source->base;
	end = source->base + source->length;
	value = mask = 0;

	while (text < end) {
		struct keyflag *p;
		unsigned int len;
		char *delim = memchr(text, '|', end - text);
		if (delim != NULL)
			len = delim - text;
		else
			len = end - text;
		for (p = keyflags; p->name != NULL; p++) {
			if (strncasecmp(p->name, text, len) == 0)
				break;
		}
		if (p->name == NULL)
			return (DNS_R_UNKNOWNFLAG);
		value |= p->value;
#ifdef notyet
		if ((mask & p->mask) != 0)
			warn("overlapping key flags");
#endif
		mask |= p->mask;
		text += len;
		if (delim != NULL)
			text++; /* Skip "|" */
	}
	*flagsp = value;
	return (ISC_R_SUCCESS);
}

/*
 * This uses lots of hard coded values, but how often do we actually
 * add classes?
 */
isc_result_t
dns_rdataclass_fromtext(dns_rdataclass_t *classp, isc_textregion_t *source) {
#define COMPARE(string, rdclass) \
	if (((sizeof(string) - 1) == source->length) \
	    && (strncasecmp(source->base, string, source->length) == 0)) { \
		*classp = rdclass; \
		return (ISC_R_SUCCESS); \
	}

	switch (tolower((unsigned char)source->base[0])) {
	case 'a':
		COMPARE("any", dns_rdataclass_any);
		break;
	case 'c':
		/*
		 * RFC1035 says the mnemonic for the CHAOS class is CH,
		 * but historical BIND practice is to call it CHAOS.
		 * We will accept both forms, but only generate CH.
		 */
		COMPARE("ch", dns_rdataclass_chaos);
		COMPARE("chaos", dns_rdataclass_chaos);

		if (source->length > 5 &&
		    source->length < (5 + sizeof("65000")) &&
		    strncasecmp("class", source->base, 5) == 0) {
			char buf[sizeof("65000")];
			char *endp;
			unsigned int val;

			strncpy(buf, source->base + 5, source->length - 5);
			buf[source->length - 5] = '\0';
			val = strtoul(buf, &endp, 10);
			if (*endp == '\0' && val <= 0xffff) {
				*classp = (dns_rdataclass_t)val;
				return (ISC_R_SUCCESS);
			}
		}
		break;
	case 'h':
		COMPARE("hs", dns_rdataclass_hs);
		COMPARE("hesiod", dns_rdataclass_hs);
		break;
	case 'i':
		COMPARE("in", dns_rdataclass_in);
		break;
	case 'n':
		COMPARE("none", dns_rdataclass_none);
		break;
	case 'r':
		COMPARE("reserved0", dns_rdataclass_reserved0);
		break;
	}

#undef COMPARE

	return (DNS_R_UNKNOWN);
}

isc_result_t
dns_rdataclass_totext(dns_rdataclass_t rdclass, isc_buffer_t *target) {
	char buf[sizeof("CLASS65535")];

	switch (rdclass) {
	case dns_rdataclass_any:
		return (str_totext("ANY", target));
	case dns_rdataclass_chaos:
		return (str_totext("CH", target));
	case dns_rdataclass_hs:
		return (str_totext("HS", target));
	case dns_rdataclass_in:
		return (str_totext("IN", target));
	case dns_rdataclass_none:
		return (str_totext("NONE", target));
	case dns_rdataclass_reserved0:
		return (str_totext("RESERVED0", target));
	default:
		snprintf(buf, sizeof(buf), "CLASS%u", rdclass);
		return (str_totext(buf, target));
	}
}

void
dns_rdataclass_format(dns_rdataclass_t rdclass,
		      char *array, unsigned int size)
{
	isc_result_t result;
	isc_buffer_t buf;

	isc_buffer_init(&buf, array, size);
	result = dns_rdataclass_totext(rdclass, &buf);
	/*
	 * Null terminate.
	 */
	if (result == ISC_R_SUCCESS) {
		if (isc_buffer_availablelength(&buf) >= 1)
			isc_buffer_putuint8(&buf, 0);
		else
			result = ISC_R_NOSPACE;
	}
	if (result != ISC_R_SUCCESS) {
		snprintf(array, size, "<unknown>");
		array[size - 1] = '\0';
	}
}
