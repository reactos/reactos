/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: acache.c,v 1.22 2008/02/07 23:46:54 tbox Exp $ */

#include <config.h>

#include <isc/atomic.h>
#include <isc/event.h>
#include <isc/hash.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>

#include <dns/acache.h>
#include <dns/db.h>
#include <dns/events.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdataset.h>
#include <dns/result.h>
#include <dns/zone.h>

#define ACACHE_MAGIC			ISC_MAGIC('A', 'C', 'H', 'E')
#define DNS_ACACHE_VALID(acache)	ISC_MAGIC_VALID(acache, ACACHE_MAGIC)

#define ACACHEENTRY_MAGIC		ISC_MAGIC('A', 'C', 'E', 'T')
#define DNS_ACACHEENTRY_VALID(entry)	ISC_MAGIC_VALID(entry, ACACHEENTRY_MAGIC)

#define DBBUCKETS	67

#if 0
#define ATRACE(m)       isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_DATABASE, \
				      DNS_LOGMODULE_ACACHE, \
				      ISC_LOG_DEBUG(3), \
				      "acache %p: %s", acache, (m))
#define AATRACE(a,m)    isc_log_write(dns_lctx, \
				      DNS_LOGCATEGORY_DATABASE, \
				      DNS_LOGMODULE_ACACHE, \
				      ISC_LOG_DEBUG(3), \
				      "acache %p: %s", (a), (m))
#else
#define ATRACE(m)
#define AATRACE(a, m)
#endif

/*
 * The following variables control incremental cleaning.
 * MINSIZE is how many bytes is the floor for dns_acache_setcachesize().
 * CLEANERINCREMENT is how many entries are examined in one pass.
 * (XXX simply derived from definitions in cache.c  There may be better
 *  constants here.)
 */
#define DNS_ACACHE_MINSIZE 		2097152	/* Bytes.  2097152 = 2 MB */
#define DNS_ACACHE_CLEANERINCREMENT	1000	/* Number of entries. */

#define DEFAULT_ACACHE_ENTRY_LOCK_COUNT	1009	/*%< Should be prime. */

#if defined(ISC_RWLOCK_USEATOMIC) && defined(ISC_PLATFORM_HAVEATOMICSTORE)
#define ACACHE_USE_RWLOCK 1
#endif

#ifdef ACACHE_USE_RWLOCK
#define ACACHE_INITLOCK(l)	isc_rwlock_init((l), 0, 0)
#define ACACHE_DESTROYLOCK(l)	isc_rwlock_destroy(l)
#define ACACHE_LOCK(l, t)	RWLOCK((l), (t))
#define ACACHE_UNLOCK(l, t)	RWUNLOCK((l), (t))

#define acache_storetime(entry, t) \
	(isc_atomic_store((isc_int32_t *)&(entry)->lastused, (t)))
#else
#define ACACHE_INITLOCK(l)	isc_mutex_init(l)
#define ACACHE_DESTROYLOCK(l)	DESTROYLOCK(l)
#define ACACHE_LOCK(l, t)	LOCK(l)
#define ACACHE_UNLOCK(l, t)	UNLOCK(l)

#define acache_storetime(entry, t) ((entry)->lastused = (t))
#endif

/* Locked by acache lock */
typedef struct dbentry {
	ISC_LINK(struct dbentry)	link;

	dns_db_t			*db;
	ISC_LIST(dns_acacheentry_t)	originlist;
	ISC_LIST(dns_acacheentry_t)	referlist;
} dbentry_t;

typedef ISC_LIST(dbentry_t) dbentrylist_t;

typedef struct acache_cleaner acache_cleaner_t;

typedef enum {
	cleaner_s_idle,	/* Waiting for cleaning-interval to expire. */
	cleaner_s_busy,	/* Currently cleaning. */
	cleaner_s_done  /* Freed enough memory after being overmem. */
} cleaner_state_t;

/*
 * Convenience macros for comprehensive assertion checking.
 */
#define CLEANER_IDLE(c) ((c)->state == cleaner_s_idle && \
			 (c)->resched_event != NULL)
#define CLEANER_BUSY(c) ((c)->state == cleaner_s_busy && \
			 (c)->resched_event == NULL)

struct acache_cleaner {
	isc_mutex_t		lock;
	/*
	 * Locks overmem_event, overmem.  (See cache.c)
	 */

	dns_acache_t		*acache;
	unsigned int		cleaning_interval; /* The cleaning-interval
						      from named.conf,
						      in seconds. */

	isc_stdtime_t		last_cleanup_time; /* The time when the last
						      cleanup task completed */

	isc_timer_t 		*cleaning_timer;
	isc_event_t		*resched_event;	/* Sent by cleaner task to
						   itself to reschedule */
	isc_event_t		*overmem_event;

	dns_acacheentry_t	*current_entry;	/* The bookmark entry to
						   restart the cleaning.
						   Locked by acache lock. */
	int 		 	increment;	/* Number of entries to
						   clean in one increment */

	unsigned long		ncleaned;	/* Number of entries cleaned
						   up (for logging purposes) */
	cleaner_state_t  	state;		/* Idle/Busy/Done. */
	isc_boolean_t	 	overmem;	/* The acache is in an overmem
						   state. */
};

struct dns_acachestats {
	unsigned int			hits;
	unsigned int			queries;
	unsigned int			misses;
	unsigned int			adds;
	unsigned int			deleted;
	unsigned int			cleaned;
	unsigned int			cleaner_runs;
	unsigned int			overmem;
	unsigned int			overmem_nocreates;
	unsigned int			nomem;
};

/*
 * The actual acache object.
 */

struct dns_acache {
	unsigned int			magic;

	isc_mem_t			*mctx;
	isc_refcount_t			refs;

#ifdef ACACHE_USE_RWLOCK
	isc_rwlock_t 			*entrylocks;
#else
	isc_mutex_t 			*entrylocks;
#endif

	isc_mutex_t			lock;

	int				live_cleaners;
	acache_cleaner_t		cleaner;
	ISC_LIST(dns_acacheentry_t)	entries;
	unsigned int			dbentries;
	dbentrylist_t			dbbucket[DBBUCKETS];

	isc_boolean_t			shutting_down;

	isc_task_t 			*task;
	isc_event_t			cevent;
	isc_boolean_t			cevent_sent;

	dns_acachestats_t		stats;
};

struct dns_acacheentry {
	unsigned int 		magic;

	unsigned int		locknum;
	isc_refcount_t 		references;

	dns_acache_t 		*acache;

	/* Data for Management of cache entries */
	ISC_LINK(dns_acacheentry_t) link;
	ISC_LINK(dns_acacheentry_t) olink;
	ISC_LINK(dns_acacheentry_t) rlink;

	dns_db_t 		*origdb; /* reference to the DB
					    holding this entry */

	/* Cache data */
	dns_zone_t 		*zone;		/* zone this entry
						   belongs to */
	dns_db_t		*db;   		/* DB this entry belongs to */
	dns_dbversion_t		*version;	/* the version of the DB */
	dns_dbnode_t 		*node;		/* node this entry
						   belongs to */
	dns_name_t 		*foundname;	/* corresponding DNS name
						   and rdataset */

