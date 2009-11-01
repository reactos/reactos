/*
 * Copyright (C) 2004-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2003  Internet Software Consortium.
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

/* $Id: nslookup.c,v 1.117.334.4 2009/05/06 11:41:57 fdupont Exp $ */

#include <config.h>

#include <stdlib.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/event.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/timer.h>
#include <isc/util.h>
#include <isc/task.h>
#include <isc/netaddr.h>

#include <dns/message.h>
#include <dns/name.h>
#include <dns/fixedname.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/byaddr.h>

#include <dig/dig.h>

static isc_boolean_t short_form = ISC_TRUE,
	tcpmode = ISC_FALSE,
	identify = ISC_FALSE, stats = ISC_TRUE,
	comments = ISC_TRUE, section_question = ISC_TRUE,
	section_answer = ISC_TRUE, section_authority = ISC_TRUE,
	section_additional = ISC_TRUE, recurse = ISC_TRUE,
	aaonly = ISC_FALSE, nofail = ISC_TRUE;

static isc_boolean_t in_use = ISC_FALSE;
static char defclass[MXRD] = "IN";
static char deftype[MXRD] = "A";
static isc_event_t *global_event = NULL;

static char domainopt[DNS_NAME_MAXTEXT];

static const char *rcodetext[] = {
	"NOERROR",
	"FORMERR",
	"SERVFAIL",
	"NXDOMAIN",
	"NOTIMP",
	"REFUSED",
	"YXDOMAIN",
	"YXRRSET",
	"NXRRSET",
	"NOTAUTH",
	"NOTZONE",
	"RESERVED11",
	"RESERVED12",
	"RESERVED13",
	"RESERVED14",
	"RESERVED15",
	"BADVERS"
};

static const char *rtypetext[] = {
	"rtype_0 = ",			/* 0 */
	"internet address = ",		/* 1 */
	"nameserver = ",		/* 2 */
	"md = ",			/* 3 */
	"mf = ",			/* 4 */
	"canonical name = ",		/* 5 */
	"soa = ",			/* 6 */
	"mb = ",			/* 7 */
	"mg = ",			/* 8 */
	"mr = ",			/* 9 */
	"rtype_10 = ",			/* 10 */
	"protocol = ",			/* 11 */
	"name = ",			/* 12 */
	"hinfo = ",			/* 13 */
	"minfo = ",			/* 14 */
	"mail exchanger = ",		/* 15 */
	"text = ",			/* 16 */
	"rp = ",       			/* 17 */
	"afsdb = ",			/* 18 */
	"x25 address = ",		/* 19 */
	"isdn address = ",		/* 20 */
	"rt = ",			/* 21 */
	"nsap = ",			/* 22 */
	"nsap_ptr = ",			/* 23 */
	"signature = ",			/* 24 */
	"key = ",			/* 25 */
	"px = ",			/* 26 */
	"gpos = ",			/* 27 */
	"has AAAA address ",		/* 28 */
	"loc = ",			/* 29 */
	"next = ",			/* 30 */
	"rtype_31 = ",			/* 31 */
	"rtype_32 = ",			/* 32 */
	"service = ",			/* 33 */
	"rtype_34 = ",			/* 34 */
	"naptr = ",			/* 35 */
	"kx = ",			/* 36 */
	"cert = ",			/* 37 */
	"v6 address = ",		/* 38 */
	"dname = ",			/* 39 */
	"rtype_40 = ",			/* 40 */
	"optional = "			/* 41 */
};

#define N_KNOWN_RRTYPES (sizeof(rtypetext) / sizeof(rtypetext[0]))

static void flush_lookup_list(void);
static void getinput(isc_task_t *task, isc_event_t *event);

static char *
rcode_totext(dns_rcode_t rcode)
{
	static char buf[sizeof("?65535")];
	union {
		const char *consttext;
		char *deconsttext;
	} totext;

	if (rcode >= (sizeof(rcodetext)/sizeof(rcodetext[0]))) {
		snprintf(buf, sizeof(buf), "?%u", rcode);
		totext.deconsttext = buf;
	} else
		totext.consttext = rcodetext[rcode];
	return totext.deconsttext;
}

void
dighost_shutdown(void) {
	isc_event_t *event = global_event;

	flush_lookup_list();
	debug("dighost_shutdown()");

	if (!in_use) {
		isc_app_shutdown();
		return;
	}

	isc_task_send(global_task, &event);
}

