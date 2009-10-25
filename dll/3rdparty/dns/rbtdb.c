/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
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

/* $Id: rbtdb.c,v 1.270.12.6 2009/05/06 23:34:30 jinmei Exp $ */

/*! \file */

/*
 * Principal Author: Bob Halley
 */

#include <config.h>

/* #define inline */

#include <isc/event.h>
#include <isc/heap.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/serial.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/acache.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/events.h>
#include <dns/fixedname.h>
#include <dns/lib.h>
#include <dns/log.h>
#include <dns/masterdump.h>
#include <dns/nsec.h>
#include <dns/nsec3.h>
#include <dns/rbt.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdataslab.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>
#include <dns/stats.h>
#include <dns/view.h>
#include <dns/zone.h>
#include <dns/zonekey.h>

#ifdef DNS_RBTDB_VERSION64
#include "rbtdb64.h"
#else
#include "rbtdb.h"
#endif

#ifdef DNS_RBTDB_VERSION64
#define RBTDB_MAGIC                     ISC_MAGIC('R', 'B', 'D', '8')
#else
#define RBTDB_MAGIC                     ISC_MAGIC('R', 'B', 'D', '4')
#endif

/*%
 * Note that "impmagic" is not the first four bytes of the struct, so
 * ISC_MAGIC_VALID cannot be used.
 */
#define VALID_RBTDB(rbtdb)      ((rbtdb) != NULL && \
				 (rbtdb)->common.impmagic == RBTDB_MAGIC)

#ifdef DNS_RBTDB_VERSION64
typedef isc_uint64_t                    rbtdb_serial_t;
/*%
 * Make casting easier in symbolic debuggers by using different names
 * for the 64 bit version.
 */
#define dns_rbtdb_t dns_rbtdb64_t
#define rdatasetheader_t rdatasetheader64_t
#define rbtdb_version_t rbtdb_version64_t
#else
typedef isc_uint32_t                    rbtdb_serial_t;
#endif

typedef isc_uint32_t                    rbtdb_rdatatype_t;

#define RBTDB_RDATATYPE_BASE(type)      ((dns_rdatatype_t)((type) & 0xFFFF))
#define RBTDB_RDATATYPE_EXT(type)       ((dns_rdatatype_t)((type) >> 16))
#define RBTDB_RDATATYPE_VALUE(b, e)     ((rbtdb_rdatatype_t)((e) << 16) | (b))

#define RBTDB_RDATATYPE_SIGNSEC \
		RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, dns_rdatatype_nsec)
#define RBTDB_RDATATYPE_SIGNSEC3 \
		RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, dns_rdatatype_nsec3)
#define RBTDB_RDATATYPE_SIGNS \
		RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, dns_rdatatype_ns)
#define RBTDB_RDATATYPE_SIGCNAME \
		RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, dns_rdatatype_cname)
#define RBTDB_RDATATYPE_SIGDNAME \
		RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, dns_rdatatype_dname)
#define RBTDB_RDATATYPE_NCACHEANY \
		RBTDB_RDATATYPE_VALUE(0, dns_rdatatype_any)

/*
 * We use rwlock for DB lock only when ISC_RWLOCK_USEATOMIC is non 0.
 * Using rwlock is effective with regard to lookup performance only when
 * it is implemented in an efficient way.
 * Otherwise, it is generally wise to stick to the simple locking since rwlock
 * would require more memory or can even make lookups slower due to its own
 * overhead (when it internally calls mutex locks).
 */
#ifdef ISC_RWLOCK_USEATOMIC
#define DNS_RBTDB_USERWLOCK 1
#else
#define DNS_RBTDB_USERWLOCK 0
#endif

#if DNS_RBTDB_USERWLOCK
#define RBTDB_INITLOCK(l)       isc_rwlock_init((l), 0, 0)
#define RBTDB_DESTROYLOCK(l)    isc_rwlock_destroy(l)
#define RBTDB_LOCK(l, t)        RWLOCK((l), (t))
#define RBTDB_UNLOCK(l, t)      RWUNLOCK((l), (t))
#else
#define RBTDB_INITLOCK(l)       isc_mutex_init(l)
#define RBTDB_DESTROYLOCK(l)    DESTROYLOCK(l)
#define RBTDB_LOCK(l, t)        LOCK(l)
#define RBTDB_UNLOCK(l, t)      UNLOCK(l)
#endif

/*
 * Since node locking is sensitive to both performance and memory footprint,
 * we need some trick here.  If we have both high-performance rwlock and
 * high performance and small-memory reference counters, we use rwlock for
 * node lock and isc_refcount for node references.  In this case, we don't have
 * to protect the access to the counters by locks.
 * Otherwise, we simply use ordinary mutex lock for node locking, and use
 * simple integers as reference counters which is protected by the lock.
 * In most cases, we can simply use wrapper macros such as NODE_LOCK and
 * NODE_UNLOCK.  In some other cases, however, we need to protect reference
 * counters first and then protect other parts of a node as read-only data.
 * Special additional macros, NODE_STRONGLOCK(), NODE_WEAKLOCK(), etc, are also
 * provided for these special cases.  When we can use the efficient backend
 * routines, we should only protect the "other members" by NODE_WEAKLOCK(read).
 * Otherwise, we should use NODE_STRONGLOCK() to protect the entire critical
 * section including the access to the reference counter.
 * Note that we cannot use NODE_LOCK()/NODE_UNLOCK() wherever the protected
 * section is also protected by NODE_STRONGLOCK().
 */
#if defined(ISC_RWLOCK_USEATOMIC) && defined(DNS_RBT_USEISCREFCOUNT)
typedef isc_rwlock_t nodelock_t;

#define NODE_INITLOCK(l)        isc_rwlock_init((l), 0, 0)
#define NODE_DESTROYLOCK(l)     isc_rwlock_destroy(l)
#define NODE_LOCK(l, t)         RWLOCK((l), (t))
#define NODE_UNLOCK(l, t)       RWUNLOCK((l), (t))
#define NODE_TRYUPGRADE(l)      isc_rwlock_tryupgrade(l)

#define NODE_STRONGLOCK(l)      ((void)0)
#define NODE_STRONGUNLOCK(l)    ((void)0)
#define NODE_WEAKLOCK(l, t)     NODE_LOCK(l, t)
#define NODE_WEAKUNLOCK(l, t)   NODE_UNLOCK(l, t)
#define NODE_WEAKDOWNGRADE(l)   isc_rwlock_downgrade(l)
#else
typedef isc_mutex_t nodelock_t;

#define NODE_INITLOCK(l)        isc_mutex_init(l)
#define NODE_DESTROYLOCK(l)     DESTROYLOCK(l)
#define NODE_LOCK(l, t)         LOCK(l)
#define NODE_UNLOCK(l, t)       UNLOCK(l)
#define NODE_TRYUPGRADE(l)      ISC_R_SUCCESS

#define NODE_STRONGLOCK(l)      LOCK(l)
#define NODE_STRONGUNLOCK(l)    UNLOCK(l)
#define NODE_WEAKLOCK(l, t)     ((void)0)
#define NODE_WEAKUNLOCK(l, t)   ((void)0)
#define NODE_WEAKDOWNGRADE(l)   ((void)0)
#endif

/*%
 * Whether to rate-limit updating the LRU to avoid possible thread contention.
 * Our performance measurement has shown the cost is marginal, so it's defined
 * to be 0 by default either with or without threads.
 */
#ifndef DNS_RBTDB_LIMITLRUUPDATE
#define DNS_RBTDB_LIMITLRUUPDATE 0
#endif

/*
 * Allow clients with a virtual time of up to 5 minutes in the past to see
 * records that would have otherwise have expired.
 */
#define RBTDB_VIRTUAL 300

struct noqname {
	dns_name_t 	name;
	void *     	neg;
	void *     	negsig;
	dns_rdatatype_t	type;
};

typedef struct acachectl acachectl_t;

typedef struct rdatasetheader {
	/*%
	 * Locked by the owning node's lock.
	 */
	rbtdb_serial_t                  serial;
	dns_ttl_t                       rdh_ttl;
	rbtdb_rdatatype_t               type;
	isc_uint16_t                    attributes;
	dns_trust_t                     trust;
	struct noqname                  *noqname;
	struct noqname                  *closest;
	/*%<
	 * We don't use the LIST macros, because the LIST structure has
	 * both head and tail pointers, and is doubly linked.
	 */

	struct rdatasetheader           *next;
	/*%<
	 * If this is the top header for an rdataset, 'next' points
	 * to the top header for the next rdataset (i.e., the next type).
	 * Otherwise, it points up to the header whose down pointer points
	 * at this header.
	 */

	struct rdatasetheader           *down;
	/*%<
	 * Points to the header for the next older version of
	 * this rdataset.
	 */

	isc_uint32_t                    count;
	/*%<
	 * Monotonously increased every time this rdataset is bound so that
	 * it is used as the base of the starting point in DNS responses
	 * when the "cyclic" rrset-order is required.  Since the ordering
	 * should not be so crucial, no lock is set for the counter for
	 * performance reasons.
	 */

	acachectl_t                     *additional_auth;
	acachectl_t                     *additional_glue;

	dns_rbtnode_t                   *node;
	isc_stdtime_t                   last_used;
	ISC_LINK(struct rdatasetheader) lru_link;
	/*%<
	 * Used for LRU-based cache management.  We should probably make
	 * these cache-DB specific.  We might also make it a pointer and
	 * ensure only the top header has a valid link to save memory.
	 * The linked-list is locked by the rbtdb->lrulock.
	 */

	/*
	 * It's possible this should not be here anymore, but instead
	 * referenced from the bucket's heap directly.
	 */
#if 0
	isc_heap_t                      *heap;
#endif
	unsigned int                    heap_index;
	/*%<
	 * Used for TTL-based cache cleaning.
	 */
	isc_stdtime_t                   resign;
} rdatasetheader_t;

typedef ISC_LIST(rdatasetheader_t)      rdatasetheaderlist_t;
typedef ISC_LIST(dns_rbtnode_t)         rbtnodelist_t;

#define RDATASET_ATTR_NONEXISTENT       0x0001
#define RDATASET_ATTR_STALE             0x0002
#define RDATASET_ATTR_IGNORE            0x0004
#define RDATASET_ATTR_RETAIN            0x0008
#define RDATASET_ATTR_NXDOMAIN          0x0010
#define RDATASET_ATTR_RESIGN            0x0020
#define RDATASET_ATTR_STATCOUNT         0x0040
#define RDATASET_ATTR_OPTOUT		0x0080

typedef struct acache_cbarg {
	dns_rdatasetadditional_t        type;
	unsigned int                    count;
	dns_db_t                        *db;
	dns_dbnode_t                    *node;
	rdatasetheader_t                *header;
} acache_cbarg_t;

struct acachectl {
	dns_acacheentry_t               *entry;
	acache_cbarg_t                  *cbarg;
};

/*
 * XXX
 * When the cache will pre-expire data (due to memory low or other
 * situations) before the rdataset's TTL has expired, it MUST
 * respect the RETAIN bit and not expire the data until its TTL is
 * expired.
 */

#undef IGNORE                   /* WIN32 winbase.h defines this. */

#define EXISTS(header) \
	(((header)->attributes & RDATASET_ATTR_NONEXISTENT) == 0)
#define NONEXISTENT(header) \
	(((header)->attributes & RDATASET_ATTR_NONEXISTENT) != 0)
#define IGNORE(header) \
	(((header)->attributes & RDATASET_ATTR_IGNORE) != 0)
#define RETAIN(header) \
	(((header)->attributes & RDATASET_ATTR_RETAIN) != 0)
#define NXDOMAIN(header) \
	(((header)->attributes & RDATASET_ATTR_NXDOMAIN) != 0)
#define RESIGN(header) \
	(((header)->attributes & RDATASET_ATTR_RESIGN) != 0)
#define OPTOUT(header) \
	(((header)->attributes & RDATASET_ATTR_OPTOUT) != 0)

#define DEFAULT_NODE_LOCK_COUNT         7       /*%< Should be prime. */

/*%
 * Number of buckets for cache DB entries (locks, LRU lists, TTL heaps).
 * There is a tradeoff issue about configuring this value: if this is too
 * small, it may cause heavier contention between threads; if this is too large,
 * LRU purge algorithm won't work well (entries tend to be purged prematurely).
 * The default value should work well for most environments, but this can
 * also be configurable at compilation time via the
 * DNS_RBTDB_CACHE_NODE_LOCK_COUNT variable.  This value must be larger than
 * 1 due to the assumption of overmem_purge().
 */
#ifdef DNS_RBTDB_CACHE_NODE_LOCK_COUNT
#if DNS_RBTDB_CACHE_NODE_LOCK_COUNT <= 1
#error "DNS_RBTDB_CACHE_NODE_LOCK_COUNT must be larger than 1"
#else
#define DEFAULT_CACHE_NODE_LOCK_COUNT DNS_RBTDB_CACHE_NODE_LOCK_COUNT
#endif
#else
#define DEFAULT_CACHE_NODE_LOCK_COUNT   16
#endif	/* DNS_RBTDB_CACHE_NODE_LOCK_COUNT */

typedef struct {
	nodelock_t                      lock;
	/* Protected in the refcount routines. */
	isc_refcount_t                  references;
	/* Locked by lock. */
	isc_boolean_t                   exiting;
} rbtdb_nodelock_t;

typedef struct rbtdb_changed {
	dns_rbtnode_t *                 node;
	isc_boolean_t                   dirty;
	ISC_LINK(struct rbtdb_changed)  link;
} rbtdb_changed_t;

typedef ISC_LIST(rbtdb_changed_t)       rbtdb_changedlist_t;

typedef enum {
	dns_db_insecure,
	dns_db_partial,
	dns_db_secure
} dns_db_secure_t;

typedef struct rbtdb_version {
	/* Not locked */
	rbtdb_serial_t                  serial;
	/*
	 * Protected in the refcount routines.
	 * XXXJT: should we change the lock policy based on the refcount
	 * performance?
	 */
	isc_refcount_t                  references;
	/* Locked by database lock. */
	isc_boolean_t                   writer;
	isc_boolean_t                   commit_ok;
	rbtdb_changedlist_t             changed_list;
	rdatasetheaderlist_t		resigned_list;
	ISC_LINK(struct rbtdb_version)  link;
	dns_db_secure_t			secure;
	isc_boolean_t			havensec3;
	/* NSEC3 parameters */
	dns_hash_t			hash;
	isc_uint8_t			flags;
	isc_uint16_t			iterations;
	isc_uint8_t			salt_length;
	unsigned char			salt[NSEC3_MAX_HASH_LENGTH];
} rbtdb_version_t;

typedef ISC_LIST(rbtdb_version_t)       rbtdb_versionlist_t;

typedef struct {
	/* Unlocked. */
	dns_db_t                        common;
#if DNS_RBTDB_USERWLOCK
	isc_rwlock_t                    lock;
#else
	isc_mutex_t                     lock;
#endif
	isc_rwlock_t                    tree_lock;
	unsigned int                    node_lock_count;
	rbtdb_nodelock_t *              node_locks;
	dns_rbtnode_t *                 origin_node;
	dns_stats_t *			rrsetstats; /* cache DB only */
	/* Locked by lock. */
	unsigned int                    active;
	isc_refcount_t                  references;
	unsigned int                    attributes;
	rbtdb_serial_t                  current_serial;
	rbtdb_serial_t                  least_serial;
	rbtdb_serial_t                  next_serial;
	rbtdb_version_t *               current_version;
	rbtdb_version_t *               future_version;
	rbtdb_versionlist_t             open_versions;
	isc_boolean_t                   overmem;
	isc_task_t *                    task;
	dns_dbnode_t                    *soanode;
	dns_dbnode_t                    *nsnode;

	/*
	 * This is a linked list used to implement the LRU cache.  There will
	 * be node_lock_count linked lists here.  Nodes in bucket 1 will be
	 * placed on the linked list rdatasets[1].
	 */
	rdatasetheaderlist_t            *rdatasets;

	/*%
	 * Temporary storage for stale cache nodes and dynamically deleted
	 * nodes that await being cleaned up.
	 */
	rbtnodelist_t                   *deadnodes;

	/*
	 * Heaps.  Each of these is used for TTL based expiry.
	 */
	isc_heap_t                      **heaps;

	/* Locked by tree_lock. */
	dns_rbt_t *                     tree;
	dns_rbt_t *			nsec3;

	/* Unlocked */
	unsigned int                    quantum;
} dns_rbtdb_t;

#define RBTDB_ATTR_LOADED               0x01
#define RBTDB_ATTR_LOADING              0x02

/*%
 * Search Context
 */
typedef struct {
	dns_rbtdb_t *           rbtdb;
	rbtdb_version_t *       rbtversion;
	rbtdb_serial_t          serial;
	unsigned int            options;
	dns_rbtnodechain_t      chain;
	isc_boolean_t           copy_name;
	isc_boolean_t           need_cleanup;
	isc_boolean_t           wild;
	dns_rbtnode_t *         zonecut;
	rdatasetheader_t *      zonecut_rdataset;
	rdatasetheader_t *      zonecut_sigrdataset;
	dns_fixedname_t         zonecut_name;
	isc_stdtime_t           now;
} rbtdb_search_t;

/*%
 * Load Context
 */
typedef struct {
	dns_rbtdb_t *           rbtdb;
	isc_stdtime_t           now;
} rbtdb_load_t;

static void rdataset_disassociate(dns_rdataset_t *rdataset);
static isc_result_t rdataset_first(dns_rdataset_t *rdataset);
static isc_result_t rdataset_next(dns_rdataset_t *rdataset);
static void rdataset_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata);
static void rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target);
static unsigned int rdataset_count(dns_rdataset_t *rdataset);
static isc_result_t rdataset_getnoqname(dns_rdataset_t *rdataset,
					dns_name_t *name,
					dns_rdataset_t *neg,
					dns_rdataset_t *negsig);
static isc_result_t rdataset_getclosest(dns_rdataset_t *rdataset,
					dns_name_t *name,
					dns_rdataset_t *neg,
					dns_rdataset_t *negsig);
static isc_result_t rdataset_getadditional(dns_rdataset_t *rdataset,
					   dns_rdatasetadditional_t type,
					   dns_rdatatype_t qtype,
					   dns_acache_t *acache,
					   dns_zone_t **zonep,
					   dns_db_t **dbp,
					   dns_dbversion_t **versionp,
					   dns_dbnode_t **nodep,
					   dns_name_t *fname,
					   dns_message_t *msg,
					   isc_stdtime_t now);
static isc_result_t rdataset_setadditional(dns_rdataset_t *rdataset,
					   dns_rdatasetadditional_t type,
					   dns_rdatatype_t qtype,
					   dns_acache_t *acache,
					   dns_zone_t *zone,
					   dns_db_t *db,
					   dns_dbversion_t *version,
					   dns_dbnode_t *node,
					   dns_name_t *fname);
static isc_result_t rdataset_putadditional(dns_acache_t *acache,
					   dns_rdataset_t *rdataset,
					   dns_rdatasetadditional_t type,
					   dns_rdatatype_t qtype);
static inline isc_boolean_t need_headerupdate(rdatasetheader_t *header,
					      isc_stdtime_t now);
static void update_header(dns_rbtdb_t *rbtdb, rdatasetheader_t *header,
			  isc_stdtime_t now);
static void expire_header(dns_rbtdb_t *rbtdb, rdatasetheader_t *header,
			  isc_boolean_t tree_locked);
static void overmem_purge(dns_rbtdb_t *rbtdb, unsigned int locknum_start,
			  isc_stdtime_t now, isc_boolean_t tree_locked);
static isc_result_t resign_insert(dns_rbtdb_t *rbtdb, int idx,
				  rdatasetheader_t *newheader);
static void prune_tree(isc_task_t *task, isc_event_t *event);

static dns_rdatasetmethods_t rdataset_methods = {
	rdataset_disassociate,
	rdataset_first,
	rdataset_next,
	rdataset_current,
	rdataset_clone,
	rdataset_count,
	NULL,
	rdataset_getnoqname,
	NULL,
	rdataset_getclosest,
	rdataset_getadditional,
	rdataset_setadditional,
	rdataset_putadditional
};

static void rdatasetiter_destroy(dns_rdatasetiter_t **iteratorp);
static isc_result_t rdatasetiter_first(dns_rdatasetiter_t *iterator);
static isc_result_t rdatasetiter_next(dns_rdatasetiter_t *iterator);
static void rdatasetiter_current(dns_rdatasetiter_t *iterator,
				 dns_rdataset_t *rdataset);

static dns_rdatasetitermethods_t rdatasetiter_methods = {
	rdatasetiter_destroy,
	rdatasetiter_first,
	rdatasetiter_next,
	rdatasetiter_current
};

typedef struct rbtdb_rdatasetiter {
	dns_rdatasetiter_t              common;
	rdatasetheader_t *              current;
} rbtdb_rdatasetiter_t;

static void             dbiterator_destroy(dns_dbiterator_t **iteratorp);
static isc_result_t     dbiterator_first(dns_dbiterator_t *iterator);
static isc_result_t     dbiterator_last(dns_dbiterator_t *iterator);
static isc_result_t     dbiterator_seek(dns_dbiterator_t *iterator,
					dns_name_t *name);
static isc_result_t     dbiterator_prev(dns_dbiterator_t *iterator);
static isc_result_t     dbiterator_next(dns_dbiterator_t *iterator);
static isc_result_t     dbiterator_current(dns_dbiterator_t *iterator,
					   dns_dbnode_t **nodep,
					   dns_name_t *name);
static isc_result_t     dbiterator_pause(dns_dbiterator_t *iterator);
static isc_result_t     dbiterator_origin(dns_dbiterator_t *iterator,
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

#define DELETION_BATCH_MAX 64

/*
 * If 'paused' is ISC_TRUE, then the tree lock is not being held.
 */
typedef struct rbtdb_dbiterator {
	dns_dbiterator_t                common;
	isc_boolean_t                   paused;
	isc_boolean_t                   new_origin;
	isc_rwlocktype_t                tree_locked;
	isc_result_t                    result;
	dns_fixedname_t                 name;
	dns_fixedname_t                 origin;
	dns_rbtnodechain_t              chain;
	dns_rbtnodechain_t		nsec3chain;
	dns_rbtnodechain_t		*current;
	dns_rbtnode_t                   *node;
	dns_rbtnode_t                   *deletions[DELETION_BATCH_MAX];
	int                             delete;
	isc_boolean_t			nsec3only;
	isc_boolean_t			nonsec3;
} rbtdb_dbiterator_t;


#define IS_STUB(rbtdb)  (((rbtdb)->common.attributes & DNS_DBATTR_STUB)  != 0)
#define IS_CACHE(rbtdb) (((rbtdb)->common.attributes & DNS_DBATTR_CACHE) != 0)

static void free_rbtdb(dns_rbtdb_t *rbtdb, isc_boolean_t log,
		       isc_event_t *event);
static void overmem(dns_db_t *db, isc_boolean_t overmem);
static void setnsec3parameters(dns_db_t *db, rbtdb_version_t *version,
			       isc_boolean_t *nsec3createflag);

/*%
 * 'init_count' is used to initialize 'newheader->count' which inturn
 * is used to determine where in the cycle rrset-order cyclic starts.
 * We don't lock this as we don't care about simultaneous updates.
 *
 * Note:
 *      Both init_count and header->count can be ISC_UINT32_MAX.
 *      The count on the returned rdataset however can't be as
 *      that indicates that the database does not implement cyclic
 *      processing.
 */
static unsigned int init_count;

/*
 * Locking
 *
 * If a routine is going to lock more than one lock in this module, then
 * the locking must be done in the following order:
 *
 *      Tree Lock
 *
 *      Node Lock       (Only one from the set may be locked at one time by
 *                       any caller)
 *
 *      Database Lock
 *
 * Failure to follow this hierarchy can result in deadlock.
 */

/*
 * Deleting Nodes
 *
 * For zone databases the node for the origin of the zone MUST NOT be deleted.
 */


/*
 * DB Routines
 */

static void
attach(dns_db_t *source, dns_db_t **targetp) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)source;

	REQUIRE(VALID_RBTDB(rbtdb));

	isc_refcount_increment(&rbtdb->references, NULL);

	*targetp = source;
}

static void
free_rbtdb_callback(isc_task_t *task, isc_event_t *event) {
	dns_rbtdb_t *rbtdb = event->ev_arg;

	UNUSED(task);

	free_rbtdb(rbtdb, ISC_TRUE, event);
}

static void
update_rrsetstats(dns_rbtdb_t *rbtdb, rdatasetheader_t *header,
		  isc_boolean_t increment)
{
	dns_rdatastatstype_t statattributes = 0;
	dns_rdatastatstype_t base = 0;
	dns_rdatastatstype_t type;

	/* At the moment we count statistics only for cache DB */
	INSIST(IS_CACHE(rbtdb));

	if (NXDOMAIN(header))
		statattributes = DNS_RDATASTATSTYPE_ATTR_NXDOMAIN;
	else if (RBTDB_RDATATYPE_BASE(header->type) == 0) {
		statattributes = DNS_RDATASTATSTYPE_ATTR_NXRRSET;
		base = RBTDB_RDATATYPE_EXT(header->type);
	} else
		base = RBTDB_RDATATYPE_BASE(header->type);

	type = DNS_RDATASTATSTYPE_VALUE(base, statattributes);
	if (increment)
		dns_rdatasetstats_increment(rbtdb->rrsetstats, type);
	else
		dns_rdatasetstats_decrement(rbtdb->rrsetstats, type);
}

static void
set_ttl(dns_rbtdb_t *rbtdb, rdatasetheader_t *header, dns_ttl_t newttl) {
	int idx;
	isc_heap_t *heap;
	dns_ttl_t oldttl;

	oldttl = header->rdh_ttl;
	header->rdh_ttl = newttl;

	if (!IS_CACHE(rbtdb))
		return;

	/*
	 * It's possible the rbtdb is not a cache.  If this is the case,
	 * we will not have a heap, and we move on.  If we do, though,
	 * we might need to adjust things.
	 */
	if (header->heap_index == 0 || newttl == oldttl)
		return;
	idx = header->node->locknum;
	if (rbtdb->heaps == NULL || rbtdb->heaps[idx] == NULL)
	    return;
	heap = rbtdb->heaps[idx];

	if (newttl < oldttl)
		isc_heap_increased(heap, header->heap_index);
	else
		isc_heap_decreased(heap, header->heap_index);
}

/*%
 * These functions allow the heap code to rank the priority of each
 * element.  It returns ISC_TRUE if v1 happens "sooner" than v2.
 */
static isc_boolean_t
ttl_sooner(void *v1, void *v2) {
	rdatasetheader_t *h1 = v1;
	rdatasetheader_t *h2 = v2;

	if (h1->rdh_ttl < h2->rdh_ttl)
		return (ISC_TRUE);
	return (ISC_FALSE);
}

static isc_boolean_t
resign_sooner(void *v1, void *v2) {
	rdatasetheader_t *h1 = v1;
	rdatasetheader_t *h2 = v2;

	if (h1->resign < h2->resign)
		return (ISC_TRUE);
	return (ISC_FALSE);
}

/*%
 * This function sets the heap index into the header.
 */
static void
set_index(void *what, unsigned int index) {
	rdatasetheader_t *h = what;

	h->heap_index = index;
}

/*%
 * Work out how many nodes can be deleted in the time between two
 * requests to the nameserver.  Smooth the resulting number and use it
 * as a estimate for the number of nodes to be deleted in the next
 * iteration.
 */
static unsigned int
adjust_quantum(unsigned int old, isc_time_t *start) {
	unsigned int pps = dns_pps;     /* packets per second */
	unsigned int interval;
	isc_uint64_t usecs;
	isc_time_t end;
	unsigned int new;

	if (pps < 100)
		pps = 100;
	isc_time_now(&end);

	interval = 1000000 / pps;       /* interval in usec */
	if (interval == 0)
		interval = 1;
	usecs = isc_time_microdiff(&end, start);
	if (usecs == 0) {
		/*
		 * We were unable to measure the amount of time taken.
		 * Double the nodes deleted next time.
		 */
		old *= 2;
		if (old > 1000)
			old = 1000;
		return (old);
	}
	new = old * interval;
	new /= (unsigned int)usecs;
	if (new == 0)
		new = 1;
	else if (new > 1000)
		new = 1000;

	/* Smooth */
	new = (new + old * 3) / 4;

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_CACHE,
		      ISC_LOG_DEBUG(1), "adjust_quantum -> %d", new);

	return (new);
}

static void
free_rbtdb(dns_rbtdb_t *rbtdb, isc_boolean_t log, isc_event_t *event) {
	unsigned int i;
	isc_ondestroy_t ondest;
	isc_result_t result;
	char buf[DNS_NAME_FORMATSIZE];
	isc_time_t start;

	if (IS_CACHE(rbtdb) && rbtdb->common.rdclass == dns_rdataclass_in)
		overmem((dns_db_t *)rbtdb, (isc_boolean_t)-1);

	REQUIRE(rbtdb->current_version != NULL || EMPTY(rbtdb->open_versions));
	REQUIRE(rbtdb->future_version == NULL);

	if (rbtdb->current_version != NULL) {
		unsigned int refs;

		isc_refcount_decrement(&rbtdb->current_version->references,
				       &refs);
		INSIST(refs == 0);
		UNLINK(rbtdb->open_versions, rbtdb->current_version, link);
		isc_refcount_destroy(&rbtdb->current_version->references);
		isc_mem_put(rbtdb->common.mctx, rbtdb->current_version,
			    sizeof(rbtdb_version_t));
	}

	/*
	 * We assume the number of remaining dead nodes is reasonably small;
	 * the overhead of unlinking all nodes here should be negligible.
	 */
	for (i = 0; i < rbtdb->node_lock_count; i++) {
		dns_rbtnode_t *node;

		node = ISC_LIST_HEAD(rbtdb->deadnodes[i]);
		while (node != NULL) {
			ISC_LIST_UNLINK(rbtdb->deadnodes[i], node, deadlink);
			node = ISC_LIST_HEAD(rbtdb->deadnodes[i]);
		}
	}

	if (event == NULL)
		rbtdb->quantum = (rbtdb->task != NULL) ? 100 : 0;
 again:
	if (rbtdb->tree != NULL) {
		isc_time_now(&start);
		result = dns_rbt_destroy2(&rbtdb->tree, rbtdb->quantum);
		if (result == ISC_R_QUOTA) {
			INSIST(rbtdb->task != NULL);
			if (rbtdb->quantum != 0)
				rbtdb->quantum = adjust_quantum(rbtdb->quantum,
								&start);
			if (event == NULL)
				event = isc_event_allocate(rbtdb->common.mctx,
							   NULL,
							 DNS_EVENT_FREESTORAGE,
							   free_rbtdb_callback,
							   rbtdb,
							   sizeof(isc_event_t));
			if (event == NULL)
				goto again;
			isc_task_send(rbtdb->task, &event);
			return;
		}
		INSIST(result == ISC_R_SUCCESS && rbtdb->tree == NULL);
	}

	if (rbtdb->nsec3 != NULL) {
		isc_time_now(&start);
		result = dns_rbt_destroy2(&rbtdb->nsec3, rbtdb->quantum);
		if (result == ISC_R_QUOTA) {
			INSIST(rbtdb->task != NULL);
			if (rbtdb->quantum != 0)
				rbtdb->quantum = adjust_quantum(rbtdb->quantum,
								&start);
			if (event == NULL)
				event = isc_event_allocate(rbtdb->common.mctx,
							   NULL,
							 DNS_EVENT_FREESTORAGE,
							   free_rbtdb_callback,
							   rbtdb,
							   sizeof(isc_event_t));
			if (event == NULL)
				goto again;
			isc_task_send(rbtdb->task, &event);
			return;
		}
		INSIST(result == ISC_R_SUCCESS && rbtdb->nsec3 == NULL);
	}

	if (event != NULL)
		isc_event_free(&event);
	if (log) {
		if (dns_name_dynamic(&rbtdb->common.origin))
			dns_name_format(&rbtdb->common.origin, buf,
					sizeof(buf));
		else
			strcpy(buf, "<UNKNOWN>");
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_CACHE, ISC_LOG_DEBUG(1),
			      "done free_rbtdb(%s)", buf);
	}
	if (dns_name_dynamic(&rbtdb->common.origin))
		dns_name_free(&rbtdb->common.origin, rbtdb->common.mctx);
	for (i = 0; i < rbtdb->node_lock_count; i++) {
		isc_refcount_destroy(&rbtdb->node_locks[i].references);
		NODE_DESTROYLOCK(&rbtdb->node_locks[i].lock);
	}

	/*
	 * Clean up LRU / re-signing order lists.
	 */
	if (rbtdb->rdatasets != NULL) {
		for (i = 0; i < rbtdb->node_lock_count; i++)
			INSIST(ISC_LIST_EMPTY(rbtdb->rdatasets[i]));
		isc_mem_put(rbtdb->common.mctx, rbtdb->rdatasets,
			    rbtdb->node_lock_count *
			    sizeof(rdatasetheaderlist_t));
	}
	/*
	 * Clean up dead node buckets.
	 */
	if (rbtdb->deadnodes != NULL) {
		for (i = 0; i < rbtdb->node_lock_count; i++)
			INSIST(ISC_LIST_EMPTY(rbtdb->deadnodes[i]));
		isc_mem_put(rbtdb->common.mctx, rbtdb->deadnodes,
		    rbtdb->node_lock_count * sizeof(rbtnodelist_t));
	}
	/*
	 * Clean up heap objects.
	 */
	if (rbtdb->heaps != NULL) {
		for (i = 0; i < rbtdb->node_lock_count; i++)
			isc_heap_destroy(&rbtdb->heaps[i]);
		isc_mem_put(rbtdb->common.mctx, rbtdb->heaps,
			    rbtdb->node_lock_count *
			    sizeof(isc_heap_t *));
	}

	if (rbtdb->rrsetstats != NULL)
		dns_stats_detach(&rbtdb->rrsetstats);

	isc_mem_put(rbtdb->common.mctx, rbtdb->node_locks,
		    rbtdb->node_lock_count * sizeof(rbtdb_nodelock_t));
	isc_rwlock_destroy(&rbtdb->tree_lock);
	isc_refcount_destroy(&rbtdb->references);
	if (rbtdb->task != NULL)
		isc_task_detach(&rbtdb->task);

	RBTDB_DESTROYLOCK(&rbtdb->lock);
	rbtdb->common.magic = 0;
	rbtdb->common.impmagic = 0;
	ondest = rbtdb->common.ondest;
	isc_mem_putanddetach(&rbtdb->common.mctx, rbtdb, sizeof(*rbtdb));
	isc_ondestroy_notify(&ondest, rbtdb);
}

