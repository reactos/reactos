/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: result.c,v 1.125 2008/09/25 04:02:38 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/once.h>
#include <isc/util.h>

#include <dns/result.h>
#include <dns/lib.h>

static const char *text[DNS_R_NRESULTS] = {
	"label too long",		       /*%< 0 DNS_R_LABELTOOLONG */
	"bad escape",			       /*%< 1 DNS_R_BADESCAPE */
	/*!
	 * Note that DNS_R_BADBITSTRING and DNS_R_BITSTRINGTOOLONG are
	 * deprecated.
	 */
	"bad bitstring",		       /*%< 2 DNS_R_BADBITSTRING */
	"bitstring too long",		       /*%< 3 DNS_R_BITSTRINGTOOLONG */
	"empty label",			       /*%< 4 DNS_R_EMPTYLABEL */

	"bad dotted quad",		       /*%< 5 DNS_R_BADDOTTEDQUAD */
	"invalid NS owner name (wildcard)",    /*%< 6 DNS_R_INVALIDNS */
	"unknown class/type",		       /*%< 7 DNS_R_UNKNOWN */
	"bad label type",		       /*%< 8 DNS_R_BADLABELTYPE */
	"bad compression pointer",	       /*%< 9 DNS_R_BADPOINTER */

	"too many hops",		       /*%< 10 DNS_R_TOOMANYHOPS */
	"disallowed (by application policy)",  /*%< 11 DNS_R_DISALLOWED */
	"extra input text",		       /*%< 12 DNS_R_EXTRATOKEN */
	"extra input data",		       /*%< 13 DNS_R_EXTRADATA */
	"text too long",		       /*%< 14 DNS_R_TEXTTOOLONG */

	"not at top of zone",		       /*%< 15 DNS_R_NOTZONETOP */
	"syntax error",			       /*%< 16 DNS_R_SYNTAX */
	"bad checksum",			       /*%< 17 DNS_R_BADCKSUM */
	"bad IPv6 address",		       /*%< 18 DNS_R_BADAAAA */
	"no owner",			       /*%< 19 DNS_R_NOOWNER */

	"no ttl",			       /*%< 20 DNS_R_NOTTL */
	"bad class",			       /*%< 21 DNS_R_BADCLASS */
	"name too long",		       /*%< 22 DNS_R_NAMETOOLONG */
	"partial match",		       /*%< 23 DNS_R_PARTIALMATCH */
	"new origin",			       /*%< 24 DNS_R_NEWORIGIN */

	"unchanged",			       /*%< 25 DNS_R_UNCHANGED */
	"bad ttl",			       /*%< 26 DNS_R_BADTTL */
	"more data needed/to be rendered",     /*%< 27 DNS_R_NOREDATA */
	"continue",			       /*%< 28 DNS_R_CONTINUE */
	"delegation",			       /*%< 29 DNS_R_DELEGATION */

	"glue",				       /*%< 30 DNS_R_GLUE */
	"dname",			       /*%< 31 DNS_R_DNAME */
	"cname",			       /*%< 32 DNS_R_CNAME */
	"bad database",			       /*%< 33 DNS_R_BADDB */
	"zonecut",			       /*%< 34 DNS_R_ZONECUT */

	"bad zone",			       /*%< 35 DNS_R_BADZONE */
	"more data",			       /*%< 36 DNS_R_MOREDATA */
	"up to date",			       /*%< 37 DNS_R_UPTODATE */
	"tsig verify failure",		       /*%< 38 DNS_R_TSIGVERIFYFAILURE */
	"tsig indicates error",		       /*%< 39 DNS_R_TSIGERRORSET */

	"RRSIG failed to verify",	       /*%< 40 DNS_R_SIGINVALID */
	"RRSIG has expired",		       /*%< 41 DNS_R_SIGEXPIRED */
	"RRSIG validity period has not begun", /*%< 42 DNS_R_SIGFUTURE */
	"key is unauthorized to sign data",    /*%< 43 DNS_R_KEYUNAUTHORIZED */
	"invalid time",			       /*%< 44 DNS_R_INVALIDTIME */

	"expected a TSIG or SIG(0)",	       /*%< 45 DNS_R_EXPECTEDTSIG */
	"did not expect a TSIG or SIG(0)",     /*%< 46 DNS_R_UNEXPECTEDTSIG */
	"TKEY is unacceptable",		       /*%< 47 DNS_R_INVALIDTKEY */
	"hint",				       /*%< 48 DNS_R_HINT */
	"drop",				       /*%< 49 DNS_R_DROP */

	"zone not loaded",		       /*%< 50 DNS_R_NOTLOADED */
	"ncache nxdomain",		       /*%< 51 DNS_R_NCACHENXDOMAIN */
	"ncache nxrrset",		       /*%< 52 DNS_R_NCACHENXRRSET */
	"wait",				       /*%< 53 DNS_R_WAIT */
	"not verified yet",		       /*%< 54 DNS_R_NOTVERIFIEDYET */

	"no identity",			       /*%< 55 DNS_R_NOIDENTITY */
	"no journal",			       /*%< 56 DNS_R_NOJOURNAL */
	"alias",			       /*%< 57 DNS_R_ALIAS */
	"use TCP",			       /*%< 58 DNS_R_USETCP */
	"no valid RRSIG",		       /*%< 59 DNS_R_NOVALIDSIG */

	"no valid NSEC",		       /*%< 60 DNS_R_NOVALIDNSEC */
	"not insecure",			       /*%< 61 DNS_R_NOTINSECURE */
	"unknown service",		       /*%< 62 DNS_R_UNKNOWNSERVICE */
	"recoverable error occurred",	       /*%< 63 DNS_R_RECOVERABLE */
	"unknown opt attribute record",	       /*%< 64 DNS_R_UNKNOWNOPT */

	"unexpected message id",	       /*%< 65 DNS_R_UNEXPECTEDID */
	"seen include file",		       /*%< 66 DNS_R_SEENINCLUDE */
	"not exact",		       	       /*%< 67 DNS_R_NOTEXACT */
	"address blackholed",	       	       /*%< 68 DNS_R_BLACKHOLED */
	"bad algorithm",		       /*%< 69 DNS_R_BADALG */

	"invalid use of a meta type",	       /*%< 70 DNS_R_METATYPE */
	"CNAME and other data",		       /*%< 71 DNS_R_CNAMEANDOTHER */
	"multiple RRs of singleton type",      /*%< 72 DNS_R_SINGLETON */
	"hint nxrrset",			       /*%< 73 DNS_R_HINTNXRRSET */
	"no master file configured",	       /*%< 74 DNS_R_NOMASTERFILE */

	"unknown protocol",		       /*%< 75 DNS_R_UNKNOWNPROTO */
	"clocks are unsynchronized",	       /*%< 76 DNS_R_CLOCKSKEW */
	"IXFR failed",			       /*%< 77 DNS_R_BADIXFR */
	"not authoritative",		       /*%< 78 DNS_R_NOTAUTHORITATIVE */
	"no valid KEY",		       	       /*%< 79 DNS_R_NOVALIDKEY */

	"obsolete",			       /*%< 80 DNS_R_OBSOLETE */
	"already frozen",		       /*%< 81 DNS_R_FROZEN */
	"unknown flag",			       /*%< 82 DNS_R_UNKNOWNFLAG */
	"expected a response",		       /*%< 83 DNS_R_EXPECTEDRESPONSE */
	"no valid DS",			       /*%< 84 DNS_R_NOVALIDDS */

	"NS is an address",		       /*%< 85 DNS_R_NSISADDRESS */
	"received FORMERR",		       /*%< 86 DNS_R_REMOTEFORMERR */
	"truncated TCP response",	       /*%< 87 DNS_R_TRUNCATEDTCP */
	"lame server detected",		       /*%< 88 DNS_R_LAME */
	"unexpected RCODE",		       /*%< 89 DNS_R_UNEXPECTEDRCODE */

	"unexpected OPCODE",		       /*%< 90 DNS_R_UNEXPECTEDOPCODE */
	"chase DS servers",		       /*%< 91 DNS_R_CHASEDSSERVERS */
	"empty name",			       /*%< 92 DNS_R_EMPTYNAME */
	"empty wild",			       /*%< 93 DNS_R_EMPTYWILD */
	"bad bitmap",			       /*%< 94 DNS_R_BADBITMAP */

	"from wildcard",		       /*%< 95 DNS_R_FROMWILDCARD */
	"bad owner name (check-names)",	       /*%< 96 DNS_R_BADOWNERNAME */
	"bad name (check-names)",	       /*%< 97 DNS_R_BADNAME */
	"dynamic zone",			       /*%< 98 DNS_R_DYNAMIC */
	"unknown command",		       /*%< 99 DNS_R_UNKNOWNCOMMAND */

	"must-be-secure",		       /*%< 100 DNS_R_MUSTBESECURE */
	"covering NSEC record returned",       /*%< 101 DNS_R_COVERINGNSEC */
	"MX is an address",		       /*%< 102 DNS_R_MXISADDRESS */
	"duplicate query",		       /*%< 103 DNS_R_DUPLICATE */
	"invalid NSEC3 owner name (wildcard)", /*%< 104 DNS_R_INVALIDNSEC3 */
};

