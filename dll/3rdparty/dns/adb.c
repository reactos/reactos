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

/* $Id: adb.c,v 1.243.42.4 2009/02/03 22:34:28 jinmei Exp $ */

/*! \file
 *
 * \note
 * In finds, if task == NULL, no events will be generated, and no events
 * have been sent.  If task != NULL but taskaction == NULL, an event has been
 * posted but not yet freed.  If neither are NULL, no event was posted.
 *
 */

#include <config.h>

#include <limits.h>

#include <isc/mutexblock.h>
#include <isc/netaddr.h>
#include <isc/random.h>
#include <isc/stats.h>
#include <isc/string.h>         /* Required for HP/UX (and others?) */
#include <isc/task.h>
#include <isc/util.h>

#include <dns/adb.h>
#include <dns/db.h>
#include <dns/events.h>
#include <dns/log.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/stats.h>

#define DNS_ADB_MAGIC             ISC_MAGIC('D', 'a', 'd', 'b')
#define DNS_ADB_VALID(x)          ISC_MAGIC_VALID(x, DNS_ADB_MAGIC)
#define DNS_ADBNAME_MAGIC         ISC_MAGIC('a', 'd', 'b', 'N')
#define DNS_ADBNAME_VALID(x)      ISC_MAGIC_VALID(x, DNS_ADBNAME_MAGIC)
#define DNS_ADBNAMEHOOK_MAGIC     ISC_MAGIC('a', 'd', 'N', 'H')
#define DNS_ADBNAMEHOOK_VALID(x)  ISC_MAGIC_VALID(x, DNS_ADBNAMEHOOK_MAGIC)
#define DNS_ADBLAMEINFO_MAGIC     ISC_MAGIC('a', 'd', 'b', 'Z')
#define DNS_ADBLAMEINFO_VALID(x)  ISC_MAGIC_VALID(x, DNS_ADBLAMEINFO_MAGIC)
#define DNS_ADBENTRY_MAGIC        ISC_MAGIC('a', 'd', 'b', 'E')
#define DNS_ADBENTRY_VALID(x)     ISC_MAGIC_VALID(x, DNS_ADBENTRY_MAGIC)
#define DNS_ADBFETCH_MAGIC        ISC_MAGIC('a', 'd', 'F', '4')
#define DNS_ADBFETCH_VALID(x)     ISC_MAGIC_VALID(x, DNS_ADBFETCH_MAGIC)
#define DNS_ADBFETCH6_MAGIC       ISC_MAGIC('a', 'd', 'F', '6')
#define DNS_ADBFETCH6_VALID(x)    ISC_MAGIC_VALID(x, DNS_ADBFETCH6_MAGIC)

/*!
 * The number of buckets needs to be a prime (for good hashing).
 *
 * XXXRTH  How many buckets do we need?
 */
#define NBUCKETS               1009     /*%< how many buckets for names/addrs */

/*!
 * For type 3 negative cache entries, we will remember that the address is
 * broken for this long.  XXXMLG This is also used for actual addresses, too.
 * The intent is to keep us from constantly asking about A/AAAA records
 * if the zone has extremely low TTLs.
 */
#define ADB_CACHE_MINIMUM       10      /*%< seconds */
#define ADB_CACHE_MAXIMUM       86400   /*%< seconds (86400 = 24 hours) */
#define ADB_ENTRY_WINDOW        1800    /*%< seconds */

/*%
 * The period in seconds after which an ADB name entry is regarded as stale
 * and forced to be cleaned up.
 * TODO: This should probably be configurable at run-time.
 */
#ifndef ADB_STALE_MARGIN
#define ADB_STALE_MARGIN        1800
#endif

#define FREE_ITEMS              64      /*%< free count for memory pools */
#define FILL_COUNT              16      /*%< fill count for memory pools */

#define DNS_ADB_INVALIDBUCKET (-1)      /*%< invalid bucket address */

#define DNS_ADB_MINADBSIZE      (1024*1024)     /*%< 1 Megabyte */

typedef ISC_LIST(dns_adbname_t) dns_adbnamelist_t;
typedef struct dns_adbnamehook dns_adbnamehook_t;
typedef ISC_LIST(dns_adbnamehook_t) dns_adbnamehooklist_t;
typedef struct dns_adblameinfo dns_adblameinfo_t;
typedef ISC_LIST(dns_adbentry_t) dns_adbentrylist_t;
typedef struct dns_adbfetch dns_adbfetch_t;
typedef struct dns_adbfetch6 dns_adbfetch6_t;

/*% dns adb structure */
struct dns_adb {
	unsigned int                    magic;

	isc_mutex_t                     lock;
	isc_mutex_t                     reflock; /*%< Covers irefcnt, erefcnt */
	isc_mutex_t                     overmemlock; /*%< Covers overmem */
	isc_mem_t                      *mctx;
	dns_view_t                     *view;

	isc_taskmgr_t                  *taskmgr;
	isc_task_t                     *task;
	isc_boolean_t                   overmem;

	isc_interval_t                  tick_interval;
	int                             next_cleanbucket;

	unsigned int                    irefcnt;
	unsigned int                    erefcnt;

	isc_mutex_t                     mplock;
	isc_mempool_t                  *nmp;    /*%< dns_adbname_t */
	isc_mempool_t                  *nhmp;   /*%< dns_adbnamehook_t */
	isc_mempool_t                  *limp;   /*%< dns_adblameinfo_t */
	isc_mempool_t                  *emp;    /*%< dns_adbentry_t */
	isc_mempool_t                  *ahmp;   /*%< dns_adbfind_t */
	isc_mempool_t                  *aimp;   /*%< dns_adbaddrinfo_t */
	isc_mempool_t                  *afmp;   /*%< dns_adbfetch_t */

	/*!
	 * Bucketized locks and lists for names.
	 *
	 * XXXRTH  Have a per-bucket structure that contains all of these?
	 */
	dns_adbnamelist_t               names[NBUCKETS];
	dns_adbnamelist_t               deadnames[NBUCKETS];
	/*% See dns_adbnamelist_t */
	isc_mutex_t                     namelocks[NBUCKETS];
	/*% See dns_adbnamelist_t */
	isc_boolean_t                   name_sd[NBUCKETS];
	/*% See dns_adbnamelist_t */
	unsigned int                    name_refcnt[NBUCKETS];

	/*!
	 * Bucketized locks for entries.
	 *
	 * XXXRTH  Have a per-bucket structure that contains all of these?
	 */
	dns_adbentrylist_t              entries[NBUCKETS];
	dns_adbentrylist_t              deadentries[NBUCKETS];
	isc_mutex_t                     entrylocks[NBUCKETS];
	isc_boolean_t                   entry_sd[NBUCKETS]; /*%< shutting down */
	unsigned int                    entry_refcnt[NBUCKETS];

	isc_event_t                     cevent;
	isc_boolean_t                   cevent_sent;
	isc_boolean_t                   shutting_down;
	isc_eventlist_t                 whenshutdown;
};

/*
 * XXXMLG  Document these structures.
 */

/*% dns_adbname structure */
struct dns_adbname {
	unsigned int                    magic;
	dns_name_t                      name;
	dns_adb_t                      *adb;
	unsigned int                    partial_result;
	unsigned int                    flags;
	int                             lock_bucket;
	dns_name_t                      target;
	isc_stdtime_t                   expire_target;
	isc_stdtime_t                   expire_v4;
	isc_stdtime_t                   expire_v6;
	unsigned int                    chains;
	dns_adbnamehooklist_t           v4;
	dns_adbnamehooklist_t           v6;
	dns_adbfetch_t                 *fetch_a;
	dns_adbfetch_t                 *fetch_aaaa;
	unsigned int                    fetch_err;
	unsigned int                    fetch6_err;
	dns_adbfindlist_t               finds;
	/* for LRU-based management */
	isc_stdtime_t                   last_used;

	ISC_LINK(dns_adbname_t)         plink;
};

/*% The adbfetch structure */
struct dns_adbfetch {
	unsigned int                    magic;
	dns_fetch_t                    *fetch;
	dns_rdataset_t                  rdataset;
};

/*%
 * This is a small widget that dangles off a dns_adbname_t.  It contains a
 * pointer to the address information about this host, and a link to the next
 * namehook that will contain the next address this host has.
 */
struct dns_adbnamehook {
	unsigned int                    magic;
	dns_adbentry_t                 *entry;
	ISC_LINK(dns_adbnamehook_t)     plink;
};

/*%
 * This is a small widget that holds qname-specific information about an
 * address.  Currently limited to lameness, but could just as easily be
 * extended to other types of information about zones.
 */
struct dns_adblameinfo {
	unsigned int                    magic;

	dns_name_t                      qname;
	dns_rdatatype_t                 qtype;
	isc_stdtime_t                   lame_timer;

	ISC_LINK(dns_adblameinfo_t)     plink;
};

/*%
 * An address entry.  It holds quite a bit of information about addresses,
 * including edns state (in "flags"), rtt, and of course the address of
 * the host.
 */
struct dns_adbentry {
	unsigned int                    magic;

	int                             lock_bucket;
	unsigned int                    refcnt;

	unsigned int                    flags;
	unsigned int                    srtt;
	isc_sockaddr_t                  sockaddr;

	isc_stdtime_t                   expires;
	/*%<
	 * A nonzero 'expires' field indicates that the entry should
	 * persist until that time.  This allows entries found
	 * using dns_adb_findaddrinfo() to persist for a limited time
	 * even though they are not necessarily associated with a
	 * name.
	 */

	ISC_LIST(dns_adblameinfo_t)     lameinfo;
	ISC_LINK(dns_adbentry_t)        plink;
};

/*
 * Internal functions (and prototypes).
 */
static inline dns_adbname_t *new_adbname(dns_adb_t *, dns_name_t *);
static inline void free_adbname(dns_adb_t *, dns_adbname_t **);
static inline dns_adbnamehook_t *new_adbnamehook(dns_adb_t *,
						 dns_adbentry_t *);
static inline void free_adbnamehook(dns_adb_t *, dns_adbnamehook_t **);
static inline dns_adblameinfo_t *new_adblameinfo(dns_adb_t *, dns_name_t *,
						 dns_rdatatype_t);
static inline void free_adblameinfo(dns_adb_t *, dns_adblameinfo_t **);
static inline dns_adbentry_t *new_adbentry(dns_adb_t *);
static inline void free_adbentry(dns_adb_t *, dns_adbentry_t **);
static inline dns_adbfind_t *new_adbfind(dns_adb_t *);
static inline isc_boolean_t free_adbfind(dns_adb_t *, dns_adbfind_t **);
static inline dns_adbaddrinfo_t *new_adbaddrinfo(dns_adb_t *, dns_adbentry_t *,
						 in_port_t);
static inline dns_adbfetch_t *new_adbfetch(dns_adb_t *);
static inline void free_adbfetch(dns_adb_t *, dns_adbfetch_t **);
static inline dns_adbname_t *find_name_and_lock(dns_adb_t *, dns_name_t *,
						unsigned int, int *);
static inline dns_adbentry_t *find_entry_and_lock(dns_adb_t *,
						  isc_sockaddr_t *, int *,
						  isc_stdtime_t);
static void dump_adb(dns_adb_t *, FILE *, isc_boolean_t debug, isc_stdtime_t);
static void print_dns_name(FILE *, dns_name_t *);
static void print_namehook_list(FILE *, const char *legend,
				dns_adbnamehooklist_t *list,
				isc_boolean_t debug,
				isc_stdtime_t now);
static void print_find_list(FILE *, dns_adbname_t *);
static void print_fetch_list(FILE *, dns_adbname_t *);
static inline isc_boolean_t dec_adb_irefcnt(dns_adb_t *);
static inline void inc_adb_irefcnt(dns_adb_t *);
static inline void inc_adb_erefcnt(dns_adb_t *);
static inline void inc_entry_refcnt(dns_adb_t *, dns_adbentry_t *,
				    isc_boolean_t);
static inline isc_boolean_t dec_entry_refcnt(dns_adb_t *, dns_adbentry_t *,
					     isc_boolean_t);
static inline void violate_locking_hierarchy(isc_mutex_t *, isc_mutex_t *);
static isc_boolean_t clean_namehooks(dns_adb_t *, dns_adbnamehooklist_t *);
static void clean_target(dns_adb_t *, dns_name_t *);
static void clean_finds_at_name(dns_adbname_t *, isc_eventtype_t,
				unsigned int);
static isc_boolean_t check_expire_namehooks(dns_adbname_t *, isc_stdtime_t);
static isc_boolean_t check_expire_entry(dns_adb_t *, dns_adbentry_t **,
					isc_stdtime_t);
static void cancel_fetches_at_name(dns_adbname_t *);
static isc_result_t dbfind_name(dns_adbname_t *, isc_stdtime_t,
				dns_rdatatype_t);
static isc_result_t fetch_name(dns_adbname_t *, isc_boolean_t,
			       dns_rdatatype_t);
static inline void check_exit(dns_adb_t *);
static void destroy(dns_adb_t *);
static isc_boolean_t shutdown_names(dns_adb_t *);
static isc_boolean_t shutdown_entries(dns_adb_t *);
static inline void link_name(dns_adb_t *, int, dns_adbname_t *);
static inline isc_boolean_t unlink_name(dns_adb_t *, dns_adbname_t *);
static inline void link_entry(dns_adb_t *, int, dns_adbentry_t *);
static inline isc_boolean_t unlink_entry(dns_adb_t *, dns_adbentry_t *);
static isc_boolean_t kill_name(dns_adbname_t **, isc_eventtype_t);
static void water(void *, int);
static void dump_entry(FILE *, dns_adbentry_t *, isc_boolean_t, isc_stdtime_t);

/*
 * MUST NOT overlap DNS_ADBFIND_* flags!
 */
#define FIND_EVENT_SENT         0x40000000
#define FIND_EVENT_FREED        0x80000000
#define FIND_EVENTSENT(h)       (((h)->flags & FIND_EVENT_SENT) != 0)
#define FIND_EVENTFREED(h)      (((h)->flags & FIND_EVENT_FREED) != 0)

#define NAME_NEEDS_POKE         0x80000000
#define NAME_IS_DEAD            0x40000000
#define NAME_HINT_OK            DNS_ADBFIND_HINTOK
#define NAME_GLUE_OK            DNS_ADBFIND_GLUEOK
#define NAME_STARTATZONE        DNS_ADBFIND_STARTATZONE
#define NAME_DEAD(n)            (((n)->flags & NAME_IS_DEAD) != 0)
#define NAME_NEEDSPOKE(n)       (((n)->flags & NAME_NEEDS_POKE) != 0)
#define NAME_GLUEOK(n)          (((n)->flags & NAME_GLUE_OK) != 0)
#define NAME_HINTOK(n)          (((n)->flags & NAME_HINT_OK) != 0)

/*
 * Private flag(s) for entries.
 * MUST NOT overlap FCTX_ADDRINFO_xxx and DNS_FETCHOPT_NOEDNS0.
 */
#define ENTRY_IS_DEAD		0x80000000

/*
 * To the name, address classes are all that really exist.  If it has a
 * V6 address it doesn't care if it came from a AAAA query.
 */
#define NAME_HAS_V4(n)          (!ISC_LIST_EMPTY((n)->v4))
#define NAME_HAS_V6(n)          (!ISC_LIST_EMPTY((n)->v6))
#define NAME_HAS_ADDRS(n)       (NAME_HAS_V4(n) || NAME_HAS_V6(n))