static inline void
maybe_free_rbtdb(dns_rbtdb_t *rbtdb) {
	isc_boolean_t want_free = ISC_FALSE;
	unsigned int i;
	unsigned int inactive = 0;

	/* XXX check for open versions here */

	if (rbtdb->soanode != NULL)
		dns_db_detachnode((dns_db_t *)rbtdb, &rbtdb->soanode);
	if (rbtdb->nsnode != NULL)
		dns_db_detachnode((dns_db_t *)rbtdb, &rbtdb->nsnode);

	/*
	 * Even though there are no external direct references, there still
	 * may be nodes in use.
	 */
	for (i = 0; i < rbtdb->node_lock_count; i++) {
		NODE_LOCK(&rbtdb->node_locks[i].lock, isc_rwlocktype_write);
		rbtdb->node_locks[i].exiting = ISC_TRUE;
		NODE_UNLOCK(&rbtdb->node_locks[i].lock, isc_rwlocktype_write);
		if (isc_refcount_current(&rbtdb->node_locks[i].references)
		    == 0) {
			inactive++;
		}
	}

	if (inactive != 0) {
		RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_write);
		rbtdb->active -= inactive;
		if (rbtdb->active == 0)
			want_free = ISC_TRUE;
		RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_write);
		if (want_free) {
			char buf[DNS_NAME_FORMATSIZE];
			if (dns_name_dynamic(&rbtdb->common.origin))
				dns_name_format(&rbtdb->common.origin, buf,
						sizeof(buf));
			else
				strcpy(buf, "<UNKNOWN>");
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
				      DNS_LOGMODULE_CACHE, ISC_LOG_DEBUG(1),
				      "calling free_rbtdb(%s)", buf);
			free_rbtdb(rbtdb, ISC_TRUE, NULL);
		}
	}
}

static void
detach(dns_db_t **dbp) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)(*dbp);
	unsigned int refs;

	REQUIRE(VALID_RBTDB(rbtdb));

	isc_refcount_decrement(&rbtdb->references, &refs);

	if (refs == 0)
		maybe_free_rbtdb(rbtdb);

	*dbp = NULL;
}

static void
currentversion(dns_db_t *db, dns_dbversion_t **versionp) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	rbtdb_version_t *version;
	unsigned int refs;

	REQUIRE(VALID_RBTDB(rbtdb));

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_read);
	version = rbtdb->current_version;
	isc_refcount_increment(&version->references, &refs);
	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_read);

	*versionp = (dns_dbversion_t *)version;
}

static inline rbtdb_version_t *
allocate_version(isc_mem_t *mctx, rbtdb_serial_t serial,
		 unsigned int references, isc_boolean_t writer)
{
	isc_result_t result;
	rbtdb_version_t *version;

	version = isc_mem_get(mctx, sizeof(*version));
	if (version == NULL)
		return (NULL);
	version->serial = serial;
	result = isc_refcount_init(&version->references, references);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, version, sizeof(*version));
		return (NULL);
	}
	version->writer = writer;
	version->commit_ok = ISC_FALSE;
	ISC_LIST_INIT(version->changed_list);
	ISC_LIST_INIT(version->resigned_list);
	ISC_LINK_INIT(version, link);

	return (version);
}

static isc_result_t
newversion(dns_db_t *db, dns_dbversion_t **versionp) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	rbtdb_version_t *version;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(versionp != NULL && *versionp == NULL);
	REQUIRE(rbtdb->future_version == NULL);

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_write);
	RUNTIME_CHECK(rbtdb->next_serial != 0);         /* XXX Error? */
	version = allocate_version(rbtdb->common.mctx, rbtdb->next_serial, 1,
				   ISC_TRUE);
	if (version != NULL) {
		version->commit_ok = ISC_TRUE;
		version->secure = rbtdb->current_version->secure;
		version->havensec3 = rbtdb->current_version->havensec3;
		if (version->havensec3) {
			version->flags = rbtdb->current_version->flags;
			version->iterations =
				rbtdb->current_version->iterations;
			version->hash = rbtdb->current_version->hash;
			version->salt_length =
				rbtdb->current_version->salt_length;
			memcpy(version->salt, rbtdb->current_version->salt,
			       version->salt_length);
		} else {
			version->flags = 0;
			version->iterations = 0;
			version->hash = 0;
			version->salt_length = 0;
			memset(version->salt, 0, sizeof(version->salt));
		}
		rbtdb->next_serial++;
		rbtdb->future_version = version;
	}
	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_write);

	if (version == NULL)
		return (ISC_R_NOMEMORY);

	*versionp = version;

	return (ISC_R_SUCCESS);
}

static void
attachversion(dns_db_t *db, dns_dbversion_t *source,
	      dns_dbversion_t **targetp)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	rbtdb_version_t *rbtversion = source;
	unsigned int refs;

	REQUIRE(VALID_RBTDB(rbtdb));

	isc_refcount_increment(&rbtversion->references, &refs);
	INSIST(refs > 1);

	*targetp = rbtversion;
}

static rbtdb_changed_t *
add_changed(dns_rbtdb_t *rbtdb, rbtdb_version_t *version,
	    dns_rbtnode_t *node)
{
	rbtdb_changed_t *changed;
	unsigned int refs;

	/*
	 * Caller must be holding the node lock if its reference must be
	 * protected by the lock.
	 */

	changed = isc_mem_get(rbtdb->common.mctx, sizeof(*changed));

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_write);

	REQUIRE(version->writer);

	if (changed != NULL) {
		dns_rbtnode_refincrement(node, &refs);
		INSIST(refs != 0);
		changed->node = node;
		changed->dirty = ISC_FALSE;
		ISC_LIST_INITANDAPPEND(version->changed_list, changed, link);
	} else
		version->commit_ok = ISC_FALSE;

	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_write);

	return (changed);
}

static void
free_acachearray(isc_mem_t *mctx, rdatasetheader_t *header,
		 acachectl_t *array)
{
	unsigned int count;
	unsigned int i;
	unsigned char *raw;     /* RDATASLAB */

	/*
	 * The caller must be holding the corresponding node lock.
	 */

	if (array == NULL)
		return;

	raw = (unsigned char *)header + sizeof(*header);
	count = raw[0] * 256 + raw[1];

	/*
	 * Sanity check: since an additional cache entry has a reference to
	 * the original DB node (in the callback arg), there should be no
	 * acache entries when the node can be freed.
	 */
	for (i = 0; i < count; i++)
		INSIST(array[i].entry == NULL && array[i].cbarg == NULL);

	isc_mem_put(mctx, array, count * sizeof(acachectl_t));
}

static inline void
free_noqname(isc_mem_t *mctx, struct noqname **noqname) {

	if (dns_name_dynamic(&(*noqname)->name))
		dns_name_free(&(*noqname)->name, mctx);
	if ((*noqname)->neg != NULL)
		isc_mem_put(mctx, (*noqname)->neg,
			    dns_rdataslab_size((*noqname)->neg, 0));
	if ((*noqname)->negsig != NULL)
		isc_mem_put(mctx, (*noqname)->negsig,
			    dns_rdataslab_size((*noqname)->negsig, 0));
	isc_mem_put(mctx, *noqname, sizeof(**noqname));
	*noqname = NULL;
}

static inline void
init_rdataset(dns_rbtdb_t *rbtdb, rdatasetheader_t *h)
{
	ISC_LINK_INIT(h, lru_link);
	h->heap_index = 0;

#if TRACE_HEADER
	if (IS_CACHE(rbtdb) && rbtdb->common.rdclass == dns_rdataclass_in)
		fprintf(stderr, "initialized header: %p\n", h);
#else
	UNUSED(rbtdb);
#endif
}

static inline rdatasetheader_t *
new_rdataset(dns_rbtdb_t *rbtdb, isc_mem_t *mctx)
{
	rdatasetheader_t *h;

	h = isc_mem_get(mctx, sizeof(*h));
	if (h == NULL)
		return (NULL);

#if TRACE_HEADER
	if (IS_CACHE(rbtdb) && rbtdb->common.rdclass == dns_rdataclass_in)
		fprintf(stderr, "allocated header: %p\n", h);
#endif
	init_rdataset(rbtdb, h);
	return (h);
}

static inline void
free_rdataset(dns_rbtdb_t *rbtdb, isc_mem_t *mctx, rdatasetheader_t *rdataset)
{
	unsigned int size;
	int idx;

	if (EXISTS(rdataset) &&
	    (rdataset->attributes & RDATASET_ATTR_STATCOUNT) != 0) {
		update_rrsetstats(rbtdb, rdataset, ISC_FALSE);
	}

	idx = rdataset->node->locknum;
	if (ISC_LINK_LINKED(rdataset, lru_link))
		ISC_LIST_UNLINK(rbtdb->rdatasets[idx], rdataset, lru_link);
	if (rdataset->heap_index != 0)
		isc_heap_delete(rbtdb->heaps[idx], rdataset->heap_index);
	rdataset->heap_index = 0;

	if (rdataset->noqname != NULL)
		free_noqname(mctx, &rdataset->noqname);
	if (rdataset->closest != NULL)
		free_noqname(mctx, &rdataset->closest);

	free_acachearray(mctx, rdataset, rdataset->additional_auth);
	free_acachearray(mctx, rdataset, rdataset->additional_glue);

	if ((rdataset->attributes & RDATASET_ATTR_NONEXISTENT) != 0)
		size = sizeof(*rdataset);
	else
		size = dns_rdataslab_size((unsigned char *)rdataset,
					  sizeof(*rdataset));
	isc_mem_put(mctx, rdataset, size);
}

static inline void
rollback_node(dns_rbtnode_t *node, rbtdb_serial_t serial) {
	rdatasetheader_t *header, *dcurrent;
	isc_boolean_t make_dirty = ISC_FALSE;

	/*
	 * Caller must hold the node lock.
	 */

	/*
	 * We set the IGNORE attribute on rdatasets with serial number
	 * 'serial'.  When the reference count goes to zero, these rdatasets
	 * will be cleaned up; until that time, they will be ignored.
	 */
	for (header = node->data; header != NULL; header = header->next) {
		if (header->serial == serial) {
			header->attributes |= RDATASET_ATTR_IGNORE;
			make_dirty = ISC_TRUE;
		}
		for (dcurrent = header->down;
		     dcurrent != NULL;
		     dcurrent = dcurrent->down) {
			if (dcurrent->serial == serial) {
				dcurrent->attributes |= RDATASET_ATTR_IGNORE;
				make_dirty = ISC_TRUE;
			}
		}
	}
	if (make_dirty)
		node->dirty = 1;
}

static inline void
clean_stale_headers(dns_rbtdb_t *rbtdb, isc_mem_t *mctx, rdatasetheader_t *top)
{
	rdatasetheader_t *d, *down_next;

	for (d = top->down; d != NULL; d = down_next) {
		down_next = d->down;
		free_rdataset(rbtdb, mctx, d);
	}
	top->down = NULL;
}

static inline void
clean_cache_node(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node) {
	rdatasetheader_t *current, *top_prev, *top_next;
	isc_mem_t *mctx = rbtdb->common.mctx;

	/*
	 * Caller must be holding the node lock.
	 */

	top_prev = NULL;
	for (current = node->data; current != NULL; current = top_next) {
		top_next = current->next;
		clean_stale_headers(rbtdb, mctx, current);
		/*
		 * If current is nonexistent or stale, we can clean it up.
		 */
		if ((current->attributes &
		     (RDATASET_ATTR_NONEXISTENT|RDATASET_ATTR_STALE)) != 0) {
			if (top_prev != NULL)
				top_prev->next = current->next;
			else
				node->data = current->next;
			free_rdataset(rbtdb, mctx, current);
		} else
			top_prev = current;
	}
	node->dirty = 0;
}

static inline void
clean_zone_node(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node,
		rbtdb_serial_t least_serial)
{
	rdatasetheader_t *current, *dcurrent, *down_next, *dparent;
	rdatasetheader_t *top_prev, *top_next;
	isc_mem_t *mctx = rbtdb->common.mctx;
	isc_boolean_t still_dirty = ISC_FALSE;

	/*
	 * Caller must be holding the node lock.
	 */
	REQUIRE(least_serial != 0);

	top_prev = NULL;
	for (current = node->data; current != NULL; current = top_next) {
		top_next = current->next;

		/*
		 * First, we clean up any instances of multiple rdatasets
		 * with the same serial number, or that have the IGNORE
		 * attribute.
		 */
		dparent = current;
		for (dcurrent = current->down;
		     dcurrent != NULL;
		     dcurrent = down_next) {
			down_next = dcurrent->down;
			INSIST(dcurrent->serial <= dparent->serial);
			if (dcurrent->serial == dparent->serial ||
			    IGNORE(dcurrent)) {
				if (down_next != NULL)
					down_next->next = dparent;
				dparent->down = down_next;
				free_rdataset(rbtdb, mctx, dcurrent);
			} else
				dparent = dcurrent;
		}

		/*
		 * We've now eliminated all IGNORE datasets with the possible
		 * exception of current, which we now check.
		 */
		if (IGNORE(current)) {
			down_next = current->down;
			if (down_next == NULL) {
				if (top_prev != NULL)
					top_prev->next = current->next;
				else
					node->data = current->next;
				free_rdataset(rbtdb, mctx, current);
				/*
				 * current no longer exists, so we can
				 * just continue with the loop.
				 */
				continue;
			} else {
				/*
				 * Pull up current->down, making it the new
				 * current.
				 */
				if (top_prev != NULL)
					top_prev->next = down_next;
				else
					node->data = down_next;
				down_next->next = top_next;
				free_rdataset(rbtdb, mctx, current);
				current = down_next;
			}
		}

		/*
		 * We now try to find the first down node less than the
		 * least serial.
		 */
		dparent = current;
		for (dcurrent = current->down;
		     dcurrent != NULL;
		     dcurrent = down_next) {
			down_next = dcurrent->down;
			if (dcurrent->serial < least_serial)
				break;
			dparent = dcurrent;
		}

		/*
		 * If there is a such an rdataset, delete it and any older
		 * versions.
		 */
		if (dcurrent != NULL) {
			do {
				down_next = dcurrent->down;
				INSIST(dcurrent->serial <= least_serial);
				free_rdataset(rbtdb, mctx, dcurrent);
				dcurrent = down_next;
			} while (dcurrent != NULL);
			dparent->down = NULL;
		}

		/*
		 * Note.  The serial number of 'current' might be less than
		 * least_serial too, but we cannot delete it because it is
		 * the most recent version, unless it is a NONEXISTENT
		 * rdataset.
		 */
		if (current->down != NULL) {
			still_dirty = ISC_TRUE;
			top_prev = current;
		} else {
			/*
			 * If this is a NONEXISTENT rdataset, we can delete it.
			 */
			if (NONEXISTENT(current)) {
				if (top_prev != NULL)
					top_prev->next = current->next;
				else
					node->data = current->next;
				free_rdataset(rbtdb, mctx, current);
			} else
				top_prev = current;
		}
	}
	if (!still_dirty)
		node->dirty = 0;
}

/*%
 * Clean up dead nodes.  These are nodes which have no references, and
 * have no data.  They are dead but we could not or chose not to delete
 * them when we deleted all the data at that node because we did not want
 * to wait for the tree write lock.
 *
 * The caller must hold a tree write lock and bucketnum'th node (write) lock.
 */
static void
cleanup_dead_nodes(dns_rbtdb_t *rbtdb, int bucketnum) {
	dns_rbtnode_t *node;
	isc_result_t result;
	int count = 10;         /* XXXJT: should be adjustable */

	node = ISC_LIST_HEAD(rbtdb->deadnodes[bucketnum]);
	while (node != NULL && count > 0) {
		ISC_LIST_UNLINK(rbtdb->deadnodes[bucketnum], node, deadlink);

		/*
		 * Since we're holding a tree write lock, it should be
		 * impossible for this node to be referenced by others.
		 */
		INSIST(dns_rbtnode_refcurrent(node) == 0 &&
		       node->data == NULL);

		INSIST(!ISC_LINK_LINKED(node, deadlink));
		if (node->nsec3)
			result = dns_rbt_deletenode(rbtdb->nsec3, node,
						    ISC_FALSE);
		else
			result = dns_rbt_deletenode(rbtdb->tree, node,
						    ISC_FALSE);
		if (result != ISC_R_SUCCESS)
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
				      DNS_LOGMODULE_CACHE, ISC_LOG_WARNING,
				      "cleanup_dead_nodes: "
				      "dns_rbt_deletenode: %s",
				      isc_result_totext(result));
		node = ISC_LIST_HEAD(rbtdb->deadnodes[bucketnum]);
		count--;
	}
}

/*
 * Caller must be holding the node lock if its reference must be protected
 * by the lock.
 */
static inline void
new_reference(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node) {
	unsigned int lockrefs, noderefs;
	isc_refcount_t *lockref;

	dns_rbtnode_refincrement0(node, &noderefs);
	if (noderefs == 1) {    /* this is the first reference to the node */
		lockref = &rbtdb->node_locks[node->locknum].references;
		isc_refcount_increment0(lockref, &lockrefs);
		INSIST(lockrefs != 0);
	}
	INSIST(noderefs != 0);
}

/*
 * This function is assumed to be called when a node is newly referenced
 * and can be in the deadnode list.  In that case the node must be retrieved
 * from the list because it is going to be used.  In addition, if the caller
 * happens to hold a write lock on the tree, it's a good chance to purge dead
 * nodes.
 * Note: while a new reference is gained in multiple places, there are only very
 * few cases where the node can be in the deadnode list (only empty nodes can
 * have been added to the list).
 */
static inline void
reactivate_node(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node,
		isc_rwlocktype_t treelocktype)
{
	isc_boolean_t need_relock = ISC_FALSE;

	NODE_STRONGLOCK(&rbtdb->node_locks[node->locknum].lock);
	new_reference(rbtdb, node);

	NODE_WEAKLOCK(&rbtdb->node_locks[node->locknum].lock,
		      isc_rwlocktype_read);
	if (ISC_LINK_LINKED(node, deadlink))
		need_relock = ISC_TRUE;
	else if (!ISC_LIST_EMPTY(rbtdb->deadnodes[node->locknum]) &&
		 treelocktype == isc_rwlocktype_write)
		need_relock = ISC_TRUE;
	NODE_WEAKUNLOCK(&rbtdb->node_locks[node->locknum].lock,
			isc_rwlocktype_read);
	if (need_relock) {
		NODE_WEAKLOCK(&rbtdb->node_locks[node->locknum].lock,
			      isc_rwlocktype_write);
		if (ISC_LINK_LINKED(node, deadlink))
			ISC_LIST_UNLINK(rbtdb->deadnodes[node->locknum],
					node, deadlink);
		if (treelocktype == isc_rwlocktype_write)
			cleanup_dead_nodes(rbtdb, node->locknum);
		NODE_WEAKUNLOCK(&rbtdb->node_locks[node->locknum].lock,
				isc_rwlocktype_write);
	}

	NODE_STRONGUNLOCK(&rbtdb->node_locks[node->locknum].lock);
}

/*
 * Caller must be holding the node lock; either the "strong", read or write
 * lock.  Note that the lock must be held even when node references are
 * atomically modified; in that case the decrement operation itself does not
 * have to be protected, but we must avoid a race condition where multiple
 * threads are decreasing the reference to zero simultaneously and at least
 * one of them is going to free the node.
 * This function returns ISC_TRUE if and only if the node reference decreases
 * to zero.
 */
static isc_boolean_t
decrement_reference(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node,
		    rbtdb_serial_t least_serial,
		    isc_rwlocktype_t nlock, isc_rwlocktype_t tlock,
		    isc_boolean_t pruning)
{
	isc_result_t result;
	isc_boolean_t write_locked;
	rbtdb_nodelock_t *nodelock;
	unsigned int refs, nrefs;
	int bucket = node->locknum;
	isc_boolean_t no_reference;

	nodelock = &rbtdb->node_locks[bucket];

	/* Handle easy and typical case first. */
	if (!node->dirty && (node->data != NULL || node->down != NULL)) {
		dns_rbtnode_refdecrement(node, &nrefs);
		INSIST((int)nrefs >= 0);
		if (nrefs == 0) {
			isc_refcount_decrement(&nodelock->references, &refs);
			INSIST((int)refs >= 0);
		}
		return ((nrefs == 0) ? ISC_TRUE : ISC_FALSE);
	}

	/* Upgrade the lock? */
	if (nlock == isc_rwlocktype_read) {
		NODE_WEAKUNLOCK(&nodelock->lock, isc_rwlocktype_read);
		NODE_WEAKLOCK(&nodelock->lock, isc_rwlocktype_write);
	}
	dns_rbtnode_refdecrement(node, &nrefs);
	INSIST((int)nrefs >= 0);
	if (nrefs > 0) {
		/* Restore the lock? */
		if (nlock == isc_rwlocktype_read)
			NODE_WEAKDOWNGRADE(&nodelock->lock);
		return (ISC_FALSE);
	}

	if (node->dirty && dns_rbtnode_refcurrent(node) == 0) {
		if (IS_CACHE(rbtdb))
			clean_cache_node(rbtdb, node);
		else {
			if (least_serial == 0) {
				/*
				 * Caller doesn't know the least serial.
				 * Get it.
				 */
				RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_read);
				least_serial = rbtdb->least_serial;
				RBTDB_UNLOCK(&rbtdb->lock,
					     isc_rwlocktype_read);
			}
			clean_zone_node(rbtdb, node, least_serial);
		}
	}

	isc_refcount_decrement(&nodelock->references, &refs);
	INSIST((int)refs >= 0);

	/*
	 * XXXDCL should this only be done for cache zones?
	 */
	if (node->data != NULL || node->down != NULL) {
		/* Restore the lock? */
		if (nlock == isc_rwlocktype_read)
			NODE_WEAKDOWNGRADE(&nodelock->lock);
		return (ISC_TRUE);
	}

	/*
	 * Attempt to switch to a write lock on the tree.  If this fails,
	 * we will add this node to a linked list of nodes in this locking
	 * bucket which we will free later.
	 */
	if (tlock != isc_rwlocktype_write) {
		/*
		 * Locking hierarchy notwithstanding, we don't need to free
		 * the node lock before acquiring the tree write lock because
		 * we only do a trylock.
		 */
		if (tlock == isc_rwlocktype_read)
			result = isc_rwlock_tryupgrade(&rbtdb->tree_lock);
		else
			result = isc_rwlock_trylock(&rbtdb->tree_lock,
						    isc_rwlocktype_write);
		RUNTIME_CHECK(result == ISC_R_SUCCESS ||
			      result == ISC_R_LOCKBUSY);

		write_locked = ISC_TF(result == ISC_R_SUCCESS);
	} else
		write_locked = ISC_TRUE;

	no_reference = ISC_TRUE;
	if (write_locked && dns_rbtnode_refcurrent(node) == 0) {
		/*
		 * We can now delete the node if the reference counter is
		 * zero.  This should be typically the case, but a different
		 * thread may still gain a (new) reference just before the
		 * current thread locks the tree (e.g., in findnode()).
		 */

		/*
		 * If this node is the only one in the level it's in, deleting
		 * this node may recursively make its parent the only node in
		 * the parent level; if so, and if no one is currently using
		 * the parent node, this is almost the only opportunity to
		 * clean it up.  But the recursive cleanup is not that trivial
		 * since the child and parent may be in different lock buckets,
		 * which would cause a lock order reversal problem.  To avoid
		 * the trouble, we'll dispatch a separate event for batch
		 * cleaning.  We need to check whether we're deleting the node
		 * as a result of pruning to avoid infinite dispatching.
		 * Note: pruning happens only when a task has been set for the
		 * rbtdb.  If the user of the rbtdb chooses not to set a task,
		 * it's their responsibility to purge stale leaves (e.g. by
		 * periodic walk-through).
		 */
		if (!pruning && node->parent != NULL &&
		    node->parent->down == node && node->left == NULL &&
		    node->right == NULL && rbtdb->task != NULL) {
			isc_event_t *ev;
			dns_db_t *db;

			ev = isc_event_allocate(rbtdb->common.mctx, NULL,
						DNS_EVENT_RBTPRUNE,
						prune_tree, node,
						sizeof(isc_event_t));
			if (ev != NULL) {
				new_reference(rbtdb, node);
				db = NULL;
				attach((dns_db_t *)rbtdb, &db);
				ev->ev_sender = db;
				isc_task_send(rbtdb->task, &ev);
				no_reference = ISC_FALSE;
			} else {
				/*
				 * XXX: this is a weird situation.  We could
				 * ignore this error case, but then the stale
				 * node will unlikely be purged except via a
				 * rare condition such as manual cleanup.  So
				 * we queue it in the deadnodes list, hoping
				 * the memory shortage is temporary and the node
				 * will be deleted later.
				 */
				isc_log_write(dns_lctx,
					      DNS_LOGCATEGORY_DATABASE,
					      DNS_LOGMODULE_CACHE,
					      ISC_LOG_INFO,
					      "decrement_reference: failed to "
					      "allocate pruning event");
				INSIST(!ISC_LINK_LINKED(node, deadlink));
				ISC_LIST_APPEND(rbtdb->deadnodes[bucket], node,
						deadlink);
			}
		} else {
			if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
				char printname[DNS_NAME_FORMATSIZE];

				isc_log_write(dns_lctx,
					      DNS_LOGCATEGORY_DATABASE,
					      DNS_LOGMODULE_CACHE,
					      ISC_LOG_DEBUG(1),
					      "decrement_reference: "
					      "delete from rbt: %p %s",
					      node,
					      dns_rbt_formatnodename(node,
							printname,
							sizeof(printname)));
			}

			INSIST(!ISC_LINK_LINKED(node, deadlink));
			if (node->nsec3)
				result = dns_rbt_deletenode(rbtdb->nsec3, node,
							    ISC_FALSE);
			else
				result = dns_rbt_deletenode(rbtdb->tree, node,
							    ISC_FALSE);
			if (result != ISC_R_SUCCESS) {
				isc_log_write(dns_lctx,
					      DNS_LOGCATEGORY_DATABASE,
					      DNS_LOGMODULE_CACHE,
					      ISC_LOG_WARNING,
					      "decrement_reference: "
					      "dns_rbt_deletenode: %s",
					      isc_result_totext(result));
			}
		}
	} else if (dns_rbtnode_refcurrent(node) == 0) {
		INSIST(!ISC_LINK_LINKED(node, deadlink));
		ISC_LIST_APPEND(rbtdb->deadnodes[bucket], node, deadlink);
	} else
		no_reference = ISC_FALSE;

	/* Restore the lock? */
	if (nlock == isc_rwlocktype_read)
		NODE_WEAKDOWNGRADE(&nodelock->lock);

	/*
	 * Relock a read lock, or unlock the write lock if no lock was held.
	 */
	if (tlock == isc_rwlocktype_none)
		if (write_locked)
			RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);

	if (tlock == isc_rwlocktype_read)
		if (write_locked)
			isc_rwlock_downgrade(&rbtdb->tree_lock);

	return (no_reference);
}

/*
 * Prune the tree by recursively cleaning-up single leaves.  In the worst
 * case, the number of iteration is the number of tree levels, which is at
 * most the maximum number of domain name labels, i.e, 127.  In practice, this
 * should be much smaller (only a few times), and even the worst case would be
 * acceptable for a single event.
 */
static void
prune_tree(isc_task_t *task, isc_event_t *event) {
	dns_rbtdb_t *rbtdb = event->ev_sender;
	dns_rbtnode_t *node = event->ev_arg;
	dns_rbtnode_t *parent;
	unsigned int locknum;

	UNUSED(task);

	isc_event_free(&event);

	RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);
	locknum = node->locknum;
	NODE_LOCK(&rbtdb->node_locks[locknum].lock, isc_rwlocktype_write);
	do {
		parent = node->parent;
		decrement_reference(rbtdb, node, 0, isc_rwlocktype_write,
				    isc_rwlocktype_write, ISC_TRUE);

		if (parent != NULL && parent->down == NULL) {
			/*
			 * node was the only down child of the parent and has
			 * just been removed.  We'll then need to examine the
			 * parent.  Keep the lock if possible; otherwise,
			 * release the old lock and acquire one for the parent.
			 */
			if (parent->locknum != locknum) {
				NODE_UNLOCK(&rbtdb->node_locks[locknum].lock,
					    isc_rwlocktype_write);
				locknum = parent->locknum;
				NODE_LOCK(&rbtdb->node_locks[locknum].lock,
					  isc_rwlocktype_write);
			}

			/*
			 * We need to gain a reference to the node before
			 * decrementing it in the next iteration.  In addition,
			 * if the node is in the dead-nodes list, extract it
			 * from the list beforehand as we do in
			 * reactivate_node().
			 */
			new_reference(rbtdb, parent);
			if (ISC_LINK_LINKED(parent, deadlink)) {
				ISC_LIST_UNLINK(rbtdb->deadnodes[locknum],
						parent, deadlink);
			}
		} else
			parent = NULL;

		node = parent;
	} while (node != NULL);
	NODE_UNLOCK(&rbtdb->node_locks[locknum].lock, isc_rwlocktype_write);
	RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);

	detach((dns_db_t **)&rbtdb);
}

static inline void
make_least_version(dns_rbtdb_t *rbtdb, rbtdb_version_t *version,
		   rbtdb_changedlist_t *cleanup_list)
{
	/*
	 * Caller must be holding the database lock.
	 */

	rbtdb->least_serial = version->serial;
	*cleanup_list = version->changed_list;
	ISC_LIST_INIT(version->changed_list);
}

static inline void
cleanup_nondirty(rbtdb_version_t *version, rbtdb_changedlist_t *cleanup_list) {
	rbtdb_changed_t *changed, *next_changed;

	/*
	 * If the changed record is dirty, then
	 * an update created multiple versions of
	 * a given rdataset.  We keep this list
	 * until we're the least open version, at
	 * which point it's safe to get rid of any
	 * older versions.
	 *
	 * If the changed record isn't dirty, then
	 * we don't need it anymore since we're
	 * committing and not rolling back.
	 *
	 * The caller must be holding the database lock.
	 */
	for (changed = HEAD(version->changed_list);
	     changed != NULL;
	     changed = next_changed) {
		next_changed = NEXT(changed, link);
		if (!changed->dirty) {
			UNLINK(version->changed_list,
			       changed, link);
			APPEND(*cleanup_list,
			       changed, link);
		}
	}
}

static void
iszonesecure(dns_db_t *db, rbtdb_version_t *version, dns_dbnode_t *origin) {
	dns_rdataset_t keyset;
	dns_rdataset_t nsecset, signsecset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_boolean_t haszonekey = ISC_FALSE;
	isc_boolean_t hasnsec = ISC_FALSE;
	isc_boolean_t hasoptbit = ISC_FALSE;
	isc_boolean_t nsec3createflag = ISC_FALSE;
	isc_result_t result;

	dns_rdataset_init(&keyset);
	result = dns_db_findrdataset(db, origin, version, dns_rdatatype_dnskey,
				     0, 0, &keyset, NULL);
	if (result == ISC_R_SUCCESS) {
		dns_rdata_t keyrdata = DNS_RDATA_INIT;
		result = dns_rdataset_first(&keyset);
		while (result == ISC_R_SUCCESS) {
			dns_rdataset_current(&keyset, &keyrdata);
			if (dns_zonekey_iszonekey(&keyrdata)) {
				haszonekey = ISC_TRUE;
				break;
			}
			result = dns_rdataset_next(&keyset);
		}
		dns_rdataset_disassociate(&keyset);
	}
	if (!haszonekey) {
		version->secure = dns_db_insecure;
		version->havensec3 = ISC_FALSE;
		return;
	}

	dns_rdataset_init(&nsecset);
	dns_rdataset_init(&signsecset);
	result = dns_db_findrdataset(db, origin, version, dns_rdatatype_nsec,
				     0, 0, &nsecset, &signsecset);
	if (result == ISC_R_SUCCESS) {
		if (dns_rdataset_isassociated(&signsecset)) {
			hasnsec = ISC_TRUE;
			result = dns_rdataset_first(&nsecset);
			if (result == ISC_R_SUCCESS) {
				dns_rdataset_current(&nsecset, &rdata);
				hasoptbit = dns_nsec_typepresent(&rdata,
							     dns_rdatatype_opt);
			}
			dns_rdataset_disassociate(&signsecset);
		}
		dns_rdataset_disassociate(&nsecset);
	}

	setnsec3parameters(db, version, &nsec3createflag);

	/*
	 * Do we have a valid NSEC/NSEC3 chain?
	 */
	if (version->havensec3 || (hasnsec && !hasoptbit))
		version->secure = dns_db_secure;
	/*
	 * Do we have a NSEC/NSEC3 chain under creation?
	 */
	else if (hasoptbit || nsec3createflag)
		version->secure = dns_db_partial;
	else
		version->secure = dns_db_insecure;
}

/*%<
 * Walk the origin node looking for NSEC3PARAM records.
 * Cache the nsec3 parameters.
 */
