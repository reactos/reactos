/*
 * Portions Copyright (C) 2005-2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1999-2001  Internet Software Consortium.
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
 * Copyright (C) 2002 Stichting NLnet, Netherlands, stichting@nlnet.nl.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND STICHTING NLNET
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * STICHTING NLNET BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * The development of Dynamically Loadable Zones (DLZ) for Bind 9 was
 * conceived and contributed by Rob Butler.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ROB BUTLER
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * ROB BUTLER BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: sdlz.c,v 1.18.50.2 2009/04/21 23:47:18 tbox Exp $ */

/*! \file */

#include <config.h>
#include <string.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/log.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/util.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/print.h>
#include <isc/region.h>

#include <dns/callbacks.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/dlz.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/master.h>
#include <dns/sdlz.h>
#include <dns/types.h>

#include "rdatalist_p.h"

/*
 * Private Types
 */

struct dns_sdlzimplementation {
	const dns_sdlzmethods_t		*methods;
	isc_mem_t			*mctx;
	void				*driverarg;
	unsigned int			flags;
	isc_mutex_t			driverlock;
	dns_dlzimplementation_t		*dlz_imp;
};

struct dns_sdlz_db {
	/* Unlocked */
	dns_db_t			common;
	void				*dbdata;
	dns_sdlzimplementation_t	*dlzimp;
	isc_mutex_t			refcnt_lock;
	/* Locked */
	unsigned int			references;
};

struct dns_sdlzlookup {
	/* Unlocked */
	unsigned int			magic;
	dns_sdlz_db_t			*sdlz;
	ISC_LIST(dns_rdatalist_t)	lists;
	ISC_LIST(isc_buffer_t)		buffers;
	dns_name_t			*name;
	ISC_LINK(dns_sdlzlookup_t)	link;
	isc_mutex_t			lock;
	dns_rdatacallbacks_t		callbacks;
	/* Locked */
	unsigned int			references;
};

typedef struct dns_sdlzlookup dns_sdlznode_t;

struct dns_sdlzallnodes {
	dns_dbiterator_t		common;
	ISC_LIST(dns_sdlznode_t)	nodelist;
	dns_sdlznode_t			*current;
	dns_sdlznode_t			*origin;
};

typedef dns_sdlzallnodes_t sdlz_dbiterator_t;

typedef struct sdlz_rdatasetiter {
	dns_rdatasetiter_t		common;
	dns_rdatalist_t			*current;
} sdlz_rdatasetiter_t;


#define SDLZDB_MAGIC		ISC_MAGIC('D', 'L', 'Z', 'S')

/*
 * Note that "impmagic" is not the first four bytes of the struct, so
 * ISC_MAGIC_VALID cannot be used.
 */

#define VALID_SDLZDB(sdlzdb)	((sdlzdb) != NULL && \
				 (sdlzdb)->common.impmagic == SDLZDB_MAGIC)

#define SDLZLOOKUP_MAGIC	ISC_MAGIC('D','L','Z','L')
#define VALID_SDLZLOOKUP(sdlzl)	ISC_MAGIC_VALID(sdlzl, SDLZLOOKUP_MAGIC)
#define VALID_SDLZNODE(sdlzn)	VALID_SDLZLOOKUP(sdlzn)

/* These values are taken from RFC 1537 */
#define SDLZ_DEFAULT_REFRESH	(60 * 60 * 8)
#define SDLZ_DEFAULT_RETRY	(60 * 60 * 2)
#define SDLZ_DEFAULT_EXPIRE	(60 * 60 * 24 * 7)
#define SDLZ_DEFAULT_MINIMUM	(60 * 60 * 24)

/* This is a reasonable value */
#define SDLZ_DEFAULT_TTL	(60 * 60 * 24)

static int dummy;

#ifdef __COVERITY__
#define MAYBE_LOCK(imp) LOCK(&imp->driverlock)
#define MAYBE_UNLOCK(imp) UNLOCK(&imp->driverlock)
#else
#define MAYBE_LOCK(imp) \
	do { \
		unsigned int flags = imp->flags; \
		if ((flags & DNS_SDLZFLAG_THREADSAFE) == 0) \
			LOCK(&imp->driverlock); \
	} while (0)

#define MAYBE_UNLOCK(imp) \
	do { \
		unsigned int flags = imp->flags; \
		if ((flags & DNS_SDLZFLAG_THREADSAFE) == 0) \
			UNLOCK(&imp->driverlock); \
	} while (0)
#endif

/*
 * Forward references.  Try to keep these to a minimum.
 */

static void list_tordataset(dns_rdatalist_t *rdatalist,
			    dns_db_t *db, dns_dbnode_t *node,
			    dns_rdataset_t *rdataset);

static void detachnode(dns_db_t *db, dns_dbnode_t **targetp);

static void		dbiterator_destroy(dns_dbiterator_t **iteratorp);
static isc_result_t	dbiterator_first(dns_dbiterator_t *iterator);
static isc_result_t	dbiterator_last(dns_dbiterator_t *iterator);
static isc_result_t	dbiterator_seek(dns_dbiterator_t *iterator,
					dns_name_t *name);
static isc_result_t	dbiterator_prev(dns_dbiterator_t *iterator);
static isc_result_t	dbiterator_next(dns_dbiterator_t *iterator);
static isc_result_t	dbiterator_current(dns_dbiterator_t *iterator,
					   dns_dbnode_t **nodep,
					   dns_name_t *name);
static isc_result_t	dbiterator_pause(dns_dbiterator_t *iterator);
static isc_result_t	dbiterator_origin(dns_dbiterator_t *iterator,
					  dns_name_t *name);

static dns_dbiteratormethods_t dbiterator_methods = {
	dbiterator_destroy,
	dbiterator_first,
	dbiterator_last,
	dbiterator_seek,
	dbiterator_prev,
	dbiterator_next,
	dbiterator_current,
	dbiterator_pause,
	dbiterator_origin
};

/*
 * Utility functions
 */

/*% Converts the input string to lowercase, in place. */

static void
dns_sdlz_tolower(char *str) {

	unsigned int len = strlen(str);
	unsigned int i;

	for (i = 0; i < len; i++) {
		if (str[i] >= 'A' && str[i] <= 'Z')
			str[i] += 32;
	}

}

static inline unsigned int
initial_size(const char *data) {
	unsigned int len = (strlen(data) / 64) + 1;
	return (len * 64 + 64);
}

/*
 * Rdataset Iterator Methods. These methods were "borrowed" from the SDB
 * driver interface.  See the SDB driver interface documentation for more info.
 */

static void
rdatasetiter_destroy(dns_rdatasetiter_t **iteratorp) {
	sdlz_rdatasetiter_t *sdlziterator =
		(sdlz_rdatasetiter_t *)(*iteratorp);

	detachnode(sdlziterator->common.db, &sdlziterator->common.node);
	isc_mem_put(sdlziterator->common.db->mctx, sdlziterator,
		    sizeof(sdlz_rdatasetiter_t));
	*iteratorp = NULL;
}