/*
 * Fetches are broken out into A and AAAA types.  In some cases,
 * however, it makes more sense to test for a particular class of fetches,
 * like V4 or V6 above.
 * Note: since we have removed the support of A6 in adb, FETCH_A and FETCH_AAAA
 * are now equal to FETCH_V4 and FETCH_V6, respectively.
 */
#define NAME_FETCH_A(n)         ((n)->fetch_a != NULL)
#define NAME_FETCH_AAAA(n)      ((n)->fetch_aaaa != NULL)
#define NAME_FETCH_V4(n)        (NAME_FETCH_A(n))
#define NAME_FETCH_V6(n)        (NAME_FETCH_AAAA(n))
#define NAME_FETCH(n)           (NAME_FETCH_V4(n) || NAME_FETCH_V6(n))

/*
 * Find options and tests to see if there are addresses on the list.
 */
#define FIND_WANTEVENT(fn)      (((fn)->options & DNS_ADBFIND_WANTEVENT) != 0)
#define FIND_WANTEMPTYEVENT(fn) (((fn)->options & DNS_ADBFIND_EMPTYEVENT) != 0)
#define FIND_AVOIDFETCHES(fn)   (((fn)->options & DNS_ADBFIND_AVOIDFETCHES) \
				 != 0)
#define FIND_STARTATZONE(fn)    (((fn)->options & DNS_ADBFIND_STARTATZONE) \
				 != 0)
#define FIND_HINTOK(fn)         (((fn)->options & DNS_ADBFIND_HINTOK) != 0)
#define FIND_GLUEOK(fn)         (((fn)->options & DNS_ADBFIND_GLUEOK) != 0)
#define FIND_HAS_ADDRS(fn)      (!ISC_LIST_EMPTY((fn)->list))
#define FIND_RETURNLAME(fn)     (((fn)->options & DNS_ADBFIND_RETURNLAME) != 0)

/*
 * These are currently used on simple unsigned ints, so they are
 * not really associated with any particular type.
 */
#define WANT_INET(x)            (((x) & DNS_ADBFIND_INET) != 0)
#define WANT_INET6(x)           (((x) & DNS_ADBFIND_INET6) != 0)

#define EXPIRE_OK(exp, now)     ((exp == INT_MAX) || (exp < now))

/*
 * Find out if the flags on a name (nf) indicate if it is a hint or
 * glue, and compare this to the appropriate bits set in o, to see if
 * this is ok.
 */
#define GLUE_OK(nf, o) (!NAME_GLUEOK(nf) || (((o) & DNS_ADBFIND_GLUEOK) != 0))
#define HINT_OK(nf, o) (!NAME_HINTOK(nf) || (((o) & DNS_ADBFIND_HINTOK) != 0))
#define GLUEHINT_OK(nf, o) (GLUE_OK(nf, o) || HINT_OK(nf, o))
#define STARTATZONE_MATCHES(nf, o) (((nf)->flags & NAME_STARTATZONE) == \
				    ((o) & DNS_ADBFIND_STARTATZONE))

#define ENTER_LEVEL             ISC_LOG_DEBUG(50)
#define EXIT_LEVEL              ENTER_LEVEL
#define CLEAN_LEVEL             ISC_LOG_DEBUG(100)
#define DEF_LEVEL               ISC_LOG_DEBUG(5)
#define NCACHE_LEVEL            ISC_LOG_DEBUG(20)

#define NCACHE_RESULT(r)        ((r) == DNS_R_NCACHENXDOMAIN || \
				 (r) == DNS_R_NCACHENXRRSET)
#define AUTH_NX(r)              ((r) == DNS_R_NXDOMAIN || \
				 (r) == DNS_R_NXRRSET)
#define NXDOMAIN_RESULT(r)      ((r) == DNS_R_NXDOMAIN || \
				 (r) == DNS_R_NCACHENXDOMAIN)
#define NXRRSET_RESULT(r)       ((r) == DNS_R_NCACHENXRRSET || \
				 (r) == DNS_R_NXRRSET || \
				 (r) == DNS_R_HINTNXRRSET)

/*
 * Error state rankings.
 */

#define FIND_ERR_SUCCESS                0  /* highest rank */
#define FIND_ERR_CANCELED               1
#define FIND_ERR_FAILURE                2
#define FIND_ERR_NXDOMAIN               3
#define FIND_ERR_NXRRSET                4
#define FIND_ERR_UNEXPECTED             5
#define FIND_ERR_NOTFOUND               6
#define FIND_ERR_MAX                    7

static const char *errnames[] = {
	"success",
	"canceled",
	"failure",
	"nxdomain",
	"nxrrset",
	"unexpected",
	"not_found"
};

#define NEWERR(old, new)        (ISC_MIN((old), (new)))

static isc_result_t find_err_map[FIND_ERR_MAX] = {
	ISC_R_SUCCESS,
	ISC_R_CANCELED,
	ISC_R_FAILURE,
	DNS_R_NXDOMAIN,
	DNS_R_NXRRSET,
	ISC_R_UNEXPECTED,
	ISC_R_NOTFOUND          /* not YET found */
};

static void
DP(int level, const char *format, ...) ISC_FORMAT_PRINTF(2, 3);

static void
DP(int level, const char *format, ...) {
	va_list args;

	va_start(args, format);
	isc_log_vwrite(dns_lctx,
		       DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_ADB,
		       level, format, args);
	va_end(args);
}

/*%
 * Increment resolver-related statistics counters.
 */
static inline void
inc_stats(dns_adb_t *adb, isc_statscounter_t counter) {
	if (adb->view->resstats != NULL)
		isc_stats_increment(adb->view->resstats, counter);
}

static inline dns_ttl_t
ttlclamp(dns_ttl_t ttl) {
	if (ttl < ADB_CACHE_MINIMUM)
		ttl = ADB_CACHE_MINIMUM;
	if (ttl > ADB_CACHE_MAXIMUM)
		ttl = ADB_CACHE_MAXIMUM;

	return (ttl);
}

/*
 * Requires the adbname bucket be locked and that no entry buckets be locked.
 *
 * This code handles A and AAAA rdatasets only.
 */
static isc_result_t
import_rdataset(dns_adbname_t *adbname, dns_rdataset_t *rdataset,
		isc_stdtime_t now)
{
	isc_result_t result;
	dns_adb_t *adb;
	dns_adbnamehook_t *nh;
	dns_adbnamehook_t *anh;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	struct in_addr ina;
	struct in6_addr in6a;
	isc_sockaddr_t sockaddr;
	dns_adbentry_t *foundentry;  /* NO CLEAN UP! */
	int addr_bucket;
	isc_boolean_t new_addresses_added;
	dns_rdatatype_t rdtype;
	unsigned int findoptions;
	dns_adbnamehooklist_t *hookhead;

	INSIST(DNS_ADBNAME_VALID(adbname));
	adb = adbname->adb;
	INSIST(DNS_ADB_VALID(adb));

	rdtype = rdataset->type;
	INSIST((rdtype == dns_rdatatype_a) || (rdtype == dns_rdatatype_aaaa));
	if (rdtype == dns_rdatatype_a)
		findoptions = DNS_ADBFIND_INET;
	else
		findoptions = DNS_ADBFIND_INET6;

	addr_bucket = DNS_ADB_INVALIDBUCKET;
	new_addresses_added = ISC_FALSE;

	nh = NULL;
	result = dns_rdataset_first(rdataset);
	while (result == ISC_R_SUCCESS) {
		dns_rdata_reset(&rdata);
		dns_rdataset_current(rdataset, &rdata);
		if (rdtype == dns_rdatatype_a) {
			INSIST(rdata.length == 4);
			memcpy(&ina.s_addr, rdata.data, 4);
			isc_sockaddr_fromin(&sockaddr, &ina, 0);
			hookhead = &adbname->v4;
		} else {
			INSIST(rdata.length == 16);
			memcpy(in6a.s6_addr, rdata.data, 16);
			isc_sockaddr_fromin6(&sockaddr, &in6a, 0);
			hookhead = &adbname->v6;
		}

		INSIST(nh == NULL);
		nh = new_adbnamehook(adb, NULL);
		if (nh == NULL) {
			adbname->partial_result |= findoptions;
			result = ISC_R_NOMEMORY;
			goto fail;
		}

		foundentry = find_entry_and_lock(adb, &sockaddr, &addr_bucket,
						 now);
		if (foundentry == NULL) {
			dns_adbentry_t *entry;

			entry = new_adbentry(adb);
			if (entry == NULL) {
				adbname->partial_result |= findoptions;
				result = ISC_R_NOMEMORY;
				goto fail;
			}

			entry->sockaddr = sockaddr;
			entry->refcnt = 1;

			nh->entry = entry;

			link_entry(adb, addr_bucket, entry);
		} else {
			for (anh = ISC_LIST_HEAD(*hookhead);
			     anh != NULL;
			     anh = ISC_LIST_NEXT(anh, plink))
				if (anh->entry == foundentry)
					break;
			if (anh == NULL) {
				foundentry->refcnt++;
				nh->entry = foundentry;
			} else
				free_adbnamehook(adb, &nh);
		}

		new_addresses_added = ISC_TRUE;
		if (nh != NULL)
			ISC_LIST_APPEND(*hookhead, nh, plink);
		nh = NULL;
		result = dns_rdataset_next(rdataset);
	}

 fail:
	if (nh != NULL)
		free_adbnamehook(adb, &nh);

	if (addr_bucket != DNS_ADB_INVALIDBUCKET)
		UNLOCK(&adb->entrylocks[addr_bucket]);

	if (rdataset->trust == dns_trust_glue ||
	    rdataset->trust == dns_trust_additional)
		rdataset->ttl = ADB_CACHE_MINIMUM;
	else
		rdataset->ttl = ttlclamp(rdataset->ttl);

	if (rdtype == dns_rdatatype_a) {
		DP(NCACHE_LEVEL, "expire_v4 set to MIN(%u,%u) import_rdataset",
		   adbname->expire_v4, now + rdataset->ttl);
		adbname->expire_v4 = ISC_MIN(adbname->expire_v4,
					     now + rdataset->ttl);
	} else {
		DP(NCACHE_LEVEL, "expire_v6 set to MIN(%u,%u) import_rdataset",
		   adbname->expire_v6, now + rdataset->ttl);
		adbname->expire_v6 = ISC_MIN(adbname->expire_v6,
					     now + rdataset->ttl);
	}

	if (new_addresses_added) {
		/*
		 * Lie a little here.  This is more or less so code that cares
		 * can find out if any new information was added or not.
		 */
		return (ISC_R_SUCCESS);
	}

	return (result);
}

/*
 * Requires the name's bucket be locked.
 */
static isc_boolean_t
kill_name(dns_adbname_t **n, isc_eventtype_t ev) {
	dns_adbname_t *name;
	isc_boolean_t result = ISC_FALSE;
	isc_boolean_t result4, result6;
	int bucket;
	dns_adb_t *adb;

	INSIST(n != NULL);
	name = *n;
	*n = NULL;
	INSIST(DNS_ADBNAME_VALID(name));
	adb = name->adb;
	INSIST(DNS_ADB_VALID(adb));

	DP(DEF_LEVEL, "killing name %p", name);

	/*
	 * If we're dead already, just check to see if we should go
	 * away now or not.
	 */
	if (NAME_DEAD(name) && !NAME_FETCH(name)) {
		result = unlink_name(adb, name);
		free_adbname(adb, &name);
		if (result)
			result = dec_adb_irefcnt(adb);
		return (result);
	}

	/*
	 * Clean up the name's various lists.  These two are destructive
	 * in that they will always empty the list.
	 */
	clean_finds_at_name(name, ev, DNS_ADBFIND_ADDRESSMASK);
	result4 = clean_namehooks(adb, &name->v4);
	result6 = clean_namehooks(adb, &name->v6);
	clean_target(adb, &name->target);
	result = ISC_TF(result4 || result6);

	/*
	 * If fetches are running, cancel them.  If none are running, we can
	 * just kill the name here.
	 */
	if (!NAME_FETCH(name)) {
		INSIST(result == ISC_FALSE);
		result = unlink_name(adb, name);
		free_adbname(adb, &name);
		if (result)
			result = dec_adb_irefcnt(adb);
	} else {
		cancel_fetches_at_name(name);
		if (!NAME_DEAD(name)) {
			bucket = name->lock_bucket;
			ISC_LIST_UNLINK(adb->names[bucket], name, plink);
			ISC_LIST_APPEND(adb->deadnames[bucket], name, plink);
			name->flags |= NAME_IS_DEAD;
		}
	}
	return (result);
}

/*
 * Requires the name's bucket be locked and no entry buckets be locked.
 */
static isc_boolean_t
check_expire_namehooks(dns_adbname_t *name, isc_stdtime_t now) {
	dns_adb_t *adb;
	isc_boolean_t result4 = ISC_FALSE;
	isc_boolean_t result6 = ISC_FALSE;

	INSIST(DNS_ADBNAME_VALID(name));
	adb = name->adb;
	INSIST(DNS_ADB_VALID(adb));

	/*
	 * Check to see if we need to remove the v4 addresses
	 */
	if (!NAME_FETCH_V4(name) && EXPIRE_OK(name->expire_v4, now)) {
		if (NAME_HAS_V4(name)) {
			DP(DEF_LEVEL, "expiring v4 for name %p", name);
			result4 = clean_namehooks(adb, &name->v4);
			name->partial_result &= ~DNS_ADBFIND_INET;
		}
		name->expire_v4 = INT_MAX;
		name->fetch_err = FIND_ERR_UNEXPECTED;
	}

	/*
	 * Check to see if we need to remove the v6 addresses
	 */
	if (!NAME_FETCH_V6(name) && EXPIRE_OK(name->expire_v6, now)) {
		if (NAME_HAS_V6(name)) {
			DP(DEF_LEVEL, "expiring v6 for name %p", name);
			result6 = clean_namehooks(adb, &name->v6);
			name->partial_result &= ~DNS_ADBFIND_INET6;
		}
		name->expire_v6 = INT_MAX;
		name->fetch6_err = FIND_ERR_UNEXPECTED;
	}

	/*
	 * Check to see if we need to remove the alias target.
	 */
	if (EXPIRE_OK(name->expire_target, now)) {
		clean_target(adb, &name->target);
		name->expire_target = INT_MAX;
	}
	return (ISC_TF(result4 || result6));
}

/*
 * Requires the name's bucket be locked.
 */
static inline void
link_name(dns_adb_t *adb, int bucket, dns_adbname_t *name) {
	INSIST(name->lock_bucket == DNS_ADB_INVALIDBUCKET);

	ISC_LIST_PREPEND(adb->names[bucket], name, plink);
	name->lock_bucket = bucket;
	adb->name_refcnt[bucket]++;
}

/*
 * Requires the name's bucket be locked.
 */
static inline isc_boolean_t
unlink_name(dns_adb_t *adb, dns_adbname_t *name) {
	int bucket;
	isc_boolean_t result = ISC_FALSE;

	bucket = name->lock_bucket;
	INSIST(bucket != DNS_ADB_INVALIDBUCKET);

	if (NAME_DEAD(name))
		ISC_LIST_UNLINK(adb->deadnames[bucket], name, plink);
	else
		ISC_LIST_UNLINK(adb->names[bucket], name, plink);
	name->lock_bucket = DNS_ADB_INVALIDBUCKET;
	INSIST(adb->name_refcnt[bucket] > 0);
	adb->name_refcnt[bucket]--;
	if (adb->name_sd[bucket] && adb->name_refcnt[bucket] == 0)
		result = ISC_TRUE;
	return (result);
}

/*
 * Requires the entry's bucket be locked.
 */