static void
setnsec3parameters(dns_db_t *db, rbtdb_version_t *version,
		   isc_boolean_t *nsec3createflag)
{
	dns_rbtnode_t *node;
	dns_rdata_nsec3param_t nsec3param;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_region_t region;
	isc_result_t result;
	rdatasetheader_t *header, *header_next;
	unsigned char *raw;             /* RDATASLAB */
	unsigned int count, length;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;

	RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
	version->havensec3 = ISC_FALSE;
	node = rbtdb->origin_node;
	NODE_LOCK(&(rbtdb->node_locks[node->locknum].lock),
		  isc_rwlocktype_read);
	for (header = node->data;
	     header != NULL;
	     header = header_next) {
		header_next = header->next;
		do {
			if (header->serial <= version->serial &&
			    !IGNORE(header)) {
				if (NONEXISTENT(header))
					header = NULL;
				break;
			} else
				header = header->down;
		} while (header != NULL);

		if (header != NULL &&
		    header->type == dns_rdatatype_nsec3param) {
			/*
			 * Find A NSEC3PARAM with a supported algorithm.
			 */
			raw = (unsigned char *)header + sizeof(*header);
			count = raw[0] * 256 + raw[1]; /* count */
#if DNS_RDATASET_FIXED
			raw += count * 4 + 2;
#else
			raw += 2;
#endif
			while (count-- > 0U) {
				length = raw[0] * 256 + raw[1];
#if DNS_RDATASET_FIXED
				raw += 4;
#else
				raw += 2;
#endif
				region.base = raw;
				region.length = length;
				raw += length;
				dns_rdata_fromregion(&rdata,
						     rbtdb->common.rdclass,
						     dns_rdatatype_nsec3param,
						     &region);
				result = dns_rdata_tostruct(&rdata,
							    &nsec3param,
							    NULL);
				INSIST(result == ISC_R_SUCCESS);
				dns_rdata_reset(&rdata);

				if (nsec3param.hash != DNS_NSEC3_UNKNOWNALG &&
				    !dns_nsec3_supportedhash(nsec3param.hash))
					continue;

#ifdef RFC5155_STRICT
				if (nsec3param.flags != 0)
					continue;
#else
				if ((nsec3param.flags & DNS_NSEC3FLAG_CREATE)
				    != 0)
					*nsec3createflag = ISC_TRUE;
				if ((nsec3param.flags & ~DNS_NSEC3FLAG_OPTOUT)
				    != 0)
					continue;
#endif

				INSIST(nsec3param.salt_length <=
				       sizeof(version->salt));
				memcpy(version->salt, nsec3param.salt,
				       nsec3param.salt_length);
				version->hash = nsec3param.hash;
				version->salt_length = nsec3param.salt_length;
				version->iterations = nsec3param.iterations;
				version->flags = nsec3param.flags;
				version->havensec3 = ISC_TRUE;
				/*
				 * Look for a better algorithm than the
				 * unknown test algorithm.
				 */
				if (nsec3param.hash != DNS_NSEC3_UNKNOWNALG)
					goto unlock;
			}
		}
	}
 unlock:
	NODE_UNLOCK(&(rbtdb->node_locks[node->locknum].lock),
		    isc_rwlocktype_read);
	RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
}

static void
closeversion(dns_db_t *db, dns_dbversion_t **versionp, isc_boolean_t commit) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	rbtdb_version_t *version, *cleanup_version, *least_greater;
	isc_boolean_t rollback = ISC_FALSE;
	rbtdb_changedlist_t cleanup_list;
	rdatasetheaderlist_t resigned_list;
	rbtdb_changed_t *changed, *next_changed;
	rbtdb_serial_t serial, least_serial;
	dns_rbtnode_t *rbtnode;
	unsigned int refs;
	rdatasetheader_t *header;
	isc_boolean_t writer;

	REQUIRE(VALID_RBTDB(rbtdb));
	version = (rbtdb_version_t *)*versionp;

	cleanup_version = NULL;
	ISC_LIST_INIT(cleanup_list);
	ISC_LIST_INIT(resigned_list);

	isc_refcount_decrement(&version->references, &refs);
	if (refs > 0) {         /* typical and easy case first */
		if (commit) {
			RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_read);
			INSIST(!version->writer);
			RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_read);
		}
		goto end;
	}

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_write);
	serial = version->serial;
	writer = version->writer;
	if (version->writer) {
		if (commit) {
			unsigned cur_ref;
			rbtdb_version_t *cur_version;

			INSIST(version->commit_ok);
			INSIST(version == rbtdb->future_version);
			/*
			 * The current version is going to be replaced.
			 * Release the (likely last) reference to it from the
			 * DB itself and unlink it from the open list.
			 */
			cur_version = rbtdb->current_version;
			isc_refcount_decrement(&cur_version->references,
					       &cur_ref);
			if (cur_ref == 0) {
				if (cur_version->serial == rbtdb->least_serial)
					INSIST(EMPTY(cur_version->changed_list));
				UNLINK(rbtdb->open_versions,
				       cur_version, link);
			}
			if (EMPTY(rbtdb->open_versions)) {
				/*
				 * We're going to become the least open
				 * version.
				 */
				make_least_version(rbtdb, version,
						   &cleanup_list);
			} else {
				/*
				 * Some other open version is the
				 * least version.  We can't cleanup
				 * records that were changed in this
				 * version because the older versions
				 * may still be in use by an open
				 * version.
				 *
				 * We can, however, discard the
				 * changed records for things that
				 * we've added that didn't exist in
				 * prior versions.
				 */
				cleanup_nondirty(version, &cleanup_list);
			}
			/*
			 * If the (soon to be former) current version
			 * isn't being used by anyone, we can clean
			 * it up.
			 */
			if (cur_ref == 0) {
				cleanup_version = cur_version;
				APPENDLIST(version->changed_list,
					   cleanup_version->changed_list,
					   link);
			}
			/*
			 * Become the current version.
			 */
			version->writer = ISC_FALSE;
			rbtdb->current_version = version;
			rbtdb->current_serial = version->serial;
			rbtdb->future_version = NULL;

			/*
			 * Keep the current version in the open list, and
			 * gain a reference for the DB itself (see the DB
			 * creation function below).  This must be the only
			 * case where we need to increment the counter from
			 * zero and need to use isc_refcount_increment0().
			 */
			isc_refcount_increment0(&version->references,
						&cur_ref);
			INSIST(cur_ref == 1);
			PREPEND(rbtdb->open_versions,
				rbtdb->current_version, link);
			resigned_list = version->resigned_list;
			ISC_LIST_INIT(version->resigned_list);
		} else {
			/*
			 * We're rolling back this transaction.
			 */
			cleanup_list = version->changed_list;
			ISC_LIST_INIT(version->changed_list);
			resigned_list = version->resigned_list;
			ISC_LIST_INIT(version->resigned_list);
			rollback = ISC_TRUE;
			cleanup_version = version;
			rbtdb->future_version = NULL;
		}
	} else {
		if (version != rbtdb->current_version) {
			/*
			 * There are no external or internal references
			 * to this version and it can be cleaned up.
			 */
			cleanup_version = version;

			/*
			 * Find the version with the least serial
			 * number greater than ours.
			 */
			least_greater = PREV(version, link);
			if (least_greater == NULL)
				least_greater = rbtdb->current_version;

			INSIST(version->serial < least_greater->serial);
			/*
			 * Is this the least open version?
			 */
			if (version->serial == rbtdb->least_serial) {
				/*
				 * Yes.  Install the new least open
				 * version.
				 */
				make_least_version(rbtdb,
						   least_greater,
						   &cleanup_list);
			} else {
				/*
				 * Add any unexecuted cleanups to
				 * those of the least greater version.
				 */
				APPENDLIST(least_greater->changed_list,
					   version->changed_list,
					   link);
			}
		} else if (version->serial == rbtdb->least_serial)
			INSIST(EMPTY(version->changed_list));
		UNLINK(rbtdb->open_versions, version, link);
	}
	least_serial = rbtdb->least_serial;
	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_write);

	/*
	 * Update the zone's secure status.
	 */
	if (writer && commit && !IS_CACHE(rbtdb))
		iszonesecure(db, version, rbtdb->origin_node);

	if (cleanup_version != NULL) {
		INSIST(EMPTY(cleanup_version->changed_list));
		isc_mem_put(rbtdb->common.mctx, cleanup_version,
			    sizeof(*cleanup_version));
	}

	/*
	 * Commit/rollback re-signed headers.
	 */
	for (header = HEAD(resigned_list);
	     header != NULL;
	     header = HEAD(resigned_list)) {
		ISC_LIST_UNLINK(resigned_list, header, lru_link);
		if (rollback) {
			nodelock_t *lock;
			lock = &rbtdb->node_locks[header->node->locknum].lock;
			NODE_LOCK(lock, isc_rwlocktype_write);
			resign_insert(rbtdb, header->node->locknum, header);
			NODE_UNLOCK(lock, isc_rwlocktype_write);
		}
		decrement_reference(rbtdb, header->node, least_serial,
				    isc_rwlocktype_write, isc_rwlocktype_none,
				    ISC_FALSE);
	}

	if (!EMPTY(cleanup_list)) {
		/*
		 * We acquire a tree write lock here in order to make sure
		 * that stale nodes will be removed in decrement_reference().
		 * If we didn't have the lock, those nodes could miss the
		 * chance to be removed until the server stops.  The write lock
		 * is expensive, but this event should be rare enough to justify
		 * the cost.
		 */
		RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);
		for (changed = HEAD(cleanup_list);
		     changed != NULL;
		     changed = next_changed) {
			nodelock_t *lock;

			next_changed = NEXT(changed, link);
			rbtnode = changed->node;
			lock = &rbtdb->node_locks[rbtnode->locknum].lock;

			NODE_LOCK(lock, isc_rwlocktype_write);
			/*
			 * This is a good opportunity to purge any dead nodes,
			 * so use it.
			 */
			cleanup_dead_nodes(rbtdb, rbtnode->locknum);

			if (rollback)
				rollback_node(rbtnode, serial);
			decrement_reference(rbtdb, rbtnode, least_serial,
					    isc_rwlocktype_write,
					    isc_rwlocktype_write, ISC_FALSE);

			NODE_UNLOCK(lock, isc_rwlocktype_write);

			isc_mem_put(rbtdb->common.mctx, changed,
				    sizeof(*changed));
		}
		RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);
	}

 end:
	*versionp = NULL;
}

/*
 * Add the necessary magic for the wildcard name 'name'
 * to be found in 'rbtdb'.
 *
 * In order for wildcard matching to work correctly in
 * zone_find(), we must ensure that a node for the wildcarding
 * level exists in the database, and has its 'find_callback'
 * and 'wild' bits set.
 *
 * E.g. if the wildcard name is "*.sub.example." then we
 * must ensure that "sub.example." exists and is marked as
 * a wildcard level.
 */
static isc_result_t
add_wildcard_magic(dns_rbtdb_t *rbtdb, dns_name_t *name) {
	isc_result_t result;
	dns_name_t foundname;
	dns_offsets_t offsets;
	unsigned int n;
	dns_rbtnode_t *node = NULL;

	dns_name_init(&foundname, offsets);
	n = dns_name_countlabels(name);
	INSIST(n >= 2);
	n--;
	dns_name_getlabelsequence(name, 1, n, &foundname);
	result = dns_rbt_addnode(rbtdb->tree, &foundname, &node);
	if (result != ISC_R_SUCCESS && result != ISC_R_EXISTS)
		return (result);
	node->nsec3 = 0;
	node->find_callback = 1;
	node->wild = 1;
	return (ISC_R_SUCCESS);
}

static isc_result_t
add_empty_wildcards(dns_rbtdb_t *rbtdb, dns_name_t *name) {
	isc_result_t result;
	dns_name_t foundname;
	dns_offsets_t offsets;
	unsigned int n, l, i;

	dns_name_init(&foundname, offsets);
	n = dns_name_countlabels(name);
	l = dns_name_countlabels(&rbtdb->common.origin);
	i = l + 1;
	while (i < n) {
		dns_rbtnode_t *node = NULL;     /* dummy */
		dns_name_getlabelsequence(name, n - i, i, &foundname);
		if (dns_name_iswildcard(&foundname)) {
			result = add_wildcard_magic(rbtdb, &foundname);
			if (result != ISC_R_SUCCESS)
				return (result);
			result = dns_rbt_addnode(rbtdb->tree, &foundname,
						 &node);
			if (result != ISC_R_SUCCESS && result != ISC_R_EXISTS)
				return (result);
			node->nsec3 = 0;
		}
		i++;
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
findnode(dns_db_t *db, dns_name_t *name, isc_boolean_t create,
	 dns_dbnode_t **nodep)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *node = NULL;
	dns_name_t nodename;
	isc_result_t result;
	isc_rwlocktype_t locktype = isc_rwlocktype_read;

	REQUIRE(VALID_RBTDB(rbtdb));

	dns_name_init(&nodename, NULL);
	RWLOCK(&rbtdb->tree_lock, locktype);
	result = dns_rbt_findnode(rbtdb->tree, name, NULL, &node, NULL,
				  DNS_RBTFIND_EMPTYDATA, NULL, NULL);
	if (result != ISC_R_SUCCESS) {
		RWUNLOCK(&rbtdb->tree_lock, locktype);
		if (!create) {
			if (result == DNS_R_PARTIALMATCH)
				result = ISC_R_NOTFOUND;
			return (result);
		}
		/*
		 * It would be nice to try to upgrade the lock instead of
		 * unlocking then relocking.
		 */
		locktype = isc_rwlocktype_write;
		RWLOCK(&rbtdb->tree_lock, locktype);
		node = NULL;
		result = dns_rbt_addnode(rbtdb->tree, name, &node);
		if (result == ISC_R_SUCCESS) {
			dns_rbt_namefromnode(node, &nodename);
#ifdef DNS_RBT_USEHASH
			node->locknum = node->hashval % rbtdb->node_lock_count;
#else
			node->locknum = dns_name_hash(&nodename, ISC_TRUE) %
				rbtdb->node_lock_count;
#endif
			node->nsec3 = 0;
			add_empty_wildcards(rbtdb, name);

			if (dns_name_iswildcard(name)) {
				result = add_wildcard_magic(rbtdb, name);
				if (result != ISC_R_SUCCESS) {
					RWUNLOCK(&rbtdb->tree_lock, locktype);
					return (result);
				}
			}
		} else if (result != ISC_R_EXISTS) {
			RWUNLOCK(&rbtdb->tree_lock, locktype);
			return (result);
		}
	}
	reactivate_node(rbtdb, node, locktype);
	RWUNLOCK(&rbtdb->tree_lock, locktype);

	*nodep = (dns_dbnode_t *)node;

	return (ISC_R_SUCCESS);
}

static isc_result_t
findnsec3node(dns_db_t *db, dns_name_t *name, isc_boolean_t create,
	      dns_dbnode_t **nodep)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *node = NULL;
	dns_name_t nodename;
	isc_result_t result;
	isc_rwlocktype_t locktype = isc_rwlocktype_read;

	REQUIRE(VALID_RBTDB(rbtdb));

	dns_name_init(&nodename, NULL);
	RWLOCK(&rbtdb->tree_lock, locktype);
	result = dns_rbt_findnode(rbtdb->nsec3, name, NULL, &node, NULL,
				  DNS_RBTFIND_EMPTYDATA, NULL, NULL);
	if (result != ISC_R_SUCCESS) {
		RWUNLOCK(&rbtdb->tree_lock, locktype);
		if (!create) {
			if (result == DNS_R_PARTIALMATCH)
				result = ISC_R_NOTFOUND;
			return (result);
		}
		/*
		 * It would be nice to try to upgrade the lock instead of
		 * unlocking then relocking.
		 */
		locktype = isc_rwlocktype_write;
		RWLOCK(&rbtdb->tree_lock, locktype);
		node = NULL;
		result = dns_rbt_addnode(rbtdb->nsec3, name, &node);
		if (result == ISC_R_SUCCESS) {
			dns_rbt_namefromnode(node, &nodename);
#ifdef DNS_RBT_USEHASH
			node->locknum = node->hashval % rbtdb->node_lock_count;
#else
			node->locknum = dns_name_hash(&nodename, ISC_TRUE) %
				rbtdb->node_lock_count;
#endif
			node->nsec3 = 1U;
		} else if (result != ISC_R_EXISTS) {
			RWUNLOCK(&rbtdb->tree_lock, locktype);
			return (result);
		}
	} else
		INSIST(node->nsec3);
	NODE_STRONGLOCK(&rbtdb->node_locks[node->locknum].lock);
	new_reference(rbtdb, node);
	NODE_STRONGUNLOCK(&rbtdb->node_locks[node->locknum].lock);
	RWUNLOCK(&rbtdb->tree_lock, locktype);

	*nodep = (dns_dbnode_t *)node;

	return (ISC_R_SUCCESS);
}

static isc_result_t
zone_zonecut_callback(dns_rbtnode_t *node, dns_name_t *name, void *arg) {
	rbtdb_search_t *search = arg;
	rdatasetheader_t *header, *header_next;
	rdatasetheader_t *dname_header, *sigdname_header, *ns_header;
	rdatasetheader_t *found;
	isc_result_t result;
	dns_rbtnode_t *onode;

	/*
	 * We only want to remember the topmost zone cut, since it's the one
	 * that counts, so we'll just continue if we've already found a
	 * zonecut.
	 */
	if (search->zonecut != NULL)
		return (DNS_R_CONTINUE);

	found = NULL;
	result = DNS_R_CONTINUE;
	onode = search->rbtdb->origin_node;

	NODE_LOCK(&(search->rbtdb->node_locks[node->locknum].lock),
		  isc_rwlocktype_read);

	/*
	 * Look for an NS or DNAME rdataset active in our version.
	 */
	ns_header = NULL;
	dname_header = NULL;
	sigdname_header = NULL;
	for (header = node->data; header != NULL; header = header_next) {
		header_next = header->next;
		if (header->type == dns_rdatatype_ns ||
		    header->type == dns_rdatatype_dname ||
		    header->type == RBTDB_RDATATYPE_SIGDNAME) {
			do {
				if (header->serial <= search->serial &&
				    !IGNORE(header)) {
					/*
					 * Is this a "this rdataset doesn't
					 * exist" record?
					 */
					if (NONEXISTENT(header))
						header = NULL;
					break;
				} else
					header = header->down;
			} while (header != NULL);
			if (header != NULL) {
				if (header->type == dns_rdatatype_dname)
					dname_header = header;
				else if (header->type ==
					   RBTDB_RDATATYPE_SIGDNAME)
					sigdname_header = header;
				else if (node != onode ||
					 IS_STUB(search->rbtdb)) {
					/*
					 * We've found an NS rdataset that
					 * isn't at the origin node.  We check
					 * that they're not at the origin node,
					 * because otherwise we'd erroneously
					 * treat the zone top as if it were
					 * a delegation.
					 */
					ns_header = header;
				}
			}
		}
	}

	/*
	 * Did we find anything?
	 */
	if (dname_header != NULL) {
		/*
		 * Note that DNAME has precedence over NS if both exist.
		 */
		found = dname_header;
		search->zonecut_sigrdataset = sigdname_header;
	} else if (ns_header != NULL) {
		found = ns_header;
		search->zonecut_sigrdataset = NULL;
	}

	if (found != NULL) {
		/*
		 * We increment the reference count on node to ensure that
		 * search->zonecut_rdataset will still be valid later.
		 */
		new_reference(search->rbtdb, node);
		search->zonecut = node;
		search->zonecut_rdataset = found;
		search->need_cleanup = ISC_TRUE;
		/*
		 * Since we've found a zonecut, anything beneath it is
		 * glue and is not subject to wildcard matching, so we
		 * may clear search->wild.
		 */
		search->wild = ISC_FALSE;
		if ((search->options & DNS_DBFIND_GLUEOK) == 0) {
			/*
			 * If the caller does not want to find glue, then
			 * this is the best answer and the search should
			 * stop now.
			 */
			result = DNS_R_PARTIALMATCH;
		} else {
			dns_name_t *zcname;

			/*
			 * The search will continue beneath the zone cut.
			 * This may or may not be the best match.  In case it
			 * is, we need to remember the node name.
			 */
			zcname = dns_fixedname_name(&search->zonecut_name);
			RUNTIME_CHECK(dns_name_copy(name, zcname, NULL) ==
				      ISC_R_SUCCESS);
			search->copy_name = ISC_TRUE;
		}
	} else {
		/*
		 * There is no zonecut at this node which is active in this
		 * version.
		 *
		 * If this is a "wild" node and the caller hasn't disabled
		 * wildcard matching, remember that we've seen a wild node
		 * in case we need to go searching for wildcard matches
		 * later on.
		 */
		if (node->wild && (search->options & DNS_DBFIND_NOWILD) == 0)
			search->wild = ISC_TRUE;
	}

	NODE_UNLOCK(&(search->rbtdb->node_locks[node->locknum].lock),
		    isc_rwlocktype_read);

	return (result);
}

static inline void
bind_rdataset(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node,
	      rdatasetheader_t *header, isc_stdtime_t now,
	      dns_rdataset_t *rdataset)
{
	unsigned char *raw;     /* RDATASLAB */

	/*
	 * Caller must be holding the node reader lock.
	 * XXXJT: technically, we need a writer lock, since we'll increment
	 * the header count below.  However, since the actual counter value
	 * doesn't matter, we prioritize performance here.  (We may want to
	 * use atomic increment when available).
	 */

	if (rdataset == NULL)
		return;

	new_reference(rbtdb, node);

	INSIST(rdataset->methods == NULL);      /* We must be disassociated. */

	rdataset->methods = &rdataset_methods;
	rdataset->rdclass = rbtdb->common.rdclass;
	rdataset->type = RBTDB_RDATATYPE_BASE(header->type);
	rdataset->covers = RBTDB_RDATATYPE_EXT(header->type);
	rdataset->ttl = header->rdh_ttl - now;
	rdataset->trust = header->trust;
	if (NXDOMAIN(header))
		rdataset->attributes |= DNS_RDATASETATTR_NXDOMAIN;
	if (OPTOUT(header))
		rdataset->attributes |= DNS_RDATASETATTR_OPTOUT;
	rdataset->private1 = rbtdb;
	rdataset->private2 = node;
	raw = (unsigned char *)header + sizeof(*header);
	rdataset->private3 = raw;
	rdataset->count = header->count++;
	if (rdataset->count == ISC_UINT32_MAX)
		rdataset->count = 0;

	/*
	 * Reset iterator state.
	 */
	rdataset->privateuint4 = 0;
	rdataset->private5 = NULL;

	/*
	 * Add noqname proof.
	 */
	rdataset->private6 = header->noqname;
	if (rdataset->private6 != NULL)
		rdataset->attributes |=  DNS_RDATASETATTR_NOQNAME;
	rdataset->private7 = header->closest;
	if (rdataset->private7 != NULL)
		rdataset->attributes |=  DNS_RDATASETATTR_CLOSEST;

	/*
	 * Copy out re-signing information.
	 */
	if (RESIGN(header)) {
		rdataset->attributes |=  DNS_RDATASETATTR_RESIGN;
		rdataset->resign = header->resign;
	} else
		rdataset->resign = 0;
}

static inline isc_result_t
setup_delegation(rbtdb_search_t *search, dns_dbnode_t **nodep,
		 dns_name_t *foundname, dns_rdataset_t *rdataset,
		 dns_rdataset_t *sigrdataset)
{
	isc_result_t result;
	dns_name_t *zcname;
	rbtdb_rdatatype_t type;
	dns_rbtnode_t *node;

	/*
	 * The caller MUST NOT be holding any node locks.
	 */

	node = search->zonecut;
	type = search->zonecut_rdataset->type;

	/*
	 * If we have to set foundname, we do it before anything else.
	 * If we were to set foundname after we had set nodep or bound the
	 * rdataset, then we'd have to undo that work if dns_name_copy()
	 * failed.  By setting foundname first, there's nothing to undo if
	 * we have trouble.
	 */
	if (foundname != NULL && search->copy_name) {
		zcname = dns_fixedname_name(&search->zonecut_name);
		result = dns_name_copy(zcname, foundname, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);
	}
	if (nodep != NULL) {
		/*
		 * Note that we don't have to increment the node's reference
		 * count here because we're going to use the reference we
		 * already have in the search block.
		 */
		*nodep = node;
		search->need_cleanup = ISC_FALSE;
	}
	if (rdataset != NULL) {
		NODE_LOCK(&(search->rbtdb->node_locks[node->locknum].lock),
			  isc_rwlocktype_read);
		bind_rdataset(search->rbtdb, node, search->zonecut_rdataset,
			      search->now, rdataset);
		if (sigrdataset != NULL && search->zonecut_sigrdataset != NULL)
			bind_rdataset(search->rbtdb, node,
				      search->zonecut_sigrdataset,
				      search->now, sigrdataset);
		NODE_UNLOCK(&(search->rbtdb->node_locks[node->locknum].lock),
			    isc_rwlocktype_read);
	}

	if (type == dns_rdatatype_dname)
		return (DNS_R_DNAME);
	return (DNS_R_DELEGATION);
}

static inline isc_boolean_t
valid_glue(rbtdb_search_t *search, dns_name_t *name, rbtdb_rdatatype_t type,
	   dns_rbtnode_t *node)
{
	unsigned char *raw;     /* RDATASLAB */
	unsigned int count, size;
	dns_name_t ns_name;
	isc_boolean_t valid = ISC_FALSE;
	dns_offsets_t offsets;
	isc_region_t region;
	rdatasetheader_t *header;

	/*
	 * No additional locking is required.
	 */

	/*
	 * Valid glue types are A, AAAA, A6.  NS is also a valid glue type
	 * if it occurs at a zone cut, but is not valid below it.
	 */
	if (type == dns_rdatatype_ns) {
		if (node != search->zonecut) {
			return (ISC_FALSE);
		}
	} else if (type != dns_rdatatype_a &&
		   type != dns_rdatatype_aaaa &&
		   type != dns_rdatatype_a6) {
		return (ISC_FALSE);
	}

	header = search->zonecut_rdataset;
	raw = (unsigned char *)header + sizeof(*header);
	count = raw[0] * 256 + raw[1];
#if DNS_RDATASET_FIXED
	raw += 2 + (4 * count);
#else
	raw += 2;
#endif

	while (count > 0) {
		count--;
		size = raw[0] * 256 + raw[1];
#if DNS_RDATASET_FIXED
		raw += 4;
#else
		raw += 2;
#endif
		region.base = raw;
		region.length = size;
		raw += size;
		/*
		 * XXX Until we have rdata structures, we have no choice but
		 * to directly access the rdata format.
		 */
		dns_name_init(&ns_name, offsets);
		dns_name_fromregion(&ns_name, &region);
		if (dns_name_compare(&ns_name, name) == 0) {
			valid = ISC_TRUE;
			break;
		}
	}

	return (valid);
}

static inline isc_boolean_t
activeempty(rbtdb_search_t *search, dns_rbtnodechain_t *chain,
	    dns_name_t *name)
{
	dns_fixedname_t fnext;
	dns_fixedname_t forigin;
	dns_name_t *next;
	dns_name_t *origin;
	dns_name_t prefix;
	dns_rbtdb_t *rbtdb;
	dns_rbtnode_t *node;
	isc_result_t result;
	isc_boolean_t answer = ISC_FALSE;
	rdatasetheader_t *header;

	rbtdb = search->rbtdb;

	dns_name_init(&prefix, NULL);
	dns_fixedname_init(&fnext);
	next = dns_fixedname_name(&fnext);
	dns_fixedname_init(&forigin);
	origin = dns_fixedname_name(&forigin);

	result = dns_rbtnodechain_next(chain, NULL, NULL);
	while (result == ISC_R_SUCCESS || result == DNS_R_NEWORIGIN) {
		node = NULL;
		result = dns_rbtnodechain_current(chain, &prefix,
						  origin, &node);
		if (result != ISC_R_SUCCESS)
			break;
		NODE_LOCK(&(rbtdb->node_locks[node->locknum].lock),
			  isc_rwlocktype_read);
		for (header = node->data;
		     header != NULL;
		     header = header->next) {
			if (header->serial <= search->serial &&
			    !IGNORE(header) && EXISTS(header))
				break;
		}
		NODE_UNLOCK(&(rbtdb->node_locks[node->locknum].lock),
			    isc_rwlocktype_read);
		if (header != NULL)
			break;
		result = dns_rbtnodechain_next(chain, NULL, NULL);
	}
	if (result == ISC_R_SUCCESS)
		result = dns_name_concatenate(&prefix, origin, next, NULL);
	if (result == ISC_R_SUCCESS && dns_name_issubdomain(next, name))
		answer = ISC_TRUE;
	return (answer);
}

static inline isc_boolean_t
activeemtpynode(rbtdb_search_t *search, dns_name_t *qname, dns_name_t *wname) {
	dns_fixedname_t fnext;
	dns_fixedname_t forigin;
	dns_fixedname_t fprev;
	dns_name_t *next;
	dns_name_t *origin;
	dns_name_t *prev;
	dns_name_t name;
	dns_name_t rname;
	dns_name_t tname;
	dns_rbtdb_t *rbtdb;
	dns_rbtnode_t *node;
	dns_rbtnodechain_t chain;
	isc_boolean_t check_next = ISC_TRUE;
	isc_boolean_t check_prev = ISC_TRUE;
	isc_boolean_t answer = ISC_FALSE;
	isc_result_t result;
	rdatasetheader_t *header;
	unsigned int n;

	rbtdb = search->rbtdb;

	dns_name_init(&name, NULL);
	dns_name_init(&tname, NULL);
	dns_name_init(&rname, NULL);
	dns_fixedname_init(&fnext);
	next = dns_fixedname_name(&fnext);
	dns_fixedname_init(&fprev);
	prev = dns_fixedname_name(&fprev);
	dns_fixedname_init(&forigin);
	origin = dns_fixedname_name(&forigin);

	/*
	 * Find if qname is at or below a empty node.
	 * Use our own copy of the chain.
	 */

	chain = search->chain;
	do {
		node = NULL;
		result = dns_rbtnodechain_current(&chain, &name,
						  origin, &node);
		if (result != ISC_R_SUCCESS)
			break;
		NODE_LOCK(&(rbtdb->node_locks[node->locknum].lock),
			  isc_rwlocktype_read);
		for (header = node->data;
		     header != NULL;
		     header = header->next) {
			if (header->serial <= search->serial &&
			    !IGNORE(header) && EXISTS(header))
				break;
		}
		NODE_UNLOCK(&(rbtdb->node_locks[node->locknum].lock),
			    isc_rwlocktype_read);
		if (header != NULL)
			break;
		result = dns_rbtnodechain_prev(&chain, NULL, NULL);
	} while (result == ISC_R_SUCCESS || result == DNS_R_NEWORIGIN);
	if (result == ISC_R_SUCCESS)
		result = dns_name_concatenate(&name, origin, prev, NULL);
	if (result != ISC_R_SUCCESS)
		check_prev = ISC_FALSE;

	result = dns_rbtnodechain_next(&chain, NULL, NULL);
	while (result == ISC_R_SUCCESS || result == DNS_R_NEWORIGIN) {
		node = NULL;
		result = dns_rbtnodechain_current(&chain, &name,
						  origin, &node);
		if (result != ISC_R_SUCCESS)
			break;
		NODE_LOCK(&(rbtdb->node_locks[node->locknum].lock),
			  isc_rwlocktype_read);
		for (header = node->data;
		     header != NULL;
		     header = header->next) {
			if (header->serial <= search->serial &&
			    !IGNORE(header) && EXISTS(header))
				break;
		}
		NODE_UNLOCK(&(rbtdb->node_locks[node->locknum].lock),
			    isc_rwlocktype_read);
		if (header != NULL)
			break;
		result = dns_rbtnodechain_next(&chain, NULL, NULL);
	}
	if (result == ISC_R_SUCCESS)
		result = dns_name_concatenate(&name, origin, next, NULL);
	if (result != ISC_R_SUCCESS)
		check_next = ISC_FALSE;

	dns_name_clone(qname, &rname);

	/*
	 * Remove the wildcard label to find the terminal name.
	 */
	n = dns_name_countlabels(wname);
	dns_name_getlabelsequence(wname, 1, n - 1, &tname);

	do {
		if ((check_prev && dns_name_issubdomain(prev, &rname)) ||
		    (check_next && dns_name_issubdomain(next, &rname))) {
			answer = ISC_TRUE;
			break;
		}
		/*
		 * Remove the left hand label.
		 */
		n = dns_name_countlabels(&rname);
		dns_name_getlabelsequence(&rname, 1, n - 1, &rname);
	} while (!dns_name_equal(&rname, &tname));
	return (answer);
}

static inline isc_result_t
find_wildcard(rbtdb_search_t *search, dns_rbtnode_t **nodep,
	      dns_name_t *qname)
{
	unsigned int i, j;
	dns_rbtnode_t *node, *level_node, *wnode;
	rdatasetheader_t *header;
	isc_result_t result = ISC_R_NOTFOUND;
	dns_name_t name;
	dns_name_t *wname;
	dns_fixedname_t fwname;
	dns_rbtdb_t *rbtdb;
	isc_boolean_t done, wild, active;
	dns_rbtnodechain_t wchain;

	/*
	 * Caller must be holding the tree lock and MUST NOT be holding
	 * any node locks.
	 */

	/*
	 * Examine each ancestor level.  If the level's wild bit
	 * is set, then construct the corresponding wildcard name and
	 * search for it.  If the wildcard node exists, and is active in
	 * this version, we're done.  If not, then we next check to see
	 * if the ancestor is active in this version.  If so, then there
	 * can be no possible wildcard match and again we're done.  If not,
	 * continue the search.
	 */

	rbtdb = search->rbtdb;
	i = search->chain.level_matches;
	done = ISC_FALSE;
	node = *nodep;
	do {
		NODE_LOCK(&(rbtdb->node_locks[node->locknum].lock),
			  isc_rwlocktype_read);

		/*
		 * First we try to figure out if this node is active in
		 * the search's version.  We do this now, even though we
		 * may not need the information, because it simplifies the
		 * locking and code flow.
		 */
		for (header = node->data;
		     header != NULL;
		     header = header->next) {
			if (header->serial <= search->serial &&
			    !IGNORE(header) && EXISTS(header))
				break;
		}
		if (header != NULL)
			active = ISC_TRUE;
		else
			active = ISC_FALSE;

		if (node->wild)
			wild = ISC_TRUE;
		else
			wild = ISC_FALSE;

		NODE_UNLOCK(&(rbtdb->node_locks[node->locknum].lock),
			    isc_rwlocktype_read);

		if (wild) {
			/*
			 * Construct the wildcard name for this level.
			 */
			dns_name_init(&name, NULL);
			dns_rbt_namefromnode(node, &name);
			dns_fixedname_init(&fwname);
			wname = dns_fixedname_name(&fwname);
			result = dns_name_concatenate(dns_wildcardname, &name,
						      wname, NULL);
			j = i;
			while (result == ISC_R_SUCCESS && j != 0) {
				j--;
				level_node = search->chain.levels[j];
				dns_name_init(&name, NULL);
				dns_rbt_namefromnode(level_node, &name);
				result = dns_name_concatenate(wname,
							      &name,
							      wname,
							      NULL);
			}
			if (result != ISC_R_SUCCESS)
				break;

			wnode = NULL;
			dns_rbtnodechain_init(&wchain, NULL);
			result = dns_rbt_findnode(rbtdb->tree, wname,
						  NULL, &wnode, &wchain,
						  DNS_RBTFIND_EMPTYDATA,
						  NULL, NULL);
			if (result == ISC_R_SUCCESS) {
				nodelock_t *lock;

				/*
				 * We have found the wildcard node.  If it
				 * is active in the search's version, we're
				 * done.
				 */
				lock = &rbtdb->node_locks[wnode->locknum].lock;
				NODE_LOCK(lock, isc_rwlocktype_read);
				for (header = wnode->data;
				     header != NULL;
				     header = header->next) {
					if (header->serial <= search->serial &&
					    !IGNORE(header) && EXISTS(header))
						break;
				}
				NODE_UNLOCK(lock, isc_rwlocktype_read);
				if (header != NULL ||
				    activeempty(search, &wchain, wname)) {
					if (activeemtpynode(search, qname,
							    wname)) {
						return (ISC_R_NOTFOUND);
					}
					/*
					 * The wildcard node is active!
					 *
					 * Note: result is still ISC_R_SUCCESS
					 * so we don't have to set it.
					 */
					*nodep = wnode;
					break;
				}
			} else if (result != ISC_R_NOTFOUND &&
				   result != DNS_R_PARTIALMATCH) {
				/*
				 * An error has occurred.  Bail out.
				 */
				break;
			}
		}

		if (active) {
			/*
			 * The level node is active.  Any wildcarding
			 * present at higher levels has no
			 * effect and we're done.
			 */
			result = ISC_R_NOTFOUND;
			break;
		}

		if (i > 0) {
			i--;
			node = search->chain.levels[i];
		} else
			done = ISC_TRUE;
	} while (!done);

	return (result);
}