static isc_result_t
rdatasetiter_first(dns_rdatasetiter_t *iterator) {
	sdlz_rdatasetiter_t *sdlziterator = (sdlz_rdatasetiter_t *)iterator;
	dns_sdlznode_t *sdlznode = (dns_sdlznode_t *)iterator->node;

	if (ISC_LIST_EMPTY(sdlznode->lists))
		return (ISC_R_NOMORE);
	sdlziterator->current = ISC_LIST_HEAD(sdlznode->lists);
	return (ISC_R_SUCCESS);
}

static isc_result_t
rdatasetiter_next(dns_rdatasetiter_t *iterator) {
	sdlz_rdatasetiter_t *sdlziterator = (sdlz_rdatasetiter_t *)iterator;

	sdlziterator->current = ISC_LIST_NEXT(sdlziterator->current, link);
	if (sdlziterator->current == NULL)
		return (ISC_R_NOMORE);
	else
		return (ISC_R_SUCCESS);
}

static void
rdatasetiter_current(dns_rdatasetiter_t *iterator, dns_rdataset_t *rdataset) {
	sdlz_rdatasetiter_t *sdlziterator = (sdlz_rdatasetiter_t *)iterator;

	list_tordataset(sdlziterator->current, iterator->db, iterator->node,
			rdataset);
}

static dns_rdatasetitermethods_t rdatasetiter_methods = {
	rdatasetiter_destroy,
	rdatasetiter_first,
	rdatasetiter_next,
	rdatasetiter_current
};

/*
 * DB routines. These methods were "borrowed" from the SDB driver interface.
 * See the SDB driver interface documentation for more info.
 */

static void
attach(dns_db_t *source, dns_db_t **targetp) {
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *) source;

	REQUIRE(VALID_SDLZDB(sdlz));

	LOCK(&sdlz->refcnt_lock);
	REQUIRE(sdlz->references > 0);
	sdlz->references++;
	UNLOCK(&sdlz->refcnt_lock);

	*targetp = source;
}

static void
destroy(dns_sdlz_db_t *sdlz) {
	isc_mem_t *mctx;
	mctx = sdlz->common.mctx;

	sdlz->common.magic = 0;
	sdlz->common.impmagic = 0;

	isc_mutex_destroy(&sdlz->refcnt_lock);

	dns_name_free(&sdlz->common.origin, mctx);

	isc_mem_put(mctx, sdlz, sizeof(dns_sdlz_db_t));
	isc_mem_detach(&mctx);
}

static void
detach(dns_db_t **dbp) {
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *)(*dbp);
	isc_boolean_t need_destroy = ISC_FALSE;

	REQUIRE(VALID_SDLZDB(sdlz));
	LOCK(&sdlz->refcnt_lock);
	REQUIRE(sdlz->references > 0);
	sdlz->references--;
	if (sdlz->references == 0)
		need_destroy = ISC_TRUE;
	UNLOCK(&sdlz->refcnt_lock);

	if (need_destroy)
		destroy(sdlz);

	*dbp = NULL;
}

static isc_result_t
beginload(dns_db_t *db, dns_addrdatasetfunc_t *addp, dns_dbload_t **dbloadp) {
	UNUSED(db);
	UNUSED(addp);
	UNUSED(dbloadp);
	return (ISC_R_NOTIMPLEMENTED);
}

static isc_result_t
endload(dns_db_t *db, dns_dbload_t **dbloadp) {
	UNUSED(db);
	UNUSED(dbloadp);
	return (ISC_R_NOTIMPLEMENTED);
}

static isc_result_t
dump(dns_db_t *db, dns_dbversion_t *version, const char *filename,
     dns_masterformat_t masterformat)
{
	UNUSED(db);
	UNUSED(version);
	UNUSED(filename);
	UNUSED(masterformat);
	return (ISC_R_NOTIMPLEMENTED);
}

static void
currentversion(dns_db_t *db, dns_dbversion_t **versionp) {
	REQUIRE(versionp != NULL && *versionp == NULL);

	UNUSED(db);

	*versionp = (void *) &dummy;
	return;
}

static isc_result_t
newversion(dns_db_t *db, dns_dbversion_t **versionp) {
	UNUSED(db);
	UNUSED(versionp);

	return (ISC_R_NOTIMPLEMENTED);
}

static void
attachversion(dns_db_t *db, dns_dbversion_t *source,
	      dns_dbversion_t **targetp)
{
	REQUIRE(source != NULL && source == (void *) &dummy);

	UNUSED(db);
	UNUSED(source);
	UNUSED(targetp);
	*targetp = source;
}

static void
closeversion(dns_db_t *db, dns_dbversion_t **versionp, isc_boolean_t commit) {
	REQUIRE(versionp != NULL && *versionp == (void *) &dummy);
	REQUIRE(commit == ISC_FALSE);

	UNUSED(db);
	UNUSED(commit);

	*versionp = NULL;
}

static isc_result_t
createnode(dns_sdlz_db_t *sdlz, dns_sdlznode_t **nodep) {
	dns_sdlznode_t *node;
	isc_result_t result;

	node = isc_mem_get(sdlz->common.mctx, sizeof(dns_sdlznode_t));
	if (node == NULL)
		return (ISC_R_NOMEMORY);

	node->sdlz = NULL;
	attach((dns_db_t *)sdlz, (dns_db_t **)&node->sdlz);
	ISC_LIST_INIT(node->lists);
	ISC_LIST_INIT(node->buffers);
	ISC_LINK_INIT(node, link);
	node->name = NULL;
	result = isc_mutex_init(&node->lock);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_mutex_init() failed: %s",
				 isc_result_totext(result));
		isc_mem_put(sdlz->common.mctx, node, sizeof(dns_sdlznode_t));
		return (ISC_R_UNEXPECTED);
	}
	dns_rdatacallbacks_init(&node->callbacks);
	node->references = 1;
	node->magic = SDLZLOOKUP_MAGIC;

	*nodep = node;
	return (ISC_R_SUCCESS);
}

static void
destroynode(dns_sdlznode_t *node) {
	dns_rdatalist_t *list;
	dns_rdata_t *rdata;
	isc_buffer_t *b;
	dns_sdlz_db_t *sdlz;
	dns_db_t *db;
	isc_mem_t *mctx;

	sdlz = node->sdlz;
	mctx = sdlz->common.mctx;

	while (!ISC_LIST_EMPTY(node->lists)) {
		list = ISC_LIST_HEAD(node->lists);
		while (!ISC_LIST_EMPTY(list->rdata)) {
			rdata = ISC_LIST_HEAD(list->rdata);
			ISC_LIST_UNLINK(list->rdata, rdata, link);
			isc_mem_put(mctx, rdata, sizeof(dns_rdata_t));
		}
		ISC_LIST_UNLINK(node->lists, list, link);
		isc_mem_put(mctx, list, sizeof(dns_rdatalist_t));
	}

	while (!ISC_LIST_EMPTY(node->buffers)) {
		b = ISC_LIST_HEAD(node->buffers);
		ISC_LIST_UNLINK(node->buffers, b, link);
		isc_buffer_free(&b);
	}

	if (node->name != NULL) {
		dns_name_free(node->name, mctx);
		isc_mem_put(mctx, node->name, sizeof(dns_name_t));
	}
	DESTROYLOCK(&node->lock);
	node->magic = 0;
	isc_mem_put(mctx, node, sizeof(dns_sdlznode_t));
	db = &sdlz->common;
	detach(&db);
}

