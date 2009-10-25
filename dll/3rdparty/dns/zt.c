/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2002  Internet Software Consortium.
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

/* $Id: zt.c,v 1.47 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/file.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/log.h>
#include <dns/name.h>
#include <dns/rbt.h>
#include <dns/rdataclass.h>
#include <dns/result.h>
#include <dns/view.h>
#include <dns/zone.h>
#include <dns/zt.h>

struct dns_zt {
	/* Unlocked. */
	unsigned int		magic;
	isc_mem_t		*mctx;
	dns_rdataclass_t	rdclass;
	isc_rwlock_t		rwlock;
	/* Locked by lock. */
	isc_uint32_t		references;
	dns_rbt_t		*table;
};

#define ZTMAGIC			ISC_MAGIC('Z', 'T', 'b', 'l')
#define VALID_ZT(zt) 		ISC_MAGIC_VALID(zt, ZTMAGIC)

static void
auto_detach(void *, void *);

static isc_result_t
load(dns_zone_t *zone, void *uap);

static isc_result_t
loadnew(dns_zone_t *zone, void *uap);

static isc_result_t
freezezones(dns_zone_t *zone, void *uap);

isc_result_t
dns_zt_create(isc_mem_t *mctx, dns_rdataclass_t rdclass, dns_zt_t **ztp)
{
	dns_zt_t *zt;
	isc_result_t result;

	REQUIRE(ztp != NULL && *ztp == NULL);

	zt = isc_mem_get(mctx, sizeof(*zt));
	if (zt == NULL)
		return (ISC_R_NOMEMORY);

	zt->table = NULL;
	result = dns_rbt_create(mctx, auto_detach, zt, &zt->table);
	if (result != ISC_R_SUCCESS)
		goto cleanup_zt;

	result = isc_rwlock_init(&zt->rwlock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_rbt;

	zt->mctx = mctx;
	zt->references = 1;
	zt->rdclass = rdclass;
	zt->magic = ZTMAGIC;
	*ztp = zt;

	return (ISC_R_SUCCESS);

   cleanup_rbt:
	dns_rbt_destroy(&zt->table);

   cleanup_zt:
	isc_mem_put(mctx, zt, sizeof(*zt));

	return (result);
}

isc_result_t
dns_zt_mount(dns_zt_t *zt, dns_zone_t *zone) {
	isc_result_t result;
	dns_zone_t *dummy = NULL;
	dns_name_t *name;

	REQUIRE(VALID_ZT(zt));

	name = dns_zone_getorigin(zone);

	RWLOCK(&zt->rwlock, isc_rwlocktype_write);

	result = dns_rbt_addname(zt->table, name, zone);
	if (result == ISC_R_SUCCESS)
		dns_zone_attach(zone, &dummy);

	RWUNLOCK(&zt->rwlock, isc_rwlocktype_write);

	return (result);
}

isc_result_t
dns_zt_unmount(dns_zt_t *zt, dns_zone_t *zone) {
	isc_result_t result;
	dns_name_t *name;

	REQUIRE(VALID_ZT(zt));

	name = dns_zone_getorigin(zone);

	RWLOCK(&zt->rwlock, isc_rwlocktype_write);

	result = dns_rbt_deletename(zt->table, name, ISC_FALSE);

	RWUNLOCK(&zt->rwlock, isc_rwlocktype_write);

	return (result);
}

isc_result_t
dns_zt_find(dns_zt_t *zt, dns_name_t *name, unsigned int options,
	    dns_name_t *foundname, dns_zone_t **zonep)
{
	isc_result_t result;
	dns_zone_t *dummy = NULL;
	unsigned int rbtoptions = 0;

	REQUIRE(VALID_ZT(zt));

	if ((options & DNS_ZTFIND_NOEXACT) != 0)
		rbtoptions |= DNS_RBTFIND_NOEXACT;

	RWLOCK(&zt->rwlock, isc_rwlocktype_read);

	result = dns_rbt_findname(zt->table, name, rbtoptions, foundname,
				  (void **) (void*)&dummy);
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH)
		dns_zone_attach(dummy, zonep);

	RWUNLOCK(&zt->rwlock, isc_rwlocktype_read);

	return (result);
}

