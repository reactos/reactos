/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: peer.c,v 1.31 2008/04/03 06:09:04 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>
#include <isc/sockaddr.h>

#include <dns/bit.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/peer.h>

/*%
 * Bit positions in the dns_peer_t structure flags field
 */
#define BOGUS_BIT			0
#define SERVER_TRANSFER_FORMAT_BIT	1
#define TRANSFERS_BIT			2
#define PROVIDE_IXFR_BIT		3
#define REQUEST_IXFR_BIT		4
#define SUPPORT_EDNS_BIT		5
#define SERVER_UDPSIZE_BIT		6
#define SERVER_MAXUDP_BIT		7
#define REQUEST_NSID_BIT                8

static void
peerlist_delete(dns_peerlist_t **list);

static void
peer_delete(dns_peer_t **peer);

isc_result_t
dns_peerlist_new(isc_mem_t *mem, dns_peerlist_t **list) {
	dns_peerlist_t *l;

	REQUIRE(list != NULL);

	l = isc_mem_get(mem, sizeof(*l));
	if (l == NULL)
		return (ISC_R_NOMEMORY);

	ISC_LIST_INIT(l->elements);
	l->mem = mem;
	l->refs = 1;
	l->magic = DNS_PEERLIST_MAGIC;

	*list = l;

	return (ISC_R_SUCCESS);
}

void
dns_peerlist_attach(dns_peerlist_t *source, dns_peerlist_t **target) {
	REQUIRE(DNS_PEERLIST_VALID(source));
	REQUIRE(target != NULL);
	REQUIRE(*target == NULL);

	source->refs++;

	ENSURE(source->refs != 0xffffffffU);

	*target = source;
}

void
dns_peerlist_detach(dns_peerlist_t **list) {
	dns_peerlist_t *plist;

	REQUIRE(list != NULL);
	REQUIRE(*list != NULL);
	REQUIRE(DNS_PEERLIST_VALID(*list));

	plist = *list;
	*list = NULL;

	REQUIRE(plist->refs > 0);

	plist->refs--;

	if (plist->refs == 0)
		peerlist_delete(&plist);
}

static void
peerlist_delete(dns_peerlist_t **list) {
	dns_peerlist_t *l;
	dns_peer_t *server, *stmp;

	REQUIRE(list != NULL);
	REQUIRE(DNS_PEERLIST_VALID(*list));

	l = *list;

	REQUIRE(l->refs == 0);

	server = ISC_LIST_HEAD(l->elements);
	while (server != NULL) {
		stmp = ISC_LIST_NEXT(server, next);
		ISC_LIST_UNLINK(l->elements, server, next);
		dns_peer_detach(&server);
		server = stmp;
	}

	l->magic = 0;
	isc_mem_put(l->mem, l, sizeof(*l));

	*list = NULL;
}

void
dns_peerlist_addpeer(dns_peerlist_t *peers, dns_peer_t *peer) {
	dns_peer_t *p = NULL;

	dns_peer_attach(peer, &p);

	/*
	 * More specifics to front of list.
	 */
	for (p = ISC_LIST_HEAD(peers->elements);
	     p != NULL;
	     p = ISC_LIST_NEXT(p, next))
		if (p->prefixlen < peer->prefixlen)
			break;

	if (p != NULL)
		ISC_LIST_INSERTBEFORE(peers->elements, p, peer, next);
	else
		ISC_LIST_APPEND(peers->elements, peer, next);

}

isc_result_t
dns_peerlist_peerbyaddr(dns_peerlist_t *servers,
			isc_netaddr_t *addr, dns_peer_t **retval)
{
	dns_peer_t *server;
	isc_result_t res;

	REQUIRE(retval != NULL);
	REQUIRE(DNS_PEERLIST_VALID(servers));

	server = ISC_LIST_HEAD(servers->elements);
	while (server != NULL) {
		if (isc_netaddr_eqprefix(addr, &server->address,
					 server->prefixlen))
			break;

		server = ISC_LIST_NEXT(server, next);
	}

	if (server != NULL) {
		*retval = server;
		res = ISC_R_SUCCESS;
	} else {
		res = ISC_R_NOTFOUND;
	}

	return (res);
}