static isc_boolean_t
matchparams(rdatasetheader_t *header, rbtdb_search_t *search)
{
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_nsec3_t nsec3;
	unsigned char *raw;                     /* RDATASLAB */
	unsigned int rdlen, count;
	isc_region_t region;
	isc_result_t result;

	REQUIRE(header->type == dns_rdatatype_nsec3);

	raw = (unsigned char *)header + sizeof(*header);
	count = raw[0] * 256 + raw[1]; /* count */
#if DNS_RDATASET_FIXED
	raw += count * 4 + 2;
#else
	raw += 2;
#endif
	while (count-- > 0) {
		rdlen = raw[0] * 256 + raw[1];
#if DNS_RDATASET_FIXED
		raw += 4;
#else
		raw += 2;
#endif
		region.base = raw;
		region.length = rdlen;
		dns_rdata_fromregion(&rdata, search->rbtdb->common.rdclass,
				     dns_rdatatype_nsec3, &region);
		raw += rdlen;
		result = dns_rdata_tostruct(&rdata, &nsec3, NULL);
		INSIST(result == ISC_R_SUCCESS);
		if (nsec3.hash == search->rbtversion->hash &&
		    nsec3.iterations == search->rbtversion->iterations &&
		    nsec3.salt_length == search->rbtversion->salt_length &&
		    memcmp(nsec3.salt, search->rbtversion->salt,
			   nsec3.salt_length) == 0)
			return (ISC_TRUE);
		dns_rdata_reset(&rdata);
	}
	return (ISC_FALSE);
}

static inline isc_result_t
find_closest_nsec(rbtdb_search_t *search, dns_dbnode_t **nodep,
		  dns_name_t *foundname, dns_rdataset_t *rdataset,
		  dns_rdataset_t *sigrdataset, dns_rbt_t *tree,
		  dns_db_secure_t secure)
{
	dns_rbtnode_t *node;
	rdatasetheader_t *header, *header_next, *found, *foundsig;
	isc_boolean_t empty_node;
	isc_result_t result;
	dns_fixedname_t fname, forigin;
	dns_name_t *name, *origin;
	dns_rdatatype_t type;
	rbtdb_rdatatype_t sigtype;
	isc_boolean_t wraps;
	isc_boolean_t need_sig = ISC_TF(secure == dns_db_secure);

	if (tree == search->rbtdb->nsec3) {
		type = dns_rdatatype_nsec3;
		sigtype = RBTDB_RDATATYPE_SIGNSEC3;
		wraps = ISC_TRUE;
	} else {
		type = dns_rdatatype_nsec;
		sigtype = RBTDB_RDATATYPE_SIGNSEC;
		wraps = ISC_FALSE;
	}

 again:
	do {
		node = NULL;
		dns_fixedname_init(&fname);
		name = dns_fixedname_name(&fname);
		dns_fixedname_init(&forigin);
		origin = dns_fixedname_name(&forigin);
		result = dns_rbtnodechain_current(&search->chain, name,
						  origin, &node);
		if (result != ISC_R_SUCCESS)
			return (result);
		NODE_LOCK(&(search->rbtdb->node_locks[node->locknum].lock),
			  isc_rwlocktype_read);
		found = NULL;
		foundsig = NULL;
		empty_node = ISC_TRUE;
		for (header = node->data;
		     header != NULL;
		     header = header_next) {
			header_next = header->next;
			/*
			 * Look for an active, extant NSEC or RRSIG NSEC.
			 */
			do {
				if (header->serial <= search->serial &&
				    !IGNORE(header)) {
					/*
					 * Is this a "this rdataset doesn't
					 * exist" record?
					 */
					if (NONEXISTENT(header))
						header = NULL;
					break;
				} else
					header = header->down;
			} while (header != NULL);
			if (header != NULL) {
				/*
				 * We now know that there is at least one
				 * active rdataset at this node.
				 */
				empty_node = ISC_FALSE;
				if (header->type == type) {
					found = header;
					if (foundsig != NULL)
						break;
				} else if (header->type == sigtype) {
					foundsig = header;
					if (found != NULL)
						break;
				}
			}
		}
		if (!empty_node) {
			if (found != NULL && search->rbtversion->havensec3 &&
			    found->type == dns_rdatatype_nsec3 &&
			    !matchparams(found, search)) {
				empty_node = ISC_TRUE;
				found = NULL;
				foundsig = NULL;
				result = dns_rbtnodechain_prev(&search->chain,
							       NULL, NULL);
			} else if (found != NULL &&
				   (foundsig != NULL || !need_sig))
			{
				/*
				 * We've found the right NSEC/NSEC3 record.
				 *
				 * Note: for this to really be the right
				 * NSEC record, it's essential that the NSEC
				 * records of any nodes obscured by a zone
				 * cut have been removed; we assume this is
				 * the case.
				 */
				result = dns_name_concatenate(name, origin,
							      foundname, NULL);
				if (result == ISC_R_SUCCESS) {
					if (nodep != NULL) {
						new_reference(search->rbtdb,
							      node);
						*nodep = node;
					}
					bind_rdataset(search->rbtdb, node,
						      found, search->now,
						      rdataset);
					if (foundsig != NULL)
						bind_rdataset(search->rbtdb,
							      node,
							      foundsig,
							      search->now,
							      sigrdataset);
				}
			} else if (found == NULL && foundsig == NULL) {
				/*
				 * This node is active, but has no NSEC or
				 * RRSIG NSEC.  That means it's glue or
				 * other obscured zone data that isn't
				 * relevant for our search.  Treat the
				 * node as if it were empty and keep looking.
				 */
				empty_node = ISC_TRUE;
				result = dns_rbtnodechain_prev(&search->chain,
							       NULL, NULL);
			} else {
				/*
				 * We found an active node, but either the
				 * NSEC or the RRSIG NSEC is missing.  This
				 * shouldn't happen.
				 */
				result = DNS_R_BADDB;
			}
		} else {
			/*
			 * This node isn't active.  We've got to keep
			 * looking.
			 */
			result = dns_rbtnodechain_prev(&search->chain, NULL,
						       NULL);
		}
		NODE_UNLOCK(&(search->rbtdb->node_locks[node->locknum].lock),
			    isc_rwlocktype_read);
	} while (empty_node && result == ISC_R_SUCCESS);

	if (result == ISC_R_NOMORE && wraps) {
		result = dns_rbtnodechain_last(&search->chain, tree,
					       NULL, NULL);
		if (result == ISC_R_SUCCESS || result == DNS_R_NEWORIGIN) {
			wraps = ISC_FALSE;
			goto again;
		}
	}

	/*
	 * If the result is ISC_R_NOMORE, then we got to the beginning of
	 * the database and didn't find a NSEC record.  This shouldn't
	 * happen.
	 */
	if (result == ISC_R_NOMORE)
		result = DNS_R_BADDB;

	return (result);
}

static isc_result_t
zone_find(dns_db_t *db, dns_name_t *name, dns_dbversion_t *version,
	  dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	  dns_dbnode_t **nodep, dns_name_t *foundname,
	  dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	dns_rbtnode_t *node = NULL;
	isc_result_t result;
	rbtdb_search_t search;
	isc_boolean_t cname_ok = ISC_TRUE;
	isc_boolean_t close_version = ISC_FALSE;
	isc_boolean_t maybe_zonecut = ISC_FALSE;
	isc_boolean_t at_zonecut = ISC_FALSE;
	isc_boolean_t wild;
	isc_boolean_t empty_node;
	rdatasetheader_t *header, *header_next, *found, *nsecheader;
	rdatasetheader_t *foundsig, *cnamesig, *nsecsig;
	rbtdb_rdatatype_t sigtype;
	isc_boolean_t active;
	dns_rbtnodechain_t chain;
	nodelock_t *lock;
	dns_rbt_t *tree;

	search.rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(search.rbtdb));

	/*
	 * We don't care about 'now'.
	 */
	UNUSED(now);

	/*
	 * If the caller didn't supply a version, attach to the current
	 * version.
	 */
	if (version == NULL) {
		currentversion(db, &version);
		close_version = ISC_TRUE;
	}

	search.rbtversion = version;
	search.serial = search.rbtversion->serial;
	search.options = options;
	search.copy_name = ISC_FALSE;
	search.need_cleanup = ISC_FALSE;
	search.wild = ISC_FALSE;
	search.zonecut = NULL;
	dns_fixedname_init(&search.zonecut_name);
	dns_rbtnodechain_init(&search.chain, search.rbtdb->common.mctx);
	search.now = 0;

	/*
	 * 'wild' will be true iff. we've matched a wildcard.
	 */
	wild = ISC_FALSE;

	RWLOCK(&search.rbtdb->tree_lock, isc_rwlocktype_read);

	/*
	 * Search down from the root of the tree.  If, while going down, we
	 * encounter a callback node, zone_zonecut_callback() will search the
	 * rdatasets at the zone cut for active DNAME or NS rdatasets.
	 */
	tree =  (options & DNS_DBFIND_FORCENSEC3) != 0 ? search.rbtdb->nsec3 :
							 search.rbtdb->tree;
	result = dns_rbt_findnode(tree, name, foundname, &node,
				  &search.chain, DNS_RBTFIND_EMPTYDATA,
				  zone_zonecut_callback, &search);

	if (result == DNS_R_PARTIALMATCH) {
	partial_match:
		if (search.zonecut != NULL) {
		    result = setup_delegation(&search, nodep, foundname,
					      rdataset, sigrdataset);
		    goto tree_exit;
		}

		if (search.wild) {
			/*
			 * At least one of the levels in the search chain
			 * potentially has a wildcard.  For each such level,
			 * we must see if there's a matching wildcard active
			 * in the current version.
			 */
			result = find_wildcard(&search, &node, name);
			if (result == ISC_R_SUCCESS) {
				result = dns_name_copy(name, foundname, NULL);
				if (result != ISC_R_SUCCESS)
					goto tree_exit;
				wild = ISC_TRUE;
				goto found;
			}
			else if (result != ISC_R_NOTFOUND)
				goto tree_exit;
		}

		chain = search.chain;
		active = activeempty(&search, &chain, name);

		/*
		 * If we're here, then the name does not exist, is not
		 * beneath a zonecut, and there's no matching wildcard.
		 */
		if ((search.rbtversion->secure == dns_db_secure &&
		     !search.rbtversion->havensec3) ||
		    (search.options & DNS_DBFIND_FORCENSEC) != 0 ||
		    (search.options & DNS_DBFIND_FORCENSEC3) != 0)
		{
			result = find_closest_nsec(&search, nodep, foundname,
						   rdataset, sigrdataset, tree,
						   search.rbtversion->secure);
			if (result == ISC_R_SUCCESS)
				result = active ? DNS_R_EMPTYNAME :
						  DNS_R_NXDOMAIN;
		} else
			result = active ? DNS_R_EMPTYNAME : DNS_R_NXDOMAIN;
		goto tree_exit;
	} else if (result != ISC_R_SUCCESS)
		goto tree_exit;

 found:
	/*
	 * We have found a node whose name is the desired name, or we
	 * have matched a wildcard.
	 */

	if (search.zonecut != NULL) {
		/*
		 * If we're beneath a zone cut, we don't want to look for
		 * CNAMEs because they're not legitimate zone glue.
		 */
		cname_ok = ISC_FALSE;
	} else {
		/*
		 * The node may be a zone cut itself.  If it might be one,
		 * make sure we check for it later.
		 */
		if (node->find_callback &&
		    (node != search.rbtdb->origin_node ||
		     IS_STUB(search.rbtdb)) &&
		    !dns_rdatatype_atparent(type))
			maybe_zonecut = ISC_TRUE;
	}

	/*
	 * Certain DNSSEC types are not subject to CNAME matching
	 * (RFC4035, section 2.5 and RFC3007).
	 *
	 * We don't check for RRSIG, because we don't store RRSIG records
	 * directly.
	 */
	if (type == dns_rdatatype_key || type == dns_rdatatype_nsec)
		cname_ok = ISC_FALSE;

	/*
	 * We now go looking for rdata...
	 */

	NODE_LOCK(&(search.rbtdb->node_locks[node->locknum].lock),
		  isc_rwlocktype_read);

	found = NULL;
	foundsig = NULL;
	sigtype = RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, type);
	nsecheader = NULL;
	nsecsig = NULL;
	cnamesig = NULL;
	empty_node = ISC_TRUE;
	for (header = node->data; header != NULL; header = header_next) {
		header_next = header->next;
		/*
		 * Look for an active, extant rdataset.
		 */
		do {
			if (header->serial <= search.serial &&
			    !IGNORE(header)) {
				/*
				 * Is this a "this rdataset doesn't
				 * exist" record?
				 */
				if (NONEXISTENT(header))
					header = NULL;
				break;
			} else
				header = header->down;
		} while (header != NULL);
		if (header != NULL) {
			/*
			 * We now know that there is at least one active
			 * rdataset at this node.
			 */
			empty_node = ISC_FALSE;

			/*
			 * Do special zone cut handling, if requested.
			 */
			if (maybe_zonecut &&
			    header->type == dns_rdatatype_ns) {
				/*
				 * We increment the reference count on node to
				 * ensure that search->zonecut_rdataset will
				 * still be valid later.
				 */
				new_reference(search.rbtdb, node);
				search.zonecut = node;
				search.zonecut_rdataset = header;
				search.zonecut_sigrdataset = NULL;
				search.need_cleanup = ISC_TRUE;
				maybe_zonecut = ISC_FALSE;
				at_zonecut = ISC_TRUE;
				/*
				 * It is not clear if KEY should still be
				 * allowed at the parent side of the zone
				 * cut or not.  It is needed for RFC3007
				 * validated updates.
				 */
				if ((search.options & DNS_DBFIND_GLUEOK) == 0
				    && type != dns_rdatatype_nsec
				    && type != dns_rdatatype_key) {
					/*
					 * Glue is not OK, but any answer we
					 * could return would be glue.  Return
					 * the delegation.
					 */
					found = NULL;
					break;
				}
				if (found != NULL && foundsig != NULL)
					break;
			}


			/*
			 * If the NSEC3 record doesn't match the chain
			 * we are using behave as if it isn't here.
			 */
			if (header->type == dns_rdatatype_nsec3 &&
			    !matchparams(header, &search))
				goto partial_match;
			/*
			 * If we found a type we were looking for,
			 * remember it.
			 */
			if (header->type == type ||
			    type == dns_rdatatype_any ||
			    (header->type == dns_rdatatype_cname &&
			     cname_ok)) {
				/*
				 * We've found the answer!
				 */
				found = header;
				if (header->type == dns_rdatatype_cname &&
				    cname_ok) {
					/*
					 * We may be finding a CNAME instead
					 * of the desired type.
					 *
					 * If we've already got the CNAME RRSIG,
					 * use it, otherwise change sigtype
					 * so that we find it.
					 */
					if (cnamesig != NULL)
						foundsig = cnamesig;
					else
						sigtype =
						    RBTDB_RDATATYPE_SIGCNAME;
				}
				/*
				 * If we've got all we need, end the search.
				 */
				if (!maybe_zonecut && foundsig != NULL)
					break;
			} else if (header->type == sigtype) {
				/*
				 * We've found the RRSIG rdataset for our
				 * target type.  Remember it.
				 */
				foundsig = header;
				/*
				 * If we've got all we need, end the search.
				 */
				if (!maybe_zonecut && found != NULL)
					break;
			} else if (header->type == dns_rdatatype_nsec &&
				   !search.rbtversion->havensec3) {
				/*
				 * Remember a NSEC rdataset even if we're
				 * not specifically looking for it, because
				 * we might need it later.
				 */
				nsecheader = header;
			} else if (header->type == RBTDB_RDATATYPE_SIGNSEC &&
				   !search.rbtversion->havensec3) {
				/*
				 * If we need the NSEC rdataset, we'll also
				 * need its signature.
				 */
				nsecsig = header;
			} else if (cname_ok &&
				   header->type == RBTDB_RDATATYPE_SIGCNAME) {
				/*
				 * If we get a CNAME match, we'll also need
				 * its signature.
				 */
				cnamesig = header;
			}
		}
	}

	if (empty_node) {
		/*
		 * We have an exact match for the name, but there are no
		 * active rdatasets in the desired version.  That means that
		 * this node doesn't exist in the desired version, and that
		 * we really have a partial match.
		 */
		if (!wild) {
			lock = &search.rbtdb->node_locks[node->locknum].lock;
			NODE_UNLOCK(lock, isc_rwlocktype_read);
			goto partial_match;
		}
	}

	/*
	 * If we didn't find what we were looking for...
	 */
	if (found == NULL) {
		if (search.zonecut != NULL) {
			/*
			 * We were trying to find glue at a node beneath a
			 * zone cut, but didn't.
			 *
			 * Return the delegation.
			 */
			lock = &search.rbtdb->node_locks[node->locknum].lock;
			NODE_UNLOCK(lock, isc_rwlocktype_read);
			result = setup_delegation(&search, nodep, foundname,
						  rdataset, sigrdataset);
			goto tree_exit;
		}
		/*
		 * The desired type doesn't exist.
		 */
		result = DNS_R_NXRRSET;
		if (search.rbtversion->secure == dns_db_secure &&
		    !search.rbtversion->havensec3 &&
		    (nsecheader == NULL || nsecsig == NULL)) {
			/*
			 * The zone is secure but there's no NSEC,
			 * or the NSEC has no signature!
			 */
			if (!wild) {
				result = DNS_R_BADDB;
				goto node_exit;
			}

			lock = &search.rbtdb->node_locks[node->locknum].lock;
			NODE_UNLOCK(lock, isc_rwlocktype_read);
			result = find_closest_nsec(&search, nodep, foundname,
						   rdataset, sigrdataset,
						   search.rbtdb->tree,
						   search.rbtversion->secure);
			if (result == ISC_R_SUCCESS)
				result = DNS_R_EMPTYWILD;
			goto tree_exit;
		}
		if ((search.options & DNS_DBFIND_FORCENSEC) != 0 &&
		    nsecheader == NULL)
		{
			/*
			 * There's no NSEC record, and we were told
			 * to find one.
			 */
			result = DNS_R_BADDB;
			goto node_exit;
		}
		if (nodep != NULL) {
			new_reference(search.rbtdb, node);
			*nodep = node;
		}
		if ((search.rbtversion->secure == dns_db_secure &&
		     !search.rbtversion->havensec3) ||
		    (search.options & DNS_DBFIND_FORCENSEC) != 0)
		{
			bind_rdataset(search.rbtdb, node, nsecheader,
				      0, rdataset);
			if (nsecsig != NULL)
				bind_rdataset(search.rbtdb, node,
					      nsecsig, 0, sigrdataset);
		}
		if (wild)
			foundname->attributes |= DNS_NAMEATTR_WILDCARD;
		goto node_exit;
	}

	/*
	 * We found what we were looking for, or we found a CNAME.
	 */

	if (type != found->type &&
	    type != dns_rdatatype_any &&
	    found->type == dns_rdatatype_cname) {
		/*
		 * We weren't doing an ANY query and we found a CNAME instead
		 * of the type we were looking for, so we need to indicate
		 * that result to the caller.
		 */
		result = DNS_R_CNAME;
	} else if (search.zonecut != NULL) {
		/*
		 * If we're beneath a zone cut, we must indicate that the
		 * result is glue, unless we're actually at the zone cut
		 * and the type is NSEC or KEY.
		 */
		if (search.zonecut == node) {
			/*
			 * It is not clear if KEY should still be
			 * allowed at the parent side of the zone
			 * cut or not.  It is needed for RFC3007
			 * validated updates.
			 */
			if (type == dns_rdatatype_nsec ||
			    type == dns_rdatatype_nsec3 ||
			    type == dns_rdatatype_key)
				result = ISC_R_SUCCESS;
			else if (type == dns_rdatatype_any)
				result = DNS_R_ZONECUT;
			else
				result = DNS_R_GLUE;
		} else
			result = DNS_R_GLUE;
		/*
		 * We might have found data that isn't glue, but was occluded
		 * by a dynamic update.  If the caller cares about this, they
		 * will have told us to validate glue.
		 *
		 * XXX We should cache the glue validity state!
		 */
		if (result == DNS_R_GLUE &&
		    (search.options & DNS_DBFIND_VALIDATEGLUE) != 0 &&
		    !valid_glue(&search, foundname, type, node)) {
			lock = &search.rbtdb->node_locks[node->locknum].lock;
			NODE_UNLOCK(lock, isc_rwlocktype_read);
			result = setup_delegation(&search, nodep, foundname,
						  rdataset, sigrdataset);
		    goto tree_exit;
		}
	} else {
		/*
		 * An ordinary successful query!
		 */
		result = ISC_R_SUCCESS;
	}

	if (nodep != NULL) {
		if (!at_zonecut)
			new_reference(search.rbtdb, node);
		else
			search.need_cleanup = ISC_FALSE;
		*nodep = node;
	}

	if (type != dns_rdatatype_any) {
		bind_rdataset(search.rbtdb, node, found, 0, rdataset);
		if (foundsig != NULL)
			bind_rdataset(search.rbtdb, node, foundsig, 0,
				      sigrdataset);
	}

	if (wild)
		foundname->attributes |= DNS_NAMEATTR_WILDCARD;

 node_exit:
	NODE_UNLOCK(&(search.rbtdb->node_locks[node->locknum].lock),
		    isc_rwlocktype_read);

 tree_exit:
	RWUNLOCK(&search.rbtdb->tree_lock, isc_rwlocktype_read);

	/*
	 * If we found a zonecut but aren't going to use it, we have to
	 * let go of it.
	 */
	if (search.need_cleanup) {
		node = search.zonecut;
		lock = &(search.rbtdb->node_locks[node->locknum].lock);

		NODE_LOCK(lock, isc_rwlocktype_read);
		decrement_reference(search.rbtdb, node, 0,
				    isc_rwlocktype_read, isc_rwlocktype_none,
				    ISC_FALSE);
		NODE_UNLOCK(lock, isc_rwlocktype_read);
	}

	if (close_version)
		closeversion(db, &version, ISC_FALSE);

	dns_rbtnodechain_reset(&search.chain);

	return (result);
}

static isc_result_t
zone_findzonecut(dns_db_t *db, dns_name_t *name, unsigned int options,
		 isc_stdtime_t now, dns_dbnode_t **nodep,
		 dns_name_t *foundname,
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

	FATAL_ERROR(__FILE__, __LINE__, "zone_findzonecut() called!");

	return (ISC_R_NOTIMPLEMENTED);
}

static isc_result_t
cache_zonecut_callback(dns_rbtnode_t *node, dns_name_t *name, void *arg) {
	rbtdb_search_t *search = arg;
	rdatasetheader_t *header, *header_prev, *header_next;
	rdatasetheader_t *dname_header, *sigdname_header;
	isc_result_t result;
	nodelock_t *lock;
	isc_rwlocktype_t locktype;

	/* XXX comment */

	REQUIRE(search->zonecut == NULL);

	/*
	 * Keep compiler silent.
	 */
	UNUSED(name);

	lock = &(search->rbtdb->node_locks[node->locknum].lock);
	locktype = isc_rwlocktype_read;
	NODE_LOCK(lock, locktype);

	/*
	 * Look for a DNAME or RRSIG DNAME rdataset.
	 */
	dname_header = NULL;
	sigdname_header = NULL;
	header_prev = NULL;
	for (header = node->data; header != NULL; header = header_next) {
		header_next = header->next;
		if (header->rdh_ttl <= search->now) {
			/*
			 * This rdataset is stale.  If no one else is
			 * using the node, we can clean it up right
			 * now, otherwise we mark it as stale, and
			 * the node as dirty, so it will get cleaned
			 * up later.
			 */
			if ((header->rdh_ttl <= search->now - RBTDB_VIRTUAL) &&
			    (locktype == isc_rwlocktype_write ||
			     NODE_TRYUPGRADE(lock) == ISC_R_SUCCESS)) {
				/*
				 * We update the node's status only when we
				 * can get write access; otherwise, we leave
				 * others to this work.  Periodical cleaning
				 * will eventually take the job as the last
				 * resort.
				 * We won't downgrade the lock, since other
				 * rdatasets are probably stale, too.
				 */
				locktype = isc_rwlocktype_write;

				if (dns_rbtnode_refcurrent(node) == 0) {
					isc_mem_t *mctx;

					/*
					 * header->down can be non-NULL if the
					 * refcount has just decremented to 0
					 * but decrement_reference() has not
					 * performed clean_cache_node(), in
					 * which case we need to purge the
					 * stale headers first.
					 */
					mctx = search->rbtdb->common.mctx;
					clean_stale_headers(search->rbtdb,
							    mctx,
							    header);
					if (header_prev != NULL)
						header_prev->next =
							header->next;
					else
						node->data = header->next;
					free_rdataset(search->rbtdb, mctx,
						      header);
				} else {
					header->attributes |=
						RDATASET_ATTR_STALE;
					node->dirty = 1;
					header_prev = header;
				}
			} else
				header_prev = header;
		} else if (header->type == dns_rdatatype_dname &&
			   EXISTS(header)) {
			dname_header = header;
			header_prev = header;
		} else if (header->type == RBTDB_RDATATYPE_SIGDNAME &&
			 EXISTS(header)) {
			sigdname_header = header;
			header_prev = header;
		} else
			header_prev = header;
	}

	if (dname_header != NULL &&
	    (dname_header->trust != dns_trust_pending ||
	     (search->options & DNS_DBFIND_PENDINGOK) != 0)) {
		/*
		 * We increment the reference count on node to ensure that
		 * search->zonecut_rdataset will still be valid later.
		 */
		new_reference(search->rbtdb, node);
		INSIST(!ISC_LINK_LINKED(node, deadlink));
		search->zonecut = node;
		search->zonecut_rdataset = dname_header;
		search->zonecut_sigrdataset = sigdname_header;
		search->need_cleanup = ISC_TRUE;
		result = DNS_R_PARTIALMATCH;
	} else
		result = DNS_R_CONTINUE;

	NODE_UNLOCK(lock, locktype);

	return (result);
}

static inline isc_result_t
find_deepest_zonecut(rbtdb_search_t *search, dns_rbtnode_t *node,
		     dns_dbnode_t **nodep, dns_name_t *foundname,
		     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	unsigned int i;
	dns_rbtnode_t *level_node;
	rdatasetheader_t *header, *header_prev, *header_next;
	rdatasetheader_t *found, *foundsig;
	isc_result_t result = ISC_R_NOTFOUND;
	dns_name_t name;
	dns_rbtdb_t *rbtdb;
	isc_boolean_t done;
	nodelock_t *lock;
	isc_rwlocktype_t locktype;

	/*
	 * Caller must be holding the tree lock.
	 */

	rbtdb = search->rbtdb;
	i = search->chain.level_matches;
	done = ISC_FALSE;
	do {
		locktype = isc_rwlocktype_read;
		lock = &rbtdb->node_locks[node->locknum].lock;
		NODE_LOCK(lock, locktype);

		/*
		 * Look for NS and RRSIG NS rdatasets.
		 */
		found = NULL;
		foundsig = NULL;
		header_prev = NULL;
		for (header = node->data;
		     header != NULL;
		     header = header_next) {
			header_next = header->next;
			if (header->rdh_ttl <= search->now) {
				/*
				 * This rdataset is stale.  If no one else is
				 * using the node, we can clean it up right
				 * now, otherwise we mark it as stale, and
				 * the node as dirty, so it will get cleaned
				 * up later.
				 */
				if ((header->rdh_ttl <= search->now -
						    RBTDB_VIRTUAL) &&
				    (locktype == isc_rwlocktype_write ||
				     NODE_TRYUPGRADE(lock) == ISC_R_SUCCESS)) {
					/*
					 * We update the node's status only
					 * when we can get write access.
					 */
					locktype = isc_rwlocktype_write;

					if (dns_rbtnode_refcurrent(node)
					    == 0) {
						isc_mem_t *m;

						m = search->rbtdb->common.mctx;
						clean_stale_headers(
							search->rbtdb,
							m, header);
						if (header_prev != NULL)
							header_prev->next =
								header->next;
						else
							node->data =
								header->next;
						free_rdataset(rbtdb, m,
							      header);
					} else {
						header->attributes |=
							RDATASET_ATTR_STALE;
						node->dirty = 1;
						header_prev = header;
					}
				} else
					header_prev = header;
			} else if (EXISTS(header)) {
				/*
				 * We've found an extant rdataset.  See if
				 * we're interested in it.
				 */
				if (header->type == dns_rdatatype_ns) {
					found = header;
					if (foundsig != NULL)
						break;
				} else if (header->type ==
					   RBTDB_RDATATYPE_SIGNS) {
					foundsig = header;
					if (found != NULL)
						break;
				}
				header_prev = header;
			} else
				header_prev = header;
		}

		if (found != NULL) {
			/*
			 * If we have to set foundname, we do it before
			 * anything else.  If we were to set foundname after
			 * we had set nodep or bound the rdataset, then we'd
			 * have to undo that work if dns_name_concatenate()
			 * failed.  By setting foundname first, there's
			 * nothing to undo if we have trouble.
			 */
			if (foundname != NULL) {
				dns_name_init(&name, NULL);
				dns_rbt_namefromnode(node, &name);
				result = dns_name_copy(&name, foundname, NULL);
				while (result == ISC_R_SUCCESS && i > 0) {
					i--;
					level_node = search->chain.levels[i];
					dns_name_init(&name, NULL);
					dns_rbt_namefromnode(level_node,
							     &name);
					result =
						dns_name_concatenate(foundname,
								     &name,
								     foundname,
								     NULL);
				}
				if (result != ISC_R_SUCCESS) {
					*nodep = NULL;
					goto node_exit;
				}
			}
			result = DNS_R_DELEGATION;
			if (nodep != NULL) {
				new_reference(search->rbtdb, node);
				*nodep = node;
			}
			bind_rdataset(search->rbtdb, node, found, search->now,
				      rdataset);
			if (foundsig != NULL)
				bind_rdataset(search->rbtdb, node, foundsig,
					      search->now, sigrdataset);
			if (need_headerupdate(found, search->now) ||
			    (foundsig != NULL &&
			     need_headerupdate(foundsig, search->now))) {
				if (locktype != isc_rwlocktype_write) {
					NODE_UNLOCK(lock, locktype);
					NODE_LOCK(lock, isc_rwlocktype_write);
					locktype = isc_rwlocktype_write;
				}
				if (need_headerupdate(found, search->now))
					update_header(search->rbtdb, found,
						      search->now);
				if (foundsig != NULL &&
				    need_headerupdate(foundsig, search->now)) {
					update_header(search->rbtdb, foundsig,
						      search->now);
				}
			}
		}

	node_exit:
		NODE_UNLOCK(lock, locktype);

		if (found == NULL && i > 0) {
			i--;
			node = search->chain.levels[i];
		} else
			done = ISC_TRUE;

	} while (!done);

	return (result);
}

static isc_result_t
find_coveringnsec(rbtdb_search_t *search, dns_dbnode_t **nodep,
		  isc_stdtime_t now, dns_name_t *foundname,
		  dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	dns_rbtnode_t *node;
	rdatasetheader_t *header, *header_next, *header_prev;
	rdatasetheader_t *found, *foundsig;
	isc_boolean_t empty_node;
	isc_result_t result;
	dns_fixedname_t fname, forigin;
	dns_name_t *name, *origin;
	rbtdb_rdatatype_t matchtype, sigmatchtype;
	nodelock_t *lock;
	isc_rwlocktype_t locktype;

	matchtype = RBTDB_RDATATYPE_VALUE(dns_rdatatype_nsec, 0);
	sigmatchtype = RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig,
					     dns_rdatatype_nsec);

	do {
		node = NULL;
		dns_fixedname_init(&fname);
		name = dns_fixedname_name(&fname);
		dns_fixedname_init(&forigin);
		origin = dns_fixedname_name(&forigin);
		result = dns_rbtnodechain_current(&search->chain, name,
						  origin, &node);
		if (result != ISC_R_SUCCESS)
			return (result);
		locktype = isc_rwlocktype_read;
		lock = &(search->rbtdb->node_locks[node->locknum].lock);
		NODE_LOCK(lock, locktype);
		found = NULL;
		foundsig = NULL;
		empty_node = ISC_TRUE;
		header_prev = NULL;
		for (header = node->data;
		     header != NULL;
		     header = header_next) {
			header_next = header->next;
			if (header->rdh_ttl <= now) {
				/*
				 * This rdataset is stale.  If no one else is
				 * using the node, we can clean it up right
				 * now, otherwise we mark it as stale, and the
				 * node as dirty, so it will get cleaned up
				 * later.
				 */
				if ((header->rdh_ttl <= now - RBTDB_VIRTUAL) &&
				    (locktype == isc_rwlocktype_write ||
				     NODE_TRYUPGRADE(lock) == ISC_R_SUCCESS)) {
					/*
					 * We update the node's status only
					 * when we can get write access.
					 */
					locktype = isc_rwlocktype_write;

					if (dns_rbtnode_refcurrent(node)
					    == 0) {
						isc_mem_t *m;

						m = search->rbtdb->common.mctx;
						clean_stale_headers(
							search->rbtdb,
							m, header);
						if (header_prev != NULL)
							header_prev->next =
								header->next;
						else
							node->data = header->next;
						free_rdataset(search->rbtdb, m,
							      header);
					} else {
						header->attributes |=
							RDATASET_ATTR_STALE;
						node->dirty = 1;
						header_prev = header;
					}
				} else
					header_prev = header;
				continue;
			}
			if (NONEXISTENT(header) ||
			    RBTDB_RDATATYPE_BASE(header->type) == 0) {
				header_prev = header;
				continue;
			}
			empty_node = ISC_FALSE;
			if (header->type == matchtype)
				found = header;
			else if (header->type == sigmatchtype)
				foundsig = header;
			header_prev = header;
		}
		if (found != NULL) {
			result = dns_name_concatenate(name, origin,
						      foundname, NULL);
			if (result != ISC_R_SUCCESS)
				goto unlock_node;
			bind_rdataset(search->rbtdb, node, found,
				      now, rdataset);
			if (foundsig != NULL)
				bind_rdataset(search->rbtdb, node, foundsig,
					      now, sigrdataset);
			new_reference(search->rbtdb, node);
			*nodep = node;
			result = DNS_R_COVERINGNSEC;
		} else if (!empty_node) {
			result = ISC_R_NOTFOUND;
		} else
			result = dns_rbtnodechain_prev(&search->chain, NULL,
						       NULL);
 unlock_node:
		NODE_UNLOCK(lock, locktype);
	} while (empty_node && result == ISC_R_SUCCESS);
	return (result);
}

