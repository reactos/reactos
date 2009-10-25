/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: cache.h,v 1.26 2007/06/19 23:47:16 tbox Exp $ */

#ifndef DNS_CACHE_H
#define DNS_CACHE_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/cache.h
 * \brief
 * Defines dns_cache_t, the cache object.
 *
 * Notes:
 *\li 	A cache object contains DNS data of a single class.
 *	Multiple classes will be handled by creating multiple
 *	views, each with a different class and its own cache.
 *
 * MP:
 *\li	See notes at the individual functions.
 *
 * Reliability:
 *
 * Resources:
 *
 * Security:
 *
 * Standards:
 */

/***
 ***	Imports
 ***/

#include <isc/lang.h>
#include <isc/stdtime.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_cache_create(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		 isc_timermgr_t *timermgr, dns_rdataclass_t rdclass,
		 const char *db_type, unsigned int db_argc, char **db_argv,
		 dns_cache_t **cachep);
/*%<
 * Create a new DNS cache.
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context
 *
 *\li	'taskmgr' is a valid task manager and 'timermgr' is a valid timer
 * 	manager, or both are NULL.  If NULL, no periodic cleaning of the
 * 	cache will take place.
 *
 *\li	'cachep' is a valid pointer, and *cachep == NULL
 *
 * Ensures:
 *
 *\li	'*cachep' is attached to the newly created cache
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

void
dns_cache_attach(dns_cache_t *cache, dns_cache_t **targetp);
/*%<
 * Attach *targetp to cache.
 *
 * Requires:
 *
 *\li	'cache' is a valid cache.
 *
 *\li	'targetp' points to a NULL dns_cache_t *.
 *
 * Ensures:
 *
 *\li	*targetp is attached to cache.
 */

void
dns_cache_detach(dns_cache_t **cachep);
/*%<
 * Detach *cachep from its cache.
 *
 * Requires:
 *
 *\li	'cachep' points to a valid cache.
 *
 * Ensures:
 *
 *\li	*cachep is NULL.
 *
 *\li	If '*cachep' is the last reference to the cache,
 *		all resources used by the cache will be freed
 */

void
dns_cache_attachdb(dns_cache_t *cache, dns_db_t **dbp);
/*%<
 * Attach *dbp to the cache's database.
 *
 * Notes:
 *
 *\li	This may be used to get a reference to the database for
 *	the purpose of cache lookups (XXX currently it is also
 * 	the way to add data to the cache, but having a
 * 	separate dns_cache_add() interface instead would allow
 * 	more control over memory usage).
 *	The caller should call dns_db_detach() on the reference
 *	when it is no longer needed.
 *
 * Requires:
 *
 *\li	'cache' is a valid cache.
 *
 *\li	'dbp' points to a NULL dns_db *.
 *
 * Ensures:
 *
 *\li	*dbp is attached to the database.
 */


isc_result_t
dns_cache_setfilename(dns_cache_t *cache, const char *filename);
/*%<
 * If 'filename' is non-NULL, make the cache persistent.
 * The cache's data will be stored in the given file.
 * If 'filename' is NULL, make the cache non-persistent.
 * Files that are no longer used are not unlinked automatically.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	Various file-related failures
 */

isc_result_t
dns_cache_load(dns_cache_t *cache);
/*%<
 * If the cache has a file name, load the cache contents from the file.
 * Previous cache contents are not discarded.
 * If no file name has been set, do nothing and return success.
 *
 * MT:
 *\li	Multiple simultaneous attempts to load or dump the cache
 * 	will be serialized with respect to one another, but
 *	the cache may be read and updated while the dump is
 *	in progress.  Updates performed during loading
 *	may or may not be preserved, and reads may return
 * 	either the old or the newly loaded data.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *  \li    Various failures depending on the database implementation type
 */

isc_result_t
dns_cache_dump(dns_cache_t *cache);
/*%<
 * If the cache has a file name, write the cache contents to disk,
 * overwriting any preexisting file.  If no file name has been set,
 * do nothing and return success.
 *
 * MT:
 *\li	Multiple simultaneous attempts to load or dump the cache
 * 	will be serialized with respect to one another, but
 *	the cache may be read and updated while the dump is
 *	in progress.  Updates performed during the dump may
 * 	or may not be reflected in the dumped file.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *  \li    Various failures depending on the database implementation type
 */

isc_result_t
dns_cache_clean(dns_cache_t *cache, isc_stdtime_t now);
/*%<
 * Force immediate cleaning of the cache, freeing all rdatasets
 * whose TTL has expired as of 'now' and that have no pending
 * references.
 */

void
dns_cache_setcleaninginterval(dns_cache_t *cache, unsigned int interval);
/*%<
 * Set the periodic cache cleaning interval to 'interval' seconds.
 */

void
dns_cache_setcachesize(dns_cache_t *cache, isc_uint32_t size);
/*%<
 * Set the maximum cache size.  0 means unlimited.
 */

isc_result_t
dns_cache_flush(dns_cache_t *cache);
/*%<
 * Flushes all data from the cache.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_cache_flushname(dns_cache_t *cache, dns_name_t *name);
/*
 * Flushes a given name from the cache.
 *
 * Requires:
 *\li	'cache' to be valid.
 *\li	'name' to be valid.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	other error returns.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_CACHE_H */
