/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: dighost.c,v 1.311.70.8 2009/02/25 02:39:21 marka Exp $ */

/*! \file
 *  \note
 * Notice to programmers:  Do not use this code as an example of how to
 * use the ISC library to perform DNS lookups.  Dig and Host both operate
 * on the request level, since they allow fine-tuning of output and are
 * intended as debugging tools.  As a result, they perform many of the
 * functions which could be better handled using the dns_resolver
 * functions in most applications.
 */

#include <config.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef WITH_IDN
#include <idn/result.h>
#include <idn/log.h>
#include <idn/resconf.h>
#include <idn/api.h>
#endif

#include <dns/byaddr.h>
#ifdef DIG_SIGCHASE
#include <dns/dnssec.h>
#include <dns/ds.h>
#include <dns/nsec.h>
#include <isc/random.h>
#include <ctype.h>
#endif
#include <dns/fixedname.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/tsig.h>

#include <dst/dst.h>

#include <isc/app.h>
#include <isc/base64.h>
#include <isc/entropy.h>
#include <isc/file.h>
#include <isc/lang.h>
#include <isc/netaddr.h>
#ifdef DIG_SIGCHASE
#include <isc/netdb.h>
#endif
#include <isc/print.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/types.h>
#include <isc/util.h>

#include <lwres/lwres.h>
#include <lwres/net.h>

#include <bind9/getaddresses.h>

#include <dig/dig.h>

#if ! defined(NS_INADDRSZ)
#define NS_INADDRSZ	 4
#endif

#if ! defined(NS_IN6ADDRSZ)
#define NS_IN6ADDRSZ	16
#endif

static lwres_context_t *lwctx = NULL;
static lwres_conf_t *lwconf;

dig_lookuplist_t lookup_list;
dig_serverlist_t server_list;
dig_searchlistlist_t search_list;

isc_boolean_t
	check_ra = ISC_FALSE,
	have_ipv4 = ISC_FALSE,
	have_ipv6 = ISC_FALSE,
	specified_source = ISC_FALSE,
	free_now = ISC_FALSE,
	cancel_now = ISC_FALSE,
	usesearch = ISC_FALSE,
	showsearch = ISC_FALSE,
	qr = ISC_FALSE,
	is_dst_up = ISC_FALSE;
in_port_t port = 53;
unsigned int timeout = 0;
unsigned int extrabytes;
isc_mem_t *mctx = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_task_t *global_task = NULL;
isc_timermgr_t *timermgr = NULL;
isc_socketmgr_t *socketmgr = NULL;
isc_sockaddr_t bind_address;
isc_sockaddr_t bind_any;
int sendcount = 0;
int recvcount = 0;
int sockcount = 0;
int ndots = -1;
int tries = 3;
int lookup_counter = 0;

#ifdef WITH_IDN
static void		initialize_idn(void);
static isc_result_t	output_filter(isc_buffer_t *buffer,
				      unsigned int used_org,
				      isc_boolean_t absolute);
static idn_result_t	append_textname(char *name, const char *origin,
					size_t namesize);
static void		idn_check_result(idn_result_t r, const char *msg);

#define MAXDLEN		256
int  idnoptions	= 0;
#endif

/*%
 * Exit Codes:
 *
 *\li	0   Everything went well, including things like NXDOMAIN
 *\li	1   Usage error
 *\li	7   Got too many RR's or Names
 *\li	8   Couldn't open batch file
 *\li	9   No reply from server
 *\li	10  Internal error
 */
int exitcode = 0;
int fatalexit = 0;
char keynametext[MXNAME];
char keyfile[MXNAME] = "";
char keysecret[MXNAME] = "";
dns_name_t *hmacname = NULL;
unsigned int digestbits = 0;
isc_buffer_t *namebuf = NULL;
dns_tsigkey_t *key = NULL;
isc_boolean_t validated = ISC_TRUE;
isc_entropy_t *entp = NULL;
isc_mempool_t *commctx = NULL;
isc_boolean_t debugging = ISC_FALSE;
isc_boolean_t memdebugging = ISC_FALSE;
char *progname = NULL;
isc_mutex_t lookup_lock;
dig_lookup_t *current_lookup = NULL;

#ifdef DIG_SIGCHASE

isc_result_t	  get_trusted_key(isc_mem_t *mctx);
dns_rdataset_t *  sigchase_scanname(dns_rdatatype_t type,
				    dns_rdatatype_t covers,
				    isc_boolean_t *lookedup,
				    dns_name_t *rdata_name);
dns_rdataset_t *  chase_scanname_section(dns_message_t *msg,
					 dns_name_t *name,
					 dns_rdatatype_t type,
					 dns_rdatatype_t covers,
					 int section);
isc_result_t	  advanced_rrsearch(dns_rdataset_t **rdataset,
				    dns_name_t *name,
				    dns_rdatatype_t type,
				    dns_rdatatype_t covers,
				    isc_boolean_t *lookedup);
isc_result_t	  sigchase_verify_sig_key(dns_name_t *name,
					  dns_rdataset_t *rdataset,
					  dst_key_t* dnsseckey,
					  dns_rdataset_t *sigrdataset,
					  isc_mem_t *mctx);
isc_result_t	  sigchase_verify_sig(dns_name_t *name,
				      dns_rdataset_t *rdataset,
				      dns_rdataset_t *keyrdataset,
				      dns_rdataset_t *sigrdataset,
				      isc_mem_t *mctx);
isc_result_t	  sigchase_verify_ds(dns_name_t *name,
				     dns_rdataset_t *keyrdataset,
				     dns_rdataset_t *dsrdataset,
				     isc_mem_t *mctx);
void		  sigchase(dns_message_t *msg);
void		  print_rdata(dns_rdata_t *rdata, isc_mem_t *mctx);
void		  print_rdataset(dns_name_t *name,
				 dns_rdataset_t *rdataset, isc_mem_t *mctx);
void		  dup_name(dns_name_t *source, dns_name_t* target,
			   isc_mem_t *mctx);
void		  free_name(dns_name_t *name, isc_mem_t *mctx);
void		  dump_database(void);
void		  dump_database_section(dns_message_t *msg, int section);
dns_rdataset_t *  search_type(dns_name_t *name, dns_rdatatype_t type,
			      dns_rdatatype_t covers);
isc_result_t	  contains_trusted_key(dns_name_t *name,
				       dns_rdataset_t *rdataset,
				       dns_rdataset_t *sigrdataset,
				       isc_mem_t *mctx);
void		  print_type(dns_rdatatype_t type);
isc_result_t	  prove_nx_domain(dns_message_t * msg,
				  dns_name_t * name,
				  dns_name_t * rdata_name,
				  dns_rdataset_t ** rdataset,
				  dns_rdataset_t ** sigrdataset);
isc_result_t	  prove_nx_type(dns_message_t * msg, dns_name_t *name,
				dns_rdataset_t *nsec,
				dns_rdataclass_t class,
				dns_rdatatype_t type,
				dns_name_t * rdata_name,
				dns_rdataset_t ** rdataset,
				dns_rdataset_t ** sigrdataset);
isc_result_t	  prove_nx(dns_message_t * msg, dns_name_t * name,
			   dns_rdataclass_t class,
			   dns_rdatatype_t type,
			   dns_name_t * rdata_name,
			   dns_rdataset_t ** rdataset,
			   dns_rdataset_t ** sigrdataset);
static void	  nameFromString(const char *str, dns_name_t *p_ret);
int		  inf_name(dns_name_t * name1, dns_name_t * name2);
isc_result_t	  opentmpkey(isc_mem_t *mctx, const char *file,
			     char **tempp, FILE **fp);
isc_result_t	  removetmpkey(isc_mem_t *mctx, const char *file);
void		  clean_trustedkey(void);
void		  insert_trustedkey(dst_key_t  * key);
#if DIG_SIGCHASE_BU
isc_result_t	  getneededrr(dns_message_t *msg);
void		  sigchase_bottom_up(dns_message_t *msg);
void		  sigchase_bu(dns_message_t *msg);
#endif
#if DIG_SIGCHASE_TD
isc_result_t	  initialization(dns_name_t *name);
isc_result_t	  prepare_lookup(dns_name_t *name);
isc_result_t	  grandfather_pb_test(dns_name_t * zone_name,
				      dns_rdataset_t *sigrdataset);
isc_result_t	  child_of_zone(dns_name_t *name,
				dns_name_t *zone_name,
				dns_name_t *child_name);
void		  sigchase_td(dns_message_t *msg);
#endif
char trustedkey[MXNAME] = "";

dns_rdataset_t *chase_rdataset = NULL;
dns_rdataset_t *chase_sigrdataset = NULL;
dns_rdataset_t *chase_dsrdataset = NULL;
dns_rdataset_t *chase_sigdsrdataset = NULL;
dns_rdataset_t *chase_keyrdataset = NULL;
dns_rdataset_t *chase_sigkeyrdataset = NULL;
dns_rdataset_t *chase_nsrdataset = NULL;

dns_name_t chase_name; /* the query name */
#if DIG_SIGCHASE_TD
/*
 * the current name is the parent name when we follow delegation
 */
dns_name_t chase_current_name;
/*
 * the child name is used for delegation (NS DS responses in AUTHORITY section)
 */
dns_name_t chase_authority_name;
#endif
#if DIG_SIGCHASE_BU
dns_name_t chase_signame;
#endif


isc_boolean_t chase_siglookedup = ISC_FALSE;
isc_boolean_t chase_keylookedup = ISC_FALSE;
isc_boolean_t chase_sigkeylookedup = ISC_FALSE;
isc_boolean_t chase_dslookedup = ISC_FALSE;
isc_boolean_t chase_sigdslookedup = ISC_FALSE;
#if DIG_SIGCHASE_TD
isc_boolean_t chase_nslookedup = ISC_FALSE;
isc_boolean_t chase_lookedup = ISC_FALSE;


isc_boolean_t delegation_follow = ISC_FALSE;
isc_boolean_t grandfather_pb = ISC_FALSE;
isc_boolean_t have_response = ISC_FALSE;
isc_boolean_t have_delegation_ns = ISC_FALSE;
dns_message_t * error_message = NULL;
#endif

isc_boolean_t dsvalidating = ISC_FALSE;
isc_boolean_t chase_name_dup = ISC_FALSE;

ISC_LIST(dig_message_t) chase_message_list;
ISC_LIST(dig_message_t) chase_message_list2;


#define MAX_TRUSTED_KEY 5
typedef struct struct_trusted_key_list {
	dst_key_t * key[MAX_TRUSTED_KEY];
	int nb_tk;
} struct_tk_list;

struct_tk_list tk_list = { {NULL, NULL, NULL, NULL, NULL}, 0};

#endif

#define DIG_MAX_ADDRESSES 20

/*%
 * Apply and clear locks at the event level in global task.
 * Can I get rid of these using shutdown events?  XXX
 */
#define LOCK_LOOKUP {\
	debug("lock_lookup %s:%d", __FILE__, __LINE__);\
	check_result(isc_mutex_lock((&lookup_lock)), "isc_mutex_lock");\
	debug("success");\
}
#define UNLOCK_LOOKUP {\
	debug("unlock_lookup %s:%d", __FILE__, __LINE__);\
	check_result(isc_mutex_unlock((&lookup_lock)),\
		     "isc_mutex_unlock");\
}

static void
cancel_lookup(dig_lookup_t *lookup);

static void
recv_done(isc_task_t *task, isc_event_t *event);

static void
send_udp(dig_query_t *query);

static void
connect_timeout(isc_task_t *task, isc_event_t *event);

static void
launch_next_query(dig_query_t *query, isc_boolean_t include_question);


static void *
mem_alloc(void *arg, size_t size) {
	return (isc_mem_get(arg, size));
}

static void
mem_free(void *arg, void *mem, size_t size) {
	isc_mem_put(arg, mem, size);
}

char *
next_token(char **stringp, const char *delim) {
	char *res;

	do {
		res = strsep(stringp, delim);
		if (res == NULL)
			break;
	} while (*res == '\0');
	return (res);
}

static int
count_dots(char *string) {
	char *s;
	int i = 0;

	s = string;
	while (*s != '\0') {
		if (*s == '.')
			i++;
		s++;
	}
	return (i);
}

static void
hex_dump(isc_buffer_t *b) {
	unsigned int len;
	isc_region_t r;

	isc_buffer_usedregion(b, &r);

	printf("%d bytes\n", r.length);
	for (len = 0; len < r.length; len++) {
		printf("%02x ", r.base[len]);
		if (len % 16 == 15)
			printf("\n");
	}
	if (len % 16 != 0)
		printf("\n");
}

/*%
 * Append 'len' bytes of 'text' at '*p', failing with
 * ISC_R_NOSPACE if that would advance p past 'end'.
 */
static isc_result_t
append(const char *text, int len, char **p, char *end) {
	if (len > end - *p)
		return (ISC_R_NOSPACE);
	memcpy(*p, text, len);
	*p += len;
	return (ISC_R_SUCCESS);
}

static isc_result_t
reverse_octets(const char *in, char **p, char *end) {
	char *dot = strchr(in, '.');
	int len;
	if (dot != NULL) {
		isc_result_t result;
		result = reverse_octets(dot + 1, p, end);
		if (result != ISC_R_SUCCESS)
			return (result);
		result = append(".", 1, p, end);
		if (result != ISC_R_SUCCESS)
			return (result);
		len = dot - in;
	} else {
		len = strlen(in);
	}
	return (append(in, len, p, end));
}

isc_result_t
get_reverse(char *reverse, size_t len, char *value, isc_boolean_t ip6_int,
	    isc_boolean_t strict)
{
	int r;
	isc_result_t result;
	isc_netaddr_t addr;

	addr.family = AF_INET6;
	r = inet_pton(AF_INET6, value, &addr.type.in6);
	if (r > 0) {
		/* This is a valid IPv6 address. */
		dns_fixedname_t fname;
		dns_name_t *name;
		unsigned int options = 0;

		if (ip6_int)
			options |= DNS_BYADDROPT_IPV6INT;
		dns_fixedname_init(&fname);
		name = dns_fixedname_name(&fname);
		result = dns_byaddr_createptrname2(&addr, options, name);
		if (result != ISC_R_SUCCESS)
			return (result);
		dns_name_format(name, reverse, len);
		return (ISC_R_SUCCESS);
	} else {
		/*
		 * Not a valid IPv6 address.  Assume IPv4.
		 * If 'strict' is not set, construct the
		 * in-addr.arpa name by blindly reversing
		 * octets whether or not they look like integers,
		 * so that this can be used for RFC2317 names
		 * and such.
		 */
		char *p = reverse;
		char *end = reverse + len;
		if (strict && inet_pton(AF_INET, value, &addr.type.in) != 1)
			return (DNS_R_BADDOTTEDQUAD);
		result = reverse_octets(value, &p, end);
		if (result != ISC_R_SUCCESS)
			return (result);
		/* Append .in-addr.arpa. and a terminating NUL. */
		result = append(".in-addr.arpa.", 15, &p, end);
		if (result != ISC_R_SUCCESS)
			return (result);
		return (ISC_R_SUCCESS);
	}
}

void
fatal(const char *format, ...) {
	va_list args;

	fflush(stdout);
	fprintf(stderr, "%s: ", progname);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	if (exitcode < 10)
		exitcode = 10;
	if (fatalexit != 0)
		exitcode = fatalexit;
	exit(exitcode);
}

void
debug(const char *format, ...) {
	va_list args;

	if (debugging) {
		fflush(stdout);
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		fprintf(stderr, "\n");
	}
}

void
check_result(isc_result_t result, const char *msg) {
	if (result != ISC_R_SUCCESS) {
		fatal("%s: %s", msg, isc_result_totext(result));
	}
}

/*%
 * Create a server structure, which is part of the lookup structure.
 * This is little more than a linked list of servers to query in hopes
 * of finding the answer the user is looking for
 */
dig_server_t *
make_server(const char *servname, const char *userarg) {
	dig_server_t *srv;

	REQUIRE(servname != NULL);

	debug("make_server(%s)", servname);
	srv = isc_mem_allocate(mctx, sizeof(struct dig_server));
	if (srv == NULL)
		fatal("memory allocation failure in %s:%d",
		      __FILE__, __LINE__);
	strncpy(srv->servername, servname, MXNAME);
	strncpy(srv->userarg, userarg, MXNAME);
	srv->servername[MXNAME-1] = 0;
	srv->userarg[MXNAME-1] = 0;
	ISC_LINK_INIT(srv, link);
	return (srv);
}

static int
addr2af(int lwresaddrtype)
{
	int af = 0;

	switch (lwresaddrtype) {
	case LWRES_ADDRTYPE_V4:
		af = AF_INET;
		break;

	case LWRES_ADDRTYPE_V6:
		af = AF_INET6;
		break;
	}

	return (af);
}

/*%
 * Create a copy of the server list from the lwres configuration structure.
 * The dest list must have already had ISC_LIST_INIT applied.
 */