static void
printsoa(dns_rdata_t *rdata) {
	dns_rdata_soa_t soa;
	isc_result_t result;
	char namebuf[DNS_NAME_FORMATSIZE];

	result = dns_rdata_tostruct(rdata, &soa, NULL);
	check_result(result, "dns_rdata_tostruct");

	dns_name_format(&soa.origin, namebuf, sizeof(namebuf));
	printf("\torigin = %s\n", namebuf);
	dns_name_format(&soa.contact, namebuf, sizeof(namebuf));
	printf("\tmail addr = %s\n", namebuf);
	printf("\tserial = %u\n", soa.serial);
	printf("\trefresh = %u\n", soa.refresh);
	printf("\tretry = %u\n", soa.retry);
	printf("\texpire = %u\n", soa.expire);
	printf("\tminimum = %u\n", soa.minimum);
	dns_rdata_freestruct(&soa);
}

static void
printa(dns_rdata_t *rdata) {
	isc_result_t result;
	char text[sizeof("255.255.255.255")];
	isc_buffer_t b;

	isc_buffer_init(&b, text, sizeof(text));
	result = dns_rdata_totext(rdata, NULL, &b);
	check_result(result, "dns_rdata_totext");
	printf("Address: %.*s\n", (int)isc_buffer_usedlength(&b),
	       (char *)isc_buffer_base(&b));
}
#ifdef DIG_SIGCHASE
/* Just for compatibility : not use in host program */
isc_result_t
printrdataset(dns_name_t *owner_name, dns_rdataset_t *rdataset,
	      isc_buffer_t *target)
{
	UNUSED(owner_name);
	UNUSED(rdataset);
	UNUSED(target);
	return(ISC_FALSE);
}
#endif
static void
printrdata(dns_rdata_t *rdata) {
	isc_result_t result;
	isc_buffer_t *b = NULL;
	unsigned int size = 1024;
	isc_boolean_t done = ISC_FALSE;

	if (rdata->type < N_KNOWN_RRTYPES)
		printf("%s", rtypetext[rdata->type]);
	else
		printf("rdata_%d = ", rdata->type);

	while (!done) {
		result = isc_buffer_allocate(mctx, &b, size);
		if (result != ISC_R_SUCCESS)
			check_result(result, "isc_buffer_allocate");
		result = dns_rdata_totext(rdata, NULL, b);
		if (result == ISC_R_SUCCESS) {
			printf("%.*s\n", (int)isc_buffer_usedlength(b),
			       (char *)isc_buffer_base(b));
			done = ISC_TRUE;
		} else if (result != ISC_R_NOSPACE)
			check_result(result, "dns_rdata_totext");
		isc_buffer_free(&b);
		size *= 2;
	}
}