static isc_result_t
cache_find(dns_db_t *db, dns_name_t *name, dns_dbversion_t *version,
	   dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	   dns_dbnode_t **nodep, dns_name_t *foundname,
	   dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	dns_rbtnode_t *node = NULL;
	isc_result_t result;
	rbtdb_search_t search;
	isc_boolean_t cname_ok = ISC_TRUE;
	isc_boolean_t empty_node;
	nodelock_t *lock;
	isc_rwlocktype_t locktype;
	rdatasetheader_t *header, *header_prev, *header_next;
	rdatasetheader_t *found, *nsheader;
	rdatasetheader_t *foundsig, *nssig, *cnamesig;
	rdatasetheader_t *update, *updatesig;
	rbtdb_rdatatype_t sigtype, negtype;

	UNUSED(version);

	search.rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(search.rbtdb));
	REQUIRE(version == NULL);

	if (now == 0)
		isc_stdtime_get(&now);

	search.rbtversion = NULL;
	search.serial = 1;
	search.options = options;
	search.copy_name = ISC_FALSE;
	search.need_cleanup = ISC_FALSE;
	search.wild = ISC_FALSE;
	search.zonecut = NULL;
	dns_fixedname_init(&search.zonecut_name);
	dns_rbtnodechain_init(&search.chain, search.rbtdb->common.mctx);
	search.now = now;
	update = NULL;
	updatesig = NULL;

	RWLOCK(&search.rbtdb->tree_lock, isc_rwlocktype_read);

	/*
	 * Search down from the root of the tree.  If, while going down, we
	 * encounter a callback node, cache_zonecut_callback() will search the
	 * rdatasets at the zone cut for a DNAME rdataset.
	 */
	result = dns_rbt_findnode(search.rbtdb->tree, name, foundname, &node,
				  &search.chain, DNS_RBTFIND_EMPTYDATA,
				  cache_zonecut_callback, &search);

	if (result == DNS_R_PARTIALMATCH) {
		if ((search.options & DNS_DBFIND_COVERINGNSEC) != 0) {
			result = find_coveringnsec(&search, nodep, now,
						   foundname, rdataset,
						   sigrdataset);
			if (result == DNS_R_COVERINGNSEC)
				goto tree_exit;
		}
		if (search.zonecut != NULL) {
		    result = setup_delegation(&search, nodep, foundname,
					      rdataset, sigrdataset);
		    goto tree_exit;
		} else {
		find_ns:
			result = find_deepest_zonecut(&search, node, nodep,
						      foundname, rdataset,
						      sigrdataset);
			goto tree_exit;
		}
	} else if (result != ISC_R_SUCCESS)
		goto tree_exit;

	/*
	 * Certain DNSSEC types are not subject to CNAME matching
	 * (RFC4035, section 2.5 and RFC3007).
	 *
	 * We don't check for RRSIG, because we don't store RRSIG records
	 * directly.
	 */
	if (type == dns_rdatatype_key || type == dns_rdatatype_nsec)
		cname_ok = ISC_FALSE;

	/*
	 * We now go looking for rdata...
	 */

	lock = &(search.rbtdb->node_locks[node->locknum].lock);
	locktype = isc_rwlocktype_read;
	NODE_LOCK(lock, locktype);

	found = NULL;
	foundsig = NULL;
	sigtype = RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, type);
	negtype = RBTDB_RDATATYPE_VALUE(0, type);
	nsheader = NULL;
	nssig = NULL;
	cnamesig = NULL;
	empty_node = ISC_TRUE;
	header_prev = NULL;
	for (header = node->data; header != NULL; header = header_next) {
		header_next = header->next;
		if (header->rdh_ttl <= now) {
			/*
			 * This rdataset is stale.  If no one else is using the
			 * node, we can clean it up right now, otherwise we
			 * mark it as stale, and the node as dirty, so it will
			 * get cleaned up later.
			 */
			if ((header->rdh_ttl <= now - RBTDB_VIRTUAL) &&
			    (locktype == isc_rwlocktype_write ||
			     NODE_TRYUPGRADE(lock) == ISC_R_SUCCESS)) {
				/*
				 * We update the node's status only when we
				 * can get write access.
				 */
				locktype = isc_rwlocktype_write;

				if (dns_rbtnode_refcurrent(node) == 0) {
					isc_mem_t *mctx;

					mctx = search.rbtdb->common.mctx;
					clean_stale_headers(search.rbtdb, mctx,
							    header);
					if (header_prev != NULL)
						header_prev->next =
							header->next;
					else
						node->data = header->next;
					free_rdataset(search.rbtdb, mctx,
						      header);
				} else {
					header->attributes |=
						RDATASET_ATTR_STALE;
					node->dirty = 1;
					header_prev = header;
				}
			} else
				header_prev = header;
		} else if (EXISTS(header)) {
			/*
			 * We now know that there is at least one active
			 * non-stale rdataset at this node.
			 */
			empty_node = ISC_FALSE;

			/*
			 * If we found a type we were looking for, remember
			 * it.
			 */
			if (header->type == type ||
			    (type == dns_rdatatype_any &&
			     RBTDB_RDATATYPE_BASE(header->type) != 0) ||
			    (cname_ok && header->type ==
			     dns_rdatatype_cname)) {
				/*
				 * We've found the answer.
				 */
				found = header;
				if (header->type == dns_rdatatype_cname &&
				    cname_ok &&
				    cnamesig != NULL) {
					/*
					 * If we've already got the CNAME RRSIG,
					 * use it, otherwise change sigtype
					 * so that we find it.
					 */
					if (cnamesig != NULL)
						foundsig = cnamesig;
					else
						sigtype =
						    RBTDB_RDATATYPE_SIGCNAME;
					foundsig = cnamesig;
				}
			} else if (header->type == sigtype) {
				/*
				 * We've found the RRSIG rdataset for our
				 * target type.  Remember it.
				 */
				foundsig = header;
			} else if (header->type == RBTDB_RDATATYPE_NCACHEANY ||
				   header->type == negtype) {
				/*
				 * We've found a negative cache entry.
				 */
				found = header;
			} else if (header->type == dns_rdatatype_ns) {
				/*
				 * Remember a NS rdataset even if we're
				 * not specifically looking for it, because
				 * we might need it later.
				 */
				nsheader = header;
			} else if (header->type == RBTDB_RDATATYPE_SIGNS) {
				/*
				 * If we need the NS rdataset, we'll also
				 * need its signature.
				 */
				nssig = header;
			} else if (cname_ok &&
				   header->type == RBTDB_RDATATYPE_SIGCNAME) {
				/*
				 * If we get a CNAME match, we'll also need
				 * its signature.
				 */
				cnamesig = header;
			}
			header_prev = header;
		} else
			header_prev = header;
	}

	if (empty_node) {
		/*
		 * We have an exact match for the name, but there are no
		 * extant rdatasets.  That means that this node doesn't
		 * meaningfully exist, and that we really have a partial match.
		 */
		NODE_UNLOCK(lock, locktype);
		goto find_ns;
	}

	/*
	 * If we didn't find what we were looking for...
	 */
	if (found == NULL ||
	    (found->trust == dns_trust_glue &&
	     ((options & DNS_DBFIND_GLUEOK) == 0)) ||
	    (found->trust == dns_trust_pending &&
	     ((options & DNS_DBFIND_PENDINGOK) == 0))) {
		/*
		 * If there is an NS rdataset at this node, then this is the
		 * deepest zone cut.
		 */
		if (nsheader != NULL) {
			if (nodep != NULL) {
				new_reference(search.rbtdb, node);
				INSIST(!ISC_LINK_LINKED(node, deadlink));
				*nodep = node;
			}
			bind_rdataset(search.rbtdb, node, nsheader, search.now,
				      rdataset);
			if (need_headerupdate(nsheader, search.now))
				update = nsheader;
			if (nssig != NULL) {
				bind_rdataset(search.rbtdb, node, nssig,
					      search.now, sigrdataset);
				if (need_headerupdate(nssig, search.now))
					updatesig = nssig;
			}
			result = DNS_R_DELEGATION;
			goto node_exit;
		}

		/*
		 * Go find the deepest zone cut.
		 */
		NODE_UNLOCK(lock, locktype);
		goto find_ns;
	}

	/*
	 * We found what we were looking for, or we found a CNAME.
	 */

	if (nodep != NULL) {
		new_reference(search.rbtdb, node);
		INSIST(!ISC_LINK_LINKED(node, deadlink));
		*nodep = node;
	}

	if (RBTDB_RDATATYPE_BASE(found->type) == 0) {
		/*
		 * We found a negative cache entry.
		 */
		if (NXDOMAIN(found))
			result = DNS_R_NCACHENXDOMAIN;
		else
			result = DNS_R_NCACHENXRRSET;
	} else if (type != found->type &&
		   type != dns_rdatatype_any &&
		   found->type == dns_rdatatype_cname) {
		/*
		 * We weren't doing an ANY query and we found a CNAME instead
		 * of the type we were looking for, so we need to indicate
		 * that result to the caller.
		 */
		result = DNS_R_CNAME;
	} else {
		/*
		 * An ordinary successful query!
		 */
		result = ISC_R_SUCCESS;
	}

	if (type != dns_rdatatype_any || result == DNS_R_NCACHENXDOMAIN ||
	    result == DNS_R_NCACHENXRRSET) {
		bind_rdataset(search.rbtdb, node, found, search.now,
			      rdataset);
		if (need_headerupdate(found, search.now))
			update = found;
		if (foundsig != NULL) {
			bind_rdataset(search.rbtdb, node, foundsig, search.now,
				      sigrdataset);
			if (need_headerupdate(foundsig, search.now))
				updatesig = foundsig;
		}
	}

 node_exit:
	if ((update != NULL || updatesig != NULL) &&
	    locktype != isc_rwlocktype_write) {
		NODE_UNLOCK(lock, locktype);
		NODE_LOCK(lock, isc_rwlocktype_write);
		locktype = isc_rwlocktype_write;
	}
	if (update != NULL && need_headerupdate(update, search.now))
		update_header(search.rbtdb, update, search.now);
	if (updatesig != NULL && need_headerupdate(updatesig, search.now))
		update_header(search.rbtdb, updatesig, search.now);

	NODE_UNLOCK(lock, locktype);

 tree_exit:
	RWUNLOCK(&search.rbtdb->tree_lock, isc_rwlocktype_read);

	/*
	 * If we found a zonecut but aren't going to use it, we have to
	 * let go of it.
	 */
	if (search.need_cleanup) {
		node = search.zonecut;
		lock = &(search.rbtdb->node_locks[node->locknum].lock);

		NODE_LOCK(lock, isc_rwlocktype_read);
		decrement_reference(search.rbtdb, node, 0,
				    isc_rwlocktype_read, isc_rwlocktype_none,
				    ISC_FALSE);
		NODE_UNLOCK(lock, isc_rwlocktype_read);
	}

	dns_rbtnodechain_reset(&search.chain);

	return (result);
}

static isc_result_t
cache_findzonecut(dns_db_t *db, dns_name_t *name, unsigned int options,
		  isc_stdtime_t now, dns_dbnode_t **nodep,
		  dns_name_t *foundname,
		  dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	dns_rbtnode_t *node = NULL;
	nodelock_t *lock;
	isc_result_t result;
	rbtdb_search_t search;
	rdatasetheader_t *header, *header_prev, *header_next;
	rdatasetheader_t *found, *foundsig;
	unsigned int rbtoptions = DNS_RBTFIND_EMPTYDATA;
	isc_rwlocktype_t locktype;

	search.rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(search.rbtdb));

	if (now == 0)
		isc_stdtime_get(&now);

	search.rbtversion = NULL;
	search.serial = 1;
	search.options = options;
	search.copy_name = ISC_FALSE;
	search.need_cleanup = ISC_FALSE;
	search.wild = ISC_FALSE;
	search.zonecut = NULL;
	dns_fixedname_init(&search.zonecut_name);
	dns_rbtnodechain_init(&search.chain, search.rbtdb->common.mctx);
	search.now = now;

	if ((options & DNS_DBFIND_NOEXACT) != 0)
		rbtoptions |= DNS_RBTFIND_NOEXACT;

	RWLOCK(&search.rbtdb->tree_lock, isc_rwlocktype_read);

	/*
	 * Search down from the root of the tree.
	 */
	result = dns_rbt_findnode(search.rbtdb->tree, name, foundname, &node,
				  &search.chain, rbtoptions, NULL, &search);

	if (result == DNS_R_PARTIALMATCH) {
	find_ns:
		result = find_deepest_zonecut(&search, node, nodep, foundname,
					      rdataset, sigrdataset);
		goto tree_exit;
	} else if (result != ISC_R_SUCCESS)
		goto tree_exit;

	/*
	 * We now go looking for an NS rdataset at the node.
	 */

	lock = &(search.rbtdb->node_locks[node->locknum].lock);
	locktype = isc_rwlocktype_read;
	NODE_LOCK(lock, locktype);

	found = NULL;
	foundsig = NULL;
	header_prev = NULL;
	for (header = node->data; header != NULL; header = header_next) {
		header_next = header->next;
		if (header->rdh_ttl <= now) {
			/*
			 * This rdataset is stale.  If no one else is using the
			 * node, we can clean it up right now, otherwise we
			 * mark it as stale, and the node as dirty, so it will
			 * get cleaned up later.
			 */
			if ((header->rdh_ttl <= now - RBTDB_VIRTUAL) &&
			    (locktype == isc_rwlocktype_write ||
			     NODE_TRYUPGRADE(lock) == ISC_R_SUCCESS)) {
				/*
				 * We update the node's status only when we
				 * can get write access.
				 */
				locktype = isc_rwlocktype_write;

				if (dns_rbtnode_refcurrent(node) == 0) {
					isc_mem_t *mctx;

					mctx = search.rbtdb->common.mctx;
					clean_stale_headers(search.rbtdb, mctx,
							    header);
					if (header_prev != NULL)
						header_prev->next =
							header->next;
					else
						node->data = header->next;
					free_rdataset(search.rbtdb, mctx,
						      header);
				} else {
					header->attributes |=
						RDATASET_ATTR_STALE;
					node->dirty = 1;
					header_prev = header;
				}
			} else
				header_prev = header;
		} else if (EXISTS(header)) {
			/*
			 * If we found a type we were looking for, remember
			 * it.
			 */
			if (header->type == dns_rdatatype_ns) {
				/*
				 * Remember a NS rdataset even if we're
				 * not specifically looking for it, because
				 * we might need it later.
				 */
				found = header;
			} else if (header->type == RBTDB_RDATATYPE_SIGNS) {
				/*
				 * If we need the NS rdataset, we'll also
				 * need its signature.
				 */
				foundsig = header;
			}
			header_prev = header;
		} else
			header_prev = header;
	}

	if (found == NULL) {
		/*
		 * No NS records here.
		 */
		NODE_UNLOCK(lock, locktype);
		goto find_ns;
	}

	if (nodep != NULL) {
		new_reference(search.rbtdb, node);
		INSIST(!ISC_LINK_LINKED(node, deadlink));
		*nodep = node;
	}

	bind_rdataset(search.rbtdb, node, found, search.now, rdataset);
	if (foundsig != NULL)
		bind_rdataset(search.rbtdb, node, foundsig, search.now,
			      sigrdataset);

	if (need_headerupdate(found, search.now) ||
	    (foundsig != NULL &&  need_headerupdate(foundsig, search.now))) {
		if (locktype != isc_rwlocktype_write) {
			NODE_UNLOCK(lock, locktype);
			NODE_LOCK(lock, isc_rwlocktype_write);
			locktype = isc_rwlocktype_write;
		}
		if (need_headerupdate(found, search.now))
			update_header(search.rbtdb, found, search.now);
		if (foundsig != NULL &&
		    need_headerupdate(foundsig, search.now)) {
			update_header(search.rbtdb, foundsig, search.now);
		}
	}

	NODE_UNLOCK(lock, locktype);

 tree_exit:
	RWUNLOCK(&search.rbtdb->tree_lock, isc_rwlocktype_read);

	INSIST(!search.need_cleanup);

	dns_rbtnodechain_reset(&search.chain);

	if (result == DNS_R_DELEGATION)
		result = ISC_R_SUCCESS;

	return (result);
}

static void
attachnode(dns_db_t *db, dns_dbnode_t *source, dns_dbnode_t **targetp) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *node = (dns_rbtnode_t *)source;
	unsigned int refs;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(targetp != NULL && *targetp == NULL);

	NODE_STRONGLOCK(&rbtdb->node_locks[node->locknum].lock);
	dns_rbtnode_refincrement(node, &refs);
	INSIST(refs != 0);
	NODE_STRONGUNLOCK(&rbtdb->node_locks[node->locknum].lock);

	*targetp = source;
}

static void
detachnode(dns_db_t *db, dns_dbnode_t **targetp) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *node;
	isc_boolean_t want_free = ISC_FALSE;
	isc_boolean_t inactive = ISC_FALSE;
	rbtdb_nodelock_t *nodelock;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(targetp != NULL && *targetp != NULL);

	node = (dns_rbtnode_t *)(*targetp);
	nodelock = &rbtdb->node_locks[node->locknum];

	NODE_LOCK(&nodelock->lock, isc_rwlocktype_read);

	if (decrement_reference(rbtdb, node, 0, isc_rwlocktype_read,
				isc_rwlocktype_none, ISC_FALSE)) {
		if (isc_refcount_current(&nodelock->references) == 0 &&
		    nodelock->exiting) {
			inactive = ISC_TRUE;
		}
	}

	NODE_UNLOCK(&nodelock->lock, isc_rwlocktype_read);

	*targetp = NULL;

	if (inactive) {
		RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_write);
		rbtdb->active--;
		if (rbtdb->active == 0)
			want_free = ISC_TRUE;
		RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_write);
		if (want_free) {
			char buf[DNS_NAME_FORMATSIZE];
			if (dns_name_dynamic(&rbtdb->common.origin))
				dns_name_format(&rbtdb->common.origin, buf,
						sizeof(buf));
			else
				strcpy(buf, "<UNKNOWN>");
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
				      DNS_LOGMODULE_CACHE, ISC_LOG_DEBUG(1),
				      "calling free_rbtdb(%s)", buf);
			free_rbtdb(rbtdb, ISC_TRUE, NULL);
		}
	}
}

static isc_result_t
expirenode(dns_db_t *db, dns_dbnode_t *node, isc_stdtime_t now) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *rbtnode = node;
	rdatasetheader_t *header;
	isc_boolean_t force_expire = ISC_FALSE;
	/*
	 * These are the category and module used by the cache cleaner.
	 */
	isc_boolean_t log = ISC_FALSE;
	isc_logcategory_t *category = DNS_LOGCATEGORY_DATABASE;
	isc_logmodule_t *module = DNS_LOGMODULE_CACHE;
	int level = ISC_LOG_DEBUG(2);
	char printname[DNS_NAME_FORMATSIZE];

	REQUIRE(VALID_RBTDB(rbtdb));

	/*
	 * Caller must hold a tree lock.
	 */

	if (now == 0)
		isc_stdtime_get(&now);

	if (rbtdb->overmem) {
		isc_uint32_t val;

		isc_random_get(&val);
		/*
		 * XXXDCL Could stand to have a better policy, like LRU.
		 */
		force_expire = ISC_TF(rbtnode->down == NULL && val % 4 == 0);

		/*
		 * Note that 'log' can be true IFF rbtdb->overmem is also true.
		 * rbtdb->overmem can currently only be true for cache
		 * databases -- hence all of the "overmem cache" log strings.
		 */
		log = ISC_TF(isc_log_wouldlog(dns_lctx, level));
		if (log)
			isc_log_write(dns_lctx, category, module, level,
				      "overmem cache: %s %s",
				      force_expire ? "FORCE" : "check",
				      dns_rbt_formatnodename(rbtnode,
							   printname,
							   sizeof(printname)));
	}

	/*
	 * We may not need write access, but this code path is not performance
	 * sensitive, so it should be okay to always lock as a writer.
	 */
	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_write);

	for (header = rbtnode->data; header != NULL; header = header->next)
		if (header->rdh_ttl <= now - RBTDB_VIRTUAL) {
			/*
			 * We don't check if refcurrent(rbtnode) == 0 and try
			 * to free like we do in cache_find(), because
			 * refcurrent(rbtnode) must be non-zero.  This is so
			 * because 'node' is an argument to the function.
			 */
			header->attributes |= RDATASET_ATTR_STALE;
			rbtnode->dirty = 1;
			if (log)
				isc_log_write(dns_lctx, category, module,
					      level, "overmem cache: stale %s",
					      printname);
		} else if (force_expire) {
			if (! RETAIN(header)) {
				set_ttl(rbtdb, header, 0);
				header->attributes |= RDATASET_ATTR_STALE;
				rbtnode->dirty = 1;
			} else if (log) {
				isc_log_write(dns_lctx, category, module,
					      level, "overmem cache: "
					      "reprieve by RETAIN() %s",
					      printname);
			}
		} else if (rbtdb->overmem && log)
			isc_log_write(dns_lctx, category, module, level,
				      "overmem cache: saved %s", printname);

	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_write);

	return (ISC_R_SUCCESS);
}

static void
overmem(dns_db_t *db, isc_boolean_t overmem) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;

	if (IS_CACHE(rbtdb))
		rbtdb->overmem = overmem;
}

static void
printnode(dns_db_t *db, dns_dbnode_t *node, FILE *out) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *rbtnode = node;
	isc_boolean_t first;

	REQUIRE(VALID_RBTDB(rbtdb));

	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_read);

	fprintf(out, "node %p, %u references, locknum = %u\n",
		rbtnode, dns_rbtnode_refcurrent(rbtnode),
		rbtnode->locknum);
	if (rbtnode->data != NULL) {
		rdatasetheader_t *current, *top_next;

		for (current = rbtnode->data; current != NULL;
		     current = top_next) {
			top_next = current->next;
			first = ISC_TRUE;
			fprintf(out, "\ttype %u", current->type);
			do {
				if (!first)
					fprintf(out, "\t");
				first = ISC_FALSE;
				fprintf(out,
					"\tserial = %lu, ttl = %u, "
					"trust = %u, attributes = %u, "
					"resign = %u\n",
					(unsigned long)current->serial,
					current->rdh_ttl,
					current->trust,
					current->attributes,
					current->resign);
				current = current->down;
			} while (current != NULL);
		}
	} else
		fprintf(out, "(empty)\n");

	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_read);
}

static isc_result_t
createiterator(dns_db_t *db, unsigned int options, dns_dbiterator_t **iteratorp)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	rbtdb_dbiterator_t *rbtdbiter;

	REQUIRE(VALID_RBTDB(rbtdb));

	rbtdbiter = isc_mem_get(rbtdb->common.mctx, sizeof(*rbtdbiter));
	if (rbtdbiter == NULL)
		return (ISC_R_NOMEMORY);

	rbtdbiter->common.methods = &dbiterator_methods;
	rbtdbiter->common.db = NULL;
	dns_db_attach(db, &rbtdbiter->common.db);
	rbtdbiter->common.relative_names =
			ISC_TF((options & DNS_DB_RELATIVENAMES) != 0);
	rbtdbiter->common.magic = DNS_DBITERATOR_MAGIC;
	rbtdbiter->common.cleaning = ISC_FALSE;
	rbtdbiter->paused = ISC_TRUE;
	rbtdbiter->tree_locked = isc_rwlocktype_none;
	rbtdbiter->result = ISC_R_SUCCESS;
	dns_fixedname_init(&rbtdbiter->name);
	dns_fixedname_init(&rbtdbiter->origin);
	rbtdbiter->node = NULL;
	rbtdbiter->delete = 0;
	rbtdbiter->nsec3only = ISC_TF((options & DNS_DB_NSEC3ONLY) != 0);
	rbtdbiter->nonsec3 = ISC_TF((options & DNS_DB_NONSEC3) != 0);
	memset(rbtdbiter->deletions, 0, sizeof(rbtdbiter->deletions));
	dns_rbtnodechain_init(&rbtdbiter->chain, db->mctx);
	dns_rbtnodechain_init(&rbtdbiter->nsec3chain, db->mctx);
	if (rbtdbiter->nsec3only)
		rbtdbiter->current = &rbtdbiter->nsec3chain;
	else
		rbtdbiter->current = &rbtdbiter->chain;

	*iteratorp = (dns_dbiterator_t *)rbtdbiter;

	return (ISC_R_SUCCESS);
}

static isc_result_t
zone_findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		  dns_rdatatype_t type, dns_rdatatype_t covers,
		  isc_stdtime_t now, dns_rdataset_t *rdataset,
		  dns_rdataset_t *sigrdataset)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *rbtnode = (dns_rbtnode_t *)node;
	rdatasetheader_t *header, *header_next, *found, *foundsig;
	rbtdb_serial_t serial;
	rbtdb_version_t *rbtversion = version;
	isc_boolean_t close_version = ISC_FALSE;
	rbtdb_rdatatype_t matchtype, sigmatchtype;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(type != dns_rdatatype_any);

	if (rbtversion == NULL) {
		currentversion(db, (dns_dbversion_t **) (void *)(&rbtversion));
		close_version = ISC_TRUE;
	}
	serial = rbtversion->serial;
	now = 0;

	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_read);

	found = NULL;
	foundsig = NULL;
	matchtype = RBTDB_RDATATYPE_VALUE(type, covers);
	if (covers == 0)
		sigmatchtype = RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, type);
	else
		sigmatchtype = 0;

	for (header = rbtnode->data; header != NULL; header = header_next) {
		header_next = header->next;
		do {
			if (header->serial <= serial &&
			    !IGNORE(header)) {
				/*
				 * Is this a "this rdataset doesn't
				 * exist" record?
				 */
				if (NONEXISTENT(header))
					header = NULL;
				break;
			} else
				header = header->down;
		} while (header != NULL);
		if (header != NULL) {
			/*
			 * We have an active, extant rdataset.  If it's a
			 * type we're looking for, remember it.
			 */
			if (header->type == matchtype) {
				found = header;
				if (foundsig != NULL)
					break;
			} else if (header->type == sigmatchtype) {
				foundsig = header;
				if (found != NULL)
					break;
			}
		}
	}
	if (found != NULL) {
		bind_rdataset(rbtdb, rbtnode, found, now, rdataset);
		if (foundsig != NULL)
			bind_rdataset(rbtdb, rbtnode, foundsig, now,
				      sigrdataset);
	}

	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_read);

	if (close_version)
		closeversion(db, (dns_dbversion_t **) (void *)(&rbtversion),
			     ISC_FALSE);

	if (found == NULL)
		return (ISC_R_NOTFOUND);

	return (ISC_R_SUCCESS);
}

static isc_result_t
cache_findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		   dns_rdatatype_t type, dns_rdatatype_t covers,
		   isc_stdtime_t now, dns_rdataset_t *rdataset,
		   dns_rdataset_t *sigrdataset)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *rbtnode = (dns_rbtnode_t *)node;
	rdatasetheader_t *header, *header_next, *found, *foundsig;
	rbtdb_rdatatype_t matchtype, sigmatchtype, negtype;
	isc_result_t result;
	nodelock_t *lock;
	isc_rwlocktype_t locktype;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(type != dns_rdatatype_any);

	UNUSED(version);

	result = ISC_R_SUCCESS;

	if (now == 0)
		isc_stdtime_get(&now);

	lock = &rbtdb->node_locks[rbtnode->locknum].lock;
	locktype = isc_rwlocktype_read;
	NODE_LOCK(lock, locktype);

	found = NULL;
	foundsig = NULL;
	matchtype = RBTDB_RDATATYPE_VALUE(type, covers);
	negtype = RBTDB_RDATATYPE_VALUE(0, type);
	if (covers == 0)
		sigmatchtype = RBTDB_RDATATYPE_VALUE(dns_rdatatype_rrsig, type);
	else
		sigmatchtype = 0;

	for (header = rbtnode->data; header != NULL; header = header_next) {
		header_next = header->next;
		if (header->rdh_ttl <= now) {
			if ((header->rdh_ttl <= now - RBTDB_VIRTUAL) &&
			    (locktype == isc_rwlocktype_write ||
			     NODE_TRYUPGRADE(lock) == ISC_R_SUCCESS)) {
				/*
				 * We update the node's status only when we
				 * can get write access.
				 */
				locktype = isc_rwlocktype_write;

				/*
				 * We don't check if refcurrent(rbtnode) == 0
				 * and try to free like we do in cache_find(),
				 * because refcurrent(rbtnode) must be
				 * non-zero.  This is so because 'node' is an
				 * argument to the function.
				 */
				header->attributes |= RDATASET_ATTR_STALE;
				rbtnode->dirty = 1;
			}
		} else if (EXISTS(header)) {
			if (header->type == matchtype)
				found = header;
			else if (header->type == RBTDB_RDATATYPE_NCACHEANY ||
				 header->type == negtype)
				found = header;
			else if (header->type == sigmatchtype)
				foundsig = header;
		}
	}
	if (found != NULL) {
		bind_rdataset(rbtdb, rbtnode, found, now, rdataset);
		if (foundsig != NULL)
			bind_rdataset(rbtdb, rbtnode, foundsig, now,
				      sigrdataset);
	}

	NODE_UNLOCK(lock, locktype);

	if (found == NULL)
		return (ISC_R_NOTFOUND);

	if (RBTDB_RDATATYPE_BASE(found->type) == 0) {
		/*
		 * We found a negative cache entry.
		 */
		if (NXDOMAIN(found))
			result = DNS_R_NCACHENXDOMAIN;
		else
			result = DNS_R_NCACHENXRRSET;
	}

	return (result);
}

static isc_result_t
allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	     isc_stdtime_t now, dns_rdatasetiter_t **iteratorp)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *rbtnode = (dns_rbtnode_t *)node;
	rbtdb_version_t *rbtversion = version;
	rbtdb_rdatasetiter_t *iterator;
	unsigned int refs;

	REQUIRE(VALID_RBTDB(rbtdb));

	iterator = isc_mem_get(rbtdb->common.mctx, sizeof(*iterator));
	if (iterator == NULL)
		return (ISC_R_NOMEMORY);

	if ((db->attributes & DNS_DBATTR_CACHE) == 0) {
		now = 0;
		if (rbtversion == NULL)
			currentversion(db,
				 (dns_dbversion_t **) (void *)(&rbtversion));
		else {
			unsigned int refs;

			isc_refcount_increment(&rbtversion->references,
					       &refs);
			INSIST(refs > 1);
		}
	} else {
		if (now == 0)
			isc_stdtime_get(&now);
		rbtversion = NULL;
	}

	iterator->common.magic = DNS_RDATASETITER_MAGIC;
	iterator->common.methods = &rdatasetiter_methods;
	iterator->common.db = db;
	iterator->common.node = node;
	iterator->common.version = (dns_dbversion_t *)rbtversion;
	iterator->common.now = now;

	NODE_STRONGLOCK(&rbtdb->node_locks[rbtnode->locknum].lock);

	dns_rbtnode_refincrement(rbtnode, &refs);
	INSIST(refs != 0);

	iterator->current = NULL;

	NODE_STRONGUNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock);

	*iteratorp = (dns_rdatasetiter_t *)iterator;

	return (ISC_R_SUCCESS);
}

static isc_boolean_t
cname_and_other_data(dns_rbtnode_t *node, rbtdb_serial_t serial) {
	rdatasetheader_t *header, *header_next;
	isc_boolean_t cname, other_data;
	dns_rdatatype_t rdtype;

	/*
	 * The caller must hold the node lock.
	 */

	/*
	 * Look for CNAME and "other data" rdatasets active in our version.
	 */
	cname = ISC_FALSE;
	other_data = ISC_FALSE;
	for (header = node->data; header != NULL; header = header_next) {
		header_next = header->next;
		if (header->type == dns_rdatatype_cname) {
			/*
			 * Look for an active extant CNAME.
			 */
			do {
				if (header->serial <= serial &&
				    !IGNORE(header)) {
					/*
					 * Is this a "this rdataset doesn't
					 * exist" record?
					 */
					if (NONEXISTENT(header))
						header = NULL;
					break;
				} else
					header = header->down;
			} while (header != NULL);
			if (header != NULL)
				cname = ISC_TRUE;
		} else {
			/*
			 * Look for active extant "other data".
			 *
			 * "Other data" is any rdataset whose type is not
			 * KEY, NSEC, SIG or RRSIG.
			 */
			rdtype = RBTDB_RDATATYPE_BASE(header->type);
			if (rdtype != dns_rdatatype_key &&
			    rdtype != dns_rdatatype_sig &&
			    rdtype != dns_rdatatype_nsec &&
			    rdtype != dns_rdatatype_rrsig) {
				/*
				 * Is it active and extant?
				 */
				do {
					if (header->serial <= serial &&
					    !IGNORE(header)) {
						/*
						 * Is this a "this rdataset
						 * doesn't exist" record?
						 */
						if (NONEXISTENT(header))
							header = NULL;
						break;
					} else
						header = header->down;
				} while (header != NULL);
				if (header != NULL)
					other_data = ISC_TRUE;
			}
		}
	}

	if (cname && other_data)
		return (ISC_TRUE);

	return (ISC_FALSE);
}