static void
copy_server_list(lwres_conf_t *confdata, dig_serverlist_t *dest) {
	dig_server_t *newsrv;
	char tmp[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	int af;
	int i;

	debug("copy_server_list()");
	for (i = 0; i < confdata->nsnext; i++) {
		af = addr2af(confdata->nameservers[i].family);

		if (af == AF_INET && !have_ipv4)
			continue;
		if (af == AF_INET6 && !have_ipv6)
			continue;

		lwres_net_ntop(af, confdata->nameservers[i].address,
				   tmp, sizeof(tmp));
		newsrv = make_server(tmp, tmp);
		ISC_LINK_INIT(newsrv, link);
		ISC_LIST_ENQUEUE(*dest, newsrv, link);
	}
}

void
flush_server_list(void) {
	dig_server_t *s, *ps;

	debug("flush_server_list()");
	s = ISC_LIST_HEAD(server_list);
	while (s != NULL) {
		ps = s;
		s = ISC_LIST_NEXT(s, link);
		ISC_LIST_DEQUEUE(server_list, ps, link);
		isc_mem_free(mctx, ps);
	}
}

void
set_nameserver(char *opt) {
	isc_result_t result;
	isc_sockaddr_t sockaddrs[DIG_MAX_ADDRESSES];
	isc_netaddr_t netaddr;
	int count, i;
	dig_server_t *srv;
	char tmp[ISC_NETADDR_FORMATSIZE];

	if (opt == NULL)
		return;

	result = bind9_getaddresses(opt, 0, sockaddrs,
				    DIG_MAX_ADDRESSES, &count);
	if (result != ISC_R_SUCCESS)
		fatal("couldn't get address for '%s': %s",
		      opt, isc_result_totext(result));

	flush_server_list();

	for (i = 0; i < count; i++) {
		isc_netaddr_fromsockaddr(&netaddr, &sockaddrs[i]);
		isc_netaddr_format(&netaddr, tmp, sizeof(tmp));
		srv = make_server(tmp, opt);
		if (srv == NULL)
			fatal("memory allocation failure");
		ISC_LIST_APPEND(server_list, srv, link);
	}
}

static isc_result_t
add_nameserver(lwres_conf_t *confdata, const char *addr, int af) {

	int i = confdata->nsnext;

	if (confdata->nsnext >= LWRES_CONFMAXNAMESERVERS)
		return (ISC_R_FAILURE);

	switch (af) {
	case AF_INET:
		confdata->nameservers[i].family = LWRES_ADDRTYPE_V4;
		confdata->nameservers[i].length = NS_INADDRSZ;
		break;
	case AF_INET6:
		confdata->nameservers[i].family = LWRES_ADDRTYPE_V6;
		confdata->nameservers[i].length = NS_IN6ADDRSZ;
		break;
	default:
		return (ISC_R_FAILURE);
	}

	if (lwres_net_pton(af, addr, &confdata->nameservers[i].address) == 1) {
		confdata->nsnext++;
		return (ISC_R_SUCCESS);
	}
	return (ISC_R_FAILURE);
}

/*%
 * Produce a cloned server list.  The dest list must have already had
 * ISC_LIST_INIT applied.
 */
void
clone_server_list(dig_serverlist_t src, dig_serverlist_t *dest) {
	dig_server_t *srv, *newsrv;

	debug("clone_server_list()");
	srv = ISC_LIST_HEAD(src);
	while (srv != NULL) {
		newsrv = make_server(srv->servername, srv->userarg);
		ISC_LINK_INIT(newsrv, link);
		ISC_LIST_ENQUEUE(*dest, newsrv, link);
		srv = ISC_LIST_NEXT(srv, link);
	}
}

/*%
 * Create an empty lookup structure, which holds all the information needed
 * to get an answer to a user's question.  This structure contains two
 * linked lists: the server list (servers to query) and the query list
 * (outstanding queries which have been made to the listed servers).
 */
dig_lookup_t *
make_empty_lookup(void) {
	dig_lookup_t *looknew;

	debug("make_empty_lookup()");

	INSIST(!free_now);

	looknew = isc_mem_allocate(mctx, sizeof(struct dig_lookup));
	if (looknew == NULL)
		fatal("memory allocation failure in %s:%d",
		       __FILE__, __LINE__);
	looknew->pending = ISC_TRUE;
	looknew->textname[0] = 0;
	looknew->cmdline[0] = 0;
	looknew->rdtype = dns_rdatatype_a;
	looknew->qrdtype = dns_rdatatype_a;
	looknew->rdclass = dns_rdataclass_in;
	looknew->rdtypeset = ISC_FALSE;
	looknew->rdclassset = ISC_FALSE;
	looknew->sendspace = NULL;
	looknew->sendmsg = NULL;
	looknew->name = NULL;
	looknew->oname = NULL;
	looknew->timer = NULL;
	looknew->xfr_q = NULL;
	looknew->current_query = NULL;
	looknew->doing_xfr = ISC_FALSE;
	looknew->ixfr_serial = ISC_FALSE;
	looknew->trace = ISC_FALSE;
	looknew->trace_root = ISC_FALSE;
	looknew->identify = ISC_FALSE;
	looknew->identify_previous_line = ISC_FALSE;
	looknew->ignore = ISC_FALSE;
	looknew->servfail_stops = ISC_TRUE;
	looknew->besteffort = ISC_TRUE;
	looknew->dnssec = ISC_FALSE;
	looknew->nsid = ISC_FALSE;
#ifdef DIG_SIGCHASE
	looknew->sigchase = ISC_FALSE;
#if DIG_SIGCHASE_TD
	looknew->do_topdown = ISC_FALSE;
	looknew->trace_root_sigchase = ISC_FALSE;
	looknew->rdtype_sigchaseset = ISC_FALSE;
	looknew->rdtype_sigchase = dns_rdatatype_any;
	looknew->qrdtype_sigchase = dns_rdatatype_any;
	looknew->rdclass_sigchase = dns_rdataclass_in;
	looknew->rdclass_sigchaseset = ISC_FALSE;
#endif
#endif
	looknew->udpsize = 0;
	looknew->edns = -1;
	looknew->recurse = ISC_TRUE;
	looknew->aaonly = ISC_FALSE;
	looknew->adflag = ISC_FALSE;
	looknew->cdflag = ISC_FALSE;
	looknew->ns_search_only = ISC_FALSE;
	looknew->origin = NULL;
	looknew->tsigctx = NULL;
	looknew->querysig = NULL;
	looknew->retries = tries;
	looknew->nsfound = 0;
	looknew->tcp_mode = ISC_FALSE;
	looknew->ip6_int = ISC_FALSE;
	looknew->comments = ISC_TRUE;
	looknew->stats = ISC_TRUE;
	looknew->section_question = ISC_TRUE;
	looknew->section_answer = ISC_TRUE;
	looknew->section_authority = ISC_TRUE;
	looknew->section_additional = ISC_TRUE;
	looknew->new_search = ISC_FALSE;
	looknew->done_as_is = ISC_FALSE;
	looknew->need_search = ISC_FALSE;
	ISC_LINK_INIT(looknew, link);
	ISC_LIST_INIT(looknew->q);
	ISC_LIST_INIT(looknew->my_server_list);
	return (looknew);
}

/*%
 * Clone a lookup, perhaps copying the server list.  This does not clone
 * the query list, since it will be regenerated by the setup_lookup()
 * function, nor does it queue up the new lookup for processing.
 * Caution: If you don't clone the servers, you MUST clone the server
 * list separately from somewhere else, or construct it by hand.
 */
dig_lookup_t *
clone_lookup(dig_lookup_t *lookold, isc_boolean_t servers) {
	dig_lookup_t *looknew;

	debug("clone_lookup()");

	INSIST(!free_now);

	looknew = make_empty_lookup();
	INSIST(looknew != NULL);
	strncpy(looknew->textname, lookold->textname, MXNAME);
#if DIG_SIGCHASE_TD
	strncpy(looknew->textnamesigchase, lookold->textnamesigchase, MXNAME);
#endif
	strncpy(looknew->cmdline, lookold->cmdline, MXNAME);
	looknew->textname[MXNAME-1] = 0;
	looknew->rdtype = lookold->rdtype;
	looknew->qrdtype = lookold->qrdtype;
	looknew->rdclass = lookold->rdclass;
	looknew->rdtypeset = lookold->rdtypeset;
	looknew->rdclassset = lookold->rdclassset;
	looknew->doing_xfr = lookold->doing_xfr;
	looknew->ixfr_serial = lookold->ixfr_serial;
	looknew->trace = lookold->trace;
	looknew->trace_root = lookold->trace_root;
	looknew->identify = lookold->identify;
	looknew->identify_previous_line = lookold->identify_previous_line;
	looknew->ignore = lookold->ignore;
	looknew->servfail_stops = lookold->servfail_stops;
	looknew->besteffort = lookold->besteffort;
	looknew->dnssec = lookold->dnssec;
	looknew->nsid = lookold->nsid;
#ifdef DIG_SIGCHASE
	looknew->sigchase = lookold->sigchase;
#if DIG_SIGCHASE_TD
	looknew->do_topdown = lookold->do_topdown;
	looknew->trace_root_sigchase = lookold->trace_root_sigchase;
	looknew->rdtype_sigchaseset = lookold->rdtype_sigchaseset;
	looknew->rdtype_sigchase = lookold->rdtype_sigchase;
	looknew->qrdtype_sigchase = lookold->qrdtype_sigchase;
	looknew->rdclass_sigchase = lookold->rdclass_sigchase;
	looknew->rdclass_sigchaseset = lookold->rdclass_sigchaseset;
#endif
#endif
	looknew->udpsize = lookold->udpsize;
	looknew->edns = lookold->edns;
	looknew->recurse = lookold->recurse;
	looknew->aaonly = lookold->aaonly;
	looknew->adflag = lookold->adflag;
	looknew->cdflag = lookold->cdflag;
	looknew->ns_search_only = lookold->ns_search_only;
	looknew->tcp_mode = lookold->tcp_mode;
	looknew->comments = lookold->comments;
	looknew->stats = lookold->stats;
	looknew->section_question = lookold->section_question;
	looknew->section_answer = lookold->section_answer;
	looknew->section_authority = lookold->section_authority;
	looknew->section_additional = lookold->section_additional;
	looknew->retries = lookold->retries;
	looknew->tsigctx = NULL;
	looknew->need_search = lookold->need_search;
	looknew->done_as_is = lookold->done_as_is;

	if (servers)
		clone_server_list(lookold->my_server_list,
				  &looknew->my_server_list);
	return (looknew);
}

/*%
 * Requeue a lookup for further processing, perhaps copying the server
 * list.  The new lookup structure is returned to the caller, and is
 * queued for processing.  If servers are not cloned in the requeue, they
 * must be added before allowing the current event to complete, since the
 * completion of the event may result in the next entry on the lookup
 * queue getting run.
 */
dig_lookup_t *
requeue_lookup(dig_lookup_t *lookold, isc_boolean_t servers) {
	dig_lookup_t *looknew;

	debug("requeue_lookup()");

	lookup_counter++;
	if (lookup_counter > LOOKUP_LIMIT)
		fatal("too many lookups");

	looknew = clone_lookup(lookold, servers);
	INSIST(looknew != NULL);

	debug("before insertion, init@%p -> %p, new@%p -> %p",
	      lookold, lookold->link.next, looknew, looknew->link.next);
	ISC_LIST_PREPEND(lookup_list, looknew, link);
	debug("after insertion, init -> %p, new = %p, new -> %p",
	      lookold, looknew, looknew->link.next);
	return (looknew);
}


static void
setup_text_key(void) {
	isc_result_t result;
	dns_name_t keyname;
	isc_buffer_t secretbuf;
	int secretsize;
	unsigned char *secretstore;

	debug("setup_text_key()");
	result = isc_buffer_allocate(mctx, &namebuf, MXNAME);
	check_result(result, "isc_buffer_allocate");
	dns_name_init(&keyname, NULL);
	check_result(result, "dns_name_init");
	isc_buffer_putstr(namebuf, keynametext);
	secretsize = strlen(keysecret) * 3 / 4;
	secretstore = isc_mem_allocate(mctx, secretsize);
	if (secretstore == NULL)
		fatal("memory allocation failure in %s:%d",
		      __FILE__, __LINE__);
	isc_buffer_init(&secretbuf, secretstore, secretsize);
	result = isc_base64_decodestring(keysecret, &secretbuf);
	if (result != ISC_R_SUCCESS)
		goto failure;

	secretsize = isc_buffer_usedlength(&secretbuf);

	result = dns_name_fromtext(&keyname, namebuf,
				   dns_rootname, ISC_FALSE,
				   namebuf);
	if (result != ISC_R_SUCCESS)
		goto failure;

	result = dns_tsigkey_create(&keyname, hmacname, secretstore,
				    secretsize, ISC_FALSE, NULL, 0, 0, mctx,
				    NULL, &key);
 failure:
	if (result != ISC_R_SUCCESS)
		printf(";; Couldn't create key %s: %s\n",
		       keynametext, isc_result_totext(result));
	else
		dst_key_setbits(key->key, digestbits);

	isc_mem_free(mctx, secretstore);
	dns_name_invalidate(&keyname);
	isc_buffer_free(&namebuf);
}

static void
setup_file_key(void) {
	isc_result_t result;
	dst_key_t *dstkey = NULL;

	debug("setup_file_key()");
	result = dst_key_fromnamedfile(keyfile, DST_TYPE_PRIVATE | DST_TYPE_KEY,
				       mctx, &dstkey);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "Couldn't read key from %s: %s\n",
			keyfile, isc_result_totext(result));
		goto failure;
	}

	switch (dst_key_alg(dstkey)) {
	case DST_ALG_HMACMD5:
		hmacname = DNS_TSIG_HMACMD5_NAME;
		break;
	case DST_ALG_HMACSHA1:
		hmacname = DNS_TSIG_HMACSHA1_NAME;
		break;
	case DST_ALG_HMACSHA224:
		hmacname = DNS_TSIG_HMACSHA224_NAME;
		break;
	case DST_ALG_HMACSHA256:
		hmacname = DNS_TSIG_HMACSHA256_NAME;
		break;
	case DST_ALG_HMACSHA384:
		hmacname = DNS_TSIG_HMACSHA384_NAME;
		break;
	case DST_ALG_HMACSHA512:
		hmacname = DNS_TSIG_HMACSHA512_NAME;
		break;
	default:
		printf(";; Couldn't create key %s: bad algorithm\n",
		       keynametext);
		goto failure;
	}
	result = dns_tsigkey_createfromkey(dst_key_name(dstkey), hmacname,
					   dstkey, ISC_FALSE, NULL, 0, 0,
					   mctx, NULL, &key);
	if (result != ISC_R_SUCCESS) {
		printf(";; Couldn't create key %s: %s\n",
		       keynametext, isc_result_totext(result));
		goto failure;
	}
	dstkey = NULL;
 failure:
	if (dstkey != NULL)
		dst_key_free(&dstkey);
}

static dig_searchlist_t *
make_searchlist_entry(char *domain) {
	dig_searchlist_t *search;
	search = isc_mem_allocate(mctx, sizeof(*search));
	if (search == NULL)
		fatal("memory allocation failure in %s:%d",
		      __FILE__, __LINE__);
	strncpy(search->origin, domain, MXNAME);
	search->origin[MXNAME-1] = 0;
	ISC_LINK_INIT(search, link);
	return (search);
}

static void
create_search_list(lwres_conf_t *confdata) {
	int i;
	dig_searchlist_t *search;

	debug("create_search_list()");
	ISC_LIST_INIT(search_list);

	for (i = 0; i < confdata->searchnxt; i++) {
		search = make_searchlist_entry(confdata->search[i]);
		ISC_LIST_APPEND(search_list, search, link);
	}
}

/*%
 * Setup the system as a whole, reading key information and resolv.conf
 * settings.
 */
void
setup_system(void) {
	dig_searchlist_t *domain = NULL;
	lwres_result_t lwresult;
	unsigned int lwresflags;

	debug("setup_system()");

	lwresflags = LWRES_CONTEXT_SERVERMODE;
	if (have_ipv4)
		lwresflags |= LWRES_CONTEXT_USEIPV4;
	if (have_ipv6)
		lwresflags |= LWRES_CONTEXT_USEIPV6;

	lwresult = lwres_context_create(&lwctx, mctx, mem_alloc, mem_free,
					lwresflags);
	if (lwresult != LWRES_R_SUCCESS)
		fatal("lwres_context_create failed");

	lwresult = lwres_conf_parse(lwctx, RESOLV_CONF);
	if (lwresult != LWRES_R_SUCCESS && lwresult != LWRES_R_NOTFOUND)
		fatal("parse of %s failed", RESOLV_CONF);

	lwconf = lwres_conf_get(lwctx);

	/* Make the search list */
	if (lwconf->searchnxt > 0)
		create_search_list(lwconf);
	else { /* No search list. Use the domain name if any */
		if (lwconf->domainname != NULL) {
			domain = make_searchlist_entry(lwconf->domainname);
			ISC_LIST_INITANDAPPEND(search_list, domain, link);
			domain  = NULL;
		}
	}

	if (ndots == -1) {
		ndots = lwconf->ndots;
		debug("ndots is %d.", ndots);
	}

	copy_server_list(lwconf, &server_list);

	/* If we don't find a nameserver fall back to localhost */
	if (ISC_LIST_EMPTY(server_list)) {
		if (have_ipv4) {
			lwresult = add_nameserver(lwconf, "127.0.0.1", AF_INET);
			if (lwresult != ISC_R_SUCCESS)
				fatal("add_nameserver failed");
		}
		if (have_ipv6) {
			lwresult = add_nameserver(lwconf, "::1", AF_INET6);
			if (lwresult != ISC_R_SUCCESS)
				fatal("add_nameserver failed");
		}

		copy_server_list(lwconf, &server_list);
	}

#ifdef WITH_IDN
	initialize_idn();
#endif

	if (keyfile[0] != 0)
		setup_file_key();
	else if (keysecret[0] != 0)
		setup_text_key();
#ifdef DIG_SIGCHASE
	/* Setup the list of messages for +sigchase */
	ISC_LIST_INIT(chase_message_list);
	ISC_LIST_INIT(chase_message_list2);
	dns_name_init(&chase_name, NULL);
#if DIG_SIGCHASE_TD
	dns_name_init(&chase_current_name, NULL);
	dns_name_init(&chase_authority_name, NULL);
#endif
#if DIG_SIGCHASE_BU
	dns_name_init(&chase_signame, NULL);
#endif

#endif

}

static void
clear_searchlist(void) {
	dig_searchlist_t *search;
	while ((search = ISC_LIST_HEAD(search_list)) != NULL) {
		ISC_LIST_UNLINK(search_list, search, link);
		isc_mem_free(mctx, search);
	}
}

/*%
 * Override the search list derived from resolv.conf by 'domain'.
 */
void
set_search_domain(char *domain) {
	dig_searchlist_t *search;

	clear_searchlist();
	search = make_searchlist_entry(domain);
	ISC_LIST_APPEND(search_list, search, link);
}

/*%
 * Setup the ISC and DNS libraries for use by the system.
 */
void
setup_libs(void) {
	isc_result_t result;

	debug("setup_libs()");

	result = isc_net_probeipv4();
	if (result == ISC_R_SUCCESS)
		have_ipv4 = ISC_TRUE;

	result = isc_net_probeipv6();
	if (result == ISC_R_SUCCESS)
		have_ipv6 = ISC_TRUE;
	if (!have_ipv6 && !have_ipv4)
		fatal("can't find either v4 or v6 networking");

	result = isc_mem_create(0, 0, &mctx);
	check_result(result, "isc_mem_create");

	result = isc_taskmgr_create(mctx, 1, 0, &taskmgr);
	check_result(result, "isc_taskmgr_create");

	result = isc_task_create(taskmgr, 0, &global_task);
	check_result(result, "isc_task_create");

	result = isc_timermgr_create(mctx, &timermgr);
	check_result(result, "isc_timermgr_create");

	result = isc_socketmgr_create(mctx, &socketmgr);
	check_result(result, "isc_socketmgr_create");

	result = isc_entropy_create(mctx, &entp);
	check_result(result, "isc_entropy_create");

	result = dst_lib_init(mctx, entp, 0);
	check_result(result, "dst_lib_init");
	is_dst_up = ISC_TRUE;

	result = isc_mempool_create(mctx, COMMSIZE, &commctx);
	check_result(result, "isc_mempool_create");
	isc_mempool_setname(commctx, "COMMPOOL");
	/*
	 * 6 and 2 set as reasonable parameters for 3 or 4 nameserver
	 * systems.
	 */
	isc_mempool_setfreemax(commctx, 6);
	isc_mempool_setfillcount(commctx, 2);

	result = isc_mutex_init(&lookup_lock);
	check_result(result, "isc_mutex_init");

	dns_result_register();
}

/*%
 * Add EDNS0 option record to a message.  Currently, the only supported
 * options are UDP buffer size, the DO bit, and NSID request.
 */
static void
add_opt(dns_message_t *msg, isc_uint16_t udpsize, isc_uint16_t edns,
	isc_boolean_t dnssec, isc_boolean_t nsid)
{
	dns_rdataset_t *rdataset = NULL;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdata_t *rdata = NULL;
	isc_result_t result;

	debug("add_opt()");
	result = dns_message_gettemprdataset(msg, &rdataset);
	check_result(result, "dns_message_gettemprdataset");
	dns_rdataset_init(rdataset);
	result = dns_message_gettemprdatalist(msg, &rdatalist);
	check_result(result, "dns_message_gettemprdatalist");
	result = dns_message_gettemprdata(msg, &rdata);
	check_result(result, "dns_message_gettemprdata");

	debug("setting udp size of %d", udpsize);
	rdatalist->type = dns_rdatatype_opt;
	rdatalist->covers = 0;
	rdatalist->rdclass = udpsize;
	rdatalist->ttl = edns << 16;
	if (dnssec)
		rdatalist->ttl |= DNS_MESSAGEEXTFLAG_DO;
	if (nsid) {
		unsigned char data[4];
		isc_buffer_t buf;

		isc_buffer_init(&buf, data, sizeof(data));
		isc_buffer_putuint16(&buf, DNS_OPT_NSID);
		isc_buffer_putuint16(&buf, 0);
		rdata->data = data;
		rdata->length = sizeof(data);
	} else {
		rdata->data = NULL;
		rdata->length = 0;
	}
	ISC_LIST_INIT(rdatalist->rdata);
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	dns_rdatalist_tordataset(rdatalist, rdataset);
	result = dns_message_setopt(msg, rdataset);
	check_result(result, "dns_message_setopt");
}

/*%
 * Add a question section to a message, asking for the specified name,
 * type, and class.
 */
static void
add_question(dns_message_t *message, dns_name_t *name,
	     dns_rdataclass_t rdclass, dns_rdatatype_t rdtype)
{
	dns_rdataset_t *rdataset;
	isc_result_t result;

	debug("add_question()");
	rdataset = NULL;
	result = dns_message_gettemprdataset(message, &rdataset);
	check_result(result, "dns_message_gettemprdataset()");
	dns_rdataset_init(rdataset);
	dns_rdataset_makequestion(rdataset, rdclass, rdtype);
	ISC_LIST_APPEND(name->list, rdataset, link);
}

/*%
 * Check if we're done with all the queued lookups, which is true iff
 * all sockets, sends, and recvs are accounted for (counters == 0),
 * and the lookup list is empty.
 * If we are done, pass control back out to dighost_shutdown() (which is
 * part of dig.c, host.c, or nslookup.c) to either shutdown the system as
 * a whole or reseed the lookup list.
 */
static void
check_if_done(void) {
	debug("check_if_done()");
	debug("list %s", ISC_LIST_EMPTY(lookup_list) ? "empty" : "full");
	if (ISC_LIST_EMPTY(lookup_list) && current_lookup == NULL &&
	    sendcount == 0) {
		INSIST(sockcount == 0);
		INSIST(recvcount == 0);
		debug("shutting down");
		dighost_shutdown();
	}
}

/*%
 * Clear out a query when we're done with it.  WARNING: This routine
 * WILL invalidate the query pointer.
 */
static void
clear_query(dig_query_t *query) {
	dig_lookup_t *lookup;

	REQUIRE(query != NULL);

	debug("clear_query(%p)", query);

	lookup = query->lookup;

	if (lookup->current_query == query)
		lookup->current_query = NULL;

	ISC_LIST_UNLINK(lookup->q, query, link);
	if (ISC_LINK_LINKED(&query->recvbuf, link))
		ISC_LIST_DEQUEUE(query->recvlist, &query->recvbuf,
				 link);
	if (ISC_LINK_LINKED(&query->lengthbuf, link))
		ISC_LIST_DEQUEUE(query->lengthlist, &query->lengthbuf,
				 link);
	INSIST(query->recvspace != NULL);
	if (query->sock != NULL) {
		isc_socket_detach(&query->sock);
		sockcount--;
		debug("sockcount=%d", sockcount);
	}
	isc_mempool_put(commctx, query->recvspace);
	isc_buffer_invalidate(&query->recvbuf);
	isc_buffer_invalidate(&query->lengthbuf);
	if (query->waiting_senddone)
		query->pending_free = ISC_TRUE;
	else
		isc_mem_free(mctx, query);
}

/*%
 * Try and clear out a lookup if we're done with it.  Return ISC_TRUE if
 * the lookup was successfully cleared.  If ISC_TRUE is returned, the
 * lookup pointer has been invalidated.
 */
static isc_boolean_t
try_clear_lookup(dig_lookup_t *lookup) {
	dig_query_t *q;

	REQUIRE(lookup != NULL);

	debug("try_clear_lookup(%p)", lookup);

	if (ISC_LIST_HEAD(lookup->q) != NULL) {
		if (debugging) {
			q = ISC_LIST_HEAD(lookup->q);
			while (q != NULL) {
				debug("query to %s still pending", q->servname);
				q = ISC_LIST_NEXT(q, link);
			}
		}
		return (ISC_FALSE);
	}

	/*
	 * At this point, we know there are no queries on the lookup,
	 * so can make it go away also.
	 */
	destroy_lookup(lookup);
	return (ISC_TRUE);
}

void
destroy_lookup(dig_lookup_t *lookup) {
	dig_server_t *s;
	void *ptr;

	debug("destroy");
	s = ISC_LIST_HEAD(lookup->my_server_list);
	while (s != NULL) {
		debug("freeing server %p belonging to %p", s, lookup);
		ptr = s;
		s = ISC_LIST_NEXT(s, link);
		ISC_LIST_DEQUEUE(lookup->my_server_list,
				 (dig_server_t *)ptr, link);
		isc_mem_free(mctx, ptr);
	}
	if (lookup->sendmsg != NULL)
		dns_message_destroy(&lookup->sendmsg);
	if (lookup->querysig != NULL) {
		debug("freeing buffer %p", lookup->querysig);
		isc_buffer_free(&lookup->querysig);
	}
	if (lookup->timer != NULL)
		isc_timer_detach(&lookup->timer);
	if (lookup->sendspace != NULL)
		isc_mempool_put(commctx, lookup->sendspace);

	if (lookup->tsigctx != NULL)
		dst_context_destroy(&lookup->tsigctx);

	isc_mem_free(mctx, lookup);
}

/*%
 * If we can, start the next lookup in the queue running.
 * This assumes that the lookup on the head of the queue hasn't been
 * started yet.  It also removes the lookup from the head of the queue,
 * setting the current_lookup pointer pointing to it.
 */