isc_result_t
dns_peerlist_currpeer(dns_peerlist_t *peers, dns_peer_t **retval) {
	dns_peer_t *p = NULL;

	p = ISC_LIST_TAIL(peers->elements);

	dns_peer_attach(p, retval);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_new(isc_mem_t *mem, isc_netaddr_t *addr, dns_peer_t **peerptr) {
	unsigned int prefixlen = 0;

	REQUIRE(peerptr != NULL);
	switch(addr->family) {
	case AF_INET:
		prefixlen = 32;
		break;
	case AF_INET6:
		 prefixlen = 128;
		break;
	default:
		INSIST(0);
	}

	return (dns_peer_newprefix(mem, addr, prefixlen, peerptr));
}

isc_result_t
dns_peer_newprefix(isc_mem_t *mem, isc_netaddr_t *addr, unsigned int prefixlen,
		   dns_peer_t **peerptr)
{
	dns_peer_t *peer;

	REQUIRE(peerptr != NULL);

	peer = isc_mem_get(mem, sizeof(*peer));
	if (peer == NULL)
		return (ISC_R_NOMEMORY);

	peer->magic = DNS_PEER_MAGIC;
	peer->address = *addr;
	peer->prefixlen = prefixlen;
	peer->mem = mem;
	peer->bogus = ISC_FALSE;
	peer->transfer_format = dns_one_answer;
	peer->transfers = 0;
	peer->request_ixfr = ISC_FALSE;
	peer->provide_ixfr = ISC_FALSE;
	peer->key = NULL;
	peer->refs = 1;
	peer->transfer_source = NULL;
	peer->notify_source = NULL;
	peer->query_source = NULL;

	memset(&peer->bitflags, 0x0, sizeof(peer->bitflags));

	ISC_LINK_INIT(peer, next);

	*peerptr = peer;

	return (ISC_R_SUCCESS);
}

void
dns_peer_attach(dns_peer_t *source, dns_peer_t **target) {
	REQUIRE(DNS_PEER_VALID(source));
	REQUIRE(target != NULL);
	REQUIRE(*target == NULL);

	source->refs++;

	ENSURE(source->refs != 0xffffffffU);

	*target = source;
}

void
dns_peer_detach(dns_peer_t **peer) {
	dns_peer_t *p;

	REQUIRE(peer != NULL);
	REQUIRE(*peer != NULL);
	REQUIRE(DNS_PEER_VALID(*peer));

	p = *peer;

	REQUIRE(p->refs > 0);

	*peer = NULL;
	p->refs--;

	if (p->refs == 0)
		peer_delete(&p);
}

static void
peer_delete(dns_peer_t **peer) {
	dns_peer_t *p;
	isc_mem_t *mem;

	REQUIRE(peer != NULL);
	REQUIRE(DNS_PEER_VALID(*peer));

	p = *peer;

	REQUIRE(p->refs == 0);

	mem = p->mem;
	p->mem = NULL;
	p->magic = 0;

	if (p->key != NULL) {
		dns_name_free(p->key, mem);
		isc_mem_put(mem, p->key, sizeof(dns_name_t));
	}

	if (p->transfer_source != NULL) {
		isc_mem_put(mem, p->transfer_source,
			    sizeof(*p->transfer_source));
	}

	isc_mem_put(mem, p, sizeof(*p));

	*peer = NULL;
}

isc_result_t
dns_peer_setbogus(dns_peer_t *peer, isc_boolean_t newval) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(BOGUS_BIT, &peer->bitflags);

	peer->bogus = newval;
	DNS_BIT_SET(BOGUS_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getbogus(dns_peer_t *peer, isc_boolean_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(BOGUS_BIT, &peer->bitflags)) {
		*retval = peer->bogus;
		return (ISC_R_SUCCESS);
	} else
		return (ISC_R_NOTFOUND);
}


isc_result_t
dns_peer_setprovideixfr(dns_peer_t *peer, isc_boolean_t newval) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(PROVIDE_IXFR_BIT, &peer->bitflags);

	peer->provide_ixfr = newval;
	DNS_BIT_SET(PROVIDE_IXFR_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getprovideixfr(dns_peer_t *peer, isc_boolean_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(PROVIDE_IXFR_BIT, &peer->bitflags)) {
		*retval = peer->provide_ixfr;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setrequestixfr(dns_peer_t *peer, isc_boolean_t newval) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(REQUEST_IXFR_BIT, &peer->bitflags);

	peer->request_ixfr = newval;
	DNS_BIT_SET(REQUEST_IXFR_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getrequestixfr(dns_peer_t *peer, isc_boolean_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(REQUEST_IXFR_BIT, &peer->bitflags)) {
		*retval = peer->request_ixfr;
		return (ISC_R_SUCCESS);
	} else
		return (ISC_R_NOTFOUND);
}

isc_result_t
dns_peer_setsupportedns(dns_peer_t *peer, isc_boolean_t newval) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SUPPORT_EDNS_BIT, &peer->bitflags);

	peer->support_edns = newval;
	DNS_BIT_SET(SUPPORT_EDNS_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getsupportedns(dns_peer_t *peer, isc_boolean_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(SUPPORT_EDNS_BIT, &peer->bitflags)) {
		*retval = peer->support_edns;
		return (ISC_R_SUCCESS);
	} else
		return (ISC_R_NOTFOUND);
}

isc_result_t
dns_peer_setrequestnsid(dns_peer_t *peer, isc_boolean_t newval) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(REQUEST_NSID_BIT, &peer->bitflags);

	peer->request_nsid = newval;
	DNS_BIT_SET(REQUEST_NSID_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getrequestnsid(dns_peer_t *peer, isc_boolean_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(REQUEST_NSID_BIT, &peer->bitflags)) {
		*retval = peer->request_nsid;
		return (ISC_R_SUCCESS);
	} else
		return (ISC_R_NOTFOUND);
}

isc_result_t
dns_peer_settransfers(dns_peer_t *peer, isc_uint32_t newval) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(TRANSFERS_BIT, &peer->bitflags);

	peer->transfers = newval;
	DNS_BIT_SET(TRANSFERS_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_gettransfers(dns_peer_t *peer, isc_uint32_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(TRANSFERS_BIT, &peer->bitflags)) {
		*retval = peer->transfers;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_settransferformat(dns_peer_t *peer, dns_transfer_format_t newval) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SERVER_TRANSFER_FORMAT_BIT,
				 &peer->bitflags);

	peer->transfer_format = newval;
	DNS_BIT_SET(SERVER_TRANSFER_FORMAT_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_gettransferformat(dns_peer_t *peer, dns_transfer_format_t *retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (DNS_BIT_CHECK(SERVER_TRANSFER_FORMAT_BIT, &peer->bitflags)) {
		*retval = peer->transfer_format;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_getkey(dns_peer_t *peer, dns_name_t **retval) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(retval != NULL);

	if (peer->key != NULL) {
		*retval = peer->key;
	}

	return (peer->key == NULL ? ISC_R_NOTFOUND : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_setkey(dns_peer_t *peer, dns_name_t **keyval) {
	isc_boolean_t exists = ISC_FALSE;

	if (peer->key != NULL) {
		dns_name_free(peer->key, peer->mem);
		isc_mem_put(peer->mem, peer->key, sizeof(dns_name_t));
		exists = ISC_TRUE;
	}

	peer->key = *keyval;
	*keyval = NULL;

	return (exists ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_setkeybycharp(dns_peer_t *peer, const char *keyval) {
	isc_buffer_t b;
	dns_fixedname_t fname;
	dns_name_t *name;
	isc_result_t result;

	dns_fixedname_init(&fname);
	isc_buffer_init(&b, keyval, strlen(keyval));
	isc_buffer_add(&b, strlen(keyval));
	result = dns_name_fromtext(dns_fixedname_name(&fname), &b,
				   dns_rootname, ISC_FALSE, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);

	name = isc_mem_get(peer->mem, sizeof(dns_name_t));
	if (name == NULL)
		return (ISC_R_NOMEMORY);

	dns_name_init(name, NULL);
	result = dns_name_dup(dns_fixedname_name(&fname), peer->mem, name);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(peer->mem, name, sizeof(dns_name_t));
		return (result);
	}

	result = dns_peer_setkey(peer, &name);
	if (result != ISC_R_SUCCESS)
		isc_mem_put(peer->mem, name, sizeof(dns_name_t));

	return (result);
}

isc_result_t
dns_peer_settransfersource(dns_peer_t *peer,
			   const isc_sockaddr_t *transfer_source)
{
	REQUIRE(DNS_PEER_VALID(peer));

	if (peer->transfer_source != NULL) {
		isc_mem_put(peer->mem, peer->transfer_source,
			    sizeof(*peer->transfer_source));
		peer->transfer_source = NULL;
	}
	if (transfer_source != NULL) {
		peer->transfer_source = isc_mem_get(peer->mem,
						sizeof(*peer->transfer_source));
		if (peer->transfer_source == NULL)
			return (ISC_R_NOMEMORY);

		*peer->transfer_source = *transfer_source;
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_gettransfersource(dns_peer_t *peer, isc_sockaddr_t *transfer_source) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(transfer_source != NULL);

	if (peer->transfer_source == NULL)
		return (ISC_R_NOTFOUND);
	*transfer_source = *peer->transfer_source;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_setnotifysource(dns_peer_t *peer,
			 const isc_sockaddr_t *notify_source)
{
	REQUIRE(DNS_PEER_VALID(peer));

	if (peer->notify_source != NULL) {
		isc_mem_put(peer->mem, peer->notify_source,
			    sizeof(*peer->notify_source));
		peer->notify_source = NULL;
	}
	if (notify_source != NULL) {
		peer->notify_source = isc_mem_get(peer->mem,
						sizeof(*peer->notify_source));
		if (peer->notify_source == NULL)
			return (ISC_R_NOMEMORY);

		*peer->notify_source = *notify_source;
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getnotifysource(dns_peer_t *peer, isc_sockaddr_t *notify_source) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(notify_source != NULL);

	if (peer->notify_source == NULL)
		return (ISC_R_NOTFOUND);
	*notify_source = *peer->notify_source;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_setquerysource(dns_peer_t *peer, const isc_sockaddr_t *query_source) {
	REQUIRE(DNS_PEER_VALID(peer));

	if (peer->query_source != NULL) {
		isc_mem_put(peer->mem, peer->query_source,
			    sizeof(*peer->query_source));
		peer->query_source = NULL;
	}
	if (query_source != NULL) {
		peer->query_source = isc_mem_get(peer->mem,
						sizeof(*peer->query_source));
		if (peer->query_source == NULL)
			return (ISC_R_NOMEMORY);

		*peer->query_source = *query_source;
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getquerysource(dns_peer_t *peer, isc_sockaddr_t *query_source) {
	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(query_source != NULL);

	if (peer->query_source == NULL)
		return (ISC_R_NOTFOUND);
	*query_source = *peer->query_source;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_peer_setudpsize(dns_peer_t *peer, isc_uint16_t udpsize) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SERVER_UDPSIZE_BIT, &peer->bitflags);

	peer->udpsize = udpsize;
	DNS_BIT_SET(SERVER_UDPSIZE_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getudpsize(dns_peer_t *peer, isc_uint16_t *udpsize) {

	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(udpsize != NULL);

	if (DNS_BIT_CHECK(SERVER_UDPSIZE_BIT, &peer->bitflags)) {
		*udpsize = peer->udpsize;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}

isc_result_t
dns_peer_setmaxudp(dns_peer_t *peer, isc_uint16_t maxudp) {
	isc_boolean_t existed;

	REQUIRE(DNS_PEER_VALID(peer));

	existed = DNS_BIT_CHECK(SERVER_MAXUDP_BIT, &peer->bitflags);

	peer->maxudp = maxudp;
	DNS_BIT_SET(SERVER_MAXUDP_BIT, &peer->bitflags);

	return (existed ? ISC_R_EXISTS : ISC_R_SUCCESS);
}

isc_result_t
dns_peer_getmaxudp(dns_peer_t *peer, isc_uint16_t *maxudp) {

	REQUIRE(DNS_PEER_VALID(peer));
	REQUIRE(maxudp != NULL);

	if (DNS_BIT_CHECK(SERVER_MAXUDP_BIT, &peer->bitflags)) {
		*maxudp = peer->maxudp;
		return (ISC_R_SUCCESS);
	} else {
		return (ISC_R_NOTFOUND);
	}
}