static isc_result_t
resign_insert(dns_rbtdb_t *rbtdb, int idx, rdatasetheader_t *newheader) {
	isc_result_t result;

	INSIST(newheader->heap_index == 0);
	INSIST(!ISC_LINK_LINKED(newheader, lru_link));
	result = isc_heap_insert(rbtdb->heaps[idx], newheader);
	return (result);
}

static isc_result_t
add(dns_rbtdb_t *rbtdb, dns_rbtnode_t *rbtnode, rbtdb_version_t *rbtversion,
    rdatasetheader_t *newheader, unsigned int options, isc_boolean_t loading,
    dns_rdataset_t *addedrdataset, isc_stdtime_t now)
{
	rbtdb_changed_t *changed = NULL;
	rdatasetheader_t *topheader, *topheader_prev, *header;
	unsigned char *merged;
	isc_result_t result;
	isc_boolean_t header_nx;
	isc_boolean_t newheader_nx;
	isc_boolean_t merge;
	dns_rdatatype_t rdtype, covers;
	rbtdb_rdatatype_t negtype;
	dns_trust_t trust;
	int idx;

	/*
	 * Add an rdatasetheader_t to a node.
	 */

	/*
	 * Caller must be holding the node lock.
	 */

	if ((options & DNS_DBADD_MERGE) != 0) {
		REQUIRE(rbtversion != NULL);
		merge = ISC_TRUE;
	} else
		merge = ISC_FALSE;

	if ((options & DNS_DBADD_FORCE) != 0)
		trust = dns_trust_ultimate;
	else
		trust = newheader->trust;

	if (rbtversion != NULL && !loading) {
		/*
		 * We always add a changed record, even if no changes end up
		 * being made to this node, because it's harmless and
		 * simplifies the code.
		 */
		changed = add_changed(rbtdb, rbtversion, rbtnode);
		if (changed == NULL) {
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			return (ISC_R_NOMEMORY);
		}
	}

	newheader_nx = NONEXISTENT(newheader) ? ISC_TRUE : ISC_FALSE;
	topheader_prev = NULL;

	negtype = 0;
	if (rbtversion == NULL && !newheader_nx) {
		rdtype = RBTDB_RDATATYPE_BASE(newheader->type);
		if (rdtype == 0) {
			/*
			 * We're adding a negative cache entry.
			 */
			covers = RBTDB_RDATATYPE_EXT(newheader->type);
			if (covers == dns_rdatatype_any) {
				/*
				 * We're adding an negative cache entry
				 * which covers all types (NXDOMAIN,
				 * NODATA(QTYPE=ANY)).
				 *
				 * We make all other data stale so that the
				 * only rdataset that can be found at this
				 * node is the negative cache entry.
				 */
				for (topheader = rbtnode->data;
				     topheader != NULL;
				     topheader = topheader->next) {
					set_ttl(rbtdb, topheader, 0);
					topheader->attributes |=
						RDATASET_ATTR_STALE;
				}
				rbtnode->dirty = 1;
				goto find_header;
			}
			negtype = RBTDB_RDATATYPE_VALUE(covers, 0);
		} else {
			/*
			 * We're adding something that isn't a
			 * negative cache entry.  Look for an extant
			 * non-stale NXDOMAIN/NODATA(QTYPE=ANY) negative
			 * cache entry.
			 */
			for (topheader = rbtnode->data;
			     topheader != NULL;
			     topheader = topheader->next) {
				if (topheader->type ==
				    RBTDB_RDATATYPE_NCACHEANY)
					break;
			}
			if (topheader != NULL && EXISTS(topheader) &&
			    topheader->rdh_ttl > now) {
				/*
				 * Found one.
				 */
				if (trust < topheader->trust) {
					/*
					 * The NXDOMAIN/NODATA(QTYPE=ANY)
					 * is more trusted.
					 */
					free_rdataset(rbtdb,
						      rbtdb->common.mctx,
						      newheader);
					if (addedrdataset != NULL)
						bind_rdataset(rbtdb, rbtnode,
							      topheader, now,
							      addedrdataset);
					return (DNS_R_UNCHANGED);
				}
				/*
				 * The new rdataset is better.  Expire the
				 * NXDOMAIN/NODATA(QTYPE=ANY).
				 */
				set_ttl(rbtdb, topheader, 0);
				topheader->attributes |= RDATASET_ATTR_STALE;
				rbtnode->dirty = 1;
				topheader = NULL;
				goto find_header;
			}
			negtype = RBTDB_RDATATYPE_VALUE(0, rdtype);
		}
	}

	for (topheader = rbtnode->data;
	     topheader != NULL;
	     topheader = topheader->next) {
		if (topheader->type == newheader->type ||
		    topheader->type == negtype)
			break;
		topheader_prev = topheader;
	}

 find_header:
	/*
	 * If header isn't NULL, we've found the right type.  There may be
	 * IGNORE rdatasets between the top of the chain and the first real
	 * data.  We skip over them.
	 */
	header = topheader;
	while (header != NULL && IGNORE(header))
		header = header->down;
	if (header != NULL) {
		header_nx = NONEXISTENT(header) ? ISC_TRUE : ISC_FALSE;

		/*
		 * Deleting an already non-existent rdataset has no effect.
		 */
		if (header_nx && newheader_nx) {
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			return (DNS_R_UNCHANGED);
		}

		/*
		 * Trying to add an rdataset with lower trust to a cache DB
		 * has no effect, provided that the cache data isn't stale.
		 */
		if (rbtversion == NULL && trust < header->trust &&
		    (header->rdh_ttl > now || header_nx)) {
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			if (addedrdataset != NULL)
				bind_rdataset(rbtdb, rbtnode, header, now,
					      addedrdataset);
			return (DNS_R_UNCHANGED);
		}

		/*
		 * Don't merge if a nonexistent rdataset is involved.
		 */
		if (merge && (header_nx || newheader_nx))
			merge = ISC_FALSE;

		/*
		 * If 'merge' is ISC_TRUE, we'll try to create a new rdataset
		 * that is the union of 'newheader' and 'header'.
		 */
		if (merge) {
			unsigned int flags = 0;
			INSIST(rbtversion->serial >= header->serial);
			merged = NULL;
			result = ISC_R_SUCCESS;

			if ((options & DNS_DBADD_EXACT) != 0)
				flags |= DNS_RDATASLAB_EXACT;
			if ((options & DNS_DBADD_EXACTTTL) != 0 &&
			     newheader->rdh_ttl != header->rdh_ttl)
					result = DNS_R_NOTEXACT;
			else if (newheader->rdh_ttl != header->rdh_ttl)
				flags |= DNS_RDATASLAB_FORCE;
			if (result == ISC_R_SUCCESS)
				result = dns_rdataslab_merge(
					     (unsigned char *)header,
					     (unsigned char *)newheader,
					     (unsigned int)(sizeof(*newheader)),
					     rbtdb->common.mctx,
					     rbtdb->common.rdclass,
					     (dns_rdatatype_t)header->type,
					     flags, &merged);
			if (result == ISC_R_SUCCESS) {
				/*
				 * If 'header' has the same serial number as
				 * we do, we could clean it up now if we knew
				 * that our caller had no references to it.
				 * We don't know this, however, so we leave it
				 * alone.  It will get cleaned up when
				 * clean_zone_node() runs.
				 */
				free_rdataset(rbtdb, rbtdb->common.mctx,
					      newheader);
				newheader = (rdatasetheader_t *)merged;
				if (loading && RESIGN(newheader) &&
				    RESIGN(header) &&
				    header->resign < newheader->resign)
					newheader->resign = header->resign;
			} else {
				free_rdataset(rbtdb, rbtdb->common.mctx,
					      newheader);
				return (result);
			}
		}
		/*
		 * Don't replace existing NS, A and AAAA RRsets
		 * in the cache if they are already exist.  This
		 * prevents named being locked to old servers.
		 * Don't lower trust of existing record if the
		 * update is forced.
		 */
		if (IS_CACHE(rbtdb) && header->rdh_ttl > now &&
		    header->type == dns_rdatatype_ns &&
		    !header_nx && !newheader_nx &&
		    header->trust >= newheader->trust &&
		    dns_rdataslab_equalx((unsigned char *)header,
					 (unsigned char *)newheader,
					 (unsigned int)(sizeof(*newheader)),
					 rbtdb->common.rdclass,
					 (dns_rdatatype_t)header->type)) {
			/*
			 * Honour the new ttl if it is less than the
			 * older one.
			 */
			if (header->rdh_ttl > newheader->rdh_ttl)
				set_ttl(rbtdb, header, newheader->rdh_ttl);
			if (header->noqname == NULL &&
			    newheader->noqname != NULL) {
				header->noqname = newheader->noqname;
				newheader->noqname = NULL;
			}
			if (header->closest == NULL &&
			    newheader->closest != NULL) {
				header->closest = newheader->closest;
				newheader->closest = NULL;
			}
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			if (addedrdataset != NULL)
				bind_rdataset(rbtdb, rbtnode, header, now,
					      addedrdataset);
			return (ISC_R_SUCCESS);
		}
		if (IS_CACHE(rbtdb) && header->rdh_ttl > now &&
		    (header->type == dns_rdatatype_a ||
		     header->type == dns_rdatatype_aaaa) &&
		    !header_nx && !newheader_nx &&
		    header->trust >= newheader->trust &&
		    dns_rdataslab_equal((unsigned char *)header,
					(unsigned char *)newheader,
					(unsigned int)(sizeof(*newheader)))) {
			/*
			 * Honour the new ttl if it is less than the
			 * older one.
			 */
			if (header->rdh_ttl > newheader->rdh_ttl)
				set_ttl(rbtdb, header, newheader->rdh_ttl);
			if (header->noqname == NULL &&
			    newheader->noqname != NULL) {
				header->noqname = newheader->noqname;
				newheader->noqname = NULL;
			}
			if (header->closest == NULL &&
			    newheader->closest != NULL) {
				header->closest = newheader->closest;
				newheader->closest = NULL;
			}
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			if (addedrdataset != NULL)
				bind_rdataset(rbtdb, rbtnode, header, now,
					      addedrdataset);
			return (ISC_R_SUCCESS);
		}
		INSIST(rbtversion == NULL ||
		       rbtversion->serial >= topheader->serial);
		if (topheader_prev != NULL)
			topheader_prev->next = newheader;
		else
			rbtnode->data = newheader;
		newheader->next = topheader->next;
		if (loading) {
			/*
			 * There are no other references to 'header' when
			 * loading, so we MAY clean up 'header' now.
			 * Since we don't generate changed records when
			 * loading, we MUST clean up 'header' now.
			 */
			newheader->down = NULL;
			free_rdataset(rbtdb, rbtdb->common.mctx, header);
		} else {
			newheader->down = topheader;
			topheader->next = newheader;
			rbtnode->dirty = 1;
			if (changed != NULL)
				changed->dirty = ISC_TRUE;
			if (rbtversion == NULL) {
				set_ttl(rbtdb, header, 0);
				header->attributes |= RDATASET_ATTR_STALE;
			}
			idx = newheader->node->locknum;
			if (IS_CACHE(rbtdb)) {
				ISC_LIST_PREPEND(rbtdb->rdatasets[idx],
						 newheader, lru_link);
				/*
				 * XXXMLG We don't check the return value
				 * here.  If it fails, we will not do TTL
				 * based expiry on this node.  However, we
				 * will do it on the LRU side, so memory
				 * will not leak... for long.
				 */
				isc_heap_insert(rbtdb->heaps[idx], newheader);
			} else if (RESIGN(newheader))
				resign_insert(rbtdb, idx, newheader);
		}
	} else {
		/*
		 * No non-IGNORED rdatasets of the given type exist at
		 * this node.
		 */

		/*
		 * If we're trying to delete the type, don't bother.
		 */
		if (newheader_nx) {
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			return (DNS_R_UNCHANGED);
		}

		if (topheader != NULL) {
			/*
			 * We have an list of rdatasets of the given type,
			 * but they're all marked IGNORE.  We simply insert
			 * the new rdataset at the head of the list.
			 *
			 * Ignored rdatasets cannot occur during loading, so
			 * we INSIST on it.
			 */
			INSIST(!loading);
			INSIST(rbtversion == NULL ||
			       rbtversion->serial >= topheader->serial);
			if (topheader_prev != NULL)
				topheader_prev->next = newheader;
			else
				rbtnode->data = newheader;
			newheader->next = topheader->next;
			newheader->down = topheader;
			topheader->next = newheader;
			rbtnode->dirty = 1;
			if (changed != NULL)
				changed->dirty = ISC_TRUE;
		} else {
			/*
			 * No rdatasets of the given type exist at the node.
			 */
			newheader->next = rbtnode->data;
			newheader->down = NULL;
			rbtnode->data = newheader;
		}
		idx = newheader->node->locknum;
		if (IS_CACHE(rbtdb)) {
			ISC_LIST_PREPEND(rbtdb->rdatasets[idx],
					 newheader, lru_link);
			isc_heap_insert(rbtdb->heaps[idx], newheader);
		} else if (RESIGN(newheader)) {
			resign_insert(rbtdb, idx, newheader);
		}
	}

	/*
	 * Check if the node now contains CNAME and other data.
	 */
	if (rbtversion != NULL &&
	    cname_and_other_data(rbtnode, rbtversion->serial))
		return (DNS_R_CNAMEANDOTHER);

	if (addedrdataset != NULL)
		bind_rdataset(rbtdb, rbtnode, newheader, now, addedrdataset);

	return (ISC_R_SUCCESS);
}

static inline isc_boolean_t
delegating_type(dns_rbtdb_t *rbtdb, dns_rbtnode_t *node,
		rbtdb_rdatatype_t type)
{
	if (IS_CACHE(rbtdb)) {
		if (type == dns_rdatatype_dname)
			return (ISC_TRUE);
		else
			return (ISC_FALSE);
	} else if (type == dns_rdatatype_dname ||
		   (type == dns_rdatatype_ns &&
		    (node != rbtdb->origin_node || IS_STUB(rbtdb))))
		return (ISC_TRUE);
	return (ISC_FALSE);
}

static inline isc_result_t
addnoqname(dns_rbtdb_t *rbtdb, rdatasetheader_t *newheader,
	   dns_rdataset_t *rdataset)
{
	struct noqname *noqname;
	isc_mem_t *mctx = rbtdb->common.mctx;
	dns_name_t name;
	dns_rdataset_t neg, negsig;
	isc_result_t result;
	isc_region_t r;

	dns_name_init(&name, NULL);
	dns_rdataset_init(&neg);
	dns_rdataset_init(&negsig);

	result = dns_rdataset_getnoqname(rdataset, &name, &neg, &negsig);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	noqname = isc_mem_get(mctx, sizeof(*noqname));
	if (noqname == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	dns_name_init(&noqname->name, NULL);
	noqname->neg = NULL;
	noqname->negsig = NULL;
	noqname->type = neg.type;
	result = dns_name_dup(&name, mctx, &noqname->name);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_rdataslab_fromrdataset(&neg, mctx, &r, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	noqname->neg = r.base;
	result = dns_rdataslab_fromrdataset(&negsig, mctx, &r, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	noqname->negsig = r.base;
	dns_rdataset_disassociate(&neg);
	dns_rdataset_disassociate(&negsig);
	newheader->noqname = noqname;
	return (ISC_R_SUCCESS);

cleanup:
	dns_rdataset_disassociate(&neg);
	dns_rdataset_disassociate(&negsig);
	free_noqname(mctx, &noqname);
	return(result);
}

static inline isc_result_t
addclosest(dns_rbtdb_t *rbtdb, rdatasetheader_t *newheader,
	   dns_rdataset_t *rdataset)
{
	struct noqname *closest;
	isc_mem_t *mctx = rbtdb->common.mctx;
	dns_name_t name;
	dns_rdataset_t neg, negsig;
	isc_result_t result;
	isc_region_t r;

	dns_name_init(&name, NULL);
	dns_rdataset_init(&neg);
	dns_rdataset_init(&negsig);

	result = dns_rdataset_getclosest(rdataset, &name, &neg, &negsig);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	closest = isc_mem_get(mctx, sizeof(*closest));
	if (closest == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	dns_name_init(&closest->name, NULL);
	closest->neg = NULL;
	closest->negsig = NULL;
	closest->type = neg.type;
	result = dns_name_dup(&name, mctx, &closest->name);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_rdataslab_fromrdataset(&neg, mctx, &r, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	closest->neg = r.base;
	result = dns_rdataslab_fromrdataset(&negsig, mctx, &r, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	closest->negsig = r.base;
	dns_rdataset_disassociate(&neg);
	dns_rdataset_disassociate(&negsig);
	newheader->closest = closest;
	return (ISC_R_SUCCESS);

 cleanup:
	dns_rdataset_disassociate(&neg);
	dns_rdataset_disassociate(&negsig);
	free_noqname(mctx, &closest);
	return(result);
}

static dns_dbmethods_t zone_methods;

static isc_result_t
addrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	    isc_stdtime_t now, dns_rdataset_t *rdataset, unsigned int options,
	    dns_rdataset_t *addedrdataset)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *rbtnode = (dns_rbtnode_t *)node;
	rbtdb_version_t *rbtversion = version;
	isc_region_t region;
	rdatasetheader_t *newheader;
	rdatasetheader_t *header;
	isc_result_t result;
	isc_boolean_t delegating;
	isc_boolean_t tree_locked = ISC_FALSE;

	REQUIRE(VALID_RBTDB(rbtdb));

	if (rbtdb->common.methods == &zone_methods)
		REQUIRE(((rbtnode->nsec3 &&
			  (rdataset->type == dns_rdatatype_nsec3 ||
			   rdataset->covers == dns_rdatatype_nsec3)) ||
			 (!rbtnode->nsec3 &&
			   rdataset->type != dns_rdatatype_nsec3 &&
			   rdataset->covers != dns_rdatatype_nsec3)));

	if (rbtversion == NULL) {
		if (now == 0)
			isc_stdtime_get(&now);
	} else
		now = 0;

	result = dns_rdataslab_fromrdataset(rdataset, rbtdb->common.mctx,
					    &region,
					    sizeof(rdatasetheader_t));
	if (result != ISC_R_SUCCESS)
		return (result);

	newheader = (rdatasetheader_t *)region.base;
	init_rdataset(rbtdb, newheader);
	set_ttl(rbtdb, newheader, rdataset->ttl + now);
	newheader->type = RBTDB_RDATATYPE_VALUE(rdataset->type,
						rdataset->covers);
	newheader->attributes = 0;
	newheader->noqname = NULL;
	newheader->closest = NULL;
	newheader->count = init_count++;
	newheader->trust = rdataset->trust;
	newheader->additional_auth = NULL;
	newheader->additional_glue = NULL;
	newheader->last_used = now;
	newheader->node = rbtnode;
	if (rbtversion != NULL) {
		newheader->serial = rbtversion->serial;
		now = 0;

		if ((rdataset->attributes & DNS_RDATASETATTR_RESIGN) != 0) {
			newheader->attributes |= RDATASET_ATTR_RESIGN;
			newheader->resign = rdataset->resign;
		} else
			newheader->resign = 0;
	} else {
		newheader->serial = 1;
		newheader->resign = 0;
		if ((rdataset->attributes & DNS_RDATASETATTR_NXDOMAIN) != 0)
			newheader->attributes |= RDATASET_ATTR_NXDOMAIN;
		if ((rdataset->attributes & DNS_RDATASETATTR_OPTOUT) != 0)
			newheader->attributes |= RDATASET_ATTR_OPTOUT;
		if ((rdataset->attributes & DNS_RDATASETATTR_NOQNAME) != 0) {
			result = addnoqname(rbtdb, newheader, rdataset);
			if (result != ISC_R_SUCCESS) {
				free_rdataset(rbtdb, rbtdb->common.mctx,
					      newheader);
				return (result);
			}
		}
		if ((rdataset->attributes & DNS_RDATASETATTR_CLOSEST) != 0) {
			result = addclosest(rbtdb, newheader, rdataset);
			if (result != ISC_R_SUCCESS) {
				free_rdataset(rbtdb, rbtdb->common.mctx,
					      newheader);
				return (result);
			}
		}
	}

	/*
	 * If we're adding a delegation type (e.g. NS or DNAME for a zone,
	 * just DNAME for the cache), then we need to set the callback bit
	 * on the node.
	 */
	if (delegating_type(rbtdb, rbtnode, rdataset->type))
		delegating = ISC_TRUE;
	else
		delegating = ISC_FALSE;

	/*
	 * If we're adding a delegation type or the DB is a cache in an overmem
	 * state, hold an exclusive lock on the tree.  In the latter case
	 * the lock does not necessarily have to be acquired but it will help
	 * purge stale entries more effectively.
	 */
	if (delegating || (IS_CACHE(rbtdb) && rbtdb->overmem)) {
		tree_locked = ISC_TRUE;
		RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);
	}

	if (IS_CACHE(rbtdb) && rbtdb->overmem)
		overmem_purge(rbtdb, rbtnode->locknum, now, tree_locked);

	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_write);

	if (rbtdb->rrsetstats != NULL) {
		newheader->attributes |= RDATASET_ATTR_STATCOUNT;
		update_rrsetstats(rbtdb, newheader, ISC_TRUE);
	}

	if (IS_CACHE(rbtdb)) {
		if (tree_locked)
			cleanup_dead_nodes(rbtdb, rbtnode->locknum);

		header = isc_heap_element(rbtdb->heaps[rbtnode->locknum], 1);
		if (header && header->rdh_ttl <= now - RBTDB_VIRTUAL)
			expire_header(rbtdb, header, tree_locked);

		/*
		 * If we've been holding a write lock on the tree just for
		 * cleaning, we can release it now.  However, we still need the
		 * node lock.
		 */
		if (tree_locked && !delegating) {
			RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);
			tree_locked = ISC_FALSE;
		}
	}

	result = add(rbtdb, rbtnode, rbtversion, newheader, options, ISC_FALSE,
		     addedrdataset, now);
	if (result == ISC_R_SUCCESS && delegating)
		rbtnode->find_callback = 1;

	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_write);

	if (tree_locked)
		RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);

	/*
	 * Update the zone's secure status.  If version is non-NULL
	 * this is deferred until closeversion() is called.
	 */
	if (result == ISC_R_SUCCESS && version == NULL && !IS_CACHE(rbtdb))
		iszonesecure(db, version, rbtdb->origin_node);

	return (result);
}

static isc_result_t
subtractrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		 dns_rdataset_t *rdataset, unsigned int options,
		 dns_rdataset_t *newrdataset)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *rbtnode = (dns_rbtnode_t *)node;
	rbtdb_version_t *rbtversion = version;
	rdatasetheader_t *topheader, *topheader_prev, *header, *newheader;
	unsigned char *subresult;
	isc_region_t region;
	isc_result_t result;
	rbtdb_changed_t *changed;

	REQUIRE(VALID_RBTDB(rbtdb));

	if (rbtdb->common.methods == &zone_methods)
		REQUIRE(((rbtnode->nsec3 &&
			  (rdataset->type == dns_rdatatype_nsec3 ||
			   rdataset->covers == dns_rdatatype_nsec3)) ||
			 (!rbtnode->nsec3 &&
			   rdataset->type != dns_rdatatype_nsec3 &&
			   rdataset->covers != dns_rdatatype_nsec3)));

	result = dns_rdataslab_fromrdataset(rdataset, rbtdb->common.mctx,
					    &region,
					    sizeof(rdatasetheader_t));
	if (result != ISC_R_SUCCESS)
		return (result);
	newheader = (rdatasetheader_t *)region.base;
	init_rdataset(rbtdb, newheader);
	set_ttl(rbtdb, newheader, rdataset->ttl);
	newheader->type = RBTDB_RDATATYPE_VALUE(rdataset->type,
						rdataset->covers);
	newheader->attributes = 0;
	newheader->serial = rbtversion->serial;
	newheader->trust = 0;
	newheader->noqname = NULL;
	newheader->closest = NULL;
	newheader->count = init_count++;
	newheader->additional_auth = NULL;
	newheader->additional_glue = NULL;
	newheader->last_used = 0;
	newheader->node = rbtnode;
	if ((rdataset->attributes & DNS_RDATASETATTR_RESIGN) != 0) {
		newheader->attributes |= RDATASET_ATTR_RESIGN;
		newheader->resign = rdataset->resign;
	} else
		newheader->resign = 0;

	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_write);

	changed = add_changed(rbtdb, rbtversion, rbtnode);
	if (changed == NULL) {
		free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
		NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
			    isc_rwlocktype_write);
		return (ISC_R_NOMEMORY);
	}

	topheader_prev = NULL;
	for (topheader = rbtnode->data;
	     topheader != NULL;
	     topheader = topheader->next) {
		if (topheader->type == newheader->type)
			break;
		topheader_prev = topheader;
	}
	/*
	 * If header isn't NULL, we've found the right type.  There may be
	 * IGNORE rdatasets between the top of the chain and the first real
	 * data.  We skip over them.
	 */
	header = topheader;
	while (header != NULL && IGNORE(header))
		header = header->down;
	if (header != NULL && EXISTS(header)) {
		unsigned int flags = 0;
		subresult = NULL;
		result = ISC_R_SUCCESS;
		if ((options & DNS_DBSUB_EXACT) != 0) {
			flags |= DNS_RDATASLAB_EXACT;
			if (newheader->rdh_ttl != header->rdh_ttl)
				result = DNS_R_NOTEXACT;
		}
		if (result == ISC_R_SUCCESS)
			result = dns_rdataslab_subtract(
					(unsigned char *)header,
					(unsigned char *)newheader,
					(unsigned int)(sizeof(*newheader)),
					rbtdb->common.mctx,
					rbtdb->common.rdclass,
					(dns_rdatatype_t)header->type,
					flags, &subresult);
		if (result == ISC_R_SUCCESS) {
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			newheader = (rdatasetheader_t *)subresult;
			init_rdataset(rbtdb, newheader);
			/*
			 * We have to set the serial since the rdataslab
			 * subtraction routine copies the reserved portion of
			 * header, not newheader.
			 */
			newheader->serial = rbtversion->serial;
			/*
			 * XXXJT: dns_rdataslab_subtract() copied the pointers
			 * to additional info.  We need to clear these fields
			 * to avoid having duplicated references.
			 */
			newheader->additional_auth = NULL;
			newheader->additional_glue = NULL;
		} else if (result == DNS_R_NXRRSET) {
			/*
			 * This subtraction would remove all of the rdata;
			 * add a nonexistent header instead.
			 */
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			newheader = new_rdataset(rbtdb, rbtdb->common.mctx);
			if (newheader == NULL) {
				result = ISC_R_NOMEMORY;
				goto unlock;
			}
			set_ttl(rbtdb, newheader, 0);
			newheader->type = topheader->type;
			newheader->attributes = RDATASET_ATTR_NONEXISTENT;
			newheader->trust = 0;
			newheader->serial = rbtversion->serial;
			newheader->noqname = NULL;
			newheader->closest = NULL;
			newheader->count = 0;
			newheader->additional_auth = NULL;
			newheader->additional_glue = NULL;
			newheader->node = rbtnode;
			newheader->resign = 0;
			newheader->last_used = 0;
		} else {
			free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
			goto unlock;
		}

		/*
		 * If we're here, we want to link newheader in front of
		 * topheader.
		 */
		INSIST(rbtversion->serial >= topheader->serial);
		if (topheader_prev != NULL)
			topheader_prev->next = newheader;
		else
			rbtnode->data = newheader;
		newheader->next = topheader->next;
		newheader->down = topheader;
		topheader->next = newheader;
		rbtnode->dirty = 1;
		changed->dirty = ISC_TRUE;
	} else {
		/*
		 * The rdataset doesn't exist, so we don't need to do anything
		 * to satisfy the deletion request.
		 */
		free_rdataset(rbtdb, rbtdb->common.mctx, newheader);
		if ((options & DNS_DBSUB_EXACT) != 0)
			result = DNS_R_NOTEXACT;
		else
			result = DNS_R_UNCHANGED;
	}

	if (result == ISC_R_SUCCESS && newrdataset != NULL)
		bind_rdataset(rbtdb, rbtnode, newheader, 0, newrdataset);

 unlock:
	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_write);

	/*
	 * Update the zone's secure status.  If version is non-NULL
	 * this is deferred until closeversion() is called.
	 */
	if (result == ISC_R_SUCCESS && version == NULL && !IS_CACHE(rbtdb))
		iszonesecure(db, rbtdb->current_version, rbtdb->origin_node);

	return (result);
}

static isc_result_t
deleterdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
	       dns_rdatatype_t type, dns_rdatatype_t covers)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *rbtnode = (dns_rbtnode_t *)node;
	rbtdb_version_t *rbtversion = version;
	isc_result_t result;
	rdatasetheader_t *newheader;

	REQUIRE(VALID_RBTDB(rbtdb));

	if (type == dns_rdatatype_any)
		return (ISC_R_NOTIMPLEMENTED);
	if (type == dns_rdatatype_rrsig && covers == 0)
		return (ISC_R_NOTIMPLEMENTED);

	newheader = new_rdataset(rbtdb, rbtdb->common.mctx);
	if (newheader == NULL)
		return (ISC_R_NOMEMORY);
	set_ttl(rbtdb, newheader, 0);
	newheader->type = RBTDB_RDATATYPE_VALUE(type, covers);
	newheader->attributes = RDATASET_ATTR_NONEXISTENT;
	newheader->trust = 0;
	newheader->noqname = NULL;
	newheader->closest = NULL;
	newheader->additional_auth = NULL;
	newheader->additional_glue = NULL;
	if (rbtversion != NULL)
		newheader->serial = rbtversion->serial;
	else
		newheader->serial = 0;
	newheader->count = 0;
	newheader->last_used = 0;
	newheader->node = rbtnode;

	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_write);

	result = add(rbtdb, rbtnode, rbtversion, newheader, DNS_DBADD_FORCE,
		     ISC_FALSE, NULL, 0);

	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_write);

	/*
	 * Update the zone's secure status.  If version is non-NULL
	 * this is deferred until closeversion() is called.
	 */
	if (result == ISC_R_SUCCESS && version == NULL && !IS_CACHE(rbtdb))
		iszonesecure(db, rbtdb->current_version, rbtdb->origin_node);

	return (result);
}

static isc_result_t
loading_addrdataset(void *arg, dns_name_t *name, dns_rdataset_t *rdataset) {
	rbtdb_load_t *loadctx = arg;
	dns_rbtdb_t *rbtdb = loadctx->rbtdb;
	dns_rbtnode_t *node;
	isc_result_t result;
	isc_region_t region;
	rdatasetheader_t *newheader;

	/*
	 * This routine does no node locking.  See comments in
	 * 'load' below for more information on loading and
	 * locking.
	 */


	/*
	 * SOA records are only allowed at top of zone.
	 */
	if (rdataset->type == dns_rdatatype_soa &&
	    !IS_CACHE(rbtdb) && !dns_name_equal(name, &rbtdb->common.origin))
		return (DNS_R_NOTZONETOP);

	if (rdataset->type != dns_rdatatype_nsec3 &&
	    rdataset->covers != dns_rdatatype_nsec3)
		add_empty_wildcards(rbtdb, name);

	if (dns_name_iswildcard(name)) {
		/*
		 * NS record owners cannot legally be wild cards.
		 */
		if (rdataset->type == dns_rdatatype_ns)
			return (DNS_R_INVALIDNS);
		/*
		 * NSEC3 record owners cannot legally be wild cards.
		 */
		if (rdataset->type == dns_rdatatype_nsec3)
			return (DNS_R_INVALIDNSEC3);
		result = add_wildcard_magic(rbtdb, name);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	node = NULL;
	if (rdataset->type == dns_rdatatype_nsec3 ||
	    rdataset->covers == dns_rdatatype_nsec3) {
		result = dns_rbt_addnode(rbtdb->nsec3, name, &node);
		if (result == ISC_R_SUCCESS)
			node->nsec3 = 1;
	} else {
		result = dns_rbt_addnode(rbtdb->tree, name, &node);
		if (result == ISC_R_SUCCESS)
			node->nsec3 = 0;
	}
	if (result != ISC_R_SUCCESS && result != ISC_R_EXISTS)
		return (result);
	if (result != ISC_R_EXISTS) {
		dns_name_t foundname;
		dns_name_init(&foundname, NULL);
		dns_rbt_namefromnode(node, &foundname);
#ifdef DNS_RBT_USEHASH
		node->locknum = node->hashval % rbtdb->node_lock_count;
#else
		node->locknum = dns_name_hash(&foundname, ISC_TRUE) %
			rbtdb->node_lock_count;
#endif
	}

	result = dns_rdataslab_fromrdataset(rdataset, rbtdb->common.mctx,
					    &region,
					    sizeof(rdatasetheader_t));
	if (result != ISC_R_SUCCESS)
		return (result);
	newheader = (rdatasetheader_t *)region.base;
	init_rdataset(rbtdb, newheader);
	set_ttl(rbtdb, newheader,
		rdataset->ttl + loadctx->now); /* XXX overflow check */
	newheader->type = RBTDB_RDATATYPE_VALUE(rdataset->type,
						rdataset->covers);
	newheader->attributes = 0;
	newheader->trust = rdataset->trust;
	newheader->serial = 1;
	newheader->noqname = NULL;
	newheader->closest = NULL;
	newheader->count = init_count++;
	newheader->additional_auth = NULL;
	newheader->additional_glue = NULL;
	newheader->last_used = 0;
	newheader->node = node;
	if ((rdataset->attributes & DNS_RDATASETATTR_RESIGN) != 0) {
		newheader->attributes |= RDATASET_ATTR_RESIGN;
		newheader->resign = rdataset->resign;
	} else
		newheader->resign = 0;

	result = add(rbtdb, node, rbtdb->current_version, newheader,
		     DNS_DBADD_MERGE, ISC_TRUE, NULL, 0);
	if (result == ISC_R_SUCCESS &&
	    delegating_type(rbtdb, node, rdataset->type))
		node->find_callback = 1;
	else if (result == DNS_R_UNCHANGED)
		result = ISC_R_SUCCESS;

	return (result);
}

static isc_result_t
beginload(dns_db_t *db, dns_addrdatasetfunc_t *addp, dns_dbload_t **dbloadp) {
	rbtdb_load_t *loadctx;
	dns_rbtdb_t *rbtdb;

	rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));

	loadctx = isc_mem_get(rbtdb->common.mctx, sizeof(*loadctx));
	if (loadctx == NULL)
		return (ISC_R_NOMEMORY);

	loadctx->rbtdb = rbtdb;
	if (IS_CACHE(rbtdb))
		isc_stdtime_get(&loadctx->now);
	else
		loadctx->now = 0;

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_write);

	REQUIRE((rbtdb->attributes & (RBTDB_ATTR_LOADED|RBTDB_ATTR_LOADING))
		== 0);
	rbtdb->attributes |= RBTDB_ATTR_LOADING;

	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_write);

	*addp = loading_addrdataset;
	*dbloadp = loadctx;

	return (ISC_R_SUCCESS);
}

