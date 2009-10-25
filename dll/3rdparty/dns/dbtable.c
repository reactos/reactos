/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
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

/*
 * $Id: dbtable.c,v 1.33 2007/06/19 23:47:16 tbox Exp $
 */

/*! \file
 * \author
 * Principal Author: DCL
 */

#include <config.h>

#include <isc/mem.h>
#include <isc/rwlock.h>
#include <isc/util.h>

#include <dns/dbtable.h>
#include <dns/db.h>
#include <dns/rbt.h>
#include <dns/result.h>

struct dns_dbtable {
	/* Unlocked. */
	unsigned int		magic;
	isc_mem_t *		mctx;
	dns_rdataclass_t	rdclass;
	isc_mutex_t		lock;
	isc_rwlock_t		tree_lock;
	/* Locked by lock. */
	unsigned int		references;
	/* Locked by tree_lock. */
	dns_rbt_t *		rbt;
	dns_db_t *		default_db;
};

#define DBTABLE_MAGIC		ISC_MAGIC('D', 'B', '-', '-')
#define VALID_DBTABLE(dbtable)	ISC_MAGIC_VALID(dbtable, DBTABLE_MAGIC)

static void
dbdetach(void *data, void *arg) {
	dns_db_t *db = data;

	UNUSED(arg);

	dns_db_detach(&db);
}