void
start_lookup(void) {
	debug("start_lookup()");
	if (cancel_now)
		return;

	/*
	 * If there's a current lookup running, we really shouldn't get
	 * here.
	 */
	INSIST(current_lookup == NULL);

	current_lookup = ISC_LIST_HEAD(lookup_list);
	/*
	 * Put the current lookup somewhere so cancel_all can find it
	 */
	if (current_lookup != NULL) {
		ISC_LIST_DEQUEUE(lookup_list, current_lookup, link);
#if DIG_SIGCHASE_TD
		if (current_lookup->do_topdown &&
		    !current_lookup->rdtype_sigchaseset) {
			dst_key_t *trustedkey = NULL;
			isc_buffer_t *b = NULL;
			isc_region_t r;
			isc_result_t result;
			dns_name_t query_name;
			dns_name_t *key_name;
			int i;

			result = get_trusted_key(mctx);
			if (result != ISC_R_SUCCESS) {
				printf("\n;; No trusted key, "
				       "+sigchase option is disabled\n");
				current_lookup->sigchase = ISC_FALSE;
				goto novalidation;
			}
			dns_name_init(&query_name, NULL);
			nameFromString(current_lookup->textname, &query_name);

			for (i = 0; i < tk_list.nb_tk; i++) {
				key_name = dst_key_name(tk_list.key[i]);

				if (dns_name_issubdomain(&query_name,
							 key_name) == ISC_TRUE)
					trustedkey = tk_list.key[i];
				/*
				 * Verify temp is really the lowest
				 * WARNING
				 */
			}
			if (trustedkey == NULL) {
				printf("\n;; The queried zone: ");
				dns_name_print(&query_name, stdout);
				printf(" isn't a subdomain of any Trusted Keys"
				       ": +sigchase option is disable\n");
				current_lookup->sigchase = ISC_FALSE;
				free_name(&query_name, mctx);
				goto novalidation;
			}
			free_name(&query_name, mctx);

			current_lookup->rdtype_sigchase
				= current_lookup->rdtype;
			current_lookup->rdtype_sigchaseset
				= current_lookup->rdtypeset;
			current_lookup->rdtype = dns_rdatatype_ns;

			current_lookup->qrdtype_sigchase
				= current_lookup->qrdtype;
			current_lookup->qrdtype = dns_rdatatype_ns;

			current_lookup->rdclass_sigchase
				= current_lookup->rdclass;
			current_lookup->rdclass_sigchaseset
				= current_lookup->rdclassset;
			current_lookup->rdclass = dns_rdataclass_in;

			strncpy(current_lookup->textnamesigchase,
				current_lookup->textname, MXNAME);

			current_lookup->trace_root_sigchase = ISC_TRUE;

			result = isc_buffer_allocate(mctx, &b, BUFSIZE);
			check_result(result, "isc_buffer_allocate");
			result = dns_name_totext(dst_key_name(trustedkey),
						 ISC_FALSE, b);
			check_result(result, "dns_name_totext");
			isc_buffer_usedregion(b, &r);
			r.base[r.length] = '\0';
			strncpy(current_lookup->textname, (char*)r.base,
				MXNAME);
			isc_buffer_free(&b);

			nameFromString(current_lookup->textnamesigchase,
				       &chase_name);

			dns_name_init(&chase_authority_name, NULL);
		}
	novalidation:
#endif
		setup_lookup(current_lookup);
		do_lookup(current_lookup);
	} else {
		check_if_done();
	}
}

/*%
 * If we can, clear the current lookup and start the next one running.
 * This calls try_clear_lookup, so may invalidate the lookup pointer.
 */
static void
check_next_lookup(dig_lookup_t *lookup) {

	INSIST(!free_now);

	debug("check_next_lookup(%p)", lookup);

	if (ISC_LIST_HEAD(lookup->q) != NULL) {
		debug("still have a worker");
		return;
	}
	if (try_clear_lookup(lookup)) {
		current_lookup = NULL;
		start_lookup();
	}
}

/*%
 * Create and queue a new lookup as a followup to the current lookup,
 * based on the supplied message and section.  This is used in trace and
 * name server search modes to start a new lookup using servers from
 * NS records in a reply. Returns the number of followup lookups made.
 */
static int
followup_lookup(dns_message_t *msg, dig_query_t *query, dns_section_t section)
{
	dig_lookup_t *lookup = NULL;
	dig_server_t *srv = NULL;
	dns_rdataset_t *rdataset = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_name_t *name = NULL;
	isc_result_t result;
	isc_boolean_t success = ISC_FALSE;
	int numLookups = 0;
	dns_name_t *domain;
	isc_boolean_t horizontal = ISC_FALSE, bad = ISC_FALSE;

	INSIST(!free_now);

	debug("following up %s", query->lookup->textname);

	for (result = dns_message_firstname(msg, section);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(msg, section)) {
		name = NULL;
		dns_message_currentname(msg, section, &name);

		if (section == DNS_SECTION_AUTHORITY) {
			rdataset = NULL;
			result = dns_message_findtype(name, dns_rdatatype_soa,
						      0, &rdataset);
			if (result == ISC_R_SUCCESS)
				return (0);
		}
		rdataset = NULL;
		result = dns_message_findtype(name, dns_rdatatype_ns, 0,
					      &rdataset);
		if (result != ISC_R_SUCCESS)
			continue;

		debug("found NS set");

		if (query->lookup->trace && !query->lookup->trace_root) {
			dns_namereln_t namereln;
			unsigned int nlabels;
			int order;

			domain = dns_fixedname_name(&query->lookup->fdomain);
			namereln = dns_name_fullcompare(name, domain,
							&order, &nlabels);
			if (namereln == dns_namereln_equal) {
				if (!horizontal)
					printf(";; BAD (HORIZONTAL) REFERRAL\n");
				horizontal = ISC_TRUE;
			} else if (namereln != dns_namereln_subdomain) {
				if (!bad)
					printf(";; BAD REFERRAL\n");
				bad = ISC_TRUE;
				continue;
			}
		}

		for (result = dns_rdataset_first(rdataset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(rdataset)) {
			char namestr[DNS_NAME_FORMATSIZE];
			dns_rdata_ns_t ns;

			if (query->lookup->trace_root &&
			    query->lookup->nsfound >= MXSERV)
				break;

			dns_rdataset_current(rdataset, &rdata);

			query->lookup->nsfound++;
			result = dns_rdata_tostruct(&rdata, &ns, NULL);
			check_result(result, "dns_rdata_tostruct");
			dns_name_format(&ns.name, namestr, sizeof(namestr));
			dns_rdata_freestruct(&ns);

			/* Initialize lookup if we've not yet */
			debug("found NS %d %s", numLookups, namestr);
			numLookups++;
			if (!success) {
				success = ISC_TRUE;
				lookup_counter++;
				lookup = requeue_lookup(query->lookup,
							ISC_FALSE);
				cancel_lookup(query->lookup);
				lookup->doing_xfr = ISC_FALSE;
				if (!lookup->trace_root &&
				    section == DNS_SECTION_ANSWER)
					lookup->trace = ISC_FALSE;
				else
					lookup->trace = query->lookup->trace;
				lookup->ns_search_only =
					query->lookup->ns_search_only;
				lookup->trace_root = ISC_FALSE;
				if (lookup->ns_search_only)
					lookup->recurse = ISC_FALSE;
				dns_fixedname_init(&lookup->fdomain);
				domain = dns_fixedname_name(&lookup->fdomain);
				dns_name_copy(name, domain, NULL);
			}
			srv = make_server(namestr, namestr);
			debug("adding server %s", srv->servername);
			ISC_LIST_APPEND(lookup->my_server_list, srv, link);
			dns_rdata_reset(&rdata);
		}
	}

	if (lookup == NULL &&
	    section == DNS_SECTION_ANSWER &&
	    (query->lookup->trace || query->lookup->ns_search_only))
		return (followup_lookup(msg, query, DNS_SECTION_AUTHORITY));

	/*
	 * Randomize the order the nameserver will be tried.
	 */
	if (numLookups > 1) {
		isc_uint32_t i, j;
		dig_serverlist_t my_server_list;

		ISC_LIST_INIT(my_server_list);

		for (i = numLookups; i > 0; i--) {
			isc_random_get(&j);
			j %= i;
			srv = ISC_LIST_HEAD(lookup->my_server_list);
			while (j-- > 0)
				srv = ISC_LIST_NEXT(srv, link);
			ISC_LIST_DEQUEUE(lookup->my_server_list, srv, link);
			ISC_LIST_APPEND(my_server_list, srv, link);
		}
		ISC_LIST_APPENDLIST(lookup->my_server_list,
				    my_server_list, link);
	}

	return (numLookups);
}

/*%
 * Create and queue a new lookup using the next origin from the search
 * list, read in setup_system().
 *
 * Return ISC_TRUE iff there was another searchlist entry.
 */
static isc_boolean_t
next_origin(dns_message_t *msg, dig_query_t *query) {
	dig_lookup_t *lookup;
	dig_searchlist_t *search;

	UNUSED(msg);

	INSIST(!free_now);

	debug("next_origin()");
	debug("following up %s", query->lookup->textname);

	if (!usesearch)
		/*
		 * We're not using a search list, so don't even think
		 * about finding the next entry.
		 */
		return (ISC_FALSE);
	if (query->lookup->origin == NULL && !query->lookup->need_search)
		/*
		 * Then we just did rootorg; there's nothing left.
		 */
		return (ISC_FALSE);
	if (query->lookup->origin == NULL && query->lookup->need_search) {
		lookup = requeue_lookup(query->lookup, ISC_TRUE);
		lookup->origin = ISC_LIST_HEAD(search_list);
		lookup->need_search = ISC_FALSE;
	} else {
		search = ISC_LIST_NEXT(query->lookup->origin, link);
		if (search == NULL && query->lookup->done_as_is)
			return (ISC_FALSE);
		lookup = requeue_lookup(query->lookup, ISC_TRUE);
		lookup->origin = search;
	}
	cancel_lookup(query->lookup);
	return (ISC_TRUE);
}

/*%
 * Insert an SOA record into the sendmessage in a lookup.  Used for
 * creating IXFR queries.
 */
static void
insert_soa(dig_lookup_t *lookup) {
	isc_result_t result;
	dns_rdata_soa_t soa;
	dns_rdata_t *rdata = NULL;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdataset_t *rdataset = NULL;
	dns_name_t *soaname = NULL;

	debug("insert_soa()");
	soa.mctx = mctx;
	soa.serial = lookup->ixfr_serial;
	soa.refresh = 0;
	soa.retry = 0;
	soa.expire = 0;
	soa.minimum = 0;
	soa.common.rdclass = lookup->rdclass;
	soa.common.rdtype = dns_rdatatype_soa;

	dns_name_init(&soa.origin, NULL);
	dns_name_init(&soa.contact, NULL);

	dns_name_clone(dns_rootname, &soa.origin);
	dns_name_clone(dns_rootname, &soa.contact);

	isc_buffer_init(&lookup->rdatabuf, lookup->rdatastore,
			sizeof(lookup->rdatastore));

	result = dns_message_gettemprdata(lookup->sendmsg, &rdata);
	check_result(result, "dns_message_gettemprdata");

	result = dns_rdata_fromstruct(rdata, lookup->rdclass,
				      dns_rdatatype_soa, &soa,
				      &lookup->rdatabuf);
	check_result(result, "isc_rdata_fromstruct");

	result = dns_message_gettemprdatalist(lookup->sendmsg, &rdatalist);
	check_result(result, "dns_message_gettemprdatalist");

	result = dns_message_gettemprdataset(lookup->sendmsg, &rdataset);
	check_result(result, "dns_message_gettemprdataset");

	dns_rdatalist_init(rdatalist);
	rdatalist->type = dns_rdatatype_soa;
	rdatalist->rdclass = lookup->rdclass;
	rdatalist->covers = 0;
	rdatalist->ttl = 0;
	ISC_LIST_INIT(rdatalist->rdata);
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);

	dns_rdataset_init(rdataset);
	dns_rdatalist_tordataset(rdatalist, rdataset);

	result = dns_message_gettempname(lookup->sendmsg, &soaname);
	check_result(result, "dns_message_gettempname");
	dns_name_init(soaname, NULL);
	dns_name_clone(lookup->name, soaname);
	ISC_LIST_INIT(soaname->list);
	ISC_LIST_APPEND(soaname->list, rdataset, link);
	dns_message_addname(lookup->sendmsg, soaname, DNS_SECTION_AUTHORITY);
}

/*%
 * Setup the supplied lookup structure, making it ready to start sending
 * queries to servers.  Create and initialize the message to be sent as
 * well as the query structures and buffer space for the replies.  If the
 * server list is empty, clone it from the system default list.
 */
void
setup_lookup(dig_lookup_t *lookup) {
	isc_result_t result;
	isc_uint32_t id;
	int len;
	dig_server_t *serv;
	dig_query_t *query;
	isc_buffer_t b;
	dns_compress_t cctx;
	char store[MXNAME];
#ifdef WITH_IDN
	idn_result_t mr;
	char utf8_textname[MXNAME], utf8_origin[MXNAME], idn_textname[MXNAME];
#endif

#ifdef WITH_IDN
	result = dns_name_settotextfilter(output_filter);
	check_result(result, "dns_name_settotextfilter");
#endif

	REQUIRE(lookup != NULL);
	INSIST(!free_now);

	debug("setup_lookup(%p)", lookup);

	result = dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER,
				    &lookup->sendmsg);
	check_result(result, "dns_message_create");

	if (lookup->new_search) {
		debug("resetting lookup counter.");
		lookup_counter = 0;
	}

	if (ISC_LIST_EMPTY(lookup->my_server_list)) {
		debug("cloning server list");
		clone_server_list(server_list, &lookup->my_server_list);
	}
	result = dns_message_gettempname(lookup->sendmsg, &lookup->name);
	check_result(result, "dns_message_gettempname");
	dns_name_init(lookup->name, NULL);

	isc_buffer_init(&lookup->namebuf, lookup->namespace,
			sizeof(lookup->namespace));
	isc_buffer_init(&lookup->onamebuf, lookup->onamespace,
			sizeof(lookup->onamespace));

#ifdef WITH_IDN
	/*
	 * We cannot convert `textname' and `origin' separately.
	 * `textname' doesn't contain TLD, but local mapping needs
	 * TLD.
	 */
	mr = idn_encodename(IDN_LOCALCONV | IDN_DELIMMAP, lookup->textname,
			    utf8_textname, sizeof(utf8_textname));
	idn_check_result(mr, "convert textname to UTF-8");
#endif

	/*
	 * If the name has too many dots, force the origin to be NULL
	 * (which produces an absolute lookup).  Otherwise, take the origin
	 * we have if there's one in the struct already.  If it's NULL,
	 * take the first entry in the searchlist iff either usesearch
	 * is TRUE or we got a domain line in the resolv.conf file.
	 */
	if (lookup->new_search) {
#ifdef WITH_IDN
		if ((count_dots(utf8_textname) >= ndots) || !usesearch) {
			lookup->origin = NULL; /* Force abs lookup */
			lookup->done_as_is = ISC_TRUE;
			lookup->need_search = usesearch;
		} else if (lookup->origin == NULL && usesearch) {
			lookup->origin = ISC_LIST_HEAD(search_list);
			lookup->need_search = ISC_FALSE;
		}
#else
		if ((count_dots(lookup->textname) >= ndots) || !usesearch) {
			lookup->origin = NULL; /* Force abs lookup */
			lookup->done_as_is = ISC_TRUE;
			lookup->need_search = usesearch;
		} else if (lookup->origin == NULL && usesearch) {
			lookup->origin = ISC_LIST_HEAD(search_list);
			lookup->need_search = ISC_FALSE;
		}
#endif
	}

#ifdef WITH_IDN
	if (lookup->origin != NULL) {
		mr = idn_encodename(IDN_LOCALCONV | IDN_DELIMMAP,
				    lookup->origin->origin, utf8_origin,
				    sizeof(utf8_origin));
		idn_check_result(mr, "convert origin to UTF-8");
		mr = append_textname(utf8_textname, utf8_origin,
				     sizeof(utf8_textname));
		idn_check_result(mr, "append origin to textname");
	}
	mr = idn_encodename(idnoptions | IDN_LOCALMAP | IDN_NAMEPREP |
			    IDN_IDNCONV | IDN_LENCHECK, utf8_textname,
			    idn_textname, sizeof(idn_textname));
	idn_check_result(mr, "convert UTF-8 textname to IDN encoding");
#else
	if (lookup->origin != NULL) {
		debug("trying origin %s", lookup->origin->origin);
		result = dns_message_gettempname(lookup->sendmsg,
						 &lookup->oname);
		check_result(result, "dns_message_gettempname");
		dns_name_init(lookup->oname, NULL);
		/* XXX Helper funct to conv char* to name? */
		len = strlen(lookup->origin->origin);
		isc_buffer_init(&b, lookup->origin->origin, len);
		isc_buffer_add(&b, len);
		result = dns_name_fromtext(lookup->oname, &b, dns_rootname,
					   ISC_FALSE, &lookup->onamebuf);
		if (result != ISC_R_SUCCESS) {
			dns_message_puttempname(lookup->sendmsg,
						&lookup->name);
			dns_message_puttempname(lookup->sendmsg,
						&lookup->oname);
			fatal("'%s' is not in legal name syntax (%s)",
			      lookup->origin->origin,
			      isc_result_totext(result));
		}
		if (lookup->trace && lookup->trace_root) {
			dns_name_clone(dns_rootname, lookup->name);
		} else {
			len = strlen(lookup->textname);
			isc_buffer_init(&b, lookup->textname, len);
			isc_buffer_add(&b, len);
			result = dns_name_fromtext(lookup->name, &b,
						   lookup->oname, ISC_FALSE,
						   &lookup->namebuf);
		}
		if (result != ISC_R_SUCCESS) {
			dns_message_puttempname(lookup->sendmsg,
						&lookup->name);
			dns_message_puttempname(lookup->sendmsg,
						&lookup->oname);
			fatal("'%s' is not in legal name syntax (%s)",
			      lookup->textname, isc_result_totext(result));
		}
		dns_message_puttempname(lookup->sendmsg, &lookup->oname);
	} else