static inline void
link_entry(dns_adb_t *adb, int bucket, dns_adbentry_t *entry) {
	int i;
	dns_adbentry_t *e;

	if (adb->overmem) {
		for (i = 0; i < 2; i++) {
			e = ISC_LIST_TAIL(adb->entries[bucket]);
			if (e == NULL)
				break;
			if (e->refcnt == 0) {
				unlink_entry(adb, e);
				free_adbentry(adb, &e);
				continue;
			}
			INSIST((e->flags & ENTRY_IS_DEAD) == 0);
			e->flags |= ENTRY_IS_DEAD;
			ISC_LIST_UNLINK(adb->entries[bucket], e, plink);
			ISC_LIST_PREPEND(adb->deadentries[bucket], e, plink);
		}
	}

	ISC_LIST_PREPEND(adb->entries[bucket], entry, plink);
	entry->lock_bucket = bucket;
	adb->entry_refcnt[bucket]++;
}

/*
 * Requires the entry's bucket be locked.
 */
static inline isc_boolean_t
unlink_entry(dns_adb_t *adb, dns_adbentry_t *entry) {
	int bucket;
	isc_boolean_t result = ISC_FALSE;

	bucket = entry->lock_bucket;
	INSIST(bucket != DNS_ADB_INVALIDBUCKET);

	if ((entry->flags & ENTRY_IS_DEAD) != 0)
		ISC_LIST_UNLINK(adb->deadentries[bucket], entry, plink);
	else
		ISC_LIST_UNLINK(adb->entries[bucket], entry, plink);
	entry->lock_bucket = DNS_ADB_INVALIDBUCKET;
	INSIST(adb->entry_refcnt[bucket] > 0);
	adb->entry_refcnt[bucket]--;
	if (adb->entry_sd[bucket] && adb->entry_refcnt[bucket] == 0)
		result = ISC_TRUE;
	return (result);
}

static inline void
violate_locking_hierarchy(isc_mutex_t *have, isc_mutex_t *want) {
	if (isc_mutex_trylock(want) != ISC_R_SUCCESS) {
		UNLOCK(have);
		LOCK(want);
		LOCK(have);
	}
}

/*
 * The ADB _MUST_ be locked before calling.  Also, exit conditions must be
 * checked after calling this function.
 */
static isc_boolean_t
shutdown_names(dns_adb_t *adb) {
	int bucket;
	isc_boolean_t result = ISC_FALSE;
	dns_adbname_t *name;
	dns_adbname_t *next_name;

	for (bucket = 0; bucket < NBUCKETS; bucket++) {
		LOCK(&adb->namelocks[bucket]);
		adb->name_sd[bucket] = ISC_TRUE;

		name = ISC_LIST_HEAD(adb->names[bucket]);
		if (name == NULL) {
			/*
			 * This bucket has no names.  We must decrement the
			 * irefcnt ourselves, since it will not be
			 * automatically triggered by a name being unlinked.
			 */
			INSIST(result == ISC_FALSE);
			result = dec_adb_irefcnt(adb);
		} else {
			/*
			 * Run through the list.  For each name, clean up finds
			 * found there, and cancel any fetches running.  When
			 * all the fetches are canceled, the name will destroy
			 * itself.
			 */
			while (name != NULL) {
				next_name = ISC_LIST_NEXT(name, plink);
				INSIST(result == ISC_FALSE);
				result = kill_name(&name,
						   DNS_EVENT_ADBSHUTDOWN);
				name = next_name;
			}
		}

		UNLOCK(&adb->namelocks[bucket]);
	}
	return (result);
}

/*
 * The ADB _MUST_ be locked before calling.  Also, exit conditions must be
 * checked after calling this function.
 */
static isc_boolean_t
shutdown_entries(dns_adb_t *adb) {
	int bucket;
	isc_boolean_t result = ISC_FALSE;
	dns_adbentry_t *entry;
	dns_adbentry_t *next_entry;

	for (bucket = 0; bucket < NBUCKETS; bucket++) {
		LOCK(&adb->entrylocks[bucket]);
		adb->entry_sd[bucket] = ISC_TRUE;

		entry = ISC_LIST_HEAD(adb->entries[bucket]);
		if (adb->entry_refcnt[bucket] == 0) {
			/*
			 * This bucket has no entries.  We must decrement the
			 * irefcnt ourselves, since it will not be
			 * automatically triggered by an entry being unlinked.
			 */
			result = dec_adb_irefcnt(adb);
		} else {
			/*
			 * Run through the list.  Cleanup any entries not
			 * associated with names, and which are not in use.
			 */
			while (entry != NULL) {
				next_entry = ISC_LIST_NEXT(entry, plink);
				if (entry->refcnt == 0 &&
				    entry->expires != 0) {
					result = unlink_entry(adb, entry);
					free_adbentry(adb, &entry);
					if (result)
						result = dec_adb_irefcnt(adb);
				}
				entry = next_entry;
			}
		}

		UNLOCK(&adb->entrylocks[bucket]);
	}
	return (result);
}

/*
 * Name bucket must be locked
 */
static void
cancel_fetches_at_name(dns_adbname_t *name) {
	if (NAME_FETCH_A(name))
	    dns_resolver_cancelfetch(name->fetch_a->fetch);

	if (NAME_FETCH_AAAA(name))
	    dns_resolver_cancelfetch(name->fetch_aaaa->fetch);
}

/*
 * Assumes the name bucket is locked.
 */
static isc_boolean_t
clean_namehooks(dns_adb_t *adb, dns_adbnamehooklist_t *namehooks) {
	dns_adbentry_t *entry;
	dns_adbnamehook_t *namehook;
	int addr_bucket;
	isc_boolean_t result = ISC_FALSE;

	addr_bucket = DNS_ADB_INVALIDBUCKET;
	namehook = ISC_LIST_HEAD(*namehooks);
	while (namehook != NULL) {
		INSIST(DNS_ADBNAMEHOOK_VALID(namehook));

		/*
		 * Clean up the entry if needed.
		 */
		entry = namehook->entry;
		if (entry != NULL) {
			INSIST(DNS_ADBENTRY_VALID(entry));

			if (addr_bucket != entry->lock_bucket) {
				if (addr_bucket != DNS_ADB_INVALIDBUCKET)
					UNLOCK(&adb->entrylocks[addr_bucket]);
				addr_bucket = entry->lock_bucket;
				LOCK(&adb->entrylocks[addr_bucket]);
			}

			result = dec_entry_refcnt(adb, entry, ISC_FALSE);
		}

		/*
		 * Free the namehook
		 */
		namehook->entry = NULL;
		ISC_LIST_UNLINK(*namehooks, namehook, plink);
		free_adbnamehook(adb, &namehook);

		namehook = ISC_LIST_HEAD(*namehooks);
	}

	if (addr_bucket != DNS_ADB_INVALIDBUCKET)
		UNLOCK(&adb->entrylocks[addr_bucket]);
	return (result);
}

static void
clean_target(dns_adb_t *adb, dns_name_t *target) {
	if (dns_name_countlabels(target) > 0) {
		dns_name_free(target, adb->mctx);
		dns_name_init(target, NULL);
	}
}

static isc_result_t
set_target(dns_adb_t *adb, dns_name_t *name, dns_name_t *fname,
	   dns_rdataset_t *rdataset, dns_name_t *target)
{
	isc_result_t result;
	dns_namereln_t namereln;
	unsigned int nlabels;
	int order;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_fixedname_t fixed1, fixed2;
	dns_name_t *prefix, *new_target;

	REQUIRE(dns_name_countlabels(target) == 0);

	if (rdataset->type == dns_rdatatype_cname) {
		dns_rdata_cname_t cname;

		/*
		 * Copy the CNAME's target into the target name.
		 */
		result = dns_rdataset_first(rdataset);
		if (result != ISC_R_SUCCESS)
			return (result);
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &cname, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);
		result = dns_name_dup(&cname.cname, adb->mctx, target);
		dns_rdata_freestruct(&cname);
		if (result != ISC_R_SUCCESS)
			return (result);
	} else {
		dns_rdata_dname_t dname;

		INSIST(rdataset->type == dns_rdatatype_dname);
		namereln = dns_name_fullcompare(name, fname, &order, &nlabels);
		INSIST(namereln == dns_namereln_subdomain);
		/*
		 * Get the target name of the DNAME.
		 */
		result = dns_rdataset_first(rdataset);
		if (result != ISC_R_SUCCESS)
			return (result);
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &dname, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);
		/*
		 * Construct the new target name.
		 */
		dns_fixedname_init(&fixed1);
		prefix = dns_fixedname_name(&fixed1);
		dns_fixedname_init(&fixed2);
		new_target = dns_fixedname_name(&fixed2);
		dns_name_split(name, nlabels, prefix, NULL);
		result = dns_name_concatenate(prefix, &dname.dname, new_target,
					      NULL);
		dns_rdata_freestruct(&dname);
		if (result != ISC_R_SUCCESS)
			return (result);
		result = dns_name_dup(new_target, adb->mctx, target);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	return (ISC_R_SUCCESS);
}

/*
 * Assumes nothing is locked, since this is called by the client.
 */
static void
event_free(isc_event_t *event) {
	dns_adbfind_t *find;

	INSIST(event != NULL);
	find = event->ev_destroy_arg;
	INSIST(DNS_ADBFIND_VALID(find));

	LOCK(&find->lock);
	find->flags |= FIND_EVENT_FREED;
	event->ev_destroy_arg = NULL;
	UNLOCK(&find->lock);
}

/*
 * Assumes the name bucket is locked.
 */
static void
clean_finds_at_name(dns_adbname_t *name, isc_eventtype_t evtype,
		    unsigned int addrs)
{
	isc_event_t *ev;
	isc_task_t *task;
	dns_adbfind_t *find;
	dns_adbfind_t *next_find;
	isc_boolean_t process;
	unsigned int wanted, notify;

	DP(ENTER_LEVEL,
	   "ENTER clean_finds_at_name, name %p, evtype %08x, addrs %08x",
	   name, evtype, addrs);

	find = ISC_LIST_HEAD(name->finds);
	while (find != NULL) {
		LOCK(&find->lock);
		next_find = ISC_LIST_NEXT(find, plink);

		process = ISC_FALSE;
		wanted = find->flags & DNS_ADBFIND_ADDRESSMASK;
		notify = wanted & addrs;

		switch (evtype) {
		case DNS_EVENT_ADBMOREADDRESSES:
			DP(ISC_LOG_DEBUG(3), "DNS_EVENT_ADBMOREADDRESSES");
			if ((notify) != 0) {
				find->flags &= ~addrs;
				process = ISC_TRUE;
			}
			break;
		case DNS_EVENT_ADBNOMOREADDRESSES:
			DP(ISC_LOG_DEBUG(3), "DNS_EVENT_ADBNOMOREADDRESSES");
			find->flags &= ~addrs;
			wanted = find->flags & DNS_ADBFIND_ADDRESSMASK;
			if (wanted == 0)
				process = ISC_TRUE;
			break;
		default:
			find->flags &= ~addrs;
			process = ISC_TRUE;
		}

		if (process) {
			DP(DEF_LEVEL, "cfan: processing find %p", find);
			/*
			 * Unlink the find from the name, letting the caller
			 * call dns_adb_destroyfind() on it to clean it up
			 * later.
			 */
			ISC_LIST_UNLINK(name->finds, find, plink);
			find->adbname = NULL;
			find->name_bucket = DNS_ADB_INVALIDBUCKET;

			INSIST(!FIND_EVENTSENT(find));

			ev = &find->event;
			task = ev->ev_sender;
			ev->ev_sender = find;
			find->result_v4 = find_err_map[name->fetch_err];
			find->result_v6 = find_err_map[name->fetch6_err];
			ev->ev_type = evtype;
			ev->ev_destroy = event_free;
			ev->ev_destroy_arg = find;

			DP(DEF_LEVEL,
			   "sending event %p to task %p for find %p",
			   ev, task, find);

			isc_task_sendanddetach(&task, (isc_event_t **)&ev);
		} else {
			DP(DEF_LEVEL, "cfan: skipping find %p", find);
		}

		UNLOCK(&find->lock);
		find = next_find;
	}

	DP(ENTER_LEVEL, "EXIT clean_finds_at_name, name %p", name);
}

static inline void
check_exit(dns_adb_t *adb) {
	isc_event_t *event;
	/*
	 * The caller must be holding the adb lock.
	 */
	if (adb->shutting_down) {
		/*
		 * If there aren't any external references either, we're
		 * done.  Send the control event to initiate shutdown.
		 */
		INSIST(!adb->cevent_sent);      /* Sanity check. */
		event = &adb->cevent;
		isc_task_send(adb->task, &event);
		adb->cevent_sent = ISC_TRUE;
	}
}

static inline isc_boolean_t
dec_adb_irefcnt(dns_adb_t *adb) {
	isc_event_t *event;
	isc_task_t *etask;
	isc_boolean_t result = ISC_FALSE;

	LOCK(&adb->reflock);

	INSIST(adb->irefcnt > 0);
	adb->irefcnt--;

	if (adb->irefcnt == 0) {
		event = ISC_LIST_HEAD(adb->whenshutdown);
		while (event != NULL) {
			ISC_LIST_UNLINK(adb->whenshutdown, event, ev_link);
			etask = event->ev_sender;
			event->ev_sender = adb;
			isc_task_sendanddetach(&etask, &event);
			event = ISC_LIST_HEAD(adb->whenshutdown);
		}
	}

	if (adb->irefcnt == 0 && adb->erefcnt == 0)
		result = ISC_TRUE;
	UNLOCK(&adb->reflock);
	return (result);
}

static inline void
inc_adb_irefcnt(dns_adb_t *adb) {
	LOCK(&adb->reflock);
	adb->irefcnt++;
	UNLOCK(&adb->reflock);
}

static inline void
inc_adb_erefcnt(dns_adb_t *adb) {
	LOCK(&adb->reflock);
	adb->erefcnt++;
	UNLOCK(&adb->reflock);
}

static inline void
inc_entry_refcnt(dns_adb_t *adb, dns_adbentry_t *entry, isc_boolean_t lock) {
	int bucket;

	bucket = entry->lock_bucket;

	if (lock)
		LOCK(&adb->entrylocks[bucket]);

	entry->refcnt++;

	if (lock)
		UNLOCK(&adb->entrylocks[bucket]);
}

static inline isc_boolean_t
dec_entry_refcnt(dns_adb_t *adb, dns_adbentry_t *entry, isc_boolean_t lock) {
	int bucket;
	isc_boolean_t destroy_entry;
	isc_boolean_t result = ISC_FALSE;

	bucket = entry->lock_bucket;

	if (lock)
		LOCK(&adb->entrylocks[bucket]);

	INSIST(entry->refcnt > 0);
	entry->refcnt--;

	destroy_entry = ISC_FALSE;
	if (entry->refcnt == 0 &&
	    (adb->entry_sd[bucket] || entry->expires == 0 || adb->overmem ||
	     (entry->flags & ENTRY_IS_DEAD) != 0)) {
		destroy_entry = ISC_TRUE;
		result = unlink_entry(adb, entry);
	}

	if (lock)
		UNLOCK(&adb->entrylocks[bucket]);

	if (!destroy_entry)
		return (result);

	entry->lock_bucket = DNS_ADB_INVALIDBUCKET;

	free_adbentry(adb, &entry);
	if (result)
		result = dec_adb_irefcnt(adb);

	return (result);
}

