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

/* $Id: forward.c,v 1.12 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/rwlock.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

#include <dns/forward.h>
#include <dns/rbt.h>
#include <dns/result.h>
#include <dns/types.h>

struct dns_fwdtable {
	/* Unlocked. */
	unsigned int		magic;
	isc_mem_t		*mctx;
	isc_rwlock_t		rwlock;
	/* Locked by lock. */
	dns_rbt_t		*table;
};

#define FWDTABLEMAGIC		ISC_MAGIC('F', 'w', 'd', 'T')
#define VALID_FWDTABLE(ft) 	ISC_MAGIC_VALID(ft, FWDTABLEMAGIC)

static void
auto_detach(void *, void *);

isc_result_t
dns_fwdtable_create(isc_mem_t *mctx, dns_fwdtable_t **fwdtablep) {
	dns_fwdtable_t *fwdtable;
	isc_result_t result;

	REQUIRE(fwdtablep != NULL && *fwdtablep == NULL);

	fwdtable = isc_mem_get(mctx, sizeof(dns_fwdtable_t));
	if (fwdtable == NULL)
		return (ISC_R_NOMEMORY);

	fwdtable->table = NULL;
	result = dns_rbt_create(mctx, auto_detach, fwdtable, &fwdtable->table);
	if (result != ISC_R_SUCCESS)
		goto cleanup_fwdtable;

	result = isc_rwlock_init(&fwdtable->rwlock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_rbt;

	fwdtable->mctx = NULL;
	isc_mem_attach(mctx, &fwdtable->mctx);
	fwdtable->magic = FWDTABLEMAGIC;
	*fwdtablep = fwdtable;

	return (ISC_R_SUCCESS);

   cleanup_rbt:
	dns_rbt_destroy(&fwdtable->table);

   cleanup_fwdtable:
	isc_mem_put(mctx, fwdtable, sizeof(dns_fwdtable_t));

	return (result);
}

isc_result_t
dns_fwdtable_add(dns_fwdtable_t *fwdtable, dns_name_t *name,
		 isc_sockaddrlist_t *addrs, dns_fwdpolicy_t fwdpolicy)
{
	isc_result_t result;
	dns_forwarders_t *forwarders;
	isc_sockaddr_t *sa, *nsa;

	REQUIRE(VALID_FWDTABLE(fwdtable));

	forwarders = isc_mem_get(fwdtable->mctx, sizeof(dns_forwarders_t));
	if (forwarders == NULL)
		return (ISC_R_NOMEMORY);

	ISC_LIST_INIT(forwarders->addrs);
	for (sa = ISC_LIST_HEAD(*addrs);
	     sa != NULL;
	     sa = ISC_LIST_NEXT(sa, link))
	{
		nsa = isc_mem_get(fwdtable->mctx, sizeof(isc_sockaddr_t));
		if (nsa == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		*nsa = *sa;
		ISC_LINK_INIT(nsa, link);
		ISC_LIST_APPEND(forwarders->addrs, nsa, link);
	}
	forwarders->fwdpolicy = fwdpolicy;

	RWLOCK(&fwdtable->rwlock, isc_rwlocktype_write);
	result = dns_rbt_addname(fwdtable->table, name, forwarders);
	RWUNLOCK(&fwdtable->rwlock, isc_rwlocktype_write);

	if (result != ISC_R_SUCCESS)
		goto cleanup;

	return (ISC_R_SUCCESS);

 cleanup:
	while (!ISC_LIST_EMPTY(forwarders->addrs)) {
		sa = ISC_LIST_HEAD(forwarders->addrs);
		ISC_LIST_UNLINK(forwarders->addrs, sa, link);
		isc_mem_put(fwdtable->mctx, sa, sizeof(isc_sockaddr_t));
	}
	isc_mem_put(fwdtable->mctx, forwarders, sizeof(dns_forwarders_t));
	return (result);
}

isc_result_t
dns_fwdtable_find(dns_fwdtable_t *fwdtable, dns_name_t *name,
		  dns_forwarders_t **forwardersp)
{
	return (dns_fwdtable_find2(fwdtable, name, NULL, forwardersp));
} 

isc_result_t
dns_fwdtable_find2(dns_fwdtable_t *fwdtable, dns_name_t *name,
		   dns_name_t *foundname, dns_forwarders_t **forwardersp)
{
	isc_result_t result;

	REQUIRE(VALID_FWDTABLE(fwdtable));

	RWLOCK(&fwdtable->rwlock, isc_rwlocktype_read);

	result = dns_rbt_findname(fwdtable->table, name, 0, foundname,
				  (void **)forwardersp);
	if (result == DNS_R_PARTIALMATCH)
		result = ISC_R_SUCCESS;

	RWUNLOCK(&fwdtable->rwlock, isc_rwlocktype_read);

	return (result);
}

void
dns_fwdtable_destroy(dns_fwdtable_t **fwdtablep) {
	dns_fwdtable_t *fwdtable;
	isc_mem_t *mctx;

	REQUIRE(fwdtablep != NULL && VALID_FWDTABLE(*fwdtablep));

	fwdtable = *fwdtablep;

	dns_rbt_destroy(&fwdtable->table);
	isc_rwlock_destroy(&fwdtable->rwlock);
	fwdtable->magic = 0;
	mctx = fwdtable->mctx;
	isc_mem_put(mctx, fwdtable, sizeof(dns_fwdtable_t));
	isc_mem_detach(&mctx);

	*fwdtablep = NULL;
}

/***
 *** Private
 ***/

static void
auto_detach(void *data, void *arg) {
	dns_forwarders_t *forwarders = data;
	dns_fwdtable_t *fwdtable = arg;
	isc_sockaddr_t *sa;

	UNUSED(arg);

	while (!ISC_LIST_EMPTY(forwarders->addrs)) {
		sa = ISC_LIST_HEAD(forwarders->addrs);
		ISC_LIST_UNLINK(forwarders->addrs, sa, link);
		isc_mem_put(fwdtable->mctx, sa, sizeof(isc_sockaddr_t));
	}
	isc_mem_put(fwdtable->mctx, forwarders, sizeof(dns_forwarders_t));
}