void
dns_zt_attach(dns_zt_t *zt, dns_zt_t **ztp) {

	REQUIRE(VALID_ZT(zt));
	REQUIRE(ztp != NULL && *ztp == NULL);

	RWLOCK(&zt->rwlock, isc_rwlocktype_write);

	INSIST(zt->references > 0);
	zt->references++;
	INSIST(zt->references != 0);

	RWUNLOCK(&zt->rwlock, isc_rwlocktype_write);

	*ztp = zt;
}

static isc_result_t
flush(dns_zone_t *zone, void *uap) {
	UNUSED(uap);
	return (dns_zone_flush(zone));
}

static void
zt_flushanddetach(dns_zt_t **ztp, isc_boolean_t need_flush) {
	isc_boolean_t destroy = ISC_FALSE;
	dns_zt_t *zt;

	REQUIRE(ztp != NULL && VALID_ZT(*ztp));

	zt = *ztp;

	RWLOCK(&zt->rwlock, isc_rwlocktype_write);

	INSIST(zt->references > 0);
	zt->references--;
	if (zt->references == 0)
		destroy = ISC_TRUE;

	RWUNLOCK(&zt->rwlock, isc_rwlocktype_write);

	if (destroy) {
		if (need_flush)
			(void)dns_zt_apply(zt, ISC_FALSE, flush, NULL);
		dns_rbt_destroy(&zt->table);
		isc_rwlock_destroy(&zt->rwlock);
		zt->magic = 0;
		isc_mem_put(zt->mctx, zt, sizeof(*zt));
	}

	*ztp = NULL;
}

void
dns_zt_flushanddetach(dns_zt_t **ztp) {
	zt_flushanddetach(ztp, ISC_TRUE);
}

void
dns_zt_detach(dns_zt_t **ztp) {
	zt_flushanddetach(ztp, ISC_FALSE);
}

isc_result_t
dns_zt_load(dns_zt_t *zt, isc_boolean_t stop) {
	isc_result_t result;

	REQUIRE(VALID_ZT(zt));

	RWLOCK(&zt->rwlock, isc_rwlocktype_read);
	result = dns_zt_apply(zt, stop, load, NULL);
	RWUNLOCK(&zt->rwlock, isc_rwlocktype_read);
	return (result);
}

static isc_result_t
load(dns_zone_t *zone, void *uap) {
	isc_result_t result;
	UNUSED(uap);
	result = dns_zone_load(zone);
	if (result == DNS_R_CONTINUE || result == DNS_R_UPTODATE)
		result = ISC_R_SUCCESS;
	return (result);
}

isc_result_t
dns_zt_loadnew(dns_zt_t *zt, isc_boolean_t stop) {
	isc_result_t result;

	REQUIRE(VALID_ZT(zt));

	RWLOCK(&zt->rwlock, isc_rwlocktype_read);
	result = dns_zt_apply(zt, stop, loadnew, NULL);
	RWUNLOCK(&zt->rwlock, isc_rwlocktype_read);
	return (result);
}

static isc_result_t
loadnew(dns_zone_t *zone, void *uap) {
	isc_result_t result;
	UNUSED(uap);
	result = dns_zone_loadnew(zone);
	if (result == DNS_R_CONTINUE || result == DNS_R_UPTODATE ||
	    result == DNS_R_DYNAMIC)
		result = ISC_R_SUCCESS;
	return (result);
}

isc_result_t
dns_zt_freezezones(dns_zt_t *zt, isc_boolean_t freeze) {
	isc_result_t result, tresult;

	REQUIRE(VALID_ZT(zt));

	RWLOCK(&zt->rwlock, isc_rwlocktype_read);
	result = dns_zt_apply2(zt, ISC_FALSE, &tresult, freezezones, &freeze);
	RWUNLOCK(&zt->rwlock, isc_rwlocktype_read);
	return ((result == ISC_R_SUCCESS) ? tresult : result);
}