static inline dns_adbname_t *
new_adbname(dns_adb_t *adb, dns_name_t *dnsname) {
	dns_adbname_t *name;

	name = isc_mempool_get(adb->nmp);
	if (name == NULL)
		return (NULL);

	dns_name_init(&name->name, NULL);
	if (dns_name_dup(dnsname, adb->mctx, &name->name) != ISC_R_SUCCESS) {
		isc_mempool_put(adb->nmp, name);
		return (NULL);
	}
	dns_name_init(&name->target, NULL);
	name->magic = DNS_ADBNAME_MAGIC;
	name->adb = adb;
	name->partial_result = 0;
	name->flags = 0;
	name->expire_v4 = INT_MAX;
	name->expire_v6 = INT_MAX;
	name->expire_target = INT_MAX;
	name->chains = 0;
	name->lock_bucket = DNS_ADB_INVALIDBUCKET;
	ISC_LIST_INIT(name->v4);
	ISC_LIST_INIT(name->v6);
	name->fetch_a = NULL;
	name->fetch_aaaa = NULL;
	name->fetch_err = FIND_ERR_UNEXPECTED;
	name->fetch6_err = FIND_ERR_UNEXPECTED;
	ISC_LIST_INIT(name->finds);
	ISC_LINK_INIT(name, plink);

	return (name);
}

static inline void
free_adbname(dns_adb_t *adb, dns_adbname_t **name) {
	dns_adbname_t *n;

	INSIST(name != NULL && DNS_ADBNAME_VALID(*name));
	n = *name;
	*name = NULL;

	INSIST(!NAME_HAS_V4(n));
	INSIST(!NAME_HAS_V6(n));
	INSIST(!NAME_FETCH(n));
	INSIST(ISC_LIST_EMPTY(n->finds));
	INSIST(!ISC_LINK_LINKED(n, plink));
	INSIST(n->lock_bucket == DNS_ADB_INVALIDBUCKET);
	INSIST(n->adb == adb);

	n->magic = 0;
	dns_name_free(&n->name, adb->mctx);

	isc_mempool_put(adb->nmp, n);
}

static inline dns_adbnamehook_t *
new_adbnamehook(dns_adb_t *adb, dns_adbentry_t *entry) {
	dns_adbnamehook_t *nh;

	nh = isc_mempool_get(adb->nhmp);
	if (nh == NULL)
		return (NULL);

	nh->magic = DNS_ADBNAMEHOOK_MAGIC;
	nh->entry = entry;
	ISC_LINK_INIT(nh, plink);

	return (nh);
}

static inline void
free_adbnamehook(dns_adb_t *adb, dns_adbnamehook_t **namehook) {
	dns_adbnamehook_t *nh;

	INSIST(namehook != NULL && DNS_ADBNAMEHOOK_VALID(*namehook));
	nh = *namehook;
	*namehook = NULL;

	INSIST(nh->entry == NULL);
	INSIST(!ISC_LINK_LINKED(nh, plink));

	nh->magic = 0;
	isc_mempool_put(adb->nhmp, nh);
}

static inline dns_adblameinfo_t *
new_adblameinfo(dns_adb_t *adb, dns_name_t *qname, dns_rdatatype_t qtype) {
	dns_adblameinfo_t *li;

	li = isc_mempool_get(adb->limp);
	if (li == NULL)
		return (NULL);

	dns_name_init(&li->qname, NULL);
	if (dns_name_dup(qname, adb->mctx, &li->qname) != ISC_R_SUCCESS) {
		isc_mempool_put(adb->limp, li);
		return (NULL);
	}
	li->magic = DNS_ADBLAMEINFO_MAGIC;
	li->lame_timer = 0;
	li->qtype = qtype;
	ISC_LINK_INIT(li, plink);

	return (li);
}

static inline void
free_adblameinfo(dns_adb_t *adb, dns_adblameinfo_t **lameinfo) {
	dns_adblameinfo_t *li;

	INSIST(lameinfo != NULL && DNS_ADBLAMEINFO_VALID(*lameinfo));
	li = *lameinfo;
	*lameinfo = NULL;

	INSIST(!ISC_LINK_LINKED(li, plink));

	dns_name_free(&li->qname, adb->mctx);

	li->magic = 0;

	isc_mempool_put(adb->limp, li);
}

static inline dns_adbentry_t *
new_adbentry(dns_adb_t *adb) {
	dns_adbentry_t *e;
	isc_uint32_t r;

	e = isc_mempool_get(adb->emp);
	if (e == NULL)
		return (NULL);

	e->magic = DNS_ADBENTRY_MAGIC;
	e->lock_bucket = DNS_ADB_INVALIDBUCKET;
	e->refcnt = 0;
	e->flags = 0;
	isc_random_get(&r);
	e->srtt = (r & 0x1f) + 1;
	e->expires = 0;
	ISC_LIST_INIT(e->lameinfo);
	ISC_LINK_INIT(e, plink);

	return (e);
}

static inline void
free_adbentry(dns_adb_t *adb, dns_adbentry_t **entry) {
	dns_adbentry_t *e;
	dns_adblameinfo_t *li;

	INSIST(entry != NULL && DNS_ADBENTRY_VALID(*entry));
	e = *entry;
	*entry = NULL;

	INSIST(e->lock_bucket == DNS_ADB_INVALIDBUCKET);
	INSIST(e->refcnt == 0);
	INSIST(!ISC_LINK_LINKED(e, plink));

	e->magic = 0;

	li = ISC_LIST_HEAD(e->lameinfo);
	while (li != NULL) {
		ISC_LIST_UNLINK(e->lameinfo, li, plink);
		free_adblameinfo(adb, &li);
		li = ISC_LIST_HEAD(e->lameinfo);
	}

	isc_mempool_put(adb->emp, e);
}

static inline dns_adbfind_t *
new_adbfind(dns_adb_t *adb) {
	dns_adbfind_t *h;
	isc_result_t result;

	h = isc_mempool_get(adb->ahmp);
	if (h == NULL)
		return (NULL);

	/*
	 * Public members.
	 */
	h->magic = 0;
	h->adb = adb;
	h->partial_result = 0;
	h->options = 0;
	h->flags = 0;
	h->result_v4 = ISC_R_UNEXPECTED;
	h->result_v6 = ISC_R_UNEXPECTED;
	ISC_LINK_INIT(h, publink);
	ISC_LINK_INIT(h, plink);
	ISC_LIST_INIT(h->list);
	h->adbname = NULL;
	h->name_bucket = DNS_ADB_INVALIDBUCKET;

	/*
	 * private members
	 */
	result = isc_mutex_init(&h->lock);
	if (result != ISC_R_SUCCESS) {
		isc_mempool_put(adb->ahmp, h);
		return (NULL);
	}

	ISC_EVENT_INIT(&h->event, sizeof(isc_event_t), 0, 0, 0, NULL, NULL,
		       NULL, NULL, h);

	inc_adb_irefcnt(adb);
	h->magic = DNS_ADBFIND_MAGIC;
	return (h);
}

static inline dns_adbfetch_t *
new_adbfetch(dns_adb_t *adb) {
	dns_adbfetch_t *f;

	f = isc_mempool_get(adb->afmp);
	if (f == NULL)
		return (NULL);

	f->magic = 0;
	f->fetch = NULL;

	dns_rdataset_init(&f->rdataset);

	f->magic = DNS_ADBFETCH_MAGIC;

	return (f);
}

static inline void
free_adbfetch(dns_adb_t *adb, dns_adbfetch_t **fetch) {
	dns_adbfetch_t *f;

	INSIST(fetch != NULL && DNS_ADBFETCH_VALID(*fetch));
	f = *fetch;
	*fetch = NULL;

	f->magic = 0;

	if (dns_rdataset_isassociated(&f->rdataset))
		dns_rdataset_disassociate(&f->rdataset);

	isc_mempool_put(adb->afmp, f);
}

static inline isc_boolean_t
free_adbfind(dns_adb_t *adb, dns_adbfind_t **findp) {
	dns_adbfind_t *find;

	INSIST(findp != NULL && DNS_ADBFIND_VALID(*findp));
	find = *findp;
	*findp = NULL;

	INSIST(!FIND_HAS_ADDRS(find));
	INSIST(!ISC_LINK_LINKED(find, publink));
	INSIST(!ISC_LINK_LINKED(find, plink));
	INSIST(find->name_bucket == DNS_ADB_INVALIDBUCKET);
	INSIST(find->adbname == NULL);

	find->magic = 0;

	DESTROYLOCK(&find->lock);
	isc_mempool_put(adb->ahmp, find);
	return (dec_adb_irefcnt(adb));
}

/*
 * Copy bits from the entry into the newly allocated addrinfo.  The entry
 * must be locked, and the reference count must be bumped up by one
 * if this function returns a valid pointer.
 */
static inline dns_adbaddrinfo_t *
new_adbaddrinfo(dns_adb_t *adb, dns_adbentry_t *entry, in_port_t port) {
	dns_adbaddrinfo_t *ai;

	ai = isc_mempool_get(adb->aimp);
	if (ai == NULL)
		return (NULL);

	ai->magic = DNS_ADBADDRINFO_MAGIC;
	ai->sockaddr = entry->sockaddr;
	isc_sockaddr_setport(&ai->sockaddr, port);
	ai->srtt = entry->srtt;
	ai->flags = entry->flags;
	ai->entry = entry;
	ISC_LINK_INIT(ai, publink);

	return (ai);
}

static inline void
free_adbaddrinfo(dns_adb_t *adb, dns_adbaddrinfo_t **ainfo) {
	dns_adbaddrinfo_t *ai;

	INSIST(ainfo != NULL && DNS_ADBADDRINFO_VALID(*ainfo));
	ai = *ainfo;
	*ainfo = NULL;

	INSIST(ai->entry == NULL);
	INSIST(!ISC_LINK_LINKED(ai, publink));

	ai->magic = 0;

	isc_mempool_put(adb->aimp, ai);
}

/*
 * Search for the name.  NOTE:  The bucket is kept locked on both
 * success and failure, so it must always be unlocked by the caller!
 *
 * On the first call to this function, *bucketp must be set to
 * DNS_ADB_INVALIDBUCKET.
 */
static inline dns_adbname_t *
find_name_and_lock(dns_adb_t *adb, dns_name_t *name,
		   unsigned int options, int *bucketp)
{
	dns_adbname_t *adbname;
	int bucket;

	bucket = dns_name_fullhash(name, ISC_FALSE) % NBUCKETS;

	if (*bucketp == DNS_ADB_INVALIDBUCKET) {
		LOCK(&adb->namelocks[bucket]);
		*bucketp = bucket;
	} else if (*bucketp != bucket) {
		UNLOCK(&adb->namelocks[*bucketp]);
		LOCK(&adb->namelocks[bucket]);
		*bucketp = bucket;
	}

	adbname = ISC_LIST_HEAD(adb->names[bucket]);
	while (adbname != NULL) {
		if (!NAME_DEAD(adbname)) {
			if (dns_name_equal(name, &adbname->name)
			    && GLUEHINT_OK(adbname, options)
			    && STARTATZONE_MATCHES(adbname, options))
				return (adbname);
		}
		adbname = ISC_LIST_NEXT(adbname, plink);
	}

	return (NULL);
}

/*
 * Search for the address.  NOTE:  The bucket is kept locked on both
 * success and failure, so it must always be unlocked by the caller.
 *
 * On the first call to this function, *bucketp must be set to
 * DNS_ADB_INVALIDBUCKET.  This will cause a lock to occur.  On
 * later calls (within the same "lock path") it can be left alone, so
 * if this function is called multiple times locking is only done if
 * the bucket changes.
 */
static inline dns_adbentry_t *
find_entry_and_lock(dns_adb_t *adb, isc_sockaddr_t *addr, int *bucketp,
	isc_stdtime_t now)
{
	dns_adbentry_t *entry, *entry_next;
	int bucket;

	bucket = isc_sockaddr_hash(addr, ISC_TRUE) % NBUCKETS;

	if (*bucketp == DNS_ADB_INVALIDBUCKET) {
		LOCK(&adb->entrylocks[bucket]);
		*bucketp = bucket;
	} else if (*bucketp != bucket) {
		UNLOCK(&adb->entrylocks[*bucketp]);
		LOCK(&adb->entrylocks[bucket]);
		*bucketp = bucket;
	}

	/* Search the list, while cleaning up expired entries. */
	for (entry = ISC_LIST_HEAD(adb->entries[bucket]);
	     entry != NULL;
	     entry = entry_next) {
		entry_next = ISC_LIST_NEXT(entry, plink);
		(void)check_expire_entry(adb, &entry, now);
		if (entry != NULL &&
		    isc_sockaddr_equal(addr, &entry->sockaddr)) {
			ISC_LIST_UNLINK(adb->entries[bucket], entry, plink);
			ISC_LIST_PREPEND(adb->entries[bucket], entry, plink);
			return (entry);
		}
	}

	return (NULL);
}

/*
 * Entry bucket MUST be locked!
 */
static isc_boolean_t
entry_is_lame(dns_adb_t *adb, dns_adbentry_t *entry, dns_name_t *qname,
	      dns_rdatatype_t qtype, isc_stdtime_t now)
{
	dns_adblameinfo_t *li, *next_li;
	isc_boolean_t is_bad;

	is_bad = ISC_FALSE;

	li = ISC_LIST_HEAD(entry->lameinfo);
	if (li == NULL)
		return (ISC_FALSE);
	while (li != NULL) {
		next_li = ISC_LIST_NEXT(li, plink);

		/*
		 * Has the entry expired?
		 */
		if (li->lame_timer < now) {
			ISC_LIST_UNLINK(entry->lameinfo, li, plink);
			free_adblameinfo(adb, &li);
		}

		/*
		 * Order tests from least to most expensive.
		 *
		 * We do not break out of the main loop here as
		 * we use the loop for house keeping.
		 */
		if (li != NULL && !is_bad && li->qtype == qtype &&
		    dns_name_equal(qname, &li->qname))
			is_bad = ISC_TRUE;

		li = next_li;
	}

	return (is_bad);
}

static void
copy_namehook_lists(dns_adb_t *adb, dns_adbfind_t *find, dns_name_t *qname,
		    dns_rdatatype_t qtype, dns_adbname_t *name,
		    isc_stdtime_t now)
{
	dns_adbnamehook_t *namehook;
	dns_adbaddrinfo_t *addrinfo;
	dns_adbentry_t *entry;
	int bucket;

	bucket = DNS_ADB_INVALIDBUCKET;

	if (find->options & DNS_ADBFIND_INET) {
		namehook = ISC_LIST_HEAD(name->v4);
		while (namehook != NULL) {
			entry = namehook->entry;
			bucket = entry->lock_bucket;
			LOCK(&adb->entrylocks[bucket]);

			if (!FIND_RETURNLAME(find)
			    && entry_is_lame(adb, entry, qname, qtype, now)) {
				find->options |= DNS_ADBFIND_LAMEPRUNED;
				goto nextv4;
			}
			addrinfo = new_adbaddrinfo(adb, entry, find->port);
			if (addrinfo == NULL) {
				find->partial_result |= DNS_ADBFIND_INET;
				goto out;
			}
			/*
			 * Found a valid entry.  Add it to the find's list.
			 */
			inc_entry_refcnt(adb, entry, ISC_FALSE);
			ISC_LIST_APPEND(find->list, addrinfo, publink);
			addrinfo = NULL;
		nextv4:
			UNLOCK(&adb->entrylocks[bucket]);
			bucket = DNS_ADB_INVALIDBUCKET;
			namehook = ISC_LIST_NEXT(namehook, plink);
		}
	}

	if (find->options & DNS_ADBFIND_INET6) {
		namehook = ISC_LIST_HEAD(name->v6);
		while (namehook != NULL) {
			entry = namehook->entry;
			bucket = entry->lock_bucket;
			LOCK(&adb->entrylocks[bucket]);

			if (!FIND_RETURNLAME(find)
			    && entry_is_lame(adb, entry, qname, qtype, now)) {
				find->options |= DNS_ADBFIND_LAMEPRUNED;
				goto nextv6;
			}
			addrinfo = new_adbaddrinfo(adb, entry, find->port);
			if (addrinfo == NULL) {
				find->partial_result |= DNS_ADBFIND_INET6;
				goto out;
			}
			/*
			 * Found a valid entry.  Add it to the find's list.
			 */
			inc_entry_refcnt(adb, entry, ISC_FALSE);
			ISC_LIST_APPEND(find->list, addrinfo, publink);
			addrinfo = NULL;
		nextv6:
			UNLOCK(&adb->entrylocks[bucket]);
			bucket = DNS_ADB_INVALIDBUCKET;
			namehook = ISC_LIST_NEXT(namehook, plink);
		}
	}

 out:
	if (bucket != DNS_ADB_INVALIDBUCKET)
		UNLOCK(&adb->entrylocks[bucket]);
}