static isc_result_t
findnode(dns_db_t *db, dns_name_t *name, isc_boolean_t create,
	 dns_dbnode_t **nodep)
{
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *)db;
	dns_sdlznode_t *node = NULL;
	isc_result_t result;
	isc_buffer_t b;
	char namestr[DNS_NAME_MAXTEXT + 1];
	isc_buffer_t b2;
	char zonestr[DNS_NAME_MAXTEXT + 1];
	isc_boolean_t isorigin;
	dns_sdlzauthorityfunc_t authority;

	REQUIRE(VALID_SDLZDB(sdlz));
	REQUIRE(create == ISC_FALSE);
	REQUIRE(nodep != NULL && *nodep == NULL);

	UNUSED(name);
	UNUSED(create);

	isc_buffer_init(&b, namestr, sizeof(namestr));
	if ((sdlz->dlzimp->flags & DNS_SDLZFLAG_RELATIVEOWNER) != 0) {
		dns_name_t relname;
		unsigned int labels;

		labels = dns_name_countlabels(name) -
			 dns_name_countlabels(&db->origin);
		dns_name_init(&relname, NULL);
		dns_name_getlabelsequence(name, 0, labels, &relname);
		result = dns_name_totext(&relname, ISC_TRUE, &b);
		if (result != ISC_R_SUCCESS)
			return (result);
	} else {
		result = dns_name_totext(name, ISC_TRUE, &b);
		if (result != ISC_R_SUCCESS)
			return (result);
	}
	isc_buffer_putuint8(&b, 0);

	isc_buffer_init(&b2, zonestr, sizeof(zonestr));
	result = dns_name_totext(&sdlz->common.origin, ISC_TRUE, &b2);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_buffer_putuint8(&b2, 0);

	result = createnode(sdlz, &node);
	if (result != ISC_R_SUCCESS)
		return (result);

	isorigin = dns_name_equal(name, &sdlz->common.origin);

	/* make sure strings are always lowercase */
	dns_sdlz_tolower(zonestr);
	dns_sdlz_tolower(namestr);

	MAYBE_LOCK(sdlz->dlzimp);

	/* try to lookup the host (namestr) */
	result = sdlz->dlzimp->methods->lookup(zonestr, namestr,
					       sdlz->dlzimp->driverarg,
					       sdlz->dbdata, node);

	/*
	 * if the host (namestr) was not found, try to lookup a
	 * "wildcard" host.
	 */
	if (result != ISC_R_SUCCESS) {
		result = sdlz->dlzimp->methods->lookup(zonestr, "*",
						       sdlz->dlzimp->driverarg,
						       sdlz->dbdata, node);
	}

	MAYBE_UNLOCK(sdlz->dlzimp);

	if (result != ISC_R_SUCCESS && !isorigin) {
		destroynode(node);
		return (result);
	}

	if (isorigin && sdlz->dlzimp->methods->authority != NULL) {
		MAYBE_LOCK(sdlz->dlzimp);
		authority = sdlz->dlzimp->methods->authority;
		result = (*authority)(zonestr, sdlz->dlzimp->driverarg,
				      sdlz->dbdata, node);
		MAYBE_UNLOCK(sdlz->dlzimp);
		if (result != ISC_R_SUCCESS &&
		    result != ISC_R_NOTIMPLEMENTED) {
			destroynode(node);
			return (result);
		}
	}

	*nodep = node;
	return (ISC_R_SUCCESS);
}

static isc_result_t
findzonecut(dns_db_t *db, dns_name_t *name, unsigned int options,
	    isc_stdtime_t now, dns_dbnode_t **nodep, dns_name_t *foundname,
	    dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	UNUSED(db);
	UNUSED(name);
	UNUSED(options);
	UNUSED(now);
	UNUSED(nodep);
	UNUSED(foundname);
	UNUSED(rdataset);
	UNUSED(sigrdataset);

	return (ISC_R_NOTIMPLEMENTED);
}

static void
attachnode(dns_db_t *db, dns_dbnode_t *source, dns_dbnode_t **targetp) {
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *)db;
	dns_sdlznode_t *node = (dns_sdlznode_t *)source;

	REQUIRE(VALID_SDLZDB(sdlz));

	UNUSED(sdlz);

	LOCK(&node->lock);
	INSIST(node->references > 0);
	node->references++;
	INSIST(node->references != 0);		/* Catch overflow. */
	UNLOCK(&node->lock);

	*targetp = source;
}

static void
detachnode(dns_db_t *db, dns_dbnode_t **targetp) {
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *)db;
	dns_sdlznode_t *node;
	isc_boolean_t need_destroy = ISC_FALSE;

	REQUIRE(VALID_SDLZDB(sdlz));
	REQUIRE(targetp != NULL && *targetp != NULL);

	UNUSED(sdlz);

	node = (dns_sdlznode_t *)(*targetp);

	LOCK(&node->lock);
	INSIST(node->references > 0);
	node->references--;
	if (node->references == 0)
		need_destroy = ISC_TRUE;
	UNLOCK(&node->lock);

	if (need_destroy)
		destroynode(node);

	*targetp = NULL;
}

static isc_result_t
expirenode(dns_db_t *db, dns_dbnode_t *node, isc_stdtime_t now) {
	UNUSED(db);
	UNUSED(node);
	UNUSED(now);
	INSIST(0);
	return (ISC_R_UNEXPECTED);
}

static void
printnode(dns_db_t *db, dns_dbnode_t *node, FILE *out) {
	UNUSED(db);
	UNUSED(node);
	UNUSED(out);
	return;
}

