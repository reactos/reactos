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

/* $Id: dig.h,v 1.107.120.2 2009/01/06 23:47:26 tbox Exp $ */

#ifndef DIG_H
#define DIG_H

/*! \file */

#include <dns/rdatalist.h>

#include <dst/dst.h>

#include <isc/boolean.h>
#include <isc/buffer.h>
#include <isc/bufferlist.h>
#include <isc/formatcheck.h>
#include <isc/lang.h>
#include <isc/list.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/sockaddr.h>
#include <isc/socket.h>

#define MXSERV 20
#define MXNAME (DNS_NAME_MAXTEXT+1)
#define MXRD 32
/*% Buffer Size */
#define BUFSIZE 512
#define COMMSIZE 0xffff
#ifndef RESOLV_CONF
/*% location of resolve.conf */
#define RESOLV_CONF "/etc/resolv.conf"
#endif
/*% output buffer */
#define OUTPUTBUF 32767
/*% Max RR Limit */
#define MAXRRLIMIT 0xffffffff
#define MAXTIMEOUT 0xffff
/*% Max number of tries */
#define MAXTRIES 0xffffffff
/*% Max number of dots */
#define MAXNDOTS 0xffff
/*% Max number of ports */
#define MAXPORT 0xffff
/*% Max serial number */
#define MAXSERIAL 0xffffffff

/*% Default TCP Timeout */
#define TCP_TIMEOUT 10
/*% Default UDP Timeout */
#define UDP_TIMEOUT 5

#define SERVER_TIMEOUT 1

#define LOOKUP_LIMIT 64
/*%
 * Lookup_limit is just a limiter, keeping too many lookups from being
 * created.  It's job is mainly to prevent the program from running away
 * in a tight loop of constant lookups.  It's value is arbitrary.
 */

/*
 * Defaults for the sigchase suboptions.  Consolidated here because
 * these control the layout of dig_lookup_t (among other things).
 */
#ifdef DIG_SIGCHASE
#ifndef DIG_SIGCHASE_BU
#define DIG_SIGCHASE_BU 1
#endif
#ifndef DIG_SIGCHASE_TD
#define DIG_SIGCHASE_TD 1
#endif
#endif

ISC_LANG_BEGINDECLS

typedef struct dig_lookup dig_lookup_t;
typedef struct dig_query dig_query_t;
typedef struct dig_server dig_server_t;
#ifdef DIG_SIGCHASE
typedef struct dig_message dig_message_t;
#endif
typedef ISC_LIST(dig_server_t) dig_serverlist_t;
typedef struct dig_searchlist dig_searchlist_t;

/*% The dig_lookup structure */
struct dig_lookup {
	isc_boolean_t
		pending, /*%< Pending a successful answer */
		waiting_connect,
		doing_xfr,
		ns_search_only, /*%< dig +nssearch, host -C */
		identify, /*%< Append an "on server <foo>" message */
		identify_previous_line, /*% Prepend a "Nameserver <foo>:"
					   message, with newline and tab */
		ignore,
		recurse,
		aaonly,
		adflag,
		cdflag,
		trace, /*% dig +trace */
		trace_root, /*% initial query for either +trace or +nssearch */
		tcp_mode,
		ip6_int,
		comments,
		stats,
		section_question,
		section_answer,
		section_authority,
		section_additional,
		servfail_stops,
		new_search,
		need_search,
		done_as_is,
		besteffort,
		dnssec,
		nsid;   /*% Name Server ID (RFC 5001) */
#ifdef DIG_SIGCHASE
isc_boolean_t	sigchase;
#if DIG_SIGCHASE_TD
	isc_boolean_t do_topdown,
		trace_root_sigchase,
		rdtype_sigchaseset,
		rdclass_sigchaseset;
	/* Name we are going to validate RRset */
	char textnamesigchase[MXNAME];
#endif
#endif

	char textname[MXNAME]; /*% Name we're going to be looking up */
	char cmdline[MXNAME];
	dns_rdatatype_t rdtype;
	dns_rdatatype_t qrdtype;
#if DIG_SIGCHASE_TD
	dns_rdatatype_t rdtype_sigchase;
	dns_rdatatype_t qrdtype_sigchase;
	dns_rdataclass_t rdclass_sigchase;
#endif
	dns_rdataclass_t rdclass;
	isc_boolean_t rdtypeset;
	isc_boolean_t rdclassset;
	char namespace[BUFSIZE];
	char onamespace[BUFSIZE];
	isc_buffer_t namebuf;
	isc_buffer_t onamebuf;
	isc_buffer_t renderbuf;
	char *sendspace;
	dns_name_t *name;
	isc_timer_t *timer;
	isc_interval_t interval;
	dns_message_t *sendmsg;
	dns_name_t *oname;
	ISC_LINK(dig_lookup_t) link;
	ISC_LIST(dig_query_t) q;
	dig_query_t *current_query;
	dig_serverlist_t my_server_list;
	dig_searchlist_t *origin;
	dig_query_t *xfr_q;
	isc_uint32_t retries;
	int nsfound;
	isc_uint16_t udpsize;
	isc_int16_t edns;
	isc_uint32_t ixfr_serial;
	isc_buffer_t rdatabuf;
	char rdatastore[MXNAME];
	dst_context_t *tsigctx;
	isc_buffer_t *querysig;
	isc_uint32_t msgcounter;
	dns_fixedname_t fdomain;
};