#endif
	{
		debug("using root origin");
		if (lookup->trace && lookup->trace_root)
			dns_name_clone(dns_rootname, lookup->name);
		else {
#ifdef WITH_IDN
			len = strlen(idn_textname);
			isc_buffer_init(&b, idn_textname, len);
			isc_buffer_add(&b, len);
			result = dns_name_fromtext(lookup->name, &b,
						   dns_rootname,
						   ISC_FALSE,
						   &lookup->namebuf);
#else
			len = strlen(lookup->textname);
			isc_buffer_init(&b, lookup->textname, len);
			isc_buffer_add(&b, len);
			result = dns_name_fromtext(lookup->name, &b,
						   dns_rootname,
						   ISC_FALSE,
						   &lookup->namebuf);
#endif
		}
		if (result != ISC_R_SUCCESS) {
			dns_message_puttempname(lookup->sendmsg,
						&lookup->name);
			isc_buffer_init(&b, store, MXNAME);
			fatal("'%s' is not a legal name "
			      "(%s)", lookup->textname,
			      isc_result_totext(result));
		}
	}
	dns_name_format(lookup->name, store, sizeof(store));
	trying(store, lookup);
	INSIST(dns_name_isabsolute(lookup->name));

	isc_random_get(&id);
	lookup->sendmsg->id = (unsigned short)id & 0xFFFF;
	lookup->sendmsg->opcode = dns_opcode_query;
	lookup->msgcounter = 0;
	/*
	 * If this is a trace request, completely disallow recursion, since
	 * it's meaningless for traces.
	 */
	if (lookup->trace || (lookup->ns_search_only && !lookup->trace_root))
		lookup->recurse = ISC_FALSE;

	if (lookup->recurse &&
	    lookup->rdtype != dns_rdatatype_axfr &&
	    lookup->rdtype != dns_rdatatype_ixfr) {
		debug("recursive query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_RD;
	}

	/* XXX aaflag */
	if (lookup->aaonly) {
		debug("AA query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_AA;
	}

	if (lookup->adflag) {
		debug("AD query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_AD;
	}

	if (lookup->cdflag) {
		debug("CD query");
		lookup->sendmsg->flags |= DNS_MESSAGEFLAG_CD;
	}

	dns_message_addname(lookup->sendmsg, lookup->name,
			    DNS_SECTION_QUESTION);

	if (lookup->trace && lookup->trace_root) {
		lookup->qrdtype = lookup->rdtype;
		lookup->rdtype = dns_rdatatype_ns;
	}

	if ((lookup->rdtype == dns_rdatatype_axfr) ||
	    (lookup->rdtype == dns_rdatatype_ixfr)) {
		/*
		 * Force TCP mode if we're doing an axfr.
		 */
		if (lookup->rdtype == dns_rdatatype_axfr) {
			lookup->doing_xfr = ISC_TRUE;
			lookup->tcp_mode = ISC_TRUE;
		} else if (lookup->tcp_mode) {
			lookup->doing_xfr = ISC_TRUE;
		}
	}

	add_question(lookup->sendmsg, lookup->name, lookup->rdclass,
		     lookup->rdtype);

	/* add_soa */
	if (lookup->rdtype == dns_rdatatype_ixfr)
		insert_soa(lookup);

	/* XXX Insist this? */
	lookup->tsigctx = NULL;
	lookup->querysig = NULL;
	if (key != NULL) {
		debug("initializing keys");
		result = dns_message_settsigkey(lookup->sendmsg, key);
		check_result(result, "dns_message_settsigkey");
	}

	lookup->sendspace = isc_mempool_get(commctx);
	if (lookup->sendspace == NULL)
		fatal("memory allocation failure");

	result = dns_compress_init(&cctx, -1, mctx);
	check_result(result, "dns_compress_init");

	debug("starting to render the message");
	isc_buffer_init(&lookup->renderbuf, lookup->sendspace, COMMSIZE);
	result = dns_message_renderbegin(lookup->sendmsg, &cctx,
					 &lookup->renderbuf);
	check_result(result, "dns_message_renderbegin");
	if (lookup->udpsize > 0 || lookup->dnssec || lookup->edns > -1) {
		if (lookup->udpsize == 0)
			lookup->udpsize = 4096;
		if (lookup->edns < 0)
			lookup->edns = 0;
		add_opt(lookup->sendmsg, lookup->udpsize,
			lookup->edns, lookup->dnssec, lookup->nsid);
	}

	result = dns_message_rendersection(lookup->sendmsg,
					   DNS_SECTION_QUESTION, 0);
	check_result(result, "dns_message_rendersection");
	result = dns_message_rendersection(lookup->sendmsg,
					   DNS_SECTION_AUTHORITY, 0);
	check_result(result, "dns_message_rendersection");
	result = dns_message_renderend(lookup->sendmsg);
	check_result(result, "dns_message_renderend");
	debug("done rendering");

	dns_compress_invalidate(&cctx);

	/*
	 * Force TCP mode if the request is larger than 512 bytes.
	 */
	if (isc_buffer_usedlength(&lookup->renderbuf) > 512)
		lookup->tcp_mode = ISC_TRUE;

	lookup->pending = ISC_FALSE;

	for (serv = ISC_LIST_HEAD(lookup->my_server_list);
	     serv != NULL;
	     serv = ISC_LIST_NEXT(serv, link)) {
		query = isc_mem_allocate(mctx, sizeof(dig_query_t));
		if (query == NULL)
			fatal("memory allocation failure in %s:%d",
			      __FILE__, __LINE__);
		debug("create query %p linked to lookup %p",
		       query, lookup);
		query->lookup = lookup;
		query->waiting_connect = ISC_FALSE;
		query->waiting_senddone = ISC_FALSE;
		query->pending_free = ISC_FALSE;
		query->recv_made = ISC_FALSE;
		query->first_pass = ISC_TRUE;
		query->first_soa_rcvd = ISC_FALSE;
		query->second_rr_rcvd = ISC_FALSE;
		query->first_repeat_rcvd = ISC_FALSE;
		query->warn_id = ISC_TRUE;
		query->first_rr_serial = 0;
		query->second_rr_serial = 0;
		query->servname = serv->servername;
		query->userarg = serv->userarg;
		query->rr_count = 0;
		query->msg_count = 0;
		query->byte_count = 0;
		ISC_LINK_INIT(query, link);
		ISC_LIST_INIT(query->recvlist);
		ISC_LIST_INIT(query->lengthlist);
		query->sock = NULL;
		query->recvspace = isc_mempool_get(commctx);
		if (query->recvspace == NULL)
			fatal("memory allocation failure");

		isc_buffer_init(&query->recvbuf, query->recvspace, COMMSIZE);
		isc_buffer_init(&query->lengthbuf, query->lengthspace, 2);
		isc_buffer_init(&query->slbuf, query->slspace, 2);
		query->sendbuf = lookup->renderbuf;

		ISC_LINK_INIT(query, link);
		ISC_LIST_ENQUEUE(lookup->q, query, link);
	}
	/* XXX qrflag, print_query, etc... */
	if (!ISC_LIST_EMPTY(lookup->q) && qr) {
		extrabytes = 0;
		printmessage(ISC_LIST_HEAD(lookup->q), lookup->sendmsg,
			     ISC_TRUE);
	}
}

/*%
 * Event handler for send completion.  Track send counter, and clear out
 * the query if the send was canceled.
 */
static void
send_done(isc_task_t *_task, isc_event_t *event) {
	isc_socketevent_t *sevent = (isc_socketevent_t *)event;
	isc_buffer_t *b = NULL;
	dig_query_t *query, *next;
	dig_lookup_t *l;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_SENDDONE);

	UNUSED(_task);

	LOCK_LOOKUP;

	debug("send_done()");
	sendcount--;
	debug("sendcount=%d", sendcount);
	INSIST(sendcount >= 0);

	for  (b = ISC_LIST_HEAD(sevent->bufferlist);
	      b != NULL;
	      b = ISC_LIST_HEAD(sevent->bufferlist))
		ISC_LIST_DEQUEUE(sevent->bufferlist, b, link);

	query = event->ev_arg;
	query->waiting_senddone = ISC_FALSE;
	l = query->lookup;

	if (l->ns_search_only && !l->trace_root) {
		debug("sending next, since searching");
		next = ISC_LIST_NEXT(query, link);
		if (next != NULL)
			send_udp(next);
	}

	isc_event_free(&event);

	if (query->pending_free)
		isc_mem_free(mctx, query);

	check_if_done();
	UNLOCK_LOOKUP;
}

/*%
 * Cancel a lookup, sending isc_socket_cancel() requests to all outstanding
 * IO sockets.  The cancel handlers should take care of cleaning up the
 * query and lookup structures
 */
static void
cancel_lookup(dig_lookup_t *lookup) {
	dig_query_t *query, *next;

	debug("cancel_lookup()");
	query = ISC_LIST_HEAD(lookup->q);
	while (query != NULL) {
		next = ISC_LIST_NEXT(query, link);
		if (query->sock != NULL) {
			isc_socket_cancel(query->sock, global_task,
					  ISC_SOCKCANCEL_ALL);
			check_if_done();
		} else {
			clear_query(query);
		}
		query = next;
	}
	if (lookup->timer != NULL)
		isc_timer_detach(&lookup->timer);
	lookup->pending = ISC_FALSE;
	lookup->retries = 0;
}

static void
bringup_timer(dig_query_t *query, unsigned int default_timeout) {
	dig_lookup_t *l;
	unsigned int local_timeout;
	isc_result_t result;

	debug("bringup_timer()");
	/*
	 * If the timer already exists, that means we're calling this
	 * a second time (for a retry).  Don't need to recreate it,
	 * just reset it.
	 */
	l = query->lookup;
	if (ISC_LIST_NEXT(query, link) != NULL)
		local_timeout = SERVER_TIMEOUT;
	else {
		if (timeout == 0)
			local_timeout = default_timeout;
		else
			local_timeout = timeout;
	}
	debug("have local timeout of %d", local_timeout);
	isc_interval_set(&l->interval, local_timeout, 0);
	if (l->timer != NULL)
		isc_timer_detach(&l->timer);
	result = isc_timer_create(timermgr, isc_timertype_once, NULL,
				  &l->interval, global_task, connect_timeout,
				  l, &l->timer);
	check_result(result, "isc_timer_create");
}

static void
force_timeout(dig_lookup_t *l, dig_query_t *query) {
	isc_event_t *event;

	event = isc_event_allocate(mctx, query, ISC_TIMEREVENT_IDLE,
				   connect_timeout, l,
				   sizeof(isc_event_t));
	if (event == NULL) {
		fatal("isc_event_allocate: %s",
		      isc_result_totext(ISC_R_NOMEMORY));
	}
	isc_task_send(global_task, &event);
}


static void
connect_done(isc_task_t *task, isc_event_t *event);

/*%
 * Unlike send_udp, this can't be called multiple times with the same
 * query.  When we retry TCP, we requeue the whole lookup, which should
 * start anew.
 */
static void
send_tcp_connect(dig_query_t *query) {
	isc_result_t result;
	dig_query_t *next;
	dig_lookup_t *l;

	debug("send_tcp_connect(%p)", query);

	l = query->lookup;
	query->waiting_connect = ISC_TRUE;
	query->lookup->current_query = query;
	result = get_address(query->servname, port, &query->sockaddr);
	if (result == ISC_R_NOTFOUND) {
		/*
		 * This servname doesn't have an address.  Try the next server
		 * by triggering an immediate 'timeout' (we lie, but the effect
		 * is the same).
		 */
		force_timeout(l, query);
		return;
	}

	if (specified_source &&
	    (isc_sockaddr_pf(&query->sockaddr) !=
	     isc_sockaddr_pf(&bind_address))) {
		printf(";; Skipping server %s, incompatible "
		       "address family\n", query->servname);
		query->waiting_connect = ISC_FALSE;
		next = ISC_LIST_NEXT(query, link);
		l = query->lookup;
		clear_query(query);
		if (next == NULL) {
			printf(";; No acceptable nameservers\n");
			check_next_lookup(l);
			return;
		}
		send_tcp_connect(next);
		return;
	}
	INSIST(query->sock == NULL);
	result = isc_socket_create(socketmgr,
				   isc_sockaddr_pf(&query->sockaddr),
				   isc_sockettype_tcp, &query->sock);
	check_result(result, "isc_socket_create");
	sockcount++;
	debug("sockcount=%d", sockcount);
	if (specified_source)
		result = isc_socket_bind(query->sock, &bind_address,
					 ISC_SOCKET_REUSEADDRESS);
	else {
		if ((isc_sockaddr_pf(&query->sockaddr) == AF_INET) &&
		    have_ipv4)
			isc_sockaddr_any(&bind_any);
		else
			isc_sockaddr_any6(&bind_any);
		result = isc_socket_bind(query->sock, &bind_any, 0);
	}
	check_result(result, "isc_socket_bind");
	bringup_timer(query, TCP_TIMEOUT);
	result = isc_socket_connect(query->sock, &query->sockaddr,
				    global_task, connect_done, query);
	check_result(result, "isc_socket_connect");
	/*
	 * If we're at the endgame of a nameserver search, we need to
	 * immediately bring up all the queries.  Do it here.
	 */
	if (l->ns_search_only && !l->trace_root) {
		debug("sending next, since searching");
		next = ISC_LIST_NEXT(query, link);
		if (next != NULL)
			send_tcp_connect(next);
	}
}

/*%
 * Send a UDP packet to the remote nameserver, possible starting the
 * recv action as well.  Also make sure that the timer is running and
 * is properly reset.
 */
static void
send_udp(dig_query_t *query) {
	dig_lookup_t *l = NULL;
	isc_result_t result;

	debug("send_udp(%p)", query);

	l = query->lookup;
	bringup_timer(query, UDP_TIMEOUT);
	l->current_query = query;
	debug("working on lookup %p, query %p", query->lookup, query);
	if (!query->recv_made) {
		/* XXX Check the sense of this, need assertion? */
		query->waiting_connect = ISC_FALSE;
		result = get_address(query->servname, port, &query->sockaddr);
		if (result == ISC_R_NOTFOUND) {
			/* This servname doesn't have an address. */
			force_timeout(l, query);
			return;
		}

		result = isc_socket_create(socketmgr,
					   isc_sockaddr_pf(&query->sockaddr),
					   isc_sockettype_udp, &query->sock);
		check_result(result, "isc_socket_create");
		sockcount++;
		debug("sockcount=%d", sockcount);
		if (specified_source) {
			result = isc_socket_bind(query->sock, &bind_address,
						 ISC_SOCKET_REUSEADDRESS);
		} else {
			isc_sockaddr_anyofpf(&bind_any,
					isc_sockaddr_pf(&query->sockaddr));
			result = isc_socket_bind(query->sock, &bind_any, 0);
		}
		check_result(result, "isc_socket_bind");

		query->recv_made = ISC_TRUE;
		ISC_LINK_INIT(&query->recvbuf, link);
		ISC_LIST_ENQUEUE(query->recvlist, &query->recvbuf,
				 link);
		debug("recving with lookup=%p, query=%p, sock=%p",
		      query->lookup, query, query->sock);
		result = isc_socket_recvv(query->sock, &query->recvlist, 1,
					  global_task, recv_done, query);
		check_result(result, "isc_socket_recvv");
		recvcount++;
		debug("recvcount=%d", recvcount);
	}
	ISC_LIST_INIT(query->sendlist);
	ISC_LIST_ENQUEUE(query->sendlist, &query->sendbuf, link);
	debug("sending a request");
	TIME_NOW(&query->time_sent);
	INSIST(query->sock != NULL);
	query->waiting_senddone = ISC_TRUE;
	result = isc_socket_sendtov(query->sock, &query->sendlist,
				    global_task, send_done, query,
				    &query->sockaddr, NULL);
	check_result(result, "isc_socket_sendtov");
	sendcount++;
}

/*%
 * IO timeout handler, used for both connect and recv timeouts.  If
 * retries are still allowed, either resend the UDP packet or queue a
 * new TCP lookup.  Otherwise, cancel the lookup.
 */
static void
connect_timeout(isc_task_t *task, isc_event_t *event) {
	dig_lookup_t *l = NULL;
	dig_query_t *query = NULL, *cq;

	UNUSED(task);
	REQUIRE(event->ev_type == ISC_TIMEREVENT_IDLE);

	debug("connect_timeout()");

	LOCK_LOOKUP;
	l = event->ev_arg;
	query = l->current_query;
	isc_event_free(&event);

	INSIST(!free_now);

	if ((query != NULL) && (query->lookup->current_query != NULL) &&
	    (ISC_LIST_NEXT(query->lookup->current_query, link) != NULL)) {
		debug("trying next server...");
		cq = query->lookup->current_query;
		if (!l->tcp_mode)
			send_udp(ISC_LIST_NEXT(cq, link));
		else {
			isc_socket_cancel(query->sock, NULL,
					  ISC_SOCKCANCEL_ALL);
			isc_socket_detach(&query->sock);
			sockcount--;
			debug("sockcount=%d", sockcount);
			send_tcp_connect(ISC_LIST_NEXT(cq, link));
		}
		UNLOCK_LOOKUP;
		return;
	}

	if (l->retries > 1) {
		if (!l->tcp_mode) {
			l->retries--;
			debug("resending UDP request to first server");
			send_udp(ISC_LIST_HEAD(l->q));
		} else {
			debug("making new TCP request, %d tries left",
			      l->retries);
			l->retries--;
			requeue_lookup(l, ISC_TRUE);
			cancel_lookup(l);
			check_next_lookup(l);
		}
	} else {
		fputs(l->cmdline, stdout);
		printf(";; connection timed out; no servers could be "
		       "reached\n");
		cancel_lookup(l);
		check_next_lookup(l);
		if (exitcode < 9)
			exitcode = 9;
	}
	UNLOCK_LOOKUP;
}

/*%
 * Event handler for the TCP recv which gets the length header of TCP
 * packets.  Start the next recv of length bytes.
 */
static void
tcp_length_done(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent;
	isc_buffer_t *b = NULL;
	isc_result_t result;
	dig_query_t *query = NULL;
	dig_lookup_t *l;
	isc_uint16_t length;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_RECVDONE);
	INSIST(!free_now);

	UNUSED(task);

	debug("tcp_length_done()");

	LOCK_LOOKUP;
	sevent = (isc_socketevent_t *)event;
	query = event->ev_arg;

	recvcount--;
	INSIST(recvcount >= 0);

	b = ISC_LIST_HEAD(sevent->bufferlist);
	INSIST(b ==  &query->lengthbuf);
	ISC_LIST_DEQUEUE(sevent->bufferlist, b, link);

	if (sevent->result == ISC_R_CANCELED) {
		isc_event_free(&event);
		l = query->lookup;
		clear_query(query);
		check_next_lookup(l);
		UNLOCK_LOOKUP;
		return;
	}
	if (sevent->result != ISC_R_SUCCESS) {
		char sockstr[ISC_SOCKADDR_FORMATSIZE];
		isc_sockaddr_format(&query->sockaddr, sockstr,
				    sizeof(sockstr));
		printf(";; communications error to %s: %s\n",
		       sockstr, isc_result_totext(sevent->result));
		l = query->lookup;
		isc_socket_detach(&query->sock);
		sockcount--;
		debug("sockcount=%d", sockcount);
		INSIST(sockcount >= 0);
		isc_event_free(&event);
		clear_query(query);
		check_next_lookup(l);
		UNLOCK_LOOKUP;
		return;
	}
	length = isc_buffer_getuint16(b);
	if (length == 0) {
		isc_event_free(&event);
		launch_next_query(query, ISC_FALSE);
		UNLOCK_LOOKUP;
		return;
	}

	/*
	 * Even though the buffer was already init'ed, we need
	 * to redo it now, to force the length we want.
	 */
	isc_buffer_invalidate(&query->recvbuf);
	isc_buffer_init(&query->recvbuf, query->recvspace, length);
	ENSURE(ISC_LIST_EMPTY(query->recvlist));
	ISC_LINK_INIT(&query->recvbuf, link);
	ISC_LIST_ENQUEUE(query->recvlist, &query->recvbuf, link);
	debug("recving with lookup=%p, query=%p", query->lookup, query);
	result = isc_socket_recvv(query->sock, &query->recvlist, length, task,
				  recv_done, query);
	check_result(result, "isc_socket_recvv");
	recvcount++;
	debug("resubmitted recv request with length %d, recvcount=%d",
	      length, recvcount);
	isc_event_free(&event);
	UNLOCK_LOOKUP;
}

/*%
 * For transfers that involve multiple recvs (XFR's in particular),
 * launch the next recv.
 */
static void
launch_next_query(dig_query_t *query, isc_boolean_t include_question) {
	isc_result_t result;
	dig_lookup_t *l;

	INSIST(!free_now);

	debug("launch_next_query()");

	if (!query->lookup->pending) {
		debug("ignoring launch_next_query because !pending");
		isc_socket_detach(&query->sock);
		sockcount--;
		debug("sockcount=%d", sockcount);
		INSIST(sockcount >= 0);
		query->waiting_connect = ISC_FALSE;
		l = query->lookup;
		clear_query(query);
		check_next_lookup(l);
		return;
	}

	isc_buffer_clear(&query->slbuf);
	isc_buffer_clear(&query->lengthbuf);
	isc_buffer_putuint16(&query->slbuf, (isc_uint16_t) query->sendbuf.used);
	ISC_LIST_INIT(query->sendlist);
	ISC_LINK_INIT(&query->slbuf, link);
	ISC_LIST_ENQUEUE(query->sendlist, &query->slbuf, link);
	if (include_question)
		ISC_LIST_ENQUEUE(query->sendlist, &query->sendbuf, link);
	ISC_LINK_INIT(&query->lengthbuf, link);
	ISC_LIST_ENQUEUE(query->lengthlist, &query->lengthbuf, link);

	result = isc_socket_recvv(query->sock, &query->lengthlist, 0,
				  global_task, tcp_length_done, query);
	check_result(result, "isc_socket_recvv");
	recvcount++;
	debug("recvcount=%d", recvcount);
	if (!query->first_soa_rcvd) {
		debug("sending a request in launch_next_query");
		TIME_NOW(&query->time_sent);
		query->waiting_senddone = ISC_TRUE;
		result = isc_socket_sendv(query->sock, &query->sendlist,
					  global_task, send_done, query);
		check_result(result, "isc_socket_sendv");
		sendcount++;
		debug("sendcount=%d", sendcount);
	}
	query->waiting_connect = ISC_FALSE;
#if 0
	check_next_lookup(query->lookup);
#endif
	return;
}

/*%
 * Event handler for TCP connect complete.  Make sure the connection was
 * successful, then pass into launch_next_query to actually send the
 * question.
 */
static void
connect_done(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = NULL;
	dig_query_t *query = NULL, *next;
	dig_lookup_t *l;

	UNUSED(task);

	REQUIRE(event->ev_type == ISC_SOCKEVENT_CONNECT);
	INSIST(!free_now);

	debug("connect_done()");

	LOCK_LOOKUP;
	sevent = (isc_socketevent_t *)event;
	query = sevent->ev_arg;

	INSIST(query->waiting_connect);

	query->waiting_connect = ISC_FALSE;

	if (sevent->result == ISC_R_CANCELED) {
		debug("in cancel handler");
		isc_socket_detach(&query->sock);
		sockcount--;
		INSIST(sockcount >= 0);
		debug("sockcount=%d", sockcount);
		query->waiting_connect = ISC_FALSE;
		isc_event_free(&event);
		l = query->lookup;
		clear_query(query);
		check_next_lookup(l);
		UNLOCK_LOOKUP;
		return;
	}
	if (sevent->result != ISC_R_SUCCESS) {
		char sockstr[ISC_SOCKADDR_FORMATSIZE];

		debug("unsuccessful connection: %s",
		      isc_result_totext(sevent->result));
		isc_sockaddr_format(&query->sockaddr, sockstr, sizeof(sockstr));
		if (sevent->result != ISC_R_CANCELED)
			printf(";; Connection to %s(%s) for %s failed: "
			       "%s.\n", sockstr,
			       query->servname, query->lookup->textname,
			       isc_result_totext(sevent->result));
		isc_socket_detach(&query->sock);
		sockcount--;
		INSIST(sockcount >= 0);
		/* XXX Clean up exitcodes */
		if (exitcode < 9)
			exitcode = 9;
		debug("sockcount=%d", sockcount);
		query->waiting_connect = ISC_FALSE;
		isc_event_free(&event);
		l = query->lookup;
		if (l->current_query != NULL)
			next = ISC_LIST_NEXT(l->current_query, link);
		else
			next = NULL;
		clear_query(query);
		if (next != NULL) {
			bringup_timer(next, TCP_TIMEOUT);
			send_tcp_connect(next);
		} else {
			check_next_lookup(l);
		}
		UNLOCK_LOOKUP;
		return;
	}
	launch_next_query(query, ISC_TRUE);
	isc_event_free(&event);
	UNLOCK_LOOKUP;
}

/*%
 * Check if the ongoing XFR needs more data before it's complete, using
 * the semantics of IXFR and AXFR protocols.  Much of the complexity of
 * this routine comes from determining when an IXFR is complete.
 * ISC_FALSE means more data is on the way, and the recv has been issued.
 */