static isc_result_t
createiterator(dns_db_t *db, unsigned int options, dns_dbiterator_t **iteratorp)
{
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *)db;
	sdlz_dbiterator_t *sdlziter;
	isc_result_t result;
	isc_buffer_t b;
	char zonestr[DNS_NAME_MAXTEXT + 1];

	REQUIRE(VALID_SDLZDB(sdlz));

	if (sdlz->dlzimp->methods->allnodes == NULL)
		return (ISC_R_NOTIMPLEMENTED);

	if ((options & DNS_DB_NSEC3ONLY) != 0 ||
	    (options & DNS_DB_NONSEC3) != 0)
		 return (ISC_R_NOTIMPLEMENTED);

	isc_buffer_init(&b, zonestr, sizeof(zonestr));
	result = dns_name_totext(&sdlz->common.origin, ISC_TRUE, &b);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_buffer_putuint8(&b, 0);

	sdlziter = isc_mem_get(sdlz->common.mctx, sizeof(sdlz_dbiterator_t));
	if (sdlziter == NULL)
		return (ISC_R_NOMEMORY);

	sdlziter->common.methods = &dbiterator_methods;
	sdlziter->common.db = NULL;
	dns_db_attach(db, &sdlziter->common.db);
	sdlziter->common.relative_names = ISC_TF(options & DNS_DB_RELATIVENAMES);
	sdlziter->common.magic = DNS_DBITERATOR_MAGIC;
	ISC_LIST_INIT(sdlziter->nodelist);
	sdlziter->current = NULL;
	sdlziter->origin = NULL;

	/* make sure strings are always lowercase */
	dns_sdlz_tolower(zonestr);

	MAYBE_LOCK(sdlz->dlzimp);
	result = sdlz->dlzimp->methods->allnodes(zonestr,
						 sdlz->dlzimp->driverarg,
						 sdlz->dbdata, sdlziter);
	MAYBE_UNLOCK(sdlz->dlzimp);
	if (result != ISC_R_SUCCESS) {
		dns_dbiterator_t *iter = &sdlziter->common;
		dbiterator_destroy(&iter);
		return (result);
	}

	if (sdlziter->origin != NULL) {
		ISC_LIST_UNLINK(sdlziter->nodelist, sdlziter->origin, link);
		ISC_LIST_PREPEND(sdlziter->nodelist, sdlziter->origin, link);
	}

	*iteratorp = (dns_dbiterator_t *)sdlziter;

	return (ISC_R_SUCCESS);
}

static isc_result_t
findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     dns_rdatatype_t type, dns_rdatatype_t covers,
	     isc_stdtime_t now, dns_rdataset_t *rdataset,
	     dns_rdataset_t *sigrdataset)
{
	dns_rdatalist_t *list;
	dns_sdlznode_t *sdlznode = (dns_sdlznode_t *)node;

	REQUIRE(VALID_SDLZNODE(node));

	UNUSED(db);
	UNUSED(version);
	UNUSED(covers);
	UNUSED(now);
	UNUSED(sigrdataset);

	if (type == dns_rdatatype_sig || type == dns_rdatatype_rrsig)
		return (ISC_R_NOTIMPLEMENTED);

	list = ISC_LIST_HEAD(sdlznode->lists);
	while (list != NULL) {
		if (list->type == type)
			break;
		list = ISC_LIST_NEXT(list, link);
	}
	if (list == NULL)
		return (ISC_R_NOTFOUND);

	list_tordataset(list, db, node, rdataset);

	return (ISC_R_SUCCESS);
}

static isc_result_t
find(dns_db_t *db, dns_name_t *name, dns_dbversion_t *version,
     dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
     dns_dbnode_t **nodep, dns_name_t *foundname,
     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *)db;
	dns_dbnode_t *node = NULL;
	dns_fixedname_t fname;
	dns_rdataset_t xrdataset;
	dns_name_t *xname;
	unsigned int nlabels, olabels;
	isc_result_t result;
	unsigned int i;

	REQUIRE(VALID_SDLZDB(sdlz));
	REQUIRE(nodep == NULL || *nodep == NULL);
	REQUIRE(version == NULL || version == (void *) &dummy);

	UNUSED(options);
	UNUSED(sdlz);

	if (!dns_name_issubdomain(name, &db->origin))
		return (DNS_R_NXDOMAIN);

	olabels = dns_name_countlabels(&db->origin);
	nlabels = dns_name_countlabels(name);

	dns_fixedname_init(&fname);
	xname = dns_fixedname_name(&fname);

	if (rdataset == NULL) {
		dns_rdataset_init(&xrdataset);
		rdataset = &xrdataset;
	}

	result = DNS_R_NXDOMAIN;

	for (i = olabels; i <= nlabels; i++) {
		/*
		 * Unless this is an explicit lookup at the origin, don't
		 * look at the origin.
		 */
		if (i == olabels && i != nlabels)
			continue;

		/*
		 * Look up the next label.
		 */
		dns_name_getlabelsequence(name, nlabels - i, i, xname);
		result = findnode(db, xname, ISC_FALSE, &node);
		if (result != ISC_R_SUCCESS) {
			result = DNS_R_NXDOMAIN;
			continue;
		}

		/*
		 * Look for a DNAME at the current label, unless this is
		 * the qname.
		 */
		if (i < nlabels) {
			result = findrdataset(db, node, version,
					      dns_rdatatype_dname,
					      0, now, rdataset, sigrdataset);
			if (result == ISC_R_SUCCESS) {
				result = DNS_R_DNAME;
				break;
			}
		}

		/*
		 * Look for an NS at the current label, unless this is the
		 * origin or glue is ok.
		 */
		if (i != olabels && (options & DNS_DBFIND_GLUEOK) == 0) {
			result = findrdataset(db, node, version,
					      dns_rdatatype_ns,
					      0, now, rdataset, sigrdataset);
			if (result == ISC_R_SUCCESS) {
				if (i == nlabels && type == dns_rdatatype_any)
				{
					result = DNS_R_ZONECUT;
					dns_rdataset_disassociate(rdataset);
					if (sigrdataset != NULL &&
					    dns_rdataset_isassociated
							(sigrdataset)) {
						dns_rdataset_disassociate
							(sigrdataset);
					}
				} else
					result = DNS_R_DELEGATION;
				break;
			}
		}

		/*
		 * If the current name is not the qname, add another label
		 * and try again.
		 */
		if (i < nlabels) {
			destroynode(node);
			node = NULL;
			continue;
		}

		/*
		 * If we're looking for ANY, we're done.
		 */
		if (type == dns_rdatatype_any) {
			result = ISC_R_SUCCESS;
			break;
		}

		/*
		 * Look for the qtype.
		 */
		result = findrdataset(db, node, version, type,
				      0, now, rdataset, sigrdataset);
		if (result == ISC_R_SUCCESS)
			break;

		/*
		 * Look for a CNAME
		 */
		if (type != dns_rdatatype_cname) {
			result = findrdataset(db, node, version,
					      dns_rdatatype_cname,
					      0, now, rdataset, sigrdataset);
			if (result == ISC_R_SUCCESS) {
				result = DNS_R_CNAME;
				break;
			}
		}

		result = DNS_R_NXRRSET;
		break;
	}

	if (rdataset == &xrdataset && dns_rdataset_isassociated(rdataset))
		dns_rdataset_disassociate(rdataset);

	if (foundname != NULL) {
		isc_result_t xresult;

		xresult = dns_name_copy(xname, foundname, NULL);
		if (xresult != ISC_R_SUCCESS) {
			if (node != NULL)
				destroynode(node);
			if (dns_rdataset_isassociated(rdataset))
				dns_rdataset_disassociate(rdataset);
			return (DNS_R_BADDB);
		}
	}

	if (nodep != NULL)
		*nodep = node;
	else if (node != NULL)
		detachnode(db, &node);

	return (result);
}