	/* Callback function and its argument */
	void 			(*callback)(dns_acacheentry_t *, void **);
	void 			*cbarg;

	/* Timestamp of the last time this entry is referred to */
	isc_stdtime32_t		lastused;
};

/*
 *	Internal functions (and prototypes).
 */
static inline isc_boolean_t check_noentry(dns_acache_t *acache);
static void destroy(dns_acache_t *acache);
static void shutdown_entries(dns_acache_t *acache);
static void shutdown_buckets(dns_acache_t *acache);
static void destroy_entry(dns_acacheentry_t *ent);
static inline void unlink_dbentries(dns_acache_t *acache,
				    dns_acacheentry_t *ent);
static inline isc_result_t finddbent(dns_acache_t *acache,
				     dns_db_t *db, dbentry_t **dbentryp);
static inline void clear_entry(dns_acache_t *acache, dns_acacheentry_t *entry);
static isc_result_t acache_cleaner_init(dns_acache_t *acache,
					isc_timermgr_t *timermgr,
					acache_cleaner_t *cleaner);
static void acache_cleaning_timer_action(isc_task_t *task, isc_event_t *event);
static void acache_incremental_cleaning_action(isc_task_t *task,
					       isc_event_t *event);
static void acache_overmem_cleaning_action(isc_task_t *task,
					   isc_event_t *event);
static void acache_cleaner_shutdown_action(isc_task_t *task,
					   isc_event_t *event);

/*
 * acache should be locked.  If it is not, the stats can get out of whack,
 * which is not a big deal for us since this is for debugging / stats
 */
static void
reset_stats(dns_acache_t *acache) {
	acache->stats.hits = 0;
	acache->stats.queries = 0;
	acache->stats.misses = 0;
	acache->stats.adds = 0;
	acache->stats.deleted = 0;
	acache->stats.cleaned = 0;
	acache->stats.overmem = 0;
	acache->stats.overmem_nocreates = 0;
	acache->stats.nomem = 0;
}

/*
 * The acache must be locked before calling.
 */
static inline isc_boolean_t
check_noentry(dns_acache_t *acache) {
	if (ISC_LIST_EMPTY(acache->entries) && acache->dbentries == 0) {
		return (ISC_TRUE);
	}

	return (ISC_FALSE);
}

/*
 * The acache must be locked before calling.
 */
static void
shutdown_entries(dns_acache_t *acache) {
	dns_acacheentry_t *entry, *entry_next;

	REQUIRE(DNS_ACACHE_VALID(acache));
	INSIST(acache->shutting_down);

	/*
	 * Release the dependency of all entries, and detach them.
	 */
	for (entry = ISC_LIST_HEAD(acache->entries);
	     entry != NULL;
	     entry = entry_next) {
		entry_next = ISC_LIST_NEXT(entry, link);

		ACACHE_LOCK(&acache->entrylocks[entry->locknum],
			    isc_rwlocktype_write);

		/*
		 * If the cleaner holds this entry, it will be unlinked and
		 * freed in the cleaner later.
		 */
		if (acache->cleaner.current_entry != entry)
			ISC_LIST_UNLINK(acache->entries, entry, link);
		unlink_dbentries(acache, entry);
		if (entry->callback != NULL) {
			(entry->callback)(entry, &entry->cbarg);
			entry->callback = NULL;
		}

		ACACHE_UNLOCK(&acache->entrylocks[entry->locknum],
			      isc_rwlocktype_write);

		if (acache->cleaner.current_entry != entry)
			dns_acache_detachentry(&entry);
	}
}

/*
 * The acache must be locked before calling.
 */
static void
shutdown_buckets(dns_acache_t *acache) {
	int i;
	dbentry_t *dbent;

	REQUIRE(DNS_ACACHE_VALID(acache));
	INSIST(acache->shutting_down);

	for (i = 0; i < DBBUCKETS; i++) {
		while ((dbent = ISC_LIST_HEAD(acache->dbbucket[i])) != NULL) {
			INSIST(ISC_LIST_EMPTY(dbent->originlist) &&
			       ISC_LIST_EMPTY(dbent->referlist));
			ISC_LIST_UNLINK(acache->dbbucket[i], dbent, link);

			dns_db_detach(&dbent->db);

			isc_mem_put(acache->mctx, dbent, sizeof(*dbent));

			acache->dbentries--;
		}
	}

	INSIST(acache->dbentries == 0);
}

static void
shutdown_task(isc_task_t *task, isc_event_t *ev) {
	dns_acache_t *acache;

	UNUSED(task);

	acache = ev->ev_arg;
	INSIST(DNS_ACACHE_VALID(acache));

	isc_event_free(&ev);

	LOCK(&acache->lock);

	shutdown_entries(acache);
	shutdown_buckets(acache);

	UNLOCK(&acache->lock);

	dns_acache_detach(&acache);
}

/* The acache and the entry must be locked before calling. */
static inline void
unlink_dbentries(dns_acache_t *acache, dns_acacheentry_t *ent) {
	isc_result_t result;
	dbentry_t *dbent;

	if (ISC_LINK_LINKED(ent, olink)) {
		INSIST(ent->origdb != NULL);
		dbent = NULL;
		result = finddbent(acache, ent->origdb, &dbent);
		INSIST(result == ISC_R_SUCCESS);

		ISC_LIST_UNLINK(dbent->originlist, ent, olink);
	}
	if (ISC_LINK_LINKED(ent, rlink)) {
		INSIST(ent->db != NULL);
		dbent = NULL;
		result = finddbent(acache, ent->db, &dbent);
		INSIST(result == ISC_R_SUCCESS);

		ISC_LIST_UNLINK(dbent->referlist, ent, rlink);
	}
}

/* There must not be a reference to this entry. */
static void
destroy_entry(dns_acacheentry_t *entry) {
	dns_acache_t *acache;

	REQUIRE(DNS_ACACHEENTRY_VALID(entry));

	acache = entry->acache;
	REQUIRE(DNS_ACACHE_VALID(acache));

	/*
	 * Since there is no reference to this entry, it is safe to call
	 * clear_entry() here.
	 */
	clear_entry(acache, entry);

	isc_mem_put(acache->mctx, entry, sizeof(*entry));

	dns_acache_detach(&acache);
}

static void
destroy(dns_acache_t *acache) {
	int i;

	REQUIRE(DNS_ACACHE_VALID(acache));

	ATRACE("destroy");

	isc_mem_setwater(acache->mctx, NULL, NULL, 0, 0);

	if (acache->cleaner.overmem_event != NULL)
		isc_event_free(&acache->cleaner.overmem_event);

	if (acache->cleaner.resched_event != NULL)
		isc_event_free(&acache->cleaner.resched_event);

	if (acache->task != NULL)
		isc_task_detach(&acache->task);

	for (i = 0; i < DEFAULT_ACACHE_ENTRY_LOCK_COUNT; i++)
		ACACHE_DESTROYLOCK(&acache->entrylocks[i]);
	isc_mem_put(acache->mctx, acache->entrylocks,
		    sizeof(*acache->entrylocks) *
		    DEFAULT_ACACHE_ENTRY_LOCK_COUNT);

	DESTROYLOCK(&acache->cleaner.lock);

	DESTROYLOCK(&acache->lock);
	acache->magic = 0;

	isc_mem_putanddetach(&acache->mctx, acache, sizeof(*acache));
}