static isc_result_t
endload(dns_db_t *db, dns_dbload_t **dbloadp) {
	rbtdb_load_t *loadctx;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(dbloadp != NULL);
	loadctx = *dbloadp;
	REQUIRE(loadctx->rbtdb == rbtdb);

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_write);

	REQUIRE((rbtdb->attributes & RBTDB_ATTR_LOADING) != 0);
	REQUIRE((rbtdb->attributes & RBTDB_ATTR_LOADED) == 0);

	rbtdb->attributes &= ~RBTDB_ATTR_LOADING;
	rbtdb->attributes |= RBTDB_ATTR_LOADED;

	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_write);

	/*
	 * If there's a KEY rdataset at the zone origin containing a
	 * zone key, we consider the zone secure.
	 */
	if (! IS_CACHE(rbtdb))
		iszonesecure(db, rbtdb->current_version, rbtdb->origin_node);

	*dbloadp = NULL;

	isc_mem_put(rbtdb->common.mctx, loadctx, sizeof(*loadctx));

	return (ISC_R_SUCCESS);
}

static isc_result_t
dump(dns_db_t *db, dns_dbversion_t *version, const char *filename,
     dns_masterformat_t masterformat) {
	dns_rbtdb_t *rbtdb;

	rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));

	return (dns_master_dump2(rbtdb->common.mctx, db, version,
				 &dns_master_style_default,
				 filename, masterformat));
}

static void
delete_callback(void *data, void *arg) {
	dns_rbtdb_t *rbtdb = arg;
	rdatasetheader_t *current, *next;

	for (current = data; current != NULL; current = next) {
		next = current->next;
		free_rdataset(rbtdb, rbtdb->common.mctx, current);
	}
}

static isc_boolean_t
issecure(dns_db_t *db) {
	dns_rbtdb_t *rbtdb;
	isc_boolean_t secure;

	rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));

	RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
	secure = ISC_TF(rbtdb->current_version->secure == dns_db_secure);
	RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);

	return (secure);
}

static isc_boolean_t
isdnssec(dns_db_t *db) {
	dns_rbtdb_t *rbtdb;
	isc_boolean_t dnssec;

	rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));

	RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
	dnssec = ISC_TF(rbtdb->current_version->secure != dns_db_insecure);
	RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);

	return (dnssec);
}

static unsigned int
nodecount(dns_db_t *db) {
	dns_rbtdb_t *rbtdb;
	unsigned int count;

	rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));

	RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
	count = dns_rbt_nodecount(rbtdb->tree);
	RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);

	return (count);
}

static void
settask(dns_db_t *db, isc_task_t *task) {
	dns_rbtdb_t *rbtdb;

	rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_write);
	if (rbtdb->task != NULL)
		isc_task_detach(&rbtdb->task);
	if (task != NULL)
		isc_task_attach(task, &rbtdb->task);
	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_write);
}

static isc_boolean_t
ispersistent(dns_db_t *db) {
	UNUSED(db);
	return (ISC_FALSE);
}

static isc_result_t
getoriginnode(dns_db_t *db, dns_dbnode_t **nodep) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *onode;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(nodep != NULL && *nodep == NULL);

	/* Note that the access to origin_node doesn't require a DB lock */
	onode = (dns_rbtnode_t *)rbtdb->origin_node;
	if (onode != NULL) {
		NODE_STRONGLOCK(&rbtdb->node_locks[onode->locknum].lock);
		new_reference(rbtdb, onode);
		NODE_STRONGUNLOCK(&rbtdb->node_locks[onode->locknum].lock);

		*nodep = rbtdb->origin_node;
	} else {
		INSIST(IS_CACHE(rbtdb));
		result = ISC_R_NOTFOUND;
	}

	return (result);
}

static isc_result_t
getnsec3parameters(dns_db_t *db, dns_dbversion_t *version, dns_hash_t *hash,
		   isc_uint8_t *flags, isc_uint16_t *iterations,
		   unsigned char *salt, size_t *salt_length)
{
	dns_rbtdb_t *rbtdb;
	isc_result_t result = ISC_R_NOTFOUND;
	rbtdb_version_t *rbtversion = version;

	rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));

	RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);

	if (rbtversion == NULL)
		rbtversion = rbtdb->current_version;

	if (rbtversion->havensec3) {
		if (hash != NULL)
			*hash = rbtversion->hash;
		if (salt != NULL && salt_length != 0) {
			REQUIRE(*salt_length > rbtversion->salt_length);
			memcpy(salt, rbtversion->salt, rbtversion->salt_length);
		}
		if (salt_length != NULL)
			*salt_length = rbtversion->salt_length;
		if (iterations != NULL)
			*iterations = rbtversion->iterations;
		if (flags != NULL)
			*flags = rbtversion->flags;
		result = ISC_R_SUCCESS;
	}
	RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);

	return (result);
}

static isc_result_t
setsigningtime(dns_db_t *db, dns_rdataset_t *rdataset, isc_stdtime_t resign) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	isc_stdtime_t oldresign;
	isc_result_t result = ISC_R_SUCCESS;
	rdatasetheader_t *header;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(!IS_CACHE(rbtdb));
	REQUIRE(rdataset != NULL);

	header = rdataset->private3;
	header--;

	NODE_LOCK(&rbtdb->node_locks[header->node->locknum].lock,
		  isc_rwlocktype_write);

	oldresign = header->resign;
	header->resign = resign;
	if (header->heap_index != 0) {
		INSIST(RESIGN(header));
		if (resign == 0) {
			isc_heap_delete(rbtdb->heaps[header->node->locknum],
					header->heap_index);
			header->heap_index = 0;
		} else if (resign < oldresign)
			isc_heap_increased(rbtdb->heaps[header->node->locknum],
					   header->heap_index);
		else
			isc_heap_decreased(rbtdb->heaps[header->node->locknum],
					   header->heap_index);
	} else if (resign && header->heap_index == 0) {
		header->attributes |= RDATASET_ATTR_RESIGN;
		result = resign_insert(rbtdb, header->node->locknum, header);
	}
	NODE_UNLOCK(&rbtdb->node_locks[header->node->locknum].lock,
		    isc_rwlocktype_write);
	return (result);
}

static isc_result_t
getsigningtime(dns_db_t *db, dns_rdataset_t *rdataset,
	       dns_name_t *foundname)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	rdatasetheader_t *header = NULL, *this;
	unsigned int i;
	isc_result_t result = ISC_R_NOTFOUND;

	REQUIRE(VALID_RBTDB(rbtdb));

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_read);

	for (i = 0; i < rbtdb->node_lock_count; i++) {
		this = isc_heap_element(rbtdb->heaps[i], 1);
		if (this == NULL)
			continue;
		if (header == NULL)
			header = this;
		else if (isc_serial_lt(this->resign, header->resign))
			header = this;
	}

	if (header == NULL)
		goto unlock;

	NODE_LOCK(&rbtdb->node_locks[header->node->locknum].lock,
		  isc_rwlocktype_read);

	bind_rdataset(rbtdb, header->node, header, 0, rdataset);

	if (foundname != NULL)
		dns_rbt_fullnamefromnode(header->node, foundname);

	NODE_UNLOCK(&rbtdb->node_locks[header->node->locknum].lock,
		    isc_rwlocktype_read);

	result = ISC_R_SUCCESS;

 unlock:
	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_read);

	return (result);
}

static void
resigned(dns_db_t *db, dns_rdataset_t *rdataset, dns_dbversion_t *version)
{
	rbtdb_version_t *rbtversion = (rbtdb_version_t *)version;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;
	dns_rbtnode_t *node;
	rdatasetheader_t *header;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(rdataset != NULL);
	REQUIRE(rbtdb->future_version == rbtversion);
	REQUIRE(rbtversion->writer);

	node = rdataset->private2;
	header = rdataset->private3;
	header--;

	RBTDB_LOCK(&rbtdb->lock, isc_rwlocktype_read);
	NODE_LOCK(&rbtdb->node_locks[node->locknum].lock,
		  isc_rwlocktype_write);
	/*
	 * Delete from heap and save to re-signed list so that it can
	 * be restored if we backout of this change.
	 */
	new_reference(rbtdb, node);
	isc_heap_delete(rbtdb->heaps[node->locknum], header->heap_index);
	header->heap_index = 0;
	ISC_LIST_APPEND(rbtversion->resigned_list, header, lru_link);

	NODE_UNLOCK(&rbtdb->node_locks[node->locknum].lock,
		    isc_rwlocktype_write);
	RBTDB_UNLOCK(&rbtdb->lock, isc_rwlocktype_read);
}

static dns_stats_t *
getrrsetstats(dns_db_t *db) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)db;

	REQUIRE(VALID_RBTDB(rbtdb));
	REQUIRE(IS_CACHE(rbtdb)); /* current restriction */

	return (rbtdb->rrsetstats);
}

static dns_dbmethods_t zone_methods = {
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
	zone_find,
	zone_findzonecut,
	attachnode,
	detachnode,
	expirenode,
	printnode,
	createiterator,
	zone_findrdataset,
	allrdatasets,
	addrdataset,
	subtractrdataset,
	deleterdataset,
	issecure,
	nodecount,
	ispersistent,
	overmem,
	settask,
	getoriginnode,
	NULL,
	getnsec3parameters,
	findnsec3node,
	setsigningtime,
	getsigningtime,
	resigned,
	isdnssec,
	NULL
};

static dns_dbmethods_t cache_methods = {
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
	cache_find,
	cache_findzonecut,
	attachnode,
	detachnode,
	expirenode,
	printnode,
	createiterator,
	cache_findrdataset,
	allrdatasets,
	addrdataset,
	subtractrdataset,
	deleterdataset,
	issecure,
	nodecount,
	ispersistent,
	overmem,
	settask,
	getoriginnode,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	isdnssec,
	getrrsetstats
};

isc_result_t
#ifdef DNS_RBTDB_VERSION64
dns_rbtdb64_create
#else
dns_rbtdb_create
#endif
		(isc_mem_t *mctx, dns_name_t *origin, dns_dbtype_t type,
		 dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
		 void *driverarg, dns_db_t **dbp)
{
	dns_rbtdb_t *rbtdb;
	isc_result_t result;
	int i;
	dns_name_t name;
	isc_boolean_t (*sooner)(void *, void *);

	/* Keep the compiler happy. */
	UNUSED(argc);
	UNUSED(argv);
	UNUSED(driverarg);

	rbtdb = isc_mem_get(mctx, sizeof(*rbtdb));
	if (rbtdb == NULL)
		return (ISC_R_NOMEMORY);

	memset(rbtdb, '\0', sizeof(*rbtdb));
	dns_name_init(&rbtdb->common.origin, NULL);
	rbtdb->common.attributes = 0;
	if (type == dns_dbtype_cache) {
		rbtdb->common.methods = &cache_methods;
		rbtdb->common.attributes |= DNS_DBATTR_CACHE;
	} else if (type == dns_dbtype_stub) {
		rbtdb->common.methods = &zone_methods;
		rbtdb->common.attributes |= DNS_DBATTR_STUB;
	} else
		rbtdb->common.methods = &zone_methods;
	rbtdb->common.rdclass = rdclass;
	rbtdb->common.mctx = NULL;

	result = RBTDB_INITLOCK(&rbtdb->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_rbtdb;

	result = isc_rwlock_init(&rbtdb->tree_lock, 0, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup_lock;

	/*
	 * Initialize node_lock_count in a generic way to support future
	 * extension which allows the user to specify this value on creation.
	 * Note that when specified for a cache DB it must be larger than 1
	 * as commented with the definition of DEFAULT_CACHE_NODE_LOCK_COUNT.
	 */
	if (rbtdb->node_lock_count == 0) {
		if (IS_CACHE(rbtdb))
			rbtdb->node_lock_count = DEFAULT_CACHE_NODE_LOCK_COUNT;
		else
			rbtdb->node_lock_count = DEFAULT_NODE_LOCK_COUNT;
	} else if (rbtdb->node_lock_count < 2 && IS_CACHE(rbtdb)) {
		result = ISC_R_RANGE;
		goto cleanup_tree_lock;
	}
	INSIST(rbtdb->node_lock_count < (1 << DNS_RBT_LOCKLENGTH));
	rbtdb->node_locks = isc_mem_get(mctx, rbtdb->node_lock_count *
					sizeof(rbtdb_nodelock_t));
	if (rbtdb->node_locks == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_tree_lock;
	}

	rbtdb->rrsetstats = NULL;
	if (IS_CACHE(rbtdb)) {
		result = dns_rdatasetstats_create(mctx, &rbtdb->rrsetstats);
		if (result != ISC_R_SUCCESS)
			goto cleanup_node_locks;
		rbtdb->rdatasets = isc_mem_get(mctx, rbtdb->node_lock_count *
					       sizeof(rdatasetheaderlist_t));
		if (rbtdb->rdatasets == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup_rrsetstats;
		}
		for (i = 0; i < (int)rbtdb->node_lock_count; i++)
			ISC_LIST_INIT(rbtdb->rdatasets[i]);
	} else
		rbtdb->rdatasets = NULL;

	/*
	 * Create the heaps.
	 */
	rbtdb->heaps = isc_mem_get(mctx, rbtdb->node_lock_count *
				   sizeof(isc_heap_t *));
	if (rbtdb->heaps == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_rdatasets;
	}
	for (i = 0; i < (int)rbtdb->node_lock_count; i++)
		rbtdb->heaps[i] = NULL;
	sooner = IS_CACHE(rbtdb) ? ttl_sooner : resign_sooner;
	for (i = 0; i < (int)rbtdb->node_lock_count; i++) {
		result = isc_heap_create(mctx, sooner, set_index, 0,
					 &rbtdb->heaps[i]);
		if (result != ISC_R_SUCCESS)
			goto cleanup_heaps;
	}

	/*
	 * Create deadnode lists.
	 */
	rbtdb->deadnodes = isc_mem_get(mctx, rbtdb->node_lock_count *
				       sizeof(rbtnodelist_t));
	if (rbtdb->deadnodes == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_heaps;
	}
	for (i = 0; i < (int)rbtdb->node_lock_count; i++)
		ISC_LIST_INIT(rbtdb->deadnodes[i]);

	rbtdb->active = rbtdb->node_lock_count;

	for (i = 0; i < (int)(rbtdb->node_lock_count); i++) {
		result = NODE_INITLOCK(&rbtdb->node_locks[i].lock);
		if (result == ISC_R_SUCCESS) {
			result = isc_refcount_init(&rbtdb->node_locks[i].references, 0);
			if (result != ISC_R_SUCCESS)
				NODE_DESTROYLOCK(&rbtdb->node_locks[i].lock);
		}
		if (result != ISC_R_SUCCESS) {
			while (i-- > 0) {
				NODE_DESTROYLOCK(&rbtdb->node_locks[i].lock);
				isc_refcount_decrement(&rbtdb->node_locks[i].references, NULL);
				isc_refcount_destroy(&rbtdb->node_locks[i].references);
			}
			goto cleanup_deadnodes;
		}
		rbtdb->node_locks[i].exiting = ISC_FALSE;
	}

	/*
	 * Attach to the mctx.  The database will persist so long as there
	 * are references to it, and attaching to the mctx ensures that our
	 * mctx won't disappear out from under us.
	 */
	isc_mem_attach(mctx, &rbtdb->common.mctx);

	/*
	 * Must be initialized before free_rbtdb() is called.
	 */
	isc_ondestroy_init(&rbtdb->common.ondest);

	/*
	 * Make a copy of the origin name.
	 */
	result = dns_name_dupwithoffsets(origin, mctx, &rbtdb->common.origin);
	if (result != ISC_R_SUCCESS) {
		free_rbtdb(rbtdb, ISC_FALSE, NULL);
		return (result);
	}

	/*
	 * Make the Red-Black Trees.
	 */
	result = dns_rbt_create(mctx, delete_callback, rbtdb, &rbtdb->tree);
	if (result != ISC_R_SUCCESS) {
		free_rbtdb(rbtdb, ISC_FALSE, NULL);
		return (result);
	}

	result = dns_rbt_create(mctx, delete_callback, rbtdb, &rbtdb->nsec3);
	if (result != ISC_R_SUCCESS) {
		free_rbtdb(rbtdb, ISC_FALSE, NULL);
		return (result);
	}

	/*
	 * In order to set the node callback bit correctly in zone databases,
	 * we need to know if the node has the origin name of the zone.
	 * In loading_addrdataset() we could simply compare the new name
	 * to the origin name, but this is expensive.  Also, we don't know the
	 * node name in addrdataset(), so we need another way of knowing the
	 * zone's top.
	 *
	 * We now explicitly create a node for the zone's origin, and then
	 * we simply remember the node's address.  This is safe, because
	 * the top-of-zone node can never be deleted, nor can its address
	 * change.
	 */
	if (!IS_CACHE(rbtdb)) {
		rbtdb->origin_node = NULL;
		result = dns_rbt_addnode(rbtdb->tree, &rbtdb->common.origin,
					 &rbtdb->origin_node);
		if (result != ISC_R_SUCCESS) {
			INSIST(result != ISC_R_EXISTS);
			free_rbtdb(rbtdb, ISC_FALSE, NULL);
			return (result);
		}
		rbtdb->origin_node->nsec3 = 0;
		/*
		 * We need to give the origin node the right locknum.
		 */
		dns_name_init(&name, NULL);
		dns_rbt_namefromnode(rbtdb->origin_node, &name);
#ifdef DNS_RBT_USEHASH
		rbtdb->origin_node->locknum =
			rbtdb->origin_node->hashval %
			rbtdb->node_lock_count;
#else
		rbtdb->origin_node->locknum =
			dns_name_hash(&name, ISC_TRUE) %
			rbtdb->node_lock_count;
#endif
	}

	/*
	 * Misc. Initialization.
	 */
	result = isc_refcount_init(&rbtdb->references, 1);
	if (result != ISC_R_SUCCESS) {
		free_rbtdb(rbtdb, ISC_FALSE, NULL);
		return (result);
	}
	rbtdb->attributes = 0;
	rbtdb->overmem = ISC_FALSE;
	rbtdb->task = NULL;

	/*
	 * Version Initialization.
	 */
	rbtdb->current_serial = 1;
	rbtdb->least_serial = 1;
	rbtdb->next_serial = 2;
	rbtdb->current_version = allocate_version(mctx, 1, 1, ISC_FALSE);
	if (rbtdb->current_version == NULL) {
		isc_refcount_decrement(&rbtdb->references, NULL);
		isc_refcount_destroy(&rbtdb->references);
		free_rbtdb(rbtdb, ISC_FALSE, NULL);
		return (ISC_R_NOMEMORY);
	}
	rbtdb->current_version->secure = dns_db_insecure;
	rbtdb->current_version->havensec3 = ISC_FALSE;
	rbtdb->current_version->flags = 0;
	rbtdb->current_version->iterations = 0;
	rbtdb->current_version->hash = 0;
	rbtdb->current_version->salt_length = 0;
	memset(rbtdb->current_version->salt, 0,
	       sizeof(rbtdb->current_version->salt));
	rbtdb->future_version = NULL;
	ISC_LIST_INIT(rbtdb->open_versions);
	/*
	 * Keep the current version in the open list so that list operation
	 * won't happen in normal lookup operations.
	 */
	PREPEND(rbtdb->open_versions, rbtdb->current_version, link);

	rbtdb->common.magic = DNS_DB_MAGIC;
	rbtdb->common.impmagic = RBTDB_MAGIC;

	*dbp = (dns_db_t *)rbtdb;

	return (ISC_R_SUCCESS);

 cleanup_deadnodes:
	isc_mem_put(mctx, rbtdb->deadnodes,
		    rbtdb->node_lock_count * sizeof(rbtnodelist_t));

 cleanup_heaps:
	if (rbtdb->heaps != NULL) {
		for (i = 0 ; i < (int)rbtdb->node_lock_count ; i++)
			if (rbtdb->heaps[i] != NULL)
				isc_heap_destroy(&rbtdb->heaps[i]);
		isc_mem_put(mctx, rbtdb->heaps,
			    rbtdb->node_lock_count * sizeof(isc_heap_t *));
	}

 cleanup_rdatasets:
	if (rbtdb->rdatasets != NULL)
		isc_mem_put(mctx, rbtdb->rdatasets, rbtdb->node_lock_count *
			    sizeof(rdatasetheaderlist_t));
 cleanup_rrsetstats:
	if (rbtdb->rrsetstats != NULL)
		dns_stats_detach(&rbtdb->rrsetstats);

 cleanup_node_locks:
	isc_mem_put(mctx, rbtdb->node_locks,
		    rbtdb->node_lock_count * sizeof(rbtdb_nodelock_t));

 cleanup_tree_lock:
	isc_rwlock_destroy(&rbtdb->tree_lock);

 cleanup_lock:
	RBTDB_DESTROYLOCK(&rbtdb->lock);

 cleanup_rbtdb:
	isc_mem_put(mctx, rbtdb,  sizeof(*rbtdb));
	return (result);
}


/*
 * Slabbed Rdataset Methods
 */

static void
rdataset_disassociate(dns_rdataset_t *rdataset) {
	dns_db_t *db = rdataset->private1;
	dns_dbnode_t *node = rdataset->private2;

	detachnode(db, &node);
}

static isc_result_t
rdataset_first(dns_rdataset_t *rdataset) {
	unsigned char *raw = rdataset->private3;        /* RDATASLAB */
	unsigned int count;

	count = raw[0] * 256 + raw[1];
	if (count == 0) {
		rdataset->private5 = NULL;
		return (ISC_R_NOMORE);
	}

#if DNS_RDATASET_FIXED
	if ((rdataset->attributes & DNS_RDATASETATTR_LOADORDER) == 0)
		raw += 2 + (4 * count);
	else
#endif
		raw += 2;

	/*
	 * The privateuint4 field is the number of rdata beyond the
	 * cursor position, so we decrement the total count by one
	 * before storing it.
	 *
	 * If DNS_RDATASETATTR_LOADORDER is not set 'raw' points to the
	 * first record.  If DNS_RDATASETATTR_LOADORDER is set 'raw' points
	 * to the first entry in the offset table.
	 */
	count--;
	rdataset->privateuint4 = count;
	rdataset->private5 = raw;

	return (ISC_R_SUCCESS);
}

static isc_result_t
rdataset_next(dns_rdataset_t *rdataset) {
	unsigned int count;
	unsigned int length;
	unsigned char *raw;     /* RDATASLAB */

	count = rdataset->privateuint4;
	if (count == 0)
		return (ISC_R_NOMORE);
	count--;
	rdataset->privateuint4 = count;

	/*
	 * Skip forward one record (length + 4) or one offset (4).
	 */
	raw = rdataset->private5;
#if DNS_RDATASET_FIXED
	if ((rdataset->attributes & DNS_RDATASETATTR_LOADORDER) == 0) {
#endif
		length = raw[0] * 256 + raw[1];
		raw += length;
#if DNS_RDATASET_FIXED
	}
	rdataset->private5 = raw + 4;           /* length(2) + order(2) */
#else
	rdataset->private5 = raw + 2;           /* length(2) */
#endif

	return (ISC_R_SUCCESS);
}

static void
rdataset_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	unsigned char *raw = rdataset->private5;        /* RDATASLAB */
#if DNS_RDATASET_FIXED
	unsigned int offset;
#endif
	unsigned int length;
	isc_region_t r;
	unsigned int flags = 0;

	REQUIRE(raw != NULL);

	/*
	 * Find the start of the record if not already in private5
	 * then skip the length and order fields.
	 */
#if DNS_RDATASET_FIXED
	if ((rdataset->attributes & DNS_RDATASETATTR_LOADORDER) != 0) {
		offset = (raw[0] << 24) + (raw[1] << 16) +
			 (raw[2] << 8) + raw[3];
		raw = rdataset->private3;
		raw += offset;
	}
#endif
	length = raw[0] * 256 + raw[1];
#if DNS_RDATASET_FIXED
	raw += 4;
#else
	raw += 2;
#endif
	if (rdataset->type == dns_rdatatype_rrsig) {
		if (*raw & DNS_RDATASLAB_OFFLINE)
			flags |= DNS_RDATA_OFFLINE;
		length--;
		raw++;
	}
	r.length = length;
	r.base = raw;
	dns_rdata_fromregion(rdata, rdataset->rdclass, rdataset->type, &r);
	rdata->flags |= flags;
}

static void
rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target) {
	dns_db_t *db = source->private1;
	dns_dbnode_t *node = source->private2;
	dns_dbnode_t *cloned_node = NULL;

	attachnode(db, node, &cloned_node);
	*target = *source;

	/*
	 * Reset iterator state.
	 */
	target->privateuint4 = 0;
	target->private5 = NULL;
}

static unsigned int
rdataset_count(dns_rdataset_t *rdataset) {
	unsigned char *raw = rdataset->private3;        /* RDATASLAB */
	unsigned int count;

	count = raw[0] * 256 + raw[1];

	return (count);
}

static isc_result_t
rdataset_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
		    dns_rdataset_t *nsec, dns_rdataset_t *nsecsig)
{
	dns_db_t *db = rdataset->private1;
	dns_dbnode_t *node = rdataset->private2;
	dns_dbnode_t *cloned_node;
	struct noqname *noqname = rdataset->private6;

	cloned_node = NULL;
	attachnode(db, node, &cloned_node);
	nsec->methods = &rdataset_methods;
	nsec->rdclass = db->rdclass;
	nsec->type = noqname->type;
	nsec->covers = 0;
	nsec->ttl = rdataset->ttl;
	nsec->trust = rdataset->trust;
	nsec->private1 = rdataset->private1;
	nsec->private2 = rdataset->private2;
	nsec->private3 = noqname->neg;
	nsec->privateuint4 = 0;
	nsec->private5 = NULL;
	nsec->private6 = NULL;
	nsec->private7 = NULL;

	cloned_node = NULL;
	attachnode(db, node, &cloned_node);
	nsecsig->methods = &rdataset_methods;
	nsecsig->rdclass = db->rdclass;
	nsecsig->type = dns_rdatatype_rrsig;
	nsecsig->covers = noqname->type;
	nsecsig->ttl = rdataset->ttl;
	nsecsig->trust = rdataset->trust;
	nsecsig->private1 = rdataset->private1;
	nsecsig->private2 = rdataset->private2;
	nsecsig->private3 = noqname->negsig;
	nsecsig->privateuint4 = 0;
	nsecsig->private5 = NULL;
	nsec->private6 = NULL;
	nsec->private7 = NULL;

	dns_name_clone(&noqname->name, name);

	return (ISC_R_SUCCESS);
}

static isc_result_t
rdataset_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
		    dns_rdataset_t *nsec, dns_rdataset_t *nsecsig)
{
	dns_db_t *db = rdataset->private1;
	dns_dbnode_t *node = rdataset->private2;
	dns_dbnode_t *cloned_node;
	struct noqname *closest = rdataset->private7;

	cloned_node = NULL;
	attachnode(db, node, &cloned_node);
	nsec->methods = &rdataset_methods;
	nsec->rdclass = db->rdclass;
	nsec->type = closest->type;
	nsec->covers = 0;
	nsec->ttl = rdataset->ttl;
	nsec->trust = rdataset->trust;
	nsec->private1 = rdataset->private1;
	nsec->private2 = rdataset->private2;
	nsec->private3 = closest->neg;
	nsec->privateuint4 = 0;
	nsec->private5 = NULL;
	nsec->private6 = NULL;
	nsec->private7 = NULL;

	cloned_node = NULL;
	attachnode(db, node, &cloned_node);
	nsecsig->methods = &rdataset_methods;
	nsecsig->rdclass = db->rdclass;
	nsecsig->type = dns_rdatatype_rrsig;
	nsecsig->covers = closest->type;
	nsecsig->ttl = rdataset->ttl;
	nsecsig->trust = rdataset->trust;
	nsecsig->private1 = rdataset->private1;
	nsecsig->private2 = rdataset->private2;
	nsecsig->private3 = closest->negsig;
	nsecsig->privateuint4 = 0;
	nsecsig->private5 = NULL;
	nsec->private6 = NULL;
	nsec->private7 = NULL;

	dns_name_clone(&closest->name, name);

	return (ISC_R_SUCCESS);
}

/*
 * Rdataset Iterator Methods
 */

static void
rdatasetiter_destroy(dns_rdatasetiter_t **iteratorp) {
	rbtdb_rdatasetiter_t *rbtiterator;

	rbtiterator = (rbtdb_rdatasetiter_t *)(*iteratorp);

	if (rbtiterator->common.version != NULL)
		closeversion(rbtiterator->common.db,
			     &rbtiterator->common.version, ISC_FALSE);
	detachnode(rbtiterator->common.db, &rbtiterator->common.node);
	isc_mem_put(rbtiterator->common.db->mctx, rbtiterator,
		    sizeof(*rbtiterator));

	*iteratorp = NULL;
}

static isc_result_t
rdatasetiter_first(dns_rdatasetiter_t *iterator) {
	rbtdb_rdatasetiter_t *rbtiterator = (rbtdb_rdatasetiter_t *)iterator;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)(rbtiterator->common.db);
	dns_rbtnode_t *rbtnode = rbtiterator->common.node;
	rbtdb_version_t *rbtversion = rbtiterator->common.version;
	rdatasetheader_t *header, *top_next;
	rbtdb_serial_t serial;
	isc_stdtime_t now;

	if (IS_CACHE(rbtdb)) {
		serial = 1;
		now = rbtiterator->common.now;
	} else {
		serial = rbtversion->serial;
		now = 0;
	}

	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_read);

	for (header = rbtnode->data; header != NULL; header = top_next) {
		top_next = header->next;
		do {
			if (header->serial <= serial && !IGNORE(header)) {
				/*
				 * Is this a "this rdataset doesn't exist"
				 * record?  Or is it too old in the cache?
				 *
				 * Note: unlike everywhere else, we
				 * check for now > header->rdh_ttl instead
				 * of now >= header->rdh_ttl.  This allows
				 * ANY and RRSIG queries for 0 TTL
				 * rdatasets to work.
				 */
				if (NONEXISTENT(header) ||
				    (now != 0 && now > header->rdh_ttl))
					header = NULL;
				break;
			} else
				header = header->down;
		} while (header != NULL);
		if (header != NULL)
			break;
	}

	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_read);

	rbtiterator->current = header;

	if (header == NULL)
		return (ISC_R_NOMORE);

	return (ISC_R_SUCCESS);
}

static isc_result_t
rdatasetiter_next(dns_rdatasetiter_t *iterator) {
	rbtdb_rdatasetiter_t *rbtiterator = (rbtdb_rdatasetiter_t *)iterator;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)(rbtiterator->common.db);
	dns_rbtnode_t *rbtnode = rbtiterator->common.node;
	rbtdb_version_t *rbtversion = rbtiterator->common.version;
	rdatasetheader_t *header, *top_next;
	rbtdb_serial_t serial;
	isc_stdtime_t now;
	rbtdb_rdatatype_t type, negtype;
	dns_rdatatype_t rdtype, covers;

	header = rbtiterator->current;
	if (header == NULL)
		return (ISC_R_NOMORE);

	if (IS_CACHE(rbtdb)) {
		serial = 1;
		now = rbtiterator->common.now;
	} else {
		serial = rbtversion->serial;
		now = 0;
	}

	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_read);

	type = header->type;
	rdtype = RBTDB_RDATATYPE_BASE(header->type);
	if (rdtype == 0) {
		covers = RBTDB_RDATATYPE_EXT(header->type);
		negtype = RBTDB_RDATATYPE_VALUE(covers, 0);
	} else
		negtype = RBTDB_RDATATYPE_VALUE(0, rdtype);
	for (header = header->next; header != NULL; header = top_next) {
		top_next = header->next;
		/*
		 * If not walking back up the down list.
		 */
		if (header->type != type && header->type != negtype) {
			do {
				if (header->serial <= serial &&
				    !IGNORE(header)) {
					/*
					 * Is this a "this rdataset doesn't
					 * exist" record?
					 *
					 * Note: unlike everywhere else, we
					 * check for now > header->ttl instead
					 * of now >= header->ttl.  This allows
					 * ANY and RRSIG queries for 0 TTL
					 * rdatasets to work.
					 */
					if ((header->attributes &
					     RDATASET_ATTR_NONEXISTENT) != 0 ||
					    (now != 0 && now > header->rdh_ttl))
						header = NULL;
					break;
				} else
					header = header->down;
			} while (header != NULL);
			if (header != NULL)
				break;
		}
	}

	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_read);

	rbtiterator->current = header;

	if (header == NULL)
		return (ISC_R_NOMORE);

	return (ISC_R_SUCCESS);
}

static void
rdatasetiter_current(dns_rdatasetiter_t *iterator, dns_rdataset_t *rdataset) {
	rbtdb_rdatasetiter_t *rbtiterator = (rbtdb_rdatasetiter_t *)iterator;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)(rbtiterator->common.db);
	dns_rbtnode_t *rbtnode = rbtiterator->common.node;
	rdatasetheader_t *header;

	header = rbtiterator->current;
	REQUIRE(header != NULL);

	NODE_LOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		  isc_rwlocktype_read);

	bind_rdataset(rbtdb, rbtnode, header, rbtiterator->common.now,
		      rdataset);

	NODE_UNLOCK(&rbtdb->node_locks[rbtnode->locknum].lock,
		    isc_rwlocktype_read);
}


/*
 * Database Iterator Methods
 */

static inline void
reference_iter_node(rbtdb_dbiterator_t *rbtdbiter) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)rbtdbiter->common.db;
	dns_rbtnode_t *node = rbtdbiter->node;

	if (node == NULL)
		return;

	INSIST(rbtdbiter->tree_locked != isc_rwlocktype_none);
	reactivate_node(rbtdb, node, rbtdbiter->tree_locked);
}

