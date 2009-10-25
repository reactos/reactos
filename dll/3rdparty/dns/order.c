/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2002  Internet Software Consortium.
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

/* $Id: order.c,v 1.10 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/types.h>
#include <isc/util.h>
#include <isc/refcount.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/order.h>
#include <dns/rdataset.h>
#include <dns/types.h>

typedef struct dns_order_ent dns_order_ent_t;
struct dns_order_ent {
	dns_fixedname_t			name;
	dns_rdataclass_t		rdclass;
	dns_rdatatype_t			rdtype;
	unsigned int			mode;
	ISC_LINK(dns_order_ent_t)	link;
};

struct dns_order {
	unsigned int			magic;
	isc_refcount_t          	references;
	ISC_LIST(dns_order_ent_t)	ents;
	isc_mem_t			*mctx;
};
	
#define DNS_ORDER_MAGIC ISC_MAGIC('O','r','d','r')
#define DNS_ORDER_VALID(order)	ISC_MAGIC_VALID(order, DNS_ORDER_MAGIC)

isc_result_t
dns_order_create(isc_mem_t *mctx, dns_order_t **orderp) {
	dns_order_t *order;
	isc_result_t result;

	REQUIRE(orderp != NULL && *orderp == NULL);

	order = isc_mem_get(mctx, sizeof(*order));
	if (order == NULL)
		return (ISC_R_NOMEMORY);
	
	ISC_LIST_INIT(order->ents);

	/* Implicit attach. */
	result = isc_refcount_init(&order->references, 1);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, order, sizeof(*order));
		return (result);
	}

	order->mctx = NULL;
	isc_mem_attach(mctx, &order->mctx);
	order->magic = DNS_ORDER_MAGIC;
	*orderp = order;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_order_add(dns_order_t *order, dns_name_t *name,
	      dns_rdatatype_t rdtype, dns_rdataclass_t rdclass,
	      unsigned int mode)
{
	dns_order_ent_t *ent;

	REQUIRE(DNS_ORDER_VALID(order));
	REQUIRE(mode == DNS_RDATASETATTR_RANDOMIZE ||
	        mode == DNS_RDATASETATTR_FIXEDORDER ||
		mode == 0 /* DNS_RDATASETATTR_CYCLIC */ );

	ent = isc_mem_get(order->mctx, sizeof(*ent));
	if (ent == NULL)
		return (ISC_R_NOMEMORY);

	dns_fixedname_init(&ent->name);
	RUNTIME_CHECK(dns_name_copy(name, dns_fixedname_name(&ent->name), NULL)
		      == ISC_R_SUCCESS);
	ent->rdtype = rdtype;
	ent->rdclass = rdclass;
	ent->mode = mode;
	ISC_LINK_INIT(ent, link);
	ISC_LIST_INITANDAPPEND(order->ents, ent, link);
	return (ISC_R_SUCCESS);
}

static inline isc_boolean_t
match(dns_name_t *name1, dns_name_t *name2) {
	
	if (dns_name_iswildcard(name2))
		return(dns_name_matcheswildcard(name1, name2));
	return (dns_name_equal(name1, name2));
}

unsigned int
dns_order_find(dns_order_t *order, dns_name_t *name,
	       dns_rdatatype_t rdtype, dns_rdataclass_t rdclass)
{
	dns_order_ent_t *ent;
	REQUIRE(DNS_ORDER_VALID(order));

	for (ent = ISC_LIST_HEAD(order->ents);
	     ent != NULL;
	     ent = ISC_LIST_NEXT(ent, link)) {
		if (ent->rdtype != rdtype && ent->rdtype != dns_rdatatype_any)
			continue;
		if (ent->rdclass != rdclass &&
		    ent->rdclass != dns_rdataclass_any)
			continue;
		if (match(name, dns_fixedname_name(&ent->name)))
			return (ent->mode);
	}
	return (0);
}

void
dns_order_attach(dns_order_t *source, dns_order_t **target) {
	REQUIRE(DNS_ORDER_VALID(source));
	REQUIRE(target != NULL && *target == NULL);
	isc_refcount_increment(&source->references, NULL);
	*target = source;
}

void
dns_order_detach(dns_order_t **orderp) {
	dns_order_t *order;
	dns_order_ent_t *ent;
	unsigned int references;

	REQUIRE(orderp != NULL);
	order = *orderp;
	REQUIRE(DNS_ORDER_VALID(order));
	isc_refcount_decrement(&order->references, &references);
	*orderp = NULL;
	if (references != 0)
		return;

	order->magic = 0;
	while ((ent = ISC_LIST_HEAD(order->ents)) != NULL) {
		ISC_LIST_UNLINK(order->ents, ent, link);
		isc_mem_put(order->mctx, ent, sizeof(*ent));
	}
	isc_refcount_destroy(&order->references);
	isc_mem_putanddetach(&order->mctx, order, sizeof(*order));
}