static inline isc_result_t
finddbent(dns_acache_t *acache, dns_db_t *db, dbentry_t **dbentryp) {
	int bucket;
	dbentry_t *dbentry;

	REQUIRE(DNS_ACACHE_VALID(acache));
	REQUIRE(db != NULL);
	REQUIRE(dbentryp != NULL && *dbentryp == NULL);

	/*
	 * The caller must be holding the acache lock.
	 */

	bucket = isc_hash_calc((const unsigned char *)&db,
			       sizeof(db), ISC_TRUE) % DBBUCKETS;

	for (dbentry = ISC_LIST_HEAD(acache->dbbucket[bucket]);
	     dbentry != NULL;
	     dbentry = ISC_LIST_NEXT(dbentry, link)) {
		if (dbentry->db == db)
			break;
	}

	*dbentryp = dbentry;

	if (dbentry == NULL)
		return (ISC_R_NOTFOUND);
	else
		return (ISC_R_SUCCESS);
}

static inline void
clear_entry(dns_acache_t *acache, dns_acacheentry_t *entry) {
	REQUIRE(DNS_ACACHE_VALID(acache));
	REQUIRE(DNS_ACACHEENTRY_VALID(entry));

	/*
	 * The caller must be holing the entry lock.
	 */

	if (entry->foundname) {
		dns_rdataset_t *rdataset, *rdataset_next;

		for (rdataset = ISC_LIST_HEAD(entry->foundname->list);
		     rdataset != NULL;
		     rdataset = rdataset_next) {
			rdataset_next = ISC_LIST_NEXT(rdataset, link);
			ISC_LIST_UNLINK(entry->foundname->list,
					rdataset, link);
			dns_rdataset_disassociate(rdataset);
			isc_mem_put(acache->mctx, rdataset, sizeof(*rdataset));
		}
		if (dns_name_dynamic(entry->foundname))
			dns_name_free(entry->foundname, acache->mctx);
		isc_mem_put(acache->mctx, entry->foundname,
			    sizeof(*entry->foundname));
		entry->foundname = NULL;
	}

	if (entry->node != NULL) {
		INSIST(entry->db != NULL);
		dns_db_detachnode(entry->db, &entry->node);
	}
	if (entry->version != NULL) {
		INSIST(entry->db != NULL);
		dns_db_closeversion(entry->db, &entry->version, ISC_FALSE);
	}
	if (entry->db != NULL)
		dns_db_detach(&entry->db);
	if (entry->zone != NULL)
		dns_zone_detach(&entry->zone);

	if (entry->origdb != NULL)
		dns_db_detach(&entry->origdb);
}

static isc_result_t
acache_cleaner_init(dns_acache_t *acache, isc_timermgr_t *timermgr,
		    acache_cleaner_t *cleaner)
{
	int result;

	ATRACE("acache cleaner init");

	result = isc_mutex_init(&cleaner->lock);
	if (result != ISC_R_SUCCESS)
		goto fail;

	cleaner->increment = DNS_ACACHE_CLEANERINCREMENT;
	cleaner->state = cleaner_s_idle;
	cleaner->acache = acache;
	cleaner->overmem = ISC_FALSE;

	cleaner->cleaning_timer = NULL;
	cleaner->resched_event = NULL;
	cleaner->overmem_event = NULL;
	cleaner->current_entry = NULL;

	if (timermgr != NULL) {
		cleaner->acache->live_cleaners++;

		result = isc_task_onshutdown(acache->task,
					     acache_cleaner_shutdown_action,
					     acache);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "acache cleaner: "
					 "isc_task_onshutdown() failed: %s",
					 dns_result_totext(result));
			goto cleanup;
		}

		cleaner->cleaning_interval = 0; /* Initially turned off. */
		isc_stdtime_get(&cleaner->last_cleanup_time);
		result = isc_timer_create(timermgr, isc_timertype_inactive,
					  NULL, NULL,
					  acache->task,
					  acache_cleaning_timer_action,
					  cleaner, &cleaner->cleaning_timer);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_timer_create() failed: %s",
					 dns_result_totext(result));
			result = ISC_R_UNEXPECTED;
			goto cleanup;
		}

		cleaner->resched_event =
			isc_event_allocate(acache->mctx, cleaner,
					   DNS_EVENT_ACACHECLEAN,
					   acache_incremental_cleaning_action,
					   cleaner, sizeof(isc_event_t));
		if (cleaner->resched_event == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		cleaner->overmem_event =
			isc_event_allocate(acache->mctx, cleaner,
					   DNS_EVENT_ACACHEOVERMEM,
					   acache_overmem_cleaning_action,
					   cleaner, sizeof(isc_event_t));
		if (cleaner->overmem_event == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
	}

	return (ISC_R_SUCCESS);

 cleanup:
	if (cleaner->overmem_event != NULL)
		isc_event_free(&cleaner->overmem_event);
	if (cleaner->resched_event != NULL)
		isc_event_free(&cleaner->resched_event);
	if (cleaner->cleaning_timer != NULL)
		isc_timer_detach(&cleaner->cleaning_timer);
	cleaner->acache->live_cleaners--;
	DESTROYLOCK(&cleaner->lock);
 fail:
	return (result);
}

static void
begin_cleaning(acache_cleaner_t *cleaner) {
	dns_acacheentry_t *head;
	dns_acache_t *acache = cleaner->acache;

	/*
	 * This function does not have to lock the cleaner, since critical
	 * parameters (except current_entry, which is locked by acache lock,)
	 * are only used in a single task context.
	 */

	REQUIRE(CLEANER_IDLE(cleaner));
	INSIST(DNS_ACACHE_VALID(acache));
	INSIST(cleaner->current_entry == NULL);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_ACACHE, ISC_LOG_DEBUG(1),
		      "begin acache cleaning, mem inuse %lu",
		      (unsigned long)isc_mem_inuse(cleaner->acache->mctx));

	LOCK(&acache->lock);

	head = ISC_LIST_HEAD(acache->entries);
	if (head != NULL)
		dns_acache_attachentry(head, &cleaner->current_entry);

	UNLOCK(&acache->lock);

	if (cleaner->current_entry != NULL) {
		cleaner->ncleaned = 0;
		cleaner->state = cleaner_s_busy;
		isc_task_send(acache->task, &cleaner->resched_event);
	}

	return;
}