isc_result_t
dns_dbtable_create(isc_mem_t *mctx, dns_rdataclass_t rdclass,
		   dns_dbtable_t **dbtablep)
{
	dns_dbtable_t *dbtable;
	isc_result_t result;

	REQUIRE(mctx != NULL);
	REQUIRE(dbtablep != NULL && *dbtablep == NULL);

	dbtable = (dns_dbtable_t *)isc_mem_get(mctx, sizeof(*dbtable));
	if (dbtable == NULL)
		return (ISC_R_NOMEMORY);

	dbtable->rbt = NULL;
	result = dns_rbt_create(mctx, dbdetach, NULL, &dbtable->rbt);
	if (result != ISC_R_SUCCESS)
		goto clean1;

	result = isc_mutex_init(&dbtable->lock);
	if (result != ISC_R_SUCCESS)
		goto clean2;

	result = isc_rwlock_init(&dbtable->tree_lock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto clean3;

	dbtable->default_db = NULL;
	dbtable->mctx = mctx;
	dbtable->rdclass = rdclass;
	dbtable->magic = DBTABLE_MAGIC;
	dbtable->references = 1;

	*dbtablep = dbtable;

	return (ISC_R_SUCCESS);

 clean3:
	DESTROYLOCK(&dbtable->lock);

 clean2:
	dns_rbt_destroy(&dbtable->rbt);

 clean1:
	isc_mem_put(mctx, dbtable, sizeof(*dbtable));

	return (result);
}

static inline void
dbtable_free(dns_dbtable_t *dbtable) {
	/*
	 * Caller must ensure that it is safe to call.
	 */

	RWLOCK(&dbtable->tree_lock, isc_rwlocktype_write);

	if (dbtable->default_db != NULL)
		dns_db_detach(&dbtable->default_db);

	dns_rbt_destroy(&dbtable->rbt);

	RWUNLOCK(&dbtable->tree_lock, isc_rwlocktype_write);

	isc_rwlock_destroy(&dbtable->tree_lock);

	dbtable->magic = 0;

	isc_mem_put(dbtable->mctx, dbtable, sizeof(*dbtable));
}

void
dns_dbtable_attach(dns_dbtable_t *source, dns_dbtable_t **targetp) {
	REQUIRE(VALID_DBTABLE(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	LOCK(&source->lock);

	INSIST(source->references > 0);
	source->references++;
	INSIST(source->references != 0);

	UNLOCK(&source->lock);

	*targetp = source;
}

void
dns_dbtable_detach(dns_dbtable_t **dbtablep) {
	dns_dbtable_t *dbtable;
	isc_boolean_t free_dbtable = ISC_FALSE;

	REQUIRE(dbtablep != NULL);
	dbtable = *dbtablep;
	REQUIRE(VALID_DBTABLE(dbtable));

	LOCK(&dbtable->lock);

	INSIST(dbtable->references > 0);
	dbtable->references--;
	if (dbtable->references == 0)
		free_dbtable = ISC_TRUE;

	UNLOCK(&dbtable->lock);

	if (free_dbtable)
		dbtable_free(dbtable);

	*dbtablep = NULL;
}

isc_result_t
dns_dbtable_add(dns_dbtable_t *dbtable, dns_db_t *db) {
	isc_result_t result;
	dns_db_t *clone;

	REQUIRE(VALID_DBTABLE(dbtable));
	REQUIRE(dns_db_class(db) == dbtable->rdclass);

	clone = NULL;
	dns_db_attach(db, &clone);

	RWLOCK(&dbtable->tree_lock, isc_rwlocktype_write);
	result = dns_rbt_addname(dbtable->rbt, dns_db_origin(clone), clone);
	RWUNLOCK(&dbtable->tree_lock, isc_rwlocktype_write);

	return (result);
}

void
dns_dbtable_remove(dns_dbtable_t *dbtable, dns_db_t *db) {
	dns_db_t *stored_data = NULL;
	isc_result_t result;
	dns_name_t *name;

	REQUIRE(VALID_DBTABLE(dbtable));

	name = dns_db_origin(db);

	/*
	 * There is a requirement that the association of name with db
	 * be verified.  With the current rbt.c this is expensive to do,
	 * because effectively two find operations are being done, but
	 * deletion is relatively infrequent.
	 * XXXDCL ... this could be cheaper now with dns_rbt_deletenode.
	 */

	RWLOCK(&dbtable->tree_lock, isc_rwlocktype_write);

	result = dns_rbt_findname(dbtable->rbt, name, 0, NULL,
				  (void **) (void *)&stored_data);

	if (result == ISC_R_SUCCESS) {
		INSIST(stored_data == db);

		(void)dns_rbt_deletename(dbtable->rbt, name, ISC_FALSE);
	}

	RWUNLOCK(&dbtable->tree_lock, isc_rwlocktype_write);
}

void
dns_dbtable_adddefault(dns_dbtable_t *dbtable, dns_db_t *db) {
	REQUIRE(VALID_DBTABLE(dbtable));
	REQUIRE(dbtable->default_db == NULL);
	REQUIRE(dns_name_compare(dns_db_origin(db), dns_rootname) == 0);

	RWLOCK(&dbtable->tree_lock, isc_rwlocktype_write);

	dbtable->default_db = NULL;
	dns_db_attach(db, &dbtable->default_db);

	RWUNLOCK(&dbtable->tree_lock, isc_rwlocktype_write);
}

void
dns_dbtable_getdefault(dns_dbtable_t *dbtable, dns_db_t **dbp) {
	REQUIRE(VALID_DBTABLE(dbtable));
	REQUIRE(dbp != NULL && *dbp == NULL);

	RWLOCK(&dbtable->tree_lock, isc_rwlocktype_read);

	dns_db_attach(dbtable->default_db, dbp);

	RWUNLOCK(&dbtable->tree_lock, isc_rwlocktype_read);
}

void
dns_dbtable_removedefault(dns_dbtable_t *dbtable) {
	REQUIRE(VALID_DBTABLE(dbtable));

	RWLOCK(&dbtable->tree_lock, isc_rwlocktype_write);

	dns_db_detach(&dbtable->default_db);

	RWUNLOCK(&dbtable->tree_lock, isc_rwlocktype_write);
}

isc_result_t
dns_dbtable_find(dns_dbtable_t *dbtable, dns_name_t *name,
		 unsigned int options, dns_db_t **dbp)
{
	dns_db_t *stored_data = NULL;
	isc_result_t result;
	unsigned int rbtoptions = 0;

	REQUIRE(dbp != NULL && *dbp == NULL);

	if ((options & DNS_DBTABLEFIND_NOEXACT) != 0)
		rbtoptions |= DNS_RBTFIND_NOEXACT;

	RWLOCK(&dbtable->tree_lock, isc_rwlocktype_read);

	result = dns_rbt_findname(dbtable->rbt, name, rbtoptions, NULL,
				  (void **) (void *)&stored_data);

	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH)
		dns_db_attach(stored_data, dbp);
	else if (dbtable->default_db != NULL) {
		dns_db_attach(dbtable->default_db, dbp);
		result = DNS_R_PARTIALMATCH;
	} else
		result = ISC_R_NOTFOUND;

	RWUNLOCK(&dbtable->tree_lock, isc_rwlocktype_read);

	return (result);
}