static isc_result_t
allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     isc_stdtime_t now, dns_rdatasetiter_t **iteratorp)
{
	sdlz_rdatasetiter_t *iterator;

	REQUIRE(version == NULL || version == &dummy);

	UNUSED(version);
	UNUSED(now);

	iterator = isc_mem_get(db->mctx, sizeof(sdlz_rdatasetiter_t));
	if (iterator == NULL)
		return (ISC_R_NOMEMORY);

	iterator->common.magic = DNS_RDATASETITER_MAGIC;
	iterator->common.methods = &rdatasetiter_methods;
	iterator->common.db = db;
	iterator->common.node = NULL;
	attachnode(db, node, &iterator->common.node);
	iterator->common.version = version;
	iterator->common.now = now;

	*iteratorp = (dns_rdatasetiter_t *)iterator;

	return (ISC_R_SUCCESS);
}

static isc_result_t
addrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	    isc_stdtime_t now, dns_rdataset_t *rdataset, unsigned int options,
	    dns_rdataset_t *addedrdataset)
{
	UNUSED(db);
	UNUSED(node);
	UNUSED(version);
	UNUSED(now);
	UNUSED(rdataset);
	UNUSED(options);
	UNUSED(addedrdataset);

	return (ISC_R_NOTIMPLEMENTED);
}

static isc_result_t
subtractrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		 dns_rdataset_t *rdataset, unsigned int options,
		 dns_rdataset_t *newrdataset)
{
	UNUSED(db);
	UNUSED(node);
	UNUSED(version);
	UNUSED(rdataset);
	UNUSED(options);
	UNUSED(newrdataset);

	return (ISC_R_NOTIMPLEMENTED);
}

static isc_result_t
deleterdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	       dns_rdatatype_t type, dns_rdatatype_t covers)
{
	UNUSED(db);
	UNUSED(node);
	UNUSED(version);
	UNUSED(type);
	UNUSED(covers);

	return (ISC_R_NOTIMPLEMENTED);
}

static isc_boolean_t
issecure(dns_db_t *db) {
	UNUSED(db);

	return (ISC_FALSE);
}

static unsigned int
nodecount(dns_db_t *db) {
	UNUSED(db);

	return (0);
}

static isc_boolean_t
ispersistent(dns_db_t *db) {
	UNUSED(db);
	return (ISC_TRUE);
}

static void
overmem(dns_db_t *db, isc_boolean_t overmem) {
	UNUSED(db);
	UNUSED(overmem);
}

static void
settask(dns_db_t *db, isc_task_t *task) {
	UNUSED(db);
	UNUSED(task);
}


static dns_dbmethods_t sdlzdb_methods = {
	attach,
	detach,
	beginload,
	endload,
	dump,
	currentversion,
	newversion,
	attachversion,
	closeversion,
	findnode,
	find,
	findzonecut,
	attachnode,
	detachnode,
	expirenode,
	printnode,
	createiterator,
	findrdataset,
	allrdatasets,
	addrdataset,
	subtractrdataset,
	deleterdataset,
	issecure,
	nodecount,
	ispersistent,
	overmem,
	settask,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/*
 * Database Iterator Methods.  These methods were "borrowed" from the SDB
 * driver interface.  See the SDB driver interface documentation for more info.
 */

static void
dbiterator_destroy(dns_dbiterator_t **iteratorp) {
	sdlz_dbiterator_t *sdlziter = (sdlz_dbiterator_t *)(*iteratorp);
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *)sdlziter->common.db;

	while (!ISC_LIST_EMPTY(sdlziter->nodelist)) {
		dns_sdlznode_t *node;
		node = ISC_LIST_HEAD(sdlziter->nodelist);
		ISC_LIST_UNLINK(sdlziter->nodelist, node, link);
		destroynode(node);
	}

	dns_db_detach(&sdlziter->common.db);
	isc_mem_put(sdlz->common.mctx, sdlziter, sizeof(sdlz_dbiterator_t));

	*iteratorp = NULL;
}

static isc_result_t
dbiterator_first(dns_dbiterator_t *iterator) {
	sdlz_dbiterator_t *sdlziter = (sdlz_dbiterator_t *)iterator;

	sdlziter->current = ISC_LIST_HEAD(sdlziter->nodelist);
	if (sdlziter->current == NULL)
		return (ISC_R_NOMORE);
	else
		return (ISC_R_SUCCESS);
}

static isc_result_t
dbiterator_last(dns_dbiterator_t *iterator) {
	sdlz_dbiterator_t *sdlziter = (sdlz_dbiterator_t *)iterator;

	sdlziter->current = ISC_LIST_TAIL(sdlziter->nodelist);
	if (sdlziter->current == NULL)
		return (ISC_R_NOMORE);
	else
		return (ISC_R_SUCCESS);
}

static isc_result_t
dbiterator_seek(dns_dbiterator_t *iterator, dns_name_t *name) {
	sdlz_dbiterator_t *sdlziter = (sdlz_dbiterator_t *)iterator;

	sdlziter->current = ISC_LIST_HEAD(sdlziter->nodelist);
	while (sdlziter->current != NULL)
		if (dns_name_equal(sdlziter->current->name, name))
			return (ISC_R_SUCCESS);
	return (ISC_R_NOTFOUND);
}

static isc_result_t
dbiterator_prev(dns_dbiterator_t *iterator) {
	sdlz_dbiterator_t *sdlziter = (sdlz_dbiterator_t *)iterator;

	sdlziter->current = ISC_LIST_PREV(sdlziter->current, link);
	if (sdlziter->current == NULL)
		return (ISC_R_NOMORE);
	else
		return (ISC_R_SUCCESS);
}

static isc_result_t
dbiterator_next(dns_dbiterator_t *iterator) {
	sdlz_dbiterator_t *sdlziter = (sdlz_dbiterator_t *)iterator;

	sdlziter->current = ISC_LIST_NEXT(sdlziter->current, link);
	if (sdlziter->current == NULL)
		return (ISC_R_NOMORE);
	else
		return (ISC_R_SUCCESS);
}

static isc_result_t
dbiterator_current(dns_dbiterator_t *iterator, dns_dbnode_t **nodep,
		   dns_name_t *name)
{
	sdlz_dbiterator_t *sdlziter = (sdlz_dbiterator_t *)iterator;

	attachnode(iterator->db, sdlziter->current, nodep);
	if (name != NULL)
		return (dns_name_copy(sdlziter->current->name, name, NULL));
	return (ISC_R_SUCCESS);
}

static isc_result_t
dbiterator_pause(dns_dbiterator_t *iterator) {
	UNUSED(iterator);
	return (ISC_R_SUCCESS);
}