static isc_boolean_t
check_for_more_data(dig_query_t *query, dns_message_t *msg,
		    isc_socketevent_t *sevent)
{
	dns_rdataset_t *rdataset = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_soa_t soa;
	isc_uint32_t serial;
	isc_result_t result;

	debug("check_for_more_data()");

	/*
	 * By the time we're in this routine, we know we're doing
	 * either an AXFR or IXFR.  If there's no second_rr_type,
	 * then we don't yet know which kind of answer we got back
	 * from the server.  Here, we're going to walk through the
	 * rr's in the message, acting as necessary whenever we hit
	 * an SOA rr.
	 */

	query->msg_count++;
	query->byte_count += sevent->n;
	result = dns_message_firstname(msg, DNS_SECTION_ANSWER);
	if (result != ISC_R_SUCCESS) {
		puts("; Transfer failed.");
		return (ISC_TRUE);
	}
	do {
		dns_name_t *name;
		name = NULL;
		dns_message_currentname(msg, DNS_SECTION_ANSWER,
					&name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			result = dns_rdataset_first(rdataset);
			if (result != ISC_R_SUCCESS)
				continue;
			do {
				query->rr_count++;
				dns_rdata_reset(&rdata);
				dns_rdataset_current(rdataset, &rdata);
				/*
				 * If this is the first rr, make sure
				 * it's an SOA
				 */
				if ((!query->first_soa_rcvd) &&
				    (rdata.type != dns_rdatatype_soa)) {
					puts("; Transfer failed.  "
					     "Didn't start with SOA answer.");
					return (ISC_TRUE);
				}
				if ((!query->second_rr_rcvd) &&
				    (rdata.type != dns_rdatatype_soa)) {
					query->second_rr_rcvd = ISC_TRUE;
					query->second_rr_serial = 0;
					debug("got the second rr as nonsoa");
					goto next_rdata;
				}

				/*
				 * If the record is anything except an SOA
				 * now, just continue on...
				 */
				if (rdata.type != dns_rdatatype_soa)
					goto next_rdata;
				/* Now we have an SOA.  Work with it. */
				debug("got an SOA");
				result = dns_rdata_tostruct(&rdata, &soa, NULL);
				check_result(result, "dns_rdata_tostruct");
				serial = soa.serial;
				dns_rdata_freestruct(&soa);
				if (!query->first_soa_rcvd) {
					query->first_soa_rcvd = ISC_TRUE;
					query->first_rr_serial = serial;
					debug("this is the first %d",
					       query->lookup->ixfr_serial);
					if (query->lookup->ixfr_serial >=
					    serial)
						goto doexit;
					goto next_rdata;
				}
				if (query->lookup->rdtype ==
				    dns_rdatatype_axfr) {
					debug("doing axfr, got second SOA");
					goto doexit;
				}
				if (!query->second_rr_rcvd) {
					if (query->first_rr_serial == serial) {
						debug("doing ixfr, got "
						      "empty zone");
						goto doexit;
					}
					debug("this is the second %d",
					       query->lookup->ixfr_serial);
					query->second_rr_rcvd = ISC_TRUE;
					query->second_rr_serial = serial;
					goto next_rdata;
				}
				if (query->second_rr_serial == 0) {
					/*
					 * If the second RR was a non-SOA
					 * record, and we're getting any
					 * other SOA, then this is an
					 * AXFR, and we're done.
					 */
					debug("done, since axfr");
					goto doexit;
				}
				/*
				 * If we get to this point, we're doing an
				 * IXFR and have to start really looking
				 * at serial numbers.
				 */
				if (query->first_rr_serial == serial) {
					debug("got a match for ixfr");
					if (!query->first_repeat_rcvd) {
						query->first_repeat_rcvd =
							ISC_TRUE;
						goto next_rdata;
					}
					debug("done with ixfr");
					goto doexit;
				}
				debug("meaningless soa %d", serial);
			next_rdata:
				result = dns_rdataset_next(rdataset);
			} while (result == ISC_R_SUCCESS);
		}
		result = dns_message_nextname(msg, DNS_SECTION_ANSWER);
	} while (result == ISC_R_SUCCESS);
	launch_next_query(query, ISC_FALSE);
	return (ISC_FALSE);
 doexit:
	received(sevent->n, &sevent->address, query);
	return (ISC_TRUE);
}

/*%
 * Event handler for recv complete.  Perform whatever actions are necessary,
 * based on the specifics of the user's request.
 */
static void
recv_done(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = NULL;
	dig_query_t *query = NULL;
	isc_buffer_t *b = NULL;
	dns_message_t *msg = NULL;
#ifdef DIG_SIGCHASE
	dig_message_t *chase_msg = NULL;
	dig_message_t *chase_msg2 = NULL;
#endif
	isc_result_t result;
	dig_lookup_t *n, *l;
	isc_boolean_t docancel = ISC_FALSE;
	isc_boolean_t match = ISC_TRUE;
	unsigned int parseflags;
	dns_messageid_t id;
	unsigned int msgflags;
#ifdef DIG_SIGCHASE
	isc_result_t do_sigchase = ISC_FALSE;

	dns_message_t *msg_temp = NULL;
	isc_region_t r;
	isc_buffer_t *buf = NULL;
#endif

	UNUSED(task);
	INSIST(!free_now);

	debug("recv_done()");

	LOCK_LOOKUP;
	recvcount--;
	debug("recvcount=%d", recvcount);
	INSIST(recvcount >= 0);

	query = event->ev_arg;
	debug("lookup=%p, query=%p", query->lookup, query);

	l = query->lookup;

	REQUIRE(event->ev_type == ISC_SOCKEVENT_RECVDONE);
	sevent = (isc_socketevent_t *)event;

	b = ISC_LIST_HEAD(sevent->bufferlist);
	INSIST(b == &query->recvbuf);
	ISC_LIST_DEQUEUE(sevent->bufferlist, &query->recvbuf, link);

	if ((l->tcp_mode) && (l->timer != NULL))
		isc_timer_touch(l->timer);
	if ((!l->pending && !l->ns_search_only) || cancel_now) {
		debug("no longer pending.  Got %s",
			isc_result_totext(sevent->result));
		query->waiting_connect = ISC_FALSE;

		isc_event_free(&event);
		clear_query(query);
		check_next_lookup(l);
		UNLOCK_LOOKUP;
		return;
	}

	if (sevent->result != ISC_R_SUCCESS) {
		if (sevent->result == ISC_R_CANCELED) {
			debug("in recv cancel handler");
			query->waiting_connect = ISC_FALSE;
		} else {
			printf(";; communications error: %s\n",
			       isc_result_totext(sevent->result));
			isc_socket_detach(&query->sock);
			sockcount--;
			debug("sockcount=%d", sockcount);
			INSIST(sockcount >= 0);
		}
		isc_event_free(&event);
		clear_query(query);
		check_next_lookup(l);
		UNLOCK_LOOKUP;
		return;
	}

	if (!l->tcp_mode &&
	    !isc_sockaddr_compare(&sevent->address, &query->sockaddr,
				  ISC_SOCKADDR_CMPADDR|
				  ISC_SOCKADDR_CMPPORT|
				  ISC_SOCKADDR_CMPSCOPE|
				  ISC_SOCKADDR_CMPSCOPEZERO)) {
		char buf1[ISC_SOCKADDR_FORMATSIZE];
		char buf2[ISC_SOCKADDR_FORMATSIZE];
		isc_sockaddr_t any;

		if (isc_sockaddr_pf(&query->sockaddr) == AF_INET)
			isc_sockaddr_any(&any);
		else
			isc_sockaddr_any6(&any);

		/*
		* We don't expect a match when the packet is
		* sent to 0.0.0.0, :: or to a multicast addresses.
		* XXXMPA broadcast needs to be handled here as well.
		*/
		if ((!isc_sockaddr_eqaddr(&query->sockaddr, &any) &&
		     !isc_sockaddr_ismulticast(&query->sockaddr)) ||
		    isc_sockaddr_getport(&query->sockaddr) !=
		    isc_sockaddr_getport(&sevent->address)) {
			isc_sockaddr_format(&sevent->address, buf1,
			sizeof(buf1));
			isc_sockaddr_format(&query->sockaddr, buf2,
			sizeof(buf2));
			printf(";; reply from unexpected source: %s,"
			" expected %s\n", buf1, buf2);
			match = ISC_FALSE;
		}
	}

	result = dns_message_peekheader(b, &id, &msgflags);
	if (result != ISC_R_SUCCESS || l->sendmsg->id != id) {
		match = ISC_FALSE;
		if (l->tcp_mode) {
			isc_boolean_t fail = ISC_TRUE;
			if (result == ISC_R_SUCCESS) {
				if (!query->first_soa_rcvd ||
				     query->warn_id)
					printf(";; %s: ID mismatch: "
					       "expected ID %u, got %u\n",
					       query->first_soa_rcvd ?
					       "WARNING" : "ERROR",
					       l->sendmsg->id, id);
				if (query->first_soa_rcvd)
					fail = ISC_FALSE;
				query->warn_id = ISC_FALSE;
			} else
				printf(";; ERROR: short "
				       "(< header size) message\n");
			if (fail) {
				isc_event_free(&event);
				clear_query(query);
				check_next_lookup(l);
				UNLOCK_LOOKUP;
				return;
			}
			match = ISC_TRUE;
		} else if (result == ISC_R_SUCCESS)
			printf(";; Warning: ID mismatch: "
			       "expected ID %u, got %u\n", l->sendmsg->id, id);
		else
			printf(";; Warning: short "
			       "(< header size) message received\n");
	}

	if (result == ISC_R_SUCCESS && (msgflags & DNS_MESSAGEFLAG_QR) == 0)
		printf(";; Warning: query response not set\n");

	if (!match)
		goto udp_mismatch;

	result = dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &msg);
	check_result(result, "dns_message_create");

	if (key != NULL) {
		if (l->querysig == NULL) {
			debug("getting initial querysig");
			result = dns_message_getquerytsig(l->sendmsg, mctx,
							  &l->querysig);
			check_result(result, "dns_message_getquerytsig");
		}
		result = dns_message_setquerytsig(msg, l->querysig);
		check_result(result, "dns_message_setquerytsig");
		result = dns_message_settsigkey(msg, key);
		check_result(result, "dns_message_settsigkey");
		msg->tsigctx = l->tsigctx;
		l->tsigctx = NULL;
		if (l->msgcounter != 0)
			msg->tcp_continuation = 1;
		l->msgcounter++;
	}

	debug("before parse starts");
	parseflags = DNS_MESSAGEPARSE_PRESERVEORDER;
#ifdef DIG_SIGCHASE
	if (!l->sigchase) {
		do_sigchase = ISC_FALSE;
	} else {
		parseflags = 0;
		do_sigchase = ISC_TRUE;
	}
#endif
	if (l->besteffort) {
		parseflags |= DNS_MESSAGEPARSE_BESTEFFORT;
		parseflags |= DNS_MESSAGEPARSE_IGNORETRUNCATION;
	}
	result = dns_message_parse(msg, b, parseflags);
	if (result == DNS_R_RECOVERABLE) {
		printf(";; Warning: Message parser reports malformed "
		       "message packet.\n");
		result = ISC_R_SUCCESS;
	}
	if (result != ISC_R_SUCCESS) {
		printf(";; Got bad packet: %s\n", isc_result_totext(result));
		hex_dump(b);
		query->waiting_connect = ISC_FALSE;
		dns_message_destroy(&msg);
		isc_event_free(&event);
		clear_query(query);
		cancel_lookup(l);
		check_next_lookup(l);
		UNLOCK_LOOKUP;
		return;
	}
	if (msg->counts[DNS_SECTION_QUESTION] != 0) {
		match = ISC_TRUE;
		for (result = dns_message_firstname(msg, DNS_SECTION_QUESTION);
		     result == ISC_R_SUCCESS && match;
		     result = dns_message_nextname(msg, DNS_SECTION_QUESTION)) {
			dns_name_t *name = NULL;
			dns_rdataset_t *rdataset;

			dns_message_currentname(msg, DNS_SECTION_QUESTION,
						&name);
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link)) {
				if (l->rdtype != rdataset->type ||
				    l->rdclass != rdataset->rdclass ||
				    !dns_name_equal(l->name, name)) {
					char namestr[DNS_NAME_FORMATSIZE];
					char typebuf[DNS_RDATATYPE_FORMATSIZE];
					char classbuf[DNS_RDATACLASS_FORMATSIZE];
					dns_name_format(name, namestr,
							sizeof(namestr));
					dns_rdatatype_format(rdataset->type,
							     typebuf,
							     sizeof(typebuf));
					dns_rdataclass_format(rdataset->rdclass,
							      classbuf,
							      sizeof(classbuf));
					printf(";; Question section mismatch: "
					       "got %s/%s/%s\n",
					       namestr, typebuf, classbuf);
					match = ISC_FALSE;
				}
			}
		}
		if (!match) {
			dns_message_destroy(&msg);
			if (l->tcp_mode) {
				isc_event_free(&event);
				clear_query(query);
				check_next_lookup(l);
				UNLOCK_LOOKUP;
				return;
			} else
				goto udp_mismatch;
		}
	}
	if ((msg->flags & DNS_MESSAGEFLAG_TC) != 0 &&
	    !l->ignore && !l->tcp_mode) {
		printf(";; Truncated, retrying in TCP mode.\n");
		n = requeue_lookup(l, ISC_TRUE);
		n->tcp_mode = ISC_TRUE;
		n->origin = query->lookup->origin;
		dns_message_destroy(&msg);
		isc_event_free(&event);
		clear_query(query);
		cancel_lookup(l);
		check_next_lookup(l);
		UNLOCK_LOOKUP;
		return;
	}
	if ((msg->rcode == dns_rcode_servfail && !l->servfail_stops) ||
	    (check_ra && (msg->flags & DNS_MESSAGEFLAG_RA) == 0 && l->recurse))
	{
		dig_query_t *next = ISC_LIST_NEXT(query, link);
		if (l->current_query == query)
			l->current_query = NULL;
		if (next != NULL) {
			debug("sending query %p\n", next);
			if (l->tcp_mode)
				send_tcp_connect(next);
			else
				send_udp(next);
		}
		/*
		 * If our query is at the head of the list and there
		 * is no next, we're the only one left, so fall
		 * through to print the message.
		 */
		if ((ISC_LIST_HEAD(l->q) != query) ||
		    (ISC_LIST_NEXT(query, link) != NULL)) {
			if( l->comments == ISC_TRUE )
				printf(";; Got %s from %s, "
				       "trying next server\n",
				       msg->rcode == dns_rcode_servfail ?
				       "SERVFAIL reply" :
				       "recursion not available",
				       query->servname);
			clear_query(query);
			check_next_lookup(l);
			dns_message_destroy(&msg);
			isc_event_free(&event);
			UNLOCK_LOOKUP;
			return;
		}
	}

	if (key != NULL) {
		result = dns_tsig_verify(&query->recvbuf, msg, NULL, NULL);
		if (result != ISC_R_SUCCESS) {
			printf(";; Couldn't verify signature: %s\n",
			       isc_result_totext(result));
			validated = ISC_FALSE;
		}
		l->tsigctx = msg->tsigctx;
		msg->tsigctx = NULL;
		if (l->querysig != NULL) {
			debug("freeing querysig buffer %p", l->querysig);
			isc_buffer_free(&l->querysig);
		}
		result = dns_message_getquerytsig(msg, mctx, &l->querysig);
		check_result(result,"dns_message_getquerytsig");
	}

	extrabytes = isc_buffer_remaininglength(b);

	debug("after parse");
	if (l->doing_xfr && l->xfr_q == NULL) {
		l->xfr_q = query;
		/*
		 * Once we are in the XFR message, increase
		 * the timeout to much longer, so brief network
		 * outages won't cause the XFR to abort
		 */
		if (timeout != INT_MAX && l->timer != NULL) {
			unsigned int local_timeout;

			if (timeout == 0) {
				if (l->tcp_mode)
					local_timeout = TCP_TIMEOUT * 4;
				else
					local_timeout = UDP_TIMEOUT * 4;
			} else {
				if (timeout < (INT_MAX / 4))
					local_timeout = timeout * 4;
				else
					local_timeout = INT_MAX;
			}
			debug("have local timeout of %d", local_timeout);
			isc_interval_set(&l->interval, local_timeout, 0);
			result = isc_timer_reset(l->timer,
						 isc_timertype_once,
						 NULL,
						 &l->interval,
						 ISC_FALSE);
			check_result(result, "isc_timer_reset");
		}
	}

	if (!l->doing_xfr || l->xfr_q == query) {
		if (msg->rcode != dns_rcode_noerror &&
		    (l->origin != NULL || l->need_search)) {
			if (!next_origin(msg, query) || showsearch) {
				printmessage(query, msg, ISC_TRUE);
				received(b->used, &sevent->address, query);
			}
		} else if (!l->trace && !l->ns_search_only) {
#ifdef DIG_SIGCHASE
			if (!do_sigchase)
#endif
				printmessage(query, msg, ISC_TRUE);
		} else if (l->trace) {
			int n = 0;
			int count = msg->counts[DNS_SECTION_ANSWER];

			debug("in TRACE code");
			if (!l->ns_search_only)
				printmessage(query, msg, ISC_TRUE);

			l->rdtype = l->qrdtype;
			if (l->trace_root || (l->ns_search_only && count > 0)) {
				if (!l->trace_root)
					l->rdtype = dns_rdatatype_soa;
				n = followup_lookup(msg, query,
						    DNS_SECTION_ANSWER);
				l->trace_root = ISC_FALSE;
			} else if (count == 0)
				n = followup_lookup(msg, query,
						    DNS_SECTION_AUTHORITY);
			if (n == 0)
				docancel = ISC_TRUE;
		} else {
			debug("in NSSEARCH code");

			if (l->trace_root) {
				/*
				 * This is the initial NS query.
				 */
				int n;

				l->rdtype = dns_rdatatype_soa;
				n = followup_lookup(msg, query,
						    DNS_SECTION_ANSWER);
				if (n == 0)
					docancel = ISC_TRUE;
				l->trace_root = ISC_FALSE;
			} else
#ifdef DIG_SIGCHASE
				if (!do_sigchase)
#endif
				printmessage(query, msg, ISC_TRUE);
		}
#ifdef DIG_SIGCHASE
		if (do_sigchase) {
			chase_msg = isc_mem_allocate(mctx,
						     sizeof(dig_message_t));
			if (chase_msg == NULL) {
				fatal("Memory allocation failure in %s:%d",
				      __FILE__, __LINE__);
			}
			ISC_LIST_INITANDAPPEND(chase_message_list, chase_msg,
					       link);
			if (dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE,
					       &msg_temp) != ISC_R_SUCCESS) {
				fatal("dns_message_create in %s:%d",
				      __FILE__, __LINE__);
			}

			isc_buffer_usedregion(b, &r);
			result = isc_buffer_allocate(mctx, &buf, r.length);

			check_result(result, "isc_buffer_allocate");
			result =  isc_buffer_copyregion(buf, &r);
			check_result(result, "isc_buffer_copyregion");

			result =  dns_message_parse(msg_temp, buf, 0);

			isc_buffer_free(&buf);
			chase_msg->msg = msg_temp;

			chase_msg2 = isc_mem_allocate(mctx,
						      sizeof(dig_message_t));
			if (chase_msg2 == NULL) {
				fatal("Memory allocation failure in %s:%d",
				      __FILE__, __LINE__);
			}
			ISC_LIST_INITANDAPPEND(chase_message_list2, chase_msg2,
					       link);
			chase_msg2->msg = msg;
		}
#endif
	}

#ifdef DIG_SIGCHASE
	if (l->sigchase && ISC_LIST_EMPTY(lookup_list)) {
		sigchase(msg_temp);
	}
#endif

	if (l->pending)
		debug("still pending.");
	if (l->doing_xfr) {
		if (query != l->xfr_q) {
			dns_message_destroy(&msg);
			isc_event_free(&event);
			query->waiting_connect = ISC_FALSE;
			UNLOCK_LOOKUP;
			return;
		}
		if (!docancel)
			docancel = check_for_more_data(query, msg, sevent);
		if (docancel) {
			dns_message_destroy(&msg);
			clear_query(query);
			cancel_lookup(l);
			check_next_lookup(l);
		}
	} else {

		if (msg->rcode == dns_rcode_noerror || l->origin == NULL) {

#ifdef DIG_SIGCHASE
			if (!l->sigchase)
#endif
				received(b->used, &sevent->address, query);
		}

		if (!query->lookup->ns_search_only)
			query->lookup->pending = ISC_FALSE;
		if (!query->lookup->ns_search_only ||
		    query->lookup->trace_root || docancel) {
#ifdef DIG_SIGCHASE
			if (!do_sigchase)
#endif
				dns_message_destroy(&msg);

			cancel_lookup(l);
		}
		clear_query(query);
		check_next_lookup(l);
	}
	if (msg != NULL) {
#ifdef DIG_SIGCHASE
		if (do_sigchase)
			msg = NULL;
		else
#endif
			dns_message_destroy(&msg);
	}
	isc_event_free(&event);
	UNLOCK_LOOKUP;
	return;

 udp_mismatch:
	isc_buffer_invalidate(&query->recvbuf);
	isc_buffer_init(&query->recvbuf, query->recvspace, COMMSIZE);
	ISC_LIST_ENQUEUE(query->recvlist, &query->recvbuf, link);
	result = isc_socket_recvv(query->sock, &query->recvlist, 1,
				  global_task, recv_done, query);
	check_result(result, "isc_socket_recvv");
	recvcount++;
	isc_event_free(&event);
	UNLOCK_LOOKUP;
	return;
}

/*%
 * Turn a name into an address, using system-supplied routines.  This is
 * used in looking up server names, etc... and needs to use system-supplied
 * routines, since they may be using a non-DNS system for these lookups.
 */
isc_result_t
get_address(char *host, in_port_t port, isc_sockaddr_t *sockaddr) {
	int count;
	isc_result_t result;

	isc_app_block();
	result = bind9_getaddresses(host, port, sockaddr, 1, &count);
	isc_app_unblock();
	if (result != ISC_R_SUCCESS)
		return (result);

	INSIST(count == 1);

	return (ISC_R_SUCCESS);
}

/*%
 * Initiate either a TCP or UDP lookup
 */
void
do_lookup(dig_lookup_t *lookup) {

	REQUIRE(lookup != NULL);

	debug("do_lookup()");
	lookup->pending = ISC_TRUE;
	if (lookup->tcp_mode)
		send_tcp_connect(ISC_LIST_HEAD(lookup->q));
	else
		send_udp(ISC_LIST_HEAD(lookup->q));
}

/*%
 * Start everything in action upon task startup.
 */
void
onrun_callback(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	isc_event_free(&event);
	LOCK_LOOKUP;
	start_lookup();
	UNLOCK_LOOKUP;
}

/*%
 * Make everything on the lookup queue go away.  Mainly used by the
 * SIGINT handler.
 */
void
cancel_all(void) {
	dig_lookup_t *l, *n;
	dig_query_t *q, *nq;

	debug("cancel_all()");

	LOCK_LOOKUP;
	if (free_now) {
		UNLOCK_LOOKUP;
		return;
	}
	cancel_now = ISC_TRUE;
	if (current_lookup != NULL) {
		if (current_lookup->timer != NULL)
			isc_timer_detach(&current_lookup->timer);
		q = ISC_LIST_HEAD(current_lookup->q);
		while (q != NULL) {
			debug("canceling query %p, belonging to %p",
			      q, current_lookup);
			nq = ISC_LIST_NEXT(q, link);
			if (q->sock != NULL) {
				isc_socket_cancel(q->sock, NULL,
						  ISC_SOCKCANCEL_ALL);
			} else {
				clear_query(q);
			}
			q = nq;
		}
	}
	l = ISC_LIST_HEAD(lookup_list);
	while (l != NULL) {
		n = ISC_LIST_NEXT(l, link);
		ISC_LIST_DEQUEUE(lookup_list, l, link);
		try_clear_lookup(l);
		l = n;
	}
	UNLOCK_LOOKUP;
}

/*%
 * Destroy all of the libs we are using, and get everything ready for a
 * clean shutdown.
 */