static isc_result_t
printsection(dig_query_t *query, dns_message_t *msg, isc_boolean_t headers,
	     dns_section_t section) {
	isc_result_t result, loopresult;
	dns_name_t *name;
	dns_rdataset_t *rdataset = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	char namebuf[DNS_NAME_FORMATSIZE];

	UNUSED(query);
	UNUSED(headers);

	debug("printsection()");

	result = dns_message_firstname(msg, section);
	if (result == ISC_R_NOMORE)
		return (ISC_R_SUCCESS);
	else if (result != ISC_R_SUCCESS)
		return (result);
	for (;;) {
		name = NULL;
		dns_message_currentname(msg, section,
					&name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			loopresult = dns_rdataset_first(rdataset);
			while (loopresult == ISC_R_SUCCESS) {
				dns_rdataset_current(rdataset, &rdata);
				switch (rdata.type) {
				case dns_rdatatype_a:
					if (section != DNS_SECTION_ANSWER)
						goto def_short_section;
					dns_name_format(name, namebuf,
							sizeof(namebuf));
					printf("Name:\t%s\n", namebuf);
					printa(&rdata);
					break;
				case dns_rdatatype_soa:
					dns_name_format(name, namebuf,
							sizeof(namebuf));
					printf("%s\n", namebuf);
					printsoa(&rdata);
					break;
				default:
				def_short_section:
					dns_name_format(name, namebuf,
							sizeof(namebuf));
					printf("%s\t", namebuf);
					printrdata(&rdata);
					break;
				}
				dns_rdata_reset(&rdata);
				loopresult = dns_rdataset_next(rdataset);
			}
		}
		result = dns_message_nextname(msg, section);
		if (result == ISC_R_NOMORE)
			break;
		else if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
detailsection(dig_query_t *query, dns_message_t *msg, isc_boolean_t headers,
	     dns_section_t section) {
	isc_result_t result, loopresult;
	dns_name_t *name;
	dns_rdataset_t *rdataset = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	char namebuf[DNS_NAME_FORMATSIZE];

	UNUSED(query);

	debug("detailsection()");

	if (headers) {
		switch (section) {
		case DNS_SECTION_QUESTION:
			puts("    QUESTIONS:");
			break;
		case DNS_SECTION_ANSWER:
			puts("    ANSWERS:");
			break;
		case DNS_SECTION_AUTHORITY:
			puts("    AUTHORITY RECORDS:");
			break;
		case DNS_SECTION_ADDITIONAL:
			puts("    ADDITIONAL RECORDS:");
			break;
		}
	}

	result = dns_message_firstname(msg, section);
	if (result == ISC_R_NOMORE)
		return (ISC_R_SUCCESS);
	else if (result != ISC_R_SUCCESS)
		return (result);
	for (;;) {
		name = NULL;
		dns_message_currentname(msg, section,
					&name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			if (section == DNS_SECTION_QUESTION) {
				dns_name_format(name, namebuf,
						sizeof(namebuf));
				printf("\t%s, ", namebuf);
				dns_rdatatype_format(rdataset->type,
						     namebuf,
						     sizeof(namebuf));
				printf("type = %s, ", namebuf);
				dns_rdataclass_format(rdataset->rdclass,
						      namebuf,
						      sizeof(namebuf));
				printf("class = %s\n", namebuf);
			}
			loopresult = dns_rdataset_first(rdataset);
			while (loopresult == ISC_R_SUCCESS) {
				dns_rdataset_current(rdataset, &rdata);

				dns_name_format(name, namebuf,
						sizeof(namebuf));
				printf("    ->  %s\n", namebuf);

				switch (rdata.type) {
				case dns_rdatatype_soa:
					printsoa(&rdata);
					break;
				default:
					printf("\t");
					printrdata(&rdata);
				}
				dns_rdata_reset(&rdata);
				loopresult = dns_rdataset_next(rdataset);
			}
		}
		result = dns_message_nextname(msg, section);
		if (result == ISC_R_NOMORE)
			break;
		else if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	return (ISC_R_SUCCESS);
}

void
received(int bytes, isc_sockaddr_t *from, dig_query_t *query)
{
	UNUSED(bytes);
	UNUSED(from);
	UNUSED(query);
}

void
trying(char *frm, dig_lookup_t *lookup) {
	UNUSED(frm);
	UNUSED(lookup);

}

isc_result_t
printmessage(dig_query_t *query, dns_message_t *msg, isc_boolean_t headers) {
	char servtext[ISC_SOCKADDR_FORMATSIZE];

	debug("printmessage()");

	isc_sockaddr_format(&query->sockaddr, servtext, sizeof(servtext));
	printf("Server:\t\t%s\n", query->userarg);
	printf("Address:\t%s\n", servtext);

	puts("");

	if (!short_form) {
		isc_boolean_t headers = ISC_TRUE;
		puts("------------");
		/*		detailheader(query, msg);*/
		detailsection(query, msg, headers, DNS_SECTION_QUESTION);
		detailsection(query, msg, headers, DNS_SECTION_ANSWER);
		detailsection(query, msg, headers, DNS_SECTION_AUTHORITY);
		detailsection(query, msg, headers, DNS_SECTION_ADDITIONAL);
		puts("------------");
	}

	if (msg->rcode != 0) {
		char nametext[DNS_NAME_FORMATSIZE];
		dns_name_format(query->lookup->name,
				nametext, sizeof(nametext));
		printf("** server can't find %s: %s\n",
		       (msg->rcode != dns_rcode_nxdomain) ? nametext :
		       query->lookup->textname, rcode_totext(msg->rcode));
		debug("returning with rcode == 0");
		return (ISC_R_SUCCESS);
	}

	if ((msg->flags & DNS_MESSAGEFLAG_AA) == 0)
		puts("Non-authoritative answer:");
	if (!ISC_LIST_EMPTY(msg->sections[DNS_SECTION_ANSWER]))
		printsection(query, msg, headers, DNS_SECTION_ANSWER);
	else
		printf("*** Can't find %s: No answer\n",
		       query->lookup->textname);

	if (((msg->flags & DNS_MESSAGEFLAG_AA) == 0) &&
	    (query->lookup->rdtype != dns_rdatatype_a)) {
		puts("\nAuthoritative answers can be found from:");
		printsection(query, msg, headers,
			     DNS_SECTION_AUTHORITY);
		printsection(query, msg, headers,
			     DNS_SECTION_ADDITIONAL);
	}
	return (ISC_R_SUCCESS);
}

static void
show_settings(isc_boolean_t full, isc_boolean_t serv_only) {
	dig_server_t *srv;
	isc_sockaddr_t sockaddr;
	dig_searchlist_t *listent;
	isc_result_t result;

	srv = ISC_LIST_HEAD(server_list);

	while (srv != NULL) {
		char sockstr[ISC_SOCKADDR_FORMATSIZE];

		result = get_address(srv->servername, port, &sockaddr);
		check_result(result, "get_address");

		isc_sockaddr_format(&sockaddr, sockstr, sizeof(sockstr));
		printf("Default server: %s\nAddress: %s\n",
			srv->userarg, sockstr);
		if (!full)
			return;
		srv = ISC_LIST_NEXT(srv, link);
	}
	if (serv_only)
		return;
	printf("\nSet options:\n");
	printf("  %s\t\t\t%s\t\t%s\n",
	       tcpmode ? "vc" : "novc",
	       short_form ? "nodebug" : "debug",
	       debugging ? "d2" : "nod2");
	printf("  %s\t\t%s\n",
	       usesearch ? "search" : "nosearch",
	       recurse ? "recurse" : "norecurse");
	printf("  timeout = %d\t\tretry = %d\tport = %d\n",
	       timeout, tries, port);
	printf("  querytype = %-8s\tclass = %s\n", deftype, defclass);
	printf("  srchlist = ");
	for (listent = ISC_LIST_HEAD(search_list);
	     listent != NULL;
	     listent = ISC_LIST_NEXT(listent, link)) {
		     printf("%s", listent->origin);
		     if (ISC_LIST_NEXT(listent, link) != NULL)
			     printf("/");
	}
	printf("\n");
}

static isc_boolean_t
testtype(char *typetext) {
	isc_result_t result;
	isc_textregion_t tr;
	dns_rdatatype_t rdtype;

	tr.base = typetext;
	tr.length = strlen(typetext);
	result = dns_rdatatype_fromtext(&rdtype, &tr);
	if (result == ISC_R_SUCCESS)
		return (ISC_TRUE);
	else {
		printf("unknown query type: %s\n", typetext);
		return (ISC_FALSE);
	}
}

static isc_boolean_t
testclass(char *typetext) {
	isc_result_t result;
	isc_textregion_t tr;
	dns_rdataclass_t rdclass;

	tr.base = typetext;
	tr.length = strlen(typetext);
	result = dns_rdataclass_fromtext(&rdclass, &tr);
	if (result == ISC_R_SUCCESS)
		return (ISC_TRUE);
	else {
		printf("unknown query class: %s\n", typetext);
		return (ISC_FALSE);
	}
}

static void
safecpy(char *dest, char *src, int size) {
	strncpy(dest, src, size);
	dest[size-1] = 0;
}

static isc_result_t
parse_uint(isc_uint32_t *uip, const char *value, isc_uint32_t max,
	   const char *desc) {
	isc_uint32_t n;
	isc_result_t result = isc_parse_uint32(&n, value, 10);
	if (result == ISC_R_SUCCESS && n > max)
		result = ISC_R_RANGE;
	if (result != ISC_R_SUCCESS) {
		printf("invalid %s '%s': %s\n", desc,
		       value, isc_result_totext(result));
		return result;
	}
	*uip = n;
	return (ISC_R_SUCCESS);
}

static void
set_port(const char *value) {
	isc_uint32_t n;
	isc_result_t result = parse_uint(&n, value, 65535, "port");
	if (result == ISC_R_SUCCESS)
		port = (isc_uint16_t) n;
}

static void
set_timeout(const char *value) {
	isc_uint32_t n;
	isc_result_t result = parse_uint(&n, value, UINT_MAX, "timeout");
	if (result == ISC_R_SUCCESS)
		timeout = n;
}

static void
set_tries(const char *value) {
	isc_uint32_t n;
	isc_result_t result = parse_uint(&n, value, INT_MAX, "tries");
	if (result == ISC_R_SUCCESS)
		tries = n;
}

static void
setoption(char *opt) {
	if (strncasecmp(opt, "all", 4) == 0) {
		show_settings(ISC_TRUE, ISC_FALSE);
	} else if (strncasecmp(opt, "class=", 6) == 0) {
		if (testclass(&opt[6]))
			safecpy(defclass, &opt[6], sizeof(defclass));
	} else if (strncasecmp(opt, "cl=", 3) == 0) {
		if (testclass(&opt[3]))
			safecpy(defclass, &opt[3], sizeof(defclass));
	} else if (strncasecmp(opt, "type=", 5) == 0) {
		if (testtype(&opt[5]))
			safecpy(deftype, &opt[5], sizeof(deftype));
	} else if (strncasecmp(opt, "ty=", 3) == 0) {
		if (testtype(&opt[3]))
			safecpy(deftype, &opt[3], sizeof(deftype));
	} else if (strncasecmp(opt, "querytype=", 10) == 0) {
		if (testtype(&opt[10]))
			safecpy(deftype, &opt[10], sizeof(deftype));
	} else if (strncasecmp(opt, "query=", 6) == 0) {
		if (testtype(&opt[6]))
			safecpy(deftype, &opt[6], sizeof(deftype));
	} else if (strncasecmp(opt, "qu=", 3) == 0) {
		if (testtype(&opt[3]))
			safecpy(deftype, &opt[3], sizeof(deftype));
	} else if (strncasecmp(opt, "q=", 2) == 0) {
		if (testtype(&opt[2]))
			safecpy(deftype, &opt[2], sizeof(deftype));
	} else if (strncasecmp(opt, "domain=", 7) == 0) {
		safecpy(domainopt, &opt[7], sizeof(domainopt));
		set_search_domain(domainopt);
		usesearch = ISC_TRUE;
	} else if (strncasecmp(opt, "do=", 3) == 0) {
		safecpy(domainopt, &opt[3], sizeof(domainopt));
		set_search_domain(domainopt);
		usesearch = ISC_TRUE;
	} else if (strncasecmp(opt, "port=", 5) == 0) {
		set_port(&opt[5]);
	} else if (strncasecmp(opt, "po=", 3) == 0) {
		set_port(&opt[3]);
	} else if (strncasecmp(opt, "timeout=", 8) == 0) {
		set_timeout(&opt[8]);
	} else if (strncasecmp(opt, "t=", 2) == 0) {
		set_timeout(&opt[2]);
	} else if (strncasecmp(opt, "rec", 3) == 0) {
		recurse = ISC_TRUE;
	} else if (strncasecmp(opt, "norec", 5) == 0) {
		recurse = ISC_FALSE;
	} else if (strncasecmp(opt, "retry=", 6) == 0) {
		set_tries(&opt[6]);
	} else if (strncasecmp(opt, "ret=", 4) == 0) {
		set_tries(&opt[4]);
	} else if (strncasecmp(opt, "def", 3) == 0) {
		usesearch = ISC_TRUE;
	} else if (strncasecmp(opt, "nodef", 5) == 0) {
		usesearch = ISC_FALSE;
	} else if (strncasecmp(opt, "vc", 3) == 0) {
		tcpmode = ISC_TRUE;
	} else if (strncasecmp(opt, "novc", 5) == 0) {
		tcpmode = ISC_FALSE;
	} else if (strncasecmp(opt, "deb", 3) == 0) {
		short_form = ISC_FALSE;
		showsearch = ISC_TRUE;
	} else if (strncasecmp(opt, "nodeb", 5) == 0) {
		short_form = ISC_TRUE;
		showsearch = ISC_FALSE;
	} else if (strncasecmp(opt, "d2", 2) == 0) {
		debugging = ISC_TRUE;
	} else if (strncasecmp(opt, "nod2", 4) == 0) {
		debugging = ISC_FALSE;
	} else if (strncasecmp(opt, "search", 3) == 0) {
		usesearch = ISC_TRUE;
	} else if (strncasecmp(opt, "nosearch", 5) == 0) {
		usesearch = ISC_FALSE;
	} else if (strncasecmp(opt, "sil", 3) == 0) {
		/* deprecation_msg = ISC_FALSE; */
	} else if (strncasecmp(opt, "fail", 3) == 0) {
		nofail=ISC_FALSE;
	} else if (strncasecmp(opt, "nofail", 3) == 0) {
		nofail=ISC_TRUE;
	} else {
		printf("*** Invalid option: %s\n", opt);
	}
}

static void
addlookup(char *opt) {
	dig_lookup_t *lookup;
	isc_result_t result;
	isc_textregion_t tr;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	char store[MXNAME];

	debug("addlookup()");
	tr.base = deftype;
	tr.length = strlen(deftype);
	result = dns_rdatatype_fromtext(&rdtype, &tr);
	if (result != ISC_R_SUCCESS) {
		printf("unknown query type: %s\n", deftype);
		rdclass = dns_rdatatype_a;
	}
	tr.base = defclass;
	tr.length = strlen(defclass);
	result = dns_rdataclass_fromtext(&rdclass, &tr);
	if (result != ISC_R_SUCCESS) {
		printf("unknown query class: %s\n", defclass);
		rdclass = dns_rdataclass_in;
	}
	lookup = make_empty_lookup();
	if (get_reverse(store, sizeof(store), opt, lookup->ip6_int, ISC_TRUE)
	    == ISC_R_SUCCESS) {
		safecpy(lookup->textname, store, sizeof(lookup->textname));
		lookup->rdtype = dns_rdatatype_ptr;
		lookup->rdtypeset = ISC_TRUE;
	} else {
		safecpy(lookup->textname, opt, sizeof(lookup->textname));
		lookup->rdtype = rdtype;
		lookup->rdtypeset = ISC_TRUE;
	}
	lookup->rdclass = rdclass;
	lookup->rdclassset = ISC_TRUE;
	lookup->trace = ISC_FALSE;
	lookup->trace_root = lookup->trace;
	lookup->ns_search_only = ISC_FALSE;
	lookup->identify = identify;
	lookup->recurse = recurse;
	lookup->aaonly = aaonly;
	lookup->retries = tries;
	lookup->udpsize = 0;
	lookup->comments = comments;
	lookup->tcp_mode = tcpmode;
	lookup->stats = stats;
	lookup->section_question = section_question;
	lookup->section_answer = section_answer;
	lookup->section_authority = section_authority;
	lookup->section_additional = section_additional;
	lookup->new_search = ISC_TRUE;
	if (nofail)
		lookup->servfail_stops = ISC_FALSE;
	ISC_LIST_INIT(lookup->q);
	ISC_LINK_INIT(lookup, link);
	ISC_LIST_APPEND(lookup_list, lookup, link);
	lookup->origin = NULL;
	ISC_LIST_INIT(lookup->my_server_list);
	debug("looking up %s", lookup->textname);
}

static void
get_next_command(void) {
	char *buf;
	char *ptr, *arg;
	char *input;

	fflush(stdout);
	buf = isc_mem_allocate(mctx, COMMSIZE);
	if (buf == NULL)
		fatal("memory allocation failure");
	fputs("> ", stderr);
	fflush(stderr);
	isc_app_block();
	ptr = fgets(buf, COMMSIZE, stdin);
	isc_app_unblock();
	if (ptr == NULL) {
		in_use = ISC_FALSE;
		goto cleanup;
	}
	input = buf;
	ptr = next_token(&input, " \t\r\n");
	if (ptr == NULL)
		goto cleanup;
	arg = next_token(&input, " \t\r\n");
	if ((strcasecmp(ptr, "set") == 0) &&
	    (arg != NULL))
		setoption(arg);
	else if ((strcasecmp(ptr, "server") == 0) ||
		 (strcasecmp(ptr, "lserver") == 0)) {
		isc_app_block();
		set_nameserver(arg);
		check_ra = ISC_FALSE;
		isc_app_unblock();
		show_settings(ISC_TRUE, ISC_TRUE);
	} else if (strcasecmp(ptr, "exit") == 0) {
		in_use = ISC_FALSE;
		goto cleanup;
	} else if (strcasecmp(ptr, "help") == 0 ||
		   strcasecmp(ptr, "?") == 0) {
		printf("The '%s' command is not yet implemented.\n", ptr);
		goto cleanup;
	} else if (strcasecmp(ptr, "finger") == 0 ||
		   strcasecmp(ptr, "root") == 0 ||
		   strcasecmp(ptr, "ls") == 0 ||
		   strcasecmp(ptr, "view") == 0) {
		printf("The '%s' command is not implemented.\n", ptr);
		goto cleanup;
	} else
		addlookup(ptr);
 cleanup:
	isc_mem_free(mctx, buf);
}

static void
parse_args(int argc, char **argv) {
	isc_boolean_t have_lookup = ISC_FALSE;

	usesearch = ISC_TRUE;
	for (argc--, argv++; argc > 0; argc--, argv++) {
		debug("main parsing %s", argv[0]);
		if (argv[0][0] == '-') {
			if (argv[0][1] != 0)
				setoption(&argv[0][1]);
			else
				have_lookup = ISC_TRUE;
		} else {
			if (!have_lookup) {
				have_lookup = ISC_TRUE;
				in_use = ISC_TRUE;
				addlookup(argv[0]);
			} else {
				set_nameserver(argv[0]);
				check_ra = ISC_FALSE;
			}
		}
	}
}

static void
flush_lookup_list(void) {
	dig_lookup_t *l, *lp;
	dig_query_t *q, *qp;
	dig_server_t *s, *sp;

	lookup_counter = 0;
	l = ISC_LIST_HEAD(lookup_list);
	while (l != NULL) {
		q = ISC_LIST_HEAD(l->q);
		while (q != NULL) {
			if (q->sock != NULL) {
				isc_socket_cancel(q->sock, NULL,
						  ISC_SOCKCANCEL_ALL);
				isc_socket_detach(&q->sock);
			}
			if (ISC_LINK_LINKED(&q->recvbuf, link))
				ISC_LIST_DEQUEUE(q->recvlist, &q->recvbuf,
						 link);
			if (ISC_LINK_LINKED(&q->lengthbuf, link))
				ISC_LIST_DEQUEUE(q->lengthlist, &q->lengthbuf,
						 link);
			isc_buffer_invalidate(&q->recvbuf);
			isc_buffer_invalidate(&q->lengthbuf);
			qp = q;
			q = ISC_LIST_NEXT(q, link);
			ISC_LIST_DEQUEUE(l->q, qp, link);
			isc_mem_free(mctx, qp);
		}
		s = ISC_LIST_HEAD(l->my_server_list);
		while (s != NULL) {
			sp = s;
			s = ISC_LIST_NEXT(s, link);
			ISC_LIST_DEQUEUE(l->my_server_list, sp, link);
			isc_mem_free(mctx, sp);

		}
		if (l->sendmsg != NULL)
			dns_message_destroy(&l->sendmsg);
		if (l->timer != NULL)
			isc_timer_detach(&l->timer);
		lp = l;
		l = ISC_LIST_NEXT(l, link);
		ISC_LIST_DEQUEUE(lookup_list, lp, link);
		isc_mem_free(mctx, lp);
	}
}

static void
getinput(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);
	if (global_event == NULL)
		global_event = event;
	while (in_use) {
		get_next_command();
		if (ISC_LIST_HEAD(lookup_list) != NULL) {
			start_lookup();
			return;
		}
	}
	isc_app_shutdown();
}

int
main(int argc, char **argv) {
	isc_result_t result;

	ISC_LIST_INIT(lookup_list);
	ISC_LIST_INIT(server_list);
	ISC_LIST_INIT(search_list);

	check_ra = ISC_TRUE;

	result = isc_app_start();
	check_result(result, "isc_app_start");

	setup_libs();
	progname = argv[0];

	parse_args(argc, argv);

	setup_system();
	if (domainopt[0] != '\0')
		set_search_domain(domainopt);
	if (in_use)
		result = isc_app_onrun(mctx, global_task, onrun_callback,
				       NULL);
	else
		result = isc_app_onrun(mctx, global_task, getinput, NULL);
	check_result(result, "isc_app_onrun");
	in_use = ISC_TF(!in_use);

	(void)isc_app_run();

	puts("");
	debug("done, and starting to shut down");
	if (global_event != NULL)
		isc_event_free(&global_event);
	cancel_all();
	destroy_libs();
	isc_app_finish();

	return (0);
}
