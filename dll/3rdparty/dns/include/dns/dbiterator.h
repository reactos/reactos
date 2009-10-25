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

/* $Id: dbiterator.h,v 1.25 2007/06/19 23:47:16 tbox Exp $ */

#ifndef DNS_DBITERATOR_H
#define DNS_DBITERATOR_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/dbiterator.h
 * \brief
 * The DNS DB Iterator interface allows iteration of all of the nodes in a
 * database.
 *
 * The dns_dbiterator_t type is like a "virtual class".  To actually use
 * it, an implementation of the class is required.  This implementation is
 * supplied by the database.
 *
 * It is the client's responsibility to call dns_db_detachnode() on all
 * nodes returned.
 *
 * XXX &lt;more&gt; XXX
 *
 * MP:
 *\li	The iterator itself is not locked.  The caller must ensure
 *	synchronization.
 *
 *\li	The iterator methods ensure appropriate database locking.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	None.
 */

/*****
 ***** Imports
 *****/

#include <isc/lang.h>
#include <isc/magic.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*****
 ***** Types
 *****/

typedef struct dns_dbiteratormethods {
	void		(*destroy)(dns_dbiterator_t **iteratorp);
	isc_result_t	(*first)(dns_dbiterator_t *iterator);
	isc_result_t	(*last)(dns_dbiterator_t *iterator);
	isc_result_t	(*seek)(dns_dbiterator_t *iterator, dns_name_t *name);
	isc_result_t	(*prev)(dns_dbiterator_t *iterator);
	isc_result_t	(*next)(dns_dbiterator_t *iterator);
	isc_result_t	(*current)(dns_dbiterator_t *iterator,
				   dns_dbnode_t **nodep, dns_name_t *name);
	isc_result_t	(*pause)(dns_dbiterator_t *iterator);
	isc_result_t	(*origin)(dns_dbiterator_t *iterator,
				  dns_name_t *name);
} dns_dbiteratormethods_t;

#define DNS_DBITERATOR_MAGIC	     ISC_MAGIC('D','N','S','I')
#define DNS_DBITERATOR_VALID(dbi)    ISC_MAGIC_VALID(dbi, DNS_DBITERATOR_MAGIC)
/*%
 * This structure is actually just the common prefix of a DNS db
 * implementation's version of a dns_dbiterator_t.
 *
 * Clients may use the 'db' field of this structure.  Except for that field,
 * direct use of this structure by clients is forbidden.  DB implementations
 * may change the structure.  'magic' must be DNS_DBITERATOR_MAGIC for any of
 * the dns_dbiterator routines to work.  DB iterator implementations must
 * maintain all DB iterator invariants.
 */
struct dns_dbiterator {
	/* Unlocked. */
	unsigned int			magic;
	dns_dbiteratormethods_t *	methods;
	dns_db_t *			db;
	isc_boolean_t			relative_names;
	isc_boolean_t			cleaning;
};

void
dns_dbiterator_destroy(dns_dbiterator_t **iteratorp);
/*%<
 * Destroy '*iteratorp'.
 *
 * Requires:
 *
 *\li	'*iteratorp' is a valid iterator.
 *
 * Ensures:
 *
 *\li	All resources used by the iterator are freed.
 *
 *\li	*iteratorp == NULL.
 */

isc_result_t
dns_dbiterator_first(dns_dbiterator_t *iterator);
/*%<
 * Move the node cursor to the first node in the database (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no nodes in the database.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_last(dns_dbiterator_t *iterator);
/*%<
 * Move the node cursor to the last node in the database (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no nodes in the database.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_seek(dns_dbiterator_t *iterator, dns_name_t *name);
/*%<
 * Move the node cursor to the node with name 'name'.
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 *\li	'name' is a valid name.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOTFOUND
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_prev(dns_dbiterator_t *iterator);
/*%<
 * Move the node cursor to the previous node in the database (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no more nodes in the
 *					database.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_next(dns_dbiterator_t *iterator);
/*%<
 * Move the node cursor to the next node in the database (if any).
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no more nodes in the
 *					database.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_current(dns_dbiterator_t *iterator, dns_dbnode_t **nodep,
		       dns_name_t *name);
/*%<
 * Return the current node.
 *
 * Notes:
 *\li	If 'name' is not NULL, it will be set to the name of the node.
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 *\li	nodep != NULL && *nodep == NULL
 *
 *\li	The node cursor of 'iterator' is at a valid location (i.e. the
 *	result of last call to a cursor movement command was ISC_R_SUCCESS).
 *
 *\li	'name' is NULL, or is a valid name with a dedicated buffer.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#DNS_R_NEWORIGIN			If this iterator was created with
 *					'relative_names' set to ISC_TRUE,
 *					then #DNS_R_NEWORIGIN will be returned
 *					when the origin the names are
 *					relative to changes.  This result
 *					can occur only when 'name' is not
 *					NULL.  This is also a successful
 *					result.
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_pause(dns_dbiterator_t *iterator);
/*%<
 * Pause iteration.
 *
 * Calling a cursor movement method or dns_dbiterator_current() may cause
 * database locks to be acquired.  Rather than reacquire these locks every
 * time one of these routines is called, the locks may simply be held.
 * Calling dns_dbiterator_pause() releases any such locks.  Iterator clients
 * should call this routine any time they are not going to execute another
 * iterator method in the immediate future.
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 *
 * Ensures:
 *\li	Any database locks being held for efficiency of iterator access are
 *	released.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

isc_result_t
dns_dbiterator_origin(dns_dbiterator_t *iterator, dns_name_t *name);
/*%<
 * Return the origin to which returned node names are relative.
 *
 * Requires:
 *
 *\li	'iterator' is a valid relative_names iterator.
 *
 *\li	'name' is a valid name with a dedicated buffer.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOSPACE
 *
 *\li	Other results are possible, depending on the DB implementation.
 */

void
dns_dbiterator_setcleanmode(dns_dbiterator_t *iterator, isc_boolean_t mode);
/*%<
 * Indicate that the given iterator is/is not cleaning the DB.
 *
 * Notes:
 *\li	When 'mode' is ISC_TRUE, 
 *
 * Requires:
 *\li	'iterator' is a valid iterator.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_DBITERATOR_H */