static isc_result_t
dbiterator_origin(dns_dbiterator_t *iterator, dns_name_t *name) {
	UNUSED(iterator);
	return (dns_name_copy(dns_rootname, name, NULL));
}

/*
 * Rdataset Methods. These methods were "borrowed" from the SDB driver
 * interface.  See the SDB driver interface documentation for more info.
 */

static void
disassociate(dns_rdataset_t *rdataset) {
	dns_dbnode_t *node = rdataset->private5;
	dns_sdlznode_t *sdlznode = (dns_sdlznode_t *) node;
	dns_db_t *db = (dns_db_t *) sdlznode->sdlz;

	detachnode(db, &node);
	isc__rdatalist_disassociate(rdataset);
}

static void
rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target) {
	dns_dbnode_t *node = source->private5;
	dns_sdlznode_t *sdlznode = (dns_sdlznode_t *) node;
	dns_db_t *db = (dns_db_t *) sdlznode->sdlz;
	dns_dbnode_t *tempdb = NULL;

	isc__rdatalist_clone(source, target);
	attachnode(db, node, &tempdb);
	source->private5 = tempdb;
}

static dns_rdatasetmethods_t rdataset_methods = {
	disassociate,
	isc__rdatalist_first,
	isc__rdatalist_next,
	isc__rdatalist_current,
	rdataset_clone,
	isc__rdatalist_count,
	isc__rdatalist_addnoqname,
	isc__rdatalist_getnoqname,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
list_tordataset(dns_rdatalist_t *rdatalist,
		dns_db_t *db, dns_dbnode_t *node,
		dns_rdataset_t *rdataset)
{
	/*
	 * The sdlz rdataset is an rdatalist with some additions.
	 *	- private1 & private2 are used by the rdatalist.
	 *	- private3 & private 4 are unused.
	 *	- private5 is the node.
	 */

	/* This should never fail. */
	RUNTIME_CHECK(dns_rdatalist_tordataset(rdatalist, rdataset) ==
		      ISC_R_SUCCESS);

	rdataset->methods = &rdataset_methods;
	dns_db_attachnode(db, node, &rdataset->private5);
}

/*
 * SDLZ core methods. This is the core of the new DLZ functionality.
 */

/*%
 * Build a 'bind' database driver structure to be returned by
 * either the find zone or the allow zone transfer method.
 * This method is only available in this source file, it is
 * not made available anywhere else.
 */

static isc_result_t
dns_sdlzcreateDBP(isc_mem_t *mctx, void *driverarg, void *dbdata,
		  dns_name_t *name, dns_rdataclass_t rdclass, dns_db_t **dbp)
{
	isc_result_t result;
	dns_sdlz_db_t *sdlzdb;
	dns_sdlzimplementation_t *imp;

	/* check that things are as we expect */
	REQUIRE(dbp != NULL && *dbp == NULL);
	REQUIRE(name != NULL);

	imp = (dns_sdlzimplementation_t *) driverarg;

	/* allocate and zero memory for driver structure */
	sdlzdb = isc_mem_get(mctx, sizeof(dns_sdlz_db_t));
	if (sdlzdb == NULL)
		return (ISC_R_NOMEMORY);
	memset(sdlzdb, 0, sizeof(dns_sdlz_db_t));

	/* initialize and set origin */
	dns_name_init(&sdlzdb->common.origin, NULL);
	result = dns_name_dupwithoffsets(name, mctx, &sdlzdb->common.origin);
	if (result != ISC_R_SUCCESS)
		goto mem_cleanup;

	/* initialize the reference count mutex */
	result = isc_mutex_init(&sdlzdb->refcnt_lock);
	if (result != ISC_R_SUCCESS)
		goto name_cleanup;

	/* set the rest of the database structure attributes */
	sdlzdb->dlzimp = imp;
	sdlzdb->common.methods = &sdlzdb_methods;
	sdlzdb->common.attributes = 0;
	sdlzdb->common.rdclass = rdclass;
	sdlzdb->common.mctx = NULL;
	sdlzdb->dbdata = dbdata;
	sdlzdb->references = 1;

	/* attach to the memory context */
	isc_mem_attach(mctx, &sdlzdb->common.mctx);

	/* mark structure as valid */
	sdlzdb->common.magic = DNS_DB_MAGIC;
	sdlzdb->common.impmagic = SDLZDB_MAGIC;
	*dbp = (dns_db_t *) sdlzdb;

	return (result);

	/*
	 * reference count mutex could not be initialized, clean up
	 * name memory
	 */
 name_cleanup:
	dns_name_free(&sdlzdb->common.origin, mctx);
 mem_cleanup:
	isc_mem_put(mctx, sdlzdb, sizeof(dns_sdlz_db_t));
	return (result);
}

static isc_result_t
dns_sdlzallowzonexfr(void *driverarg, void *dbdata, isc_mem_t *mctx,
		     dns_rdataclass_t rdclass, dns_name_t *name,
		     isc_sockaddr_t *clientaddr, dns_db_t **dbp)
{
	isc_buffer_t b;
	isc_buffer_t b2;
	char namestr[DNS_NAME_MAXTEXT + 1];
	char clientstr[(sizeof "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255")
		       + 1];
	isc_netaddr_t netaddr;
	isc_result_t result;
	dns_sdlzimplementation_t *imp;

	/*
	 * Perform checks to make sure data is as we expect it to be.
	 */
	REQUIRE(driverarg != NULL);
	REQUIRE(name != NULL);
	REQUIRE(clientaddr != NULL);
	REQUIRE(dbp != NULL && *dbp == NULL);

	imp = (dns_sdlzimplementation_t *) driverarg;

	/* Convert DNS name to ascii text */
	isc_buffer_init(&b, namestr, sizeof(namestr));
	result = dns_name_totext(name, ISC_TRUE, &b);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_buffer_putuint8(&b, 0);

	/* convert client address to ascii text */
	isc_buffer_init(&b2, clientstr, sizeof(clientstr));
	isc_netaddr_fromsockaddr(&netaddr, clientaddr);
	result = isc_netaddr_totext(&netaddr, &b2);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_buffer_putuint8(&b2, 0);

	/* make sure strings are always lowercase */
	dns_sdlz_tolower(namestr);
	dns_sdlz_tolower(clientstr);

	/* Call SDLZ driver's find zone method */
	if (imp->methods->allowzonexfr != NULL) {
		MAYBE_LOCK(imp);
		result = imp->methods->allowzonexfr(imp->driverarg, dbdata,
						    namestr, clientstr);
		MAYBE_UNLOCK(imp);
		/*
		 * if zone is supported and transfers allowed build a 'bind'
		 * database driver
		 */
		if (result == ISC_R_SUCCESS)
			result = dns_sdlzcreateDBP(mctx, driverarg, dbdata,
						   name, rdclass, dbp);
		return (result);
	}

	return (ISC_R_NOTIMPLEMENTED);
}

static isc_result_t
dns_sdlzcreate(isc_mem_t *mctx, const char *dlzname, unsigned int argc,
	       char *argv[], void *driverarg, void **dbdata)
{
	dns_sdlzimplementation_t *imp;
	isc_result_t result = ISC_R_NOTFOUND;

	/* Write debugging message to log */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
		      "Loading SDLZ driver.");

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(driverarg != NULL);
	REQUIRE(dlzname != NULL);
	REQUIRE(dbdata != NULL);
	UNUSED(mctx);

	imp = driverarg;

	/* If the create method exists, call it. */
	if (imp->methods->create != NULL) {
		MAYBE_LOCK(imp);
		result = imp->methods->create(dlzname, argc, argv,
					      imp->driverarg, dbdata);
		MAYBE_UNLOCK(imp);
	}

	/* Write debugging message to log */
	if (result == ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
			      "SDLZ driver loaded successfully.");
	} else {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "SDLZ driver failed to load.");
	}

	return (result);
}