void
destroy_libs(void) {
#ifdef DIG_SIGCHASE
	void * ptr;
	dig_message_t *chase_msg;
#endif
#ifdef WITH_IDN
	isc_result_t result;
#endif

	debug("destroy_libs()");
	if (global_task != NULL) {
		debug("freeing task");
		isc_task_detach(&global_task);
	}
	/*
	 * The taskmgr_destroy() call blocks until all events are cleared
	 * from the task.
	 */
	if (taskmgr != NULL) {
		debug("freeing taskmgr");
		isc_taskmgr_destroy(&taskmgr);
	}
	LOCK_LOOKUP;
	REQUIRE(sockcount == 0);
	REQUIRE(recvcount == 0);
	REQUIRE(sendcount == 0);

	INSIST(ISC_LIST_HEAD(lookup_list) == NULL);
	INSIST(current_lookup == NULL);
	INSIST(!free_now);

	free_now = ISC_TRUE;

	lwres_conf_clear(lwctx);
	lwres_context_destroy(&lwctx);

	flush_server_list();

	clear_searchlist();

#ifdef WITH_IDN
	result = dns_name_settotextfilter(NULL);
	check_result(result, "dns_name_settotextfilter");
#endif
	dns_name_destroy();

	if (commctx != NULL) {
		debug("freeing commctx");
		isc_mempool_destroy(&commctx);
	}
	if (socketmgr != NULL) {
		debug("freeing socketmgr");
		isc_socketmgr_destroy(&socketmgr);
	}
	if (timermgr != NULL) {
		debug("freeing timermgr");
		isc_timermgr_destroy(&timermgr);
	}
	if (key != NULL) {
		debug("freeing key %p", key);
		dns_tsigkey_detach(&key);
	}
	if (namebuf != NULL)
		isc_buffer_free(&namebuf);

	if (is_dst_up) {
		debug("destroy DST lib");
		dst_lib_destroy();
		is_dst_up = ISC_FALSE;
	}
	if (entp != NULL) {
		debug("detach from entropy");
		isc_entropy_detach(&entp);
	}

	UNLOCK_LOOKUP;
	DESTROYLOCK(&lookup_lock);
#ifdef DIG_SIGCHASE

	debug("Destroy the messages kept for sigchase");
	/* Destroy the messages kept for sigchase */
	chase_msg = ISC_LIST_HEAD(chase_message_list);

	while (chase_msg != NULL) {
		INSIST(chase_msg->msg != NULL);
		dns_message_destroy(&(chase_msg->msg));
		ptr = chase_msg;
		chase_msg = ISC_LIST_NEXT(chase_msg, link);
		isc_mem_free(mctx, ptr);
	}

	chase_msg = ISC_LIST_HEAD(chase_message_list2);

	while (chase_msg != NULL) {
		INSIST(chase_msg->msg != NULL);
		dns_message_destroy(&(chase_msg->msg));
		ptr = chase_msg;
		chase_msg = ISC_LIST_NEXT(chase_msg, link);
		isc_mem_free(mctx, ptr);
	}
	if (dns_name_dynamic(&chase_name))
		free_name(&chase_name, mctx);
#if DIG_SIGCHASE_TD
	if (dns_name_dynamic(&chase_current_name))
		free_name(&chase_current_name, mctx);
	if (dns_name_dynamic(&chase_authority_name))
		free_name(&chase_authority_name, mctx);
#endif
#if DIG_SIGCHASE_BU
	if (dns_name_dynamic(&chase_signame))
		free_name(&chase_signame, mctx);
#endif

	debug("Destroy memory");

#endif
	if (memdebugging != 0)
		isc_mem_stats(mctx, stderr);
	if (mctx != NULL)
		isc_mem_destroy(&mctx);
}

#ifdef WITH_IDN
static void
initialize_idn(void) {
	idn_result_t r;
	isc_result_t result;

#ifdef HAVE_SETLOCALE
	/* Set locale */
	(void)setlocale(LC_ALL, "");
#endif
	/* Create configuration context. */
	r = idn_nameinit(1);
	if (r != idn_success)
		fatal("idn api initialization failed: %s",
		      idn_result_tostring(r));

	/* Set domain name -> text post-conversion filter. */
	result = dns_name_settotextfilter(output_filter);
	check_result(result, "dns_name_settotextfilter");
}

static isc_result_t
output_filter(isc_buffer_t *buffer, unsigned int used_org,
	      isc_boolean_t absolute)
{
	char tmp1[MAXDLEN], tmp2[MAXDLEN];
	size_t fromlen, tolen;
	isc_boolean_t end_with_dot;

	/*
	 * Copy contents of 'buffer' to 'tmp1', supply trailing dot
	 * if 'absolute' is true, and terminate with NUL.
	 */
	fromlen = isc_buffer_usedlength(buffer) - used_org;
	if (fromlen >= MAXDLEN)
		return (ISC_R_SUCCESS);
	memcpy(tmp1, (char *)isc_buffer_base(buffer) + used_org, fromlen);
	end_with_dot = (tmp1[fromlen - 1] == '.') ? ISC_TRUE : ISC_FALSE;
	if (absolute && !end_with_dot) {
		fromlen++;
		if (fromlen >= MAXDLEN)
			return (ISC_R_SUCCESS);
		tmp1[fromlen - 1] = '.';
	}
	tmp1[fromlen] = '\0';

	/*
	 * Convert contents of 'tmp1' to local encoding.
	 */
	if (idn_decodename(IDN_DECODE_APP, tmp1, tmp2, MAXDLEN) != idn_success)
		return (ISC_R_SUCCESS);
	strcpy(tmp1, tmp2);

	/*
	 * Copy the converted contents in 'tmp1' back to 'buffer'.
	 * If we have appended trailing dot, remove it.
	 */
	tolen = strlen(tmp1);
	if (absolute && !end_with_dot && tmp1[tolen - 1] == '.')
		tolen--;

	if (isc_buffer_length(buffer) < used_org + tolen)
		return (ISC_R_NOSPACE);

	isc_buffer_subtract(buffer, isc_buffer_usedlength(buffer) - used_org);
	memcpy(isc_buffer_used(buffer), tmp1, tolen);
	isc_buffer_add(buffer, tolen);

	return (ISC_R_SUCCESS);
}

static idn_result_t
append_textname(char *name, const char *origin, size_t namesize) {
	size_t namelen = strlen(name);
	size_t originlen = strlen(origin);

	/* Already absolute? */
	if (namelen > 0 && name[namelen - 1] == '.')
		return idn_success;

	/* Append dot and origin */

	if (namelen + 1 + originlen >= namesize)
		return idn_buffer_overflow;

	name[namelen++] = '.';
	(void)strcpy(name + namelen, origin);
	return idn_success;
}

static void
idn_check_result(idn_result_t r, const char *msg) {
	if (r != idn_success) {
		exitcode = 1;
		fatal("%s: %s", msg, idn_result_tostring(r));
	}
}
#endif /* WITH_IDN */

#ifdef DIG_SIGCHASE
void
print_type(dns_rdatatype_t type)
{
	isc_buffer_t * b = NULL;
	isc_result_t result;
	isc_region_t r;

	result = isc_buffer_allocate(mctx, &b, 4000);
	check_result(result, "isc_buffer_allocate");

	result = dns_rdatatype_totext(type, b);
	check_result(result, "print_type");

	isc_buffer_usedregion(b, &r);
	r.base[r.length] = '\0';

	printf("%s", r.base);

	isc_buffer_free(&b);
}

void
dump_database_section(dns_message_t *msg, int section)
{
	dns_name_t *msg_name=NULL;

	dns_rdataset_t *rdataset;

	do {
		dns_message_currentname(msg, section, &msg_name);

		for (rdataset = ISC_LIST_HEAD(msg_name->list); rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			dns_name_print(msg_name, stdout);
			printf("\n");
			print_rdataset(msg_name, rdataset, mctx);
			printf("end\n");
		}
		msg_name = NULL;
	} while (dns_message_nextname(msg, section) == ISC_R_SUCCESS);
}

void
dump_database(void) {
	dig_message_t * msg;

	for (msg = ISC_LIST_HEAD(chase_message_list);  msg != NULL;
	     msg = ISC_LIST_NEXT(msg, link)) {
		if (dns_message_firstname(msg->msg, DNS_SECTION_ANSWER)
		    == ISC_R_SUCCESS)
			dump_database_section(msg->msg, DNS_SECTION_ANSWER);

		if (dns_message_firstname(msg->msg, DNS_SECTION_AUTHORITY)
		    == ISC_R_SUCCESS)
			dump_database_section(msg->msg, DNS_SECTION_AUTHORITY);

		if (dns_message_firstname(msg->msg, DNS_SECTION_ADDITIONAL)
		    == ISC_R_SUCCESS)
			dump_database_section(msg->msg, DNS_SECTION_ADDITIONAL);
	}
}


dns_rdataset_t *
search_type(dns_name_t *name, dns_rdatatype_t type, dns_rdatatype_t covers) {
	dns_rdataset_t *rdataset;
	dns_rdata_sig_t siginfo;
	dns_rdata_t sigrdata = DNS_RDATA_INIT;
	isc_result_t result;

	for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link)) {
		if (type == dns_rdatatype_any) {
			if (rdataset->type != dns_rdatatype_rrsig)
				return (rdataset);
		} else if ((type == dns_rdatatype_rrsig) &&
			   (rdataset->type == dns_rdatatype_rrsig)) {
			result = dns_rdataset_first(rdataset);
			check_result(result, "empty rdataset");
			dns_rdataset_current(rdataset, &sigrdata);
			result = dns_rdata_tostruct(&sigrdata, &siginfo, NULL);
			check_result(result, "sigrdata tostruct siginfo");

			if ((siginfo.covered == covers) ||
			    (covers == dns_rdatatype_any)) {
				dns_rdata_reset(&sigrdata);
				dns_rdata_freestruct(&siginfo);
				return (rdataset);
			}
			dns_rdata_reset(&sigrdata);
			dns_rdata_freestruct(&siginfo);
		} else if (rdataset->type == type)
			return (rdataset);
	}
	return (NULL);
}

dns_rdataset_t *
chase_scanname_section(dns_message_t *msg, dns_name_t *name,
		       dns_rdatatype_t type, dns_rdatatype_t covers,
		       int section)
{
	dns_rdataset_t *rdataset;
	dns_name_t *msg_name = NULL;

	do {
		dns_message_currentname(msg, section, &msg_name);
		if (dns_name_compare(msg_name, name) == 0) {
			rdataset = search_type(msg_name, type, covers);
			if (rdataset != NULL)
				return (rdataset);
		}
		msg_name = NULL;
	} while (dns_message_nextname(msg, section) == ISC_R_SUCCESS);

	return (NULL);
}


dns_rdataset_t *
chase_scanname(dns_name_t *name, dns_rdatatype_t type, dns_rdatatype_t covers)
{
	dns_rdataset_t *rdataset = NULL;
	dig_message_t * msg;

	for (msg = ISC_LIST_HEAD(chase_message_list2);  msg != NULL;
	     msg = ISC_LIST_NEXT(msg, link)) {
		if (dns_message_firstname(msg->msg, DNS_SECTION_ANSWER)
		    == ISC_R_SUCCESS)
			rdataset = chase_scanname_section(msg->msg, name,
							  type, covers,
							  DNS_SECTION_ANSWER);
			if (rdataset != NULL)
				return (rdataset);
		if (dns_message_firstname(msg->msg, DNS_SECTION_AUTHORITY)
		    == ISC_R_SUCCESS)
			rdataset =
				chase_scanname_section(msg->msg, name,
						       type, covers,
						       DNS_SECTION_AUTHORITY);
			if (rdataset != NULL)
				return (rdataset);
		if (dns_message_firstname(msg->msg, DNS_SECTION_ADDITIONAL)
		    == ISC_R_SUCCESS)
			rdataset =
				chase_scanname_section(msg->msg, name, type,
						       covers,
						       DNS_SECTION_ADDITIONAL);
			if (rdataset != NULL)
				return (rdataset);
	}

	return (NULL);
}

dns_rdataset_t *
sigchase_scanname(dns_rdatatype_t type, dns_rdatatype_t covers,
		  isc_boolean_t * lookedup, dns_name_t *rdata_name)
{
	dig_lookup_t *lookup;
	isc_buffer_t *b = NULL;
	isc_region_t r;
	isc_result_t result;
	dns_rdataset_t * temp;
	dns_rdatatype_t querytype;

	temp = chase_scanname(rdata_name, type, covers);
	if (temp != NULL)
		return (temp);

	if (*lookedup == ISC_TRUE)
		return (NULL);

	lookup = clone_lookup(current_lookup, ISC_TRUE);
	lookup->trace_root = ISC_FALSE;
	lookup->new_search = ISC_TRUE;

	result = isc_buffer_allocate(mctx, &b, BUFSIZE);
	check_result(result, "isc_buffer_allocate");
	result = dns_name_totext(rdata_name, ISC_FALSE, b);
	check_result(result, "dns_name_totext");
	isc_buffer_usedregion(b, &r);
	r.base[r.length] = '\0';
	strcpy(lookup->textname, (char*)r.base);
	isc_buffer_free(&b);

	if (type ==  dns_rdatatype_rrsig)
		querytype = covers;
	else
		querytype = type;

	if (querytype == 0 || querytype == 255) {
		printf("Error in the queried type: %d\n", querytype);
		return (NULL);
	}

	lookup->rdtype = querytype;
	lookup->rdtypeset = ISC_TRUE;
	lookup->qrdtype = querytype;
	*lookedup = ISC_TRUE;

	ISC_LIST_APPEND(lookup_list, lookup, link);
	printf("\n\nLaunch a query to find a RRset of type ");
	print_type(type);
	printf(" for zone: %s\n", lookup->textname);
	return (NULL);
}

void
insert_trustedkey(dst_key_t * key)
{
	if (key == NULL)
		return;
	if (tk_list.nb_tk >= MAX_TRUSTED_KEY)
		return;

	tk_list.key[tk_list.nb_tk++] = key;
	return;
}

void
clean_trustedkey()
{
	int i = 0;

	for (i= 0; i < MAX_TRUSTED_KEY; i++) {
		if (tk_list.key[i] != NULL) {
			dst_key_free(&tk_list.key[i]);
			tk_list.key[i] = NULL;
		} else
			break;
	}
	tk_list.nb_tk = 0;
	return;
}

char alphnum[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

isc_result_t
removetmpkey(isc_mem_t *mctx, const char *file)
{
	char *tempnamekey = NULL;
	int tempnamekeylen;
	isc_result_t result;

	tempnamekeylen = strlen(file)+10;

	tempnamekey = isc_mem_allocate(mctx, tempnamekeylen);
	if (tempnamekey == NULL)
		return (ISC_R_NOMEMORY);

	memset(tempnamekey, 0, tempnamekeylen);

	strcat(tempnamekey, file);
	strcat(tempnamekey,".key");
	isc_file_remove(tempnamekey);

	result = isc_file_remove(tempnamekey);
	isc_mem_free(mctx, tempnamekey);
	return (result);
}

isc_result_t
opentmpkey(isc_mem_t *mctx, const char *file, char **tempp, FILE **fp) {
	FILE *f = NULL;
	isc_result_t result;
	char *tempname = NULL;
	char *tempnamekey = NULL;
	int tempnamelen;
	int tempnamekeylen;
	char *x;
	char *cp;
	isc_uint32_t which;

	while (1) {
		tempnamelen = strlen(file) + 20;
		tempname = isc_mem_allocate(mctx, tempnamelen);
		if (tempname == NULL)
			return (ISC_R_NOMEMORY);
		memset(tempname, 0, tempnamelen);

		result = isc_file_mktemplate(file, tempname, tempnamelen);
		if (result != ISC_R_SUCCESS)
			goto cleanup;

		cp = tempname;
		while (*cp != '\0')
			cp++;
		if (cp == tempname) {
			isc_mem_free(mctx, tempname);
			return (ISC_R_FAILURE);
		}

		x = cp--;
		while (cp >= tempname && *cp == 'X') {
			isc_random_get(&which);
			*cp = alphnum[which % (sizeof(alphnum) - 1)];
			x = cp--;
		}

		tempnamekeylen = tempnamelen+5;
		tempnamekey = isc_mem_allocate(mctx, tempnamekeylen);
		if (tempnamekey == NULL)
			return (ISC_R_NOMEMORY);

		memset(tempnamekey, 0, tempnamekeylen);
		strncpy(tempnamekey, tempname, tempnamelen);
		strcat(tempnamekey ,".key");


		if (isc_file_exists(tempnamekey)) {
			isc_mem_free(mctx, tempnamekey);
			isc_mem_free(mctx, tempname);
			continue;
		}

		if ((f = fopen(tempnamekey, "w")) == NULL) {
			printf("get_trusted_key(): trusted key not found %s\n",
			       tempnamekey);
			return (ISC_R_FAILURE);
		}
		break;
	}
	isc_mem_free(mctx, tempnamekey);
	*tempp = tempname;
	*fp = f;
	return (ISC_R_SUCCESS);

 cleanup:
	isc_mem_free(mctx, tempname);

	return (result);
}


isc_result_t
get_trusted_key(isc_mem_t *mctx)
{
	isc_result_t result;
	const char *filename = NULL;
	char *filetemp = NULL;
	char buf[1500];
	FILE *fp, *fptemp;
	dst_key_t *key = NULL;

	result = isc_file_exists(trustedkey);
	if (result !=  ISC_TRUE) {
		result = isc_file_exists("/etc/trusted-key.key");
		if (result !=  ISC_TRUE) {
			result = isc_file_exists("./trusted-key.key");
			if (result !=  ISC_TRUE)
				return (ISC_R_FAILURE);
			else
				filename = "./trusted-key.key";
		} else
			filename = "/etc/trusted-key.key";
	} else
		filename = trustedkey;

	if (filename == NULL) {
		printf("No trusted key\n");
		return (ISC_R_FAILURE);
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		printf("get_trusted_key(): trusted key not found %s\n",
		       filename);
		return (ISC_R_FAILURE);
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		result = opentmpkey(mctx,"tmp_file", &filetemp, &fptemp);
		if (result != ISC_R_SUCCESS) {
			fclose(fp);
			return (ISC_R_FAILURE);
		}
		if (fputs(buf, fptemp) < 0) {
			fclose(fp);
			fclose(fptemp);
			return (ISC_R_FAILURE);
		}
		fclose(fptemp);
		result = dst_key_fromnamedfile(filetemp, DST_TYPE_PUBLIC,
					       mctx, &key);
		removetmpkey(mctx, filetemp);
		isc_mem_free(mctx, filetemp);
		if (result !=  ISC_R_SUCCESS) {
			fclose(fp);
			return (ISC_R_FAILURE);
		}
		insert_trustedkey(key);
#if 0
		dst_key_tofile(key, DST_TYPE_PUBLIC,"/tmp");
#endif
		key = NULL;
	}
	return (ISC_R_SUCCESS);
}


static void
nameFromString(const char *str, dns_name_t *p_ret) {
	size_t len = strlen(str);
	isc_result_t result;
	isc_buffer_t buffer;
	dns_fixedname_t fixedname;

	REQUIRE(p_ret != NULL);
	REQUIRE(str != NULL);

	isc_buffer_init(&buffer, str, len);
	isc_buffer_add(&buffer, len);

	dns_fixedname_init(&fixedname);
	result = dns_name_fromtext(dns_fixedname_name(&fixedname), &buffer,
				   dns_rootname, ISC_TRUE, NULL);
	check_result(result, "nameFromString");

	if (dns_name_dynamic(p_ret))
		free_name(p_ret, mctx);

	result = dns_name_dup(dns_fixedname_name(&fixedname), mctx, p_ret);
	check_result(result, "nameFromString");
}


#if DIG_SIGCHASE_TD
isc_result_t
prepare_lookup(dns_name_t *name)
{
	isc_result_t result;
	dig_lookup_t *lookup = NULL;
	dig_server_t *s;
	void *ptr;

	lookup = clone_lookup(current_lookup, ISC_TRUE);
	lookup->trace_root = ISC_FALSE;
	lookup->new_search = ISC_TRUE;
	lookup->trace_root_sigchase = ISC_FALSE;

	strncpy(lookup->textname, lookup->textnamesigchase, MXNAME);

	lookup->rdtype = lookup->rdtype_sigchase;
	lookup->rdtypeset = ISC_TRUE;
	lookup->qrdtype = lookup->qrdtype_sigchase;

	s = ISC_LIST_HEAD(lookup->my_server_list);
	while (s != NULL) {
		debug("freeing server %p belonging to %p",
		      s, lookup);
		ptr = s;
		s = ISC_LIST_NEXT(s, link);
		ISC_LIST_DEQUEUE(lookup->my_server_list,
				 (dig_server_t *)ptr, link);
		isc_mem_free(mctx, ptr);
	}


	for (result = dns_rdataset_first(chase_nsrdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(chase_nsrdataset)) {
		char namestr[DNS_NAME_FORMATSIZE];
		dns_rdata_ns_t ns;
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dig_server_t * srv = NULL;
#define __FOLLOW_GLUE__
#ifdef __FOLLOW_GLUE__
		isc_buffer_t *b = NULL;
		isc_result_t result;
		isc_region_t r;
		dns_rdataset_t *rdataset = NULL;
		isc_boolean_t true = ISC_TRUE;
#endif

		memset(namestr, 0, DNS_NAME_FORMATSIZE);

		dns_rdataset_current(chase_nsrdataset, &rdata);

		result = dns_rdata_tostruct(&rdata, &ns, NULL);
		check_result(result, "dns_rdata_tostruct");

#ifdef __FOLLOW_GLUE__

		result = advanced_rrsearch(&rdataset, &ns.name,
					   dns_rdatatype_aaaa,
					   dns_rdatatype_any, &true);
		if (result == ISC_R_SUCCESS) {
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset)) {
				dns_rdata_t aaaa = DNS_RDATA_INIT;
				dns_rdataset_current(rdataset, &aaaa);

				result = isc_buffer_allocate(mctx, &b, 80);
				check_result(result, "isc_buffer_allocate");

				dns_rdata_totext(&aaaa, &ns.name, b);
				isc_buffer_usedregion(b, &r);
				r.base[r.length] = '\0';
				strncpy(namestr, (char*)r.base,
					DNS_NAME_FORMATSIZE);
				isc_buffer_free(&b);
				dns_rdata_reset(&aaaa);


				srv = make_server(namestr, namestr);

				ISC_LIST_APPEND(lookup->my_server_list,
						srv, link);
			}
		}

		rdataset = NULL;
		result = advanced_rrsearch(&rdataset, &ns.name, dns_rdatatype_a,
					   dns_rdatatype_any, &true);
		if (result == ISC_R_SUCCESS) {
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset)) {
				dns_rdata_t a = DNS_RDATA_INIT;
				dns_rdataset_current(rdataset, &a);

				result = isc_buffer_allocate(mctx, &b, 80);
				check_result(result, "isc_buffer_allocate");

				dns_rdata_totext(&a, &ns.name, b);
				isc_buffer_usedregion(b, &r);
				r.base[r.length] = '\0';
				strncpy(namestr, (char*)r.base,
					DNS_NAME_FORMATSIZE);
				isc_buffer_free(&b);
				dns_rdata_reset(&a);
				printf("ns name: %s\n", namestr);


				srv = make_server(namestr, namestr);

				ISC_LIST_APPEND(lookup->my_server_list,
						srv, link);
			}
		}