static void
end_cleaning(acache_cleaner_t *cleaner, isc_event_t *event) {
	dns_acache_t *acache = cleaner->acache;

	REQUIRE(CLEANER_BUSY(cleaner));
	REQUIRE(event != NULL);
	REQUIRE(DNS_ACACHEENTRY_VALID(cleaner->current_entry));

	/* No need to lock the cleaner (see begin_cleaning()). */

	LOCK(&acache->lock);

	/*
	 * Even if the cleaner has the last reference to the entry, which means
	 * the entry has been unused, it may still be linked if unlinking the
	 * entry has been delayed due to the reference.
	 */
	if (isc_refcount_current(&cleaner->current_entry->references) == 1) {
		INSIST(cleaner->current_entry->callback == NULL);

		if (ISC_LINK_LINKED(cleaner->current_entry, link)) {
			ISC_LIST_UNLINK(acache->entries,
					cleaner->current_entry, link);
		}
	}
	dns_acache_detachentry(&cleaner->current_entry);

	if (cleaner->overmem)
		acache->stats.overmem++;
	acache->stats.cleaned += cleaner->ncleaned;
	acache->stats.cleaner_runs++;

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_ACACHE,
		      ISC_LOG_NOTICE,
		      "acache %p stats: hits=%d misses=%d queries=%d "
		      "adds=%d deleted=%d "
		      "cleaned=%d cleaner_runs=%d overmem=%d "
		      "overmem_nocreates=%d nomem=%d",
		      acache,
		      acache->stats.hits, acache->stats.misses,
		      acache->stats.queries,
		      acache->stats.adds, acache->stats.deleted,
		      acache->stats.cleaned, acache->stats.cleaner_runs,
		      acache->stats.overmem, acache->stats.overmem_nocreates,
		      acache->stats.nomem);
	reset_stats(acache);

	isc_stdtime_get(&cleaner->last_cleanup_time);

	UNLOCK(&acache->lock);

	dns_acache_setcleaninginterval(cleaner->acache,
				       cleaner->cleaning_interval);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_ACACHE,
		      ISC_LOG_DEBUG(1), "end acache cleaning, "
		      "%lu entries cleaned, mem inuse %lu",
		      cleaner->ncleaned,
		      (unsigned long)isc_mem_inuse(cleaner->acache->mctx));

	if (cleaner->overmem) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_ACACHE, ISC_LOG_NOTICE,
			      "acache is still in overmem state "
			      "after cleaning");
	}

	cleaner->ncleaned = 0;
	cleaner->state = cleaner_s_idle;
	cleaner->resched_event = event;
}

/*
 * This is run once for every acache-cleaning-interval as defined
 * in named.conf.
 */
static void
acache_cleaning_timer_action(isc_task_t *task, isc_event_t *event) {
	acache_cleaner_t *cleaner = event->ev_arg;

	UNUSED(task);

	INSIST(event->ev_type == ISC_TIMEREVENT_TICK);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_ACACHE,
		      ISC_LOG_DEBUG(1), "acache cleaning timer fired, "
		      "cleaner state = %d", cleaner->state);

	if (cleaner->state == cleaner_s_idle)
		begin_cleaning(cleaner);

	isc_event_free(&event);
}

/* The caller must hold entry lock. */
static inline isc_boolean_t
entry_stale(acache_cleaner_t *cleaner, dns_acacheentry_t *entry,
	    isc_stdtime32_t now32, unsigned int interval)
{
	/*
	 * If the callback has been canceled, we definitely do not need the
	 * entry.
	 */
	if (entry->callback == NULL)
		return (ISC_TRUE);

	if (interval > cleaner->cleaning_interval)
		interval = cleaner->cleaning_interval;

	if (entry->lastused + interval < now32)
		return (ISC_TRUE);

	/*
	 * If the acache is in the overmem state, probabilistically decide if
	 * the entry should be purged, based on the time passed from its last
	 * use and the cleaning interval.
	 */
	if (cleaner->overmem) {
		unsigned int passed =
			now32 - entry->lastused; /* <= interval */
		isc_uint32_t val;

		if (passed > interval / 2)
			return (ISC_TRUE);
		isc_random_get(&val);
		if (passed > interval / 4)
			return (ISC_TF(val % 4 == 0));
		return (ISC_TF(val % 8 == 0));
	}

	return (ISC_FALSE);
}

/*
 * Do incremental cleaning.
 */
static void
acache_incremental_cleaning_action(isc_task_t *task, isc_event_t *event) {
	acache_cleaner_t *cleaner = event->ev_arg;
	dns_acache_t *acache = cleaner->acache;
	dns_acacheentry_t *entry, *next = NULL;
	int n_entries;
	isc_stdtime32_t now32, last32;
	isc_stdtime_t now;
	unsigned int interval;

	INSIST(DNS_ACACHE_VALID(acache));
	INSIST(task == acache->task);
	INSIST(event->ev_type == DNS_EVENT_ACACHECLEAN);

	if (cleaner->state == cleaner_s_done) {
		cleaner->state = cleaner_s_busy;
		end_cleaning(cleaner, event);
		return;
	}

	INSIST(CLEANER_BUSY(cleaner));

	n_entries = cleaner->increment;

	isc_stdtime_get(&now);
	isc_stdtime_convert32(now, &now32);

	LOCK(&acache->lock);

	entry = cleaner->current_entry;
	isc_stdtime_convert32(cleaner->last_cleanup_time, &last32);
	INSIST(now32 > last32);
	interval = now32 - last32;

	while (n_entries-- > 0) {
		isc_boolean_t is_stale = ISC_FALSE;

		INSIST(entry != NULL);

		next = ISC_LIST_NEXT(entry, link);

		ACACHE_LOCK(&acache->entrylocks[entry->locknum],
			    isc_rwlocktype_write);

		is_stale = entry_stale(cleaner, entry, now32, interval);
		if (is_stale) {
			ISC_LIST_UNLINK(acache->entries, entry, link);
			unlink_dbentries(acache, entry);
			if (entry->callback != NULL)
				(entry->callback)(entry, &entry->cbarg);
			entry->callback = NULL;

			cleaner->ncleaned++;
		}

		ACACHE_UNLOCK(&acache->entrylocks[entry->locknum],
			      isc_rwlocktype_write);

		if (is_stale)
			dns_acache_detachentry(&entry);

		if (next == NULL) {
			if (cleaner->overmem) {
				entry = ISC_LIST_HEAD(acache->entries);
				if (entry != NULL) {
					/*
					 * If we are still in the overmem
					 * state, keep cleaning.
					 */
					isc_log_write(dns_lctx,
						      DNS_LOGCATEGORY_DATABASE,
						      DNS_LOGMODULE_ACACHE,
						      ISC_LOG_DEBUG(1),
						      "acache cleaner: "
						      "still overmem, "
						      "reset and try again");
					continue;
				}
			}

			UNLOCK(&acache->lock);
			end_cleaning(cleaner, event);
			return;
		}

		entry = next;
	}

	/*
	 * We have successfully performed a cleaning increment but have
	 * not gone through the entire cache.  Remember the entry that will
	 * be the starting point in the next clean-up, and reschedule another
	 * batch.  If it fails, just try to continue anyway.
	 */
	INSIST(next != NULL && next != cleaner->current_entry);
	dns_acache_detachentry(&cleaner->current_entry);
	dns_acache_attachentry(next, &cleaner->current_entry);

	UNLOCK(&acache->lock);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_ACACHE,
		      ISC_LOG_DEBUG(1), "acache cleaner: checked %d entries, "
		      "mem inuse %lu, sleeping", cleaner->increment,
		      (unsigned long)isc_mem_inuse(cleaner->acache->mctx));

	isc_task_send(task, &event);
	INSIST(CLEANER_BUSY(cleaner));

	return;
}