static void
dns_sdlzdestroy(void *driverdata, void **dbdata)
{

	dns_sdlzimplementation_t *imp;

	/* Write debugging message to log */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
		      "Unloading SDLZ driver.");

	imp = driverdata;

	/* If the destroy method exists, call it. */
	if (imp->methods->destroy != NULL) {
		MAYBE_LOCK(imp);
		imp->methods->destroy(imp->driverarg, dbdata);
		MAYBE_UNLOCK(imp);
	}
}

static isc_result_t
dns_sdlzfindzone(void *driverarg, void *dbdata, isc_mem_t *mctx,
		 dns_rdataclass_t rdclass, dns_name_t *name, dns_db_t **dbp)
{
	isc_buffer_t b;
	char namestr[DNS_NAME_MAXTEXT + 1];
	isc_result_t result;
	dns_sdlzimplementation_t *imp;

	/*
	 * Perform checks to make sure data is as we expect it to be.
	 */
	REQUIRE(driverarg != NULL);
	REQUIRE(name != NULL);
	REQUIRE(dbp != NULL && *dbp == NULL);

	imp = (dns_sdlzimplementation_t *) driverarg;

	/* Convert DNS name to ascii text */
	isc_buffer_init(&b, namestr, sizeof(namestr));
	result = dns_name_totext(name, ISC_TRUE, &b);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_buffer_putuint8(&b, 0);

	/* make sure strings are always lowercase */
	dns_sdlz_tolower(namestr);

	/* Call SDLZ driver's find zone method */
	MAYBE_LOCK(imp);
	result = imp->methods->findzone(imp->driverarg, dbdata, namestr);
	MAYBE_UNLOCK(imp);

	/*
	 * if zone is supported build a 'bind' database driver
	 * structure to return
	 */
	if (result == ISC_R_SUCCESS)
		result = dns_sdlzcreateDBP(mctx, driverarg, dbdata, name,
					   rdclass, dbp);

	return (result);
}

static dns_dlzmethods_t sdlzmethods = {
	dns_sdlzcreate,
	dns_sdlzdestroy,
	dns_sdlzfindzone,
	dns_sdlzallowzonexfr
};

/*
 * Public functions.
 */

isc_result_t
dns_sdlz_putrr(dns_sdlzlookup_t *lookup, const char *type, dns_ttl_t ttl,
	       const char *data)
{
	dns_rdatalist_t *rdatalist;
	dns_rdata_t *rdata;
	dns_rdatatype_t typeval;
	isc_consttextregion_t r;
	isc_buffer_t b;
	isc_buffer_t *rdatabuf = NULL;
	isc_lex_t *lex;
	isc_result_t result;
	unsigned int size;
	isc_mem_t *mctx;
	dns_name_t *origin;

	REQUIRE(VALID_SDLZLOOKUP(lookup));
	REQUIRE(type != NULL);
	REQUIRE(data != NULL);

	mctx = lookup->sdlz->common.mctx;

	r.base = type;
	r.length = strlen(type);
	result = dns_rdatatype_fromtext(&typeval, (void *) &r);
	if (result != ISC_R_SUCCESS)
		return (result);

	rdatalist = ISC_LIST_HEAD(lookup->lists);
	while (rdatalist != NULL) {
		if (rdatalist->type == typeval)
			break;
		rdatalist = ISC_LIST_NEXT(rdatalist, link);
	}

	if (rdatalist == NULL) {
		rdatalist = isc_mem_get(mctx, sizeof(dns_rdatalist_t));
		if (rdatalist == NULL)
			return (ISC_R_NOMEMORY);
		rdatalist->rdclass = lookup->sdlz->common.rdclass;
		rdatalist->type = typeval;
		rdatalist->covers = 0;
		rdatalist->ttl = ttl;
		ISC_LIST_INIT(rdatalist->rdata);
		ISC_LINK_INIT(rdatalist, link);
		ISC_LIST_APPEND(lookup->lists, rdatalist, link);
	} else
		if (rdatalist->ttl != ttl)
			return (DNS_R_BADTTL);

	rdata = isc_mem_get(mctx, sizeof(dns_rdata_t));
	if (rdata == NULL)
		return (ISC_R_NOMEMORY);
	dns_rdata_init(rdata);

	if ((lookup->sdlz->dlzimp->flags & DNS_SDLZFLAG_RELATIVERDATA) != 0)
		origin = &lookup->sdlz->common.origin;
	else
		origin = dns_rootname;

	lex = NULL;
	result = isc_lex_create(mctx, 64, &lex);
	if (result != ISC_R_SUCCESS)
		goto failure;

	size = initial_size(data);
	do {
		isc_buffer_init(&b, data, strlen(data));
		isc_buffer_add(&b, strlen(data));

		result = isc_lex_openbuffer(lex, &b);
		if (result != ISC_R_SUCCESS)
			goto failure;

		rdatabuf = NULL;
		result = isc_buffer_allocate(mctx, &rdatabuf, size);
		if (result != ISC_R_SUCCESS)
			goto failure;

		result = dns_rdata_fromtext(rdata, rdatalist->rdclass,
					    rdatalist->type, lex,
					    origin, ISC_FALSE,
					    mctx, rdatabuf,
					    &lookup->callbacks);
		if (result != ISC_R_SUCCESS)
			isc_buffer_free(&rdatabuf);
		size *= 2;
	} while (result == ISC_R_NOSPACE);

	if (result != ISC_R_SUCCESS)
		goto failure;

	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	ISC_LIST_APPEND(lookup->buffers, rdatabuf, link);

	if (lex != NULL)
		isc_lex_destroy(&lex);

	return (ISC_R_SUCCESS);

 failure:
	if (rdatabuf != NULL)
		isc_buffer_free(&rdatabuf);
	if (lex != NULL)
		isc_lex_destroy(&lex);
	isc_mem_put(mctx, rdata, sizeof(dns_rdata_t));

	return (result);
}