#else

		dns_name_format(&ns.name, namestr, sizeof(namestr));
		printf("ns name: ");
		dns_name_print(&ns.name, stdout);
		printf("\n");
		srv = make_server(namestr, namestr);

		ISC_LIST_APPEND(lookup->my_server_list, srv, link);

#endif
		dns_rdata_freestruct(&ns);
		dns_rdata_reset(&rdata);

	}

	ISC_LIST_APPEND(lookup_list, lookup, link);
	printf("\nLaunch a query to find a RRset of type ");
	print_type(lookup->rdtype);
	printf(" for zone: %s", lookup->textname);
	printf(" with nameservers:");
	printf("\n");
	print_rdataset(name, chase_nsrdataset, mctx);
	return (ISC_R_SUCCESS);
}


isc_result_t
child_of_zone(dns_name_t * name, dns_name_t * zone_name,
	      dns_name_t * child_name)
{
	dns_namereln_t name_reln;
	int orderp;
	unsigned int nlabelsp;

	name_reln = dns_name_fullcompare(name, zone_name, &orderp, &nlabelsp);
	if (name_reln != dns_namereln_subdomain ||
	    dns_name_countlabels(name) <= dns_name_countlabels(zone_name) + 1) {
		printf("\n;; ERROR : ");
		dns_name_print(name, stdout);
		printf(" is not a subdomain of: ");
		dns_name_print(zone_name, stdout);
		printf(" FAILED\n\n");
		return (ISC_R_FAILURE);
	}

	dns_name_getlabelsequence(name,
				  dns_name_countlabels(name) -
				  dns_name_countlabels(zone_name) -1,
				  dns_name_countlabels(zone_name) +1,
				  child_name);
	return (ISC_R_SUCCESS);
}

isc_result_t
grandfather_pb_test(dns_name_t *zone_name, dns_rdataset_t  *sigrdataset)
{
	isc_result_t result;
	dns_rdata_t sigrdata = DNS_RDATA_INIT;
	dns_rdata_sig_t siginfo;

	result = dns_rdataset_first(sigrdataset);
	check_result(result, "empty RRSIG dataset");
	dns_rdata_init(&sigrdata);

	do {
		dns_rdataset_current(sigrdataset, &sigrdata);

		result = dns_rdata_tostruct(&sigrdata, &siginfo, NULL);
		check_result(result, "sigrdata tostruct siginfo");

		if (dns_name_compare(&siginfo.signer, zone_name) == 0) {
			dns_rdata_freestruct(&siginfo);
			dns_rdata_reset(&sigrdata);
			return (ISC_R_SUCCESS);
		}

		dns_rdata_freestruct(&siginfo);
		dns_rdata_reset(&sigrdata);

	} while (dns_rdataset_next(chase_sigkeyrdataset) == ISC_R_SUCCESS);

	dns_rdata_reset(&sigrdata);

	return (ISC_R_FAILURE);
}


isc_result_t
initialization(dns_name_t *name)
{
	isc_result_t   result;
	isc_boolean_t  true = ISC_TRUE;

	chase_nsrdataset = NULL;
	result = advanced_rrsearch(&chase_nsrdataset, name, dns_rdatatype_ns,
				   dns_rdatatype_any, &true);
	if (result != ISC_R_SUCCESS) {
		printf("\n;; NS RRset is missing to continue validation:"
		       " FAILED\n\n");
		return (ISC_R_FAILURE);
	}
	INSIST(chase_nsrdataset != NULL);
	prepare_lookup(name);

	dup_name(name, &chase_current_name, mctx);

	return (ISC_R_SUCCESS);
}
#endif

void
print_rdataset(dns_name_t *name, dns_rdataset_t *rdataset, isc_mem_t *mctx)
{
	isc_buffer_t *b = NULL;
	isc_result_t result;
	isc_region_t r;

	result = isc_buffer_allocate(mctx, &b, 9000);
	check_result(result, "isc_buffer_allocate");

	printrdataset(name, rdataset, b);

	isc_buffer_usedregion(b, &r);
	r.base[r.length] = '\0';


	printf("%s\n", r.base);

	isc_buffer_free(&b);
}


void
dup_name(dns_name_t *source, dns_name_t *target, isc_mem_t *mctx) {
	isc_result_t result;

	if (dns_name_dynamic(target))
		free_name(target, mctx);
	result = dns_name_dup(source, mctx, target);
	check_result(result, "dns_name_dup");
}

void
free_name(dns_name_t *name, isc_mem_t *mctx) {
	dns_name_free(name, mctx);
	dns_name_init(name, NULL);
}

/*
 *
 * take a DNSKEY RRset and the RRSIG RRset corresponding in parameter
 * return ISC_R_SUCCESS if the DNSKEY RRset contains a trusted_key
 * 			and the RRset is valid
 * return ISC_R_NOTFOUND if not contains trusted key
			or if the RRset isn't valid
 * return ISC_R_FAILURE if problem
 *
 */
isc_result_t
contains_trusted_key(dns_name_t *name, dns_rdataset_t *rdataset,
		     dns_rdataset_t *sigrdataset,
		     isc_mem_t *mctx)
{
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dst_key_t *trustedKey = NULL;
	dst_key_t *dnsseckey = NULL;
	int i;

	if (name == NULL || rdataset == NULL)
		return (ISC_R_FAILURE);

	result = dns_rdataset_first(rdataset);
	check_result(result, "empty rdataset");

	do {
		dns_rdataset_current(rdataset, &rdata);
		INSIST(rdata.type == dns_rdatatype_dnskey);

		result = dns_dnssec_keyfromrdata(name, &rdata,
						 mctx, &dnsseckey);
		check_result(result, "dns_dnssec_keyfromrdata");


		for (i = 0; i < tk_list.nb_tk; i++) {
			if (dst_key_compare(tk_list.key[i], dnsseckey)
			    == ISC_TRUE) {
				dns_rdata_reset(&rdata);

				printf(";; Ok, find a Trusted Key in the "
				       "DNSKEY RRset: %d\n",
				       dst_key_id(dnsseckey));
				if (sigchase_verify_sig_key(name, rdataset,
							    dnsseckey,
							    sigrdataset,
							    mctx)
				    == ISC_R_SUCCESS) {
					dst_key_free(&dnsseckey);
					dnsseckey = NULL;
					return (ISC_R_SUCCESS);
				}
			}
		}

		dns_rdata_reset(&rdata);
		if (dnsseckey != NULL)
			dst_key_free(&dnsseckey);
	} while (dns_rdataset_next(rdataset) == ISC_R_SUCCESS);

	if (trustedKey != NULL)
		dst_key_free(&trustedKey);
	trustedKey = NULL;

	return (ISC_R_NOTFOUND);
}

isc_result_t
sigchase_verify_sig(dns_name_t *name, dns_rdataset_t *rdataset,
		    dns_rdataset_t *keyrdataset,
		    dns_rdataset_t *sigrdataset,
		    isc_mem_t *mctx)
{
	isc_result_t result;
	dns_rdata_t keyrdata = DNS_RDATA_INIT;
	dst_key_t *dnsseckey = NULL;

	result = dns_rdataset_first(keyrdataset);
	check_result(result, "empty DNSKEY dataset");
	dns_rdata_init(&keyrdata);

	do {
		dns_rdataset_current(keyrdataset, &keyrdata);
		INSIST(keyrdata.type == dns_rdatatype_dnskey);

		result = dns_dnssec_keyfromrdata(name, &keyrdata,
						 mctx, &dnsseckey);
		check_result(result, "dns_dnssec_keyfromrdata");

		result = sigchase_verify_sig_key(name, rdataset, dnsseckey,
						 sigrdataset, mctx);
		if (result == ISC_R_SUCCESS) {
			dns_rdata_reset(&keyrdata);
			dst_key_free(&dnsseckey);
			return (ISC_R_SUCCESS);
		}
		dst_key_free(&dnsseckey);
		dns_rdata_reset(&keyrdata);
	} while (dns_rdataset_next(chase_keyrdataset) == ISC_R_SUCCESS);

	dns_rdata_reset(&keyrdata);

	return (ISC_R_NOTFOUND);
}

isc_result_t
sigchase_verify_sig_key(dns_name_t *name, dns_rdataset_t *rdataset,
			dst_key_t *dnsseckey, dns_rdataset_t *sigrdataset,
			isc_mem_t *mctx)
{
	isc_result_t result;
	dns_rdata_t sigrdata = DNS_RDATA_INIT;
	dns_rdata_sig_t siginfo;

	result = dns_rdataset_first(sigrdataset);
	check_result(result, "empty RRSIG dataset");
	dns_rdata_init(&sigrdata);

	do {
		dns_rdataset_current(sigrdataset, &sigrdata);

		result = dns_rdata_tostruct(&sigrdata, &siginfo, NULL);
		check_result(result, "sigrdata tostruct siginfo");

		/*
		 * Test if the id of the DNSKEY is
		 * the id of the DNSKEY signer's
		 */
		if (siginfo.keyid == dst_key_id(dnsseckey)) {

			result = dns_rdataset_first(rdataset);
			check_result(result, "empty DS dataset");

			result = dns_dnssec_verify(name, rdataset, dnsseckey,
						   ISC_FALSE, mctx, &sigrdata);

			printf(";; VERIFYING ");
			print_type(rdataset->type);
			printf(" RRset for ");
			dns_name_print(name, stdout);
			printf(" with DNSKEY:%d: %s\n", dst_key_id(dnsseckey),
			       isc_result_totext(result));

			if (result == ISC_R_SUCCESS) {
				dns_rdata_reset(&sigrdata);
				return (result);
			}
		}
		dns_rdata_freestruct(&siginfo);
		dns_rdata_reset(&sigrdata);

	} while (dns_rdataset_next(chase_sigkeyrdataset) == ISC_R_SUCCESS);

	dns_rdata_reset(&sigrdata);

	return (ISC_R_NOTFOUND);
}


isc_result_t
sigchase_verify_ds(dns_name_t *name, dns_rdataset_t *keyrdataset,
		   dns_rdataset_t *dsrdataset, isc_mem_t *mctx)
{
	isc_result_t result;
	dns_rdata_t keyrdata = DNS_RDATA_INIT;
	dns_rdata_t newdsrdata = DNS_RDATA_INIT;
	dns_rdata_t dsrdata = DNS_RDATA_INIT;
	dns_rdata_ds_t dsinfo;
	dst_key_t *dnsseckey = NULL;
	unsigned char dsbuf[DNS_DS_BUFFERSIZE];

	result = dns_rdataset_first(dsrdataset);
	check_result(result, "empty DSset dataset");
	do {
		dns_rdataset_current(dsrdataset, &dsrdata);

		result = dns_rdata_tostruct(&dsrdata, &dsinfo, NULL);
		check_result(result, "dns_rdata_tostruct for DS");

		result = dns_rdataset_first(keyrdataset);
		check_result(result, "empty KEY dataset");

		do {
			dns_rdataset_current(keyrdataset, &keyrdata);
			INSIST(keyrdata.type == dns_rdatatype_dnskey);

			result = dns_dnssec_keyfromrdata(name, &keyrdata,
							 mctx, &dnsseckey);
			check_result(result, "dns_dnssec_keyfromrdata");

			/*
			 * Test if the id of the DNSKEY is the
			 * id of DNSKEY referenced by the DS
			 */
			if (dsinfo.key_tag == dst_key_id(dnsseckey)) {

				result = dns_ds_buildrdata(name, &keyrdata,
							   dsinfo.digest_type,
							   dsbuf, &newdsrdata);
				dns_rdata_freestruct(&dsinfo);

				if (result != ISC_R_SUCCESS) {
					dns_rdata_reset(&keyrdata);
					dns_rdata_reset(&newdsrdata);
					dns_rdata_reset(&dsrdata);
					dst_key_free(&dnsseckey);
					dns_rdata_freestruct(&dsinfo);
					printf("Oops: impossible to build"
					       " new DS rdata\n");
					return (result);
				}


				if (dns_rdata_compare(&dsrdata,
						      &newdsrdata) == 0) {
					printf(";; OK a DS valids a DNSKEY"
					       " in the RRset\n");
					printf(";; Now verify that this"
					       " DNSKEY validates the "
					       "DNSKEY RRset\n");

					result = sigchase_verify_sig_key(name,
							 keyrdataset,
							 dnsseckey,
							 chase_sigkeyrdataset,
							 mctx);
					if (result ==  ISC_R_SUCCESS) {
						dns_rdata_reset(&keyrdata);
						dns_rdata_reset(&newdsrdata);
						dns_rdata_reset(&dsrdata);
						dst_key_free(&dnsseckey);

						return (result);
					}
				} else {
					printf(";; This DS is NOT the DS for"
					       " the chasing KEY: FAILED\n");
				}

				dns_rdata_reset(&newdsrdata);
			}
			dst_key_free(&dnsseckey);
			dns_rdata_reset(&keyrdata);
			dnsseckey = NULL;
		} while (dns_rdataset_next(chase_keyrdataset) == ISC_R_SUCCESS);
		dns_rdata_reset(&dsrdata);

	} while (dns_rdataset_next(chase_dsrdataset) == ISC_R_SUCCESS);

	dns_rdata_reset(&keyrdata);
	dns_rdata_reset(&newdsrdata);
	dns_rdata_reset(&dsrdata);

	return (ISC_R_NOTFOUND);
}

/*
 *
 * take a pointer on a rdataset in parameter and try to resolv it.
 * the searched rrset is a rrset on 'name' with type 'type'
 * (and if the type is a rrsig the signature cover 'covers').
 * the lookedup is to known if you have already done the query on the net.
 * ISC_R_SUCCESS: if we found the rrset
 * ISC_R_NOTFOUND: we do not found the rrset in cache
 * and we do a query on the net
 * ISC_R_FAILURE: rrset not found
 */
isc_result_t
advanced_rrsearch(dns_rdataset_t **rdataset, dns_name_t *name,
		  dns_rdatatype_t type, dns_rdatatype_t covers,
		  isc_boolean_t *lookedup)
{
	isc_boolean_t  tmplookedup;

	INSIST(rdataset != NULL);

	if (*rdataset != NULL)
		return (ISC_R_SUCCESS);

	tmplookedup = *lookedup;
	if ((*rdataset = sigchase_scanname(type, covers,
					   lookedup, name)) == NULL) {
		if (tmplookedup)
			return (ISC_R_FAILURE);
		return (ISC_R_NOTFOUND);
	}
	*lookedup = ISC_FALSE;
	return (ISC_R_SUCCESS);
}



#if DIG_SIGCHASE_TD
void
sigchase_td(dns_message_t *msg)
{
	isc_result_t result;
	dns_name_t *name = NULL;
	isc_boolean_t have_answer = ISC_FALSE;
	isc_boolean_t true = ISC_TRUE;

	if ((result = dns_message_firstname(msg, DNS_SECTION_ANSWER))
	    == ISC_R_SUCCESS) {
		dns_message_currentname(msg, DNS_SECTION_ANSWER, &name);
		if (current_lookup->trace_root_sigchase) {
			initialization(name);
			return;
		}
		have_answer = true;
	} else {
		if (!current_lookup->trace_root_sigchase) {
			result = dns_message_firstname(msg,
						       DNS_SECTION_AUTHORITY);
			if (result == ISC_R_SUCCESS)
				dns_message_currentname(msg,
							DNS_SECTION_AUTHORITY,
							&name);
			chase_nsrdataset
				= chase_scanname_section(msg, name,
							 dns_rdatatype_ns,
							 dns_rdatatype_any,
							 DNS_SECTION_AUTHORITY);
			dup_name(name, &chase_authority_name, mctx);
			if (chase_nsrdataset != NULL) {
				have_delegation_ns = ISC_TRUE;
				printf("no response but there is a delegation"
				       " in authority section:");
				dns_name_print(name, stdout);
				printf("\n");
			} else {
				printf("no response and no delegation in "
				       "authority section but a reference"
				       " to: ");
				dns_name_print(name, stdout);
				printf("\n");
				error_message = msg;
			}
		} else {
			printf(";; NO ANSWERS: %s\n",
			       isc_result_totext(result));
			free_name(&chase_name, mctx);
			clean_trustedkey();
			return;
		}
	}


	if (have_answer) {
		chase_rdataset
			= chase_scanname_section(msg, &chase_name,
						 current_lookup
						 ->rdtype_sigchase,
						 dns_rdatatype_any,
						 DNS_SECTION_ANSWER);
		if (chase_rdataset != NULL)
			have_response = ISC_TRUE;
	}

	result = advanced_rrsearch(&chase_keyrdataset,
				   &chase_current_name,
				   dns_rdatatype_dnskey,
				   dns_rdatatype_any,
				   &chase_keylookedup);
	if (result == ISC_R_FAILURE) {
		printf("\n;; DNSKEY is missing to continue validation:"
		       " FAILED\n\n");
		goto cleanandgo;
	}
	if (result == ISC_R_NOTFOUND)
		return;
	INSIST(chase_keyrdataset != NULL);
	printf("\n;; DNSKEYset:\n");
	print_rdataset(&chase_current_name , chase_keyrdataset, mctx);


	result = advanced_rrsearch(&chase_sigkeyrdataset,
				   &chase_current_name,
				   dns_rdatatype_rrsig,
				   dns_rdatatype_dnskey,
				   &chase_sigkeylookedup);
	if (result == ISC_R_FAILURE) {
		printf("\n;; RRSIG of DNSKEY is missing to continue validation:"
		       " FAILED\n\n");
		goto cleanandgo;
	}
	if (result == ISC_R_NOTFOUND)
		return;
	INSIST(chase_sigkeyrdataset != NULL);
	printf("\n;; RRSIG of the DNSKEYset:\n");
	print_rdataset(&chase_current_name , chase_sigkeyrdataset, mctx);


	if (!chase_dslookedup && !chase_nslookedup) {
		if (!delegation_follow) {
			result = contains_trusted_key(&chase_current_name,
						      chase_keyrdataset,
						      chase_sigkeyrdataset,
						      mctx);
		} else {
			INSIST(chase_dsrdataset != NULL);
			INSIST(chase_sigdsrdataset != NULL);
			result = sigchase_verify_ds(&chase_current_name,
						    chase_keyrdataset,
						    chase_dsrdataset,
						    mctx);
		}

		if (result != ISC_R_SUCCESS) {
			printf("\n;; chain of trust can't be validated:"
			       " FAILED\n\n");
			goto cleanandgo;
		} else {
			chase_dsrdataset = NULL;
			chase_sigdsrdataset = NULL;
		}
	}

	if (have_response || (!have_delegation_ns && !have_response)) {
		/* test if it's a grand father case */

		if (have_response) {
			result = advanced_rrsearch(&chase_sigrdataset,
						   &chase_name,
						   dns_rdatatype_rrsig,
						   current_lookup
						   ->rdtype_sigchase,
						   &true);
			if (result == ISC_R_FAILURE) {
				printf("\n;; RRset is missing to continue"
				       " validation SHOULD NOT APPEND:"
				       " FAILED\n\n");
				goto cleanandgo;
			}

		} else {
			result = advanced_rrsearch(&chase_sigrdataset,
						   &chase_authority_name,
						   dns_rdatatype_rrsig,
						   dns_rdatatype_any,
						   &true);
			if (result == ISC_R_FAILURE) {
				printf("\n;; RRSIG is missing  to continue"
				       " validation SHOULD NOT APPEND:"
				       " FAILED\n\n");
				goto cleanandgo;
			}
		}
		result =  grandfather_pb_test(&chase_current_name,
					      chase_sigrdataset);
		if (result != ISC_R_SUCCESS) {
			dns_name_t tmp_name;

			printf("\n;; We are in a Grand Father Problem:"
			       " See 2.2.1 in RFC 3568\n");
			chase_rdataset = NULL;
			chase_sigrdataset = NULL;
			have_response = ISC_FALSE;
			have_delegation_ns = ISC_FALSE;

			dns_name_init(&tmp_name, NULL);
			result = child_of_zone(&chase_name, &chase_current_name,
					       &tmp_name);
			if (dns_name_dynamic(&chase_authority_name))
				free_name(&chase_authority_name, mctx);
			dup_name(&tmp_name, &chase_authority_name, mctx);
			printf(";; and we try to continue chain of trust"
			       " validation of the zone: ");
			dns_name_print(&chase_authority_name, stdout);
			printf("\n");
			have_delegation_ns = ISC_TRUE;
		} else {
			if (have_response)
				goto finalstep;
			else
				chase_sigrdataset = NULL;
		}
	}

	if (have_delegation_ns) {
		chase_nsrdataset = NULL;
		result = advanced_rrsearch(&chase_nsrdataset,
					   &chase_authority_name,
					   dns_rdatatype_ns,
					   dns_rdatatype_any,
					   &chase_nslookedup);
		if (result == ISC_R_FAILURE) {
			printf("\n;;NSset is missing to continue validation:"
			       " FAILED\n\n");
			goto cleanandgo;
		}
		if (result == ISC_R_NOTFOUND) {
			return;
		}
		INSIST(chase_nsrdataset != NULL);

		result = advanced_rrsearch(&chase_dsrdataset,
					   &chase_authority_name,
					   dns_rdatatype_ds,
					   dns_rdatatype_any,
					   &chase_dslookedup);
		if (result == ISC_R_FAILURE) {
			printf("\n;; DSset is missing to continue validation:"
			       " FAILED\n\n");
			goto cleanandgo;
		}
		if (result == ISC_R_NOTFOUND)
			return;
		INSIST(chase_dsrdataset != NULL);
		printf("\n;; DSset:\n");
		print_rdataset(&chase_authority_name , chase_dsrdataset, mctx);

		result = advanced_rrsearch(&chase_sigdsrdataset,
					   &chase_authority_name,
					   dns_rdatatype_rrsig,
					   dns_rdatatype_ds,
					   &true);
		if (result != ISC_R_SUCCESS) {
			printf("\n;; DSset is missing to continue validation:"
			       " FAILED\n\n");
			goto cleanandgo;
		}
		printf("\n;; RRSIGset of DSset\n");
		print_rdataset(&chase_authority_name,
			       chase_sigdsrdataset, mctx);
		INSIST(chase_sigdsrdataset != NULL);

		result = sigchase_verify_sig(&chase_authority_name,
					     chase_dsrdataset,
					     chase_keyrdataset,
					     chase_sigdsrdataset, mctx);
		if (result != ISC_R_SUCCESS) {
			printf("\n;; Impossible to verify the DSset:"
			       " FAILED\n\n");
			goto cleanandgo;
		}
		chase_keyrdataset = NULL;
		chase_sigkeyrdataset = NULL;


		prepare_lookup(&chase_authority_name);

		have_response = ISC_FALSE;
		have_delegation_ns = ISC_FALSE;
		delegation_follow = ISC_TRUE;
		error_message = NULL;
		dup_name(&chase_authority_name, &chase_current_name, mctx);
		free_name(&chase_authority_name, mctx);
		return;
	}


	if (error_message != NULL) {
		dns_rdataset_t *rdataset;
		dns_rdataset_t *sigrdataset;
		dns_name_t rdata_name;
		isc_result_t ret = ISC_R_FAILURE;

		dns_name_init(&rdata_name, NULL);
		result = prove_nx(error_message, &chase_name,
				  current_lookup->rdclass_sigchase,
				  current_lookup->rdtype_sigchase, &rdata_name,
				  &rdataset, &sigrdataset);
		if (rdataset == NULL || sigrdataset == NULL ||
		    dns_name_countlabels(&rdata_name) == 0) {
			printf("\n;; Impossible to verify the non-existence,"
			       " the NSEC RRset can't be validated:"
			       " FAILED\n\n");
			goto cleanandgo;
		}
		ret = sigchase_verify_sig(&rdata_name, rdataset,
					  chase_keyrdataset,
					  sigrdataset, mctx);
		if (ret != ISC_R_SUCCESS) {
			free_name(&rdata_name, mctx);
			printf("\n;; Impossible to verify the NSEC RR to prove"
			       " the non-existence : FAILED\n\n");
			goto cleanandgo;
		}
		free_name(&rdata_name, mctx);
		if (result != ISC_R_SUCCESS) {
			printf("\n;; Impossible to verify the non-existence:"
			       " FAILED\n\n");
			goto cleanandgo;
		} else {
			printf("\n;; OK the query doesn't have response but"
			       " we have validate this fact : SUCCESS\n\n");
			goto cleanandgo;
		}
	}

 cleanandgo:
	printf(";; cleanandgo \n");
	if (dns_name_dynamic(&chase_current_name))
		free_name(&chase_current_name, mctx);
	if (dns_name_dynamic(&chase_authority_name))
		free_name(&chase_authority_name, mctx);
	clean_trustedkey();
	return;

	finalstep :
		result = advanced_rrsearch(&chase_rdataset, &chase_name,
					   current_lookup->rdtype_sigchase,
					   dns_rdatatype_any ,
					   &true);
	if (result == ISC_R_FAILURE) {
		printf("\n;; RRsig of RRset is missing to continue validation"
		       " SHOULD NOT APPEND: FAILED\n\n");
		goto cleanandgo;
	}
	result = sigchase_verify_sig(&chase_name, chase_rdataset,
				     chase_keyrdataset,
				     chase_sigrdataset, mctx);
	if (result != ISC_R_SUCCESS) {
		printf("\n;; Impossible to verify the RRset : FAILED\n\n");
		/*
		  printf("RRset:\n");
		  print_rdataset(&chase_name , chase_rdataset, mctx);
		  printf("DNSKEYset:\n");
		  print_rdataset(&chase_name , chase_keyrdataset, mctx);
		  printf("RRSIG of RRset:\n");
		  print_rdataset(&chase_name , chase_sigrdataset, mctx);
		  printf("\n");
		*/
		goto cleanandgo;
	} else {
		printf("\n;; The Answer:\n");
		print_rdataset(&chase_name , chase_rdataset, mctx);

		printf("\n;; FINISH : we have validate the DNSSEC chain"
		       " of trust: SUCCESS\n\n");
		goto cleanandgo;
	}
}