static inline void
dereference_iter_node(rbtdb_dbiterator_t *rbtdbiter) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)rbtdbiter->common.db;
	dns_rbtnode_t *node = rbtdbiter->node;
	nodelock_t *lock;

	if (node == NULL)
		return;

	lock = &rbtdb->node_locks[node->locknum].lock;
	NODE_LOCK(lock, isc_rwlocktype_read);
	decrement_reference(rbtdb, node, 0, isc_rwlocktype_read,
			    rbtdbiter->tree_locked, ISC_FALSE);
	NODE_UNLOCK(lock, isc_rwlocktype_read);

	rbtdbiter->node = NULL;
}

static void
flush_deletions(rbtdb_dbiterator_t *rbtdbiter) {
	dns_rbtnode_t *node;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)rbtdbiter->common.db;
	isc_boolean_t was_read_locked = ISC_FALSE;
	nodelock_t *lock;
	int i;

	if (rbtdbiter->delete != 0) {
		/*
		 * Note that "%d node of %d in tree" can report things like
		 * "flush_deletions: 59 nodes of 41 in tree".  This means
		 * That some nodes appear on the deletions list more than
		 * once.  Only the last occurence will actually be deleted.
		 */
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_CACHE, ISC_LOG_DEBUG(1),
			      "flush_deletions: %d nodes of %d in tree",
			      rbtdbiter->delete,
			      dns_rbt_nodecount(rbtdb->tree));

		if (rbtdbiter->tree_locked == isc_rwlocktype_read) {
			RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
			was_read_locked = ISC_TRUE;
		}
		RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);
		rbtdbiter->tree_locked = isc_rwlocktype_write;

		for (i = 0; i < rbtdbiter->delete; i++) {
			node = rbtdbiter->deletions[i];
			lock = &rbtdb->node_locks[node->locknum].lock;

			NODE_LOCK(lock, isc_rwlocktype_read);
			decrement_reference(rbtdb, node, 0,
					    isc_rwlocktype_read,
					    rbtdbiter->tree_locked, ISC_FALSE);
			NODE_UNLOCK(lock, isc_rwlocktype_read);
		}

		rbtdbiter->delete = 0;

		RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_write);
		if (was_read_locked) {
			RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
			rbtdbiter->tree_locked = isc_rwlocktype_read;

		} else {
			rbtdbiter->tree_locked = isc_rwlocktype_none;
		}
	}
}

static inline void
resume_iteration(rbtdb_dbiterator_t *rbtdbiter) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)rbtdbiter->common.db;

	REQUIRE(rbtdbiter->paused);
	REQUIRE(rbtdbiter->tree_locked == isc_rwlocktype_none);

	RWLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
	rbtdbiter->tree_locked = isc_rwlocktype_read;

	rbtdbiter->paused = ISC_FALSE;
}

static void
dbiterator_destroy(dns_dbiterator_t **iteratorp) {
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)(*iteratorp);
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)rbtdbiter->common.db;
	dns_db_t *db = NULL;

	if (rbtdbiter->tree_locked == isc_rwlocktype_read) {
		RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
		rbtdbiter->tree_locked = isc_rwlocktype_none;
	} else
		INSIST(rbtdbiter->tree_locked == isc_rwlocktype_none);

	dereference_iter_node(rbtdbiter);

	flush_deletions(rbtdbiter);

	dns_db_attach(rbtdbiter->common.db, &db);
	dns_db_detach(&rbtdbiter->common.db);

	dns_rbtnodechain_reset(&rbtdbiter->chain);
	dns_rbtnodechain_reset(&rbtdbiter->nsec3chain);
	isc_mem_put(db->mctx, rbtdbiter, sizeof(*rbtdbiter));
	dns_db_detach(&db);

	*iteratorp = NULL;
}

static isc_result_t
dbiterator_first(dns_dbiterator_t *iterator) {
	isc_result_t result;
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)iterator;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)iterator->db;
	dns_name_t *name, *origin;

	if (rbtdbiter->result != ISC_R_SUCCESS &&
	    rbtdbiter->result != ISC_R_NOMORE)
		return (rbtdbiter->result);

	if (rbtdbiter->paused)
		resume_iteration(rbtdbiter);

	dereference_iter_node(rbtdbiter);

	name = dns_fixedname_name(&rbtdbiter->name);
	origin = dns_fixedname_name(&rbtdbiter->origin);
	dns_rbtnodechain_reset(&rbtdbiter->chain);
	dns_rbtnodechain_reset(&rbtdbiter->nsec3chain);

	if (rbtdbiter->nsec3only) {
		rbtdbiter->current = &rbtdbiter->nsec3chain;
		result = dns_rbtnodechain_first(rbtdbiter->current,
						rbtdb->nsec3, name, origin);
	} else {
		rbtdbiter->current = &rbtdbiter->chain;
		result = dns_rbtnodechain_first(rbtdbiter->current,
						rbtdb->tree, name, origin);
		if (!rbtdbiter->nonsec3 && result == ISC_R_NOTFOUND) {
			rbtdbiter->current = &rbtdbiter->nsec3chain;
			result = dns_rbtnodechain_first(rbtdbiter->current,
							rbtdb->nsec3, name,
							origin);
		}
	}
	if (result == ISC_R_SUCCESS || result == DNS_R_NEWORIGIN) {
		result = dns_rbtnodechain_current(rbtdbiter->current, NULL,
						  NULL, &rbtdbiter->node);
		if (result == ISC_R_SUCCESS) {
			rbtdbiter->new_origin = ISC_TRUE;
			reference_iter_node(rbtdbiter);
		}
	} else {
		INSIST(result == ISC_R_NOTFOUND);
		result = ISC_R_NOMORE; /* The tree is empty. */
	}

	rbtdbiter->result = result;

	return (result);
}

static isc_result_t
dbiterator_last(dns_dbiterator_t *iterator) {
	isc_result_t result;
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)iterator;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)iterator->db;
	dns_name_t *name, *origin;

	if (rbtdbiter->result != ISC_R_SUCCESS &&
	    rbtdbiter->result != ISC_R_NOMORE)
		return (rbtdbiter->result);

	if (rbtdbiter->paused)
		resume_iteration(rbtdbiter);

	dereference_iter_node(rbtdbiter);

	name = dns_fixedname_name(&rbtdbiter->name);
	origin = dns_fixedname_name(&rbtdbiter->origin);
	dns_rbtnodechain_reset(&rbtdbiter->chain);
	dns_rbtnodechain_reset(&rbtdbiter->nsec3chain);

	result = ISC_R_NOTFOUND;
	if (rbtdbiter->nsec3only && !rbtdbiter->nonsec3) {
		rbtdbiter->current = &rbtdbiter->nsec3chain;
		result = dns_rbtnodechain_last(rbtdbiter->current,
					       rbtdb->nsec3, name, origin);
	}
	if (!rbtdbiter->nsec3only && result == ISC_R_NOTFOUND) {
		rbtdbiter->current = &rbtdbiter->chain;
		result = dns_rbtnodechain_last(rbtdbiter->current, rbtdb->tree,
					       name, origin);
	}
	if (result == ISC_R_SUCCESS || result == DNS_R_NEWORIGIN) {
		result = dns_rbtnodechain_current(rbtdbiter->current, NULL,
						  NULL, &rbtdbiter->node);
		if (result == ISC_R_SUCCESS) {
			rbtdbiter->new_origin = ISC_TRUE;
			reference_iter_node(rbtdbiter);
		}
	} else {
		INSIST(result == ISC_R_NOTFOUND);
		result = ISC_R_NOMORE; /* The tree is empty. */
	}

	rbtdbiter->result = result;

	return (result);
}

static isc_result_t
dbiterator_seek(dns_dbiterator_t *iterator, dns_name_t *name) {
	isc_result_t result;
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)iterator;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)iterator->db;
	dns_name_t *iname, *origin;

	if (rbtdbiter->result != ISC_R_SUCCESS &&
	    rbtdbiter->result != ISC_R_NOTFOUND &&
	    rbtdbiter->result != ISC_R_NOMORE)
		return (rbtdbiter->result);

	if (rbtdbiter->paused)
		resume_iteration(rbtdbiter);

	dereference_iter_node(rbtdbiter);

	iname = dns_fixedname_name(&rbtdbiter->name);
	origin = dns_fixedname_name(&rbtdbiter->origin);
	dns_rbtnodechain_reset(&rbtdbiter->chain);
	dns_rbtnodechain_reset(&rbtdbiter->nsec3chain);

	if (rbtdbiter->nsec3only) {
		rbtdbiter->current = &rbtdbiter->nsec3chain;
		result = dns_rbt_findnode(rbtdb->nsec3, name, NULL,
					  &rbtdbiter->node,
					  rbtdbiter->current,
					  DNS_RBTFIND_EMPTYDATA, NULL, NULL);
	} else if (rbtdbiter->nonsec3) {
		rbtdbiter->current = &rbtdbiter->chain;
		result = dns_rbt_findnode(rbtdb->tree, name, NULL,
					  &rbtdbiter->node,
					  rbtdbiter->current,
					  DNS_RBTFIND_EMPTYDATA, NULL, NULL);
	} else {
		/*
		 * Stay on main chain if not found on either chain.
		 */
		rbtdbiter->current = &rbtdbiter->chain;
		result = dns_rbt_findnode(rbtdb->tree, name, NULL,
					  &rbtdbiter->node,
					  rbtdbiter->current,
					  DNS_RBTFIND_EMPTYDATA, NULL, NULL);
		if (result == DNS_R_PARTIALMATCH) {
			dns_rbtnode_t *node = NULL;
			result = dns_rbt_findnode(rbtdb->nsec3, name, NULL,
						  &node, &rbtdbiter->nsec3chain,
						  DNS_RBTFIND_EMPTYDATA,
						  NULL, NULL);
			if (result == ISC_R_SUCCESS) {
				rbtdbiter->node = node;
				rbtdbiter->current = &rbtdbiter->nsec3chain;
			}
		}
	}

#if 1
	if (result == ISC_R_SUCCESS) {
		result = dns_rbtnodechain_current(rbtdbiter->current, iname,
						  origin, NULL);
		if (result == ISC_R_SUCCESS) {
			rbtdbiter->new_origin = ISC_TRUE;
			reference_iter_node(rbtdbiter);
		}
	} else if (result == DNS_R_PARTIALMATCH) {
		result = ISC_R_NOTFOUND;
		rbtdbiter->node = NULL;
	}

	rbtdbiter->result = result;
#else
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		isc_result_t tresult;
		tresult = dns_rbtnodechain_current(rbtdbiter->current, iname,
						   origin, NULL);
		if (tresult == ISC_R_SUCCESS) {
			rbtdbiter->new_origin = ISC_TRUE;
			reference_iter_node(rbtdbiter);
		} else {
			result = tresult;
			rbtdbiter->node = NULL;
		}
	} else
		rbtdbiter->node = NULL;

	rbtdbiter->result = (result == DNS_R_PARTIALMATCH) ?
			    ISC_R_SUCCESS : result;
#endif

	return (result);
}

static isc_result_t
dbiterator_prev(dns_dbiterator_t *iterator) {
	isc_result_t result;
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)iterator;
	dns_name_t *name, *origin;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)iterator->db;

	REQUIRE(rbtdbiter->node != NULL);

	if (rbtdbiter->result != ISC_R_SUCCESS)
		return (rbtdbiter->result);

	if (rbtdbiter->paused)
		resume_iteration(rbtdbiter);

	name = dns_fixedname_name(&rbtdbiter->name);
	origin = dns_fixedname_name(&rbtdbiter->origin);
	result = dns_rbtnodechain_prev(rbtdbiter->current, name, origin);
	if (result == ISC_R_NOMORE && !rbtdbiter->nsec3only &&
	    !rbtdbiter->nonsec3 &&
	    &rbtdbiter->nsec3chain == rbtdbiter->current) {
		rbtdbiter->current = &rbtdbiter->chain;
		dns_rbtnodechain_reset(rbtdbiter->current);
		result = dns_rbtnodechain_last(rbtdbiter->current, rbtdb->tree,
					       name, origin);
		if (result == ISC_R_NOTFOUND)
			result = ISC_R_NOMORE;
	}

	dereference_iter_node(rbtdbiter);

	if (result == DNS_R_NEWORIGIN || result == ISC_R_SUCCESS) {
		rbtdbiter->new_origin = ISC_TF(result == DNS_R_NEWORIGIN);
		result = dns_rbtnodechain_current(rbtdbiter->current, NULL,
						  NULL, &rbtdbiter->node);
	}

	if (result == ISC_R_SUCCESS)
		reference_iter_node(rbtdbiter);

	rbtdbiter->result = result;

	return (result);
}

static isc_result_t
dbiterator_next(dns_dbiterator_t *iterator) {
	isc_result_t result;
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)iterator;
	dns_name_t *name, *origin;
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)iterator->db;

	REQUIRE(rbtdbiter->node != NULL);

	if (rbtdbiter->result != ISC_R_SUCCESS)
		return (rbtdbiter->result);

	if (rbtdbiter->paused)
		resume_iteration(rbtdbiter);

	name = dns_fixedname_name(&rbtdbiter->name);
	origin = dns_fixedname_name(&rbtdbiter->origin);
	result = dns_rbtnodechain_next(rbtdbiter->current, name, origin);
	if (result == ISC_R_NOMORE && !rbtdbiter->nsec3only &&
	    !rbtdbiter->nonsec3 && &rbtdbiter->chain == rbtdbiter->current) {
		rbtdbiter->current = &rbtdbiter->nsec3chain;
		dns_rbtnodechain_reset(rbtdbiter->current);
		result = dns_rbtnodechain_first(rbtdbiter->current,
						rbtdb->nsec3, name, origin);
		if (result == ISC_R_NOTFOUND)
			result = ISC_R_NOMORE;
	}

	dereference_iter_node(rbtdbiter);

	if (result == DNS_R_NEWORIGIN || result == ISC_R_SUCCESS) {
		rbtdbiter->new_origin = ISC_TF(result == DNS_R_NEWORIGIN);
		result = dns_rbtnodechain_current(rbtdbiter->current, NULL,
						  NULL, &rbtdbiter->node);
	}
	if (result == ISC_R_SUCCESS)
		reference_iter_node(rbtdbiter);

	rbtdbiter->result = result;

	return (result);
}

static isc_result_t
dbiterator_current(dns_dbiterator_t *iterator, dns_dbnode_t **nodep,
		   dns_name_t *name)
{
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)iterator->db;
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)iterator;
	dns_rbtnode_t *node = rbtdbiter->node;
	isc_result_t result;
	dns_name_t *nodename = dns_fixedname_name(&rbtdbiter->name);
	dns_name_t *origin = dns_fixedname_name(&rbtdbiter->origin);

	REQUIRE(rbtdbiter->result == ISC_R_SUCCESS);
	REQUIRE(rbtdbiter->node != NULL);

	if (rbtdbiter->paused)
		resume_iteration(rbtdbiter);

	if (name != NULL) {
		if (rbtdbiter->common.relative_names)
			origin = NULL;
		result = dns_name_concatenate(nodename, origin, name, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);
		if (rbtdbiter->common.relative_names && rbtdbiter->new_origin)
			result = DNS_R_NEWORIGIN;
	} else
		result = ISC_R_SUCCESS;

	NODE_STRONGLOCK(&rbtdb->node_locks[node->locknum].lock);
	new_reference(rbtdb, node);
	NODE_STRONGUNLOCK(&rbtdb->node_locks[node->locknum].lock);

	*nodep = rbtdbiter->node;

	if (iterator->cleaning && result == ISC_R_SUCCESS) {
		isc_result_t expire_result;

		/*
		 * If the deletion array is full, flush it before trying
		 * to expire the current node.  The current node can't
		 * fully deleted while the iteration cursor is still on it.
		 */
		if (rbtdbiter->delete == DELETION_BATCH_MAX)
			flush_deletions(rbtdbiter);

		expire_result = expirenode(iterator->db, *nodep, 0);

		/*
		 * expirenode() currently always returns success.
		 */
		if (expire_result == ISC_R_SUCCESS && node->down == NULL) {
			unsigned int refs;

			rbtdbiter->deletions[rbtdbiter->delete++] = node;
			NODE_STRONGLOCK(&rbtdb->node_locks[node->locknum].lock);
			dns_rbtnode_refincrement(node, &refs);
			INSIST(refs != 0);
			NODE_STRONGUNLOCK(&rbtdb->node_locks[node->locknum].lock);
		}
	}

	return (result);
}

static isc_result_t
dbiterator_pause(dns_dbiterator_t *iterator) {
	dns_rbtdb_t *rbtdb = (dns_rbtdb_t *)iterator->db;
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)iterator;

	if (rbtdbiter->result != ISC_R_SUCCESS &&
	    rbtdbiter->result != ISC_R_NOMORE)
		return (rbtdbiter->result);

	if (rbtdbiter->paused)
		return (ISC_R_SUCCESS);

	rbtdbiter->paused = ISC_TRUE;

	if (rbtdbiter->tree_locked != isc_rwlocktype_none) {
		INSIST(rbtdbiter->tree_locked == isc_rwlocktype_read);
		RWUNLOCK(&rbtdb->tree_lock, isc_rwlocktype_read);
		rbtdbiter->tree_locked = isc_rwlocktype_none;
	}

	flush_deletions(rbtdbiter);

	return (ISC_R_SUCCESS);
}

static isc_result_t
dbiterator_origin(dns_dbiterator_t *iterator, dns_name_t *name) {
	rbtdb_dbiterator_t *rbtdbiter = (rbtdb_dbiterator_t *)iterator;
	dns_name_t *origin = dns_fixedname_name(&rbtdbiter->origin);

	if (rbtdbiter->result != ISC_R_SUCCESS)
		return (rbtdbiter->result);

	return (dns_name_copy(origin, name, NULL));
}

/*%
 * Additional cache routines.
 */
static isc_result_t
rdataset_getadditional(dns_rdataset_t *rdataset, dns_rdatasetadditional_t type,
		       dns_rdatatype_t qtype, dns_acache_t *acache,
		       dns_zone_t **zonep, dns_db_t **dbp,
		       dns_dbversion_t **versionp, dns_dbnode_t **nodep,
		       dns_name_t *fname, dns_message_t *msg,
		       isc_stdtime_t now)
{
	dns_rbtdb_t *rbtdb = rdataset->private1;
	dns_rbtnode_t *rbtnode = rdataset->private2;
	unsigned char *raw = rdataset->private3;        /* RDATASLAB */
	unsigned int current_count = rdataset->privateuint4;
	unsigned int count;
	rdatasetheader_t *header;
	nodelock_t *nodelock;
	unsigned int total_count;
	acachectl_t *acarray;
	dns_acacheentry_t *entry;
	isc_result_t result;

	UNUSED(qtype); /* we do not use this value at least for now */
	UNUSED(acache);

	header = (struct rdatasetheader *)(raw - sizeof(*header));

	total_count = raw[0] * 256 + raw[1];
	INSIST(total_count > current_count);
	count = total_count - current_count - 1;

	acarray = NULL;

	nodelock = &rbtdb->node_locks[rbtnode->locknum].lock;
	NODE_LOCK(nodelock, isc_rwlocktype_read);

	switch (type) {
	case dns_rdatasetadditional_fromauth:
		acarray = header->additional_auth;
		break;
	case dns_rdatasetadditional_fromcache:
		acarray = NULL;
		break;
	case dns_rdatasetadditional_fromglue:
		acarray = header->additional_glue;
		break;
	default:
		INSIST(0);
	}

	if (acarray == NULL) {
		if (type != dns_rdatasetadditional_fromcache)
			dns_acache_countquerymiss(acache);
		NODE_UNLOCK(nodelock, isc_rwlocktype_read);
		return (ISC_R_NOTFOUND);
	}

	if (acarray[count].entry == NULL) {
		dns_acache_countquerymiss(acache);
		NODE_UNLOCK(nodelock, isc_rwlocktype_read);
		return (ISC_R_NOTFOUND);
	}

	entry = NULL;
	dns_acache_attachentry(acarray[count].entry, &entry);

	NODE_UNLOCK(nodelock, isc_rwlocktype_read);

	result = dns_acache_getentry(entry, zonep, dbp, versionp,
				     nodep, fname, msg, now);

	dns_acache_detachentry(&entry);

	return (result);
}

static void
acache_callback(dns_acacheentry_t *entry, void **arg) {
	dns_rbtdb_t *rbtdb;
	dns_rbtnode_t *rbtnode;
	nodelock_t *nodelock;
	acachectl_t *acarray = NULL;
	acache_cbarg_t *cbarg;
	unsigned int count;

	REQUIRE(arg != NULL);
	cbarg = *arg;

	/*
	 * The caller must hold the entry lock.
	 */

	rbtdb = (dns_rbtdb_t *)cbarg->db;
	rbtnode = (dns_rbtnode_t *)cbarg->node;

	nodelock = &rbtdb->node_locks[rbtnode->locknum].lock;
	NODE_LOCK(nodelock, isc_rwlocktype_write);

	switch (cbarg->type) {
	case dns_rdatasetadditional_fromauth:
		acarray = cbarg->header->additional_auth;
		break;
	case dns_rdatasetadditional_fromglue:
		acarray = cbarg->header->additional_glue;
		break;
	default:
		INSIST(0);
	}

	count = cbarg->count;
	if (acarray != NULL && acarray[count].entry == entry) {
		acarray[count].entry = NULL;
		INSIST(acarray[count].cbarg == cbarg);
		isc_mem_put(rbtdb->common.mctx, cbarg, sizeof(acache_cbarg_t));
		acarray[count].cbarg = NULL;
	} else
		isc_mem_put(rbtdb->common.mctx, cbarg, sizeof(acache_cbarg_t));

	dns_acache_detachentry(&entry);

	NODE_UNLOCK(nodelock, isc_rwlocktype_write);

	dns_db_detachnode((dns_db_t *)rbtdb, (dns_dbnode_t **)(void*)&rbtnode);
	dns_db_detach((dns_db_t **)(void*)&rbtdb);

	*arg = NULL;
}

static void
acache_cancelentry(isc_mem_t *mctx, dns_acacheentry_t *entry,
		      acache_cbarg_t **cbargp)
{
	acache_cbarg_t *cbarg;

	REQUIRE(mctx != NULL);
	REQUIRE(entry != NULL);
	REQUIRE(cbargp != NULL && *cbargp != NULL);

	cbarg = *cbargp;

	dns_acache_cancelentry(entry);
	dns_db_detachnode(cbarg->db, &cbarg->node);
	dns_db_detach(&cbarg->db);

	isc_mem_put(mctx, cbarg, sizeof(acache_cbarg_t));

	*cbargp = NULL;
}

static isc_result_t
rdataset_setadditional(dns_rdataset_t *rdataset, dns_rdatasetadditional_t type,
		       dns_rdatatype_t qtype, dns_acache_t *acache,
		       dns_zone_t *zone, dns_db_t *db,
		       dns_dbversion_t *version, dns_dbnode_t *node,
		       dns_name_t *fname)
{
	dns_rbtdb_t *rbtdb = rdataset->private1;
	dns_rbtnode_t *rbtnode = rdataset->private2;
	unsigned char *raw = rdataset->private3;        /* RDATASLAB */
	unsigned int current_count = rdataset->privateuint4;
	rdatasetheader_t *header;
	unsigned int total_count, count;
	nodelock_t *nodelock;
	isc_result_t result;
	acachectl_t *acarray;
	dns_acacheentry_t *newentry, *oldentry = NULL;
	acache_cbarg_t *newcbarg, *oldcbarg = NULL;

	UNUSED(qtype);

	if (type == dns_rdatasetadditional_fromcache)
		return (ISC_R_SUCCESS);

	header = (struct rdatasetheader *)(raw - sizeof(*header));

	total_count = raw[0] * 256 + raw[1];
	INSIST(total_count > current_count);
	count = total_count - current_count - 1; /* should be private data */

	newcbarg = isc_mem_get(rbtdb->common.mctx, sizeof(*newcbarg));
	if (newcbarg == NULL)
		return (ISC_R_NOMEMORY);
	newcbarg->type = type;
	newcbarg->count = count;
	newcbarg->header = header;
	newcbarg->db = NULL;
	dns_db_attach((dns_db_t *)rbtdb, &newcbarg->db);
	newcbarg->node = NULL;
	dns_db_attachnode((dns_db_t *)rbtdb, (dns_dbnode_t *)rbtnode,
			  &newcbarg->node);
	newentry = NULL;
	result = dns_acache_createentry(acache, (dns_db_t *)rbtdb,
					acache_callback, newcbarg, &newentry);
	if (result != ISC_R_SUCCESS)
		goto fail;
	/* Set cache data in the new entry. */
	result = dns_acache_setentry(acache, newentry, zone, db,
				     version, node, fname);
	if (result != ISC_R_SUCCESS)
		goto fail;

	nodelock = &rbtdb->node_locks[rbtnode->locknum].lock;
	NODE_LOCK(nodelock, isc_rwlocktype_write);

	acarray = NULL;
	switch (type) {
	case dns_rdatasetadditional_fromauth:
		acarray = header->additional_auth;
		break;
	case dns_rdatasetadditional_fromglue:
		acarray = header->additional_glue;
		break;
	default:
		INSIST(0);
	}

	if (acarray == NULL) {
		unsigned int i;

		acarray = isc_mem_get(rbtdb->common.mctx, total_count *
				      sizeof(acachectl_t));

		if (acarray == NULL) {
			NODE_UNLOCK(nodelock, isc_rwlocktype_write);
			goto fail;
		}

		for (i = 0; i < total_count; i++) {
			acarray[i].entry = NULL;
			acarray[i].cbarg = NULL;
		}
	}
	switch (type) {
	case dns_rdatasetadditional_fromauth:
		header->additional_auth = acarray;
		break;
	case dns_rdatasetadditional_fromglue:
		header->additional_glue = acarray;
		break;
	default:
		INSIST(0);
	}

	if (acarray[count].entry != NULL) {
		/*
		 * Swap the entry.  Delay cleaning-up the old entry since
		 * it would require a node lock.
		 */
		oldentry = acarray[count].entry;
		INSIST(acarray[count].cbarg != NULL);
		oldcbarg = acarray[count].cbarg;
	}
	acarray[count].entry = newentry;
	acarray[count].cbarg = newcbarg;

	NODE_UNLOCK(nodelock, isc_rwlocktype_write);

	if (oldentry != NULL) {
		acache_cancelentry(rbtdb->common.mctx, oldentry, &oldcbarg);
		dns_acache_detachentry(&oldentry);
	}

	return (ISC_R_SUCCESS);

 fail:
	if (newcbarg != NULL) {
		if (newentry != NULL) {
			acache_cancelentry(rbtdb->common.mctx, newentry,
					   &newcbarg);
			dns_acache_detachentry(&newentry);
		} else {
			dns_db_detachnode((dns_db_t *)rbtdb, &newcbarg->node);
			dns_db_detach(&newcbarg->db);
			isc_mem_put(rbtdb->common.mctx, newcbarg,
			    sizeof(*newcbarg));
		}
	}

	return (result);
}

static isc_result_t
rdataset_putadditional(dns_acache_t *acache, dns_rdataset_t *rdataset,
		       dns_rdatasetadditional_t type, dns_rdatatype_t qtype)
{
	dns_rbtdb_t *rbtdb = rdataset->private1;
	dns_rbtnode_t *rbtnode = rdataset->private2;
	unsigned char *raw = rdataset->private3;        /* RDATASLAB */
	unsigned int current_count = rdataset->privateuint4;
	rdatasetheader_t *header;
	nodelock_t *nodelock;
	unsigned int total_count, count;
	acachectl_t *acarray;
	dns_acacheentry_t *entry;
	acache_cbarg_t *cbarg;

	UNUSED(qtype);          /* we do not use this value at least for now */
	UNUSED(acache);

	if (type == dns_rdatasetadditional_fromcache)
		return (ISC_R_SUCCESS);

	header = (struct rdatasetheader *)(raw - sizeof(*header));

	total_count = raw[0] * 256 + raw[1];
	INSIST(total_count > current_count);
	count = total_count - current_count - 1;

	acarray = NULL;
	entry = NULL;

	nodelock = &rbtdb->node_locks[rbtnode->locknum].lock;
	NODE_LOCK(nodelock, isc_rwlocktype_write);

	switch (type) {
	case dns_rdatasetadditional_fromauth:
		acarray = header->additional_auth;
		break;
	case dns_rdatasetadditional_fromglue:
		acarray = header->additional_glue;
		break;
	default:
		INSIST(0);
	}

	if (acarray == NULL) {
		NODE_UNLOCK(nodelock, isc_rwlocktype_write);
		return (ISC_R_NOTFOUND);
	}

	entry = acarray[count].entry;
	if (entry == NULL) {
		NODE_UNLOCK(nodelock, isc_rwlocktype_write);
		return (ISC_R_NOTFOUND);
	}

	acarray[count].entry = NULL;
	cbarg = acarray[count].cbarg;
	acarray[count].cbarg = NULL;

	NODE_UNLOCK(nodelock, isc_rwlocktype_write);

	if (entry != NULL) {
		if (cbarg != NULL)
			acache_cancelentry(rbtdb->common.mctx, entry, &cbarg);
		dns_acache_detachentry(&entry);
	}

	return (ISC_R_SUCCESS);
}

/*%
 * Routines for LRU-based cache management.
 */

/*%
 * See if a given cache entry that is being reused needs to be updated
 * in the LRU-list.  From the LRU management point of view, this function is
 * expected to return true for almost all cases.  When used with threads,
 * however, this may cause a non-negligible performance penalty because a
 * writer lock will have to be acquired before updating the list.
 * If DNS_RBTDB_LIMITLRUUPDATE is defined to be non 0 at compilation time, this
 * function returns true if the entry has not been updated for some period of
 * time.  We differentiate the NS or glue address case and the others since
 * experiments have shown that the former tends to be accessed relatively
 * infrequently and the cost of cache miss is higher (e.g., a missing NS records
 * may cause external queries at a higher level zone, involving more
 * transactions).
 *
 * Caller must hold the node (read or write) lock.
 */
static inline isc_boolean_t
need_headerupdate(rdatasetheader_t *header, isc_stdtime_t now) {
	if ((header->attributes &
	     (RDATASET_ATTR_NONEXISTENT|RDATASET_ATTR_STALE)) != 0)
		return (ISC_FALSE);

#if DNS_RBTDB_LIMITLRUUPDATE
	if (header->type == dns_rdatatype_ns ||
	    (header->trust == dns_trust_glue &&
	     (header->type == dns_rdatatype_a ||
	      header->type == dns_rdatatype_aaaa))) {
		/*
		 * Glue records are updated if at least 60 seconds have passed
		 * since the previous update time.
		 */
		return (header->last_used + 60 <= now);
	}

	/* Other records are updated if 5 minutes have passed. */
	return (header->last_used + 300 <= now);
#else
	UNUSED(now);

	return (ISC_TRUE);
#endif
}

/*%
 * Update the timestamp of a given cache entry and move it to the head
 * of the corresponding LRU list.
 *
 * Caller must hold the node (write) lock.
 *
 * Note that the we do NOT touch the heap here, as the TTL has not changed.
 */
static void
update_header(dns_rbtdb_t *rbtdb, rdatasetheader_t *header,
	      isc_stdtime_t now)
{
	INSIST(IS_CACHE(rbtdb));

	/* To be checked: can we really assume this? XXXMLG */
	INSIST(ISC_LINK_LINKED(header, lru_link));

	ISC_LIST_UNLINK(rbtdb->rdatasets[header->node->locknum],
			header, lru_link);
	header->last_used = now;
	ISC_LIST_PREPEND(rbtdb->rdatasets[header->node->locknum],
			 header, lru_link);
}

/*%
 * Purge some expired and/or stale (i.e. unused for some period) cache entries
 * under an overmem condition.  To recover from this condition quickly, up to
 * 2 entries will be purged.  This process is triggered while adding a new
 * entry, and we specifically avoid purging entries in the same LRU bucket as
 * the one to which the new entry will belong.  Otherwise, we might purge
 * entries of the same name of different RR types while adding RRsets from a
 * single response (consider the case where we're adding A and AAAA glue records
 * of the same NS name).
 */
static void
overmem_purge(dns_rbtdb_t *rbtdb, unsigned int locknum_start,
	      isc_stdtime_t now, isc_boolean_t tree_locked)
{
	rdatasetheader_t *header, *header_prev;
	unsigned int locknum;
	int purgecount = 2;

	for (locknum = (locknum_start + 1) % rbtdb->node_lock_count;
	     locknum != locknum_start && purgecount > 0;
	     locknum = (locknum + 1) % rbtdb->node_lock_count) {
		NODE_LOCK(&rbtdb->node_locks[locknum].lock,
			  isc_rwlocktype_write);

		header = isc_heap_element(rbtdb->heaps[locknum], 1);
		if (header && header->rdh_ttl <= now - RBTDB_VIRTUAL) {
			expire_header(rbtdb, header, tree_locked);
			purgecount--;
		}

		for (header = ISC_LIST_TAIL(rbtdb->rdatasets[locknum]);
		     header != NULL && purgecount > 0;
		     header = header_prev) {
			header_prev = ISC_LIST_PREV(header, lru_link);
			/*
			 * Unlink the entry at this point to avoid checking it
			 * again even if it's currently used someone else and
			 * cannot be purged at this moment.  This entry won't be
			 * referenced any more (so unlinking is safe) since the
			 * TTL was reset to 0.
			 */
			ISC_LIST_UNLINK(rbtdb->rdatasets[locknum], header,
					lru_link);
			expire_header(rbtdb, header, tree_locked);
			purgecount--;
		}

		NODE_UNLOCK(&rbtdb->node_locks[locknum].lock,
				    isc_rwlocktype_write);
	}
}

static void
expire_header(dns_rbtdb_t *rbtdb, rdatasetheader_t *header,
	      isc_boolean_t tree_locked)
{
	set_ttl(rbtdb, header, 0);
	header->attributes |= RDATASET_ATTR_STALE;
	header->node->dirty = 1;

	/*
	 * Caller must hold the node (write) lock.
	 */

	if (dns_rbtnode_refcurrent(header->node) == 0) {
		/*
		 * If no one else is using the node, we can clean it up now.
		 * We first need to gain a new reference to the node to meet a
		 * requirement of decrement_reference().
		 */
		new_reference(rbtdb, header->node);
		decrement_reference(rbtdb, header->node, 0,
				    isc_rwlocktype_write,
				    tree_locked ? isc_rwlocktype_write :
				    isc_rwlocktype_none, ISC_FALSE);
	}
}