/*
 * This is called when the acache either surpasses its upper limit
 * or shrinks beyond its lower limit.
 */
static void
acache_overmem_cleaning_action(isc_task_t *task, isc_event_t *event) {
	acache_cleaner_t *cleaner = event->ev_arg;
	isc_boolean_t want_cleaning = ISC_FALSE;

	UNUSED(task);

	INSIST(event->ev_type == DNS_EVENT_ACACHEOVERMEM);
	INSIST(cleaner->overmem_event == NULL);

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_ACACHE,
		      ISC_LOG_DEBUG(1), "overmem_cleaning_action called, "
		      "overmem = %d, state = %d", cleaner->overmem,
		      cleaner->state);

	LOCK(&cleaner->lock);

	if (cleaner->overmem) {
		if (cleaner->state == cleaner_s_idle)
			want_cleaning = ISC_TRUE;
	} else {
		if (cleaner->state == cleaner_s_busy)
			/*
			 * end_cleaning() can't be called here because
			 * then both cleaner->overmem_event and
			 * cleaner->resched_event will point to this
			 * event.  Set the state to done, and then
			 * when the acache_incremental_cleaning_action() event
			 * is posted, it will handle the end_cleaning.
			 */
			cleaner->state = cleaner_s_done;
	}

	cleaner->overmem_event = event;

	UNLOCK(&cleaner->lock);

	if (want_cleaning)
		begin_cleaning(cleaner);
}

static void
water(void *arg, int mark) {
	dns_acache_t *acache = arg;
	isc_boolean_t overmem = ISC_TF(mark == ISC_MEM_HIWATER);

	REQUIRE(DNS_ACACHE_VALID(acache));

	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_ACACHE, ISC_LOG_DEBUG(1),
		      "acache memory reaches %s watermark, mem inuse %lu",
		      overmem ? "high" : "low",
		      (unsigned long)isc_mem_inuse(acache->mctx));

	LOCK(&acache->cleaner.lock);

	if (acache->cleaner.overmem != overmem) {
		acache->cleaner.overmem = overmem;

		if (acache->cleaner.overmem_event != NULL)
			isc_task_send(acache->task,
				      &acache->cleaner.overmem_event);
		isc_mem_waterack(acache->mctx, mark);
	}

	UNLOCK(&acache->cleaner.lock);
}

/*
 * The cleaner task is shutting down; do the necessary cleanup.
 */
static void
acache_cleaner_shutdown_action(isc_task_t *task, isc_event_t *event) {
	dns_acache_t *acache = event->ev_arg;
	isc_boolean_t should_free = ISC_FALSE;

	INSIST(task == acache->task);
	INSIST(event->ev_type == ISC_TASKEVENT_SHUTDOWN);
	INSIST(DNS_ACACHE_VALID(acache));

	ATRACE("acache cleaner shutdown");

	if (CLEANER_BUSY(&acache->cleaner))
		end_cleaning(&acache->cleaner, event);
	else
		isc_event_free(&event);

	LOCK(&acache->lock);

	acache->live_cleaners--;
	INSIST(acache->live_cleaners == 0);

	if (isc_refcount_current(&acache->refs) == 0) {
		INSIST(check_noentry(acache) == ISC_TRUE);
		should_free = ISC_TRUE;
	}

	/*
	 * By detaching the timer in the context of its task,
	 * we are guaranteed that there will be no further timer
	 * events.
	 */
	if (acache->cleaner.cleaning_timer != NULL)
		isc_timer_detach(&acache->cleaner.cleaning_timer);

	/* Make sure we don't reschedule anymore. */
	(void)isc_task_purge(task, NULL, DNS_EVENT_ACACHECLEAN, NULL);

	UNLOCK(&acache->lock);

	if (should_free)
		destroy(acache);
}

/*
 *	Public functions.
 */

isc_result_t
dns_acache_create(dns_acache_t **acachep, isc_mem_t *mctx,
		  isc_taskmgr_t *taskmgr, isc_timermgr_t *timermgr)
{
	int i;
	isc_result_t result;
	dns_acache_t *acache;

	REQUIRE(acachep != NULL && *acachep == NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(taskmgr != NULL);

	acache = isc_mem_get(mctx, sizeof(*acache));
	if (acache == NULL)
		return (ISC_R_NOMEMORY);

	ATRACE("create");

	result = isc_refcount_init(&acache->refs, 1);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, acache, sizeof(*acache));
		return (result);
	}

	result = isc_mutex_init(&acache->lock);
	if (result != ISC_R_SUCCESS) {
		isc_refcount_decrement(&acache->refs, NULL);
		isc_refcount_destroy(&acache->refs);
		isc_mem_put(mctx, acache, sizeof(*acache));
		return (result);
	}

	acache->mctx = NULL;
	isc_mem_attach(mctx, &acache->mctx);
	ISC_LIST_INIT(acache->entries);

	acache->shutting_down = ISC_FALSE;

	acache->task = NULL;
	acache->entrylocks = NULL;

	result = isc_task_create(taskmgr, 1, &acache->task);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_task_create() failed(): %s",
				 dns_result_totext(result));
		result = ISC_R_UNEXPECTED;
		goto cleanup;
	}
	isc_task_setname(acache->task, "acachetask", acache);
	ISC_EVENT_INIT(&acache->cevent, sizeof(acache->cevent), 0, NULL,
		       DNS_EVENT_ACACHECONTROL, shutdown_task, NULL,
		       NULL, NULL, NULL);
	acache->cevent_sent = ISC_FALSE;

	acache->dbentries = 0;
	for (i = 0; i < DBBUCKETS; i++)
		ISC_LIST_INIT(acache->dbbucket[i]);

	acache->entrylocks = isc_mem_get(mctx, sizeof(*acache->entrylocks) *
					 DEFAULT_ACACHE_ENTRY_LOCK_COUNT);
	if (acache->entrylocks == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	for (i = 0; i < DEFAULT_ACACHE_ENTRY_LOCK_COUNT; i++) {
		result = ACACHE_INITLOCK(&acache->entrylocks[i]);
		if (result != ISC_R_SUCCESS) {
			while (i-- > 0)
				ACACHE_DESTROYLOCK(&acache->entrylocks[i]);
			isc_mem_put(mctx, acache->entrylocks,
				    sizeof(*acache->entrylocks) *
				    DEFAULT_ACACHE_ENTRY_LOCK_COUNT);
			acache->entrylocks = NULL;
			goto cleanup;
		}
	}

	acache->live_cleaners = 0;
	result = acache_cleaner_init(acache, timermgr, &acache->cleaner);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	acache->stats.cleaner_runs = 0;
	reset_stats(acache);

	acache->magic = ACACHE_MAGIC;

	*acachep = acache;
	return (ISC_R_SUCCESS);

 cleanup:
	if (acache->task != NULL)
		isc_task_detach(&acache->task);
	DESTROYLOCK(&acache->lock);
	isc_refcount_decrement(&acache->refs, NULL);
	isc_refcount_destroy(&acache->refs);
	if (acache->entrylocks != NULL) {
		for (i = 0; i < DEFAULT_ACACHE_ENTRY_LOCK_COUNT; i++)
			ACACHE_DESTROYLOCK(&acache->entrylocks[i]);
		isc_mem_put(mctx, acache->entrylocks,
			    sizeof(*acache->entrylocks) *
			    DEFAULT_ACACHE_ENTRY_LOCK_COUNT);
	}
	isc_mem_put(mctx, acache, sizeof(*acache));
	isc_mem_detach(&mctx);

	return (result);
}