static void
shutdown_task(isc_task_t *task, isc_event_t *ev) {
	dns_adb_t *adb;

	UNUSED(task);

	adb = ev->ev_arg;
	INSIST(DNS_ADB_VALID(adb));

	isc_event_free(&ev);
	/*
	 * Wait for lock around check_exit() call to be released.
	 */
	LOCK(&adb->lock);
	UNLOCK(&adb->lock);
	destroy(adb);
}

/*
 * Name bucket must be locked; adb may be locked; no other locks held.
 */
static isc_boolean_t
check_expire_name(dns_adbname_t **namep, isc_stdtime_t now) {
	dns_adbname_t *name;
	isc_boolean_t result = ISC_FALSE;

	INSIST(namep != NULL && DNS_ADBNAME_VALID(*namep));
	name = *namep;

	if (NAME_HAS_V4(name) || NAME_HAS_V6(name))
		return (result);
	if (NAME_FETCH(name))
		return (result);
	if (!EXPIRE_OK(name->expire_v4, now))
		return (result);
	if (!EXPIRE_OK(name->expire_v6, now))
		return (result);
	if (!EXPIRE_OK(name->expire_target, now))
		return (result);

	/*
	 * The name is empty.  Delete it.
	 */
	result = kill_name(&name, DNS_EVENT_ADBEXPIRED);
	*namep = NULL;

	/*
	 * Our caller, or one of its callers, will be calling check_exit() at
	 * some point, so we don't need to do it here.
	 */
	return (result);
}

/*%
 * Examine the tail entry of the LRU list to see if it expires or is stale
 * (unused for some period); if so, the name entry will be freed.  If the ADB
 * is in the overmem condition, the tail and the next to tail entries
 * will be unconditionally removed (unless they have an outstanding fetch).
 * We don't care about a race on 'overmem' at the risk of causing some
 * collateral damage or a small delay in starting cleanup, so we don't bother
 * to lock ADB (if it's not locked).
 *
 * Name bucket must be locked; adb may be locked; no other locks held.
 */
static void
check_stale_name(dns_adb_t *adb, int bucket, isc_stdtime_t now) {
	int victims, max_victims;
	isc_boolean_t result;
	dns_adbname_t *victim, *next_victim;
	isc_boolean_t overmem = adb->overmem;
	int scans = 0;

	INSIST(bucket != DNS_ADB_INVALIDBUCKET);

	max_victims = overmem ? 2 : 1;

	/*
	 * We limit the number of scanned entries to 10 (arbitrary choice)
	 * in order to avoid examining too many entries when there are many
	 * tail entries that have fetches (this should be rare, but could
	 * happen).
	 */
	victim = ISC_LIST_TAIL(adb->names[bucket]);
	for (victims = 0;
	     victim != NULL && victims < max_victims && scans < 10;
	     victim = next_victim) {
		INSIST(!NAME_DEAD(victim));
		scans++;
		next_victim = ISC_LIST_PREV(victim, plink);
		result = check_expire_name(&victim, now);
		if (victim == NULL) {
			victims++;
			goto next;
		}

		if (!NAME_FETCH(victim) &&
		    (overmem || victim->last_used + ADB_STALE_MARGIN <= now)) {
			RUNTIME_CHECK(kill_name(&victim,
						DNS_EVENT_ADBCANCELED) ==
				      ISC_FALSE);
			victims++;
		}

	next:
		if (!overmem)
			break;
	}
}

/*
 * Entry bucket must be locked; adb may be locked; no other locks held.
 */
static isc_boolean_t
check_expire_entry(dns_adb_t *adb, dns_adbentry_t **entryp, isc_stdtime_t now)
{
	dns_adbentry_t *entry;
	isc_boolean_t result = ISC_FALSE;

	INSIST(entryp != NULL && DNS_ADBENTRY_VALID(*entryp));
	entry = *entryp;

	if (entry->refcnt != 0)
		return (result);

	if (entry->expires == 0 || entry->expires > now)
		return (result);

	/*
	 * The entry is not in use.  Delete it.
	 */
	DP(DEF_LEVEL, "killing entry %p", entry);
	INSIST(ISC_LINK_LINKED(entry, plink));
	result = unlink_entry(adb, entry);
	free_adbentry(adb, &entry);
	if (result)
		dec_adb_irefcnt(adb);
	*entryp = NULL;
	return (result);
}

/*
 * ADB must be locked, and no other locks held.
 */
static isc_boolean_t
cleanup_names(dns_adb_t *adb, int bucket, isc_stdtime_t now) {
	dns_adbname_t *name;
	dns_adbname_t *next_name;
	isc_boolean_t result = ISC_FALSE;

	DP(CLEAN_LEVEL, "cleaning name bucket %d", bucket);

	LOCK(&adb->namelocks[bucket]);
	if (adb->name_sd[bucket]) {
		UNLOCK(&adb->namelocks[bucket]);
		return (result);
	}

	name = ISC_LIST_HEAD(adb->names[bucket]);
	while (name != NULL) {
		next_name = ISC_LIST_NEXT(name, plink);
		INSIST(result == ISC_FALSE);
		result = check_expire_namehooks(name, now);
		if (!result)
			result = check_expire_name(&name, now);
		name = next_name;
	}
	UNLOCK(&adb->namelocks[bucket]);
	return (result);
}

/*
 * ADB must be locked, and no other locks held.
 */
static isc_boolean_t
cleanup_entries(dns_adb_t *adb, int bucket, isc_stdtime_t now) {
	dns_adbentry_t *entry, *next_entry;
	isc_boolean_t result = ISC_FALSE;

	DP(CLEAN_LEVEL, "cleaning entry bucket %d", bucket);

	LOCK(&adb->entrylocks[bucket]);
	entry = ISC_LIST_HEAD(adb->entries[bucket]);
	while (entry != NULL) {
		next_entry = ISC_LIST_NEXT(entry, plink);
		INSIST(result == ISC_FALSE);
		result = check_expire_entry(adb, &entry, now);
		entry = next_entry;
	}
	UNLOCK(&adb->entrylocks[bucket]);
	return (result);
}

static void
destroy(dns_adb_t *adb) {
	adb->magic = 0;

	isc_task_detach(&adb->task);

	isc_mempool_destroy(&adb->nmp);
	isc_mempool_destroy(&adb->nhmp);
	isc_mempool_destroy(&adb->limp);
	isc_mempool_destroy(&adb->emp);
	isc_mempool_destroy(&adb->ahmp);
	isc_mempool_destroy(&adb->aimp);
	isc_mempool_destroy(&adb->afmp);

	DESTROYMUTEXBLOCK(adb->entrylocks, NBUCKETS);
	DESTROYMUTEXBLOCK(adb->namelocks, NBUCKETS);

	DESTROYLOCK(&adb->reflock);
	DESTROYLOCK(&adb->lock);
	DESTROYLOCK(&adb->mplock);
	DESTROYLOCK(&adb->overmemlock);

	isc_mem_putanddetach(&adb->mctx, adb, sizeof(dns_adb_t));
}


/*
 * Public functions.
 */

isc_result_t
dns_adb_create(isc_mem_t *mem, dns_view_t *view, isc_timermgr_t *timermgr,
	       isc_taskmgr_t *taskmgr, dns_adb_t **newadb)
{
	dns_adb_t *adb;
	isc_result_t result;
	int i;

	REQUIRE(mem != NULL);
	REQUIRE(view != NULL);
	REQUIRE(timermgr != NULL); /* this is actually unused */
	REQUIRE(taskmgr != NULL);
	REQUIRE(newadb != NULL && *newadb == NULL);

	UNUSED(timermgr);

	adb = isc_mem_get(mem, sizeof(dns_adb_t));
	if (adb == NULL)
		return (ISC_R_NOMEMORY);

	/*
	 * Initialize things here that cannot fail, and especially things
	 * that must be NULL for the error return to work properly.
	 */
	adb->magic = 0;
	adb->erefcnt = 1;
	adb->irefcnt = 0;
	adb->nmp = NULL;
	adb->nhmp = NULL;
	adb->limp = NULL;
	adb->emp = NULL;
	adb->ahmp = NULL;
	adb->aimp = NULL;
	adb->afmp = NULL;
	adb->task = NULL;
	adb->mctx = NULL;
	adb->view = view;
	adb->taskmgr = taskmgr;
	adb->next_cleanbucket = 0;
	ISC_EVENT_INIT(&adb->cevent, sizeof(adb->cevent), 0, NULL,
		       DNS_EVENT_ADBCONTROL, shutdown_task, adb,
		       adb, NULL, NULL);
	adb->cevent_sent = ISC_FALSE;
	adb->shutting_down = ISC_FALSE;
	adb->overmem = ISC_FALSE;
	ISC_LIST_INIT(adb->whenshutdown);

	isc_mem_attach(mem, &adb->mctx);

	result = isc_mutex_init(&adb->lock);
	if (result != ISC_R_SUCCESS)
		goto fail0b;

	result = isc_mutex_init(&adb->mplock);
	if (result != ISC_R_SUCCESS)
		goto fail0c;

	result = isc_mutex_init(&adb->reflock);
	if (result != ISC_R_SUCCESS)
		goto fail0d;

	result = isc_mutex_init(&adb->overmemlock);
	if (result != ISC_R_SUCCESS)
		goto fail0e;

	/*
	 * Initialize the bucket locks for names and elements.
	 * May as well initialize the list heads, too.
	 */
	result = isc_mutexblock_init(adb->namelocks, NBUCKETS);
	if (result != ISC_R_SUCCESS)
		goto fail1;
	for (i = 0; i < NBUCKETS; i++) {
		ISC_LIST_INIT(adb->names[i]);
		ISC_LIST_INIT(adb->deadnames[i]);
		adb->name_sd[i] = ISC_FALSE;
		adb->name_refcnt[i] = 0;
		adb->irefcnt++;
	}
	for (i = 0; i < NBUCKETS; i++) {
		ISC_LIST_INIT(adb->entries[i]);
		ISC_LIST_INIT(adb->deadentries[i]);
		adb->entry_sd[i] = ISC_FALSE;
		adb->entry_refcnt[i] = 0;
		adb->irefcnt++;
	}
	result = isc_mutexblock_init(adb->entrylocks, NBUCKETS);
	if (result != ISC_R_SUCCESS)
		goto fail2;

	/*
	 * Memory pools
	 */
#define MPINIT(t, p, n) do { \
	result = isc_mempool_create(mem, sizeof(t), &(p)); \
	if (result != ISC_R_SUCCESS) \
		goto fail3; \
	isc_mempool_setfreemax((p), FREE_ITEMS); \
	isc_mempool_setfillcount((p), FILL_COUNT); \
	isc_mempool_setname((p), n); \
	isc_mempool_associatelock((p), &adb->mplock); \
} while (0)

	MPINIT(dns_adbname_t, adb->nmp, "adbname");
	MPINIT(dns_adbnamehook_t, adb->nhmp, "adbnamehook");
	MPINIT(dns_adblameinfo_t, adb->limp, "adblameinfo");
	MPINIT(dns_adbentry_t, adb->emp, "adbentry");
	MPINIT(dns_adbfind_t, adb->ahmp, "adbfind");
	MPINIT(dns_adbaddrinfo_t, adb->aimp, "adbaddrinfo");
	MPINIT(dns_adbfetch_t, adb->afmp, "adbfetch");

#undef MPINIT

	/*
	 * Allocate an internal task.
	 */
	result = isc_task_create(adb->taskmgr, 0, &adb->task);
	if (result != ISC_R_SUCCESS)
		goto fail3;
	isc_task_setname(adb->task, "ADB", adb);

	/*
	 * Normal return.
	 */
	adb->magic = DNS_ADB_MAGIC;
	*newadb = adb;
	return (ISC_R_SUCCESS);

 fail3:
	if (adb->task != NULL)
		isc_task_detach(&adb->task);

	/* clean up entrylocks */
	DESTROYMUTEXBLOCK(adb->entrylocks, NBUCKETS);

 fail2: /* clean up namelocks */
	DESTROYMUTEXBLOCK(adb->namelocks, NBUCKETS);

 fail1: /* clean up only allocated memory */
	if (adb->nmp != NULL)
		isc_mempool_destroy(&adb->nmp);
	if (adb->nhmp != NULL)
		isc_mempool_destroy(&adb->nhmp);
	if (adb->limp != NULL)
		isc_mempool_destroy(&adb->limp);
	if (adb->emp != NULL)
		isc_mempool_destroy(&adb->emp);
	if (adb->ahmp != NULL)
		isc_mempool_destroy(&adb->ahmp);
	if (adb->aimp != NULL)
		isc_mempool_destroy(&adb->aimp);
	if (adb->afmp != NULL)
		isc_mempool_destroy(&adb->afmp);

	DESTROYLOCK(&adb->overmemlock);
 fail0e:
	DESTROYLOCK(&adb->reflock);
 fail0d:
	DESTROYLOCK(&adb->mplock);
 fail0c:
	DESTROYLOCK(&adb->lock);
 fail0b:
	isc_mem_putanddetach(&adb->mctx, adb, sizeof(dns_adb_t));

	return (result);
}

void
dns_adb_attach(dns_adb_t *adb, dns_adb_t **adbx) {

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(adbx != NULL && *adbx == NULL);

	inc_adb_erefcnt(adb);
	*adbx = adb;
}

void
dns_adb_detach(dns_adb_t **adbx) {
	dns_adb_t *adb;
	isc_boolean_t need_exit_check;

	REQUIRE(adbx != NULL && DNS_ADB_VALID(*adbx));

	adb = *adbx;
	*adbx = NULL;

	INSIST(adb->erefcnt > 0);

	LOCK(&adb->reflock);
	adb->erefcnt--;
	need_exit_check = ISC_TF(adb->erefcnt == 0 && adb->irefcnt == 0);
	UNLOCK(&adb->reflock);

	if (need_exit_check) {
		LOCK(&adb->lock);
		INSIST(adb->shutting_down);
		check_exit(adb);
		UNLOCK(&adb->lock);
	}
}