#endif


#if DIG_SIGCHASE_BU

isc_result_t
getneededrr(dns_message_t *msg)
{
	isc_result_t result;
	dns_name_t *name = NULL;
	dns_rdata_t sigrdata = DNS_RDATA_INIT;
	dns_rdata_sig_t siginfo;
	isc_boolean_t   true = ISC_TRUE;

	if ((result = dns_message_firstname(msg, DNS_SECTION_ANSWER))
	    != ISC_R_SUCCESS) {
		printf(";; NO ANSWERS: %s\n", isc_result_totext(result));

		if (chase_name.ndata == NULL)
			return (ISC_R_ADDRNOTAVAIL);
	} else {
		dns_message_currentname(msg, DNS_SECTION_ANSWER, &name);
	}

	/* What do we chase? */
	if (chase_rdataset == NULL) {
		result = advanced_rrsearch(&chase_rdataset, name,
					   dns_rdatatype_any,
					   dns_rdatatype_any, &true);
		if (result != ISC_R_SUCCESS) {
			printf("\n;; No Answers: Validation FAILED\n\n");
			return (ISC_R_NOTFOUND);
		}
		dup_name(name, &chase_name, mctx);
		printf(";; RRset to chase:\n");
		print_rdataset(&chase_name, chase_rdataset, mctx);
	}
	INSIST(chase_rdataset != NULL);


	if (chase_sigrdataset == NULL) {
		result = advanced_rrsearch(&chase_sigrdataset, name,
					   dns_rdatatype_rrsig,
					   chase_rdataset->type,
					   &chase_siglookedup);
		if (result == ISC_R_FAILURE) {
			printf("\n;; RRSIG is missing for continue validation:"
			       " FAILED\n\n");
			if (dns_name_dynamic(&chase_name))
				free_name(&chase_name, mctx);
			return (ISC_R_NOTFOUND);
		}
		if (result == ISC_R_NOTFOUND) {
			return (ISC_R_NOTFOUND);
		}
		printf("\n;; RRSIG of the RRset to chase:\n");
		print_rdataset(&chase_name, chase_sigrdataset, mctx);
	}
	INSIST(chase_sigrdataset != NULL);


	/* first find the DNSKEY name */
	result = dns_rdataset_first(chase_sigrdataset);
	check_result(result, "empty RRSIG dataset");
	dns_rdataset_current(chase_sigrdataset, &sigrdata);
	result = dns_rdata_tostruct(&sigrdata, &siginfo, NULL);
	check_result(result, "sigrdata tostruct siginfo");
	dup_name(&siginfo.signer, &chase_signame, mctx);
	dns_rdata_freestruct(&siginfo);
	dns_rdata_reset(&sigrdata);

	/* Do we have a key?  */
	if (chase_keyrdataset == NULL) {
		result = advanced_rrsearch(&chase_keyrdataset,
					   &chase_signame,
					   dns_rdatatype_dnskey,
					   dns_rdatatype_any,
					   &chase_keylookedup);
		if (result == ISC_R_FAILURE) {
			printf("\n;; DNSKEY is missing to continue validation:"
			       " FAILED\n\n");
			free_name(&chase_signame, mctx);
			if (dns_name_dynamic(&chase_name))
				free_name(&chase_name, mctx);
			return (ISC_R_NOTFOUND);
		}
		if (result == ISC_R_NOTFOUND) {
			free_name(&chase_signame, mctx);
			return (ISC_R_NOTFOUND);
		}
		printf("\n;; DNSKEYset that signs the RRset to chase:\n");
		print_rdataset(&chase_signame, chase_keyrdataset, mctx);
	}
	INSIST(chase_keyrdataset != NULL);

	if (chase_sigkeyrdataset == NULL) {
		result = advanced_rrsearch(&chase_sigkeyrdataset,
					   &chase_signame,
					   dns_rdatatype_rrsig,
					   dns_rdatatype_dnskey,
					   &chase_sigkeylookedup);
		if (result == ISC_R_FAILURE) {
			printf("\n;; RRSIG for DNSKEY  is missing  to continue"
			       " validation : FAILED\n\n");
			free_name(&chase_signame, mctx);
			if (dns_name_dynamic(&chase_name))
				free_name(&chase_name, mctx);
			return (ISC_R_NOTFOUND);
		}
		if (result == ISC_R_NOTFOUND) {
			free_name(&chase_signame, mctx);
			return (ISC_R_NOTFOUND);
		}
		printf("\n;; RRSIG of the DNSKEYset that signs the "
		       "RRset to chase:\n");
		print_rdataset(&chase_signame, chase_sigkeyrdataset, mctx);
	}
	INSIST(chase_sigkeyrdataset != NULL);


	if (chase_dsrdataset == NULL) {
		result = advanced_rrsearch(&chase_dsrdataset, &chase_signame,
					   dns_rdatatype_ds,
					   dns_rdatatype_any,
		&chase_dslookedup);
		if (result == ISC_R_FAILURE) {
			printf("\n;; WARNING There is no DS for the zone: ");
			dns_name_print(&chase_signame, stdout);
			printf("\n");
		}
		if (result == ISC_R_NOTFOUND) {
			free_name(&chase_signame, mctx);
			return (ISC_R_NOTFOUND);
		}
		if (chase_dsrdataset != NULL) {
			printf("\n;; DSset of the DNSKEYset\n");
			print_rdataset(&chase_signame, chase_dsrdataset, mctx);
		}
	}

	if (chase_dsrdataset != NULL) {
		/*
		 * if there is no RRSIG of DS,
		 * we don't want to search on the network
		 */
		result = advanced_rrsearch(&chase_sigdsrdataset,
					   &chase_signame,
					   dns_rdatatype_rrsig,
					   dns_rdatatype_ds, &true);
		if (result == ISC_R_FAILURE) {
			printf(";; WARNING : NO RRSIG DS : RRSIG DS"
			       " should come with DS\n");
			/*
			 * We continue even the DS couldn't be validated,
			 * because the DNSKEY could be a Trusted Key.
			 */
			chase_dsrdataset = NULL;
		} else {
			printf("\n;; RRSIG of the DSset of the DNSKEYset\n");
			print_rdataset(&chase_signame, chase_sigdsrdataset,
				       mctx);
		}
	}
	return (1);
}



void
sigchase_bu(dns_message_t *msg)
{
	isc_result_t result;
	int ret;

	if (tk_list.nb_tk == 0) {
		result = get_trusted_key(mctx);
		if (result != ISC_R_SUCCESS) {
			printf("No trusted keys present\n");
			return;
		}
	}


	ret = getneededrr(msg);
	if (ret == ISC_R_NOTFOUND)
		return;

	if (ret == ISC_R_ADDRNOTAVAIL) {
		/* We have no response */
		dns_rdataset_t *rdataset;
		dns_rdataset_t *sigrdataset;
		dns_name_t rdata_name;
		dns_name_t query_name;


		dns_name_init(&query_name, NULL);
		dns_name_init(&rdata_name, NULL);
		nameFromString(current_lookup->textname, &query_name);

		result = prove_nx(msg, &query_name, current_lookup->rdclass,
				  current_lookup->rdtype, &rdata_name,
				  &rdataset, &sigrdataset);
		free_name(&query_name, mctx);
		if (rdataset == NULL || sigrdataset == NULL ||
		    dns_name_countlabels(&rdata_name) == 0) {
			printf("\n;; Impossible to verify the Non-existence,"
			       " the NSEC RRset can't be validated: "
			       "FAILED\n\n");
			clean_trustedkey();
			return;
		}

		if (result != ISC_R_SUCCESS) {
			printf("\n No Answers and impossible to prove the"
			       " unsecurity : Validation FAILED\n\n");
			clean_trustedkey();
			return;
		}
		printf(";; An NSEC prove the non-existence of a answers,"
		       " Now we want validate this NSEC\n");

		dup_name(&rdata_name, &chase_name, mctx);
		free_name(&rdata_name, mctx);
		chase_rdataset =  rdataset;
		chase_sigrdataset = sigrdataset;
		chase_keyrdataset = NULL;
		chase_sigkeyrdataset = NULL;
		chase_dsrdataset = NULL;
		chase_sigdsrdataset = NULL;
		chase_siglookedup = ISC_FALSE;
		chase_keylookedup = ISC_FALSE;
		chase_dslookedup = ISC_FALSE;
		chase_sigdslookedup = ISC_FALSE;
		sigchase(msg);
		clean_trustedkey();
		return;
	}


	printf("\n\n\n;; WE HAVE MATERIAL, WE NOW DO VALIDATION\n");

	result = sigchase_verify_sig(&chase_name, chase_rdataset,
				     chase_keyrdataset,
				     chase_sigrdataset, mctx);
	if (result != ISC_R_SUCCESS) {
		free_name(&chase_name, mctx);
		free_name(&chase_signame, mctx);
		printf(";; No DNSKEY is valid to check the RRSIG"
		       " of the RRset: FAILED\n");
		clean_trustedkey();
		return;
	}
	printf(";; OK We found DNSKEY (or more) to validate the RRset\n");

	result = contains_trusted_key(&chase_signame, chase_keyrdataset,
				      chase_sigkeyrdataset, mctx);
	if (result ==  ISC_R_SUCCESS) {
		free_name(&chase_name, mctx);
		free_name(&chase_signame, mctx);
		printf("\n;; Ok this DNSKEY is a Trusted Key,"
		       " DNSSEC validation is ok: SUCCESS\n\n");
		clean_trustedkey();
		return;
	}

	printf(";; Now, we are going to validate this DNSKEY by the DS\n");

	if (chase_dsrdataset == NULL) {
		free_name(&chase_name, mctx);
		free_name(&chase_signame, mctx);
		printf(";; the DNSKEY isn't trusted-key and there isn't"
		       " DS to validate the DNSKEY: FAILED\n");
		clean_trustedkey();
		return;
	}

	result =  sigchase_verify_ds(&chase_signame, chase_keyrdataset,
				     chase_dsrdataset, mctx);
	if (result !=  ISC_R_SUCCESS) {
		free_name(&chase_signame, mctx);
		free_name(&chase_name, mctx);
		printf(";; ERROR no DS validates a DNSKEY in the"
		       " DNSKEY RRset: FAILED\n");
		clean_trustedkey();
		return;
	} else
		printf(";; OK this DNSKEY (validated by the DS) validates"
		       " the RRset of the DNSKEYs, thus the DNSKEY validates"
		       " the RRset\n");
	INSIST(chase_sigdsrdataset != NULL);

	dup_name(&chase_signame, &chase_name, mctx);
	free_name(&chase_signame, mctx);
	chase_rdataset = chase_dsrdataset;
	chase_sigrdataset = chase_sigdsrdataset;
	chase_keyrdataset = NULL;
	chase_sigkeyrdataset = NULL;
	chase_dsrdataset = NULL;
	chase_sigdsrdataset = NULL;
	chase_siglookedup = chase_keylookedup = ISC_FALSE;
	chase_dslookedup = chase_sigdslookedup = ISC_FALSE;

	printf(";; Now, we want to validate the DS :  recursive call\n");
	sigchase(msg);
	return;
}
#endif

void
sigchase(dns_message_t *msg) {
#if DIG_SIGCHASE_TD
	if (current_lookup->do_topdown) {
		sigchase_td(msg);
		return;
	}
#endif
#if DIG_SIGCHASE_BU
	sigchase_bu(msg);
	return;
#endif
}


/*
 * return 1  if name1  <  name2
 *	  0  if name1  == name2
 *	  -1 if name1  >  name2
 *    and -2 if problem
 */
int
inf_name(dns_name_t *name1, dns_name_t *name2)
{
	dns_label_t  label1;
	dns_label_t  label2;
	unsigned int nblabel1;
	unsigned int nblabel2;
	int min_lum_label;
	int i;
	int ret = -2;

	nblabel1 = dns_name_countlabels(name1);
	nblabel2 = dns_name_countlabels(name2);

	if (nblabel1 >= nblabel2)
		min_lum_label = nblabel2;
	else
		min_lum_label = nblabel1;


	for (i=1 ; i < min_lum_label; i++) {
		dns_name_getlabel(name1, nblabel1 -1  - i, &label1);
		dns_name_getlabel(name2, nblabel2 -1  - i, &label2);
		if ((ret = isc_region_compare(&label1, &label2)) != 0) {
			if (ret < 0)
				return (-1);
			else if (ret > 0)
				return (1);
		}
	}
	if (nblabel1 == nblabel2)
		return (0);

	if (nblabel1 < nblabel2)
		return (-1);
	else
		return (1);
}

/**
 *
 *
 *
 */
isc_result_t
prove_nx_domain(dns_message_t *msg,
		dns_name_t *name,
		dns_name_t *rdata_name,
		dns_rdataset_t **rdataset,
		dns_rdataset_t **sigrdataset)
{
	isc_result_t ret = ISC_R_FAILURE;
	isc_result_t result = ISC_R_NOTFOUND;
	dns_rdataset_t *nsecset = NULL;
	dns_rdataset_t *signsecset = NULL ;
	dns_rdata_t nsec = DNS_RDATA_INIT;
	dns_name_t *nsecname;
	dns_rdata_nsec_t nsecstruct;

	if ((result = dns_message_firstname(msg, DNS_SECTION_AUTHORITY))
	    != ISC_R_SUCCESS) {
		printf(";; nothing in authority section : impossible to"
		       " validate the non-existence : FAILED\n");
		return (ISC_R_FAILURE);
	}

	do {
		nsecname = NULL;
		dns_message_currentname(msg, DNS_SECTION_AUTHORITY, &nsecname);
		nsecset = search_type(nsecname, dns_rdatatype_nsec,
				      dns_rdatatype_any);
		if (nsecset == NULL)
			continue;

		printf("There is a NSEC for this zone in the"
		       " AUTHORITY section:\n");
		print_rdataset(nsecname, nsecset, mctx);

		for (result = dns_rdataset_first(nsecset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(nsecset)) {
			dns_rdataset_current(nsecset, &nsec);


			signsecset
				= chase_scanname_section(msg, nsecname,
						 dns_rdatatype_rrsig,
						 dns_rdatatype_nsec,
						 DNS_SECTION_AUTHORITY);
			if (signsecset == NULL) {
				printf(";; no RRSIG NSEC in authority section:"
				       " impossible to validate the "
				       "non-existence: FAILED\n");
				return (ISC_R_FAILURE);
			}

			ret = dns_rdata_tostruct(&nsec, &nsecstruct, NULL);
			check_result(ret,"dns_rdata_tostruct");

			if ((inf_name(nsecname, &nsecstruct.next) == 1 &&
			     inf_name(name, &nsecstruct.next) == 1) ||
			    (inf_name(name, nsecname) == 1 &&
			     inf_name(&nsecstruct.next, name) == 1)) {
				dns_rdata_freestruct(&nsecstruct);
				*rdataset = nsecset;
				*sigrdataset = signsecset;
				dup_name(nsecname, rdata_name, mctx);

				return (ISC_R_SUCCESS);
			}

			dns_rdata_freestruct(&nsecstruct);
			dns_rdata_reset(&nsec);
		}
	} while (dns_message_nextname(msg, DNS_SECTION_AUTHORITY)
		 == ISC_R_SUCCESS);

	*rdataset = NULL;
	*sigrdataset =  NULL;
	rdata_name = NULL;
	return (ISC_R_FAILURE);
}

/**
 *
 *
 *
 *
 *
 */
isc_result_t
prove_nx_type(dns_message_t *msg, dns_name_t *name, dns_rdataset_t *nsecset,
	      dns_rdataclass_t class, dns_rdatatype_t type,
	      dns_name_t *rdata_name, dns_rdataset_t **rdataset,
	      dns_rdataset_t **sigrdataset)
{
	isc_result_t ret;
	dns_rdataset_t *signsecset;
	dns_rdata_t nsec = DNS_RDATA_INIT;

	UNUSED(class);

	ret = dns_rdataset_first(nsecset);
	check_result(ret,"dns_rdataset_first");

	dns_rdataset_current(nsecset, &nsec);

	ret = dns_nsec_typepresent(&nsec, type);
	if (ret == ISC_R_SUCCESS)
		printf("OK the NSEC said that the type doesn't exist \n");

	signsecset = chase_scanname_section(msg, name,
					    dns_rdatatype_rrsig,
					    dns_rdatatype_nsec,
					    DNS_SECTION_AUTHORITY);
	if (signsecset == NULL) {
		printf("There isn't RRSIG NSEC for the zone \n");
		return (ISC_R_FAILURE);
	}
	dup_name(name, rdata_name, mctx);
	*rdataset = nsecset;
	*sigrdataset = signsecset;

	return (ret);
}

/**
 *
 *
 *
 *
 */
isc_result_t
prove_nx(dns_message_t *msg, dns_name_t *name, dns_rdataclass_t class,
	 dns_rdatatype_t type, dns_name_t *rdata_name,
	 dns_rdataset_t **rdataset, dns_rdataset_t **sigrdataset)
{
	isc_result_t ret;
	dns_rdataset_t *nsecset = NULL;

	printf("We want to prove the non-existence of a type of rdata %d"
	       " or of the zone: \n", type);

	if ((ret = dns_message_firstname(msg, DNS_SECTION_AUTHORITY))
	    != ISC_R_SUCCESS) {
		printf(";; nothing in authority section : impossible to"
		       " validate the non-existence : FAILED\n");
		return (ISC_R_FAILURE);
	}

	nsecset = chase_scanname_section(msg, name, dns_rdatatype_nsec,
					 dns_rdatatype_any,
					 DNS_SECTION_AUTHORITY);
	if (nsecset != NULL) {
		printf("We have a NSEC for this zone :OK\n");
		ret = prove_nx_type(msg, name, nsecset, class,
				    type, rdata_name, rdataset,
				    sigrdataset);
		if (ret != ISC_R_SUCCESS) {
			printf("prove_nx: ERROR type exist\n");
			return (ret);
		} else {
			printf("prove_nx: OK type does not exist\n");
			return (ISC_R_SUCCESS);
		}
	} else {
		printf("there is no NSEC for this zone: validating "
		       "that the zone doesn't exist\n");
		ret = prove_nx_domain(msg, name, rdata_name,
				      rdataset, sigrdataset);
		return (ret);
	}
	/* Never get here */
}
#endif