isc_result_t
dns_sdlz_putnamedrr(dns_sdlzallnodes_t *allnodes, const char *name,
		    const char *type, dns_ttl_t ttl, const char *data)
{
	dns_name_t *newname, *origin;
	dns_fixedname_t fnewname;
	dns_sdlz_db_t *sdlz = (dns_sdlz_db_t *)allnodes->common.db;
	dns_sdlznode_t *sdlznode;
	isc_mem_t *mctx = sdlz->common.mctx;
	isc_buffer_t b;
	isc_result_t result;

	dns_fixedname_init(&fnewname);
	newname = dns_fixedname_name(&fnewname);

	if ((sdlz->dlzimp->flags & DNS_SDLZFLAG_RELATIVERDATA) != 0)
		origin = &sdlz->common.origin;
	else
		origin = dns_rootname;
	isc_buffer_init(&b, name, strlen(name));
	isc_buffer_add(&b, strlen(name));

	result = dns_name_fromtext(newname, &b, origin, ISC_FALSE, NULL);
	if (result != ISC_R_SUCCESS)
		return (result);

	if (allnodes->common.relative_names) {
		/* All names are relative to the root */
		unsigned int nlabels = dns_name_countlabels(newname);
		dns_name_getlabelsequence(newname, 0, nlabels - 1, newname);
	}

	sdlznode = ISC_LIST_HEAD(allnodes->nodelist);
	if (sdlznode == NULL || !dns_name_equal(sdlznode->name, newname)) {
		sdlznode = NULL;
		result = createnode(sdlz, &sdlznode);
		if (result != ISC_R_SUCCESS)
			return (result);
		sdlznode->name = isc_mem_get(mctx, sizeof(dns_name_t));
		if (sdlznode->name == NULL) {
			destroynode(sdlznode);
			return (ISC_R_NOMEMORY);
		}
		dns_name_init(sdlznode->name, NULL);
		result = dns_name_dup(newname, mctx, sdlznode->name);
		if (result != ISC_R_SUCCESS) {
			isc_mem_put(mctx, sdlznode->name, sizeof(dns_name_t));
			destroynode(sdlznode);
			return (result);
		}
		ISC_LIST_PREPEND(allnodes->nodelist, sdlznode, link);
		if (allnodes->origin == NULL &&
		    dns_name_equal(newname, &sdlz->common.origin))
			allnodes->origin = sdlznode;
	}
	return (dns_sdlz_putrr(sdlznode, type, ttl, data));

}

isc_result_t
dns_sdlz_putsoa(dns_sdlzlookup_t *lookup, const char *mname, const char *rname,
		isc_uint32_t serial)
{
	char str[2 * DNS_NAME_MAXTEXT + 5 * (sizeof("2147483647")) + 7];
	int n;

	REQUIRE(mname != NULL);
	REQUIRE(rname != NULL);

	n = snprintf(str, sizeof str, "%s %s %u %u %u %u %u",
		     mname, rname, serial,
		     SDLZ_DEFAULT_REFRESH, SDLZ_DEFAULT_RETRY,
		     SDLZ_DEFAULT_EXPIRE, SDLZ_DEFAULT_MINIMUM);
	if (n >= (int)sizeof(str) || n < 0)
		return (ISC_R_NOSPACE);
	return (dns_sdlz_putrr(lookup, "SOA", SDLZ_DEFAULT_TTL, str));
}

isc_result_t
dns_sdlzregister(const char *drivername, const dns_sdlzmethods_t *methods,
		 void *driverarg, unsigned int flags, isc_mem_t *mctx,
		 dns_sdlzimplementation_t **sdlzimp)
{

	dns_sdlzimplementation_t *imp;
	isc_result_t result;

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(drivername != NULL);
	REQUIRE(methods != NULL);
	REQUIRE(methods->findzone != NULL);
	REQUIRE(methods->lookup != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(sdlzimp != NULL && *sdlzimp == NULL);
	REQUIRE((flags & ~(DNS_SDLZFLAG_RELATIVEOWNER |
			   DNS_SDLZFLAG_RELATIVERDATA |
			   DNS_SDLZFLAG_THREADSAFE)) == 0);

	/* Write debugging message to log */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
		      "Registering SDLZ driver '%s'", drivername);

	/*
	 * Allocate memory for a sdlz_implementation object.  Error if
	 * we cannot.
	 */
	imp = isc_mem_get(mctx, sizeof(dns_sdlzimplementation_t));
	if (imp == NULL)
		return (ISC_R_NOMEMORY);

	/* Make sure memory region is set to all 0's */
	memset(imp, 0, sizeof(dns_sdlzimplementation_t));

	/* Store the data passed into this method */
	imp->methods = methods;
	imp->driverarg = driverarg;
	imp->flags = flags;
	imp->mctx = NULL;

	/* attach the new sdlz_implementation object to a memory context */
	isc_mem_attach(mctx, &imp->mctx);

	/*
	 * initialize the driver lock, error if we cannot
	 * (used if a driver does not support multiple threads)
	 */
	result = isc_mutex_init(&imp->driverlock);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_mutex_init() failed: %s",
				 isc_result_totext(result));
		goto cleanup_mctx;
	}

	imp->dlz_imp = NULL;

	/*
	 * register the DLZ driver.  Pass in our "extra" sdlz information as
	 * a driverarg.  (that's why we stored the passed in driver arg in our
	 * sdlz_implementation structure)  Also, store the dlz_implementation
	 * structure in our sdlz_implementation.
	 */
	result = dns_dlzregister(drivername, &sdlzmethods, imp, mctx,
				 &imp->dlz_imp);

	/* if registration fails, cleanup and get outta here. */
	if (result != ISC_R_SUCCESS)
		goto cleanup_mutex;

	*sdlzimp = imp;

	return (ISC_R_SUCCESS);

 cleanup_mutex:
	/* destroy the driver lock, we don't need it anymore */
	DESTROYLOCK(&imp->driverlock);

 cleanup_mctx:
	/*
	 * return the memory back to the available memory pool and
	 * remove it from the memory context.
	 */
	isc_mem_put(mctx, imp, sizeof(dns_sdlzimplementation_t));
	isc_mem_detach(&mctx);
	return (result);
}

void
dns_sdlzunregister(dns_sdlzimplementation_t **sdlzimp) {
	dns_sdlzimplementation_t *imp;
	isc_mem_t *mctx;

	/* Write debugging message to log */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
		      "Unregistering SDLZ driver.");

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(sdlzimp != NULL && *sdlzimp != NULL);

	imp = *sdlzimp;

	/* Unregister the DLZ driver implementation */
	dns_dlzunregister(&imp->dlz_imp);

	/* destroy the driver lock, we don't need it anymore */
	DESTROYLOCK(&imp->driverlock);

	mctx = imp->mctx;

	/*
	 * return the memory back to the available memory pool and
	 * remove it from the memory context.
	 */
	isc_mem_put(mctx, imp, sizeof(dns_sdlzimplementation_t));
	isc_mem_detach(&mctx);

	*sdlzimp = NULL;
}