void
dns_adb_whenshutdown(dns_adb_t *adb, isc_task_t *task, isc_event_t **eventp) {
	isc_task_t *clone;
	isc_event_t *event;
	isc_boolean_t zeroirefcnt = ISC_FALSE;

	/*
	 * Send '*eventp' to 'task' when 'adb' has shutdown.
	 */

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(eventp != NULL);

	event = *eventp;
	*eventp = NULL;

	LOCK(&adb->lock);

	LOCK(&adb->reflock);
	zeroirefcnt = ISC_TF(adb->irefcnt == 0);

	if (adb->shutting_down && zeroirefcnt &&
	    isc_mempool_getallocated(adb->ahmp) == 0) {
		/*
		 * We're already shutdown.  Send the event.
		 */
		event->ev_sender = adb;
		isc_task_send(task, &event);
	} else {
		clone = NULL;
		isc_task_attach(task, &clone);
		event->ev_sender = clone;
		ISC_LIST_APPEND(adb->whenshutdown, event, ev_link);
	}

	UNLOCK(&adb->reflock);
	UNLOCK(&adb->lock);
}

void
dns_adb_shutdown(dns_adb_t *adb) {
	isc_boolean_t need_check_exit;

	/*
	 * Shutdown 'adb'.
	 */

	LOCK(&adb->lock);

	if (!adb->shutting_down) {
		adb->shutting_down = ISC_TRUE;
		isc_mem_setwater(adb->mctx, water, adb, 0, 0);
		need_check_exit = shutdown_names(adb);
		if (!need_check_exit)
			need_check_exit = shutdown_entries(adb);
		if (need_check_exit)
			check_exit(adb);
	}

	UNLOCK(&adb->lock);
}

isc_result_t
dns_adb_createfind(dns_adb_t *adb, isc_task_t *task, isc_taskaction_t action,
		   void *arg, dns_name_t *name, dns_name_t *qname,
		   dns_rdatatype_t qtype, unsigned int options,
		   isc_stdtime_t now, dns_name_t *target,
		   in_port_t port, dns_adbfind_t **findp)
{
	dns_adbfind_t *find;
	dns_adbname_t *adbname;
	int bucket;
	isc_boolean_t want_event, start_at_zone, alias, have_address;
	isc_result_t result;
	unsigned int wanted_addresses;
	unsigned int wanted_fetches;
	unsigned int query_pending;

	REQUIRE(DNS_ADB_VALID(adb));
	if (task != NULL) {
		REQUIRE(action != NULL);
	}
	REQUIRE(name != NULL);
	REQUIRE(qname != NULL);
	REQUIRE(findp != NULL && *findp == NULL);
	REQUIRE(target == NULL || dns_name_hasbuffer(target));

	REQUIRE((options & DNS_ADBFIND_ADDRESSMASK) != 0);

	result = ISC_R_UNEXPECTED;
	wanted_addresses = (options & DNS_ADBFIND_ADDRESSMASK);
	wanted_fetches = 0;
	query_pending = 0;
	want_event = ISC_FALSE;
	start_at_zone = ISC_FALSE;
	alias = ISC_FALSE;

	if (now == 0)
		isc_stdtime_get(&now);

	/*
	 * XXXMLG  Move this comment somewhere else!
	 *
	 * Look up the name in our internal database.
	 *
	 * Possibilities:  Note that these are not always exclusive.
	 *
	 *      No name found.  In this case, allocate a new name header and
	 *      an initial namehook or two.  If any of these allocations
	 *      fail, clean up and return ISC_R_NOMEMORY.
	 *
	 *      Name found, valid addresses present.  Allocate one addrinfo
	 *      structure for each found and append it to the linked list
	 *      of addresses for this header.
	 *
	 *      Name found, queries pending.  In this case, if a task was
	 *      passed in, allocate a job id, attach it to the name's job
	 *      list and remember to tell the caller that there will be
	 *      more info coming later.
	 */

	find = new_adbfind(adb);
	if (find == NULL)
		return (ISC_R_NOMEMORY);

	find->port = port;

	/*
	 * Remember what types of addresses we are interested in.
	 */
	find->options = options;
	find->flags |= wanted_addresses;
	if (FIND_WANTEVENT(find)) {
		REQUIRE(task != NULL);
	}

	/*
	 * Try to see if we know anything about this name at all.
	 */
	bucket = DNS_ADB_INVALIDBUCKET;
	adbname = find_name_and_lock(adb, name, find->options, &bucket);
	if (adb->name_sd[bucket]) {
		DP(DEF_LEVEL,
		   "dns_adb_createfind: returning ISC_R_SHUTTINGDOWN");
		RUNTIME_CHECK(free_adbfind(adb, &find) == ISC_FALSE);
		result = ISC_R_SHUTTINGDOWN;
		goto out;
	}

	/*
	 * Nothing found.  Allocate a new adbname structure for this name.
	 */
	if (adbname == NULL) {
		/*
		 * See if there is any stale name at the end of list, and purge
		 * it if so.
		 */
		check_stale_name(adb, bucket, now);

		adbname = new_adbname(adb, name);
		if (adbname == NULL) {
			RUNTIME_CHECK(free_adbfind(adb, &find) == ISC_FALSE);
			result = ISC_R_NOMEMORY;
			goto out;
		}
		link_name(adb, bucket, adbname);
		if (FIND_HINTOK(find))
			adbname->flags |= NAME_HINT_OK;
		if (FIND_GLUEOK(find))
			adbname->flags |= NAME_GLUE_OK;
		if (FIND_STARTATZONE(find))
			adbname->flags |= NAME_STARTATZONE;
	} else {
		/* Move this name forward in the LRU list */
		ISC_LIST_UNLINK(adb->names[bucket], adbname, plink);
		ISC_LIST_PREPEND(adb->names[bucket], adbname, plink);
	}
	adbname->last_used = now;

	/*
	 * Expire old entries, etc.
	 */
	RUNTIME_CHECK(check_expire_namehooks(adbname, now) == ISC_FALSE);

	/*
	 * Do we know that the name is an alias?
	 */
	if (!EXPIRE_OK(adbname->expire_target, now)) {
		/*
		 * Yes, it is.
		 */
		DP(DEF_LEVEL,
		   "dns_adb_createfind: name %p is an alias (cached)",
		   adbname);
		alias = ISC_TRUE;
		goto post_copy;
	}

	/*
	 * Try to populate the name from the database and/or
	 * start fetches.  First try looking for an A record
	 * in the database.
	 */
	if (!NAME_HAS_V4(adbname) && EXPIRE_OK(adbname->expire_v4, now)
	    && WANT_INET(wanted_addresses)) {
		result = dbfind_name(adbname, now, dns_rdatatype_a);
		if (result == ISC_R_SUCCESS) {
			DP(DEF_LEVEL,
			   "dns_adb_createfind: found A for name %p in db",
			   adbname);
			goto v6;
		}

		/*
		 * Did we get a CNAME or DNAME?
		 */
		if (result == DNS_R_ALIAS) {
			DP(DEF_LEVEL,
			   "dns_adb_createfind: name %p is an alias",
			   adbname);
			alias = ISC_TRUE;
			goto post_copy;
		}

		/*
		 * If the name doesn't exist at all, don't bother with
		 * v6 queries; they won't work.
		 *
		 * If the name does exist but we didn't get our data, go
		 * ahead and try AAAA.
		 *
		 * If the result is neither of these, try a fetch for A.
		 */
		if (NXDOMAIN_RESULT(result))
			goto fetch;
		else if (NXRRSET_RESULT(result))
			goto v6;

		if (!NAME_FETCH_V4(adbname))
			wanted_fetches |= DNS_ADBFIND_INET;
	}

 v6:
	if (!NAME_HAS_V6(adbname) && EXPIRE_OK(adbname->expire_v6, now)
	    && WANT_INET6(wanted_addresses)) {
		result = dbfind_name(adbname, now, dns_rdatatype_aaaa);
		if (result == ISC_R_SUCCESS) {
			DP(DEF_LEVEL,
			   "dns_adb_createfind: found AAAA for name %p",
			   adbname);
			goto fetch;
		}

		/*
		 * Did we get a CNAME or DNAME?
		 */
		if (result == DNS_R_ALIAS) {
			DP(DEF_LEVEL,
			   "dns_adb_createfind: name %p is an alias",
			   adbname);
			alias = ISC_TRUE;
			goto post_copy;
		}

		/*
		 * Listen to negative cache hints, and don't start
		 * another query.
		 */
		if (NCACHE_RESULT(result) || AUTH_NX(result))
			goto fetch;

		if (!NAME_FETCH_V6(adbname))
			wanted_fetches |= DNS_ADBFIND_INET6;
	}

 fetch:
	if ((WANT_INET(wanted_addresses) && NAME_HAS_V4(adbname)) ||
	    (WANT_INET6(wanted_addresses) && NAME_HAS_V6(adbname)))
		have_address = ISC_TRUE;
	else
		have_address = ISC_FALSE;
	if (wanted_fetches != 0 &&
	    ! (FIND_AVOIDFETCHES(find) && have_address)) {
		/*
		 * We're missing at least one address family.  Either the
		 * caller hasn't instructed us to avoid fetches, or we don't
		 * know anything about any of the address families that would
		 * be acceptable so we have to launch fetches.
		 */

		if (FIND_STARTATZONE(find))
			start_at_zone = ISC_TRUE;

		/*
		 * Start V4.
		 */
		if (WANT_INET(wanted_fetches) &&
		    fetch_name(adbname, start_at_zone,
			       dns_rdatatype_a) == ISC_R_SUCCESS) {
			DP(DEF_LEVEL,
			   "dns_adb_createfind: started A fetch for name %p",
			   adbname);
		}

		/*
		 * Start V6.
		 */
		if (WANT_INET6(wanted_fetches) &&
		    fetch_name(adbname, start_at_zone,
			       dns_rdatatype_aaaa) == ISC_R_SUCCESS) {
			DP(DEF_LEVEL,
			   "dns_adb_createfind: "
			   "started AAAA fetch for name %p",
			   adbname);
		}
	}

	/*
	 * Run through the name and copy out the bits we are
	 * interested in.
	 */
	copy_namehook_lists(adb, find, qname, qtype, adbname, now);

 post_copy:
	if (NAME_FETCH_V4(adbname))
		query_pending |= DNS_ADBFIND_INET;
	if (NAME_FETCH_V6(adbname))
		query_pending |= DNS_ADBFIND_INET6;

	/*
	 * Attach to the name's query list if there are queries
	 * already running, and we have been asked to.
	 */
	want_event = ISC_TRUE;
	if (!FIND_WANTEVENT(find))
		want_event = ISC_FALSE;
	if (FIND_WANTEMPTYEVENT(find) && FIND_HAS_ADDRS(find))
		want_event = ISC_FALSE;
	if ((wanted_addresses & query_pending) == 0)
		want_event = ISC_FALSE;
	if (alias)
		want_event = ISC_FALSE;
	if (want_event) {
		find->adbname = adbname;
		find->name_bucket = bucket;
		ISC_LIST_APPEND(adbname->finds, find, plink);
		find->query_pending = (query_pending & wanted_addresses);
		find->flags &= ~DNS_ADBFIND_ADDRESSMASK;
		find->flags |= (find->query_pending & DNS_ADBFIND_ADDRESSMASK);
		DP(DEF_LEVEL, "createfind: attaching find %p to adbname %p",
		   find, adbname);
	} else {
		/*
		 * Remove the flag so the caller knows there will never
		 * be an event, and set internal flags to fake that
		 * the event was sent and freed, so dns_adb_destroyfind() will
		 * do the right thing.
		 */
		find->query_pending = (query_pending & wanted_addresses);
		find->options &= ~DNS_ADBFIND_WANTEVENT;
		find->flags |= (FIND_EVENT_SENT | FIND_EVENT_FREED);
		find->flags &= ~DNS_ADBFIND_ADDRESSMASK;
	}

	find->partial_result |= (adbname->partial_result & wanted_addresses);
	if (alias) {
		if (target != NULL) {
			result = dns_name_copy(&adbname->target, target, NULL);
			if (result != ISC_R_SUCCESS)
				goto out;
		}
		result = DNS_R_ALIAS;
	} else
		result = ISC_R_SUCCESS;

	/*
	 * Copy out error flags from the name structure into the find.
	 */
	find->result_v4 = find_err_map[adbname->fetch_err];
	find->result_v6 = find_err_map[adbname->fetch6_err];

 out:
	if (find != NULL) {
		*findp = find;

		if (want_event) {
			isc_task_t *taskp;

			INSIST((find->flags & DNS_ADBFIND_ADDRESSMASK) != 0);
			taskp = NULL;
			isc_task_attach(task, &taskp);
			find->event.ev_sender = taskp;
			find->event.ev_action = action;
			find->event.ev_arg = arg;
		}
	}

	UNLOCK(&adb->namelocks[bucket]);

	return (result);
}

void
dns_adb_destroyfind(dns_adbfind_t **findp) {
	dns_adbfind_t *find;
	dns_adbentry_t *entry;
	dns_adbaddrinfo_t *ai;
	int bucket;
	dns_adb_t *adb;

	REQUIRE(findp != NULL && DNS_ADBFIND_VALID(*findp));
	find = *findp;
	*findp = NULL;

	LOCK(&find->lock);

	DP(DEF_LEVEL, "dns_adb_destroyfind on find %p", find);

	adb = find->adb;
	REQUIRE(DNS_ADB_VALID(adb));

	REQUIRE(FIND_EVENTFREED(find));

	bucket = find->name_bucket;
	INSIST(bucket == DNS_ADB_INVALIDBUCKET);

	UNLOCK(&find->lock);

	/*
	 * The find doesn't exist on any list, and nothing is locked.
	 * Return the find to the memory pool, and decrement the adb's
	 * reference count.
	 */
	ai = ISC_LIST_HEAD(find->list);
	while (ai != NULL) {
		ISC_LIST_UNLINK(find->list, ai, publink);
		entry = ai->entry;
		ai->entry = NULL;
		INSIST(DNS_ADBENTRY_VALID(entry));
		RUNTIME_CHECK(dec_entry_refcnt(adb, entry, ISC_TRUE) ==
			      ISC_FALSE);
		free_adbaddrinfo(adb, &ai);
		ai = ISC_LIST_HEAD(find->list);
	}

	/*
	 * WARNING:  The find is freed with the adb locked.  This is done
	 * to avoid a race condition where we free the find, some other
	 * thread tests to see if it should be destroyed, detects it should
	 * be, destroys it, and then we try to lock it for our check, but the
	 * lock is destroyed.
	 */
	LOCK(&adb->lock);
	if (free_adbfind(adb, &find))
		check_exit(adb);
	UNLOCK(&adb->lock);
}

void
dns_adb_cancelfind(dns_adbfind_t *find) {
	isc_event_t *ev;
	isc_task_t *task;
	dns_adb_t *adb;
	int bucket;
	int unlock_bucket;

	LOCK(&find->lock);

	DP(DEF_LEVEL, "dns_adb_cancelfind on find %p", find);

	adb = find->adb;
	REQUIRE(DNS_ADB_VALID(adb));

	REQUIRE(!FIND_EVENTFREED(find));
	REQUIRE(FIND_WANTEVENT(find));

	bucket = find->name_bucket;
	if (bucket == DNS_ADB_INVALIDBUCKET)
		goto cleanup;

	/*
	 * We need to get the adbname's lock to unlink the find.
	 */
	unlock_bucket = bucket;
	violate_locking_hierarchy(&find->lock, &adb->namelocks[unlock_bucket]);
	bucket = find->name_bucket;
	if (bucket != DNS_ADB_INVALIDBUCKET) {
		ISC_LIST_UNLINK(find->adbname->finds, find, plink);
		find->adbname = NULL;
		find->name_bucket = DNS_ADB_INVALIDBUCKET;
	}
	UNLOCK(&adb->namelocks[unlock_bucket]);
	bucket = DNS_ADB_INVALIDBUCKET;

 cleanup:

	if (!FIND_EVENTSENT(find)) {
		ev = &find->event;
		task = ev->ev_sender;
		ev->ev_sender = find;
		ev->ev_type = DNS_EVENT_ADBCANCELED;
		ev->ev_destroy = event_free;
		ev->ev_destroy_arg = find;
		find->result_v4 = ISC_R_CANCELED;
		find->result_v6 = ISC_R_CANCELED;

		DP(DEF_LEVEL, "sending event %p to task %p for find %p",
		   ev, task, find);

		isc_task_sendanddetach(&task, (isc_event_t **)&ev);
	}

	UNLOCK(&find->lock);
}