static isc_result_t
freezezones(dns_zone_t *zone, void *uap) {
	isc_boolean_t freeze = *(isc_boolean_t *)uap;
	isc_boolean_t frozen;
	isc_result_t result = ISC_R_SUCCESS;
	char classstr[DNS_RDATACLASS_FORMATSIZE];
	char zonename[DNS_NAME_FORMATSIZE];
	dns_view_t *view;
	char *journal;
	const char *vname;
	const char *sep;
	int level;

	if (dns_zone_gettype(zone) != dns_zone_master)
		return (ISC_R_SUCCESS);

	frozen = dns_zone_getupdatedisabled(zone);
	if (freeze) {
		if (frozen)
			result = DNS_R_FROZEN;
		if (result == ISC_R_SUCCESS)
			result = dns_zone_flush(zone);
		if (result == ISC_R_SUCCESS) {
			journal = dns_zone_getjournal(zone);
			if (journal != NULL)
				(void)isc_file_remove(journal);
		}
	} else {
		if (frozen) {
			result = dns_zone_load(zone);
			if (result == DNS_R_CONTINUE ||
			    result == DNS_R_UPTODATE)
				result = ISC_R_SUCCESS;
		}
	}
	if (result == ISC_R_SUCCESS)
		dns_zone_setupdatedisabled(zone, freeze);
	view = dns_zone_getview(zone);
	if (strcmp(view->name, "_bind") == 0 ||
	    strcmp(view->name, "_default") == 0)
	{
		vname = "";
		sep = "";
	} else {
		vname = view->name;
		sep = " ";
	}
	dns_rdataclass_format(dns_zone_getclass(zone), classstr,
			      sizeof(classstr));
	dns_name_format(dns_zone_getorigin(zone), zonename, sizeof(zonename));
	level = (result != ISC_R_SUCCESS) ? ISC_LOG_ERROR : ISC_LOG_DEBUG(1);
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_ZONE,
		      level, "%s zone '%s/%s'%s%s: %s",
		      freeze ? "freezing" : "thawing",
		      zonename, classstr, sep, vname,
		      isc_result_totext(result));
	return (result);
}

isc_result_t
dns_zt_apply(dns_zt_t *zt, isc_boolean_t stop,
	     isc_result_t (*action)(dns_zone_t *, void *), void *uap)
{
	return (dns_zt_apply2(zt, stop, NULL, action, uap));
}

isc_result_t
dns_zt_apply2(dns_zt_t *zt, isc_boolean_t stop, isc_result_t *sub,
	      isc_result_t (*action)(dns_zone_t *, void *), void *uap)
{
	dns_rbtnode_t *node;
	dns_rbtnodechain_t chain;
	isc_result_t result, tresult = ISC_R_SUCCESS;
	dns_zone_t *zone;

	REQUIRE(VALID_ZT(zt));
	REQUIRE(action != NULL);

	dns_rbtnodechain_init(&chain, zt->mctx);
	result = dns_rbtnodechain_first(&chain, zt->table, NULL, NULL);
	if (result == ISC_R_NOTFOUND) {
		/*
		 * The tree is empty.
		 */
		result = ISC_R_NOMORE;
	}
	while (result == DNS_R_NEWORIGIN || result == ISC_R_SUCCESS) {
		result = dns_rbtnodechain_current(&chain, NULL, NULL,
						  &node);
		if (result == ISC_R_SUCCESS) {
			zone = node->data;
			if (zone != NULL)
				result = (action)(zone, uap);
			if (result != ISC_R_SUCCESS && stop) {
				tresult = result;
				goto cleanup;	/* don't break */
			} else if (result != ISC_R_SUCCESS &&
				   tresult == ISC_R_SUCCESS)
				tresult = result;
		}
		result = dns_rbtnodechain_next(&chain, NULL, NULL);
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

 cleanup:
	dns_rbtnodechain_invalidate(&chain);
	if (sub != NULL)
		*sub = tresult;

	return (result);
}

/***
 *** Private
 ***/

static void
auto_detach(void *data, void *arg) {
	dns_zone_t *zone = data;

	UNUSED(arg);

	dns_zone_detach(&zone);
}