/*% The dig_query structure */
struct dig_query {
	dig_lookup_t *lookup;
	isc_boolean_t waiting_connect,
		pending_free,
		waiting_senddone,
		first_pass,
		first_soa_rcvd,
		second_rr_rcvd,
		first_repeat_rcvd,
		recv_made,
		warn_id;
	isc_uint32_t first_rr_serial;
	isc_uint32_t second_rr_serial;
	isc_uint32_t msg_count;
	isc_uint32_t rr_count;
	char *servname;
	char *userarg;
	isc_bufferlist_t sendlist,
		recvlist,
		lengthlist;
	isc_buffer_t recvbuf,
		lengthbuf,
		slbuf;
	char *recvspace,
		lengthspace[4],
		slspace[4];
	isc_socket_t *sock;
	ISC_LINK(dig_query_t) link;
	isc_sockaddr_t sockaddr;
	isc_time_t time_sent;
	isc_uint64_t byte_count;
	isc_buffer_t sendbuf;
};

struct dig_server {
	char servername[MXNAME];
	char userarg[MXNAME];
	ISC_LINK(dig_server_t) link;
};

struct dig_searchlist {
	char origin[MXNAME];
	ISC_LINK(dig_searchlist_t) link;
};
#ifdef DIG_SIGCHASE
struct dig_message {
		dns_message_t *msg;
		ISC_LINK(dig_message_t) link;
};
#endif

typedef ISC_LIST(dig_searchlist_t) dig_searchlistlist_t;
typedef ISC_LIST(dig_lookup_t) dig_lookuplist_t;

/*
 * Externals from dighost.c
 */

extern dig_lookuplist_t lookup_list;
extern dig_serverlist_t server_list;
extern dig_searchlistlist_t search_list;
extern unsigned int extrabytes;

extern isc_boolean_t check_ra, have_ipv4, have_ipv6, specified_source,
	usesearch, showsearch, qr;
extern in_port_t port;
extern unsigned int timeout;
extern isc_mem_t *mctx;
extern dns_messageid_t id;
extern int sendcount;
extern int ndots;
extern int lookup_counter;
extern int exitcode;
extern isc_sockaddr_t bind_address;
extern char keynametext[MXNAME];
extern char keyfile[MXNAME];
extern char keysecret[MXNAME];
extern dns_name_t *hmacname;
extern unsigned int digestbits;
#ifdef DIG_SIGCHASE
extern char trustedkey[MXNAME];
#endif
extern dns_tsigkey_t *key;
extern isc_boolean_t validated;
extern isc_taskmgr_t *taskmgr;
extern isc_task_t *global_task;
extern isc_boolean_t free_now;
extern isc_boolean_t debugging, memdebugging;

extern char *progname;
extern int tries;
extern int fatalexit;
#ifdef WITH_IDN
extern int idnoptions;
#endif

/*
 * Routines in dighost.c.
 */
isc_result_t
get_address(char *host, in_port_t port, isc_sockaddr_t *sockaddr);

isc_result_t
get_reverse(char *reverse, size_t len, char *value, isc_boolean_t ip6_int,
	    isc_boolean_t strict);

void
fatal(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

void
debug(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

void
check_result(isc_result_t result, const char *msg);

void
setup_lookup(dig_lookup_t *lookup);

void
destroy_lookup(dig_lookup_t *lookup);

void
do_lookup(dig_lookup_t *lookup);

void
start_lookup(void);

void
onrun_callback(isc_task_t *task, isc_event_t *event);

int
dhmain(int argc, char **argv);

void
setup_libs(void);

void
setup_system(void);

dig_lookup_t *
requeue_lookup(dig_lookup_t *lookold, isc_boolean_t servers);

dig_lookup_t *
make_empty_lookup(void);

dig_lookup_t *
clone_lookup(dig_lookup_t *lookold, isc_boolean_t servers);

dig_server_t *
make_server(const char *servname, const char *userarg);

void
flush_server_list(void);

void
set_nameserver(char *opt);

void
clone_server_list(dig_serverlist_t src,
		  dig_serverlist_t *dest);

void
cancel_all(void);

void
destroy_libs(void);

void
set_search_domain(char *domain);

#ifdef DIG_SIGCHASE
void
clean_trustedkey(void);
#endif

/*
 * Routines to be defined in dig.c, host.c, and nslookup.c.
 */
#ifdef DIG_SIGCHASE
isc_result_t
printrdataset(dns_name_t *owner_name, dns_rdataset_t *rdataset,
	      isc_buffer_t *target);
#endif

isc_result_t
printmessage(dig_query_t *query, dns_message_t *msg, isc_boolean_t headers);
/*%<
 * Print the final result of the lookup.
 */

void
received(int bytes, isc_sockaddr_t *from, dig_query_t *query);
/*%<
 * Print a message about where and when the response
 * was received from, like the final comment in the
 * output of "dig".
 */

void
trying(char *frm, dig_lookup_t *lookup);

void
dighost_shutdown(void);

char *
next_token(char **stringp, const char *delim);

#ifdef DIG_SIGCHASE
/* Chasing functions */
dns_rdataset_t *
chase_scanname(dns_name_t *name, dns_rdatatype_t type, dns_rdatatype_t covers);
void
chase_sig(dns_message_t *msg);
#endif

ISC_LANG_ENDDECLS

#endif