void
dns_adb_dump(dns_adb_t *adb, FILE *f) {
	int i;
	isc_stdtime_t now;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(f != NULL);

	/*
	 * Lock the adb itself, lock all the name buckets, then lock all
	 * the entry buckets.  This should put the adb into a state where
	 * nothing can change, so we can iterate through everything and
	 * print at our leisure.
	 */

	LOCK(&adb->lock);
	isc_stdtime_get(&now);

	for (i = 0; i < NBUCKETS; i++)
		RUNTIME_CHECK(cleanup_names(adb, i, now) == ISC_FALSE);
	for (i = 0; i < NBUCKETS; i++)
		RUNTIME_CHECK(cleanup_entries(adb, i, now) == ISC_FALSE);

	dump_adb(adb, f, ISC_FALSE, now);
	UNLOCK(&adb->lock);
}

static void
dump_ttl(FILE *f, const char *legend, isc_stdtime_t value, isc_stdtime_t now) {
	if (value == INT_MAX)
		return;
	fprintf(f, " [%s TTL %d]", legend, value - now);
}

static void
dump_adb(dns_adb_t *adb, FILE *f, isc_boolean_t debug, isc_stdtime_t now) {
	int i;
	dns_adbname_t *name;
	dns_adbentry_t *entry;

	fprintf(f, ";\n; Address database dump\n;\n");
	if (debug)
		fprintf(f, "; addr %p, erefcnt %u, irefcnt %u, finds out %u\n",
			adb, adb->erefcnt, adb->irefcnt,
			isc_mempool_getallocated(adb->nhmp));

	for (i = 0; i < NBUCKETS; i++)
		LOCK(&adb->namelocks[i]);
	for (i = 0; i < NBUCKETS; i++)
		LOCK(&adb->entrylocks[i]);

	/*
	 * Dump the names
	 */
	for (i = 0; i < NBUCKETS; i++) {
		name = ISC_LIST_HEAD(adb->names[i]);
		if (name == NULL)
			continue;
		if (debug)
			fprintf(f, "; bucket %d\n", i);
		for (;
		     name != NULL;
		     name = ISC_LIST_NEXT(name, plink))
		{
			if (debug)
				fprintf(f, "; name %p (flags %08x)\n",
					name, name->flags);

			fprintf(f, "; ");
			print_dns_name(f, &name->name);
			if (dns_name_countlabels(&name->target) > 0) {
				fprintf(f, " alias ");
				print_dns_name(f, &name->target);
			}

			dump_ttl(f, "v4", name->expire_v4, now);
			dump_ttl(f, "v6", name->expire_v6, now);
			dump_ttl(f, "target", name->expire_target, now);

			fprintf(f, " [v4 %s] [v6 %s]",
				errnames[name->fetch_err],
				errnames[name->fetch6_err]);

			fprintf(f, "\n");

			print_namehook_list(f, "v4", &name->v4, debug, now);
			print_namehook_list(f, "v6", &name->v6, debug, now);

			if (debug)
				print_fetch_list(f, name);
			if (debug)
				print_find_list(f, name);

		}
	}

	fprintf(f, ";\n; Unassociated entries\n;\n");

	for (i = 0; i < NBUCKETS; i++) {
		entry = ISC_LIST_HEAD(adb->entries[i]);
		while (entry != NULL) {
			if (entry->refcnt == 0)
				dump_entry(f, entry, debug, now);
			entry = ISC_LIST_NEXT(entry, plink);
		}
	}

	/*
	 * Unlock everything
	 */
	for (i = 0; i < NBUCKETS; i++)
		UNLOCK(&adb->entrylocks[i]);
	for (i = 0; i < NBUCKETS; i++)
		UNLOCK(&adb->namelocks[i]);
}

static void
dump_entry(FILE *f, dns_adbentry_t *entry, isc_boolean_t debug,
	   isc_stdtime_t now)
{
	char addrbuf[ISC_NETADDR_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	isc_netaddr_t netaddr;
	dns_adblameinfo_t *li;

	isc_netaddr_fromsockaddr(&netaddr, &entry->sockaddr);
	isc_netaddr_format(&netaddr, addrbuf, sizeof(addrbuf));

	if (debug)
		fprintf(f, ";\t%p: refcnt %u\n", entry, entry->refcnt);

	fprintf(f, ";\t%s [srtt %u] [flags %08x]",
		addrbuf, entry->srtt, entry->flags);
	if (entry->expires != 0)
		fprintf(f, " [ttl %d]", entry->expires - now);
	fprintf(f, "\n");
	for (li = ISC_LIST_HEAD(entry->lameinfo);
	     li != NULL;
	     li = ISC_LIST_NEXT(li, plink)) {
		fprintf(f, ";\t\t");
		print_dns_name(f, &li->qname);
		dns_rdatatype_format(li->qtype, typebuf, sizeof(typebuf));
		fprintf(f, " %s [lame TTL %d]\n", typebuf,
			li->lame_timer - now);
	}
}

void
dns_adb_dumpfind(dns_adbfind_t *find, FILE *f) {
	char tmp[512];
	const char *tmpp;
	dns_adbaddrinfo_t *ai;
	isc_sockaddr_t *sa;

	/*
	 * Not used currently, in the API Just In Case we
	 * want to dump out the name and/or entries too.
	 */

	LOCK(&find->lock);

	fprintf(f, ";Find %p\n", find);
	fprintf(f, ";\tqpending %08x partial %08x options %08x flags %08x\n",
		find->query_pending, find->partial_result,
		find->options, find->flags);
	fprintf(f, ";\tname_bucket %d, name %p, event sender %p\n",
		find->name_bucket, find->adbname, find->event.ev_sender);

	ai = ISC_LIST_HEAD(find->list);
	if (ai != NULL)
		fprintf(f, "\tAddresses:\n");
	while (ai != NULL) {
		sa = &ai->sockaddr;
		switch (sa->type.sa.sa_family) {
		case AF_INET:
			tmpp = inet_ntop(AF_INET, &sa->type.sin.sin_addr,
					 tmp, sizeof(tmp));
			break;
		case AF_INET6:
			tmpp = inet_ntop(AF_INET6, &sa->type.sin6.sin6_addr,
					 tmp, sizeof(tmp));
			break;
		default:
			tmpp = "UnkFamily";
		}

		if (tmpp == NULL)
			tmpp = "BadAddress";

		fprintf(f, "\t\tentry %p, flags %08x"
			" srtt %u addr %s\n",
			ai->entry, ai->flags, ai->srtt, tmpp);

		ai = ISC_LIST_NEXT(ai, publink);
	}

	UNLOCK(&find->lock);
}

static void
print_dns_name(FILE *f, dns_name_t *name) {
	char buf[DNS_NAME_FORMATSIZE];

	INSIST(f != NULL);

	dns_name_format(name, buf, sizeof(buf));
	fprintf(f, "%s", buf);
}

static void
print_namehook_list(FILE *f, const char *legend, dns_adbnamehooklist_t *list,
		    isc_boolean_t debug, isc_stdtime_t now)
{
	dns_adbnamehook_t *nh;

	for (nh = ISC_LIST_HEAD(*list);
	     nh != NULL;
	     nh = ISC_LIST_NEXT(nh, plink))
	{
		if (debug)
			fprintf(f, ";\tHook(%s) %p\n", legend, nh);
		dump_entry(f, nh->entry, debug, now);
	}
}

static inline void
print_fetch(FILE *f, dns_adbfetch_t *ft, const char *type) {
	fprintf(f, "\t\tFetch(%s): %p -> { fetch %p }\n",
		type, ft, ft->fetch);
}

static void
print_fetch_list(FILE *f, dns_adbname_t *n) {
	if (NAME_FETCH_A(n))
		print_fetch(f, n->fetch_a, "A");
	if (NAME_FETCH_AAAA(n))
		print_fetch(f, n->fetch_aaaa, "AAAA");
}

static void
print_find_list(FILE *f, dns_adbname_t *name) {
	dns_adbfind_t *find;

	find = ISC_LIST_HEAD(name->finds);
	while (find != NULL) {
		dns_adb_dumpfind(find, f);
		find = ISC_LIST_NEXT(find, plink);
	}
}

static isc_result_t
dbfind_name(dns_adbname_t *adbname, isc_stdtime_t now, dns_rdatatype_t rdtype)
{
	isc_result_t result;
	dns_rdataset_t rdataset;
	dns_adb_t *adb;
	dns_fixedname_t foundname;
	dns_name_t *fname;

	INSIST(DNS_ADBNAME_VALID(adbname));
	adb = adbname->adb;
	INSIST(DNS_ADB_VALID(adb));
	INSIST(rdtype == dns_rdatatype_a || rdtype == dns_rdatatype_aaaa);

	dns_fixedname_init(&foundname);
	fname = dns_fixedname_name(&foundname);
	dns_rdataset_init(&rdataset);

	if (rdtype == dns_rdatatype_a)
		adbname->fetch_err = FIND_ERR_UNEXPECTED;
	else
		adbname->fetch6_err = FIND_ERR_UNEXPECTED;

	result = dns_view_find(adb->view, &adbname->name, rdtype, now,
			       NAME_GLUEOK(adbname) ? DNS_DBFIND_GLUEOK : 0,
			       ISC_TF(NAME_HINTOK(adbname)),
			       NULL, NULL, fname, &rdataset, NULL);

	/* XXXVIX this switch statement is too sparse to gen a jump table. */
	switch (result) {
	case DNS_R_GLUE:
	case DNS_R_HINT:
	case ISC_R_SUCCESS:
		/*
		 * Found in the database.  Even if we can't copy out
		 * any information, return success, or else a fetch
		 * will be made, which will only make things worse.
		 */
		if (rdtype == dns_rdatatype_a)
			adbname->fetch_err = FIND_ERR_SUCCESS;
		else
			adbname->fetch6_err = FIND_ERR_SUCCESS;
		result = import_rdataset(adbname, &rdataset, now);
		break;
	case DNS_R_NXDOMAIN:
	case DNS_R_NXRRSET:
		/*
		 * We're authoritative and the data doesn't exist.
		 * Make up a negative cache entry so we don't ask again
		 * for a while.
		 *
		 * XXXRTH  What time should we use?  I'm putting in 30 seconds
		 * for now.
		 */
		if (rdtype == dns_rdatatype_a) {
			adbname->expire_v4 = now + 30;
			DP(NCACHE_LEVEL,
			   "adb name %p: Caching auth negative entry for A",
			   adbname);
			if (result == DNS_R_NXDOMAIN)
				adbname->fetch_err = FIND_ERR_NXDOMAIN;
			else
				adbname->fetch_err = FIND_ERR_NXRRSET;
		} else {
			DP(NCACHE_LEVEL,
			   "adb name %p: Caching auth negative entry for AAAA",
			   adbname);
			adbname->expire_v6 = now + 30;
			if (result == DNS_R_NXDOMAIN)
				adbname->fetch6_err = FIND_ERR_NXDOMAIN;
			else
				adbname->fetch6_err = FIND_ERR_NXRRSET;
		}
		break;
	case DNS_R_NCACHENXDOMAIN:
	case DNS_R_NCACHENXRRSET:
		/*
		 * We found a negative cache entry.  Pull the TTL from it
		 * so we won't ask again for a while.
		 */
		rdataset.ttl = ttlclamp(rdataset.ttl);
		if (rdtype == dns_rdatatype_a) {
			adbname->expire_v4 = rdataset.ttl + now;
			if (result == DNS_R_NCACHENXDOMAIN)
				adbname->fetch_err = FIND_ERR_NXDOMAIN;
			else
				adbname->fetch_err = FIND_ERR_NXRRSET;
			DP(NCACHE_LEVEL,
			  "adb name %p: Caching negative entry for A (ttl %u)",
			   adbname, rdataset.ttl);
		} else {
			DP(NCACHE_LEVEL,
		       "adb name %p: Caching negative entry for AAAA (ttl %u)",
			   adbname, rdataset.ttl);
			adbname->expire_v6 = rdataset.ttl + now;
			if (result == DNS_R_NCACHENXDOMAIN)
				adbname->fetch6_err = FIND_ERR_NXDOMAIN;
			else
				adbname->fetch6_err = FIND_ERR_NXRRSET;
		}
		break;
	case DNS_R_CNAME:
	case DNS_R_DNAME:
		/*
		 * Clear the hint and glue flags, so this will match
		 * more often.
		 */
		adbname->flags &= ~(DNS_ADBFIND_GLUEOK | DNS_ADBFIND_HINTOK);

		rdataset.ttl = ttlclamp(rdataset.ttl);
		clean_target(adb, &adbname->target);
		adbname->expire_target = INT_MAX;
		result = set_target(adb, &adbname->name, fname, &rdataset,
				    &adbname->target);
		if (result == ISC_R_SUCCESS) {
			result = DNS_R_ALIAS;
			DP(NCACHE_LEVEL,
			   "adb name %p: caching alias target",
			   adbname);
			adbname->expire_target = rdataset.ttl + now;
		}
		if (rdtype == dns_rdatatype_a)
			adbname->fetch_err = FIND_ERR_SUCCESS;
		else
			adbname->fetch6_err = FIND_ERR_SUCCESS;
		break;
	}

	if (dns_rdataset_isassociated(&rdataset))
		dns_rdataset_disassociate(&rdataset);

	return (result);
}

static void
fetch_callback(isc_task_t *task, isc_event_t *ev) {
	dns_fetchevent_t *dev;
	dns_adbname_t *name;
	dns_adb_t *adb;
	dns_adbfetch_t *fetch;
	int bucket;
	isc_eventtype_t ev_status;
	isc_stdtime_t now;
	isc_result_t result;
	unsigned int address_type;
	isc_boolean_t want_check_exit = ISC_FALSE;

	UNUSED(task);

	INSIST(ev->ev_type == DNS_EVENT_FETCHDONE);
	dev = (dns_fetchevent_t *)ev;
	name = ev->ev_arg;
	INSIST(DNS_ADBNAME_VALID(name));
	adb = name->adb;
	INSIST(DNS_ADB_VALID(adb));

	bucket = name->lock_bucket;
	LOCK(&adb->namelocks[bucket]);

	INSIST(NAME_FETCH_A(name) || NAME_FETCH_AAAA(name));
	address_type = 0;
	if (NAME_FETCH_A(name) && (name->fetch_a->fetch == dev->fetch)) {
		address_type = DNS_ADBFIND_INET;
		fetch = name->fetch_a;
		name->fetch_a = NULL;
	} else if (NAME_FETCH_AAAA(name)
		   && (name->fetch_aaaa->fetch == dev->fetch)) {
		address_type = DNS_ADBFIND_INET6;
		fetch = name->fetch_aaaa;
		name->fetch_aaaa = NULL;
	} else
		fetch = NULL;

	INSIST(address_type != 0 && fetch != NULL);

	dns_resolver_destroyfetch(&fetch->fetch);
	dev->fetch = NULL;

	ev_status = DNS_EVENT_ADBNOMOREADDRESSES;

	/*
	 * Cleanup things we don't care about.
	 */
	if (dev->node != NULL)
		dns_db_detachnode(dev->db, &dev->node);
	if (dev->db != NULL)
		dns_db_detach(&dev->db);

	/*
	 * If this name is marked as dead, clean up, throwing away
	 * potentially good data.
	 */
	if (NAME_DEAD(name)) {
		free_adbfetch(adb, &fetch);
		isc_event_free(&ev);

		want_check_exit = kill_name(&name, DNS_EVENT_ADBCANCELED);

		UNLOCK(&adb->namelocks[bucket]);

		if (want_check_exit) {
			LOCK(&adb->lock);
			check_exit(adb);
			UNLOCK(&adb->lock);
		}

		return;
	}

	isc_stdtime_get(&now);

	/*
	 * If we got a negative cache response, remember it.
	 */
	if (NCACHE_RESULT(dev->result)) {
		dev->rdataset->ttl = ttlclamp(dev->rdataset->ttl);
		if (address_type == DNS_ADBFIND_INET) {
			DP(NCACHE_LEVEL, "adb fetch name %p: "
			   "caching negative entry for A (ttl %u)",
			   name, dev->rdataset->ttl);
			name->expire_v4 = ISC_MIN(name->expire_v4,
						  dev->rdataset->ttl + now);
			if (dev->result == DNS_R_NCACHENXDOMAIN)
				name->fetch_err = FIND_ERR_NXDOMAIN;
			else
				name->fetch_err = FIND_ERR_NXRRSET;
			inc_stats(adb, dns_resstatscounter_gluefetchv4fail);
		} else {
			DP(NCACHE_LEVEL, "adb fetch name %p: "
			   "caching negative entry for AAAA (ttl %u)",
			   name, dev->rdataset->ttl);
			name->expire_v6 = ISC_MIN(name->expire_v6,
						  dev->rdataset->ttl + now);
			if (dev->result == DNS_R_NCACHENXDOMAIN)
				name->fetch6_err = FIND_ERR_NXDOMAIN;
			else
				name->fetch6_err = FIND_ERR_NXRRSET;
			inc_stats(adb, dns_resstatscounter_gluefetchv6fail);
		}
		goto out;
	}

	/*
	 * Handle CNAME/DNAME.
	 */
	if (dev->result == DNS_R_CNAME || dev->result == DNS_R_DNAME) {
		dev->rdataset->ttl = ttlclamp(dev->rdataset->ttl);
		clean_target(adb, &name->target);
		name->expire_target = INT_MAX;
		result = set_target(adb, &name->name,
				    dns_fixedname_name(&dev->foundname),
				    dev->rdataset,
				    &name->target);
		if (result == ISC_R_SUCCESS) {
			DP(NCACHE_LEVEL,
			   "adb fetch name %p: caching alias target",
			   name);
			name->expire_target = dev->rdataset->ttl + now;
		}
		goto check_result;
	}

	/*
	 * Did we get back junk?  If so, and there are no more fetches
	 * sitting out there, tell all the finds about it.
	 */
	if (dev->result != ISC_R_SUCCESS) {
		char buf[DNS_NAME_FORMATSIZE];

		dns_name_format(&name->name, buf, sizeof(buf));
		DP(DEF_LEVEL, "adb: fetch of '%s' %s failed: %s",
		   buf, address_type == DNS_ADBFIND_INET ? "A" : "AAAA",
		   dns_result_totext(dev->result));
		/* XXXMLG Don't pound on bad servers. */
		if (address_type == DNS_ADBFIND_INET) {
			name->expire_v4 = ISC_MIN(name->expire_v4, now + 300);
			name->fetch_err = FIND_ERR_FAILURE;
			inc_stats(adb, dns_resstatscounter_gluefetchv4fail);
		} else {
			name->expire_v6 = ISC_MIN(name->expire_v6, now + 300);
			name->fetch6_err = FIND_ERR_FAILURE;
			inc_stats(adb, dns_resstatscounter_gluefetchv6fail);
		}
		goto out;
	}

	/*
	 * We got something potentially useful.
	 */
	result = import_rdataset(name, &fetch->rdataset, now);

 check_result:
	if (result == ISC_R_SUCCESS) {
		ev_status = DNS_EVENT_ADBMOREADDRESSES;
		if (address_type == DNS_ADBFIND_INET)
			name->fetch_err = FIND_ERR_SUCCESS;
		else
			name->fetch6_err = FIND_ERR_SUCCESS;
	}

 out:
	free_adbfetch(adb, &fetch);
	isc_event_free(&ev);

	clean_finds_at_name(name, ev_status, address_type);

	UNLOCK(&adb->namelocks[bucket]);
}

static isc_result_t
fetch_name(dns_adbname_t *adbname,
	   isc_boolean_t start_at_zone,
	   dns_rdatatype_t type)
{
	isc_result_t result;
	dns_adbfetch_t *fetch = NULL;
	dns_adb_t *adb;
	dns_fixedname_t fixed;
	dns_name_t *name;
	dns_rdataset_t rdataset;
	dns_rdataset_t *nameservers;
	unsigned int options;

	INSIST(DNS_ADBNAME_VALID(adbname));
	adb = adbname->adb;
	INSIST(DNS_ADB_VALID(adb));

	INSIST((type == dns_rdatatype_a && !NAME_FETCH_V4(adbname)) ||
	       (type == dns_rdatatype_aaaa && !NAME_FETCH_V6(adbname)));

	adbname->fetch_err = FIND_ERR_NOTFOUND;

	name = NULL;
	nameservers = NULL;
	dns_rdataset_init(&rdataset);

	options = DNS_FETCHOPT_NOVALIDATE;
	if (start_at_zone) {
		DP(ENTER_LEVEL,
		   "fetch_name: starting at zone for name %p",
		   adbname);
		dns_fixedname_init(&fixed);
		name = dns_fixedname_name(&fixed);
		result = dns_view_findzonecut2(adb->view, &adbname->name, name,
					       0, 0, ISC_TRUE, ISC_FALSE,
					       &rdataset, NULL);
		if (result != ISC_R_SUCCESS && result != DNS_R_HINT)
			goto cleanup;
		nameservers = &rdataset;
		options |= DNS_FETCHOPT_UNSHARED;
	}

	fetch = new_adbfetch(adb);
	if (fetch == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	result = dns_resolver_createfetch(adb->view->resolver, &adbname->name,
					  type, name, nameservers, NULL,
					  options, adb->task, fetch_callback,
					  adbname, &fetch->rdataset, NULL,
					  &fetch->fetch);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	if (type == dns_rdatatype_a) {
		adbname->fetch_a = fetch;
		inc_stats(adb, dns_resstatscounter_gluefetchv4);
	} else {
		adbname->fetch_aaaa = fetch;
		inc_stats(adb, dns_resstatscounter_gluefetchv6);
	}
	fetch = NULL;  /* Keep us from cleaning this up below. */

 cleanup:
	if (fetch != NULL)
		free_adbfetch(adb, &fetch);
	if (dns_rdataset_isassociated(&rdataset))
		dns_rdataset_disassociate(&rdataset);

	return (result);
}

/*
 * XXXMLG Needs to take a find argument and an address info, no zone or adb,
 * since these can be extracted from the find itself.
 */
isc_result_t
dns_adb_marklame(dns_adb_t *adb, dns_adbaddrinfo_t *addr, dns_name_t *qname,
		 dns_rdatatype_t qtype, isc_stdtime_t expire_time)
{
	dns_adblameinfo_t *li;
	int bucket;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));
	REQUIRE(qname != NULL);

	bucket = addr->entry->lock_bucket;
	LOCK(&adb->entrylocks[bucket]);
	li = ISC_LIST_HEAD(addr->entry->lameinfo);
	while (li != NULL &&
	       (li->qtype != qtype || !dns_name_equal(qname, &li->qname)))
		li = ISC_LIST_NEXT(li, plink);
	if (li != NULL) {
		if (expire_time > li->lame_timer)
			li->lame_timer = expire_time;
		goto unlock;
	}
	li = new_adblameinfo(adb, qname, qtype);
	if (li == NULL) {
		result = ISC_R_NOMEMORY;
		goto unlock;
	}

	li->lame_timer = expire_time;

	ISC_LIST_PREPEND(addr->entry->lameinfo, li, plink);
 unlock:
	UNLOCK(&adb->entrylocks[bucket]);

	return (result);
}