void
dns_acache_attach(dns_acache_t *source, dns_acache_t **targetp) {
	REQUIRE(DNS_ACACHE_VALID(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	AATRACE(source, "attach");

	isc_refcount_increment(&source->refs, NULL);

	*targetp = source;
}

void
dns_acache_countquerymiss(dns_acache_t *acache) {
	acache->stats.misses++; 	/* XXXSK danger: unlocked! */
	acache->stats.queries++;	/* XXXSK danger: unlocked! */
}

void
dns_acache_detach(dns_acache_t **acachep) {
	dns_acache_t *acache;
	unsigned int refs;
	isc_boolean_t should_free = ISC_FALSE;

	REQUIRE(acachep != NULL && DNS_ACACHE_VALID(*acachep));
	acache = *acachep;

	ATRACE("detach");

	isc_refcount_decrement(&acache->refs, &refs);
	if (refs == 0) {
		INSIST(check_noentry(acache) == ISC_TRUE);
		should_free = ISC_TRUE;
	}

	*acachep = NULL;

	/*
	 * If we're exiting and the cleaner task exists, let it free the cache.
	 */
	if (should_free && acache->live_cleaners > 0) {
		isc_task_shutdown(acache->task);
		should_free = ISC_FALSE;
	}

	if (should_free)
		destroy(acache);
}

void
dns_acache_shutdown(dns_acache_t *acache) {
	REQUIRE(DNS_ACACHE_VALID(acache));

	LOCK(&acache->lock);

	ATRACE("shutdown");

	if (!acache->shutting_down) {
		isc_event_t *event;
		dns_acache_t *acache_evarg = NULL;

		INSIST(!acache->cevent_sent);

		acache->shutting_down = ISC_TRUE;

		isc_mem_setwater(acache->mctx, NULL, NULL, 0, 0);

		/*
		 * Self attach the object in order to prevent it from being
		 * destroyed while waiting for the event.
		 */
		dns_acache_attach(acache, &acache_evarg);
		event = &acache->cevent;
		event->ev_arg = acache_evarg;
		isc_task_send(acache->task, &event);
		acache->cevent_sent = ISC_TRUE;
	}

	UNLOCK(&acache->lock);
}

isc_result_t
dns_acache_setdb(dns_acache_t *acache, dns_db_t *db) {
	int bucket;
	dbentry_t *dbentry;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(DNS_ACACHE_VALID(acache));
	REQUIRE(db != NULL);

	ATRACE("setdb");

	LOCK(&acache->lock);

	dbentry = NULL;
	result = finddbent(acache, db, &dbentry);
	if (result == ISC_R_SUCCESS) {
		result = ISC_R_EXISTS;
		goto end;
	}
	result = ISC_R_SUCCESS;

	dbentry = isc_mem_get(acache->mctx, sizeof(*dbentry));
	if (dbentry == NULL) {
		result = ISC_R_NOMEMORY;
		goto end;
	}

	ISC_LINK_INIT(dbentry, link);
	ISC_LIST_INIT(dbentry->originlist);
	ISC_LIST_INIT(dbentry->referlist);

	dbentry->db = NULL;
	dns_db_attach(db, &dbentry->db);

	bucket = isc_hash_calc((const unsigned char *)&db,
			       sizeof(db), ISC_TRUE) % DBBUCKETS;

	ISC_LIST_APPEND(acache->dbbucket[bucket], dbentry, link);

	acache->dbentries++;

 end:
	UNLOCK(&acache->lock);

	return (result);
}

isc_result_t
dns_acache_putdb(dns_acache_t *acache, dns_db_t *db) {
	int bucket;
	isc_result_t result;
	dbentry_t *dbentry;
	dns_acacheentry_t *entry;

	REQUIRE(DNS_ACACHE_VALID(acache));
	REQUIRE(db != NULL);

	ATRACE("putdb");

	LOCK(&acache->lock);

	dbentry = NULL;
	result = finddbent(acache, db, &dbentry);
	if (result != ISC_R_SUCCESS) {
		/*
		 * The entry may have not been created due to memory shortage.
		 */
		UNLOCK(&acache->lock);
		return (ISC_R_NOTFOUND);
	}

	/*
	 * Release corresponding cache entries: for each entry, release all
	 * links the entry has, and then callback to the entry holder (if any).
	 * If no other external references exist (this can happen if the
	 * original holder has canceled callback,) destroy it here.
	 */
	while ((entry = ISC_LIST_HEAD(dbentry->originlist)) != NULL) {
		ACACHE_LOCK(&acache->entrylocks[entry->locknum],
			    isc_rwlocktype_write);

		/*
		 * Releasing olink first would avoid finddbent() in
		 * unlink_dbentries().
		 */
		ISC_LIST_UNLINK(dbentry->originlist, entry, olink);
		if (acache->cleaner.current_entry != entry)
			ISC_LIST_UNLINK(acache->entries, entry, link);
		unlink_dbentries(acache, entry);

		if (entry->callback != NULL)
			(entry->callback)(entry, &entry->cbarg);
		entry->callback = NULL;

		ACACHE_UNLOCK(&acache->entrylocks[entry->locknum],
			      isc_rwlocktype_write);

		if (acache->cleaner.current_entry != entry)
			dns_acache_detachentry(&entry);
	}
	while ((entry = ISC_LIST_HEAD(dbentry->referlist)) != NULL) {
		ACACHE_LOCK(&acache->entrylocks[entry->locknum],
			    isc_rwlocktype_write);

		ISC_LIST_UNLINK(dbentry->referlist, entry, rlink);
		if (acache->cleaner.current_entry != entry)
			ISC_LIST_UNLINK(acache->entries, entry, link);
		unlink_dbentries(acache, entry);

		if (entry->callback != NULL)
			(entry->callback)(entry, &entry->cbarg);
		entry->callback = NULL;

		ACACHE_UNLOCK(&acache->entrylocks[entry->locknum],
			      isc_rwlocktype_write);

		if (acache->cleaner.current_entry != entry)
			dns_acache_detachentry(&entry);
	}

	INSIST(ISC_LIST_EMPTY(dbentry->originlist) &&
	       ISC_LIST_EMPTY(dbentry->referlist));

	bucket = isc_hash_calc((const unsigned char *)&db,
			       sizeof(db), ISC_TRUE) % DBBUCKETS;
	ISC_LIST_UNLINK(acache->dbbucket[bucket], dbentry, link);
	dns_db_detach(&dbentry->db);

	isc_mem_put(acache->mctx, dbentry, sizeof(*dbentry));

	acache->dbentries--;

	acache->stats.deleted++;

	UNLOCK(&acache->lock);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_acache_createentry(dns_acache_t *acache, dns_db_t *origdb,
		       void (*callback)(dns_acacheentry_t *, void **),
		       void *cbarg, dns_acacheentry_t **entryp)
{
	dns_acacheentry_t *newentry;
	isc_result_t result;
	isc_uint32_t r;

	REQUIRE(DNS_ACACHE_VALID(acache));
	REQUIRE(entryp != NULL && *entryp == NULL);
	REQUIRE(origdb != NULL);

	/*
	 * Should we exceed our memory limit for some reason (for
	 * example, if the cleaner does not run aggressively enough),
	 * then we will not create additional entries.
	 *
	 * XXXSK: It might be better to lock the acache->cleaner->lock,
	 * but locking may be an expensive bottleneck. If we misread
	 * the value, we will occasionally refuse to create a few
	 * cache entries, or create a few that we should not. I do not
	 * expect this to happen often, and it will not have very bad
	 * effects when it does. So no lock for now.
	 */
	if (acache->cleaner.overmem) {
		acache->stats.overmem_nocreates++; /* XXXSK danger: unlocked! */
		return (ISC_R_NORESOURCES);
	}

	newentry = isc_mem_get(acache->mctx, sizeof(*newentry));
	if (newentry == NULL) {
		acache->stats.nomem++;  /* XXXMLG danger: unlocked! */
		return (ISC_R_NOMEMORY);
	}

	isc_random_get(&r);
	newentry->locknum = r % DEFAULT_ACACHE_ENTRY_LOCK_COUNT;

	result = isc_refcount_init(&newentry->references, 1);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(acache->mctx, newentry, sizeof(*newentry));
		return (result);
	};

	ISC_LINK_INIT(newentry, link);
	ISC_LINK_INIT(newentry, olink);
	ISC_LINK_INIT(newentry, rlink);

	newentry->acache = NULL;
	dns_acache_attach(acache, &newentry->acache);

	newentry->zone = NULL;
	newentry->db = NULL;
	newentry->version = NULL;
	newentry->node = NULL;
	newentry->foundname = NULL;

	newentry->callback = callback;
	newentry->cbarg = cbarg;
	newentry->origdb = NULL;
	dns_db_attach(origdb, &newentry->origdb);

	isc_stdtime_get(&newentry->lastused);

	newentry->magic = ACACHEENTRY_MAGIC;

	*entryp = newentry;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_acache_getentry(dns_acacheentry_t *entry, dns_zone_t **zonep,
		    dns_db_t **dbp, dns_dbversion_t **versionp,
		    dns_dbnode_t **nodep, dns_name_t *fname,
		    dns_message_t *msg, isc_stdtime_t now)
{
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdataset_t *erdataset;
	isc_stdtime32_t	now32;
	dns_acache_t *acache;
	int locknum;

	REQUIRE(DNS_ACACHEENTRY_VALID(entry));
	REQUIRE(zonep == NULL || *zonep == NULL);
	REQUIRE(dbp != NULL && *dbp == NULL);
	REQUIRE(versionp != NULL && *versionp == NULL);
	REQUIRE(nodep != NULL && *nodep == NULL);
	REQUIRE(fname != NULL);
	REQUIRE(msg != NULL);
	acache = entry->acache;
	REQUIRE(DNS_ACACHE_VALID(acache));

	locknum = entry->locknum;
	ACACHE_LOCK(&acache->entrylocks[locknum], isc_rwlocktype_read);

	isc_stdtime_convert32(now, &now32);
	acache_storetime(entry, now32);

	if (entry->zone != NULL && zonep != NULL)
		dns_zone_attach(entry->zone, zonep);

	if (entry->db == NULL) {
		*dbp = NULL;
		*versionp = NULL;
	} else {
		dns_db_attach(entry->db, dbp);
		dns_db_attachversion(entry->db, entry->version, versionp);
	}
	if (entry->node == NULL)
		*nodep = NULL;
	else {
		dns_db_attachnode(entry->db, entry->node, nodep);

		INSIST(entry->foundname != NULL);
		dns_name_copy(entry->foundname, fname, NULL);
		for (erdataset = ISC_LIST_HEAD(entry->foundname->list);
		     erdataset != NULL;
		     erdataset = ISC_LIST_NEXT(erdataset, link)) {
			dns_rdataset_t *ardataset;

			ardataset = NULL;
			result = dns_message_gettemprdataset(msg, &ardataset);
			if (result != ISC_R_SUCCESS) {
				ACACHE_UNLOCK(&acache->entrylocks[locknum],
					      isc_rwlocktype_read);
				goto fail;
			}

			/*
			 * XXXJT: if we simply clone the rdataset, we'll get
			 * lost wrt cyclic ordering.  We'll need an additional
			 * trick to get the latest counter from the original
			 * header.
			 */
			dns_rdataset_init(ardataset);
			dns_rdataset_clone(erdataset, ardataset);
			ISC_LIST_APPEND(fname->list, ardataset, link);
		}
	}

	entry->acache->stats.hits++; /* XXXMLG danger: unlocked! */
	entry->acache->stats.queries++;

	ACACHE_UNLOCK(&acache->entrylocks[locknum], isc_rwlocktype_read);

	return (result);

  fail:
	while ((erdataset = ISC_LIST_HEAD(fname->list)) != NULL) {
		ISC_LIST_UNLINK(fname->list, erdataset, link);
		dns_rdataset_disassociate(erdataset);
		dns_message_puttemprdataset(msg, &erdataset);
	}
	if (*nodep != NULL)
		dns_db_detachnode(*dbp, nodep);
	if (*versionp != NULL)
		dns_db_closeversion(*dbp, versionp, ISC_FALSE);
	if (*dbp != NULL)
		dns_db_detach(dbp);
	if (zonep != NULL && *zonep != NULL)
		dns_zone_detach(zonep);

	return (result);
}

isc_result_t
dns_acache_setentry(dns_acache_t *acache, dns_acacheentry_t *entry,
		    dns_zone_t *zone, dns_db_t *db, dns_dbversion_t *version,
		    dns_dbnode_t *node, dns_name_t *fname)
{
	isc_result_t result;
	dbentry_t *odbent;
	dbentry_t *rdbent = NULL;
	isc_boolean_t close_version = ISC_FALSE;
	dns_acacheentry_t *dummy_entry = NULL;

	REQUIRE(DNS_ACACHE_VALID(acache));
	REQUIRE(DNS_ACACHEENTRY_VALID(entry));

	LOCK(&acache->lock);	/* XXX: need to lock it here for ordering */
	ACACHE_LOCK(&acache->entrylocks[entry->locknum], isc_rwlocktype_write);

	/* Set zone */
	if (zone != NULL)
		dns_zone_attach(zone, &entry->zone);
	/* Set DB */
	if (db != NULL)
		dns_db_attach(db, &entry->db);
	/*
	 * Set DB version.  If the version is not given by the caller,
	 * which is the case for glue or cache DBs, use the current version.
	 */
	if (version == NULL) {
		if (db != NULL) {
			dns_db_currentversion(db, &version);
			close_version = ISC_TRUE;
		}
	}
	if (version != NULL) {
		INSIST(db != NULL);
		dns_db_attachversion(db, version, &entry->version);
	}
	if (close_version)
		dns_db_closeversion(db, &version, ISC_FALSE);
	/* Set DB node. */
	if (node != NULL) {
		INSIST(db != NULL);
		dns_db_attachnode(db, node, &entry->node);
	}

	/*
	 * Set list of the corresponding rdatasets, if given.
	 * To minimize the overhead and memory consumption, we'll do this for
	 * positive cache only, in which case the DB node is non NULL.
	 * We do not want to cache incomplete information, so give up the
	 * entire entry when a memory shortage happen during the process.
	 */
	if (node != NULL) {
		dns_rdataset_t *ardataset, *crdataset;

		entry->foundname = isc_mem_get(acache->mctx,
					       sizeof(*entry->foundname));

		if (entry->foundname == NULL) {
			result = ISC_R_NOMEMORY;
			goto fail;
		}
		dns_name_init(entry->foundname, NULL);
		result = dns_name_dup(fname, acache->mctx,
				      entry->foundname);
		if (result != ISC_R_SUCCESS)
			goto fail;

		for (ardataset = ISC_LIST_HEAD(fname->list);
		     ardataset != NULL;
		     ardataset = ISC_LIST_NEXT(ardataset, link)) {
			crdataset = isc_mem_get(acache->mctx,
						sizeof(*crdataset));
			if (crdataset == NULL) {
				result = ISC_R_NOMEMORY;
				goto fail;
			}

			dns_rdataset_init(crdataset);
			dns_rdataset_clone(ardataset, crdataset);
			ISC_LIST_APPEND(entry->foundname->list, crdataset,
					link);
		}
	}

	odbent = NULL;
	result = finddbent(acache, entry->origdb, &odbent);
	if (result != ISC_R_SUCCESS)
		goto fail;
	if (db != NULL) {
		rdbent = NULL;
		result = finddbent(acache, db, &rdbent);
		if (result != ISC_R_SUCCESS)
			goto fail;
	}

	ISC_LIST_APPEND(acache->entries, entry, link);
	ISC_LIST_APPEND(odbent->originlist, entry, olink);
	if (rdbent != NULL)
		ISC_LIST_APPEND(rdbent->referlist, entry, rlink);

	/*
	 * The additional cache needs an implicit reference to entries in its
	 * link.
	 */
	dns_acache_attachentry(entry, &dummy_entry);

	ACACHE_UNLOCK(&acache->entrylocks[entry->locknum],
		      isc_rwlocktype_write);

	acache->stats.adds++;
	UNLOCK(&acache->lock);

	return (ISC_R_SUCCESS);

 fail:
	clear_entry(acache, entry);

	ACACHE_UNLOCK(&acache->entrylocks[entry->locknum],
		      isc_rwlocktype_write);
	UNLOCK(&acache->lock);

	return (result);
}

void
dns_acache_cancelentry(dns_acacheentry_t *entry) {
	dns_acache_t *acache = entry->acache;

	REQUIRE(DNS_ACACHEENTRY_VALID(entry));
	INSIST(DNS_ACACHE_VALID(acache));

	LOCK(&acache->lock);
	ACACHE_LOCK(&acache->entrylocks[entry->locknum], isc_rwlocktype_write);

	/*
	 * Release dependencies stored in this entry as much as possible.
	 * The main link cannot be released, since the acache object has
	 * a reference to this entry; the empty entry will be released in
	 * the next cleaning action.
	 */
	unlink_dbentries(acache, entry);
	clear_entry(entry->acache, entry);

	entry->callback = NULL;
	entry->cbarg = NULL;

	ACACHE_UNLOCK(&acache->entrylocks[entry->locknum],
		      isc_rwlocktype_write);
	UNLOCK(&acache->lock);
}

void
dns_acache_attachentry(dns_acacheentry_t *source,
		       dns_acacheentry_t **targetp)
{
	REQUIRE(DNS_ACACHEENTRY_VALID(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references, NULL);

	*targetp = source;
}

void
dns_acache_detachentry(dns_acacheentry_t **entryp) {
	dns_acacheentry_t *entry;
	unsigned int refs;

	REQUIRE(entryp != NULL && DNS_ACACHEENTRY_VALID(*entryp));
	entry = *entryp;

	isc_refcount_decrement(&entry->references, &refs);

	/*
	 * If there are no references to the entry, the entry must have been
	 * unlinked and can be destroyed safely.
	 */
	if (refs == 0) {
		INSIST(!ISC_LINK_LINKED(entry, link));
		(*entryp)->acache->stats.deleted++;
		destroy_entry(entry);
	}

	*entryp = NULL;
}

void
dns_acache_setcleaninginterval(dns_acache_t *acache, unsigned int t) {
	isc_interval_t interval;
	isc_result_t result;

	REQUIRE(DNS_ACACHE_VALID(acache));

	ATRACE("dns_acache_setcleaninginterval");

	LOCK(&acache->lock);

	/*
	 * It may be the case that the acache has already shut down.
	 * If so, it has no timer.  (Not sure if this can really happen.)
	 */
	if (acache->cleaner.cleaning_timer == NULL)
		goto unlock;

	acache->cleaner.cleaning_interval = t;

	if (t == 0) {
		result = isc_timer_reset(acache->cleaner.cleaning_timer,
					 isc_timertype_inactive,
					 NULL, NULL, ISC_TRUE);
	} else {
		isc_interval_set(&interval, acache->cleaner.cleaning_interval,
				 0);
		result = isc_timer_reset(acache->cleaner.cleaning_timer,
					 isc_timertype_ticker,
					 NULL, &interval, ISC_FALSE);
	}
	if (result != ISC_R_SUCCESS)
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_ACACHE, ISC_LOG_WARNING,
			      "could not set acache cleaning interval: %s",
			      isc_result_totext(result));
	else
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_ACACHE, ISC_LOG_NOTICE,
			      "acache %p cleaning interval set to %d.",
			      acache, t);

 unlock:
	UNLOCK(&acache->lock);
}

/*
 * This function was derived from cache.c:dns_cache_setcachesize().  See the
 * function for more details about the logic.
 */
void
dns_acache_setcachesize(dns_acache_t *acache, isc_uint32_t size) {
	isc_uint32_t lowater;
	isc_uint32_t hiwater;

	REQUIRE(DNS_ACACHE_VALID(acache));

	if (size != 0 && size < DNS_ACACHE_MINSIZE)
		size = DNS_ACACHE_MINSIZE;

	hiwater = size - (size >> 3);
	lowater = size - (size >> 2);

	if (size == 0 || hiwater == 0 || lowater == 0)
		isc_mem_setwater(acache->mctx, water, acache, 0, 0);
	else
		isc_mem_setwater(acache->mctx, water, acache,
				 hiwater, lowater);
}
