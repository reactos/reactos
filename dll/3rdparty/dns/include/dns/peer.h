/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001, 2003  Internet Software Consortium.
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

/* $Id: peer.h,v 1.33.118.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_PEER_H
#define DNS_PEER_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/peer.h
 * \brief
 * Data structures for peers (e.g. a 'server' config file statement)
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/netaddr.h>

#include <dns/types.h>

#define DNS_PEERLIST_MAGIC	ISC_MAGIC('s','e','R','L')
#define DNS_PEER_MAGIC		ISC_MAGIC('S','E','r','v')

#define DNS_PEERLIST_VALID(ptr)	ISC_MAGIC_VALID(ptr, DNS_PEERLIST_MAGIC)
#define DNS_PEER_VALID(ptr)	ISC_MAGIC_VALID(ptr, DNS_PEER_MAGIC)

/***
 *** Types
 ***/

struct dns_peerlist {
	unsigned int		magic;
	isc_uint32_t		refs;

	isc_mem_t	       *mem;

	ISC_LIST(dns_peer_t) elements;
};

struct dns_peer {
	unsigned int		magic;
	isc_uint32_t		refs;

	isc_mem_t	       *mem;

	isc_netaddr_t		address;
	unsigned int		prefixlen;
	isc_boolean_t		bogus;
	dns_transfer_format_t	transfer_format;
	isc_uint32_t		transfers;
	isc_boolean_t		support_ixfr;
	isc_boolean_t		provide_ixfr;
	isc_boolean_t		request_ixfr;
	isc_boolean_t		support_edns;
	isc_boolean_t		request_nsid;
	dns_name_t	       *key;
	isc_sockaddr_t	       *transfer_source;
	isc_sockaddr_t	       *notify_source;
	isc_sockaddr_t	       *query_source;
	isc_uint16_t		udpsize;		/* receive size */
	isc_uint16_t		maxudp;			/* transmit size */

	isc_uint32_t		bitflags;

	ISC_LINK(dns_peer_t)	next;
};

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
dns_peerlist_new(isc_mem_t *mem, dns_peerlist_t **list);

void
dns_peerlist_attach(dns_peerlist_t *source, dns_peerlist_t **target);

void
dns_peerlist_detach(dns_peerlist_t **list);

/*
 * After return caller still holds a reference to peer.
 */
void
dns_peerlist_addpeer(dns_peerlist_t *peers, dns_peer_t *peer);

/*
 * Ditto. */
isc_result_t
dns_peerlist_peerbyaddr(dns_peerlist_t *peers, isc_netaddr_t *addr,
			dns_peer_t **retval);

/*
 * What he said.
 */
isc_result_t
dns_peerlist_currpeer(dns_peerlist_t *peers, dns_peer_t **retval);

isc_result_t
dns_peer_new(isc_mem_t *mem, isc_netaddr_t *ipaddr, dns_peer_t **peer);

isc_result_t
dns_peer_newprefix(isc_mem_t *mem, isc_netaddr_t *ipaddr,
		   unsigned int prefixlen, dns_peer_t **peer);

void
dns_peer_attach(dns_peer_t *source, dns_peer_t **target);

void
dns_peer_detach(dns_peer_t **list);

isc_result_t
dns_peer_setbogus(dns_peer_t *peer, isc_boolean_t newval);

isc_result_t
dns_peer_getbogus(dns_peer_t *peer, isc_boolean_t *retval);

isc_result_t
dns_peer_setrequestixfr(dns_peer_t *peer, isc_boolean_t newval);

isc_result_t
dns_peer_getrequestixfr(dns_peer_t *peer, isc_boolean_t *retval);

isc_result_t
dns_peer_setprovideixfr(dns_peer_t *peer, isc_boolean_t newval);

isc_result_t
dns_peer_getprovideixfr(dns_peer_t *peer, isc_boolean_t *retval);

isc_result_t
dns_peer_setrequestnsid(dns_peer_t *peer, isc_boolean_t newval);

isc_result_t
dns_peer_getrequestnsid(dns_peer_t *peer, isc_boolean_t *retval);

isc_result_t
dns_peer_setsupportedns(dns_peer_t *peer, isc_boolean_t newval);

isc_result_t
dns_peer_getsupportedns(dns_peer_t *peer, isc_boolean_t *retval);

isc_result_t
dns_peer_settransfers(dns_peer_t *peer, isc_uint32_t newval);

isc_result_t
dns_peer_gettransfers(dns_peer_t *peer, isc_uint32_t *retval);

isc_result_t
dns_peer_settransferformat(dns_peer_t *peer, dns_transfer_format_t newval);

isc_result_t
dns_peer_gettransferformat(dns_peer_t *peer, dns_transfer_format_t *retval);

isc_result_t
dns_peer_setkeybycharp(dns_peer_t *peer, const char *keyval);

isc_result_t
dns_peer_getkey(dns_peer_t *peer, dns_name_t **retval);

isc_result_t
dns_peer_setkey(dns_peer_t *peer, dns_name_t **keyval);

isc_result_t
dns_peer_settransfersource(dns_peer_t *peer,
			   const isc_sockaddr_t *transfer_source);

isc_result_t
dns_peer_gettransfersource(dns_peer_t *peer, isc_sockaddr_t *transfer_source);

isc_result_t
dns_peer_setudpsize(dns_peer_t *peer, isc_uint16_t udpsize);

isc_result_t
dns_peer_getudpsize(dns_peer_t *peer, isc_uint16_t *udpsize);

isc_result_t
dns_peer_setmaxudp(dns_peer_t *peer, isc_uint16_t maxudp);

isc_result_t
dns_peer_getmaxudp(dns_peer_t *peer, isc_uint16_t *maxudp);

isc_result_t
dns_peer_setnotifysource(dns_peer_t *peer, const isc_sockaddr_t *notify_source);

isc_result_t
dns_peer_getnotifysource(dns_peer_t *peer, isc_sockaddr_t *notify_source);

isc_result_t
dns_peer_setquerysource(dns_peer_t *peer, const isc_sockaddr_t *query_source);

isc_result_t
dns_peer_getquerysource(dns_peer_t *peer, isc_sockaddr_t *query_source);

ISC_LANG_ENDDECLS

#endif /* DNS_PEER_H */