void
dns_adb_adjustsrtt(dns_adb_t *adb, dns_adbaddrinfo_t *addr,
		   unsigned int rtt, unsigned int factor)
{
	int bucket;
	unsigned int new_srtt;
	isc_stdtime_t now;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));
	REQUIRE(factor <= 10);

	bucket = addr->entry->lock_bucket;
	LOCK(&adb->entrylocks[bucket]);

	if (factor == DNS_ADB_RTTADJAGE)
		new_srtt = addr->entry->srtt * 98 / 100;
	else
		new_srtt = (addr->entry->srtt / 10 * factor)
			+ (rtt / 10 * (10 - factor));

	addr->entry->srtt = new_srtt;
	addr->srtt = new_srtt;

	isc_stdtime_get(&now);
	addr->entry->expires = now + ADB_ENTRY_WINDOW;

	UNLOCK(&adb->entrylocks[bucket]);
}

void
dns_adb_changeflags(dns_adb_t *adb, dns_adbaddrinfo_t *addr,
		    unsigned int bits, unsigned int mask)
{
	int bucket;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));

	bucket = addr->entry->lock_bucket;
	LOCK(&adb->entrylocks[bucket]);

	addr->entry->flags = (addr->entry->flags & ~mask) | (bits & mask);
	/*
	 * Note that we do not update the other bits in addr->flags with
	 * the most recent values from addr->entry->flags.
	 */
	addr->flags = (addr->flags & ~mask) | (bits & mask);

	UNLOCK(&adb->entrylocks[bucket]);
}

isc_result_t
dns_adb_findaddrinfo(dns_adb_t *adb, isc_sockaddr_t *sa,
		     dns_adbaddrinfo_t **addrp, isc_stdtime_t now)
{
	int bucket;
	dns_adbentry_t *entry;
	dns_adbaddrinfo_t *addr;
	isc_result_t result;
	in_port_t port;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(addrp != NULL && *addrp == NULL);

	UNUSED(now);

	result = ISC_R_SUCCESS;
	bucket = DNS_ADB_INVALIDBUCKET;
	entry = find_entry_and_lock(adb, sa, &bucket, now);
	if (adb->entry_sd[bucket]) {
		result = ISC_R_SHUTTINGDOWN;
		goto unlock;
	}
	if (entry == NULL) {
		/*
		 * We don't know anything about this address.
		 */
		entry = new_adbentry(adb);
		if (entry == NULL) {
			result = ISC_R_NOMEMORY;
			goto unlock;
		}
		entry->sockaddr = *sa;
		link_entry(adb, bucket, entry);
		DP(ENTER_LEVEL, "findaddrinfo: new entry %p", entry);
	} else
		DP(ENTER_LEVEL, "findaddrinfo: found entry %p", entry);

	port = isc_sockaddr_getport(sa);
	addr = new_adbaddrinfo(adb, entry, port);
	if (addr == NULL) {
		result = ISC_R_NOMEMORY;
	} else {
		inc_entry_refcnt(adb, entry, ISC_FALSE);
		*addrp = addr;
	}

 unlock:
	UNLOCK(&adb->entrylocks[bucket]);

	return (result);
}

void
dns_adb_freeaddrinfo(dns_adb_t *adb, dns_adbaddrinfo_t **addrp) {
	dns_adbaddrinfo_t *addr;
	dns_adbentry_t *entry;
	int bucket;
	isc_stdtime_t now;
	isc_boolean_t want_check_exit = ISC_FALSE;

	REQUIRE(DNS_ADB_VALID(adb));
	REQUIRE(addrp != NULL);
	addr = *addrp;
	REQUIRE(DNS_ADBADDRINFO_VALID(addr));
	entry = addr->entry;
	REQUIRE(DNS_ADBENTRY_VALID(entry));

	isc_stdtime_get(&now);

	*addrp = NULL;

	bucket = addr->entry->lock_bucket;
	LOCK(&adb->entrylocks[bucket]);

	entry->expires = now + ADB_ENTRY_WINDOW;

	want_check_exit = dec_entry_refcnt(adb, entry, ISC_FALSE);

	UNLOCK(&adb->entrylocks[bucket]);

	addr->entry = NULL;
	free_adbaddrinfo(adb, &addr);

	if (want_check_exit) {
		LOCK(&adb->lock);
		check_exit(adb);
		UNLOCK(&adb->lock);
	}
}

void
dns_adb_flush(dns_adb_t *adb) {
	unsigned int i;

	INSIST(DNS_ADB_VALID(adb));

	LOCK(&adb->lock);

	/*
	 * Call our cleanup routines.
	 */
	for (i = 0; i < NBUCKETS; i++)
		RUNTIME_CHECK(cleanup_names(adb, i, INT_MAX) == ISC_FALSE);
	for (i = 0; i < NBUCKETS; i++)
		RUNTIME_CHECK(cleanup_entries(adb, i, INT_MAX) == ISC_FALSE);

#ifdef DUMP_ADB_AFTER_CLEANING
	dump_adb(adb, stdout, ISC_TRUE, INT_MAX);
#endif

	UNLOCK(&adb->lock);
}

void
dns_adb_flushname(dns_adb_t *adb, dns_name_t *name) {
	dns_adbname_t *adbname;
	dns_adbname_t *nextname;
	int bucket;

	INSIST(DNS_ADB_VALID(adb));

	LOCK(&adb->lock);
	bucket = dns_name_hash(name, ISC_FALSE) % NBUCKETS;
	LOCK(&adb->namelocks[bucket]);
	adbname = ISC_LIST_HEAD(adb->names[bucket]);
	while (adbname != NULL) {
		nextname = ISC_LIST_NEXT(adbname, plink);
		if (!NAME_DEAD(adbname) &&
		    dns_name_equal(name, &adbname->name)) {
			RUNTIME_CHECK(kill_name(&adbname,
						DNS_EVENT_ADBCANCELED) ==
				      ISC_FALSE);
		}
		adbname = nextname;
	}
	UNLOCK(&adb->namelocks[bucket]);
	UNLOCK(&adb->lock);
}

static void
water(void *arg, int mark) {
	dns_adb_t *adb = arg;
	isc_boolean_t overmem = ISC_TF(mark == ISC_MEM_HIWATER);

	REQUIRE(DNS_ADB_VALID(adb));

	DP(ISC_LOG_DEBUG(1),
	   "adb reached %s water mark", overmem ? "high" : "low");

	/*
	 * We can't use adb->lock as there is potential for water
	 * to be called when adb->lock is held.
	 */
	LOCK(&adb->overmemlock);
	if (adb->overmem != overmem) {
		adb->overmem = overmem;
		isc_mem_waterack(adb->mctx, mark);
	}
	UNLOCK(&adb->overmemlock);
}

void
dns_adb_setadbsize(dns_adb_t *adb, isc_uint32_t size) {
	isc_uint32_t hiwater;
	isc_uint32_t lowater;

	INSIST(DNS_ADB_VALID(adb));

	if (size != 0 && size < DNS_ADB_MINADBSIZE)
		size = DNS_ADB_MINADBSIZE;

	hiwater = size - (size >> 3);   /* Approximately 7/8ths. */
	lowater = size - (size >> 2);   /* Approximately 3/4ths. */

	if (size == 0 || hiwater == 0 || lowater == 0)
		isc_mem_setwater(adb->mctx, water, adb, 0, 0);
	else
		isc_mem_setwater(adb->mctx, water, adb, hiwater, lowater);
}