static const char *rcode_text[DNS_R_NRCODERESULTS] = {
	"NOERROR",				/*%< 0 DNS_R_NOEROR */
	"FORMERR",				/*%< 1 DNS_R_FORMERR */
	"SERVFAIL",				/*%< 2 DNS_R_SERVFAIL */
	"NXDOMAIN",				/*%< 3 DNS_R_NXDOMAIN */
	"NOTIMP",				/*%< 4 DNS_R_NOTIMP */

	"REFUSED",				/*%< 5 DNS_R_REFUSED */
	"YXDOMAIN",				/*%< 6 DNS_R_YXDOMAIN */
	"YXRRSET",				/*%< 7 DNS_R_YXRRSET */
	"NXRRSET",				/*%< 8 DNS_R_NXRRSET */
	"NOTAUTH",				/*%< 9 DNS_R_NOTAUTH */

	"NOTZONE",				/*%< 10 DNS_R_NOTZONE */
	"<rcode 11>",				/*%< 11 has no macro */
	"<rcode 12>",				/*%< 12 has no macro */
	"<rcode 13>",				/*%< 13 has no macro */
	"<rcode 14>",				/*%< 14 has no macro */

	"<rcode 15>",				/*%< 15 has no macro */
	"BADVERS",				/*%< 16 DNS_R_BADVERS */
};

#define DNS_RESULT_RESULTSET			2
#define DNS_RESULT_RCODERESULTSET		3

static isc_once_t		once = ISC_ONCE_INIT;

static void
initialize_action(void) {
	isc_result_t result;

	result = isc_result_register(ISC_RESULTCLASS_DNS, DNS_R_NRESULTS,
				     text, dns_msgcat, DNS_RESULT_RESULTSET);
	if (result == ISC_R_SUCCESS)
		result = isc_result_register(ISC_RESULTCLASS_DNSRCODE,
					     DNS_R_NRCODERESULTS,
					     rcode_text, dns_msgcat,
					     DNS_RESULT_RCODERESULTSET);
	if (result != ISC_R_SUCCESS)
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_result_register() failed: %u", result);
}

static void
initialize(void) {
	dns_lib_initmsgcat();
	RUNTIME_CHECK(isc_once_do(&once, initialize_action) == ISC_R_SUCCESS);
}

const char *
dns_result_totext(isc_result_t result) {
	initialize();

	return (isc_result_totext(result));
}

void
dns_result_register(void) {
	initialize();
}

dns_rcode_t
dns_result_torcode(isc_result_t result) {
	dns_rcode_t rcode = dns_rcode_servfail;

	if (DNS_RESULT_ISRCODE(result)) {
		/*
		 * Rcodes can't be bigger than 12 bits, which is why we
		 * AND with 0xFFF instead of 0xFFFF.
		 */
		return ((dns_rcode_t)((result) & 0xFFF));
	}
	/*
	 * Try to supply an appropriate rcode.
	 */
	switch (result) {
	case ISC_R_SUCCESS:
		rcode = dns_rcode_noerror;
		break;
	case ISC_R_BADBASE64:
	case ISC_R_NOSPACE:
	case ISC_R_RANGE:
	case ISC_R_UNEXPECTEDEND:
	case DNS_R_BADAAAA:
	/* case DNS_R_BADBITSTRING: deprecated */
	case DNS_R_BADCKSUM:
	case DNS_R_BADCLASS:
	case DNS_R_BADLABELTYPE:
	case DNS_R_BADPOINTER:
	case DNS_R_BADTTL:
	case DNS_R_BADZONE:
	/* case DNS_R_BITSTRINGTOOLONG: deprecated */
	case DNS_R_EXTRADATA:
	case DNS_R_LABELTOOLONG:
	case DNS_R_NOREDATA:
	case DNS_R_SYNTAX:
	case DNS_R_TEXTTOOLONG:
	case DNS_R_TOOMANYHOPS:
	case DNS_R_TSIGERRORSET:
	case DNS_R_UNKNOWN:
		rcode = dns_rcode_formerr;
		break;
	case DNS_R_DISALLOWED:
		rcode = dns_rcode_refused;
		break;
	case DNS_R_TSIGVERIFYFAILURE:
	case DNS_R_CLOCKSKEW:
		rcode = dns_rcode_notauth;
		break;
	default:
		rcode = dns_rcode_servfail;
	}

	return (rcode);
}
